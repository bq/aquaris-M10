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
/* brief Helper. Page Index, index0 is for first-level page table index1 is for second-level. */


#ifndef _YL_MMU_INDEX_HELPER_H_
#define _YL_MMU_INDEX_HELPER_H_

#include "yl_mmu_index_public_class.h"

static INLINE void PageIndex_add(PageIndex index, int inc1);
static INLINE void PageIndex_subtract(PageIndex index, int inc1);
static INLINE unsigned short PageIndex_tail_padding_for_align(unsigned short index1_container_tail,
						unsigned short index1_item_tail);







static INLINE void PageIndex_add(PageIndex index, int inc1)
{
	unsigned short inc0 = index->s.index0;

	/* Input Assumption */
	YL_MMU_ASSERT(inc1 >= 0);


	if (index->s.index1 > 0) /* -1 if not using, 0 don't have to add */
		inc1 += index->s.index1;
	/* else */
	/* inc1 = inc1; */

	inc0 += (unsigned short)(inc1 / SMALL_LENGTH);
	inc1 %= SMALL_LENGTH;

	YL_MMU_ASSERT(inc0 < SECTION_LENGTH);
	YL_MMU_ASSERT((unsigned)inc1 < SMALL_LENGTH);
	/* YL_MMU_ASSERT( inc0 >= 0 ); */
	YL_MMU_ASSERT(inc1 >= 0);

	index->s.index0 = inc0;
	index->s.index1 = (unsigned short)inc1;
}


static INLINE void PageIndex_subtract(PageIndex index, int inc1)
{
	int inc0 = (int)index->s.index0;

	/* Input Assumption */
	YL_MMU_ASSERT(inc1 >= 0);


	if (index->s.index1 > 0) /* -1 if not using, 0 don't have to add */
		inc1 = (int)index->s.index1 - inc1;
	else
		inc1 = -inc1;

	/* (a/b)*b + a%b shall equal a. */
	if (inc1 < 0) {
		inc0 += inc1 / SMALL_LENGTH; /* index0 + carry */
		inc1 = inc1 % SMALL_LENGTH; /* remainder */

		if (inc1 < 0) {
			--inc0;
			inc1 += SMALL_LENGTH;
		}
	}

	YL_MMU_ASSERT((unsigned)inc0 < SECTION_LENGTH);
	YL_MMU_ASSERT((unsigned)inc1 < SMALL_LENGTH);
	YL_MMU_ASSERT(inc0 >= 0);
	YL_MMU_ASSERT(inc1 >= 0);

	index->s.index0 = (unsigned int)inc0;
	index->s.index1 = (unsigned int)inc1;
}

static INLINE unsigned short
PageIndex_tail_padding_for_align(unsigned short index1_next, unsigned short index1_item_tail)
{
	unsigned short padding = SMALL_LENGTH + index1_next - (index1_item_tail + 1);

	YL_MMU_ASSERT(index1_item_tail < SMALL_LENGTH);

	YL_MMU_ASSERT(SMALL_LENGTH + index1_next == padding + index1_item_tail + 1U);

	padding %= SMALL_LENGTH;

	return padding;
}

#endif /* _YL_MMU_INDEX_HELPER_H_ */