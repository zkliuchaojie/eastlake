/*
 * Functions about podtable.h
 */

#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/cred.h>
#include <asm-generic/io.h>
#include <uapi/asm-generic/errno.h>
#include <uapi/asm-generic/errno-base.h>
#include <linux/podtable.h>
#include <linux/sched.h>
#include <uapi/asm-generic/fcntl.h>
#include <linux/mman.h>
#include <linux/pmalloc.h>
#include <linux/mm.h>
#include <linux/string.h>

inline int pos_insert(struct po_desc *desc)
{
	int i;
	struct po_desc **po_array, **new_po_array;
    int old_size, new_size;

	po_array = current->pos->po_array;
	for (i = 0; i < current->pos->size; i++) {
		if (po_array[i] == NULL) {
			po_array[i] = desc;
			return i;
		}
	}

	/* double po_array */
    old_size = current->pos->size;
    new_size = old_size == 0 ? NR_OPEN_DEFAULT : 2*old_size;
    new_po_array = kmalloc(new_size * sizeof(struct po_desc *) ,GFP_KERNEL);
    memset(new_po_array, 0, new_size * sizeof(struct po_desc *));
    if (old_size != 0) {
        for (i = 0; i < old_size; i++)
            new_po_array[i] = po_array[i];
        kfree(po_array);
    }
    new_po_array[old_size] = desc;
    current->pos->size = new_size;
    current->pos->po_array = new_po_array;
	
    return old_size;
}

inline struct po_desc *pos_get(unsigned int pod)
{
	struct po_desc **po_array;

	if (pod < 0 || pod >= current->pos->size)
		return NULL;

	po_array = current->pos->po_array;
	return po_array[pod];
}

inline int pos_delete(unsigned int pod)
{
	struct po_desc **po_array;

	if (pod < 0 || pod >= current->pos->size)
		return -EBADF;

	po_array = current->pos->po_array;
	if (po_array[pod] == NULL)
		return -EBADF;
	po_array[pod] = NULL;
	return 0;
}

inline bool pos_is_open(struct po_desc *desc)
{
	int i;
	struct po_desc **po_array;

	po_array = current->pos->po_array;
	for (i = 0; i < current->pos->size; i++) {
		if (po_array[i] == desc)
			return true;
	}
	return false;
}

struct pos_struct init_pos = {
    .count      = ATOMIC_INIT(1),
	.size		= 0,
	.po_array	= NULL,
};

void exit_pos(struct task_struct *tsk)
{
	struct pos_struct *pos = tsk->pos;

	if (pos) {
		tsk->pos = NULL;
		if (atomic_dec_and_test(&pos->count))
			kfree(pos);
	}
}