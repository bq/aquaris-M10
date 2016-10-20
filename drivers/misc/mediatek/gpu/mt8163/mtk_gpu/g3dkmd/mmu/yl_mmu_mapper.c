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

#include "yl_mmu_mapper.h"
#include "yl_mmu_mapper_helper.h"

#include "yl_mmu_index_helper.h"
#include "yl_mmu_hw_table.h"
#include "yl_mmu_hw_table_helper.h"
#include "yl_mmu_trunk_internal.h"
#include "yl_mmu_rcache.h"
#include "yl_mmu_mapper_internal.h"

#include "yl_mmu_kernel_alloc.h"

#include "yl_mmu_system_address_translator.h"

#include "yl_mmu_trunk_metadata_allocator.h"

#include "g3d_memory.h"
#include "g3dkmd_internal_memory.h"
#include "g3dkmd_api_mmu.h"
#include "g3dkmd_iommu.h"

#ifdef linux

#if defined(linux_user_mode)	/* linux user space */
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif /* !PAGE_SIZE */
#else /* ! linux_user_mode */
#include <linux/slab.h>
#endif /* linux user mode */

#else
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif /* !PAGE_SIZE */
#endif



/*********************** Mapper Related ************************************/
/* g3d_va_base can be 0. Since the first 4k will never be alloc */
#if MMU_MIRROR
/*
Mapper_init_mirror(struct Mapper *m, g3d_va_t g3d_va_base, void * page_space,
			void * meta_space, void * page_space_mirror, int cache_enable )
*/
void Mapper_init_mirror(struct Mapper *m, const g3d_va_t g3d_va_base, /* < Starting address for our own G3dVa */
			const G3dMemory page_mem, /* < hw see the physical, and address in lvl.1 table is physical. */
			const void *meta_space, /* < virtual */
			const G3dMemory mirror_mem, /* < mirror means va to va */

			const int cache_enable) /* < The 4k cache */
{
	struct FirstPageTable *space = (struct FirstPageTable *) mirror_mem.data;

	YL_MMU_ASSERT(m);

	Mapper_init(m, g3d_va_base, page_mem, meta_space, cache_enable);

	YL_MMU_ASSERT(((uintptr_t) space & 0x3FFF) == 0); /* alignment */

	m->Mirror_mem = mirror_mem;
	m->Mirror_space = space;
	yl_mmu_zeromem((void *)space, Mapper_get_required_size_page_table());
}
#endif

void Mapper_init(struct Mapper *m, const g3d_va_t g3d_va_base, /* < Starting address for our own G3dVa */
		 const G3dMemory page_mem, /* < hw see the physical, and address in lvl.1 table is physical. */
		 const void *meta_space, /* < virtual */

		 const int cache_enable) /* < The 4k cache */
{
	struct FirstPageTable *page_space = (struct FirstPageTable *)page_mem.data;

	YL_MMU_ASSERT(m);
	YL_MMU_ASSERT(page_space);
	YL_MMU_ASSERT(meta_space);

#ifndef G3DKMD_SUPPORT_IOMMU
	YL_MMU_ASSERT(((unsigned int)page_space & 0x3FFF) == 0); /* alignment */
#endif

	yl_mmu_zeromem((void *)m, sizeof(struct Mapper));

	m->G3d_va_base = g3d_va_base;
	m->Page_mem = page_mem;
	m->Page_space = page_space;
	m->Meta_space = (struct FirstMetaTable *) meta_space;
	m->Cache_enable = cache_enable;
	m->is_pa_given = 0;

#if MMU_MIRROR
	g3dbaseCleanG3dMemory(&m->Mirror_mem);
	m->Mirror_space = NULL;
#endif

	yl_mmu_zeromem((void *)page_space, Mapper_get_required_size_page_table());
	yl_mmu_zeromem((void *)meta_space, Mapper_get_required_size_meta_table());

	BinList_initialize(&m->Bin_list);

	/* TODO other initialization */
	Rcache_Init(&m->Rcache);

#if MMU_LOOKBACK_VA
	if (!allocatePhysicalMemory(&m->lookback_mem, Mapper_get_required_size_page_table(), PAGE_SIZE, 0)) {
		m->Page_space_lookback = 0;
	} else {
		m->Page_space_lookback = m->lookback_mem.data;
		yl_mmu_zeromem((void *)m->Page_space_lookback,
			       Mapper_get_required_size_page_table());
	}
#endif
}

