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

#include "yl_mmu_trunk_metadata_allocator.h"
#include "yl_mmu_mapper_internal.h"
#include "yl_mmu_kernel_alloc.h"







#define BARREL_SIZE 1000

struct TrunkWrapper {
	struct Trunk root;
	struct TrunkWrapper *pre; /* this Free => pre free   // same as occupied */
	struct TrunkWrapper *next; /* this free => next free */
};


struct TrunkBarrel {
	struct TrunkWrapper trunks[BARREL_SIZE];
	struct TrunkBarrel *pre; /* this Free => pre free   // same as occupied */
	struct TrunkBarrel *next; /* this free => next free */
};


#define FIRST_TRUNK(barrel) ((barrel)->trunks)
#define LAST_TRUNK(barrel)  ((barrel)->trunks + (BARREL_SIZE-1))


struct TrunkBarrel *trunk_barrel_head = 0; /* trunk memory block */
struct TrunkBarrel *trunk_barrel_tail = 0;

struct TrunkWrapper *free_trunk_head = 0;
struct TrunkWrapper *free_trunk_tail = 0;

struct TrunkWrapper *used_trunk_head = 0;
struct TrunkWrapper *used_trunk_tail = 0;


/************************ TrunkBarrel ***************************/


static void insert_trunks(struct TrunkWrapper *anchor, struct TrunkWrapper *first, struct TrunkWrapper *last)
{
	struct TrunkWrapper *next = anchor->next;

	anchor->next = first;
	first->pre = anchor;

	next->pre = last;
	last->next = next;
}

static void insert_trunk(struct TrunkWrapper *anchor, struct TrunkWrapper *item)
{
	struct TrunkWrapper *next = anchor->next;

	anchor->next = item;
	item->pre = anchor;

	next->pre = item;
	item->next = next;
}

static struct TrunkWrapper *cut_next_trunk(struct TrunkWrapper *anchor)
{
	struct TrunkWrapper *item = anchor->next;
	struct TrunkWrapper *next = item->next;

	YL_MMU_ASSERT(anchor != item);
	YL_MMU_ASSERT(item != next);
	YL_MMU_ASSERT(anchor != next);

	anchor->next = next;
	next->pre = anchor;

	item->pre = NULL;
	item->next = NULL;

	return item;
}




/* new_trunk_barrel()
 * Create a trunk barrel and arrange the link from top to down
 */
static struct TrunkBarrel *TrunkBarrel_new(void)
{
	size_t size = sizeof(struct TrunkBarrel);

	/* alloc ... */
	struct TrunkBarrel *barrel = (struct TrunkBarrel *) yl_mmu_alloc(size);

	if (!barrel)
		return NULL;

	/* Make all trunks free but wrong links */
	yl_mmu_zeromem(barrel, size);

	/* Fix free trunks */
	{
		struct TrunkWrapper *current = FIRST_TRUNK(barrel);
		struct TrunkWrapper *last = LAST_TRUNK(barrel);
		struct TrunkWrapper *next = NULL;

		current->pre = NULL;
		for (; current != last; current = next) {
			next = current + 1;

			current->root.index.all = 0;

			current->next = next;
			next->pre = current;

			next->next = NULL;	/* set the last one will do, but put it here */
			/* Free */
			Trunk_unset_use(&next->root);
		}
	}

	return barrel;
}



static YL_BOOL expand_trunk_barrel_if_needed(void)
{
	if (free_trunk_tail->pre == free_trunk_head) {
		struct TrunkBarrel *new_barrel = 0;
		struct TrunkWrapper *trunk_first = 0;
		struct TrunkWrapper *trunk_last = 0;

		YL_MMU_ASSERT(free_trunk_head->next == free_trunk_tail);

		new_barrel = TrunkBarrel_new();

		if (!new_barrel)
			return 0;

		trunk_first = FIRST_TRUNK(new_barrel);
		trunk_last = LAST_TRUNK(new_barrel);

		insert_trunks(free_trunk_tail->pre, trunk_first, trunk_last);

		trunk_barrel_tail->next = new_barrel;
		new_barrel->pre = trunk_barrel_tail;
		trunk_barrel_tail = new_barrel;
	}

	return 1;
}


/************************ Trunk Metadata Allocator ***************************/


