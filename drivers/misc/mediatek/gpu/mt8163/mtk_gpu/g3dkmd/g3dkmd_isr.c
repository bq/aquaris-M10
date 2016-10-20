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

#ifdef G3DKMD_SUPPORT_ISR

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)
#include <windows.h>
#include <stdlib.h>
#elif defined(linux) && defined(linux_user_mode)	/* linux user space */
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#else
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#define ISR_USING_TASKLET
#endif

#include "sapphire_reg.h"
#include "g3dkmd_api.h"
#include "g3dkmd_task.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_util.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_isr.h"
#include "g3dkmd_fdq.h"
#include "g3dkmd_pattern.h"
#include "g3dkmd_macro.h"
#include "g3dkmd_signal.h"
#include "g3dkmd_scheduler.h"
#if defined(YL_MET) || defined(G3DKMD_MET_CTX_SWITCH)
#include "g3dkmd_met.h"
#endif
#include "g3dkmd_power.h"

#include "test/tester_non_polling_flushcmd.h"
#include "platform/g3dkmd_platform_waitqueue.h"

/* static variable */

#ifdef FPGA_G3D_HW
unsigned long __yl_irq;
static __DEFINE_SPIN_LOCK(isrLock);
static __DEFINE_SPIN_LOCK(hangLock);
#endif

void lock_isr(void)
{
#ifdef FPGA_G3D_HW
	/* use lock_BH will disable softirq and not allow to lock the code */
	/* which will sleep like kmalloc/vmalloc, but pr_debug will not sleep */
	__SPIN_LOCK_BH(isrLock);
#endif
}

void unlock_isr(void)
{
#ifdef FPGA_G3D_HW
	__SPIN_UNLOCK_BH(isrLock);
#endif
}

void lock_hang_detect(void)
{
#ifdef FPGA_G3D_HW
	__SPIN_LOCK(hangLock);
#endif
}

void unlock_hang_detect(void)
{
#ifdef FPGA_G3D_HW
	__SPIN_UNLOCK(hangLock);
#endif
}

#if defined(G3DKMD_RECOVERY_BY_IRQ) && !defined(linux_user_mode)
static void g3dKmdIsrWork(struct work_struct *work)
{
	/* we use work to let we able to sleep to execute following code */
	struct g3dExeInst *pInst = NULL;

	lock_g3d();

	pInst = gExecInfo.currentExecInst;

	disable_irq(G3DHW_IRQ_ID);

	if (!pInst)
		goto ISR_WORK_EXIT;


	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ISR, "[ISR_RECOVERY] skip HT\n");
	g3dKmdRegRestart(-1, G3DKMD_RECOVERY_SKIP);


ISR_WORK_EXIT:

	enable_irq(G3DHW_IRQ_ID);
	unlock_g3d();
}

static DECLARE_WORK(mtk_gpu_work, g3dKmdIsrWork);
#endif

#ifdef ISR_USING_TASKLET
static void g3dKmdIsrTop(unsigned long var);
static DECLARE_TASKLET(mtk_gpu_tasklet, g3dKmdIsrTop, (unsigned long)(&gExecInfo));
static inline void g3dKmdDisableTasklet(void)
{
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "\n");
	tasklet_disable(&mtk_gpu_tasklet);
}

static inline void g3dKmdEnableTasklet(void)
{
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "\n");
	tasklet_enable(&mtk_gpu_tasklet);
}

#ifdef G3D_HW
static irqreturn_t g3dKmdIsrHandler(int irq, void *dev_id)
{
	/* according to Ying's suggestion */
	if (dev_id != (void *)(&gExecInfo))
		return IRQ_NONE;

	/* use _nosync to avoid deadlock */
	disable_irq_nosync(G3DHW_IRQ_ID);
	tasklet_schedule(&mtk_gpu_tasklet);
	return IRQ_HANDLED;
}
#endif
#endif

void g3dKmdSuspendIsr(void)
{
#ifdef ISR_USING_TASKLET
	g3dKmdDisableTasklet();
#endif
}

void g3dKmdResumeIsr(void)
{
#ifdef ISR_USING_TASKLET
	g3dKmdEnableTasklet();
#endif
}

#if defined(TEST_NON_POLLING_FLUSHCMD)
/* because g3dKmdIsrTop is static, add a wrapper function. */
void g3dKmdInvokeIsrTop(void)
{
	g3dKmdIsrTop(0);
}
#endif

