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

#include "yl_mmu_utility_statistics_trunk.h"
#include "yl_mmu_utility_common.h"
#include "yl_mmu_utility_private.h"
#include "yl_mmu_utility_helper.h"
#include "../yl_mmu_trunk_internal.h"

#include <stdio.h>
#include <assert.h>

void Statistics_Trunk_Usage_Map(struct Mapper *m, FILE *fp)
{
	struct Trunk *head = m->Bin_list.first_trunk;
	struct Trunk *t = m->Bin_list.first_trunk;
	struct Trunk *next;

	char buf[SECTION_LENGTH * SMALL_LENGTH + 1];	/* don't use in kernal mode */
	unsigned int idx = 0;
	char section_symbol = 'A';
	char small_symbol = '0';
	unsigned int symbol_idx = 0;
	char c_section = section_symbol;
	char c_small = small_symbol;

	buf[idx] = '\0';

	assert(fp);
	assert(m);
	assert(head);
	assert(t);

	assert(t->size % granularity == 0);
	assert(!Trunk_in_use(t));

	next = t->next;
	while (next != head) {
		int nblocks;

		assert(t->size);
		assert(!Trunk_is_pivot_trunk(t));
		assert(!Trunk_is_pivot_bin(t));

		nblocks = (t->size + granularity - 1) / granularity;

		assert((unsigned)nblocks < SECTION_LENGTH * SMALL_LENGTH);

		idx += render(m, t, c_section, c_small, buf + idx);

		symbol_idx = (symbol_idx + 1) % 8;
		c_section = section_symbol + symbol_idx;
		c_small = small_symbol + symbol_idx;

		buf[idx] = '\0';

		t = next;
		next = t->next;
	}

	Dump_Usage_Map(buf, fp);

	return;
}
