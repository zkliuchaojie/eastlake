/*
 * Author Chen
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

#define debug 1
#define debugburst 1
#define BURST_LIMIT 4

/*
void *po_alloc(int size)
	//return kmalloc(size,GFP_KERNEL);//依赖难以解决，暂时用malloc吧

{
	//return kmalloc(size,GFP_KERNEL);//依赖难以解决，暂时用malloc吧
	return malloc(size);//,GFP_KERNEL);
}
*/

void po_super_init(struct po_super *po_super)
{
	struct po_ns_trie_node *root;
	struct po_vma *vma;
	int i;

	root = kpmalloc(sizeof(struct po_ns_trie_node), GFP_KERNEL);
	for (i = 0; i < PO_NS_LENGTH; i++)
		root->ptrs[i] = NULL;
	root->depth = 1;
	po_super->trie_root = root;
	po_super->trie_node_count = 1;
	po_super->container_count = 0;
	po_super->po_count = 0;
	// init non-continuous address space
	vma = kpmalloc(sizeof(struct po_vma), GFP_KERNEL);
	vma->start = PO_NON_CONTINUOUS_MAP_AREA_START;
	vma->size = PO_NON_CONTINUOUS_MAP_SIZE;
	vma->next_pa = NULL;
	po_super->vma_free_list_pa = virt_to_phys(vma);
	pr_info("PO_MAP_AREA_START: %#lx, PO_MAP_AREA_END: %#lx", \
		PO_MAP_AREA_START, PO_MAP_AREA_END);
}

struct po_super* po_get_super(void) //以后应该上层提供，暂时先这样写
{
	struct pm_zone 	*pm_zone;
	struct pm_super	*pm_super;

	pm_zone = NODE_DATA(0)->node_pm_zones + ZONE_PM_EMU;
	pm_super = pm_zone->super;
	return &(pm_super->po_super);
}

struct po_ns_record *po_ns_search_container(struct po_ns_container *cont, \
	int depth, const char *str, int strlen)
{
	struct po_ns_record *rc;

	rc = cont->record_first;
	while (rc != NULL) {
		if ((rc->strlen == strlen - depth) && \
			 (memcmp(str+depth, rc->str, strlen-depth)==0))
				return rc;
		rc = rc->next;
	}
	return NULL;
}

struct po_ns_record *po_insert_record(struct po_ns_trie_node *prev_trie_node, \
	int prev_index, struct po_ns_record *record)
{
	struct po_ns_container *cont;

	cont = (struct po_ns_container *)(prev_trie_node->ptrs[prev_index]);
	record->next = cont->record_first;
	cont->record_first = record;
	//之后补充burst判定
	cont->cnt_limit++;
	if(po_ns_need_burst(cont))
		return po_ns_burst(prev_trie_node, prev_index);
	return record;
}

struct po_ns_record * po_ns_search(const char* str, int strlen)
{
	struct po_super *ps;
	struct po_ns_trie_node *prev, *curr;
	int depth;
	int index;

	depth = 1;
	ps = po_get_super();
	prev = NULL;
	curr = ps->trie_root;
	while(depth < strlen + 2) {
		if(depth == (int)(curr->depth)) {
			prev = curr;
			index = (int)str[depth-1];
			curr = curr->ptrs[index];
			if (curr == NULL)
				return NULL;
			depth++;
		} else {
			return po_ns_search_container((struct po_ns_container *)(curr), \
				depth - 1, str, strlen);
		}
	}
	return (struct po_ns_record *)prev->ptrs[0];
}

struct po_ns_record *po_ns_insert(const char* str, int strlen)
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
		index = (int)str[depth-1];
		prev = curr;
		curr = curr->ptrs[index];
		if (curr == NULL) {
			cont = kpmalloc(sizeof(struct po_ns_container), GFP_KERNEL);
			rc = kpmalloc(sizeof(struct po_ns_record), GFP_KERNEL);
			rc->strlen = strlen - depth;
			if (rc->strlen != 0) {
				rc->str = kpmalloc(rc->strlen, GFP_KERNEL);
				for(strindex=depth; strindex<strlen; strindex++)
					rc->str[strindex-depth] = str[strindex];
			} else {
				rc->str = NULL;
			}
			rc->next = NULL;
			cont->record_first = rc;
			cont->cnt_limit = 1;
			prev->ptrs[index]=(struct po_ns_trie_node *)cont;
			return rc;
		}
		if (curr->depth != depth+1) {
			// means it is a container
			if(po_ns_search_container((struct po_ns_container*)curr, \
				prev->depth, str, strlen) == NULL) {
				rc = kpmalloc(sizeof(struct po_ns_record), GFP_KERNEL);
				rc->desc = NULL;
				rc->strlen = strlen-depth;
				if (rc->strlen != 0) {
					rc->str = kpmalloc(rc->strlen, GFP_KERNEL);
					for(strindex=depth; strindex<strlen; strindex++)
						rc->str[strindex-depth] = str[strindex];
				} else {
					rc->str = NULL;
				}
				rc->next=NULL;
				return po_insert_record(prev, index, rc);
			}
			else {
				return NULL;
			}
		}
		depth++;
	}
	//走到这里是trie_node中第一个元素的逻辑
	if (curr->ptrs[0] == NULL) {
		rc = kpmalloc(sizeof(struct po_ns_record), GFP_KERNEL);
		rc->strlen = 0;
		rc->next = NULL;
		curr->ptrs[0] = (struct po_ns_trie_node *)rc;
		return rc;
	} else {
		//第一个元素已经存在
		return NULL;
	}
}

