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
/* brief Helper. MMU Table for Hardware reading. */

#ifndef _YL_MMU_HW_TABLE_HELPER_H_
#define _YL_MMU_HW_TABLE_HELPER_H_

#include "yl_mmu_hw_table.h"
#include "../g3dbase/g3d_memory_utility.h"

/**************** API *****************/

static INLINE size_t SmallPage_start_offset(unsigned short first_level_entry_index) __attribute__ ((always_inline));
static INLINE size_t SmallMeta_start_offset(unsigned short first_level_entry_index) __attribute__ ((always_inline));

static INLINE struct SecondPageTable *SmallPage_start(struct FirstPageTable *start,
					      unsigned short first_level_entry_index) __attribute__ ((always_inline));
static INLINE struct SecondMetaTable *SmallMeta_start(struct FirstMetaTable *start,
					      unsigned short first_level_entry_index) __attribute__ ((always_inline));

static INLINE void PageTable_g3d_va_to_index(g3d_va_t g3d_va_base, g3d_va_t g3d_va,
					     PageIndex index) __attribute__ ((always_inline));

static INLINE g3d_va_t SectionPage_index_to_g3d_va(struct FirstPageTable *master, g3d_va_t g3d_va_base,
						   unsigned short index0, va_t va) __attribute__ ((always_inline));

static INLINE g3d_va_t SmallPage_index_to_g3d_va(struct FirstPageTable *master, g3d_va_t g3d_va_base,
						 PageIndexRec index, va_t va) __attribute__ ((always_inline));

static INLINE pa_t SectionPage_index_to_pa(struct FirstPageTable *master, unsigned short index0,
					   unsigned int general_address) __attribute__ ((always_inline));

static INLINE pa_t SmallPage_index_to_pa(struct FirstPageTable *master, PageIndexRec index,
					 unsigned int general_address) __attribute__ ((always_inline));

static INLINE struct Trunk *SectionMeta_trunk_by_index(struct FirstMetaTable *start0,
						unsigned short index0) __attribute__ ((always_inline));

static INLINE struct Trunk *SmallMeta_trunk_by_index(struct FirstMetaTable *start0,
					     PageIndexRec index) __attribute__ ((always_inline));

static INLINE struct Trunk *PageTable_trunk_by_index(struct FirstPageTable *page0, struct FirstMetaTable *meta0,
					     PageIndexRec index) __attribute__ ((always_inline));


/**************** Implementation *****************/

static INLINE struct Trunk *SectionMeta_trunk_by_index(struct FirstMetaTable *start0, unsigned short index0)
{
	return start0->entries[index0].trunk;
}

static INLINE struct Trunk *SmallMeta_trunk_by_index(struct FirstMetaTable *start0, PageIndexRec index)
{
	struct SecondMetaTable *meta_table;

	YL_MMU_ASSERT(start0->entries[index.s.index0].trunk == NULL);

	meta_table = SmallMeta_start(start0, index.s.index0);

	return meta_table->entries[index.s.index1].trunk;

}

static INLINE struct Trunk *PageTable_trunk_by_index(struct FirstPageTable *page0, struct FirstMetaTable *meta0,
					     PageIndexRec index)
{
	if (PageTable_has_second_level(page0, index.s.index0))
		return SmallMeta_trunk_by_index(meta0, index);
	else
		return SectionMeta_trunk_by_index(meta0, index.s.index0);

}

/* FROM_TTBR */
static INLINE size_t SmallPage_start_offset(unsigned short first_level_entry_index)
{
	YL_MMU_ASSERT(first_level_entry_index < SECTION_LENGTH);

	return FIRST_PAGE_OCCUPIED_ALIGNED + SECOND_PAGE_OCCUPIED * first_level_entry_index;
}

/* from start of first level meta */
static INLINE size_t SmallMeta_start_offset(unsigned short first_level_entry_index)
{
	YL_MMU_ASSERT(first_level_entry_index < SECTION_LENGTH);

	return FIRST_META_OCCUPIED + SECOND_META_OCCUPIED * first_level_entry_index;
}

/* FROM_TTBR */
static INLINE struct SecondPageTable *SmallPage_start(struct FirstPageTable *start,
							 unsigned short first_level_entry_index)
{
	return (struct SecondPageTable *) (((char *)start) +
					    SmallPage_start_offset(first_level_entry_index));
}

/* from start of first level meta */
static INLINE struct SecondMetaTable *SmallMeta_start(struct FirstMetaTable *start,
							 unsigned short first_level_entry_index)
{
	return (struct SecondMetaTable *) (((char *)start) +
				  SmallMeta_start_offset(first_level_entry_index));
}

static INLINE g3d_va_t
SectionPage_index_to_g3d_va(struct FirstPageTable *master, g3d_va_t g3d_va_base, unsigned short index0,
			    va_t va)
{
	YL_MMU_ASSERT(master->entries[index0].Common.type == FIRST_TYPE_SECTION);

	return g3d_va_base + index0 * MMU_SECTION_SIZE + Section_offset_part(va);

}

static INLINE g3d_va_t
SmallPage_index_to_g3d_va(struct FirstPageTable *master, g3d_va_t g3d_va_base, PageIndexRec index, va_t va)
{
	YL_MMU_ASSERT(master->entries[index.s.index0].Common.type == FIRST_TYPE_TABLE);

	return g3d_va_base
	    + index.s.index0 * MMU_SECTION_SIZE
	    + index.s.index1 * MMU_SMALL_SIZE + SmallPage_offset_part(va);
}


