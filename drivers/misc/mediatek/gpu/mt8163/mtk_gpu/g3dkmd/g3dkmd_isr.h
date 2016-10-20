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

#ifndef _G3DKMD_ISR_H_
#define _G3DKMD_ISR_H_

#include "g3dkmd_define.h"
#include "g3dkmd_util.h"

#ifdef FPGA_G3D_HW
extern unsigned long __yl_irq;
#define G3DHW_IRQ_ID    __yl_irq
#else
#define G3DHW_IRQ_ID    0
#endif
#define G3DHW_IRQ_NAME  "yolig3d"

void lock_isr(void);
void unlock_isr(void);

void lock_hang_detect(void);
void unlock_hang_detect(void);

G3DKMD_BOOL g3dKmdIsrInstall(void);
void g3dKmdIsrUninstall(void);

/* suspendIsr (tasklet and softirq */
void g3dKmdSuspendIsr(void);
void g3dKmdResumeIsr(void);

void g3dKmdInvokeIsrTop(void);

#ifndef G3D_HW
void g3dKmdIsrTrigger(void);
#endif

#endif /* _G3DKMD_ISR_H_ */
