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
/* brief Class definition. MMU Table for Hardware reading. */

#ifndef _YL_MMU_HW_TABLE_PUBLIC_CLASS_H_
#define _YL_MMU_HW_TABLE_PUBLIC_CLASS_H_

#include "yl_mmu_index_public_class.h"

/* /////// Enums */
#define FIRST_TYPE_TABLE      1
#define FIRST_TYPE_SECTION    2

#define SECOND_TYPE_LARGE      1
#define SECOND_TYPE_SMALL      2


/* #define MMU_SECTION_SIZE       1048576 */
/* #define MMU_SMALL_SIZE         4096 */

#define MMU_PAGE_TABLE_ALIGNMENT ((~SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK)+1)
#define MMU_PAGE_TABLE_ALIGNMENT_OFFSET (~SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK)

#define FIRST_ENTRY_OCCUPIED      sizeof(struct FirstPageEntry)
#define SECOND_ENTRY_OCCUPIED     sizeof(struct SecondPageEntry)

#define FIRST_PAGE_OCCUPIED             sizeof(struct FirstPageTable)
#define FIRST_PAGE_OCCUPIED_ALIGNED \
	((sizeof(struct FirstPageTable) + MMU_PAGE_TABLE_ALIGNMENT_OFFSET) & SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK)
#define SECOND_PAGE_OCCUPIED   sizeof(struct SecondPageTable)

#define SECOND_PAGE_START_OFFSET_FROM_TTBR(first_level_entry_index) \
	(FIRST_PAGE_OCCUPIED_ALIGNED + SECOND_PAGE_OCCUPIED * first_level_entry_index)


/* Metadata */
#define FIRST_META_ENTRY_OCCUPIED      sizeof(struct FirstMetaEntry)
#define SECOND_META_ENTRY_OCCUPIED     sizeof(struct SecondMetaEntry)

#define FIRST_META_OCCUPIED    sizeof(struct FirstMetaTable)
#define SECOND_META_OCCUPIED   sizeof(struct SecondMetaTable)










struct FirstPageEntry {
	union {
		unsigned int all;

		struct		/* Common_ */
		{
			unsigned int type:2;
			unsigned int ignored:30;
		} Common;

		struct		/* Fault_ */
		{
			unsigned int type:2;	/* = 00 */
			unsigned int ignored:30;
		} Fault;

		struct		/* Table_ */
		{
			unsigned int type:2;	/* [1:0] = 01 */
			unsigned int SBZ0:1;	/* [2:2] */
			unsigned int NS:1;	/* [3:3]    1:not-secured */
			unsigned int SBZ1:1;	/* [4:4] */
			unsigned int Domain:4;	/* [8:5]    0 // ignore first */
			unsigned int IMP:1;	/* [9:9]    0 // Implementation defined */

			unsigned int base:22;	/* [31:10] */
		} Table;

		struct		/* Section_  // 1M page */
		{
			unsigned int type:2;	/* [1:0] = 10 */
			unsigned int B:1;	/* [2:2] */
			unsigned int C:1;	/* [3:3] */
			unsigned int XN:1;	/* [4:4] */
			unsigned int Domain:4;	/* [8:5] */
			unsigned int IMP:1;	/* [9:9] */
			unsigned int AP0:2;	/* [11:10] */
			unsigned int TEX:3;	/* [14:12] */
			unsigned int AP2:1;	/* [15:15] */
			unsigned int S:1;	/* [16:16] */
			unsigned int nG:1;	/* [17:17] */
			unsigned int sectionbit:1;	/* [18:18] section=0 (supersection=1) */
			unsigned int NS:1;	/* [19:19] */
			unsigned int Base:12;	/* [31:20] */
		} Section;

		struct		/* SuperSection_ */
		{
			unsigned int type:2;	/* [1:0] = 10 */
			unsigned int B:1;	/* [2:2] */
			unsigned int C:1;	/* [3:3] */
			unsigned int XN:1;	/* [4:4] */
			unsigned int ignored:27;	/* [31:5] */
		} SuperSection;

		struct		/* Reserved_ */
		{
			unsigned int type:2;	/* [1:0] = 11 */
			unsigned int reserved:30;	/* [31:2] */
		} Reserved;

	};			/* union */

};


struct SecondPageEntry {
	union {
		unsigned int all;

		struct		/* Common_ */
		{
			unsigned int type:2;
			unsigned int ignored:30;
		} Common;

		struct		/* Fault_ */
		{
			unsigned int type:2;	/* = 00 */
			unsigned int ignored:30;
		} Fault;

		struct		/* LargePage_ */
		{
			unsigned int type:2;	/* [1:0]    == 01 */
			unsigned int B:1;	/* [2:2] */
			unsigned int C:1;	/* [3:3] */
			unsigned int AP0:2;	/* [5:4]    11  // RW / RW */
			unsigned int SBZ:3;	/* [8:6] */
			unsigned int AP2:1;	/* [9:9]    0   // RW / RW */
			unsigned int S:1;	/* [10:10] */
			unsigned int nG:1;	/* [11:11] */
			unsigned int TEX:3;	/* [14:12] */
			unsigned int XN:1;	/* [15:15]  == 1 */
			unsigned int bass:16;	/* [31:16] */
		} LargePage;

		struct		/* SmallPage_ */
		{
			unsigned int type:2;	/* [1:0]    == 11 (including XN) */
			unsigned int B:1;	/* [2:2] */
			unsigned int C:1;	/* [3:3] */
			unsigned int AP0:2;	/* [5:4]    11  // RW / RW */
			unsigned int TEX:3;	/* [8:6] */
			unsigned int AP2:1;	/* [9:9]    0   // RW / RW */
			unsigned int S:1;	/* [10:10] */
			unsigned int nG:1;	/* [11:11] */
			unsigned int bass:20;	/* [31:16] */
		} SmallPage;
	};			/* union */

};


struct FirstMetaEntry {
	struct Trunk *trunk;	/* Inverted lookup */

};


struct SecondMetaEntry {
	struct Trunk *trunk;	/* Inverted lookup */

};


struct FirstPageTable {
	/* 256 M / 1M = 256 */
	/* 512 M / 1M = 512 */
	struct FirstPageEntry entries[SECTION_LENGTH];

};


struct SecondPageTable {
	struct SecondPageEntry entries[SMALL_LENGTH];	/* 1 M / 4K = 256 */

};


/* Can put in Virtual Memory */
struct FirstMetaTable {
	struct FirstMetaEntry entries[SECTION_LENGTH];
	int cnt; /* count of 1M entry */

};


/* Can put in Virtual Memory */
struct SecondMetaTable {
	struct SecondMetaEntry entries[SMALL_LENGTH];
	int cnt;

};

#endif /* _YL_MMU_HW_TABLE_PUBLIC_CLASS_H_ */
