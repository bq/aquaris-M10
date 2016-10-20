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

#ifndef _YL_MMU_UTILITY_H_
#define _YL_MMU_UTILITY_H_

#include "yl_mmu.h"
#include "dump/yl_mmu_utility_sanity_trunk.h"

int mmuMapperValid(struct Mapper *m);
struct Mapper *mmuInit(g3d_va_t g3d_va_base);
void mmuUninit(struct Mapper *m);
void mmuGetTable(struct Mapper *m, void **swAddr, void **hwAddr, unsigned int *size);
void mmuGetMirrorTable(struct Mapper *m, void **swAddr, void **hwAddr, unsigned int *size);
unsigned int mmuG3dVaToG3dVa(struct Mapper *m, g3d_va_t g3d_va);
unsigned int mmuG3dVaToPa(struct Mapper *m, g3d_va_t g3d_va);
void *mmuGetMapper(void);

#endif /* _YL_MMU_UTILITY_H_ */