#if (defined(linux) && !defined(linux_user_mode)) && defined(ENABLE_NON_POLLING_FLUSHCMD)
static INLINE void _g3dKmdWakeUpFlushCmdWq(void)
{
	/* [Louis 2014/08/14]: There may some Ht slots released, so wake up the processes
	 *                     waiting the ht slots. ( g3dkmd_api.c::g3dkmdFlushCommand())
	 */
	tester_wake_up();
	wake_up_interruptible(&gExecInfo.s_flushCmdWq);
}
#else
#define _g3dKmdWakeUpFlushCmdWq()
#endif


#if defined(ENABLE_QLOAD_NON_POLLING)
static INLINE void _g3dKmdWakeUpQLoadWq(void)
{
	gExecInfo.s_isHwQloadDone = G3DKMD_TRUE;
	g3dkmd_wake_up_interruptible(&gExecInfo.s_QLoadWq);
}
#else
#define _g3dKmdWakeUpQLoadWq()
#endif


static INLINE void _g3dKmdDoFrameEnd(void)
{
	_g3dKmdWakeUpFlushCmdWq();
}

static INLINE void _g3dKmdDoFrameFlush(void)
{
	_g3dKmdWakeUpQLoadWq();
}

#ifdef G3DKMD_SUPPORT_SYNC_SW
static INLINE void _g3dKmdDoSwSync(void)
{
	while (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, VAL_IRQ_PA_SYNC_SW, 5) != 0) {
		/* check whether pa_flush_ltc_done (areg 0x194) [1] is on */
		while (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FLUSH_LTC_DONE, MSK_AREG_PA_SYNC_SW_DONE,
			SFT_AREG_PA_SYNC_SW_DONE) != 0) {
			/* set pa_flush_ltc_set (areg 0x190)[1] on */
			g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FLUSH_LTC_SET, 1,
				       MSK_AREG_PA_SYNC_SW_EN, SFT_AREG_PA_SYNC_SW_EN);
		}
		/* set pa_flush_ltc_set [1] off */
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FLUSH_LTC_SET, 0, MSK_AREG_PA_SYNC_SW_EN,
			       SFT_AREG_PA_SYNC_SW_EN);

		/* clear all isq */
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 1, VAL_IRQ_PA_SYNC_SW, 5);
	}
}
#else
#define _g3dKmdDoSwSync()
#endif

#ifdef G3DKMD_SUPPORT_CL_PRINT
/* ToDo: it's a long latency work, should move to workqueue */


