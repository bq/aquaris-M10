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

#if defined(_WIN32)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#elif defined(linux) && defined(linux_user_mode)	/* linux user space */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#elif defined(linux)		/* linux kernel mode */
#include <linux/string.h>
#endif

/* #include "yl_mmu_common.h" */
#include "yl_mmu_kernel_alloc.h"
#include "yl_mmu_utility.h"
#include "yl_mmu_mapper.h"
#include "dump/yl_mmu_utility_statistics_trunk.h"

#include "g3d_memory.h"
#include "g3dkmd_define.h"
#include "g3dkmd_internal_memory.h"
#include "g3dkmd_iommu.h"
#include "g3dkmd_mmce.h"
#include "g3dkmd_cfg.h"

/* #include <time.h> */
/* #undef BOOL => change to YL_BOOL, no need */
/* #include <Windows.h> */

/* #define YL_MMU_FULL_SIZE (unsigned int)((256*1024*1024) - (4*1024))//256M */
/* #define YL_MMU_TEST_MAX_ALLOC_SIZE (unsigned int)(1 * 1024 * 1024)//1M */
/* #define YL_MMU_G_MEM_LENGTH 100 */

/* in sapphire, we use external IOMMU module, such that we have to define SHARE_ONE_COPY_OF_MMU */
#define SHARE_ONE_COPY_OF_MMU

#ifdef SHARE_ONE_COPY_OF_MMU
static struct Mapper *s_mapper;
static int s_mapper_ref;
#endif

#ifdef SHARE_ONE_COPY_OF_MMU
/* this function is implemeted if SHARE_ONE_COPY_OF_MMU on only. if off , we need more implementations */
int mmuMapperValid(struct Mapper *m)
{
	return (m != NULL && m == s_mapper) ? 1 : 0;
}
#endif

struct Mapper *mmuInit(g3d_va_t g3d_va_base)
{
	void *meta_table_space;
	G3dMemory page_mem;
#if MMU_MIRROR
	G3dMemory mirror_mem;
#endif

#ifdef G3DKMD_SUPPORT_IOMMU
	unsigned int page_ofst = ((g3dKmdCfgGet(CFG_MMU_VABASE) / MMU_SECTION_SIZE) * 4);
#endif

#ifdef SHARE_ONE_COPY_OF_MMU
	if (s_mapper_ref == 0) {
		unsigned int flags = 0;
#if defined(G3D_NONCACHE_BUFFERS)
		flags |= G3DKMD_MMU_DMA;
#endif
		s_mapper = (struct Mapper *) yl_mmu_alloc(Mapper_get_required_size_mapper());

		if (!s_mapper)
			return NULL;


		memset(&page_mem, 0, sizeof(G3dMemory));
#ifdef G3DKMD_SUPPORT_IOMMU
		if (!allocatePhysicalMemory
		    (&page_mem, Mapper_get_required_size_page_table() + page_ofst, 0x4000, flags)) {
			yl_mmu_free(s_mapper);
			return NULL;
		}

		memset(page_mem.data, 0, page_mem.size);
		page_mem.data = (void *)((uintptr_t) page_mem.data + page_ofst);
		page_mem.hwAddr = page_mem.hwAddr + page_ofst;
#else
		if (!allocatePhysicalMemory
		    (&page_mem, Mapper_get_required_size_page_table(), 0x4000, flags)) {
			yl_mmu_free(s_mapper);
			return NULL;
		}
#endif

#if MMU_MIRROR
		memset(&mirror_mem, 0, sizeof(G3dMemory));
		if (!allocatePhysicalMemory
		    (&mirror_mem, Mapper_get_required_size_page_table(), 0x4000, 0)) {
			freePhysicalMemory(&page_mem);
			yl_mmu_free(s_mapper);
			return NULL;
		}
#endif

		meta_table_space = yl_mmu_alloc(Mapper_get_required_size_meta_table());

		if (!meta_table_space) {
			freePhysicalMemory(&page_mem);
			yl_mmu_free(s_mapper);
			return NULL;
		}
#if MMU_MIRROR
		Mapper_init_mirror(s_mapper, g3d_va_base, page_mem, meta_table_space, mirror_mem,
				   0);
#else
		Mapper_init(s_mapper, g3d_va_base, page_mem, meta_table_space, 0);
#endif
		/* s_mapper->Page_table = page_mem; */

#ifdef G3DKMD_SUPPORT_IOMMU
		g3dKmdIommuInit(IOMMU_MOD_1, page_mem.hwAddr - page_ofst);
#ifdef G3DKMD_SUPPORT_EXT_IOMMU
		g3dKmdIommuInit(IOMMU_MOD_2, page_mem.hwAddr - page_ofst);
#endif
#endif
	}

