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
/* brief Internal.  Link related trunk helper functions. */

#ifndef _YL_MMU_TRUNK_HELPER_LINK_H_
#define _YL_MMU_TRUNK_HELPER_LINK_H_

#include "yl_mmu_trunk_public_class.h"
#include "yl_mmu_trunk_internal.h"
#include "yl_mmu_hw_table_helper.h"



static INLINE void Trunk_link_reset(struct Trunk *target) __attribute__ ((always_inline));

static INLINE void _Trunk_insert_before(struct Trunk *current, struct Trunk *new_one) __attribute__ ((always_inline));

static INLINE void _Trunk_insert_after(struct Trunk *current, struct Trunk *new_one) __attribute__ ((always_inline));

static INLINE void _Trunk_detach_second(struct Trunk *pre, struct Trunk *target) __attribute__ ((always_inline));

static INLINE void _Trunk_detach_second_and_third(struct Trunk *first, struct Trunk *second,
						  struct Trunk *third) __attribute__ ((always_inline));

static INLINE void Trunk_siblings_reset(struct Trunk *target) __attribute__ ((always_inline));

static INLINE void Trunk_siblings_connect(struct Trunk *left, struct Trunk *right) __attribute__ ((always_inline));

static INLINE void Trunk_siblings_insert_after(struct Trunk *anchor,
						struct Trunk *new_one) __attribute__ ((always_inline));

static INLINE void _Trunk_siblings_detach(struct Trunk *target) __attribute__ ((always_inline));


static INLINE void
Trunk_link_reset(struct Trunk *target)
{
	target->pre = NULL;
	target->next = NULL;
}

static INLINE void
_Trunk_insert_before(struct Trunk *current, struct Trunk *new_one)
{
	struct Trunk *prev = current->pre;

	/* you can't insert before head. */
	/* You will ruin the datastructure. */
	YL_MMU_ASSERT(!Trunk_is_head(current));
	YL_MMU_ASSERT(current != new_one);
	YL_MMU_ASSERT(prev != NULL);
	YL_MMU_ASSERT(current != NULL);
	YL_MMU_ASSERT(new_one != NULL);

	prev->next = new_one;
	new_one->pre = prev;

	new_one->next = current;
	current->pre = new_one;
}

static INLINE void
_Trunk_insert_after(struct Trunk *current, struct Trunk *new_one)
{
	struct Trunk *next = current->next;

	YL_MMU_ASSERT(current != new_one);
	YL_MMU_ASSERT(current != NULL);
	YL_MMU_ASSERT(new_one != NULL);

	current->next = new_one;
	new_one->pre = current;

	new_one->next = next;
	next->pre = new_one;
}

static INLINE void
_Trunk_detach_second(struct Trunk *pre, struct Trunk *target)
{
	struct Trunk *next = target->next;

	pre->next = next;
	next->pre = pre;

	/* For safety */
	Trunk_link_reset(target); /* not necessary */
}


static INLINE void
_Trunk_detach_second_and_third(struct Trunk *first, struct Trunk *second, struct Trunk *third)
{
	struct Trunk *forth;

	YL_MMU_ASSERT(first != NULL);
	YL_MMU_ASSERT(second != NULL);
	YL_MMU_ASSERT(third != NULL);

	forth = third->next;
	first->next = forth;
	forth->pre = first;

	/* For safety */
	Trunk_link_reset(second); /* not necessary */
	Trunk_link_reset(third); /* not necessary */
}

static INLINE void
Trunk_siblings_reset(struct Trunk *target)
{
	target->left_sibling = NULL;
	target->right_sibling = NULL;
}

static INLINE void
Trunk_siblings_connect(struct Trunk *left, struct Trunk *right)
{
	YL_MMU_ASSERT(left != NULL);
	YL_MMU_ASSERT(right != NULL);

	left->right_sibling = right;
	right->left_sibling = left;
}

static INLINE void
Trunk_siblings_insert_after(struct Trunk *anchor, struct Trunk *new_one)
{
	struct Trunk *next = anchor->right_sibling;

	YL_MMU_ASSERT(anchor != NULL);
	YL_MMU_ASSERT(new_one != NULL);
	YL_MMU_ASSERT(next != NULL);

	Trunk_siblings_connect(anchor, new_one);
	Trunk_siblings_connect(new_one, next);
}

static INLINE void
_Trunk_siblings_detach(struct Trunk *target)
{
	struct Trunk *left;
	struct Trunk *right;

	YL_MMU_ASSERT(target);
	YL_MMU_ASSERT(!Trunk_is_pivot_trunk(target)); /* something wrong if you want to detach a trunk actually a bin */
	YL_MMU_ASSERT(!Trunk_is_pivot_bin(target)); /* something wrong if you want to detach a trunk actually a bin */

	left = target->left_sibling;
	right = target->right_sibling;

	YL_MMU_ASSERT(left != NULL);
	YL_MMU_ASSERT(right != NULL);

	Trunk_siblings_connect(left, right);
	Trunk_siblings_reset(target);
}

#endif /* _YL_MMU_TRUNK_HELPER_LINK_H_ */
