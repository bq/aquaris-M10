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

#include "yl_mmu_hw_table.h"

#include "../g3dbase/g3d_memory_utility.h"
#include "../g3dkmd_internal_memory.h"
#include "yl_mmu_hw_table_private.h"
#include "yl_mmu_hw_table_helper.h"

#include "yl_mmu_kernel_alloc.h"

#include "yl_mmu_mapper_public_class.h"


/*
YL_BOOL PageTable_Initialize(struct FirstPageTable *pageTable, struct FirstMetaTable *metaTable )
{
	yl_mmu_zeromem( pageTable, sizeof(struct FirstPageTable) );

	// *metaTable = (struct FirstMetaTable *) yl_mmu_alloc( sizeof(struct FirstMetaTable) );
	// if ( *metaTable == NULL ) return 0;
	// yl_mmu_zeromem( *metaTable, sizeof(struct FirstMetaTable) );

	yl_mmu_zeromem( metaTable, sizeof(struct FirstMetaTable) );

	return 1;
}
*/

void PageTable_Release(struct FirstPageTable *master, struct FirstMetaTable *metaTable)
{
	{
		unsigned int i;

		for (i = 0; i < SECTION_LENGTH; ++i) {
			if (SectionEntry_has_second_level(master->entries[i])) {
				SecondPage_release(SmallPage_start(master, i));
				SecondMeta_release(SmallMeta_start(metaTable, i));
			}

			FirstPage_entry_release(&master->entries[i]);
			FirstMeta_entry_release(&metaTable->entries[i]);
			metaTable->cnt = 0;
		}

		/* The metaTable is passing in, please don't release it... */
		/* yl_mmu_free( *metaTable ); */
		/* *metaTable = NULL; */
	}
}


/* Do the following things: */
/* Set First level page for section  (a) address = pa  (b) flag = TABLE */
/* Set First level meta for section  (a) increase cnt  (b) trunk = trunk */
/* Return 0 if failed */
YL_BOOL
Section_register(struct Mapper *m, unsigned short index0, pa_t pa, struct Trunk *trunk, va_t va, unsigned int flag)
{
	G3dMemory *page_table = &m->Page_mem;
	struct FirstMetaTable *metaTable = m->Meta_space;

#if MMU_MIRROR
	G3dMemory *mirror_table = &m->Mirror_mem;
	struct FirstPageTable *mirror = (struct FirstPageTable *) mirror_table->data;
#else
	struct FirstPageTable *mirror = NULL;
#endif

#if MMU_LOOKBACK_VA
	G3dMemory *lookback_mem = &m->lookback_mem;
#endif

	/* Page Table */
	struct FirstPageTable *master = (struct FirstPageTable *) page_table->data;
	struct FirstPageEntry prepared_entry;
	unsigned int address = pa;
	/* Meta */

	YL_MMU_ASSERT(trunk != NULL);
	YL_MMU_ASSERT(master->entries[index0].all == 0);

	if (master->entries[index0].all > 0)
		return 0;

	/* put the address into this table */
	/* the address must be section(1M) alignment */
	/* [NOTE] this must be done before hw fetch */

	prepared_entry.all = address & SECTION_BASE_ADDRESS_MASK;
	prepared_entry.Section.type = FIRST_TYPE_SECTION;
	prepared_entry.Section.S = !!(flag & G3DKMD_MMU_SHARE);
	prepared_entry.Section.C = !!(flag & G3DKMD_MMU_SHARE);
	prepared_entry.Section.B = !!(flag & G3DKMD_MMU_SHARE);
	prepared_entry.Section.NS = !(flag & G3DKMD_MMU_SECURE);

	/* META */
	metaTable->entries[index0].trunk = trunk;
	metaTable->cnt++;

	/* Last line: enable this page ( visible to HW ) */
	master->entries[index0] = prepared_entry;

	if (mirror) {
		prepared_entry.all = index0 << G3D_VA_LEVEL0_INDEX_START_BIT;
		prepared_entry.Section.type = FIRST_TYPE_SECTION;
		prepared_entry.Section.S = !!(flag & G3DKMD_MMU_SHARE);
		prepared_entry.Section.NS = !(flag & G3DKMD_MMU_SECURE);
		mirror->entries[index0] = prepared_entry;
	}
#if MMU_LOOKBACK_VA
	if (lookback_mem->data) {
		struct FirstPageTable *lookback = (struct FirstPageTable *) lookback_mem->data;

		YL_MMU_ASSERT(((unsigned int)va & SECTION_BASE_ADDRESS_MASK) == (unsigned int)va);
		prepared_entry.all = (unsigned int)(va & SECTION_BASE_ADDRESS_MASK);
		prepared_entry.Section.type = FIRST_TYPE_SECTION;
		prepared_entry.Section.S = !!(flag & G3DKMD_MMU_SHARE);
		prepared_entry.Section.NS = !(flag & G3DKMD_MMU_SECURE);
		lookback->entries[index0] = prepared_entry;
	}
#endif

	return 1;
}

