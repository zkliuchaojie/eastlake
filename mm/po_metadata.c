/*
 * @Descripttion: 
 * @version: 
 * @Author: Chenqinglin
 * @Date: 2020-10-11 19:40:27
 * @LastEditors: Chenqinglin
 * @LastEditTime: 2020-10-19 16:36:13
 * @FilePath: /eastlake/mm/po_metadata.c
 */
#include <linux/slab.h>
#include <linux/po_map.h>
#include <linux/syscalls.h>
#include <linux/po_metadata.h>
#include <linux/cred.h>
#include <asm-generic/io.h>
#include <uapi/asm-generic/errno.h>
#include <uapi/asm-generic/errno-base.h>
#include <linux/podtable.h>
#include <linux/sched.h>
#include <linux/po_map.h>
#include <uapi/asm-generic/mman-common.h>
#include <uapi/asm-generic/fcntl.h>
#include <linux/mman.h>
#include <asm/tlbflush.h>
#include <linux/pmalloc.h>

#include <linux/pflush.h>

#define BURST_LIMIT 4

void po_super_init(struct po_super *po_super)
{
	struct po_ns_trie_node *root;
	struct po_vma *vma;
	int i;

	root = kpmalloc(sizeof(struct po_ns_trie_node), GFP_KERNEL);
	for (i = 0; i < PO_NS_LENGTH; i++)
		root->ptrs[i] = NULL;

	root->depth = 1;
	flush_clwb(root, sizeof(struct po_ns_trie_node));
	_mm_sfence();
	po_super->trie_root = root;
	po_super->trie_node_count = 1;
	po_super->container_count = 0;
	po_super->po_count = 0;
	po_super->redolog = kpmalloc(sizeof(struct po_redolog4del), GFP_KERNEL);
	po_super->redolog->valid = 0;
	flush_clwb(po_super->redolog, sizeof(struct po_redolog4del));
	_mm_sfence();

	// init non-continuous address space
	vma = kpmalloc(sizeof(struct po_vma), GFP_KERNEL);
	vma->start = PO_NON_CONTINUOUS_MAP_AREA_START;
	vma->size = PO_NON_CONTINUOUS_MAP_SIZE;
	vma->next_pa = NULL;
	flush_clwb(vma, sizeof(struct po_vma));
	_mm_sfence();

	po_super->vma_free_list_pa = virt_to_phys(vma);
	flush_clwb(po_super, sizeof(struct po_super));
	_mm_sfence();
	pr_info("po_super_init finished\n");
	pr_info("PO_MAP_AREA_START: %#lx, PO_MAP_AREA_END: %#lx",
		PO_MAP_AREA_START, PO_MAP_AREA_END);
}

struct po_super *po_get_super(void)
{
	struct pm_zone *pm_zone;
	struct pm_super *pm_super;

	pm_zone = NODE_DATA(0)->node_pm_zones + ZONE_PM_EMU;
	pm_super = pm_zone->super;
	return &(pm_super->po_super);
}

struct po_ns_record *po_ns_search_container(struct po_ns_container *cont,
					    int depth, const char *str,
					    int strlen)
{
	struct po_ns_record *rc;

	rc = cont->record_first;
	while (rc != NULL) {
		if ((rc->strlen == strlen - depth) &&
		    (memcmp(str + depth, rc->str, strlen - depth) == 0))
			return rc;
		rc = rc->next;
	}
	return NULL;
}

struct po_ns_record *po_insert_record(struct po_ns_trie_node *prev_trie_node,
				      int prev_index,
				      struct po_ns_record *record)
{
	struct po_ns_container *cont, *new_cont;
	cont = (struct po_ns_container *)(prev_trie_node->ptrs[prev_index]);