void Mapper_uninit(struct Mapper *m)
{
	BinList_release(&m->Bin_list);
	Rcache_Uninit(&m->Rcache);
	TrunkMetaAlloc_release();	/* Take care the dependeny */
#if MMU_LOOKBACK_VA
	freePhysicalMemory(&m->lookback_mem);
#endif
}


void Mapper_free_all_memory(struct Mapper *m)
{
	struct FirstPageTable *page_space = (struct FirstPageTable *) m->Page_mem.data;
	struct FirstMetaTable *meta_space = m->Meta_space;

	BinList_release(&m->Bin_list);
	Rcache_Uninit(&m->Rcache);
	TrunkMetaAlloc_release();	/* Take care the dependeny */

	TrunkMetaAlloc_initialize();
	Rcache_Init(&m->Rcache);
	BinList_initialize(&m->Bin_list);

	yl_mmu_zeromem((void *)page_space, Mapper_get_required_size_page_table());
	yl_mmu_zeromem((void *)meta_space, Mapper_get_required_size_meta_table());

#if MMU_MIRROR
	{
		struct FirstPageTable *space = (struct FirstPageTable *) m->Mirror_mem.data;

		if (space)
			yl_mmu_zeromem((void *)space, Mapper_get_required_size_page_table());

	}
#endif

#if MMU_LOOKBACK_VA
	{
		if (m->Page_space_lookback) {
			yl_mmu_zeromem((void *)m->Page_space_lookback,
				       Mapper_get_required_size_page_table());
		}
	}
#endif
}


/*
 * Usage :
 *   void *yl_vp = mapper_mapping(m, vp, size);
 */
g3d_va_t
Mapper_map(struct Mapper *m,
	   const va_t va,
	   const size_t size, const YL_BOOL could_be_section, const unsigned int flag)
{
	YL_MMU_ASSERT(size > 0);
	YL_MMU_ASSERT(size < MAX_ADDRESSING_SIZE);
	YL_MMU_ASSERT(size <= MAX_ALLOCATION_SIZE);

	/* Release mode. */
	if (size <= 0 || size >= MAX_ADDRESSING_SIZE || size > MAX_ALLOCATION_SIZE)
		return 0;


	{
		struct Trunk *t = NULL;
		struct Trunk *t2 = NULL;
		struct Trunk *first_trunk = m->Bin_list.first_trunk;
		size_t occupied_size;
		unsigned short index1_start = 0;
		unsigned short index1_tail = 0;
		YL_BOOL succ = 1;
		YL_BOOL has_section = 0;
		YL_BOOL use_first_trunk = 0;
		/* YL_BOOL                added_front         = 0; */
		g3d_va_t g3d_va = 0;
/* The priority: */
/* A). Section Mapping ( Make HW faster ) */
/* B). Small page packing ( Use TLB wisely, and save some addressing space ) */
/* C). Find free but potentially space (around 2* size ~  4*size) ( Fast ) */
/* D). Search for just-fit page (around size ~ 2*size) */
		occupied_size = PageTable_calc_size_in_small_blocks(va, size);

	/****************** Cache ***********************/
		if (occupied_size == MMU_SMALL_SIZE && m->Cache_enable) {
			Rcache_link_t iter;
			pa_t pa_base;

			pa_base = va_to_pa_for_rcache(SmallPage_vabase_part(va));
			iter = Rcache_lookup(&m->Rcache, pa_base);

			if (iter && (iter < REMAPPER_RCACHE_ITEM_LENGTH)) {
				struct Trunk *curr;

				/* exists */
				curr = m->Rcache.items[iter].trunk;
				Rcache_count_inc(&m->Rcache, iter);
				g3d_va = Trunk_get_g3d_va(curr, m->Page_space, m->G3d_va_base, va);

				return g3d_va;
			}
		}
	/****************** Cache ***********************/


		/* I think searching empty bins is costly than checking continuous section. */
		/* So I filter by this checking first. */
		if (could_be_section
		    && _Mapper_contain_continuous_section(m, &m->Pa_section_lookup_table, va,
							  occupied_size)) {
			index1_start = PageTable_calc_index1_section_aligned(va);
			index1_tail =
			    (index1_start + occupied_size / MMU_SMALL_SIZE - 1) % SMALL_LENGTH;

			t = BinList_scan_for_empty_trunk_section_fit(&m->Bin_list, occupied_size,
								     index1_start, index1_tail);

			if (!t && (occupied_size + MMU_SECTION_SIZE <= first_trunk->size)) {
				t = first_trunk;
				use_first_trunk = 1;
			}
		}

		if (t) {
			/* can occupied whole section. */
			has_section = 1;
		} else {
			/* find first empty bin */
			t = BinList_scan_for_first_empty_trunk(&m->Bin_list, occupied_size);
		}

		if (!t) {
			if (occupied_size >= first_trunk->size) {
				/* No large enough space available */
				return 0;
			}

			YL_MMU_ASSERT(occupied_size < first_trunk->size);

			t = first_trunk;
			use_first_trunk = 1;
		}

		YL_MMU_ASSERT(t);
		YL_MMU_ASSERT(t->size >= occupied_size);

		if (!use_first_trunk)
			_Trunk_siblings_detach(t);

		/* Valid even if has section. */
		if (t->size == occupied_size) {
			YL_MMU_ASSERT(!use_first_trunk);

			t2 = t;
		} else { /* t->size > occupied_size */
			if (has_section) {
				/* cut off padding and make it aligned */
				PageIndexRec end_idx = t->next->index;
				unsigned short index1_padding =
				    PageIndex_tail_padding_for_align(end_idx.s.index1, index1_tail);

				if (index1_padding) {
					struct Trunk *t3;

					t3 = Trunk_split_tail(t, MMU_SMALL_SIZE * index1_padding);

					if (t3 == NULL)
						return 0;

					BinList_attach_trunk_to_empty_list(&m->Bin_list, t3);
				}
			}

			t2 = Trunk_split_tail(t, occupied_size);
			/* Attach empty trunk with new size back to empty list */
			if (!use_first_trunk)
				BinList_attach_trunk_to_empty_list(&m->Bin_list, t);
		}


		if (t2 == NULL)
			return 0;


		BinList_attach_trunk_to_occupied_list(&m->Bin_list, t2);

		/* Register table */
		succ = register_table(m, t2, va, has_section, flag);
		YL_MMU_ASSERT(succ);
		if (!succ)
			return 0;

		/* Register cache */
		if (t2->size == MMU_SMALL_SIZE && m->Cache_enable) {
			pa_t pa_base = SmallPage_index_to_pa(m->Page_space, t2->index, 0);

			Rcache_add(&m->Rcache, pa_base, t2);
		}

		g3d_va = Trunk_get_g3d_va(t2, m->Page_space, m->G3d_va_base, va);

#ifdef G3DKMD_SUPPORT_IOMMU
		if (g3d_va && size)
			g3dKmdIommuInvalidTlbRange(IOMMU_MOD_1, g3d_va, g3d_va + size - 1);
#endif

		return g3d_va;
	}
}