/* Set First level page for small page  (a) address = second level  (b) flag = TABLE        (c) increase subcnt */
/* Set Second level page for small page (a) address = pa            (b) flag = small page   (c) increase cnt */
/* Return 0 if failed */
YL_BOOL
Small_register(struct Mapper *m, PageIndexRec index, pa_t pa, struct Trunk *trunk, va_t va, unsigned int flag)
{
	G3dMemory *page_mem = &m->Page_mem;
	struct FirstMetaTable *metaTable = m->Meta_space;

#if MMU_MIRROR
	G3dMemory *mirror_mem = &m->Mirror_mem;
	struct FirstPageTable *mirror = (struct FirstPageTable *) mirror_mem->data;
#else
	struct FirstPageTable *mirror = NULL;
#endif

#if MMU_LOOKBACK_VA
	G3dMemory *lookback_mem = &m->lookback_mem;
#endif

	struct FirstPageTable *master = (struct FirstPageTable *) page_mem->data;
	struct FirstPageEntry prepared_entry = master->entries[index.s.index0];
	struct SecondPageEntry prepared_entry2;
	struct SecondPageTable *start_address2 = NULL;
	pa_t start_address2_pa = 0;

	/* Initialized but not a small page */
	YL_MMU_ASSERT((prepared_entry.all == 0) || (prepared_entry.Table.type == FIRST_TYPE_TABLE));
	if (prepared_entry.all > 0 && prepared_entry.Table.type != FIRST_TYPE_TABLE)
		return 0;

	/* Note that the second level cleanup occurred when
	 *	(a) module initializing
	 *	(b) when 2nd level page is released
	 * So we don't have to clean it here.
	*/

	start_address2 = SmallPage_start(master, index.s.index0);
	start_address2_pa = (pa_t) g3dbaseVaToContinuousPa(page_mem, (void *)start_address2);

	YL_MMU_ASSERT(start_address2->entries[index.s.index1].all == 0);
	YL_MMU_ASSERT(((unsigned int)(uintptr_t) start_address2 &
		       SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK) ==
		      (unsigned int)(uintptr_t) start_address2);

	/* The address is for reading 2nd level page */
	/* prepared_entry.all              = (unsigned int)start_address2 & SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK; */
	prepared_entry.all = (unsigned int)start_address2_pa & SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK;
	prepared_entry.Table.type = FIRST_TYPE_TABLE;
	prepared_entry.Table.NS = !(flag & G3DKMD_MMU_SECURE);

	prepared_entry2.all = (unsigned int)pa & SMALL_PAGE_LEVEL1_BASE_ADDRESS_MASK;
	prepared_entry2.SmallPage.type = SECOND_TYPE_SMALL;
	prepared_entry2.SmallPage.S = !!(flag & G3DKMD_MMU_SHARE);
	prepared_entry2.SmallPage.C = !!(flag & G3DKMD_MMU_SHARE);
	prepared_entry2.SmallPage.B = !!(flag & G3DKMD_MMU_SHARE);

	{
		/* META */
		struct SecondMetaTable *meta_start_address2 = NULL;
		/* Clean first level (for sure) */
		metaTable->entries[index.s.index0].trunk = NULL;

		meta_start_address2 = SmallMeta_start(metaTable, index.s.index0);
		meta_start_address2->entries[index.s.index1].trunk = trunk;
		meta_start_address2->cnt++;
	}

	/* Last line: enable this page0ared_entry; */
	start_address2->entries[index.s.index1] = prepared_entry2;
	master->entries[index.s.index0] = prepared_entry;

#if MMU_MIRROR
	if (mirror) {
		start_address2 = SmallPage_start(mirror, index.s.index0);
		start_address2_pa =
		    (pa_t) g3dbaseVaToContinuousPa(mirror_mem, (void *)start_address2);

		YL_MMU_ASSERT(start_address2->entries[index.s.index1].all == 0);
		YL_MMU_ASSERT(((unsigned int)(uintptr_t) start_address2 &
			       SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK) ==
			      (unsigned int)(uintptr_t) start_address2);

		prepared_entry.all =
		    (unsigned int)start_address2_pa & SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK;
		prepared_entry.Table.type = FIRST_TYPE_TABLE;
		prepared_entry.Table.NS = !(flag & G3DKMD_MMU_SECURE);

		prepared_entry2.all =
		    ((unsigned int)index.s.index0 << G3D_VA_LEVEL0_INDEX_START_BIT) +
		    ((unsigned int)index.s.index1 << G3D_VA_LEVEL1_INDEX_START_BIT);
		prepared_entry2.SmallPage.type = SECOND_TYPE_SMALL;
		prepared_entry2.SmallPage.S = !!(flag & G3DKMD_MMU_SHARE);

		start_address2->entries[index.s.index1] = prepared_entry2;
		mirror->entries[index.s.index0] = prepared_entry;
	}
#endif

#if MMU_LOOKBACK_VA
	if (lookback_mem->data) {
		struct FirstPageTable *lookback = (struct FirstPageTable *) lookback_mem->data;

		start_address2 = SmallPage_start(lookback, index.s.index0);
		start_address2_pa =
		    (pa_t) g3dbaseVaToContinuousPa(lookback_mem, (void *)start_address2);

		YL_MMU_ASSERT(start_address2->entries[index.s.index1].all == 0);
		YL_MMU_ASSERT(((unsigned int)(uintptr_t) start_address2 &
			       SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK) ==
			      (unsigned int)(uintptr_t) start_address2);
		YL_MMU_ASSERT(((unsigned int)va & SMALL_PAGE_LEVEL1_BASE_ADDRESS_MASK) ==
			      (unsigned int)va);

		prepared_entry.all =
		    (unsigned int)start_address2_pa & SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK;
		prepared_entry.Table.type = FIRST_TYPE_TABLE;
		prepared_entry.Table.NS = !(flag & G3DKMD_MMU_SECURE);


		prepared_entry2.all = ((unsigned int)va & SMALL_PAGE_LEVEL1_BASE_ADDRESS_MASK);
		prepared_entry2.SmallPage.type = SECOND_TYPE_SMALL;
		prepared_entry2.SmallPage.S = !!(flag & G3DKMD_MMU_SHARE);

		start_address2->entries[index.s.index1] = prepared_entry2;
		lookback->entries[index.s.index0] = prepared_entry;
	}
#endif

	return 1;
}


