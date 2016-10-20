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

#ifndef _G3DKMD_FAKEMEM_H_

#define _G3DKMD_FAKEMEM_H_

#if defined(G3DKMD_FAKE_HW_ADDR) || defined(G3DKMD_PHY_HEAP)
extern unsigned int g_pa_base;
#endif

#ifdef G3DKMD_PHY_HEAP
extern unsigned int g_max_pa_addr;
#endif

extern unsigned int mmAllocByVa(unsigned int uSize, void *va);

extern unsigned int mmAlloc(unsigned int uSize);

extern void mmFree(unsigned int uAddr, unsigned int uSize);


#endif
