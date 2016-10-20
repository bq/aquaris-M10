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
#include <windows.h>
#include <stdio.h>
#elif defined(linux) && defined(linux_user_mode)
#include <stdio.h>
#else
#include <linux/module.h>
#include <linux/radix-tree.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>
#include <asm/page.h>
#include <linux/slab.h> /*for kmalloc */

#endif

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3d_memory.h"
#include "g3dkmd_pattern.h"
#include "g3dkmd_task.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_util.h"
#ifdef G3DKMD_PHY_HEAP
#include "g3dkmd_fakemem.h"
#endif


#ifdef G3DKMD_SNAPSHOT_PATTERN
static int s_ssp_count;
static int sspEnable;
#define SSP_FILENAMEMAX 64
char ssp_path[SSP_PATHNAMEMAX] = "/sdcard/";
#endif

#ifdef PATTERN_DUMP
#include "sapphire_reg.h"
#include "mmu/yl_mmu_kernel_alloc.h"
#include "g3dkmd_file_io.h"
#include "mmu/yl_mmu_utility.h"
#include "g3dkmd_api_mmu.h"
#include "g3dkmd_iommu.h"
#include "g3dkmd_mmce.h"
#include "g3dkmd_gce.h"
#include "g3dkmd_cfg.h"
#include "g3dkmd_fdq.h"
#include "g3dkmd_hwdbg.h"

#endif

#ifdef YL_MMU_PATTERN_DUMP_V1
#include "g3d_api.h"
#endif

/* Define */
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifdef ANDROID

/* For SSP_WRITE_BIN */
#define MMU_SMALL_SIZE_ID     1
#define MMU_LARGE_SIZE_ID     2
#define MMU_SECTION_SIZE_ID   3
#define MMU_SUPSECT_SIZE_ID   4

#endif /* ANDROID */

#if MMU_SECTION_DETECTION
#define PATTERN_PAGE_SIZE 1048576
#define PATTERN_PAGE_MASK 1048575
#else
#define PATTERN_PAGE_SIZE 4096
#define PATTERN_PAGE_MASK 4095
#endif

#ifdef PATTERN_DUMP
/* Global */

static unsigned int s_pa_base = 1 * 1024 * 1024;
unsigned int curPhyAddr = 1 * 1024 * 1024;

#ifdef YL_SECURE
unsigned int curPhyAddr_secure = 2048UL * 1024 * 1024 + 1UL * 1024 * 1024; /* 2G + 1M */
#define G3D_MAX_PA_ADDR  (4095UL*1024*1024) /* 4G */
#define G3D_MAX_PA_NON_SECURE_ADDR (2047UL*1024*1024) /* 2G */

#else /* ! YL_SECURE */
#define G3D_MAX_PA_ADDR  (4095UL*1024*1024)
#endif /* YL_SECURE */

/* #define G3D_MAX_PA_ADDR 511*1024*1024 */
/* #define G3D_VA_BASE 0 */
/* #define G3D_VA_SIZE 256*1024*1024 */

void g3dKmdPatternSetPhysicalStartAddr(unsigned int pa_base)
{
	s_pa_base = pa_base;
	curPhyAddr = s_pa_base;
#ifdef G3DKMD_PHY_HEAP
	g_pa_base = pa_base;
#endif
}

void g3dKmdPatternFreeAll(void)
{
	curPhyAddr = s_pa_base;
}

/* Pattern is 16byte aligned */
unsigned int g3dKmdPatternAddPhysicalAddress(int size)
{
	unsigned int addr;

#ifdef G3DKMD_PHY_HEAP
	if (g3dKmdCfgGet(CFG_PHY_HEAP_ENABLE)) {
		addr = mmAlloc(size);
	} else
#endif
	{
		YL_KMD_ASSERT(size > 0);

		curPhyAddr = (curPhyAddr + 15) & ~15UL;
		size = (size + 15) & ~15UL;

		YL_KMD_ASSERT(size > 0);

		addr = curPhyAddr;
		curPhyAddr += size;

		/* assert(curPhyAddr < (s_pa_base + G3D_PA_SIZE)); */

		if (curPhyAddr >= G3D_MAX_PA_ADDR) {
			/* YL_KMD_ASSERT(curPhyAddr < G3D_MAX_PA_ADDR); */
			return 0; /* out of memory */
		}

	}
	return addr;
}

#ifdef YL_SECURE
static unsigned int
g3dKmdPatternAddPhysicalAddressByVa(void *va, int size, unsigned int secure_flag)
#else
static unsigned int g3dKmdPatternAddPhysicalAddressByVa(void *va, int size)
#endif
{
	unsigned int addr;
#ifdef G3DKMD_PHY_HEAP
	if (g3dKmdCfgGet(CFG_PHY_HEAP_ENABLE)) {
		YL_KMD_ASSERT(PATTERN_PAGE_SIZE >= 16);
		YL_KMD_ASSERT(size > 0);

		addr = mmAllocByVa(size, va);
	} else
#endif
	{
		unsigned int va_offset;
		unsigned int pa_offset;


#ifdef YL_SECURE
		/* [brady todo] */
		/* if is_secure, then use secure_base */
		/* else        , then use normal_base */
		const unsigned int const_mmu_secure = (0x1ul << 1);
		unsigned int is_secure = secure_flag & const_mmu_secure;
#endif


		YL_KMD_ASSERT(PATTERN_PAGE_SIZE >= 16);
		YL_KMD_ASSERT(size > 0);

		va_offset = (unsigned int)(uintptr_t) va & PATTERN_PAGE_MASK;
		pa_offset = curPhyAddr & PATTERN_PAGE_MASK;

		if (va_offset > pa_offset) {
#ifdef YL_SECURE
			/* floor */
			if (is_secure)
				curPhyAddr_secure = (curPhyAddr_secure & (~PATTERN_PAGE_MASK)) | va_offset;
			else
				curPhyAddr = (curPhyAddr & (~PATTERN_PAGE_MASK)) | va_offset;

#else
			/* floor */
			curPhyAddr = (curPhyAddr & (~PATTERN_PAGE_MASK)) | va_offset;
#endif
		} else {
#ifdef YL_SECURE
			/* Ceiling */
			if (is_secure) {
				curPhyAddr_secure = ((curPhyAddr_secure +
				      PATTERN_PAGE_MASK) & (~PATTERN_PAGE_MASK)) | va_offset;
			} else {
				curPhyAddr = ((curPhyAddr + PATTERN_PAGE_MASK) & (~PATTERN_PAGE_MASK)) | va_offset;
			}
#else
			/* Ceiling */
			curPhyAddr = ((curPhyAddr + PATTERN_PAGE_MASK) & (~PATTERN_PAGE_MASK)) | va_offset;
#endif
		}

		size = (size + 15) & ~15UL;

		YL_KMD_ASSERT(size > 0);

#ifdef YL_SECURE
		if (is_secure) {
			addr = curPhyAddr_secure;
			curPhyAddr_secure += size;

			/* S world : 2G+1M ~ 4G */
			YL_KMD_ASSERT(curPhyAddr_secure < G3D_MAX_PA_ADDR);

			if (curPhyAddr_secure >= G3D_MAX_PA_ADDR) {
				YL_KMD_ASSERT(curPhyAddr_secure < G3D_MAX_PA_ADDR);
				return 0; /* out of memory */
			}
		} else {
			addr = curPhyAddr;
			curPhyAddr += size;

			/* NS world : 1M~2G */
			YL_KMD_ASSERT(curPhyAddr < G3D_MAX_PA_NON_SECURE_ADDR);

			if (curPhyAddr >= G3D_MAX_PA_NON_SECURE_ADDR) {
				YL_KMD_ASSERT(curPhyAddr < G3D_MAX_PA_NON_SECURE_ADDR);
				return 0; /* out of memory */
			}
		}
#else /* ! YL_SECURE */
		addr = curPhyAddr;
		curPhyAddr += size;

		YL_KMD_ASSERT(curPhyAddr < G3D_MAX_PA_ADDR);

		if (curPhyAddr >= G3D_MAX_PA_ADDR) {
			YL_KMD_ASSERT(curPhyAddr < G3D_MAX_PA_ADDR);
			return 0; /* out of memory */
		}
#endif /* YL_SECURE */
	}
	return addr;
}

unsigned int g3dKmdPatternAllocPhysical(unsigned int size)
{
	return g3dKmdPatternAddPhysicalAddress(size);
}

#ifdef YL_SECURE
unsigned int
g3dKmdPatternRemapPhysical(G3d_Memory_Type memType,
			   void *va, unsigned int size, unsigned int secure_flag)
