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
/* brief Rcache : Reversed cache pa_base -> trunk */

/* #include <assert.h> */


#ifndef _YL_MMU_RCACHE_H_
#define _YL_MMU_RCACHE_H_

#include "yl_mmu_rcache_public_class.h"


/* Init it and the memory for the object is provided outside. */
void Rcache_Init(struct Rcache *self);

/* Unit it after used. Won't release the memory for this object. */
void Rcache_Uninit(struct Rcache *self);

/* For debug. Return the number of cache entries in use. */
static INLINE unsigned int Rcache_get_use_count(struct Rcache *r) __attribute__ ((always_inline));

/* slot = hash(pa); */
static INLINE Rcache_slot_link_t Rcache_get_slot(struct Rcache *r, pa_t pa) __attribute__ ((always_inline));

/* Return index of cache entry; return 0 if not found. */
static INLINE Rcache_link_t Rcache_lookup(struct Rcache *r, pa_t pa_base) __attribute__ ((always_inline));

/* Insert the entry = (pa_base, trunk). pa_base must not in cache before insert. */
static INLINE Rcache_link_t Rcache_add(struct Rcache *r, pa_t pa_base,
					struct Trunk *trunk) __attribute__ ((always_inline));

/* Remove the entry = hash(pa_base). return 0 if not existed in cache. */
static INLINE Rcache_link_t Rcache_remove_by_pa_base(struct Rcache *r, pa_t pa_base) __attribute__ ((always_inline));



/* Private. Return the index of the slot. slot = hash(key). */
static INLINE Rcache_slot_link_t _Rcache_calc_hash(pa_t key) __attribute__ ((always_inline));

/* Private. Enqueue(q, item) */
static INLINE void _Rcache_enqueue(struct Rcache *r, struct RcacheQueue *q,
					Rcache_link_t item) __attribute__ ((always_inline));

/* Private. item = Dequeue(q) */
static INLINE Rcache_link_t _Rcache_dequeue(struct Rcache *r, struct RcacheQueue *q) __attribute__ ((always_inline));

/* Private. item = Enqueue( empty queue, item ); */
static INLINE void _Rcache_enqueue_to_empty_queue(struct Rcache *r, Rcache_link_t item) __attribute__ ((always_inline));

/* Private. item = Dequeue( empty queue ); */
static INLINE Rcache_link_t _Rcache_dequeue_from_empty_queue(struct Rcache *r) __attribute__ ((always_inline));



/* Implementation */
static INLINE unsigned int
Rcache_get_use_count(struct Rcache *r)
{
	return r->used_count;
}

static INLINE Rcache_slot_link_t Rcache_get_slot(struct Rcache *r, pa_t pa)
{
	Rcache_slot_link_t idx = _Rcache_calc_hash(pa);

	YL_MMU_ASSERT(idx < REMAPPER_RCACHE_SLOT_LENGTH);

	return idx;
}

static INLINE Rcache_link_t Rcache_lookup(struct Rcache *r, pa_t pa_base)
{
	struct RcacheSlot slot = r->slots[Rcache_get_slot(r, pa_base)];
	Rcache_link_t iter = slot.link;

	while (iter) {
		if (r->items[iter].pa_base == pa_base)
			return iter;
		/* Next */
		iter = r->items[iter].h_next;
	}

	return 0;
}

static INLINE Rcache_link_t Rcache_add(struct Rcache *r, pa_t pa_base, struct Trunk *trunk)
{
	Rcache_slot_link_t slot = Rcache_get_slot(r, pa_base);
	Rcache_link_t item;

	YL_MMU_ASSERT(Rcache_lookup(r, pa_base) == 0);

	/* dequeue */
	item = _Rcache_dequeue_from_empty_queue(r);
	if (!item || item >= REMAPPER_RCACHE_ITEM_LENGTH)
		return 0;

	if (slot >= REMAPPER_RCACHE_SLOT_LENGTH)
		return 0;

	/* r->items[ item ].prev = 0; */
	r->items[item].pa_base = pa_base;
	r->items[item].trunk = trunk;
	Rcache_set_count(r, item, 1);
	/* Insert as first */
	r->items[item].h_next = r->slots[slot].link;
	r->slots[slot].link = item;

	r->used_count++;

	return item;
}

static INLINE Rcache_link_t Rcache_remove_by_pa_base(struct Rcache *r, pa_t pa_base)
{
	Rcache_slot_link_t slot = Rcache_get_slot(r, pa_base);
	Rcache_link_t item = r->slots[slot].link;
	Rcache_link_t prev = 0;
	Rcache_link_t next;

	if (!r->slots[slot].link)
		return 0;

	/* YL_MMU_ASSERT( slot.link ); */
	/* YL_MMU_ASSERT( Rcache_lookup( r, pa ) ); */

	while (item) {
		if (r->items[item].pa_base == pa_base) {
			/* found */
			next = r->items[item].h_next;
			if (prev)
				r->items[prev].h_next = next;
			else
				r->slots[slot].link = next;

			/* enqueue */
			_Rcache_enqueue_to_empty_queue(r, item);
			r->used_count--;
			return item;
		}
		/* Next */
		prev = item;
		item = r->items[item].h_next;
	}

	return 0;
}



/* Private Implementation */

/** Assumption: 32 bit */
static INLINE Rcache_slot_link_t _Rcache_calc_hash(pa_t key)
{
	YL_MMU_ASSERT(sizeof(key) == 4);

	return
	    /* TYPE 2 */
	    /* 12..21 */
	    /* 22..31 */
	    (Rcache_slot_link_t)
	    (((key >> 22) ^ (key >> 12)
	     )
	     & (REMAPPER_RCACHE_SLOT_LENGTH - 1)
	    );

	/* TYPE 1 */
	/* 0.. 7 */
	/* 8..15 */
	/* 16..23 */
	/* 24..31 */
	/* (unsigned char) */
	/* (((key >> 24) + (key >> 16) + (key >>  8) + (key)) & 255 ); */

}

static INLINE void
_Rcache_enqueue(struct Rcache *r, struct RcacheQueue *q, Rcache_link_t item)
{
	Rcache_link_t q_prev;

	q_prev = q->tail;
	q->tail = item;

	if (q_prev)
		r->items[q_prev].q_next = item;
	else
		q->head = item;

}

static INLINE Rcache_link_t _Rcache_dequeue(struct Rcache *r, struct RcacheQueue *q)
{
	Rcache_link_t item, q_next;

	item = q->head;

	if (!item)
		return (Rcache_link_t) 0;

	q_next = r->items[item].q_next;
	q->head = q_next;

	/* if ( next ) */
	/* r->items[ next ].q_prev = r->empty_queue; */
	if (!q_next)
		q->tail = (Rcache_link_t) 0;

	/* r->items[ item ].q_prev = (Rcache_link_t)0; */
	r->items[item].q_next = (Rcache_link_t) 0;

	return item;
}

static INLINE void
_Rcache_enqueue_to_empty_queue(struct Rcache *r, Rcache_link_t item)
{
	_Rcache_enqueue(r, &r->empty_queue, item);
}

static INLINE Rcache_link_t _Rcache_dequeue_from_empty_queue(struct Rcache *r)
{
	return _Rcache_dequeue(r, &r->empty_queue);
}


#endif /* _YL_MMU_RCACHE_H_ */