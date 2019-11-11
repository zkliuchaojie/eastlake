/*
 * Author Chen
 */
#include <linux/po_metadata.h>
//include<linux/po_alloc.h> 上层提供的
//#define unsign_to_p(a) (a) //指针和物理地址具体转换还不清楚
//#define p_to_unsign(a) (a)
//#include<linux/slab.h>

//GFP_KERNEL 依赖暂时无法解决，直接摘出来
//#define ___GFP_IO               0x40u
//#define ___GFP_FS               0x80u

//#define __GFP_IO               0x40u
//#define __GFP_FS               0x80u
//#define __GFP_RECLAIM ((__force gfp_t)(___GFP_DIRECT_RECLAIM|___GFP_KS    WAPD_RECLAIM))
//#define GFP_KERNEL      (__GFP_RECLAIM | __GFP_IO | __GFP_FS)

//#define GFP_KERNEL      (__GFP_IO | __GFP_FS) //依赖难以解决，删掉第一个试试
//#include<linux/slab.h>

#define debug 0
#define debugburst 1
#define BURST_LIMIT 4
unsigned long long po_super;
void *po_alloc(int size)
{
	//return kmalloc(size,GFP_KERNEL);//依赖难以解决，暂时用malloc吧
	return malloc(size);//,GFP_KERNEL);
}




void po_super_init()
{
	struct po_super  *ps;
	struct po_ns_trie_node *root;
	int i;

	ps=po_alloc(sizeof(struct po_super));
	po_super=ps;
	root=po_alloc(sizeof(struct po_ns_trie_node));
	for(i=0;i<PO_NS_LENGTH;i++)
	{
		root->ptrs[i]=NULL;
	}
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
	if(debug) printf("search container depth:%d\n",depth);
	struct po_ns_record *rc;
	int i;
	rc=container->record_first;
	//上次工作到这里，bug没找到
	while(rc!=NULL)
	{
		if(debug) printf("record in container first letter:%s,rc.strlen:%d,str_len:%d,depth:%d.\n",rc->str,rc->strlen,str_len,depth);
		if(rc->strlen!=str_len-depth)
		{
			if(debug) printf("strlen is NOT OK");
			rc=rc->next;
			continue;
		}
		else
		{
			if(debug)	printf("strlen is OK\n");
			if(debug) printf("strlen is really OK\nstr+depth:%s\nrc->str:%s\nstr_len-depth:%d\n",str+depth,rc->str,str_len-depth);
			if(str_len-depth==0&&rc->strlen==0)
			{
				return rc;
			}
			if(memcmp(str+depth,rc->str,str_len-depth)==0)
			{
				if(debug) printf("successfully returned\n");
				return rc;
			}
			else 
			{
				if(debug) printf("NOT OK\n");
				rc=rc->next;
				continue;
			}
		}
	}
	
	return NULL;
}

void po_insert_record(struct po_ns_trie_node *prev_trie_node,int prev_index,struct po_ns_record *record) {
	struct po_ns_container *container;
	container=prev_trie_node->ptrs[prev_index];
	record->next=container->record_first;
	container->record_first=record;
	//之后补充burst判定
	container->cnt_limit++;
	if(po_ns_need_burst(container))
	{
		po_ns_burst(prev_trie_node,prev_index);
	}
}

struct po_ns_record * po_ns_search(const char* str,int strlen){
	struct po_super *ps;
	struct po_ns_trie_node *node,*prev_node;
	struct po_ns_container *cont;
	struct po_ns_record *rc;
	int depth;
	int index;

	depth=1;
	ps=po_get_super();
	node=ps->trie_root;
	prev_node=NULL;
	while(depth<strlen+2)
	{
		index=(int)str[depth-1];
		if(debug) printf("%c\n",index);
		if(depth==(int)(node->depth))
		{
			depth++;
			if(debug) printf("node depth:%d\n",node->depth);
			prev_node=node;
			node=node->ptrs[index];
			if(node==NULL)
				return NULL;
			continue;
		}
		else
		{
			return po_ns_search_container((struct po_ns_container *)(node),depth-1,str,strlen);
		}
	}
	return prev_node->ptrs[0];
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
		if(debug) printf("depth change:%d\n",depth);
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
			rc->next=NULL;
			//用mmcpy应该快一些，暂时先用for循环，也有可能编译器已经优化的一样了
			int strindex;
			char *stmp=rc->str;
			for(strindex=depth;strindex<strlen;strindex++)
			{

				(*stmp)=str[strindex];
				if(debug) printf("record letter:%c\n",*stmp);
				stmp++;
			}
			cont->record_first=rc;
			prev_trie_node->ptrs[index]=cont;
			return rc;
		}
		if(trie_node->depth!=depth+1)
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
					if(debug) printf("record letter:%c\n",*stmp);
					stmp++;
				}
				rc->next=NULL;
				po_insert_record(prev_trie_node,index,rc);
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
	//测试用的参数
	if(container->cnt_limit>BURST_LIMIT)
		return 1;
	else
		return 0;//暂时
}

void po_ns_burst(struct po_ns_trie_node *prev_trie_node,int prev_index){
	if(debugburst) printf("burst start!\n\n\n");
	struct po_ns_trie_node *new_node;
	struct po_ns_container *cont,*newcont;
	struct po_ns_record *curr_record,*free_record;
	struct po_ns_record *new_record;
	char *newstr;
	int i;
	int index;
	cont=prev_trie_node->ptrs[prev_index];
	new_node=po_alloc(sizeof(struct po_ns_trie_node));
	new_node->depth=prev_trie_node->depth+1;
	for(i=0;i<PO_NS_LENGTH;i++)
	{
		new_node->ptrs[i]=NULL;
	}	
	curr_record=cont->record_first;
	while(curr_record!=NULL)
	{
		if(debug) printf("%dth busrt record\n",i);
		new_record=po_alloc(sizeof(struct po_ns_record));
		new_record->strlen=curr_record->strlen-1;
		if(0)//new_record->strlen==0)
		{
			new_record->str=NULL;
			new_record->next=NULL;
			new_node->ptrs[0]=new_record;
		}
		else
		{
			//新申请字符串
			newstr=po_alloc(new_record->strlen);
			for(i=0;i<new_record->strlen;i++)
			{
	//if(debug)printf("@@@@@@@@@@@@@@@@@@@@@@@\n");
				newstr[i]=curr_record->str[i+1];
			}
			new_record->str=newstr;
			index=(int)curr_record->str[0];
			if(new_node->ptrs[index]==NULL)
			{
				if(debug)printf("new index: %d******************\n",index);
				newcont=po_alloc(sizeof(struct po_ns_container));
				newcont->cnt_limit=1;
				newcont->record_first=new_record;
				new_record->next=NULL;
				new_node->ptrs[index]=newcont;
			}
			else
			{
				//Nove 7 错误在这里
				new_record->next=((struct po_ns_container *)(new_node->ptrs[index]))->record_first;
		//		po_insert_record(new_node,index,new_record);
				((struct po_ns_container*)(new_node->ptrs[index]))->record_first=new_record;
				((struct po_ns_container*)(new_node->ptrs[index]))->cnt_limit++;


			}

		}
		free_record=curr_record;	
		curr_record=curr_record->next;
		free(free_record->str);
		free(free_record);
	}
	free(prev_trie_node->ptrs[prev_index]);
	prev_trie_node->ptrs[prev_index]=new_node;
	if(debug)printf("new node depth:%d,prev index:%d\n",new_node->depth,prev_index);
}
#define num 20
#define ind 2
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

