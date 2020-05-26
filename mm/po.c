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
#include <linux/mman.h>
#include <uapi/asm-generic/fcntl.h>
#include <linux/mman.h>
#include <asm/tlbflush.h>
#include <linux/pmalloc.h>
#include <linux/mm.h>


void po_free_chunk(struct po_chunk *chunk);
void po_free_nc_chunk(struct po_chunk *nc_map_metadata);

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
	desc->tail_pa = NULL;
	desc->uid = current_uid().val;
	desc->gid = current_gid().val;
	desc->mode = mode;
	desc->flags = O_CREAT|O_RDWR;
	rcd->desc = (struct po_desc *)virt_to_phys(desc);


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

SYSCALL_DEFINE1(po_unlink, const char __user *, poname)
{
	char *kponame;
	int len, i;
	struct po_ns_record *rcd;
	struct po_desc *desc;
	struct po_chunk *curr, *next, *tmp;
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
	if (rcd == NULL) {
		kfree(kponame);
		return -ENOENT;
	}
	desc = (struct po_desc *)phys_to_virt(rcd->desc);
	if (pos_is_open(desc) == true) {
		kfree(kponame);
		return -EBUSY;
	}

	if (desc->size != 0) {
		curr = desc->data_pa;
		while (curr != NULL) {
			curr = phys_to_virt(curr);
			tmp = curr->next_pa;
			po_free_chunk(curr);
			curr = tmp;
		}
	}

	po_ns_delete(kponame, len);
	kpfree(rcd);
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
			desc->tail_pa = NULL;
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
		desc->flags = flags;
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
	unsigned long cnt, chunk_size;
	long retval = 0, tmp;

	desc = pos_get(pod);
	if (!desc)
		return -EBADF;
	/* check len and pgoff */
	if ((pgoff < 0) || ((PAGE_SIZE_REDEFINED - 1) & pgoff) \
		|| (pgoff >= desc->size))
		return -EINVAL;
	if ((len <= 0) || ((PAGE_SIZE_REDEFINED - 1) & len) \
		|| (pgoff + len > desc->size))
		return -EINVAL;
	/* check prot, just support PROT_NONE, PROT_READ and PROT_WRITE */
	if (prot != PROT_NONE \
		&& prot != PROT_READ && prot != PROT_WRITE \
		&& (prot != (PROT_READ | PROT_WRITE)))
		return -EINVAL;
	if (prot & PROT_READ)
		if ((!(desc->flags & O_RDONLY)) && (!(desc->flags & O_RDWR)))
			return -EINVAL;
	if (prot & PROT_WRITE)
		if ((!(desc->flags & O_WRONLY)) && (!(desc->flags & O_RDWR)))
			return -EINVAL;
	/* check flags, just support MAP_PRIVATE and MAP_ANONYMOUS */
	if (flags != MAP_ANONYMOUS && flags != MAP_PRIVATE \
		&& (flags != (MAP_ANONYMOUS | MAP_PRIVATE)))
		return -EINVAL;
	if ((flags & MAP_ANONYMOUS) && (pod != -1 || pgoff != 0))
		return -EINVAL;

	chunk = (struct po_chunk *)phys_to_virt(desc->data_pa);
	pos = 0;
	while (chunk != NULL) {
		chunk_size = get_chunk_size(chunk);
		if (pos + chunk_size > pgoff) {
			break;
		}
		pos += chunk_size;
		chunk = (struct po_chunk *)phys_to_virt(chunk->next_pa);
	}
	cnt = 0;
	while (cnt < len) {
		tmp = po_prepare_map_chunk(chunk, prot, flags | MAP_USE_PM);
		if (tmp < 0)
			return -ENOMEM;
		if (retval == 0)
			retval = tmp;
		chunk_size = get_chunk_size(chunk);
		cnt += chunk_size;
		chunk = phys_to_virt(chunk->next_pa);
	}
	return retval;
}

/*
 * it is like po_shrink, just not freeing actuall pages.
 */
SYSCALL_DEFINE2(po_munmap, unsigned long, addr, size_t, len)
{
	if ((PAGE_SIZE_REDEFINED - 1) & addr)
		return -EINVAL;
	if (!(addr >= PO_MAP_AREA_START && addr < PO_MAP_AREA_END))
		return -EINVAL;
	return po_unmap_chunk(addr);
}

long po_prepare_map_chunk(struct po_chunk *chunk, unsigned long prot, \
	unsigned long flags)
{
	struct po_chunk *nc_map_metadata;
	struct po_vma *vma;
	unsigned long long cnt;

	if (IS_NC_MAP(chunk->start)) {
		nc_map_metadata = phys_to_virt(chunk->size);
		if (!nc_map_metadata->next_pa)
			return 0;
		vma = phys_to_virt(nc_map_metadata->start);
		chunk = phys_to_virt(nc_map_metadata->next_pa);
		cnt = 0;
		do {
			if (po_map_chunk(chunk->start, chunk->size, prot, flags, vma->start+cnt) < 0)
				return 0;
			cnt += chunk->size;
			if (chunk->next_pa == NULL)
				break;
			chunk = phys_to_virt(chunk->next_pa);
		} while (cnt < nc_map_metadata->size);
		return vma->start;
	} else {
		return po_map_chunk(chunk->start, chunk->size, prot, flags, chunk->start+PO_MAP_AREA_START);
	}
}

