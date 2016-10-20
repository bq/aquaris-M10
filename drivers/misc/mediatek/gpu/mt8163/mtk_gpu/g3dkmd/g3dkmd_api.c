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

#include "g3dkmd_file_io.h"

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)
#include <windows.h>
#include <stdlib.h>
#elif defined(linux) && defined(linux_user_mode) /* linux user space */
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#else
#include <linux/ftrace_event.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/memory.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/wait.h>
#endif

#if defined(YL_MET) || defined(G3DKMD_MET_CTX_SWITCH)
#include "g3dkmd_met.h"
#endif

#include "sapphire_reg.h"
#include "g3dkmd_cfg.h"
#include "g3dkmd_util.h"

#ifdef USE_GCE
#include "g3dkmd_gce.h"
#ifdef PATTERN_DUMP
#include "g3dkmd_mmcq.h"
#endif
#endif

#include "g3dkmd_api.h"
#include "g3dkmd_api_mmu.h"
#include "g3dkmd_task.h"
#include "g3dkmd_engine.h"
#include "g3dbase_buffer.h"
#include "g3dkmd_buffer.h"
#ifdef ANDROID
#include "g3dkmd_ion.h"
#endif
#ifdef PATTERN_DUMP
#include "g3dkmd_pattern.h"
#endif

#include "g3dkmd_internal_memory.h"
#include "mmu/yl_mmu_utility.h"
#include "g3dkmd_scheduler.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_macro.h"
#include "g3dkmd_power.h"
#include "g3dkmd_memory_list.h"

#ifdef G3DKMD_USE_BUFFERFILE
#include "bufferfile/g3dkmd_bufferfile_io.h"
#endif


#include "g3dbase_ioctl.h"

#ifdef G3DKMD_SUPPORT_MSGQ
#include "g3dkmd_msgq.h"
#endif

#ifdef G3DKMD_SUPPORT_FDQ
#include "g3dkmd_fdq.h"
#endif

#ifdef G3DKMD_SUPPORT_ISR
#include "g3dkmd_isr.h"
#endif

#ifdef G3DKMD_SUPPORT_PM
#include "g3dkmd_pm.h"
#endif

#ifdef G3DKMD_SUPPORT_HW_DEBUG
#include "g3dkmd_hwdbg.h"
#endif

#ifdef G3DKMD_SUPPORT_IOMMU
#include "g3dkmd_iommu.h"
#endif

#include "platform/g3dkmd_platform_waitqueue.h"
#include "platform/g3dkmd_platform_pid.h"

#if defined(G3DKMD_RECOVERY_BY_TIMER) || defined(G3D_SWRDBACK_RECOVERY)
#include "g3dkmd_recovery.h"
#endif

#include "test/tester_non_polling_flushcmd.h" /* for tester_fence_block()  when defined(TEST_NON_POLLING_FLUSHCMD) */
#include "test/tester_non_polling_qload.h"

#include "g3dkmd_profiler.h"

#if defined(linux) && !defined(linux_user_mode) /* linux kernel */
#include "debugger/debugfs/g3dkmd_debugfs.h"
#endif

#include "g3dkmd_fakemem.h" /* for g_max_pa_addr */

/* static variable */
struct g3dExecuteInfo gExecInfo;
static int g3dRefCount;

#ifdef PATTERN_DUMP
void *gfAllocInfoKernal = NULL;
#endif

static __DEFINE_SEM_LOCK(g3dLock);
static __DEFINE_SPIN_LOCK(g3dPwrStateLock);
static __DEFINE_SPIN_LOCK(g3dFdqLock);

#define MAX_SORTING_TASK 16

/* static function */
static void _g3dKmdInitExecuteList(void);
static void _g3dKmdDestroyExecuteList(void);

/* ////////////////////////////////////////////////////////////////// */

void lock_g3d(void)
{
	__SEM_LOCK(g3dLock);
}

void unlock_g3d(void)
{
	__SEM_UNLOCK(g3dLock);
}

void lock_g3dPwrState(void)
{
	__SPIN_LOCK_BH(g3dPwrStateLock);
}

void unlock_g3dPwrState(void)
{
	__SPIN_UNLOCK_BH(g3dPwrStateLock);
}

void lock_g3dFdq(void)
{
	__SPIN_LOCK_BH(g3dFdqLock);
}

void unlock_g3dFdq(void)
{
	__SPIN_UNLOCK_BH(g3dFdqLock);
}

#ifdef G3DKMD_CALLBACK_INT
pid_t g3dKmdGetTaskProcess(int taskID)
{
	struct g3dTask *pTask = g3dKmdGetTask(taskID);

	if (NULL == pTask)
		return 0;

	return pTask->tid;
}
#endif

G3DKMD_APICALL int G3DAPIENTRY g3dKmdInitialize(void)
{
	int i;
	unsigned int value = 0;

	__SEM_LOCK(g3dLock);

	if (g3dRefCount == 0) {
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API, "\n");

#if !(defined(_WIN32) || (defined(linux) && defined(linux_user_mode)))
		g3dKmdCfgInit();
#endif

		g3dKmdInitFileIo(); /* this must been done before any file operations */

#ifdef G3DKMDDEBUG
		g3dKmd_openDebugFile();
#endif
		/* -2. initialize G3DMemory Link List head. (MUST before all mem alloc) */
		_g3dKmdMemList_Init();

		/* -1. init profilers */
		_g3dKmdProfilerModuleInit();

		/* 0. init MsgQ */
#ifdef G3DKMD_SUPPORT_MSGQ
		g3dKmdMsgQInit();
#endif
		/* 1. reset list info */
		{
			memset((void *)&gExecInfo, 0, sizeof(struct g3dExecuteInfo));
			for (i = 0; i < NUM_HW_CMD_QUEUE; i++) {
				gExecInfo.hwStatus[i].inst_idx = EXE_INST_NONE;
				gExecInfo.hwStatus[i].state = G3DKMD_HWSTATE_IDLE;
			}

			gExecInfo.exeuteMode = G3DKMD_EXEC_MODE;

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
			gExecInfo.PwrState = G3DKMD_PWRSTATE_POWERON;
#endif
		}
#ifdef PATTERN_DUMP
		if (g3dKmdCfgGet(CFG_DUMP_ENABLE)) {
			if (gfAllocInfoKernal == NULL)
				gfAllocInfoKernal = g_pKmdFileIo->fileOpen("alloc_info_kernal.txt");
			YL_KMD_ASSERT(gfAllocInfoKernal);
		}
#endif

#ifdef USE_GCE
		/* 2. init Gce */
		g3dKmdGceInit();
#endif

#ifdef G3DKMD_SIGNAL_CL_PRINT
		gExecInfo.cl_print_read_stamp = 0;
#endif


#if (USE_MMU == MMU_USING_WCP)
		/* 4. init MMU */
		if (g3dKmdCfgGet(CFG_MMU_ENABLE)) {
			gExecInfo.mmu = g3dKmdMmuInit(g3dKmdCfgGet(CFG_MMU_VABASE));
		} else {
			gExecInfo.mmu = NULL;
#ifdef G3DKMD_SUPPORT_IOMMU
			g3dKmdIommuDisable(IOMMU_MOD_1);
#ifdef G3DKMD_SUPPORT_EXT_IOMMU
			g3dKmdIommuDisable(IOMMU_MOD_2);
#endif
#endif
		}


		/* 3. reset hw */
		g3dKmdRegReset();


		/* ?. init all list queue */
		_g3dKmdInitExecuteList();

#else

		/* 3. reset hw */
		g3dKmdRegReset();


		/* ?. init all list queue */
		_g3dKmdInitExecuteList();

		/* 4. init MMU */
		if (g3dKmdCfgGet(CFG_MMU_ENABLE)) {
			gExecInfo.mmu = g3dKmdMmuInit(g3dKmdCfgGet(CFG_MMU_VABASE));
		} else {
			gExecInfo.mmu = NULL;
#ifdef G3DKMD_SUPPORT_IOMMU
			g3dKmdIommuDisable(IOMMU_MOD_1);
#ifdef G3DKMD_SUPPORT_EXT_IOMMU
			g3dKmdIommuDisable(IOMMU_MOD_2);
#endif
#endif
		}
#endif
		/* 5 Insert a block of memory to shift range of both Virtual and Physical memory. */
		value = g3dKmdCfgGet(CFG_MEM_SHIFT_SIZE);
		if (value) {
			G3dMemory memPadding;

			if (!allocatePhysicalMemory(&memPadding, value * 1024 * 1024, 16, G3DKMD_MMU_ENABLE)) {
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API,
				       "allocatePhysicalMemory for memPadding fail\n");
				YL_KMD_ASSERT(0);
				return 0;
			}
		}
#ifdef G3DKMD_SUPPORT_FDQ
		/* 6. init FDQ */
		g3dKmdFdqInit();
#endif
		/* 7. init exeInstance */
		g3dKmdExeInstInit();

#ifdef G3D_SUPPORT_SWRDBACK
		/* 8. create swrdback buffers */
		g3dKmdAllocSwRdback(&gExecInfo, 4096);	/* should not over 4096 (must also consider alignment size) */
