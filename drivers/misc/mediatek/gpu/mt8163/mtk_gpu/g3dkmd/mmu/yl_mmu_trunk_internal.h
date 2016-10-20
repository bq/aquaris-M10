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
/* brief Internal.  Trunk. */

#ifndef _YL_MMU_TRUNK_INTERNAL_H_
#define _YL_MMU_TRUNK_INTERNAL_H_

#if defined(_WIN32)
#include <limits.h>
#else
#if defined(linux) && defined(linux_user_mode)
#include <limits.h>
/* #include <linux/types.h> */
#endif
#endif

#include "yl_mmu_common.h"
#include "yl_mmu_trunk_public_class.h"
#include "yl_mmu_hw_table.h"
#include "yl_mmu_trunk_private.h"
#include "yl_mmu_trunk_helper_link.h"



/********************* Private Function Declarations *****************/

/* For initialization */
struct Trunk *Trunk_create_first_trunk_and_pivot(size_t size);

/* For Termination */
void Trunk_release_list(struct Trunk *trunk_head);

struct Trunk *Trunk_split_tail(struct Trunk *current, size_t second_size);
struct Trunk *Trunk_split_front(struct Trunk *current, size_t amount1);
struct Trunk *Trunk_free(struct Trunk *target, struct Trunk *first_trunk);
YL_BOOL Trunk_is_start_from_small_page(struct Trunk *current);
static INLINE g3d_va_t Trunk_get_g3d_va(struct Trunk *self, struct FirstPageTable *master,
					g3d_va_t g3d_va_base, va_t va)__attribute__ ((always_inline));



/* unsigned short */
/* Trunk_reference_count(struct Trunk *self) */
/* { */
/* return self->ref_cnt; */
/* } */
/*  */
/* unsigned short */
/* Trunk_increase_reference_count(struct Trunk *self) */
/* { */
/* YL_MMU_ASSERT( self->ref_cnt < USHRT_MAX ); */
/*  */
/* return ++self->ref_cnt; */
/* } */
/*  */
/* unsigned short */
/* Trunk_decrease_reference_count(struct Trunk *self) */
/* { */
/* YL_MMU_ASSERT( self->ref_cnt > 0 ); */
/*  */
/* return --self->ref_cnt; */
/* } */

/********************* Private Function Declarations *****************/

static INLINE g3d_va_t
Trunk_get_g3d_va(struct Trunk *self, struct FirstPageTable *master, g3d_va_t g3d_va_base, va_t va)
{
	/* if ( Trunk_is_start_from_small_page( self ) ) */
	/* if ( Section_has_second_level(master, self->index0) ) */
	if (PageTable_has_second_level(master, self->index.s.index0)) {
		return SmallPage_index_to_g3d_va(master, g3d_va_base, self->index, va);
	} else {
		return SectionPage_index_to_g3d_va(master, g3d_va_base,
						   self->index.s.index0, va);
	}
}


#endif /* _YL_MMU_TRUNK_INTERNAL_H_ */

