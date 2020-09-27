#ifndef _LINUX_PMEM_H
#define _LINUX_PMEM_H

#include <linux/types.h>
#include <linux/list.h>

#ifdef CONFIG_ZONE_PM_EMU

struct pmeminfo {
	__kernel_ulong_t totalpmem;  /* Total usable persistent memory */
	__kernel_ulong_t freepmem;   /* Available persistent memory */
};

struct virtual_memory_sections {
	struct list_head list;
	int 		 number;
};

struct virtual_memory_section {
	struct list_head list;
	unsigned long long start;
};

extern unsigned long global_pm_zone_free_pages(void);
extern void extend_memory_with_pmem(void);
extern void release_memory_to_pmem(void);

#endif

#endif
