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

/* #include <assert.h> */

#include "yl_mmu_trunk_internal.h"

#include "yl_mmu_hw_table_helper.h"
#include "yl_mmu_trunk_metadata_allocator.h"

#include "yl_mmu_trunk_helper_size.h"
#include "yl_mmu_trunk_helper_link.h"

/*********************** Exported Functions ************************************/

/*
 *
 * Return the first (big) free trunk for the initialization step and
 * a pivot which is and will be
 * (1) the pre of first and
 * (2) the last of all the trunk list.
 */
struct Trunk *Trunk_create_first_trunk_and_pivot(size_t size)
{
	struct Trunk *first;
	struct Trunk *pivot;


	first = TrunkMetaAlloc_alloc();
	YL_MMU_ASSERT(first != NULL);
	if (first == NULL)
		return NULL;

	pivot = TrunkMetaAlloc_alloc();
	YL_MMU_ASSERT(pivot != NULL);
	if (pivot == NULL) {
		TrunkMetaAlloc_free(first);
		return NULL;
	}

	YL_MMU_ASSERT(first != pivot);
	YL_MMU_ASSERT(sizeof(first->index) == sizeof(first->index.all));

	first->pre = pivot;
	first->next = pivot;
	Trunk_unset_use(first);
	first->size = size;

	pivot->pre = first;
	pivot->next = first;
	/* Avoiding merging process in free trunks */
	Trunk_set_use(pivot);
	pivot->size = 0;

	first->index.all = 0;	/* The very beginning */

	pivot->index.s.index0 = SECTION_LENGTH;	/* Above the very end */
	pivot->index.s.index1 = 0;

	return first;
}

/*
 *
 * Release all the trunks in the link list, starting from the first one.
 *
 */
void Trunk_release_list(struct Trunk *trunk_head)
{
	struct Trunk *pivot = trunk_head->pre;

	YL_MMU_ASSERT(trunk_head != NULL);
	YL_MMU_ASSERT(trunk_head->pre->size == 0);	/* pivot */

	{
		struct Trunk *c = trunk_head;
		struct Trunk *t = NULL;

		while (c != pivot) {
			t = c;
			c = c->next;
			t->pre = NULL;	/* not necessary */
			t->next = NULL;	/* not necessary */
			TrunkMetaAlloc_free(t);
		}
	}
}

/*
 *
 * Return the newly created trunk which will be connected at the tail of current trunk
 * Note: After this current-> size may not become 0
 */
struct Trunk *Trunk_split_tail(struct Trunk *current, size_t amount2)
{
	YL_MMU_ASSERT(current->size % MIN_ADDRESSING_SIZE == 0);
	YL_MMU_ASSERT(amount2 % MIN_ADDRESSING_SIZE == 0);

	YL_MMU_ASSERT(amount2);
	YL_MMU_ASSERT(current->size >= amount2);

	{
#ifdef _DEBUG
		size_t original_size = current->size;
#endif
		struct Trunk *t = TrunkMetaAlloc_alloc();
		/* struct Trunk *next = NULL; */

		if (t == NULL)	/* Failed, OOM */
			return NULL;


		_Trunk_insert_after(current, t);

		_Trunk_distribute_size_to_second(current, t, amount2);

		YL_MMU_ASSERT(current->size + t->size == original_size);

		return t;
	}
}


struct Trunk *Trunk_split_front(struct Trunk *current, size_t amount1)
{
	YL_MMU_ASSERT(amount1 % MIN_ADDRESSING_SIZE == 0);
	YL_MMU_ASSERT(current->size % MIN_ADDRESSING_SIZE == 0);

	if (current->size <= amount1)	/* oom */
		return NULL;

	{
#ifdef _DEBUG
		size_t original_size = current->size;
#endif
		struct Trunk *t = TrunkMetaAlloc_alloc();
		/* struct Trunk *next = NULL; */

		if (t == NULL)	/* Failed, OOM */
			return NULL;

		_Trunk_insert_before(current, t);

		_Trunk_distribute_size_to_first(t, current, amount1);

		YL_MMU_ASSERT(current->size + t->size == original_size);

		return t;
	}
}

/* Ret: Trunk need to adjust [0 or 1] */
struct Trunk *Trunk_free(struct Trunk *target, struct Trunk *first_trunk)
{
	struct Trunk *pre = target->pre;
	struct Trunk *next = target->next;
	struct Trunk *touched = NULL;
	/* int    merged  = 0; */

	YL_MMU_ASSERT(target);
	YL_MMU_ASSERT(target->size > 0);

	Trunk_unset_use(target);

	/* 1. combine the neighbers (up and down) */

	/* _Trunk_siblings_detach( target );  // Mindos occupied */

	/* cases: */
	/* pre next */
	/* 1   1 */
	/* 0   1 */
	/* 1   0 */
	/* 0   0 */
	if (Trunk_in_use(pre)) {
		if (Trunk_in_use(next)) {
			/* pre & next are in use */
			/* nothing to do */
			touched = target;
		} else {
			YL_MMU_ASSERT(!Trunk_is_pivot_bin(next));
			YL_MMU_ASSERT(!Trunk_is_pivot_trunk(next));

			/* next is freed */
			/* merge (target, next) */
			_Trunk_merge_size_to_second(target, next);
			_Trunk_detach_second(pre, target);
			/* 2. detach from bins */
			_Trunk_siblings_detach(next);
			/* 2. Free this node. */
			TrunkMetaAlloc_free(target);
			touched = next;
		}
	} else {
		/* pre is freed */

		if (pre != first_trunk)
			touched = pre;

		YL_MMU_ASSERT(!Trunk_is_pivot_bin(pre));
		YL_MMU_ASSERT(!Trunk_is_pivot_trunk(pre));

		if (Trunk_in_use(next)) {
			/* merge (pre, target) */
			_Trunk_merge_size_to_first(pre, target);
			_Trunk_detach_second(pre, target);
			/* 2. detach from bins */
			if (pre != first_trunk)
				_Trunk_siblings_detach(pre);
			/* 2. Free this node. */
			TrunkMetaAlloc_free(target);
		} else {
			YL_MMU_ASSERT(!Trunk_is_pivot_bin(next));
			YL_MMU_ASSERT(!Trunk_is_pivot_trunk(next));

			/* merge (pre, target, next) */
			_Trunk_merge_size_to_first3(pre, target, next);
			_Trunk_detach_second_and_third(pre, target, next);
			/* 2. detach from bins */
			if (pre != first_trunk)
				_Trunk_siblings_detach(pre);
			_Trunk_siblings_detach(next);
			/* 2. Free this node. */
			TrunkMetaAlloc_free(target);
			TrunkMetaAlloc_free(next);
		}
	}
	/* anything to do? */
	return touched;
}
