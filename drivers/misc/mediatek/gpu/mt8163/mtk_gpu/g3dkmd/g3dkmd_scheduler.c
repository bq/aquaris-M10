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

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)
#include <windows.h>
#include <stdlib.h>
#else
#if defined(linux_user_mode)
#include <stdlib.h>
#else
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/slab.h>
#endif
#endif

#include "platform/g3dkmd_platform_workqueue.h"

#include "sapphire_reg.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_scheduler.h"
#include "g3dkmd_api.h"
#include "g3dkmd_task.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_util.h"
#include "g3dkmd_power.h"
#if defined(YL_MET) || defined(G3DKMD_MET_CTX_SWITCH)
#include "g3dkmd_met.h"
#endif

#ifdef USE_GCE
#include "g3dkmd_gce.h"
#endif

#ifdef G3DKMD_SUPPORT_FDQ
#include "g3dkmd_fdq.h"
#endif

/* #ifdef PATTERN_DUMP */
#if 1
#include "g3dkmd_pattern.h"
#include "g3dkmd_cfg.h"
#endif

#define CTX_DPF(type, ...) KMDDPF(type, __VA_ARGS__)

#if 0 /* for debug */
#define MIGRATION_TAG "[MGRTN]"
#define LDPF(fmt, ...) pr_debug(MIGRATION_TAG "[%s]" fmt, __func__,  __VA_ARGS__)
#ifdef G3DKMD_LLVL_NOTICE
#undef G3DKMD_LLVL_NOTICE
#define G3DKMD_LLVL_NOTICE G3DKMD_LLVL_CRIT
#endif
#else
#define LDPF(fmt, ...)
#endif

/* ------------------------------------------------------------------------ */
/* Description : external variable */
/* ------------------------------------------------------------------------ */


/* ------------------------------------------------------------------------ */
/* Description : Get the time period from last check */
/* Parameter : */
/* Return : time period from last check */
/* ------------------------------------------------------------------------ */
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
static unsigned int _g3dKmdGetTimeDiff(void)
{
#if defined(linux) && !defined(linux_user_mode)
	static struct timeval s_last_tv;
	unsigned int time_diff;
	struct timeval cur_tv;

	do_gettimeofday(&cur_tv);

	time_diff = (cur_tv.tv_sec - s_last_tv.tv_sec) * 1000 +
			(cur_tv.tv_usec - s_last_tv.tv_usec) / 1000;

	s_last_tv = cur_tv;

	return time_diff;
#else
	return G3DKMD_SCHEDULER_FREQ;
#endif
}

/* ------------------------------------------------------------------------ */
/* Description : Update time slice of the running task */
/* Parameter : */
/* Return : */
/* ------------------------------------------------------------------------ */
static void _g3dKmdUpdateTimeSlice(void)
{
	struct g3dExeInst *pInst = NULL;
	unsigned int time_diff;
	int i;
	/* CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"[%s]\n", __FUNCTION__); */
	time_diff = _g3dKmdGetTimeDiff();
	for (i = 0; i < gExecInfo.exeInfoCount; i++) {
		pInst = gExecInfo.exeList[i];
		if (pInst && pInst->state == G3DEXESTATE_RUNNING) {
			/* for two-queue mode, the urgent instance won't timeout */
			if (!(gExecInfo.exeuteMode == eG3dExecMode_two_queues && pInst->isUrgentPrio)) {
				if (pInst->time_slice > time_diff)
					pInst->time_slice -= time_diff;
				else
					pInst->time_slice = 0;
			}
		}
	}
}
#endif


#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION

