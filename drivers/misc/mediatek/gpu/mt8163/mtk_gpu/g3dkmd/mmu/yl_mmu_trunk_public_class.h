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
/* brief Class Definition. Trunk. */

#ifndef _YL_MMU_TRUNK_PUBLIC_CLASS_H_
#define _YL_MMU_TRUNK_PUBLIC_CLASS_H_

#include "yl_mmu_common.h"
#include "yl_mmu_hw_table_public_class.h"



/********************* Data Structure *****************/

struct Trunk {
	size_t size;
	struct Trunk *pre; /* in addressing space */
	struct Trunk *next; /* in addressing space */
	struct Trunk *left_sibling; /* inside bin */
	struct Trunk *right_sibling; /* inside bin */

	PageIndexRec index;

	unsigned char flags;

};

static INLINE void Trunk_set_use(struct Trunk *self) __attribute__ ((always_inline));
static INLINE void Trunk_unset_use(struct Trunk *self) __attribute__ ((always_inline));
static INLINE YL_BOOL Trunk_in_use(struct Trunk *self) __attribute__ ((always_inline));
static INLINE void Trunk_unset_pivot_trunk(struct Trunk *self) __attribute__ ((always_inline));
static INLINE YL_BOOL Trunk_is_pivot_trunk(struct Trunk *self) __attribute__ ((always_inline));
static INLINE void Trunk_set_pivot_bin(struct Trunk *self) __attribute__ ((always_inline));
static INLINE void Trunk_unset_pivot_bin(struct Trunk *self) __attribute__ ((always_inline));
static INLINE YL_BOOL Trunk_is_pivot_bin(struct Trunk *self) __attribute__ ((always_inline));


/******** Small tools ********/
static INLINE void
Trunk_set_use(struct Trunk *self)
{
	self->flags |= 0x1;
}

static INLINE void
Trunk_unset_use(struct Trunk *self)
{
	self->flags &= ~0x1;
}

static INLINE YL_BOOL Trunk_in_use(struct Trunk *self)
{
	return (self->flags & 0x1) > 0;
}

static INLINE void
Trunk_set_pivot_trunk(struct Trunk *self)
{
	self->flags |= 0x2;
}

static INLINE void
Trunk_unset_pivot_trunk(struct Trunk *self)
{
	self->flags &= ~0x2;
}

static INLINE YL_BOOL Trunk_is_pivot_trunk(struct Trunk *self)
{
	return (self->flags & 0x2) > 0;
	/* return self->is_pivot_trunk > 0; */
}

static INLINE void
Trunk_set_pivot_bin(struct Trunk *self)
{
	self->flags |= 0x4;
}

static INLINE void
Trunk_unset_pivot_bin(struct Trunk *self)
{
	self->flags &= ~0x4;
}

static INLINE YL_BOOL Trunk_is_pivot_bin(struct Trunk *self)
{
	return (self->flags & 0x4) > 0;
	/* return self->is_pivot_bin > 0; */
}


#endif /* _YL_MMU_TRUNK_PUBLIC_CLASS_H_ */
