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
#include "g3dkmd_task.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_scheduler.h"
#include "g3dkmd_mmce.h"
#include "g3dkmd_mmcq.h"
#include "g3dkmd_gce.h"
#include "g3dkmd_mmce_reg.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_util.h"
#include "g3dkmd_cfg.h"
#include "g3dkmd_pattern.h"
#include "sapphire_reg.h"

#ifdef linux
#if !defined(linux_user_mode)
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/time.h>
#endif
#endif

#include "test/tester_non_polling_flushcmd.h"	/* for tester_fence_block() {} */

#ifdef G3DKMD_SUPPORT_FDQ
#include "g3dkmd_fdq.h"
#endif

#define G3D_ADDR(reg, ch)           (reg + G3D_MOD_OFST)

#ifdef USE_GCE

void g3dKmdGcePrepareQld(struct g3dExeInst *pInst)
{
	MMCQ_HANDLE mmcq = pInst->mmcq_handle;
	unsigned int hwCh = pInst->hwChannel;

	if (!g3dKmdCfgGet(CFG_GCE_ENABLE)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_GCE,
		       "[%s] GCE is not enabled, this API should not be called!\n");
		return;
	}

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_GCE, "inst %p\n", pInst);

	/* reset mmcq queue */
	g3dKmdMmcqReset(mmcq);

	/* wait for enough queue size */
	if (!g3dKmdMmcqWaitSize(mmcq, MMCQ_INSTRUCTION_SIZE * ((13 + 1) * 2))) {/* "+ 1" for possible ringback */
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_GCE,
		       "g3dKmdMmcqWaitSize error, can't wait enough buffer size (%d)\n",
		       MMCQ_INSTRUCTION_SIZE * 8);
		YL_KMD_ASSERT(0);
		return;
	}

	g3dKmdMmcqSetIsSuspendable(mmcq, MMCQ_NON_SUSPENDABLE);

	/* clear frame flush */
	g3dKmdMmcqUpdateToken(mmcq, MMCQ_TOKEN_FRAME_FLUSH, MMCQ_TOKEN_VALUE_FALSE);
	/*
	g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_MMCE, REG_CMDQ_SYNC_TOKEN_UPDATE,
			TOKEN_UPDATE_PAIR(MMCQ_TOKEN_FRAME_FLUSH, MMCQ_TOKEN_VALUE_FALSE),
			MMCQ_DEFAULT_MASK);
	*/

	/* enable prefetch */
	g3dKmdMmcqAddMarker(mmcq, MMCQ_MARKER_PREFETCH_EN);

#ifdef YL_SECURE
	/* set security flag */
	if (g3dKmdCfgGet(CFG_SECURE_ENABLE)) {
		unsigned int value;

		g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_CQ_SECURE_CTRL, hwCh), 0x1,
				MSK_AREG_CQ_SECURITY);

		value = 0x1 << SFT_AREG_MX_SECURE0_ENABLE;
		g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU,
				G3D_ADDR(REG_AREG_MX_SECURE0_BASE_ADDR, hwCh), value,
				MSK_AREG_MX_SECURE0_ENABLE);

		value = (G3DKMD_FLOOR(pInst->qbaseBuffer.hwAddr, 0x1000) >> 12);
		g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU,
				G3D_ADDR(REG_AREG_MX_SECURE0_BASE_ADDR, hwCh), value,
				MSK_AREG_MX_SECURE0_BASE_ADDR);

		value =
		    G3DKMD_CEIL(pInst->qbaseBuffer.hwAddr + pInst->qbaseBufferSize, 0x1000) >> 12;
		g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_MX_SECURE0_TOP_ADDR, hwCh),
				value, MSK_AREG_MX_SECURE0_TOP_ADDR);
	} else {
		g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_CQ_SECURE_CTRL, hwCh), 0x0,
				MSK_AREG_CQ_SECURITY);
	}
