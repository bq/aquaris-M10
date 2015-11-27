#ifndef _G3DKMD_GCE_H_
#define _G3DKMD_GCE_H_

#include "g3dkmd_task.h"

void g3dKmdGceInit(void);
void g3dKmdGceUninit(void);
void g3dKmdGceResetHW(void);

unsigned char g3dKmdGceSendCommand(pG3dExeInst pInst);
void g3dKmdGceDoContextSwitch(pG3dExeInst pInstFrom, pG3dExeInst pInstTo, int freeHwCh);
void g3dKmdGcePrepareQld(pG3dExeInst pInst);
//void g3dKmdGceQueueEmpty(unsigned int hwCh);

unsigned char g3dKmdGceCreateMmcq(pG3dExeInst pInst);
void g3dKmdGceReleaseMmcq(pG3dExeInst pInst);
void g3dKmdGceWaitIdle(void);

#endif  // _G3DKMD_GCE_H_