#endif /* G3D_SUPPORT_SWRDBACK */

		/* 9. init schedule */
		g3dKmdCreateScheduler();

#ifdef G3DKMD_SUPPORT_ISR
		/* 10. install ISR */
		g3dKmdIsrInstall();
#endif

#ifdef G3DKMD_SUPPORT_PM
		/* 11. init PM */
		g3dKmdKmifAllocMemoryLocked(G3DKMD_PM_BUF_SIZE, G3DKMD_PM_ALIGN);
#endif

#ifdef YL_HW_DEBUG_DUMP_TEMP
		/* 12. init HW DEBUG */
		g3dKmdHwDbgAllocMemory(G3DKMD_HWDBG_BUF_SIZE, G3DKMD_HWDBG_ALIGN);
#endif
#if defined(FPGA_G3D_HW) && defined(G3DKMD_RECOVERY_BY_TIMER)
		/* 14. init and trigger recovery timer */
		g3dKmd_recovery_timer_init();
#endif /* G3DKMD_RECOVERY_BY_TIMER */


		/* Non polling wait queues initialization */
#if defined(ENABLE_QLOAD_NON_POLLING)
		/* 15. QLoad wait queue. */
		g3dkmd_init_waitqueue_head(&gExecInfo.s_QLoadWq);

#if defined(ENABLE_TESTER) && defined(ENABLE_TESTER_NON_POLLING_QLOAD)
		/* register QLoad testcases. */
		tester_reg_qload_tcases();
#endif
#endif

#if defined(ENABLE_NON_POLLING_FLUSHCMD)
		/* 16. initial flushCmdWq for non-polling flushCmd */
		g3dkmd_init_waitqueue_head(&gExecInfo.s_flushCmdWq);
#endif

		/* 17. debugfs */
#if defined(linux) && !defined(linux_user_mode) /* linux kernel */
		{
			/* loglevel controller */
			g3dKmd_debugfs_create();
		}
#endif /* linux kernel */

		/* 18. Init Fence sync */
#ifdef G3DKMD_SUPPORT_FENCE
		g3dkmdFenceInit();
#endif

#ifdef G3DKMD_SUPPORT_CL_PRINT
#if defined(linux) && !defined(linux_user_mode)
		g3dkmd_init_waitqueue_head(&gExecInfo.s_waitqueue_head_cl);
#endif
#endif /* G3DKMD_SUPPORT_CL_PRINT */
	}

	g3dRefCount++;
	__SEM_UNLOCK(g3dLock);

	return 1;
}

G3DKMD_APICALL int G3DAPIENTRY g3dKmdTerminate(void)
{
	__SEM_LOCK(g3dLock);

	g3dRefCount--;

	if (g3dRefCount == 0) {
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API, "\n");

#if defined(linux) && !defined(linux_user_mode) /* linux kernel */
		g3dKmd_debugfs_destroy();
#endif /* linux kernel */

		/* 0. wait hw idle */

#ifdef G3DKMD_SUPPORT_ISR
		/* 1. install ISR */
		g3dKmdIsrUninstall();
#endif

		/* 2. release scheduler thread */
		g3dKmdReleaseScheduler();

#ifdef USE_GCE
		/* 3. uninit Gce */
		g3dKmdGceUninit();
#endif

		/* 4. uninit exeInstance */
		g3dKmdExeInstUninit();

#ifdef G3D_SUPPORT_SWRDBACK
		/* 5. destroy swrdback buffers */
		g3dKmdFreeSwRdback(&gExecInfo);
#endif /* G3D_SUPPORT_SWRDBACK */

#ifdef G3DKMD_SUPPORT_FDQ
		/* 6. destroy fdq */
		g3dKmdFdqUninit();
#endif

#ifdef YL_HW_DEBUG_DUMP_TEMP
		/* 7. uninit HW DEBUG */
		g3dKmdHwDbgFreeMemory();
#endif

#ifdef G3DKMD_SUPPORT_PM
		/* 8. uninit PM */
		g3dKmdKmifFreeMemoryLocked();
#endif
		/* 9. uninit MMU */
		g3dKmdMmuUninit(gExecInfo.mmu);
		gExecInfo.mmu = NULL;

		/* 10. destroy all list queue */
		_g3dKmdDestroyExecuteList();

		/* 11. uninit MsgQ */
#ifdef G3DKMD_SUPPORT_MSGQ
		g3dKmdMsgQUninit();
#endif

#if defined(FPGA_G3D_HW) && defined(G3DKMD_RECOVERY_BY_TIMER)
		/* 12. uninit and destroy recovery timer */
		g3dKmd_recovery_timer_uninit();
#endif

		/* 13. uninit  Fence sync */
#ifdef G3DKMD_SUPPORT_FENCE
		g3dkmdFenceUninit();
#endif

		/* -1. terminate profilers */
		_g3dKmdProfilerModuleTerminate();

		/* -2. cleanup memlist. */
		_g3dKmdMemList_Terminate();


#ifdef G3DKMDDEBUG
		g3dKmd_closeDebugFile();
#endif

#ifdef PATTERN_DUMP
		if (gfAllocInfoKernal)
			g_pKmdFileIo->fileClose(gfAllocInfoKernal);
#endif
	}

	__SEM_UNLOCK(g3dLock);

	return 1;
}



G3DKMD_APICALL int G3DAPIENTRY g3dKmdDeviceOpen(void *filp)
{
	return 0;
}


G3DKMD_APICALL int G3DAPIENTRY g3dKmdDeviceRelease(void *filp)
{
	_g3dKmdMemList_CleanDeviceMemlist(filp);
	return 0;
}


static void _g3dKmdInitExecuteList(void)
{
	g3dKmdInitTaskList();
}

static void _g3dKmdDestroyExecuteList(void)
{
	g3dKmdDestroyTaskList();
}

static unsigned int _g3dKmdAssignExeInst(G3DKMD_DEV_REG_PARAM *param, struct g3dTask *task)
{
	int taskIdx;
	unsigned int instIdx = EXE_INST_NONE;
	struct g3dTask *old_task;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API, "pid 0x%x, cur task_idx %d\n", task->pid, task->task_idx);

	/* if the tasks belong to the same process, we should use the same ExeInst */
	for (taskIdx = 0; taskIdx < gExecInfo.taskInfoCount; taskIdx++) {
		old_task = g3dKmdGetTask(taskIdx);
		if (old_task && (old_task != task) && (old_task->pid == task->pid)) {
			KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_API,
			       "found same pid, pid 0x%x, inst_idx %d, (old task_idx %d, cur task_idx %d)\n",
			       task->pid, old_task->inst_idx, old_task->task_idx, task->task_idx);

			instIdx = old_task->inst_idx;
			break;
		}
	}

	if (instIdx == EXE_INST_NONE) {
#ifdef ANDROID_UNIQUE_QUEUE
		if (gExecInfo.exeuteMode == eG3dExecMode_per_task_queue) {
#else
		if ((gExecInfo.exeuteMode == eG3dExecMode_per_task_queue) || param->usrDfrEnable) {
#endif
			instIdx = g3dKmdExeInstCreate(param);
			if (instIdx == EXE_INST_NONE) {
				/* g3dKmdDestroyTask(task, NULL, 0); */
				g3dKmdUnregisterAndDestroyTask(task, NULL, 0);
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API,
				       "g3dKmdExeInstCreate : out of memory\n");
				__SEM_UNLOCK(g3dLock);
				return G3DKMD_FALSE;
			}

			g3dKmdExeInstSetPriority(instIdx, !!(param->flag & G3DKMD_CTX_URGENT));
		} else if (gExecInfo.exeuteMode == eG3dExecMode_two_queues) {
			if (param->flag & G3DKMD_CTX_URGENT)
				instIdx = g3dKmdGetUrgentExeInstIdx();
			else
				instIdx = g3dKmdGetNormalExeInstIdx();
		} else if (gExecInfo.exeuteMode == eG3dExecMode_single_queue) {
			instIdx = g3dKmdGetNormalExeInstIdx();
#ifdef ANDROID_UNIQUE_QUEUE
			{
				struct g3dExeInst *pInst = gExecInfo.exeList[instIdx];

				if ((pInst->dfrBufFromUsr == G3DKMD_FALSE) && (param->usrDfrEnable == G3DKMD_TRUE)) {
					g3dKmdCreateBuffers(pInst, param);
					g3dKmdInitQbaseBuffer(pInst);
					if (pInst->hwChannel >= 0) {
						gExecInfo.hwStatus[pInst->hwChannel].inst_idx = EXE_INST_NONE;
						gExecInfo.hwStatus[pInst->hwChannel].state = G3DKMD_HWSTATE_IDLE;
					}
					pInst->hwChannel = G3DKMD_HWCHANNEL_NONE;
					pInst->time_slice = 0;
					pInst->state = G3DEXESTATE_IDLE;
				}
			}
#endif
		} else {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "Wrong executeMode\n");
			YL_KMD_ASSERT(0);
		}
	}

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API, "cur task_idx %d, instIdx %d\n", task->task_idx, instIdx);

	g3dKmdTaskAssociateInstIdx(task, instIdx);

	return G3DKMD_TRUE;
}