/*
 * we do not free po_ns_trie_node or po_ns_container,
 * please implement it in the future.
 */
struct po_ns_record * po_ns_delete(const char* str, int strlen)
{
	struct po_super 	*ps;
	struct po_ns_trie_node 	*prev, *curr;
	struct po_ns_record	*retval = NULL;
	int	depth;
	int 	index;

	ps = po_get_super();
	curr = ps->trie_root;
	prev = NULL;
	depth = 1;
	while (depth < strlen + 2) {
		if (depth == (int)(curr->depth)) {
			prev = curr;
			index = (int)str[depth-1];
			curr = curr->ptrs[index];
			if (curr == NULL)
				return retval;
			depth++;
		} else {
			return po_ns_delete_record( \
				(struct po_ns_container *)(curr), depth-1, str, strlen);
		}
	}
	if (prev->ptrs[0] != NULL) {
		retval = (struct po_ns_record *)prev->ptrs[0];
		prev->ptrs[0] = NULL;
	}
	return retval;
}


struct po_ns_record *po_ns_delete_record(struct po_ns_container *container, \
	int depth, const char *str, int strlen)
{
	struct po_ns_record *prev, *curr;

	prev = NULL;
	curr = container->record_first;
	while (curr != NULL) {
		if ((curr->strlen == strlen - depth) && \
			(memcmp(str+depth, curr->str, strlen-depth)==0)) {
			if (curr->strlen != 0)
				kpfree(curr->str);
			if (prev == NULL)
				container->record_first = curr->next;
			else
				prev->next = curr->next;
			container->cnt_limit--;
			return curr;
		}
		prev = curr;
		curr = curr->next;
	}
	return NULL;
}

int po_ns_need_burst(struct po_ns_container *cont)
{
	//测试用的参数
	if(cont->cnt_limit > BURST_LIMIT)
		return 1;
	else
		return 0;//暂时
}

struct po_ns_record *po_ns_burst(struct po_ns_trie_node *prev_trie_node, int prev_index)
{
	struct po_ns_trie_node *new_node;
	struct po_ns_container *cont, *newcont;
	struct po_ns_record *curr_record, *free_record;
	struct po_ns_record *new_record, *retval = NULL;
	int i;
	int index;

	cont = (struct po_ns_container *)prev_trie_node->ptrs[prev_index];
	new_node = kpmalloc(sizeof(struct po_ns_trie_node), GFP_KERNEL);
	new_node->depth = prev_trie_node->depth + 1;
	for (i = 0; i < PO_NS_LENGTH; i++)
		new_node->ptrs[i] = NULL;
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
			new_node->ptrs[0] = (struct po_ns_trie_node *)new_record;
		} else {
			new_record->strlen = curr_record->strlen - 1;
			if (new_record->strlen != 0) {
				new_record->str = kpmalloc(new_record->strlen, GFP_KERNEL);
				for (i=0; i<new_record->strlen; i++)
					new_record->str[i] = curr_record->str[i+1];
			} else {
				new_record->str = NULL;
			}
			index = (int)curr_record->str[0];
			if (new_node->ptrs[index] == NULL) {
				newcont = kpmalloc(sizeof(struct po_ns_container), GFP_KERNEL);
				newcont->cnt_limit = 1;
				newcont->record_first = new_record;
				new_node->ptrs[index] = (struct po_ns_trie_node *)newcont;
			} else {
				cont = (struct po_ns_container *)(new_node->ptrs[index]);
				new_record->next = cont->record_first;
				cont->record_first = new_record;
				cont->cnt_limit++;
			}
		}
		free_record=curr_record;	
		curr_record=curr_record->next;
		if(free_record->strlen != 0)
			kpfree(free_record->str);
		kpfree(free_record);
	}
	kpfree(prev_trie_node->ptrs[prev_index]);
	prev_trie_node->ptrs[prev_index]=new_node;
	return retval;
}
