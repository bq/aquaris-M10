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

#if defined(linux) && !defined(linux_user_mode)
#include <linux/module.h>
#endif
#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dbase_buffer.h"
#include "g3dkmd_internal_memory.h"

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)
#elif defined(linux) && defined(linux_user_mode) /* linux user space */
#include "string.h"
#include <stdlib.h> /* malloc */
#else /* linux kernel space */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
/* #include <linux/kthread.h> */
#include <linux/memory.h>
#include <linux/errno.h>
/* #include <linux/types.h> */
#include <linux/string.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#endif

#include "g3dkmd_util.h"
#include "g3dkmd_api_mmu.h"
#include "g3dkmd_pattern.h"
#include "g3dkmd_cfg.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_file_io.h"
#ifdef G3DKMD_PHY_HEAP
#include "g3dkmd_fakemem.h"
#endif

#include "mmu/yl_mmu_utility.h"

#ifdef G3DKMD_SUPPORT_MSGQ
#include "g3dkmd_msgq.h"
#endif

#if ((USE_MMU == MMU_USING_M4U) || (USE_MMU == MMU_USING_WCP))
#include "g3dkmd_internal_memory.h"
#endif

#if (USE_MMU == MMU_USING_M4U)
#include "m4u/m4u.h"
#endif

#if (USE_MMU == MMU_USING_WCP)
#include "sapphire_reg.h"

#if defined(FPGA_mt6752_fpga) || defined(FPGA_fpga8163) || defined(FPGA_fpga8163_64) || defined(MT8163)
#include <m4u.h>
#include <m4u_port.h>
#define  iommu_alloc_mva     m4u_alloc_mva
#define  iommu_dealloc_mva   m4u_dealloc_mva
#define  iommu_cache_sync    m4u_cache_sync
#elif defined(FPGA_mt6592_fpga)
#include "mach/gpu_mmu.h"
#include "mach/gpu_mmu_port.h"
#define  iommu_alloc_mva     gpu_mmu_alloc_mva
#define  iommu_dealloc_mva   gpu_mmu_dealloc_mva
#define  iommu_cache_sync    gpu_mmu_cache_sync
#else
#error "currently mt6752/mt6592 FPGA are supported only"
#endif
#endif

#include "mmu/yl_mmu_system_address_translator.h" /*For va_to_pa_after_mmap() */


static __DEFINE_SEM_LOCK(mmuLock);

#if (USE_MMU == MMU_USING_GPU) || defined(G3DKMD_FAKE_M4U)

#ifdef PATTERN_DUMP
static unsigned int _G3dVaToPa_pattern(void *m, unsigned int g3d_va)
{
#if MMU_MIRROR_ROUTE
	return mmuG3dVaToG3dVa((struct Mapper *) m, g3d_va);
#else
	return mmuG3dVaToPa((struct Mapper *) m, g3d_va);
#endif
}
#endif /* PATTERN_DUMP */

static unsigned int _G3dVaToPa(void *m, unsigned int g3d_va)
{
#if MMU_VIRTUAL_ROUTE
	return Mapper_g3d_va_to_va((struct Mapper *) m, g3d_va);
#else
	return mmuG3dVaToPa((struct Mapper *) m, g3d_va);
#endif
}

void *g3dKmdMmuInit_ex(unsigned int va_base)
{
	return (void *)mmuInit(va_base);
}

void g3dKmdMmuUninit_ex(void *m)
{
	mmuUninit((struct Mapper *) m);
}

void g3dKmdMmuGetTable_ex(void *m, void **swAddr, void **hwAddr, unsigned int *size)
{
#if defined(PATTERN_DUMP) || (defined(SUPPORT_MPD) && defined(_WIN32))
	if (
#if defined(SUPPORT_MPD) && defined(_WIN32)
		   G3DKMD_TRUE ||
#endif
		   g3dKmdCfgGet(CFG_DUMP_ENABLE)
	    ) {
#if MMU_MIRROR_ROUTE
		mmuGetMirrorTable((struct Mapper *) m, swAddr, hwAddr, size);
#else
		mmuGetTable((struct Mapper *) m, swAddr, hwAddr, size);
#endif
	} else
#endif
	{
		mmuGetTable((struct Mapper *) m, swAddr, hwAddr, size);
	}
}

unsigned int g3dKmdMmuG3dVaToPa_ex(void *m, unsigned int g3d_va)
{
	unsigned int pa = 0;

#if defined(PATTERN_DUMP) || (defined(SUPPORT_MPD) && defined(_WIN32))
	if (
#if defined(SUPPORT_MPD) && defined(_WIN32)
		   G3DKMD_TRUE ||
#endif
		   g3dKmdCfgGet(CFG_DUMP_ENABLE)
	    ) {
		pa = _G3dVaToPa_pattern(m, g3d_va);
	} else
#endif
	{
		pa = _G3dVaToPa(m, g3d_va);
	}

	return pa;
}