YL_BOOL TrunkMetaAlloc_initialize(void)
{
	struct TrunkWrapper *first = NULL;
	struct TrunkWrapper *last = NULL;

	trunk_barrel_head = trunk_barrel_tail = TrunkBarrel_new();

	if (!trunk_barrel_head)
		return 0;

	/* Create a dummy used trunk */
	{
		used_trunk_head = trunk_barrel_head->trunks;	/* [0] */
		used_trunk_tail = trunk_barrel_head->trunks + 1;	/* [1] */
		Trunk_set_use(&used_trunk_head->root);
		Trunk_set_use(&used_trunk_tail->root);
		used_trunk_head->pre = NULL;
		used_trunk_head->next = used_trunk_tail;
		used_trunk_tail->pre = used_trunk_head;
		used_trunk_tail->next = NULL;
	}

	/* Create a dummy free trunk as head */
	{

		free_trunk_head = trunk_barrel_head->trunks + 2;	/* [2] */
		free_trunk_tail = trunk_barrel_head->trunks + 3;	/* [3] */
		first = trunk_barrel_head->trunks + 4;
		last = trunk_barrel_head->trunks + (BARREL_SIZE - 1);
		/* it's not empty, in some aspect */
		Trunk_set_use(&free_trunk_head->root);
		Trunk_set_use(&free_trunk_tail->root);
		free_trunk_head->pre = NULL;
		free_trunk_tail->next = NULL;
		free_trunk_head->next = first;
		first->pre = free_trunk_head;
		free_trunk_tail->pre = last;
		last->next = free_trunk_tail;
	}

	YL_MMU_ASSERT(used_trunk_head != used_trunk_tail);
	YL_MMU_ASSERT(used_trunk_head != free_trunk_head);
	YL_MMU_ASSERT(free_trunk_head != free_trunk_tail);
	YL_MMU_ASSERT(free_trunk_tail != first);
	YL_MMU_ASSERT(first != last);

	return 1;
}

YL_BOOL TrunkMetaAlloc_release(void)
{
	struct TrunkBarrel *t, *to_be_freed, *tail;

	if (!trunk_barrel_head)
		return 0;


	t = trunk_barrel_head;
	tail = trunk_barrel_tail;	/* for assertion only. */

	/* Disable the pointer as first as possible */
	trunk_barrel_head = trunk_barrel_tail = NULL;

	while (t) {
		to_be_freed = t;
		t = t->next;
		yl_mmu_free(to_be_freed);
	}

	YL_MMU_ASSERT(to_be_freed == tail);

	return 1;
}


static YL_BOOL check_and_init_trunk_manager(void)
{
	if (!trunk_barrel_head)
		return TrunkMetaAlloc_initialize();

	return 1;
}

struct Trunk *TrunkMetaAlloc_alloc(void)
{
	YL_BOOL success = 1;

	success = check_and_init_trunk_manager();
	if (!success)
		return NULL;

	success = expand_trunk_barrel_if_needed();
	if (!success)
		return NULL;

	{
		/* cut down the last free_trunk */
		struct TrunkWrapper *item = cut_next_trunk(free_trunk_head);

		/* connect to the used_trunk */
		insert_trunk(used_trunk_head, item);
		/* item->root.used = 1; */

		return (struct Trunk *) item;
	}
}

void TrunkMetaAlloc_free(struct Trunk *t)
{
	/* Each trunk is actually contained in a TrunkWrapper w same address. */
	struct TrunkWrapper *item = (struct TrunkWrapper *) t;
	{
		/* cut down target trunk */
		struct TrunkWrapper *t = cut_next_trunk(item->pre);

		/* connect to the used_trunk */
		insert_trunk(free_trunk_head, t);
		Trunk_unset_use(&t->root);
	}
}

struct Trunk *TrunkMetaAlloc_get_empty_head(void)
{
	YL_BOOL success = 1;

	success = check_and_init_trunk_manager();
	if (!success)
		return NULL;

	return (struct Trunk *) free_trunk_head;
}

struct Trunk *TrunkMetaAlloc_get_used_head(void)
{
	YL_BOOL success = 1;

	success = check_and_init_trunk_manager();
	if (!success)
		return NULL;

	return (struct Trunk *) used_trunk_head;
}