#endif

	/* get token */
	g3dKmdMmcqPoll(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_G3D_TOKEN0, hwCh),
		       (0x1 << SFT_AREG_G3D_TOKEN0), MSK_AREG_G3D_TOKEN0);

	/* set qinfo buffer addr */
	g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_CQ_QBUF_START_ADDR, hwCh),
			pInst->qbaseBuffer.hwAddr, MSK_AREG_CQ_QBUF_START_ADDR);
	g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_CQ_QBUF_END_ADDR, hwCh),
			(pInst->qbaseBuffer.hwAddr + pInst->qbaseBufferSize),
			MSK_AREG_CQ_QBUF_END_ADDR);

	/* trigger qld upd */
	g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_CQ_CTRL, hwCh),
			(0x1 << SFT_AREG_CQ_QLD_UPD), MSK_AREG_CQ_QLD_UPD);

	/* enable clk (fire), auto clear */
	g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_G3D_ENG_FIRE, hwCh),
			(0x1 << SFT_AREG_G3D_ENG_FIRE), MSK_AREG_G3D_ENG_FIRE);

	/* disable qld upd */
	g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_CQ_CTRL, hwCh),
			(0x0 << SFT_AREG_CQ_QLD_UPD), MSK_AREG_CQ_QLD_UPD);

	/* wait frame flush */
	g3dKmdMmcqWaitForEvent(mmcq, MMCQ_TOKEN_FRAME_FLUSH, MMCQ_TOKEN_VALUE_TRUE);

	/* release token */
	g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_G3D_TOKEN0, hwCh),
			(0x1 << SFT_AREG_G3D_TOKEN0), MSK_AREG_G3D_TOKEN0);

	/* Jump to last suspended PC */
	g3dKmdMmcqJumpToNextInst(mmcq);

	/* Update buffer address of end0 & base1 */
	g3dKmdMmcqUpdateBuffAddr(mmcq);

#ifndef G3D_HW
/* when change thread, we have to restore previous head/ tail pointer, otherwise the h/t pointer will be wrong;
 * in HW case, HW will do it (CMODEL case)
 */
	g3dKmdRegUpdateReadPtr(hwCh, pInst->pHtDataForHwRead->htInfo->wtPtr);
	g3dKmdRegUpdateWritePtr(hwCh, pInst->pHtDataForHwRead->htInfo->wtPtr);
#endif
}

unsigned char g3dKmdGceSendCommand(struct g3dExeInst *pInst)
{
	void *mmcq = pInst->mmcq_handle;
	unsigned int hwCh = pInst->hwChannel;

	if (!g3dKmdCfgGet(CFG_GCE_ENABLE)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_GCE,
		       "[%s] GCE is not enabled, this API should not be called!\n");
		return G3DKMD_FALSE;
	}

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_GCE, "inst %p\n", pInst);

#ifdef G3D_HW
	if (!g3dKmdCheckBufferConsistency
	    (hwCh, pInst->pHtDataForHwRead->htBuffer.hwAddr,
	     pInst->pHtDataForHwRead->htBufferSize)) {
		/* this should not happen, because it's already set when context switch (g3dKmdPrepareMmcqGbl()) */
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_GCE, "H/T buffer start/end addr mismatch\n");
		YL_KMD_ASSERT(0);
		return G3DKMD_FALSE;
	}
#endif

	/* wait for enough queue size */
	if (!g3dKmdMmcqWaitSize(mmcq, MMCQ_INSTRUCTION_SIZE * ((7 + 1) * 2))) { /* "+ 1" for possible ringback */
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_GCE,
		       "g3dKmdMmcqWaitSize error, can't wait enough buffer size (%d)\n",
		       MMCQ_INSTRUCTION_SIZE * 13);
		return G3DKMD_FALSE;
	}

	g3dKmdMmcqSetIsSuspendable(mmcq, MMCQ_NON_SUSPENDABLE);

	/* update tail pointer */
	g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_CQ_HTBUF_WPTR, hwCh),
			pInst->pHtDataForHwRead->htInfo->wtPtr, MSK_AREG_CQ_HTBUF_WPTR);

	/* trigger tail upd */
	g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_CQ_CTRL, hwCh),
			(0x1 << SFT_AREG_CQ_HTBUF_WPTR_UPD), MSK_AREG_CQ_HTBUF_WPTR_UPD);

	/* enable clk (fire) */
	g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_G3D_ENG_FIRE, hwCh),
			(0x1 << SFT_AREG_G3D_ENG_FIRE), MSK_AREG_G3D_ENG_FIRE);

	/* disable tail upd */
	g3dKmdMmcqWrite(mmcq, MMCQ_SUBSYS_GPU, G3D_ADDR(REG_AREG_CQ_CTRL, hwCh),
			(0x0 << SFT_AREG_CQ_HTBUF_WPTR_UPD), MSK_AREG_CQ_HTBUF_WPTR_UPD);

	g3dKmdMmcqSetIsSuspendable(mmcq, MMCQ_SUSPENDABLE);

	/* flush, this also is the command which can be suspended */
	g3dKmdMmcqFlush(mmcq);

	/* auto consume h/t buffer when CMODEL mode */
	if (pInst->state == G3DEXESTATE_RUNNING) {
#ifndef G3D_HW
		if (tester_fence_block()) {
			/* KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_GCE, "fence is opened!\n"); */
			g3dKmdRegUpdateReadPtr(hwCh, pInst->pHtDataForHwRead->htInfo->wtPtr);	/* HW is done. */
		}		/* tester_fence_block(); */
#endif
	}

	return G3DKMD_TRUE;
}

