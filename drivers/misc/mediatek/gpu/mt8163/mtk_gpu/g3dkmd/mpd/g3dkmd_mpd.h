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

#ifndef _G3DKMD_MPD_H_
#define _G3DKMD_MPD_H_


#ifdef SUPPORT_MPD
/* For cmodel pattern dump */

#if defined(_WIN32)
__declspec(dllimport)
void __stdcall CMRiuCommand(int command, int data);
__declspec(dllimport)
void __stdcall CMSetMemory(unsigned int PA, unsigned int G3DVA, unsigned int size, void *pdata);
__declspec(dllimport)
void __stdcall CMFreeMemory(unsigned int PA, unsigned int G3DVA, unsigned int size, void *pdata);

#elif defined(linux) && defined(linux_user_mode)
__attribute__ ((visibility("default")))
void CMRiuCommand(int command, int data);
__attribute__ ((visibility("default")))
void CMSetMemory(unsigned int PA, unsigned int G3DVA, unsigned int size, void *pdata);
__attribute__ ((visibility("default")))
void CMFreeMemory(unsigned int PA, unsigned int G3DVA, unsigned int size, void *pdata);

#elif defined(linux) && !defined(linux_user_mode)

#include "g3dbase_ioctl.h"

#ifndef G3DKMD_USE_BUFFERFILE
#error "buffer file required for mpd"
#endif

void CMMPDControl(struct _g3dkmd_IOCTL_MPD_Control *cmd);
void MPDReleaseUser(void);

void CMRiuCommand(int command, int data);
void CMSetMemory_Kernel(unsigned int PA, unsigned int G3DVA, unsigned int size, void *pdata);
void CMFreeMemory_Kernel(unsigned int PA, unsigned int G3DVA, unsigned int size, void *pdata);
#define CMSetMemory CMSetMemory_Kernel
#define CMFreeMemory CMFreeMemory_Kernel
#endif

#endif

#endif /* _G3DKMD_MPD_H_ */
