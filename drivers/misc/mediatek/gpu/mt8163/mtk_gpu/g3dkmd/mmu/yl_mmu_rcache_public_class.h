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
/* brief Class Definition. Reversed cache */


#ifndef _YL_MMU_RCACHE_PUBLIC_CLASS_H_
#define _YL_MMU_RCACHE_PUBLIC_CLASS_H_

#include "yl_mmu_common.h"
#include "yl_mmu_hw_table_public_class.h"


/* 1.. 2047 can be used. */
#define REMAPPER_RCACHE_ITEM_LENGTH 2048
/* change _Rcache_calc_hash() if u change this */
#define REMAPPER_RCACHE_SLOT_LENGTH 1024

typedef unsigned short Rcache_link_t;	/*< an index type for entry */
typedef unsigned short Rcache_slot_link_t;	/*< an index type for slot */

/* Private. Each slot contains a entry linklist. */
struct RcacheSlot {
	Rcache_link_t link;

};

/* Private. Each cache entry is a (pa_t, trunk) pair. */
struct RcacheEntry {
	pa_t pa_base;	/*< Key */
	struct Trunk *trunk;	/*< Value */
	Rcache_link_t h_next;	/*< hash Link */
	Rcache_link_t q_next;	/*< queue Link */

};

/* Private. Reference count for cache entry. */
struct RcacheEntryCount {
	unsigned short ref_cnt;	/*< Reference Count. */
	/*< Number of allocation currently use this entry. */

};

/* Private. Queue to contain cache entries. Order of entries is not important */
struct RcacheQueue {
	Rcache_link_t head;	/*< head point to first element */
	Rcache_link_t tail;	/*< tail point to last element */

};

/* Public. Main class for Rcache. */
/** size is 4 bytes aligned. **/
struct Rcache {
	struct RcacheEntry items[REMAPPER_RCACHE_ITEM_LENGTH];	/*< item[0] won't be used */
	struct RcacheEntryCount item_cnts[REMAPPER_RCACHE_ITEM_LENGTH];	/*< item[0] won't be used */
	struct RcacheSlot slots[REMAPPER_RCACHE_SLOT_LENGTH];

	struct RcacheQueue empty_queue;	/*< contains free entries. */
	unsigned int used_count;	/*< debug purpose. */

};


/* Get reference count */
static INLINE unsigned short Rcache_get_count(struct Rcache *r, Rcache_link_t iter) __attribute__ ((always_inline));
/* Set reference count */
static INLINE void Rcache_set_count(struct Rcache *r, Rcache_link_t iter,
					unsigned short value) __attribute__ ((always_inline));
/* reference count ++ */
static INLINE unsigned short Rcache_count_inc(struct Rcache *r, Rcache_link_t iter) __attribute__ ((always_inline));
/* reference count -- */
static INLINE unsigned short Rcache_count_dec(struct Rcache *r, Rcache_link_t iter) __attribute__ ((always_inline));


/* Implementation */

static INLINE unsigned short
Rcache_get_count(struct Rcache *r, Rcache_link_t iter)
{
	return r->item_cnts[iter].ref_cnt;
}

static INLINE void
Rcache_set_count(struct Rcache *r, Rcache_link_t iter, unsigned short value)
{
	r->item_cnts[iter].ref_cnt = value;
}

static INLINE unsigned short
Rcache_count_inc(struct Rcache *r, Rcache_link_t iter)
{
	return ++(r->item_cnts[iter].ref_cnt);
}

static INLINE unsigned short
Rcache_count_dec(struct Rcache *r, Rcache_link_t iter)
{
	return --(r->item_cnts[iter].ref_cnt);
}


#endif /* _YL_MMU_RCACHE_PUBLIC_CLASS_H_ */
