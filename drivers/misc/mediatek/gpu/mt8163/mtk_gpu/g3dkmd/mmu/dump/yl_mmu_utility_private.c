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

#include "yl_mmu_utility_private.h"

#include <assert.h>

#include "../yl_mmu_hw_table.h"

/**************************** Graphical **********************/

size_t times(char c, int n, char *buf)
{
	int i;

	for (i = 0; i < n; ++i)
		*buf++ = c;


	assert(i >= 0);

	return (size_t) i;
}


size_t render(struct Mapper *m, struct Trunk *t, char c_section, char c_small, char *buf)
{

	unsigned short index0 = t->index.s.index0;
	unsigned short index1 = t->index.s.index1;
	/* next always exists, since we have pivot */
	unsigned short end_index0 = t->next->index.s.index0;
	unsigned short end_index1 = t->next->index.s.index1;
	YL_BOOL ending_in_small = end_index1 > 0;
	struct FirstPageTable *page0 = m->Page_space;

	int nblocks = (t->size + 4095) / 4096;	/* [TODO] ceiling? */
	int idx = 0;

	if (end_index1 == 0) {
		--end_index0;
		end_index1 = SMALL_LENGTH;
	}

	{
		YL_BOOL section_mode;

		/* while not the ending section, AND if the ending section, not the ending small page */
		while ((index0 < end_index0) || ((index0 == end_index0) && (index1 < end_index1))) {


			section_mode = 1;

			/* 1). If small page mode => Fill small page one by one */
			/* 2). If section mode    => Fill section page */

			if ((index0 == end_index0) && (ending_in_small))
				section_mode = 0;
			else if ((index1 != 0))
				section_mode = 0;

			if (!Trunk_in_use(t)) {
				if (section_mode) {
					idx += times('.', 256, buf + idx);
					++index0;
				} else {
					idx += times('.', 1, buf + idx);

					if (++index1 == SMALL_LENGTH) {
						++index0;
						index1 = 0;
					}
				}
				continue;
			}

			if (section_mode && Section_has_second_level(page0, index0))
				section_mode = 0;

			/* c. Free page table and meta */
			/* 1). Reset that page entry directly if 1M */
			/* 2). Reset that page entry if 4K */
			/* a). Recycle that table if count=0 */

			/* [TODO] May require a loop to register all the pa in pa_list */
			/* Even pa is continuous, each page entry used must be filled. */
			/* loop is outside of this if-else */
			if (section_mode) {
				idx += times(c_section, 256, buf + idx);
				++index0;
			} else {
				idx += times(c_small, 1, buf + idx);

				if (++index1 == SMALL_LENGTH) {
					++index0;
					index1 = 0;
				}
			}
		}		/* while */
	}

	assert(idx == nblocks);

	return idx;
}
