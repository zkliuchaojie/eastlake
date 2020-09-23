#ifndef _LINUX_PMEM_H
#define _LINUX_PMEM_H

#include <linux/types.h>
#include <linux/mmzone.h>
#include <linux/nodemask.h>
#include <linux/gfp.h>
#include <linux/memory_hotplug.h>
#include <asm/sparsemem.h>

#ifdef CONFIG_ZONE_PM_EMU

struct pmeminfo {
	__kernel_ulong_t totalpmem;  /* Total usable persistent memory */
	__kernel_ulong_t freepmem;   /* Available persistent memory */
};

static inline unsigned long global_pm_zone_free_pages(void)
{
	unsigned long free_pages = 0;
	int nid, i;

	for_each_online_node(nid) {
		pg_data_t *pgdat = NODE_DATA(nid);
		if (pgdat->nr_pm_zones != 0) {
			for(i = 0; i < MAX_NR_PM_ZONES; i++) {
				struct pm_zone *zone = &pgdat->node_pm_zones[i];
				struct pm_super *super = zone->super;
				free_pages += super->free;
			}
		}
	}
	return free_pages;
}

static inline void extend_memory_with_pmem(void)
{
	struct pt_page *page;
	unsigned long long start, size;
	int result;

	pr_info("MAX_ORDER - 1: %d", MAX_ORDER - 1);
	pr_info("SECTION_SIZE_BITS: %d", SECTION_SIZE_BITS);


	page = alloc_pt_pages_node(0, GPFP_KERNEL, MAX_ORDER - 1);
	if (page != NULL) {
		start = (pt_page_to_pfn(page)<<12);
		size = (1UL << (MAX_ORDER - 1 + 12));
		pr_info("extend memory with pmem: [mem %#018llx-%#018llx]\n", \
			   start, start + size -1);
		/*
		result = add_memory(0, start, size);
		if (result < 0)
			pr_info("extend memory with pmem failed");
		*/
	}
}

#endif

#endif