	//update the new record first
	record->next = cont->record_first;
	flush_clwb(record, sizeof(struct po_ns_record));
	_mm_sfence();
	/* 	 we employ COW mechanism to update container because 
	cnt_limit and record_first must be update atomically,and
	we need add and update other parameters for other burst 
	judgement mode in the future*/
	new_cont = (struct po_ns_container *)kpmalloc(sizeof(struct po_ns_container), GFP_KERNEL);
	new_cont->cnt_limit = cont->cnt_limit + 1;
	new_cont->record_first = record;
	flush_clwb(new_cont, sizeof(struct po_ns_container));
	_mm_sfence();
	//we have to ensure flush ptr[index] before kpfree
	prev_trie_node->ptrs[prev_index] = (struct po_ns_trie_node *)new_cont;
	flush_clwb(&(prev_trie_node->ptrs[prev_index]),
		   sizeof(struct po_ns_container *));
	_mm_sfence();
	kpfree(cont);

	if (po_ns_need_burst(new_cont))
		return po_ns_burst(prev_trie_node, prev_index);
	return record;
}

struct po_ns_record *po_ns_search(const char *str, int strlen)
{
	struct po_super *ps;
	struct po_ns_trie_node *prev, *curr;
	int depth;
	int index;

	depth = 1;
	ps = po_get_super();
	prev = NULL;
	curr = ps->trie_root;
	while (depth < strlen + 2) {
		if (depth == (int)(curr->depth)) {
			prev = curr;
			index = (int)str[depth - 1];
			curr = curr->ptrs[index];
			if (curr == NULL)
				return NULL;
			depth++;
		} else {
			return po_ns_search_container(
				(struct po_ns_container *)(curr), depth - 1,
				str, strlen);
		}
	}
	return (struct po_ns_record *)prev->ptrs[0];
}

struct po_ns_record *po_ns_insert(const char *str, int strlen)
{
	struct po_super *ps;
	int depth;
	int index, strindex;
	struct po_ns_trie_node *prev, *curr, *root;
	struct po_ns_container *cont;
	struct po_ns_record *rc;

	ps = po_get_super();
	root = ps->trie_root;
	depth = 1;
	prev = NULL;
	curr = root;
	while (depth < strlen + 1) {
		index = (int)str[depth - 1];
		prev = curr;
		curr = curr->ptrs[index];
		if (curr == NULL) {
			cont = kpmalloc(sizeof(struct po_ns_container),
					GFP_KERNEL);
			rc = kpmalloc(sizeof(struct po_ns_record), GFP_KERNEL);
			rc->strlen = strlen - depth;
			if (rc->strlen != 0) {
				rc->str = kpmalloc(rc->strlen, GFP_KERNEL);
				for (strindex = depth; strindex < strlen;
				     strindex++)
					rc->str[strindex - depth] =
						str[strindex];
				flush_clwb(rc->str, rc->strlen);
				_mm_sfence();
			} else {
				rc->str = NULL;
			}
			rc->next = NULL;
			flush_clwb(rc, sizeof(struct po_ns_record));
			_mm_sfence();

			cont->record_first = rc;
			cont->cnt_limit = 1;
			flush_clwb(cont, sizeof(struct po_ns_container));
			_mm_sfence();
			prev->ptrs[index] = (struct po_ns_trie_node *)cont;
			flush_clwb(&(prev->ptrs[index]),
				   sizeof(struct po_ns_container *));
			_mm_sfence();
			return rc;
		}
		if (curr->depth != depth + 1) {
			// means it is a container
			if (po_ns_search_container(
				    (struct po_ns_container *)curr, prev->depth,
				    str, strlen) == NULL) {
				rc = kpmalloc(sizeof(struct po_ns_record),
					      GFP_KERNEL);
				rc->desc = NULL;
				rc->strlen = strlen - depth;
				if (rc->strlen != 0) {
					rc->str = kpmalloc(rc->strlen,
							   GFP_KERNEL);
					for (strindex = depth;
					     strindex < strlen; strindex++)
						rc->str[strindex - depth] =
							str[strindex];
					flush_clwb(rc->str, rc->strlen);
					_mm_sfence();

				} else {
					rc->str = NULL;
				}
				rc->next = NULL;
				flush_clwb(rc, sizeof(struct po_ns_record));
				_mm_sfence();

				return po_insert_record(prev, index, rc);
			} else {
				return NULL;
			}
		}
		depth++;
	}
	//the first element in trie_node
	if (curr->ptrs[0] == NULL) {
		rc = kpmalloc(sizeof(struct po_ns_record), GFP_KERNEL);
		rc->strlen = 0;
		rc->next = NULL;
		flush_clwb(rc, sizeof(struct po_ns_record));
		_mm_sfence();
		curr->ptrs[0] = (struct po_ns_trie_node *)rc;
		flush_clwb(&(curr->ptrs[0]), sizeof(struct po_ns_record *));
		_mm_sfence();
		return rc;
	} else {
		//the first element is exist
		return NULL;
	}
}

