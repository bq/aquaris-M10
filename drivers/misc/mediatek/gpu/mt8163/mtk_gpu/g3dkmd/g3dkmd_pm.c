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

#ifdef G3DKMD_SUPPORT_PM

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)
#include <windows.h>
#include <stdlib.h>
#elif defined(linux) && defined(linux_user_mode)	/* linux user space */
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#else
#include <linux/kernel.h>
#include <linux/module.h>
#endif

#include "g3dkmd_util.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_internal_memory.h"
#include "g3dkmd_macro.h"
#include "g3dkmd_task.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_api.h"
#include "g3dkmd_pm.h"
#include "g3dkmd_memory_list.h"	/* for memory total usage. */

#if !defined(linux) || defined(linux_user_mode)
#define EXPORT_SYMBOL(x)
#endif

void g3dKmdKmifAllocMemoryLocked(unsigned int size, unsigned int align)
{
	unsigned int flags = G3DKMD_MMU_ENABLE | G3DKMD_MMU_SHARE | G3DKMD_MMU_DMA;

	if (!allocatePhysicalMemory(&gExecInfo.pm, size, align, flags)) {
		YL_KMD_ASSERT(0);
		return;
	}

	gExecInfo.kmif_mem_dec.hw_addr = gExecInfo.pm.hwAddr;
	gExecInfo.kmif_mem_dec.size = gExecInfo.pm.size;
	gExecInfo.kmif_mem_dec.vaddr = gExecInfo.pm.data;

	/* confirm1 set or not with perf team, current to follow the original way */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PM_DRAM_START_ADDR, gExecInfo.pm.hwAddr,
		       MSK_AREG_PM_DRAM_START_ADDR, SFT_AREG_PM_DRAM_START_ADDR);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PM_DRAM_SIZE, gExecInfo.pm.size,
		       MSK_AREG_PM_DRAM_SIZE, SFT_AREG_PM_DRAM_SIZE);
}

void g3dKmdKmifFreeMemoryLocked(void)
{
	freePhysicalMemory(&gExecInfo.pm);
}

int g3dKmdKmifRegister(G3DKMD_KMIF_OPS *ops)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM, "g3dKmdKmifRegister()\n");
	lock_g3d();
	memcpy(&gExecInfo.kmif_ops, ops, sizeof(G3DKMD_KMIF_OPS));
	unlock_g3d();
	return 0;
}
EXPORT_SYMBOL(g3dKmdKmifRegister);

int g3dKmdKmifDeregister(void)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM, "g3dKmdKmifDeregister()\n");
	lock_g3d();
	memset(&gExecInfo.kmif_ops, 0, sizeof(gExecInfo.kmif_ops));
	unlock_g3d();
	return 0;
}
EXPORT_SYMBOL(g3dKmdKmifDeregister);

G3DKMD_KMIF_MEM_DESC *g3dKmdKmifAllocMemory(unsigned int size, unsigned int align)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM, "g3dKmdKmifAllocMemory(0x%x, 0x%x)\n", size,
	       align);
	lock_g3d();
	g3dKmdKmifAllocMemoryLocked(size, align);
	unlock_g3d();
	return &gExecInfo.kmif_mem_dec;
}
EXPORT_SYMBOL(g3dKmdKmifAllocMemory);

void g3dKmdKmifFreeMemory(G3DKMD_KMIF_MEM_DESC *desc)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM, "g3dKmdKmifFreeMemory()\n");
	lock_g3d();
	g3dKmdKmifFreeMemoryLocked();
	memset(desc, 0, sizeof(G3DKMD_KMIF_MEM_DESC));
	unlock_g3d();
}
EXPORT_SYMBOL(g3dKmdKmifFreeMemory);

unsigned char *g3dKmdKmifGetRegBase(void)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM, "g3dKmdKmifGetRegBase()\n");
	return (unsigned char *)YL_BASE;
}
EXPORT_SYMBOL(g3dKmdKmifGetRegBase);