void g3dKmdGceDoContextSwitch(struct g3dExeInst *pInstFrom, struct g3dExeInst *pInstTo, int freeHwCh)
{
	_DEFINE_BYPASS_;
	unsigned int inst_idx;

	if (!g3dKmdCfgGet(CFG_GCE_ENABLE)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_GCE,
		       "[%s] GCE is not enabled, this API should not be called!\n");
		return;
	}

	inst_idx = g3dKmdExeInstGetIdx(pInstTo);
	YL_KMD_ASSERT(inst_idx != EXE_INST_NONE);

	if (pInstFrom) {
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_GCE,
		       "suspend task (%d, %p), mmcq_handle %p, thd_handle %d\n",
		       g3dKmdExeInstGetIdx(pInstFrom), pInstFrom, pInstFrom->mmcq_handle,
		       pInstFrom->thd_handle);

		/* suspend MMCE */
		g3dKmdMmceSuspend(pInstFrom->thd_handle, pInstFrom->hwChannel);

		/* update TaskFrom status */
		pInstFrom->hwChannel = G3DKMD_HWCHANNEL_NONE;
		pInstFrom->thd_handle = MMCE_INVALID_THD_HANDLE;
		pInstFrom->state = G3DEXESTATE_WAITQUEUE;
	}
	/* resume MMCE */
	pInstTo->hwChannel = freeHwCh;
	pInstTo->thd_handle = g3dKmdMmceGetFreeThd();

	YL_KMD_ASSERT(pInstTo->thd_handle != MMCE_INVALID_THD_HANDLE);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_GCE,
	       "resume task (%d, %p), mmcq_handle %p, thd_handle %d\n", inst_idx, pInstTo,
	       pInstTo->mmcq_handle, pInstTo->thd_handle);

	g3dKmdMmcqSetThread(pInstTo->mmcq_handle, pInstTo->thd_handle);

	if (pInstTo->state == G3DEXESTATE_IDLE) {
		pInstTo->time_slice = G3DKMD_TIME_SLICE_NEW_THD;
		_BYPASS_(BYPASS_RIU);
		g3dKmdGcePrepareQld(pInstTo);
		_RESTORE_();
	} else {
		pInstTo->time_slice =
		    G3DKMD_TIME_SLICE_BASE + pInstTo->prioWeighting * G3DKMD_TIME_SLICE_INC;
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_GCE, "TimeSlice %d, prio %d\n",
		       pInstTo->time_slice, pInstTo->prioWeighting);
	}
	g3dKmdMmceResume(pInstTo->thd_handle, pInstTo->mmcq_handle, pInstTo->hwChannel);

	pInstTo->state = G3DEXESTATE_RUNNING;
	gExecInfo.hwStatus[freeHwCh].inst_idx = inst_idx;
	gExecInfo.hwStatus[freeHwCh].state = G3DKMD_HWSTATE_RUNNING;

	g3dKmdGceSendCommand(pInstTo);


#if !defined(G3D_HW) && defined(TEST_NON_POLLING_FLUSHCMD)
	/* In GceSendCommand(), */
	/* The updateReadPtr() is block by TEST_NON_POLLING_FLUSHCMD, so add it when Qload. */
	g3dKmdRegUpdateReadPtr(pInstTo->hwChannel, pInstTo->pHtDataForHwRead->htInfo->rdPtr);	/* HW restore rdPtr. */
