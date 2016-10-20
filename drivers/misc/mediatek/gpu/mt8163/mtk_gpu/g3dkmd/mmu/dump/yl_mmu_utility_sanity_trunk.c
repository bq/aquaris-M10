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

#include "yl_mmu_utility_sanity_trunk.h"
#include <assert.h>

YL_BOOL Sanity_Trunk_ALL(struct Mapper *m)
{
	struct Trunk *head = m->Bin_list.first_trunk;
	struct Trunk *t = m->Bin_list.first_trunk;
	struct Trunk *next;

	next = t->next;
	while (next != head) {
		/* Property Checking */
		assert(t->size > 0);
		assert(!Trunk_is_pivot_trunk(t));
		assert(!Trunk_is_pivot_bin(t));

		/* Link Checking */
		assert(next->pre == t);
		assert(t->next == next);

		t = next;
		next = t->next;
	}

	return 1;
}
