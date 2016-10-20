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
/* brief Iterator is a fast way to manipulate the memory block inside an allocation. */

#ifndef _YL_MMU_ITERATOR_H_
#define _YL_MMU_ITERATOR_H_

#include "yl_mmu_iterator_public_class.h"
#include "yl_mmu_index_helper.h"
#include "yl_mmu_hw_table_helper.h"

/* Sample Usage: */
/* @code */
/*
Remapper_memset(struct Mapper *m, g3d_va_t ptr, unsigned char value, size_t size )
{
	pa_t pa;
	va_t va;
	int length;
	struct RemapperIterator iterRec;

	RemapperIterator_Init( & iterRec, m, ptr, size );
	while( RemapperIterator_valid( & iterRec ) ) {
		pa = RemapperIterator_get_pa( & iterRec );
		length = RemapperIterator_next( & iterRec );

		va = pa_to_va( pa );

		yl_mmu_memset( (void *)va, value, size);
	}
	return 1;
}
*/
/* @endcode */



/* This signature looks weired, but you can alloc instance on stack and initialize it, */
/* to prevent dynamic memory allocation. */
void RemapperIterator_Init(struct RemapperIterator *iter, struct Mapper *m, g3d_va_t g3d_va, size_t size);

/* Just set to 0 for all field, but still good practice to call it. */
void RemapperIterator_Uninit(struct RemapperIterator *iter);

/* Next page. Depends on page boundary */
static INLINE int RemapperIterator_next(struct RemapperIterator *iter) __attribute__ ((always_inline));

/* Next 4K alignment */
static INLINE int RemapperIterator_next_4k(struct RemapperIterator *iter) __attribute__ ((always_inline));

/* Next 4K or 1M boundary. Depends on pages. */
static INLINE int RemapperIterator_next_1M(struct RemapperIterator *iter) __attribute__ ((always_inline));

/*  */
static INLINE YL_BOOL RemapperIterator_valid(struct RemapperIterator *iter) __attribute__ ((always_inline));

/* Get physical address stored in page table. No checking for valid. */
static INLINE pa_t RemapperIterator_get_pa(struct RemapperIterator *iter) __attribute__ ((always_inline));

/*  */
static INLINE int RemapperIterator_size_to_next_boundary(struct RemapperIterator *iter) __attribute__ ((always_inline));

/*  */
static INLINE int _RemapperIterator_update_size_to_next_boundary(struct RemapperIterator *iter)
			__attribute__ ((always_inline));

/*/ Assumption: Index must be ready. */
static INLINE int
_RemapperIterator_update_size_to_next_boundary(struct RemapperIterator *iter)
{
	int offset;
	int remained = iter->size - iter->cursor;
	/* if 4k alignment */
	/* get the 4K offset */
	/* if not 4k alignment */
	/* get the 1M offset */

	YL_MMU_ASSERT(remained >= 0);

	if ((iter->index.s.index0 >= SECTION_LENGTH) || (remained <= 0)) {
		iter->size_to_next_boundary = 0;
		return 0;
	}

	if (Section_has_second_level(iter->Page_space, iter->index.s.index0)) {
		offset = 4096 - SmallPage_offset_part(iter->cursor);
		offset = offset ? offset : 4096;
	} else {
		offset = 1048576 - Section_offset_part(iter->cursor);
		offset = offset ? offset : 1048576;
	}

	iter->size_to_next_boundary = (offset <= remained) ? offset : remained;
	return iter->size_to_next_boundary;
}


static INLINE int
RemapperIterator_size_to_next_boundary(struct RemapperIterator *iter)
{
	return iter->size_to_next_boundary;
}


static INLINE pa_t RemapperIterator_get_pa(struct RemapperIterator *iter)
{
	YL_MMU_ASSERT(RemapperIterator_valid(iter));

	return PageTable_index_to_pa(iter->m->Page_space, iter->index,
				     iter->g3d_address + iter->cursor);
}


static INLINE int
RemapperIterator_next(struct RemapperIterator *iter)
{
	int offset;
	int num_pages;

	if (!iter->size_to_next_boundary)
		return 0;

	offset = iter->size_to_next_boundary;
	num_pages = (offset + 4096 - 1) / 4096;
	PageIndex_add(&iter->index, num_pages);

	iter->cursor += offset;

	_RemapperIterator_update_size_to_next_boundary(iter);

	YL_MMU_ASSERT(iter->cursor % 4096 == 0);
	YL_MMU_ASSERT(iter->size_to_next_boundary % 4096 == 0);

	return offset; /* number of bytes advanced */
}

static INLINE int
RemapperIterator_next_4k(struct RemapperIterator *iter)
{
	int offset;

	if (!iter->size_to_next_boundary)
		return 0;

	offset = iter->size_to_next_boundary % 4096;
	offset = offset ? offset : 4096;

	PageIndex_add(&iter->index, 1);

	iter->cursor += offset;
	iter->size_to_next_boundary -= offset;

	if (!iter->size_to_next_boundary)
		_RemapperIterator_update_size_to_next_boundary(iter);

	YL_MMU_ASSERT(iter->cursor % 4096 == 0);

	return offset;
}


static INLINE int
RemapperIterator_next_1M(struct RemapperIterator *iter)
{
	return RemapperIterator_next(iter);
}


static INLINE YL_BOOL RemapperIterator_valid(struct RemapperIterator *iter)
{
	return (iter->size_to_next_boundary > 0) && (iter->cursor >= iter->size);
}

#endif /* _YL_MMU_ITERATOR_H_ */
