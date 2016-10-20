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
/* brief BinList class. */

#ifndef _YL_MMU_BIN_INTERNAL_H_
#define _YL_MMU_BIN_INTERNAL_H_

#include "yl_mmu_common.h"
#include "yl_mmu_bin_public_class.h"


/* Init before use. Please pass in the memory. */
void BinList_initialize(struct BinList *self);
/* Uninit after use. Won't release the memory. */
void BinList_release(struct BinList *self);
/* return the bin where bin.size exactly <= size. */
/** bin[0].size == 4k */
struct Bin *BinList_floor(struct Bin bins[], size_t size);
/* Nearly best-fit-first search for suitable trunk */
/** The returned bin is large enough to contain size */
struct Trunk *BinList_scan_for_first_empty_trunk(struct BinList *b, size_t size);
/* Only Set in_use to true. Due to algorithm change. */
void BinList_attach_trunk_to_occupied_list(struct BinList *b, struct Trunk *trunk);
/* Put trunk back to empty list. */
void BinList_attach_trunk_to_empty_list(struct BinList *b, struct Trunk *trunk);
/* Cut down the first trunk in this bin. Error if nothing inside the bin. */
struct Trunk *Bin_cut_down_first_trunk(struct Bin *bin);
/* Return the ceiling index for given size. */
static INLINE int BinList_index_ceiling(size_t size) __attribute__ ((always_inline));
/* Return the ceiling index for given size. But minimum size is a section. */
static INLINE int BinList_index_ceiling_section(size_t size) __attribute__ ((always_inline));
/* Return the floor index for given size. */
static INLINE int BinList_index_floor(size_t size) __attribute__ ((always_inline));
/* Return the floor index for given size. But minimum size is a section. */
static INLINE int BinList_index_floor_section(size_t size) __attribute__ ((always_inline));
/* Return both floor index and the ceiling index for given size. */
static INLINE void BinList_index_rounding(size_t size, int *out_index_floor,
					  int *out_index_ceiling)__attribute__ ((always_inline));
/* Return both floor index and the ceiling index for given size. But minimum size is a section. */
static INLINE void BinList_index_rounding_section(size_t size, int *out_index_floor,
						  int *out_index_ceiling) __attribute__ ((always_inline));
/* Helper. Return the first trunk of this bin. */
static INLINE struct Trunk *Bin_first_trunk(struct Bin *bin) __attribute__ ((always_inline));

/* [Section Combining] Search for the empty trunk which can be used for section combining. */
struct Trunk *BinList_scan_for_empty_trunk_section_fit(struct BinList *bin_list, size_t size,
					       unsigned short index1_start,
					       unsigned short index1_tail);

/* Private. Hook trunk on BinList according to trunk.size */
void _BinList_attach_trunk(struct Bin bins[], struct Trunk *trunk);

/* Private. Find the index of ceiling bin for given size. Start at (idx, bin_size) */
static INLINE int _BinList_index_ceiling_generic(size_t size, int idx,
				size_t bin_size) __attribute__ ((always_inline));

/* Private. Find the index of floor bin for given size. Start at (idx, bin_size) */
static INLINE int _BinList_index_floor_generic(size_t size, int idx,
				size_t bin_size) __attribute__ ((always_inline));

/* Private. Return both floor index and the ceiling index. */
static INLINE void
_BinList_index_rounding_generic(size_t size, int idx, size_t bin_size,
				 int *out_index_floor, int *out_index_ceiling)__attribute__ ((always_inline));






/* Implementation */
static INLINE struct Trunk *Bin_first_trunk(struct Bin *bin)
{
	return bin->trunk_head.right_sibling;
}

static INLINE int
_BinList_index_ceiling_generic(size_t size, int idx, size_t bin_size)
{
	while (bin_size < size) {
		bin_size <<= 1;
		++idx;
	}

	YL_MMU_ASSERT(bin_size >= size);

	return idx;
}

static INLINE int
_BinList_index_floor_generic(size_t size, int idx, size_t bin_size)
{
	size_t next_bin_size = bin_size << 1;

	while (next_bin_size <= size) {
		next_bin_size <<= 1;
		++idx;
	}

	YL_MMU_ASSERT(next_bin_size > size);

	return idx;
}

static INLINE void
_BinList_index_rounding_generic(size_t size, int idx, size_t bin_size,
				 int *out_index_floor, int *out_index_ceiling)
{
	while (bin_size < size) {
		bin_size <<= 1;
		++idx;
	}

	YL_MMU_ASSERT(bin_size >= size);

	*out_index_floor = (bin_size == size) ? idx : idx - 1;
	*out_index_ceiling = idx;
}

static INLINE int
BinList_index_ceiling(size_t size)
{
	return _BinList_index_ceiling_generic(size, 0, MMU_SMALL_SIZE);
}

static INLINE int
BinList_index_ceiling_section(size_t size)
{
	return _BinList_index_ceiling_generic(size, SECTION_BIT - SMALL_BIT,
					      MMU_SECTION_SIZE);
}

static INLINE int
BinList_index_floor(size_t size)
{
	return _BinList_index_floor_generic(size, 0, MMU_SMALL_SIZE);
}

static INLINE int
BinList_index_floor_section(size_t size)
{
	return _BinList_index_floor_generic(size, SECTION_BIT - SMALL_BIT,
					    MMU_SECTION_SIZE);
}

static INLINE void
BinList_index_rounding(size_t size, int *out_index_floor, int *out_index_ceiling)
{
	_BinList_index_rounding_generic(size, 0, MMU_SMALL_SIZE, out_index_floor,
					out_index_ceiling);
}

static INLINE void
BinList_index_rounding_section(size_t size, int *out_index_floor, int *out_index_ceiling)
{
	_BinList_index_rounding_generic(size, SECTION_BIT - SMALL_BIT, MMU_SECTION_SIZE,
					out_index_floor, out_index_ceiling);
}

#endif /* _YL_MMU_BIN_INTERNAL_H_ */