#else
unsigned int g3dKmdPatternRemapPhysical(G3d_Memory_Type memType, void *va, unsigned int size)
#endif
{
	if (memType == G3D_MEM_PHYSICAL) {
#ifdef YL_SECURE
		return g3dKmdPatternAddPhysicalAddressByVa(va, size, 0);	/* we don't have to secure phy memory */
#else
		return g3dKmdPatternAddPhysicalAddressByVa(va, size);
#endif
	} else if (memType == G3D_MEM_VIRTUAL) {
#ifdef YL_SECURE
		/* [brady todo] pass flag to g3dKmdPatternAddPhysicalAddressByVa */
		return g3dKmdPatternAddPhysicalAddressByVa(va, size, secure_flag);
#else
		return g3dKmdPatternAddPhysicalAddressByVa(va, size);
#endif
	} else {
		YL_KMD_ASSERT(0);
	}

	return 0;
}

void g3dKmdDumpDataToFile(void *file, unsigned char *data, unsigned int hwData, unsigned int size)
{
	int zeros = hwData & 0xf;
	int i, j = 0;
	unsigned int *dwData;

	if (!data || !file)
		return;

	g_pKmdFileIo->fPrintf(file, "F0E0F0E0_F0E0F0E0_F0E0F0E0_%08x\n", (hwData >> 4));
	if (zeros) {
		for (i = (15 - zeros); i >= 0; i--) {
			g_pKmdFileIo->fPrintf(file, "%02x", data[i]);
			j++;
			if ((j & 3) == 0)
				g_pKmdFileIo->fPrintf(file, "_");
		}
		for (i = 0; i < zeros; i++) {
			g_pKmdFileIo->fPrintf(file, "00");
			j++;
			if (((j & 3) == 0) & (j != 16))
				g_pKmdFileIo->fPrintf(file, "_");
		}
		g_pKmdFileIo->fPrintf(file, "\n");

		size -= (16 - zeros);
		data += (16 - zeros);
	}

	dwData = (unsigned int *)data;

	while (size >= 16) {
		g_pKmdFileIo->fPrintf(file, "%08x_%08x_%08x_%08x\n", dwData[3], dwData[2], dwData[1], dwData[0]);
		dwData += 4;
		size -= 16;
	}
	j = 0;
	if (size) {
		data = (unsigned char *)dwData;
		zeros = 16 - size;
		for (i = 0; i < zeros; i++) {
			g_pKmdFileIo->fPrintf(file, "00");
			j++;
			if ((j & 3) == 0)
				g_pKmdFileIo->fPrintf(file, "_");
		}
		for (i = 0; i < (int)size; i++) {
			g_pKmdFileIo->fPrintf(file, "%02x", data[size - i - 1]);
			j++;
			if (((j & 3) == 0) & (j != 16))
				g_pKmdFileIo->fPrintf(file, "_");
		}
		g_pKmdFileIo->fPrintf(file, "\n");
	}
}


#ifdef YL_MMU_PATTERN_DUMP_V1
void g3dKmdDumpDataToFileFull(void *file, unsigned char *data, void *m, unsigned int size)
{
	unsigned int i, slot_size, slot_offset = 0;
	unsigned int num_hwPairData;
	unsigned int (*hwPairData)[2];

	YL_KMD_ASSERT(m);
	num_hwPairData = ((V_TO_P_ENTRY *) m)->num;
	hwPairData = ((V_TO_P_ENTRY *) m)->pair;

	for (i = 0; i < num_hwPairData; i++) {
		if ((slot_offset + hwPairData[i][1]) > size)
			slot_size = size - slot_offset;
		else
			slot_size = hwPairData[i][1];

		g3dKmdDumpDataToFile(file, (data + slot_offset), hwPairData[i][0], slot_size);
		slot_offset += slot_size;

		if (slot_offset >= size)
			break;
	}

	YL_KMD_ASSERT(slot_offset == size);
}

void g3dKmdDumpDataToFileFullOffset(void *file, unsigned char *data, void *m, unsigned int size, unsigned int offset)
{
	unsigned int i, slot_size, slot_start_offset, slot_offset = 0;
	unsigned int num_hwPairData;
	unsigned int (*hwPairData)[2];

	YL_KMD_ASSERT(m);
	num_hwPairData = ((V_TO_P_ENTRY *) m)->num;
	hwPairData = ((V_TO_P_ENTRY *) m)->pair;

	size += offset;

	for (i = 0; i < num_hwPairData; i++) {
		if ((slot_offset + hwPairData[i][1]) <= offset) {
			slot_offset += hwPairData[i][1];
			continue;
		}

		if (slot_offset < offset)
			slot_start_offset = offset - slot_offset;
		else
			slot_start_offset = 0;

		if ((slot_offset + hwPairData[i][1]) > size)
			slot_size = size - slot_offset;
		else
			slot_size = hwPairData[i][1];

		g3dKmdDumpDataToFile(file, (data + slot_offset + slot_start_offset),
				     (hwPairData[i][0] + slot_start_offset),
				     (slot_size - slot_start_offset));
		slot_offset += slot_size;

		if (slot_offset >= size)
			break;
	}

	YL_KMD_ASSERT(slot_offset == size);
}
#endif
#ifdef YL_MMU_PATTERN_DUMP_V2
void g3dKmdDumpDataToFilePage(void *m, void *file, unsigned char *data, unsigned int g3d_va, unsigned int size)
{
	unsigned int /*ptr */ next_pa = 0xFFFFFFFF;
	unsigned int continuous_pa_head = 0;
	uintptr_t continuous_va_head = 0;
	unsigned int continuous_size = 0;
	uintptr_t /*ptr */ data_ptr = (uintptr_t) data;
	unsigned int /*ptr */ g3dva_ptr = g3d_va;
	unsigned int size_left = size;

	while (size_left != 0) {
		unsigned int offset_mask = 0;
		unsigned int unit_size = 0;
		unsigned int /*ptr */ pa, pa_head;
		unsigned int current_size;

		unit_size = MMU_SMALL_SIZE;	/* 4K */
		offset_mask = SMALL_PAGE_OFFSET_MASK;	/* 0xFFF */

		/* g3dva => pa */
		pa = g3dKmdMmuG3dVaToPa(m, g3dva_ptr);
		pa_head = pa & ~(offset_mask);

		/* current_size */
		current_size = unit_size - (pa & offset_mask);
		if (size_left < current_size)
			current_size = size_left;

		/* continuous */
		if (pa_head == next_pa) {
			continuous_size += current_size;
		} else {
			/* flush last chunck */
			if (next_pa != 0xFFFFFFFF) {
				g3dKmdDumpDataToFile(file, (unsigned char *)continuous_va_head,
						     continuous_pa_head, continuous_size);
			}

			/* new chunck */
			continuous_pa_head = pa;
			continuous_va_head = data_ptr;
			continuous_size = current_size;
		}

		next_pa = pa_head + unit_size;
		data_ptr += current_size;
		g3dva_ptr += current_size;
		size_left -= current_size;
	}
	/* flush last chunck */
	if (next_pa != 0xFFFFFFFF)
		g3dKmdDumpDataToFile(file, (unsigned char *)continuous_va_head, continuous_pa_head, continuous_size);
}
#endif


