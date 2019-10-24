/*
 * Author Chen
 */
#include <linux/po_metadata.h>
//include<linux/po_malloc.h> 上层提供的
//#define unsign_to_p(a) (a) //指针和物理地址具体转换还不清楚
//#define p_to_unsign(a) (a)
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
	po_super=ps;
	root=po_alloc(sizeof(struct po_ns_trie_node));
	root->depth=1;
	ps->trie_root=root;
	ps->trie_node_count=1;
	ps->container_count=0;
	ps->po_count=0;
}
struct po_super* po_get_super() //以后应该上层提供，暂时先这样写
{
	return po_super;
}

struct po_ns_record *po_ns_search_container(struct po_ns_container *container,int depth,char *str,int str_len){
	return NULL;
}

void po_insert_record(struct po_ns_container *container,struct po_ns_record *record) {
	record->next=container->record_first;
	container->record_first=record;
	//之后补充burst判定
	container->cnt_limit++;
}

struct po_ns_record * po_ns_search(const char* str,int strlen){

}
struct po_ns_record * po_ns_insert(const char* str,int strlen){
	struct po_super *ps;
	int depth;
	int index;
	struct po_ns_trie_node *prev_trie_node,*trie_node,*root;
	struct po_ns_record *rec;
	ps=po_get_super();
	root=ps->trie_root;
	depth=1;
	prev_trie_node=NULL;
	trie_node=root;
	while(depth<=strlen)
	{
		index=(int)str[depth-1];
		prev_trie_node=trie_node;
		trie_node=trie_node->ptrs[index];
		if(trie_node==NULL)
		{
			//中间是空的，新建节点
			struct po_ns_container *cont=po_alloc(sizeof(struct po_ns_container));
			struct po_ns_record *rc=po_alloc(sizeof(struct po_ns_record));
			rc->strlen=strlen-depth;
			rc->str=po_alloc(rc->strlen);
			//用mmcpy应该快一些，暂时先用for循环，也有可能编译器已经优化的一样了
			int strindex;
			char *stmp=rc->str;
			for(strindex=depth;strindex<=strlen;strindex++)
			{

				(*stmp)=str[strindex];
				stmp++;
			}
			rc->next=NULL;
			cont->record_first=rc;
			prev_trie_node->ptrs[index]=cont;
			return rc;
		}
		if(trie_node->depth!=depth)
		{
			//是个容器,搜容器，有返回，没新建
			if(po_ns_search_container((struct po_ns_container*)trie_node,prev_trie_node->depth,str,strlen)==NULL)
			{
				struct po_ns_record *rc=po_alloc(sizeof(struct po_ns_record));
				
				rc->strlen=strlen-depth;
				rc->str=po_alloc(rc->strlen);
				//用mmcpy应该快一些，暂时先用for循环，也有可能编译器已经优化的一样了
				int strindex;
				char *stmp=rc->str;
				for(strindex=depth;strindex<=strlen;strindex++)
				{
					(*stmp)=str[strindex];
					stmp++;
				}
				rc->next=NULL;
				po_insert_record((struct po_ns_container*)trie_node,rc);
				return rc;
			}
			else
				return NULL;
		}
		depth++;
	}
	//走到这里是trie_node中第一个元素的逻辑
	if(trie_node->ptrs[0]==NULL)
	{
		struct po_ns_record *rc=po_alloc(sizeof(struct po_ns_record));
		rc->strlen=0;
		rc->next=NULL;
		trie_node->ptrs[0]=rc;
		return rc;
	}
	else
	{
		//第一个元素已经存在
		return NULL;
	}
	


}
struct po_ns_record * po_ns_delete(const char* str,int strlen){

}


struct po_ns_record *po_ns_delete_record(struct po_ns_container *container,int depth,const char *str,int str_length){

}

int po_ns_need_burst(struct po_ns_container *container){

}

void po_ns_burst(struct po_ns_trie_node *prev_trie_node,int prev_index){

}
