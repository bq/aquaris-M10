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

#include <linux/clk.h>
#include <linux/clk-provider.h>

#include "cmdq_ccf.h"
#include "ddp_cmdq.h"

#if 0
struct clk *cmdq_clk_map[CCF_CLK_MAX_NUM];
static struct clk *sys_vde;
static struct clk *sys_isp;
static struct clk *sys_dis;

char *cmdq_core_get_clk_name(CMDQ_CLK_ENUM clk_enum)
{
	switch (clk_enum) {
	case CCF_CLK_ISP0_SMI_LARB0:
		return "CCF_CLK_ISP0_SMI_LARB0";
	case CCF_CLK_DISP0_MDP_BLS_26M:
		return "CCF_CLK_DISP0_MDP_BLS_26M";
	case CCF_CLK_DISP0_SMI_COMMON:
		return "CCF_CLK_DISP0_SMI_COMMON";
	case CCF_CLK_DISP0_SMI_LARB0:
		return "CCF_CLK_DISP0_SMI_LARB0";
	case CCF_CLK_DISP0_MM_CMDQ:
		return "CCF_CLK_DISP0_MM_CMDQ";
	case CCF_CLK_DISP0_MUTEX:
		return "CCF_CLK_DISP0_MUTEX";
	case CCF_CLK_IMAGE_CAM_SMI:
		return "CCF_CLK_IMAGE_CAM_SMI";
	case CCF_CLK_IMAGE_CAM_CAM:
		return "CCF_CLK_IMAGE_CAM_CAM";
	case CCF_CLK_IMAGE_SEN_TG:
		return "CCF_CLK_IMAGE_SEN_TG";
	case CCF_CLK_IMAGE_SEN_CAM:
		return "CCF_CLK_IMAGE_SEN_CAM";
	case CCF_CLK_IMAGE_LARB2_SMI:
		return "CCF_CLK_IMAGE_LARB2_SMI";
	case CCF_CLK_DISP0_CAM_MDP:
		return "CCF_CLK_DISP0_CAM_MDP";
	case CCF_CLK_DISP0_MDP_RDMA:
		return "CCF_CLK_DISP0_MDP_RDMA";
	case CCF_CLK_DISP0_MDP_RSZ0:
		return "CCF_CLK_DISP0_MDP_RSZ0";
	case CCF_CLK_DISP0_MDP_RSZ1:
		return "CCF_CLK_DISP0_MDP_RSZ1";
	case CCF_CLK_DISP0_MDP_TDSHP:
		return "CCF_CLK_DISP0_MDP_TDSHP";
	case CCF_CLK_DISP0_MDP_WROT:
		return "CCF_CLK_DISP0_MDP_WROT";
	case CCF_CLK_DISP0_MDP_WDMA:
		return "CCF_CLK_DISP0_MDP_WDMA";
	case CCF_CLK_DISP0_DISP_WDMA:
		return "CCF_CLK_DISP0_DISP_WDMA";
	case CCF_CLK_DISP0_MUTEX_32K:
		return "CCF_CLK_DISP0_MUTEX_32K";
	case CCF_CLK_DISP0_DISP_OVL:
		return "CCF_CLK_DISP0_DISP_OVL";
	case CCF_CLK_DISP0_DISP_COLOR:
		return "CCF_CLK_DISP0_DISP_COLOR";
	case CCF_CLK_DISP0_DISP_BLS:
		return "CCF_CLK_DISP0_DISP_BLS";
	case CCF_CLK_DISP0_DISP_RDMA:
		return "CCF_CLK_DISP0_DISP_RDMA";
	default:
		CMDQ_ERR("invalid clk id=%d\n", clk_enum);
		return "UNKWON";
	}
}

/* init ccf */
void cmdq_core_get_clk_map(struct platform_device *pDevice)
{
	int i;

	CMDQ_ERR("harry>>> ccf\n");
/*	for (i = 0; i < CCF_CLK_MAX_NUM; i++) { */
	for (i = 0; i < 1; i++) {
		cmdq_clk_map[i] = devm_clk_get(&pDevice->dev, cmdq_core_get_clk_name(i));
		if (IS_ERR(cmdq_clk_map[i])) {
			CMDQ_ERR("Get Clock:%s(ID=%d) Fail!\n", cmdq_core_get_clk_name(i), i);
			BUG_ON(1);
		}
	}

	sys_vde = devm_clk_get(&pDevice->dev, "sys_vde");
	if (IS_ERR(sys_vde)) {
		CMDQ_ERR("Get Clock:sys_vde Fail!\n");
		BUG_ON(1);
	}
	sys_isp = devm_clk_get(&pDevice->dev, "sys_isp");
	if (IS_ERR(sys_isp)) {
		CMDQ_ERR("Get Clock:sys_isp Fail!\n");
		BUG_ON(1);
	}
	sys_dis = devm_clk_get(&pDevice->dev, "sys_dis");
	if (IS_ERR(sys_dis)) {
		CMDQ_ERR("Get Clock:sys_dis Fail!\n");
		BUG_ON(1);
	}
}


