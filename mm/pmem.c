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

SYSCALL_DEFINE0(pmem_init)
{
	int nid, i;
	for_each_online_node(nid) {
		pg_data_t *pgdat = NODE_DATA(nid);
		if (pgdat->nr_pm_zones != 0) {
			for(i = 0; i < MAX_NR_PM_ZONES; i++) {
				struct pm_zone *zone = &pgdat->node_pm_zones[i];
				struct pm_super *super = zone->super;
				super->initialized = false;
				flush_clwb(super, 64);
				_mm_sfence();
			}
		}
	}
	return 0;
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

	page = alloc_pt_pages_node(NUMA_NO_NODE, GPFP_KERNEL, SECTION_SIZE_BITS - PAGE_SHIFT);
	if (page != NULL) {
		start = (pt_page_to_pfn(page)<<12);
		size = (1UL << (SECTION_SIZE_BITS));
		pr_info("extend memory with pmem: [mem %#018llx-%#018llx]\n", \
			   start, start + size -1);

		result = add_memory(0, start, size);
		if (result < 0) {
			pr_info("extend memory with pmem failed, result: %d\n", result);
			__free_pt_pages(page, SECTION_SIZE_BITS - PAGE_SHIFT);
			spin_unlock(&vms_list_lock);
			return;
		}

		vms = kmalloc(sizeof(*vms), GFP_KERNEL);
		if (vms == NULL) {
			pr_info("extend memory with pmem failed, error: %ld\n", (unsigned long)vms);
			remove_memory(0, start, 1UL << (SECTION_SIZE_BITS));
			__free_pt_pages(page, SECTION_SIZE_BITS - PAGE_SHIFT);
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
	remove_memory(0, vms->start, 1UL << (SECTION_SIZE_BITS));
	__free_pt_pages(pfn_to_pt_page(vms->start>>PAGE_SHIFT), SECTION_SIZE_BITS - PAGE_SHIFT);
	list_del(&vms->list);
	kfree(vms);
	vms_list.number--;

	list_for_each_entry(vms, &vms_list.list, list) {
		pr_info("virtual memory section, start: %#018llx", vms->start);
	}

	spin_unlock(&vms_list_lock);
}

/* we do not use lock, just return number is ok */
inline int get_virtual_memory_sections_number(void)
{
	return vms_list.number;
}

/* /sys/kernel/mm/virtual_memory/ */
struct vmstate vmstate;
static struct kobject *vm_kobj;

#define VMSTATE_ATTR_RO(_name) \
	static struct kobj_attribute _name##_attr = __ATTR_RO(_name)

#define VMSTATE_ATTR(_name) \
	static struct kobj_attribute _name##_attr = \
		__ATTR(_name, 0644, _name##_show, _name##_store)

static ssize_t max_vm_size_show(struct kobject *kobj,
				       struct kobj_attribute *attr, char *buf)
{
	unsigned long max_vm_size;

	max_vm_size = vmstate.max_vm_size;
	return sprintf(buf, "%lu\n", max_vm_size);
}

static ssize_t max_vm_size_store(struct kobject *kobj,
	       struct kobj_attribute *attr, const char *buf, size_t len)
{
	unsigned long count;
	int err;

	err = kstrtoul(buf, 10, &count);
	if (err)
		return err;
	vmstate.max_vm_size = count;

	return len;
}
VMSTATE_ATTR(max_vm_size);

static ssize_t vm_size_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	unsigned long vm_size;

	vm_size = vmstate.vm_size;
	return sprintf(buf, "%lu\n", vm_size);
}
VMSTATE_ATTR_RO(vm_size);

static struct attribute *vmstate_attrs[] = {
	&max_vm_size_attr.attr,
	&vm_size_attr.attr,
	NULL,
};

static const struct attribute_group vmstate_attr_group = {
	.attrs = vmstate_attrs,
};

static void __init vm_sysfs_init(void)
{
	int retval;

	vm_kobj = kobject_create_and_add(vmstate.name, mm_kobj);
	if (!vm_kobj)
		return;

	retval = sysfs_create_group(vm_kobj, &vmstate_attr_group);
	if (retval) {
		pr_err("virtual memory: Unable to add vmstate %s", vmstate.name);
		kobject_put(vm_kobj);
	}

	return;
}

static int __init vm_init(void)
{
	vmstate.max_vm_size = DEFAULT_MAX_VM_SIZE;
	vmstate.vm_size = 0;
	snprintf(vmstate.name, VMSTATE_NAME_LEN, "virtual_memory");

	vm_sysfs_init();

	return 0;
}
subsys_initcall(vm_init);