/*
 * we do not free po_ns_trie_node or po_ns_container,
 * please implement it in the future.
 */
struct po_ns_record *po_ns_delete(const char *str, int strlen)
{
	struct po_super *ps;
	struct po_ns_trie_node *prev, *curr;
	struct po_ns_record *retval = NULL;
	int depth;
	int index;

	ps = po_get_super();
	curr = ps->trie_root;
	prev = NULL;
	depth = 1;
	while (depth < strlen + 2) {
		if (depth == (int)(curr->depth)) {
			prev = curr;
			index = (int)str[depth - 1];
			curr = curr->ptrs[index];
			if (curr == NULL)
				return retval;
			depth++;
		} else {
			return po_ns_delete_record(
				(struct po_ns_container *)(curr), depth - 1,
				str, strlen);
		}
	}
	if (prev->ptrs[0] != NULL) {
		retval = (struct po_ns_record *)prev->ptrs[0];
		prev->ptrs[0] = NULL;
	}
	return retval;
}

struct po_ns_record *po_ns_delete_record(struct po_ns_container *container,
					 int depth, const char *str, int strlen)
{
	struct po_ns_record *prev, *curr, *predo;

	prev = NULL;
	curr = container->record_first;
	while (curr != NULL) {
		if ((curr->strlen == strlen - depth) &&
		    (memcmp(str + depth, curr->str, strlen - depth) == 0)) {
			/* same as po_ns_insert_record,we need update 
			cnt_limit and next pointer atomically,we employ
			log mechisim to guarantee it*/
			if (prev == NULL) {
				//remove the first record in container
				if (curr->next == NULL)
					predo = NULL;
				else
					predo = curr->next;
				write_redolog(&curr, predo, container,
					      container->cnt_limit - 1);
				_mm_sfence();
				container->record_first = curr->next;
				_mm_sfence();
				flush_clwb(&(container->record_first),
					   sizeof(struct po_ns_record *));

			} else {
				write_redolog(&(prev->next), curr->next,
					      container, container->cnt_limit - 1);
				_mm_sfence();
				prev->next = curr->next;
				_mm_sfence();
				flush_clwb(&(prev->next),
					   sizeof(struct po_ns_record *));
			}
			if (curr->strlen != 0)
				kpfree(curr->str);
			container->cnt_limit--;
			flush_clwb(&(container->cnt_limit), sizeof(int));
			_mm_sfence();
			erasure_redolog();

			// kpfree(curr);
			// the caller need to free it
			return curr;
		}
		prev = curr;
		curr = curr->next;
	}
	return NULL;
}

int po_ns_need_burst(struct po_ns_container *cont)
{
	//we set cnt_limit for test
	if (cont->cnt_limit > BURST_LIMIT)
		return 1;
	else
		return 0;
}