unsigned int g3dKmdMmuG3dVaToVa_ex(void *m, unsigned int g3d_va)
{
	unsigned int pa = 0;

	pa = Mapper_g3d_va_to_va((struct Mapper *) m, g3d_va);

	return pa;
}

#ifdef YL_SECURE
unsigned int
g3dKmdMmuMap_ex(void *m,
		void *va,
		unsigned int size,
		int could_be_section, unsigned int flag, unsigned int secure_flag)
#else
unsigned int
g3dKmdMmuMap_ex(void *m, void *va, unsigned int size, int could_be_section, unsigned int flag)
#endif
{
	g3d_va_t g3d_va = 0;

#if (defined(PATTERN_DUMP) && !defined(G3D_HW)) || (defined(SUPPORT_MPD) && defined(_WIN32))
	if (
#if defined(SUPPORT_MPD) && defined(_WIN32)
		   G3DKMD_TRUE ||
#endif
		   g3dKmdCfgGet(CFG_DUMP_ENABLE)
	    ) {
		unsigned int pa = 0;
		/* For pattern & C-Model */
#ifdef YL_SECURE
		/* [brady todo] pass secure_flag to g3dKmdPatternRemapPhysical, */
		/* don't need to shift 5 bits right */
		pa = g3dKmdPatternRemapPhysical(G3D_MEM_VIRTUAL, va, size, secure_flag);

#else
		pa = g3dKmdPatternRemapPhysical(G3D_MEM_VIRTUAL, va, size);
#endif

		YL_MMU_ASSERT(pa);

		if (!pa)
			return 0;

		g3d_va = Mapper_map_given_pa((struct Mapper *) m, (va_t) va, (pa_t) pa, (size_t) size,
					could_be_section, flag);
	} else
#endif
	{
		g3d_va = Mapper_map((struct Mapper *) m, (va_t) va, (size_t) size, could_be_section, flag);
	}

	return (unsigned int)g3d_va;
}

void g3dKmdMmuUnmap_ex(void *m, unsigned int g3d_va, unsigned int size)
{
	g3d_va_t g3d_va_cast = (g3d_va_t) g3d_va;

	/*
	 * The releasing-skiping is for two reasons :
	 * the resource could be released before dump. ex
	 * alloc -> use -> release -> dump
	 * in mirror mode, the physical memory could be overlaped.
	 */
#if defined(PATTERN_DUMP)
	if (g3dKmdCfgGet(CFG_DUMP_ENABLE)) {
#ifdef G3DKMD_PHY_HEAP
		if (g3dKmdCfgGet(CFG_PHY_HEAP_ENABLE)) {
			unsigned int pa;

			pa = g3dKmdMmuG3dVaToPa_ex(m, g3d_va);

			mmFree(pa, size);

			if (mmuMapperValid(m))
				Mapper_unmap((struct Mapper *) m, g3d_va_cast, size);
		} else {
			/* Don't release MMU allocation */
			/* to prevent pattern address overlap. */
		}
#endif
	} else
#endif
	{
#ifdef SUPPORT_MPD
#ifdef G3DKMD_PHY_HEAP
		if (g3dKmdCfgGet(CFG_PHY_HEAP_ENABLE)) {
			unsigned int pa;

			pa = g3dKmdMmuG3dVaToPa_ex(m, g3d_va);

			mmFree(pa, size);
		}
#endif
#endif
		if (mmuMapperValid(m))
			Mapper_unmap((struct Mapper *) m, g3d_va_cast, size);
	}
}

unsigned int g3dKmdAllocHw_ex(uint64_t phyAddr, unsigned int size)
{
	unsigned int hwAddr;

#ifdef G3DKMD_FAKE_HW_ADDR
	hwAddr = mmAlloc(size);
#else
	hwAddr = (unsigned int)phyAddr;
	if ((uint64_t) hwAddr != phyAddr)
		return 0;
#endif

#ifdef G3DKMD_SUPPORT_MSGQ
	g3dKmdMsgQAllocMem(hwAddr, phyAddr, size);
#endif

	return hwAddr;
}

void g3dKmdFreeHw_ex(unsigned int hwAddr, unsigned int size)
{
#ifdef G3DKMD_FAKE_HW_ADDR
	mmFree(hwAddr, size);
#endif

#ifdef G3DKMD_SUPPORT_MSGQ
	g3dKmdMsgQFreeMem(hwAddr, size);
#endif
}
#endif /* (USE_MMU == MMU_USING_GPU) || defined(G3DKMD_FAKE_M4U) */

