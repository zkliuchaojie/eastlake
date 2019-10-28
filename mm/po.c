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
#include <linux/po_map.h>
#include <uapi/asm-generic/mman-common.h>
#include <uapi/asm-generic/fcntl.h>
#include <linux/mman.h>

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
 * implement the above three interface with a simple array and
 * assume that the lenght of poname is 1 and all ponames consists
 * of character.
 */
struct po_ns_record *rcds['z' - 'a' + 1] = {NULL};

struct po_ns_record *po_ns_search(const char *str, int strlen)
{
	struct po_ns_record *rcd;
	struct po_desc *desc;
	struct po_chunk *chunk;

	if (rcds[*str - 'a'] == NULL)
		return NULL;
	rcd = rcds[*str - 'a'];
	desc = (struct po_desc *)phys_to_virt(rcd->desc);
	pr_info("po_ns_search, rcd: %p", rcd);
	pr_info("po_ns_search, desc: %p", desc);
	desc->size = PAGE_SIZE_REDEFINED * 2;
	desc->flags = O_RDWR;
	chunk = (struct po_chunk *)kpmalloc(sizeof(*chunk), GFP_KERNEL);
	chunk->size = desc->size;
	chunk->start = virt_to_phys(kpmalloc(chunk->size, GFP_KERNEL));
	chunk->next_pa = NULL;
	desc->data_pa = (struct po_chunk *)virt_to_phys(chunk);

	pr_info("ok");
	return rcd;
}

struct po_ns_record *po_ns_insert(const char *str, int strlen)
{
	struct po_ns_record *rcd;

	if (rcds[*str - 'a'] != NULL)
		return NULL;
	rcd = kpmalloc(sizeof(*rcd), GFP_KERNEL);
	pr_info("po_ns_insert: %p", rcd);
	rcd->desc = NULL;
	rcds[*str - 'a'] = rcd;
	return rcd;
}

struct po_ns_record *po_ns_delete(const char *str, int strlen)
{
	struct po_ns_record *rcd;

	if (rcds[*str - 'a'] == NULL)
		return NULL;
	rcd = rcds[*str - 'a'];
	rcds[*str - 'a'] = NULL;
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
	kpfree(phys_to_virt(chunk->start));
	/* free po_chunk */
	kpfree(chunk);
}

/*
 * Functions about podtable.h
 */

inline int pos_insert(struct po_desc *desc)
{
	int i;
	struct po_desc **po_array;

	po_array = current->pos->po_array;
	for (i = 0; i < NR_OPEN_DEFAULT; i++) {
		if (po_array[i] == NULL) {
			po_array[i] = desc;
			return i;
		}
	}
	return -EMFILE;
}