/* ------------------------------------------------------------------------ */
/* Description : Active the inactive exeInst(s) which can join context switch */
/* and delete the exeInst(s) which are migrated out if its */
/* refCnt == 0. */
/* Parameter : N/A */
/* Return : N/A */
/* ------------------------------------------------------------------------ */
static void _g3dKmdCheckAndActiveMigratingInsts(void) /* must withint g3dlock */
{
	struct g3dExeInst *pInst, *pInstChild;
	int idx;

	if (_g3dKmdTestMigrationCountLessEq(0)) /* if no Migration now */
		return;

	/* find migrating parent Inst. */
	for (idx = 0; idx < gExecInfo.exeInfoCount; idx++) {
		pInst = gExecInfo.exeList[idx];

		if (NULL == pInst)
			continue;

		/* if 1. current inst is migratnig, */
		/* 2. parent is currently running, */
		/* 3. and parents' HtCmds is finished. */
		if (G3DKMD_IS_INST_MIGRATING_SOURCE(pInst) &&
		    g3dKmdIsHtInfoEmpty(pInst->hwChannel, pInst->pHtDataForHwRead->htInfo)) {
			LDPF("inst[%d] (%p)->childIdx: %d start checking migration\n",
			     idx, pInst, pInst->childInstIdx);

			if (pInst->childInstIdx > gExecInfo.exeInfoCount || pInst->childInstIdx < 0) {
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_DDFR,
				       "pInst(%p)->childInstIdx(%u) > (%u)gExecInfo.exeInfoCount\n",
				       pInst, pInst->childInstIdx, gExecInfo.exeInfoCount);
				goto cleanup_stage;
			}
			/* let childInst join context switch queue. */
			pInstChild = gExecInfo.exeList[pInst->childInstIdx];

			if (NULL == pInstChild) {
				/* YL_KMD_ASSERT(NULL != pInstChild); */
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_DDFR,
				       "pInstChild is NULL. pInst->childInstIdx(%d)\n",
				       pInst->childInstIdx);
				goto cleanup_stage;
			}


			KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_DDFR,
			       "childInstID: %d join schedule, by parentInstIdx: %d\n",
			       pInst->childInstIdx, idx);

			pInstChild->state = G3DEXESTATE_IDLE;

			LDPF("inst[%d] is joining schedule now.\n", pInst->childInstIdx);

			/* set parent Inst state to "migrated / Ready to clean up." */
			/* pInstChild->parentInstIdx = EXE_INST_NONE; */
			/* pInst->childInstIdx = EXE_INST_MIGRATED_WAIT_CLEANUP; */
			G3DKMD_INST_MIGRATION_DISASSOCIATE_AND_WAIT_CLEAN_UP(pInst, pInstChild);
		}

cleanup_stage:
		/* clean up, only the pInst is not running can update and release. */
		/*  */
		/* ToDo: Bug, when using single_queue setting, the running queue may */
		/* stock in hwHtInstance. Have No timming to change to swHtInstance */
		/* due to that we can ONLY change hwHt to swHt when the queue is not */
		/* running. ( May add qload to solve the problem.) */
		if (G3DKMD_IS_INST_MIGRATED_WAIT_CLEANUP_SOURCE(pInst) &&
		    pInst->state != G3DEXESTATE_RUNNING) {
			LDPF("Clean Up inst[%d] (%p) starting.\n", idx, pInst);

			if (G3DKMD_IS_EXEINST_HAS_NO_ASSOCIATED_TASK(pInst)) { /* no task use it. */
				if (pInst->state != G3DEXESTATE_RUNNING) {

					/* set pInst cleanuped to avoid re-entering */
					G3DKMD_INST_MIGRATION_SET_NORMAL(pInst);
					LDPF("request releasing Inst[%d] (%p)\n", idx, pInst);
					/* inactive parentInst and release it. */
					pInst->state = G3DEXESTATE_INACTIVED;

					/* use workqueue to release parentInst. */
					g3dKmdRequestExeInstRelease(idx);
					LDPF("request released Inst[%d] (%p)\n", idx, pInst);
				}
			} else { /* stil some task(s) use it */
				struct g3dHtInst *oldHtInst = pInst->pHtDataForHwRead;

				pInst->pHtDataForHwRead = pInst->pHtDataForSwWrite;
				G3DKMD_INST_MIGRATION_SET_NORMAL(pInst); /* set pInst cleanuped to avoid re-entering */

				/* clean up the back htInstance */
				g3dKmdReleaseHtInst(oldHtInst);

				/* update HtInfo to qBuffer. */
				/* g3dKmdUpdateHtInQbaseBuffer(pInst, G3DKMD_TRUE); */
				g3dKmdUpdateHtInQbaseBuffer(pInst, G3DKMD_FALSE);

				_g3dKmdDecreaseMigrationCount();	/* decrease migration cnt. */

				LDPF("inst[%d] (%p) finished migration\n", idx, pInst);
			}
		}
	}
}
#else
#define _g3dKmdCheckAndActiveMigratingInsts()
#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */


/* ------------------------------------------------------------------------ */
/* Description : Find the task which will be resumed */
/* Parameter : ppTaskOut: The task which will be resumed */
/* pTaskOutId: The instIdx of ppTaskOut */
/* Return : */
/* ------------------------------------------------------------------------ */
static struct g3dExeInst *_g3dKmdFindSwitchToInstance(unsigned debug_info_enable)
{
	struct g3dExeInst *pInstTo = NULL;
	struct g3dExeInst *pInst;
	/* unsigned int instIdx = 0; */
	int i, cur_inst, loop, start_inst, end_inst;

	/* CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"[%s] in\n", __FUNCTION__); */

	/* find current instance */
	for (cur_inst = 0; cur_inst < gExecInfo.exeInfoCount; cur_inst++) {
		pInst = gExecInfo.exeList[cur_inst];
		if (pInst && (pInst->state == G3DEXESTATE_RUNNING))
			break;
	}

	/*
	 * find swith-to instance (idle or high-priority or waitqueue,
	 * but idle & high-priority has higher priority than waitqueue)
	 */
	start_inst = cur_inst;
	end_inst = gExecInfo.exeInfoCount;

	for (loop = 0; loop < 2; loop++) {
		for (i = start_inst; i < end_inst; i++) {
			pInst = gExecInfo.exeList[i];
			if (pInst && pInst->state != G3DEXESTATE_INACTIVED) {
				unsigned char isDone = g3dKmdIsFiredHtInfoDone(pInst->hwChannel,
									      pInst->pHtDataForHwRead->htInfo);
				unsigned char isNew = (pInst->state == G3DEXESTATE_IDLE);
				unsigned char isPending = (pInst->state == G3DEXESTATE_WAITQUEUE);

				/* CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, */
				/* "[%s][%d] nonEmpty %d, isNew %d, isPending %d, " */
				/* "isUrgentPrio %d\n", __FUNCTION__, i, nonEmpty, */
				/* isNew, isPending, pInst->isUrgentPrio); */

				if (!isDone) {
					/* find new task, or high priority task */
					if (isNew || (isPending && pInst->isUrgentPrio)) {
						pInstTo = pInst;
						/* instIdx = i; */
						goto SWITCH_TO_OUT;
					}

					if ((pInstTo == NULL) && isPending) {
						pInstTo = pInst;
						/* instIdx = i; */
					}
				}
			}
		}

		start_inst = 0;
		end_inst = cur_inst;
	}

SWITCH_TO_OUT:
	CTX_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_SCHDLR, "[%s] out, instance 0x%x, instIdx %d\n",
		__func__, pInstTo, g3dKmdExeInstGetIdx(pInstTo));
	return pInstTo;
}