static void _g3dKmdDumpHTBuffer(struct g3dExeInst *pInst, unsigned int dumpCount)
{
	void *fp;
	void *copyPtr;
	int size, realsize;
	unsigned int start, readptr, writeptr, offset = 0;

	/* [Brady]:pattern generation refinement */
	int strSize = 0;
	char *basename;

	copyPtr = (void *)pInst->pHtDataForHwRead->htBuffer.data;
	/* start = pInst->htBuffer.hwAddr; //why? */
	size = pInst->pHtDataForHwRead->htBufferSize;

	readptr = pInst->sDumpPtr;
	writeptr = pInst->pHtDataForHwRead->htInfo->wtPtr;

	if (readptr < writeptr) {
		offset = (pInst->sDumpPtr - pInst->pHtDataForHwRead->htBuffer.hwAddr);
		copyPtr = (void *)((uintptr_t) pInst->pHtDataForHwRead->htBuffer.data + offset);
		start = pInst->pHtDataForHwRead->htBuffer.patPhyAddr + offset;
		size = writeptr - readptr;
	} else {
		/* dump whole buffer? this is an easy way but dump more than what really necessary */
		start = pInst->pHtDataForHwRead->htBuffer.patPhyAddr;
	}
	realsize = size;

	strSize = snprintf(NULL, 0, "htbuffer_%03d.hex", dumpCount);
	basename = (char *)yl_mmu_alloc(strSize + 1);
	snprintf(basename, strSize + 1, "htbuffer_%03d.hex", dumpCount);

	fp = g_pKmdFileIo->fileOpen(basename);

	yl_mmu_free(basename);

	if (fp) {
#ifdef YL_MMU_PATTERN_DUMP_V2
		if (g3dKmdMmuGetHandle())
			g3dKmdDumpDataToFilePage(g3dKmdMmuGetHandle(), fp, (unsigned char *)copyPtr,
						 (pInst->pHtDataForHwRead->htBuffer.hwAddr +
						  offset), size);
		else
#endif
#ifdef YL_MMU_PATTERN_DUMP_V1
		if (pInst->pHtDataForHwRead->htBuffer.pa_pair_info) {
			g3dKmdDumpDataToFileFullOffset(fp,
						       (unsigned char *)pInst->pHtDataForHwRead->
						       htBuffer.data,
						       pInst->pHtDataForHwRead->htBuffer.
						       pa_pair_info, size, offset);
		} else
#endif
		{
			g3dKmdDumpDataToFile(fp, (unsigned char *)copyPtr, start, size);
		}
		g_pKmdFileIo->fileClose(fp);

	}

	readptr = readptr - pInst->pHtDataForHwRead->htBuffer.hwAddr;	/* relatived position */
	writeptr = writeptr - pInst->pHtDataForHwRead->htBuffer.hwAddr;

	strSize = snprintf(NULL, 0, "dbg_htbufferinfo_%03d.txt", dumpCount);
	basename = (char *)yl_mmu_alloc(strSize + 1);
	snprintf(basename, strSize + 1, "dbg_htbufferinfo_%03d.txt", dumpCount);

	fp = g_pKmdFileIo->fileOpen(basename);


	yl_mmu_free(basename);

	if (fp) {
		g_pKmdFileIo->fPrintf(fp, "ld_htbuffer_bas %x size %x\n", start, (realsize >> 4));
		g_pKmdFileIo->fPrintf(fp, "ht_buffer_start_adr %x size %x\n",
				      pInst->pHtDataForHwRead->htBuffer.hwAddr,
				      (pInst->pHtDataForHwRead->htBufferSize >> 4));
		g_pKmdFileIo->fPrintf(fp, "cq_readptr %x cq_writeptr %x\n", (readptr >> 4), (writeptr >> 4));
/* #ifdef CMODEL */
#ifdef G3DKMD_PHY_HEAP
		if (g3dKmdCfgGet(CFG_PHY_HEAP_ENABLE))
			g_pKmdFileIo->fPrintf(fp, "curPhyAddr = 0x%08x\n", g_max_pa_addr);
		else
#endif
			g_pKmdFileIo->fPrintf(fp, "curPhyAddr = 0x%08x\n", curPhyAddr);

/* #endif */
		g_pKmdFileIo->fileClose(fp);

	}
}

void g3dKmdDumpQbaseBuffer(struct g3dExeInst *pInst, unsigned int dumpCount)
{
	unsigned char *pData;
	unsigned int start, size;

	if (!g3dKmdCfgGet(CFG_DUMP_ENABLE))
		return;

	/* prepare data */
	pData = (unsigned char *)pInst->qbaseBuffer.data;
	size = pInst->qbaseBufferSize;
/* #ifdef CMODEL */
	start = pInst->qbaseBuffer.patPhyAddr;
/* #else */
	/* start = pInst->qbaseBuffer.hwAddr; */
/* #endif */

	if (pData == NULL || size == 0) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN, "qbaseBuffer not initialized\n");
		return;
	}

	if (pInst->fp_qbuf) {
#ifdef YL_MMU_PATTERN_DUMP_V2
		if (g3dKmdMmuGetHandle())
			g3dKmdDumpDataToFilePage(g3dKmdMmuGetHandle(), pInst->fp_qbuf, pData,
						 pInst->qbaseBuffer.hwAddr, size);
		else
#endif
#ifdef YL_MMU_PATTERN_DUMP_V1
		if (pInst->qbaseBuffer.pa_pair_info)
			g3dKmdDumpDataToFileFull(pInst->fp_qbuf,
						 (unsigned char *)pInst->qbaseBuffer.data,
						 pInst->qbaseBuffer.pa_pair_info, size);
		else
#endif
			g3dKmdDumpDataToFile(pInst->fp_qbuf, pData, start, size);
	}
}

static void _g3dKmdDumpMMUTable(struct g3dExeInst *pInst, unsigned int dumpCount, unsigned int inSnapshot)
{
	void *swAddr, *hwAddr, *hMmu;
	unsigned int size, i;
	char *basename;
	void *fp;
	unsigned int *lone_va;

	hMmu = g3dKmdMmuGetHandle();

#ifdef G3DKMD_SNAPSHOT_PATTERN
	if ((!inSnapshot && !g3dKmdCfgGet(CFG_DUMP_ENABLE)) || !hMmu)
		return;
#else
	if (!g3dKmdCfgGet(CFG_DUMP_ENABLE) || !hMmu)
		return;
#endif

	/* open file */
#ifdef G3DKMD_SNAPSHOT_PATTERN
	if (inSnapshot) {

#if defined(ANDROID) && !defined(YL_FAKE_X11)
		size = snprintf(NULL, 0, "%s/g3dssp/g3dssp_mmu_%x.hex", ssp_path, s_ssp_count);
		basename = (char *)yl_mmu_alloc(size + 1);
		snprintf(basename, size + 1, "%s/g3dssp/g3dssp_mmu_%x.hex", ssp_path, s_ssp_count);
#else /* ! ANDROID */
		size = snprintf(NULL, 0, "g3dssp_mmu_%x.hex", s_ssp_count);
		basename = (char *)yl_mmu_alloc(size + 1);
		snprintf(basename, size + 1, "g3dssp_mmu_%x.hex", s_ssp_count);
#endif /* ANDROID */

		fp = g_pKmdFileIo->fileOpen(basename);

		yl_mmu_free(basename);
	} else
#endif
	{
#if defined(ANDROID) && !defined(YL_FAKE_X11)
		size = snprintf(NULL, 0, "/data/mmu_buffer_%03d.hex", dumpCount);
		basename = (char *)yl_mmu_alloc(size + 1);
		snprintf(basename, size + 1, "/data/mmu_buffer_%03d.hex", dumpCount);
#else /* ! ANDROID */
		size = snprintf(NULL, 0, "mmu_buffer_%03d.hex", dumpCount);
		basename = (char *)yl_mmu_alloc(size + 1);
		snprintf(basename, size + 1, "mmu_buffer_%03d.hex", dumpCount);
#endif /* ANDROID */

		fp = g_pKmdFileIo->fileOpen(basename);

		yl_mmu_free(basename);
	}

	/* get MMU table */
	g3dKmdMmuGetTable(hMmu, &swAddr, &hwAddr, &size);

	/* dump L1 */
	g3dKmdDumpDataToFile(fp, (unsigned char *)swAddr, (unsigned int)(uintptr_t) hwAddr, size);

	/* parse L1 */
	lone_va = (unsigned int *)swAddr;

	for (i = 0; i < size / 4; i++) {
		unsigned int lone_entry = lone_va[i];

		if ((lone_entry & 0x3) == 0x1) {
			/* page */
			unsigned int ltwo_pa = lone_entry & SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK;
#if defined(WIN32) || (defined(linux) && defined(linux_user_mode))
			G3dMemory *page_mem = &(((struct Mapper *) hMmu)->Page_mem);
			unsigned int *ltwo_va = (unsigned int *)((uintptr_t) page_mem->data +
							(ltwo_pa - page_mem->hwAddr));
			g3dKmdDumpDataToFile(fp, (unsigned char *)ltwo_va, ltwo_pa, 1024);
#elif defined(linux)
			struct page *l2_page = (pfn_to_page(ltwo_pa >> PAGE_SHIFT));
			unsigned int *ltwo_va = (unsigned int *)vmap(&l2_page, 1, VM_MAP, PAGE_KERNEL);
			void *entry1;

			entry1 = (struct SecondPageEntry *) ((uintptr_t) ltwo_va | (ltwo_pa & 0xC00));

			g3dKmdDumpDataToFile(fp, entry1, ltwo_pa, 1024);

			vunmap(ltwo_va);

#endif
		}
	}

	/* close file */
	g_pKmdFileIo->fileClose(fp);

}

void _g3dKmdDumpCommandBegin(int taskID, unsigned int dumpCount)
{
	struct g3dExeInst *pInst;
	int strSize;
	char *basename;

	if (!g3dKmdCfgGet(CFG_DUMP_ENABLE))
		return;

	pInst = g3dKmdTaskGetInstPtr(taskID);

	if (pInst == NULL)
		return;

	if (gfAllocInfoKernal)
		g_pKmdFileIo->fPrintf(gfAllocInfoKernal, "ps_%08x\n", dumpCount);

	pInst->sDumpPtr = pInst->pHtDataForHwRead->htInfo->wtPtr;
	pInst->sDumpIndexCount = 0;

	/* open global file */
	strSize = snprintf(NULL, 0, "qbasebuffer_%03d.hex", dumpCount);
	basename = (char *)yl_mmu_alloc(strSize + 1);
	snprintf(basename, strSize + 1, "qbasebuffer_%03d.hex", dumpCount);
	pInst->fp_qbuf = g_pKmdFileIo->fileOpen(basename);

	YL_KMD_ASSERT(pInst->fp_qbuf);
	yl_mmu_free(basename);

	g3dKmdRiuOpen(dumpCount);

#ifdef USE_GCE
	if (g3dKmdCfgGet(CFG_GCE_ENABLE))
		g3dKmdMmcqDumpBegin(pInst->mmcq_handle);
#endif
}

