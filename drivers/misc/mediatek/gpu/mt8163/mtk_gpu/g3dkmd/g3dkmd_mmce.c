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

#ifdef _WIN32
#include <Windows.h>
#endif

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dkmd_util.h"
#include "g3dkmd_task.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_mmce.h"
#include "g3dkmd_mmce_reg.h"
#include "g3dkmd_macro.h"
#include "g3dkmd_file_io.h"

#ifdef USE_GCE

/* local define */
#define MMCE_MAX_THD_CNT    1
#define MMCE_TIMEOUT_CNT    1000
#define MMCE_THD0_ID        0
#define MMCE_THD1_ID        1

REG_ACCESS_DATA_IMP(MMCE);
REG_ACCESS_FUNC_IMP(MMCE);

enum MMCE_STATE {
	MMCE_STATE_STOP,
	MMCE_STATE_RUN,
	MMCE_STATE_PAUSE
};

struct MMCE_THD {
	enum MMCE_STATE state;
	unsigned int thd_id;
	MMCQ_HANDLE mmcq_handle;
};

struct MmceExecuteInfo {
	struct MMCE_THD mmceThd[MMCE_MAX_THD_CNT];
};

/* local variable definition */
static struct MmceExecuteInfo s_mmceInfo;
static unsigned int s_mmceThdMap[] = { MMCE_THD0_ID };

/* local function declaration */

/* implementation */
static unsigned int _g3dKmdMmceReadToken(unsigned token_id)
{
	unsigned value;

	MMCE_REG_WRITE32_MSK_SFT(REG_CMDQ_SYNC_TOKEN_ID, token_id, MSK_TOKEN_ID, SFT_TOKEN_ID);
	value =
	    MMCE_REG_READ32_MSK_SFT(REG_CMDQ_SYNC_TOKEN_VALUE, MSK_TOKEN_VALUE, SFT_TOKEN_VALUE);

	return value;
}

void g3dKmdMmceWriteToken(unsigned int token_id, unsigned int value)
{
	MMCE_REG_WRITE32(REG_CMDQ_SYNC_TOKEN_UPDATE, TOKEN_UPDATE_PAIR(token_id, value));
}

unsigned int g3dKmdMmceResetThread(unsigned int thd_id)
{
	_DEFINE_BYPASS_;
	unsigned int cnt;

	/* reset thread */
	MMCE_REG_WRITE32_MSK_SFT(CMDQ_THRx_RESET(thd_id), 1, MSK_THR0_WARM_RST, SFT_THR0_WARM_RST);

#ifndef G3D_HW			/* this is only to simulate HW */
	_BYPASS_(BYPASS_RIU);
	MMCE_REG_WRITE32_MSK_SFT(CMDQ_THRx_RESET(thd_id), 0, MSK_THR0_WARM_RST, SFT_THR0_WARM_RST);
	_RESTORE_();
#endif
	/* wait reset complete */
	cnt = 0;
	while (cnt < MMCE_TIMEOUT_CNT) {
		if (MMCE_REG_READ32_MSK_SFT
		    (CMDQ_THRx_RESET(thd_id), MSK_THR0_WARM_RST, SFT_THR0_WARM_RST) == 0) {
			break;
		}
		G3DKMD_SLEEP(1);
		cnt++;
	}

#ifdef PATTERN_DUMP
	if (!g3dKmdIsBypass(BYPASS_RIU)) {
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfff8, MSK_THR0_WARM_RST);
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n",
			       MOD_OFST(MMCE, CMDQ_THRx_RESET(thd_id)), 0);
	}
#endif

	YL_KMD_ASSERT(cnt < MMCE_TIMEOUT_CNT);
	if (cnt == MMCE_TIMEOUT_CNT) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "Wait MMCE_reset_complete fail\n");
		return G3DKMD_FALSE;
	}

	return G3DKMD_TRUE;
}

void g3dKmdMmceResetThreads(void)
{
	struct MMCE_THD *pThd;
	unsigned int i;

	for (i = 0; i < MMCE_MAX_THD_CNT; i++) {
		pThd = &s_mmceInfo.mmceThd[i];
		g3dKmdMmceResetThread(pThd->thd_id);
	}

#ifdef MMCE_SUPPORT_IOMMU
	MMCE_REG_WRITE32_MSK_SFT(REG_CMDQ_BUS_CTRL, 0x1, MSK_MMU_EN, SFT_MMU_EN);
	MMCE_REG_WRITE32_MSK_SFT(REG_CMDQ_BUS_CTRL, 0x1, MSK_COHERENCE_EN, SFT_COHERENCE_EN);
#endif
}