/* Set First level page for section  (a) entry = 0 */
/* Set First level meta for section  (a) decrease cnt  (b) trunk = NULL */
/* Return 0 if failed */
YL_BOOL Section_unregister(struct Mapper *m, unsigned short index0)
{
	G3dMemory *page_mem = &m->Page_mem;
	struct FirstMetaTable *metaTable = m->Meta_space;

#if MMU_MIRROR
	G3dMemory *mirror_table = &m->Mirror_mem;
	struct FirstPageTable *mirror = (struct FirstPageTable *) mirror_table->data;
#else
	struct FirstPageTable *mirror = NULL;
#endif

#if MMU_LOOKBACK_VA
	G3dMemory *lookback_mem = &m->lookback_mem;
#endif

	struct FirstPageTable *master = (struct FirstPageTable *) page_mem->data;

	YL_MMU_ASSERT(metaTable->cnt > 0);

	/* Something wrong if already cleaned */
	YL_MMU_ASSERT(master->entries[index0].Common.type == FIRST_TYPE_SECTION);

	/* First line: disable this page ( visible to HW ) */
	master->entries[index0].all = 0;
	if (mirror)
		mirror->entries[index0].all = 0;

#if MMU_LOOKBACK_VA
	if (lookback_mem->data) {
		struct FirstPageTable *lookback = (struct FirstPageTable *) lookback_mem->data;

		lookback->entries[index0].all = 0;
	}
#endif
	/* META */
	metaTable->entries[index0].trunk = NULL;
	metaTable->cnt--;

	return 1;
}

