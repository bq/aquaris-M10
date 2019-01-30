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

#include "cmdq_platform.h"
#include "cmdq_reg.h"
#include "mt_smi.h"

#include <linux/seq_file.h>

void cmdq_core_get_reg_id_from_hwflag(uint64_t hwflag, enum CMDQ_DATA_REGISTER_ENUM *valueRegId,
				      enum CMDQ_DATA_REGISTER_ENUM *destRegId,
				      enum CMDQ_EVENT_ENUM *regAccessToken)
{
	*regAccessToken = CMDQ_SYNC_TOKEN_INVALID;

	if (hwflag & (1LL << CMDQ_ENG_JPEG_ENC)) {
		*valueRegId = CMDQ_DATA_REG_JPEG;
		*destRegId = CMDQ_DATA_REG_JPEG_DST;
		*regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_0;
	} else if (hwflag & (1LL << CMDQ_ENG_MDP_TDSHP0)) {
		*valueRegId = CMDQ_DATA_REG_2D_SHARPNESS_0;
		*destRegId = CMDQ_DATA_REG_2D_SHARPNESS_0_DST;
		*regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_1;
	} else if (hwflag & ((1LL << CMDQ_ENG_DISP_COLOR0 | (1LL << CMDQ_ENG_DISP_COLOR1)))) {
		*valueRegId = CMDQ_DATA_REG_PQ_COLOR;
		*destRegId = CMDQ_DATA_REG_PQ_COLOR_DST;
		*regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_3;
	} else {
		/* assume others are debug cases */
		*valueRegId = CMDQ_DATA_REG_DEBUG;
		*destRegId = CMDQ_DATA_REG_DEBUG_DST;
		*regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_4;
	}
}


const char *cmdq_core_module_from_event_id(enum CMDQ_EVENT_ENUM event, uint32_t instA,
					   uint32_t instB)
{
	const char *module = "CMDQ";

	switch (event) {
	case CMDQ_EVENT_DISP_RDMA0_SOF:
	case CMDQ_EVENT_DISP_RDMA1_SOF:
	/* case CMDQ_EVENT_DISP_RDMA2_SOF: */
	case CMDQ_EVENT_DISP_RDMA0_EOF:
	case CMDQ_EVENT_DISP_RDMA1_EOF:
	/* case CMDQ_EVENT_DISP_RDMA2_EOF: */
	case CMDQ_EVENT_DISP_RDMA0_UNDERRUN:
	case CMDQ_EVENT_DISP_RDMA1_UNDERRUN:
	/* case CMDQ_EVENT_DISP_RDMA2_UNDERRUN: */
		module = "DISP_RDMA";
		break;

	case CMDQ_EVENT_DISP_WDMA0_SOF:
	case CMDQ_EVENT_DISP_WDMA1_SOF:
	case CMDQ_EVENT_DISP_WDMA0_EOF:
	case CMDQ_EVENT_DISP_WDMA1_EOF:
		module = "DISP_WDMA";
		break;

	case CMDQ_EVENT_DISP_OVL0_SOF:
	case CMDQ_EVENT_DISP_OVL1_SOF:
	case CMDQ_EVENT_DISP_OVL0_EOF:
	case CMDQ_EVENT_DISP_OVL1_EOF:
		module = "DISP_OVL";
		break;

	/* case CMDQ_EVENT_MDP_DSI0_TE_SOF: */
	/*case CMDQ_EVENT_MDP_DSI1_TE_SOF: */
	case CMDQ_EVENT_DISP_COLOR_SOF ... CMDQ_EVENT_DISP_PWM0_SOF:
	case CMDQ_EVENT_DISP_COLOR_EOF ... CMDQ_EVENT_DISP_DPI0_EOF:
	case CMDQ_EVENT_MUTEX0_STREAM_EOF ... CMDQ_EVENT_MUTEX4_STREAM_EOF:
		module = "DISP";
		break;
	case CMDQ_SYNC_TOKEN_CONFIG_DIRTY:
	case CMDQ_SYNC_TOKEN_STREAM_EOF:
		module = "DISP";
		break;

	case CMDQ_EVENT_MDP_RDMA0_SOF ... CMDQ_EVENT_MDP_WROT_SOF:
	case CMDQ_EVENT_MDP_RDMA0_EOF ... CMDQ_EVENT_MDP_WROT_READ_EOF:
	case CMDQ_EVENT_MUTEX5_STREAM_EOF ... CMDQ_EVENT_MUTEX9_STREAM_EOF:
		module = "MDP";
		break;

	case CMDQ_EVENT_ISP_2_EOF:
	case CMDQ_EVENT_ISP_1_EOF:
	case CMDQ_EVENT_ISP_SENINF1_FULL:
		module = "ISP";
		break;

	case CMDQ_EVENT_JPEG_ENC_EOF:
	case CMDQ_EVENT_JPEG_DEC_EOF:
		module = "JPGENC";
		break;

	default:
		module = "CMDQ";
		break;
	}

	return module;
}

