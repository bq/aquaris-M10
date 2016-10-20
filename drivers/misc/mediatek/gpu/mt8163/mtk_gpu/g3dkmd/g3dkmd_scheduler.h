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

#ifndef _G3DKMD_SCHEDULER_H_
#define _G3DKMD_SCHEDULER_H_

#define G3DKMD_SCHEDULER_FREQ      10	/* ms */
#define G3DKMD_TIME_SLICE_BASE     10	/* ms */
#define G3DKMD_TIME_SLICE_INC      1	/* ms */
#define G3DKMD_TIME_SLICE_NEW_THD  100	/* ms */

void g3dKmdCreateScheduler(void);
void g3dKmdReleaseScheduler(void);
void g3dKmdTriggerScheduler(void);
void g3dKmdContextSwitch(unsigned debug_info_enable);

#ifdef KERNEL_FENCE_DEFER_WAIT
void g3dKmdUpdateReadyPtr(void);
#endif

#endif /* _G3DKMD_SCHEDULER_H_ */