G3DKMD_APICALL int G3DAPIENTRY g3dKmdRegisterContext(G3DKMD_DEV_REG_PARAM *param)
{
	int index = -1;
#if defined(linux) && !defined(linux_user_mode)
	struct task_struct *current_task = current->group_leader;
#endif
	struct g3dTask *task = g3dKmdCreateTask(param->cmdBufStart, param->cmdBufSize, param->flag);

	if (task == 0) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "g3dKmdCreateTask : out of memory\n");
		return -1;
	}

	__SEM_LOCK(g3dLock);

	index = g3dKmdRegisterNewTask(&gExecInfo, task);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API, "[%03d]\n", index);

#ifdef YL_SECURE
	/* [brady todo] hack for security, and there is no such "per_task_queue" case */
	/* set frmCmd range */
	if (gExecInfo.exeuteMode == eG3dExecMode_single_queue) {
		if (param->flag & G3DKMD_CTX_SECURE) { /* << 1 because G3DKMD_MMU_SECURE = 2 & G3D_MMU_SECURE = 1 */
			/* set addr for dram */
			struct g3dExeInst *pInst = gExecInfo.exeList[0];
			SapphireQreg *pInfo = NULL;

			if (!pInst) {
				YL_KMD_ASSERT(pInst != NULL);
				return -1;
			}
			pInfo = (SapphireQreg *) pInst->qbaseBuffer.data;
			pInfo->reg_mx_secure1_base_addr.s.mx_secure1_base_addr = task->frameCmd_base;
			pInfo->reg_mx_secure1_top_addr.s.mx_secure1_top_addr = task->frameCmd_base
										+ task->frameCmd_size;
		}
	}
#endif

	if (!_g3dKmdAssignExeInst(param, task)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "_g3dKmdAssignExeInst fail\n");
		return -1;
	}
#ifdef FPGA_G3D_HW
	if (task->isDVFSPerf) {
		/* enable runtime_pm & dvfs perf mode */
		g3dKmdPerfModeEnable();
	}
#endif

#if defined(linux) && !defined(linux_user_mode)
	task_lock(current_task);
	strncpy(task->task_comm, current_task->comm, sizeof(current_task->comm));
	task_unlock(current_task);
#endif

	/* trigger scheduler, in order to switch to this new task */
	g3dKmdTriggerScheduler();

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API, "done: taskID %d, instIdx %d\n", task->task_idx, task->inst_idx);

	__SEM_UNLOCK(g3dLock);
	return index;
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdReleaseContext(int taskID, int abnormal)
{
	struct g3dTask *task;

	__SEM_LOCK(g3dLock);

	if ((taskID == -1) || (taskID >= gExecInfo.taskInfoCount)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "Wrong taskID\n");
		__SEM_UNLOCK(g3dLock);
		return;
	}

	task = g3dKmdGetTask(taskID);
	if (task) {
		int instIdx = g3dKmdTaskGetInstIdx(taskID);

		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API, "[%03d]\n", taskID);

#if (G3DKMD_CHECK_TASK_DONE_TIMEOUT_CNT > 0) && defined(G3D_SUPPORT_SWRDBACK)
		if (abnormal == 0) { /* check them only when normal task release */
			int poll = 0;
			g3dFrameSwData *ptr = (g3dFrameSwData *) gExecInfo.SWRdbackMem.data;

			if (ptr) {
				while (!(G3D_STAMP_DONE(task->lastFlushStamp, ptr[taskID].stamp,
							task->lastFlushStamp))) {
					G3DKMD_SLEEP(1);
					if (++poll >= G3DKMD_CHECK_TASK_DONE_TIMEOUT_CNT)
						break;
				}
			}
			YL_KMD_ASSERT(gExecInfo.taskList[taskID]->flushTigger == 0);
		}
#endif

#ifndef ANDROID_PREF_NDEBUG
		{
			unsigned int div, rem;

			UNUSED(div);
			UNUSED(rem);
			/* show the average queued HT slots when log in flush cmd and fdq notify */
			div = (task->htCmdCnt) ? task->accFlushHTCmdCnt / task->htCmdCnt : 0;
			rem = (task->htCmdCnt) ? task->accFlushHTCmdCnt % task->htCmdCnt : 0;
			rem = (task->htCmdCnt) ? (rem * 100 + (task->htCmdCnt / 2)) / task->htCmdCnt : 0;/* round */
			KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_API,
			       "task[%03d] average queued HT (%d.%02d)(%d,%d) when flush cmd notify\n",
			       taskID, div, rem, task->accFlushHTCmdCnt, task->htCmdCnt);
			rem = 0;
		}
#ifdef G3DKMD_FPS_BY_FDQ
		g3dkmdResetFpsInfo(); /* reset it if running the next AP. */
#endif
#endif

#ifdef FPGA_G3D_HW
		if (task->isDVFSPerf) {
			/* disable runtime_pm & dvfs perf mode */
			g3dKmdPerfModeDisable();
		}
#endif

		/* delete exeInst object */
		g3dKmdTaskDissociateInstIdx(task);
		/* g3dKmdExeInstDecRef(instIdx); */
		g3dKmdExeInstRelease(instIdx);

		/* delete task object */
		g3dKmdUnregisterAndDestroyTask(task, &gExecInfo, taskID);

		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API,
		       "g3dKmdReleaseContext done: taskID %d, instIdx %d\n", taskID, instIdx);
	}

	__SEM_UNLOCK(g3dLock);
}

#ifdef G3D_SUPPORT_FLUSHTRIGGER

static void g3dKmdTaskFlushTrigger(unsigned int set, int taskID, unsigned int *value)
{
	struct g3dTask *task = g3dKmdGetTask(taskID);

	if (task == NULL)
		return;
	if (set)
		task->flushTigger = *value;
	else
		*value = task->flushTigger;
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdFlushTrigger(unsigned int set, int taskID, unsigned int *value)
{
	__SEM_LOCK(g3dLock);

	if (value) {
		g3dKmdTaskFlushTrigger(set, taskID, value);
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API, "g3dKmdFlushTrigger[%u,%03d] = %u\n", set, taskID, *value);
	}

	__SEM_UNLOCK(g3dLock);
}

#endif /* G3D_SUPPORT_FLUSHTRIGGER */

#if defined(ANDROID) && defined(linux) && !defined(linux_user_mode)

/* update per pass result of slots in data per process */
G3DKMD_APICALL void G3DAPIENTRY g3dKmdLockCheckDone(unsigned int num_task, struct ionGrallocLockInfo *data)
{
	unsigned int i, j;
	unsigned int result;
	g3dFrameSwData *ptr;

	if (num_task == 0 || data == NULL)
		return;

	__SEM_LOCK(g3dLock);

	ptr = (g3dFrameSwData *) gExecInfo.SWRdbackMem.data;
	for (i = 0; i < num_task && ptr != NULL; i++) {
		result = ALL_PASS; /* if task not found, we think H/W don't care it in default */

		if ((data[i].pass & ALL_PASS) == ALL_PASS)
			continue;

		for (j = 0; j < gExecInfo.taskInfoCount; j++) {
			struct g3dTask *taskj = g3dKmdGetTask(j);

			if (taskj && j == data[i].taskID && taskj->pid == (pid_t) data[i].pid) {
#ifdef G3D_SUPPORT_FLUSHTRIGGER
				if (G3D_STAMP_NOT_FLUSH(data[i].write_stamp,
							taskj->lastFlushStamp,
							taskj->lastFlushStamp)
				    || G3D_STAMP_NOT_FLUSH(data[i].read_stamp,
							   taskj->lastFlushStamp,
							   taskj->lastFlushStamp)) {
					unsigned int value = 1;

					g3dKmdTaskFlushTrigger(1, j, &value);
					KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_API,
					       "flushtrigger taskID[%d,%d] by stamp (%d, %d)\n",
					       j, taskj->lastFlushStamp, data[i].write_stamp, data[i].read_stamp);
				}
#endif
				if (!(G3D_STAMP_DONE(data[i].write_stamp, ptr[j].stamp, taskj->lastFlushStamp)))
					result &= (~WRITE_PASS);

				if (!(G3D_STAMP_DONE(data[i].read_stamp, ptr[j].stamp, taskj->lastFlushStamp)))
					result &= (~READ_PASS);

				taskj->isGrallocLockReq = (result == ALL_PASS) ? G3D_FALSE : G3D_TRUE;
				break;
			} /* if */
		}	 /* for */
		data[i].pass = result;
	}
	__SEM_UNLOCK(g3dLock);
}

