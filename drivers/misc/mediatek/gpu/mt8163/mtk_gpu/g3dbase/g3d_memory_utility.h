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

/* Shared by G3D / G3DKMD */
#ifndef _G3D_MEMORY_UTILITY_H_
#define _G3D_MEMORY_UTILITY_H_

#include "g3dbase_common.h"
#include "g3d_memory.h"

static INLINE void g3dbaseCleanG3dMemory(G3dMemory *target)
{
	target->data0 = 0;
	target->data = 0;
	target->hwAddr0 = 0;
	target->hwAddr = 0;
	target->phyAddr = 0;
	target->size0 = 0;
	target->size = 0;
	target->remap = 0;
/* #if defined(YL_PATTERN) && defined(CMODEL) */
	target->patPhyAddr = 0;
#ifdef YL_MMU_PATTERN_DUMP_V1
	target->pa_pair_info = 0;
#endif
/* #endif */
	target->deferAlloc = 0;
/* target->type    = G3D_MEM_NONE; */
}


static INLINE unsigned int g3dbaseVaToContinuousPa(const G3dMemory *base, void *va)
{
	return (unsigned int)((uintptr_t) va - (uintptr_t) base->data) + base->hwAddr;
}

static INLINE void *g3dbaseContinuousPaToVa(const G3dMemory *base, unsigned int pa)
{
	return (void *)(pa - base->hwAddr + (uintptr_t) base->data);
}

#endif /* _G3D_MEMORY_H_ */