const char *cmdq_core_parse_module_from_reg_addr(uint32_t reg_addr)
{
	const uint32_t addr_base_and_page = (reg_addr & 0xFFFFF000);
	const uint32_t addr_base_shifted = (reg_addr & 0xFFFF0000) >> 16;
	const char *module = "CMDQ";

	/* for well-known base, we check them with 12-bit mask */
	/* defined in mt_reg_base.h */
	switch (addr_base_and_page) {
	case 0x14000000: return "MMSYS_CONFIG";	/* DISPSYS_BASE */
	case 0x14001000: return "MDP_RDMA";	/* MDP_RDMA0_BASE */
	case 0x14002000: return "MDP_RSZ0";	/* MDP_RSZ1_BASE */
	case 0x14003000: return "MDP_RSZ1";	/* MDP_WDMA_BASE */
	case 0x14004000: return "MDP_WDMA";	/* MDP_TDSHP_BASE */
	case 0x14005000: return "MDP_WROT";	/* DISP_RDMA_BASE */
	case 0x14006000: return "MDP_TDSHP";	/* DISP_BLS_BASE */
	case 0x14007000: return "DISP_OVL0";	/* DSI_BASE */
	case 0x14008000: return "DISP_OVL1";	/* DSI_BASE */
	case 0x14009000: return "DISP_RDMA0";	/* DSI_BASE */
	case 0x1400A000: return "DISP_RDMA1";	/* DSI_BASE */
	case 0x1400B000: return "DISP_WDMA0";	/* DSI_BASE */
	case 0x1400C000: return "DISP_COLOR";	/* DSI_BASE */
	case 0x1400D000: return "DISP_CCORR";	/* DPI_BASE */
	case 0x1400E000: return "DISP_AAL";	/* MMSYS_MUTEX_base */
	case 0x1400F000: return "DISP_GAMMA";	/* MMSYS_MUTEX_base */

	case 0x14010000: return "DISP_DITHER"; /* MMSYS_MUTEX_base */
	case 0x14011000: return "DISP_UFOE";	/* MMSYS_MUTEX_base */
	case 0x14012000: return "DISP_DSI0";	/* MMSYS_MUTEX_base */
	case 0x14013000: return "DISP_DPI0";	/* MMSYS_MUTEX_base */
	case 0x14014000: return "DISP_PWM0";	/* MMSYS_MUTEX_base */
	case 0x14015000: return "MM_MUTEX";	/* MMSYS_MUTEX_base */
	case 0x14016000: return "SMI_LARB0";	/* MMSYS_MUTEX_base */
	case 0x14017000: return "SMI_COMMON";	/* MMSYS_MUTEX_base */
	case 0x14018000: return "DISP_WDMA1";	/* MMSYS_MUTEX_base */
	case 0x14019000: return "UFOD_RDMA0"; /* MMSYS_MUTEX_base */
	case 0x1401A000: return "UFOD_RDMA1";	/* MMSYS_MUTEX_base */
	case 0x1401C000: return "DPI1";	/* MMSYS_MUTEX_base */
	case 0x1401D000: return "HDMI";	/* MMSYS_MUTEX_base */

	case 0x16010000: return "SMI_LARB1";
	case 0x15001000: return "SMI_LARB2";

	case 0x17000000: return "VENC_GCON";
	case 0x17001000: return "SMI_LARB3";
	case 0x17002000: return "VENC";
	case 0x17003000: return "JPGENC";
	case 0x17004000: return "JPGDEC";
	}

	/* for other register address we rely on GCE subsys to group them with */
	/* 16-bit mask. */
#undef DECLARE_CMDQ_SUBSYS
#define DECLARE_CMDQ_SUBSYS(msb, id, grp, base) case msb: return #grp;
	switch (addr_base_shifted) {
#include "cmdq_subsys.h"
	}
#undef DECLARE_CMDQ_SUBSYS
	return module;
}


