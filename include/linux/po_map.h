/*
 * Do po map.
 * The name and position of this file are not good,
 * and we should refector it in the future.
 * We just support x86_64 architecture.
 */

#ifndef _LINUX_PO_MAP_H
#define _LINUX_PO_MAP_H

/*
 * PAGE_SHIFT is defained in arch/x86/include/asm/page_types.h.
 * Here, we redefine it, and this need to refactor.
 */
#define PAGE_SHIFT_REDEFINED	12
#define PAGE_SIZE_REDEFINED	(1UL << PAGE_SHIFT_REDEFINED)

/*
 * DEFAULT_MAP_WINDOW is defained in arch/x86/include/asm/processor.h.
 * Here, we redefine it, and this need to refactor.
 */
#define DEFAULT_MAP_WINDOW_REDEFINED	((1UL << 47) - (1UL << PAGE_SHIFT_REDEFINED))

#define MAX_GAP			((0x3fffffUL << PAGE_SHIFT_REDEFINED) + \
				(256UL << PAGE_SHIFT_REDEFINED))
#define MAX_RANDOM		(((1UL << 32) - 1) << PAGE_SHIFT_REDEFINED)

#define PO_MAP_AREA_SIZE	0x200000000000	// 32TB
#define PO_MAP_AREA_START	(DEFAULT_MAP_WINDOW_REDEFINED - \
				MAX_GAP - \
				MAX_RANDOM - \
				PO_MAP_AREA_SIZE)
#define PO_MAP_AREA_END		(PO_MAP_AREA_START + PO_MAP_AREA_SIZE)

#endif