static INLINE void _g3dKmdDoCLPrintf(void)
{
	struct wave_cnt_t {
		unsigned int taskID;
		unsigned int appear_cnt;
	};

	unsigned int syscall_wp0_info = 0;
	unsigned int syscall_wp2_info = 0;
	unsigned int syscall_wp_pack = 0;
	unsigned int iter_wave = 0;
	unsigned int iter_task_idx = 0;
	unsigned int valid_num_exist_task_id = 0;

/*	struct wave_cnt_t running_task[gExecInfo.taskInfoCount]; */
	struct wave_cnt_t running_task[0x100];

	for (iter_task_idx = 0; iter_task_idx < (unsigned)gExecInfo.taskInfoCount; ++iter_task_idx) {
		struct g3dTask *task = g3dKmdGetTask(iter_task_idx);

		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ISR, "Reset Task[%u](%p)\n", iter_task_idx, task);

		if (!task)
			continue;
		task->clPrintfInfo.waveID = 0;
		running_task[iter_task_idx].taskID = (unsigned)-1;
		running_task[iter_task_idx].appear_cnt = 0;
	}

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ISR, "UX Dwp0 Resp IRQ Read\n");

	while (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 0x100000, 20) != 0) {
		/* read printf interrupt flag from OS.*/

		/* step 1 write one clear */
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 0x1, 0x100000, 20);
		/* reset OS printf interrupt flag. */

		/* step 2 read syscall_wp0_info & syscall_wp1_info in UX_DWP_REG */
		/* do we need to fire them first before read them? */
		syscall_wp0_info = g3dKmdRegRead(G3DKMD_REG_UX_DWP, REG_UX_DWP_SYSCALL_INFO_0,
					MSK_UX_DWP_SYSCALL_WP0_INFO, SFT_UX_DWP_SYSCALL_WP0_INFO); /* [11: 0] */
		syscall_wp2_info = g3dKmdRegRead(G3DKMD_REG_UX_DWP, REG_UX_DWP_SYSCALL_INFO_0,
					MSK_UX_DWP_SYSCALL_WP2_INFO, SFT_UX_DWP_SYSCALL_WP2_INFO);
					/* [27:16]   1111 1111 1111 0000 0000 0000 0000 */
		syscall_wp_pack = ((syscall_wp2_info & 0xFFF) << 12) | (syscall_wp0_info & 0xFFF);

		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ISR, "%s G3DKMD_REG_UX_DWP(wp0: 0x%x, wp2: 0x%x)\n",
			__func__, syscall_wp0_info, syscall_wp2_info);

		for (iter_wave = 0; iter_wave < 24; ++iter_wave) {
			unsigned int sft = (0x1 << iter_wave);
			unsigned int hw_cid = 0;
			unsigned int hw_cid1 = 0;
			unsigned int hw_cid2 = 0;
			unsigned int hw_cid3 = 0;
			unsigned int ctx_global = 0;
			unsigned int cl_proc_id = 0;
			unsigned int cl_ctx_sid = 0;
			unsigned int iter_ctx = 0;	/* how many ctx are mapped to the same task */

			if ((syscall_wp_pack & (0x1 << iter_wave)) == 0) /* skip waves which are not activated */
				continue;
			else
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ISR,
					"handle %dth wave because it's 1\n", iter_wave);

			hw_cid1 = g3dKmdRegRead(G3DKMD_REG_UX_DWP, REG_UX_DWP_SYSCALL_INFO_1, sft, iter_wave);
			hw_cid2 = g3dKmdRegRead(G3DKMD_REG_UX_DWP, REG_UX_DWP_SYSCALL_INFO_2, sft, iter_wave);
			hw_cid3 = g3dKmdRegRead(G3DKMD_REG_UX_DWP, REG_UX_DWP_SYSCALL_INFO_3, sft, iter_wave);

			hw_cid = hw_cid1 | ((0x1 & hw_cid2) << 1) | ((0x1 & hw_cid3) << 2);
			/* try to get the taskID */
			/* when we programming sel mode, do we have to clear hw_cid value ? */
			/* combine sel_mode and hw_cid together, Set set sel =8 for process id access */
			g3dKmdRegWrite(G3DKMD_REG_UX_CMN, REG_UX_CMN_CX_GLOBAL_READ_0 , ((0x8 << 4) | hw_cid),
				MSK_UX_CMN_CXM_GLOBAL_CID | MSK_UX_CMN_CXM_GLOBAL_SEL, SFT_UX_CMN_CXM_GLOBAL_CID);

			ctx_global = g3dKmdRegRead(G3DKMD_REG_UX_CMN, REG_UX_CMN_CX_GLOBAL_READ_1,
						MSK_UX_CMN_CXM_GLOBAL_DATA, SFT_UX_CMN_CXM_GLOBAL_DATA); /* [31: 0] */
			cl_proc_id = ctx_global & 0xFF;	/* [7: 0] */

			/* try to get the sid */
			cl_ctx_sid = (ctx_global >> 8) & 0xFF;	/* [15: 8] */

			/* cl_proc_id = 1; */
			/* existed_task_id is empty or no such task id in it */
#if 0
			if (valid_num_exist_task_id == 0) {
				running_task[valid_num_exist_task_id].taskID = cl_proc_id;
				running_task[valid_num_exist_task_id].appear_cnt = 1;
				valid_num_exist_task_id = 1;
			}
