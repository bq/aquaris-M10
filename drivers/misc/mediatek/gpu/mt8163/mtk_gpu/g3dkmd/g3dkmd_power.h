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

#ifndef _G3DKMD_POWER_H_
#define _G3DKMD_POWER_H_

#ifdef WIN32

#define gpu_may_power_low()
#define gpu_set_power_normal()
#define gpu_increment_flush_count()
#define gpu_decrement_flush_count()

#else
#include <linux/types.h>

/******************************************************************************
* Type Definition
******************************************************************************/
enum G3DKMD_PWRSTATE {
	G3DKMD_PWRSTATE_POWEROFF,
	G3DKMD_PWRSTATE_POWERON,
};

/******************************************************************************
* Function prototypes
******************************************************************************/

int gpu_power_state_freeze(bool fgForcePowerOn);
void gpu_power_state_unfreeze(bool fgForcePowerOn);


/******************************************************************************
* Early suspend / resume APIs
******************************************************************************/
void gpu_runtime_pm_enable(void);
void gpu_runtime_pm_disable(void);
void gpu_register_early_suspend(void);
void gpu_unregister_early_suspend(void);
void gpu_suspend(void);
void gpu_resume(void);

/******************************************************************************
* RuntimePM APIs
******************************************************************************/
int gpu_pending_work_done(void);
void gpu_set_power_on(void);
void gpu_may_power_off(void);

/* Func: gpu_power_state_freeze(bool fgForcePowerOn)
 * Param:
 * fgForcePoweron : false: only get the state
 *		    true : request gpu power on if it's power off.
 *			   only return the state if it's power on.
 * Return : 0: unlocked and power off.
 *		if it is power off, m4u_invalid_tlb will return directly,
 *		it don't call unlock. so please keep it unlocked.
 *          1: locked and power on.
 *		if it is power on, "unlocked" is needed, please keep it locked.
 *	    <0: unlocked and failed while force power on.
 * Return is 0 or 1 while fgForcePoweron is false.
 */
int gpu_power_state_freeze(bool fgForcePowerOn);
void gpu_power_state_unfreeze(bool fgForcePowerOn);

void gpu_increment_flush_count(void);
void gpu_decrement_flush_count(void);

#endif /* WIN32 */
#endif /* _G3DKMD_POWER_H_ */
