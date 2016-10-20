/*
 * Copyright (c) 2014 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _G3DBASE_COMMON_UFO_H_
#define _G3DBASE_COMMON_UFO_H_

#include "yl_define.h"
#include "g3d_memory.h"

#define UFO_LEN_SURFACE_SIZE_ALIGN      128
#define UFO_LEN_SURFACE_SIZE_PADDING    128
#define UFO_LEN_SURFACE_BASE_ALIGN      128

#define UFO_FLAG_ENABLE             (0x1 << 0)
#define UFO_FLAG_NEED_RANGE         (0x1 << 1)
#define UFO_FLAG_DEC_ONLY           (0x1 << 2)
#define UFO_FLAG_FOREVER_DISABLE    (0x1 << 3)


/*
 *  UFO_INFO
 *
 *  This struct should be 64bit aligned. Due to struct UFO_INFO will pass between
 *  32-bit and 64-bit applications. Therefore we need to ensure the sizes of
 *  this structure are the same.
 *
 *  To achieve that, the addresses of all member of this struct should be 64-bit
 *  aligned.
 */
typedef struct _UFO_INFO_ {
	/*
	 * ufo_lc_mem:
	 *   A G3dMemory store the memory information about ufo.
	 *
	 * Force G3dmemory taking and aligning 64 bits space and address:
	 *    Use uint64_t to force be 64-bit variable. Because this struct will be
	 *    used by UFO_INFO and it will pass between 32-bit & 64-bit process at
	 *    runtime. Therefore we need to force all pointer a) 64-bit aligned b)
	 *    64-bit space.
	 */
	union _ufo_lc_mem_ {
		G3dMemory *mem;
		uint64_t pad; /* force ufo_lc_mem take 64 bits. */
	} ufo_lc_mem __aligned(8); /* force 64-bit aligned. */

	unsigned int ufo_clr_color[4];

	unsigned int ufo_flag;
	unsigned int ufoRange;

	G3dStampSlots ufo_stamps[G3D_MAX_NUM_STAMPS]; /* G3dStampSlots is 64bit aligned. */

	unsigned int ufo_range_bak;
	unsigned int ufo_lc_mem_hwaddr;

	unsigned char ufo_lc_mem_1st_create;
	unsigned char ufo_pat_replace_en;
	unsigned char reserved0[6]; /* align 64bit (char * 8.) */
} UFO_INFO;

#ifdef YL_UFO_NOT_DISABLE_AFTER_DECOMPRESS
#define _yl_ufo_calc_length_buffer_size(range_addr, range_size) \
	(((((range_addr + range_size - (range_addr & 0xFFFFC000) + (UFO_LEN_SURFACE_SIZE_ALIGN - 1)) / \
		UFO_LEN_SURFACE_SIZE_ALIGN) + (UFO_LEN_SURFACE_SIZE_PADDING - 1)) & \
		(~(UFO_LEN_SURFACE_SIZE_PADDING - 1))) + 128)

#else
#define _yl_ufo_calc_length_buffer_size(range_addr, range_size) \
	((((range_addr + range_size - (range_addr & 0xFFFFC000) + (UFO_LEN_SURFACE_SIZE_ALIGN - 1)) / \
		UFO_LEN_SURFACE_SIZE_ALIGN) + (UFO_LEN_SURFACE_SIZE_PADDING - 1)) & \
		(~(UFO_LEN_SURFACE_SIZE_PADDING - 1)))

#endif

#endif /* _G3DBASE_COMMON_UFO_H_ */
