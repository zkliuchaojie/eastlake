/*
 * Author: liuchaojie
 */

#ifndef _LINUX_PO_H_
#define _LINUX_PO_H_

#ifndef _LINUX_PO_ME
#include <linux/po_metadata.h>
#endif
#include <asm-generic/bitsperlong.h>

#define NR_OPEN_DEFAULT BITS_PER_LONG

/*
 * Persistent Objects Structure
 * Used in task_struct likes files_struct.
 * As you seen, the implementation of pos_struct is
 * very simple, in which the number of po is fixed
 * and tranversing po_array to find empty slots, and
 * we will enhance it.
 */
struct pos_struct {
	struct po_desc *po_array[NR_OPEN_DEFAULT];
};

/*
 * Insert a persistent object descriptor into pos, if
 * there are no empty slots, return -EMFILE, or return
 * the index of persistent object descriptor stored in
 * po_array. There is a coincidence, the shortage name
 * of persistent object descriptor is pod, which is same
 * with the index, but it is a unsigned int type.
 */
int pos_insert(struct po_desc *pod);

int pos_delete(unsigned int pod);

bool pos_is_open(struct po_desc *pod);

#endif