/* update all result of slots for all processes if it's stamp are all flushed */
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdLockCheckFlush(unsigned int num_task, struct ionGrallocLockInfo *data)
{
	unsigned int i, j;
	unsigned int result, all_result = 1;

	if (num_task == 0 || data == NULL)
		return 1;

	__SEM_LOCK(g3dLock);

	for (i = 0; i < num_task; i++) {
		result = 1; /* if task not found, we think H/W don't care it in default */
		for (j = 0; j < gExecInfo.taskInfoCount; j++) {
			struct g3dTask *taskj = g3dKmdGetTask(j);

			if (taskj && j == data[i].taskID && taskj->pid == (pid_t) data[i].pid) {
				if (G3D_STAMP_NOT_FLUSH(data[i].write_stamp,
							taskj->lastFlushStamp,
							taskj->lastFlushStamp))
					all_result = result = 0;

				if (G3D_STAMP_NOT_FLUSH(data[i].read_stamp,
							taskj->lastFlushStamp,
							taskj->lastFlushStamp))
					all_result = result = 0;

				taskj->isGrallocLockReq = (result == 1) ? G3D_FALSE : G3D_TRUE;
				break;
			}
		}
	}

	__SEM_UNLOCK(g3dLock);
	return all_result;
}
#endif

#if 0
G3DKMD_APICALL G3DKMDRETVAL G3DAPIENTRY g3dKmdWaitIdStamp(int taskID, int objIndex, unsigned int objstamp)
{
	pstruct g3dTask *curtask, *objtask, *tmptask;
	int *sortingBuffer = NULL;
	int i;
	int index;


	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API,
	       "g3dKmdWaitIdStamp[%03d] : need wait task[%03d](stamp = 0x%08x)\n", taskID, objIndex, objstamp);

	if (taskID == objIndex)
		return G3DKMDRET_SUCCESS;

	if ((taskID == -1) || (taskID >= gExecInfo.taskInfoCount)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "First Task not exist!!!!\n");
		return G3DKMDRET_ERROR;
	}

	if ((objIndex == -1) || (objIndex >= gExecInfo.taskInfoCount)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "Object Task not exist!!!!\n");
		return G3DKMDRET_ERROR;
	}


	sortingBuffer = (int *)G3DKMDMALLOC(sizeof(struct g3dTask *) * gExecInfo.taskInfoCount);
	if (sortingBuffer == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "Can't create sorting buffer!!!!\n");
		return G3DKMDRET_ERROR;
	}

	memset(sortingBuffer, 0xff, sizeof(int) * gExecInfo.taskInfoCount);

	tmptask = g3dKmdGetTask(taskID);
	sortingBuffer[taskID] = 1;
	while (tmptask->remapIndex != -1) {
		sortingBuffer[tmptask->remapIndex] = 1;
		tmptask = g3dKmdGetTask(tmptask->remapIndex);
	}

	tmptask = g3dKmdGetTask(objIndex);
	sortingBuffer[objIndex] = 1;
	while (tmptask->remapIndex != -1) {
		sortingBuffer[tmptask->remapIndex] = 1;
		tmptask = g3dKmdGetTask(tmptask->remapIndex);
	}

	index = 0;
	memset(&taskIndex[0], 0xff, sizeof(int) * MAX_SORTING_TASK);
	for (i = 0; i < gExecInfo.taskInfoCount; i++) {
		if (sortingBuffer[i] == 1) {
			if (index < MAX_SORTING_TASK) {
				taskIndex[index] = i;
				taskQueue[index] = g3dKmdGetTask(i);
				index++;
			} else {
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "Can't create sorting buffer!!!!\n");
				G3DKMDFREE(sortingBuffer);
				return G3DKMDRET_ERROR;
			}
		}
	}

	if (index == 0) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "index 0 not exist!!!!\n");
		G3DKMDFREE(sortingBuffer);
		return G3DKMDRET_ERROR;
	}

	curtask = g3dKmdGetTask(taskIndex[0]);
	if (curtask->htInfo == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "Object Task not exist!!!!\n");
		G3DKMDFREE(sortingBuffer);
		return G3DKMDRET_ERROR;
	}

	for (i = 1; i < index; i++) {
		objtask = g3dKmdGetTask(taskIndex[i]);

		if (objtask->remapIndex != taskIndex[0])
			curtask->priorityWeighting++;

		objtask->remapIndex = taskIndex[0];

		if (objtask->state == G3DTASKSTATE_RUNNING) {
			if ((objtask->htInfo) && (objtask->htInfo->rdPtr == objtask->htInfo->wtPtr)) {
				/* remove it by handling hw interrupt */
				objtask->state = G3DTASKSTATE_REMAP;
			} else {
				G3DKMDFREE(sortingBuffer);
				return G3DKMDRET_FAIL; /* wait util objtask->state change */
			}
		} else {
			if ((objtask->htInfo) && (objtask->htInfo->rdPtr != objtask->htInfo->wtPtr)) {
				int need = (MAX_HT_COMMAND - g3dKmdCheckFreeHtSlots(-1, objtask->htInfo));

				if (need > g3dKmdCheckFreeHtSlots(curtask->hwChannel, curtask->htInfo)) {
					G3DKMDFREE(sortingBuffer);
					return G3DKMDRET_FAIL; /* no space */
				}

				for (i = 0; i < MAX_HT_COMMAND; i++) {
					int index = (objtask->htInfo->rdPtr + i) % MAX_HT_COMMAND;

					if (index == objtask->htInfo->wtPtr)
						break;

					g3dKmdCopyCmd(curtask->htInfo,
						      (struct g3dHtCommand *) &objtask->htInfo->htCommand[index]);

					/* g3dKmdAddList(curtask->htInfo,
							objtask->htInfo->shortInfo[index].index,
							objtask->htInfo->shortInfo[index].wptr); */
				}
			}
		}
		g3dKmdDestroyHtInfo(objtask->htInfo);
		objtask->htInfo = NULL;
		objtask->state = G3DTASKSTATE_REMAP;
	}

	G3DKMDFREE(sortingBuffer);
	return G3DKMDRET_SUCCESS;
}
#endif /* 0 */

#if 0 /* defined(PATTERN_DUMP) && defined(CMODEL) */
static void _g3dKmdRemapHwAddr(G3dMemory *mem, unsigned int size, unsigned int alignment)
{
	unsigned int expanded_size;

#define _IS_POW_TWO(align) (!(align & (align - 1)))
#define _ALIGN_PTR(addr, align) ((align == 0) ? addr : (addr + align - 1) & ~(align - 1))

	YL_KMD_ASSERT(_IS_POW_TWO(alignment));

	/* since HW pattern format requires 16 bytes aligned, */
	/* this is for not to overwrite adjacent buffer's data */
	/* size+=15; */

	/* mindos : The 16byte alignment for exact size is reasonable, but adding 15 bytes */
	/* without align it make the size irregular. */
	size = (size + 15) & ~15UL;

	expanded_size = size + alignment;

	if (mem->type == G3D_MEM_VIRTUAL) {
		mem->hwAddr = g3dKmdMmuMap(mmuGetMapper(), 0, mem->data, expanded_size, MMU_SECTION_DETECTION, 0);
		mem->hwAddr0 = 0;
#ifdef CMODEL
		mem->patPhyAddr = g3dKmdMmuG3dVaToPa(mmuGetMapper(), mem->hwAddr);

#ifdef YL_MMU_PATTERN_DUMP_V1
		if (g3dKmdCfgGet(CFG_DUMP_ENABLE)) {
			YL_KMD_ASSERT(mem->pa_pair_info == 0);
			mem->pa_pair_info = g3dKmdMmuG3dVaToPaFull(g3dGbl.mmu, mem->hwAddr, mem->size);
		}
#endif
#endif
	} else {
		mem->hwAddr0 = g3dKmdRemapPhysical(mem->data, expanded_size);
		mem->hwAddr = _ALIGN_PTR(mem->hwAddr0, alignment);
#ifdef CMODEL
		mem->patPhyAddr = mem->hwAddr;
#endif

#ifdef YL_MMU_PATTERN_DUMP_V1
		mem->pa_pair_info = 0;
#endif
	}
}
#endif

G3DKMD_APICALL G3DKMDRETVAL G3DAPIENTRY g3dKmdLTCFlush(void)
{
	__SEM_LOCK(g3dLock);

	if (!g3dKmdEngLTCFlush())
		return G3DKMDRET_ERROR;

	__SEM_UNLOCK(g3dLock);

	return G3DKMDRET_SUCCESS;
}


/* [non-polling flush command]
 *  Check if enough Ht space: condition function. [Louis 2014/08/14]
 */
#if defined(ENABLE_NON_POLLING_FLUSHCMD)
/* static int _checkIfEnoughHtSlots(struct g3dExeInst *pInst) */
static int _checkIfEnoughHtSlots(const int taskID)
{
	int ret;
	struct g3dExeInst *pInst;

	__SEM_LOCK(g3dLock); /* make sure to get the right pInst from taskID. */
	UNUSED(pInst);
	pInst = g3dKmdTaskGetInstPtr(taskID);
	if (unlikely(pInst == NULL)) {
		KMDDPF(G3DKMD_LLVL_WARNING || G3DKMD_MSG_TASK,
		       "getKmdTaskGetInstPtr(taskID: %d) fail.\n", taskID);
		__SEM_UNLOCK(g3dLock);
		return G3DKMD_FALSE;
	}

	ret = g3dKmdCheckIfEnoughHtSlots(pInst->hwChannel, pInst->pHtDataForSwWrite->htInfo);
	ret = tester_isHtNotFull(ret, taskID); /* let tester can modify the ret value. */
	__SEM_UNLOCK(g3dLock);

	return ret;
}

