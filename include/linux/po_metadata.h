/*
 * Management of metadata and namespace for persistent object
 * Author Chen
 * */
#include <linux/list.h> //vm_list
#include <types.h> //uid gid

#define PO_NS_LENGTH 128		//ASCII 0~128

struct po_super
{
	unsigned long long trie_root;
	int trie_node_count;
	int container_count;
	int po_count;
};

struct po_desc
{
	struct list_head d_vm_list;
	unsigned long long prime_seg;//记录持久对象的首地址。
	uid_t	d_uid;//用户id
	gid_t	d_gid;//组id
};

struct po_ns_record
{
	int strlen;
	unsigned long long str;
	unsigned long long desc;
	unsigned long long next;
};

struct po_ns_container
{
	unsigned long long record_first;
	int cnt_limit;
};

struct po_ns_trie_node
{
	int depth;
	unsigned long long ptrs[PO_NS_LENGTH];
};

extern struct po_ns_record * po_ns_search(const char* str,int strlen); //extren 关键字有的内核源码加了，有的地方没加，暂时加上吧
extern struct po_ns_record * po_ns_insert(const char* str,int strlen);
extern struct po_ns_record * po_ns_delete(const char* str,int strlen);
