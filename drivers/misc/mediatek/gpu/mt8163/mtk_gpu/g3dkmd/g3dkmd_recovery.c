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

#ifdef G3DKMD_RECOVERY_SIGNAL
#include <signal.h>
#endif
#include <linux/time.h>
#include <linux/interrupt.h>

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#include "g3dkmd_recovery.h"
#include "g3dkmd_file_io.h"	/* to use GKMDDPF */
#include "g3dkmd_util.h"
#include "g3dkmd_api.h"
#include "g3dkmd_isr.h"
#ifdef G3DKMD_SUPPORT_PM
#include "gpad/gpad_kmif.h"
#endif

#define G3DKMD_RECOVERY_SHARE_CNT
#define G3DKMD_RECOVERY_WQ


#ifdef G3DKMD_RECOVERY_BY_TIMER

#define MAX_POLLING_THRESHOLD 5	/* 10 */

static struct timer_list recovery_timer;	/* Timer list. */
static unsigned int recovery_timer_interval = 500;
static unsigned int polling_count;	/* Counter. */

void g3dKmd_recovery_reset_counter(void)
{
	struct g3dExeInst *pInst;
	struct g3dHtInfo *ht;
	unsigned int i;

	for (i = 0; i < gExecInfo.exeInfoCount; i++) {

		pInst = gExecInfo.exeList[i];
		ht = pInst->pHtDataForHwRead->htInfo;
		if (pInst != NULL) {
			ht->recovery_polling_cnt = 0;
			ht->prev_rdPtr = ht->htCommandBaseHw;
		}
	}
	polling_count = 0;
}
#ifdef G3DKMD_RECOVERY_WQ
static void g3dKmd_recovery_workqueue(struct work_struct *work)
{
	struct g3dExeInst *pInst = gExecInfo.currentExecInst;

	if (!pInst) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "pInst is NULL\n");
		return;
	}

	lock_g3d();
	disable_irq(G3DHW_IRQ_ID);
	g3dKmdSuspendIsr();

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_RECOVERY, "[TIMER_RECOVERY] skip HT with rptr 0x%x\n",
		g3dKmdRegGetReadPtr(pInst->hwChannel));

	g3dKmdRegRestart(-1, G3DKMD_RECOVERY_SKIP);

	enable_irq(G3DHW_IRQ_ID);
	g3dKmdResumeIsr();
	unlock_g3d();
}

static DECLARE_WORK(mtk_gpu_recovery_work, g3dKmd_recovery_workqueue);
#endif /* G3DKMD_RECOVERY_WQ */

void g3dKmd_recovery_timer_enable(void)
{
	unsigned long expires;

	expires = jiffies + recovery_timer_interval * HZ / 1000;
	mod_timer(&recovery_timer, expires);
}

void g3dKmd_recovery_timer_disable(void)
{
	del_timer(&recovery_timer);
}
void g3dKmd_recovery_timer_uninit(void)
{
	del_timer_sync(&recovery_timer);
}

static void g3dKmd_recovery_timer_callback(unsigned long unUsed)
{
	struct g3dExeInst *pInst = gExecInfo.currentExecInst;

	if (pInst && pInst->pHtDataForHwRead->htInfo && (pInst->hwChannel != G3DKMD_HWCHANNEL_NONE)) {
		lock_hang_detect();

		if (gExecInfo.hwStatus[pInst->hwChannel].state != G3DKMD_HWSTATE_HANG) {
			g3dKmd_recovery_detect_hang(pInst);

#ifdef G3DKMD_RECOVERY_SHARE_CNT
			if (polling_count > MAX_POLLING_THRESHOLD) {
#else /* !G3DKMD_RECOVERY_SHARE_CNT */
			if (pInst->pHtDataForHwRead->htInfo->recovery_polling_cnt > MAX_POLLING_THRESHOLD) {
#endif /* G3DKMD_RECOVERY_SHARE_CNT */
				gExecInfo.hwStatus[pInst->hwChannel].state = G3DKMD_HWSTATE_HANG;

#ifdef G3DKMD_DUMP_HW_HANG_INFO
				g3dKmdQueryHWIdleStatus(G3DKMD_SW_DETECT_HANG);
#endif

				g3dKmd_recovery_reset_hw(pInst);
			}
		}

		unlock_hang_detect();
	}

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
	if (gpu_power_state_freeze(false) == G3DKMD_PWRSTATE_POWERON) {
#endif
		g3dKmd_recovery_timer_enable();
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
		gpu_power_state_unfreeze(false);
	}
#endif
}

void g3dKmd_recovery_timer_init(void)
{
	init_timer(&recovery_timer);
	recovery_timer.expires = jiffies + recovery_timer_interval * HZ / 1000;
	recovery_timer.function = g3dKmd_recovery_timer_callback;
	add_timer(&recovery_timer);
}

void g3dKmd_recovery_detect_hang(struct g3dExeInst *pInst)
{
	unsigned task_cnt = gExecInfo.taskInfoCount;
	unsigned int pa_idle0, pa_idle1, cq_wptr, cq_rptr, irq_raw;

	if (pInst == NULL || task_cnt == 0) {
		/*
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "pInst = %x, PID = %d, Task Count = %d, return\n",
			pInst, (int)pid, task_cnt);
		*/
		return;
	}

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
	gpu_set_power_on();
#endif


	pa_idle0 = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS0, 0xFFFFFFFF, 0);
	pa_idle1 = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS1, 0xFFFFFFFF, 0);
	irq_raw = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 0xFFFFFFFF, 0);
	cq_wptr = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_CQ_HTBUF_WPTR,
				MSK_AREG_CQ_HTBUF_WPTR, SFT_AREG_CQ_HTBUF_WPTR);
	cq_rptr = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_CQ_DONE_HTBUF_RPTR,
				MSK_AREG_CQ_DONE_HTBUF_RPTR, SFT_AREG_CQ_DONE_HTBUF_RPTR);
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
	gpu_may_power_off();