/* Set Second level page for small page (a) entry = 0 (c) --cnt */
/* if second.cnt == 0 */
/* Clean First level page for small page (a) entry = 0 */
/* else */
/* do nothing */
/* Return 0 if failed */
YL_BOOL Small_unregister(struct Mapper *m, PageIndexRec index)
{
	G3dMemory *page_mem = &m->Page_mem;
	struct FirstMetaTable *metaTable = m->Meta_space;

#if MMU_MIRROR
	G3dMemory *mirror_table = &m->Mirror_mem;
	struct FirstPageTable *mirror = (struct FirstPageTable *) mirror_table->data;
#else
	struct FirstPageTable *mirror = NULL;
#endif

#if MMU_LOOKBACK_VA
	G3dMemory *lookback_mem = &m->lookback_mem;
#endif

	struct FirstPageTable *master = (struct FirstPageTable *) page_mem->data;

	struct SecondPageTable *start_address2;
	struct SecondMetaTable *meta_start_address2;

	/* not a small page */
	YL_MMU_ASSERT(master->entries[index.s.index0].Table.type == FIRST_TYPE_TABLE);

	start_address2 = SmallPage_start(master, index.s.index0);

	/* check if the address is valid */
	YL_MMU_ASSERT(((unsigned int)(uintptr_t) start_address2 &
		       SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK) ==
		      (unsigned int)(uintptr_t) start_address2);

	/* META part 1 */
	meta_start_address2 = SmallMeta_start(metaTable, index.s.index0);

	YL_MMU_ASSERT(metaTable->entries[index.s.index0].trunk == NULL);
	YL_MMU_ASSERT(master->entries[index.s.index0].Common.type == FIRST_TYPE_TABLE);
	YL_MMU_ASSERT(start_address2->entries[index.s.index1].Common.type == SECOND_TYPE_SMALL);

	/* First line: disable this page (visible to HW) */
	start_address2->entries[index.s.index1].all = 0;
	/* if there is no second page items responding to first page, the first page content shall be reset */
	if (meta_start_address2->cnt == 1) {
		/* First line: disable this page (visible to HW) */
		master->entries[index.s.index0].all = 0;
	}

	if (mirror) {
		start_address2 = SmallPage_start(mirror, index.s.index0);

		YL_MMU_ASSERT(((unsigned int)(uintptr_t) start_address2 &
			       SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK) ==
			      (unsigned int)(uintptr_t) start_address2);
		YL_MMU_ASSERT(mirror->entries[index.s.index0].Common.type == FIRST_TYPE_TABLE);
		YL_MMU_ASSERT(start_address2->entries[index.s.index1].Common.type ==
			      SECOND_TYPE_SMALL);

		start_address2->entries[index.s.index1].all = 0;
		/* start_address2->entries[index.s.index1].all = 0xFFFFAAA0; // debug */
		if (meta_start_address2->cnt == 1)
			mirror->entries[index.s.index0].all = 0;
	}
#if MMU_LOOKBACK_VA
	if (lookback_mem->data) {
		struct FirstPageTable *lookback = (struct FirstPageTable *) lookback_mem->data;

		start_address2 = SmallPage_start(lookback, index.s.index0);

		YL_MMU_ASSERT(((unsigned int)(uintptr_t) start_address2 &
			       SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK) ==
			      (unsigned int)(uintptr_t) start_address2);
		YL_MMU_ASSERT(lookback->entries[index.s.index0].Common.type == FIRST_TYPE_TABLE);
		YL_MMU_ASSERT(start_address2->entries[index.s.index1].Common.type ==
			      SECOND_TYPE_SMALL);

		start_address2->entries[index.s.index1].all = 0;
		if (meta_start_address2->cnt == 1)
			lookback->entries[index.s.index0].all = 0;
	}
#endif

	/* META part 2 */
	meta_start_address2->entries[index.s.index1].trunk = NULL;
	meta_start_address2->cnt--;

	return 1;
}
