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
#ifndef _G3D_MEMORY_H_
#define _G3D_MEMORY_H_
#include "yl_define.h"
#include "g3dbase_common_define.h"
#include "g3dbase_common.h"

typedef enum {
	G3D_RES_NONE = 0,
	G3D_RES_RENDER,
	G3D_RES_TEXTURE,
	G3D_RES_BUFFER,
	G3D_RES_CMDQ,
	G3D_RES_RINGBUFFER,
	G3D_RES_SHADER_INST,
	G3D_RES_OTHERS,
	G3D_RES_DMA,
	G3D_RES_PRIVATE_MEMORY,
	G3D_RES_DISPLAY,
	G3D_RES_VMALLOC,
	G3D_RES_MAX_NUM,
} G3d_Resource_Type;

typedef enum {
	G3D_MEM_NONE = 0,
	G3D_MEM_PHYSICAL = 0,
#if 1				/* def YL_QUICK_STAMP2 */
#ifdef ANDROID
	G3D_MEM_VIRTUAL = 0x01,
	G3D_MEM_GRALLOC = 0x10,	/* use bit 0x10 to indicate gralloc buffers */
	G3D_MEM_GRALLOC_FB = 0x11,
	G3D_MEM_GRALLOC_SEC = 0x12,
#else
	G3D_MEM_VIRTUAL = 0x01,
#endif
#else
#ifdef ANDROID
	G3D_MEM_VIRTUAL,
	G3D_MEM_GRALLOC,
	G3D_MEM_GRALLOC_FB,
	G3D_MEM_GRALLOC_SEC
#else
	G3D_MEM_VIRTUAL
#endif
#endif
} G3d_Memory_Type;

#if 1				/* def YL_QUICK_STAMP2 */
#define G3D_MEM_GRALLOC_MSK 0x10
#endif

typedef struct G3dStampSlots_ {
	/* void*               cmdqueue; */
	union __cmdqueue__ {
		uintptr_t ptr;

		uint64_t __enlargeSize __aligned(8);
	} cmdqueue;
	unsigned int stamp;
	unsigned int reserved;
} G3dStampSlots;


#ifdef YL_MMU_PATTERN_DUMP_V1
typedef struct _V_TO_P_ENTRY {
	unsigned int num;	/* how many (pa, size) pairs */
	unsigned int (*pair)[2];	/* array of (pa, size) pair */
} V_TO_P_ENTRY;
#endif

#define GSSI110_TAG(Prog) ((unsigned *)Prog.data)[(Prog.size>>2)-1]

typedef struct G3dMemory_ {
	unsigned int type:8;	/* G3d_Memory_Type */
	unsigned int resType:8;	/* G3d_Resource_Type */
	unsigned int remap:1;
	unsigned int deferAlloc:1;
	unsigned int instPack:1;
	unsigned int gssi110:1;
	unsigned int reserve:12;
	unsigned int gssi110_tag;

	unsigned int reserved2;
	int alignment;

	void *data0;		/* pointer returned from allocator */
	void *data;		/* aligned pointer */
	unsigned int hwAddr0;	/* HW base returned from mapper */
	unsigned int hwAddr;	/* aligned HW base (physical or G3D_VA if type = virtual) */
	int size0;
	int size;

	G3dStampSlots stamps[G3D_MAX_NUM_STAMPS];

#ifdef ANDROID
	int fd;			/* fd from share mem driver */
	int offset;		/* if partial memory of fd */
#endif

	uint64_t phyAddr;	/* if type = virtual, this field to store the physical address after mmap */

#ifdef YL_MMU_PATTERN_DUMP_V1
	void *pa_pair_info;
#endif

	unsigned int patPhyAddr;	/* to keep physical address for pattern dump */

	/* if type = virtual, this field to store the respondiong page number after get_user_page */
	unsigned int nrPages;

	/* if type = virtual, this field to store the respondiong page pointer array after get_user_page */
	void *pageArray;
#if defined(ENABLE_HEAP_MM)
	void *heap_ptr;
#endif
#if defined(ENABLE_G3DMEMORY_LINK_LIST)
	/* void*               memlist_nodep; */
#endif /* defined(ENABLE_G3DMEMORY_LINK_LIST) */

} G3dMemory;

/* return value of allocate memory */
#define G3D_MEM_FAIL        0
#define G3D_MEM_SUCCESS     1
#define G3D_MEM_DEFER_ALLOC 2


#if defined(ANDROID) && defined(YL_GRALLOC)
	#define IS_FROM_GRALLOC(pMem) (((pMem)->type & 0xFF) == G3D_MEM_GRALLOC)
	#define IS_FROM_GRALLOC_FB(pMem) (((pMem)->type & 0xFF) == G3D_MEM_GRALLOC_FB)
#else
	#define IS_FROM_GRALLOC(pMem)    0
	#define IS_FROM_GRALLOC_FB(pMem) 0
#endif

#endif /* _G3D_MEMORY_H_ */