inline struct po_desc *pos_get(unsigned int pod)
{
	struct po_desc **po_array;

	if (pod < 0 || pod >= NR_OPEN_DEFAULT)
		return NULL;

	po_array = current->pos->po_array;
	if (po_array[pod] == NULL)
		return NULL;
	return po_array[pod];
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

inline bool pos_is_open(struct po_desc *desc)
{
	int i;
	struct po_desc **po_array;

	po_array = current->pos->po_array;
	for (i = 0; i < NR_OPEN_DEFAULT; i++) {
		if (po_array[i] == desc)
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
			kpfree(pos);
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
		kpfree(kponame);
		return len;
	}
	if (len == MAX_PO_NAME_LENGTH) {
		kpfree(kponame);
		return -ENAMETOOLONG;
	}
	for (i = 0; i < len; i++) {
		if (kponame[i] == '/') {
			kpfree(kponame);
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
		kpfree(kponame);
		kpfree(desc);
		return retval;
	}

	kpfree(kponame);
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
		kpfree(kponame);
		return len;
	}
	if (len == MAX_PO_NAME_LENGTH) {
		kpfree(kponame);
		return -ENAMETOOLONG;
	}
	for (i = 0; i < len; i++) {
		if (kponame[i] == '/') {
			kpfree(kponame);
			return -EINVAL;
		}
	}

	/* check whether po is opened(busy) */
	rcd = po_ns_search(kponame, len);
	pr_info("rcd: %p", rcd);
	if (rcd == NULL) {
		kpfree(kponame);
		return -ENOENT;
	}
	pr_info("rcd->desc: %p", rcd->desc);
	desc = (struct po_desc *)phys_to_virt(rcd->desc);
	pr_info("desc: %p", desc);
	if (pos_is_open(desc) == true) {
		kpfree(kponame);
		return -EBUSY;
	}
	if (desc->size != 0) {
		pr_info("desc->data_pa: %p", desc->data_pa);
		curr = (struct po_chunk *)phys_to_virt(desc->data_pa);
		pr_info("curr: %p", curr);
		while (curr != NULL) {
			next = NULL;
			if (curr->next_pa != NULL)
				next = (struct po_chunk *)phys_to_virt(curr->next_pa);
			free_chunk(curr);
			curr = next;
		}
	}

	po_ns_delete(kponame, len);
	kpfree(rcd);
	kpfree(kponame);
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
		kpfree(kponame);
		return len;
	}
	if (len == MAX_PO_NAME_LENGTH) {
		kpfree(kponame);
		return -ENAMETOOLONG;
	}
	for (i = 0; i < len; i++) {
		if (kponame[i] == '/') {
			kpfree(kponame);
			return -EINVAL;
		}
	}

	rcd = po_ns_search(kponame, len);
	if (rcd == NULL) {
		if (flags & O_CREAT) {
			rcd = po_ns_insert(kponame, len);
			if (rcd == NULL) {
				kpfree(kponame);
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
			kpfree(kponame);
			return -ENOENT;
		}
	} else {
		desc = (struct po_desc *)phys_to_virt(rcd->desc);
		desc->flags = flags;
	}

	retval = pos_insert(desc);
	if (retval < 0) {
		po_ns_delete(kponame, len);
		kpfree(kponame);
		kpfree(desc);
		return retval;
	}

	kpfree(kponame);
	return retval;
}

/*
 * for now, we just free the slot of po_array.
 * Note that, if a po has been modified, we should
 * flush the changes to NVM.
 * TODO: flush modified data.
 */
SYSCALL_DEFINE1(po_close, unsigned int, pod)
{
	return pos_delete(pod);
}

/*
 * Ignore addr parameter.
 * pgoff and len should be aligned
 */
SYSCALL_DEFINE6(po_mmap, unsigned long, addr, unsigned long, len, \
	unsigned long, prot, unsigned long, flags, \
	unsigned long, pod, unsigned long, pgoff)
{
	struct po_desc *desc;
	struct po_chunk *chunk;
	unsigned long pos;
	unsigned long cnt;
	unsigned long address;
	struct mm_struct *mm = current->mm;
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep;
	pte_t entry;
	vm_flags_t vm_flags = 0;
	pgprot_t pgprot;
	long retval = 0;

	desc = pos_get(pod);
	if (!desc)
		return -EBADF;
	pr_info("EBADF");
	/* check len and pgoff */
	if ((pgoff < 0) || ((PAGE_SIZE_REDEFINED - 1) & pgoff) \
		|| (pgoff >= desc->size))
		return -EINVAL;
	pr_info("pgoff");
	if ((len <= 0) || ((PAGE_SIZE_REDEFINED - 1) & len) \
		|| (pgoff + len > desc->size))
		return -EINVAL;
	pr_info("len");
	/* check prot, just support PROT_READ and PROT_WRITE for now*/
	if (prot != PROT_READ && prot != PROT_WRITE \
		&& (prot != (PROT_READ | PROT_WRITE)))
		return -EINVAL;
	if (prot & PROT_READ)
		if ((!(desc->flags & O_RDONLY)) && (!(desc->flags & O_RDWR)))
			return -EINVAL;
	if (prot & PROT_WRITE)
		if ((!(desc->flags & O_WRONLY)) && (!(desc->flags & O_RDWR)))
			return -EINVAL;
	pr_info("prot");
	/* check flags, just support MAP_PRIVATE for now */
	if (flags != MAP_PRIVATE)
		return -EINVAL;
	pr_info("flags");
	chunk = (struct po_chunk *)phys_to_virt(desc->data_pa);
	pos = 0;
	while (chunk != NULL) {
		if (pos + chunk->size > pgoff) {
			pos = pgoff - pos;
			break;
		}
		pos += chunk->size;
		chunk = (struct po_chunk *)phys_to_virt(chunk->next_pa);
	}
	pr_info("pos: %d", pos);

	/*
	 * start to map: chunk, start, len.
	 */

	/*
	 * for more info about vm_flags, please visit:
	 * https://elixir.bootlin.com/linux/v4.19.73/source/mm/mmap.c#L1360
	 */
	vm_flags |= calc_vm_prot_bits(prot, 0) | calc_vm_flag_bits(flags) |
		mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC;
	pgprot = vm_get_page_prot(vm_flags);

	/*
	 * For now, we do not consider huge page: 2MB and 1GB.
	 * And we just assume that there are 4-level page tables.
	 */
	cnt = 0;
	while (cnt < len) {
		address = chunk->start + pos + PO_MAP_AREA_START;
		if (retval == 0)
			retval = address;
		pgd = pgd_offset(mm, address);
		p4d = p4d_alloc(mm, pgd, address);
		if (!p4d)
			return -ENOMEM;
		pud = pud_alloc(mm, p4d, address);
		if (!pud)
			return -ENOMEM;
		pmd = pmd_alloc(mm, pud, address);
		if (!pmd)
			return -ENOMEM;
		if (pte_alloc(mm, pmd, address))
			return -ENOMEM;
		entry = pfn_pte((chunk->start+pos)>>PAGE_SHIFT_REDEFINED, pgprot);
		if (prot & PROT_WRITE)
			entry = pte_mkwrite(entry);
		// check the write flag
		if (entry.pte & 0x2)
			pr_info("we can write");
		else
			pr_info("just can read");
		ptep = pte_offset_map(pmd, address);
		set_pte(ptep, entry);

		cnt += PAGE_SIZE_REDEFINED;
		pos += PAGE_SIZE_REDEFINED;
		if (pos == chunk->size) {
			chunk = (struct po_chunk *)phys_to_virt(chunk->next_pa);
			pos = 0;
		}
	}
	return retval;
}

/*
 * For now, we define po_munmap as NOP */
SYSCALL_DEFINE2(po_munmap, unsigned long, addr, size_t, len)
{
	/* check addr and len */
	if (((PAGE_SIZE_REDEFINED - 1) & addr) \
		|| ((PAGE_SIZE_REDEFINED - 1) & len) \
		|| (len < 0))
		return -EINVAL;
	if (addr < PO_MAP_AREA_START || addr + len > PO_MAP_AREA_END)
		return -EINVAL;
	return 0;
}
