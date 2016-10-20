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
/* brief Memory functions. Not necessary. */

#ifndef _YL_MMU_MEMORY_FUNCTIONS_H_
#define _YL_MMU_MEMORY_FUNCTIONS_H_

#include "yl_mmu_common.h"
#include "yl_mmu_mapper.h"
#include "yl_mmu_iterator.h"


YL_BOOL Remapper_memset(struct Mapper *m, g3d_va_t ptr, unsigned char value, size_t size);
YL_BOOL Remapper_memcpy(struct Mapper *m, g3d_va_t destination, const g3d_va_t source, size_t num);
YL_BOOL Remapper_memcpy_from_g3d_va(struct Mapper *m, va_t destination, const g3d_va_t source,
				    size_t num);
YL_BOOL Remapper_memcpy_to_g3d_va(struct Mapper *m, g3d_va_t destination, const va_t source,
				  size_t num);
YL_BOOL RemapperIterator_memset(struct RemapperIterator *iter, g3d_va_t ptr, int value, size_t num);
YL_BOOL RemapperIterator_memcpy(struct RemapperIterator *iter, g3d_va_t destination,
				const g3d_va_t source, size_t num);
YL_BOOL RemapperIterator_memcpy_from_g3d_va(struct RemapperIterator *iter, va_t destination,
					    const g3d_va_t source, size_t num);
YL_BOOL RemapperIterator_memcpy_to_g3d_va(struct RemapperIterator *iter, g3d_va_t destination,
					  const va_t source, size_t num);
int Remapper_memcmp(struct Mapper *m, va_t va, const g3d_va_t gva, size_t size);

#endif /* _YL_MMU_MEMORY_FUNCTIONS_H_ */
