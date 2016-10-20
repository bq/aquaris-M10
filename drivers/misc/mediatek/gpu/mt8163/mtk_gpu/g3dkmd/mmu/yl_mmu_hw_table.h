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
/* brief MMU Table for Hardware reading. */

/* Assumption: int = 32 bits */

/* [TODO] Add second level page table point into FirstPageTable / Entry */

#ifndef _YL_MMU_HW_TABLE_H_
#define _YL_MMU_HW_TABLE_H_

#include "yl_mmu_common.h"
#include "yl_mmu_trunk_public_class.h"
#include "yl_mmu_hw_table_public_class.h"
#include "yl_mmu_mapper_public_class.h"


#if MMU_FOUR_MB

#define G3D_VA_LEVEL0_INDEX_MASK            0xFFC00000UL
#define G3D_VA_LEVEL0_INDEX_START_BIT       22
#define G3D_VA_LEVEL1_INDEX_MASK            0x003FF000UL

#define SECTION_BASE_ADDRESS_MASK           0xFFC00000UL
/* #define SECTION_BASE_ADDRESS_START_BIT      22 */

/* [31:10] in first level entry */
#define SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK    0xFFFFF000UL

#else
/* [31:20] 11111111111100000000000000000000 */
#define G3D_VA_LEVEL0_INDEX_MASK            0xFFF00000UL
#define G3D_VA_LEVEL0_INDEX_START_BIT       20
#define G3D_VA_LEVEL1_INDEX_MASK            0x000FF000UL

#define SECTION_BASE_ADDRESS_MASK           0xFFF00000UL
#define SUPER_SECTION_BASE_ADDRESS_MASK     0xFF000000UL
/* #define SECTION_BASE_ADDRESS_START_BIT      20 */

/* [31:10] in first level entry */
#define SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK    0xFFFFFC00UL

#endif

#define G3D_VA_LEVEL1_INDEX_START_BIT       12

/* #define MMU_SECTION_SIZE                  0x00100000UL */
/* #define MMU_SMALL_SIZE                    0x00001000UL */




/* [31:16] */
#define LARGE_PAGE_LEVEL1_BASE_ADDRESS_MASK   0xFFFF0000UL
/* [31:12] */
#define SMALL_PAGE_LEVEL1_BASE_ADDRESS_MASK   0xFFFFF000UL


/**************** API *****************/
static INLINE size_t PageTable_get_required_size(void) __attribute__ ((always_inline));

static INLINE size_t MetaTable_get_required_size(void) __attribute__ ((always_inline));

/* YL_BOOL PageTable_Initialize(struct FirstPageTable *master, struct FirstMetaTable *metaTable ); */

void PageTable_Release(struct FirstPageTable *master, struct FirstMetaTable *metaTable);



YL_BOOL Section_register(struct Mapper *m, unsigned short index0, pa_t pa, struct Trunk *trunk, va_t va,
			 unsigned int flag);
YL_BOOL Small_register(struct Mapper *m, PageIndexRec index, pa_t pa, struct Trunk *trunk, va_t va,
		       unsigned int flag);
YL_BOOL Section_unregister(struct Mapper *m, unsigned short index0);
YL_BOOL Small_unregister(struct Mapper *m, PageIndexRec index);




static INLINE YL_BOOL SectionEntry_has_second_level(struct FirstPageEntry entry) __attribute__ ((always_inline));

static INLINE YL_BOOL Section_has_second_level(struct FirstPageTable *page0,
	unsigned short index0) __attribute__ ((always_inline));


/**************** Implementation *****************/
static INLINE size_t PageTable_get_required_size(void)
{
	YL_MMU_ASSERT(FIRST_PAGE_OCCUPIED_ALIGNED >= FIRST_PAGE_OCCUPIED);
	return FIRST_PAGE_OCCUPIED_ALIGNED + SECOND_PAGE_OCCUPIED * SECTION_LENGTH;
}

static INLINE size_t MetaTable_get_required_size(void)
{
	return FIRST_META_OCCUPIED + SECOND_META_OCCUPIED * SECTION_LENGTH;
}

static INLINE unsigned int
Section_offset_part(va_t va)
{
	return (unsigned int)va & SECTION_OFFSET_MASK;
}

static INLINE unsigned int
SmallPage_offset_part(va_t va)
{
	return (unsigned int)va & SMALL_PAGE_OFFSET_MASK;
}

static INLINE va_t SmallPage_vabase_part(va_t va)
{
	return (va_t) (va & ~SMALL_PAGE_OFFSET_MASK);
}

static INLINE pa_t SmallPage_pabase_part(pa_t pa)
{
	return (pa_t) (pa & ~SMALL_PAGE_OFFSET_MASK);
}

static INLINE YL_BOOL SectionEntry_has_second_level(struct FirstPageEntry entry)
{
	return (entry.Table.type == FIRST_TYPE_TABLE) ? 1 : 0;
}

static INLINE YL_BOOL Section_has_second_level(struct FirstPageTable *page0, unsigned short index0)
{
	YL_MMU_ASSERT(index0 < SECTION_LENGTH);

	return SectionEntry_has_second_level(page0->entries[index0]);
}

static INLINE YL_BOOL SectionMetaEntry_has_second_level(struct FirstMetaEntry meta_entry)
{
	return meta_entry.trunk == NULL;
}

static INLINE YL_BOOL
/* PageTable_has_second_level( FirstPageTable page0, int index0, int index1 ) */
PageTable_has_second_level(struct FirstPageTable *page0, unsigned short index0)
{
	return Section_has_second_level(page0, index0);
}

static INLINE YL_BOOL SectionMeta_has_second_level(struct FirstPageTable *table, int index0)
{
	return SectionEntry_has_second_level(table->entries[index0]);
}



static INLINE size_t ceiling_to_granularity(size_t size);





static INLINE size_t ceiling_to_granularity(size_t size)
{
	return (size + MIN_ADDRESSING_SIZE - 1) & ~(MIN_ADDRESSING_SIZE - 1);
}

static INLINE size_t ceiling_to_small(size_t size)
{
	return (size + MMU_SMALL_SIZE - 1) & ~(MMU_SMALL_SIZE - 1);
}

static INLINE size_t ceiling_to_section(size_t size)
{
	return (size + MMU_SECTION_SIZE - 1) & ~(MMU_SECTION_SIZE - 1);
}


/* can't be unsigned short [overflow] */
static INLINE size_t PageTable_calc_size_in_small_blocks(va_t va, size_t size)
{
	unsigned int offset = SmallPage_offset_part(va);

	return ceiling_to_small(offset + size);
}

/* can't be unsigned short [overflow] */
static INLINE size_t PageTable_calc_size_in_section_blocks(va_t va, size_t size)
{
	unsigned int offset = SmallPage_offset_part(va);

	return ceiling_to_small(offset + size);
}


static INLINE unsigned short
PageTable_calc_index1_section_aligned(va_t va)
{
	unsigned int offset = Section_offset_part(va);
	unsigned short index1 = (unsigned short)(offset / MMU_SMALL_SIZE);

	YL_MMU_ASSERT(index1 < SMALL_LENGTH);

	return index1;
}

#endif /* _YL_MMU_HW_TABLE_H_ */