#if (USE_MMU == MMU_USING_WCP)
m4u_callback_ret_t g3dKmdMmuTransFaultCallback(int port, unsigned int mva, void *data)
{
#if 0 /* def YL_DEBUG_MMU_TRANS_FAULT */
	int instIdx = g3dKmdGetNormalExeInstIdx();
	struct g3dExeInst *pInst = gExecInfo.exeList[instIdx];
	unsigned int readPtr = g3dKmdRegGetReadPtr(pInst->hwChannel);
	struct g3dHtInfo *ht = pInst->pHtDataForSwWrite->htInfo;
	unsigned int rdIndex = (readPtr - ht->htCommandBaseHw) / HT_CMD_SIZE;
	struct g3dHtCommand *pHtCmd1 = &ht->htCommand[rdIndex];
	struct g3dHtCommand *pHtCmd2 = (rdIndex == 0) ? &ht->htCommand[MAX_HT_COMMAND-1] : &ht->htCommand[rdIndex-1];
#endif

	gExecInfo.mmuTransFault++;

#if 0 /* def YL_DEBUG_MMU_TRANS_FAULT */
	pr_debug("fault %d rdptr=(0x%x,%d) currentTask=%d previousTask=%d\n",
			gExecInfo.mmuTransFault, readPtr, rdIndex,
			pHtCmd1->command, pHtCmd2->command);
#endif
	return M4U_CALLBACK_HANDLED;
}
#endif

G3DKMD_APICALL void *G3DAPIENTRY g3dKmdMmuGetHandle(void)
{
	return gExecInfo.mmu;
}

