#ifndef _LINUX_PMEM_H
#define _LINUX_PMEM_H

#include <linux/types.h>
#include <linux/mmzone.h>
#include <linux/nodemask.h>

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
#endif

#endif
