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

#include <linux/export.h>
#include "g3dkmd_define.h"

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
/******************************************************************************
******************************************************************************/
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#include <linux/timer.h>
#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#include "g3dkmd_cfg.h"		/* for g3dKmdCfgGet() */
#include "sapphire_reg.h"	/* for SapphireQreg */
#include "g3dkmd_engine.h"	/* for g3dKmdRegRead(), g3dKmdRegWrite() */
#include "g3dkmd_fdq.h"		/* for g3dKmdFdqBackupBufPtr(), ... */
#include "g3dkmd_task.h"	/* for g3dKmdIsHtInfoEmpty() */
#include "g3dkmd_file_io.h"	/* for KMDDPF() */
#include "sapphire_areg.h"	/* for REG_AREG_PA_IDLE_STATUS0 */
#include "g3dkmd_scheduler.h"	/* for g3dKmdTriggerScheduler() */
#include "g3dkmd_recovery.h"	/* for g3dKmd_recovery_timer_enable(), g3dKmd_recovery_timer_disable() */
#include "g3dkmd_api.h"		/* for lock_g3dPwrState(), unlock_g3dPwrState() */
#include "g3dkmd_power.h"
#include "gs_gpufreq.h"

#if (USE_MMU == MMU_USING_WCP)
#if defined(FPGA_mt6752_fpga) || defined(FPGA_fpga8163) || defined(FPGA_fpga8163_64) || defined(MT8163)
#include <m4u.h>
#define GPU_IOMMU_ID 2
#elif defined(FPGA_mt6592_fpga)
#include "mach/gpu_mmu.h"
#define GPU_IOMMU_ID
#else
#error "currently mt6752/mt6592 FPGA are supported only"
#endif
#endif


/******************************************************************************
* Macros
******************************************************************************/
#define MSG_LEVEL_ERROR                     (G3DKMD_LLVL_ERROR | G3DKMD_MSG_POWER)
#define MSG_LEVEL_NOTICE                    (G3DKMD_LLVL_NOTICE | G3DKMD_MSG_POWER)
#define MSG_LEVEL_INFO                      (G3DKMD_LLVL_INFO | G3DKMD_MSG_POWER)
#define MSG_LEVEL_DEBUG                     (G3DKMD_LLVL_DEBUG | G3DKMD_MSG_POWER)

#define EARLY_SUSPEND_LEVEL                 (300)
#define EARLY_SUSPEND_TIMER_DATA            (0)
#define EARLY_SUSPEND_TIMER_EXPIRES         (50)	/* ms */

#define POWEROFF_TIMER_DATA                 (0)
#define POWEROFF_TIMER_EXPIRES              (50)	/* ms */
#define POWEROFF_TIMEOUT_COUNT              (4)	/* 4 times */

/* TODO: Relocate the extern into a header file */
/******************************************************************************
* External variables and functions
******************************************************************************/


/******************************************************************************
* Function prototypes
******************************************************************************/
static void gpu_early_suspend(struct early_suspend *h);
static void gpu_late_resume(struct early_suspend *h);

static void _early_suspend_timer_action(unsigned long);
static void _poweroff_timer_action(unsigned long);


/******************************************************************************
* Static global variables
******************************************************************************/
static struct early_suspend gpu_early_suspend_desc = {
	.level = EARLY_SUSPEND_LEVEL,
	.suspend = gpu_early_suspend,
	.resume = gpu_late_resume,
};

static struct m4u_gpu_cb gpu_power_callback_desc = {
	.gpu_power_state_freeze = gpu_power_state_freeze,
	.gpu_power_state_unfreeze = gpu_power_state_unfreeze,
};

static DEFINE_TIMER(gpu_early_suspend_timer, _early_suspend_timer_action,
		    EARLY_SUSPEND_TIMER_EXPIRES, EARLY_SUSPEND_TIMER_DATA);