G3DKMD_APICALL void *G3DAPIENTRY g3dKmdMmuInit(unsigned int va_base)
{
	void *m = NULL;

	__SEM_LOCK(mmuLock);

	if (g3dKmdCfgGet(CFG_MMU_ENABLE)) {
#if (USE_MMU == MMU_USING_GPU)
		g3dKmdIommuRegCallback(g3dKmdRiuWriteIommu);
		m = g3dKmdMmuInit_ex(va_base);

#elif (USE_MMU == MMU_USING_M4U)
		struct M4U_PORT port;

		m4u_register_regwirte_callback((m4u_client_t *)m, g3dKmdRiuWriteIommu);

		m = (void *)g3d_m4u_create_client();
		YL_KMD_ASSERT(m);

		port.ePortID = M4U_PORT_GPU;
		port.Direction = 0;
		port.Distance = 1;
		port.domain = 0;
		port.Security = 0;
		port.Virtuality = G3DKMD_TRUE;
		if (m4u_config_port(&port))
			YL_KMD_ASSERT(0);
#elif (USE_MMU == MMU_USING_WCP)
		g3dKmdRegWrite(G3DKMD_REG_MFG, 0x18, 0x1, 0xFFFFFFFF, 0);
		m = (void *)m4u_create_client();
		YL_KMD_ASSERT(m);
		m4u_register_regwirte_callback((m4u_client_t *)m, g3dKmdRiuWriteIommu);
		m4u_register_fault_callback(M4U_PORT_GPU, g3dKmdMmuTransFaultCallback, m);
#else
		YL_KMD_ASSERT(0);
#endif
	}

	__SEM_UNLOCK(mmuLock);

	return m;
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuUninit(void *m)
{
	__SEM_LOCK(mmuLock);

	if (m) {
#if (USE_MMU == MMU_USING_GPU)
		g3dKmdMmuUninit_ex(m);
#elif (USE_MMU == MMU_USING_M4U)
		g3d_m4u_destroy_client((m4u_client_t *)m);
#elif (USE_MMU == MMU_USING_WCP)
		m4u_destroy_client((m4u_client_t *)m);
#else
		YL_KMD_ASSERT(0);
#endif
	}

	__SEM_UNLOCK(mmuLock);
}

G3DKMD_APICALL int G3DAPIENTRY g3dKmdMmuGetUsrBufUsage(unsigned int *pSize)
{
	int ret;

#if (USE_MMU == MMU_USING_WCP) && defined(ENABLE_WCP_MMU_GET_USR_BUF_USAGE)
	__SEM_LOCK(mmuLock);

	ret = m4u_get_usr_buf_usage(pSize);

	__SEM_UNLOCK(mmuLock);
#else
	ret = 1;
#endif
	return ret;
}


#ifdef PATTERN_DUMP
G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuGetTable(void *m, void **swAddr, void **hwAddr,
						  unsigned int *size)
{
	__SEM_LOCK(mmuLock);
#if (USE_MMU == MMU_USING_GPU)
	g3dKmdMmuGetTable_ex(m, swAddr, hwAddr, size);
#elif (USE_MMU == MMU_USING_M4U)
	m4u_get_pgd((m4u_client_t *)m, M4U_PORT_GPU, swAddr, hwAddr, size);
#elif (USE_MMU == MMU_USING_WCP)
	m4u_get_pgd((m4u_client_t *)m, M4U_PORT_GPU, swAddr, hwAddr, size);
#else
	YL_KMD_ASSERT(0);
#endif
	__SEM_UNLOCK(mmuLock);
}

static unsigned int _g3dKmdMmuG3dVaToPa(void *m, unsigned int g3d_va)
{
	unsigned int pa = 0;

#if (USE_MMU == MMU_USING_GPU)
	pa = g3dKmdMmuG3dVaToPa_ex(m, g3d_va);
#elif (USE_MMU == MMU_USING_M4U)
	pa = (unsigned int)m4u_mva_to_pa((m4u_client_t *)m, M4U_PORT_GPU, g3d_va);
#elif (USE_MMU == MMU_USING_WCP)
	pa = (unsigned int)m4u_mva_to_pa((m4u_client_t *)m, M4U_PORT_GPU, g3d_va);
#else
	YL_KMD_ASSERT(0);
#endif
	return pa;
}

G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdMmuG3dVaToPa(void *m, unsigned int g3d_va)
{
	unsigned int pa = 0;

	__SEM_LOCK(mmuLock);

	pa = _g3dKmdMmuG3dVaToPa(m, g3d_va);

	__SEM_UNLOCK(mmuLock);

	return pa;
}
#endif


#ifdef YL_MMU_PATTERN_DUMP_V1
G3DKMD_APICALL void *G3DAPIENTRY g3dKmdMmuG3dVaToPaFull(void *m, unsigned int g3d_va, unsigned int size)
{
	V_TO_P_ENTRY *entry;

	entry = (V_TO_P_ENTRY *) G3DKMDVMALLOC(sizeof(V_TO_P_ENTRY));
	memset((void *)entry, 0, sizeof(V_TO_P_ENTRY));
	if (!entry)
		return NULL;

#if ((USE_MMU == MMU_USING_GPU) || (USE_MMU == MMU_USING_M4U))
	{
/*  // method1, parse the PageTable Manually

    void* pageTbl;
    pageTbl = ((struct Mapper *) m)->Page_mem.data;
    g3d_va -= g3dKmdCfgGet(CFG_MMU_VABASE);

    // now we have pageTbl as page table, g3d_va and its size
    // renference to PageTable_g3d_va_to_pa for the detail
*/

/*  // method2, use a for loop to call _g3dKmdMmuG3dVaToPa, in order to get all (pa, size) pairs
*/
		/* int i; */
		unsigned int offset_mask = 0;
		unsigned int unit_size = 0, continuous_size, current_size;
		unsigned int pa, pa_head, next_pa, continuous_pa_head = 0, va_ptr;
		unsigned int (*pair)[2];
		unsigned int buffer_size;

		unit_size = MMU_SMALL_SIZE;	/* 4K */
		offset_mask = SMALL_PAGE_OFFSET_MASK;	/* 0xFFF */


		buffer_size = (size / unit_size) + 2;

		pair = (unsigned int (*)[2])G3DKMDVMALLOC(buffer_size * sizeof(unsigned int[2]));
		memset((void *)pair, 0, (buffer_size * sizeof(unsigned int[2])));
		if (!pair) {
			G3DKMDVFREE(entry);
			return NULL;
		}

		continuous_size = 0;	/* size of continuous pa */
		current_size = 0;	/* current page size, <4k for head/tail */
		entry->num = -1;
		next_pa = 0xFFFFFFFF;	/* next pa if continuous */
		va_ptr = g3d_va;

		while (size != 0) {
			pa = g3dKmdMmuG3dVaToPa(m, va_ptr);
			pa_head = pa & ~(offset_mask);	/* head of current 4K */
			current_size = unit_size - (pa & offset_mask);
			if (size < current_size)
				current_size = size;
			if (pa_head == next_pa) {	/* continuous */
				continuous_size += current_size;
			} else {
				continuous_pa_head = pa;
				continuous_size = current_size;

				entry->num++;
			}

			pair[entry->num][0] = continuous_pa_head;
			pair[entry->num][1] = continuous_size;

			next_pa = pa_head + unit_size;
			va_ptr += current_size;
			size -= current_size;
		}
		entry->num++;
		entry->pair = pair;
	}
#else
	entry->num = 0;
	/* entry->pair = NULL; */
#endif

	return (void *)entry;
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuPaPairBufInfo(void *m, unsigned int g3d_va,
						       unsigned int size, unsigned int *buffer_size,
						       unsigned int *unit_size,
						       unsigned int *offset_mask)
{
#if ((USE_MMU == MMU_USING_GPU) || (USE_MMU == MMU_USING_M4U))
	{
		unsigned int index0 =
		    PageTable_index0_from_g3d_va(g3d_va - ((struct Mapper *) m)->G3d_va_base);
		struct FirstPageTable *page0 = (FirstPageTable) ((struct Mapper *) m)->Page_mem.data;
		struct FirstPageEntry entry0;

		entry0 = page0->entries[index0];
		if (entry0.Common.type == FIRST_TYPE_SECTION) {
			*unit_size = MMU_SECTION_SIZE;	/* 1M */
			*offset_mask = SECTION_OFFSET_MASK;	/* 0xFFFFF */
		} else if (entry0.Common.type == FIRST_TYPE_TABLE) {
			*unit_size = MMU_SMALL_SIZE;	/* 4K */
			*offset_mask = SMALL_PAGE_OFFSET_MASK;	/* 0xFFF */
		} else {
			YL_MMU_ASSERT(0);
		}

		*buffer_size = ((size / *unit_size) + 2) * sizeof(unsigned int[2]);
	}
#else
	*unit_size = *offset_mask = *buffer_size = 0;
	return;
#endif
}
#endif /* YL_MMU_PATTERN_DUMP_V1 */

#ifdef YL_MMU_PATTERN_DUMP_V2
G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuG3dVaToPaChunk(void *m, unsigned int data,
							unsigned int size, unsigned int *chunk_pa,
							unsigned int *chunk_size)
{
	unsigned int /*ptr */ next_pa = 0xFFFFFFFF;
	unsigned int /*ptr */ g3dva_ptr = data;
	unsigned int size_left = size;

	while (size_left != 0) {
		unsigned int offset_mask = 0;
		unsigned int unit_size = 0;
		unsigned int /*ptr */ pa;
		unsigned int current_size;

		unit_size = MMU_SMALL_SIZE;	/* 4K */
		offset_mask = SMALL_PAGE_OFFSET_MASK;	/* 0xFFF */

		/* g3dva => pa */
		pa = g3dKmdMmuG3dVaToPa(m, g3dva_ptr);

		/* current_size */
		current_size = unit_size - (pa & offset_mask);
		if (size_left < current_size)
			current_size = size_left;

		if (next_pa == 0xFFFFFFFF) { /* first pa */
			*chunk_pa = pa;
			*chunk_size = current_size;
		} else if (pa == next_pa) { /* continuous */
			*chunk_size += current_size;
		} else { /* done chunk */

			break;
		}

		next_pa = pa + current_size;
		g3dva_ptr += current_size;
		size_left -= current_size;
	}
	/* return chunk_pa & chunk_size */
}
#endif

/*
G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuFreeAll( void *m )
{
    Mapper_free_all_memory( (struct Mapper *)m );
#ifdef CMODEL
    g3dKmdPatternFreeAll();
#endif
}
*/
G3DKMD_APICALL unsigned int G3DAPIENTRY
g3dKmdMmuMap(void *m, void *va, unsigned int size, int could_be_section, unsigned int flag)
{
	g3d_va_t g3d_va = 0;

	__SEM_LOCK(mmuLock);

#if (USE_MMU == MMU_USING_GPU)
#ifdef YL_SECURE
	g3d_va = g3dKmdMmuMap_ex(m, pid, va, size, could_be_section, flag, flag);
#else /* !YL_SECURE */
	g3d_va = g3dKmdMmuMap_ex(m, va, size, could_be_section, flag);
#endif /* YL_SECURE */

#elif ((USE_MMU == MMU_USING_M4U) || (USE_MMU == MMU_USING_WCP))
	{
		unsigned int prot = M4U_PROT_READ | M4U_PROT_WRITE;
#ifndef G3D_NONCACHE_BUFFERS
		/* prot |= (flag & G3DKMD_MMU_SHARE) ? M4U_PROT_CACHE : 0; */
		/* prot |= (flag & G3DKMD_MMU_SECURE) ? M4U_PROT_SEC : 0; */
#endif
#if (USE_MMU == MMU_USING_M4U)
#ifdef YL_SECURE
		/* [brady to do] to pass flag information to g3dKmdMmuMap_ex */
		if (g3d_m4u_alloc_mva((m4u_client_t *)m, M4U_PORT_GPU, va, NULL, size, prot, flag, &g3d_va)) {
#else
		if (g3d_m4u_alloc_mva((m4u_client_t *)m, M4U_PORT_GPU, va, NULL, size, prot, 0, &g3d_va)) {
#endif
			YL_MMU_ASSERT(0);

#else /* MMU_USING_WCP */
#if defined(linux) && !defined(linux_user_mode)
		if ((flag & G3DKMD_MMU_DMA) != 0) {
			struct sg_table *table;
			struct scatterlist *sg;
			struct page *page;

			YL_KMD_ASSERT(gExecInfo.pDMAMemory && size == gExecInfo.pDMAMemory->size);
			/* table = kmalloc(sizeof(struct sg_table), GFP_KERNEL); */
			table = G3DKMDMALLOC(sizeof(struct sg_table));

			if (unlikely(NULL == table)) {
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMU, "kalloc fail\n");
				g3dkmd_backtrace();
				YL_KMD_ASSERT(table != NULL);
				__SEM_UNLOCK(mmuLock);
				return 0; /* MmuMap fail */
			}

			if (gExecInfo.pDMAMemory != NULL && table != NULL) {

				sg_alloc_table(table,
					       (gExecInfo.pDMAMemory->size + PAGE_SIZE - 1) / PAGE_SIZE, GFP_KERNEL);
				sg = table->sgl;
				/* sg_dma_address(sg) = gExecInfo.pDMAMemory->phyAddr; */
				/* sg_dma_len(sg) = (gExecInfo.pDMAMemory->size + PAGE_SIZE-1) & ~(PAGE_SIZE - 1); */
				page = phys_to_page(gExecInfo.pDMAMemory->phyAddr);
				sg_set_page(sg, page,
					    (gExecInfo.pDMAMemory->size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1), 0);

				if (iommu_alloc_mva((m4u_client_t *)m,
						     M4U_PORT_GPU, 0, table, size, prot, 0, &g3d_va)) {
					KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMU, "kalloc fail\n");
					g3dkmd_backtrace();
					YL_MMU_ASSERT(0);
					__SEM_UNLOCK(mmuLock);
					return 0;
				}

				KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_MMU,
				       "dma m4u_alloc_mva: 0x%x 0x%x 0x%x 0x%x\n",
				       gExecInfo.pDMAMemory->data, gExecInfo.pDMAMemory->phyAddr,
				       g3d_va, size);
				/* borrow pageArray field to put sg_table, it will be free when alloc iommu mva */
				gExecInfo.pDMAMemory->pageArray = (void *)table;
			}
		} else
#endif
		{
#ifdef YL_SECURE
			/* [brady to do] to pass flag information to g3dKmdMmuMap_ex //TODO */
#endif /* YL_SECURE */

			if (iommu_alloc_mva((m4u_client_t *)m, M4U_PORT_GPU, (unsigned long)va, NULL, size, prot,
					     0, &g3d_va)) {
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMU, "kalloc fail\n");
				g3dkmd_backtrace();
				YL_MMU_ASSERT(0);
				__SEM_UNLOCK(mmuLock);
				return 0;
			}
		}
#endif /* MMU_USING_WCP */

		/*
		   KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_MMU,"[%s]: va 0x%x, size %d, prot 0x%x, mva 0x%x\n",
		   __FUNCTION__, va, size, prot, g3d_va);
		 */
	}
#else /* ! if ((USE_MMU == MMU_USING_M4U) || (USE_MMU == MMU_USING_WCP)) */
	YL_KMD_ASSERT(0);
#endif

	__SEM_UNLOCK(mmuLock);

	return (unsigned int)g3d_va;
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuUnmap(void *m, unsigned int g3d_va, unsigned int size)
{
	__SEM_LOCK(mmuLock);

#if (USE_MMU == MMU_USING_GPU)
	g3dKmdMmuUnmap_ex(m, g3d_va, size);
#elif (USE_MMU == MMU_USING_M4U)
	g3d_m4u_dealloc_mva((m4u_client_t *)m, M4U_PORT_GPU, g3d_va, size);
#elif (USE_MMU == MMU_USING_WCP)
	iommu_dealloc_mva((m4u_client_t *)m, M4U_PORT_GPU, g3d_va);
#else
	YL_KMD_ASSERT(0);
#endif

	__SEM_UNLOCK(mmuLock);
}


G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdMmuRangeSync(void *m, unsigned long va,
							   unsigned int size, unsigned int g3d_va,
							   unsigned int cache_action)
{
	unsigned int ret = 1;

#if (USE_MMU == MMU_USING_WCP)

	M4U_CACHE_SYNC_ENUM sync_type;

	__SEM_LOCK(mmuLock);

	switch (cache_action) {
	case G3D_CACHE_CLEAN_BY_RANGE:
		sync_type = M4U_CACHE_CLEAN_BY_RANGE;
		break;
	case G3D_CACHE_INVALID_BY_RANGE:
		sync_type = M4U_CACHE_INVALID_BY_RANGE;
		break;
	case G3D_CACHE_FLUSH_BY_RANGE:
		sync_type = M4U_CACHE_FLUSH_BY_RANGE;
		break;
	default:
		ret = 0;
	}
	if (ret) {
		/* in K2, shall be
		 * if (m4u_cache_sync((m4u_client_t *)m, M4U_PORT_GPU, va, size, g3d_va, sync_type) < 0)
		 */
		if (iommu_cache_sync((m4u_client_t *) m, M4U_PORT_GPU, (unsigned int)va, size, g3d_va, sync_type) < 0)
			ret = 0;
	}

	__SEM_UNLOCK(mmuLock);
#endif

	return ret;
}

#ifdef linux
#if !defined(linux_user_mode)	/* linux user space */

static __DEFINE_SEM_LOCK(memLock);
/*
 * MEM_LOCK may be able to removed ...
 * cause "BUG: scheduling while atomic" kernel warning during get_user_pages calling =>
 * maybe cause by UART irq so we can use I use local_irq_disable/local_irq_enable or __LOCK_MEM/__UNLOCK_MEM in here
 * finally, I think the code protected by spinlock can't be scheduled,
 * but some or linux APIs will do that like get_user_page(..)
*/

G3DKMD_APICALL uint64_t G3DAPIENTRY g3dKmdPhyAlloc(unsigned int size)
{
	uint64_t phyAddr;

	__SEM_LOCK(memLock);
	phyAddr = g3dKmdAllocPhysical(size);
	__SEM_UNLOCK(memLock);
	return phyAddr;
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdPhyFree(uint64_t phyAddr, unsigned int size)
{
	__SEM_LOCK(memLock);
	g3dKmdFreePhysical(phyAddr, size);
	__SEM_UNLOCK(memLock);
}

G3DKMD_APICALL int G3DAPIENTRY g3dKmdUserPageGet(unsigned long uaddr, unsigned long size,
						 uintptr_t *ret_pages, unsigned int *ret_nr_pages)
{
	unsigned long first, last;
	unsigned long start_chk_addr, end_chk_addr, addr;
	unsigned long ret, j;
	unsigned int nr_pages;
	struct page **pages;

	__SEM_LOCK(memLock);
	first = (uaddr & PAGE_MASK) >> PAGE_SHIFT;
	last = ((uaddr + size) & PAGE_MASK) >> PAGE_SHIFT;
	nr_pages = last - first + 1;

	pages = G3DKMDMALLOC(nr_pages * sizeof(*pages));
	if (pages == NULL) {
		__SEM_UNLOCK(memLock);
		return -ENOMEM;
	}

	/* Try to fault in all of the necessary pages */
	down_read(&current->mm->mmap_sem);
	ret = get_user_pages(current, current->mm, uaddr, nr_pages,
				1,/* whether pages will be written to by the caller */
				0,/* don't force */
				pages,
				NULL);/* when get_user_pages, does we already get_page with gotten pages ? */
	up_read(&current->mm->mmap_sem);

	/* Errors and no page mapped should return here */
	if (ret < nr_pages)
		goto out_unmap;

	/* checking the ptr directly */
	start_chk_addr = uaddr;
	end_chk_addr = uaddr + size;
	addr = start_chk_addr;
	j = 0;

	while ((addr >= start_chk_addr) && (addr <= end_chk_addr)) {
		pa_t pa = va_to_pa_after_mmap(addr);

		if ((pa >> PAGE_SHIFT) != page_to_pfn(pages[j]))
			goto out_unmap;
		j++;
		addr += PAGE_SIZE;
	}

	__SEM_UNLOCK(memLock);

	*ret_pages = (uintptr_t) pages;
	*ret_nr_pages = nr_pages;
	return ret;

out_unmap:

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMU, "g3dKmdUserPageGet: get_user_pages fail\n");

	if (ret > 0) {
		for (j = 0; j < ret; j++)
			put_page(pages[j]);
		ret = 0;
	}

	G3DKMDFREE(pages);
	__SEM_UNLOCK(memLock);

	*ret_pages = (uintptr_t) NULL;
	*ret_nr_pages = ret;
	return ret;
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdUserPageFree(uintptr_t pageArray, unsigned int nr_pages)
{
	struct page **pages = (struct page **)pageArray;
	unsigned int i;

	__SEM_LOCK(memLock);
	for (i = 0; i < nr_pages; i++)
		put_page(pages[i]);/* decrease pages refcount */


	G3DKMDFREE(pages);
	__SEM_UNLOCK(memLock);
}
#endif
#endif
/* 0 if oom */
G3DKMD_APICALL uint64_t G3DAPIENTRY g3dKmdAllocPhysical(unsigned int size)
{
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))	/* linux user space */
	/* _g3dDeviceAllocMallocPattern change to use g3dKmdRemapPhysical */
	return (uint64_t) g3dKmdPatternAllocPhysical(size);
#elif (defined(linux) && !defined(linux_user_mode))

	void *p;
	uint64_t retval;

	p = G3DKMDMALLOC(size);

	if (p != NULL && ((unsigned long)p) < (unsigned long)high_memory) {
		retval = (uint64_t) __pa(p); /* kernel logical address */
	} else {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMU, "wrong memory allocation 0x%p 0x%x\n", p,
		       size);
		if (p != NULL)
			G3DKMDFREE(p);
		retval = 0;
	}

	return retval;
