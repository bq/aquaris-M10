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

#include "yl_mmu_mapper_internal.h"
#include "yl_mmu_mapper_public_class.h"

#include "yl_mmu_common.h"
#include "yl_mmu_hw_table.h"
#include "yl_mmu_trunk_metadata_allocator.h"
#include "yl_mmu_trunk_internal.h"
#include "yl_mmu_bin_internal.h"

#include "yl_mmu_system_address_translator.h"

#include "yl_mmu_section_lookup.h"

/* #include <stdlib.h>     // For malloc */
			/* not available in kernel ? */




/* Use pa_section_lookup & pa_section_lookup_is_valid */
YL_BOOL _Mapper_is_continuous_section(struct Mapper *m, struct PaSectionLookupTable *lookup_table, va_t va)
{
	/* int     i; */
	va_t end_va = va + MMU_SMALL_SIZE * SMALL_LENGTH;
	pa_t *out_pa = lookup_table->entries;
	pa_t pre_pa;
	pa_t pa;
	YL_BOOL is_continuous = 1;

	YL_MMU_ASSERT(_Mapper_is_section_aligned(va));

	/* Init */
	pre_pa = va_to_pa(m, va);
	*out_pa = pre_pa;

	/* Next */
	va += MMU_SMALL_SIZE;
	++out_pa;

	/* Iteration */
	while (va < end_va) {
		/* Current */
		pa = va_to_pa(m, va);
		*out_pa = pa;
		if (pa != pre_pa + MMU_SMALL_SIZE)
			is_continuous = 0;

		/* Next */
		pre_pa = pa;
		va += MMU_SMALL_SIZE;
		++out_pa;
	}

	PaSectionLookupTable_set_valid(lookup_table);

	return is_continuous;
}

YL_BOOL
_Mapper_contain_continuous_section(struct Mapper *m, struct PaSectionLookupTable *lookup_table, va_t va,
				   size_t occupied_size)
{
	unsigned short index1_start = PageTable_calc_index1_section_aligned(va);
	unsigned short index1_aligned = (SMALL_LENGTH - index1_start) % SMALL_LENGTH;

	YL_MMU_ASSERT(occupied_size % MMU_SMALL_SIZE == 0);

	/* size check */
	if (occupied_size < MMU_SECTION_SIZE + MMU_SMALL_SIZE * index1_aligned)
		return 0;

	{
		size_t size_remained = occupied_size - MMU_SMALL_SIZE * index1_aligned;
		va_t va_aligned = (va + MMU_SECTION_SIZE - 1) & ~SECTION_OFFSET_MASK;
		size_t s = size_remained;

		/* skip the last one if fragment */
		while (s >= MMU_SECTION_SIZE) {
			YL_MMU_ASSERT(s % MMU_SMALL_SIZE == 0);

			if (_Mapper_is_continuous_section(m, lookup_table, va_aligned))
				return 1;

			va_aligned += MMU_SECTION_SIZE;
			s -= MMU_SECTION_SIZE;
		}
	}

	return 0;
}


