/*
 * Author Chen
 */
#include <linux/po_metadata.h>
#define unsign_to_p()
#define p_to_unsign()
void po_super_init()
{

}

struct po_ns_record * po_ns_search(const char* str,int strlen){

}
struct po_ns_record * po_ns_insert(const char* str,int strlen){

}
struct po_ns_record * po_ns_delete(const char* str,int strlen){

}

struct po_ns_record *po_ns_search_container(struct po_ns_container *container,int depth,const char *str,int str_len){

}

void po_insert_record(struct po_ns_container,struct po_ns_record){

}

struct po_ns_record *po_ns_delete_record(struct po_ns_container *container,int depth,const char *str,int str_length){

}

int po_ns_need_burst(struct po_ns_container *container){

}

void po_ns_burst(struct po_ns_trie_node *prev_trie_node,int prev_index){

}