void Mapper_unmap(struct Mapper *m, g3d_va_t g3d_va, size_t size)
{
	YL_MMU_ASSERT(m != NULL);
	YL_MMU_ASSERT(g3d_va);
	/* YL_MMU_ASSERT( g3d_va - m->G3d_va_base >= 0 ); */
	YL_MMU_ASSERT(g3d_va - m->G3d_va_base <= MAX_ADDRESSING_SIZE);



	/* return ; */


	{
		YL_BOOL succ = 1;
		PageIndexRec index;
		struct Trunk *trunk;
		struct Trunk *touched;

		/* 1 a. find target page index */
		{
			PageTable_g3d_va_to_index(m->G3d_va_base, g3d_va, &index);

			trunk = PageTable_trunk_by_index(m->Page_space, m->Meta_space, index);

			YL_MMU_ASSERT(trunk);
			YL_MMU_ASSERT(trunk->size > 0);
		}

		/* Unregister cache */
		if (trunk->size == MMU_SMALL_SIZE) {
			pa_t pa_base;
			Rcache_link_t iter;

			/* if( 0 == Trunk_decrease_reference_counter( trunk ) ) */
			pa_base = SmallPage_index_to_pa(m->Page_space, trunk->index, 0);
			iter = Rcache_lookup(&m->Rcache, pa_base);

			if (iter && (iter < REMAPPER_RCACHE_ITEM_LENGTH)) {
				if (Rcache_count_dec(&m->Rcache, iter)) {
					/* reduce ref cnt but do nothing. */
					return;
				}

				Rcache_remove_by_pa_base(&m->Rcache,
							 SmallPage_pabase_part(pa_base));
			}
		}
		/* 1.find this trunk */
		/* a. find target page index */
		/* c. Free page table and meta */
		/* 1). Reset that page entry directly if 1M */
		/* 2). Reset that page entry if 4K */
		/* a). Recycle that table if count=0 */
		/* b. using meta page */
		/* 1). determine section (1mb) or small page (4kb) */
		/* 2). Find target trunk */
		/* d. Got target trunk */
		/* e. free target trunk */
		/* 1). Free that trunk */


		/* Unregister table */
		succ = unregister_table(m, trunk);

		if (!succ)
			return;	/* failed */


		touched = Trunk_free(trunk, m->Bin_list.first_trunk);

		if (touched)
			BinList_attach_trunk_to_empty_list(&m->Bin_list, touched);

#ifdef G3DKMD_SUPPORT_IOMMU
		if (g3d_va && size)
			g3dKmdIommuInvalidTlbRange(IOMMU_MOD_1, g3d_va, g3d_va + size - 1);
#endif
	}
}