/* [non-polling flush command]
 *  Only active on Linux_KO mode. Win32/Linux_user using polling approch.
 */
static int _g3dKmdSleepTillEnoughHtSlots(int taskID)
{
#if 0
	struct g3dExeInst *pInst;

	__SEM_LOCK(g3dLock); /* make sure to get the right pInst from taskID. */

	/* pInst already check in caller. */

	pInst = g3dKmdTaskGetInstPtr(taskID);

	__SEM_UNLOCK(g3dLock);
#endif


	return g3dkmd_wait_event_interruptible_timeout(gExecInfo.s_flushCmdWq,
							_checkIfEnoughHtSlots(taskID) /* condition */,
#if 0
							_checkIfEnoughHtSlots(pInst) /* condition */,
#endif
						       G3DKMD_NON_POLLING_FLUSHCMD_WAIT_TIMEOUT);
}
#endif

G3DKMD_APICALL G3DKMDRETVAL G3DAPIENTRY g3dKmdFlushCommand(int taskID, void *head, void *tail, int fence,
							   unsigned int lastFlushStamp,
							   unsigned int carry,
#ifdef G3D_SWRDBACK_RECOVERY
							   unsigned int lastFlushPtr,
#endif
							   unsigned int flushDCache)
{
	struct g3dExeInst *pInst;
	struct g3dTask *task;
	int wq_ret;

	if ((taskID == -1) || (taskID >= gExecInfo.taskInfoCount) || (head > tail))
		return G3DKMDRET_ERROR;


	__SEM_LOCK(g3dLock);

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
	gpu_set_power_on();
#endif

	task = g3dKmdGetTask(taskID);

	if (task == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API,
		       "g3dKmdFlushCommand task in gExecInfo is NULL\n");
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
		gpu_may_power_off();
#endif
		__SEM_UNLOCK(g3dLock);
		return G3DKMDRET_ERROR;
	}

	task->lastFlushStamp = lastFlushStamp;
	task->carry = carry;
	pInst = g3dKmdTaskGetInstPtr(taskID);

#ifdef G3D_SUPPORT_HW_NOFIRE
	/* For driver overhead testing */
	if (task->isNoFire && !task->isUrgentPrio) {
#ifdef G3D_SUPPORT_FLUSHTRIGGER
		task->flushTigger = 0;
#endif
#ifdef G3DKMD_SUPPORT_FENCE
		g3dkmdUpdateTimeline(taskID, lastFlushStamp);
#endif
#ifdef G3D_SUPPORT_SWRDBACK
		g3dFrameSwData *pSwData = (g3dFrameSwData *) ((unsigned char *)gExecInfo.SWRdbackMem.data +
					  G3D_SWRDBACK_SLOT_SIZE * taskID);
		pSwData->readPtr = (uintptr_t) lastFlushPtr;
		pSwData->stamp = lastFlushStamp;
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
		gpu_may_power_off();
#endif
		__SEM_UNLOCK(g3dLock);
		return G3DKMDRET_SUCCESS;
	}
#endif

	if ((pInst == NULL) || (pInst->pHtDataForSwWrite->htInfo == NULL)
	    || (pInst->pHtDataForSwWrite->htInfo->htCommand == NULL)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API,
		       "task pHtDataForSwWrite->htInfo or it's htCommand is NULL\n");
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
		gpu_may_power_off();
#endif
		__SEM_UNLOCK(g3dLock);

		/* [Louis 2014/08/12]: everything is NULL, don't need to retry. */
		/* return G3DKMDRET_FAIL; */
		return G3DKMDRET_ERROR;
	}
#if defined(ENABLE_NON_POLLING_FLUSHCMD)
	/* [non-polling flush command]
	 * [Louis 2014/08/14]: wait until there is enough Ht slots. HW will send a
	 *   interrupt to Isr, and IsrTop() will wake up all process wait in
	 *   gExecInfo.s_flushCmdWq.
	 *    WIN/Linux_user: wq_ret will return 1, go the default way.
	 */

	__SEM_UNLOCK(g3dLock);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API,
	       "wait-queue in, taskID %d, Inst State %d\n", taskID, pInst->state);

	gpu_increment_flush_count();
	wq_ret = _g3dKmdSleepTillEnoughHtSlots(taskID);

	switch (wq_ret) {

	case -ERESTARTSYS: /* signal happened, go back to usermode immediately, and retry */
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API,
		       "wait-queue out for system interrupted, taskID %d, Inst State %d\n", taskID, pInst->state);
		tester_wq_signaled(taskID);
		gpu_decrement_flush_count();
		return G3DKMDRET_FAIL;

	case 0: /* timeout */
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API,
		       "wait-queue out for timeout, taskID %d, Inst State %d\n", taskID, pInst->state);
		tester_wq_timeout(taskID);
		gpu_decrement_flush_count();
		return G3DKMDRET_FAIL;

	default: /* the condition is true( there are enough Ht slots) */
		/* continue to flush command. */
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API,
		       "wait-queue out for default, taskID %d, Inst State %d\n", taskID, pInst->state);
		tester_wq_release(taskID);
		break;
	}

	__SEM_LOCK(g3dLock);
#endif /* defined(ENABLE_NON_POLLING_FLUSHCMD) */

	task = g3dKmdGetTask(taskID);
	if (unlikely(NULL == task)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "task in gExecInfo is NULL\n");
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
		gpu_may_power_off();
#endif
		__SEM_UNLOCK(g3dLock);
		gpu_decrement_flush_count();
		return G3DKMDRET_ERROR;
	}

	pInst = g3dKmdTaskGetInstPtr(taskID);
	if (unlikely(NULL == pInst)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API,
		       "g3dKmdTaskGetInstPtr(taskID: %d) fail.\n", taskID);
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
		gpu_may_power_off();
#endif
		__SEM_UNLOCK(g3dLock);
		gpu_decrement_flush_count();
		return G3DKMDRET_ERROR;
	}

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API, "in, taskID %d, Inst State %d\n", taskID, pInst->state);
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_FENCE, "taskID %d, lastFlushStamp 0x%x\n", taskID, lastFlushStamp);
	/* KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API,
			"g3dKmdFlushCommand SW[%03d] : (head,tail) = (0x%08x,0x%08x); htCommand %p, htBuffer 0x%x\n",
			taskID, head, tail, pInst->pHtDataForSwWrite->htInfo->htCommand, pInst->htBuffer.hwAddr0); */

	if ((unsigned)(g3dKmdCheckFreeHtSlots(pInst->hwChannel,
						pInst->pHtDataForSwWrite->htInfo)) < HT_BUF_LOW_THD) {/* always 2 */
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "available free slot is not enought\n");
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
		gpu_may_power_off();
#endif
		__SEM_UNLOCK(g3dLock);
		gpu_decrement_flush_count();
		return G3DKMDRET_FAIL;
	}
#ifdef YL_MET
	met_tag_async_end(current->pid, "PREP-POLL_RingBuf", lastFlushStamp);
	met_tag_async_start(current->pid, "PREP-POLL_RingBuf", lastFlushStamp + 1);
#endif

#ifdef G3DKMD_SUPPORT_FENCE
	if (task->timeline == NULL)
		g3dkmd_create_task_timeline(taskID, task);
#endif


	if (task->task_idx != (unsigned int)taskID) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API,
		       "task->task_idx(%u) != (%u)taskID\n", task->task_idx, taskID);
		YL_KMD_ASSERT(task->task_idx == (unsigned int)taskID);
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
		gpu_may_power_off();
#endif
		__SEM_UNLOCK(g3dLock);
		return G3DKMDRET_ERROR;
	}
#ifdef G3D_SWRDBACK_RECOVERY
	g3dKmdRecoveryAddInfo(pInst, lastFlushPtr, lastFlushStamp);
#endif

	g3dKmdAddHTCmd(pInst, head, tail, task->task_idx, fence);

#ifdef G3DKMD_MET_CTX_SWITCH
	met_tag_oneshot(0x8163, "g3dKmdFlushCommand", task->inst_idx);
	met_tag_oneshot(0x8163, "HT_CMD_NS",
			(HT_BUF_SIZE -
			 g3dKmdCheckFreeHtSlots(pInst->hwChannel, pInst->pHtDataForSwWrite->htInfo)) / HT_CMD_SIZE);
#endif

	/* if overflow, reset them all */
	if (task->htCmdCnt == 0xffffffff) {
		task->accFlushHTCmdCnt = 0;
		task->htCmdCnt = 0;
	} else {
		task->htCmdCnt++;
	}

	g3dKmdTaskUpdateInstIdx(task);

#ifdef PATTERN_DUMP
	pInst->sDumpIndexCount++;
#endif

#ifdef G3DKMD_SNAPSHOT_PATTERN
	{
		/* overflow protect */
		if (pInst->htCommandFlushedCount == 0xffffffff) {
			pInst->htCommandFlushedCount = 1;
			pInst->htCommandFlushedCountOverflow++;
		} else {
			pInst->htCommandFlushedCount++;
		}
	}