#endif
			/* get corresponding activated wave cnt */
			for (iter_task_idx = 0;
				iter_task_idx < valid_num_exist_task_id; /* valid_num_exist_task_id init to be 0 */
				++iter_task_idx) {

				/* found the wanted task and get the appear cnt */
				if (cl_proc_id == running_task[iter_task_idx].taskID) {
					/* apear_cnt will be at least 1 if ever met */
					iter_ctx = running_task[iter_task_idx].appear_cnt;
					running_task[iter_task_idx].appear_cnt++;
					break;
				}
			}

			if (iter_ctx == 0) { /* this is a brand new task */
				running_task[valid_num_exist_task_id].taskID = cl_proc_id;
				running_task[valid_num_exist_task_id].appear_cnt = 1;
				valid_num_exist_task_id++;
			}
			/* step 3 record data to g3dTask */
			{
				struct g3dTask *task = g3dKmdGetTask(cl_proc_id);

				if (!task) {
					KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ISR,
						"TaskID: %u is not existed\n", cl_proc_id);
					continue;
				}
				task->clPrintfInfo.isIsrTrigger = 1;
				task->clPrintfInfo.waveID |= (0x1 << iter_wave);
				task->clPrintfInfo.ctxsID = cl_ctx_sid;

				g3dkmd_wake_up_interruptible(&gExecInfo.s_waitqueue_head_cl);
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL, "g3dkmd_wake_up_interruptible\n");


				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ISR,
					"taskID(%p)[%u]->waveID[0] = 0x%x context global: 0x%x\n",
					task, cl_proc_id, task->clPrintfInfo.waveID, ctx_global);
			}

		} /* for (iter_wave = 0; iter_wave < 24; ++iter_wave) */

	} /* while (g3dKmdRegRead(REG_AREG_IRQ_STATUS_RAW, 0x1, 0x100000, 20) != 0) */

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ISR, "%s done\n", __func__);
/* [signal] */
#if 0
#ifdef G3DKMD_SIGNAL_CL_PRINT
	for (iter_task = 0; iter_task < valid_num_exist_task_id; ++iter_task) {
		unsigned int polling_cnt = 0;
		unsigned int taskID = running_task[iter_task].taskID;

		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR,
		       "ready to trigger a signal for taskID = %d and pid = %d\n", taskID,
		       g3dKmdGetTask(taskID)->pid);

		++cl_print_write_stamp;
		trigger_cl_printf_stamp(g3dKmdGetTask(taskID)->pid, cl_print_write_stamp);

		/* need to wait for CL user mode driver finish, then we can continue */
		/* is it good to do the busy waiting here? */
		do {
			if (polling_cnt > SIGNAL_CL_PRINT_MAX_POLLING_CNT) {
				KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR,
				       "timeout, fire signal again to pid %d\n",
				       g3dKmdGetTask(taskID)->pid);
				trigger_cl_printf_stamp(g3dKmdGetTask(taskID)->pid, cl_print_write_stamp);
				polling_cnt = 0;
			} else {
				++polling_cnt;
			}

			G3DKMD_SLEEP(1);
		} while (cl_print_write_stamp != gExecInfo.cl_print_read_stamp);
	}
#endif /* G3DKMD_SIGNAL_CL_PRINT */
#endif /* 0 */

	for (iter_task_idx = 0; iter_task_idx < valid_num_exist_task_id; ++iter_task_idx) {
		running_task[iter_task_idx].taskID = (unsigned)-1;
		running_task[iter_task_idx].appear_cnt = 0;
	}

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "%s resumes hw success\n", __func__);
}
#else
#define _g3dKmdDoCLPrintf()
#endif

#if defined(ISR_USING_TASKLET)
static INLINE void _g3dKmdDoHangErrorHandle(unsigned int isr_mask)
{
	if (isr_mask & VAL_IRQ_G3D_HANG)
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ISR, "[ISR] Detect G3D Hang\n");

	if (isr_mask & VAL_IRQ_CQ_ECC) {
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x0, MSK_AREG_MCU2G3D_SET,
			       SFT_AREG_MCU2G3D_SET);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x1, MSK_AREG_MCU2G3D_SET,
			       SFT_AREG_MCU2G3D_SET);

		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ISR,
		       "[ISR] Detect ECC on, ecc_htbuf_rptr 0x%x\n", g3dKmdRegRead(G3DKMD_REG_G3D,
										   REG_AREG_QUEUE_STATUS,
										   MSK_AREG_ECC_HTBUF_RPTR,
										   SFT_AREG_ECC_HTBUF_RPTR));
	}
#if defined(G3DKMD_RECOVERY_BY_IRQ)
	{
		struct g3dExeInst *pInst = gExecInfo.currentExecInst;

		if (pInst) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ISR,
				"Before check hang, InstIdx %d, HwChannel %d, State %d\n",
				g3dKmdExeInstGetIdx(pInst), pInst->hwChannel,
				gExecInfo.hwStatus[pInst->hwChannel].state);
		}

		if (pInst && (pInst->hwChannel != G3DKMD_HWCHANNEL_NONE)) {
			lock_hang_detect();

			if (gExecInfo.hwStatus[pInst->hwChannel].state != G3DKMD_HWSTATE_HANG) {
				gExecInfo.hwStatus[pInst->hwChannel].state = G3DKMD_HWSTATE_HANG;

#if !defined(linux_user_mode)	/* enable workqueue */
				/* turn it off in here but it will enable them in g3dKmdRegReset */
				g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASK,
					       VAL_IRQ_G3D_HANG, VAL_IRQ_G3D_HANG,
					       SFT_AREG_IRQ_MASK);
				g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASK,
					       VAL_IRQ_CQ_ECC, VAL_IRQ_CQ_ECC, SFT_AREG_IRQ_MASK);
				schedule_work(&mtk_gpu_work);