/* ------------------------------------------------------------------------ */
/* Description : Find the exeInst which will be suspended, criteria is => */
/* (1) exeInst is running (2) (time_slice == 0) or !checkTimeSlice */
/* Parameter : checkTimeSlice: when finding exeinst to, if we should check (time_slice == 0) or not */
/* Return : The exeInst which will be suspended */
/* ------------------------------------------------------------------------ */
static struct g3dExeInst *_g3dKmdFindSwitchFromInstance(unsigned char forceSwitch,
						 unsigned debug_info_enable)
{
	int i;
	unsigned char isDone, isTimeout;
	struct g3dExeInst *pInst;
	struct g3dExeInst *pInstFrom = NULL;
	struct g3dExeInst *pNormal = NULL, *pNormalTimeout = NULL, *pNormalEmpty = NULL;
	struct g3dExeInst *pUrgent = NULL, *pUrgentTimeout = NULL, *pUrgentEmpty = NULL;

	/* CTX_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_SCHDLR,"[%s] in\n", __FUNCTION__); */

	/* find running instance */
	for (i = 0; i < gExecInfo.exeInfoCount; i++) {
		pInst = gExecInfo.exeList[i];
		if (pInst && (pInst->state == G3DEXESTATE_RUNNING)) {
			isTimeout = (pInst->time_slice == 0);
			isDone = g3dKmdIsFiredHtInfoDone(pInst->hwChannel, pInst->pHtDataForHwRead->htInfo);

			CTX_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_SCHDLR,
				"[%s][%d] forceSwitch %d, isTimeout %d, isDone %d, isUrgentPrio %d\n",
				__func__, i, forceSwitch, isTimeout, isDone, pInst->isUrgentPrio);

			if (pInst->isUrgentPrio) {
				if (isTimeout)
					pUrgentTimeout = pInst;

				if (isDone)
					pUrgentEmpty = pInst;

				if (!pUrgent || (pInst->prioWeighting > pUrgent->prioWeighting))
					pUrgent = pInst;

			} else {
				if (isTimeout)
					pNormalTimeout = pInst;

				if (isDone)
					pNormalEmpty = pInst;

				if (!pNormal || (pInst->prioWeighting > pNormal->prioWeighting))
					pNormal = pInst;

			}
		}
	}

	pInstFrom = pNormalTimeout ?
			pNormalTimeout : pNormalEmpty ?
				 pNormalEmpty : pUrgentEmpty ?
					pUrgentEmpty : NULL;
	if (forceSwitch && !pInstFrom) {
		pInstFrom = pUrgentTimeout ?
			pUrgentTimeout : pNormal ?
				pNormal : NULL;
	}


	CTX_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_SCHDLR, "[%s] out, instFrom %d\n", __func__,
		g3dKmdExeInstGetIdx(pInstFrom));

	return pInstFrom;
}

#if 1				/* ndef USE_GCE */

#if 0
static INLINE void _g3dKmdDumpRsm(struct g3dExeInst *pInstTo)
{
	g3dKmdCacheFlushInvalidate();
	pr_debug("resume buffer data: 0x%x\n", ((unsigned char *)pInstTo->rsmBuffer.data)[2]);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "resume buffer data: 0x%x\n",
	       ((unsigned char *)pInstTo->rsmBuffer.data)[2]);
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"q buffer data 1st Byte: 0x%x\n",
		((unsigned char*)pInstTo->qbaseBuffer.data)[0]));
	*/
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"q buffer data 2nd Byte: 0x%x\n",
		((unsigned char*)pInstTo->qbaseBuffer.data)[1]));
	*/
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"q buffer data 3rd Byte: 0x%x\n",
		((unsigned char*)pInstTo->qbaseBuffer.data)[2]));
	*/
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"q buffer data 4th Byte: 0x%x\n",
		((unsigned char*)pInstTo->qbaseBuffer.data)[3]));
	*/
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "q buffer data 5th Byte: 0x%x\n",
	       ((unsigned char *)pInstTo->qbaseBuffer.data)[4]);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "q buffer data 6th Byte: 0x%x\n",
	       ((unsigned char *)pInstTo->qbaseBuffer.data)[5]);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "q buffer data 7th Byte: 0x%x\n",
	       ((unsigned char *)pInstTo->qbaseBuffer.data)[6]);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "q buffer data 8th Byte: 0x%x\n",
	       ((unsigned char *)pInstTo->qbaseBuffer.data)[7]);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "q buffer data 9th Byte: 0x%x\n",
	       ((unsigned char *)pInstTo->qbaseBuffer.data)[8]);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "q buffer data 10th Byte: 0x%x\n",
	       ((unsigned char *)pInstTo->qbaseBuffer.data)[9]);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "q buffer data 11th Byte: 0x%x\n",
	       ((unsigned char *)pInstTo->qbaseBuffer.data)[10]);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "q buffer data 12th Byte: 0x%x\n",
	       ((unsigned char *)pInstTo->qbaseBuffer.data)[11]);
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"q buffer data 13th Byte: 0x%x\n",
		((unsigned char*)pInstTo->qbaseBuffer.data)[12]));
	*/
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"q buffer data 14th Byte: 0x%x\n",
		((unsigned char*)pInstTo->qbaseBuffer.data)[13]));
	*/
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"q buffer data 15th Byte: 0x%x\n",
		((unsigned char*)pInstTo->qbaseBuffer.data)[14]));
	*/
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"q buffer data 16th Byte: 0x%x\n",
		((unsigned char*)pInstTo->qbaseBuffer.data)[15]));
	*/
}
#endif

