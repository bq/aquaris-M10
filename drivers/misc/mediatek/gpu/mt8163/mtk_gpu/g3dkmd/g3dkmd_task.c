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

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include <windows.h>
#include <stdlib.h>
#else
#if defined(linux_user_mode)
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#else
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
/* #include <linux/kthread.h> */
#include <linux/memory.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/slab.h>		/* kmalloc() */
#endif
#endif

#include "platform/g3dkmd_platform_workqueue.h"
#include "platform/g3dkmd_platform_sysinfo.h"

#include "sapphire_reg.h"
#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dkmd_task.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_api.h"
#include "g3dkmd_internal_memory.h"
#include "g3dbase_buffer.h"
#include "g3dkmd_buffer.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_util.h"
#include "g3dkmd_cfg.h"
#include "g3dkmd_isr.h"
#if defined(YL_MET) || defined(G3DKMD_MET_CTX_SWITCH)
#include "g3dkmd_met.h"
#endif

#ifdef PATTERN_DUMP
#include "g3dkmd_pattern.h"
#endif

#ifdef G3DKMD_SUPPORT_FENCE
#include "g3dkmd_fence.h"
#endif

#ifdef USE_GCE
#include "g3dkmd_gce.h"
#endif

#ifdef PATTERN_DUMP
#include "mmu/yl_mmu_kernel_alloc.h"
#endif


#define G3DKMD_PREFULL_THD  75



/* Check dependency for G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */
#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
#if !defined(G3DKMD_SCHEDULER_OS_CTX_SWTICH)
#error G3DKMD_ENABLE_TASK_EXEINST_MIGRATION require G3DKMD_SCHEDULER_OS_CTX_SWTICH
#endif

#if G3DKMD_EXEC_MODE != eG3dExecMode_two_queues
#error G3DKMD_ENABLE_TASK_EXEINST_MIGRATION ONLY support eG3dExecMode_two_queues now.
#endif
#endif

#if 0
#ifdef G3DKMD_LLVL_INFO
#undef G3DKMD_LLVL_INFO
#define G3DKMD_LLVL_INFO G3DKMD_LLVL_CRIT
#endif
#endif

static void _g3dKmdExeInstIncRef(unsigned int instIdx);
static void _g3dKmdExeInstDecRef(unsigned int instIdx);


void g3dKmdInitQbaseBuffer(struct g3dExeInst *pInst)
{
	SapphireQreg *pInfo;
	G3dMemory *va_buffer;
	G3dMemory *hd_buffer;
	G3dMemory *bk_buffer;
	G3dMemory *tb_buffer;
	unsigned int vasize;
	unsigned int hdsize;
	unsigned int bksize;
	unsigned int tbsize;
	unsigned int va_preboff_size, bk_preboff_size, tb_preboff_size;


	if (pInst == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "[%s] pInst == NULL\n", __func__);
		return;
	}

	va_buffer = &pInst->defer_vary;
	hd_buffer = &pInst->defer_binHd;
	bk_buffer = &pInst->defer_binBk;
	tb_buffer = &pInst->defer_binTb;
	vasize = va_buffer->size;
	hdsize = hd_buffer->size / 2;
	bksize = bk_buffer->size / 2;
	tbsize = tb_buffer->size / 2;


	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "g3dExeInst %p %p 0x%x\n", pInst,
	       pInst->qbaseBuffer.data, pInst->qbaseBuffer.hwAddr);

	pInfo = (SapphireQreg *) pInst->qbaseBuffer.data;
	if (pInfo == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "[%s] pInfo == NULL\n", __func__);
		return;
	}
	memset(pInfo, 0, sizeof(*pInfo));

	pInfo->reg_cq_rsm.dwValue = 0;
	pInfo->reg_cq_htbuf_rptr.s.cq_htbuf_rptr = pInst->pHtDataForHwRead->htInfo->wtPtr;
	pInfo->reg_cq_htbuf_wptr.s.cq_htbuf_wptr = pInst->pHtDataForHwRead->htInfo->wtPtr;
	pInfo->reg_cq_fm_head.s.cq_fm_head = 0;
	pInfo->reg_cq_htbuf_start_addr.s.cq_htbuf_start_addr = pInst->pHtDataForHwRead->htBuffer.hwAddr;
	pInfo->reg_cq_htbuf_end_addr.s.cq_htbuf_end_addr =
	    pInst->pHtDataForHwRead->htBuffer.hwAddr + pInst->pHtDataForHwRead->htBufferSize;
	pInfo->reg_cq_rsm_start_addr.s.cq_rsm_start_addr = pInst->rsmBuffer.hwAddr;
	pInfo->reg_cq_rsm_end_addr.s.cq_rsm_end_addr = pInst->rsmBuffer.hwAddr + pInst->rsmBufferSize;
	pInfo->reg_cq_rsmvpreg_start_addr.s.cq_rsmvpreg_start_addr = pInst->rsmVpBuffer.hwAddr;
	pInfo->reg_cq_rsmvpreg_end_addr.s.cq_rsmvpreg_end_addr = pInst->rsmVpBuffer.hwAddr + pInst->rsmVpBufferSize;
	pInfo->reg_vx_binhd_base0.s.vx_binhd_base0 = hd_buffer->hwAddr;
	pInfo->reg_vx_bintb_base0.s.vx_bintb_base0 = tb_buffer->hwAddr;
	pInfo->reg_vx_binbk_base0.s.vx_binbk_base0 = bk_buffer->hwAddr;
	pInfo->reg_vx_vary_base.s.vx_vary_base = va_buffer->hwAddr;
	pInfo->reg_vx_binhd_base1.s.vx_binhd_base1 = hd_buffer->hwAddr +
		((hdsize + G3DKMD_DEFER_BASE_ALIGN_MSK) & (~G3DKMD_DEFER_BASE_ALIGN_MSK)); /* 128B align */
	pInfo->reg_vx_bintb_base1.s.vx_bintb_base1 = tb_buffer->hwAddr +
		((tbsize + G3DKMD_DEFER_BASE_ALIGN_MSK) & (~G3DKMD_DEFER_BASE_ALIGN_MSK)); /* 128B align */
	pInfo->reg_vx_binbk_base1.s.vx_binbk_base1 = bk_buffer->hwAddr +
		((bksize + G3DKMD_DEFER_BASE_ALIGN_MSK) & (~G3DKMD_DEFER_BASE_ALIGN_MSK)); /* 128B align */
	pInfo->reg_vx_vary_msize.s.vx_vary_msize = vasize;
	pInfo->reg_vx_tb_msize.s.vx_tb_msize = tbsize;
	pInfo->reg_vx_bk_msize.s.vx_bk_msize = bksize;

	va_preboff_size = (vasize / 2 - VX_VW_BUF_MGN_SIZE * 1024);
	tb_preboff_size = tbsize;
	bk_preboff_size = bksize;
	pInfo->reg_vx_vary_preboff_msize.s.vx_vary_preboff_msize =
		((va_preboff_size * G3DKMD_PREFULL_THD) / 100) & (~0x3FF); /* multiple of 1KB */
	pInfo->reg_vx_tb_preboff_msize.s.vx_tb_preboff_msize =
		((tb_preboff_size * G3DKMD_PREFULL_THD) / 100) & (~0xF); /* multiple of 16B */
	pInfo->reg_vx_bk_preboff_msize.s.vx_bk_preboff_msize =
		((bk_preboff_size * G3DKMD_PREFULL_THD) / 100) & (~0x7F); /* multiple of 128B */
	pInfo->reg_vx_hd_preboff_num.s.vx_hd_preboff_num = 0xf000;

	pInst->dataSynced = G3DKMD_FALSE;

	/* criteria checking */
	YL_KMD_ASSERT((pInst->qbaseBufferSize != 0) && (pInst->qbaseBuffer.data != NULL));
	YL_KMD_ASSERT((pInfo->reg_vx_binhd_base0.s.vx_binhd_base0 & G3DKMD_DEFER_BASE_ALIGN_MSK) == 0);
	YL_KMD_ASSERT((pInfo->reg_vx_bintb_base0.s.vx_bintb_base0 & G3DKMD_DEFER_BASE_ALIGN_MSK) == 0);
	YL_KMD_ASSERT((pInfo->reg_vx_binbk_base0.s.vx_binbk_base0 & G3DKMD_DEFER_BASE_ALIGN_MSK) == 0);
	YL_KMD_ASSERT((pInfo->reg_vx_vary_base.s.vx_vary_base & G3DKMD_DEFER_BASE_ALIGN_MSK) == 0);
	YL_KMD_ASSERT((pInfo->reg_vx_binhd_base1.s.vx_binhd_base1 & G3DKMD_DEFER_BASE_ALIGN_MSK) == 0);
	YL_KMD_ASSERT((pInfo->reg_vx_bintb_base1.s.vx_bintb_base1 & G3DKMD_DEFER_BASE_ALIGN_MSK) == 0);
	YL_KMD_ASSERT((pInfo->reg_vx_binbk_base1.s.vx_binbk_base1 & G3DKMD_DEFER_BASE_ALIGN_MSK) == 0);
	YL_KMD_ASSERT((pInfo->reg_vx_vary_msize.s.vx_vary_msize & G3DKMD_VARY_SIZE_ALIGN_MSK) == 0);
	YL_KMD_ASSERT((pInfo->reg_vx_tb_msize.s.vx_tb_msize & G3DKMD_BINTB_SIZE_ALIGN_MSK) == 0);
	YL_KMD_ASSERT((pInfo->reg_vx_bk_msize.s.vx_bk_msize & G3DKMD_BINBK_SIZE_ALIGN_MSK) == 0);

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "g3dExeInst 0x%x out\n", pInst);
}