#else
	/* the real allocation mechanism */
	return 0;
#endif
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdFreePhysical(uint64_t addr, unsigned int size)
{
#if defined(CMODEL) && (defined(_WIN32) || (defined(linux) && defined(linux_user_mode)))
#ifdef G3DKMD_PHY_HEAP
	if (g3dKmdCfgGet(CFG_PHY_HEAP_ENABLE)) {
		mmFree((unsigned int)addr, size);
	} else
#endif
	{
		/* For pattern & C-Model */
		/* can not do anything */
	}
#elif defined(linux)
#if defined(__KERNEL__) /* linux user space */
	if (addr != 0)
		G3DKMDFREE(__va(addr));
#endif
#else
	/* the real free mechanism */
#endif
}

G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdAllocHw(uint64_t phyAddr, unsigned int size)
{
	unsigned int hwAddr;

	__SEM_LOCK(mmuLock);

	hwAddr = g3dKmdAllocHw_ex(phyAddr, size);

	__SEM_UNLOCK(mmuLock);
	return hwAddr;
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdFreeHw(unsigned int hwAddr, unsigned int size)
{
	__SEM_LOCK(mmuLock);

	g3dKmdFreeHw_ex(hwAddr, size);

	__SEM_UNLOCK(mmuLock);
}

#ifdef G3D_SUPPORT_SWRDBACK
void g3dKmdAllocSwRdback(struct g3dExecuteInfo *execute, unsigned int size)
{
#if defined(linux) && !defined(linux_user_mode)
	if (!allocatePhysicalMemory
	    (&execute->SWRdbackMem, size, G3D_SWRDBACK_SLOT_SIZE,
	     G3DKMD_MMU_DMA | G3DKMD_MMU_ENABLE)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMU, "wrong memory allocation 0x%x\n", size);
	}
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMU, "g3dva 0x%x, pa 0x%x\n",
	       execute->SWRdbackMem.hwAddr, execute->SWRdbackMem.patPhyAddr);