#if !defined(G3D_HW) && defined(G3DKMD_SUPPORT_FDQ)
static INLINE void _g3dKmdTriggerFDQ(unsigned int inst_idx)
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
#else
#define _g3dKmdTriggerFDQ(inst_idx)
#endif

/* ------------------------------------------------------------------------ */
/* Description : Real implementation of context switch when MMCE not enabled, it will directly set areg. */
/* Parameter : pTaskFrom: The task which will be suspended */
/* pTaskTo: The task which will be resume */
/* freeHwCh: Which G3D HW channel to use */
/* taskID: taskID of pTaskTo */
/* Return : */
/* ------------------------------------------------------------------------ */
static void _g3dKmdDoContextSwitch(struct g3dExeInst *pInstFrom, struct g3dExeInst *pInstTo, int freeHwCh,
				   unsigned debug_info_enable)
{
#ifndef SUPPORT_MPD
	_DEFINE_BYPASS_;
#endif
	unsigned int qload_ok = G3DKMD_TRUE;
	unsigned int instIdx;

	/* dump rsm buf to determine whether it is prim boundary or not */
	/* _g3dKmdDumpRsm(pInstTo); */

#ifndef SUPPORT_MPD
	_BYPASS_(BYPASS_RIU);
#endif
	if (KMDDPFCHECK(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_SCHDLR)) {
		SapphireQreg *pInfo;

		pInfo = (SapphireQreg *) pInstTo->qbaseBuffer.data;

		CTX_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_SCHDLR,
			"pInstTo(%p) qbuffer[rptr: 0x%x wptr: 0x%x range(0x%x~0x%x)\n",
			pInstTo,
			pInfo->reg_cq_htbuf_rptr.s.cq_htbuf_rptr,
			pInfo->reg_cq_htbuf_wptr.s.cq_htbuf_wptr,
			pInfo->reg_cq_htbuf_start_addr.s.cq_htbuf_start_addr,
			pInfo->reg_cq_htbuf_end_addr.s.cq_htbuf_end_addr);
	}

	qload_ok = g3dKmdQLoad(pInstTo);

#ifndef SUPPORT_MPD
	_RESTORE_();
#endif

	if (qload_ok) {
		instIdx = g3dKmdExeInstGetIdx(pInstTo);

		/* Clear status for InstFrom */
		if (pInstFrom) {
			CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "[%s] suspend inst %d\n",
				__func__, g3dKmdExeInstGetIdx(pInstFrom));
			pInstFrom->hwChannel = G3DKMD_HWCHANNEL_NONE;
			pInstFrom->time_slice = 0;
			pInstFrom->state = G3DEXESTATE_WAITQUEUE;
		}
		/* Set status for InstTo */
		pInstTo->hwChannel = freeHwCh;

		CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "[%s] resume inst %d\n", __func__,
			g3dKmdExeInstGetIdx(pInstTo));

		if (pInstTo->state == G3DEXESTATE_IDLE)
			pInstTo->time_slice = G3DKMD_TIME_SLICE_NEW_THD;
		else {
			pInstTo->time_slice = G3DKMD_TIME_SLICE_BASE +
						pInstTo->prioWeighting * G3DKMD_TIME_SLICE_INC;
		}
		/*
		CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"TimeSlice %d, prio %d\n", pInstTo->time_slice,
			pInstTo->prioWeighting);
		*/

		pInstTo->state = G3DEXESTATE_RUNNING;
		gExecInfo.hwStatus[freeHwCh].inst_idx = instIdx;
		gExecInfo.hwStatus[freeHwCh].state = G3DKMD_HWSTATE_RUNNING;

