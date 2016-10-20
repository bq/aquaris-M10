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

#ifndef _G3DKMD_MEMORY_LIST_H_
#define _G3DKMD_MEMORY_LIST_H_

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3d_memory.h"		/* for G3dmemory */

#if defined(ENABLE_G3DMEMORY_LINK_LIST)

enum G3d_Mem_HEAP_Type {
	G3D_Mem_Kernel = 0,
	G3D_Mem_User,
	G3D_Mem_Heap_MAX,
};


/* _g3dKmdMemList_HeadInit()
 *
 * - initialize G3DMemory Link List head.
 */
void _g3dKmdMemList_Init(void);


/* _g3dKmdMemList_Terminate()
 *
 * - cleanup everything.
 */
void _g3dKmdMemList_Terminate(void);


/* _g3dKmdMemList_CleanProcMemlist()
 *
 * - Cleanup the memlist with the target pid.
 *   @pid: current pid.
 */
/* void _g3dKmdMemList_CleanProcMemlist(G3Dint pid); */


/* _g3dKmdMemList_CleanDeviceMemlist()
 *
 * - Cleanup the memlist with the target device file pointer.
 *   @fd: device file pointer
 */
void _g3dKmdMemList_CleanDeviceMemlist(void *fd);


/* _g3dKmdMemList_Add()
 *
 * - insert a G3dMemory to global list
 *   @fd:       device file pointer, NULL if from kernel.
 *   @pid:      process ID
 *   @va:       virtual address
 *   @size:     size of virtual address
 *   @hwAddr:   hardware Address of va
 *   @type:     memory alloc from kernel or userland.
 */
/* void  _g3dKmdMemList_Add(G3dMemory *mem, enum G3d_Mem_HEAP_Type type); */
void _g3dKmdMemList_Add(void *fd, G3Dint pid, void *va, G3Duint size,
			G3Duint hwAddr, enum G3d_Mem_HEAP_Type type);


/* _g3dKmdMemList_Del()
 *
 * - remove a G3dMemory to global list
 *   @va:       virtual address.
 */
/* void _g3dKmdMemList_Del(G3dMemory *mem); */
void _g3dKmdMemList_Del(void *va);


/* _g3dKmdMemList_GetTotalUsage()
 *
 * - return a total usage memory
 */
G3Duint _g3dKmdMemList_GetTotalMemoryUsage(void);


/* _g3dKmdMemList_ListAll()
 *
 * - list all memory to seq
 */
void _g3dKmdMemList_ListAll(void *seq);


#else

#define _g3dKmdMemList_Init(...)
#define _g3dKmdMemList_Terminate(...)
#define _g3dKmdMemList_CleanCurrentProcMemlist(...)
#define _g3dKmdMemList_Add(...)
#define _g3dKmdMemList_Del(...)
#define _g3dKmdMemList_GetTotalMemoryUsage() 0
#define _g3dKmdMemList_CleanDeviceMemlist(...)
#define _g3dKmdMemList_ListAll(...)

#endif

#endif /* _G3DKMD_MEMORY_LIST_H_ */
