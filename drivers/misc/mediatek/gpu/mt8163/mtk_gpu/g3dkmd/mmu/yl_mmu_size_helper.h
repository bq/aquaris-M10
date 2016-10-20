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
/* brief Helper for size conversion. */

#ifndef _YL_MMU_SIZE_HELPER_H_
#define _YL_MMU_SIZE_HELPER_H_

#include "yl_mmu_common.h"

static INLINE size_t Mapper_to_size(mmu_size_t msize)
{
	YL_MMU_ASSERT(msize);
	return msize << MMU_GRANULARITY_BIT;
}

/* Simple Floor */
static INLINE mmu_size_t Mapper_to_msize(size_t size)
{
	YL_MMU_ASSERT(size);
	return size >> MMU_GRANULARITY_BIT;
}


#endif /* _YL_MMU_SIZE_HELPER_H_ */