	s_mapper_ref++;

	return s_mapper;
#else
	struct Mapper *m = (struct Mapper *) yl_mmu_alloc(Mapper_get_required_size_mapper());

	if (!m)
		return NULL;

	if (!allocatePhysicalMemory(&page_mem, Mapper_get_required_size_page_table(), 0x4000, 0)) {
		free(m);
		return NULL;
	}

	page_table_space = page_mem.data;/* (void *)(((unsigned int)page_table_space0 + 0x3FFFUL ) &
						( ~ 0x3FFFUL )); */
	meta_table_space = yl_mmu_alloc(Mapper_get_required_size_meta_table());

	if (!meta_table_space) {
		freePhysicalMemory(&page_mem);
		yl_mmu_free(m);
		return NULL;
	}

	Mapper_init(m, g3d_va_base, page_table_space, meta_table_space, 0);
	m->Page_table = page_mem;

	return m;
#endif
}

void mmuUninit(struct Mapper *m)
{
	void *meta;

	/* TODO need to be atomic */
#ifdef SHARE_ONE_COPY_OF_MMU
	s_mapper_ref--;
	if (s_mapper_ref != 0)
		return;

	YL_MMU_ASSERT(m == s_mapper);
	s_mapper = NULL;
#endif

	meta = Mapper_get_meta_space(m);

	freePhysicalMemory(&m->Page_mem);

#if MMU_MIRROR
	freePhysicalMemory(&m->Mirror_mem);
#endif

	Mapper_uninit(m);
	yl_mmu_free(m);
	yl_mmu_free(meta);

}


void mmuGetTable(struct Mapper *m, void **swAddr, void **hwAddr, unsigned int *size)
{
#ifdef G3DKMD_SUPPORT_IOMMU
	unsigned int page_ofst = ((g3dKmdCfgGet(CFG_MMU_VABASE) / MMU_SECTION_SIZE) * 4);

	*swAddr = (void *)((uintptr_t) m->Page_mem.data - page_ofst);
	*hwAddr = (void *)((uintptr_t) m->Page_mem.hwAddr - page_ofst);
#else
	*swAddr = m->Page_mem.data;
	*hwAddr = m->Page_mem.hwAddr;
#endif
	*size =
	    FIRST_PAGE_OCCUPIED_ALIGNED + ((g3dKmdCfgGet(CFG_MMU_VABASE) / MMU_SECTION_SIZE) * 4);
}


unsigned int mmuG3dVaToPa(struct Mapper *m, g3d_va_t g3d_va)
{
	return Mapper_g3d_va_to_pa(m, g3d_va);
}

void *mmuGetMapper(void)
{
#ifdef SHARE_ONE_COPY_OF_MMU
	return (void *)s_mapper;
#else
	return NULL;
#endif
}

#if 0				/* MMU_LOOKBACK_VA */
void mmuGetVirtualTable(struct Mapper *m, void **swAddr, unsigned int *hwAddr, unsigned int *size)
{
	*swAddr = m->lookback_mem.data;
	*hwAddr = m->lookback_mem.hwAddr;
	*size = m->lookback_mem.size;
}

unsigned int mmuG3dVaToVa(struct Mapper *m, g3d_va_t g3d_va)
{
	return Mapper_g3d_va_to_va(m, g3d_va);
}
#endif


#if MMU_MIRROR
void mmuGetMirrorTable(struct Mapper *m, void **swAddr, void **hwAddr, unsigned int *size)
{
	*swAddr = m->Mirror_mem.data;
	*hwAddr = (void *)(uintptr_t) m->Mirror_mem.hwAddr;
	*size = m->Mirror_mem.size;
}

unsigned int mmuG3dVaToG3dVa(struct Mapper *m, g3d_va_t g3d_va)
{
	return Mapper_g3d_va_to_g3d_va(m, g3d_va);
}
#endif