void g3dKmdUpdateHtInQbaseBuffer(struct g3dExeInst *pInst, unsigned char isUpdateHtRdPtr)
{
	SapphireQreg *pInfo;

	pInfo = (SapphireQreg *) pInst->qbaseBuffer.data;
	if (isUpdateHtRdPtr)
		pInfo->reg_cq_htbuf_rptr.s.cq_htbuf_rptr = pInst->pHtDataForHwRead->htInfo->rdPtr;

	pInfo->reg_cq_htbuf_wptr.s.cq_htbuf_wptr = pInst->pHtDataForHwRead->htInfo->wtPtr;
	pInfo->reg_cq_htbuf_start_addr.s.cq_htbuf_start_addr = pInst->pHtDataForHwRead->htBuffer.hwAddr;
	pInfo->reg_cq_htbuf_end_addr.s.cq_htbuf_end_addr =
	    pInst->pHtDataForHwRead->htBuffer.hwAddr + pInst->pHtDataForHwRead->htBufferSize;

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
	       "g3dExeInst %p InstIdx: %d, HW(wtPtr 0x%x, range: 0x%x~0x%x) SW(wtPtr 0x%x, range:0x%x~0x%x) htInst(sw,hw): (%p, %p)out\n",
	       pInst, g3dKmdExeInstGetIdx(pInst),
	       pInst->pHtDataForHwRead->htInfo->wtPtr,
	       pInst->pHtDataForHwRead->htBuffer.hwAddr,
	       pInst->pHtDataForHwRead->htBuffer.hwAddr +
	       pInst->pHtDataForHwRead->htBufferSize,
	       pInst->pHtDataForSwWrite->htInfo->wtPtr,
	       pInst->pHtDataForSwWrite->htBuffer.hwAddr,
	       pInst->pHtDataForSwWrite->htBuffer.hwAddr +
	       pInst->pHtDataForSwWrite->htBufferSize,
	       pInst->pHtDataForSwWrite,
	       pInst->pHtDataForHwRead);
}

/*
 the different between tgid, pid and ppid
 PID: Process Id
 PPID: Parent Process Id (the one which launched this PID)
 TGID: Thread Group Id
 >> a simpe diagram to describe the relationship between pid,tgid when fork or create a thread <<
		       USER VIEW
  <-- PID 43 --> <----------------- PID 42 ----------------->
		      +---------+
		      | process |
		     _| pid=42  |_
		   _/ | tgid=42 | \_ (new thread) _
	_ (fork) _/   +---------+                  \
       /                                        +---------+
 +---------+                                    | process |
 | process |                                    | pid=44  |
 | pid=43  |                                    | tgid=42 |
 | tgid=43 |                                    +---------+
 +---------+
  <-- PID 43 --> <--------- PID 42 --------> <--- PID 44 --->
		      KERNEL VIEW
*/

#if !defined(ANDROID) && !defined(WIN32) && defined(linux_user_mode)
static pid_t gettid(void)
{
	return (pid_t) syscall(SYS_gettid);
}
#endif

struct g3dTask *g3dKmdCreateTask(unsigned int bufferStart, unsigned int size, unsigned int flag)
{
	struct g3dTask *task = (struct g3dTask *) G3DKMDVMALLOC(sizeof(*task));

	if (task) {
		memset(task, 0, sizeof(*task));

#ifdef _WIN32
		task->pid = GetCurrentProcessId();
		task->tid = GetCurrentThreadId();
		task->PrioClass = (unsigned long)GetPriorityClass(GetCurrentProcess());
#elif defined(linux)
#if defined(linux_user_mode)
		task->pid = getpid();
		task->tid = gettid();
		task->prio = getpriority(PRIO_PROCESS, task->pid);
#else				/* ! defined(linux_user_mode) */
		task->pid = current->tgid;
		task->tid = current->pid;

		/* normally, it's 120, non-realtime process, if it's more smaller, priority is higher. */
		task->prio = current->prio;

#ifdef G3DKMD_RECOVERY_BY_TIMER
#ifdef G3DKMD_RECOVERY_SIGNAL
		gExecInfo.current_tid = current->pid;
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_TASK, "create task with pid = %d\n",
		       (unsigned)(gExecInfo.current_tid));
#endif /* G3DKMD_RECOVERY_SIGNAL */
#endif /* G3DKMD_RECOVERY_BY_TIMER */

#endif /* defined(linux_user_mode) */
#endif /* _WIN32 */

		task->frameCmd_base = bufferStart;
		task->frameCmd_size = size;
		task->lastFlushStamp = 0;
		task->carry = 0;
#if defined(linux) && !defined(linux_user_mode)
		task->task_comm[0] = 0;
#endif

		task->accFlushHTCmdCnt = 0;
		task->htCmdCnt = 0;
		task->inst_idx = EXE_INST_NONE;
		task->isUrgentPrio = (flag & G3DKMD_CTX_URGENT) ? G3DKMD_TRUE : G3DKMD_FALSE;
		task->isDVFSPerf = (flag & G3DKMD_CTX_DVFSPERF) ? G3DKMD_TRUE : G3DKMD_FALSE;
#ifdef G3D_SUPPORT_HW_NOFIRE
		task->isNoFire = (flag & G3DKMD_CTX_NOFIRE) ? G3DKMD_TRUE : G3DKMD_FALSE;
#endif

		task->hw_reset_cnt = 0;
		task->clPrintfInfo.isIsrTrigger = 0;
		task->clPrintfInfo.waveID = 0;

#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
		task->tryAllocMemFailCnt = 0;
#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */

#ifdef ANDROID
		task->isGrallocLockReq = G3DKMD_FALSE;
#endif
#if defined(ANDROID) && !defined(linux_user_mode) && defined(G3DKMD_SUPPORT_FENCE)
		task->timeline = NULL;
		task->lasttime = 0;	/* the value in the last time to update timeline */
#endif
	}

	return task;
}

void g3dKmdUnregisterAndDestroyTask(struct g3dTask *task, struct g3dExecuteInfo *execute, int taskID)
{
#ifdef G3DKMD_SUPPORT_ISR
	lock_isr();
#endif

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "taskID 0x%x\n", taskID);

	if (execute)
		execute->taskList[taskID] = 0;

	if (task) {
#ifdef G3DKMD_SUPPORT_FENCE
		if (task->timeline)
			g3dkmd_timeline_destroy(task->timeline);
#endif
	}
#ifdef G3DKMD_SUPPORT_ISR
	unlock_isr();
#endif

	if (task) {
		G3DKMDVFREE(task);
		task = NULL;
	}
}

/*
    give G3dTask pointer and return the register index in struct G3dExecuteInfo
*/
int g3dKmdRegisterNewTask(struct g3dExecuteInfo *execute, struct g3dTask *task)
{
	struct g3dTask **newTaskList = NULL;
	struct g3dTask **oldTaskList = NULL;
	int i = 0;

	if ((task == NULL) || (execute == NULL))
		return -1;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "\n");

	for (i = 0; i < execute->taskInfoCount; i++) {
		/* if task slots have been allocated, find an empty one to use */
		if (execute->taskList[i] == NULL) {
#ifdef G3DKMD_SUPPORT_ISR
			lock_isr();
#endif
			execute->taskList[i] = task;
			task->task_idx = i;
#ifdef G3DKMD_SUPPORT_ISR
			unlock_isr();
#endif
			return i;
		}
	}

	if (execute->taskAllocCount >= G3DKMD_MAX_TASK_CNT) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "already reach the Max task count 255\n");
		return -1;
	}
	/* allocate task array, increase 10 slots per time */
	if (execute->taskAllocCount <= execute->taskInfoCount) {
		execute->taskAllocCount +=
		    ((execute->taskAllocCount + 10 >= G3DKMD_MAX_TASK_CNT) ?
			(G3DKMD_MAX_TASK_CNT - execute->taskAllocCount) : 10);
		newTaskList = (struct g3dTask **) G3DKMDVMALLOC(sizeof(struct g3dTask *) * execute->taskAllocCount);
		if (newTaskList == NULL) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
			       "G3DKMDVMALLOC fail for taskList, taskAllocCount 0x%x\n", execute->taskAllocCount);
			return -1;
		}

		memset(&newTaskList[execute->taskInfoCount], 0, sizeof(struct g3dTask *) * 10);
		if (execute->taskInfoCount) {
			memcpy(newTaskList, execute->taskList, sizeof(struct g3dTask *) * execute->taskInfoCount);
			oldTaskList = execute->taskList;
		}
	}
#ifdef G3DKMD_SUPPORT_ISR
	lock_isr();
#endif
	if (newTaskList)
		execute->taskList = newTaskList;

	task->task_idx = execute->taskInfoCount;
	execute->taskList[execute->taskInfoCount] = task;
	execute->taskInfoCount++;
#ifdef G3DKMD_SUPPORT_ISR
	unlock_isr();
#endif

	if (oldTaskList)
		G3DKMDVFREE(oldTaskList);

	return execute->taskInfoCount - 1;
}


void g3dKmdInitTaskList(void)
{
	/* do nothing. Because everything will prepared when first time to call create task. */
}

void g3dKmdDestroyTaskList(void)
{
	int taskIdx;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "\n");

	for (taskIdx = 0; taskIdx < gExecInfo.taskAllocCount; taskIdx++)
		g3dKmdUnregisterAndDestroyTask(g3dKmdGetTask(taskIdx), &gExecInfo, taskIdx);


	G3DKMDVFREE(gExecInfo.taskList);
	memset(&gExecInfo, 0, sizeof(struct g3dExecuteInfo));
}



/* Execution Instance */

static unsigned gExeInstNormal = EXE_INST_NORMAL;
static unsigned gExeInstUrgent = EXE_INST_URGENT;


unsigned g3dKmdGetNormalExeInstIdx(void)
{
	return gExeInstNormal;
}

unsigned g3dKmdGetUrgentExeInstIdx(void)
{
	return gExeInstUrgent;
}

