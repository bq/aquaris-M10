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

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#ifdef G3DKMD_SUPPORT_FDQ

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)
#include <windows.h>
#include <stdlib.h>
#elif defined(linux) && defined(linux_user_mode)	/* linux user space */
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#else
#include <linux/ftrace_event.h>
#endif

#include "sapphire_reg.h"

#include "g3dbase_type.h"
#include "g3dkmd_util.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_task.h"
#include "g3dkmd_internal_memory.h"
#include "g3dkmd_fdq.h"
#include "g3dkmd_isr.h"
#include "g3dkmd_macro.h"
#if defined(YL_MET) || defined(G3DKMD_MET_CTX_SWITCH)
#include "g3dkmd_met.h"
#endif

/* #ifdef G3DKMD_SUPPORT_FENCE */
/* move to g3dkmd_task.h */
/* #include "g3dkmd_fence.h" */
/* #endif */

struct FRM_RTN_DATA {
	union {
		unsigned int dwValue;

		struct {
			unsigned int frame_swap:1;
			unsigned int msaa_abort:1;
			unsigned int reserved:6;
			unsigned int checksum_low:24;	/* used as checksum_low, the lower 24 bits */
		} type0_data0;

		struct {
			unsigned int boff_va_ovf_cnt:8;
			unsigned int boff_bk_ovf_cnt:8;
			unsigned int boff_tb_ovf_cnt:8;
			unsigned int boff_hd_ovf_cnt:8;
		} type0_data1;
	} data;

	unsigned int type:8;	/* used as checksum_high, the higher 8 bits */
	unsigned int Fid:16;
	unsigned int Qid:8;
};

#ifdef G3DKMD_FPS_BY_FDQ
struct g3dkmd_fps_info {
	unsigned long lastSwapTime;	/* the last time to call swap buffer */
	unsigned long lastLogTime;	/* the last time to log fps in a interval */
	unsigned long lastLogDuration;	/* the last time duration to log fps */
	unsigned long lastMaxDuration;	/* the max duration between swap buffer in the last time of fps log */
	unsigned long lastMinDuration;	/* the min duration between swap buffer in the last time of fps log */
	unsigned long logFrameCount;	/* the frame count used in the log interval */
};

/* static variable */
static struct g3dkmd_fps_info g_fps_info;
static unsigned long g_fpsLogInterval = 1000;	/* sync with G3dGbl ?? */
#endif



/* API for debugfs_fdq */
void *_g3dkmd_getFdqDataPtr(void)
{
	return gExecInfo.fdq.data;
}

unsigned int _g3dkmd_getFdqDataSize(void)
{
	return gExecInfo.fdq_size;
}

unsigned int _g3dkmd_getFdqReadIdx(void)
{
	return gExecInfo.fdq_rptr;
}

unsigned int _g3dkmd_getFdqWriteIdx(void)
{
	return g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_HWWPTR,
			     MSK_AREG_PA_FDQ_HWWPTR, SFT_AREG_PA_FDQ_HWWPTR);
}


void g3dKmdFdqInit(void)
{
	unsigned int size = G3DKMD_ALIGN(G3DKMD_FDQ_BUF_SIZE, 4096);	/* page align, in order for G3DKMD_MMU_DMA */

	if (!allocatePhysicalMemory(&gExecInfo.fdq, size, G3DKMD_FDQ_ALIGN,
	     G3DKMD_MMU_ENABLE | G3DKMD_MMU_SHARE | G3DKMD_MMU_DMA)) {
		YL_KMD_ASSERT(0);
		return;
	}
	gExecInfo.fdq_size = G3DKMD_FDQ_BUF_SIZE / 8;	/* 64bit per unit */
	gExecInfo.fdq_rptr = 0;	/* 0-base, 64bit unit */
	gExecInfo.fdq_wptr = 0;

	g3dKmdFdqUpdateBufPtr();

#ifdef G3DKMD_FPS_BY_FDQ
	g3dkmdResetFpsInfo();
#endif
}

void g3dKmdFdqUninit(void)
{
	freePhysicalMemory(&gExecInfo.fdq);
}