static DEFINE_TIMER(gpu_poweroff_timer, _poweroff_timer_action, POWEROFF_TIMER_EXPIRES, POWEROFF_TIMER_DATA);

static unsigned int _poweron_refcnt;
static unsigned int _poweroff_tocnt;
static unsigned int _store_refcnt;
static unsigned int _freeze_refcnt;
static bool _lateinit_done;
static int _runtime_pm_en = 1;

static struct wake_lock gpu_wake_lock;

#if 0
/******************************************************************************
* Notify KMIF, the performance minotor "daemon", of the "power high" event.
******************************************************************************/
static void _kmif_power_high(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

#ifdef G3DKMD_SUPPORT_PM
	if (gExecInfo.kmif_ops.notify_g3d_power_high)
		gExecInfo.kmif_ops.notify_g3d_power_high();
	else
		KMDDPF(MSG_LEVEL_ERROR, "notify_g3d_power_high() is not found!\n");
#endif /* G3DKMD_SUPPORT_PM */

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
* Notify KMIF, the performance minotor "daemon", of the "power low" event.
******************************************************************************/
static void _kmif_power_low(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

#ifdef G3DKMD_SUPPORT_PM
	if (gExecInfo.kmif_ops.notify_g3d_power_low)
		gExecInfo.kmif_ops.notify_g3d_power_low();
	else
		KMDDPF(MSG_LEVEL_ERROR, "notify_g3d_power_low() is not found!\n");
#endif /* G3DKMD_SUPPORT_PM */

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}
#endif


/******************************************************************************
* Notify KMIF, the performance minotor "daemon", of the "power on" event.
******************************************************************************/
static void _kmif_power_on(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

#ifdef G3DKMD_SUPPORT_PM
	if (gExecInfo.kmif_ops.notify_g3d_power_on)
		gExecInfo.kmif_ops.notify_g3d_power_on();
	else
		KMDDPF(MSG_LEVEL_ERROR, "notify_g3d_power_on() is not found!\n");
#endif /* G3DKMD_SUPPORT_PM */

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
* Notify KMIF, the performance minotor "daemon", of the "power off" event.
******************************************************************************/
static void _kmif_power_off(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

#ifdef G3DKMD_SUPPORT_PM
	if (gExecInfo.kmif_ops.notify_g3d_power_off)
		gExecInfo.kmif_ops.notify_g3d_power_off();
	else
		KMDDPF(MSG_LEVEL_ERROR, "notify_g3d_power_off() is not found!\n");
#endif /* G3DKMD_SUPPORT_PM */

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
* Notify KMIF, the performance minotor "daemon", of the "power on begin" event.
******************************************************************************/
static void _kmif_power_on_begin(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

#ifdef G3DKMD_SUPPORT_PM
	if (gExecInfo.kmif_ops.notify_g3d_power_on_begin)
		gExecInfo.kmif_ops.notify_g3d_power_on_begin();
	else
		KMDDPF(MSG_LEVEL_ERROR, "notify_g3d_power_on_begin() is not found!\n");
#endif /* G3DKMD_SUPPORT_PM */

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
* Notify KMIF, the performance minotor "daemon", of the "power on end" event.
******************************************************************************/
static void _kmif_power_on_end(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

#ifdef G3DKMD_SUPPORT_PM
	if (gExecInfo.kmif_ops.notify_g3d_power_on_end)
		gExecInfo.kmif_ops.notify_g3d_power_on_end();
	else
		KMDDPF(MSG_LEVEL_ERROR, "notify_g3d_power_on_end() is not found!\n");
#endif /* G3DKMD_SUPPORT_PM */

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
* Notify KMIF, the performance minotor "daemon", of the "power off begin" event.
******************************************************************************/
static void _kmif_power_off_begin(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

#ifdef G3DKMD_SUPPORT_PM
	if (gExecInfo.kmif_ops.notify_g3d_power_off_begin)
		gExecInfo.kmif_ops.notify_g3d_power_off_begin();
	else
		KMDDPF(MSG_LEVEL_ERROR, "notify_g3d_power_off_begin() is not found!\n");
#endif /* G3DKMD_SUPPORT_PM */

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
* Notify KMIF, the performance minotor "daemon", of the "power off end" event.
******************************************************************************/
static void _kmif_power_off_end(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

#ifdef G3DKMD_SUPPORT_PM
	if (gExecInfo.kmif_ops.notify_g3d_power_off_end)
		gExecInfo.kmif_ops.notify_g3d_power_off_end();
	else
		KMDDPF(MSG_LEVEL_ERROR, "notify_g3d_power_off_end() is not found!\n");
#endif /* G3DKMD_SUPPORT_PM */

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
******************************************************************************/
static void _power_off(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	_kmif_power_off();
#ifdef G3DKMD_RECOVERY_BY_TIMER
	g3dKmd_recovery_timer_disable();
#endif
	gs_gpufreq_mtcmos_off();

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
******************************************************************************/
static void _power_on(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	gs_gpufreq_mtcmos_on();
#ifdef G3DKMD_RECOVERY_BY_TIMER
	g3dKmd_recovery_timer_enable();
#endif
	_kmif_power_on();

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
******************************************************************************/
void gpu_increment_flush_count(void)
{
/* atomic_inc(&_pm_flush_count); */
}


/******************************************************************************
******************************************************************************/
void gpu_decrement_flush_count(void)
{
/* atomic_dec(&_pm_flush_count); */
}


/******************************************************************************
* Check if Read/Write poitners move or not.
* FIXME: Need a lock to check if Read/Write pointers are equal (for all contexts)?
*
* g3dKmdCheckFreeHtSlots():
*   hwIndex = -1 for no-active context, hwIndex = 0 for active context
* g3dKmdIsHtInfoEmpty()
* g3dKmdExeInstUninit()
* g3dKmdFlushCommand()
* gExecInfo.exeInfoCount is number of contexts
******************************************************************************/
static bool _pending_work_done(void)
{
	struct g3dExeInst *pInst = NULL;
	int i;

	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	/* Check if bit 1 of REG_AREG_PA_IDLE_STATUS0 is 1, which means HW idle. */
	if (!g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS0, VAL_PA_IDLE_ST0_G3D_ALL_IDLE, 0)) {
		KMDDPF(MSG_LEVEL_INFO, "Hardware is not idle.\n");
		return false;
	}
	/* Check if the head-tail buffers of all contexts are empty. */
	for (i = 0; i < gExecInfo.exeInfoCount; i++) {
		pInst = gExecInfo.exeList[i];
		if (pInst) {
			if (!g3dKmdIsHtInfoEmpty(pInst->hwChannel, pInst->pHtDataForHwRead->htInfo)) {
				KMDDPF(MSG_LEVEL_INFO, "Head-tail of %d is not empty.\n", i);
				return false;
			}
		}
	}

	/* Check if FDQ buffer is empty. */
	if (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_HWWPTR, MSK_AREG_PA_FDQ_HWWPTR,
			  SFT_AREG_PA_FDQ_HWWPTR) != gExecInfo.fdq_rptr) {
		KMDDPF(MSG_LEVEL_INFO, "fdq_wptr = 0x%x, fdq_rptr = 0x%x.\n",
		       g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FDQ_HWWPTR, MSK_AREG_PA_FDQ_HWWPTR,
				     SFT_AREG_PA_FDQ_HWWPTR), gExecInfo.fdq_rptr);
		KMDDPF(MSG_LEVEL_INFO, "FDQ is not empty.\n");
		return false;
	}
	/* Check if M4U translation faults are handled */
	if (m4u_get_gpu_translationfault_state()) {
		KMDDPF(MSG_LEVEL_INFO, "m4u translation fault state = 0x%x.\n",
		       m4u_get_gpu_translationfault_state());
		KMDDPF(MSG_LEVEL_INFO, "M4U translation fault state is not empty.\n");
		return false;
	}

	KMDDPF(MSG_LEVEL_INFO, "Pending work done.\n");

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");

	return true;
}


/******************************************************************************
* Return TRUE if all pending work has finished. Otherwise, return FALSE.
******************************************************************************/
int gpu_pending_work_done(void)
{
	return _pending_work_done();
}


/******************************************************************************
******************************************************************************/
static void _store_state(void)
{
	unsigned int i;
	struct g3dExeInst *pInst;
	SapphireQreg *qreg;

	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	/* 1. backup m4u */
#if (USE_MMU == MMU_USING_WCP)
	if (g3dKmdCfgGet(CFG_MMU_ENABLE))
		m4u_reg_backup(GPU_IOMMU_ID);
#endif

	/* 2. reset fdq read/write ptr */
	/* FIXME: use g3dlock to protect the variable? */
	gExecInfo.fdq_rptr = 0;	/* 0-base, 64bit unit */
	gExecInfo.fdq_wptr = 0;

	/* 3. reset qbase buffer */
	/* FIXME: use g3dlock to protect the variable? */
	for (i = 0; i < gExecInfo.exeInfoCount; i++) {
		pInst = gExecInfo.exeList[i];
		if (pInst != NULL) {
			qreg = (SapphireQreg *) pInst->qbaseBuffer.data;
			qreg->reg_cq_rsm.dwValue = 0x0;
			qreg->reg_cq_htbuf_rptr.s.cq_htbuf_rptr = pInst->pHtDataForHwRead->htInfo->htCommandBaseHw;
			qreg->reg_cq_htbuf_wptr.s.cq_htbuf_wptr = pInst->pHtDataForHwRead->htInfo->htCommandBaseHw;
			qreg->reg_cq_fm_head.dwValue = 0x0;

			pInst->pHtDataForHwRead->htInfo->wtPtr = pInst->pHtDataForHwRead->htInfo->htCommandBaseHw;
			pInst->pHtDataForHwRead->htInfo->rdPtr = pInst->pHtDataForHwRead->htInfo->htCommandBaseHw;
			pInst->pHtDataForHwRead->htInfo->readyPtr = pInst->pHtDataForHwRead->htInfo->htCommandBaseHw;
		}
	}

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
******************************************************************************/
static void _restore_state(void)
{
	unsigned int i;
	struct g3dExeInst *pInst;

	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	/* 1. restore m4u */
#if (USE_MMU == MMU_USING_WCP)
	if (g3dKmdCfgGet(CFG_MMU_ENABLE))
		m4u_reg_restore(GPU_IOMMU_ID);
#endif

	/* 2. reset context states */
	/* FIXME: use g3dlock to protect the variavle? */
	gExecInfo.hwStatus[0].inst_idx = EXE_INST_NONE;
	gExecInfo.hwStatus[0].state = G3DKMD_HWSTATE_IDLE;
	for (i = 0; i < gExecInfo.exeInfoCount; i++) {
		pInst = gExecInfo.exeList[i];
		if (pInst != NULL) {
			pInst->hwChannel = G3DKMD_HWCHANNEL_NONE;
			pInst->time_slice = 0;
			pInst->state = G3DEXESTATE_IDLE; /* set to IDLE, such that scheduler will pick it next time */
		}
	}

	/* 3. reset Regs, avoid m4u opreation because we have done in step 1. */
	g3dKmdRegReset();

	/* 4. restore Fdq read/write ptrs. */
	g3dKmdFdqUpdateBufPtr();

	/* 5. do qload */
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
	g3dKmdTriggerScheduler();
#else
	if (gExecInfo.currentExecInst != NULL) {
		if (g3dKmdQLoad(gExecInfo.currentExecInst))
			g3dKmdRegSendCommand(gExecInfo.currentExecInst->hwChannel, gExecInfo.currentExecInst);
	}
#endif

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
* Wait until all pending work has finished.
******************************************************************************/
static void _early_suspend_timer_action(unsigned long timer_data)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");
	KMDDPF(MSG_LEVEL_INFO, "in, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);

	lock_g3dPwrState();
	if (gExecInfo.PwrState == G3DKMD_PWRSTATE_POWERON) {
		if (_pending_work_done() == false) {
			unlock_g3dPwrState();
			mod_timer(&gpu_early_suspend_timer, jiffies + msecs_to_jiffies(EARLY_SUSPEND_TIMER_EXPIRES));

			KMDDPF(MSG_LEVEL_INFO, "next, PwrState = %u\n", gExecInfo.PwrState);
			KMDDPF(MSG_LEVEL_INFO, "Re-add early suspend timer @jiffies = %u.\n", jiffies);
			KMDDPF(MSG_LEVEL_NOTICE, "out\n");

			return;
		}
	}
	unlock_g3dPwrState();

	wake_unlock(&gpu_wake_lock);

	KMDDPF(MSG_LEVEL_INFO, "out, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);
	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
* FIXME: May need to use a atomic variable to ensure only one timer is added.
******************************************************************************/
static void gpu_early_suspend(struct early_suspend *h)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	KMDDPF(MSG_LEVEL_INFO, "Add early suspend timer @jiffies = %u.\n", jiffies);
	mod_timer(&gpu_early_suspend_timer, jiffies + msecs_to_jiffies(EARLY_SUSPEND_TIMER_EXPIRES));

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
******************************************************************************/
static void gpu_late_resume(struct early_suspend *h)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	wake_lock(&gpu_wake_lock);

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
* FIXME: Check the effect of wake_lock_init() when gpu_register_early_suspend()
*        is called multiple times.
******************************************************************************/
void gpu_register_early_suspend(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	wake_lock_init(&gpu_wake_lock, WAKE_LOCK_SUSPEND, "GPU_WAIT_LOCK");
	register_early_suspend(&gpu_early_suspend_desc);

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
******************************************************************************/
void gpu_unregister_early_suspend(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	unregister_early_suspend(&gpu_early_suspend_desc);

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
******************************************************************************/
void gpu_suspend(void)
{
#if 0
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");
	KMDDPF(MSG_LEVEL_INFO, "in, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);

	lock_g3dPwrState();
	if (_poweron_refcnt == 0 && gExecInfo.PwrState == G3DKMD_PWRSTATE_POWERON) {
		gExecInfo.PwrState = G3DKMD_PWRSTATE_POWEROFF;
		_store_state();
		_power_off();
	}
	unlock_g3dPwrState();

	KMDDPF(MSG_LEVEL_INFO, "out, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);
	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
#endif
}


/******************************************************************************
******************************************************************************/
void gpu_resume(void)
{
#if 0
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");
	KMDDPF(MSG_LEVEL_INFO, "in, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);

	lock_g3dPwrState();
	if (_poweron_refcnt == 0 && gExecInfo.PwrState == G3DKMD_PWRSTATE_SET_POWEROFF) {
		_power_on();
		_restore_state();
		gExecInfo.PwrState = G3DKMD_PWRSTATE_POWERON;
	}
	unlock_g3dPwrState();

	KMDDPF(MSG_LEVEL_INFO, "out, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);
	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
#endif
}


/******************************************************************************
******************************************************************************/
void gpu_may_power_off(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	KMDDPF(MSG_LEVEL_INFO, "in, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);
	KMDDPF(MSG_LEVEL_INFO, "in, _poweron_refcnt = %u\n", _poweron_refcnt);
	KMDDPF(MSG_LEVEL_INFO, "in, _poweroff_tocnt = %u\n", _poweroff_tocnt);
	KMDDPF(MSG_LEVEL_INFO, "in, _store_refcnt = %u\n", _store_refcnt);

#if 1
	lock_g3dPwrState();
	_poweron_refcnt = (_poweron_refcnt > 0) ? (_poweron_refcnt - 1) : (0);
	_poweroff_tocnt = 0;
	if (_lateinit_done == true && _runtime_pm_en > 0 && _poweron_refcnt == 0) {
		unlock_g3dPwrState();
		mod_timer(&gpu_poweroff_timer, jiffies + msecs_to_jiffies(POWEROFF_TIMER_EXPIRES));
		KMDDPF(MSG_LEVEL_INFO, "Add poweroff timer @jiffies = %u.\n", jiffies);
	} else {
		unlock_g3dPwrState();
	}
#endif

	KMDDPF(MSG_LEVEL_INFO, "out, _store_refcnt = %u\n", _store_refcnt);
	KMDDPF(MSG_LEVEL_INFO, "out, _poweroff_tocnt = %u\n", _poweroff_tocnt);
	KMDDPF(MSG_LEVEL_INFO, "out, _poweron_refcnt = %u\n", _poweron_refcnt);
	KMDDPF(MSG_LEVEL_INFO, "out, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}
EXPORT_SYMBOL(gpu_may_power_off);


/******************************************************************************
******************************************************************************/
void gpu_set_power_on(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	KMDDPF(MSG_LEVEL_INFO, "in, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);
	KMDDPF(MSG_LEVEL_INFO, "in, _poweron_refcnt = %u\n", _poweron_refcnt);
	KMDDPF(MSG_LEVEL_INFO, "in, _poweroff_tocnt = %u\n", _poweroff_tocnt);
	KMDDPF(MSG_LEVEL_INFO, "in, _store_refcnt = %u\n", _store_refcnt);

#if 1
	lock_g3dPwrState();
	_poweron_refcnt++;
	_poweroff_tocnt = 0;
	if (_lateinit_done == true) {
		if (gExecInfo.PwrState == G3DKMD_PWRSTATE_POWEROFF) {
			_kmif_power_on_begin();
			_power_on();
			if (_store_refcnt > 0) {
				_store_refcnt--;
				_restore_state();
			}
			_kmif_power_on_end();
		}
		gExecInfo.PwrState = G3DKMD_PWRSTATE_POWERON;
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
		wake_up(&gExecInfo.s_SchedWq);
#endif
	}
	unlock_g3dPwrState();
#endif

	KMDDPF(MSG_LEVEL_INFO, "out, _store_refcnt = %u\n", _store_refcnt);
	KMDDPF(MSG_LEVEL_INFO, "out, _poweroff_tocnt = %u\n", _poweroff_tocnt);
	KMDDPF(MSG_LEVEL_INFO, "out, _poweron_refcnt = %u\n", _poweron_refcnt);
	KMDDPF(MSG_LEVEL_INFO, "out, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}
EXPORT_SYMBOL(gpu_set_power_on);


/******************************************************************************
* Wait until poweroff timeout.
******************************************************************************/
static void _poweroff_timer_action(unsigned long timer_data)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	KMDDPF(MSG_LEVEL_INFO, "in, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);
	KMDDPF(MSG_LEVEL_INFO, "in, _poweron_refcnt = %u\n", _poweron_refcnt);
	KMDDPF(MSG_LEVEL_INFO, "in, _poweroff_tocnt = %u\n", _poweroff_tocnt);
	KMDDPF(MSG_LEVEL_INFO, "in, _store_refcnt = %u\n", _store_refcnt);

#if 1
	lock_g3dPwrState();
	if (_runtime_pm_en <= 0) {
		unlock_g3dPwrState();
		KMDDPF(MSG_LEVEL_NOTICE, "out, _runtime_pm_en = %d\n", _runtime_pm_en);
		KMDDPF(MSG_LEVEL_NOTICE, "out\n");
		return;
	}

	/* put _poweron_refcnt into condition list to make it robust */
	if (_poweron_refcnt == 0 && gExecInfo.PwrState == G3DKMD_PWRSTATE_POWERON) {
		_poweroff_tocnt = (_pending_work_done() == true) ? (_poweroff_tocnt + 1) : (0);
		if (_poweroff_tocnt < POWEROFF_TIMEOUT_COUNT) {
			unlock_g3dPwrState();
			if (!timer_pending(&gpu_poweroff_timer)) {
				mod_timer(&gpu_poweroff_timer, jiffies + msecs_to_jiffies(POWEROFF_TIMER_EXPIRES));
				KMDDPF(MSG_LEVEL_INFO, "Re-add poweroff timer @jiffies = %u.\n", jiffies);
			}

			KMDDPF(MSG_LEVEL_INFO, "out, _store_refcnt = %u\n", _store_refcnt);
			KMDDPF(MSG_LEVEL_INFO, "out, _poweroff_tocnt = %u\n", _poweroff_tocnt);
			KMDDPF(MSG_LEVEL_INFO, "out, _poweron_refcnt = %u\n", _poweron_refcnt);
			KMDDPF(MSG_LEVEL_INFO, "out, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);

			KMDDPF(MSG_LEVEL_NOTICE, "out\n");

			return;
		}

		gExecInfo.PwrState = G3DKMD_PWRSTATE_POWEROFF;
		_kmif_power_off_begin();
		_store_state();
		_store_refcnt++;
		_power_off();
		_kmif_power_off_end();
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
		wake_up(&gExecInfo.s_SchedWq);
#endif
	}
	unlock_g3dPwrState();
#endif

	KMDDPF(MSG_LEVEL_INFO, "out, _store_cnt = %u\n", _store_refcnt);
	KMDDPF(MSG_LEVEL_INFO, "out, _poweroff_tocnt = %u\n", _poweroff_tocnt);
	KMDDPF(MSG_LEVEL_INFO, "out, _poweron_refcnt = %u\n", _poweron_refcnt);
	KMDDPF(MSG_LEVEL_INFO, "out, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
 * Func: gpu_power_state_freeze(bool fgForcePowerOn)
 * Param:
 * fgForcePoweron : false: only get the state
 *		    true : request gpu power on if it's power off.
 *			   only return the state if it's power on.
 * Return : 0: unlocked and power off.
 *		if it is power off, m4u_invalid_tlb will return directly,
 *		it don't call unlock. so please keep it unlocked.
 *          1: locked and power on.
 *		if it is power on, "unlocked" is needed, please keep it locked.
 *	    <0: unlocked and failed while force power on.
 * Return is 0 or 1 while fgForcePoweron is false.
******************************************************************************/
int gpu_power_state_freeze(bool fgForcePowerOn)
{
	int ret;

	KMDDPF(MSG_LEVEL_DEBUG, "in\n");

	KMDDPF(MSG_LEVEL_DEBUG, "in, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);
	KMDDPF(MSG_LEVEL_DEBUG, "in, _freeze_refcnt = %u\n", _freeze_refcnt);

	lock_g3dPwrState();
	ret = gExecInfo.PwrState;
	if (fgForcePowerOn == true || gExecInfo.PwrState == G3DKMD_PWRSTATE_POWERON) {
		_freeze_refcnt++;
		unlock_g3dPwrState();
		gpu_set_power_on();
		lock_g3dPwrState();
	}
	unlock_g3dPwrState();

	KMDDPF(MSG_LEVEL_DEBUG, "out, _freeze_refcnt = %u\n", _freeze_refcnt);
	KMDDPF(MSG_LEVEL_DEBUG, "out, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);

	KMDDPF(MSG_LEVEL_DEBUG, "out\n");

	return ret;
}
EXPORT_SYMBOL(gpu_power_state_freeze);


/******************************************************************************
******************************************************************************/
void gpu_power_state_unfreeze(bool fgForcePowerOn)
{
	KMDDPF(MSG_LEVEL_DEBUG, "in\n");

	KMDDPF(MSG_LEVEL_DEBUG, "in, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);
	KMDDPF(MSG_LEVEL_DEBUG, "in, _freeze_refcnt = %u\n", _freeze_refcnt);

	lock_g3dPwrState();
	if (_freeze_refcnt > 0) {
		unlock_g3dPwrState();
		gpu_may_power_off();
		lock_g3dPwrState();
		_freeze_refcnt--;
	}
	unlock_g3dPwrState();

	KMDDPF(MSG_LEVEL_DEBUG, "out, _freeze_refcnt = %u\n", _freeze_refcnt);
	KMDDPF(MSG_LEVEL_DEBUG, "out, gExecInfo.PwrState = %u\n", gExecInfo.PwrState);

	KMDDPF(MSG_LEVEL_DEBUG, "out\n");
}
EXPORT_SYMBOL(gpu_power_state_unfreeze);


/******************************************************************************
******************************************************************************/
static void gpu_register_power_callback(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	m4u_register_gpu_clock_callback(0, &gpu_power_callback_desc);

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
******************************************************************************/
void gpu_runtime_pm_enable(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	lock_g3dPwrState();
	_runtime_pm_en++;
	unlock_g3dPwrState();
	gpu_set_power_on();
	gpu_may_power_off();

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
******************************************************************************/
void gpu_runtime_pm_disable(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	lock_g3dPwrState();
	_runtime_pm_en--;
	unlock_g3dPwrState();
	gpu_set_power_on();
	gpu_may_power_off();

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");
}


/******************************************************************************
******************************************************************************/
static int __init gpu_power_init(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	gpu_register_power_callback();

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");

	return 0;
}


/******************************************************************************
******************************************************************************/
static int __init gpu_power_lateinit(void)
{
	KMDDPF(MSG_LEVEL_NOTICE, "in\n");

	_lateinit_done = true;

	KMDDPF(MSG_LEVEL_NOTICE, "out\n");

	return 0;
}


subsys_initcall(gpu_power_init);
late_initcall(gpu_power_lateinit);


#else


/******************************************************************************
******************************************************************************/
#include "g3dkmd_power.h"


/******************************************************************************
******************************************************************************/
void gpu_register_early_suspend(void)
{
	/* Empty! */
}


/******************************************************************************
******************************************************************************/
void gpu_unregister_early_suspend(void)
{
	/* Empty! */
}


/******************************************************************************
******************************************************************************/
void gpu_suspend(void)
{
	/* Empty! */
}


/******************************************************************************
******************************************************************************/
void gpu_resume(void)
{
	/* Empty! */
}


/******************************************************************************
******************************************************************************/
void gpu_may_power_off(void)
{
	/* Empty! */
}
EXPORT_SYMBOL(gpu_may_power_off);


/******************************************************************************
******************************************************************************/
void gpu_set_power_on(void)
{
	/* Empty! */
}
EXPORT_SYMBOL(gpu_set_power_on);


/******************************************************************************
******************************************************************************/
int gpu_power_state_freeze(bool fgForcePowerOn)
{
	return 1;
}


/******************************************************************************
******************************************************************************/
void gpu_power_state_unfreeze(bool fgForcePowerOn)
{
	/* Empty! */
}


/******************************************************************************
******************************************************************************/
void gpu_runtime_pm_enable(void)
{
	/* Empty! */
}


/******************************************************************************
******************************************************************************/
void gpu_runtime_pm_disable(void)
{
	/* Empty! */
}


/******************************************************************************
******************************************************************************/
void gpu_increment_flush_count(void)
{
	/* Empty! */
}


/******************************************************************************
******************************************************************************/
void gpu_decrement_flush_count(void)
{
	/* Empty! */
}
#endif /* CONFIG_HAS_EARLYSUSPEND */