#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
static void _g3dKmdSetNormalExeInstIdx(const unsigned int instIdx)
{
	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK,
	       "set instIdx(%u) as Normal exeinst\n", __func__, instIdx);
	gExeInstNormal = instIdx;
}

static void _g3dKmdSetUrgentExeInstIdx(const unsigned int instIdx)
{
	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK,
	       "set instIdx(%u) as Urgent exeinst\n", __func__, instIdx);
	gExeInstUrgent = instIdx;
}
#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */


static struct g3dHtInfo *_g3dKmdCreateHtInfo(void)
{
	struct g3dHtInfo *htInfo = (struct g3dHtInfo *) G3DKMDVMALLOC(sizeof(struct g3dHtInfo));

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "\n");

	if (htInfo)
		memset(htInfo, 0, sizeof(struct g3dHtInfo));

	return htInfo;

}

static void _g3dKmdDestroyHtInfo(struct g3dHtInfo *ht)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "\n");

	if (ht)
		G3DKMDVFREE(ht);
}

int g3dKmdGetQueuedHt2Slots(int hwIndex, struct g3dHtInfo *ht)
{
	unsigned int queued, rdPtr;

	if (ht == NULL || hwIndex == -1)
		return -1;

	rdPtr = g3dKmdRegGetReadPtr(hwIndex);

	if (rdPtr == 0) /* wait HW fix it */
		rdPtr = ht->htCommandBaseHw;

	if (rdPtr <= ht->readyPtr)
		queued = ht->readyPtr - rdPtr;
	else
		queued = HT_BUF_SIZE - (rdPtr - ht->readyPtr);

	return queued/HT_CMD_SIZE;
}
int g3dKmdGetQueuedHtSlots(int hwIndex, struct g3dHtInfo *ht)
{
	unsigned int queued, rdPtr;

	if (ht == NULL || hwIndex == -1)
		return -1;

	rdPtr = g3dKmdRegGetReadPtr(hwIndex);

	if (rdPtr == 0) /* wait HW fix it */
		rdPtr = ht->htCommandBaseHw;

	if (rdPtr <= ht->wtPtr)
		queued = ht->wtPtr - rdPtr;
	else
		queued = HT_BUF_SIZE - (rdPtr - ht->wtPtr);

	return queued / HT_CMD_SIZE;
}

int g3dKmdCheckFreeHtSlots(int hwIndex, struct g3dHtInfo *ht)
{
	unsigned int available = 0;

	if (ht == NULL)
		return 0;

	/* update HW read pointer: */
	if (hwIndex != G3DKMD_HWCHANNEL_NONE) {
		unsigned int rdPtr = g3dKmdRegGetReadPtr(hwIndex);

		if ((rdPtr >= ht->htCommandBaseHw) && (rdPtr < ht->htCommandBaseHw + HT_BUF_SIZE))
			ht->rdPtr = rdPtr;
#ifdef TEST_NON_POLLING_FLUSHCMD
		if (0 == ht->rdPtr)
			ht->rdPtr = ht->htCommandBaseHw;
#endif
	}

	if (ht->rdPtr > ht->wtPtr)
		available = ht->rdPtr - ht->wtPtr;
	else
		available = HT_BUF_SIZE - (ht->wtPtr - ht->rdPtr);


	/* a safe waterlevel to avoid wtPtr catching up with rdPtr then we can't know it's overflow or underflow */
	if (available <= HT_BUF_LOW_THD)
		available = 0;
	return available;
}

#if defined(ENABLE_NON_POLLING_FLUSHCMD)
int g3dKmdCheckIfEnoughHtSlots(int hwIndex, struct g3dHtInfo *ht)
{
	return !((unsigned)(g3dKmdCheckFreeHtSlots(hwIndex, ht)) < HT_BUF_LOW_THD);
}
#endif

unsigned char g3dKmdIsHtInfoEmpty(int hwIndex, struct g3dHtInfo *ht)
{
	return g3dKmdCheckFreeHtSlots(hwIndex, ht) == MAX_HT_COMMAND * HT_CMD_SIZE;
}

#ifdef KERNEL_FENCE_DEFER_WAIT
unsigned char g3dKmdIsFiredHtInfoDone(int hwIndex, struct g3dHtInfo *ht)
{
	if (ht == NULL)
		return G3DKMD_TRUE;

	/* update HW read pointer: */
	if (hwIndex != G3DKMD_HWCHANNEL_NONE) {
		unsigned int rdPtr = g3dKmdRegGetReadPtr(hwIndex);

		if ((rdPtr >= ht->htCommandBaseHw) && (rdPtr < ht->htCommandBaseHw + HT_BUF_SIZE))
			ht->rdPtr = rdPtr;
	}

	if (ht->rdPtr == ht->readyPtr)
		return G3DKMD_TRUE;


	return G3DKMD_FALSE;
}
#else
unsigned char g3dKmdIsFiredHtInfoDone(int hwIndex, struct g3dHtInfo *ht)
{
	return g3dKmdIsHtInfoEmpty(hwIndex, ht);
}
#endif
void g3dKmdCopyCmd(struct g3dHtInfo *ht, struct g3dHtCommand *pSrcCmd)
{
	unsigned int wtIndex = (ht->wtPtr - ht->htCommandBaseHw) / HT_CMD_SIZE;
	struct g3dHtCommand *pCmd = &ht->htCommand[wtIndex];

	memcpy(pCmd, pSrcCmd, sizeof(struct g3dHtCommand));

	ht->wtPtr += HT_CMD_SIZE;
	if (ht->wtPtr >= ht->htCommandBaseHw + HT_BUF_SIZE)
		ht->wtPtr = ht->htCommandBaseHw;
}


void g3dKmdAddHTCmd(struct g3dExeInst *pInst, void *head, void *tail, unsigned int task_id, int fence)
{
	struct g3dHtInfo *ht = pInst->pHtDataForSwWrite->htInfo;
	unsigned int wtIndex = (ht->wtPtr - ht->htCommandBaseHw) / HT_CMD_SIZE;
	struct g3dHtCommand *pHtCmd = &ht->htCommand[wtIndex];

#ifdef KERNEL_FENCE_DEFER_WAIT
	g3dkmd_fence_ClearHTSignaledStatus(pInst, wtIndex);
#endif
	pHtCmd->command = task_id;	/* QID */
	pHtCmd->val0 = 0;
	pHtCmd->val1 = (uintptr_t) head;
	pHtCmd->val2 = (uintptr_t) tail;

	ht->wtPtr += HT_CMD_SIZE;
	if (ht->wtPtr >= ht->htCommandBaseHw + HT_BUF_SIZE)
		ht->wtPtr = ht->htCommandBaseHw;

	pInst->dataSynced = G3DKMD_FALSE;

#ifdef KERNEL_FENCE_DEFER_WAIT
	g3dkmd_fence_wait_async(fence, pInst, wtIndex);
#endif

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "head %p, tail %p, task_id 0x%x, wtPtr 0x%x\n",
	       head, tail, task_id, ht->wtPtr);

	/* Debug information */
	if (KMDDPFCHECK(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK)) {
		struct g3dTask *task = g3dKmdGetTask(task_id);

		if (NULL != task) {
			KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
			       "pInst: %p InstIdx:%u state(%d), lastStamp: %u pHt(sw/hw):(%p, %p) head %p, tail %p, task_id 0x%x, wtPtr 0x%x rdPtr: 0x%x range(0x%x ~ 0x%x) pHtCmd: %p\n",
			       pInst, g3dKmdExeInstGetIdx(pInst),
			       pInst->state, task->lastFlushStamp,
			       pInst->pHtDataForSwWrite, pInst->pHtDataForHwRead,
			       head, tail, task_id, ht->wtPtr, ht->rdPtr,
			       pInst->pHtDataForSwWrite->htBuffer.hwAddr,
			       pInst->pHtDataForSwWrite->htBuffer.hwAddr + pInst->pHtDataForSwWrite->htBufferSize,
			       pHtCmd);
		}
	}
}

/*
int g3dKmdCaculateIndex(struct g3dHtInfo *ht, unsigned int wptr, int index)
{
	int ret = index;
	uintptr_t start = (uintptr_t) ht->htCommand;
	unsigned int end   = start + sizeof(struct g3dHtCommand)*MAX_HT_COMMAND;
	if(start==0)
		return ret;
	if( (wptr <= end) && (wptr >= start) ) {
		ret = (wptr - start)/sizeof(struct g3dHtCommand);
		if(ret >= MAX_HT_COMMAND)
			ret = 0;
		}
	}
	return ret;
}
*/
struct g3dTask *g3dKmdGetTask(int taskID)
{
	struct g3dTask *task;

	if (taskID < 0 || taskID >= gExecInfo.taskInfoCount)
		return NULL;

	task = gExecInfo.taskList[taskID];

	return task;
}

#ifdef CMODEL
const void *g3dKmdGetQreg(unsigned int taskID)
{
	struct g3dExeInst *pInst = g3dKmdTaskGetInstPtr(taskID);

	if (!pInst)
		return NULL;

	return (const void *)pInst->qbaseBuffer.data;
}
#endif

void g3dKmdTaskAssociateInstIdx(struct g3dTask *task, const unsigned int instIdx)
{
	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK, "(%p, %p)\n", task, instIdx);

	YL_KMD_ASSERT(task->inst_idx == EXE_INST_NONE);	/* ths task must not belong to */
							/* any other exeInstance, or */
							/* the refCnt of previous instance */
							/* will not be decreased. */
	task->inst_idx = instIdx;
	_g3dKmdExeInstIncRef(instIdx);
}

void g3dKmdTaskDissociateInstIdx(struct g3dTask *task)
{
	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK, "(%p)\n", task);

	if (task && task->inst_idx != EXE_INST_NONE) {
		_g3dKmdExeInstDecRef(task->inst_idx);
		task->inst_idx = EXE_INST_NONE;
	}
}