void _g3dKmdDumpCommandEnd(int taskID, unsigned int dumpCount)
{
	struct g3dExeInst *pInst;

	if (!g3dKmdCfgGet(CFG_DUMP_ENABLE))
		return;

	pInst = g3dKmdTaskGetInstPtr(taskID);

	/* error  : need to increase MAX_HT_COMMAND */
	if ((pInst == NULL) || (pInst->sDumpIndexCount >= MAX_HT_COMMAND))
		return;

	g3dKmdDumpQbaseBuffer(pInst, dumpCount);

	_g3dKmdDumpHTBuffer(pInst, dumpCount);

	_g3dKmdDumpMMUTable(pInst, dumpCount, 0);

	/* close global file */
	if (pInst->fp_qbuf) {
		g_pKmdFileIo->fileClose(pInst->fp_qbuf);
		pInst->fp_qbuf = NULL;
	}

	g3dKmdRiuClose(dumpCount);

#ifdef USE_GCE
	if (g3dKmdCfgGet(CFG_GCE_ENABLE)) {
		g3dKmdMmcqDumpEnd(pInst->mmcq_handle, dumpCount);
		g3dKmdRiuGceDump(pInst->mmcq_handle, dumpCount);
	}
#endif
}

void g3dKmdDumpInitRegister(int taskID, unsigned int dumpCount)
{
#ifdef USE_GCE
	if (g3dKmdCfgGet(CFG_GCE_ENABLE))
		g3dKmdGceResetHW();
#endif

	g3dKmdRegReset();

#ifdef G3DKMD_SUPPORT_FDQ
	g3dKmdFdqUpdateBufPtr();
#endif

#if (USE_MMU == MMU_USING_WCP)
	/* the iommu registers are already done in m4u_reg_restore() in g3dKmdRegReset() */
#elif (USE_MMU == MMU_USING_GPU || USE_MMU == MMU_USING_M4U)
	{
		void *swAddr, *hwAddr;
		unsigned int size;

		if (g3dKmdMmuGetHandle()) {
			g3dKmdMmuGetTable(g3dKmdMmuGetHandle(), &swAddr, &hwAddr, &size);
			g3dKmdIommuInit(IOMMU_MOD_1, (unsigned int)(uintptr_t) hwAddr);
#ifdef G3DKMD_SUPPORT_EXT_IOMMU
			g3dKmdIommuInit(IOMMU_MOD_2, (unsigned int)(uintptr_t) hwAddr);
#endif
		} else {
			g3dKmdIommuDisable(IOMMU_MOD_1);
#ifdef G3DKMD_SUPPORT_EXT_IOMMU
			g3dKmdIommuDisable(IOMMU_MOD_2);
#endif
		}
	}
#endif

#ifdef USE_GCE
	if (g3dKmdCfgGet(CFG_GCE_ENABLE)) {
		struct g3dExeInst *pInst = g3dKmdTaskGetInstPtr(taskID);

		if ((dumpCount != 0) && (pInst->thd_handle != MMCE_INVALID_THD_HANDLE))
			g3dKmdMmceResume(pInst->thd_handle, pInst->mmcq_handle, pInst->hwChannel);
	}
#endif

#ifdef YL_HW_DEBUG_DUMP_TEMP

	/* write MX debug reg ( 32KB buffer address ) */
	g3dKmdHwDbgSetDumpFlushBase();

#endif


	if (gfAllocInfoKernal)
		g_pKmdFileIo->fPrintf(gfAllocInfoKernal, "pe\n");
}

struct G3DKMD_RIU_ST {
	unsigned char enabled;
	void *fRiu;
	char fname[64];
};

static struct G3DKMD_RIU_ST _riu_st[] = {
	{G3DKMD_TRUE, NULL, "riu_g3d"},
/*
#ifdef USE_GCE
	{G3DKMD_TRUE, NULL, "riu_gce"},
#endif
*/
	{G3DKMD_FALSE, NULL, ""}
};

#if 0 /* unused function */
static void _g3dKmdRiuRemove(unsigned int dumpCount)
{
	unsigned int i;
	unsigned int strSize;
	char *basename;

	g3dKmdRiuClose(dumpCount);

	for (i = 0; i < sizeof(_riu_st) / sizeof(struct G3DKMD_RIU_ST); i++) {
		if (_riu_st[i].enabled) {
			strSize = snprintf(NULL, 0, "%s_%03d.hex", _riu_st[i].fname, dumpCount);
			basename = (char *)yl_mmu_alloc(strSize + 1);
			snprintf(basename, strSize + 1, "%s_%03d.hex", _riu_st[i].fname, dumpCount);

			g3dKmdFileRemove(basename);

			yl_mmu_free(basename);
		}
	}
}
#endif

#if defined(inux) && !defined(linux_user_mode)
#define PATTERN_DUMP_MT_SAFE
#endif

#ifdef PATTERN_DUMP_MT_SAFE
RADIX_TREE(gPidmapper, GFP_KERNEL); /* Declare and initialize */
static __DEFINE_SPIN_LOCK(g3dPattern);
void radix_tree_insert_safe(struct radix_tree_root *tree, unsigned long key, void *item)
{
	int result = 0;

	if (tree == NULL)
		return;

	result = radix_tree_insert(tree, key, item);
	if (result == -EEXIST) {
		radix_tree_delete(tree, key);
		result = radix_tree_insert(tree, key, item);
	}
	if (result == -ENOMEM) {
		result = radix_tree_preload(GFP_KERNEL);
		if (result == 0) {
			result = radix_tree_insert(tree, key, item);
			radix_tree_preload_end();
		}
	}
	YL_KMD_ASSERT(result == 0);
}
#else
static unsigned int _g3dkmd_bypass;
#endif

unsigned int g3dKmdBypass(unsigned int bypass)
{
	unsigned int old_bypass = 0;
#ifdef SUPPORT_MPD
	if (1) {
#else
	if (g3dKmdCfgGet(CFG_DUMP_ENABLE)) {
#endif

#ifdef PATTERN_DUMP_MT_SAFE
		__SPIN_LOCK2_DECLARE(flag);
		__SPIN_LOCK2(g3dPattern, flag);
		old_bypass = (unsigned int)radix_tree_lookup(&gPidmapper, (unsigned long)current->pid);
		radix_tree_insert_safe(&gPidmapper, (unsigned long)current->pid, (void *)(old_bypass | bypass));
		__SPIN_UNLOCK2(g3dPattern, flag);
#else
		old_bypass = _g3dkmd_bypass;
		_g3dkmd_bypass |= bypass;
#endif
	} else
		old_bypass = (bypass == BYPASS_REG) ? G3DKMD_FALSE : G3DKMD_TRUE;

	return old_bypass;
}

void g3dKmdBypassRestore(unsigned int old_bypass)
{
#ifndef SUPPORT_MPD
	if (g3dKmdCfgGet(CFG_DUMP_ENABLE)) {
#else
	{
#endif
#ifdef PATTERN_DUMP_MT_SAFE
		__SPIN_LOCK2_DECLARE(flag);
		__SPIN_LOCK2(g3dPattern, flag);
		if (old_bypass == 0)
			radix_tree_delete(&gPidmapper, (unsigned long)current->pid);
		else
			radix_tree_insert_safe(&gPidmapper, (unsigned long)current->pid, (void *)old_bypass);
		__SPIN_UNLOCK2(g3dPattern, flag);
#else
		_g3dkmd_bypass = old_bypass;
#endif
	}
}

unsigned char g3dKmdIsBypass(unsigned int bypass)
{
	unsigned int old_bypass = BYPASS_RIU;
#ifndef SUPPORT_MPD
	if (g3dKmdCfgGet(CFG_DUMP_ENABLE)) {
#else
	{
#endif

#ifdef PATTERN_DUMP_MT_SAFE
		__SPIN_LOCK2_DECLARE(flag);
		__SPIN_LOCK2(g3dPattern, flag);
		old_bypass = (unsigned int)radix_tree_lookup(&gPidmapper, (unsigned long)current->pid);
		__SPIN_UNLOCK2(g3dPattern, flag);
#else
		old_bypass = _g3dkmd_bypass;
#endif
	}
	return old_bypass & bypass;
}

void g3dKmdRiuWrite(unsigned int mod, const char *fmt, ...)
{
	va_list a_list;

	va_start(a_list, fmt);
	g_pKmdFileIo->vfPrintf(_riu_st[mod].fRiu, fmt, a_list);
	va_end(a_list);
}

void g3dKmdRiuOpen(unsigned int dumpCount)
{
	/*
	if (g3dKmdCfgGet(CFG_DUMP_ENABLE) && (dumpCount >= g3dKmdCfgGet(CFG_DUMP_START_FRM)) &&
		(dumpCount <= g3dKmdCfgGet(CFG_DUMP_END_FRM)))
	*/
	{
		unsigned int i;
		unsigned int strSize;
		char *basename;

		for (i = 0; i < sizeof(_riu_st) / sizeof(struct G3DKMD_RIU_ST); i++) {
			/*
			#ifdef USE_GCE
			if ((i != G3DKMD_RIU_GCE) || g3dKmdCfgGet(CFG_GCE_ENABLE))
			#endif
			*/
			{
				if (_riu_st[i].fRiu) {
					if (i == G3DKMD_RIU_G3D)
						g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfff1, dumpCount);

					g_pKmdFileIo->fileClose(_riu_st[i].fRiu);
					_riu_st[i].fRiu = NULL;
				}

				if (_riu_st[i].enabled) {
					strSize = snprintf(NULL, 0, "%s_%03d.hex", _riu_st[i].fname, dumpCount);
					basename = (char *)yl_mmu_alloc(strSize + 1);
					snprintf(basename, strSize + 1, "%s_%03d.hex", _riu_st[i].fname, dumpCount);

					_riu_st[i].fRiu = g_pKmdFileIo->fileOpen(basename);

					if (!_riu_st[i].fRiu)
						YL_KMD_ASSERT(0);

					yl_mmu_free(basename);

					if (i == G3DKMD_RIU_G3D)
						g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfff0, dumpCount);
				}
			}
		}
	}
}

void g3dKmdRiuClose(unsigned int dumpCount)
{
	unsigned int i;

	for (i = 0; i < sizeof(_riu_st) / sizeof(struct G3DKMD_RIU_ST); i++) {
		if (i == G3DKMD_RIU_G3D)
			g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfff1, dumpCount);
		g_pKmdFileIo->fileClose(_riu_st[i].fRiu);
		_riu_st[i].fRiu = NULL;
	}
}