/*
void Mapper_get_page_space0(struct Mapper *m )
{
	return m->Page_space0;
}
*/

void *Mapper_get_page_space(struct Mapper *m)
{
	return m->Page_space;
}

void *Mapper_get_meta_space(struct Mapper *m)
{
	return m->Meta_space;
}


pa_t Mapper_g3d_va_to_pa(struct Mapper *m, g3d_va_t g3d_va)
{
	YL_MMU_ASSERT(g3d_va >= m->G3d_va_base);
	return PageTable_g3d_va_to_pa(&m->Page_mem, g3d_va - m->G3d_va_base);
}

#if MMU_MIRROR
pa_t Mapper_g3d_va_to_g3d_va(struct Mapper *m, g3d_va_t g3d_va)
{
	YL_MMU_ASSERT(g3d_va >= m->G3d_va_base);
	YL_MMU_ASSERT(m->Mirror_mem.data);
	return PageTable_g3d_va_to_pa(&m->Mirror_mem, g3d_va - m->G3d_va_base) + m->G3d_va_base;
}
#endif

#if MMU_LOOKBACK_VA
va_t Mapper_g3d_va_to_va(struct Mapper *m, g3d_va_t g3d_va)
{
	if (g3d_va < m->G3d_va_base)
		return 0;

	YL_MMU_ASSERT(g3d_va >= m->G3d_va_base);
	YL_MMU_ASSERT(m->Page_space_lookback);
	return PageTable_g3d_va_to_pa(&m->lookback_mem, g3d_va - m->G3d_va_base);
}
#endif

g3d_va_t
Mapper_map_given_pa(struct Mapper *m, va_t va, pa_t pa, size_t size, YL_BOOL could_be_section,
		    unsigned int flag)
{
	g3d_va_t g3d_va;
	va_t va2;

	m->is_pa_given = 1;

	va2 = (va & ((~0UL) - 4095)) | (pa & 4095);

	YL_MMU_ASSERT((va2 & 4095) == (pa & 4095));
	YL_MMU_ASSERT((va & 4095) == (pa & 4095));

	m->GIVEN_PA_va_base = va2;
	m->GIVEN_PA_pa_base = pa;

	g3d_va = Mapper_map(m, va, size, could_be_section, flag);

	m->is_pa_given = 0;

#if MMU_MIRROR_ROUTE
	YL_MMU_ASSERT(g3d_va == Mapper_g3d_va_to_g3d_va(m, g3d_va));
#endif

#if MMU_LOOKBACK_VA
	YL_MMU_ASSERT(va == Mapper_g3d_va_to_va(m, g3d_va));
#endif

	YL_MMU_ASSERT(pa == Mapper_g3d_va_to_pa(m, g3d_va));

	return g3d_va;
}


/* Mapper */
#if 0
Mapper_new(g3d_va_t g3d_va_base, void *page_space, void *meta_space, int cache_enable)
{
	struct Mapper *m;

	m = (struct Mapper *)yl_mmu_alloc(sizeof(struct Mapper));
	if (m == NULL)
		return NULL;

	Mapper_init(m, g3d_va_base, page_space, meta_space, cache_enable);

	/* Atomic when possible */

	return m;
}
#endif


/* Won't release memory for page table and meta table.  Should I? */
#if 0
void
Mapper_delete(struct Mapper **mapper_ref)
{
	struct Mapper *m = *mapper_ref;

	/* Atomic when possible */
	*mapper_ref = NULL;

	Mapper_uninit(*mapper_ref);
	yl_mmu_free(m);
}
#endif