#else
				lock_g3d();
				g3dKmdRegRestart(-1, G3DKMD_RECOVERY_RETRY);
				unlock_g3d();
#endif
			}
			unlock_hang_detect();
		}
	}
#endif
}
#endif

static INLINE void _g3dKmdClearIntStatus(void)
{
#if defined(G3DKMD_SUPPORT_ECC) && defined(G3DKMD_RECOVERY_BY_IRQ)
	/* if ECC interrupt is cleared, HW will continue doing the next ht-entry */
	/* since our strategy for ECC is HW reset, and retry/skip current ht-entry, */
	/* we don't have to let HW auto processing next ht-entry, we just do not clear ECC interrupt */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW,
		       (MSK_AREG_IRQ_RAW & (~VAL_IRQ_CQ_ECC)), MSK_AREG_IRQ_RAW, SFT_AREG_IRQ_RAW);
#else
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, MSK_AREG_IRQ_RAW, MSK_AREG_IRQ_RAW,
		       SFT_AREG_IRQ_RAW);
#endif

#ifndef G3D_HW
	/* [Louis 2014/09/10]: Fixed the kernel hang problem.
	 *	Using disable Tasklet during QLoad waiting areg signal to avoid softIrq causes race condition problem
	 */
	/* [Louis 2014/09/01]: Remove if without TEST,
	 *	because the following two HW simulation will cause regress multiple thread (>=5) fail.(kernel Hang!)
	  */
	/* [Louis 2014/08/20]: To simulate HW, aftter interrupt handler, */
	/* Set all flags to 0 manually. */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 0, MSK_AREG_IRQ_RAW,
		       SFT_AREG_IRQ_RAW);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASKED, 0, MSK_AREG_IRQ_MASKED,
		       SFT_AREG_IRQ_MASKED);
#endif
}

#if defined(G3D_HW) && !defined(ISR_USING_TASKLET)
static irqreturn_t g3dKmdIsrTop(int irq, void *dev_id)
#else
static void g3dKmdIsrTop(unsigned long var)
#endif
{
	_DEFINE_BYPASS_;

	unsigned int isr_mask;

	_BYPASS_(BYPASS_RIU);
	isr_mask = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASKED, MSK_AREG_IRQ_MASKED,
			  SFT_AREG_IRQ_MASKED);

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "mask 0x%x\n", isr_mask);

	if (isr_mask & VAL_IRQ_PA_FRAME_END) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "Frame End!\n");
#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "FRM-END_ISR", 1);
		met_tag_oneshot(0x8163, "FRM-END_ISR", 0);
#endif
		_g3dKmdDoFrameEnd();
	}
#ifdef G3DKMD_SUPPORT_PM
	if (gExecInfo.kmif_dfd_debug_enable) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "DFD Debug Enable\n");
		gExecInfo.kmif_ops.notify_g3d_dfd_interrupt(isr_mask);
	}
#endif
	if (isr_mask & VAL_IRQ_G3D_MMCE_FRAME_FLUSH) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "VAL_IRQ_G3D_MMCE_FRAME_FLUSH.\n");
#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "FRM-FLUSH_ISR", 1);
		met_tag_oneshot(0x8163, "FRM-FLUSH_ISR", 0);
#endif
		_g3dKmdDoFrameFlush();
	}

	if (isr_mask & VAL_IRQ_PA_DATA_QUEUE_NOT_EMPTY) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "FDQ IRQ Read!\n");
#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "FDQ_ISR", 1);
#endif
		g3dKmdFdqReadQ();	/* Is it suitable to move to workqueue ? */
#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "FDQ_ISR", 0);
#endif