void g3dKmdRiuWriteIommu(unsigned int reg, unsigned int value)
{
#ifdef SUPPORT_MPD
	CMRiuCommand(IOMMU_MOD_OFST | (reg & 0xFFF), value);
#endif
	g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", IOMMU_MOD_OFST | (reg & 0xFFF), value);
}
#endif /* PATTERN_DUMP */


#ifdef G3DKMD_SNAPSHOT_PATTERN

static void _g3dKmdDumpSwData(void *file, const char *str, void *data, unsigned int size)
{
	unsigned int *pdw = (unsigned int *)data;

	if (!data || !file)
		return;

	g_pKmdFileIo->fPrintf(file, "%s\n", str);

	while (size >= 4) {
		g_pKmdFileIo->fPrintf(file, "%08x\n", *pdw++);
		size -= 4;
	}

	if (size) {
		unsigned char *p = (unsigned char *)pdw;

		switch (size) {
		case 1:
			g_pKmdFileIo->fPrintf(file, "000000%02x ", p[0]);
			break;
		case 2:
			g_pKmdFileIo->fPrintf(file, "0000%02x%02x ", p[1], p[0]);
			break;
		case 3:
			g_pKmdFileIo->fPrintf(file, "00%02x%02x%02x ", p[2], p[1], p[0]);
			break;
		}
	}

}

#if 0
/* Shrchin's version of g3dKmdMmuG3dVaToPa */
static unsigned int _g3dKmdGetPaFromG3dVa(unsigned int g3d_va)
{
	struct FirstPageTable *pageTable;
	struct FirstPageEntry *entry0;
	struct SecondPageEntry *entry1;
	void *m = gExecInfo.mmu;
	unsigned int pa = 0;
	unsigned int index0, index1, size;
	void *swAddr, *hwAddr;

	if (!m || !g3d_va)
		return 0;

	/* get MMU table */
	g3dKmdMmuGetTable(gExecInfo.mmu, &swAddr, &hwAddr, &size);
	pageTable = (struct FirstPageTable *) swAddr;

	index0 = PageTable_index0_from_g3d_va(g3d_va /*- g3dva_offset*/);
	entry0 = &pageTable->entries[index0];

	if (entry0->Common.type == FIRST_TYPE_SECTION) {
		/* 1M continue */
		pa = entry0->all & SECTION_BASE_ADDRESS_MASK;
		pa += g3d_va & ~SECTION_BASE_ADDRESS_MASK;
	} else if (entry0->Common.type == FIRST_TYPE_TABLE) {
		/* 4K or 64k, need to parse L2 table */
		unsigned int ltwo_pa = entry0->all & SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK;
#if defined(WIN32) || (defined(linux) && defined(linux_user_mode))
		G3dMemory *page_mem = &(((struct Mapper *) m)->Page_mem);
		unsigned int *ltwo_va =
		    (unsigned int *)((uintptr_t) page_mem->data + (ltwo_pa - page_mem->hwAddr));
#elif defined(linux)
		unsigned int *ltwo_va = __va(ltwo_pa);
#endif
		index1 = PageTable_index1_from_g3d_va(g3d_va /*- g3dva_offset*/);
		entry1 = ((struct SecondPageEntry *) ltwo_va) + index1;

		if (entry1->Common.type) {
			if (entry1->Common.type & SECOND_TYPE_SMALL) {
				/* 4k */
				pa = entry1->all & SMALL_PAGE_LEVEL1_BASE_ADDRESS_MASK;
				pa += g3d_va & ~SMALL_PAGE_LEVEL1_BASE_ADDRESS_MASK;
			} else {
				/* SECOND_TYPE_LARGE */
				/* 64k */
				pa = entry1->all & LARGE_PAGE_LEVEL1_BASE_ADDRESS_MASK;
				pa += g3d_va & ~LARGE_PAGE_LEVEL1_BASE_ADDRESS_MASK;
			}
		}
	} else {
		YL_MMU_ASSERT(0);
	}

	return pa;
}
#endif


static int g3dKmdDumpBinToFile(void *file, unsigned char *data, unsigned int hwData,
			       unsigned int size, unsigned int tag_id)
{
	/* move the following declarations to define */
	unsigned int ssp_phy_addr = (hwData >> 4);
	unsigned char ssp_tag[8] = "F0E0F0E";
	int retval = 0;

	if (!file)
		return -EINVAL;

	/* write header */
	retval = g_pKmdFileIo->fileWrite(file, (unsigned char *)ssp_tag, sizeof(ssp_tag));

	if (retval >= 0)
		retval = g_pKmdFileIo->fileWrite(file, (unsigned char *)&tag_id, sizeof(unsigned int));

	if (retval >= 0)
		retval = g_pKmdFileIo->fileWrite(file, (unsigned char *)ssp_tag, sizeof(ssp_tag));

	if (retval >= 0)
		retval = g_pKmdFileIo->fileWrite(file, (unsigned char *)&ssp_phy_addr, sizeof(unsigned int));

	if (retval >= 0)
		retval = g_pKmdFileIo->fileWrite(file, (unsigned char *)ssp_tag, sizeof(ssp_tag));

	/* write 4k content */
	if (retval >= 0)
		retval = g_pKmdFileIo->fileWrite(file, data, 4096);


	if (retval < 0) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN, "[SSP][g3dKmdDumpBinToFile] write error %d\n", retval);
		if (retval == -ENOSPC)
			sspEnable = 0;
	}

	return retval;
}


static int _g3dkmdDumpDram_FirstLevel(struct FirstPageEntry *entry0, unsigned int g3d_va, void *fp)
{
	unsigned int pa;
	unsigned int size;
	int numPage;
	void *data;
	int retval = 0;
	int j;

	(void)pa;
	(void)data;

	if (entry0->Section.sectionbit == 1) {
		/* 16M super section */
		pa = entry0->all & SUPER_SECTION_BASE_ADDRESS_MASK;
		size = MMU_SECTION_SIZE << 4;	/* MMU_SECTION_SIZE*16 */
	} else {
		/* 1M section */
		pa = entry0->all & SECTION_BASE_ADDRESS_MASK;
		size = MMU_SECTION_SIZE;
	}

#if defined(CMODEL) && (defined(WIN32) || (defined(linux) && defined(linux_user_mode)))
	/* ToDo: result will be wrong @ 64bit */
	data = (void *)(uintptr_t) g3dKmdMmuG3dVaToVa(hMmu, g3d_va);
	g3dKmdDumpDataToFile(fp, (unsigned char *)data, pa, size);
#elif defined(linux)

	numPage = size / MMU_SMALL_SIZE;
	for (j = 0; j < numPage; j++) {
		struct page *page = (pfn_to_page(pa >> PAGE_SHIFT));

		data = vmap(&page, 1, VM_MAP, PAGE_KERNEL);

		retval = g3dKmdDumpBinToFile(fp, (unsigned char *)data, pa,
						MMU_SMALL_SIZE,
						MMU_SECTION_SIZE_ID);


		vunmap(data);

		if (retval < 0)
			goto out;

		pa += MMU_SMALL_SIZE;
	}
#endif

out:
	return retval;

}