#endif
}

void g3dKmdFreeSwRdback(struct g3dExecuteInfo *execute)
{
#if defined(linux) && !defined(linux_user_mode)
	if (execute->SWRdbackMem.data)
		freePhysicalMemory(&execute->SWRdbackMem);
	g3dbaseCleanG3dMemory(&execute->SWRdbackMem);
#endif
}

uint64_t g3dKmdSwRdbackAlloc(int taskID, unsigned int *hwAddr)
{
#if defined(linux) && !defined(linux_user_mode)
	uint64_t phyAddr;

	if ((taskID == -1) || (taskID >= 256) || hwAddr == NULL ||	/* 4K = 256x16 */
	    (taskID >= gExecInfo.taskInfoCount) || (gExecInfo.SWRdbackMem.phyAddr == 0) ||
	    (NULL == g3dKmdGetTask(taskID))) {
		return 0;
	}

	__SEM_LOCK(memLock);
	*hwAddr = gExecInfo.SWRdbackMem.hwAddr + G3D_SWRDBACK_SLOT_SIZE * taskID;
	phyAddr = gExecInfo.SWRdbackMem.phyAddr + G3D_SWRDBACK_SLOT_SIZE * taskID;
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMU, "[%03d]: g3dva 0x%x, pa 0x%x\n", taskID, *hwAddr, phyAddr);

	__SEM_UNLOCK(memLock);
	return phyAddr;
