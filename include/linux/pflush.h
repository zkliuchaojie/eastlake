#ifndef _LINUX_PFLUSH_H
#define _LINUX_PFLUSH_H

#include <linux/types.h>
#include <asm/special_insns.h>

#define FLUSH_ALIGN ((uintptr_t)64)

#define _mm_sfence() asm volatile("sfence" ::: "memory")

static inline void flush_clwb(const void* addr, size_t len)
{
	uintptr_t uptr;

	/*
	 * Loop through cache-line-size (typically 64B) aligned chunks
	 * covering the given range.
	 */
	for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
		uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN) {
        clwb((void *)uptr);
	}
}

#endif