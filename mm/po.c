/*
 * Author: liuchaojie
 */

#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/po_metadata.h>
#include <linux/cred.h>
#include <asm-generic/io.h>
#include <uapi/asm-generic/errno.h>
#include <uapi/asm-generic/errno-base.h>
#include <linux/podtable.h>
#include <linux/sched.h>

/*
 * This micro should be defined in linux/po_metadata.h.
 * Including the trailing NUL, the max length of po is 256.
 */
#ifndef MAX_PO_NAME_LENGTH
#define MAX_PO_NAME_LENGTH	256
#endif

/*
 * We define kpmalloc(kpfree) as kmalloc(kfree), and it is
 * used to allocate space from persistent memory.
 * In the future, someone(yes, someone) will implement them.
 */
#define kpmalloc	kmalloc
#define kpfree		kfree

/*
 * Syscalls about persistent object depend on metadata module,
 * which provides three interfaces: po_ns_search, po_ns_insert
 * and po_ns_delete. To test these syscalls independently, we
 * implement the above three interfaces simply with the following
 * codes.
 */

struct po_ns_record *po_ns_search(const char *str, int strlen)
{
	struct po_ns_record *rcd;
	struct po_desc *desc;

	rcd = kpmalloc(sizeof(*rcd), GFP_KERNEL);
	desc = kpmalloc(sizeof(*desc), GFP_KERNEL);
	desc->size = 0;
	rcd->desc = (struct po_desc *)virt_to_phys(desc);
	return rcd;
}

struct po_ns_record *po_ns_insert(const char *str, int strlen)
{
	struct po_ns_record *rcd;

	rcd = kpmalloc(sizeof(*rcd), GFP_KERNEL);
	rcd->desc = NULL;
	return rcd;
}

struct po_ns_record *po_ns_delete(const char *str, int strlen)
{
	struct po_ns_record *rcd;
	struct po_desc *desc;

	rcd = kpmalloc(sizeof(*rcd), GFP_KERNEL);
	desc = kpmalloc(sizeof(*desc), GFP_KERNEL);
	desc->size = 0;
	rcd->desc = (struct po_desc *)virt_to_phys(desc);
	return rcd;
}

void free_chunk(struct po_chunk *chunk)
{
	/*
	 * First we should free pages allocated from pmm(lifan).
	 * There should be a mechanism to transfer start(a
	 * physical address) to the corresponding instance of
	 * struct pt_page(may be by a global variable).
	 */

	/*
	 * TODO: free pages.
	 */

	kpfree(chunk);
}

/*
 * Functions about podtable.h
 */

inline int pos_insert(struct po_desc *pod)
{
	int i;
	struct po_desc **po_array;

	po_array = current->pos->po_array;
	for (i = 0; i < NR_OPEN_DEFAULT; i++) {
		if (po_array[i] == NULL) {
			po_array[i] = pod;
			return i;
		}
	}
	return -EMFILE;
}

inline int pos_delete(unsigned int pod)
{
	struct po_desc **po_array;

	if (pod < 0 || pod >= NR_OPEN_DEFAULT)
		return -EBADF;

	po_array = current->pos->po_array;
	if (po_array[pod] == NULL)
		return -EBADF;
	po_array[pod] = NULL;
	return 0;
}

inline bool pos_is_open(struct po_desc *pod)
{
	int i;
	struct po_desc **po_array;

	po_array = current->pos->po_array;
	for (i = 0; i < NR_OPEN_DEFAULT; i++) {
		if (po_array[i] == pod)
			return true;
	}
	return false;
}