int cmdq_core_enable_ccf_clk(CMDQ_CLK_ENUM clk_enum)
{
	int ret = 0;

	if (IS_ERR(cmdq_clk_map[clk_enum])) {
		CMDQ_ERR("Enable CCF Clock:%s(ID=%d) Fail!\n", cmdq_core_get_clk_name(clk_enum),
			 clk_enum);
		ret = -1;
		BUG_ON(1);
	} else {
		ret += clk_prepare(cmdq_clk_map[clk_enum]);
		ret += clk_enable(cmdq_clk_map[clk_enum]);
		if (ret != 0)
			CMDQ_ERR("Prepare/Enable CCF Clock:%s(ID=%d) Fail!\n",
				 cmdq_core_get_clk_name(clk_enum), clk_enum);
	}

	return ret;
}

int cmdq_core_disable_ccf_clk(CMDQ_CLK_ENUM clk_enum)
{
	int ret = 0;

	if (IS_ERR(cmdq_clk_map[clk_enum])) {
		CMDQ_ERR("Disable CCF Clock:%s(ID=%d) Fail!\n", cmdq_core_get_clk_name(clk_enum),
			 clk_enum);
		ret = -1;
		BUG_ON(1);
	} else {
		clk_disable(cmdq_clk_map[clk_enum]);
		clk_unprepare(cmdq_clk_map[clk_enum]);
	}

	return ret;
}

bool cmdq_core_clock_is_on(CMDQ_CLK_ENUM clk_enum)
{
#if 1
	if (__clk_get_enable_count(cmdq_clk_map[clk_enum]) > 0)
		return 1;
	else
		return 0;
#else
	return 0;
#endif
}

bool cmdq_core_subsys_is_on(CMDQ_SUBSYS_ENUM clk_enum)
{
	struct clk *subsys_clk = NULL;

	switch (clk_enum) {
	case CMDQ_SUBSYSCLK_SYS_VDE:
		subsys_clk = sys_vde;
		break;
	case CMDQ_SUBSYSCLK_SYS_ISP:
		subsys_clk = sys_isp;
		break;
	case CMDQ_SUBSYSCLK_SYS_DIS:
		subsys_clk = sys_dis;
		break;
	default:
		CMDQ_ERR("invalid subsys clk id=%d\n", clk_enum);
	}

	if (subsys_clk == NULL || IS_ERR(subsys_clk)) {
		CMDQ_ERR("subsys_clk uninitialized, id=%d\n", clk_enum);
		return 0;
	}

	if (__clk_get_enable_count(subsys_clk) > 0)
		return 1;
	else
		return 0;
}
#else

/*
**	enalbe clock use
**		static inline int clk_prepare_enable(struct clk *clk) instead
**	disable clock use
**		static inline void clk_disable_unprepare(struct clk *clk)
*/
bool cmdq_core_clock_is_on(struct clk *clk)
{
	if (__clk_get_enable_count(clk) > 0)
		return 1;
	else
		return 0;
}

int cmdq_core_enable_ccf_clk(int clk_id, int index)
{
	struct clk *clock = disp_dev.clk_map[clk_id][index];

	if ((NULL == clock) || clk_prepare_enable(clock)) {
		CMDQ_ERR("enable clk[%p] id[%d], index[%d] failed\n", clock, clk_id, index);
		return -1;
	}
	return 0;
}

int cmdq_core_disable_ccf_clk(int clk_id, int index)
{
	struct clk *clock = disp_dev.clk_map[clk_id][index];

	if (NULL == clock) {
		CMDQ_ERR("disable clk[%p] id[%d], index[%d] failed\n", clock, clk_id, index);
		return -1;
	}

	clk_disable_unprepare(clock);
	return 0;
}

#endif