#endif

	if (pInst->state == G3DEXESTATE_RUNNING) {

#ifdef USE_GCE
		if (g3dKmdCfgGet(CFG_GCE_ENABLE)) {
			if (!g3dKmdGceSendCommand(pInst)) {
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API,
				       "g3dKmdGceSendCommand available free slot is not enought\n");
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
				gpu_may_power_off();
#endif
				__SEM_UNLOCK(g3dLock);
				gpu_decrement_flush_count();
				return G3DKMDRET_FAIL;
			}
		} else
#endif
		{
#ifdef YL_HW_DEBUG_DUMP_TEMP
			unsigned int readPtr = g3dKmdRegGetReadPtr(pInst->hwChannel);
#endif

			g3dKmdRegSendCommand(pInst->hwChannel, pInst);

#ifdef YL_HW_DEBUG_DUMP_TEMP
			if (lastFlushStamp - 1 >= g3dKmdHwDbgGetBeginFrameID()
			    && lastFlushStamp - 1 <= g3dKmdHwDbgGetEndFrameID()) {
				unsigned int i;

				for (i = 0; i < g3dKmdHwDbgGetRepeatTimes(); i++)
					g3dKmdRegRepeatCommand(pInst->hwChannel, pInst, readPtr);

#ifndef G3D_HW
				/* when testing, we need to stop HW to let the queue full. No update HW.raPtr. */
				if (tester_fence_block()) {
					KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_API, "fence is opened!\n");
					g3dKmdRegUpdateReadPtr(pInst->hwChannel,
							       pInst->pHtDataForHwRead->htInfo->wtPtr);
				} /* tester_fence_blcok(); */
#endif
			}
#endif
		}
#if !defined(G3D_HW) && defined(G3DKMD_SUPPORT_FDQ)
		g3dKmdFdqWriteQ(taskID);
#endif
	}
	/* trigger context switch for below condition */
	/* 1. active instance is empty */
	/* 2. current instance is urgent priority task, and not RUNNING */
	if (gExecInfo.currentExecInst
	    && g3dKmdIsFiredHtInfoDone(gExecInfo.currentExecInst->hwChannel,
			gExecInfo.currentExecInst->pHtDataForHwRead->htInfo)) {
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API,
		       "triggers context switch because active task empty\n");
#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "FlushCurEmpty_FORCE-TRIG", 1);
#endif
		g3dKmdTriggerScheduler();
#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "FlushCurEmpty_FORCE-TRIG", 0);
#endif
	} else if (pInst->isUrgentPrio && (pInst->state != G3DEXESTATE_RUNNING)) {
		/* trigger context switch for urgent priority task */
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API,
		       "triggers context switch for urgent task\n");
#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "Urgent_FORCE-TRIG", 1);
#endif
		g3dKmdTriggerScheduler();
#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "Urgent_FORCE-TRIG", 0);
#endif
	}
#ifndef G3DKMD_SCHEDULER_OS_CTX_SWTICH
	g3dKmdContextSwitch(1);
#endif

#ifdef G3DKMD_SNAPSHOT_PATTERN
	if (0) {/* try to snapshot pattern from current states //@@ */
		static int dump;

		if (!dump) {
			if (g3dKmdSnapshotPattern(taskID, SSP_DUMP_BYTASKID) == G3DKMDRET_SUCCESS)
				dump = 1;
		}
	}
#endif

#ifdef G3D_SUPPORT_FLUSHTRIGGER
	{
		unsigned int value = 0;

		g3dKmdTaskFlushTrigger(1, taskID, &value);
	}
#endif

	/* for the average queued slots */
#if defined(G3D_HW) && (!defined(ANDROID_PREF_NDEBUG) || defined(ANDROID_HT_COUNT))
	{
		int queued_slots = 1;

		if (pInst->hwChannel != -1) {
			/* Louis: don't know if this is for HwRead or SwWrite. */
			queued_slots = g3dKmdGetQueuedHtSlots(pInst->hwChannel, pInst->pHtDataForHwRead->htInfo);

			/* HtData for HwRead and SwWrite could be different htInstance. */
			if (pInst->pHtDataForHwRead != pInst->pHtDataForSwWrite) {
				/* if pointing to differnt htInstance, add slots in HtForSwWrite.*/
				int slots = g3dKmdGetQueuedHtSlots(pInst->hwChannel, pInst->pHtDataForSwWrite->htInfo);

				queued_slots = (queued_slots > 0 && slots > 0) ? queued_slots : queued_slots+slots;
			}

			if (queued_slots > 0)
				task->accFlushHTCmdCnt += queued_slots;
		}

		event_trace_printk(0, "C|%d|%s|%d\n", 0, "HT", queued_slots);
	}
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
	gpu_may_power_off();
#endif

	__SEM_UNLOCK(g3dLock);

	gpu_decrement_flush_count();
	return G3DKMDRET_SUCCESS;
}

void g3dKmdRegRestart(int taskID, int mode)
{
#ifdef G3DKMD_SNAPSHOT_PATTERN
	static unsigned int hasDumpSSP = G3DKMD_FALSE;
#endif
	struct g3dExeInst *pInst;

#ifdef G3DKMD_SUPPORT_PM
	/* if in shader debug mode and dfd debug mode, disable reset due to error recovery */
	if (gExecInfo.kmif_shader_debug_enable || gExecInfo.kmif_dfd_debug_enable)
		return;
#endif

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API, "taskID 0x%x, mode 0x%x\n", taskID, mode);

	pInst = (taskID < 0) ? gExecInfo.currentExecInst : g3dKmdTaskGetInstPtr(taskID);

#ifdef G3DKMD_SNAPSHOT_PATTERN
	if (!hasDumpSSP) {
		hasDumpSSP = G3DKMD_TRUE;
		g3dKmdSnapshotPattern(taskID, SSP_DUMP_CURRENT);
	}
#endif

#ifdef G3DKMD_SUPPORT_PM
	/* Call g3d hang callback function */
	if (likely(gExecInfo.kmif_ops.notify_g3d_hang))
		gExecInfo.kmif_ops.notify_g3d_hang();
#endif

#ifndef G3DKMD_RECOVERY_DETECT_ONLY
	/* reset G3D engine */
	g3dKmdRegRestartHW(taskID, mode);

#ifdef G3DKMD_SUPPORT_PM
	/* Call notify_reset callback function */
	if (likely(gExecInfo.kmif_ops.notify_g3d_reset))
		gExecInfo.kmif_ops.notify_g3d_reset();
#endif

	/* set current instance to IDLE, and release its hwChannel */
	if (pInst && pInst->state == G3DEXESTATE_RUNNING) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_API, "Reset instance info, Idx %d, hwChannel %d\n",
			g3dKmdExeInstGetIdx(pInst), pInst->hwChannel);

		gExecInfo.hwStatus[pInst->hwChannel].inst_idx = EXE_INST_NONE;
		gExecInfo.hwStatus[pInst->hwChannel].state = G3DKMD_HWSTATE_IDLE;
		pInst->hwChannel = G3DKMD_HWCHANNEL_NONE;
		pInst->time_slice = 0;
		pInst->state = G3DEXESTATE_IDLE;	/* set to IDLE, such that scheduler will pick it next time */

	}
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
	g3dKmdTriggerScheduler();
#else
	g3dKmdContextSwitch(1);
/*
    if (pInst != NULL)
    {
	if (g3dKmdQLoad(pInst))
	{
	    g3dKmdRegSendCommand(pInst->hwChannel, pInst);
	}
    }
*/
#endif
#endif /* G3DKMD_RECOVERY_DETECT_ONLY */
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_API, "[%s] out, taskID %d\n", __func__, taskID);
}

G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdCheckHWStatus(int param, int hw_action)
{
	unsigned int ret = 0;

	__SEM_LOCK(g3dLock);

	switch (hw_action) {
#ifdef FPGA_G3D_HW /* used in FPGA now */
	case G3DKMD_ACTION_HW_RESET:
/* not let user space trigger hw reset if reset by irq */
#ifndef G3DKMD_SUPPORT_AUTO_RECOVERY
		{
			struct g3dExeInst *pInst = gExecInfo.currentExecInst;

			if (!pInst) {
				__SEM_UNLOCK(g3dLock);
				return 1;
			}
			g3dKmdRegRestart(param, G3DKMD_RECOVERY_SKIP);
		}
#endif
		ret = 1;
		break;

	case G3DKMD_ACTION_CACHE_FLUSH_INVALIDATE:
#ifdef G3D_NONCACHE_BUFFERS
		G3DKMD_BARRIER();
#else
		g3dKmdCacheFlushInvalidate();
#endif
		ret = 1;
		break;

	case G3DKMD_ACTION_CHECK_HW_IDLE:
		ret = g3dKmdCheckHwIdle();
		break;

	case G3DKMD_ACTION_QUERY_HW_RESET_COUNT:
		{
			struct g3dTask *task = g3dKmdGetTask(param);

			ret = (task == NULL) ? 0 : task->hw_reset_cnt;
			break;
		}

	case G3DKMD_ACTION_QUERY_MMU_TRANSFAULT:
		ret = gExecInfo.mmuTransFault;
		break;
#endif
		/* maybe G3D hw workaround can be done in here */
	}
	__SEM_UNLOCK(g3dLock);

	return ret;
}