/********************* Private Function Declarations *****************/
YL_BOOL register_table(struct Mapper *m, struct Trunk *t, va_t va, YL_BOOL section_detected, unsigned int flag)
{
	/* int offset        = (unsigned int)va & ( MMU_SMALL_SIZE - 1 ); */
	YL_BOOL succ = 1;
	va_t va_base = SmallPage_vabase_part(va);
	pa_t pa = va_to_pa(m, va_base);

	PageIndexRec index = t->index;
	/* next always exists, since we have pivot */
	unsigned short end_index0 = t->next->index.s.index0;
	unsigned short end_index1 = t->next->index.s.index1;
	/* G3dMemory *    page_mem             = & m->Page_mem; */
	/* struct FirstMetaTable *meta                 = m->Meta_space; */
	YL_BOOL ending_in_small = (end_index1 > 0);

#if MMU_MIRROR
	/* G3dMemory *    mirror_mem     = & m->Mirror_mem; */
#endif

	/* struct FirstPageTable *mirror               = (struct FirstPageTable *)m->Mirror_mem.data; */


	if (end_index1 == 0) {
		YL_MMU_ASSERT(end_index0);

		--end_index0;
		end_index1 = SMALL_LENGTH;
	}


	{
		YL_BOOL section_mode;
		YL_BOOL debug_section_used = 0;

		PaSectionLookupTable_set_invalid(&m->Pa_section_lookup_table);	/* lookup table haven't initialized */

		/* while not the ending section, AND if the ending section, not the ending small page */
		while (succ
		       && ((index.s.index0 < end_index0)
			   || ((index.s.index0 == end_index0) && (index.s.index1 < end_index1)))) {
			section_mode = section_detected;

			/* 1). If small page mode => Fill small page one by one */
			/* 2). If section mode    => Fill section page */

			if ((index.s.index0 == end_index0) && (ending_in_small)) {
				/* disable lookup table */
				PaSectionLookupTable_set_invalid(&m->Pa_section_lookup_table);
				section_mode = 0;
			} else if ((index.s.index1 != 0)) {
				section_mode = 0;
			}

			/* P => Q == !P || Q */
			/*
			YL_MMU_ASSERT( !section_mode || \
					( index1 == 0 && _Mapper_is_section_aligned( va_base ) ) );
			*/

			/* P => Q == !P || Q */
			YL_MMU_ASSERT(!section_mode || _Mapper_is_section_aligned(va_base));
			/* P => Q == !P || Q */
			YL_MMU_ASSERT(!section_mode || index.s.index1 == 0);

			if (section_mode
			    && !_Mapper_is_continuous_section(m, &m->Pa_section_lookup_table,
							      va_base)) {
				section_mode = 0;
			}
			/* c. Free page table and meta */
			/* 1). Reset that page entry directly if 1M */
			/* 2). Reset that page entry if 4K */
			/* a). Recycle that table if count=0 */

			/* [TODO] May require a loop to register all the pa in pa_list */
			/* Even pa is continuous, each page entry used must be filled. */
			/* loop is outside of this if-else */
			if (section_mode) {
				pa = PaSectionLookupTable_use_if_availabile(m,
									    &m->
									    Pa_section_lookup_table,
									    0, va_base);
				succ = Section_register(m, index.s.index0, pa, t, va_base, flag);
				YL_MMU_ASSERT(succ);

				if (succ) {
					va_base += MMU_SECTION_SIZE;
					++index.s.index0;
					debug_section_used = 1;

				}
			} else {
				pa = PaSectionLookupTable_use_if_availabile(m,
									    &m->
									    Pa_section_lookup_table,
									    index.s.index1,
									    va_base);
				succ = Small_register(m, index, pa, t, va_base, flag);
				YL_MMU_ASSERT(succ);

				if (succ) {
					va_base += MMU_SMALL_SIZE;

					if (++index.s.index1 == SMALL_LENGTH) {
						++index.s.index0;
						index.s.index1 = 0;
					}
				}
			}
		}		/* while */
		/* P => Q   !P ^ Q */
		YL_MMU_ASSERT(!section_detected || debug_section_used);
	}
	return succ;
}


YL_BOOL unregister_table(struct Mapper *m, struct Trunk *t)
{
	YL_BOOL succ = 1;
	PageIndexRec index = t->index;
	/* unsigned short index0              = t->index.s.index0; */
	/* unsigned short index1              = t->index.s.index1; */
	/* next always exists, since we have pivot */
	unsigned short end_index0 = t->next->index.s.index0;
	unsigned short end_index1 = t->next->index.s.index1;
	YL_BOOL ending_in_small = end_index1 > 0;
	G3dMemory *page_mem = &m->Page_mem;
	/* struct FirstMetaTable *meta = m->Meta_space; */
#if MMU_MIRROR
	/* G3dMemory *    mirror_mem = & m->Mirror_mem; */
#endif

	struct FirstPageTable *page = (struct FirstPageTable *) page_mem->data;
	/* struct FirstPageTable *mirror = (struct FirstPageTable *)mirror_mem->data; */

	YL_MMU_ASSERT(t->size > 0);

	if (end_index1 == 0) {
		YL_MMU_ASSERT(end_index0);

		--end_index0;
		end_index1 = SMALL_LENGTH;
	}

	{
		YL_BOOL section_mode;
		YL_BOOL succ = 1;

		while ((index.s.index0 < end_index0) ||
		       ((index.s.index0 == end_index0) && (index.s.index1 < end_index1))) {
			section_mode = 1;

			/* 1). If small page mode => Fill small page one by one */
			/* 2). If section mode    => Fill section page */

			if ((index.s.index1 != 0) ||
			    ((index.s.index0 == end_index0) && (ending_in_small))) {
				section_mode = 0;
			}
			/* P => Q == !P || Q */
			YL_MMU_ASSERT(!section_mode || (index.s.index1 == 0));

			if (section_mode
			    && SectionEntry_has_second_level(page->entries[index.s.index0])) {
				section_mode = 0;
			}
			/* c. Free page table and meta */
			/* 1). Reset that page entry directly if 1M */
			/* 2). Reset that page entry if 4K */
			/* a). Recycle that table if count=0 */

			if (section_mode) {
				YL_MMU_ASSERT(index.s.index1 == 0);
				succ = Section_unregister(m, index.s.index0) & succ;
				++index.s.index0;
			} else {
				succ = Small_unregister(m, index) & succ;
				if (++index.s.index1 == SMALL_LENGTH) {
					++index.s.index0;
					index.s.index1 = 0;
				}
			}
		}		/* while */
	}
	return succ;
}