void g3dKmdMmceResetCore(void)
{
	_DEFINE_BYPASS_;
	unsigned int cnt;

	/* reset thread */
	MMCE_REG_WRITE32_MSK_SFT(REG_CMDQ_CORE_RESET, 1, MSK_CORE_WARM_RST, SFT_CORE_WARM_RST);

#ifndef G3D_HW			/* this is only to simulate HW */
	_BYPASS_(BYPASS_RIU);
	MMCE_REG_WRITE32_MSK_SFT(REG_CMDQ_CORE_RESET, 0, MSK_CORE_WARM_RST, SFT_CORE_WARM_RST);
	_RESTORE_();
#endif
	/* wait reset complete */
	cnt = 0;
	while (cnt < MMCE_TIMEOUT_CNT) {
		if (MMCE_REG_READ32_MSK_SFT
		    (REG_CMDQ_CORE_RESET, MSK_CORE_WARM_RST, SFT_CORE_WARM_RST) == 0) {
			break;
		}
		G3DKMD_SLEEP(1);
		cnt++;
	}

#ifdef PATTERN_DUMP
	if (!g3dKmdIsBypass(BYPASS_RIU)) {
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfff8, MSK_CORE_WARM_RST);
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", MOD_OFST(MMCE, REG_CMDQ_CORE_RESET),
			       MSK_CORE_WARM_RST);
	}
#endif

	YL_KMD_ASSERT(cnt < MMCE_TIMEOUT_CNT);
	if (cnt == MMCE_TIMEOUT_CNT) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "Wait REG_CMDQ_CORE_RESET fail\n");
		return;
	}
#ifdef MMCE_SUPPORT_IOMMU
	MMCE_REG_WRITE32_MSK_SFT(REG_CMDQ_BUS_CTRL, 0x1, MSK_MMU_EN, SFT_MMU_EN);
	MMCE_REG_WRITE32_MSK_SFT(REG_CMDQ_BUS_CTRL, 0x1, MSK_COHERENCE_EN, SFT_COHERENCE_EN);
#endif
}

void g3dKmdMmceInit(void)
{
	struct MMCE_THD *pThd;
	unsigned int i;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCE, "\n");

	for (i = 0; i < MMCE_MAX_THD_CNT; i++) {
		pThd = &s_mmceInfo.mmceThd[i];
		pThd->state = MMCE_STATE_STOP;
		pThd->thd_id = s_mmceThdMap[i];
		pThd->mmcq_handle = NULL;
	}

	g3dKmdMmceResetThreads();

	/* init frame_flush token value */
	g3dKmdMmceWriteToken(MMCQ_TOKEN_FRAME_FLUSH, MMCQ_TOKEN_VALUE_FALSE);
}

MMCE_THD_HANDLE g3dKmdMmceGetFreeThd(void)
{
	unsigned int i;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCE, "Thd");

	for (i = 0; i < MMCE_MAX_THD_CNT; i++) {
		if (s_mmceInfo.mmceThd[i].state != MMCE_STATE_RUN) {
			KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_MMCE, " %d\n", i);
			return i;
		}
	}

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, " Invalid\n");

	return MMCE_INVALID_THD_HANDLE;
}

unsigned int g3dKmdMmceGetThreadId(MMCE_THD_HANDLE handle)
{
	struct MMCE_THD *mmce = (MMCE_THD *) &s_mmceInfo.mmceThd[handle];

	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCE,"mmce %d\n", handle)); */

	if (handle >= MMCE_MAX_THD_CNT) {
		YL_KMD_ASSERT(handle < MMCE_MAX_THD_CNT);
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "mmce invalid handle\n");
		return MMCE_INVALID_THD_ID;
	}

	return mmce->thd_id;
}

unsigned int g3dKmdMmceGetReadPtr(MMCE_THD_HANDLE handle)
{
	unsigned int ptr;
	struct MMCE_THD *mmce = (struct MMCE_THD *) &s_mmceInfo.mmceThd[handle];

	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCE,"mmce %d", handle)); */

	if (handle >= MMCE_MAX_THD_CNT) {
		YL_KMD_ASSERT(handle < MMCE_MAX_THD_CNT);
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "\nmmce invalid handle\n");
		return 0;
	}

	if (mmce->state == MMCE_STATE_RUN) {
		/* ptr = MMCE_REG_READ32(CMDQ_THRx_PC(mmce->thd_id)); */
		ptr = MMCE_REG_READ32(CMDQ_THRx_EXEC_CMDS_CNT(mmce->thd_id));
		return ptr;
	}

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "\nmmce not in runnnig state\n");
	return 0;
}

