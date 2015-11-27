#ifndef _G3DKMD_RECOVERY_H_
#define _G3DKMD_RECOVERY_H_

#include "g3dkmd_define.h" 
#include "g3dkmd_macro.h"
#include "g3dkmd_engine.h"

#ifdef G3DKMD_RECOVERY_BY_TIMER
void g3dKmd_recovery_timer_init(void);
void g3dKmd_recovery_detect_hang(pG3dExeInst pInst);
void g3dKmd_recovery_reset_hw(pG3dExeInst pInst);
void g3dKmd_recovery_timer_uninit(void);
#endif

#ifdef G3D_SWRDBACK_RECOVERY
void g3dKmdRecoveryAddInfo(pG3dExeInst pInst, unsigned int ptr, unsigned int stamp);
void g3dKmdRecoveryUpdateSWReadBack(pG3dExeInst pInst, unsigned int rptr);
#endif

#endif //_G3DKMD_RECOVERY_H_