struct g3dExeInst *g3dKmdTaskGetInstPtr(unsigned int taskID)
{
	unsigned inst_idx;

	if (gExecInfo.taskList == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "gExecInfo.taskList == NULL\n");
		inst_idx = g3dKmdGetNormalExeInstIdx();
	} else if (unlikely(taskID >= gExecInfo.taskInfoCount)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "taskID %u >= %u gExecInfo.taskInfoCount\n",
		       taskID, gExecInfo.taskInfoCount);
		goto fail;
	} else {
		struct g3dTask *task;

		task = gExecInfo.taskList[taskID];
		if (unlikely(task == NULL)) {
			KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_TASK,
			       "taskList[taskID(%u)] == NULL\n", taskID);
			goto fail;
		}

		inst_idx = task->inst_idx;
	}

	if (gExecInfo.exeList == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "gExecInfo.exeList == NULL\n");
		goto fail;
	} else if (unlikely(((int)inst_idx >= gExecInfo.exeInfoCount))) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "inst_idx (%u) >= (%u) gExecInfo.exeInfoCount",
		       inst_idx, gExecInfo.exeInfoCount);
		/* YL_KMD_ASSERT((int)task->inst_idx < gExecInfo.exeInfoCount); */
		goto fail;
	}

	return gExecInfo.exeList[inst_idx];

fail:
	return NULL;
}

int g3dKmdTaskGetInstIdx(unsigned int taskID)
{
	unsigned inst_idx;

	if (unlikely(gExecInfo.taskList == NULL)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "gExecInfo.taskList == NULL\n");
		inst_idx = g3dKmdGetNormalExeInstIdx();
	} else if (unlikely(taskID >= gExecInfo.taskInfoCount)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "taskID %u >= %u gExecInfo.taskInfoCount\n",
		       taskID, gExecInfo.taskInfoCount);
		goto fail;
	} else {
		struct g3dTask *task;

		task = gExecInfo.taskList[taskID];
		if (unlikely(task == NULL)) {
			KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_TASK,
			       "taskList[taskID(%u)] == NULL\n", taskID);
			goto fail;
		} else if (unlikely(((int)task->inst_idx >= gExecInfo.exeInfoCount))) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
			       "task->inst_idx (%u) >= (%u) gExecInfo.exeInfoCount",
			       task->inst_idx, gExecInfo.exeInfoCount);
			/* YL_KMD_ASSERT((int)task->inst_idx < gExecInfo.exeInfoCount); */
			goto fail;
		}

		inst_idx = task->inst_idx;
	}

	return inst_idx;

fail:
	inst_idx = EXE_INST_NONE;
	g3dkmd_backtrace();
	return inst_idx;
}

void g3dKmdTaskUpdateInstIdx(struct g3dTask *task)
{
	/* No Update when the Inst of task is migrating. */
#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
	struct g3dExeInst *pInst = g3dKmdExeInstGetInst(task->inst_idx);
#endif

	if ((gExecInfo.exeuteMode == eG3dExecMode_two_queues) &&
		(task->inst_idx == g3dKmdGetUrgentExeInstIdx()) &&
		(!task->isUrgentPrio) &&
		(task->htCmdCnt > EXE_INST_URGENT_PRIO_THRESHOLD) &&
		(!G3DKMD_IS_INST_MIGRATING(pInst))) {
		KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK,
		       "Task(%d) move from inst(%d) -> inst(%d)",
		       task->task_idx, task->inst_idx, g3dKmdGetNormalExeInstIdx());

		_g3dKmdExeInstDecRef(task->inst_idx);
		task->inst_idx = g3dKmdGetNormalExeInstIdx();
		_g3dKmdExeInstIncRef(task->inst_idx);
	}
}

static void _g3dKmdFreeHtInst(struct G3dHtInst *pHtInst)
{
	if (!pHtInst)
		return;

	freePhysicalMemory(&pHtInst->htBuffer);

	if (pHtInst->htInfo)
		_g3dKmdDestroyHtInfo(pHtInst->htInfo);
}

/* Initialize Ht Instance */

static void _g3dKmdInitHtInst(struct G3dHtInst *pHtInst)
{
	pHtInst->isUsed = G3DKMD_FALSE;
	pHtInst->htInfo->htCommand = (struct g3dHtCommand *) pHtInst->htBuffer.data;
	pHtInst->htInfo->htCommandBaseHw = pHtInst->htBuffer.hwAddr;
	pHtInst->htInfo->wtPtr = pHtInst->htInfo->rdPtr = pHtInst->htInfo->htCommandBaseHw;
#ifdef KERNEL_FENCE_DEFER_WAIT
	pHtInst->htInfo->readyPtr = pHtInst->htInfo->wtPtr;
	pHtInst->htInfo->fenceUpdated = G3DKMD_FALSE;
	memset(pHtInst->htInfo->fenceSignaled, 0, sizeof(pHtInst->htInfo->fenceSignaled));
#endif
	pHtInst->htInfo->prev_rdPtr = 0;
	pHtInst->htInfo->recovery_polling_cnt = 0;
	pHtInst->htInfo->recovery_first_attempt = G3DKMD_TRUE;
}

/* Relase(free) Ht Instance */
static void _g3dKmdReleaseHtInst(struct G3dHtInst *pHtInst)
{
	if (!pHtInst) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_DDFR,
			"pHtInst is NULL.");
		g3dkmd_backtrace();
		return;
	}

	_g3dKmdInitHtInst(pHtInst);
	pHtInst->isUsed = G3DKMD_FALSE;

}

/* Create(initialized) Ht Instance */
static G3DKMD_BOOL _g3dKmdCreateHtInst(struct G3dHtInst *pHtInst, const int vG3DKMD_BUF_BASE_ALIGN,
				       const int vG3DKMD_BUF_SIZE_ALIGN)
{
	if (!pHtInst)
		return G3DKMD_FALSE;

	if (NULL == pHtInst->htInfo) {
		unsigned int flags;

		pHtInst->htBufferSize = G3DKMD_ALIGN(HT_BUF_SIZE, vG3DKMD_BUF_SIZE_ALIGN);
		pHtInst->htInfo = _g3dKmdCreateHtInfo();
		if (NULL == pHtInst->htInfo) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
			       "[%s] _g3dKmdCreateHtInfo fail\n", __func__);
			return G3DKMD_FALSE;
		}

		flags = G3DKMD_MMU_ENABLE | G3DKMD_MMU_SHARE;
#if defined(G3D_NONCACHE_BUFFERS)
		flags |= G3DKMD_MMU_DMA;
#endif
		if (!allocatePhysicalMemory(&pHtInst->htBuffer, pHtInst->htBufferSize, vG3DKMD_BUF_BASE_ALIGN, flags)) {

			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "[%s] allocatePhysicalMemory fail\n", __func__);
			_g3dKmdDestroyHtInfo(pHtInst->htInfo);
			return G3DKMD_FALSE;
		}

		_g3dKmdInitHtInst(pHtInst);

	}

	return G3DKMD_TRUE;
}


void g3dKmdReleaseHtInst(struct G3dHtInst *pHtInst)
{
	_g3dKmdReleaseHtInst(pHtInst);
}

static struct G3dHtInst *_getAllocFreeHtInstance(struct g3dExeInst *pInst)
{
	int idx;
	struct G3dHtInst *pRetHtInst = NULL;

	for (idx = 0; idx < G3DKMD_MAX_HT_INSTANCE; ++idx) {
		if (pInst->htInstance[idx].isUsed == G3DKMD_FALSE) {
			pRetHtInst = &(pInst->htInstance[idx]);
			pRetHtInst->isUsed = G3DKMD_TRUE;
			return pRetHtInst;
		}
	}

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_DDFR, "pInst(%p) No free HtInstance\n", pInst);

	for (idx = 0; idx < G3DKMD_MAX_HT_INSTANCE; ++idx) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_DDFR,
		       "pInst(%p) htInst[%d] = %p\n", pInst, idx, &(pInst->htInstance[idx]));
	}
	g3dkmd_backtrace();

	return NULL;
}


static struct g3dExeInst *_g3dKmdExeInstCreate(G3DKMD_DEV_REG_PARAM *param, const enum g3dExeState state_in)
{
#define G3DKMD_BUF_BASE_ALIGN   16
#define G3DKMD_BUF_SIZE_ALIGN   16
	int i;

	struct g3dExeInst *pInst;
	unsigned int flags;

	pInst = (struct g3dExeInst *) G3DKMDVMALLOC(sizeof(struct g3dExeInst));
	if (!pInst) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "G3DKMDVMALLOC fail\n");
		/* YL_KMD_ASSERT(0); */
		return NULL;
	}

	memset(pInst, 0, sizeof(*pInst));
	/* pInst->state        = G3DEXESTATE_IDLE;      // Leroy: Not necessary! */
	pInst->state = state_in;
	pInst->hwChannel = G3DKMD_HWCHANNEL_NONE;
	pInst->refCnt = 0;	/* Leroy: Not necessary! */
#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
	pInst->tryEnlargeFailCnt = 0;
#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */

#ifdef KERNEL_FENCE_DEFER_WAIT
	INIT_LIST_HEAD(&pInst->fenceWaiter_list_head);
#endif
	/* initialize & alloc htInstances */
	for (i = 0; i < G3DKMD_MAX_HT_INSTANCE; ++i) {
		pInst->htInstance[i].htInfo = 0;

		if (G3DKMD_FALSE == _g3dKmdCreateHtInst(&pInst->htInstance[i],
							G3DKMD_BUF_BASE_ALIGN,
							G3DKMD_BUF_SIZE_ALIGN)) {
			goto ERR_EXIT;
		}
	}
	/* get free htInstance */
	{
		struct G3dHtInst *pHtInst = _getAllocFreeHtInstance(pInst);

		pInst->pHtDataForHwRead = pInst->pHtDataForSwWrite = pHtInst;
	}