/*
 * There are a lot of redundant codes, please
 * refactor it in the furute.
 */
SYSCALL_DEFINE4(po_extend, unsigned long, pod, size_t, len, \
	unsigned long, prot, unsigned long, flags)
{
	struct po_chunk *new_chunk, *nc_map_metadata, *prev, *curr;
	struct po_chunk *tail_chunk;
	struct po_vma *po_vma;
	struct po_desc *desc;
	unsigned long v_start;
	unsigned long cnt, alloc_size;
	long retval = 0;

	/* check pod */
	desc = pos_get(pod);
	if (!desc)
		return -EBADF;
	/* check len */
	if ((len <= 0) || ((PAGE_SIZE_REDEFINED - 1) & len))
		return -EINVAL;
	/* check prot, just support PROT_NONE, PROT_READ and PROT_WRITE */
	if (prot != PROT_NONE \
		&& prot != PROT_READ && prot != PROT_WRITE \
		&& (prot != (PROT_READ | PROT_WRITE)))
		return -EINVAL;
	if (prot & PROT_READ)
		if ((!(desc->flags & O_RDONLY)) && (!(desc->flags & O_RDWR)))
			return -EINVAL;
	if (prot & PROT_WRITE)
		if ((!(desc->flags & O_WRONLY)) && (!(desc->flags & O_RDWR)))
			return -EINVAL;
	/* check flags, just support MAP_PRIVATE and MAP_ANONYMOUS */
	if (flags != MAP_ANONYMOUS && flags != MAP_PRIVATE \
		&& (flags != (MAP_ANONYMOUS | MAP_PRIVATE)))
		return -EINVAL;

	new_chunk = (struct po_chunk *)kpmalloc(sizeof(*new_chunk), GFP_KERNEL);
	if (!new_chunk)
		return -ENOMEM;
	if (len > MAX_BUDDY_ALLOC_SIZE) {
		po_vma = po_vma_alloc(len);
		if (!po_vma)
			return -ENOMEM;
		nc_map_metadata = (struct po_chunk *)kpmalloc(sizeof(*new_chunk), GFP_KERNEL);
		if (!nc_map_metadata)
			return -ENOMEM;
		nc_map_metadata->start = virt_to_phys(po_vma);
		nc_map_metadata->size = len;
		nc_map_metadata->next_pa = NULL;
		new_chunk->start = SET_NC_MAP_FLAG(po_vma->start);
		// use the size filed to store nc_map_metadata
		new_chunk->size = virt_to_phys(nc_map_metadata);
		new_chunk->next_pa = NULL;
		/* alloc physical space, and we do this in PG in the future. */
		cnt = 0;
		prev = nc_map_metadata;
		while (cnt < len) {
			alloc_size = (len-cnt > MAX_BUDDY_ALLOC_SIZE) ? \
				MAX_BUDDY_ALLOC_SIZE : len - cnt;
			v_start = (flags & MAP_ANONYMOUS) ? \
				po_alloc_pt_pages_zeroed(alloc_size, GPFP_KERNEL) : \
				po_alloc_pt_pages(alloc_size, GPFP_KERNEL);
			if (!v_start)
				return -ENOMEM;
			curr = (struct po_chunk *)kpmalloc(sizeof(*new_chunk), GFP_KERNEL);
			curr->start = virt_to_phys(v_start);
			curr->size = alloc_size;
			curr->next_pa = NULL;
			prev->next_pa = (struct po_chunk *)virt_to_phys(curr);
			prev = curr;
			cnt += alloc_size;
		}
	} else {
		alloc_size = len;
		v_start = (flags & MAP_ANONYMOUS) ? \
			po_alloc_pt_pages_zeroed(alloc_size, GPFP_KERNEL) : \
			po_alloc_pt_pages(alloc_size, GPFP_KERNEL);
		if (!v_start) {
			kpfree(new_chunk);
			return -ENOMEM;
		}
		new_chunk->start = virt_to_phys(v_start);
		new_chunk->size = len;
		new_chunk->next_pa = NULL;
	}

	if (desc->tail_pa == NULL) {
		desc->data_pa = (struct po_chunk *)virt_to_phys(new_chunk);
		desc->tail_pa = desc->data_pa;
	} else {
		tail_chunk = (struct po_chunk *)phys_to_virt(desc->tail_pa);
		tail_chunk->next_pa = (struct po_chunk *)virt_to_phys(new_chunk);
		desc->tail_pa = tail_chunk->next_pa;
	}

	retval = po_prepare_map_chunk(new_chunk, prot, flags | MAP_USE_PM);
	if (retval < 0)
		return retval;
	desc->size += len;
	return retval;
}