void g3dKmdFdqBackupBufPtr(void)
{
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x0, MSK_AREG_MCU2G3D_SET, SFT_AREG_MCU2G3D_SET);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x1, MSK_AREG_MCU2G3D_SET, SFT_AREG_MCU2G3D_SET);

	gExecInfo.fdq_wptr =
	    g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_HWWPTR, MSK_AREG_PA_FDQ_HWWPTR, SFT_AREG_PA_FDQ_HWWPTR);

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_FDQ, "rptr 0x%x, wptr 0x%x\n", gExecInfo.fdq_rptr, gExecInfo.fdq_wptr);
}

void g3dKmdFdqUpdateBufPtr(void)
{
	int i;

	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASK, 0, VAL_IRQ_PA_DATA_QUEUE_NOT_EMPTY, SFT_AREG_IRQ_MASK);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FDQBUF_BASE, gExecInfo.fdq.hwAddr,
		       MSK_AREG_PA_FDQBUF_BASE, SFT_AREG_PA_FDQBUF_BASE);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FDQBUF_SIZE, gExecInfo.fdq_size,
		       MSK_AREG_PA_FDQBUF_SIZE, SFT_AREG_PA_FDQBUF_SIZE);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_SWRPTR, gExecInfo.fdq_rptr,
		       MSK_AREG_PA_FDQ_SWRPTR, SFT_AREG_PA_FDQ_SWRPTR);

	/* workaround, do it twice, such that we can read the correct REG_AREG_PA_FDQ_HWWPTR that we set */
	for (i = 0; i < 2; i++) {
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_HWWPTR_RB, gExecInfo.fdq_wptr,
			       MSK_AREG_PA_FDQ_HWWPTR_RB, SFT_AREG_PA_FDQ_HWWPTR_RB);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_HWWPTR_RB, 0x1,
			       MSK_AREG_PA_FDQ_HWWPTR_RB_UPD, SFT_AREG_PA_FDQ_HWWPTR_RB_UPD);
#ifndef G3D_HW
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_HWWPTR, gExecInfo.fdq_wptr,
			       MSK_AREG_PA_FDQ_HWWPTR, SFT_AREG_PA_FDQ_HWWPTR);
#endif

		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x0, MSK_AREG_MCU2G3D_SET,
			       SFT_AREG_MCU2G3D_SET);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x1, MSK_AREG_MCU2G3D_SET,
			       SFT_AREG_MCU2G3D_SET);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_HWWPTR_RB, 0x0,
			       MSK_AREG_PA_FDQ_HWWPTR_RB_UPD, SFT_AREG_PA_FDQ_HWWPTR_RB_UPD);
	}

#ifdef G3DKMDDEBUG
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x0, MSK_AREG_MCU2G3D_SET, SFT_AREG_MCU2G3D_SET);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x1, MSK_AREG_MCU2G3D_SET, SFT_AREG_MCU2G3D_SET);

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_FDQ, "rptr 0x%x, wptr 0x%x\n",
	       g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_SWRPTR, MSK_AREG_PA_FDQ_SWRPTR,
			     SFT_AREG_PA_FDQ_SWRPTR), g3dKmdRegRead(G3DKMD_REG_G3D,
								    REG_AREG_PA_FDQ_HWWPTR,
								    MSK_AREG_PA_FDQ_HWWPTR,
								    SFT_AREG_PA_FDQ_HWWPTR));
#endif
}

#ifdef G3DKMD_FPS_BY_FDQ

void g3dkmdResetFpsInfo(void)
{
	memset(&g_fps_info, 0, sizeof(g_fps_info));
}

static unsigned long systemTimeMs(void)
{
	return (unsigned long)(jiffies * 1000 / HZ);
}

