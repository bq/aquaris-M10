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

/* file */
/* brief Class definition.  Mapper is the main object of this module. */

#ifndef _YL_MMU_MAPPER_PUBLIC_CLASS_H_
#define _YL_MMU_MAPPER_PUBLIC_CLASS_H_

#include "yl_mmu_common.h"
#include "yl_mmu_bin_public_class.h"
#include "yl_mmu_hw_table_public_class.h"
#include "yl_mmu_rcache_public_class.h"
#include "yl_mmu_section_lookup_public_class.h"
#include "g3d_memory.h"

/* Mapper, the main object for this module. */
struct Mapper {
	/* The base address for our G3dVa */
	g3d_va_t G3d_va_base;	/* Can be 0 */

	/* Page Table : use internally and also read by hw */
	G3dMemory Page_mem;
	struct FirstPageTable *Page_space;	/* give me the continuous physical space to store the page table */

	/* Internal data structure */
	struct FirstMetaTable *Meta_space;
	struct BinList Bin_list;

	/* [Small Packing] Reversed cache. */
	struct Rcache Rcache;

	/* An temp object to reduce the amount of table lookup. */
	struct PaSectionLookupTable Pa_section_lookup_table;

	/* Cache for small page size squeezing */
	int Cache_enable;

	/* Mirror : G3dVa -> G3dVa */
#if MMU_MIRROR
	G3dMemory Mirror_mem;
	struct FirstPageTable *Mirror_space;
#endif

	int8_t is_pa_given;
	va_t GIVEN_PA_va_base;
	pa_t GIVEN_PA_pa_base;

#if MMU_LOOKBACK_VA		/* used for lookback to get va from g3d_va */
	G3dMemory lookback_mem;
	/* void*                   Page_space_lookback0; // from alloc memory and can't be physical memory */
	struct FirstPageTable *Page_space_lookback;	/* pointer to struct FirstPageTable */
#endif

};

#endif /* _YL_MMU_MAPPER_PUBLIC_CLASS_H_ */