#else
	return 0;
#endif
}

#endif /* G3D_SUPPORT_SWRDBACK */

/* 0 if oom */
#ifdef YL_SECURE
G3DKMD_APICALL unsigned int G3DAPIENTRY
g3dKmdRemapPhysical(void *va, unsigned int size, unsigned int secure_flag)
#else
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdRemapPhysical(void *va, unsigned int size)
#endif
{
	/* For pattern & C-Model */
#ifdef YL_SECURE
	return g3dKmdPatternRemapPhysical(G3D_MEM_PHYSICAL, va, size, secure_flag);
#else
	return g3dKmdPatternRemapPhysical(G3D_MEM_PHYSICAL, va, size);
#endif
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdSetPhysicalStartAddr(unsigned int pa_base)
{
	/* For pattern & C-Model */
	g3dKmdPatternSetPhysicalStartAddr(pa_base);
}

#if defined(CMODEL) || defined(G3DKMD_SNAPSHOT_PATTERN)
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdMmuG3dVaToVa(void *m, unsigned int g3d_va)
{				/* add by Frank  Fu, 2012/12/21 - B */
	unsigned int pa = 0;

	__SEM_LOCK(mmuLock);

#if (USE_MMU == MMU_USING_GPU)
	pa = Mapper_g3d_va_to_va((struct Mapper *) m, g3d_va);
#elif (USE_MMU == MMU_USING_M4U)
	pa = m4u_mva_to_va((m4u_client_t *) m, M4U_PORT_GPU, g3d_va);
#elif (USE_MMU == MMU_USING_WCP)
	/* do nothing */
#else
	YL_KMD_ASSERT(0);
#endif

	__SEM_UNLOCK(mmuLock);

	return pa;
}
#endif
/* add by Frank  Fu, 2012/12/21 - E */