#ifdef PATTERN_DUMP
void g3dKmdMmceSetReadPtr(MMCE_THD_HANDLE handle, unsigned int phy_addr)
{
	_DEFINE_BYPASS_;
	struct MMCE_THD *mmce = (struct MMCE_THD *) &s_mmceInfo.mmceThd[handle];

	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCE,"mmce %d", handle)); */

	if (handle >= MMCE_MAX_THD_CNT) {
		YL_KMD_ASSERT(handle < MMCE_MAX_THD_CNT);
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "\nmmce invalid handle\n");
		return;
	}

	if (mmce->state == MMCE_STATE_RUN) {
		_BYPASS_(BYPASS_RIU);
		MMCE_REG_WRITE32(CMDQ_THRx_EXEC_CMDS_CNT(mmce->thd_id), phy_addr);
		_RESTORE_();
		return;
	}

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "\nmmce not in runnnig state\n");
}
#endif

void g3dKmdMmceResume(MMCE_THD_HANDLE handle, void *mmcq_handle, unsigned int hwChannel)
{
	unsigned int phy_addr;
	struct MMCE_THD *mmce = (struct MMCE_THD *) &s_mmceInfo.mmceThd[handle];

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCE, "mmce %d, mmcq 0x%x\n", handle, mmcq_handle);

	if ((handle >= MMCE_MAX_THD_CNT) || (mmcq_handle == NULL)) {
		YL_KMD_ASSERT((handle < MMCE_MAX_THD_CNT) && (mmcq_handle != NULL));
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "mmce or mmcq NULL\n");
		return;
	}
	/* update jump target (backup_pc_phy) */
	g3dKmdMmcqSetJumpTarget(mmcq_handle, g3dKmdMmcqGetPC(mmcq_handle));

	/* reset thread */
	if (!g3dKmdMmceResetThread(mmce->thd_id)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "Wait MMCE_reset_complete fail\n");
		return;
	}
	/* set init value (PC, end_addr ... etc) */
	phy_addr = g3dKmdMmcqGetAddr(mmcq_handle, MMCQ_ADDR_BASE0);
	MMCE_REG_WRITE32(CMDQ_THRx_PC(mmce->thd_id), phy_addr);

	if (g3dKmdMmcqGetState(mmcq_handle) == MMCQ_STATE_STOP) {	/* newly launched AP */
		/* read/write point to the same addr */
		MMCE_REG_WRITE32(CMDQ_THRx_END_ADDR(mmce->thd_id), phy_addr);
	} else {		/* previously suspended AP */
		/* write point to the last instruction */
		phy_addr = g3dKmdMmcqGetAddr(mmcq_handle, MMCQ_ADDR_WRITE_PTR);
		MMCE_REG_WRITE32(CMDQ_THRx_END_ADDR(mmce->thd_id), phy_addr);
	}

	/* set MMCQ ring buffer read address */
	MMCE_REG_WRITE32(CMDQ_THRx_EXEC_CMDS_CNT(mmce->thd_id), g3dKmdMmcqGetPC(mmcq_handle));

	/* set timeout cycles (max value is 0x1B, around 500ms) */
	MMCE_REG_WRITE32_MSK_SFT(CMDQ_THRx_INSTN_TIMEOUT_CYCLES(mmce->thd_id), 0x1B,
				 MSK_THR0_INSTN_TIMEOUT_CYCLES, SFT_THR0_INSTN_TIMEOUT_CYCLES);

	/* enable ISR (TBD, add ISR to do error handling) */
	MMCE_REG_WRITE32_MSK_SFT(CMDQ_THRx_IRQ_FLAG_EN(mmce->thd_id), 0x0,
				 MSK_THR0_INSTN_TIMEOUT_FLAG_EN, SFT_THR0_INSTN_TIMEOUT_FLAG_EN);
	MMCE_REG_WRITE32_MSK_SFT(CMDQ_THRx_IRQ_FLAG_EN(mmce->thd_id), 0x0,
				 MSK_THR0_INVALID_INSTN_FLAG_EN, SFT_THR0_INVALID_INSTN_FLAG_EN);

	/* enable thread */
	MMCE_REG_WRITE32_MSK_SFT(CMDQ_THRx_EN(mmce->thd_id), 1, MSK_THR0_EN, SFT_THR0_EN);

	/* update state */
	mmce->state = MMCE_STATE_RUN;
	mmce->mmcq_handle = mmcq_handle;
	g3dKmdMmcqResume(mmcq_handle);
}

void g3dKmdMmceSuspend(MMCE_THD_HANDLE handle, unsigned int hwChannel)
{
	_DEFINE_BYPASS_;
	unsigned int read_ptr, cnt = 0;
	struct MMCE_THD *mmce = (struct MMCE_THD *) &s_mmceInfo.mmceThd[handle];

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCE, "mmce %d\n", handle);

	if (handle >= MMCE_MAX_THD_CNT) {
		YL_KMD_ASSERT(handle < MMCE_MAX_THD_CNT);
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "mmce invalid handle\n");
		return;
	}
	/* wait until pervious task already finished qld stage */