static int _g3dkmdDumpDram_SecondLevel(struct FirstPageEntry *entry0, unsigned int g3d_va, void *fp)
{
	struct SecondPageEntry *entry1;
	unsigned int pa;
	void *data;
	int retval = 0;
	int j;

	/* 4K or 64k, need to parse L2 table */
	unsigned int ltwo_pa = entry0->all & SMALL_PAGE_LEVEL0_BASE_ADDRESS_MASK; /* [31:10] */

#if defined(WIN32) || (defined(linux) && defined(linux_user_mode))
	G3dMemory *page_mem = &(((struct Mapper *) hMmu)->Page_mem);
	unsigned int *ltwo_va =
	    (unsigned int *)((uintptr_t) page_mem->data +
			     (ltwo_pa - page_mem->hwAddr));
#elif defined(linux)
	struct page *l2_page = (pfn_to_page(ltwo_pa >> PAGE_SHIFT));
	unsigned int *ltwo_va = (unsigned int *)vmap(&l2_page, 1, VM_MAP, PAGE_KERNEL);
#endif
	unsigned int g3d_va2 = g3d_va;

	entry1 = (struct SecondPageEntry *) ((uintptr_t) ltwo_va | (ltwo_pa & 0xC00));

	for (j = 0; j < (int)SMALL_LENGTH; j++) { /* (1M/4k) how many 4k unit */
	/*if (entry1->Common.type) { */
		/* unsigned int entry1_type = (entry1->all & 0x3); */

		if (entry1->Common.type == SECOND_TYPE_SMALL) { /* 10 (4k) */
			/* 4k */
#if defined(CMODEL) && (defined(WIN32) || (defined(linux) && defined(linux_user_mode)))
			pa = entry1->all & SMALL_PAGE_LEVEL1_BASE_ADDRESS_MASK;

			/* ToDo: result will be wrong @ 64bit */
			data = (void *)(uintptr_t) g3dKmdMmuG3dVaToVa(hMmu, g3d_va2);
			g3dKmdDumpDataToFile(fp,
					     (unsigned char *)data,
					     pa, MMU_SMALL_SIZE);
#elif defined(linux)
			struct page *page;

			pa = entry1->all & SMALL_PAGE_LEVEL1_BASE_ADDRESS_MASK;
			page = (pfn_to_page(pa >> PAGE_SHIFT));
			data = vmap(&page, 1, VM_MAP, PAGE_KERNEL);


			retval = g3dKmdDumpBinToFile(fp, (unsigned char *) data, pa, MMU_SMALL_SIZE,
						     MMU_SMALL_SIZE_ID);

			vunmap(data);

			if (retval < 0) {
				vunmap(ltwo_va);
				goto out;
			}
#endif
			entry1++;
			g3d_va2 += MMU_SMALL_SIZE;
		} else if (entry1->Common.type == SECOND_TYPE_LARGE) {/* 01 (64k) */
#if defined(CMODEL) && (defined(WIN32) || (defined(linux) && defined(linux_user_mode)))
			pa = entry1->all & LARGE_PAGE_LEVEL1_BASE_ADDRESS_MASK;

			/* ToDo: result will be wrong @ 64bit */
			data = (void *)(uintptr_t) g3dKmdMmuG3dVaToVa(hMmu, g3d_va2);
			g3dKmdDumpDataToFile(fp, (unsigned char *)data, pa, MMU_LARGE_SIZE);
#elif defined(linux)
			unsigned int k = 0;
			struct page *page;

			pa = entry1->all & LARGE_PAGE_LEVEL1_BASE_ADDRESS_MASK;

			for (k = 0; k < MMU_LARGE_SIZE / MMU_SMALL_SIZE; k++) { /* 16 */
				page = (pfn_to_page(pa >> PAGE_SHIFT));
				data = vmap(&page, 1, VM_MAP, PAGE_KERNEL);

				retval = g3dKmdDumpBinToFile(fp, (unsigned char *) data, pa, MMU_SMALL_SIZE,
								MMU_LARGE_SIZE_ID);


				vunmap(data);

				if (retval < 0)
					goto out;

				pa += MMU_SMALL_SIZE;
			}

#endif
			entry1 += (MMU_LARGE_SIZE / MMU_SMALL_SIZE);	/* 16 */
			g3d_va2 += MMU_LARGE_SIZE;
		} else {
			entry1++;
		}
	/*}	*//* if (entry1->Common.type) 4k or 64k */
	}	/* for (j=0; j<(int)SMALL_LENGTH; j++) //(1M/4k) how many 4k unit */

#if defined(linux) && !defined(linux_user_mode)
	vunmap(ltwo_va);
#endif


out:
	return retval;
}




static void _g3dKmdDumpDram(void)
{
	void *fp;
	char *fileName = NULL;
	struct FirstPageTable *pageTable;
	struct FirstPageEntry *entry0;
	void *hMmu = g3dKmdMmuGetHandle();
	unsigned int g3d_va;
	unsigned int size;
	void *swAddr, *hwAddr;
	int i;
	int retval = 0;


	if (!hMmu)
		return;

	fileName =  (char *)G3DKMDMALLOC(SSP_PATHNAMEMAX+SSP_FILENAMEMAX);
	if (!fileName) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN, "[SSP][_g3dKmdDumpDram] G3DKMDMALLOC fail\n");
		return;
	}

#if defined(ANDROID) && !defined(YL_FAKE_X11)
	sprintf(fileName, "%s/g3dssp/g3dssp_dram_%x.bin", ssp_path, s_ssp_count);
#else
	sprintf(fileName, "g3dssp_dram_%x.bin", s_ssp_count);
#endif

	fp = g_pKmdFileIo->fileOpen(fileName);

	G3DKMDFREE(fileName);

	if (!fp)
		return;

	g3dKmdMmuGetTable(hMmu, &swAddr, &hwAddr, &size);
	pageTable = (struct FirstPageTable *) swAddr;

	entry0 = pageTable->entries;

	g3d_va = 0; /*g3dva_offset */;

	/* TODO: SECTION_LENGTH need to change according to real case */
	for (i = 0; i < (int)(size / sizeof(struct FirstPageEntry)); i++) {
		if (entry0->Common.type == FIRST_TYPE_TABLE) { /* 01 is page table */
			retval = _g3dkmdDumpDram_SecondLevel(entry0, g3d_va, fp);
			if (retval < 0)
				goto out;
		} /* L1 is table */
		else if (entry0->Common.type == FIRST_TYPE_SECTION) {
			retval = _g3dkmdDumpDram_FirstLevel(entry0, g3d_va, fp);
			if (retval < 0)
				goto out;
		}
		/*
		else
		{
			YL_MMU_ASSERT(0);
		}
		*/
		entry0++;
		g3d_va += MMU_SECTION_SIZE;
	}
out:
	g_pKmdFileIo->fileClose(fp);

}


/* to dump data to file, */
/* base on g3dKmdDumpDataToFilePage() but no va given */
static void g3dKmdDumpDataToFilePage_noVa(void *hMmu, void *file, unsigned int g3d_va,
					  unsigned int size)
{
	struct FirstPageTable *pageTable;
	unsigned int /*ptr */ g3dva_ptr = g3d_va;
	unsigned int size_left = size;
	unsigned char *data;
	void *swAddr, *hwAddr;

	(void)data;

	/* get MMU table */
	g3dKmdMmuGetTable(hMmu, &swAddr, &hwAddr, &size);
	pageTable = (struct FirstPageTable *) swAddr;

	while (size_left != 0) {
		unsigned int index0 = PageTable_index0_from_g3d_va(g3dva_ptr /*- g3dva_offset*/);
		struct FirstPageEntry entry0 = pageTable->entries[index0];
		unsigned int offset_mask = 0;
		unsigned int unit_size = 0;
		unsigned int /*ptr */ pa, pa_head;
		unsigned int current_size;

		if (entry0.Common.type == FIRST_TYPE_SECTION) {
			unit_size = MMU_SECTION_SIZE;	/* 1M */
			offset_mask = SECTION_OFFSET_MASK;	/* 0xFFFFF */
		} else if (entry0.Common.type == FIRST_TYPE_TABLE) {
			unit_size = MMU_SMALL_SIZE;	/* 4K */
			offset_mask = SMALL_PAGE_OFFSET_MASK;	/* 0xFFF */
		} else {
			YL_MMU_ASSERT(0);
		}

		/* g3dva => pa */
		pa = g3dKmdMmuG3dVaToPa(hMmu, g3dva_ptr);

		pa_head = pa & ~(offset_mask);

		/* current_size */
		current_size = unit_size - (pa & offset_mask);
		if (size_left < current_size)
			current_size = size_left;

		/* get VA */
#if defined(CMODEL) && (defined(WIN32) || (defined(linux) && defined(linux_user_mode)))
		(void)pa_head;
		/* ToDo: result will be wrong @ 64bit */
		data = (unsigned char *)(uintptr_t) g3dKmdMmuG3dVaToVa(hMmu, g3dva_ptr);
		g3dKmdDumpDataToFile(file, data, pa, current_size);
#elif defined(linux)
		{

			struct page *page;

			page = (pfn_to_page(pa_head >> PAGE_SHIFT));
			data = vmap(&page, 1, VM_MAP, PAGE_KERNEL);


			g3dKmdDumpDataToFile(file, data + (pa & offset_mask), pa, current_size);
			vunmap(data);


		}
#endif
		g3dva_ptr += current_size;
		size_left -= current_size;
	}

}