int g3dKmdGetAverageFps(void)
{
	unsigned long fps_div, fps_rem;
	unsigned long lastLogDuration, lastDuration, stampms = systemTimeMs();

	if (g_fps_info.lastSwapTime == 0 || g_fps_info.lastSwapTime >= stampms) {
		g_fps_info.lastSwapTime = g_fps_info.lastLogTime = stampms;
		return 0;
	}

	g_fps_info.logFrameCount++;
	/* count duration from last time update */
	lastDuration = stampms - g_fps_info.lastSwapTime;
	g_fps_info.lastSwapTime = stampms;

	if ((0 == g_fps_info.lastMaxDuration) || (lastDuration > g_fps_info.lastMaxDuration))
		g_fps_info.lastMaxDuration = lastDuration;

	if ((0 == g_fps_info.lastMinDuration) || (lastDuration < g_fps_info.lastMinDuration))
		g_fps_info.lastMinDuration = lastDuration;

	/* check if reach statistics interval, print result and reset for next */
	lastLogDuration = stampms - g_fps_info.lastLogTime;

	if (lastLogDuration > (unsigned long)g_fpsLogInterval) {
		/* update data for FPS result */
		/* fps = (float)g_fps_info.logFrameCount * 1000 / lastLogDuration; */
		fps_div = g_fps_info.logFrameCount * 1000 / lastLogDuration;
		fps_rem = g_fps_info.logFrameCount * 1000 % lastLogDuration;
		fps_rem = (fps_rem * 10000 + (lastLogDuration / 2)) / lastLogDuration;	/* round */

		g_fps_info.lastLogDuration = lastLogDuration;
		g_fps_info.lastLogTime = stampms;

		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_FDQ,
		       "kernel fps:%d.%04d,dur:%d,fcnt:%d,max:%d,min:%d\n", fps_div, fps_rem,
		       lastLogDuration, g_fps_info.logFrameCount, g_fps_info.lastMaxDuration,
		       g_fps_info.lastMinDuration);

		/* reset counting data for next */
		g_fps_info.logFrameCount = 0;
		g_fps_info.lastMaxDuration = 0;
		g_fps_info.lastMinDuration = 0;
	}

	return 1;
}
#endif /* G3DKMD_FPS_BY_FDQ */

void g3dKmdFdqReadQ(void)
{
	_DEFINE_BYPASS_;
	unsigned int wIdx, rIdx, size;
	struct FRM_RTN_DATA *pData;

	UNUSED(pData);

	_BYPASS_(BYPASS_RIU);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_FDQ, "Enter FDQ\n");

	/* before reading hw wptr, we need to set REG_AREG_MCU2G3D_SET to 1 to make sure the dram data are updated */

	lock_g3dFdq();
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x0, MSK_AREG_MCU2G3D_SET,
		       SFT_AREG_MCU2G3D_SET);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x1, MSK_AREG_MCU2G3D_SET,
		       SFT_AREG_MCU2G3D_SET);

	pData = (struct FRM_RTN_DATA *) gExecInfo.fdq.data;
	wIdx =
	    g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_HWWPTR, MSK_AREG_PA_FDQ_HWWPTR,
			  SFT_AREG_PA_FDQ_HWWPTR);
	rIdx = gExecInfo.fdq_rptr;
	size = gExecInfo.fdq_size;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_FDQ, "r %d, w %d, size %d\n", rIdx, wIdx, size);

	while (rIdx != wIdx) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_DDFR | G3DKMD_MSG_FDQ,
		       "[%dth] Qid %d, Fid %d, type %d\n",
		       rIdx, pData[rIdx].Qid, pData[rIdx].Fid, pData[rIdx].type);
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_FDQ,
		       "[Data_0] frame_swap %d, msaa_abort %d, checksum 0x%x\n",
		       pData[rIdx].data.type0_data0.frame_swap,
		       pData[rIdx].data.type0_data0.msaa_abort,
		       (pData[rIdx].type << 24) | pData[rIdx].data.type0_data0.checksum_low);

#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "FDQ-INST-ID", gExecInfo.taskList[pData[rIdx].Qid]->inst_idx);
		met_tag_oneshot(0x8163, "FDQ-INST-ID", 0);
#endif

#ifdef G3DKMD_FPS_BY_FDQ
		if (pData[rIdx].data.type0_data0.frame_swap)
			g3dKmdGetAverageFps();