static INLINE pa_t
SectionPage_index_to_pa(struct FirstPageTable *master, unsigned short index0, unsigned int general_address)
{
	YL_MMU_ASSERT(master->entries[index0].Common.type == FIRST_TYPE_SECTION);

	return (pa_t) ((master->entries[index0].all & SECTION_BASE_ADDRESS_MASK) +
		       Section_offset_part(general_address));
}


static INLINE pa_t
SmallPage_index_to_pa(struct FirstPageTable *master, PageIndexRec index, unsigned int general_address)
{
	struct SecondPageTable *start1;

	YL_MMU_ASSERT(master->entries[index.s.index0].Common.type == FIRST_TYPE_TABLE);

	start1 = SmallPage_start(master, index.s.index0);

	return (pa_t) ((start1->entries[index.s.index1].all & SMALL_PAGE_LEVEL1_BASE_ADDRESS_MASK) +
		       SmallPage_offset_part(general_address));
}

static INLINE pa_t
PageTable_index_to_pa(struct FirstPageTable *master, PageIndexRec index, unsigned int general_address)
{

	if (PageTable_has_second_level(master, index.s.index0))
		return SmallPage_index_to_pa(master, index, general_address);
	else
		return SectionPage_index_to_pa(master, index.s.index0, general_address);

}


/* since we don't have the information to know whether small page is used. */
/* index1 won't set to -1 */
static INLINE void PageTable_g3d_va_to_index(g3d_va_t g3d_va_base, g3d_va_t g3d_va, PageIndex index)
{
	unsigned int offset = g3d_va - g3d_va_base;
	unsigned int idx0;
	unsigned int idx1;

	idx0 = offset / MMU_SECTION_SIZE;	/* Floor */
	idx1 = (offset - idx0 * MMU_SECTION_SIZE) / MMU_SMALL_SIZE;	/* Floor */

	index->s.index0 = idx0;
	index->s.index1 = idx1;
}





/********************* External Use ********************/
static INLINE unsigned int PageTable_index0_from_g3d_va(g3d_va_t g3d_va_wo_base)
{
	return (g3d_va_wo_base & G3D_VA_LEVEL0_INDEX_MASK) >> G3D_VA_LEVEL0_INDEX_START_BIT;
}

static INLINE unsigned int PageTable_index1_from_g3d_va(g3d_va_t g3d_va_wo_base)
{
	return (g3d_va_wo_base & G3D_VA_LEVEL1_INDEX_MASK) >> G3D_VA_LEVEL1_INDEX_START_BIT;
}

static INLINE unsigned int SectionEntry_pa_base_address(struct FirstPageEntry entry0)
{
	return entry0.all & SECTION_BASE_ADDRESS_MASK;
}

static INLINE unsigned int SectionEntry_pa_offset(g3d_va_t g3d_va_wo_translation_base)
{
	return g3d_va_wo_translation_base & SECTION_OFFSET_MASK;
}

static INLINE unsigned int SmallEntry_pa_base_address(struct SecondPageEntry entry1)
{
	return entry1.all & SMALL_PAGE_LEVEL1_BASE_ADDRESS_MASK;
}

static INLINE unsigned int SmallEntry_pa_offset(g3d_va_t g3d_va_wo_translation_base)
{
	return g3d_va_wo_translation_base & SMALL_PAGE_OFFSET_MASK;
}

static INLINE pa_t
PageTable_g3d_va_to_pa(G3dMemory *page0_mem, g3d_va_t g3d_va_wo_translation_base)
{
	unsigned int index0 = PageTable_index0_from_g3d_va(g3d_va_wo_translation_base);
	struct FirstPageTable *page0 = (struct FirstPageTable *) page0_mem->data;
	struct FirstPageEntry entry0;

	YL_MMU_ASSERT(g3d_va_wo_translation_base > 0);

	entry0 = page0->entries[index0];

	YL_MMU_ASSERT(entry0.all);
	YL_MMU_ASSERT((entry0.Common.type == FIRST_TYPE_SECTION) ||
		      (entry0.Common.type == FIRST_TYPE_TABLE));

	if (entry0.Common.type == FIRST_TYPE_SECTION) {
		return SectionEntry_pa_base_address(entry0) +
		    SectionEntry_pa_offset(g3d_va_wo_translation_base);
	} else if (entry0.Common.type == FIRST_TYPE_TABLE) {
		pa_t page1_pa;
		struct SecondPageTable *page1;
		unsigned int index1;
		struct SecondPageEntry entry1;

		page1_pa = entry0.all & SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK;
		page1 = (struct SecondPageTable *) g3dbaseContinuousPaToVa(page0_mem, page1_pa);

		index1 = PageTable_index1_from_g3d_va(g3d_va_wo_translation_base);

		entry1 = page1->entries[index1];

		YL_MMU_ASSERT(entry1.Common.type == SECOND_TYPE_SMALL);

		return SmallEntry_pa_base_address(entry1) +
		    SmallEntry_pa_offset(g3d_va_wo_translation_base);
	}

	YL_MMU_ASSERT(0);

	return 0;
}

#endif /* _YL_MMU_HW_TABLE_HELPER_H_ */