static void _g3dKmdDumpFrameCmds(struct g3dExeInst *pInst, unsigned int g3d_va, int length)
{
	void *fp;
	char *fileName = NULL;
	void *hMmu = g3dKmdMmuGetHandle();

	if (!hMmu)
		return;

	if (length <= 0)
		return;

	fileName = (char *)G3DKMDMALLOC(SSP_PATHNAMEMAX + SSP_FILENAMEMAX);
	if (!fileName) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN, "[SSP][_g3dKmdDumpFrameCmds] G3DKMDMALLOC fail\n");
		return;
	}


#if defined(ANDROID) && !defined(YL_FAKE_X11)
	sprintf(fileName, "%s/g3dssp/g3dssp_fcmd_%x.hex", ssp_path, s_ssp_count);
#else
	sprintf(fileName, "g3dssp_fcmd_%x.hex", s_ssp_count);
#endif

	fp = g_pKmdFileIo->fileOpen(fileName);

	G3DKMDFREE(fileName);

	if (!fp)
		return;

	g3dKmdDumpDataToFilePage_noVa(hMmu, fp, g3d_va, length);

	g_pKmdFileIo->fileClose(fp);

}

static int _g3dKmdDumpHt(struct g3dExeInst *pInst,  int taskID, enum sspMode mode)
{
	void *fp;
	unsigned int dwTemp;
	char *fileName = NULL;
	unsigned char *pCmd = NULL;
	void *hMmu = g3dKmdMmuGetHandle();
	int retval = 0;

	if (!hMmu) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN, "[%s] Error! hMmu is NULL\n", __func__);
		return -1;
	}

	if (pInst == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN, "[%s] Error! pInst is NULL\n", __func__);
		return -1;
	}
	/* (0) calculate head/tail entry address */
	if (mode == SSP_DUMP_PREVIOUS || mode == SSP_DUMP_CURRENT) {
		dwTemp = g3dKmdRegGetReadPtr(pInst->hwChannel);

		if ((dwTemp >= pInst->pHtDataForHwRead->htInfo->htCommandBaseHw)
		    && (dwTemp < pInst->pHtDataForHwRead->htInfo->htCommandBaseHw + HT_BUF_SIZE)) {

			if (mode == SSP_DUMP_PREVIOUS) {
				dwTemp -= sizeof(struct g3dHtCommand);

				if (dwTemp < pInst->pHtDataForHwRead->htBuffer.hwAddr)
					dwTemp += pInst->pHtDataForHwRead->htBuffer.size;
			}
			/*
			pr_debug("ssp dump h/t at %x, h/t base at = %x\n",
				dwTemp, pInst->pHtDataForHwRead->htBuffer.hwAddr);
			*/

			pCmd = (unsigned char *)pInst->pHtDataForHwRead->htInfo->htCommand +
				(dwTemp - pInst->pHtDataForHwRead->htInfo->htCommandBaseHw);
		} else {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN,
			       "[%s] Error! g3dKmdRegGetReadPtr(pInst->hwChannel) = 0x%x, skip dump HT_CMD\n",
			       __func__, dwTemp);
			return -1;
		}
	} else {
	/*
	 * SSP_DUMP_BYTASKID case, dump taskid last command.
	 *
	 */

		struct g3dHtInfo *ht = pInst->pHtDataForSwWrite->htInfo;
		unsigned int wtIndex = (ht->wtPtr - ht->htCommandBaseHw) / HT_CMD_SIZE;
		unsigned int lastIndex = MAX_HT_COMMAND - 1;
		struct g3dHtCommand *pHtCmd;
		int found = G3DKMD_FALSE;
		int i;

		for (i = wtIndex-1; i >= 0; i--) {
			/* search from write index - 1 to  first index */
			pHtCmd = &pInst->pHtDataForSwWrite->htInfo->htCommand[i];
			if (pHtCmd->command == (unsigned int)taskID) {
				found = G3DKMD_TRUE;
				pCmd = (unsigned char *)pHtCmd;
				break;
			}
		}

		if (!found && wtIndex != lastIndex) {
			/* not found yet, search from last index to write index + 1. */
			for (i = lastIndex; i > wtIndex; i--) {
				pHtCmd = &pInst->pHtDataForSwWrite->htInfo->htCommand[i];
				if (pHtCmd->command == (unsigned int)taskID) {
					found = G3DKMD_TRUE;
					pCmd = (unsigned char *)pHtCmd;
					break;
				}
			}
		}

		if (!found) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN,
				"[%s] Error! can't find tasID %d last HT_CMD, skip dump HT_CMD\n",
				 __func__, taskID);
			return -1;
		}

	}


	fileName = (char *)G3DKMDMALLOC(SSP_PATHNAMEMAX+SSP_FILENAMEMAX);
	if (!fileName) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN, "[SSP][_g3dKmdDumpHt] G3DKMDMALLOC fail\n");
		return -1;
	}

	/*(1) dump H/T misc info */
#if defined(ANDROID) && !defined(YL_FAKE_X11)
	sprintf(fileName, "%s/g3dssp/g3dssp_ht_%x.hex", ssp_path, s_ssp_count);
#else
	sprintf(fileName, "g3dssp_ht_%x.hex", s_ssp_count);
#endif

	fp = g_pKmdFileIo->fileOpen(fileName);

	if (fp) {
		/* dump H/T buffer PA to file */
		_g3dKmdDumpSwData(fp, "HT_MVA", &pInst->pHtDataForHwRead->htBuffer.hwAddr,
				  sizeof(unsigned int));
		dwTemp = g3dKmdMmuG3dVaToPa(hMmu, pInst->pHtDataForHwRead->htBuffer.hwAddr);
		_g3dKmdDumpSwData(fp, "HT_PA", &dwTemp, sizeof(dwTemp));

		if (pCmd) {
			/* dump ht command */
			_g3dKmdDumpSwData(fp, "HT_CMD", pCmd, sizeof(struct g3dHtCommand));
		}
		/* dump Qbuffer PA */
		dwTemp = g3dKmdMmuG3dVaToPa(hMmu, pInst->qbaseBuffer.hwAddr);
		_g3dKmdDumpSwData(fp, "QBUF_PA", &dwTemp, sizeof(dwTemp));

		g_pKmdFileIo->fileClose(fp);

	}

	if (pCmd) {
		/* (2) dump frame commands to file */
		_g3dKmdDumpFrameCmds(pInst, ((struct g3dHtCommand *) pCmd)->val1,
				     ((struct g3dHtCommand *) pCmd)->val2 - ((struct g3dHtCommand *) pCmd)->val1);
	}
	/* (3) dump whole H/T buffer to file */
#if defined(ANDROID) && !defined(YL_FAKE_X11)
	sprintf(fileName, "%s/g3dssp/g3dssp_whole_ht_%x.hex", ssp_path, s_ssp_count);
#else
	sprintf(fileName, "g3dssp_whole_ht_%x.hex", s_ssp_count);
#endif

	fp = g_pKmdFileIo->fileOpen(fileName);

	if (fp) {
		_g3dKmdDumpSwData(fp, "HT_CMD", pInst->pHtDataForHwRead->htBuffer.data,
				  pInst->pHtDataForHwRead->htBuffer.size);
		g_pKmdFileIo->fileClose(fp);
	}

	G3DKMDFREE(fileName);

	return retval;
}