#if 0
G3DKMD_APICALL void G3DAPIENTRY g3dKmdSeviceISR(void)
{
	int hwChannel = 0; /* get hw queue index */
	int index = 0;
	int i;
	int pickTask = -1;
	int weighting = -1;
	struct g3dTask *task, *tempTask;
	unsigned int hwReadPtr = 0; /* read form hw */


	index = gExecInfo.hwTaskIndex[hwChannel];

	task = g3dKmdGetTask(index); /* finish task */


	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_API,
	       "g3dKmdSeviceISR  HW[%03d] task[%03d] readptr = 0x%08x\n", hwChannel, index, hwReadPtr);

	if (task) { /* task == null ==> error ?? */
		if (gExecInfo.enableHWCtxInterrupt) {
			task->hwChannel = -1;
			gExecInfo.hwTaskIndex[hwChannel] = -1;
			task->state = G3DTASKSTATE_WAITQUEUE;
		}

		task->missingCount = 0;
		if (task->htInfo) {
			task->htInfo->rdPtr =
			    g3dKmdCaculateIndex(task->htInfo, (unsigned int)hwReadPtr, task->htInfo->rdPtr);
			/* task->hwCurrent = hwReadPtr; */
			if (task->htInfo->wtPtr == task->htInfo->rdPtr) {
				if (task->remapIndex != -1) {
					task->state = G3DTASKSTATE_REMAP;
					g3dKmdDestroyHtInfo(task->htInfo);
					task->htInfo = NULL;
				}
				task->hwChannel = -1;
				gExecInfo.hwTaskIndex[hwChannel] = -1;
			}
		} else {
			/* task->hwCurrent= hwReadPtr; */
			task->hwChannel = -1;
			gExecInfo.hwTaskIndex[hwChannel] = -1;
		}
	}

	if (gExecInfo.enableHWCtxInterrupt) {
		/* choose priority */
		for (i = 0; i < gExecInfo.taskInfoCount; i++) {
			int check = 0;

			tempTask = g3dKmdGetTask(i);
			if (tempTask && (tempTask->state == G3DTASKSTATE_WAITQUEUE)) {
				if (tempTask->htInfo && (tempTask->htInfo->rdPtr != tempTask->htInfo->wtPtr))
					check = 1;

				if (check) {
					if ((tempTask->priorityWeighting + tempTask->missingCount) >  weighting) {
						weighting = tempTask->priorityWeighting + tempTask->missingCount;
						pickTask = i;
					}

				}
				tempTask->missingCount++;
			}
		}

		if (pickTask != -1) {
			tempTask = g3dKmdGetTask(pickTask);

			if (tempTask->htInfo) {
				task->hwChannel = hwChannel;
				g3dKmdRegSendCommand(hwChannel, tempTask);
				gExecInfo.hwTaskIndex[hwChannel] = pickTask;
			} else {
				/* error */
			}
		}
	}
	/* disable mask and clear */
}
#endif

G3DKMD_APICALL void G3DAPIENTRY g3dKmdDumpCommandBegin(int taskID, unsigned int dumpCount)
{
#ifdef PATTERN_DUMP
	_DEFINE_BYPASS_;
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
	unsigned delay_cnt = 0;
#endif

	struct g3dExeInst *pInst = g3dKmdTaskGetInstPtr(taskID);

	if (pInst == NULL)
		return;

	/* wait all htCmd of previous frame to be done by scheduler */
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
	while (!g3dKmdIsHtInfoEmpty(pInst->hwChannel, pInst->pHtDataForHwRead->htInfo) || delay_cnt < 10) {
		delay_cnt++;
		G3DKMD_SLEEP(1);
	}
#endif

	_BYPASS_(BYPASS_REG);

	_g3dKmdDumpCommandBegin(taskID, dumpCount);

	if ((g3dKmdCfgGet(CFG_DUMP_CE_MODE) != G3DKMD_DUMP_CE_MODE_DRIVER) ||
		(dumpCount == g3dKmdCfgGet(CFG_DUMP_START_FRM))) {
		/* in g3dKmdInitialize, the first riu file is not opened */
		/* so we dump registers to riu here, for the initialization in g3dKmdInitialize */

		/* this instructs test-bench, that the below riu settings are only for initialization */
		/* if test-bench is running for driver mode, it can skip the initialization except the start frame */
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfffa, 0x0);

		g3dKmdDumpInitRegister(taskID, dumpCount);

		g3dKmdInitQbaseBuffer(pInst);

#ifdef USE_GCE
		if (g3dKmdCfgGet(CFG_GCE_ENABLE)) {
			/* g3dKmdGcePrepareQld(pInst); */
			g3dKmdMmcqSetJumpTarget(pInst->mmcq_handle,
						g3dKmdMmcqGetAddr(pInst->mmcq_handle, MMCQ_ADDR_WRITE_PTR));
		} else
#endif
		{
			g3dKmdQLoad(pInst);
		}

		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfffa, 0x1);
	}

	_RESTORE_();
#endif
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdDumpCommandEnd(int taskID, unsigned int dumpCount)
{
#ifdef PATTERN_DUMP
	_g3dKmdDumpCommandEnd(taskID, dumpCount);
#endif
}


