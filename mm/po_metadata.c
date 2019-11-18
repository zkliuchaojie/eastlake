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

/*
 * the following codes is the same with mm/po.c,
 * please refactor it in the future.
 */
#ifdef CONFIG_ZONE_PM_EMU

#define pt_page_to_virt(page)	phys_to_virt(pt_page_to_pfn(page)<<PAGE_SHIFT_REDEFINED)
#define pt_page_to_phys(page)	(pt_page_to_pfn(page)<<PAGE_SHIFT_REDEFINED)
#define virt_to_pt_page(p)	pfn_to_pt_page((virt_to_phys(p)>>PAGE_SHIFT_REDEFINED))
#define phys_to_pt_page(phys)	pfn_to_pt_page(phys>>PAGE_SHIFT_REDEFINED)

/*
 * for now, kpmalloc/kpfree is used to alloc/free AEP space not bigger than 4KB.
 */
static void *kpmalloc(size_t size, gpfp_t flags)
{
	struct pt_page *page;

	if (size > PAGE_SIZE_REDEFINED)
		return NULL;
	page = alloc_pt_pages_node(0, flags, 0);
	if (page == NULL)
		return NULL;
	pr_info("page: %#lx", (unsigned long)page);
	pr_info("pt_page_to_pfn: %ld", pt_page_to_pfn(page));
	return (void*)pt_page_to_virt(page);
}

static void kpfree(void *objp)
{
	__free_pt_pages(virt_to_pt_page(objp), 0);
}

#else

/*
 * We define kpmalloc(kpfree) as kmalloc(kfree), and it is
 * used to allocate space from persistent memory.
 * In the future, someone(yes, someone) will implement them.
 */
#define kpmalloc	kmalloc
#define kpfree		kfree

#endif // CONFIG_ZONE_PM_EMU

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
	int i;

	root = kpmalloc(sizeof(struct po_ns_trie_node), GFP_KERNEL);
	for (i = 0; i < PO_NS_LENGTH; i++)
		root->ptrs[i] = NULL;
	root->depth = 1;
	po_super->trie_root = root;
	po_super->trie_node_count = 1;
	po_super->container_count = 0;
	po_super->po_count = 0;
	pr_info("%#lx", po_super);
}

struct po_super* po_get_super(void) //以后应该上层提供，暂时先这样写
{
	struct pm_zone 	*pm_zone;
	struct pm_super	*pm_super;

	pm_zone = NODE_DATA(0)->node_pm_zones + ZONE_PM_EMU;
	pm_super = pm_zone->super;
	pr_info("%#lx", &(pm_super->po_super));
	return &(pm_super->po_super);
}

struct po_ns_record *po_ns_search_container(struct po_ns_container *cont, \
	int depth, const char *str, int strlen)
{
	struct po_ns_record *rc;

	rc = cont->record_first;
	while (rc != NULL) {
		pr_info("rc->str: %s", rc->str);
		if ((rc->strlen == strlen - depth) && \
			 (memcmp(str+depth, rc->str, strlen-depth)==0))
				return rc;
		pr_info("rc: %#lx", rc);
		rc = rc->next;
	}
	return NULL;
}

