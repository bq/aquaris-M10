/*
* Copyright (C) 2017 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#ifndef __CMDQ_CCF_H__
#define __CMDQ_CCF_H__
#include "ddp_drv.h"
#if 0
#include <linux/platform_device.h>

typedef enum CMDQ_CLK_ENUM {
	CCF_CLK_ISP0_SMI_LARB0,
	CCF_CLK_DISP0_MDP_BLS_26M,
	CCF_CLK_DISP0_SMI_COMMON,
	CCF_CLK_DISP0_SMI_LARB0,
	CCF_CLK_DISP0_MM_CMDQ,
	CCF_CLK_DISP0_MUTEX,
	CCF_CLK_IMAGE_CAM_SMI,
	CCF_CLK_IMAGE_CAM_CAM,
	CCF_CLK_IMAGE_SEN_TG,
	CCF_CLK_IMAGE_SEN_CAM,
	CCF_CLK_IMAGE_LARB2_SMI,
	CCF_CLK_DISP0_CAM_MDP,
	CCF_CLK_DISP0_MDP_RDMA,
	CCF_CLK_DISP0_MDP_RSZ0,
	CCF_CLK_DISP0_MDP_RSZ1,
	CCF_CLK_DISP0_MDP_TDSHP,
	CCF_CLK_DISP0_MDP_WROT,
	CCF_CLK_DISP0_MDP_WDMA,
	CCF_CLK_DISP0_DISP_WDMA,
	CCF_CLK_DISP0_MUTEX_32K,
	CCF_CLK_DISP0_DISP_OVL,
	CCF_CLK_DISP0_DISP_COLOR,
	CCF_CLK_DISP0_DISP_BLS,
	CCF_CLK_DISP0_DISP_RDMA,
	CCF_CLK_MAX_NUM
} CMDQ_CLK_ENUM;


typedef enum CMDQ_SUBSYS_ENUM {
	CMDQ_SUBSYSCLK_SYS_VDE,
	CMDQ_SUBSYSCLK_SYS_ISP,
	CMDQ_SUBSYSCLK_SYS_DIS,
	CMDQ_SUBSYSCLK_NUM
} CMDQ_SUBSYS_ENUM;


char *cmdq_core_get_clk_name(CMDQ_CLK_ENUM clk_enum);

/* init ccf */
void cmdq_core_get_clk_map(struct platform_device *pDevice);

int cmdq_core_enable_ccf_clk(CMDQ_CLK_ENUM clk_enum);

int cmdq_core_disable_ccf_clk(CMDQ_CLK_ENUM clk_enum);

bool cmdq_core_clock_is_on(CMDQ_CLK_ENUM clk_enum);

bool cmdq_core_subsys_is_on(CMDQ_SUBSYS_ENUM clk_enum);
#else

bool cmdq_core_clock_is_on(struct clk *clk);
int cmdq_core_enable_ccf_clk(int clk_id, int index);
int cmdq_core_disable_ccf_clk(int clk_id, int index);

#endif

#endif
