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
#include "g3dkmd_hwdbg.h"

#ifdef G3DKMD_SUPPORT_HW_DEBUG

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)
#include <windows.h>
#include <stdlib.h>
#elif defined(linux) && defined(linux_user_mode) /* linux user space */
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#else
#include <linux/kernel.h>
#include <linux/module.h>
#endif

#include "g3dkmd_util.h"
#include "g3dkmd_internal_memory.h"
#include "g3dkmd_macro.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_hwdbg.h"

#ifdef YL_HW_DEBUG_DUMP_TEMP
G3dMemory gMem;

static unsigned int beginFrameID;
static unsigned int endFrameID;
static unsigned int repeatTimes;

REG_ACCESS_DATA_IMP(MX)
REG_ACCESS_FUNC_IMP(MX)

void g3dKmdHwDbgAllocMemory(unsigned int size, unsigned int align)
{
	unsigned int flags = G3DKMD_MMU_SHARE;

#if defined(G3D_NONCACHE_BUFFERS)
	flags |= G3DKMD_MMU_DMA;
#endif

	if (!allocatePhysicalMemory(&gMem, size, align, G3DKMD_MMU_ENABLE | flags)) {
		YL_KMD_ASSERT(0);
		return;
	}

	g3dKmdHwDbgSetDumpFlushBase();
}

void g3dKmdHwDbgFreeMemory(void)
{
	freePhysicalMemory(&gMem);
}

void g3dKmdHwDbgSetDumpFlushBase(void)
{
	g3dKmdRegWrite(G3DKMD_REG_MX, REG_MX_DBG_INTERNAL_DUMP, gMem.hwAddr,
		       MSK_MX_DBG_DBG_DUMP_LDRAM_FLUSH_BASE, 0);
}

void g3dKmdHwDbgSetDumpReg(unsigned int value)
{
	g3dKmdRegWrite(G3DKMD_REG_MX, REG_MX_DBG_INTERNAL_DUMP, value,
		       MSK_MX_DBG_DBG_DUMP_LDRAM_ENABLE, 0);
	g3dKmdRegWrite(G3DKMD_REG_MX, REG_MX_DBG_INTERNAL_DUMP, value,
		       MSK_MX_DBG_DBG_DUMP_LDRAM_FLUSH_EN, 0);
}

void g3dKmdHwDbgSetFrameID(unsigned int uBegin, unsigned int uEnd)
{
	beginFrameID = uBegin;
	endFrameID = uEnd;
}

unsigned int g3dKmdHwDbgGetBeginFrameID(void)
{
	return beginFrameID;
}

unsigned int g3dKmdHwDbgGetEndFrameID(void)
{
	return endFrameID;
}

void g3dKmdHwDbgSetRepeatTimes(unsigned int uTimes)
{
	repeatTimes = uTimes;
}

unsigned int g3dKmdHwDbgGetRepeatTimes(void)
{
	return repeatTimes;
}

#ifdef G3DKMD_SNAPSHOT_PATTERN
unsigned int g3dKmdHwDbgGetDumpFlushBase(void)
{
	unsigned int temp =
	    g3dKmdRegRead(G3DKMD_REG_MX, REG_MX_DBG_INTERNAL_DUMP,
			  MSK_MX_DBG_DBG_DUMP_LDRAM_FLUSH_BASE,
			  SFT_MX_DBG_DBG_DUMP_LDRAM_FLUSH_BASE);
	return temp << SFT_MX_DBG_DBG_DUMP_LDRAM_FLUSH_BASE;
}
#endif

#endif

#endif /* G3DKMD_SUPPORT_HW_DEBUG */