#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "ACTIVE-INSTANCE", g3dKmdExeInstGetIdx(pInstTo));
#endif

		/* Send command for InstTo */
		g3dKmdRegSendCommand(freeHwCh, pInstTo);

		/* Trigger FDQ for previous command (only need when !G3D_HW) */
		_g3dKmdTriggerFDQ(instIdx);

		if (KMDDPFCHECK(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_SCHDLR)) {
			SapphireQreg *pInfo;

			pInfo = (SapphireQreg *) pInstTo->qbaseBuffer.data;

			CTX_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_SCHDLR,
				"pInstTo(%p) qbuffer[rptr: 0x%x wptr: 0x%x range(0x%x~0x%x) out\n",
				pInstTo,
				pInfo->reg_cq_htbuf_rptr.s.cq_htbuf_rptr,
				pInfo->reg_cq_htbuf_wptr.s.cq_htbuf_wptr,
				pInfo->reg_cq_htbuf_start_addr.s.cq_htbuf_start_addr,
				pInfo->reg_cq_htbuf_end_addr.s.cq_htbuf_end_addr);
		}


	} else {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_SCHDLR,
		       " qload fail! (pInstTo: %p)\n", pInstTo);
	}
}
#endif

/* ------------------------------------------------------------------------ */
/* Description : Find switch-from and switch-to tasks, then call DoContextSwitch() to perform context swith jobs */
/* Parameter : */
/* Return : */
/* ------------------------------------------------------------------------ */
void g3dKmdContextSwitch(unsigned debug_info_enable)
{
	struct g3dExeInst *pInstFrom = NULL, *pInstTo = NULL;
	int i, freeHwCh = -1;

	/* CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"[%s]\n", __FUNCTION__); */

	/* check if any inactive exeInst can be actived. */
	_g3dKmdCheckAndActiveMigratingInsts();


	/* find switch-to task */
	pInstTo = _g3dKmdFindSwitchToInstance(debug_info_enable);
	if (pInstTo == NULL) {
		CTX_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_SCHDLR, "No pInstTo\n");
		return;
	}
	/* find free hw channel */
	for (i = 0; i < NUM_HW_CMD_QUEUE; i++) {
		if (gExecInfo.hwStatus[i].state == G3DKMD_HWSTATE_IDLE) {
			freeHwCh = i;
			break;
		}
	}

	/* find switch-from task */
	if (freeHwCh == G3DKMD_HWCHANNEL_NONE) { /* no inst from */
		unsigned char forceSwitch = G3DKMD_FALSE;

		if ((pInstTo->state == G3DEXESTATE_IDLE) || pInstTo->isUrgentPrio)
			forceSwitch = G3DKMD_TRUE;


		pInstFrom = _g3dKmdFindSwitchFromInstance(forceSwitch, debug_info_enable);

		if (pInstFrom)
			freeHwCh = pInstFrom->hwChannel;

	} else {
		pInstFrom = NULL;
	}

	if ((freeHwCh == G3DKMD_HWCHANNEL_NONE) ||
		(gExecInfo.hwStatus[freeHwCh].state == G3DKMD_HWSTATE_HANG)) {/* no inst from */
		/* if (debug_info_enable != 0) */
		/* pr_debug("[ctx switch caller err] cant find hw channel\n"); */
		CTX_DPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_SCHDLR, "[%s] no Free hwChannel\n",
			__func__);
		return;
	}

	CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,
		"[%s] instFrom (%d, 0x%x), instTo (%d, 0x%x), freeHwCh %d\n", __func__,
		g3dKmdExeInstGetIdx(pInstFrom), pInstFrom, g3dKmdExeInstGetIdx(pInstTo), pInstTo,
		freeHwCh);

	/* do context switch */
#ifdef USE_GCE
	if (g3dKmdCfgGet(CFG_GCE_ENABLE)) {
		g3dKmdGceDoContextSwitch(pInstFrom, pInstTo, freeHwCh);
	} else
