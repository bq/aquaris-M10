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

#ifndef _G3DKMD_GCE_H_
#define _G3DKMD_GCE_H_

#include "g3dkmd_task.h"

void g3dKmdGceInit(void);
void g3dKmdGceUninit(void);
void g3dKmdGceResetHW(void);

unsigned char g3dKmdGceSendCommand(struct g3dExeInst *pInst);
void g3dKmdGceDoContextSwitch(struct g3dExeInst *pInstFrom, struct g3dExeInst *pInstTo, int freeHwCh);
void g3dKmdGcePrepareQld(struct g3dExeInst *pInst);
/* void g3dKmdGceQueueEmpty(unsigned int hwCh); */

unsigned char g3dKmdGceCreateMmcq(struct g3dExeInst *pInst);
void g3dKmdGceReleaseMmcq(struct g3dExeInst *pInst);
void g3dKmdGceWaitIdle(void);

#endif /* _G3DKMD_GCE_H_ */