#ifdef G3DKMD_SNAPSHOT_PATTERN
	pInst->htCommandFlushedCount = 0;	/* Leroy: Not necessary! */
	pInst->htCommandFlushedCountOverflow = 0;	/* Leroy: Not necessary! */
#endif

#ifndef G3D_HW
	/* g3dKmdRegUpdateReadPtr(task->hwChannel, task->htInfo->rdPtr); */
#endif
	/* create defer buffers */
	if (!g3dKmdCreateBuffers(pInst, param)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "g3dKmdCreateBuffers fail\n");
		goto ERR_EXIT;
	}
	/* create rsm buffer */
	pInst->rsmBufferSize = G3DKMD_ALIGN(sizeof(SapphireRsm), G3DKMD_BUF_SIZE_ALIGN);

	flags = G3DKMD_MMU_ENABLE;

	if (!allocatePhysicalMemory(&pInst->rsmBuffer, pInst->rsmBufferSize, G3DKMD_BUF_BASE_ALIGN, flags)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "allocatePhysicalMemory fail\n");
		goto ERR_EXIT;
	}
	/* create rsm_vp buffer */
	pInst->rsmVpBufferSize = G3DKMD_ALIGN(SIZE_OF_VPREG, G3DKMD_BUF_SIZE_ALIGN);

	flags = G3DKMD_MMU_ENABLE;

	if (!allocatePhysicalMemory(&pInst->rsmVpBuffer, pInst->rsmVpBufferSize, G3DKMD_BUF_BASE_ALIGN, flags)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "allocatePhysicalMemory fail\n");
		goto ERR_EXIT;
	}
	/* create qbase buffer */

	pInst->qbaseBufferSize = G3DKMD_ALIGN(sizeof(SapphireQreg), G3DKMD_BUF_SIZE_ALIGN);

#ifdef YL_SECURE
	if (g3dKmdCfgGet(CFG_SECURE_ENABLE)) {
		flags = G3DKMD_MMU_SECURE | G3DKMD_MMU_ENABLE | G3DKMD_MMU_SHARE;
#if defined(G3D_NONCACHE_BUFFERS)
		flags |= G3DKMD_MMU_DMA;
#endif
		if (!allocatePhysicalMemory(&pInst->qbaseBuffer, pInst->qbaseBufferSize, 16, flags)) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "allocatePhysicalMemory fail\n");
			goto ERR_EXIT;
		}
	} else
#endif
	{
		flags = G3DKMD_MMU_ENABLE | G3DKMD_MMU_SHARE;
#if defined(G3D_NONCACHE_BUFFERS)
		flags |= G3DKMD_MMU_DMA;
#endif
		if (!allocatePhysicalMemory(&pInst->qbaseBuffer, pInst->qbaseBufferSize, 16, flags)) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "allocatePhysicalMemory fail\n");
			goto ERR_EXIT;
		}
	}

	g3dKmdInitQbaseBuffer(pInst);

#ifdef USE_GCE
	/* create mmce related component */
	if (!g3dKmdGceCreateMmcq(pInst)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "g3dKmdGceCreateMmcq fail\n");
		goto ERR_EXIT;
	}
#endif

	pInst->isUrgentPrio = 0;
	pInst->time_slice = 0;

#ifdef _WIN32
	switch ((unsigned long)GetPriorityClass(GetCurrentProcess())) {
	case REALTIME_PRIORITY_CLASS:
		pInst->prioWeighting = 90;
		break;
	case HIGH_PRIORITY_CLASS:
		pInst->prioWeighting = 80;
		break;
	case ABOVE_NORMAL_PRIORITY_CLASS:
		pInst->prioWeighting = 60;
		break;
	default:
	case NORMAL_PRIORITY_CLASS:
		pInst->prioWeighting = 40;
		break;
	case BELOW_NORMAL_PRIORITY_CLASS:
		pInst->prioWeighting = 20;
		break;
	case IDLE_PRIORITY_CLASS:
		pInst->prioWeighting = 10;
		break;
	}
#elif defined(linux)
#if defined(linux_user_mode)
	pInst->prioWeighting = 0;	/* we don't need in user mode */
#else
	/*
	 * range of current->prio is 0~139 (lower means higher priority),
	 * we just remap to 1~100 (higher has higher priority)
	 */
	pInst->prioWeighting = ((139 - current->prio) * 100 / 140) + 1;
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "[task] %d, [Priority] %d -> %d\n\n",
	       current->tgid, current->prio, pInst->prioWeighting);
#endif
#endif

	G3DKMD_INST_MIGRATION_INIT(pInst);

	return pInst;
ERR_EXIT:
	{ /* free Ht */

		for (i = 0; i < G3DKMD_MAX_HT_INSTANCE; ++i) {
			struct G3dHtInst *pHtInst = &(pInst->htInstance[i]);

			if (pHtInst)
				_g3dKmdFreeHtInst(pHtInst);
		}
		/*
		freePhysicalMemory(&pInst->htBuffer);
		if (pInst->htInfo)
			_g3dKmdDestroyHtInfo(pInst->htInfo);
		*/
	}


	g3dKmdDestroyBuffers(pInst);
	freePhysicalMemory(&pInst->rsmBuffer);
	freePhysicalMemory(&pInst->rsmVpBuffer);
	freePhysicalMemory(&pInst->qbaseBuffer);

	G3DKMDVFREE(pInst);

	return NULL;
}

static void _g3dKmdExeInstRelease(struct g3dExeInst *pInst)
{
	int i;

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK, "(pInst: %p)\n", pInst);

	if (!pInst)
		return;

	for (i = 0; i < G3DKMD_MAX_HT_INSTANCE; ++i) {
		struct G3dHtInst *pHtInst = &(pInst->htInstance[i]);

		if (pHtInst->htInfo) {
			_g3dKmdDestroyHtInfo(pHtInst->htInfo);
			freePhysicalMemory(&pHtInst->htBuffer);
		}

	}

	/* destroy defer buffers */
	g3dKmdDestroyBuffers(pInst);

	freePhysicalMemory(&pInst->rsmBuffer);
	freePhysicalMemory(&pInst->rsmVpBuffer);
	freePhysicalMemory(&pInst->qbaseBuffer);

#ifdef USE_GCE
	g3dKmdGceReleaseMmcq(pInst);
#endif

	if (pInst == gExecInfo.currentExecInst) {
		g3dKmdRegResetCmdQ();
		gExecInfo.currentExecInst = NULL;
	}

	G3DKMDVFREE(pInst);
}

static unsigned int _g3dKmdExeInstRegister(struct g3dExeInst *pInst)
{
	int i = 0;

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK, "(pInst: %p)\n", pInst);

	if (pInst == NULL)
		return EXE_INST_NONE;


	for (i = 0; i < gExecInfo.exeInfoCount; i++) {
		/* if task slots have been allocated, find an empty one to use */
		if (gExecInfo.exeList[i] == NULL) {
			gExecInfo.exeList[i] = pInst;

#ifdef G3DKMD_MET_CTX_SWITCH
			met_tag_oneshot(0x8163, "g3dKmdExeInstRegister", i);
#endif
			return i;
		}
	}

	/* allocate task array, increase 10 slots per time */
	if (gExecInfo.exeAllocCount <= gExecInfo.exeInfoCount) {
		struct g3dExeInst **newInst = NULL;

		gExecInfo.exeAllocCount += 10;
		newInst = (struct g3dExeInst **) G3DKMDVMALLOC(sizeof(struct g3dExeInst *) * gExecInfo.exeAllocCount);

		if (newInst == NULL) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "vmalloc for exeInfo fail\n");
			return EXE_INST_NONE;
		}

		memset(&newInst[gExecInfo.exeInfoCount], 0, sizeof(struct g3dExeInst *) * 10);
		if (gExecInfo.exeInfoCount) {
			memcpy(newInst, gExecInfo.exeList, sizeof(struct g3dExeInst *) * gExecInfo.exeInfoCount);
			G3DKMDVFREE(gExecInfo.exeList);
		}

		gExecInfo.exeList = newInst;
	}

	gExecInfo.exeList[gExecInfo.exeInfoCount] = pInst;
	gExecInfo.exeInfoCount++;

#ifdef G3DKMD_MET_CTX_SWITCH
	met_tag_oneshot(0x8163, "g3dKmdExeInstRegister", (gExecInfo.exeInfoCount - 1));
#endif

	return gExecInfo.exeInfoCount - 1;
}

static void _g3dKmdExeInstUnRegister(unsigned int instIdx)
{
	struct g3dExeInst *pInst;
	unsigned int hwCh;
	unsigned int actInstIdx;

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK, "(instIdx: %u)\n", instIdx);

	if (instIdx >= (unsigned int)gExecInfo.exeInfoCount) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "instIdx(%u) >= %d)gExecinfo.exeInfoCount\n",
		       instIdx, gExecInfo.exeInfoCount);
		return;
	}

	pInst = gExecInfo.exeList[instIdx];
	if (!pInst) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       " pInst(NULL) = gExecInfo.exeList[%u]", instIdx);
		return;
	}

	hwCh = pInst->hwChannel;
	actInstIdx = (hwCh != -1) ? gExecInfo.hwStatus[hwCh].inst_idx : EXE_INST_NONE;

	/* clean responding hwStatus task index */
	if (actInstIdx == instIdx) {
		gExecInfo.hwStatus[hwCh].inst_idx = EXE_INST_NONE;
		gExecInfo.hwStatus[hwCh].state = G3DKMD_HWSTATE_IDLE;
	}

	gExecInfo.exeList[instIdx] = 0;
}

