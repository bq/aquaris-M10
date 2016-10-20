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
/* brief Definition of BinList class */

#ifndef _YL_MMU_BIN_PUBLIC_CLASS_H_
#define _YL_MMU_BIN_PUBLIC_CLASS_H_

#include "yl_mmu_trunk_public_class.h"

/********************* Data Structure *****************/
/* Haven't count the pivot bin. */
#define BIN_LENGTH      (MAX_ADDRESSING_BIT - MIN_ADDRESSING_BIT + 1)
/* Handy shortcut. Last bin before the pivot bin */
#define BIN_LAST_INDEX  (BIN_LENGTH - 1)

/* A Bin contains an dummy trunk. */
/** Reuse the trunk properties for the bin properties
 *  The design simplifies the insertion/deletion of trunks inside the bin.
 */
struct Bin {
	struct Trunk trunk_head;	/* circular / doubly-linklist list */
	/* size_t      size;        /// use the size in trunk_head */

};

/* A BinList categorizes trunks into different bin by their size. */
/** Using Pivot.
 */
struct BinList {
	/* 28 - 12 + 1 + 1 // the last one is pivot */
	/* struct Bin occupied[BIN_LENGTH + 1 ];  // Mindos occupied */

    /** 28 - 12 + 1 + 1
     * the last one is pivot.
     * Trunks inside a bin is stored in doubly-linklist
     */
	struct Bin empty[BIN_LENGTH + 1];

    /** First Trunk is unique. It doesn't belong the bin list.
      * And should be always the topmost trunk in the entire addressing space.
      * It always empty. i.e. in_use == 0
      */
	struct Trunk *first_trunk;

};

#endif /* _YL_MMU_BIN_PUBLIC_CLASS_H_ */