#ifndef G3D_HW			/* this is only to simulate status update after suspend when CMODEL */
	_BYPASS_(BYPASS_RIU);
	MMCE_REG_WRITE32_MSK_SFT(REG_CMDQ_SYNC_TOKEN_VALUE, MMCQ_TOKEN_VALUE_TRUE, MSK_TOKEN_VALUE,
				 SFT_TOKEN_VALUE);
	_RESTORE_();
#endif
	cnt = 0;
	while (cnt < MMCE_TIMEOUT_CNT) {
		if (_g3dKmdMmceReadToken(MMCQ_TOKEN_FRAME_FLUSH) == MMCQ_TOKEN_VALUE_TRUE)
			break;

		G3DKMD_SLEEP(1);
		cnt++;
	}

	YL_KMD_ASSERT(cnt < MMCE_TIMEOUT_CNT);
	if (cnt == MMCE_TIMEOUT_CNT) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "Wait FrameFlush fail\n");
		return;
	}

	mmce->state = MMCE_STATE_PAUSE;

	/* suspend mmce */
	MMCE_REG_WRITE32_MSK_SFT(CMDQ_THRx_SUSPEND(mmce->thd_id), 1, MSK_THR0_SUSPEND,
				 SFT_THR0_SUSPEND);
#ifndef G3D_HW			/* this is only to simulate status update after suspend when CMODEL */
	_BYPASS_(BYPASS_RIU);
	MMCE_REG_WRITE32_MSK_SFT(CMDQ_THRx_STATUS(mmce->thd_id), 1, MSK_THR0_SUSPENDED,
				 SFT_THR0_SUSPENDED);
	_RESTORE_();
#endif
	cnt = 0;
	while (cnt < MMCE_TIMEOUT_CNT) {
		if (MMCE_REG_READ32_MSK_SFT(CMDQ_THRx_STATUS(mmce->thd_id), MSK_THR0_SUSPENDED, SFT_THR0_SUSPENDED))
			break;

		G3DKMD_SLEEP(1);
		cnt++;
	}

	YL_KMD_ASSERT(cnt < MMCE_TIMEOUT_CNT);
	if (cnt == MMCE_TIMEOUT_CNT) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "Suspend fail\n");
		return;
	}
	/* backup PC */
	read_ptr = MMCE_REG_READ32(CMDQ_THRx_PC(mmce->thd_id));
	g3dKmdMmcqSetPC(mmce->mmcq_handle, read_ptr);

	/* set mmcq state to pause */
	g3dKmdMmcqSuspend(mmce->mmcq_handle);
}

void g3dKmdMmceFlush(MMCE_THD_HANDLE handle)
{
	_DEFINE_BYPASS_;
	unsigned int phy_addr;
	struct MMCE_THD *mmce = (struct MMCE_THD *) &s_mmceInfo.mmceThd[handle];

	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCE,"mmce %d\n", handle)); */

	if (handle >= MMCE_MAX_THD_CNT) {
		YL_KMD_ASSERT(handle < MMCE_MAX_THD_CNT);
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "mmce invalid handle\n");
		return;
	}

	if (mmce->state == MMCE_STATE_RUN) {
		phy_addr = g3dKmdMmcqGetAddr(mmce->mmcq_handle, MMCQ_ADDR_WRITE_PTR);
		MMCE_REG_WRITE32(CMDQ_THRx_END_ADDR(mmce->thd_id), phy_addr);

#ifndef G3D_HW			/* when CMODEL, move current PC ASAP */
		_BYPASS_(BYPASS_RIU);
		MMCE_REG_WRITE32(CMDQ_THRx_PC(mmce->thd_id), phy_addr);
		MMCE_REG_WRITE32(CMDQ_THRx_EXEC_CMDS_CNT(mmce->thd_id), phy_addr);
		_RESTORE_();
#endif
	}
}

void g3dKmdMmceWaitIdle(MMCE_THD_HANDLE handle)
{
#define TIME_OUT_CNT 100
	struct MMCE_THD *mmce = (struct MMCE_THD *) &s_mmceInfo.mmceThd[handle];
	unsigned int thr_pc, thr_end_addr, cnt;

	if (handle >= MMCE_MAX_THD_CNT) {
		YL_KMD_ASSERT(handle < MMCE_MAX_THD_CNT);
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCE, "mmce invalid handle\n");
		return;
	}

	if (mmce->state == MMCE_STATE_RUN) {
		thr_end_addr = MMCE_REG_READ32(CMDQ_THRx_END_ADDR(mmce->thd_id));
		thr_pc = MMCE_REG_READ32(CMDQ_THRx_PC(mmce->thd_id));

		cnt = 0;
		while (cnt++ < TIME_OUT_CNT) {
			if (thr_end_addr == thr_pc)
				break;

			G3DKMD_NDELAY(1);
		}

		YL_KMD_ASSERT(cnt < TIME_OUT_CNT);
	}
}
#endif /* USE_GCE */
