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

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)
#include <windows.h>
#include <stdlib.h>
#else
#if defined(linux) && defined(linux_user_mode) /* linux user space */
#include <stdlib.h>
#else
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/memory.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#endif
#endif

#include "g3dkmd_api_mmu.h"
#include "g3dkmd_task.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_internal_memory.h"
#include "g3dkmd_iommu.h"
#include "g3dkmd_util.h"
#include "g3dkmd_pattern.h"
#include "mmu/yl_mmu_utility.h"
#include "profiler/g3dkmd_prf_kmemory.h"
#include "g3dkmd_memory_list.h"
#include "platform/g3dkmd_platform_pid.h" /* for g3dkmd_get_pid() */

#if defined(linux) && !defined(linux_user_mode)	/* linux kernel */
#if defined(USING_MISC_DEVICE) && defined(CONFIG_ARM64)
#include "g3dbase_ioctl.h" /* for g3dGetPlatformDevice() */
#endif
#endif


#ifdef YL_MMU_PATTERN_DUMP_V1
#include "g3d_api.h"
#include "g3dkmd_cfg.h"
#endif

#ifdef PATTERN_DUMP
#include "g3dkmd_file_io.h"
#endif

/* we skip MMU_USING_WCP due to m4u_alloc_mva will fail if va from __get_free_pages */
#if defined(linux) && !defined(linux_user_mode) && (USE_MMU != MMU_USING_WCP)
#define USE_PAGE_ALLOC_FUNC
#endif

static uint64_t g3dKmdAllocPhysical2(G3d_Resource_Type resType, unsigned int size)
{
	uint64_t retval;
#ifdef USE_PAGE_ALLOC_FUNC
	if ((1 << get_order(size)) * PAGE_SIZE == size) {
		void *va = (void *)__get_free_pages(GFP_KERNEL, get_order(size));

		if (va != NULL && ((unsigned long)va) < (unsigned long)high_memory) {
			retval = (uint64_t) __pa(va);
		} else {
			if (va != NULL)
				free_pages((unsigned long)va, get_order(size));
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MEM,
			       "g3dKmdAllocPhysical2 : wrong memory allocation 0x%p 0x%x\n", va,
			       size);
			retval = 0;
		}
	} else
#endif
	{
		retval = g3dKmdAllocPhysical(size);
	}

	return retval;
}

static void g3dKmdFreePhysical2(uint64_t addr, unsigned int size)
{
#ifdef USE_PAGE_ALLOC_FUNC
	if ((1 << get_order(size)) * PAGE_SIZE == size) {
		free_pages((unsigned long)__va(addr), get_order(size));
	} else
#endif
	{
		g3dKmdFreePhysical(addr, size);
	}
}

/* for KMD use only */
/* baseAlign must be power of 2 */
int allocatePhysicalMemory(G3dMemory *memory, unsigned int size,
			   unsigned int baseAlign, unsigned int flag)
{
	uint64_t pa;
	unsigned int expanded_size;
	void *hMmu = g3dKmdMmuGetHandle();
	unsigned char useMMU = (((flag) & G3DKMD_MMU_ENABLE) && hMmu);
	unsigned char sizePower2 = 0;
	unsigned char useDMA = ((flag) & G3DKMD_MMU_DMA);


	if (NULL == memory)
		return 0;

	g3dbaseCleanG3dMemory(memory);

	/* [brady todo] */
#ifdef YL_SECURE
	{
		unsigned int is_secure = flag & G3DKMD_MMU_SECURE;

		if (is_secure) {
			memory->resType =
			    (G3d_Resource_Type) ((unsigned)memory->resType | (flag << 5));
		}
	}
#endif

#ifdef USE_PAGE_ALLOC_FUNC
	sizePower2 = ((1 << get_order(size)) * PAGE_SIZE == size);
#endif

	expanded_size = (sizePower2 || useDMA) ? size : size + baseAlign;

#ifdef linux			/* PAGE_SIZE*1024 is the upper bound of kmalloc size */
#if !defined(linux_user_mode)
	if (size <= KMALLOC_UPPER_BOUND && expanded_size > KMALLOC_UPPER_BOUND
	    && (size % baseAlign == 0)) {
		expanded_size = size;
	}
#endif
#endif

#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))
	if (useMMU)
		pa = 0;
	else
		pa = g3dKmdAllocPhysical2(G3D_RES_OTHERS, expanded_size);

	memory->data0 = malloc(expanded_size);

#elif defined(linux)
#if defined(linux) && !defined(linux_user_mode)	/* linux kernel */
	if (useDMA) {
		dma_addr_t dma_handle;
#if defined(USING_MISC_DEVICE) && defined(CONFIG_ARM64)
		/* allocated memory shall be PAGE alignment */
		struct platform_device *g3dpdev = (struct platform_device *)g3dGetPlatformDevice();
		struct device *g3ddev = (NULL != g3dpdev) ? &(g3dpdev->dev) : NULL;

		memory->data0 =
		    dma_alloc_coherent(g3ddev, size, &dma_handle, GFP_KERNEL | __GFP_DMA);
#else
		memory->data0 = dma_alloc_coherent(NULL, size, &dma_handle, GFP_KERNEL | __GFP_DMA);
#endif /* USING_MISC_DEVICE */

		pa = (uint64_t) dma_handle;
		YL_KMD_ASSERT(((uintptr_t) memory->data0 & (PAGE_SIZE - 1)) == 0
			      && (pa & (PAGE_SIZE - 1)) == 0);
		memory->resType = G3D_RES_DMA;
	} else if (useMMU) {
		memory->data0 = (void *)G3DKMDVMALLOC(expanded_size);
		pa = (uint64_t)memory->data0;
		memory->resType = G3D_RES_VMALLOC;
	} else
#endif /* linux kernel (end) */
	{
		pa = g3dKmdAllocPhysical2(G3D_RES_OTHERS, expanded_size);
		memory->data0 = __va(pa);
		memory->resType = G3D_RES_OTHERS;
	}

	if (pa == 0) {
		/* fail to allocate */
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MEM, "Alloc physical failed (pa)\n");
		g3dbaseCleanG3dMemory(memory);
		return 0;
	}
