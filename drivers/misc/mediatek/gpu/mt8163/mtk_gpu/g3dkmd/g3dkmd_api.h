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

#ifndef _G3DKMD_API_H_
#define _G3DKMD_API_H_

#if defined(WIN32) || (defined(linux) && defined(linux_user_mode))
#include <stdint.h>
#else
#include <linux/types.h>
#endif

#ifdef G3DKMD_CALLBACK_INT
#if defined(linux_user_mode)
#include <unistd.h>
#else
#include <linux/sched.h>
#endif
#endif

#include "g3dkmd_api_common.h"
#include "g3dkmd_api_mmu.h"


#ifdef PATTERN_DUMP
extern void *gfAllocInfoKernal;
#endif

/* Lifetime */
G3DKMD_APICALL int G3DAPIENTRY g3dKmdInitialize(void);
G3DKMD_APICALL int G3DAPIENTRY g3dKmdTerminate(void);

G3DKMD_APICALL int G3DAPIENTRY g3dKmdDeviceOpen(void *filp);
G3DKMD_APICALL int G3DAPIENTRY g3dKmdDeviceRelease(void *filp);

/* Control */
G3DKMD_APICALL G3DKMDRETVAL G3DAPIENTRY g3dKmdFlushCommand(int taskID, void *head, void *tail, int fence,
								   unsigned int lastFlushStamp,
								   unsigned int carry,
#ifdef G3D_SWRDBACK_RECOVERY
								   unsigned int lastFlushPtr,
#endif
								   unsigned int flushDCache);


/* G3DKMD_APICALL G3DKMDRETVAL G3DAPIENTRY g3dKmdWaitIdStamp(int taskID, int objIndex, unsigned int objstamp); */
G3DKMD_APICALL G3DKMDRETVAL G3DAPIENTRY g3dKmdLTCFlush(void);

struct ionGrallocLockInfo;

G3DKMD_APICALL int G3DAPIENTRY g3dKmdRegisterContext(G3DKMD_DEV_REG_PARAM *param);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdReleaseContext(int taskID, int abnormal);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdLockCheckDone(unsigned int num_task, struct ionGrallocLockInfo *data);
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdLockCheckFlush(unsigned int num_task, struct ionGrallocLockInfo *data);
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdCheckHWStatus(int taskID, int hw_reset);

/* FOR PATTERN */
G3DKMD_APICALL void G3DAPIENTRY g3dKmdDumpCommandBegin(int taskID, unsigned int dumpCount);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdDumpCommandEnd(int taskID, unsigned int dumpCount);

/*  FOR Bufferfile */
G3DKMD_APICALL void G3DAPIENTRY g3dKmdBufferFileDumpBegin(void *filp, unsigned int *root, unsigned int *len);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdBufferFileDumpEnd(void *filp, unsigned int isFree);
/*
G3DKMD_APICALL void G3DAPIENTRY g3dKmdBufferFileReleasePidEverything(void);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdBufferFileReleaseEverything(void);
*/

/* FOR flushtrigger */
G3DKMD_APICALL void G3DAPIENTRY g3dKmdFlushTrigger(unsigned int set, int taskID, unsigned int *value);

#ifdef G3DKMD_CALLBACK_INT
pid_t g3dKmdGetTaskProcess(int taskID);
#endif

G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdGetDrvInfo(unsigned int cmd, unsigned int arg,
								 void *buf, unsigned int bufSize, void *fd);
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdSetDrvInfo(unsigned int cmd, unsigned int arg,
								 void *buf, unsigned int bufSize, void *fd);

#if defined(WIN32) || (defined(linux) && defined(linux_user_mode))
G3DKMD_APICALL void G3DAPIENTRY g3dKmdConfig(G3DKMD_CFG *cfg);
#endif

struct _G3DKMD_PATTERN_CFG_;
G3DKMD_APICALL void G3DAPIENTRY g3dKmdPatternConfig(struct _G3DKMD_PATTERN_CFG_ *cfg);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdUxRegDebug(unsigned int val1, unsigned int val2);

G3DKMD_APICALL void G3DAPIENTRY g3dKmdPerfModeEnable(void);
G3DKMD_APICALL void G3DAPIENTRY g3dKmdPerfModeDisable(void);

extern void g3dKmdRegRestart(int taskID, int mode);
extern void lock_g3d(void);
extern void unlock_g3d(void);
extern void lock_g3dPwrState(void);
extern void unlock_g3dPwrState(void);
extern void lock_g3dFdq(void);
extern void unlock_g3dFdq(void);


#ifdef G3DKMD_SNAPSHOT_PATTERN

enum {
	G3DKMD_DRV_SSPCMD_INVALID = 0,
	G3DKMD_DRV_SSPCMD_DUMP = 1,
	G3DKMD_DRV_SSPCMD_DUMPBYPID = 2,
	G3DKMD_DRV_SSPCMD_ENABLE = 3,
	G3DKMD_DRV_SSPCMD_PATH = 4,
	G3DKMD_DRV_SSPCMD_DUMPBYTASKID = 5,
};

G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdSSPControl(int bIsQuery, int cmd, void *pBuf, unsigned int bufSize);
#endif


#endif /* _G3DKMD_API_H_ */
