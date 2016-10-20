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
/* brief Class Definition. Iterator is a fast way to manipulate the memory block inside an allocation. */

#ifndef _YL_MMU_ITERATOR_PUBLIC_CLASS_H_
#define _YL_MMU_ITERATOR_PUBLIC_CLASS_H_

#include "yl_mmu_mapper.h"


struct RemapperIterator {
	struct Mapper *m;
	struct FirstPageTable *Page_space;
	g3d_va_t g3d_address;
	struct Trunk *trunk;
	PageIndexRec index;
	int size;
	int cursor;
	int size_to_next_boundary;
};

#endif /* _YL_MMU_ITERATOR_PUBLIC_CLASS_H_ */
