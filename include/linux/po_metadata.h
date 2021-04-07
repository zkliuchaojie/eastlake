/*
 * @Descripttion: Management of metadata and namespace for persistent object
 * @version: 
 * @Author: Chenqinglin
 * @Date: 2020-10-11 19:40:26
 * @LastEditors: Chenqinglin
 * @LastEditTime: 2020-10-19 16:36:59
 * @FilePath: /eastlake/include/linux/po_metadata.h
 */

#ifndef _LINUX_PO_METADATA_H
#define _LINUX_PO_METADATA_H

#include <linux/list.h> //vm_list
#include <linux/types.h> //uid gid
#include <linux/po_map.h>

#define MAX_PO_NAME_LENGTH 256
#define PO_NS_LENGTH 128 //ASCII 0~128

struct po_super {
	struct po_ns_trie_node *trie_root;
	int trie_node_count;
	int container_count;
	int po_count;
	struct po_redolog4del *redolog;
	/*
	 * maintain non-continuous mapping.
	 * the virtual addr from PO_NON_CONTINUOUS_MAP_AREA_START
	 * to PO_MAP_AREA_END.
	 */
	struct po_vma *vma_free_list_pa;
};

struct po_chunk {
	unsigned long long start;
	unsigned long long size; //bytes
	struct po_chunk *next_pa;
};

struct po_desc {
	unsigned long long size;
	struct po_chunk *data_pa; //pa means physical address
	uid_t uid; //user id
	gid_t gid; //group id
	umode_t mode;
	unsigned int flags; // assign, when open or creat
};

/*
 * NOTE: we only support 64 bit architectures.
 * And we will add more states in po_stat structure.
 */
struct po_stat {
	mode_t st_mode;
	uid_t st_uid;
	gid_t st_gid;
	off_t st_size;
};

struct po_ns_record {
	int strlen;
	char *str;
	struct po_desc *desc;
	struct po_ns_record *next;
};

struct po_ns_container {
	struct po_ns_record *record_first; //it has to be first
	int cnt_limit;
};

struct po_ns_trie_node {
	int depth;
	struct po_ns_trie_node *ptrs[PO_NS_LENGTH];
};

struct po_redolog4del {
	int valid;
	struct po_ns_record **p;
	struct po_ns_record *predo;
	struct po_ns_container *cont;
	int cnt_limit;
};

struct po_ns_record *po_ns_search(const char *str, int strlen);
struct po_ns_record *po_ns_insert(const char *str, int strlen);
struct po_ns_record *po_ns_delete(const char *str, int strlen);

int po_ns_need_burst(struct po_ns_container *container);
struct po_ns_record *po_ns_burst(struct po_ns_trie_node *prev_trie_node,
				 int prev_index);
struct po_ns_record *po_ns_delete_record(struct po_ns_container *container,
					 int depth, const char *str,
					 int str_length);

void po_super_init(struct po_super *po_super);
struct po_super *po_get_super(void);
void recover_from_redolog(void);
void write_redolog(struct po_ns_record **p, struct po_ns_record *predo,
		   struct po_ns_container *cont, int cnt_limit);
void erasure_redolog(void);

/*
 * po mapping and unmapping functions.
 */
long po_prepare_map_chunk(struct po_chunk *chunk, unsigned long prot,
			  unsigned long flags);
long get_chunk_size(struct po_chunk *chunk);
long get_chunk_map_start(struct po_chunk *chunk);

#endif