unsigned char *g3dKmdKmifGetMMURegBase(void)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM, "g3dKmdKmifGetMMURegBase()\n");
	return (unsigned char *)IOMMU_REG_BASE;
}
EXPORT_SYMBOL(g3dKmdKmifGetMMURegBase);

unsigned int g3dKmdKmifMvaToPa(unsigned int mva)
{
	void *hMmu = g3dKmdMmuGetHandle();

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM, "g3dKmdKmifMvaToPa()\n");

	if (!hMmu) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PM,
		       "g3dKmdKmifMvaToPa: can't get mmu handle\n");
		return 0;
	}
	return g3dKmdMmuG3dVaToPa(hMmu, mva);
}
EXPORT_SYMBOL(g3dKmdKmifMvaToPa);

void g3dKmdKmifSetShaderDebugMode(int enable)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM, "g3dKmdKmifChangeShaderDebugMode()\n");
	lock_g3d();
	gExecInfo.kmif_shader_debug_enable = enable;
	unlock_g3d();
}
EXPORT_SYMBOL(g3dKmdKmifSetShaderDebugMode);

void g3dKmdKmifSetDFDDebugMode(int enable)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM,
	       "TODO: g3dKmdKmifChangeDFDDebugMode(): enable: %d\n", enable);
	lock_g3d();
	gExecInfo.kmif_dfd_debug_enable = enable;
	/* (1) disable recovery in g3dkmd_api.c */
	/* (1) disable timeout */

	unlock_g3d();

}
EXPORT_SYMBOL(g3dKmdKmifSetDFDDebugMode);

/*
 * Allocate a buffer for dfd to sotre internal dump inforamtion
 */

void g3dKmdKmifDFDAllocMemoryLocked(unsigned int size, unsigned int align)
{
	unsigned int flags = G3DKMD_MMU_SHARE | G3DKMD_MMU_DMA;

	if (!allocatePhysicalMemory(&gExecInfo.dfd, size, align, G3DKMD_MMU_ENABLE | flags)) {
		YL_KMD_ASSERT(0);
		return;
	}
	gExecInfo.kmif_dfd_mem_dec.hw_addr = gExecInfo.dfd.hwAddr;
	gExecInfo.kmif_dfd_mem_dec.size = gExecInfo.dfd.size;
	gExecInfo.kmif_dfd_mem_dec.vaddr = gExecInfo.dfd.data;

	g3dKmdRegWrite(G3DKMD_REG_MX, REG_MX_DBG_INTERNAL_DUMP, gExecInfo.dfd.hwAddr,
		       MSK_MX_DBG_DBG_DUMP_LDRAM_FLUSH_BASE, 0);
}

void g3dKmdDFDFreeMemoryLocked(void)
{
	freePhysicalMemory(&gExecInfo.dfd);
}

G3DKMD_KMIF_MEM_DESC *g3dKmdKmifDFDAllocMemory(unsigned int size, unsigned int align)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM, "g3dKmdKmifAllocMemory(0x%x, 0x%x)\n", size,
	       align);
	lock_g3d();
	g3dKmdKmifDFDAllocMemoryLocked(size, align);
	unlock_g3d();
	return &gExecInfo.kmif_dfd_mem_dec;
}
EXPORT_SYMBOL(g3dKmdKmifDFDAllocMemory);

void g3dKmdKmifDFDFreeMemory(G3DKMD_KMIF_MEM_DESC *desc)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM, "g3dKmdKmifFreeMemory()\n");
	lock_g3d();
	g3dKmdDFDFreeMemoryLocked();
	memset(desc, 0, sizeof(G3DKMD_KMIF_MEM_DESC));
	unlock_g3d();
}
EXPORT_SYMBOL(g3dKmdKmifDFDFreeMemory);


unsigned int g3dKmdKmifQueryMemoryUsage_MET(void)
{
	unsigned int retVal = 0;

	/* KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_PM, "\n"); */

	{ /* get the memory usage from g3dkmd. */
		retVal = _g3dKmdMemList_GetTotalMemoryUsage();
	}
	return retVal;
}
EXPORT_SYMBOL(g3dKmdKmifQueryMemoryUsage_MET);

#endif /* G3DKMD_SUPPORT_PM */
