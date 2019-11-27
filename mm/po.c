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
#include <asm/tlbflush.h>
/*
 * This micro should be defined in linux/po_metadata.h.
 * Including the trailing NUL, the max length of po is 256.
 */
#ifndef MAX_PO_NAME_LENGTH
#define MAX_PO_NAME_LENGTH	256
#endif

#ifdef CONFIG_ZONE_PM_EMU

#define pt_page_to_virt(page)	phys_to_virt(pt_page_to_pfn(page)<<PAGE_SHIFT_REDEFINED)
#define pt_page_to_phys(page)	(pt_page_to_pfn(page)<<PAGE_SHIFT_REDEFINED)
#define virt_to_pt_page(p)	pfn_to_pt_page((virt_to_phys(p)>>PAGE_SHIFT_REDEFINED))
#define phys_to_pt_page(phys)	pfn_to_pt_page(phys>>PAGE_SHIFT_REDEFINED)

/*
 * alloc_pt_pages/free_pt_pages is used to alloc/free AEP space,
 * and the return value is a virt address of the starting memory space.
 * Difference with kpmalloc, alloc_pt_pages just alloc space for the
 * data of persistent object.
 */
void *po_alloc_pt_pages(size_t size, gpfp_t flags)
{
	struct pt_page *page;
	unsigned int order;
	unsigned long tmp;

	size = ALIGN(size, PAGE_SIZE_REDEFINED);
	tmp = 1 * PAGE_SIZE_REDEFINED;
	order = 0;
	for (; tmp < size; order++)
		tmp *= 2;
	page = alloc_pt_pages_node(0, flags, order);
	if (page == NULL)
		return NULL;
	return (void*)pt_page_to_virt(page);
}

/*
 * there should be a number, making pow(2, number) equal size.
 */
void po_free_pt_pages(void *p, size_t size)
{
	unsigned int order;
	unsigned long tmp;

	tmp = 1 * PAGE_SIZE_REDEFINED;
	order = 0;
	for (; tmp < size; order++)
		tmp *= 2;
	__free_pt_pages(virt_to_pt_page(p), order);
}

/*
 * for now, kpmalloc/kpfree is used to alloc/free AEP space not bigger than 4KB.
 */
void *kpmalloc(size_t size, gpfp_t flags)
{
	struct pt_page *page;

	if (size > PAGE_SIZE_REDEFINED)
		return NULL;
	page = alloc_pt_pages_node(0, flags, 0);
	if (page == NULL)
		return NULL;
	pr_info("page: %#lx", (unsigned long)page);
	pr_info("pt_page_to_pfn: %lld", pt_page_to_pfn(page));
	return (void*)pt_page_to_virt(page);
}

void kpfree(void *objp)
{
	__free_pt_pages(virt_to_pt_page(objp), 0);
}

#else

/*
 * We define kpmalloc(kpfree) as kmalloc(kfree), and it is
 * used to allocate space from persistent memory.
 * In the future, someone(yes, someone) will implement them.
 */
#define kpmalloc	kmalloc
#define kpfree		kfree

#endif // CONFIG_ZONE_PM_EMU

/*
 * Syscalls about persistent object depend on metadata module,
 * which provides three interfaces: po_ns_search, po_ns_insert
 * and po_ns_delete. To test these syscalls independently, we
 * implement the above three interface with a simple array and
 * assume that the lenght of poname is 1 and all ponames consists
 * of character.
 */

/*
struct po_ns_record *rcds['z' - 'a' + 1] = {NULL};
struct po_ns_record *po_ns_search(const char *str, int strlen)
{
	struct po_ns_record *rcd;

	if (rcds[*str - 'a'] == NULL)
		return NULL;
	rcd = rcds[*str - 'a'];
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
*/

