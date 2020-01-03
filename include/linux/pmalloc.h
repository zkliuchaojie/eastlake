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

#ifdef CONFIG_ZONE_PM_EMU

#define pt_page_to_virt(page)	phys_to_virt(pt_page_to_pfn(page)<<PAGE_SHIFT_REDEFINED)
#define pt_page_to_phys(page)	(pt_page_to_pfn(page)<<PAGE_SHIFT_REDEFINED)
#define virt_to_pt_page(p)	pfn_to_pt_page((virt_to_phys(p)>>PAGE_SHIFT_REDEFINED))
#define phys_to_pt_page(phys)	pfn_to_pt_page(phys>>PAGE_SHIFT_REDEFINED)
#define MAX_BUDDY_ALLOC_SIZE	(PAGE_SIZE_REDEFINED << 10)

/*
 * alloc_pt_pages/free_pt_pages is used to alloc/free AEP space,
 * and the return value is a virt address of the starting memory space.
 * Difference with kpmalloc, alloc_pt_pages just alloc space for the
 * data of persistent object.
 */
static void *po_alloc_pt_pages(size_t size, gpfp_t flags)
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
 * get zeroed pages.
 */
static void *po_alloc_pt_pages_zeroed(size_t size, gpfp_t flags)
{
	void *ret;

	ret = po_alloc_pt_pages(size, flags);
	if (ret == NULL)
		return ret;
	memset(ret, 0, size);
	return ret;
}

/*
 * there should be a number, making pow(2, number) equal size.
 */
static void po_free_pt_pages(void *p, size_t size)
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
static void *kpmalloc(size_t size, gpfp_t flags)
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

static void kpfree(void *objp)
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