void g3dKmdExeInstSetPriority(const unsigned int instIdx, const unsigned char isUrgent)
{
	struct g3dExeInst *pInst;

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK, "(instIdx: %u, %x)\n", instIdx, isUrgent);

	if (instIdx >= (unsigned int)gExecInfo.exeInfoCount) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "instIdx(%u) >= gExecInfo.exeInfoCount(%d)\n",
		       instIdx, gExecInfo.exeInfoCount);
		return;
	}

	pInst = gExecInfo.exeList[instIdx];
	if (!pInst) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "gExecInfo.exeList[%u] is NULL", gExecInfo.exeList[instIdx]);
		return;
	}

	pInst->isUrgentPrio = isUrgent;
}

static void _g3dKmdExeInstIncRef(unsigned int instIdx)
{
	struct g3dExeInst *pInst;

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK, "(instIdx: %u)\n", instIdx);

	if (instIdx >= (unsigned int)gExecInfo.exeInfoCount) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "instIdx(%u) >= gExecInfo.exeInfoCount(%d)\n",
		       instIdx, gExecInfo.exeInfoCount);
		return;
	}

	pInst = gExecInfo.exeList[instIdx];
	if (!pInst) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "gExecInfo.exeList[%u] is NULL", gExecInfo.exeList[instIdx]);
		return;
	}

	pInst->refCnt++;

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
	       " inst[%u]->ref: %u -> %u (%p)\n", instIdx, pInst->refCnt - 1, pInst->refCnt, pInst);
}

static void _g3dKmdExeInstDecRef(unsigned int instIdx)
{
	struct g3dExeInst *pInst;

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK, "(instIdx: %u)\n", instIdx);

	if (instIdx >= (unsigned int)gExecInfo.exeInfoCount) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "instIdx(%u) >= gExecInfo.exeInfoCount(%d)\n",
		       instIdx, gExecInfo.exeInfoCount);
		return;
	}

	pInst = gExecInfo.exeList[instIdx];
	if (!pInst || pInst->refCnt <= 0) {
		if (!pInst) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
			       "gExecInfo.exeList[%u] is NULL\n", instIdx);
			return;
		}
		if (pInst->refCnt <= 0) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
			       "gExecInfo.exeList[%u]->refCnt is %u\n", instIdx, pInst->refCnt);
		}
		return;
	}

	pInst->refCnt--;
	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
	       " inst[%u]->ref: %u -> %u (%p)\n", instIdx, pInst->refCnt + 1, pInst->refCnt, pInst);
}

unsigned int g3dKmdExeInstCreateWithState(G3DKMD_DEV_REG_PARAM *param, const enum g3dExeState state_in)
{
	unsigned int instIdx;
	struct g3dExeInst *pInst;

	pInst = _g3dKmdExeInstCreate(param, state_in);

	if (!pInst) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "_g3dKmdExeInstCreate fail\n");
		return EXE_INST_NONE;
	}

	instIdx = _g3dKmdExeInstRegister(pInst);

	if (EXE_INST_NONE == instIdx) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "Register Inst fail. %p\n", pInst);
		_g3dKmdExeInstRelease(pInst);
		return EXE_INST_NONE;
	}

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "instIdx %d\n", instIdx);
	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
	       "(param: %p, state: %d) instIdx: %d\n", param, (int)state_in, instIdx);

	return instIdx;
}

void g3dKmdExeInstRelease(unsigned int instIdx)
{
	struct g3dExeInst *pInst;

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK, "(instIdx: %u)\n", instIdx);

	if ((instIdx == EXE_INST_NONE) || instIdx >= (unsigned int)gExecInfo.exeInfoCount) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "instIdx(%u) >= gExecInfo.exeInfoCount(%d)\n",
		       instIdx, gExecInfo.exeInfoCount);
		return;
	}

	pInst = gExecInfo.exeList[instIdx];

	if (!pInst) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
			"Inst is NULL with idx: %d", instIdx);
	} else {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_TASK, "instIdx %d, refCnt %d\n", instIdx, pInst->refCnt);
	}


	if (!pInst || (pInst->refCnt > 0)) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_TASK,
		       "instIdx: %u, pInst(%p)->refCnt %u > 0\n",
		       instIdx, pInst, pInst ? pInst->refCnt : 0u);
		return;
	}

	_g3dKmdExeInstUnRegister(instIdx);
	_g3dKmdExeInstRelease(pInst);
	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK, "real release, instIdx %d\n", instIdx);
}

void g3dKmdExeInstInit(void)
{
	unsigned int i, size = 0;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "exeuteMode %d\n", gExecInfo.exeuteMode);

	if (gExecInfo.exeuteMode == eG3dExecMode_single_queue)
		size = 1;
	else if (gExecInfo.exeuteMode == eG3dExecMode_two_queues)
		size = 2;


	if (size) {
		for (i = 0; i < size; i++) {
			unsigned int instIdx = g3dKmdExeInstCreate(NULL);

			YL_KMD_ASSERT(instIdx != EXE_INST_NONE);

			g3dKmdExeInstSetPriority(instIdx, (i == EXE_INST_URGENT));

			/* add ref, such that these instances won't be freed when AP release */
			_g3dKmdExeInstIncRef(instIdx);
		}
	} else {
		gExecInfo.exeList = NULL;
	}
	gExecInfo.exeInfoCount = size;
	gExecInfo.currentExecInst = NULL;
}

void g3dKmdExeInstUninit(void)
{
	int i;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK, "\n");

	if (gExecInfo.exeList) {
		for (i = 0; i < gExecInfo.exeInfoCount; i++) {
			_g3dKmdExeInstDecRef(i);
			g3dKmdExeInstRelease(i);
		}

		G3DKMDVFREE(gExecInfo.exeList);
		gExecInfo.exeList = NULL;
		gExecInfo.exeInfoCount = 0;
	}
}

struct g3dExeInst *g3dKmdExeInstGetInst(unsigned int instIdx)
{
	struct g3dExeInst *pInst = NULL;

	if (instIdx >= (unsigned int)gExecInfo.exeInfoCount) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "instIdx(%u) >= gExecInfo.exeInfoCount(%d)\n",
		       instIdx, gExecInfo.exeInfoCount);
		g3dkmd_backtrace();
		return NULL;
	}

	pInst = gExecInfo.exeList[instIdx];
	if (!pInst) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
		       "gExecInfo.exeList[%u] is NULL", gExecInfo.exeList[instIdx]);
		g3dkmd_backtrace();
		return NULL;
	}
	return pInst;
}

unsigned int g3dKmdExeInstGetIdx(struct g3dExeInst *pInst)
{
	unsigned int i;

	if (!pInst)
		return EXE_INST_NONE;

	for (i = 0; i < (unsigned int)gExecInfo.exeInfoCount; i++) {
		if (gExecInfo.exeList[i] == pInst)
			return i;
	}

	return EXE_INST_NONE;
}

#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
/* =================================================================== /// */
/* Workqueue for create/release instance. */

struct G3dKmdExeInstAllocWork {
	struct work_struct work;
	int taskID;
	G3DKMD_BOOL isMigratingAllTasks;

	unsigned int boff_va_ovf_cnt;
	unsigned int boff_bk_ovf_cnt;
	unsigned int boff_tb_ovf_cnt;
	unsigned int boff_hd_ovf_cnt;
};


struct G3dKmdExeInstReleaseWork {
	struct work_struct work;
	int instIdx;
};