#endif
	{
		_g3dKmdDoContextSwitch(pInstFrom, pInstTo, freeHwCh, debug_info_enable);
	}
	/* CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR,"[%s] OUT\n", __FUNCTION__); */
}
#ifdef KERNEL_FENCE_DEFER_WAIT
void g3dKmdUpdateReadyPtr(void)
{
	struct g3dExeInst *pInst = NULL;
	int i, waitingIndex;
	unsigned int firePtr;

	for (i = 0; i < gExecInfo.exeInfoCount; i++) {

		pInst = gExecInfo.exeList[i];
		if (pInst) {
			waitingIndex = g3dkmd_fence_GetHTWaitingIndex(pInst);

			if (waitingIndex < 0)
				continue;

			firePtr = waitingIndex * HT_CMD_SIZE + pInst->pHtDataForHwRead->htInfo->htCommandBaseHw;

			if (pInst->pHtDataForHwRead->htInfo->readyPtr == firePtr)
				continue;
			/*
			 * KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_FENCE,
			 *	"[%s] pInst %d %p, wtPtr 0x%x, new readyPtr 0x%x, old readyPtr 0x%x, waitingIndex 0x%x,
			 *		CommandBaseHw 0x%x\n",
			 *	__func__, i, pInst, pInst->pHtDataForHwRead->htInfo->wtPtr, firePtr,
			 *	pInst->pHtDataForHwRead->htInfo->readyPtr, waitingIndex,
			 *	pInst->pHtDataForHwRead->htInfo->htCommandBaseHw);
			 */
			pInst->pHtDataForHwRead->htInfo->readyPtr = firePtr;
		}
	}
}

static void _g3dKmdProcessCmd(void)
{
	struct g3dExeInst *pInst = gExecInfo.currentExecInst;

	g3dKmdUpdateReadyPtr();

	if (pInst)
		g3dKmdRegSendCommand(pInst->hwChannel, pInst);
}
#else
#define _g3dKmdProcessCmd()
#endif

/* ------------------------------------------------------------------------ */
/* Description : Scheduler function, when G3DKMD_SCHEDULER_FREQ timeout,*/
/*		or manually trigger by g3dKmdTriggerScheduler, it will wake-up and do context switch jobs */
/* Parameter : */
/* Return : */
/* ------------------------------------------------------------------------ */
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
#ifdef linux
static int _g3dKmdScheduler(void *data)
#elif defined(_WIN32)
static DWORD WINAPI _g3dKmdScheduler(LPVOID pvParam)
#endif
{

#ifdef linux
	while (!kthread_should_stop()) {
#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_end(0x8163, "SCHEDULER-INV-PERIOD");
		met_tag_start(0x8163, "SCHEDULER-PERIOD");
		met_tag_oneshot(0x8163, "CTX-SWITCH", 1);
#endif

		lock_g3d();

#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "CTX-SWITCH", 2);
#endif

		_g3dKmdUpdateTimeSlice();

#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "CTX-SWITCH", 3);
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
		{
			bool _work_done;

			lock_g3dPwrState();
			if (gExecInfo.PwrState == G3DKMD_PWRSTATE_POWERON)
				_work_done = gpu_pending_work_done();
			else
				_work_done = true;
			unlock_g3dPwrState();
			if (_work_done == false) {
				if (gpu_power_state_freeze(0) == G3DKMD_PWRSTATE_POWERON) {
					_g3dKmdProcessCmd();
					g3dKmdContextSwitch(0);
					gpu_power_state_unfreeze(0);
				}
			}
		}
#else
		_g3dKmdProcessCmd();
		g3dKmdContextSwitch(0);
#endif

#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "CTX-SWITCH", 4);
#endif

		unlock_g3d();

#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "CTX-SWITCH", 0);
		met_tag_end(0x8163, "SCHEDULER-PERIOD");
		met_tag_start(0x8163, "SCHEDULER-INV-PERIOD");
#endif

		wait_event_interruptible_timeout(gExecInfo.s_SchedWq, gExecInfo.activateSch,
						 msecs_to_jiffies(G3DKMD_SCHEDULER_FREQ));
		gExecInfo.activateSch = 0;

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
		wait_event_interruptible(gExecInfo.s_SchedWq,
					 gExecInfo.PwrState == G3DKMD_PWRSTATE_POWERON);
#endif
	}
#elif defined(_WIN32)
	while (G3DKMD_TRUE) {
		lock_g3d();
		_g3dKmdUpdateTimeSlice();
		g3dKmdContextSwitch();
		unlock_g3d();
		WaitForSingleObject(gExecInfo.hActThd, G3DKMD_SCHEDULER_FREQ);
		if (WaitForSingleObject(gExecInfo.hStopThd, 0) == WAIT_OBJECT_0)
			break;

	}
#endif
	return 0;
}
#endif /* G3DKMD_SCHEDULER_OS_CTX_SWTICH */
/* ------------------------------------------------------------------------ */
/* Description : Create scheduler, when g3dkmd context is created */
/* Parameter : */
/* Return : */
/* ------------------------------------------------------------------------ */
void g3dKmdCreateScheduler(void)
{
	CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "[%s]\n", __func__);
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
#ifdef linux
	init_waitqueue_head(&gExecInfo.s_SchedWq);

	gExecInfo.pSchThd = kthread_create(_g3dKmdScheduler, NULL, "MMCE_SCHEDULER");
	if (IS_ERR(gExecInfo.pSchThd)) {
		gExecInfo.pSchThd = NULL;
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_SCHDLR, "kthread_create fail, err code %d\n",
		       PTR_ERR(gExecInfo.pSchThd));
		YL_KMD_ASSERT(0);
	} else {
		wake_up_process(gExecInfo.pSchThd);
	}
