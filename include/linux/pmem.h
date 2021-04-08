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
extern int get_virtual_memory_sections_number(void);

/* /sys/kernel/mm/virtual_memory/ */
/* 0 means the is size of virtual memory is not limited */
#define DEFAULT_MAX_VM_SIZE	(0UL)
#define VMSTATE_NAME_LEN 32
struct vmstate {
	unsigned long max_vm_size;
	unsigned long vm_size;
	char name[VMSTATE_NAME_LEN];
};

extern struct vmstate vmstate;



#endif

#endif
