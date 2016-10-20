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

#ifndef _G3DKMD_MMCE_H_
#define _G3DKMD_MMCE_H_

#include "g3dkmd_define.h"

#ifdef G3DKMD_SUPPORT_EXT_IOMMU
	/* #define MMCE_SUPPORT_IOMMU */
#endif

#define MMCE_INVALID_THD_ID     0xFFFFFFFF
#define MMCE_INVALID_THD_HANDLE 0xFFFFFFFF

typedef unsigned int MMCE_THD_HANDLE;

void g3dKmdMmceInit(void);
void g3dKmdMmceResetCore(void);
unsigned int g3dKmdMmceResetThread(unsigned int thd_id);
void g3dKmdMmceResetThreads(void);
MMCE_THD_HANDLE g3dKmdMmceGetFreeThd(void);
void g3dKmdMmceWriteToken(unsigned int token_id, unsigned int value);

unsigned int g3dKmdMmceGetThreadId(MMCE_THD_HANDLE);
unsigned int g3dKmdMmceGetReadPtr(MMCE_THD_HANDLE);
#ifdef PATTERN_DUMP
void g3dKmdMmceSetReadPtr(MMCE_THD_HANDLE handle, unsigned int phy_addr);
#endif

void g3dKmdMmceResume(MMCE_THD_HANDLE, void *, unsigned int hwChannel);
void g3dKmdMmceSuspend(MMCE_THD_HANDLE, unsigned int hwChannel);
void g3dKmdMmceFlush(MMCE_THD_HANDLE);
void g3dKmdMmceWaitIdle(MMCE_THD_HANDLE handle);
#endif /* _G3DKMD_MMCE_H_ */