#elif defined(_WIN32)
	gExecInfo.pSchThd = CreateThread(NULL, 0, _g3dKmdScheduler, NULL, 0, NULL);
	if (gExecInfo.pSchThd == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_SCHDLR, "CreateThread fail\n");
		YL_KMD_ASSERT(0);
	}
	gExecInfo.hStopThd = CreateEvent(NULL, G3DKMD_FALSE, G3DKMD_FALSE, NULL);
	gExecInfo.hActThd = CreateEvent(NULL, G3DKMD_FALSE, G3DKMD_FALSE, NULL);
#else
#error "Unsupported OS"
#endif
#endif /* G3DKMD_SCHEDULER_OS_CTX_SWTICH */
}

/* ------------------------------------------------------------------------ */
/* Description : Release scheduler when g3dkmd context is destroyed */
/* Parameter : */
/* Return : */
/* ------------------------------------------------------------------------ */
void g3dKmdReleaseScheduler(void)
{
	CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "[%s]\n", __func__);
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
	if (gExecInfo.pSchThd) {
#ifdef linux
		kthread_stop(gExecInfo.pSchThd);
#elif defined(_WIN32)
		SetEvent(gExecInfo.hActThd);
		SetEvent(gExecInfo.hStopThd);

		/* wait till the thread terminates */
		/* we have to mark this line, because it might be infinite-loop when running in wine system */
		/* while (WaitForSingleObject(gExecInfo.pSchThd, 0) != WAIT_OBJECT_0); */

		CloseHandle(gExecInfo.hStopThd);
		CloseHandle(gExecInfo.hActThd);
		CloseHandle(gExecInfo.pSchThd);
#endif
	}
#endif /* G3DKMD_SCHEDULER_OS_CTX_SWTICH */
}

/* ------------------------------------------------------------------------ */
/* Description : Manually trigger scheduler, such that scheduler can select high-prio task to run */
/* Parameter : */
/* Return : */
/* ------------------------------------------------------------------------ */
void g3dKmdTriggerScheduler(void)
{
	CTX_DPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_SCHDLR, "[%s]\n", __func__);
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
#ifdef linux
	gExecInfo.activateSch = 1;
	wake_up_interruptible(&gExecInfo.s_SchedWq);
#elif defined(_WIN32)
	SetEvent(gExecInfo.hActThd);
#endif
#endif /* G3DKMD_SCHEDULER_OS_CTX_SWTICH */
}