void free_chunk(struct po_chunk *chunk)
{
	/*
	 * free pages.
	 */
#ifdef CONFIG_ZONE_PM_EMU
	po_free_pt_pages(phys_to_virt(chunk->start), chunk->size);
#else
	kpfree(phys_to_virt(chunk->start));
#endif
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

	pr_info("po creat: %s", poname);
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

	pr_info("rcd->desc: %#lx", rcd->desc);

	retval = pos_insert(desc);
	if (retval < 0) {
		pr_info("retval less than 0");
		po_ns_delete(kponame, len);
		kfree(kponame);
		kpfree(desc);
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
	unsigned long cnt;
	unsigned long address;
	struct mm_struct *mm = current->mm;
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep;

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
	pr_info("rcd: %p", rcd);
	if (rcd == NULL) {
		kfree(kponame);
		return -ENOENT;
	}
	pr_info("rcd->desc: %p", rcd->desc);
	desc = (struct po_desc *)phys_to_virt(rcd->desc);
	pr_info("desc: %p", desc);
	if (pos_is_open(desc) == true) {
		kfree(kponame);
		return -EBUSY;
	}
	pr_info("pos is open");
	if (desc->size != 0) {
		pr_info("desc->data_pa: %p", desc->data_pa);
		curr = (struct po_chunk *)phys_to_virt(desc->data_pa);
		pr_info("curr: %p", curr);
		while (curr != NULL) {
			cnt = 0;
			while (cnt < curr->size) {
				address = curr->start + cnt + PO_MAP_AREA_START;
				cnt += PAGE_SIZE_REDEFINED;

				pgd = pgd_offset(mm, address);
				if (pgd_none_or_clear_bad(pgd))
					continue;
				pr_info("pgd");
				p4d = p4d_offset(pgd, address);
				if (p4d_none_or_clear_bad(p4d))
					continue;
				pr_info("p4d");
				pud = pud_offset(p4d, address);
				if (pud_none_or_clear_bad(pud))
					continue;
				pr_info("pud");
				pmd = pmd_offset(pud, address);
				if (pmd_none_or_clear_bad(pmd))
					continue;
				pr_info("pmd");
				ptep = pte_offset_map(pmd, address);
				if (pte_none(*ptep))
					continue;
				pr_info("pte");
				pte_clear(mm, address, ptep);
			}
			/* the following line does not work, I do not know why */
			//flush_tlb_mm_range(mm, addr, addr + curr->size, 0);
			flush_tlb_mm(mm);
			next = curr->next_pa == NULL ? NULL : \
				(struct po_chunk *)phys_to_virt(curr->next_pa);
			free_chunk(curr);
			curr = next;
		}
	}

	po_ns_delete(kponame, len);
	kpfree(rcd);
	kfree(kponame);
	pr_info("after curr:");
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
	pr_info("rcd: %#lx", rcd);
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
	} else {
		desc = (struct po_desc *)phys_to_virt(rcd->desc);
		pr_info("phys desc: %#lx, desc: %#lx", rcd->desc, desc);
		desc->flags = flags;
		pr_info("nothing");
	}

	retval = pos_insert(desc);
	if (retval < 0) {
		po_ns_delete(kponame, len);
		kfree(kponame);
		kpfree(desc);
		return retval;
	}

	kfree(kponame);
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
	pr_info("desc->size: %d", desc->size);
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
 * For now, we define po_munmap as NOP.
 */
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

/*
 * There are a lot of redundant codes, please
 * refactor it in the furute.
 */
SYSCALL_DEFINE4(po_extend, unsigned long, pod, size_t, len, \
	unsigned long, prot, unsigned long, flags)
{
	struct po_chunk *new_chunk, *curr;
	struct po_desc *desc;
	unsigned long v_start;
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

	/* check pod */
	desc = pos_get(pod);
	if (!desc)
		return -EBADF;
	/* check len */
	if ((len <= 0) || ((PAGE_SIZE_REDEFINED - 1) & len))
		return -EINVAL;
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
	/* check flags, just support MAP_PRIVATE for now */
	if (flags != MAP_PRIVATE)
		return -EINVAL;

	new_chunk = (struct po_chunk *)kpmalloc(sizeof(*new_chunk), GFP_KERNEL);
	if (!new_chunk)
		return -ENOMEM;
#ifdef CONFIG_ZONE_PM_EMU
	v_start = po_alloc_pt_pages(len, GPFP_KERNEL);
#else
	v_start = kpmalloc(len, GFP_KERNEL);
#endif
	if (!v_start) {
		kpfree(new_chunk);
		return -ENOMEM;
	}
	pr_info("po_extend: ");
	new_chunk->start = virt_to_phys(v_start);
	new_chunk->size = len;
	new_chunk->next_pa = NULL;
	if (desc->data_pa == NULL) {
		desc->data_pa = (struct po_chunk *)virt_to_phys(new_chunk);
	} else {
		curr = (struct po_chunk *)phys_to_virt(desc->data_pa);
		while (curr->next_pa != NULL)
			curr = (struct po_chunk *)phys_to_virt(curr->next_pa);
		curr->next_pa = (struct po_chunk *)virt_to_phys(new_chunk);
	}

	/* mmap new_chunk */
	vm_flags |= calc_vm_prot_bits(prot, 0) | calc_vm_flag_bits(flags) |
		mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC;
	pgprot = vm_get_page_prot(vm_flags);
	/*
	 * For now, we do not consider huge page: 2MB and 1GB.
	 * And we just assume that there are 4-level page tables.
	 */
	cnt = 0;
	while (cnt < len) {
		address = new_chunk->start + cnt + PO_MAP_AREA_START;
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
		entry = pfn_pte((new_chunk->start+cnt)>>PAGE_SHIFT_REDEFINED, pgprot);
		if (prot & PROT_WRITE)
			entry = pte_mkwrite(entry);
		ptep = pte_offset_map(pmd, address);
		set_pte(ptep, entry);

		cnt += PAGE_SIZE_REDEFINED;
	}
	desc->size += len;
	return retval;
}
/*
 * for now, ignore len, and addr must be a virtual address of a chunk.
 */
SYSCALL_DEFINE3(po_shrink, unsigned long, pod, unsigned long, addr, size_t, len)
{
	struct po_desc *desc;
	struct po_chunk *prev, *curr;
	unsigned long cnt;
	unsigned long address;
	struct mm_struct *mm = current->mm;
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep;

	pr_info("po_shrink");
	desc = pos_get(pod);
	if (!desc)
		return -EBADF;
	/* check addr and len */
	if ((addr <= 0) || ((PAGE_SIZE_REDEFINED - 1) & addr))
		return -EINVAL;
	if ((len <= 0) || ((PAGE_SIZE_REDEFINED - 1) & len))
		return -EINVAL;

	if (desc->data_pa == NULL)
		return 0;
	pr_info("desc->data_pa is not NULL");
	curr = (struct po_chunk *)phys_to_virt(desc->data_pa);
	if (addr == curr->start + PO_MAP_AREA_START) {
		desc->data_pa = curr->next_pa;
		pr_info("first chunk");
		goto unmap_and_free_chunk;
	}

	prev = curr;
	curr = prev->next_pa == NULL ? NULL : phys_to_virt(prev->next_pa);
	while (curr != NULL) {
		if (addr == curr->start + PO_MAP_AREA_START) {
			prev->next_pa = curr->next_pa;
			goto unmap_and_free_chunk;
		}
		prev = curr;
		curr = prev->next_pa == NULL ? NULL : phys_to_virt(prev->next_pa);
	}
	if (curr == NULL)
		return 0;

unmap_and_free_chunk:
	cnt = 0;
	while (cnt < curr->size) {
		address = curr->start + cnt + PO_MAP_AREA_START;
		cnt += PAGE_SIZE_REDEFINED;

		pgd = pgd_offset(mm, address);
		if (pgd_none_or_clear_bad(pgd))
			continue;
		pr_info("pgd");
		p4d = p4d_offset(pgd, address);
		if (p4d_none_or_clear_bad(p4d))
			continue;
		pr_info("p4d");
		pud = pud_offset(p4d, address);
		if (pud_none_or_clear_bad(pud))
			continue;
		pr_info("pud");
		pmd = pmd_offset(pud, address);
		if (pmd_none_or_clear_bad(pmd))
			continue;
		pr_info("pmd");
		ptep = pte_offset_map(pmd, address);
		if (pte_none(*ptep))
			continue;
		pr_info("pte");
		pte_clear(mm, address, ptep);
	}
	/* the following line does not work, I do not know why */
	//flush_tlb_mm_range(mm, addr, addr + curr->size, 0);
	flush_tlb_mm(mm);
	desc->size -= curr->size;
	free_chunk(curr);
	return 0;
}

SYSCALL_DEFINE2(po_stat, const char __user *, poname, struct po_stat __user *, statbuf)
{
	char *kponame;
	unsigned long len, i;
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

	rcd = po_ns_search(kponame, len);
	if (rcd == NULL)
		return -ENOENT;
	desc = (struct po_desc *)phys_to_virt(rcd->desc);
	statbuf->st_mode = desc->mode;
	statbuf->st_uid = desc->uid;
	statbuf->st_gid = desc->gid;
	statbuf->st_size = desc->size;
	return 0;
}

SYSCALL_DEFINE2(po_fstat, unsigned long, pod, struct po_stat __user *, statbuf)
{
	struct po_desc *desc;

	desc = pos_get(pod);
	if (!desc)
		return -EBADF;
	statbuf->st_mode = desc->mode;
	statbuf->st_uid = desc->uid;
	statbuf->st_gid = desc->gid;
	statbuf->st_size = desc->size;
	return 0;
}