void po_free_chunk(struct po_chunk *chunk)
{
	if (IS_NC_MAP(chunk->start)) {
		po_free_nc_chunk(phys_to_virt(chunk->size));
		/* we should also free nc map metadata */
		kpfree(chunk);
		return;
	}
	/*
	 * free pages.
	 */
	po_free_pt_pages(phys_to_virt(chunk->start), chunk->size);
	/* free po_chunk */
	kpfree(chunk);
}

void po_free_nc_chunk(struct po_chunk *nc_map_metadata)
{
	struct po_chunk *chunk;
	struct po_vma *vma;
	unsigned long long cnt;

	if (nc_map_metadata->next_pa == NULL)
		return;
	vma = phys_to_virt(nc_map_metadata->start);
	chunk = phys_to_virt(nc_map_metadata->next_pa);
	cnt = 0;
	do {
		po_free_chunk(chunk);
		cnt += chunk->size;
		if (chunk->next_pa == NULL)
			break;
		chunk = phys_to_virt(chunk->next_pa);
	} while (cnt < nc_map_metadata->size);
}

/*
 * for now, ignore len, and addr must be a virtual address of a chunk.
 */
SYSCALL_DEFINE3(po_shrink, unsigned long, pod, unsigned long, addr, size_t, len)
{
	struct po_desc *desc;
	struct po_chunk *prev, *curr;
	struct po_chunk *tail_chunk;
	unsigned long start;
	long ret = 0;

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
	curr = (struct po_chunk *)phys_to_virt(desc->data_pa);
	start = IS_NC_MAP(curr->start) ? GET_NC_MAP(curr->start) \
		: curr->start + PO_MAP_AREA_START;
	if (addr == start) {
		desc->data_pa = curr->next_pa;
		if (desc->data_pa == NULL)
			desc->tail_pa = NULL;
		goto unmap_and_free_chunk;
	}

	prev = curr;
	curr = prev->next_pa == NULL ? NULL : phys_to_virt(prev->next_pa);
	while (curr != NULL) {
		start = IS_NC_MAP(curr->start) ? GET_NC_MAP(curr->start) \
			: curr->start + PO_MAP_AREA_START;
		if (addr == start) {
			prev->next_pa = curr->next_pa;
			if (curr == (struct po_chunk *)phys_to_virt(desc->tail_pa)) {
				desc->tail_pa = (struct po_chunk *)virt_to_phys(prev);
			}
			goto unmap_and_free_chunk;
		}
		prev = curr;
		curr = prev->next_pa == NULL ? NULL : phys_to_virt(prev->next_pa);
	}
	if (curr == NULL)
		return 0;

unmap_and_free_chunk:
	ret = po_unmap_chunk(addr);
	if (ret < 0)
		return ret;
	po_free_chunk(curr);
	desc->size -= curr->size;
	return 0;
}

SYSCALL_DEFINE2(po_stat, const char __user *, poname, struct po_stat __user *, statbuf)
{
	char *kponame;
	unsigned long len, i;
	struct po_ns_record *rcd;
	struct po_desc *desc;
	struct po_stat *tmp;

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
	/* check statbuf */
	if (!access_ok(VERIFY_WRITE, statbuf, sizeof(struct po_stat))) {
		kfree(kponame);
		return -EFAULT;
	}

	rcd = po_ns_search(kponame, len);
	kfree(kponame);
	if (rcd == NULL)
		return -ENOENT;
	desc = (struct po_desc *)phys_to_virt(rcd->desc);
	tmp = kmalloc(sizeof(struct po_stat), GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;
	tmp->st_mode = desc->mode;
	tmp->st_uid = desc->uid;
	tmp->st_gid = desc->gid;
	tmp->st_size = desc->size;
	copy_to_user(statbuf, tmp, sizeof(struct po_stat));
	kfree(tmp);
	return 0;
}

SYSCALL_DEFINE2(po_fstat, unsigned long, pod, struct po_stat __user *, statbuf)
{
	struct po_desc *desc;
	struct po_stat *tmp;

	/* check statbuf */
	if (!access_ok(VERIFY_WRITE, statbuf, sizeof(struct po_stat)))
		return -EFAULT;
	desc = pos_get(pod);
	if (!desc)
		return -EBADF;
	tmp = kmalloc(sizeof(struct po_stat), GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;
	tmp->st_mode = desc->mode;
	tmp->st_uid = desc->uid;
	tmp->st_gid = desc->gid;
	tmp->st_size = desc->size;
	copy_to_user(statbuf, tmp, sizeof(struct po_stat));
	kfree(tmp);
	return 0;
}

long get_chunk_size(struct po_chunk *chunk)
{
	return IS_NC_MAP(chunk->start) ? \
		((struct po_vma *)phys_to_virt(chunk->size))->size : \
		chunk->size;
}