#if defined(G3DKMD_SCHEDULER_OS_CTX_SWTICH)
		{
			struct g3dExeInst *pInst = gExecInfo.currentExecInst;

			if (pInst &&
				g3dKmdIsFiredHtInfoDone(pInst->hwChannel, pInst->pHtDataForHwRead->htInfo)) {
#ifdef G3DKMD_MET_CTX_SWITCH
				met_tag_oneshot(0x8163, "ISRCurEmpty_FORCE-TRIG", 1);
#endif
				g3dKmdTriggerScheduler();
#ifdef G3DKMD_MET_CTX_SWITCH
				met_tag_oneshot(0x8163, "ISRCurEmpty_FORCE-TRIG", 0);
#endif
			}
		}
#endif
	}

	if (isr_mask & VAL_IRQ_UX_CMN_RESPONSE)
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "UX Cmn Resp IRQ Read\n");

#ifdef G3DKMD_SUPPORT_SYNC_SW	/* SYNC_SW function is not supported by HW anymore */
	if (isr_mask & VAL_IRQ_PA_SYNC_SW) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "Sync SW IRQ Read\n");
		_g3dKmdDoSwSync();
	}
#endif

	/* [CL printf] */
	if (isr_mask & VAL_IRQ_UX_DWP0_RESPONSE)
		_g3dKmdDoCLPrintf();

	/* if (isr_mask & VAL_IRQ_UX_DWP0_RESPONSE) */

	if ((isr_mask & VAL_IRQ_G3D_HANG) || (isr_mask & VAL_IRQ_CQ_ECC)) {
		_g3dKmdDoHangErrorHandle(isr_mask);

		if (KMDDPFCHECK(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_ISR)) {
			struct g3dExeInst *pInst = gExecInfo.currentExecInst;

			KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_DDFR | G3DKMD_MSG_ISR,
			       "IRQ G3D HANG! InstIdx: %u, channel: %d, hwState: %d\n",
			       g3dKmdExeInstGetIdx(pInst), pInst->hwChannel,
			       gExecInfo.hwStatus[pInst->hwChannel].state);
		}
	}
	/* Clear interrupt status */
	_g3dKmdClearIntStatus();

	_RESTORE_();

#ifdef G3D_HW
#ifdef ISR_USING_TASKLET
	enable_irq(G3DHW_IRQ_ID);
#else /* ! ISR_USING_TASKLET */
	return IRQ_HANDLED;
#endif /* ISR_USING_TASKLET */
#endif /* G3D_HW */
}

#ifndef G3D_HW
#ifdef ISR_USING_TASKLET
void g3dKmdIsrTrigger(void)
{
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "\n");
	tasklet_schedule(&mtk_gpu_tasklet);
}
#else
void g3dKmdIsrTrigger(void)
{
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "\n");
	g3dKmdIsrTop(0);
}
#endif
#endif /* ! G3D_HW */

G3DKMD_BOOL g3dKmdIsrInstall(void)
{
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "\n");

#if defined(linux) && !defined(linux_user_mode) && defined(G3D_HW)
	{
		int code;

		/* clean all previous irq status // SW reset will not clean it */
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, MSK_AREG_IRQ_RAW,
			       MSK_AREG_IRQ_RAW, SFT_AREG_IRQ_RAW);

#ifdef ISR_USING_TASKLET
		code = request_irq(G3DHW_IRQ_ID, g3dKmdIsrHandler, IRQF_TRIGGER_LOW, G3DHW_IRQ_NAME,
				 (void *)(&gExecInfo));
#else
		code = request_irq(G3DHW_IRQ_ID, g3dKmdIsrTop, IRQF_TRIGGER_LOW, G3DHW_IRQ_NAME,
				 (void *)(&gExecInfo));
#endif
		if (code != 0) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ISR, "request_irq fail, err %d\n", code);
			YL_KMD_ASSERT(0);
			return G3DKMD_FALSE;
		}

		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "request_irq %d success\n",
		       G3DHW_IRQ_ID);

		/*
		 * after request_irq, irq is enabled already but if more enable_irq will
		 * cause the warning "Unbalanced enable for irq"
		 */
		/* enable_irq(G3DHW_IRQ_ID); */
	}
#endif

	return G3DKMD_TRUE;
}

void g3dKmdIsrUninstall(void)
{
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ISR, "\n");

#if defined(linux) && !defined(linux_user_mode) && defined(G3D_HW)
	disable_irq(G3DHW_IRQ_ID);
	free_irq(G3DHW_IRQ_ID, (void *)(&gExecInfo));
#endif
}
#endif /* G3DKMD_SUPPORT_ISR */
