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
	desc->uid = current_uid().val;
	desc->gid = current_gid().val;
	desc->mode = mode;

	rcd = po_ns_insert(kponame, len);
	rcd->desc = (struct po_desc *)virt_to_phys(desc);

	return 0;
}

SYSCALL_DEFINE1(po_unlink, const char __user *, poname)
{
	char *kponame;
	int len, i;
	struct po_ns_record *rcd;
	struct po_desc *desc;
	struct po_chunk *curr, *next;

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

	rcd = po_ns_delete(kponame, len);
	if (rcd == NULL)
		return -ENOENT;
	desc = (struct po_desc *)phys_to_virt(rcd->desc);
	if (desc->size != 0) {
		curr = (struct po_chunk *)phys_to_virt(desc->data_pa);
		while (curr != NULL) {
			next = (struct po_chunk *)phys_to_virt(curr->next_pa);
			free_chunk(curr);
			curr = next;
		}
	}

	return 0;
}
