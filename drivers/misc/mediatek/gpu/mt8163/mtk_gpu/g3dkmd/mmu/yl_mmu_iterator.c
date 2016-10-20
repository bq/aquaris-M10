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

#include "yl_mmu_iterator.h"

#include "yl_mmu_trunk_internal.h"
#include "yl_mmu_kernel_alloc.h"


void RemapperIterator_Init(struct RemapperIterator *iter, struct Mapper *m, g3d_va_t g3d_va, size_t size)
{
	iter->m = m;
	iter->Page_space = m->Page_space;
	iter->g3d_address = g3d_va - m->G3d_va_base;

	PageTable_g3d_va_to_index(m->G3d_va_base, g3d_va, &iter->index);

	iter->size = size;
	iter->cursor = 0;

	_RemapperIterator_update_size_to_next_boundary(iter);
}

void RemapperIterator_Uninit(struct RemapperIterator *iter)
{

	iter->m = NULL;
	iter->Page_space = NULL;
	iter->g3d_address = 0;
	iter->trunk = NULL;
	iter->index.all = 0;
	iter->size = 0;
	iter->cursor = 0;
}
