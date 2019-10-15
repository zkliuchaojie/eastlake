/*
 * Author: liuchaojie
 */

#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/po_metadata.h>
#include <asm-generic/io.h>
#include <uapi/asm-generic/errno.h>
#include <uapi/asm-generic/errno-base.h>

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

	rcd = kpmalloc(sizeof(*rcd), GFP_KERNEL);
	rcd->desc = NULL;
	return rcd;
}

struct po_ns_record *po_ns_insert(const char *str, int strlen)
{

}

struct po_ns_record *po_ns_delete(const char *str, int strlen)
{

}

/*
 * For now, we didn't consider the parameter of mode.
 */
SYSCALL_DEFINE2(po_creat, const char __user *, poname, umode_t, mode)
{
	char *kponame;
	int len, i;
	struct po_ns_record *rcd;
	struct po_desc *desc;

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

	desc = kpmalloc(sizeof(*desc), GFP_KERNEL);
	desc->size = 0;
	desc->data_pa = NULL;
	desc->d_uid = current_uid();
	desc->d_gid = current_gid();
	desc->mode = mode;

	rcd = po_ns_insert(kponame, len);
	rcd->desc = (struct po_desc *)virt_to_phys(desc);

	return 0;
}
