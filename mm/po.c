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
#include <linux/string.h>


/*
 * protect syscalls of persistent object.
 */
DEFINE_SPINLOCK(po_lock);

void po_free_chunk(struct po_chunk *chunk);
void po_free_nc_chunk(struct po_chunk *nc_map_metadata);

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
	unsigned long flags;

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

	spin_lock_irqsave(&po_lock, flags);
	rcd = po_ns_insert(kponame, len);
	if (rcd == NULL) {
		spin_unlock_irqrestore(&po_lock, flags);
		return -EEXIST;
	}

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
		spin_unlock_irqrestore(&po_lock, flags);
		return retval;
	}

	kfree(kponame);
	spin_unlock_irqrestore(&po_lock, flags);
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
	unsigned long flags;

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

	spin_lock_irqsave(&po_lock, flags);
	/* check whether po is opened(busy) */
	rcd = po_ns_search(kponame, len);
	if (rcd == NULL) {
		kfree(kponame);
		spin_unlock_irqrestore(&po_lock, flags);
		return -ENOENT;
	}
	desc = (struct po_desc *)phys_to_virt(rcd->desc);
	if (pos_is_open(desc) == true) {
		kfree(kponame);
		spin_unlock_irqrestore(&po_lock, flags);
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
	spin_unlock_irqrestore(&po_lock, flags);
	return 0;
}