struct po_ns_record *po_insert_record(struct po_ns_trie_node *prev_trie_node, \
	int prev_index, struct po_ns_record *record)
{
	struct po_ns_container *cont;

	pr_info("po insert record");
	cont = (struct po_ns_container *)(prev_trie_node->ptrs[prev_index]);
	if (cont->record_first != NULL)
		pr_info("str: %s", cont->record_first->str);
	record->next = cont->record_first;
	cont->record_first = record;
	//之后补充burst判定
	cont->cnt_limit++;
	pr_info("%d, cont: %#lx", cont->cnt_limit, cont);
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

	pr_info("po ns search, str: %s", str);
	depth = 1;
	ps = po_get_super();
	prev = NULL;
	curr = ps->trie_root;
	while(depth < strlen + 2) {
		if (debug)
			pr_info("depth change:%d\n",depth);
		if (debug)
			pr_info("%c\n",index);
		if(depth == (int)(curr->depth)) {
			if (debug)
				pr_info("node depth:%d\n", curr->depth);
			prev = curr;
			index = (int)str[depth-1];
			pr_info("prev: %#lx, index: %d", prev, index);
			curr = curr->ptrs[index];
			pr_info("%#lx", curr);
			if (curr == NULL)
				return NULL;
			depth++;
		} else {
			if (debug)
				pr_info("search container");
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

	pr_info("po ns insert: %s", str);

	ps = po_get_super();
	root = ps->trie_root;
	depth = 1;
	prev = NULL;
	curr = root;
	while (depth < strlen + 1) {
		if (debug)
			pr_info("depth change:%d\n",depth);
		index = (int)str[depth-1];
		prev = curr;
		curr = curr->ptrs[index];
		pr_info("index: %d, %#lx, %#lx", index, prev, curr);
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
			pr_info("%d", rc->strlen);
			rc->next = NULL;
			cont->record_first = rc;
			pr_info("prev: %#lx, index: %d", prev, index);
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
				pr_info("container: %#lx", prev->ptrs[index]);
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

	pr_info("po ns delete, str: %s, strlen: %d", str, strlen);

	ps = po_get_super();
	curr = ps->trie_root;
	prev = NULL;
	depth = 1;
	while (depth < strlen + 2) {
		pr_info("depth: %d", depth);
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

	pr_info("burst");

	cont = (struct po_ns_container *)prev_trie_node->ptrs[prev_index];
	new_node = kpmalloc(sizeof(struct po_ns_trie_node), GFP_KERNEL);
	new_node->depth = prev_trie_node->depth + 1;
	for (i = 0; i < PO_NS_LENGTH; i++)
		new_node->ptrs[i] = NULL;
	curr_record = cont->record_first;
	while (curr_record != NULL) {
		pr_info("curr_record->str: %s, %d", curr_record->str, curr_record->strlen);
		new_record = kpmalloc(sizeof(struct po_ns_record), GFP_KERNEL);
		new_record->desc = curr_record->desc;
		pr_info("new_record: %#lx, new_record->desc: %#lx", new_record, new_record->desc);
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
				pr_info("new_record->strlen == 0");
				new_record->str = NULL;
			}
			index = (int)curr_record->str[0];
			if (new_node->ptrs[index] == NULL) {
				pr_info("index: %c", curr_record->str[0]);
				newcont = kpmalloc(sizeof(struct po_ns_container), GFP_KERNEL);
				newcont->cnt_limit = 1;
				newcont->record_first = new_record;
				new_node->ptrs[index] = (struct po_ns_trie_node *)newcont;
				pr_info("po ns burst, new_node: %#lx, newcont: %#lx", new_node, newcont);
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
#define num 20
#define ind 2
/*
int main()
{
	po_super_init();
	struct po_super *p=po_get_super();
	printf("%d",p->po_count);
	printf("%d\n",p->po_count);
	int i;
	char s[4]="abcd";
	for(i=0;i<num;i++)
	{

		if(debug)printf("=====%dth insert %s=====\n",i,s);
		struct po_ns_record * p1=po_ns_insert(s,4);
		s[ind]++;
		printf("P1:\t%p\t%d\n",p1,i);
		if(debug)printf("=====%dth insert finished=====\n",i);
	}
	
	char s2[4]="abcd";
	printf("::::::::::::::::::::::::::::::::::::::::\n");
	for(i=0;i<num;i++)
	{
		if(debug)printf("======%d search %s=====\n",i,s2);
		struct po_ns_record * p2=po_ns_search(s2,4);
		s2[ind]++;
		printf("P2:\t%p\t%d\n",p2,i);
		if(debug)printf("======%d search finished\n",i);
	}
	

		struct po_ns_record * p1=po_ns_insert("a",1);
		printf("P1:\t%p\n",p1);
		struct po_ns_record * p2=po_ns_search("a",1);
		printf("P2:\t%p\n",p2);
	return 0;
}
*/
