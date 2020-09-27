/*
 * Author: liuchaojie
 */

#include <linux/pmem.h>
#include <linux/syscalls.h>
#include <linux/mmzone.h>
#include <linux/nodemask.h>
#include <linux/gfp.h>
#include <linux/memory_hotplug.h>
#include <asm/sparsemem.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

DEFINE_SPINLOCK(vms_list_lock);
static struct virtual_memory_sections vms_list = {
	.number = -1,
};

/* this syscall is used to debug persistent memory management */
SYSCALL_DEFINE1(debugger, unsigned int, op)
{
	if (op == 0)
		return 0;
	if (op == 1) {
		extend_memory_with_pmem();
	} else if (op == 2) {
		release_memory_to_pmem();
	}

	return op;
}

inline unsigned long global_pm_zone_free_pages(void)
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

inline void extend_memory_with_pmem(void)
{
	struct pt_page *page;
	unsigned long long start, size;
	int result;
	struct virtual_memory_section *vms;

	if (!spin_trylock(&vms_list_lock))
		return;
	/* not initialized */
	if (vms_list.number == -1) {
		INIT_LIST_HEAD(&vms_list.list);
		vms_list.number = 0;
	}

	pr_info("MAX_ORDER - 1: %d", MAX_ORDER - 1);
	pr_info("SECTION_SIZE_BITS: %d", SECTION_SIZE_BITS);

	page = alloc_pt_pages_node(0, GPFP_KERNEL, MAX_ORDER - 1);
	if (page != NULL) {
		start = (pt_page_to_pfn(page)<<12);
		size = (1UL << (MAX_ORDER - 1 + 12));
		pr_info("extend memory with pmem: [mem %#018llx-%#018llx]\n", \
			   start, start + size -1);

		result = add_memory(0, start, size);
		if (result < 0)
			pr_info("extend memory with pmem failed, result: %d", result);

		vms = kmalloc(sizeof(*vms), GFP_KERNEL);
		if (vms == NULL) {
			// TODO: release memory
			spin_unlock(&vms_list_lock);
			return;
		}
		INIT_LIST_HEAD(&vms->list);
		vms->start = start;
		list_add(&vms->list, &vms_list.list);
		vms_list.number++;
	}

	spin_unlock(&vms_list_lock);
}

inline void release_memory_to_pmem(void)
{
	struct virtual_memory_section *vms;

	if (vms_list.number == 0 || !spin_trylock(&vms_list_lock))
		return;
	/* check again */
	if (vms_list.number == 0)
		return;

	pr_info("virtual memory section number: %d", vms_list.number);
	list_for_each_entry(vms, &vms_list.list, list) {
		pr_info("virtual memory section, start: %#018llx", vms->start);
	}

	vms = list_first_entry(&vms_list.list, struct virtual_memory_section, list);
	remove_memory(0, vms->start, 1UL << (MAX_ORDER - 1 + 12));
	list_del(&vms->list);
	kfree(vms);
	vms_list.number--;

	list_for_each_entry(vms, &vms_list.list, list) {
		pr_info("virtual memory section, start: %#018llx", vms->start);
	}

	spin_unlock(&vms_list_lock);
}