struct pos_struct init_pos = {
	.count		= ATOMIC_INIT(1),
	.po_array	= {0},
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

/*
 * TODO: check if mode and flags are legal.
 */

/*
 * For now, we didn't consider the parameter of mode.
 */
SYSCALL_DEFINE2(po_creat, const char __user *, poname, umode_t, mode)
{
	char *kponame;
	int len, i;
	struct po_ns_record *rcd;
	struct po_desc *desc;
	int retval;

	kponame = kmalloc(MAX_PO_NAME_LENGTH, GFP_KERNEL);
	if (!kponame)
		return -ENOMEM;
	len = strncpy_from_user(kponame, poname, MAX_PO_NAME_LENGTH);
	if (len < 0) {
		kfree(kponame);
		return len;
	}
	if (len == MAX_PO_NAME_LENGTH) {
		kfree(kponame);
		return -ENAMETOOLONG;
	}
	for (i = 0; i < len; i++) {
		if (kponame[i] == '/') {
			kfree(kponame);
			return -EINVAL;
		}
	}

	rcd = po_ns_insert(kponame, len);
	if (rcd == NULL)
		return -EEXIST;

	desc = kpmalloc(sizeof(*desc), GFP_KERNEL);
	desc->size = 0;
	desc->data_pa = NULL;
	desc->uid = current_uid().val;
	desc->gid = current_gid().val;
	desc->mode = mode;
	desc->flags = O_CREAT|O_RDWR;
	rcd->desc = (struct po_desc *)virt_to_phys(desc);

	retval = pos_insert(desc);
	if (retval < 0) {
		po_ns_delete(kponame, len);
		kfree(kponame);
		kfree(desc);
		return retval;
	}

	kfree(kponame);
	return retval;
}

SYSCALL_DEFINE1(po_unlink, const char __user *, poname)
{
	char *kponame;
	int len, i;
	struct po_ns_record *rcd;
	struct po_desc *desc;
	struct po_chunk *curr, *next;
	int retval;

	kponame = kmalloc(MAX_PO_NAME_LENGTH, GFP_KERNEL);
	if (!kponame)
		return -ENOMEM;
	len = strncpy_from_user(kponame, poname, MAX_PO_NAME_LENGTH);
	if (len < 0) {
		kfree(kponame);
		return len;
	}
	if (len == MAX_PO_NAME_LENGTH) {
		kfree(kponame);
		return -ENAMETOOLONG;
	}
	for (i = 0; i < len; i++) {
		if (kponame[i] == '/') {
			kfree(kponame);
			return -EINVAL;
		}
	}

	/* check whether po is opened(busy) */
	rcd = po_ns_search(kponame, len);
	if (rcd == NULL) {
		kfree(kponame);
		return -ENOENT;
	}
	desc = (struct po_desc *)phys_to_virt(rcd->desc);
	if (pos_is_open(desc) == true) {
		kfree(kponame);
		return -EBUSY;
	}

	po_ns_delete(kponame, len);
	desc = (struct po_desc *)phys_to_virt(rcd->desc);
	if (desc->size != 0) {
		curr = (struct po_chunk *)phys_to_virt(desc->data_pa);
		while (curr != NULL) {
			next = (struct po_chunk *)phys_to_virt(curr->next_pa);
			free_chunk(curr);
			curr = next;
		}
	}

	kfree(kponame);
	return 0;
}

SYSCALL_DEFINE3(po_open, const char __user *, poname, int, flags, umode_t, mode)
{
	char *kponame;
	int len, i;
	struct po_ns_record *rcd;
	struct po_desc *desc;
	int retval;

	kponame = kmalloc(MAX_PO_NAME_LENGTH, GFP_KERNEL);
	if (!kponame)
		return -ENOMEM;
	len = strncpy_from_user(kponame, poname, MAX_PO_NAME_LENGTH);
	if (len < 0) {
		kfree(kponame);
		return len;
	}
	if (len == MAX_PO_NAME_LENGTH) {
		kfree(kponame);
		return -ENAMETOOLONG;
	}
	for (i = 0; i < len; i++) {
		if (kponame[i] == '/') {
			kfree(kponame);
			return -EINVAL;
		}
	}

	rcd = po_ns_search(kponame, len);
	if (rcd == NULL) {
		if (flags & O_CREAT) {
			rcd = po_ns_insert(kponame, len);
			if (rcd == NULL) {
				kfree(kponame);
				return -ENOENT;
			}
			desc = kpmalloc(sizeof(*desc), GFP_KERNEL);
			desc->size = 0;
			desc->data_pa = NULL;
			desc->uid = current_uid().val;
			desc->gid = current_gid().val;
			desc->mode = mode;
			desc->flags = flags;
			rcd->desc = (struct po_desc *)virt_to_phys(desc);
		} else {
			kfree(kponame);
			return -ENOENT;
		}
	}

	retval = pos_insert(desc);
	if (retval < 0) {
		po_ns_delete(kponame, len);
		kfree(kponame);
		kfree(desc);
		return retval;
	}

	kfree(kponame);
	return retval;
}