SYSCALL_DEFINE3(po_open, const char __user *, poname, int, flags, umode_t, mode)
{
	char *kponame;
	int len, i;
	struct po_ns_record *rcd;
	struct po_desc *desc;
	int retval;
	unsigned long irq_flags;

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

	spin_lock_irqsave(&po_lock, irq_flags);
	rcd = po_ns_search(kponame, len);
	if (rcd == NULL) {
		if (flags & O_CREAT) {
			rcd = po_ns_insert(kponame, len);
			if (rcd == NULL) {
				kfree(kponame);
				spin_unlock_irqrestore(&po_lock, irq_flags);
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
			spin_unlock_irqrestore(&po_lock, irq_flags);
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
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return retval;
	}

	kfree(kponame);
	spin_unlock_irqrestore(&po_lock, irq_flags);
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
	int retval;
	unsigned long flags;

	spin_lock_irqsave(&po_lock, flags);
	retval = pos_delete(pod);
	spin_unlock_irqrestore(&po_lock, flags);

	return retval;
}

SYSCALL_DEFINE4(po_chunk_mmap, unsigned long, pod, unsigned long, addr, \
	unsigned long, prot, unsigned long, flags)
{
	struct po_desc *desc;
	struct po_chunk *chunk, *chunk_pa;
	unsigned long retval;
	unsigned long irq_flags;

	spin_lock_irqsave(&po_lock, irq_flags);
	desc = pos_get(pod);

	if (!desc) {
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return -EBADF;
	}
	/* check prot, just support PROT_NONE, PROT_READ and PROT_WRITE */
	if (prot != PROT_NONE \
		&& prot != PROT_READ && prot != PROT_WRITE \
		&& (prot != (PROT_READ | PROT_WRITE))) {
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return -EINVAL;
	}
	if (prot & PROT_READ)
		if ((!(desc->flags & O_RDONLY)) && (!(desc->flags & O_RDWR))) {
			spin_unlock_irqrestore(&po_lock, irq_flags);
			return -EINVAL;
		}
	if (prot & PROT_WRITE)
		if ((!(desc->flags & O_WRONLY)) && (!(desc->flags & O_RDWR))) {
			spin_unlock_irqrestore(&po_lock, irq_flags);
			return -EINVAL;
		}
	/* check flags, just support MAP_PRIVATE, MAP_ANONYMOUS and MAP_HUGETLB*/
	if ((flags | MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB) != \
		(MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB)) {
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return -EINVAL;
	}
	if ((flags & MAP_ANONYMOUS) && (pod != -1)) {
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return -EINVAL;
	}

	chunk_pa = desc->data_pa;
	while (chunk_pa != NULL) {
		chunk = (struct po_chunk *)phys_to_virt(chunk_pa);
		if (addr == get_chunk_map_start(chunk))
			break;
		chunk_pa = chunk->next_pa;
	}
	if (!chunk_pa) {
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return -EINVAL;
	}
	retval = po_prepare_map_chunk(chunk, prot, flags | MAP_USE_PM);
	spin_unlock_irqrestore(&po_lock, irq_flags);
	return retval;
}

/*
 * it is like po_shrink, just not freeing actuall pages.
 */
SYSCALL_DEFINE1(po_chunk_munmap, unsigned long, addr)
{
	unsigned long retval;
	unsigned long flags;

	if ((PAGE_SIZE_REDEFINED - 1) & addr)
		return -EINVAL;
	if (!(addr >= PO_MAP_AREA_START && addr < PO_MAP_AREA_END))
		return -EINVAL;

	spin_lock_irqsave(&po_lock, flags);
	retval = po_unmap_chunk(addr);
	spin_unlock_irqrestore(&po_lock, flags);

	return retval;
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
	unsigned long align_size;
	long retval = 0;
	unsigned long irq_flags;

	/* check pod */
	spin_lock_irqsave(&po_lock, irq_flags);
	desc = pos_get(pod);
	if (!desc) {
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return -EBADF;
	}
	/* check len */
	if ((len <= 0) || ((PAGE_SIZE_REDEFINED - 1) & len)) {
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return -EINVAL;
	}
	/* check prot, just support PROT_NONE, PROT_READ and PROT_WRITE */
	if (prot != PROT_NONE \
		&& prot != PROT_READ && prot != PROT_WRITE \
		&& (prot != (PROT_READ | PROT_WRITE))) {
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return -EINVAL;
	}
	if (prot & PROT_READ)
		if ((!(desc->flags & O_RDONLY)) && (!(desc->flags & O_RDWR))) {
			spin_unlock_irqrestore(&po_lock, irq_flags);
			return -EINVAL;
		}
	if (prot & PROT_WRITE)
		if ((!(desc->flags & O_WRONLY)) && (!(desc->flags & O_RDWR))) {
			spin_unlock_irqrestore(&po_lock, irq_flags);
			return -EINVAL;
		}
	/* check flags, just support MAP_PRIVATE, MAP_ANONYMOUS and MAP_HUGETLB*/
	if ((flags | MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_NUMA_AWARE) != \
		(MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_NUMA_AWARE)) {
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return -EINVAL;
	}

	new_chunk = (struct po_chunk *)kpmalloc(sizeof(*new_chunk), GFP_KERNEL);
	if (!new_chunk) {
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return -ENOMEM;
	}

	if (len > MAX_BUDDY_ALLOC_SIZE) {
		align_size = 0;
		/*
		 * for now, the max continuous physical memory is less than 1GB.
		 * so, we use 2MB directly.
		 */
		if (flags & MAP_HUGETLB) {
			align_size = 1UL << PMD_SHIFT;
		}
		po_vma = po_vma_alloc(len, align_size);
		if (!po_vma) {
			spin_unlock_irqrestore(&po_lock, irq_flags);
			return -ENOMEM;
		}
		nc_map_metadata = (struct po_chunk *)kpmalloc(sizeof(*new_chunk), GFP_KERNEL);
		if (!nc_map_metadata) {
			spin_unlock_irqrestore(&po_lock, irq_flags);
			return -ENOMEM;
		}
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
				po_alloc_pt_pages_zeroed(alloc_size, GPFP_KERNEL | ((flags&MAP_NUMA_AWARE) ? ___GPFP_NUMA_AWARE : 0x0)) : \
				po_alloc_pt_pages(alloc_size, GPFP_KERNEL | ((flags&MAP_NUMA_AWARE) ? ___GPFP_NUMA_AWARE : 0x0));
			if (!v_start) {
				spin_unlock_irqrestore(&po_lock, irq_flags);
				return -ENOMEM;
			}
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
			po_alloc_pt_pages_zeroed(alloc_size, GPFP_KERNEL | ((flags&MAP_NUMA_AWARE) ? ___GPFP_NUMA_AWARE : 0x0)) : \
			po_alloc_pt_pages(alloc_size, GPFP_KERNEL | ((flags&MAP_NUMA_AWARE) ? ___GPFP_NUMA_AWARE : 0x0));
		if (!v_start) {
			kpfree(new_chunk);
			spin_unlock_irqrestore(&po_lock, irq_flags);
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
	if (retval < 0) {
		spin_unlock_irqrestore(&po_lock, irq_flags);
		return retval;
	}
	desc->size += len;
	spin_unlock_irqrestore(&po_lock, irq_flags);
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
	unsigned long flags;

	spin_lock_irqsave(&po_lock, flags);
	desc = pos_get(pod);

	if (!desc) {
		spin_unlock_irqrestore(&po_lock, flags);
		return -EBADF;
	}
	/* check addr and len */
	if ((addr <= 0) || ((PAGE_SIZE_REDEFINED - 1) & addr)) {
		spin_unlock_irqrestore(&po_lock, flags);
		return -EINVAL;
	}
	if ((len <= 0) || ((PAGE_SIZE_REDEFINED - 1) & len)) {
		spin_unlock_irqrestore(&po_lock, flags);
		return -EINVAL;
	}

	if (desc->data_pa == NULL) {
		spin_unlock_irqrestore(&po_lock, flags);
		return 0;
	}

	curr = (struct po_chunk *)phys_to_virt(desc->data_pa);
	start = get_chunk_map_start(curr);
	if (addr == start) {
		desc->data_pa = curr->next_pa;
		if (desc->data_pa == NULL)
			desc->tail_pa = NULL;
		goto unmap_and_free_chunk;
	}

	prev = curr;
	curr = prev->next_pa == NULL ? NULL : phys_to_virt(prev->next_pa);
	while (curr != NULL) {
		start = get_chunk_map_start(curr);
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
	if (curr == NULL) {
		spin_unlock_irqrestore(&po_lock, flags);
		return 0;
	}

unmap_and_free_chunk:
	ret = po_unmap_chunk(addr);
	if (ret < 0) {
		spin_unlock_irqrestore(&po_lock, flags);
		return ret;
	}
	po_free_chunk(curr);
	desc->size -= curr->size;
	spin_unlock_irqrestore(&po_lock, flags);
	return 0;
}

SYSCALL_DEFINE2(po_stat, const char __user *, poname, struct po_stat __user *, statbuf)
{
	char *kponame;
	unsigned long len, i;
	struct po_ns_record *rcd;
	struct po_desc *desc;
	struct po_stat *tmp;
	unsigned long flags;

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

	spin_lock_irqsave(&po_lock, flags);
	rcd = po_ns_search(kponame, len);
	kfree(kponame);
	if (rcd == NULL) {
		spin_unlock_irqrestore(&po_lock, flags);
		return -ENOENT;
	}
	desc = (struct po_desc *)phys_to_virt(rcd->desc);
	tmp = kmalloc(sizeof(struct po_stat), GFP_KERNEL);
	if (!tmp) {
		spin_unlock_irqrestore(&po_lock, flags);
		return -ENOMEM;
	}
	tmp->st_mode = desc->mode;
	tmp->st_uid = desc->uid;
	tmp->st_gid = desc->gid;
	tmp->st_size = desc->size;
	copy_to_user(statbuf, tmp, sizeof(struct po_stat));
	kfree(tmp);
	spin_unlock_irqrestore(&po_lock, flags);
	return 0;
}

SYSCALL_DEFINE2(po_fstat, unsigned long, pod, struct po_stat __user *, statbuf)
{
	struct po_desc *desc;
	struct po_stat *tmp;
	unsigned long flags;

	/* check statbuf */
	if (!access_ok(VERIFY_WRITE, statbuf, sizeof(struct po_stat)))
		return -EFAULT;

	spin_lock_irqsave(&po_lock, flags);
	desc = pos_get(pod);
	if (!desc) {
		spin_unlock_irqrestore(&po_lock, flags);
		return -EBADF;
	}
	tmp = kmalloc(sizeof(struct po_stat), GFP_KERNEL);
	if (!tmp) {
		spin_unlock_irqrestore(&po_lock, flags);
		return -ENOMEM;
	}
	tmp->st_mode = desc->mode;
	tmp->st_uid = desc->uid;
	tmp->st_gid = desc->gid;
	tmp->st_size = desc->size;
	copy_to_user(statbuf, tmp, sizeof(struct po_stat));
	kfree(tmp);
	spin_unlock_irqrestore(&po_lock, flags);
	return 0;
}

SYSCALL_DEFINE4(po_chunk_next, unsigned long, pod, unsigned long, last, \
	size_t, size, unsigned long __user *, addrbuf)
{
	struct po_desc *desc;
	unsigned long *tmp;
	struct po_chunk *chunk, *chunk_pa;
	int cnt;
	unsigned long flags;

	/* check addrbuf */
	if (!access_ok(VERIFY_WRITE, addrbuf, sizeof(unsigned long)*size))
		return -EFAULT;
	spin_lock_irqsave(&po_lock, flags);
	desc = pos_get(pod);
	if (!desc) {
		spin_unlock_irqrestore(&po_lock, flags);
		return -EBADF;
	}
	tmp = kmalloc(sizeof(unsigned long)*size, GFP_KERNEL);
	if (!tmp) {
		spin_unlock_irqrestore(&po_lock, flags);
		return -ENOMEM;
	}
	memset(tmp, 0, sizeof(unsigned long)*size);

	chunk_pa = desc->data_pa;
	while (chunk_pa != NULL) {
		chunk = (struct po_chunk *)phys_to_virt(chunk_pa);
                if (last == NULL || last == get_chunk_map_start(chunk)) {
			if (last != NULL)
				chunk_pa = chunk->next_pa;
			cnt = 0;
			while (chunk_pa && cnt < size) {
				chunk = (struct po_chunk *)phys_to_virt(chunk_pa);
				tmp[cnt] = get_chunk_map_start(chunk);
				cnt++;
				chunk_pa = chunk->next_pa;
			}
			break;
		}
		chunk_pa = chunk->next_pa;
	}
	copy_to_user(addrbuf, tmp, sizeof(unsigned long)*size);
	kfree(tmp);
	spin_unlock_irqrestore(&po_lock, flags);
	return 0;
}

long get_chunk_size(struct po_chunk *chunk)
{
	return IS_NC_MAP(chunk->start) ? \
		((struct po_vma *)phys_to_virt(chunk->size))->size : \
		chunk->size;
}

/* return the chunk's mapping addr */
long get_chunk_map_start(struct po_chunk *chunk)
{
	return IS_NC_MAP(chunk->start) ? \
		GET_NC_MAP(chunk->start) : \
		(chunk->start + PO_MAP_AREA_START);
}
