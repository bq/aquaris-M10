/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __DDP_DEBUG_H__
#define __DDP_DEBUG_H__

#include <linux/kernel.h>
#include <mmprofile.h>
#include "ddp_drv.h"
#include "disp_drv_platform.h"

#ifndef STR_CONVERT
#define STR_CONVERT(p, val, base, action)\
	do {			\
		int ret = 0;	\
		const char *tmp;	\
		tmp = strsep(p, ","); \
		if (tmp == NULL) \
			break; \
		if (strcmp(#base, "int") == 0)\
			ret = kstrtoint(tmp, 0, (int *)val); \
		else if (strcmp(#base, "uint") == 0)\
			ret = kstrtouint(tmp, 0, (unsigned int *)val); \
		else if (strcmp(#base, "ul") == 0)\
			ret = kstrtoul(tmp, 0, (unsigned long *)val); \
		if (0 != ret) {\
			DISP_ERR("kstrtoint/kstrtouint/kstrtoul return error: %d\n" \
				"  file : %s, line : %d\n",		\
				ret, __FILE__, __LINE__);\
			action; \
		} \
	} while (0)
#endif

extern struct DDP_MMP_Events_t {
	MMP_Event DDP;
	MMP_Event MutexParent;
	MMP_Event Mutex[6];
	MMP_Event BackupReg;
	MMP_Event DDP_IRQ;
	MMP_Event SCL_IRQ;
	MMP_Event ROT_IRQ;
	MMP_Event OVL_IRQ;
	MMP_Event WDMA0_IRQ;
	MMP_Event WDMA1_IRQ;
	MMP_Event RDMA0_IRQ;
	MMP_Event RDMA1_IRQ;
	MMP_Event COLOR_IRQ;
	MMP_Event BLS_IRQ;
	MMP_Event TDSHP_IRQ;
	MMP_Event CMDQ_IRQ;
	MMP_Event Mutex_IRQ;
	MMP_Event WAIT_INTR;
	MMP_Event Debug;
} DDP_MMP_Events;

extern unsigned int dbg_log;
extern unsigned int irq_log;
extern unsigned int irq_err_log;
extern unsigned char aal_debug_flag;
extern unsigned char pq_debug_flag;
extern unsigned int gNeedToRecover;
extern unsigned int isAEEEnabled;
extern unsigned int disp_log_level;
extern unsigned char *disp_module_name[];
extern unsigned int gUltraLevel;
extern unsigned int gEnableUltra;

void ddp_debug_init(void);
void ddp_debug_exit(void);
int ddp_mem_test(void);
int ddp_mem_test2(void);
void ddp_enable_bls(int BLS_switch);
int ddp_dump_info(DISP_MODULE_ENUM module);
extern void mtkfb_dump_layer_info(void);

#endif				/* __DDP_DEBUG_H__ */