const int32_t cmdq_core_can_module_entry_suspend(struct EngineStruct *engineList)
{
	int32_t status = 0;
	int i;
	enum CMDQ_ENG_ENUM e = 0;

	enum CMDQ_ENG_ENUM mdpEngines[] = {
		CMDQ_ENG_ISP_IMGI, CMDQ_ENG_MDP_RDMA0, CMDQ_ENG_MDP_RSZ0,
		CMDQ_ENG_MDP_RSZ1, CMDQ_ENG_MDP_TDSHP0, CMDQ_ENG_MDP_WROT0,
		CMDQ_ENG_MDP_WDMA
	};

	for (i = 0; i < (sizeof(mdpEngines) / sizeof(enum CMDQ_ENG_ENUM)); ++i) {
		e = mdpEngines[i];
		if (0 != engineList[e].userCount) {
			CMDQ_ERR("suspend but engine %d has userCount %d, owner=%d\n",
				 e, engineList[e].userCount, engineList[e].currOwner);
			status = -EBUSY;
		}
	}

	return status;
}

void cmdq_core_enable_common_clock_locked_impl(bool enable)
{
#ifdef CMDQ_PWR_AWARE
	if (enable) {
		CMDQ_MSG("[CLOCK] Enable CMDQ(GCE) Clock\n");
		cmdq_core_enable_cmdq_clock_locked_impl(enable, CMDQ_DRIVER_DEVICE_NAME);


		CMDQ_MSG("[CLOCK] Enable SMI & LARB0 Clock\n");
		mtk_smi_larb_clock_on(0, true);
		/* cmdq_core_enable_ccf_clk(CMDQ_CLK_DISP0_SMI_COMMON); */
#ifdef CMDQ_CG_M4U_LARB0
		m4u_larb0_enable("CMDQ_MDP");
#else
		/* cmdq_core_enable_ccf_clk(CMDQ_CLK_DISP0_SMI_LARB0); */
#endif
	} else {
		CMDQ_MSG("[CLOCK] Disable CMDQ(GCE) Clock\n");
		cmdq_core_enable_cmdq_clock_locked_impl(enable, CMDQ_DRIVER_DEVICE_NAME);


		CMDQ_MSG("[CLOCK] Disable SMI & LARB0 Clock\n");
		/* disable, reverse the sequence */
#ifdef CMDQ_CG_M4U_LARB0
		m4u_larb0_disable("CMDQ_MDP");
#else
		/* cmdq_core_disable_ccf_clk(CMDQ_CLK_DISP0_SMI_LARB0); */
#endif
		mtk_smi_larb_clock_off(0, true);
		/* cmdq_core_disable_ccf_clk(CMDQ_CLK_DISP0_SMI_COMMON); */
	}
#endif				/* CMDQ_PWR_AWARE */
}

ssize_t cmdq_core_print_event(char *buf)
{
	char *pBuffer = buf;
	const uint32_t events[] = { CMDQ_SYNC_TOKEN_CABC_EOF,
		CMDQ_SYNC_TOKEN_CONFIG_DIRTY,
		CMDQ_EVENT_DSI_TE,
		CMDQ_SYNC_TOKEN_STREAM_EOF
	};
	uint32_t eventValue = 0;
	int i = 0;

	for (i = 0; i < (sizeof(events) / sizeof(uint32_t)); ++i) {
		const uint32_t e = 0x3FF & events[i];

		CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_ID, e);
		eventValue = CMDQ_REG_GET32(CMDQ_SYNC_TOKEN_VAL);
		pBuffer += sprintf(pBuffer, "%s = %d\n", cmdq_core_get_event_name(e), eventValue);
	}

	return pBuffer - buf;
}

void cmdq_core_print_event_seq(struct seq_file *m)
{
	const uint32_t events[] = { CMDQ_SYNC_TOKEN_CABC_EOF,
		CMDQ_SYNC_TOKEN_CONFIG_DIRTY,
		CMDQ_EVENT_DSI_TE,
		CMDQ_SYNC_TOKEN_STREAM_EOF
	};
	uint32_t eventValue = 0;
	int i = 0;

	seq_puts(m, "====== Events =======\n");

	for (i = 0; i < (sizeof(events) / sizeof(uint32_t)); ++i) {
		const uint32_t e = 0x3FF & events[i];

		CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_ID, e);
		eventValue = CMDQ_REG_GET32(CMDQ_SYNC_TOKEN_VAL);
		seq_printf(m, "[EVENT]%s = %d\n", cmdq_core_get_event_name(e), eventValue);
	}
}