#endif

		rIdx = ((rIdx + 1) >= size) ? 0 : rIdx + 1;
		YL_KMD_ASSERT(rIdx != wIdx);

		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_DDFR | G3DKMD_MSG_FDQ,
		       "[%dth] Qid %d, Fid %d, type %d\n",
		       rIdx, pData[rIdx].Qid, pData[rIdx].Fid, pData[rIdx].type);
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_DDFR | G3DKMD_MSG_FDQ,
		       "[Data_1] va 0x%x, bk 0x%x, tb 0x%x, hd 0x%x\n",
		       pData[rIdx].data.type0_data1.boff_va_ovf_cnt,
		       pData[rIdx].data.type0_data1.boff_bk_ovf_cnt,
		       pData[rIdx].data.type0_data1.boff_tb_ovf_cnt,
		       pData[rIdx].data.type0_data1.boff_hd_ovf_cnt);
#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
		/* Louis: resize if numbers of boff is too much. */
		if (pData[rIdx].data.type0_data1.boff_va_ovf_cnt > G3DKMD_BOFF_VA_THRLHLD ||
		    pData[rIdx].data.type0_data1.boff_bk_ovf_cnt > G3DKMD_BOFF_BK_THRLHLD ||
		    pData[rIdx].data.type0_data1.boff_tb_ovf_cnt > G3DKMD_BOFF_TB_THRLHLD ||
		    pData[rIdx].data.type0_data1.boff_hd_ovf_cnt > G3DKMD_BOFF_HD_THRLHLD) {
			g3dKmdTriggerCreateExeInst_workqueue(
				pData[rIdx].Qid,
				/* G3DKMD_FALSE,     // move single task group. */
				G3DKMD_TRUE,	/* move all tasks in same instance. */
				pData[rIdx].data.type0_data1.boff_va_ovf_cnt,
				pData[rIdx].data.type0_data1.boff_bk_ovf_cnt,
				pData[rIdx].data.type0_data1.boff_tb_ovf_cnt,
				pData[rIdx].data.type0_data1.boff_hd_ovf_cnt);
		}
#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */

#ifdef G3DKMD_SUPPORT_FENCE
		g3dKmdTaskUpdateTimeline(pData[rIdx].Qid, pData[rIdx].Fid);
#endif
		rIdx = ((rIdx + 1) >= size) ? 0 : rIdx + 1;
	}

	/* query queued HT slots*/
	#if defined(G3D_HW) && (!defined(ANDROID_PREF_NDEBUG) || defined(ANDROID_HT_COUNT))
	{
		struct g3dExeInst *pInst = gExecInfo.currentExecInst; /*g3dKmdTaskGetInstPtr(taskID);*/
		int queued_slots = g3dKmdGetQueuedHtSlots(pInst->hwChannel,
			pInst->pHtDataForHwRead->htInfo); /* Louis: don't know if this is for HwRead or SwWrite.*/

		/* HtData for HwRead and SwWrite could be different htInstance. */
		if (pInst->pHtDataForHwRead != pInst->pHtDataForSwWrite) {
			/* if pointing to differnt htInstance, add slots in HtForSwWrite. */
			int slots = g3dKmdGetQueuedHtSlots(pInst->hwChannel, pInst->pHtDataForSwWrite->htInfo);

			queued_slots = (queued_slots > 0 && slots > 0) ? queued_slots : queued_slots+slots;
		}

		event_trace_printk(0, "C|%d|%s|%d\n", 0, "HT", queued_slots);

		pInst = gExecInfo.currentExecInst; /* g3dKmdTaskGetInstPtr(taskID); */

		/* Louis: don't know if this is for HwRead or SwWrite. */
		queued_slots = g3dKmdGetQueuedHt2Slots(pInst->hwChannel, pInst->pHtDataForHwRead->htInfo);

		/* HtData for HwRead and SwWrite could be different htInstance. */
		if (pInst->pHtDataForHwRead != pInst->pHtDataForSwWrite) {
			/* if pointing to differnt htInstance, add slots in HtForSwWrite. */
			int slots = g3dKmdGetQueuedHt2Slots(pInst->hwChannel, pInst->pHtDataForSwWrite->htInfo);

			queued_slots = (queued_slots > 0 && slots > 0) ? queued_slots : queued_slots + slots;
		}

		event_trace_printk(0, "C|%d|%s|%d\n", 0, "HT2", queued_slots);
	}
	#endif
	gExecInfo.fdq_rptr = rIdx;
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_SWRPTR, rIdx, MSK_AREG_PA_FDQ_SWRPTR, SFT_AREG_PA_FDQ_SWRPTR);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x0, MSK_AREG_MCU2G3D_SET, SFT_AREG_MCU2G3D_SET);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x1, MSK_AREG_MCU2G3D_SET, SFT_AREG_MCU2G3D_SET);
	unlock_g3dFdq();

	_RESTORE_();
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_FDQ, "Leave FDQ\n");
}


