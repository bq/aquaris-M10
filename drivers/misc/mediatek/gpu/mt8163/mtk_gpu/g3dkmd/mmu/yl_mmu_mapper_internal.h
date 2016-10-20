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
/* brief Used inside the module. Mapper is the main object of this module. */

/*
 *
 * Used by yl_mmu_mapper and yl_mmu_mapper_internal
 *
 */

#ifndef _YL_MMU_MAPPER_INTERNAL_H_
#define _YL_MMU_MAPPER_INTERNAL_H_

#include "yl_mmu_common.h"

#include "yl_mmu_mapper.h"

#include "yl_mmu_hw_table.h"
#include "yl_mmu_trunk_internal.h"
#include "yl_mmu_bin_internal.h"

/********************* Private Function Declarations *****************/

/* void release_trunks(struct Trunk *trunk); */

/* struct Trunk *create_trunk(); */

/* void initialize_bin_list(struct BinList *theList); */

YL_BOOL register_table(struct Mapper *m, struct Trunk *t, va_t va, YL_BOOL section_detected, unsigned int flag);

YL_BOOL unregister_table(struct Mapper *m, struct Trunk *t);


/************** small private helper functions *****************/
/************** Really need to think about where to put them and what's the naming convension *************/

static INLINE YL_BOOL _Mapper_is_section_aligned(va_t address) __attribute__ ((always_inline));

/* Use pa_section_lookup & pa_section_lookup_is_valid */
YL_BOOL _Mapper_is_continuous_section(struct Mapper *m, struct PaSectionLookupTable *lookup_table, va_t va);

YL_BOOL _Mapper_contain_continuous_section(struct Mapper *m, struct PaSectionLookupTable *lookup_table,
					   va_t va, size_t occupied_size);






static INLINE YL_BOOL _Mapper_is_section_aligned(va_t address)
{
	return (address & SECTION_OFFSET_MASK) ? 0 : 1;
}
/************** END small private helper functions *****************/

#endif /* _YL_MMU_MAPPER_INTERNAL_H_ */