#else
	/* real case: map from physical */
#endif

	if (memory->data0 == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MEM, "Alloc virtual failed (va)\n");
		g3dKmdFreePhysical2(pa, expanded_size);
		g3dbaseCleanG3dMemory(memory);
		return 0;
	}

	memory->size0 = expanded_size;
	memory->size = size;
	memory->data = (void *)G3DKMD_ALIGN((uintptr_t) memory->data0, (uintptr_t) baseAlign);
	memory->phyAddr = pa;

#ifdef PHY_ADDR_FROM_VIRT_ADDR
	{
		static unsigned int fakeAddr_for_64bits = 1 * 1024 * 1024;

		memory->hwAddr0 =
		    fakeAddr_for_64bits | (unsigned int)((uintptr_t) memory->
							 data0 & (PAGE_SIZE - 1));
		memory->hwAddr = G3DKMD_ALIGN(memory->hwAddr0, baseAlign);
		fakeAddr_for_64bits = memory->hwAddr0 + expanded_size;
		fakeAddr_for_64bits = G3DKMD_ALIGN(fakeAddr_for_64bits, PAGE_SIZE);
	}
#else
	if (useMMU) {
#if defined(linux) && !defined(linux_user_mode)
		if (useDMA)
			gExecInfo.pDMAMemory = memory;

#endif
		/* only this case, data0 and data will different but hwAddr0 = hwAddr */
		memory->hwAddr =
			memory->hwAddr0 =
				g3dKmdMmuMap(hMmu, memory->data, expanded_size, MMU_SECTION_DETECTION, flag);
#if defined(linux) && !defined(linux_user_mode)

		if (useDMA) {
#if (USE_MMU == MMU_USING_WCP)
			memory->pageArray = gExecInfo.pDMAMemory->pageArray;
#endif
			gExecInfo.pDMAMemory = NULL;
		}
#endif
	} else {
		memory->hwAddr0 = g3dKmdAllocHw_ex(pa, expanded_size);
		memory->hwAddr = G3DKMD_ALIGN(memory->hwAddr0, baseAlign);
	}
#endif

	if (useMMU) {
		memory->type = G3D_MEM_VIRTUAL;

#if defined(PATTERN_DUMP) && !defined(PHY_ADDR_FROM_VIRT_ADDR)
		memory->patPhyAddr = g3dKmdMmuG3dVaToPa(hMmu, memory->hwAddr);

#ifdef YL_MMU_PATTERN_DUMP_V1
		if (g3dKmdCfgGet(CFG_DUMP_ENABLE))
			memory->pa_pair_info =
			    g3dKmdMmuG3dVaToPaFull(hMmu, memory->hwAddr, memory->size);
#endif
#endif
	} else {
		memory->type = G3D_MEM_PHYSICAL;
#ifdef PATTERN_DUMP
		memory->patPhyAddr = (unsigned int)(pa + (memory->hwAddr - memory->hwAddr0));
#endif

#ifdef YL_MMU_PATTERN_DUMP_V1
		memory->pa_pair_info = 0;
#endif
	}