static void _g3dKmdDumpRegs(struct g3dExeInst *pInst)
{
	void *fp;
	unsigned int dwTemp;
	char *fileName = NULL;

	if (pInst == NULL)
		return;

	if (!g3dKmdMmuGetHandle())	/* mmu must enable */
		return;


	fileName = (char *)G3DKMDMALLOC(SSP_PATHNAMEMAX + SSP_FILENAMEMAX);
	if (!fileName) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN, "[SSP][_g3dKmdDumpRegs] G3DKMDMALLOC fail\n");
		return;
	}
	/* open file */
#if defined(ANDROID) && !defined(YL_FAKE_X11)
	sprintf(fileName, "%s/g3dssp/g3dssp_reg_%x.hex", ssp_path, s_ssp_count);
#else
	sprintf(fileName, "g3dssp_reg_%x.hex", s_ssp_count);
#endif

	fp = g_pKmdFileIo->fileOpen(fileName);

	G3DKMDFREE(fileName);

	if (!fp)
		return;

	dwTemp = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PM_DRAM_START_ADDR,
				MSK_AREG_PM_DRAM_START_ADDR, SFT_AREG_PM_DRAM_START_ADDR);

	dwTemp <<= SFT_AREG_PM_DRAM_START_ADDR;
	_g3dKmdDumpSwData(fp, "REG_AREG_PM_DRAM_START_ADDR", &dwTemp, sizeof(dwTemp));

	dwTemp = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PM_DRAM_SIZE, MSK_AREG_PM_DRAM_SIZE, SFT_AREG_PM_DRAM_SIZE);
	dwTemp <<= SFT_AREG_PM_DRAM_SIZE;
	_g3dKmdDumpSwData(fp, "REG_AREG_PM_DRAM_SIZE", &dwTemp, sizeof(dwTemp));

	dwTemp = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FDQBUF_BASE, MSK_AREG_PA_FDQBUF_BASE,
				SFT_AREG_PA_FDQBUF_BASE);

	dwTemp <<= SFT_AREG_PA_FDQBUF_BASE;
	_g3dKmdDumpSwData(fp, "REG_AREG_PA_FDQBUF_BASE", &dwTemp, sizeof(dwTemp));

	dwTemp = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FDQBUF_SIZE,
				MSK_AREG_PA_FDQBUF_SIZE, SFT_AREG_PA_FDQBUF_SIZE);

	dwTemp <<= SFT_AREG_PA_FDQBUF_SIZE;
	_g3dKmdDumpSwData(fp, "REG_AREG_PA_FDQBUF_SIZE", &dwTemp, sizeof(dwTemp));


#ifdef G3DKMD_SUPPORT_IOMMU
	dwTemp = g3dKmdIommuGetPtBase(IOMMU_MOD_1);
#ifdef G3DKMD_SUPPORT_EXT_IOMMU
	dwTemp = g3dKmdIommuGetPtBase(IOMMU_MOD_2);
#endif
#endif
	_g3dKmdDumpSwData(fp, "REG_MMU_PT_BASE_ADDR", &dwTemp, sizeof(dwTemp));

#ifdef YL_HW_DEBUG_DUMP_TEMP
	dwTemp = g3dKmdHwDbgGetDumpFlushBase();
	_g3dKmdDumpSwData(fp, "REG_MX_DBG_INTERNAL_DUMP", &dwTemp, sizeof(dwTemp));
#endif

	dwTemp = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_CQ_QBUF_START_ADDR,
				MSK_AREG_CQ_QBUF_START_ADDR, SFT_AREG_CQ_QBUF_START_ADDR);

	dwTemp <<= SFT_AREG_CQ_QBUF_START_ADDR;
	_g3dKmdDumpSwData(fp, "REG_AREG_CQ_QBUF_START_ADDR", &dwTemp, sizeof(dwTemp));

	dwTemp = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_CQ_QBUF_END_ADDR,
				MSK_AREG_CQ_QBUF_END_ADDR, SFT_AREG_CQ_QBUF_END_ADDR);

	dwTemp <<= SFT_AREG_CQ_QBUF_END_ADDR;
	_g3dKmdDumpSwData(fp, "REG_AREG_CQ_QBUF_END_ADDR", &dwTemp, sizeof(dwTemp));

	/* close file */
	g_pKmdFileIo->fileClose(fp);

}

unsigned int g3dKmdSnapshotPattern(int taskID, enum sspMode mode)
{
	struct g3dExeInst *pInst;
	int retval;
	char *fileName = NULL;
	void *fp;

	if (!sspEnable) {
		KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_PATTERN,
		       "[SSP][g3dKmdSanpshotPattern] SSP not enable\n");
		return G3DKMDRET_FAIL;
	}

	if (mode == SSP_DUMP_BYTASKID && taskID < 0) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN,
			"[SSP][g3dKmdSnapshotPattern] SSP_DUMP_BYTASKID, invalid taskID = %d\n", taskID);
		return G3DKMDRET_FAIL;
	}

	pInst = (taskID < 0) ? gExecInfo.currentExecInst : g3dKmdTaskGetInstPtr(taskID);
	if (pInst == NULL) {
		KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_PATTERN, "[SSP][g3dKmdSnapshotPattern] pInst == NULL\n");

		return G3DKMDRET_FAIL;
	}


	if (!g3dKmdMmuGetHandle())/* mmu must enable */
		return G3DKMDRET_FAIL;

	/* a temp solution to workaround h/t readptr not valid issue on linux */
	if (mode == SSP_DUMP_PREVIOUS) {
		unsigned int dwTemp = g3dKmdRegGetReadPtr(pInst->hwChannel);/* pInst->htInfo->wtPtr */

		if (dwTemp < pInst->pHtDataForHwRead->htBuffer.hwAddr
		    || (pInst->htCommandFlushedCount < 2
			&& pInst->htCommandFlushedCountOverflow == 0))
			return G3DKMDRET_FAIL;
	} else if (mode == SSP_DUMP_CURRENT) {

		if (g3dKmdRegGetReadPtr(pInst->hwChannel) == pInst->pHtDataForHwRead->htInfo->wtPtr)
			mode = SSP_DUMP_PREVIOUS;
	}


	/* switch fileIO to file dump directly */
	g3dKmdBackupFileIo();
	g3dKmdSetFileIo(G3DKMD_FILEIO_KFILE);

	/* dump H/T data */
	retval = _g3dKmdDumpHt(pInst, taskID, mode);

	if (retval == 0) {
		/* dump mmu table */
		_g3dKmdDumpMMUTable(pInst, 0, 1);

		/* dump valid DRAM data via page table */
		_g3dKmdDumpDram();

		/* dump registers */
		_g3dKmdDumpRegs(pInst);
	}


	fileName = (char *)G3DKMDMALLOC(SSP_PATHNAMEMAX+SSP_FILENAMEMAX);
	if (fileName) {
		if (retval < 0) {
			/* open file */
#if defined(ANDROID) && !defined(YL_FAKE_X11)
			sprintf(fileName, "%s/g3dssp/g3dssp_error_%x.hex", ssp_path, s_ssp_count);
#else
			sprintf(fileName, "g3dssp_error_%x.hex", s_ssp_count);
#endif

		} else {
#if defined(ANDROID) && !defined(YL_FAKE_X11)
			sprintf(fileName, "%s/g3dssp/g3dssp_done_%x.hex", ssp_path, s_ssp_count);
#else
			sprintf(fileName, "g3dssp_done_%x.hex", s_ssp_count);
#endif
		}

		fp = g_pKmdFileIo->fileOpen(fileName);
		if (fp)
			g_pKmdFileIo->fileClose(fp);

		G3DKMDFREE(fileName);
	} else {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PATTERN, "[SSP][g3dKmdSnapshotPattern] G3DKMDMALLOC fail\n");
	}
	/* restore original fileIO mode */
	g3dKmdRestoreFileIo();

	s_ssp_count++;

	return (retval == 0) ? G3DKMDRET_SUCCESS : G3DKMDRET_FAIL;

}


unsigned int g3dKmdSSPEnable(int bIsQuery, int *pValue)
{

	if (bIsQuery) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_PATTERN,
		       "[SSP][g3dKmdSSPEnable] Get sspEnable = %d\n", sspEnable);
		*pValue = sspEnable;
	} else {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_PATTERN,
		       "[SSP][g3dKmdSSPEnable] Set sspEnable = %d\n", *pValue);
		sspEnable = *pValue;
	}

	return G3DKMDRET_SUCCESS;
}

int g3dkmdSSPPath(int bIsQuery, char *path, int len)
{
	if (bIsQuery) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_PATTERN, "[SSP][g3dkmdSSPPath] Get ssp_path = %s\n", ssp_path);

		strcpy(path, ssp_path);
	} else {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_PATTERN,
			"[SSP][g3dkmdSSPPath] set ssp_path = %s, len = %d\n", path, len);

		if (len < SSP_PATHNAMEMAX)
			strcpy(ssp_path, path);
	}

	return G3DKMDRET_SUCCESS;
}
#endif /* G3DKMD_SNAPSHOT_PATTERN */
