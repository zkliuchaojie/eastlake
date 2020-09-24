#ifndef _LINUX_PMEM_H
#define _LINUX_PMEM_H

#include <linux/types.h>
#include <linux/mmzone.h>
#include <linux/nodemask.h>

#ifdef CONFIG_ZONE_PM_EMU

// #define FLUSH_ALIGN ((uintptr_t)64)

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

// static inline void flush_clwb(const void* addr, size_t len)
// {
// 	uintptr_t uptr;

// 	/*
// 	 * Loop through cache-line-size (typically 64B) aligned chunks
// 	 * covering the given range.
// 	 */
// 	for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
// 		uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN) {
// 		clwb((char *)uptr);
// 	}
// }

// fence
// _mm_mfence() _mm_sfence()...

// ntstore 
// void _mm_stream_si32 (int* mem_addr, int a)
// void _mm_stream_pi (__m64* mem_addr, __m64 a)
// static inline void NTwrite32(void* addr, int a)
// {
// 	_mm_stream_si32(addr, a);
// }

// static inline void NTwrite64(void* addr, uint64_t a) 
// {
// 	_mm_stream_pi(addr, a);
// }

#endif

#endif