struct po_ns_record *po_ns_burst(struct po_ns_trie_node *prev_trie_node,
				 int prev_index)
{
	struct po_ns_trie_node *new_node;
	struct po_ns_container *cont, *newcont;
	struct po_ns_record *curr_record, *free_record, *pfree_record;
	struct po_ns_record *new_record, *retval = NULL;
	int i;
	int index;

	cont = (struct po_ns_container *)prev_trie_node->ptrs[prev_index];
	new_node = kpmalloc(sizeof(struct po_ns_trie_node), GFP_KERNEL);
	new_node->depth = prev_trie_node->depth + 1;
	for (i = 0; i < PO_NS_LENGTH; i++)
		new_node->ptrs[i] = NULL; //we can use memset directly
	flush_clwb(new_node, sizeof(struct po_ns_trie_node));
	_mm_sfence();

	curr_record = cont->record_first;
	while (curr_record != NULL) {
		new_record = kpmalloc(sizeof(struct po_ns_record), GFP_KERNEL);
		new_record->desc = curr_record->desc;
		/* yes, return the first record */
		if (retval == NULL)
			retval = new_record;
		new_record->next = NULL;
		if (curr_record->strlen == 0) {
			new_record->strlen = 0;
			new_record->str = NULL;
			new_record->next = NULL;
			new_node->ptrs[0] =
				(struct po_ns_trie_node *)new_record;
		} else {
			new_record->strlen = curr_record->strlen - 1;
			if (new_record->strlen != 0) {
				new_record->str = kpmalloc(new_record->strlen,
							   GFP_KERNEL);
				for (i = 0; i < new_record->strlen; i++)
					new_record->str[i] =
						curr_record->str[i + 1];
			} else {
				new_record->str = NULL;
			}
			index = (int)curr_record->str[0];
			if (new_node->ptrs[index] == NULL) {
				newcont = kpmalloc(sizeof(struct po_ns_container),
						 GFP_KERNEL);
				newcont->cnt_limit = 1;
				newcont->record_first = new_record;
				flush_clwb(newcont,
					   sizeof(struct po_ns_container));
				_mm_sfence();
				new_node->ptrs[index] =
					(struct po_ns_trie_node *)newcont;
			} else {
				newcont = (struct po_ns_container
						   *)(new_node->ptrs[index]);
				new_record->next = newcont->record_first;
				newcont->record_first = new_record;
				newcont->cnt_limit++;
				flush_clwb(newcont,
					   sizeof(struct po_ns_container));
				_mm_sfence();
			}
		}
		flush_clwb(new_record, sizeof(struct po_ns_record));
		_mm_sfence();
		//free_record = curr_record;
		curr_record = curr_record->next;
		// if (free_record->strlen != 0)
		// 	kpfree(free_record->str);
		// kpfree(free_record);
	}
	flush_clwb(new_node, sizeof(struct po_ns_trie_node));
	_mm_sfence();
	prev_trie_node->ptrs[prev_index] = new_node;
	flush_clwb(&(prev_trie_node->ptrs[prev_index]),
		   sizeof(struct po_ns_trie_node *));
	_mm_sfence();
	//we should switch the pointer before free
	free_record = cont->record_first;
	while (free_record != NULL) {
		pfree_record = free_record;
		free_record = free_record->next;
		if (pfree_record->strlen != 0)
			kpfree(pfree_record->str);
		kpfree(pfree_record);
	}
	kpfree(cont);
	return retval;
}

void write_redolog(struct po_ns_record **p, struct po_ns_record *predo,
		   struct po_ns_container *cont, int cnt_limit)
{
	struct po_super *ps;
	ps = po_get_super();
	ps->redolog->p = p;
	ps->redolog->predo = predo;
	ps->redolog->cont = cont;
	ps->redolog->cnt_limit = cnt_limit;
	flush_clwb(ps->redolog, sizeof(struct po_redolog4del));
	_mm_sfence();
	ps->redolog->valid = 1;
	flush_clwb(&(ps->redolog->valid), sizeof(int));
	_mm_sfence();
}

void erasure_redolog()
{
	struct po_super *ps;
	ps = po_get_super();
	ps->redolog->valid = 0;
	flush_clwb(&(ps->redolog->valid), sizeof(int));
	_mm_sfence();
}
void recover_from_redolog(struct po_super *ps)
{
	if (ps->redolog->valid == 0)
		return;

	pr_info("start to recover namespace\n");
	ps->redolog->cont->cnt_limit = ps->redolog->cnt_limit;
	if (*(ps->redolog->p) != ps->redolog->predo) {
		struct po_ns_record *rec_free = *(ps->redolog->p);
		//original next pointer not update yet
		*(ps->redolog->p) = ps->redolog->predo;
		flush_clwb((ps->redolog->p), sizeof(struct po_ns_record *));
		_mm_sfence();
		if (rec_free->strlen != NULL)
			kpfree(rec_free->str);
	}
	flush_clwb(&(ps->redolog->cont->cnt_limit), sizeof(int));
	_mm_sfence();
	ps->redolog->valid = 0;
	flush_clwb(&(ps->redolog->valid), sizeof(int));
	_mm_sfence();
	return;
}
