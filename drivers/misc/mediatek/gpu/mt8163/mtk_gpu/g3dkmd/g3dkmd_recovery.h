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

#ifndef _G3DKMD_RECOVERY_H_
#define _G3DKMD_RECOVERY_H_

#include "g3dkmd_define.h"
#include "g3dkmd_macro.h"
#include "g3dkmd_engine.h"

#ifdef G3DKMD_RECOVERY_BY_TIMER
void g3dKmd_recovery_reset_counter(void);
void g3dKmd_recovery_timer_enable(void);
void g3dKmd_recovery_timer_disable(void);
void g3dKmd_recovery_timer_init(void);
void g3dKmd_recovery_detect_hang(struct g3dExeInst *pInst);
void g3dKmd_recovery_reset_hw(struct g3dExeInst *pInst);
void g3dKmd_recovery_timer_uninit(void);
#endif

#ifdef G3D_SWRDBACK_RECOVERY
void g3dKmdRecoveryAddInfo(struct g3dExeInst *pInst, unsigned int ptr, unsigned int stamp);
void g3dKmdRecoveryUpdateSWReadBack(struct g3dExeInst *pInst, unsigned int rptr);
#endif

#endif /* _G3DKMD_RECOVERY_H_ */