#ifdef PATTERN_DUMP
	if (gfAllocInfoKernal)
		g_pKmdFileIo->fPrintf(gfAllocInfoKernal, "al_%08x_%08x_%04x\n", memory->hwAddr,
				      memory->size0, baseAlign);
#endif

#ifdef SUPPORT_MPD
	CMSetMemory(memory->patPhyAddr, memory->hwAddr, memory->size, memory->data);
#endif

	KMDDPF_ENG(G3DKMD_LLVL_INFO | G3DKMD_MSG_MEM, "size 0x%x, va 0x%x, g3dva 0x%x\n",
		   memory->size, memory->data, memory->hwAddr);
	_g3dKmdProfilerKMemoryUsageAlloc(memory->size, memory->resType, current->tgid);

	/* Add to G3dMemory List. */
	_g3dKmdMemList_Add(NULL, (G3Dint) g3dkmd_get_pid(), memory->data0, memory->size0,
			   memory->hwAddr, G3D_Mem_Kernel);

	return 1;
}

/* for KMD use only */
void freePhysicalMemory(G3dMemory *memory)
{
	if (memory == NULL)
		return;

	if (memory->size0 == 0)
		return;


	KMDDPF_ENG(G3DKMD_LLVL_INFO | G3DKMD_MSG_MEM, "size 0x%x, va 0x%x, g3dva 0x%x\n",
		   memory->size, memory->data, memory->hwAddr);
	_g3dKmdProfilerKMemoryUsageFree(memory->size, memory->resType, current->tgid);
	/* Del from G3dMemory List */
	_g3dKmdMemList_Del(memory->data0);

#ifdef PATTERN_DUMP
	if (gfAllocInfoKernal)
		g_pKmdFileIo->fPrintf(gfAllocInfoKernal, "fr_%08x\n", memory->hwAddr);
#endif

#ifdef SUPPORT_MPD
	CMFreeMemory(memory->patPhyAddr, memory->hwAddr, memory->size, memory->data);
#endif

#ifndef PHY_ADDR_FROM_VIRT_ADDR
	if (memory->type == G3D_MEM_VIRTUAL)
		g3dKmdMmuUnmap(g3dKmdMmuGetHandle(), memory->hwAddr, memory->size0);
	else
		g3dKmdFreeHw_ex(memory->hwAddr0, memory->size0);
#endif

	if (memory->phyAddr) {
#if defined(linux) && !defined(linux_user_mode)
		if (memory->resType == G3D_RES_DMA) {
#if (USE_MMU == MMU_USING_WCP)
			struct sg_table *table = (void *)memory->pageArray;

			if (table) {
				sg_free_table(table);
				kfree(table);
			}
#endif
			{
#if defined(USING_MISC_DEVICE) && defined(CONFIG_ARM64)
				struct platform_device *g3dpdev =
				    (struct platform_device *)g3dGetPlatformDevice();
				struct device *g3ddev = (NULL != g3dpdev) ? &(g3dpdev->dev) : NULL;

				dma_free_coherent(g3ddev, memory->size, memory->data,
						  memory->phyAddr);
#else
				dma_free_coherent(NULL, memory->size, memory->data,
						  memory->phyAddr);
#endif /* USING_MISC_DEVICE && CONFIG_ARM64 */
			}
		} else if (memory->resType == G3D_RES_VMALLOC) {
			G3DKMDVFREE(memory->data0);
		} else
#endif
		{
			g3dKmdFreePhysical2(memory->phyAddr, memory->size0);
		}
	}

	memory->hwAddr0 = 0;
	memory->hwAddr = 0;
	memory->phyAddr = 0;

	if (memory->data0) {
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))
		free(memory->data0);
#elif defined(linux)
		/* kfree(memory->data0); */
#else
		/* real case: map from physical */
#endif
	}
#ifdef YL_MMU_PATTERN_DUMP_V1
	if (memory->pa_pair_info) {
		V_TO_P_ENTRY *entry = (V_TO_P_ENTRY *) memory->pa_pair_info;

		if (entry->pair)
			G3DKMDVFREE(entry->pair);
		G3DKMDVFREE(memory->pa_pair_info);
		memory->pa_pair_info = 0;
	}
#endif

	memory->data0 = 0;
	memory->data = 0;
}
