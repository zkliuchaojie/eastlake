/*
 * Author Chen
 */
#include <linux/po_metadata.h>
//include<linux/po_malloc.h> 上层提供的
#define unsign_to_p(a) (a) //指针和物理地址具体转换还不清楚
#define p_to_unsign(a) (a)
//#include<linux/slab.h>

//GFP_KERNEL 依赖暂时无法解决，直接摘出来
//#define ___GFP_IO               0x40u
//#define ___GFP_FS               0x80u

#define __GFP_IO               0x40u
#define __GFP_FS               0x80u
//#define __GFP_RECLAIM ((__force gfp_t)(___GFP_DIRECT_RECLAIM|___GFP_KS    WAPD_RECLAIM))
//#define GFP_KERNEL      (__GFP_RECLAIM | __GFP_IO | __GFP_FS)

#define GFP_KERNEL      (__GFP_IO | __GFP_FS) //依赖难以解决，删掉第一个试试

unsigned long long po_super;
void *po_alloc(int size)
{
	//return kmalloc(size,GFP_KERNEL);//依赖难以解决，暂时用malloc吧
	return malloc(size);
}




void po_super_init()
{
	struct po_super  *ps;
	struct po_ns_trie_node *root;
	ps=po_alloc(sizeof(struct po_super));
	po_super=p_to_unsign(ps);
	root=po_alloc(sizeof(struct po_ns_trie_node));
	root->depth=1;
	ps->trie_root=unsign_to_p(root);
	ps->trie_node_count=1;
	ps->container_count=0;
	ps->po_count=0;
}

struct po_super* po_get_super() //以后应该上层提供，暂时先这样写
{
	return p_to_unsign(po_super);
}

struct po_ns_record * po_ns_search(const char* str,int strlen){

}
struct po_ns_record * po_ns_insert(const char* str,int strlen){
	struct po_super *ps;
	int depth;
	struct po_ns_trie_node *prev_trie_node,*trie_node,root;
	ps=po_get_super();
	root=ps->unsign_to_p(trie_root);
	depth=1;
	prev_trie_node=NULL;
	trie_node=root;
	while(depth<=strlen)
	{
		index=(int)str[depth-1];
		prev_trie_node=trie_node;
		//trie_node=(struct po_ns_trie_node *)unsign_to_p()

	}


}
struct po_ns_record * po_ns_delete(const char* str,int strlen){

}

struct po_ns_record *po_ns_search_container(struct po_ns_container *container,int depth,const char *str,int str_len){

}

void po_insert_record(struct po_ns_container *container,struct po_ns_record *record) {
	record->next=unsign_to_p(container->record_first);
	container->record_first=p_to_unsign(record);
	container->cnt_limit++;
}

struct po_ns_record *po_ns_delete_record(struct po_ns_container *container,int depth,const char *str,int str_length){

}

int po_ns_need_burst(struct po_ns_container *container){

}

void po_ns_burst(struct po_ns_trie_node *prev_trie_node,int prev_index){

}
int main()
{
	po_super_init();
	struct po_super *p=po_get_super();
	printf("%d",p->po_count);
	return 0;
}