static void _g3dkmd_create_exeInst_workqueue(struct work_struct *work)
{
	struct G3dKmdExeInstAllocWork *my_work = (struct G3dKmdExeInstAllocWork *) work;
	G3DKMD_BOOL isMigratingAllTasks = my_work->isMigratingAllTasks;
	unsigned int boff_va_ovf_cnt = my_work->boff_va_ovf_cnt;
	unsigned int boff_bk_ovf_cnt = my_work->boff_bk_ovf_cnt;
	unsigned int boff_tb_ovf_cnt = my_work->boff_tb_ovf_cnt;
	unsigned int boff_hd_ovf_cnt = my_work->boff_hd_ovf_cnt;
	int taskID = my_work->taskID;

	int newInstIdx = EXE_INST_NONE;
	int oldInstIdx = EXE_INST_NONE;
	struct g3dExeInst *oldInst = NULL;
	struct g3dExeInst *newInst = NULL;
	struct g3dTask *pTask = NULL;

	G3DKMD_DEV_REG_PARAM param_inst = { 0 };
	G3DKMD_DEV_REG_PARAM *param = &param_inst;

	G3DKMDFREE(work);


	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
	       "(taskID: %d, isMigratingAllTasks: %c)\n",
	       taskID, isMigratingAllTasks ? 'Y' : 'N');

	/* 0. prepare memory related works */
	/* 0-1. create newInst */
	/* 0-2. create backHt for oldInst if need. */
	/*  */
	/* 1. Move task to new Instance */
	/* 1-1. move migrating task(s) to newInst. */
	/*  */
	/* 2. if if any task in oldInst: (nTask(oldInst) - migrating task(s)) >0 */
	/* 2-1. move oldInst->pHtDataForSwWrite to backHt   : so that the new cmds will go to backHt. */
	/*  */
	/* 3. Let Scheduler can active newInst. */

	/* check if the request migration work can be done or not. */
	{
		struct sysinfo sysinfo;

		g3dkmd_get_sysinfo(&sysinfo);
		if (sysinfo.freeram < G3DKMD_DDFR_SYSRAM_REJECT_THRLHLD) {
			KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
			       "Free System memory(%u) < thrlhld (%u) Stop migrating Task(%d).\n",
			       sysinfo.freeram, G3DKMD_DDFR_SYSRAM_REJECT_THRLHLD, taskID);
			return;
		}
	}

	lock_g3d();
	{
		if (!_g3dKmdTestMigrationCountLessEq(0)) { /* if any Migration now */
			KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
			       "Some other task migration is going now. Stop migrating Task(%d).\n", taskID);
			unlock_g3d();
			return;
		}

		pTask = g3dKmdGetTask(taskID);
		if (NULL == pTask) {
			KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_TASK,
			       "Get pTask(%d) fail. Stop migration task(%d).\n", taskID, taskID);
			unlock_g3d();
			return;
		}
		if (pTask->tryAllocMemFailCnt > G3DKMD_DDFR_TASK_TRY_MEM_FAIL_MAX) {
			KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_TASK,
			       "Get task(%d)->tryAllocMemFailCnt (%u) > MAX(%u). Stop migration task(%d).\n",
			       taskID, pTask->tryAllocMemFailCnt,
			       G3DKMD_DDFR_TASK_TRY_MEM_FAIL_MAX, taskID);
			unlock_g3d();
			return;
		}

		oldInstIdx = g3dKmdTaskGetInstIdx(taskID);
		if (oldInstIdx == EXE_INST_NONE) {
			KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_TASK,
			       "Get taskID's(%d) InstInstIdx fail. Stop migration task(%d).\n",
			       taskID, taskID);
			unlock_g3d();
			return;
		}

		oldInst = g3dKmdExeInstGetInst(oldInstIdx);
		if (NULL == oldInst) {
			KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_TASK,
			       "The Inst(%d) is NULL. Stop migration task(%d).\n",
			       oldInstIdx, taskID);
			unlock_g3d();
			return;
		}
		if (oldInst->tryEnlargeFailCnt > G3DKMD_DDFR_INST_ENLARGE_FAIL_MAX) {
			KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_TASK,
			       "The Inst(%d)->tryEnlargeFailCnt(%u) > MAX(%u). Stop migration task(%d).\n",
			       oldInstIdx, oldInst->tryEnlargeFailCnt,
			       G3DKMD_DDFR_INST_ENLARGE_FAIL_MAX, taskID);
			unlock_g3d();
			return;
		}
		if (G3DKMD_IS_INST_MIGRATING_SOURCE(oldInst) ||
		    G3DKMD_IS_INST_MIGRATING_TARGET(oldInst)) {
			KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_TASK,
			       "The Inst(%d)%p Idx(child,parent): %d,%d of task(%d) is migrating. Stop migration task!!!\n",
			       oldInstIdx, oldInst, oldInst->childInstIdx,
			       oldInst->parentInstIdx, taskID);
			unlock_g3d();
			return;
		}
		/* prepare the param for creating new inst. */
		{
			G3Duint incCnt = 0;

			param->usrDfrEnable = G3DKMD_FALSE;

			/* query current size from oldInst. */
			g3dKmdQueryBuffSizesFromInst(oldInst, param);

			KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK,
			       "Dfr(va,bk,tb,hd): (%u %u %u %u)\n",
			       param->dfrVarySize,
			       param->dfrBinbkSize,
			       param->dfrBintbSize,
			       param->dfrBinhdSize);
#ifndef MAX
#define MAX(a, b) (((a) > (b))?(a):(b))
#endif
#ifndef MIN
#define MIN(a, b) (((a) < (b))?(a):(b))
#endif
			/* increase dfr size if need. */
			if (boff_va_ovf_cnt > G3DKMD_BOFF_VA_THRLHLD &&
			    param->dfrVarySize < G3DKMD_DDFR_VA_MAX) {
				/* param->dfrVarySize += G3DKMD_BOFF_VA_INC_SIZE; */
				/*
				param->dfrVarySize = MAX(param->dfrVarySize +
					G3DKMD_BOFF_VA_INC_SIZE, G3DKMD_DDFR_VA_MAX)
				*/
				param->dfrVarySize = MIN(param->dfrVarySize * 2, G3DKMD_DDFR_VA_MAX);
				incCnt += 1;
			}
			if (boff_bk_ovf_cnt > G3DKMD_BOFF_BK_THRLHLD &&
			    param->dfrBinbkSize < G3DKMD_DDFR_BK_MAX) {
				/* param->dfrBinbkSize += G3DKMD_BOFF_BK_INC_SIZE; */
				/*
				param->dfrBinbkSize = MAX(param->dfrBinbkSize +
					G3DKMD_BOFF_BK_INC_SIZE, G3DKMD_DDFR_BK_MAX)
				*/
				param->dfrBinbkSize = MIN(param->dfrBinbkSize * 2, G3DKMD_DDFR_BK_MAX);
				incCnt += 1;
			}
			if (boff_tb_ovf_cnt > G3DKMD_BOFF_TB_THRLHLD &&
			    param->dfrBintbSize < G3DKMD_DDFR_TB_MAX) {
				/* param->dfrBintbSize += G3DKMD_BOFF_TB_INC_SIZE; */
				/*
				param->dfrBintbSize = MAX(param->dfrBintbSize +
					G3DKMD_BOFF_TB_INC_SIZE, G3DKMD_DDFR_TB_MAX)
				*/
				param->dfrBintbSize = MIN(param->dfrBintbSize * 2, G3DKMD_DDFR_TB_MAX);
				incCnt += 1;
			}
			if (boff_hd_ovf_cnt > G3DKMD_BOFF_HD_THRLHLD &&
			    param->dfrBinhdSize < G3DKMD_DDFR_HD_MAX) {
				/* param->dfrBinhdSize += G3DKMD_BOFF_HD_INC_SIZE; */
				/*
				param->dfrBinhdSize = MAX(param->dfrBinhdSize +
					G3DKMD_BOFF_HD_INC_SIZE, G3DKMD_DDFR_HD_MAX)
				*/
				param->dfrBinhdSize = MIN(param->dfrBinhdSize * 2, G3DKMD_DDFR_HD_MAX);
				incCnt += 1;
			}
			if (incCnt <= 0) {
				KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_DDFR,
				       "The Inst(%d)%p from task(%d) does not enlarge Dfrs.\n",
				       oldInstIdx, oldInst, taskID);
				oldInst->tryEnlargeFailCnt++;
				unlock_g3d();
				return;
			}
			KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK,
			       "Dfr(va,bk,tb,hd): (%u %u %u %u)\n",
			       param->dfrVarySize,
			       param->dfrBinbkSize,
			       param->dfrBintbSize,
			       param->dfrBinhdSize);
		}
		_g3dKmdIncreaseMigrationCount(); /* increase migration cnt. */

		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK,
		       "(taskID: %d, isMigratingAllTasks: %c, boff(va,bk,tb,hd): (%u, %u, %u, %u)\n",
		       taskID, isMigratingAllTasks ? 'Y' : 'N',
		       boff_va_ovf_cnt, boff_bk_ovf_cnt, boff_tb_ovf_cnt, boff_hd_ovf_cnt);
	}
	unlock_g3d(); /* releae for alloc memory. */

	/* do the heavy works which require no locker */
	{
		newInstIdx = g3dKmdExeInstCreateWithState(param, G3DEXESTATE_INACTIVED);
		if (EXE_INST_NONE == newInstIdx) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
			       "Create Inst fail.\n");
			lock_g3d();
			{
				struct g3dTask *pTask = g3dKmdGetTask(taskID);

				if (NULL == pTask)
					pTask->tryAllocMemFailCnt++;

				_g3dKmdDecreaseMigrationCount();
			}
			unlock_g3d();
			return;
		}
	}

	lock_g3d();
	{
		struct g3dTask *pTask = NULL;

		/* 1. newInst Created */
		pTask = g3dKmdGetTask(taskID);
		oldInstIdx = g3dKmdTaskGetInstIdx(taskID);
		if (oldInstIdx == EXE_INST_NONE) {
			KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_TASK,
			       "Get taskID's(%d) InstInstIdx fail. Stop migration task(%d).\n",
			       taskID, taskID);
			g3dKmdExeInstRelease(newInstIdx); /* some error clean up. */
			_g3dKmdDecreaseMigrationCount(); /* decrease migration cnt. */
			unlock_g3d();
			return;
		}
		oldInst = g3dKmdExeInstGetInst(oldInstIdx);
		newInst = g3dKmdExeInstGetInst(newInstIdx);

		KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
		       "taskID: %d oldInst: %p oldInstIdx: %d newInstIdx: %d\n",
		       taskID, oldInst, oldInstIdx, newInstIdx);

		if (NULL == pTask || NULL == oldInst || NULL == newInst) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK,
			       "One of oldInst(%d)/newInst(%d)/pTask(%d) is NULL. Stop migration task(%d).\n",
			       oldInstIdx, newInstIdx, taskID);

			g3dKmdExeInstRelease(newInstIdx); /* some error clean up. */
			_g3dKmdDecreaseMigrationCount(); /* decrease migration cnt. */
			unlock_g3d();
			return;
		}
		/* Migration Real Start: no rejection after here. */
		{
			/* reset DDFR early rejection counters */
			oldInst->tryEnlargeFailCnt = 0;
			pTask->tryAllocMemFailCnt = 0;
		}


		/* move other task if migrate all task. */
		if (isMigratingAllTasks) {
#if 1
			int i;
			int nTaskMoved = 0;
			struct g3dTask *pgTask = NULL;

			for (i = 0, nTaskMoved = 0; i < gExecInfo.taskInfoCount; i++) {
				pgTask = gExecInfo.taskList[i];

				if (NULL != pgTask && pgTask->inst_idx == oldInstIdx) {
					KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_DDFR,
					       "Move task(%d) from Inst(%d) to Inst(%d)\n",
					       i, oldInstIdx, newInstIdx);
					g3dKmdTaskDissociateInstIdx(pgTask);
					g3dKmdTaskAssociateInstIdx(pgTask, newInstIdx);
					nTaskMoved++;
				}
			}
			if (nTaskMoved == 0)
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_DDFR, "No Task moved\n");

			/* migrate defalut Inst settings if need */
			/* bug when not two_queue setting. */
			if (g3dKmdGetNormalExeInstIdx() == oldInstIdx) {
				KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
				       "move additinal refCnt from %u -> %u(old is normalQ)\n",
				       oldInstIdx, newInstIdx);

				_g3dKmdExeInstDecRef(oldInstIdx); /* move the additional refCnt */
				_g3dKmdExeInstIncRef(newInstIdx); /* of default Inst to newInst. */
				_g3dKmdSetNormalExeInstIdx(newInstIdx);
			} else if ((gExecInfo.exeuteMode == eG3dExecMode_two_queues) &&
				   (g3dKmdGetUrgentExeInstIdx() == oldInstIdx)) {
				KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
				       "move additinal refCnt from %u -> %u (old is urgentQ)\n",
				       oldInstIdx, newInstIdx);
				_g3dKmdExeInstDecRef(oldInstIdx); /* move the additional refCnt */
				_g3dKmdExeInstIncRef(newInstIdx); /* of default Inst to newInst. */
				_g3dKmdSetUrgentExeInstIdx(newInstIdx);
			}
