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
/* brief Private.  Trunk. */

#ifndef _YL_MMU_TRUNK_PRIVATE_H_
#define _YL_MMU_TRUNK_PRIVATE_H_

#include "yl_mmu_common.h"
#include "yl_mmu_trunk_public_class.h"


static INLINE YL_BOOL Trunk_is_pivot(struct Trunk *self) __attribute__ ((always_inline));
static INLINE YL_BOOL Trunk_is_head(struct Trunk *self) __attribute__ ((always_inline));



static INLINE YL_BOOL Trunk_is_pivot(struct Trunk *self)
{
	return self->size == 0;
}

static INLINE YL_BOOL Trunk_is_head(struct Trunk *self)
{
	return self->pre->size == 0;
}

#endif /* _YL_MMU_TRUNK_PRIVATE_H_ */