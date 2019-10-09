/*
 * Author: liuchaojie
 */

#include <linux/slab.h>
#include <linux/syscalls.h>
//#include <linux/po_metadata.h>
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
 * For now, we didn't consider the parameter of mode.
 */
SYSCALL_DEFINE2(po_creat, const char __user *, poname, umode_t, mode)
{
	char *kponame;
	int len, i;

	kponame = kmalloc(MAX_PO_NAME_LENGTH, GFP_KERNEL);
	if (!kponame)
		return -ENOMEM;
	len = strncpy_from_user(kponame, poname, MAX_PO_NAME_LENGTH);
	if (len < 0)
		return len;
	if (len == MAX_PO_NAME_LENGTH)
		return -ENAMETOOLONG;
	for (i = 0; i < len; i++) {
		if (kponame[i] == '/')
			return -EINVAL;
	}

	return 0;
}