#endif

#if 0 /* def G3DKMD_SUPPORT_ISR */
	lock_isr(); /* to avoid resetting another thread that we don't wat */
#endif

	/* Check g3d engine idle with (rdPtr != wtPtr) || (prev_rdPtr != rdPtr) conditions */
	if ((cq_wptr == cq_rptr)
	    || (pInst->pHtDataForHwRead->htInfo->prev_rdPtr != pInst->pHtDataForHwRead->htInfo->rdPtr)) {
#ifdef G3DKMD_RECOVERY_SHARE_CNT
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_RECOVERY,
		       "is_idle 0x%x, prev_rdPtr 0x%x, cur_rdPtr 0x%x\n", pa_idle0 & 0x01,
		       pInst->pHtDataForHwRead->htInfo->prev_rdPtr, pInst->pHtDataForHwRead->htInfo->rdPtr);
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_RECOVERY,
		       "IDLE0 0x%x, IDLE1 0x%x, RAW 0x%x\n", pa_idle0, pa_idle1, irq_raw);
		polling_count = 0;

#else				/* ! G3DKMD_RECOVERY_SHA RE_CNT */

		unsigned iter_inst = 0;

		for (iter_inst = 0; iter_inst < gExecInfo.exeInfoCount; iter_inst++)
			gExecInfo.exeList[iter_inst]->pHtDataForHwRead->htInfo->recovery_polling_cnt = 0;

#endif /* G3DKMD_RECOVERY_SHARE_CNT */

		/* KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "HW alive\n"); */
	} else {
#ifdef G3DKMD_RECOVERY_SHARE_CNT
		++polling_count;
		KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_RECOVERY,
			"Recovery detection inst %d, cnt %d, cur_rdPtr 0x%x\n",
			g3dKmdExeInstGetIdx(pInst), polling_count, pInst->pHtDataForHwRead->htInfo->rdPtr);
#else /* ! G3DKMD_RECOVERY_SHARE_CNT */
		++pInst->htInfo->recovery_polling_cnt;
		KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_RECOVERY,
			"Recovery detection inst %d, cnt %d, cur_rdPtr 0x%x\n",
			g3dKmdExeInstGetIdx(pInst), pInst->htInfo->recovery_polling_cnt,
			pInst->pHtDataForHwRead->htInfo->rdPtr);

#endif /* G3DKMD_RECOVERY_SHARE_CNT */
	}

	pInst->pHtDataForHwRead->htInfo->prev_rdPtr = pInst->pHtDataForHwRead->htInfo->rdPtr;

#if 0 /* def G3DKMD_SUPPORT_ISR */
	unlock_isr();
#endif
}

void g3dKmd_recovery_reset_hw(struct g3dExeInst *pInst)
{
#ifndef G3DKMD_RECOVERY_WQ
	unsigned int iter_inst = 0;
#endif /* !G3DKMD_RECOVERY_WQ */

	if (pInst == NULL)
		return;

#ifdef G3DKMD_RECOVERY_WQ

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_RECOVERY, "Reset HW via workqueue\n");
	schedule_work(&mtk_gpu_recovery_work);

#else /* ! G3DKMD_RECOVERY_WQ */

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_RECOVERY, "Reset HW via function calls\n");
	unsigned rptr_bak = g3dKmdRegGetReadPtr(pInst->hwChannel);

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "Second Hang => Skip HT with read ptr 0x%x\n", rptr_bak);
	g3dKmdRegRestart(-1, G3DKMD_RECOVERY_SKIP);

