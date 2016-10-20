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

#ifndef _G3DKMD_API_MMU_H_
#define _G3DKMD_API_MMU_H_

#include "g3dkmd_api_common.h"

struct g3dExecuteInfo;

/* swrdback */
G3DKMD_APICALL uint64_t G3DAPIENTRY g3dKmdSwRdbackAlloc(int taskID, unsigned int *hwAddr);

/* MMU */
G3DKMD_APICALL void *G3DAPIENTRY g3dKmdMmuGetHandle(void);
G3DKMD_APICALL void *G3DAPIENTRY g3dKmdMmuInit(unsigned int va_base);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuUninit(void *m);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuFreeAll(void *m);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuGetTable(void *m, void **swAddr, void **hwAddr,
							  unsigned int *size);
G3DKMD_APICALL int G3DAPIENTRY g3dKmdMmuGetUsrBufUsage(unsigned int *pSize);
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdMmuG3dVaToPa(void *m, unsigned int g3d_va);

#ifdef YL_MMU_PATTERN_DUMP_V1
G3DKMD_APICALL void *G3DAPIENTRY g3dKmdMmuG3dVaToPaFull(void *m, unsigned int g3d_va,
								unsigned int size);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuPaPairBufInfo(void *m, unsigned int g3d_va,
							       unsigned int size,
							       unsigned int *buffer_size,
							       unsigned int *unit_size,
							       unsigned int *offset_mask);
#endif
#ifdef YL_MMU_PATTERN_DUMP_V2
G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuG3dVaToPaChunk(void *m, unsigned int data,
								unsigned int size,
								unsigned int *chunk_pa,
								unsigned int *chunk_size);
#endif

#ifdef CMODEL
/* added by Frank Fu, 2012/12/21 */
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdMmuG3dVaToVa(void *m, unsigned int g3d_va);
#endif

G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdMmuMap(void *m, void *va, unsigned int size,
							     int could_be_section,
							     unsigned int flag);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdMmuUnmap(void *m, unsigned int g3d_va,
						       unsigned int size);
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdMmuRangeSync(void *m, unsigned long va,
								   unsigned int size,
								   unsigned int g3d_va,
								   unsigned int cache_action);

/* Physical */
G3DKMD_APICALL uint64_t G3DAPIENTRY g3dKmdAllocPhysical(unsigned int size);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdFreePhysical(uint64_t addr, unsigned int size);

/* pattern */
#ifdef YL_SECURE
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdRemapPhysical(void *va, unsigned int size,
								    unsigned int secure_flag);
#else
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdRemapPhysical(void *va, unsigned int size);
#endif

/* Setup */
G3DKMD_APICALL void G3DAPIENTRY g3dKmdSetPhysicalStartAddr(unsigned int pa_base);

/* swrdback */
void g3dKmdAllocSwRdback(struct g3dExecuteInfo *execute, unsigned int size);
void g3dKmdFreeSwRdback(struct g3dExecuteInfo *execute);

/* Linux */
#ifdef linux
#if !defined(linux_user_mode)
G3DKMD_APICALL uint64_t G3DAPIENTRY g3dKmdPhyAlloc(unsigned int size);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdPhyFree(uint64_t phyAddr, unsigned int size);
G3DKMD_APICALL int G3DAPIENTRY g3dKmdUserPageGet(unsigned long uaddr, unsigned long size,
							 uintptr_t *ret_pages,
							 unsigned int *ret_nr_pages);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdUserPageFree(uintptr_t pageArray,
							   unsigned int nr_pages);
#endif
#endif

G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdAllocHw(uint64_t phyAddr, unsigned int size);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdFreeHw(unsigned int hwAddr, unsigned int size);

/* for G3DKMD only */
extern unsigned int g3dKmdAllocHw_ex(uint64_t phyAddr, unsigned int size);
extern void g3dKmdFreeHw_ex(unsigned int hwAddr, unsigned int size);

#ifdef YL_SECURE
extern unsigned int g3dKmdMmuMap_ex(void *m, void *va, unsigned int size,
					    int could_be_section, unsigned int flag,
					    unsigned int secure_flag);
#else
extern unsigned int g3dKmdMmuMap_ex(void *m, void *va, unsigned int size,
					    int could_be_section, unsigned int flag);
#endif

extern void g3dKmdMmuUnmap_ex(void *m, unsigned int g3d_va, unsigned int size);
extern unsigned int g3dKmdMmuG3dVaToPa_ex(void *m, unsigned int g3d_va);
extern unsigned int g3dKmdMmuG3dVaToVa_ex(void *m, unsigned int g3d_va);
extern void g3dKmdMmuGetTable_ex(void *m, void **swAddr, void **hwAddr, unsigned int *size);
extern void *g3dKmdMmuInit_ex(unsigned int va_base);
extern void g3dKmdMmuUninit_ex(void *m);

#endif/* _G3DKMD_API_MMU_H_ */