#endif
		} else {
			/* 1. move all tasks with PID == pTask->pid. */
			int pid = pTask->pid;
			struct g3dTask *pgTask = NULL;
			int nTaskMoved = 0;
			int i;
			struct G3dHtInst *pHtInst;

			/* 2-1. create backHt for oldInst */
			pHtInst = _getAllocFreeHtInstance(oldInst);
			if (pHtInst == NULL) {
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_DDFR,
				       "Inst(%d) get _getAllocFreehtinstance() fail.\n",
				       oldInstIdx);
				g3dKmdExeInstRelease(newInstIdx); /* some error clean up. */
				_g3dKmdDecreaseMigrationCount(); /* decrease migration cnt. */
				unlock_g3d();
				return;
			}
			/* YL_KMD_ASSERT(pHtInst != NULL); // "pHtInst(idx:%u) getFreeInstance() is null."); */

			for (i = 0, nTaskMoved = 0; i < gExecInfo.taskInfoCount; i++) {
				pgTask = gExecInfo.taskList[i];
				if (NULL == pgTask)
					continue;


				if (pgTask->pid == pid) {
					KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_DDFR,
					       "Move task(%d) from Inst(%d) to Inst(%d)\n",
					       i, oldInstIdx, newInstIdx);
					g3dKmdTaskDissociateInstIdx(pgTask);
					g3dKmdTaskAssociateInstIdx(pgTask, newInstIdx);
					nTaskMoved++;
				}
			}
			if (nTaskMoved == 0)
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_DDFR, "No Task moved\n");

			KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_DDFR,
			       "%d tasks moved with pid: %u\n", nTaskMoved, pid);
			/* 2. if any task in oldInst, "not move all" */
			if (oldInst->refCnt > 0) {
				/*
				 * 2-2. move oldInst->pHtDataForSwWrite to backHt   :
				 * so that the new cmds will go to backHt.
				*/

				oldInst->pHtDataForSwWrite = pHtInst;
			}
		}

		/* 3. store active info, so Scheduler can know coresponding newInst. */
		/* set as migration soure & target. */
		G3DKMD_INST_MIGRATION_ASSOCIATE(oldInst, newInst, oldInstIdx,
			newInstIdx);


		KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
		       "Task(%d) migrated from Inst(%p, %d) to Inst(%d) setuped.\n",
		       taskID, oldInst, oldInstIdx, newInstIdx);
	}
	unlock_g3d();

}


void g3dKmdTriggerCreateExeInst_workqueue(const int taskID,
					  const G3DKMD_BOOL isMigratingAllTasks,
					  const unsigned int boff_va_ovf_cnt,
					  const unsigned int boff_bk_ovf_cnt,
					  const unsigned int boff_tb_ovf_cnt,
					  const unsigned int boff_hd_ovf_cnt)
{
	/* prepare worker */
	struct G3dKmdExeInstAllocWork *pWork;

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK,
	       "(taskID: %d, isMigratingAllTasks: %c, boff(va,bk,tb,hd): (%u, %u, %u, %u)\n",
	       taskID, isMigratingAllTasks ? 'Y' : 'N',
	       boff_va_ovf_cnt, boff_bk_ovf_cnt, boff_tb_ovf_cnt, boff_hd_ovf_cnt);

	/* early reject if numbers of boff is small. */
	if (boff_va_ovf_cnt <= G3DKMD_BOFF_VA_THRLHLD &&
		boff_bk_ovf_cnt <= G3DKMD_BOFF_BK_THRLHLD &&
		boff_tb_ovf_cnt <= G3DKMD_BOFF_TB_THRLHLD &&
		boff_hd_ovf_cnt <= G3DKMD_BOFF_HD_THRLHLD) {

		KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_TASK,
		       "(taskID: %d, isMigratingAllTasks: %c, boff(va,bk,tb,hd): (%u, %u, %u, %u)REJECT due to too few boff.\n",
		       taskID, isMigratingAllTasks ? 'Y' : 'N',
		       boff_va_ovf_cnt, boff_bk_ovf_cnt, boff_tb_ovf_cnt, boff_hd_ovf_cnt);
		return;
	}

	/* free by work. */
	pWork = (struct G3dKmdExeInstAllocWork *) G3DKMDMALLOC_ATOMIC(
			sizeof(struct G3dKmdExeInstAllocWork)); /* free by work. */

	if (!pWork) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, "kmalloc fail.\n");
		return;
	}


	pWork->taskID = taskID;
	pWork->isMigratingAllTasks = isMigratingAllTasks;
	pWork->boff_va_ovf_cnt = boff_va_ovf_cnt;
	pWork->boff_bk_ovf_cnt = boff_bk_ovf_cnt;
	pWork->boff_tb_ovf_cnt = boff_tb_ovf_cnt;
	pWork->boff_hd_ovf_cnt = boff_hd_ovf_cnt;

	G3DKMD_INIT_WORK((struct work_struct *)pWork, _g3dkmd_create_exeInst_workqueue);

	g3dKmd_schedule_work((struct work_struct *)pWork, _g3dkmd_create_exeInst_workqueue);
}




static void _g3dkmd_release_exeInst_workqueue(struct work_struct *work)
{
	struct G3dKmdExeInstReleaseWork *my_work;
	int instIdx;

	if (!work) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_TASK, " work is NULL\n");
		return;
	}

	my_work = (struct G3dKmdExeInstReleaseWork *) work;
	instIdx = my_work->instIdx;
	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
	       "(instIdx: %d, work: %p)\n", instIdx);

	G3DKMDFREE(my_work);	/* free work */

	lock_g3d();
	{
		g3dKmdExeInstRelease(instIdx);	/* in lock may be slow. */
		_g3dKmdDecreaseMigrationCount();	/* decrease migration cnt. */
	}
	unlock_g3d();

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
	       "(instIdx:%d) is released\n", instIdx);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK,
		"(instIdx:%d) freed, Migration End.\n", instIdx);
}

void g3dKmdRequestExeInstRelease(const int instIdx)
{
	 /* free by work. */
	struct G3dKmdExeInstReleaseWork *pWork =
		(struct G3dKmdExeInstReleaseWork *) G3DKMDMALLOC(sizeof(struct G3dKmdExeInstReleaseWork));

	if (!pWork) {
		KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_TASK,
		       "========= ERROR: NOT Expected ====== (instIdx:%d) release is requested, but alloc struct work fail.\n");

		g3dKmdExeInstRelease(instIdx); /* release the parent Instance in scheduler if fail. */
		_g3dKmdDecreaseMigrationCount(); /* decrease migration cnt. */

		KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_TASK,
		       "(instIdx:%d) is released directly.\n", instIdx);
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_TASK,
		       "(instIdx:%d) freed, Migration End.\n", instIdx);
	} else {

		KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
		       "instIdx(%d) release is requesting (pWork: %p).\n",
		       instIdx, pWork);

		G3DKMD_INIT_WORK((struct work_struct *)pWork,
			_g3dkmd_release_exeInst_workqueue);

		pWork->instIdx = instIdx;

		g3dKmd_schedule_work((struct work_struct *)pWork,
				     _g3dkmd_release_exeInst_workqueue);

		KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_TASK,
		       "instIdx(%d) release is requested.\n", instIdx);
	}
}



/* ===================================================================
 * The gobal data for task migration
 *
 * MigrationCnt:
 *    Use to record the number of tasks is migration.
 *    The migration Count APIs don't require atomic counter, because these
 *    APIs need within lock_g3d() & unlock_g3d().
 */

struct _task_migration_globals_ {
	int migratingCnt;
} _g_task_migration_data = {0};

/*
void _g3dKmdInitMigrationData()
{
	_g_task_migration_data.migratingCnt = 0;
}
*/
void _g3dKmdIncreaseMigrationCount(void)
{
	_g_task_migration_data.migratingCnt++;

	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR, "[CNT] MigrationCount: %d\n",
	       _g_task_migration_data.migratingCnt);
	{
		static unsigned long cnt;

		cnt++;
		KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR,
		       "[CNT] Migration Enter Times: %d\n", cnt);
	}
}

void _g3dKmdDecreaseMigrationCount(void)
{
	_g_task_migration_data.migratingCnt--;
	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR, "[CNT] MigrationCount: %d\n",
	       _g_task_migration_data.migratingCnt);
	{
		static unsigned long cnt;

		cnt++;
		KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR,
		       "[CNT] Migration Leave Times: %d\n", cnt);
	}
}

unsigned char _g3dKmdTestMigrationCountLessEq(const int val)
{
	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR,
	       "[CNT] MigrationCount Test: %d <= %d bool: %c\n",
	       _g_task_migration_data.migratingCnt, val,
	       (_g_task_migration_data.migratingCnt <= val) ? 'T' : 'F');
	return _g_task_migration_data.migratingCnt <= val;
}

#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */
