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
/* brief Internal.  Size related trunk helper functions. */

#ifndef _YL_MMU_TRUNK_HELPER_SIZE_H_
#define _YL_MMU_TRUNK_HELPER_SIZE_H_

#include "yl_mmu_trunk_public_class.h"
#include "yl_mmu_hw_table_helper.h"
#include "yl_mmu_index_helper.h"


static INLINE void _Trunk_merge_size_to_second(struct Trunk *first,
							struct Trunk *second) __attribute__ ((always_inline));
static INLINE void _Trunk_merge_size_to_first(struct Trunk *first,
						struct Trunk *second) __attribute__ ((always_inline));
static INLINE void _Trunk_merge_size_to_first3(struct Trunk *first, struct Trunk *second,
						struct Trunk *third) __attribute__ ((always_inline));
static INLINE void _Trunk_distribute_size_to_second(struct Trunk *first, struct Trunk *second,
						    size_t amount) __attribute__ ((always_inline));
static INLINE void _Trunk_distribute_size_to_first(struct Trunk *first, struct Trunk *second,
						   size_t amount1) __attribute__ ((always_inline));
static INLINE void _Trunk_distribute_size_from_first3(struct Trunk *first, struct Trunk *second,
						      struct Trunk *third, size_t amount2,
						      size_t amount3)__attribute__ ((always_inline));
static INLINE void _Trunk_distribute_size_from_third3(struct Trunk *first, struct Trunk *second,
						      struct Trunk *third, size_t amount1,
						      size_t amount2) __attribute__ ((always_inline));


/************************** Impl ***************************/

static INLINE void
_Trunk_merge_size_to_second(struct Trunk *first, struct Trunk *second)
{
	YL_MMU_ASSERT(first != NULL);
	YL_MMU_ASSERT(second != NULL);

	second->size += first->size;

	second->index = first->index;

	/* not necessary */
	first->size = 0;
	first->index.all = 0;
}

static INLINE void
_Trunk_merge_size_to_first(struct Trunk *first, struct Trunk *second)
{
	YL_MMU_ASSERT(first != NULL);
	YL_MMU_ASSERT(second != NULL);
	YL_MMU_ASSERT(first->size % MIN_ADDRESSING_SIZE == 0);
	YL_MMU_ASSERT(second->size % MIN_ADDRESSING_SIZE == 0);

	first->size += second->size;

	/* not necessary */
	second->size = 0;
	second->index.all = 0;
}


static INLINE void
_Trunk_merge_size_to_first3(struct Trunk *first, struct Trunk *second, struct Trunk *third)
{
	YL_MMU_ASSERT(first != NULL);
	YL_MMU_ASSERT(second != NULL);
	YL_MMU_ASSERT(third != NULL);

	first->size += second->size + third->size;

	/* not necessary */
	second->size = 0;
	second->index.all = 0;

	third->size = 0;
	third->index.all = 0;
}


static INLINE void
_Trunk_distribute_size_to_second(struct Trunk *first, struct Trunk *second, size_t amount2)
{
	size_t original_size = first->size;
	size_t amount1 = original_size - amount2;

	YL_MMU_ASSERT(original_size);
	YL_MMU_ASSERT(amount1);
	YL_MMU_ASSERT(amount2);

	first->size = amount1;
	second->size = amount2;

	YL_MMU_ASSERT(amount1 % MIN_ADDRESSING_SIZE == 0);
	YL_MMU_ASSERT(amount2 % MIN_ADDRESSING_SIZE == 0);

	{
		int inc1 = amount1 / MIN_ADDRESSING_SIZE;

		second->index = first->index;
		PageIndex_add(&second->index, inc1);
	}
}


static INLINE void
_Trunk_distribute_size_to_first(struct Trunk *first, struct Trunk *second, size_t amount1)
{
	size_t original_size = second->size;
	size_t amount2 = original_size - amount1;

	YL_MMU_ASSERT(original_size);
	YL_MMU_ASSERT(amount1);
	YL_MMU_ASSERT(amount2);

	first->size = amount1;
	second->size = amount2;

	YL_MMU_ASSERT(amount1 % MIN_ADDRESSING_SIZE == 0);
	YL_MMU_ASSERT(amount2 % MIN_ADDRESSING_SIZE == 0);

	{
		int inc1 = amount1 / MIN_ADDRESSING_SIZE;

		first->index = second->index;

		PageIndex_add(&second->index, inc1);
	}
}


static INLINE void _Trunk_distribute_size_from_first3(struct Trunk *first, struct Trunk *second, struct Trunk *third,
								size_t amount2, size_t amount3)
{
	size_t original_size = first->size;
	size_t amount1 = original_size - amount2 - amount3;

	YL_MMU_ASSERT(original_size);
	YL_MMU_ASSERT(amount1);
	YL_MMU_ASSERT(amount2);
	YL_MMU_ASSERT(amount3);

	first->size = amount1;
	second->size = amount2;
	third->size = amount3;

	YL_MMU_ASSERT(amount1 % MIN_ADDRESSING_SIZE == 0);
	YL_MMU_ASSERT(amount2 % MIN_ADDRESSING_SIZE == 0);
	YL_MMU_ASSERT(amount3 % MIN_ADDRESSING_SIZE == 0);

	{
		PageIndexRec index = first->index;
		int inc1a = amount1 / MIN_ADDRESSING_SIZE;
		int inc1b = amount2 / MIN_ADDRESSING_SIZE;

		PageIndex_add(&index, inc1a);
		second->index = index;

		PageIndex_add(&index, inc1b);
		third->index = index;
	}
}


static INLINE void
_Trunk_distribute_size_from_third3(struct Trunk *first, struct Trunk *second, struct Trunk *third,
					  size_t amount1, size_t amount2)
{
	size_t original_size = third->size;
	size_t amount3 = original_size - amount1 - amount2;

	YL_MMU_ASSERT(original_size);
	YL_MMU_ASSERT(amount1);
	YL_MMU_ASSERT(amount2);
	YL_MMU_ASSERT(amount3);

	first->size = amount1;
	second->size = amount2;
	third->size = amount3;

	YL_MMU_ASSERT(amount1 % MIN_ADDRESSING_SIZE == 0);
	YL_MMU_ASSERT(amount2 % MIN_ADDRESSING_SIZE == 0);
	YL_MMU_ASSERT(amount3 % MIN_ADDRESSING_SIZE == 0);

	{
		PageIndexRec index = third->index;
		int inc1a = amount1 / MIN_ADDRESSING_SIZE;
		int inc1b = amount2 / MIN_ADDRESSING_SIZE;

		first->index = index;

		PageIndex_add(&index, inc1a);
		second->index = index;

		PageIndex_add(&index, inc1b);
		third->index = index;
	}
}

#endif /* _YL_MMU_TRUNK_HELPER_LINK_H_ */