#endif

#if !defined(G3D_HW) && defined(G3DKMD_SUPPORT_FDQ)
	{
		unsigned int task_id;
		struct g3dTask *task;

		for (task_id = 0; task_id < (unsigned int)gExecInfo.taskAllocCount; task_id++) {
			task = g3dKmdGetTask(task_id);
			if ((task != NULL) && (task->inst_idx == inst_idx)) {
				g3dKmdFdqWriteQ(task_id);
				break;
			}
		}
	}
#endif
}

/* leon TBD, connect to G3D ISR */
#if 0
void g3dKmdGceQueueEmpty(unsigned int hwCh)
{
	int i;
	struct g3dTask *pTask;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_GCE, "hwCh %d\n", hwCh);

	if (hwCh >= NUM_HW_CMD_QUEUE) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_GCE, "hwChannel(%d) >= NUM_HW_CMD_QUEUE\n",
		       hwCh);
		return;
	}

	lock_g3d();
	for (i = 0; i < gExecInfo.taskInfoCount; i++) {
		pTask = g3dKmdGetTask(i);
		if (pTask && (pTask->hwChannel == hwCh) && (pTask->isNewTask == 0)) {
			YL_KMD_ASSERT(pTask->state == G3DTASKSTATE_RUNNING);
			pTask->time_slice = 0;
			break;
		}
	}
	unlock_g3d();

	if (i >= gExecInfo.taskInfoCount)
		return;

	g3dKmdTriggerScheduler();
}
#endif

unsigned char g3dKmdGceCreateMmcq(struct g3dExeInst *pInst)
{
	void *mmcq;

	if (!g3dKmdCfgGet(CFG_GCE_ENABLE))
		return G3DKMD_TRUE;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_GCE, "g3dExeInst %p\n", pInst);

	pInst->thd_handle = MMCE_INVALID_THD_HANDLE;

	mmcq = pInst->mmcq_handle = g3dKmdMmcqCreate();
	if (mmcq == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_GCE, "g3dKmdMmcqCreate fail\n");
		return G3DKMD_FALSE;
	}

	return G3DKMD_TRUE;
}

void g3dKmdGceReleaseMmcq(struct g3dExeInst *pInst)
{
	if (!g3dKmdCfgGet(CFG_GCE_ENABLE))
		return;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_GCE, "g3dExeInst %p\n", pInst);

	if (pInst->thd_handle != MMCE_INVALID_THD_HANDLE) {
		/*
		 * leon TBD, if the suspend task is currently active. after suspend,
		 * should we context switch to another task ?
		 */
		g3dKmdMmceSuspend(pInst->thd_handle, pInst->hwChannel);
	}

	if (pInst->mmcq_handle)
		g3dKmdMmcqRelease(pInst->mmcq_handle);

}

void g3dKmdGceInit(void)
{
	if (g3dKmdCfgGet(CFG_GCE_ENABLE))
		g3dKmdMmceInit();

}

void g3dKmdGceUninit(void)
{
	if (g3dKmdCfgGet(CFG_GCE_ENABLE)) {
		/*
		 * TODO ?
		 */
	}
}

void g3dKmdGceResetHW(void)
{
	if (g3dKmdCfgGet(CFG_GCE_ENABLE)) {
		g3dKmdMmceResetThreads();

		/* init frame_flush token value */
		g3dKmdMmceWriteToken(MMCQ_TOKEN_FRAME_FLUSH, MMCQ_TOKEN_VALUE_FALSE);
	}
}

void g3dKmdGceWaitIdle(void)
{
	struct g3dExeInst *pInst;
	int i;

	if (!g3dKmdCfgGet(CFG_GCE_ENABLE))
		return;

	for (i = 0; i < gExecInfo.exeInfoCount; i++) {
		pInst = gExecInfo.exeList[i];
		if (pInst && (pInst->state == G3DEXESTATE_RUNNING))
			g3dKmdMmcqWaitIdle(pInst->mmcq_handle);

	}
}
#endif /* USE_GCE */