G3DKMD_APICALL void G3DAPIENTRY g3dKmdBufferFileDumpBegin(void *filp, unsigned int *root, unsigned int *len)
{
#ifdef G3DKMD_USE_BUFFERFILE
	return _g3dKmdBufferFileDumpBegin((struct file *)filp, (void **)root, len);
#else
	/* return NULL; */
#endif
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdBufferFileDumpEnd(void *filp, unsigned int isFree)
{
#ifdef G3DKMD_USE_BUFFERFILE
	return _g3dKmdBufferFileDumpEnd((struct file *)filp, isFree);
#else
	/* return NULL; */
#endif
}

/*
G3DKMD_APICALL void G3DAPIENTRY g3dKmdBufferFileReleasePidEverything(void)
{
#ifdef G3DKMD_USE_BUFFERFILE
	_g3dKmdBufferFileReleasePidEverything();
#endif
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdBufferFileReleaseEverything(void)
{
#ifdef G3DKMD_USE_BUFFERFILE
	_g3dKmdBufferFileReleaseEverything();
#endif
}
*/

G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdGetDrvInfo(unsigned int cmd, unsigned int arg,
							 void *buf, unsigned int bufSize, void *fd)
{
	unsigned int size = 0;

	UNUSED(fd);

	switch (cmd) {
#if defined(G3DKMD_SUPPORT_FDQ) && defined(PATTERN_DUMP)
	case G3DKMD_DRV_INFO_FDQ:
		{
			size = 8;
			if (size <= bufSize) {
				unsigned int *data = (unsigned int *)buf;

				data[0] = gExecInfo.fdq.patPhyAddr; /* fdq buffer phy addr */
				data[1] = gExecInfo.fdq_size; /* number of 64bit entries */
			}
		}
		break;
#endif

#ifdef CMODEL
	case G3DKMD_DRV_INFO_AREG:
		{
			size = G3D_REG_MAX;
			if (size <= bufSize) {
				void *areg = (void *)g3dKmdRegGetG3DRegister();

				if (areg)
					memcpy(buf, areg, size);
				else
					size = 0;
			}
		}
		break;

	case G3DKMD_DRV_INFO_QREG:
		{
			size = sizeof(SapphireQreg);
			if (size <= bufSize) {
				void *qreg = (void *)g3dKmdGetQreg(arg);

				if (qreg)
					memcpy(buf, qreg, size);
				else
					size = 0;
			}
		}
		break;

	case G3DKMD_DRV_INFO_UX_DWP:
		{
			size = sizeof(SapphireUxDwp);
			if (size <= bufSize) {
				void *dwpreg = (void *)g3dKmdRegGetUxDwpRegister();

				memcpy(buf, dwpreg, size);
			}
		}
		break;
#endif

	case G3DKMD_DRV_INFO_DFR_SIZE:
		{
			size = 16;
			if (size <= bufSize) {
				unsigned int *data = (unsigned int *)buf;
#if 1
				data[0] = g3dKmdCfgGet(CFG_DEFER_BINHD_SIZE);
				data[1] = g3dKmdCfgGet(CFG_DEFER_BINTB_SIZE);
				data[2] = g3dKmdCfgGet(CFG_DEFER_BINBK_SIZE);
				data[3] = g3dKmdCfgGet(CFG_DEFER_VARY_SIZE);
#else
				data[0] = G3DKMD_MAP_VX_BINHD_SIZE;
				data[1] = G3DKMD_MAP_VX_BINTB_SIZE;
				data[2] = G3DKMD_MAP_VX_BINBK_SIZE;
				data[3] = G3DKMD_MAP_VX_VARY_SIZE;
#endif

			}
		}
		break;


#ifdef PATTERN_DUMP
	case G3DKMD_DRV_INFO_MEM_USAGE:
		{
			size = 4;
			if (size <= bufSize) {
				/* extern unsigned int g_max_pa_addr, curPhyAddr; */
				unsigned int *data = (unsigned int *)buf;
#ifdef G3DKMD_PHY_HEAP
				if (g3dKmdCfgGet(CFG_PHY_HEAP_ENABLE))
					data[0] = g_max_pa_addr;
				else
#endif
					data[0] = curPhyAddr;
			}
		}
		break;
#endif

	case G3DKMD_DRV_INFO_EXEC_MODE:
		{
			size = 4;
			if (size <= bufSize) {
				unsigned int *data = (unsigned int *)buf;

				data[0] = gExecInfo.exeuteMode;
			}
		}
		break;

	case G3DKMD_DRV_INFO_PM:
		{
			size = 8;
			if (size <= bufSize) {
				unsigned int *data = (unsigned int *)buf;

				data[0] = gExecInfo.pm.patPhyAddr;
				data[1] = gExecInfo.pm.size;
			}
		}
		break;

#ifdef G3DKMD_SNAPSHOT_PATTERN
	case G3DKMD_DRV_INFO_SSP:
		{
			size = sizeof(int);
			if (size <= bufSize)
				g3dKmdSSPControl(G3DKMD_TRUE, arg, buf, bufSize);
		}
		break;
#endif

	case G3DKMD_DRV_INFO_G3DID:
	{
		size = sizeof(unsigned int);
		if (size <= bufSize) {
			unsigned int *data = (unsigned int *) buf;

			data[0] = g3dKmdCheckDeviceID();
		}
	}
		break;
	default:
		size = 0;
		break;
	}

	return size;
}

G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdSetDrvInfo(unsigned int cmd, unsigned int arg,
							 void *buf, unsigned int bufSize, void *fd)
{
	unsigned int rtnVal = G3DKMD_TRUE;

	UNUSED(bufSize);
	UNUSED(fd);

	switch (cmd) {
#ifdef YL_HW_DEBUG_DUMP
	case G3DKMD_DRV_INFO_DFD_SETTING:
		{
			unsigned int *data = (unsigned int *)buf;

			if (bufSize < sizeof(unsigned int) * 3) {
				rtnVal = G3DKMD_FALSE;
				break;
			}
			/* set debug frame id */
			g3dKmdHwDbgSetFrameID(data[0], data[1]);

#ifdef YL_HW_DEBUG_DUMP_TEMP
			g3dKmdHwDbgSetRepeatTimes(data[2]);
			g3dKmdHwDbgSetDumpReg(arg);
#endif
		}
		break;
#endif

#ifdef G3DKMD_SNAPSHOT_PATTERN
	case G3DKMD_DRV_INFO_SSP_SETTING:
		{
			rtnVal = g3dKmdSSPControl(G3DKMD_FALSE, arg, buf, bufSize);
			rtnVal = (rtnVal == G3DKMDRET_SUCCESS) ? G3DKMD_TRUE : G3DKMD_FALSE;
		}
		break;
#endif


#if defined(ENABLE_G3DMEMORY_LINK_LIST)
	case G3DKMD_DRV_INFO_MEMLIST_ADD:
		{
			g3dkmd_IOCTL_DRV_INFO_MEMLIST *mem;

			mem = (g3dkmd_IOCTL_DRV_INFO_MEMLIST *) buf;
			_g3dKmdMemList_Add(fd, (G3Dint) g3dkmd_get_pid(), (void *)mem->va,
					   mem->size, mem->hwAddr, G3D_Mem_User);
		}
		break;
	case G3DKMD_DRV_INFO_MEMLIST_DEL:
		{
			g3dkmd_IOCTL_DRV_INFO_MEMLIST *mem;

			mem = (g3dkmd_IOCTL_DRV_INFO_MEMLIST *) buf;
			_g3dKmdMemList_Del((void *)mem->va);
		}
		break;
#endif

	default:
		break;
	}

	return rtnVal;
}

#if defined(WIN32) || (defined(linux) && defined(linux_user_mode))
G3DKMD_APICALL void G3DAPIENTRY g3dKmdConfig(G3DKMD_CFG *cfg)
{
	g3dKmdCfgInit();

	if (cfg) {
		g3dKmdCfgSet(CFG_LTC_ENABLE, cfg->ltc_enable);
		g3dKmdCfgSet(CFG_MMU_ENABLE, cfg->mmu_enable);
		g3dKmdCfgSet(CFG_MMU_VABASE, cfg->mmu_vabase);
		g3dKmdCfgSet(CFG_DEFER_VARY_SIZE, cfg->vary_size);
		g3dKmdCfgSet(CFG_DEFER_BINHD_SIZE, cfg->binhd_size);
		g3dKmdCfgSet(CFG_DEFER_BINBK_SIZE, cfg->binbk_size);
		g3dKmdCfgSet(CFG_DEFER_BINTB_SIZE, cfg->bintb_size);
		g3dKmdCfgSet(CFG_GCE_ENABLE, cfg->gce_enable);

#ifdef G3DKMD_PHY_HEAP
		g3dKmdCfgSet(CFG_PHY_HEAP_ENABLE, cfg->phy_heap_enable);
#endif
		g3dKmdCfgSet(CFG_DUMP_ENABLE, cfg->dump_enable);
		g3dKmdCfgSet(CFG_MEM_SHIFT_SIZE, cfg->mem_shift_size);
		g3dKmdCfgSet(CFG_VX_BS_HD_TEST, cfg->vx_bs_hd_test);
		g3dKmdCfgSet(CFG_CQ_PMT_MD, cfg->cq_pmt_md);
		g3dKmdCfgSet(CFG_BKJMP_EN, cfg->bkjmp_en);
	}
}
#endif

G3DKMD_APICALL void G3DAPIENTRY g3dKmdPatternConfig(G3DKMD_PATTERN_CFG *cfg)
{
	if (cfg) {
		g3dKmdCfgSet(CFG_DUMP_START_FRM, cfg->dump_start_frm);
		g3dKmdCfgSet(CFG_DUMP_END_FRM, cfg->dump_end_frm);
		g3dKmdCfgSet(CFG_DUMP_CE_MODE, cfg->dump_ce_mode);
	}
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdUxRegDebug(unsigned int val1, unsigned int val2)
{
	g3dKmdCfgSet(CFG_UXCMM_REG_DBG_EN, val1);
	g3dKmdCfgSet(CFG_UXDWP_REG_ERR_DETECT, val2);
}

G3DKMD_APICALL void G3DAPIENTRY g3dKmdPerfModeEnable(void)
{
	gpu_runtime_pm_disable();

	if (gExecInfo.kmif_ops.notify_g3d_performance_mode)
		gExecInfo.kmif_ops.notify_g3d_performance_mode(1);
	else
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API,
		       "notify_g3d_performance_mode() is not found!\n");
}
EXPORT_SYMBOL(g3dKmdPerfModeEnable);

G3DKMD_APICALL void G3DAPIENTRY g3dKmdPerfModeDisable(void)
{
	gpu_runtime_pm_enable();

	if (gExecInfo.kmif_ops.notify_g3d_performance_mode)
		gExecInfo.kmif_ops.notify_g3d_performance_mode(0);
	else
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API,
		       "notify_g3d_performance_mode() is not found!\n");
}
EXPORT_SYMBOL(g3dKmdPerfModeDisable);

#ifdef G3DKMD_SNAPSHOT_PATTERN
G3DKMD_APICALL unsigned int G3DAPIENTRY g3dKmdSSPControl(int bIsQuery, int cmd, void *pBuf, unsigned int bufSize)
{
	G3DKMDRETVAL retval;

	__SEM_LOCK(g3dLock);

	switch (cmd) {
	case G3DKMD_DRV_SSPCMD_DUMP:
		{

			G3DKMD_SSP_DUMP_PARAM *param = (G3DKMD_SSP_DUMP_PARAM *) pBuf;

			KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_API,
			       "[SSP]%s taskID = %d, mode = %d\n", __func__, param->taskID, param->mode);

			retval = g3dKmdSnapshotPattern(param->taskID, param->mode);

			break;
		}

	case G3DKMD_DRV_SSPCMD_ENABLE:
		{
			retval = g3dKmdSSPEnable(bIsQuery, (int *)pBuf);
			break;
		}

	case G3DKMD_DRV_SSPCMD_PATH:
		{
			retval = g3dkmdSSPPath(bIsQuery, (char *)pBuf, bufSize);
			break;
		}

	default:
		{
			retval = G3DKMDRET_ERROR;
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_API, "[SSP]%s, unknown cmd 0x%x\n", __func__, cmd);
			break;
		}

	}


	__SEM_UNLOCK(g3dLock);

	return retval;
}
#endif


#if defined(_WIN32) && defined(G3DKMD_EXPORTS)

BOOL WINAPI DllMain(__in HINSTANCE hinstDLL, __in DWORD fdwReason, __in LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		/* g3dKmdInitialize(); */
		break;

	case DLL_PROCESS_DETACH:
		/* g3dKmdTerminate(); */
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;
	}

	return G3DKMD_TRUE;
}
#endif