#ifndef G3D_HW

void g3dKmdFdqWriteQ(int taskID)
{
	_DEFINE_BYPASS_;
	static unsigned int cnt;
	unsigned int wIdx, size;
	unsigned int rIdx;
	unsigned int *pBase;
	unsigned int stamp;
	struct g3dTask *task;

	UNUSED(rIdx);

	task = g3dKmdGetTask(taskID);
	if (task == NULL)
		return;

	_BYPASS_(BYPASS_RIU);

	stamp = task->lastFlushStamp;

	wIdx = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_HWWPTR, MSK_AREG_PA_FDQ_HWWPTR, SFT_AREG_PA_FDQ_HWWPTR);
	rIdx = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_SWRPTR, MSK_AREG_PA_FDQ_SWRPTR, SFT_AREG_PA_FDQ_SWRPTR);
	size = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FDQBUF_SIZE, MSK_AREG_PA_FDQBUF_SIZE, SFT_AREG_PA_FDQBUF_SIZE);
	pBase = gExecInfo.fdq.data;

	/* write type0/data0 (64bit) */
	pBase[wIdx * 2 + 1] = (taskID << 24) | ((stamp & 0xFFFF) << 8) | 0; /* QID[63:56] + FID[55:40] + type[39:32] */
	pBase[wIdx * 2 + 0] = cnt & 0x3;	/* msaa_abort[1:1] + frame_swap[0:0] */
	wIdx = ((wIdx + 1) >= size) ? 0 : wIdx + 1;

	/* write type0/data1 (64bit) */
	pBase[wIdx * 2 + 1] = (taskID << 24) | ((stamp & 0xFFFF) << 8) | 0; /* QID[63:56] + FID[55:40] + type[39:32] */
	pBase[wIdx * 2 + 0] = ((cnt & 0xFF) << 24) | ((cnt & 0xFF) << 16) | ((cnt & 0xFF) << 8) | ((cnt & 0xFF) << 0);
	/* boff_hd_ovf_cnt[31:24] + boff_tb_ovf_cnt[23:16] + boff_bk_ovf_cnt[15:8] + boff_va_ovf_cnt[7:0] */
	wIdx = ((wIdx + 1) >= size) ? 0 : wIdx + 1;
	YL_KMD_ASSERT(wIdx != rIdx);

	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_HWWPTR, wIdx, MSK_AREG_PA_FDQ_HWWPTR, SFT_AREG_PA_FDQ_HWWPTR);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASK, 0, VAL_IRQ_PA_DATA_QUEUE_NOT_EMPTY,
		       SFT_AREG_IRQ_MASK);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 0, VAL_IRQ_PA_DATA_QUEUE_NOT_EMPTY, SFT_AREG_IRQ_RAW);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, VAL_IRQ_PA_DATA_QUEUE_NOT_EMPTY,
		       VAL_IRQ_PA_DATA_QUEUE_NOT_EMPTY, SFT_AREG_IRQ_RAW);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASKED, VAL_IRQ_PA_DATA_QUEUE_NOT_EMPTY,
		       VAL_IRQ_PA_DATA_QUEUE_NOT_EMPTY, SFT_AREG_IRQ_RAW);

	gExecInfo.fdq_wptr = wIdx;
	cnt++;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_FDQ, "task %d(%d) r: %d, w: %d, s: %d\n",
		taskID, task->pid, rIdx, wIdx, size);

	_RESTORE_();

#ifdef G3DKMD_SUPPORT_ISR
	g3dKmdIsrTrigger();
#endif /* G3DKMD_SUPPORT_ISR */
}
#endif /* #ifndef G3D_HW */
#endif /* G3DKMD_SUPPORT_FDQ */
