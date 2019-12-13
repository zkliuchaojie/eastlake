/*
 * Management of metadata and namespace for persistent object
 * Author Chen
 */

#ifndef _LINUX_PO_METADATA_H_
#define _LINUX_PO_METADATA_H_

#include <linux/list.h> //vm_list
#include <linux/types.h> //uid gid

#define PO_NS_LENGTH 128		//ASCII 0~128

struct po_super
{
	struct po_ns_trie_node *trie_root;
	int trie_node_count;
	int container_count;
	int po_count;
};
struct po_chunk
{
	unsigned long long start;
	unsigned long long size;//bytes
	struct po_chunk *next_pa;
};

struct po_desc
{
	unsigned long long size;
	struct po_chunk *data_pa;//pa means physical address
	uid_t	uid;//用户id
	gid_t	gid;//组id
	umode_t mode;
	unsigned int flags; // assign, when open or creat
};

/*
 * NOTE: we only support 64 bit architectures.
 * And we will add more states in po_stat structure.
 */
struct po_stat {
	mode_t	st_mode;
	uid_t	st_uid;
	gid_t	st_gid;
	off_t 	st_size;
};

struct po_ns_record
{
	int strlen;
	char *str;
	struct po_desc *desc;
	struct po_ns_record *next;
};

struct po_ns_container
{
	struct po_ns_record *record_first;//貌似必须第一个
	int cnt_limit;
};

struct po_ns_trie_node
{
	int depth;
	struct po_ns_trie_node *ptrs[PO_NS_LENGTH];
};

extern struct po_ns_record * po_ns_search(const char* str,int strlen); //extren 关键字有的内核源码加了，有的地方没加，暂时加上吧
extern struct po_ns_record * po_ns_insert(const char* str,int strlen);
extern struct po_ns_record * po_ns_delete(const char* str,int strlen);

int po_ns_need_burst(struct po_ns_container *container);
struct po_ns_record *po_ns_burst(struct po_ns_trie_node *prev_trie_node,int prev_index);
struct po_ns_record *po_ns_delete_record(struct po_ns_container *container, \
	int depth, const char *str, int str_length);
void po_super_init(struct po_super *po_super);
#endif