#endif /* ! G3DKMD_RECOVERY_WQ */

#ifdef G3DKMD_RECOVERY_SIGNAL
	/* try to send a signal to user space, target is _g3dDeviceSignalHandlerForRecovery() */
	{
		pid_t pid = gExecInfo.current_tid;

		if (pid != 0) {
			struct siginfo info;
			struct task_struct *pTask;

			pTask = pid_task(find_vpid(pid), PIDTYPE_PID);

			if (pTask == NULL || pid == 0) {
				KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY,
				       "pTask is NULL or pid is zero\n");
				return;
			}

			KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY,
			       "Send recovery signal to proc %d\n", pid);
			info.si_signo = SIGUSR1;
			info.si_pid = pid;
			info.si_code = -1;	/* __SI_RT; */
			info.si_int = 0x1234;	/* user data */
			send_sig_info(SIGUSR1, &info, pTask);
		}
	}
#endif /* G3DKMD_RECOVERY_SIGNAL */
}

#endif /* G3DKMD_RECOVERY_BY_TIMER */

#ifdef G3D_SWRDBACK_RECOVERY
void g3dKmdRecoveryAddInfo(struct g3dExeInst *pInst, unsigned int ptr, unsigned int stamp)
{
	unsigned int widx =
	    (pInst->pHtDataForHwRead->htInfo->wtPtr -
	     pInst->pHtDataForHwRead->htInfo->htCommandBaseHw) / HT_CMD_SIZE;

#if 1				/* debug info */
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY,
	       "[STAMP INFO] insert to arr[%d].ptr with val 0x%x\n", widx, ptr);
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY,
	       "[STAMP INFO] insert to arr[%d].stamp with val 0x%x\n", widx, stamp);
#endif

	/* backup the necessary data such as stamp and ptr for recovery */
	pInst->recoveryDataArray.dataArray[widx].wPtr = ptr;
	pInst->recoveryDataArray.dataArray[widx].stamp = stamp;
}

void g3dKmdRecoveryUpdateSWReadBack(struct g3dExeInst *pInst, unsigned int rptr)
{
	const uint64_t maxStampValue = 0x100000000;
	const uint64_t maxDeltaValue = maxStampValue - (COMMAND_QUEUE_SIZE/(SIZE_OF_FREG/2)) - 1;
	uint64_t delta;
	int taskID;
	unsigned int pLastFlushPtr, pLastFlushStamp, wIdx;
	unsigned char *pCmd;
	struct g3dHtInfo *ht;
	g3dFrameSwData *pSwData;

	if ((pInst == NULL) || (pInst->hwChannel >= NUM_HW_CMD_QUEUE))
		return;

	ht = pInst->pHtDataForHwRead->htInfo;
	pCmd = (unsigned char *)ht->htCommand + /*sw base */
		(rptr - ht->htCommandBaseHw); /* hw offset */
	wIdx = (rptr - ht->htCommandBaseHw) / HT_CMD_SIZE;
	taskID = ((struct g3dHtCommand *)pCmd)->command; /* first 4B is the taskID */

	if (unlikely(g3dKmdGetTask(taskID) == NULL)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_RECOVERY, "taskID %d does not exist\n", taskID);
		return;
	}
	pLastFlushPtr = pInst->recoveryDataArray.dataArray[wIdx].wPtr;
	pLastFlushStamp = pInst->recoveryDataArray.dataArray[wIdx].stamp;

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY,
	       "write wPtr 0x%x back to sw read back mem\n", pLastFlushPtr);
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY,
	       "write stamp 0x%x back to sw read back mem\n", pLastFlushStamp);

	pSwData = (g3dFrameSwData *) ((unsigned char *)gExecInfo.SWRdbackMem.data + G3D_SWRDBACK_SLOT_SIZE * taskID);

	delta = (pLastFlushStamp >= pSwData->stamp) ? (uint64_t)(pLastFlushStamp - pSwData->stamp) :
			(uint64_t)((uint64_t)pLastFlushStamp + maxStampValue - (uint64_t)pSwData->stamp);

	if (delta < maxDeltaValue) {
		pSwData->readPtr = (uintptr_t) pLastFlushPtr;
		pSwData->stamp = pLastFlushStamp;
	}

#ifdef G3DKMD_SUPPORT_FENCE
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "taskID %d, stamp %d\n", taskID, pLastFlushStamp);

	g3dKmdTaskUpdateTimeline(taskID, pLastFlushStamp & 0xFFFF);
#endif

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "update recovery stamp done\n");
}
#endif /* G3D_SWRDBACK_RECOVERY */
