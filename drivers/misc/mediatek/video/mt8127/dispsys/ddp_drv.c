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

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <generated/autoconf.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/param.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <asm/io.h>
#include "ddp_drv.h"
#include "ddp_reg.h"
#include "ddp_hal.h"
#include "ddp_path.h"
#include "ddp_debug.h"
#include "ddp_color.h"
#include "disp_drv_ddp.h"
#include "ddp_wdma.h"
#include "ddp_bls.h"
#include "disp_debug.h"
#include "dpi_drv.h"
#if defined(CONFIG_MTK_HDMI_SUPPORT)
#include "dpi1_drv.h"
#endif
#include "dsi_drv.h"
#include "ddp_aal.h"
#include "disp_drv.h"
#include "ddp_cmdq_reg.h"
#include "ddp_cmdq.h"

#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif

#ifdef CONFIG_ANDROID
#include "ddp_cmdq.h"
/* #include <mach/xlog.h> */
#include "mt-plat/mt_smi.h"
struct ion_sys_data sys_data;
struct ion_mm_data mm_data;
#endif

#define DISP_INDEX_OFFSET 0x0	/* must be consistent with ddp_rdma.c */
#define DISP_DEVNAME "mtk_disp"

unsigned int dbg_log = 0;
/* must disable irq level log by default, else will block uart output, open it only for debug use */
unsigned int irq_log = 0;
unsigned int irq_err_log = 0;
unsigned int disp_log_level = DISPLOG_MSG;/*DISPLOG_FENCE;(DISPLOG_ALL & (~DISPLOG_IRQ));*/
/* device and driver */
static dev_t disp_devno;
static struct cdev *disp_cdev;
static struct class *disp_class;
struct disp_device disp_dev;
/* ION */
unsigned char ion_init = 0;
unsigned char dma_init = 0;
/* NCSTool for Color Tuning */
unsigned char ncs_tuning_mode = 0;
/* flag for gamma lut update */
unsigned char bls_gamma_dirty = 0;
struct ion_client *cmdqIONClient;
struct ion_handle *cmdqIONHandle;
unsigned long *cmdq_pBuffer;
unsigned int cmdq_pa;
unsigned int cmdq_pa_len;

#ifdef CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT
/* the session to communicate with TA */
/* code shall access these session handles via [ddp_session_handle()] API */
static KREE_SESSION_HANDLE ddp_session;
static KREE_SESSION_HANDLE ddp_mem_session;	/* use [ddp_mem_session_handle()] */
#endif

unsigned int gOvlSecure = 0;
unsigned int gOvlLayerSecure[4] = { 0, 0, 0, 0 };
unsigned int gOvlLayerSecureLast[4] = { 0, 0, 0, 0 };

unsigned int gMemOutSecure = 0;
unsigned int gBitbltSecure = 0;

/* used by IRQ from TEE to REE
  * NOTICE: must be consistent with defined in tz_ddp_hal.c
  */
#define MT8127_MDP_ROT_IRQ_BIT     0
#define MT8127_MDP_WDMA_IRQ_BIT    1
#define MT8127_DISP_OVL_IRQ_BIT    2
#define MT8127_DISP_WDMA_IRQ_BIT  3
#define MT8127_DISP_RDMA0_IRQ_BIT  4
#define MT8127_DISP_RDMA1_IRQ_BIT  5

typedef struct {
	unsigned int MDP_ROT_IRQ_STATUS;
	unsigned int MDP_WDMA_IRQ_STATUS;
	unsigned int OVL_IRQ_STATUS;
	unsigned int DISP_WDMA_IRQ_STATUS;
	unsigned int RDMA0_IRQ_STATUS;
	unsigned int RDMA1_IRQ_STATUS;
} MTEE_DISP_IRQ_STATUS;


/* irq */
#define DISP_REGISTER_IRQ(irq_num) {\
	if (request_irq(irq_num , (irq_handler_t)disp_irq_handler,\
				IRQF_TRIGGER_NONE, DISP_DEVNAME , NULL)){ \
			DISP_ERR("ddp register irq failed! %d\n", irq_num);\
		} \
	}

#define DISP_UNREGISTER_IRQ(irq_num) free_irq(irq_num , 0)

/* -------------------------------------------------------------------------------// */
/* global variables */
typedef struct {
	spinlock_t irq_lock;
	unsigned int irq_src;	/* one bit represent one module */
} disp_irq_struct;

typedef struct {
	pid_t open_pid;
	pid_t open_tgid;
	struct list_head testList;
	unsigned int u4LockedResource;
	unsigned int u4Clock;
	spinlock_t node_lock;
} disp_node_struct;

#define DISP_MAX_IRQ_CALLBACK   10
static DDP_IRQ_CALLBACK g_disp_irq_table[DISP_MODULE_MAX][DISP_MAX_IRQ_CALLBACK];
disp_irq_struct g_disp_irq;
static DECLARE_WAIT_QUEUE_HEAD(g_disp_irq_done_queue);

/* cmdq thread */
unsigned char cmdq_thread[CMDQ_THREAD_NUM] = { 1, 1, 1, 1, 1, 1, 1 };

spinlock_t gCmdqLock;
wait_queue_head_t cmq_wait_queue[CMDQ_THREAD_NUM];

/* G2d Variables */
spinlock_t gResourceLock;
unsigned int gLockedResource;	/* lock dpEngineType_6582 */
static DECLARE_WAIT_QUEUE_HEAD(gResourceWaitQueue);

/* Overlay Variables */
spinlock_t gOvlLock;
int disp_run_dp_framework = 0;
int disp_layer_enable = 0;
int disp_mutex_status = 0;

DISP_OVL_INFO disp_layer_info[DDP_OVL_LAYER_MUN];

/* AAL variables */
static unsigned long u4UpdateFlag;

/* Register update lock */
spinlock_t gRegisterUpdateLock;
spinlock_t gPowerOperateLock;
/* Clock gate management */
/* static unsigned long g_u4ClockOnTbl = 0; */

/* IRQ log print kthread */
static struct task_struct *disp_irq_log_task;
static wait_queue_head_t disp_irq_log_wq;
static volatile int disp_irq_log_module;

static DISPLAY_TDSHP_T g_TDSHP_Index;

static int g_irq_err_print;	/* print aee warning 2s one time */
#define DDP_ERR_IRQ_INTERVAL_TIME 5
static unsigned int disp_irq_err;
unsigned int cnt_rdma_underflow = 0;
unsigned int cnt_rdma1_underflow = 0;
unsigned int cnt_rdma_abnormal = 0;
unsigned int cnt_rdma1_abnormal = 0;
unsigned int cnt_ovl_underflow = 0;
unsigned int cnt_ovl_abnormal = 0;
unsigned int cnt_wdma_underflow = 0;
unsigned int cnt_mutex_timeout = 0;
#define DDP_IRQ_OVL_L0_ABNORMAL  (1<<0)
#define DDP_IRQ_OVL_L1_ABNORMAL  (1<<1)
#define DDP_IRQ_OVL_L2_ABNORMAL  (1<<2)
#define DDP_IRQ_OVL_L3_ABNORMAL  (1<<3)
#define DDP_IRQ_OVL_L0_UNDERFLOW (1<<4)
#define DDP_IRQ_OVL_L1_UNDERFLOW (1<<5)
#define DDP_IRQ_OVL_L2_UNDERFLOW (1<<6)
#define DDP_IRQ_OVL_L3_UNDERFLOW (1<<7)
#define DDP_IRQ_RDMA_ABNORMAL       (1<<8)
#define DDP_IRQ_RDMA_UNDERFLOW      (1<<9)
#define DDP_IRQ_WDMA_ABNORMAL       (1<<10)
#define DDP_IRQ_WDMA_UNDERFLOW      (1<<11)

static struct timer_list disp_irq_err_timer;
static void disp_irq_err_timer_handler(unsigned long lparam)
{
	g_irq_err_print = 1;
}

DISPLAY_TDSHP_T *get_TDSHP_index(void)
{
	return &g_TDSHP_Index;
}


/* internal function */
static int disp_wait_intr(DISP_MODULE_ENUM module, unsigned int timeout_ms);
static int disp_get_mutex_status(void);
static int disp_set_needupdate(DISP_MODULE_ENUM eModule, unsigned long u4En);
static void disp_power_off(DISP_MODULE_ENUM eModule, unsigned int *pu4Record);
static void disp_power_on(DISP_MODULE_ENUM eModule, unsigned int *pu4Record);
unsigned int *pRegBackup = NULL;

/* -------------------------------------------------------------------------------// */
/* functions */

void disp_check_clock_tree(void)
{
	unsigned int mutexID = 0;
	unsigned int mutex_mod = 0;
	unsigned int mutex_sof = 0;

	return;

	DISP_MSG("0xf0000000=0x%x, 0xf0000050=0x%x, 0xf0000040=0x%x\n",
		 *(volatile unsigned int *)(0xf0000000),
		 *(volatile unsigned int *)(0xf0000050), *(volatile unsigned int *)(0xf0000040));
	/* All need */
	if ((DISP_REG_GET(0xf0000040) & 0xff) != 0x01)
		DISP_ERR("CLK_CFG_0 abnormal: hf_faxi_ck is off! 0xf0000040=0x%x\n",
			 DISP_REG_GET(0xf0000040));
	if ((DISP_REG_GET(0xf0000040) & 0xff000000) != 0x1000000)
		DISP_ERR("CLK_CFG_0 abnormal: hf_fmm_ck is off! 0xf0000040=0x%x\n",
			 DISP_REG_GET(0xf0000040));
	if ((DISP_REG_GET(0xf4000100) & (1 << 0)) != 0)
		DISP_ERR("MMSYS_CG_CON0 abnormal: SMI_COMMON is off!\n");
	if ((DISP_REG_GET(0xf4000100) & (1 << 1)) != 0)
		DISP_ERR("MMSYS_CG_CON0 abnormal: SMI_LARB0 is off!\n");
	if ((DISP_REG_GET(0xf4000100) & (1 << 3)) != 0)
		DISP_ERR("MMSYS_CG_CON0 abnormal: MUTEX is off(bit3 = 1) 0xf4000100 = 0x%x!\n",
			 DISP_REG_GET(0xf4000100));
	for (mutexID = 0; mutexID < 4; mutexID++) {
		mutex_mod = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD(mutexID));
		mutex_sof = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_SOF(mutexID));
		if (mutex_mod & (1 << 3))
			if ((DISP_REG_GET(0xf4000100) & (1 << 8)) != 0)
				DISP_ERR("MMSYS_CG_CON0 abnormal: DISP_OVL is off!\n");
		if (mutex_mod & (1 << 6))
			if ((DISP_REG_GET(0xf4000100) & (1 << 6)) != 0)
				DISP_MSG("MMSYS_CG_CON0 abnormal: DISP_wdma is off!\n");
		if (mutex_mod & (1 << 7))
			if ((DISP_REG_GET(0xf4000100) & (1 << 4)) != 0)
				DISP_ERR("MMSYS_CG_CON0 abnormal: DISP_COLOR is off!\n");
		if (mutex_mod & (1 << 9)) {
			/* MMSYS_CG_CON0 */
			if ((DISP_REG_GET(0xf4000100) & (1 << 5)) != 0)
				DISP_ERR("MMSYS_CG_CON0 abnormal: DISP_BLS is off!\n");
			/* CLK_CFG_1 */
			if ((DISP_REG_GET(0xf0000050) & 0x0ff) == 0)
				DISP_ERR("CLK_CFG_1 abnormal: fpwm_ck is off!\n");
		}
		if (mutex_mod & (1 << 10))
			if ((DISP_REG_GET(0xf4000100) & (1 << 7)) != 0)
				DISP_ERR("MMSYS_CG_CON0 abnormal: DISP_RDMA is off!\n");
		if (mutex_mod & (1 << 12))
			if ((DISP_REG_GET(0xf4000100) & (1 << 19)) != 0)
				DISP_ERR("MMSYS_CG_CON0 abnormal: DISP_RDMA1 is off!\n");
		/*  */

		/* DSI CMD/VDO */
		if (mutex_sof == 0x1) {
			/* MMSYS_CG_CON1 */
			if ((DISP_REG_GET(0xf4000110) & 0x07) != 0)
				DISP_ERR("MMSYS_CG_CON1 abnormal: DSI is off!\n");
			/* CLK_CFG_1 */
			/* if (DISP_REG_GET(0xf0000100)&(1<<37) == 0) { */
			/* DISP_ERR("CLK_CFG_1 abnormal: ad_dsi0_intc_dsiclk is off!\n"); */
			/* } */
		}
		/* DPI0 */
		if (mutex_sof == 0x2) {
			/* CLK_CFG_1 */
			if ((DISP_REG_GET(0xf0000104) & (1 << 28)) == 0)
				DISP_ERR("CLK_CFG_1 abnormal: hf_dpi0_ck is off!\n");
		}
	}
}

static int disp_irq_log_kthread_func(void *data)
{
	unsigned int i = 0;
	unsigned int module;

	while (1) {
		wait_event_interruptible(disp_irq_log_wq, disp_irq_log_module);
		/* reset wakeup flag */
		module = disp_irq_log_module;
		disp_irq_log_module = 0;
		DISP_NOTICE
		    ("disp_irq_log_kthread_func dump intr register: disp_irq_log_module=0x%X\n",
		     module);
		for (i = 0; i < DISP_MODULE_MAX; i++)
			if ((module & (1 << i)) != 0)
				disp_print_reg(i);

		if ((disp_irq_err == 1) && (g_irq_err_print == 1)) {
#if 1
			if (disp_irq_err & DDP_IRQ_OVL_L0_ABNORMAL)
				DISP_IRQERR("OVL_RDMA0_ABNORMAL");
			if (disp_irq_err & DDP_IRQ_OVL_L1_ABNORMAL)
				DISP_IRQERR("OVL_RDMA1_ABNORMAL");
			if (disp_irq_err & DDP_IRQ_OVL_L2_ABNORMAL)
				DISP_IRQERR("OVL_RDMA2_ABNORMAL");
			if (disp_irq_err & DDP_IRQ_OVL_L3_ABNORMAL)
				DISP_IRQERR("OVL_RDMA3_ABNORMAL");
			if (disp_irq_err & DDP_IRQ_OVL_L0_UNDERFLOW)
				DISP_IRQERR("OVL_RDMA0_ABNORMAL");
			if (disp_irq_err & DDP_IRQ_OVL_L1_UNDERFLOW)
				DISP_IRQERR("OVL_RDMA1_ABNORMAL");
			if (disp_irq_err & DDP_IRQ_OVL_L2_UNDERFLOW)
				DISP_IRQERR("OVL_RDMA2_ABNORMAL");
			if (disp_irq_err & DDP_IRQ_OVL_L3_UNDERFLOW)
				DISP_IRQERR("OVL_RDMA3_ABNORMAL");
#endif
			if (disp_irq_err & DDP_IRQ_RDMA_ABNORMAL)
				DISP_IRQERR("RDMA_ABNORMAL");
			if (disp_irq_err & DDP_IRQ_RDMA_UNDERFLOW)
				DISP_IRQERR("RDMA_UNDERFLOW");
			if (disp_irq_err & DDP_IRQ_WDMA_UNDERFLOW)
				DISP_IRQERR("WDMA_UNDERFLOW");
			disp_irq_err = 0;

			g_irq_err_print = 0;	/* at most, 5s print one frame */
			mod_timer(&disp_irq_err_timer, jiffies + DDP_ERR_IRQ_INTERVAL_TIME * HZ);
		}

	}

	return 0;
}

unsigned int disp_ms2jiffies(unsigned long ms)
{
	return (ms * HZ + 512) >> 10;
}

int disp_lock_cmdq_thread(void)
{
	int i = 0;

	DISP_MSG("disp_lock_cmdq_thread()called\n");

	spin_lock(&gCmdqLock);
	for (i = 0; i < CMDQ_THREAD_NUM; i++) {
		if (cmdq_thread[i] == 1) {
			cmdq_thread[i] = 0;
			break;
		}
	}
	spin_unlock(&gCmdqLock);

	DISP_MSG("disp_lock_cmdq_thread(), i=%d\n", i);

	return (i >= CMDQ_THREAD_NUM) ? -1 : i;

}

int disp_unlock_cmdq_thread(unsigned int idx)
{
	if (idx >= CMDQ_THREAD_NUM)
		return -1;

	spin_lock(&gCmdqLock);
	cmdq_thread[idx] = 1;	/* free thread availbility */
	spin_unlock(&gCmdqLock);

	return 0;
}

/* if return is not 0, should wait again */
static int disp_wait_intr(DISP_MODULE_ENUM module, unsigned int timeout_ms)
{
	int ret;
	unsigned long flags;

	unsigned long long end_time = 0;
	unsigned long long start_time = sched_clock();

	MMProfileLogEx(DDP_MMP_Events.WAIT_INTR, MMProfileFlagStart, 0, module);
	/* wait until irq done or timeout */
	ret = wait_event_interruptible_timeout(g_disp_irq_done_queue,
					       g_disp_irq.irq_src & (1 << module),
					       msecs_to_jiffies(timeout_ms));

	/*wake-up from sleep */
	if (ret == 0) {		/* timeout */
		MMProfileLogEx(DDP_MMP_Events.WAIT_INTR, MMProfileFlagPulse, 0, module);
		MMProfileLog(DDP_MMP_Events.WAIT_INTR, MMProfileFlagEnd);
		DISP_ERR("Wait Done Timeout! pid=%d, module=%d\n", current->pid, module);
		ddp_dump_info(module);
		ddp_dump_info(DISP_MODULE_CONFIG);
		ddp_dump_info(DISP_MODULE_MUTEX);
		disp_check_clock_tree();
		return -EAGAIN;
	} else if (ret < 0) {	/* intr by a signal */
		MMProfileLogEx(DDP_MMP_Events.WAIT_INTR, MMProfileFlagPulse, 1, module);
		MMProfileLog(DDP_MMP_Events.WAIT_INTR, MMProfileFlagEnd);
		DISP_ERR("Wait Done interrupted by a signal! pid=%d, module=%d\n", current->pid,
			 module);
		ddp_dump_info(module);
		return -EAGAIN;
	}

	MMProfileLogEx(DDP_MMP_Events.WAIT_INTR, MMProfileFlagEnd, 0, module);
	spin_lock_irqsave(&g_disp_irq.irq_lock, flags);
	g_disp_irq.irq_src &= ~(1 << module);
	spin_unlock_irqrestore(&g_disp_irq.irq_lock, flags);

	end_time = sched_clock();
	DISP_DBG("disp_wait_intr wait %d us\n",
		 ((unsigned int)end_time - (unsigned int)start_time) / 1000);

	return 0;
}

int disp_register_irq(DISP_MODULE_ENUM module, DDP_IRQ_CALLBACK cb)
{
	int i;

	if (module >= DISP_MODULE_MAX) {
		DISP_ERR("Register IRQ with invalid module ID. module=%d\n", module);
		return -1;
	}
	if (cb == NULL) {
		DISP_ERR("Register IRQ with invalid cb.\n");
		return -1;
	}
	for (i = 0; i < DISP_MAX_IRQ_CALLBACK; i++) {
		if (g_disp_irq_table[module][i] == cb)
			break;
	}
	if (i < DISP_MAX_IRQ_CALLBACK)	/* Already registered. */
		return 0;
	for (i = 0; i < DISP_MAX_IRQ_CALLBACK; i++) {
		if (g_disp_irq_table[module][i] == NULL)
			break;
	}
	if (i == DISP_MAX_IRQ_CALLBACK) {
		DISP_ERR("No enough callback entries for module %d.\n", module);
		return -1;
	}
	g_disp_irq_table[module][i] = cb;
	return 0;
}

int disp_unregister_irq(DISP_MODULE_ENUM module, DDP_IRQ_CALLBACK cb)
{
	int i;

	for (i = 0; i < DISP_MAX_IRQ_CALLBACK; i++) {
		if (g_disp_irq_table[module][i] == cb) {
			g_disp_irq_table[module][i] = NULL;
			break;
		}
	}
	if (i == DISP_MAX_IRQ_CALLBACK) {
		DISP_ERR
		    ("Try to unregister callback function with was not registered. module=%d cb=0x%08X\n",
		     module, (unsigned int)cb);
		return -1;
	}
	return 0;
}

void disp_invoke_irq_callbacks(DISP_MODULE_ENUM module, unsigned int param)
{
	int i;

	for (i = 0; i < DISP_MAX_IRQ_CALLBACK; i++) {
		if (g_disp_irq_table[module][i])
			g_disp_irq_table[module][i] (param);
	}
}

static void disp_check_mutex_status(void)
{
	unsigned int reg = 0;
	unsigned int mutexID = 0;

	for (mutexID = 0; mutexID < 4; mutexID++) {
		if ((DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA) &
		     (1 << (mutexID + 9))) == (1 << (mutexID + 9))) {
			if (cnt_mutex_timeout % 256 == 0) {
				DISP_ERR
				    ("disp_path_release_mutex() timeout! mutexID = %d\n", mutexID);
				disp_irq_log_module |= (1 << DISP_MODULE_CONFIG);
				disp_irq_log_module |= (1 << DISP_MODULE_MUTEX);
				ddp_dump_info(DISP_MODULE_CONFIG);
				ddp_dump_info(DISP_MODULE_MUTEX);
				reg = DISP_REG_GET(DISP_REG_CONFIG_REG_COMMIT);
				/* print error engine */
				if (reg != 0) {
					if (reg & (1 << 3)) {
						DISP_ERR
						    (" OVL update reg timeout! cnt=%d\n",
						     cnt_mutex_timeout);
						disp_irq_log_module |= (1 << DISP_MODULE_OVL);
						disp_dump_reg(DISP_MODULE_OVL);
					}
					if (reg & (1 << 7)) {
						DISP_ERR
						    (" COLOR update reg timeout! cnt=%d\n",
						     cnt_mutex_timeout);
						disp_irq_log_module |= (1 << DISP_MODULE_COLOR);
						disp_dump_reg(DISP_MODULE_COLOR);
					}
					if (reg & (1 << 6)) {
						DISP_ERR
						    (" WDMA0 update reg timeout! cnt=%d\n",
						     cnt_mutex_timeout);
						disp_irq_log_module |= (1 << DISP_MODULE_WDMA);
						disp_dump_reg(DISP_MODULE_WDMA);
					}
					if (reg & (1 << 10)) {
						DISP_ERR
						    (" RDMA0 update reg timeout! cnt=%d\n",
						     cnt_mutex_timeout);
						disp_irq_log_module |= (1 << DISP_MODULE_RDMA0);
						disp_dump_reg(DISP_MODULE_RDMA0);
					}
					if (reg & (1 << 9)) {
						DISP_ERR
						    (" BLS update reg timeout! cnt=%d\n",
						     cnt_mutex_timeout);
						disp_irq_log_module |= (1 << DISP_MODULE_BLS);
						disp_dump_reg(DISP_MODULE_BLS);
					}
					if (reg & (1 << 12)) {
						DISP_MSG(" RDMA1 update reg timeout!\n");
						disp_irq_log_module |= (1 << DISP_MODULE_RDMA1);
						disp_dump_reg(DISP_MODULE_RDMA1);
					}
				}
			}
			cnt_mutex_timeout++;
			disp_check_clock_tree();
			/* reset mutex */
			DISP_REG_SET(DISP_REG_CONFIG_MUTEX_RST(mutexID), 1);
			DISP_REG_SET(DISP_REG_CONFIG_MUTEX_RST(mutexID), 0);
			DISP_MSG("mutex reset done!\n");
		}
	}
}

static /*__tcmfunc*/ irqreturn_t disp_irq_handler(int irq, void *dev_id)
{
	unsigned int reg_val;
	/* unsigned long index; */
	/* struct timeval tv; */
	/*1. Process ISR */
	if (irq == disp_dev.irq[DISP_REG_OVL]) {
		reg_val = DISP_REG_GET(DISP_REG_OVL_INTSTA);
		if (reg_val & (1 << 0))
			DISP_IRQ("OVL reg update done!\n");
		if (reg_val & (1 << 1)) {
			DISP_IRQ("OVL frame done!\n");
			g_disp_irq.irq_src |= (1 << DISP_MODULE_OVL);
		}
		if (reg_val & (1 << 2)) {
			if (cnt_ovl_underflow % 256 == 0) {
				DISP_ERR("IRQ: OVL frame underrun! cnt=%d\n", cnt_ovl_underflow++);
				disp_check_clock_tree();
				disp_irq_log_module |= (1 << DISP_MODULE_CONFIG);
				disp_irq_log_module |= (1 << DISP_MODULE_MUTEX);
				disp_irq_log_module |= (1 << DISP_MODULE_OVL);
				disp_irq_log_module |= (1 << DISP_MODULE_RDMA0);
				ddp_dump_info(DISP_MODULE_OVL);
				ddp_dump_info(DISP_MODULE_RDMA0);
				ddp_dump_info(DISP_MODULE_COLOR);
				ddp_dump_info(DISP_MODULE_BLS);
				ddp_dump_info(DISP_MODULE_CONFIG);
				ddp_dump_info(DISP_MODULE_MUTEX);
				ddp_dump_info(DISP_MODULE_DSI_CMD);

				disp_dump_reg(DISP_MODULE_OVL);
				disp_dump_reg(DISP_MODULE_RDMA0);
				disp_dump_reg(DISP_MODULE_CONFIG);
				disp_dump_reg(DISP_MODULE_MUTEX);
			} else
				cnt_ovl_underflow++;

		}
		if (reg_val & (1 << 3))
			DISP_IRQ("OVL SW reset done!\n");
		if (reg_val & (1 << 4))
			DISP_IRQ("OVL HW reset done!\n");
		if (reg_val & (1 << 5)) {
			DISP_ERR("IRQ: OVL-L0 not complete until EOF!\n");
			disp_irq_err |= DDP_IRQ_OVL_L0_ABNORMAL;
			disp_check_clock_tree();
		}
		if (reg_val & (1 << 6)) {
			DISP_ERR("IRQ: OVL-L1 not complete until EOF!\n");
			disp_irq_err |= DDP_IRQ_OVL_L1_ABNORMAL;
			disp_check_clock_tree();
		}
		if (reg_val & (1 << 7)) {
			DISP_ERR("IRQ: OVL-L2 not complete until EOF!\n");
			disp_irq_err |= DDP_IRQ_OVL_L2_ABNORMAL;
			disp_check_clock_tree();
		}
		if (reg_val & (1 << 8)) {
			DISP_ERR("IRQ: OVL-L3 not complete until EOF!\n");
			disp_irq_err |= DDP_IRQ_OVL_L3_ABNORMAL;
			disp_check_clock_tree();
		}
		if (reg_val & (1 << 9)) {
			if (cnt_ovl_underflow % 256 == 0)
				DISP_ERR("IRQ: OVL-L0 fifo underflow!\n");
			disp_irq_err |= DDP_IRQ_OVL_L0_UNDERFLOW;
			disp_check_clock_tree();
		}
		if (reg_val & (1 << 10)) {
			if (cnt_ovl_underflow % 256 == 0)
				DISP_ERR("IRQ: OVL-L1 fifo underflow!\n");
			disp_irq_err |= DDP_IRQ_OVL_L1_UNDERFLOW;
			disp_check_clock_tree();
		}
		if (reg_val & (1 << 11)) {
			if (cnt_ovl_underflow % 256 == 0)
				DISP_ERR("IRQ: OVL-L2 fifo underflow!\n");
			disp_irq_err |= DDP_IRQ_OVL_L2_UNDERFLOW;
			disp_check_clock_tree();
		}
		if (reg_val & (1 << 12)) {
			if (cnt_ovl_underflow % 256 == 0)
				DISP_ERR("IRQ: OVL-L3 fifo underflow!\n");
			disp_irq_err |= DDP_IRQ_OVL_L3_UNDERFLOW;
			disp_check_clock_tree();
		}
		/* clear intr */
		DISP_REG_SET(DISP_REG_OVL_INTSTA, ~reg_val);
		MMProfileLogEx(DDP_MMP_Events.OVL_IRQ, MMProfileFlagPulse, reg_val, 0);
		disp_invoke_irq_callbacks(DISP_MODULE_OVL, reg_val);
	} else if (irq == disp_dev.irq[DISP_REG_WDMA]) {
		reg_val = DISP_REG_GET(DISP_REG_WDMA_INTSTA);
		if (reg_val & (1 << 0)) {
			DISP_IRQ("WDMA0 frame done! cnt=%d\n", cnt_wdma_underflow++);
			g_disp_irq.irq_src |= (1 << DISP_MODULE_WDMA);
		}
		if (reg_val & (1 << 1)) {
			DISP_ERR("IRQ: WDMA0 underrun!\n");
			disp_irq_err |= DDP_IRQ_WDMA_UNDERFLOW;
			disp_check_clock_tree();
		}
		/* clear intr */
		DISP_REG_SET(DISP_REG_WDMA_INTSTA, ~reg_val);
		MMProfileLogEx(DDP_MMP_Events.WDMA0_IRQ, MMProfileFlagPulse, reg_val,
			       DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE));
		disp_invoke_irq_callbacks(DISP_MODULE_WDMA, reg_val);
	} else if (irq == disp_dev.irq[DISP_REG_RDMA0]) {
		reg_val = DISP_REG_GET(DISP_REG_RDMA_INT_STATUS);
		if (reg_val & (1 << 0))
			DISP_IRQ("RDMA0 reg update done!\n");
		if (reg_val & (1 << 1)) {
			DISP_IRQ("RDMA0 frame start!\n");
			on_disp_aal_alarm_set();
		}
		if (reg_val & (1 << 2)) {
			DISP_IRQ("RDMA0 frame done!\n");
			g_disp_irq.irq_src |= (1 << DISP_MODULE_RDMA0);
		}
		if (reg_val & (1 << 3)) {
			if (cnt_rdma_abnormal % 256 == 0) {
				DISP_ERR("IRQ: RDMA0 abnormal! cnt=%d\n", cnt_rdma_abnormal++);
				/*disp_check_clock_tree();
				disp_irq_log_module |= (1 << DISP_MODULE_CONFIG);
				disp_irq_log_module |= (1 << DISP_MODULE_MUTEX);
				disp_irq_log_module |= (1 << DISP_MODULE_OVL);
				disp_irq_log_module |= (1 << DISP_MODULE_RDMA0);
				disp_dump_reg(DISP_MODULE_RDMA0);
				disp_dump_reg(DISP_MODULE_CONFIG);
				disp_dump_reg(DISP_MODULE_MUTEX);
				disp_dump_reg(DISP_MODULE_OVL);

				ddp_dump_info(DISP_MODULE_OVL);
				ddp_dump_info(DISP_MODULE_RDMA0);
				ddp_dump_info(DISP_MODULE_CONFIG);
				ddp_dump_info(DISP_MODULE_MUTEX);
				ddp_dump_info(DISP_MODULE_DPI0);
				disp_irq_err |= DDP_IRQ_RDMA_ABNORMAL;*/
			} else
				cnt_rdma_abnormal++;
		}
		if (reg_val & (1 << 4)) {
			if (cnt_rdma_underflow % 256 == 0) {
				DISP_ERR("IRQ: RDMA0 underflow! cnt=%d\n", cnt_rdma_underflow++);
				disp_check_clock_tree();
				disp_irq_log_module |= (1 << DISP_MODULE_OVL);
				disp_irq_log_module |= (1 << DISP_MODULE_RDMA0);
				disp_irq_log_module |= (1 << DISP_MODULE_CONFIG);
				disp_irq_log_module |= (1 << DISP_MODULE_MUTEX);

				disp_print_reg(DISP_MODULE_DPI0);

				ddp_dump_info(DISP_MODULE_RDMA0);
				ddp_dump_info(DISP_MODULE_CONFIG);
				ddp_dump_info(DISP_MODULE_MUTEX);
				ddp_dump_info(DISP_MODULE_DPI0);

				disp_dump_reg(DISP_MODULE_RDMA0);
				disp_dump_reg(DISP_MODULE_CONFIG);
				disp_dump_reg(DISP_MODULE_MUTEX);
				disp_irq_err |= DDP_IRQ_RDMA_UNDERFLOW;
			} else
				cnt_rdma_underflow++;

		}
		/* clear intr */
		DISP_REG_SET(DISP_REG_RDMA_INT_STATUS, ~reg_val);
		MMProfileLogEx(DDP_MMP_Events.RDMA0_IRQ, MMProfileFlagPulse, reg_val, 0);
		disp_invoke_irq_callbacks(DISP_MODULE_RDMA0, reg_val);
	} else if (irq == disp_dev.irq[DISP_REG_RDMA1]) {
		reg_val = DISP_REG_GET(DISP_REG_RDMA_INT_STATUS + DISP_RDMA_OFFSET);
		if (reg_val & (1 << 0))
			DISP_IRQ("RDMA1 reg update done!\n");
		if (reg_val & (1 << 1)) {
			DISP_IRQ("RDMA1 frame start!\n");
			/* if(disp_needWakeUp()) */
			/* { */
			/* disp_update_hist(); */
			/* disp_wakeup_aal(); */
			/* } */
			on_disp_aal_alarm_set();
		}
		if (reg_val & (1 << 2)) {
			DISP_IRQ("RDMA1 frame done!\n");
			g_disp_irq.irq_src |= (1 << DISP_MODULE_RDMA1);
		}
		if (reg_val & (1 << 3)) {
			if (cnt_rdma1_abnormal % 256 == 0) {
				DISP_ERR("IRQ: RDMA1 abnormal! cnt=%d\n", cnt_rdma1_abnormal++);
				disp_check_clock_tree();
				disp_irq_log_module |= (1 << DISP_MODULE_CONFIG);
				disp_irq_log_module |= (1 << DISP_MODULE_MUTEX);
				disp_irq_log_module |= (1 << DISP_MODULE_OVL);
				disp_irq_log_module |= (1 << DISP_MODULE_RDMA1);
				disp_dump_reg(DISP_MODULE_RDMA1);
				disp_dump_reg(DISP_MODULE_CONFIG);
				disp_dump_reg(DISP_MODULE_MUTEX);

				ddp_dump_info(DISP_MODULE_RDMA1);
				ddp_dump_info(DISP_MODULE_CONFIG);
				ddp_dump_info(DISP_MODULE_MUTEX);
				ddp_dump_info(DISP_MODULE_DPI0);
				disp_irq_err |= DDP_IRQ_RDMA_ABNORMAL;
			} else
				cnt_rdma1_abnormal++;
		}
		if (reg_val & (1 << 4)) {
			if (cnt_rdma1_underflow % 256 == 0) {
				DISP_ERR("IRQ: RDMA1 underflow! cnt=%d\n", cnt_rdma1_underflow++);
				disp_check_clock_tree();
				disp_irq_log_module |= (1 << DISP_MODULE_RDMA1);
				disp_irq_log_module |= (1 << DISP_MODULE_CONFIG);
				disp_irq_log_module |= (1 << DISP_MODULE_MUTEX);
				disp_print_reg(DISP_MODULE_DPI0);

				ddp_dump_info(DISP_MODULE_RDMA1);
				ddp_dump_info(DISP_MODULE_CONFIG);
				ddp_dump_info(DISP_MODULE_MUTEX);
				ddp_dump_info(DISP_MODULE_DPI0);

				disp_dump_reg(DISP_MODULE_RDMA1);
				disp_dump_reg(DISP_MODULE_CONFIG);
				disp_dump_reg(DISP_MODULE_MUTEX);
				disp_irq_err |= DDP_IRQ_RDMA_UNDERFLOW;
			} else
				cnt_rdma1_underflow++;

		}
		/* clear intr */
		DISP_REG_SET(DISP_REG_RDMA_INT_STATUS + DISP_RDMA_OFFSET, ~reg_val);
		MMProfileLogEx(DDP_MMP_Events.RDMA1_IRQ, MMProfileFlagPulse, reg_val, 0);
		disp_invoke_irq_callbacks(DISP_MODULE_RDMA1, reg_val);
	} else if (irq == disp_dev.irq[DISP_REG_COLOR]) {
		reg_val = DISP_REG_GET(DISPSYS_COLOR_BASE + 0x0F08);
		/* read LUMA histogram */
		/* clear intr */
		DISP_REG_SET(DISPSYS_COLOR_BASE + 0x0F08, ~reg_val);
		MMProfileLogEx(DDP_MMP_Events.COLOR_IRQ, MMProfileFlagPulse, reg_val, 0);
		/* disp_invoke_irq_callbacks(DISP_MODULE_COLOR, reg_val); */
	} else if (irq == disp_dev.irq[DISP_REG_BLS]) {
		reg_val = DISP_REG_GET(DISP_REG_BLS_INTSTA);

		/* read LUMA & MAX(R,G,B) histogram */
		if (reg_val & 0x1) {
			disp_update_hist();
			disp_wakeup_aal();
		}
		/* clear intr */
		DISP_REG_SET(DISP_REG_BLS_INTSTA, ~reg_val);
		MMProfileLogEx(DDP_MMP_Events.BLS_IRQ, MMProfileFlagPulse, reg_val, 0);
	} else if (irq == disp_dev.irq[DISP_REG_MUTEX]) {
		/* can not do reg update done status after release mutex(for ECO requirement), */
		/* so we have to check update timeout intr here */
		reg_val = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA) & 0x01e0f;

		if (reg_val & 0x01e00)
			disp_check_mutex_status();

		DISP_REG_SET(DISP_REG_CONFIG_MUTEX_INTSTA, ~reg_val);
		disp_invoke_irq_callbacks(DISP_MODULE_MUTEX, reg_val);
	} else if (irq == disp_dev.irq[DISP_REG_CMDQ]) {
#ifdef MTK_ENABLE_CMDQ
		unsigned long value;
		int i;

		reg_val = DISP_REG_GET(DISP_REG_CMDQ_IRQ_FLAG) & 0x00003fff;
		for (i = 0; ((0x00003fff != reg_val) && (i < CMDQ_MAX_THREAD_COUNT)); i++) {
			/* STATUS bit set to 0 means IRQ asserted */
			if (0x0 == (reg_val & (1 << i))) {
				value = DISP_REG_GET(DISP_REG_CMDQ_THRx_IRQ_FLAG(i));
				if (value & 0x12)
					cmdqHandleError(i, (uint32_t) value);
				else if (value & 0x01)
					cmdqHandleDone(i, (uint32_t) value);
				/* mark flag to 1 to denote finished processing */
				/* and we can early-exit if no more threads being asserted */
				reg_val |= (1 << i);
			}
			MMProfileLogEx(DDP_MMP_Events.CMDQ_IRQ, MMProfileFlagPulse, reg_val, i);
		}
#endif
	}

	/* Wakeup event */
	mb();			/* Add memory barrier before the other CPU (may) wakeup */
	wake_up_interruptible(&g_disp_irq_done_queue);

	if ((disp_irq_log_module != 0) || (disp_irq_err != 0))
		wake_up_interruptible(&disp_irq_log_wq);

	return IRQ_HANDLED;
}

void disp_register_rdma1_irq(void)
{
	DISP_REGISTER_IRQ(disp_dev.irq[DISP_REG_RDMA1]);
}

void disp_unregister_rdma1_irq(void)
{
	DISP_UNREGISTER_IRQ(disp_dev.irq[DISP_REG_RDMA1]);
}

static void disp_power_on(DISP_MODULE_ENUM eModule, unsigned int *pu4Record)
{
	unsigned long flag;
	/* unsigned int ret = 0; */
	spin_lock_irqsave(&gPowerOperateLock, flag);

#ifdef DDP_82_72_TODO
	if ((1 << eModule) & g_u4ClockOnTbl)
		DISP_MSG("DDP power %lu is already enabled\n", (unsigned long)eModule);
	else {
		switch (eModule) {

		case DISP_MODULE_WDMA:
			enable_clock(MT_CG_DISP0_DISP_WDMA, "DDP_DRV");
			/* enable_clock(MT_CG_DISP0_WDMA0_SMI , "DDP_DRV"); */
			break;

		case DISP_MODULE_G2D:
			/* enable_clock(MT_CG_DISP0_G2D_ENGINE , "DDP_DRV"); */
			/* enable_clock(MT_CG_DISP0_G2D_SMI , "DDP_DRV"); */
			break;
		default:
			DISP_ERR("disp_power_on:unknown module:%d\n", eModule);
			ret = -1;
			break;
		}

		if (0 == ret) {
			if (0 == g_u4ClockOnTbl)
				enable_clock(MT_CG_DISP0_SMI_LARB0, "DDP_DRV");
			g_u4ClockOnTbl |= (1 << eModule);
			*pu4Record |= (1 << eModule);
		}
	}
#endif

	spin_unlock_irqrestore(&gPowerOperateLock, flag);
}

static void disp_power_off(DISP_MODULE_ENUM eModule, unsigned int *pu4Record)
{
	unsigned long flag;
	/* unsigned int ret = 0; */
	spin_lock_irqsave(&gPowerOperateLock, flag);

#ifdef DDP_82_72_TODO
/* DISP_MSG("power off : %d\n" , eModule); */

	if ((1 << eModule) & g_u4ClockOnTbl) {
		switch (eModule) {
		case DISP_MODULE_WDMA:
			WDMAStop(0);
			WDMAReset(0);
			disable_clock(MT_CG_DISP0_DISP_WDMA, "DDP_DRV");
			/* disable_clock(MT_CG_DISP0_WDMA0_SMI , "DDP_DRV"); */
			break;
		case DISP_MODULE_G2D:
			/* disable_clock(MT_CG_DISP0_G2D_ENGINE , "DDP_DRV"); */
			/* disable_clock(MT_CG_DISP0_G2D_SMI , "DDP_DRV"); */
			break;
		default:
			DISP_ERR("disp_power_off:unsupported format:%d\n", eModule);
			ret = -1;
			break;
		}

		if (0 == ret) {
			g_u4ClockOnTbl &= (~(1 << eModule));
			*pu4Record &= (~(1 << eModule));

			if (0 == g_u4ClockOnTbl)
				disable_clock(MT_CG_DISP0_SMI_LARB0, "DDP_DRV");

		}
	} else
		DISP_MSG("DDP power %lu is already disabled\n", (unsigned long)eModule);

#endif

	spin_unlock_irqrestore(&gPowerOperateLock, flag);
}

unsigned int inAddr = 0, outAddr = 0;

static int disp_set_needupdate(DISP_MODULE_ENUM eModule, unsigned long u4En)
{
	unsigned long flag;

	spin_lock_irqsave(&gRegisterUpdateLock, flag);

	if (u4En)
		u4UpdateFlag |= (1 << eModule);
	else
		u4UpdateFlag &= ~(1 << eModule);

	spin_unlock_irqrestore(&gRegisterUpdateLock, flag);

	return 0;
}

void DISP_REG_SET_FIELD(unsigned long field, unsigned long reg32, unsigned long val)
{
	unsigned long flag;

	spin_lock_irqsave(&gRegisterUpdateLock, flag);
	mt_reg_sync_writel((*(volatile unsigned int *)(reg32) & ~(REG_FLD_MASK(field))) |
			       REG_FLD_VAL((field), (val)), reg32);
	spin_unlock_irqrestore(&gRegisterUpdateLock, flag);
}

int CheckAALUpdateFunc(int i4IsNewFrame)
{
	return (((1 << DISP_MODULE_BLS) & u4UpdateFlag) || i4IsNewFrame
		|| is_disp_aal_alarm_on()) ? 1 : 0;
}

int ConfAALFunc(int i4IsNewFrame)
{
	/*
	 * [ALPS01197868]
	 * When phone resume, if AALService did not calculate the Y curve yet
	 * but the screen refresh is triggered, this function will be called
	 * while the Y curve/backlight may be not valid.
	 * We should enable the Y curve only if the valid Y curve/backlight
	 * is re-calculated, i.e., the BLS update flag is set.
	 */
	if (i4IsNewFrame)
		g_AAL_NewFrameUpdate = 1;

	if ((1 << DISP_MODULE_BLS) & u4UpdateFlag)
		disp_onConfig_aal(i4IsNewFrame);

	disp_set_needupdate(DISP_MODULE_BLS, 0);
	return 0;
}

static int AAL_init;
void disp_aal_lock(void)
{
	if (0 == AAL_init) {
		/* DISP_MSG("disp_aal_lock: register update func\n"); */
		DISP_RegisterExTriggerSource(CheckAALUpdateFunc, ConfAALFunc);
		AAL_init = 1;
	}
	GetUpdateMutex();
}

void disp_aal_unlock(void)
{
	ReleaseUpdateMutex();
	disp_set_needupdate(DISP_MODULE_BLS, 1);
}

int CheckColorUpdateFunc(int i4NotUsed)
{
	return (((1 << DISP_MODULE_COLOR) & u4UpdateFlag) || bls_gamma_dirty) ? 1 : 0;
}

int ConfColorFunc(int i4NotUsed)
{
	DISP_MSG("ConfColorFunc: BLS_EN=0x%x, bls_gamma_dirty=%d\n", DISP_REG_GET(DISP_REG_BLS_EN),
		 bls_gamma_dirty);
	if (bls_gamma_dirty != 0) {
		/* disable BLS */
		if (DISP_REG_GET(DISP_REG_BLS_EN) & 0x1) {
			DISP_MSG("ConfColorFunc: Disable BLS\n");
			DISP_REG_SET(DISP_REG_BLS_EN, 0x00010000);
		}
	} else {
		if (ncs_tuning_mode == 0) {	/* normal mode */
			DpEngine_COLORonInit(DISP_GetScreenWidth(), DISP_GetScreenHeight());
			/* DpEngine_COLORonConfig(fb_width,fb_height); */
			DpEngine_COLORonConfig(DISP_GetScreenWidth(), DISP_GetScreenHeight());
		} else
			ncs_tuning_mode = 0;
		/* enable BLS */
		DISP_REG_SET(DISP_REG_BLS_EN, 0x00010001);
		disp_set_needupdate(DISP_MODULE_COLOR, 0);
	}
	DISP_MSG("ConfColorFunc done: BLS_EN=0x%x, bls_gamma_dirty=%d\n",
		 DISP_REG_GET(DISP_REG_BLS_EN), bls_gamma_dirty);
	return 0;
}

#ifdef CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT
struct task_struct *disp_secure_irq_task = NULL;

/* the API for other code to acquire ddp / ddp_mem session handle */
/* a NULL handle is returned when it fails */
KREE_SESSION_HANDLE ddp_session_handle(void)
{
	DISP_DBG("ddp_session_handle() acquire TEE session\n");
	/* TODO: the race condition here is not taken into consideration. */
	if (NULL == ddp_session) {
		TZ_RESULT ret;

		DISP_DBG("ddp_session_handle() create session\n");

		ret = KREE_CreateSession(TZ_TA_DDP_UUID, &ddp_session);
		if (ret != TZ_RESULT_SUCCESS) {
			DISP_ERR("ddp_session_handle() failed to create session, ret=%d\n", ret);
			return NULL;
		}
	}

	DISP_DBG("ddp_session_handle() session=%x\n", (unsigned int)ddp_session);
	return ddp_session;
}

KREE_SESSION_HANDLE ddp_mem_session_handle(void)
{
	DISP_DBG("ddp_mem_session_handle() acquires TEE memory session\n");
	/* TODO: the race condition here is not taken into consideration. */
	if (NULL == ddp_mem_session) {
		TZ_RESULT ret;

		DISP_DBG("ddp_mem_session_handle() create memory session\n");

		ret = KREE_CreateSession(TZ_TA_MEM_UUID, &ddp_mem_session);
		if (ret != TZ_RESULT_SUCCESS) {
			DISP_ERR("ddp_mem_session_handle() failed to create session: ret=%d\n",
				 ret);
			return NULL;
		}
	}

	DISP_DBG("ddp_mem_session_handle() session=%x\n", (unsigned int)ddp_mem_session);
	return ddp_mem_session;
}

int disp_secure_intr_callback(void *data)
{
	MTEEC_PARAM param[4];
	unsigned int paramTypes;
	TZ_RESULT ret;
	unsigned int irq;

	MTEE_DISP_IRQ_STATUS IrqStatus;
	MTEE_DISP_IRQ_STATUS *pIrqStatus = &IrqStatus;

	KREE_SHAREDMEM_HANDLE p_IRQ_status_share_handle = 0;
	KREE_SHAREDMEM_PARAM sharedParam;
	/* pr_err(" in callback before TEE calls"); */
	/* Register shared memory */
	sharedParam.buffer = pIrqStatus;
	sharedParam.size = sizeof(MTEE_DISP_IRQ_STATUS);
	ret =
	    KREE_RegisterSharedmem(ddp_mem_session_handle(), &p_IRQ_status_share_handle,
				   &sharedParam);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_ERR("disp_register_share_memory Error: %d, line:%d, ddp_mem_session(%d)", ret,
			 __LINE__, (unsigned int)ddp_mem_session_handle());
		return 0;
	}

	static unsigned int ree_wdma0_cnt;
	static unsigned int ree_rdma0_cnt;
	static unsigned int ree_ovl_cnt;

	param[0].value.a = 0;
	param[1].value.a = 0;	/* intr type */
	param[2].value.a = 0;	/* intr status structure */
	param[3].value.a = 0;
	paramTypes =
	    TZ_ParamTypes4(TZPT_VALUE_INPUT, TZPT_VALUE_OUTPUT, TZPT_MEMREF_OUTPUT,
			   TZPT_VALUE_OUTPUT);

	/* set up param[2] data type */
	param[2].memref.handle = (uint32_t) p_IRQ_status_share_handle;
	param[2].memref.offset = 0;
	param[2].memref.size = sizeof(MTEE_DISP_IRQ_STATUS);
	DISP_MSG("config_layer handle=0x%x\n", param[2].memref.handle);

	DISP_MSG("disp_secure_intr_callback start run!\n");
	while (1) {
		ret =
		    KREE_TeeServiceCall(ddp_session_handle(), TZCMD_DDP_INTR_CALLBACK, paramTypes,
					param);
		if (ret != TZ_RESULT_SUCCESS)
			DISP_ERR("KREE_TeeServiceCall(TZCMD_DDP_INTR_CALLBACK) fail, ret=%d\n",
				 ret);
		else
			DISP_DBG("disp_secure_intr_callback called irq=%d, statue_reg=0x%x\n",
				 param[1].value.a, param[2].value.a);

		irq = param[1].value.a;
		DISP_DBG
		    ("irq=0x%08x ovl_status=0x%08x disp_wdma_status=0x%08x rdma0_status=0x%08x",
		     irq, pIrqStatus->OVL_IRQ_STATUS, pIrqStatus->DISP_WDMA_IRQ_STATUS,
		     pIrqStatus->RDMA0_IRQ_STATUS);

		if (1 == ((irq >> MT8127_MDP_ROT_IRQ_BIT) & 0x00000001)) {
			if ((pIrqStatus->MDP_ROT_IRQ_STATUS) & (1 << 0))
				g_disp_irq.irq_src |= (1 << DISP_MODULE_MDP_ROT);
			disp_invoke_irq_callbacks(DISP_MODULE_MDP_ROT,
						  pIrqStatus->MDP_ROT_IRQ_STATUS);
		}
		if (1 == ((irq >> MT8127_DISP_OVL_IRQ_BIT) & 0x00000001)) {
			if ((pIrqStatus->OVL_IRQ_STATUS) & (1 << 1))
				g_disp_irq.irq_src |= (1 << DISP_MODULE_OVL);
			disp_invoke_irq_callbacks(DISP_MODULE_OVL, pIrqStatus->OVL_IRQ_STATUS);
		}
		if (1 == ((irq >> MT8127_MDP_WDMA_IRQ_BIT) & 0x00000001)) {
			if ((pIrqStatus->MDP_WDMA_IRQ_STATUS) & (1 << 0)) {
				g_disp_irq.irq_src |= (1 << DISP_MODULE_MDP_WDMA);
				DISP_DBG("ree mdp_wdma intr done, cnt=%d\n", ree_wdma0_cnt++);
			}
			disp_invoke_irq_callbacks(DISP_MODULE_MDP_WDMA,
						  pIrqStatus->MDP_WDMA_IRQ_STATUS);
		}
		if (1 == ((irq >> MT8127_DISP_WDMA_IRQ_BIT) & 0x00000001)) {
			if ((pIrqStatus->DISP_WDMA_IRQ_STATUS) & (1 << 0)) {
				g_disp_irq.irq_src |= (1 << DISP_MODULE_WDMA);
				DISP_DBG("ree wdma0/ovl intr done, cnt=%d\n", ree_ovl_cnt++);
			}
			disp_invoke_irq_callbacks(DISP_MODULE_WDMA,
						  pIrqStatus->DISP_WDMA_IRQ_STATUS);
		}
		if (1 == ((irq >> MT8127_DISP_RDMA0_IRQ_BIT) & 0x00000001)) {
			if ((pIrqStatus->RDMA0_IRQ_STATUS) & (1 << 2)) {
				g_disp_irq.irq_src |= (1 << DISP_MODULE_RDMA0);
				DISP_DBG("ree rdma0 intr done, cnt=%d\n", ree_rdma0_cnt++);
			}
			disp_invoke_irq_callbacks(DISP_MODULE_RDMA0, pIrqStatus->RDMA0_IRQ_STATUS);
		}
		if (1 == ((irq >> MT8127_DISP_RDMA1_IRQ_BIT) & 0x00000001)) {
			if ((pIrqStatus->RDMA1_IRQ_STATUS) & (1 << 2))
				g_disp_irq.irq_src |= (1 << DISP_MODULE_RDMA1);
			disp_invoke_irq_callbacks(DISP_MODULE_RDMA1, pIrqStatus->RDMA1_IRQ_STATUS);
		}
		/* Wakeup event */
		mb();		/* Add memory barrier before the other CPU (may) wakeup */
		wake_up_interruptible(&g_disp_irq_done_queue);
	}
	return 0;
}

#endif

static const char *disp_reg_name_spy(DISP_DTS_REG_ENUM regs)
{
	switch (regs) {
	case DISP_REG_CONFIG:
		return "mediatek,mt8127-mmsys";
	case DISP_REG_OVL:
		return "mediatek,mt8127-disp-ovl";
	case DISP_REG_RDMA0:
		return "mediatek,mt8127-disp-rdma0";
	case DISP_REG_WDMA:
		return "mediatek,mt8127-disp-wdma";
	case DISP_REG_BLS:
		return "mediatek,mt8127-disp-bls";
	case DISP_REG_COLOR:
		return "mediatek,mt8127-disp-color";
	case DISP_REG_DSI:
		return "mediatek,mt8127-disp-dsi";
	case DISP_REG_DPI0:
		return "mediatek,mt8127-disp-dpi0";
	case DISP_REG_MUTEX:
		return "mediatek,mt8127-mm-mutex";
	case DISP_REG_CMDQ:
		return "mediatek,mt8127-mm-cmdq";
	case DISP_REG_SMI_LARB0:
		return "mediatek,mt8127-smi-larb";
	case DISP_REG_SMI_COMMON:
		return "mediatek,mt8127-smi";
	case DISP_REG_RDMA1:
		return "mediatek,mt8127-disp-rdma1";
	case DISP_REG_UFOE:
		return "mediatek,mt8127-disp-ufoe";
	case DISP_REG_DPI1:
		return "mediatek,mt8127-disp-dpi1";
	case DISP_REG_HDMI:
		return "mediatek,mt8127-disp-hdmi";
	case DISP_REG_LVDS:
		return "mediatek,mt8127-disp-lvds";
	case DISP_REG_TVE:
		return "mediatek,mt8127-disp-tve";
	case DISP_REG_MIPI:
		return "mediatek,mt8127-disp-mipi";
	case DISP_REG_MDP_RDMA:
		return "mediatek,mt8127-mdp-rdma";
	case DISP_REG_MDP_RSZ0:
		return "mediatek,mt8127-mdp-rsz0";
	case DISP_REG_MDP_RSZ1:
		return "mediatek,mt8127-mdp-rsz1";
	case DISP_REG_MDP_TDSHP:
		return "mediatek,mt8127-mdp-tdshp";
	case DISP_REG_MDP_WDMA:
		return "mediatek,mt8127-mdp-wdma";
	case DISP_REG_MDP_WROT:
		return "mediatek,mt8127-mdp-wrot";
	default:
		return "mediatek,mt8127-disp-unknown";
	}
}

static const char *disp_clk_name_spy(DISP_DTS_REG_ENUM regs, unsigned int index)
{
	switch (regs) {
	case DISP_REG_CONFIG:
		if (index == 0)
			return "clk_mm_sel";
	case DISP_REG_OVL:
		if (index == 0)
			return "clk_disp_ovl";
	case DISP_REG_RDMA0:
		if (index == 0)
			return "clk_disp_rdma0";
	case DISP_REG_WDMA:
		if (index == 0)
			return "clk_disp_wdma";
	case DISP_REG_BLS:
		if (index == 0)
			return "clk_disp_bls";
		if (index == 1)
			return "clk_disp_bls_26m";
		if (index == 2)
			return "clk26m";
		if (index == 3)
			return "pwm_sel";
	case DISP_REG_COLOR:
		if (index == 0)
			return "clk_disp_color";
	case DISP_REG_DSI:
		if (index == 0)
			return "clk_dsi_engine";
		else if (index == 1)
			return "clk_dsi_dig";
	case DISP_REG_DPI0:
		if (index == 0)
			return "clk_dpi_digl";
		else if (index == 1)
			return "clk_dpi_engine";
		else if (index == 2)
			return "clk_lvds_pixel";
		else if (index == 3)
			return "clk_lvds_cts";
	case DISP_REG_MUTEX:
		if (index == 0)
			return "clk_mm_mutex_32k";
		else if (index == 1)
			return "clk_mm_mutex";
	case DISP_REG_CMDQ:
		if (index == 0)
			return "clk_mm_cmdq";
	case DISP_REG_SMI_LARB0:
		if (index == 0)
			return "smi";
	case DISP_REG_SMI_COMMON:
		if (index == 0)
			return "clk_smi_common";
	case DISP_REG_RDMA1:
		if (index == 0)
			return "clk_disp_rdma1";
	case DISP_REG_UFOE:
		if (index == 0)
			return "clk_disp_ufoe";
	case DISP_REG_DPI1:
		if (index == 0)
			return "clk_dpi1_digl";
		else if (index == 1)
			return "clk_dpi1_engine";
	case DISP_REG_HDMI:
		if (index == 0)
			return "clk_hdmi_pixel";
		else if (index == 1)
			return "clk_hdmi_pll";
		else if (index == 2)
			return "clk_hdmi_audio";
		else if (index == 3)
			return "clk_hdmi_spdif";
	case DISP_REG_LVDS:
		if (index == 0)
			return "clk_lvds_pixel";
		else if (index == 1)
			return "clk_lvds_cts";
	case DISP_REG_TVE:
		if (index == 0)
			return "clk_tve_output";
		else if (index == 1)
			return "clk_tve_input";
		/* else if (index == 2)
			return "clk_tve_fmm"; */
	case DISP_REG_MIPI:
		if (index == 0)
			return "clk_mipipll";
	case DISP_REG_MDP_RDMA:
		if (index == 0)
			return "clk_mdp_rdma";
		else if (index == 1)
			return "clk_cam_mdp";
		else if (index == 2)
			return "clk_img_cam_smi";
		else if (index == 3)
			return "clk_img_cam_cam";
	case DISP_REG_MDP_RSZ0:
		if (index == 0)
			return "clk_mdp_rsz0";
	case DISP_REG_MDP_RSZ1:
		if (index == 0)
			return "clk_mdp_rsz1";
	case DISP_REG_MDP_WDMA:
		if (index == 0)
			return "clk_mdp_wdma";
		else if (index == 1)
			return "clk_img_sen_tg";
		else if (index == 2)
			return "clk_img_sen_cam";
		else if (index == 3)
			return "clk_img_larb2_smi";
	case DISP_REG_MDP_WROT:
		if (index == 0)
			return "clk_mdp_wrot";
	case DISP_REG_MDP_TDSHP:
		if (index == 0)
			return "clk_mdp_tdshp";
		else if (index == 1)
			return "clk_img_nr";
	default:
		return "clk_unknown";
	}

	return "clk_unknown";
}

static int disp_is_intr_enable(DISP_DTS_REG_ENUM module)
{
	switch (module) {
	case DISP_REG_MUTEX:
	case DISP_REG_RDMA0:
	case DISP_REG_RDMA1:
	case DISP_REG_WDMA:
	case DISP_REG_CMDQ:
	case DISP_REG_COLOR:
	case DISP_REG_BLS:
		return 1;
	case DISP_REG_CONFIG:
	case DISP_REG_OVL:
	case DISP_REG_DSI:
	case DISP_REG_UFOE:
	case DISP_REG_DPI0:
	case DISP_REG_SMI_LARB0:
	case DISP_REG_SMI_COMMON:
		return 0;
	default:
		return 0;
	}
}

void disp_register_intr(unsigned int irq, unsigned int secure)
{
#ifdef CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT
	MTEEC_PARAM param[4];
	unsigned int paramTypes;
	TZ_RESULT ret;

	param[0].value.a = irq;
	param[1].value.a = secure;
	paramTypes = TZ_ParamTypes2(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);

	DISP_MSG("disp_register_intr irq: %d, secure: %d\n", irq, secure);

	if (secure == 0) {
		/* unregister irq in TEE */
		ret =
		    KREE_TeeServiceCall(ddp_session_handle(), TZCMD_DDP_REGISTER_INTR, paramTypes,
					param);
		if (ret != TZ_RESULT_SUCCESS) {
			DISP_ERR("KREE_TeeServiceCall(TZCMD_DDP_REGISTER_INTR) fail, ret=%d\n",
				 ret);
		}

		if (irq == disp_dev.irq[DISP_REG_WDMA])
			MMProfileLogEx(MTKFB_MMP_Events.OVL_WDMA_IRQ_InTEE, MMProfileFlagEnd, irq,
				       secure);
		else if (irq == disp_dev.irq[DISP_REG_RDMA0])
			MMProfileLogEx(MTKFB_MMP_Events.RDMA0_IRQ_InTEE, MMProfileFlagEnd, irq,
				       secure);
		else if (irq == disp_dev.irq[DISP_REG_RDMA1])
			MMProfileLogEx(MTKFB_MMP_Events.RDMA1_IRQ_InTEE, MMProfileFlagEnd, irq,
				       secure);

		/* register irq in REE */
		DISP_REGISTER_IRQ(irq);
		/* enable_irq(irq);        // request irq will also enable irq */
	} else {
		/* unregister irq in REE */
		disable_irq(irq);
		DISP_UNREGISTER_IRQ(irq);

		if (irq == disp_dev.irq[DISP_REG_WDMA])
			MMProfileLogEx(MTKFB_MMP_Events.OVL_WDMA_IRQ_InTEE, MMProfileFlagStart, irq,
				       secure);
		else if (irq == disp_dev.irq[DISP_REG_RDMA0])
			MMProfileLogEx(MTKFB_MMP_Events.RDMA0_IRQ_InTEE, MMProfileFlagStart, irq,
				       secure);
		else if (irq == disp_dev.irq[DISP_REG_RDMA1])
			MMProfileLogEx(MTKFB_MMP_Events.RDMA1_IRQ_InTEE, MMProfileFlagStart, irq,
				       secure);

		/* register irq in TEE */
		ret =
		    KREE_TeeServiceCall(ddp_session_handle(), TZCMD_DDP_REGISTER_INTR, paramTypes,
					param);
		if (ret != TZ_RESULT_SUCCESS) {
			DISP_ERR("KREE_TeeServiceCall(TZCMD_DDP_REGISTER_INTR) fail, ret=%d\n",
				 ret);
		}
	}
#endif
}


void disp_request_irq_init(void)
{
	int i, ret;

	for (i = 0; i < DISP_REG_NUM; i++) {
		if (disp_is_intr_enable(i) && disp_dev.irq[i]) {
			ret = request_irq(disp_dev.irq[i], (irq_handler_t) disp_irq_handler,
					  IRQF_TRIGGER_NONE, disp_reg_name_spy(i), NULL);
			if (ret)
				DISP_ERR
					("Unable to request %s IRQ, request_irq fail, i=%d, irq=%d\n",
					 disp_reg_name_spy(i), i, disp_dev.irq[i]);
			else
				DISP_MSG("disp_request_irq_init: %s\n", disp_reg_name_spy(i));
		}
	}
}


int disp_color_set_pq_param(void *arg)
{
	DISP_PQ_PARAM *pq_param;

	DISP_RegisterExTriggerSource(CheckColorUpdateFunc, ConfColorFunc);

	GetUpdateMutex();

	pq_param = get_Color_config();
	if (copy_from_user(pq_param, (void *)arg, sizeof(DISP_PQ_PARAM))) {
		DISP_MSG("disp driver : DISP_IOCTL_SET_PQPARAM Copy from user failed\n");
		ReleaseUpdateMutex();
		return -EFAULT;
	}

	ReleaseUpdateMutex();

	disp_set_needupdate(DISP_MODULE_COLOR, 1);

	return 0;
}

static int disp_get_mutex_status(void)
{
	return disp_mutex_status;
}

unsigned int g_reg_cfg[100];
unsigned int g_reg_mtx[50];
unsigned int g_reg_ovl[100];
unsigned int g_reg_clr[20];
unsigned int g_reg_bls[50];
unsigned int g_reg_wdma[50];
unsigned int g_reg_rdma[50];
int disp_dump_reg(DISP_MODULE_ENUM module)
{
	unsigned int i = 0;
	unsigned int idx = 0;

	DISP_NOTICE("disp_dump_reg: %d\n", module);

#ifdef CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT
	if ((gBitbltSecure == 1) &&
	    (module == DISP_MODULE_MDP_ROT || module == DISP_MODULE_WDMA
	     || module == DISP_MODULE_MDP_WDMA || module == DISP_MODULE_OVL)) {
		MTEEC_PARAM param[4];
		unsigned int paramTypes;
		TZ_RESULT ret;

		param[0].value.a = module;
		paramTypes = TZ_ParamTypes1(TZPT_VALUE_INPUT);
		ret =
		    KREE_TeeServiceCall(ddp_session_handle(), TZCMD_DDP_DUMP_REG, paramTypes,
					param);
		if (ret != TZ_RESULT_SUCCESS)
			DISP_ERR("KREE_TeeServiceCall(TZCMD_DDP_DUMP_REG) fail, ret=%d\n", ret);

		return 0;
	}
#endif

	switch (module) {
	case DISP_MODULE_OVL:
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_STA);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_INTEN);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_INTSTA);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_EN);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_TRIG);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RST);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_ROI_SIZE);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_DATAPATH_CON);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_ROI_BGCLR);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_SRC_CON);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L0_CON);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L0_SRCKEY);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L0_SRC_SIZE);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L0_OFFSET);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L0_ADDR);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L0_PITCH);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA0_CTRL);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA0_MEM_START_TRIG);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA0_MEM_SLOW_CON);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA0_FIFO_CTRL);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L1_CON);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L1_SRCKEY);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L1_SRC_SIZE);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L1_OFFSET);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L1_ADDR);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L1_PITCH);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA1_CTRL);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA1_MEM_START_TRIG);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA1_MEM_SLOW_CON);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA1_FIFO_CTRL);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L2_CON);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L2_SRCKEY);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L2_SRC_SIZE);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L2_OFFSET);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L2_ADDR);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L2_PITCH);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA2_CTRL);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA2_MEM_START_TRIG);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA2_MEM_SLOW_CON);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA2_FIFO_CTRL);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L3_CON);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L3_SRCKEY);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L3_SRC_SIZE);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L3_OFFSET);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L3_ADDR);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_L3_PITCH);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA3_CTRL);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA3_MEM_START_TRIG);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA3_MEM_SLOW_CON);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA3_FIFO_CTRL);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_DEBUG_MON_SEL);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING2);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING2);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING2);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING2);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_FLOW_CTRL_DBG);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_ADDCON_DBG);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_OUTMUX_DBG);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA0_DBG);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA1_DBG);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA2_DBG);
		g_reg_ovl[i++] = DISP_REG_GET(DISP_REG_OVL_RDMA3_DBG);
		break;

	case DISP_MODULE_COLOR:
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_START);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_INTEN);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_INTSTA);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_OUT_SEL);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_FRAME_DONE_DEL);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_CRC);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_SW_SCRATCH);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_RDY_SEL);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_RDY_SEL_EN);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_CK_ON);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_INTERNAL_IP_WIDTH);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_INTERNAL_IP_HEIGHT);
		g_reg_clr[i++] = DISP_REG_GET(DISP_REG_COLOR_CM1_EN);
		break;

	case DISP_MODULE_BLS:
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_EN);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_RST);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_INTEN);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_INTSTA);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_BLS_SETTING);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_FANA_SETTING);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_SRC_SIZE);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_GAIN_SETTING);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_MANUAL_GAIN);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_MANUAL_MAXCLR);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_GAMMA_SETTING);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_GAMMA_BOUNDARY);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_LUT_UPDATE);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_MAXCLR_THD);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_DISTPT_THD);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_MAXCLR_LIMIT);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_DISTPT_LIMIT);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_AVE_SETTING);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_AVE_LIMIT);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_DISTPT_SETTING);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_HIS_CLEAR);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_SC_DIFF_THD);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_SC_BIN_THD);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_MAXCLR_GRADUAL);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_DISTPT_GRADUAL);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_FAST_IIR_XCOEFF);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_FAST_IIR_YCOEFF);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_SLOW_IIR_XCOEFF);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_SLOW_IIR_YCOEFF);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_PWM_DUTY);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_PWM_GRADUAL);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_PWM_CON);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_PWM_MANUAL);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_DEBUG);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_PATTERN);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_CHKSUM);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_PWM_DUTY_RD);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_FRAME_AVE_RD);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_MAXCLR_RD);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_DISTPT_RD);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_GAIN_RD);
		g_reg_bls[i++] = DISP_REG_GET(DISP_REG_BLS_SC_RD);

		break;

	case DISP_MODULE_WDMA:
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_INTEN);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_INTSTA);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_EN);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_RST);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_SMI_CON);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_CFG);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_DST_ADDR);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_DST_W_IN_BYTE);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_ALPHA);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_BUF_ADDR);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_STA);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_BUF_CON1);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_BUF_CON2);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_PRE_ADD0);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_PRE_ADD2);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_POST_ADD0);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_POST_ADD2);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_DST_U_ADDR);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_DST_V_ADDR);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_DST_UV_PITCH);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_DITHER_CON);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_FLOW_CTRL_DBG);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_EXEC_DBG);
		g_reg_wdma[i++] = DISP_REG_GET(DISP_REG_WDMA_CLIP_DBG);
		break;

	case DISP_MODULE_RDMA0:
	case DISP_MODULE_RDMA1:
		if (module == DISP_MODULE_RDMA0)
			idx = 0;
		else if (module == DISP_MODULE_RDMA1)
			idx = 1;

		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_INT_ENABLE + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_INT_STATUS + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_GLOBAL_CON + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_1 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_MEM_CON + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_MEM_START_ADDR + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_MEM_SRC_PITCH + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_MEM_GMC_SETTING_0 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_MEM_SLOW_CON + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_MEM_GMC_SETTING_1 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_FIFO_CON + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_00 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_01 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_02 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_10 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_11 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_12 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_20 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_21 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_22 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_PRE_ADD0 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_PRE_ADD1 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_PRE_ADD2 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_POST_ADD0 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_POST_ADD1 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_CF_POST_ADD2 + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_DUMMY + DISP_RDMA_OFFSET * idx);
		g_reg_rdma[i++] = DISP_REG_GET(DISP_REG_RDMA_DEBUG_OUT_SEL + DISP_RDMA_OFFSET * idx);
		break;

	case DISP_MODULE_DPI0:
		DISP_MSG("===== DISP DPI0 Reg Dump: ============\n");
		DPI_DumpRegisters();
		break;

	case DISP_MODULE_DSI_VDO:
	case DISP_MODULE_DSI_CMD:
		DSI_DumpRegisters();
		break;

	case DISP_MODULE_CONFIG:
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_CAM_MDP_MOUT_EN);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MDP_RDMA_MOUT_EN);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MDP_RSZ0_MOUT_EN);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MDP_RSZ1_MOUT_EN);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MDP_TDSHP_MOUT_EN);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL_MOUT_EN);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MOUT_RST);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MDP_RSZ0_SEL);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MDP_RSZ1_SEL);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MDP_TDSHP_SEL);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MDP_WROT_SEL);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MDP_WDMA_SEL);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_DISP_OUT_SEL);	/* 0x4c */
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_DISP_UFOE_SEL);	/* 0x50 */
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_DPI0_SEL);	/* 0x54 */
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_DPI1_SEL);	/* 0x64 */
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_SET0);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CLR0);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_SET1);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CLR1);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_HW_DCM_DIS0);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_SET0);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_CLR0);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_HW_DCM_DIS1);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_SET1);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_CLR1);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_SW_RST_B);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_LCM_RST_B);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MBIST_DONE);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MBIST_FAIL0);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MBIST_FAIL1);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MBIST_HOLDB);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MBIST_MODE);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MBIST_BSEL0);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MBIST_BSEL1);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MBIST_CON);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MEM_DELSEL0);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MEM_DELSEL1);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MEM_DELSEL2);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MEM_DELSEL3);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DEBUG_OUT_SEL);
		g_reg_cfg[i++] = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DUMMY);
		g_reg_cfg[i++] = DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x860);
		g_reg_cfg[i++] = DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x868);
		break;

	case DISP_MODULE_MUTEX:
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTEN);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_REG_UPD_TIMEOUT);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_REG_COMMIT);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_EN);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX0);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_RST);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_MOD);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_SOF);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_EN);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX1);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_RST);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_MOD);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_SOF);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_EN);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX2);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_RST);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_MOD);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_SOF);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_EN);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX3);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_RST);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_MOD);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_SOF);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_EN);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX4);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_RST);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_MOD);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_SOF);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_EN);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX5);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_RST);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_MOD);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_SOF);
		g_reg_mtx[i++] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_DEBUG_OUT_SEL);
		break;

	default:
		DISP_MSG("error, reg_dump, unknown module=%d\n", module);
		break;
	}

	disp_irq_log_module |= (1 << module);
	wake_up_interruptible(&disp_irq_log_wq);
	return 0;
}

void disp_print_reg(DISP_MODULE_ENUM module)
{
	unsigned int i = 0;

	switch (module) {
	case DISP_MODULE_OVL:
		DISP_MSG("===== DISP OVL Reg Dump: ============\n");
		DISP_MSG("(000)OVL_STA			   =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(004)OVL_INTEN                   =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(008)OVL_INTSTA                  =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(00C)OVL_EN                      =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(010)OVL_TRIG                    =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(014)OVL_RST                     =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(020)OVL_ROI_SIZE                =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(020)OVL_DATAPATH_CON            =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(028)OVL_ROI_BGCLR               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(02C)OVL_SRC_CON                 =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(030)OVL_L0_CON                  =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(034)OVL_L0_SRCKEY               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(038)OVL_L0_SRC_SIZE             =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(03C)OVL_L0_OFFSET               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(040)OVL_L0_ADDR                 =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(044)OVL_L0_PITCH                =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0C0)OVL_RDMA0_CTRL              =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0C4)OVL_RDMA0_MEM_START_TRIG    =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0C8)OVL_RDMA0_MEM_GMC_SETTING   =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0CC)OVL_RDMA0_MEM_SLOW_CON      =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0D0)OVL_RDMA0_FIFO_CTRL	   =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(050)OVL_L1_CON                  =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(054)OVL_L1_SRCKEY               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(058)OVL_L1_SRC_SIZE             =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(05C)OVL_L1_OFFSET               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(060)OVL_L1_ADDR                 =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(064)OVL_L1_PITCH                =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0E0)OVL_RDMA1_CTRL              =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0E4)OVL_RDMA1_MEM_START_TRIG    =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0E8)OVL_RDMA1_MEM_GMC_SETTING   =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0EC)OVL_RDMA1_MEM_SLOW_CON      =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0F0)OVL_RDMA1_FIFO_CTRL         =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(070)OVL_L2_CON                  =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(074)OVL_L2_SRCKEY               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(078)OVL_L2_SRC_SIZE             =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(07C)OVL_L2_OFFSET               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(080)OVL_L2_ADDR                 =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(084)OVL_L2_PITCH                =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(100)OVL_RDMA2_CTRL              =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(104)OVL_RDMA2_MEM_START_TRIG    =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(108)OVL_RDMA2_MEM_GMC_SETTING   =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(10C)OVL_RDMA2_MEM_SLOW_CON      =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(110)OVL_RDMA2_FIFO_CTRL         =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(090)OVL_L3_CON                  =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(094)OVL_L3_SRCKEY               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(098)OVL_L3_SRC_SIZE             =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(09C)OVL_L3_OFFSET               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0A0)OVL_L3_ADDR                 =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(0A4)OVL_L3_PITCH                =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(120)OVL_RDMA3_CTRL              =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(124)OVL_RDMA3_MEM_START_TRIG    =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(128)OVL_RDMA3_MEM_GMC_SETTING   =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(12C)OVL_RDMA3_MEM_SLOW_CON      =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(130)OVL_RDMA3_FIFO_CTRL         =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(1C4)OVL_DEBUG_MON_SEL           =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(1C4)OVL_RDMA0_MEM_GMC_SETTING2  =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(1C8)OVL_RDMA1_MEM_GMC_SETTING2  =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(1CC)OVL_RDMA2_MEM_GMC_SETTING2  =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(1D0)OVL_RDMA3_MEM_GMC_SETTING2  =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(240)OVL_FLOW_CTRL_DBG           =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(244)OVL_ADDCON_DBG              =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(248)OVL_OUTMUX_DBG              =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(24C)OVL_RDMA0_DBG               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(250)OVL_RDMA1_DBG               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(254)OVL_RDMA2_DBG               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("(258)OVL_RDMA3_DBG               =0x%x\n", g_reg_ovl[i++]);
		DISP_MSG("TOTAL dump %d registers\n", i);
		break;

	case DISP_MODULE_COLOR:
		DISP_MSG("===== DISP Color Reg Dump: ============\n");
		DISP_MSG("(0x0F00)DISP_COLOR_START             =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F04)DISP_COLOR_INTEN             =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F08)DISP_COLOR_INTSTA            =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F0C)DISP_COLOR_OUT_SEL           =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F10)DISP_COLOR_FRAME_DONE_DEL    =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F14)DISP_COLOR_CRC               =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F18)DISP_COLOR_SW_SCRATCH        =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F20)DISP_COLOR_RDY_SEL           =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F24)DISP_COLOR_RDY_SEL_EN        =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F28)DISP_COLOR_CK_ON             =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F50)DISP_COLOR_INTERNAL_IP_WIDTH =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F54)DISP_COLOR_INTERNAL_IP_HEIGHT=0x%x\n", g_reg_clr[i++]);
		DISP_MSG("(0x0F60)DISP_COLOR_CM1_EN            =0x%x\n", g_reg_clr[i++]);
		DISP_MSG("TOTAL dump %d registers\n", i);
		break;

	case DISP_MODULE_BLS:
		DISP_MSG("===== DISP BLS Reg Dump: ============\n");
		DISP_MSG("(0x0 )BLS_EN                =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x4 )BLS_RST               =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x8 )BLS_INTEN             =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0xC )BLS_INTSTA            =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x10)BLS_BLS_SETTING       =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x14)BLS_FANA_SETTING      =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x18)BLS_SRC_SIZE          =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x20)BLS_GAIN_SETTING      =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x24)BLS_MANUAL_GAIN       =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x28)BLS_MANUAL_MAXCLR     =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x30)BLS_GAMMA_SETTING     =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x34)BLS_GAMMA_BOUNDARY    =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x38)BLS_LUT_UPDATE        =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x60)BLS_MAXCLR_THD        =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x64)BLS_DISTPT_THD        =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x68)BLS_MAXCLR_LIMIT      =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x6C)BLS_DISTPT_LIMIT      =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x70)BLS_AVE_SETTING       =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x74)BLS_AVE_LIMIT         =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x78)BLS_DISTPT_SETTING    =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x7C)BLS_HIS_CLEAR         =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x80)BLS_SC_DIFF_THD       =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x84)BLS_SC_BIN_THD        =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x88)BLS_MAXCLR_GRADUAL    =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x8C)BLS_DISTPT_GRADUAL    =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x90)BLS_FAST_IIR_XCOEFF   =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x94)BLS_FAST_IIR_YCOEFF   =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x98)BLS_SLOW_IIR_XCOEFF   =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x9C)BLS_SLOW_IIR_YCOEFF   =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0xA0)BLS_PWM_DUTY          =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0xA4)BLS_PWM_GRADUAL       =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0xA8)BLS_PWM_CON           =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0xAC)BLS_PWM_MANUAL        =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0xB0)BLS_DEBUG             =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0xB4)BLS_PATTERN           =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0xB8)BLS_CHKSUM            =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x200)BLS_PWM_DUTY_RD      =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x204)BLS_FRAME_AVE_RD     =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x208)BLS_MAXCLR_RD        =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x20C)BLS_DISTPT_RD        =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x210)BLS_GAIN_RD          =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("(0x214)BLS_SC_RD            =0x%x\n", g_reg_bls[i++]);
		DISP_MSG("TOTAL dump %d registers\n", i);
		break;

	case DISP_MODULE_WDMA:
		DISP_MSG("===== DISP WDMA0 Reg Dump: ============\n");
		DISP_MSG("(000)WDMA_INTEN          =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(004)WDMA_INTSTA         =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(008)WDMA_EN             =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(00C)WDMA_RST            =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(010)WDMA_SMI_CON        =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(014)WDMA_CFG            =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(018)WDMA_SRC_SIZE       =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(01C)WDMA_CLIP_SIZE      =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(020)WDMA_CLIP_COORD     =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(024)WDMA_DST_ADDR       =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(028)WDMA_DST_W_IN_BYTE  =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(02C)WDMA_ALPHA          =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(030)WDMA_BUF_ADDR       =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(034)WDMA_STA            =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(038)WDMA_BUF_CON1       =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(03C)WDMA_BUF_CON2       =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(040)WDMA_C00            =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(044)WDMA_C02            =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(048)WDMA_C10            =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(04C)WDMA_C12            =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(050)WDMA_C20            =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(054)WDMA_C22            =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(058)WDMA_PRE_ADD0       =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(05C)WDMA_PRE_ADD2       =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(060)WDMA_POST_ADD0      =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(064)WDMA_POST_ADD2      =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(070)WDMA_DST_U_ADDR     =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(074)WDMA_DST_V_ADDR     =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(078)WDMA_DST_UV_PITCH   =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(090)WDMA_DITHER_CON     =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(0A0)WDMA_FLOW_CTRL_DBG  =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(0A4)WDMA_EXEC_DBG       =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("(0A8)WDMA_CLIP_DBG       =0x%x\n", g_reg_wdma[i++]);
		DISP_MSG("TOTAL dump %d registers\n", i);
		break;

	case DISP_MODULE_RDMA0:
	case DISP_MODULE_RDMA1:
		if (module == DISP_MODULE_RDMA0)
			DISP_MSG("===== DISP RDMA0 Reg Dump: ========\n");
		else if (module == DISP_MODULE_RDMA1)
			DISP_MSG("===== DISP RDMA1 Reg Dump: ========\n");

		DISP_MSG("(000)RDMA_INT_ENABLE        =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(004)RDMA_INT_STATUS        =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(010)RDMA_GLOBAL_CON        =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(014)RDMA_SIZE_CON_0        =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(018)RDMA_SIZE_CON_1        =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(024)RDMA_MEM_CON           =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(028)RDMA_MEM_START_ADDR    =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(02C)RDMA_MEM_SRC_PITCH     =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(030)RDMA_MEM_GMC_SETTING_0 =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(034)RDMA_MEM_SLOW_CON      =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(030)RDMA_MEM_GMC_SETTING_1 =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(040)RDMA_FIFO_CON          =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(054)RDMA_CF_00             =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(058)RDMA_CF_01             =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(05C)RDMA_CF_02             =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(060)RDMA_CF_10             =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(064)RDMA_CF_11             =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(068)RDMA_CF_12             =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(06C)RDMA_CF_20             =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(070)RDMA_CF_21             =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(074)RDMA_CF_22             =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(078)RDMA_CF_PRE_ADD0       =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(07C)RDMA_CF_PRE_ADD1       =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(080)RDMA_CF_PRE_ADD2       =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(084)RDMA_CF_POST_ADD0      =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(088)RDMA_CF_POST_ADD1      =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(08C)RDMA_CF_POST_ADD2      =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(090)RDMA_DUMMY             =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("(094)RDMA_DEBUG_OUT_SEL     =0x%x\n", g_reg_rdma[i++]);
		DISP_MSG("TOTAL dump %d registers\n", i);
		break;

	case DISP_MODULE_DPI0:
		break;

	case DISP_MODULE_DSI_VDO:
	case DISP_MODULE_DSI_CMD:
		break;

	case DISP_MODULE_CONFIG:
		DISP_MSG("===== DISP DISP_REG_MM_CONFIG Reg Dump: ============\n");
		DISP_MSG("(0x01c)CAM_MDP_MOUT_EN         =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x020)MDP_RDMA_MOUT_EN        =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x024)MDP_RSZ0_MOUT_EN        =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x028)MDP_RSZ1_MOUT_EN        =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x02c)MDP_TDSHP_MOUT_EN       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x030)DISP_OVL_MOUT_EN        =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x034)MMSYS_MOUT_RST          =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x038)MDP_RSZ0_SEL            =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x03c)MDP_RSZ1_SEL            =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x040)MDP_TDSHP_SEL           =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x044)MDP_WROT_SEL            =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x048)MDP_WDMA_SEL            =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x04c)DISP_OUT_SEL            =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x050)DISP_UFOE_SEL           =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x054)DPI0_SEL                =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x064)DPI1_SEL                =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x100)MMSYS_CG_CON0           =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x104)MMSYS_CG_SET0           =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x108)MMSYS_CG_CLR0           =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x110)MMSYS_CG_CON1           =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x114)MMSYS_CG_SET1           =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x118)MMSYS_CG_CLR1           =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x120)MMSYS_HW_DCM_DIS0       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x124)MMSYS_HW_DCM_DIS_SET0   =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x128)MMSYS_HW_DCM_DIS_CLR0   =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x12c)MMSYS_HW_DCM_DIS1       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x130)MMSYS_HW_DCM_DIS_SET1   =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x134)MMSYS_HW_DCM_DIS_CLR1   =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x138)MMSYS_SW_RST_B          =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x13c)MMSYS_LCM_RST_B         =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x800)MMSYS_MBIST_DONE        =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x804)MMSYS_MBIST_FAIL0       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x808)MMSYS_MBIST_FAIL1       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x80C)MMSYS_MBIST_HOLDB       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x810)MMSYS_MBIST_MODE        =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x814)MMSYS_MBIST_BSEL0       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x818)MMSYS_MBIST_BSEL1       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x81c)MMSYS_MBIST_CON         =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x820)MMSYS_MEM_DELSEL0       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x824)MMSYS_MEM_DELSEL1       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x828)MMSYS_MEM_DELSEL2       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x82c)MMSYS_MEM_DELSEL3       =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x830)MMSYS_DEBUG_OUT_SEL     =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(0x840)MMSYS_DUMMY             =0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(10206040)CONFIG_CLOCK_DUMMY=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(14000860)CONFIG_VALID=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(14000868)CONFIG_READY=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(000)SMI_0=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(010)SMI_10=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(060)SMI_60=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(064)SMI_64=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(08c)SMI_8c=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(450)SMI_450_REQ0=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(454)SMI_454_REQ1=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(600)SMI_600=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(604)SMI_604=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(610)SMI_610=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("(614)SMI_614=0x%x\n", g_reg_cfg[i++]);
		DISP_MSG("TOTAL dump %d registers\n", i);
		break;

	case DISP_MODULE_MUTEX:
		DISP_MSG("===== DISP DISP_REG_MUTEX_CONFIG Reg Dump: ============\n");
		DISP_MSG
		    ("(0x0  )DISP_MUTEX_INTEN							 =0x%x\n",
		     g_reg_mtx[i++]);
		DISP_MSG("(0x4  )DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA       =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x8  )DISP_REG_GET(DISP_REG_CONFIG_REG_UPD_TIMEOUT    =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0xC  )DISP_REG_GET(DISP_REG_CONFIG_REG_COMMIT         =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x20)DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_EN           =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x24)DISP_REG_GET(DISP_REG_CONFIG_MUTEX0              =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x28)DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_RST          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x2C)DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_MOD          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x30)DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_SOF          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x40)DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_EN           =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x44)DISP_REG_GET(DISP_REG_CONFIG_MUTEX1              =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x48)DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_RST          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x4C)DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_MOD          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x50)DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_SOF          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x60)DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_EN           =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x64)DISP_REG_GET(DISP_REG_CONFIG_MUTEX2              =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x68)DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_RST          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x6C)DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_MOD          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x70)DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_SOF          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x80)DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_EN           =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x84)DISP_REG_GET(DISP_REG_CONFIG_MUTEX3              =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x88)DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_RST          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x8C)DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_MOD          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x90)DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_SOF          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0xA0)DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_EN           =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0xA4)DISP_REG_GET(DISP_REG_CONFIG_MUTEX4              =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0xA8)DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_RST          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0xAC)DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_MOD          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0xB0)DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_SOF          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0xC0)DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_EN           =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0xC4)DISP_REG_GET(DISP_REG_CONFIG_MUTEX5              =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0xC8)DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_RST          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0xCC)DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_MOD          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0xD0)DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_SOF          =0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("(0x100)DISP_REG_GET(DISP_REG_CONFIG_MUTEX_DEBUG_OUT_SEL=0x%x\n",
			 g_reg_mtx[i++]);
		DISP_MSG("TOTAL dump %d registers\n", i);
		break;

	default:
		DISP_MSG("error, reg_dump, unknown module=%d\n", module);
		break;
	}
}

void disp_dump_all_reg_info(void)
{
	disp_dump_reg(DISP_MODULE_CONFIG);
	msleep(50);
	disp_dump_reg(DISP_MODULE_MUTEX);
	msleep(50);
	disp_dump_reg(DISP_MODULE_OVL);
	msleep(50);
	disp_dump_reg(DISP_MODULE_WDMA);
	msleep(50);
	disp_dump_reg(DISP_MODULE_RDMA0);
	msleep(50);
	disp_dump_reg(DISP_MODULE_RDMA1);
	msleep(50);
	disp_dump_reg(DISP_MODULE_COLOR);
	msleep(50);
	disp_dump_reg(DISP_MODULE_BLS);
	msleep(50);

	/*DPI_DumpRegisters();*/
	DSI_DumpRegisters();

	/*for debug */
	/*BLS pattern */
	disp_path_get_mutex();
	DISP_REG_SET(DISP_REG_BLS_PATTERN, 0x41);
	/*dpi0 pattern */
	DISP_REG_SET(DISPSYS_DPI0_BASE + 0xF00, 0x41);
	/*dpi1 pattern */
	DISP_REG_SET(DISPSYS_DPI1_BASE + 0xF00, 0x41);
	disp_path_release_mutex();
}

int disp_module_clock_on(DISP_MODULE_ENUM module, char *caller_name)
{
	return 0;
}

int disp_module_clock_off(DISP_MODULE_ENUM module, char *caller_name)
{
	return 0;
}

#define DISP_REG_CLK_CFG0 0xf0000040
#define DISP_REG_CLK_CFG1 0xf0000050
int disp_clock_check(void)
{
	int ret = 0;

	/* 0:SMI COMMON, 1:SMI LARB0, 3:MUTEX, 4:DISP_COLOR */
	/* 5:DISP_BLS, 7:DISP_RDMA, 8:DISP_OVL */
	if (DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0) & 0x1bb) {
		DISP_ERR("clock error(ddp), CONFIG_CG_CON0=0x%x\n",
			 DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
		ret = -1;
	}
	/* Just for DSI, 0:DSI engine, 1:DSI digital */
	if (DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1) & 0x3) {
		DISP_ERR("clock error(dsi), CONFIG_CG_CON1=0x%x\n",
			 DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1));
		ret = -1;
	}
	/* 31: mm, 7:axi */
	if (DISP_REG_GET(DISP_REG_CLK_CFG0) & 0x80000080) {
		DISP_ERR("clock error(mm and axi), DISP_REG_CLK_CFG0=0x%x\n",
			 DISP_REG_GET(DISP_REG_CLK_CFG0));
		ret = -1;
	}
	/* for bls, 7:pwm */
	if (DISP_REG_GET(DISP_REG_CLK_CFG1) & 0x80) {
		DISP_ERR("clock error(pwm), DISP_REG_CLK_CFG1=0x%x\n",
			 DISP_REG_GET(DISP_REG_CLK_CFG1));
		ret = -1;
	}
	/* for mm clock freq (optional) */
	{

	}

	return ret;
}

long disp_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	DISP_WRITE_REG wParams;
	DISP_READ_REG rParams;
	DISP_READ_REG_TABLE rTableParams;
	unsigned int ret = 0;
	unsigned int value;
	DISP_MODULE_ENUM module;
	DISP_OVL_INFO ovl_info;
	DISP_PQ_PARAM *pq_param;
	DISP_PQ_PARAM *pq_cam_param;
	DISP_PQ_PARAM *pq_gal_param;
	DISPLAY_PQ_T *pq_index;
	DISPLAY_TDSHP_T *tdshp_index;
	DISPLAY_GAMMA_T *gamma_index;
	int layer, mutex_id;
	disp_wait_irq_struct wait_irq_struct;
	unsigned long lcmindex = 0;
	int count;

#if defined(CONFIG_MTK_AAL_SUPPORT)
	DISP_AAL_PARAM *aal_param;
#endif

#ifdef DDP_DBG_DDP_PATH_CONFIG
	struct disp_path_config_struct config;
#endif

	disp_node_struct *pNode = (disp_node_struct *) file->private_data;

	switch (cmd) {
	case DISP_IOCTL_WRITE_REG:

		if (copy_from_user(&wParams, (void *)arg, sizeof(DISP_WRITE_REG))) {
			DISP_ERR("DISP_IOCTL_WRITE_REG, copy_from_user failed\n");
			return -EFAULT;
		}

		DISP_DDP("write  0x%x = 0x%x (0x%x)\n", wParams.reg, wParams.val, wParams.mask);
		wParams.reg = ddp_addr_convert_pa2va(wParams.reg & 0x1FFFFFFF);
		if (wParams.reg > DISPSYS_REG_ADDR_MAX || wParams.reg < DISPSYS_REG_ADDR_MIN) {
			DISP_ERR("reg write, addr invalid, addr min=0x%x, max=0x%x, addr=0x%x\n",
				 DISPSYS_REG_ADDR_MIN, DISPSYS_REG_ADDR_MAX, wParams.reg);
			return -EFAULT;
		}

		*(volatile unsigned int *)wParams.reg =
		    (*(volatile unsigned int *)wParams.reg & ~wParams.
		     mask) | (wParams.val & wParams.mask);
		/* mt_reg_sync_writel(wParams.reg, value); */
		break;

	case DISP_IOCTL_READ_REG:
		if (copy_from_user(&rParams, (void *)arg, sizeof(DISP_READ_REG))) {
			DISP_ERR("DISP_IOCTL_READ_REG, copy_from_user failed\n");
			return -EFAULT;
		}
		rParams.reg = ddp_addr_convert_pa2va(rParams.reg & 0x1FFFFFFF);
		if (rParams.reg > DISPSYS_REG_ADDR_MAX || rParams.reg < DISPSYS_REG_ADDR_MIN) {
			DISP_ERR("reg read, addr invalid, addr min=0x%x, max=0x%x, addr=0x%x\n",
				 DISPSYS_REG_ADDR_MIN, DISPSYS_REG_ADDR_MAX, rParams.reg);
			return -EFAULT;
		}

		rParams.val = (*(volatile unsigned int *)rParams.reg) & rParams.mask;

		DISP_DDP("read 0x%x = 0x%x (0x%x)\n", rParams.reg, rParams.val, rParams.mask);

		if (copy_to_user((void *)arg, &rParams, sizeof(DISP_READ_REG))) {
			DISP_ERR("DISP_IOCTL_READ_REG, copy_to_user failed\n");
			return -EFAULT;
		}
		break;

	case DISP_IOCTL_READ_REG_TABLE:
		if (copy_from_user(&rTableParams, (void *)arg, sizeof(DISP_READ_REG_TABLE))) {
			DISP_ERR("DISP_IOCTL_READ_REG_TABLE, copy_from_user failed\n");
			return -EFAULT;
		}

		for (count = 0; count < rTableParams.count; count++) {
			if (rTableParams.reg[count] > DISPSYS_REG_ADDR_MAX
			    || rTableParams.reg[count] < DISPSYS_REG_ADDR_MIN) {
				DISP_ERR
				    ("reg read, addr invalid, addr min=0x%x, max=0x%x, addr=0x%x\n",
				     DISPSYS_REG_ADDR_MIN, DISPSYS_REG_ADDR_MAX,
				     rTableParams.reg[count]);
				continue;
			}

			rTableParams.val[count] =
			    (*(volatile unsigned int *)rTableParams.
			     reg[count]) & rTableParams.mask[count];
		}

		break;

	case DISP_IOCTL_WAIT_IRQ:
		if (copy_from_user(&wait_irq_struct, (void *)arg, sizeof(wait_irq_struct))) {
			DISP_ERR("DISP_IOCTL_WAIT_IRQ, copy_from_user failed\n");
			return -EFAULT;
		}
		ret = disp_wait_intr(wait_irq_struct.module, wait_irq_struct.timeout_ms);
		break;

	case DISP_IOCTL_DUMP_REG:
		if (copy_from_user(&module, (void *)arg, sizeof(module))) {
			DISP_ERR("DISP_IOCTL_DUMP_REG, copy_from_user failed\n");
			return -EFAULT;
		}
		ret = disp_dump_reg(module);
		ddp_dump_info(module);
		break;

	case DISP_IOCTL_LOCK_THREAD:
		DISP_MSG("DISP_IOCTL_LOCK_THREAD!\n");
		value = disp_lock_cmdq_thread();
		if (copy_to_user((void *)arg, &value, sizeof(unsigned int))) {
			DISP_ERR("DISP_IOCTL_LOCK_THREAD, copy_to_user failed\n");
			return -EFAULT;
		}
		break;

	case DISP_IOCTL_UNLOCK_THREAD:
		if (copy_from_user(&value, (void *)arg, sizeof(value))) {
			DISP_ERR("DISP_IOCTL_UNLOCK_THREAD, copy_from_user failed\n");
			return -EFAULT;
		}
		ret = disp_unlock_cmdq_thread(value);
		break;

	case DISP_IOCTL_MARK_CMQ:
		if (copy_from_user(&value, (void *)arg, sizeof(value))) {
			DISP_ERR("DISP_IOCTL_MARK_CMQ, copy_from_user failed\n");
			return -EFAULT;
		}
		if (value >= CMDQ_THREAD_NUM)
			return -EFAULT;
/* cmq_status[value] = 1; */
		break;

	case DISP_IOCTL_WAIT_CMQ:
		if (copy_from_user(&value, (void *)arg, sizeof(value))) {
			DISP_ERR("DISP_IOCTL_WAIT_CMQ, copy_from_user failed\n");
			return -EFAULT;
		}
		if (value >= CMDQ_THREAD_NUM)
			return -EFAULT;
		/*
		   wait_event_interruptible_timeout(cmq_wait_queue[value], cmq_status[value], 3 * HZ);
		   if(cmq_status[value] != 0)
		   {
		   cmq_status[value] = 0;
		   return -EFAULT;
		   }
		 */
		break;

	case DISP_IOCTL_LOCK_RESOURCE:
		if (copy_from_user(&mutex_id, (void *)arg, sizeof(int))) {
			DISP_ERR("DISP_IOCTL_LOCK_RESOURCE, copy_from_user failed\n");
			return -EFAULT;
		}
		if ((-1) != mutex_id) {
			int ret = wait_event_interruptible_timeout(gResourceWaitQueue,
								   (gLockedResource &
								    (1 << mutex_id)) == 0,
								   disp_ms2jiffies(50));

			if (ret <= 0) {
				DISP_ERR("DISP_IOCTL_LOCK_RESOURCE, mutex_id 0x%x failed\n",
					 gLockedResource);
				return -EFAULT;
			}

			spin_lock(&gResourceLock);
			gLockedResource |= (1 << mutex_id);
			spin_unlock(&gResourceLock);

			spin_lock(&pNode->node_lock);
			pNode->u4LockedResource = gLockedResource;
			spin_unlock(&pNode->node_lock);
		} else {
			DISP_ERR("DISP_IOCTL_LOCK_RESOURCE, mutex_id = -1 failed\n");
			return -EFAULT;
		}
		break;


	case DISP_IOCTL_UNLOCK_RESOURCE:
		if (copy_from_user(&mutex_id, (void *)arg, sizeof(int))) {
			DISP_ERR("DISP_IOCTL_UNLOCK_RESOURCE, copy_from_user failed\n");
			return -EFAULT;
		}
		if ((-1) != mutex_id) {
			spin_lock(&gResourceLock);
			gLockedResource &= ~(1 << mutex_id);
			spin_unlock(&gResourceLock);

			spin_lock(&pNode->node_lock);
			pNode->u4LockedResource = gLockedResource;
			spin_unlock(&pNode->node_lock);

			wake_up_interruptible(&gResourceWaitQueue);
		} else {
			DISP_ERR("DISP_IOCTL_UNLOCK_RESOURCE, mutex_id = -1 failed\n");
			return -EFAULT;
		}
		break;

	case DISP_IOCTL_SYNC_REG:
		mb();
		break;

	case DISP_IOCTL_SET_INTR:
		DISP_DBG("DISP_IOCTL_SET_INTR!\n");
		if (copy_from_user(&value, (void *)arg, sizeof(int))) {
			DISP_ERR("DISP_IOCTL_SET_INTR, copy_from_user failed\n");
			return -EFAULT;
		}
		/* enable intr */
		if ((value & 0xffff0000) != 0) {
			disable_irq(value & 0xff);
			DISP_MSG("disable_irq %d\n", value & 0xff);
		} else {
			DISP_REGISTER_IRQ(value & 0xff);
			DISP_MSG("enable irq: %d\n", value & 0xff);
		}
		break;

	case DISP_IOCTL_RUN_DPF:
		DISP_DBG("DISP_IOCTL_RUN_DPF!\n");
		if (copy_from_user(&value, (void *)arg, sizeof(int))) {
			DISP_ERR("DISP_IOCTL_SET_INTR, copy_from_user failed, %d\n", ret);
			return -EFAULT;
		}

		spin_lock(&gOvlLock);

		disp_run_dp_framework = value;

		spin_unlock(&gOvlLock);

		if (value == 1) {
			while (disp_get_mutex_status() != 0) {
				DISP_ERR("disp driver : wait fb release hw mutex\n");
				mdelay(3);
			}
		}
		break;

	case DISP_IOCTL_CHECK_OVL:
		DISP_DBG("DISP_IOCTL_CHECK_OVL!\n");
		value = disp_layer_enable;

		if (copy_to_user((void *)arg, &value, sizeof(int))) {
			DISP_ERR("disp driver : Copy to user error (result)\n");
			return -EFAULT;
		}
		break;

	case DISP_IOCTL_GET_OVL:
		DISP_DBG("DISP_IOCTL_GET_OVL!\n");
		if (copy_from_user(&ovl_info, (void *)arg, sizeof(DISP_OVL_INFO))) {
			DISP_ERR("DISP_IOCTL_SET_INTR, copy_from_user failed, %d\n", ret);
			return -EFAULT;
		}

		layer = ovl_info.layer;

		spin_lock(&gOvlLock);
		ovl_info = disp_layer_info[layer];
		spin_unlock(&gOvlLock);

		if (copy_to_user((void *)arg, &ovl_info, sizeof(DISP_OVL_INFO))) {
			DISP_ERR("disp driver : Copy to user error (result)\n");
			return -EFAULT;
		}

		break;

	case DISP_IOCTL_AAL_EVENTCTL:
#if !defined(CONFIG_MTK_AAL_SUPPORT)
		DISP_MSG
		    ("Invalid operation DISP_IOCTL_AAL_EVENTCTL since AAL is not turned on, in %s\n",
		     __func__);
		return -EFAULT;
#else
		if (copy_from_user(&value, (void *)arg, sizeof(int))) {
			DISP_MSG("disp driver : DISP_IOCTL_AAL_EVENTCTL Copy from user failed\n");
			return -EFAULT;
		}
		disp_set_aal_alarm(value);
		disp_set_needupdate(DISP_MODULE_BLS, 1);
		ret = 0;
#endif
		break;

	case DISP_IOCTL_GET_AALSTATISTICS:
#if !defined(CONFIG_MTK_AAL_SUPPORT)
		DISP_MSG
		    ("Invalid operation DISP_IOCTL_GET_AALSTATISTICS since AAL is not turned on, in %s\n",
		     __func__);
		return -EFAULT;
#else
		/* 1. Wait till new interrupt comes */
		if (disp_wait_hist_update(60)) {
			DISP_MSG("disp driver : DISP_IOCTL_GET_AALSTATISTICS wait time out\n");
			return -EFAULT;
		}
		/* 2. read out color engine histogram */
		disp_set_hist_readlock(1);
		if (copy_to_user
		    ((void *)arg, (void *)(disp_get_hist_ptr()), sizeof(DISP_AAL_STATISTICS))) {
			DISP_MSG
			    ("disp driver : DISP_IOCTL_GET_AALSTATISTICS Copy to user failed\n");
			return -EFAULT;
		}
		disp_set_hist_readlock(0);
		ret = 0;
#endif
		break;

	case DISP_IOCTL_SET_AALPARAM:
#if !defined(CONFIG_MTK_AAL_SUPPORT)
		DISP_MSG
		    ("Invalid operation : DISP_IOCTL_SET_AALPARAM since AAL is not turned on, in %s\n",
		     __func__);
		return -EFAULT;
#else
/* disp_set_needupdate(DISP_MODULE_BLS , 0); */

		disp_aal_lock();

		aal_param = get_aal_config();

		if (copy_from_user(aal_param, (void *)arg, sizeof(DISP_AAL_PARAM))) {
			DISP_MSG("disp driver : DISP_IOCTL_SET_AALPARAM Copy from user failed\n");
			return -EFAULT;
		}

		disp_aal_unlock();
#endif
		break;

	case DISP_IOCTL_SET_PQPARAM:

		ret = disp_color_set_pq_param((void *)arg);

		break;

	case DISP_IOCTL_SET_PQINDEX:

		pq_index = get_Color_index();
		if (copy_from_user(pq_index, (void *)arg, sizeof(DISPLAY_PQ_T))) {
			DISP_MSG("disp driver : DISP_IOCTL_SET_PQINDEX Copy from user failed\n");
			return -EFAULT;
		}

		break;

	case DISP_IOCTL_GET_PQPARAM:
		/* this is duplicated to cmdq_proc_unlocked_ioctl */
		/* be careful when modify the definition */
		pq_param = get_Color_config();
		if (copy_to_user((void *)arg, pq_param, sizeof(DISP_PQ_PARAM))) {
			DISP_MSG("disp driver : DISP_IOCTL_GET_PQPARAM Copy to user failed\n");
			return -EFAULT;
		}

		break;

	case DISP_IOCTL_SET_TDSHPINDEX:
		/* this is duplicated to cmdq_proc_unlocked_ioctl */
		/* be careful when modify the definition */
		tdshp_index = get_TDSHP_index();
		if (copy_from_user(tdshp_index, (void *)arg, sizeof(DISPLAY_TDSHP_T))) {
			DISP_MSG("disp driver : DISP_IOCTL_SET_TDSHPINDEX Copy from user failed\n");
			return -EFAULT;
		}

		break;

	case DISP_IOCTL_GET_TDSHPINDEX:

		tdshp_index = get_TDSHP_index();
		if (copy_to_user((void *)arg, tdshp_index, sizeof(DISPLAY_TDSHP_T))) {
			DISP_MSG("disp driver : DISP_IOCTL_GET_TDSHPINDEX Copy to user failed\n");
			return -EFAULT;
		}

		break;

	case DISP_IOCTL_SET_GAMMALUT:

		DISP_MSG("DISP_IOCTL_SET_GAMMALUT\n");

		gamma_index = get_gamma_index();
		if (copy_from_user(gamma_index, (void *)arg, sizeof(DISPLAY_GAMMA_T))) {
			DISP_MSG("disp driver : DISP_IOCTL_SET_GAMMALUT Copy from user failed\n");
			return -EFAULT;
		}
		/* disable BLS and suspend AAL */
		GetUpdateMutex();
		bls_gamma_dirty = 1;
		aal_debug_flag = 1;
		ReleaseUpdateMutex();

		disp_set_needupdate(DISP_MODULE_COLOR, 1);

		count = 0;
		while (DISP_REG_GET(DISP_REG_BLS_EN) & 0x1) {
			udelay(1000);
			count++;
			if (count > 1000) {
				DISP_ERR("fail to disable BLS (0x%x)\n",
					 DISP_REG_GET(DISP_REG_BLS_EN));
				break;
			}
		}

		/* update gamma lut */
		/* enable BLS and resume AAL */
		GetUpdateMutex();
		disp_bls_update_gamma_lut();
		bls_gamma_dirty = 0;
		aal_debug_flag = 0;
		ReleaseUpdateMutex();

		disp_set_needupdate(DISP_MODULE_COLOR, 1);

		break;

	case DISP_IOCTL_SET_CLKON:
		if (copy_from_user(&module, (void *)arg, sizeof(DISP_MODULE_ENUM))) {
			DISP_MSG("disp driver : DISP_IOCTL_SET_CLKON Copy from user failed\n");
			return -EFAULT;
		}

		disp_power_on(module, &(pNode->u4Clock));
		break;

	case DISP_IOCTL_SET_CLKOFF:
		if (copy_from_user(&module, (void *)arg, sizeof(DISP_MODULE_ENUM))) {
			DISP_MSG("disp driver : DISP_IOCTL_SET_CLKOFF Copy from user failed\n");
			return -EFAULT;
		}

		disp_power_off(module, &(pNode->u4Clock));
		break;

	case DISP_IOCTL_MUTEX_CONTROL:
		if (copy_from_user(&value, (void *)arg, sizeof(int))) {
			DISP_MSG("disp driver : DISP_IOCTL_MUTEX_CONTROL Copy from user failed\n");
			return -EFAULT;
		}

		DISP_MSG("DISP_IOCTL_MUTEX_CONTROL: %d, BLS_EN = %d\n", value,
			 DISP_REG_GET(DISP_REG_BLS_EN));

		if (value == 1) {

			/* disable BLS and suspend AAL */
			GetUpdateMutex();
			bls_gamma_dirty = 1;
			aal_debug_flag = 1;
			ReleaseUpdateMutex();

			disp_set_needupdate(DISP_MODULE_COLOR, 1);

			count = 0;
			while (DISP_REG_GET(DISP_REG_BLS_EN) & 0x1) {
				udelay(1000);
				count++;
				if (count > 1000) {
					DISP_ERR("fail to disable BLS (0x%x)\n",
						 DISP_REG_GET(DISP_REG_BLS_EN));
					break;
				}
			}

			ncs_tuning_mode = 1;
			GetUpdateMutex();
		} else if (value == 2) {
			/* enable BLS and resume AAL */
			bls_gamma_dirty = 0;
			aal_debug_flag = 0;
			ReleaseUpdateMutex();

			disp_set_needupdate(DISP_MODULE_COLOR, 1);
		} else {
			DISP_MSG("disp driver : DISP_IOCTL_MUTEX_CONTROL invalid control\n");
			return -EFAULT;
		}

		DISP_MSG("DISP_IOCTL_MUTEX_CONTROL done: %d, BLS_EN = %d\n", value,
			 DISP_REG_GET(DISP_REG_BLS_EN));

		break;

	case DISP_IOCTL_GET_LCMINDEX:

		lcmindex = DISP_GetLCMIndex();
		if (copy_to_user((void *)arg, &lcmindex, sizeof(unsigned long))) {
			DISP_MSG("disp driver : DISP_IOCTL_GET_LCMINDEX Copy to user failed\n");
			return -EFAULT;
		}

		break;

		break;

	case DISP_IOCTL_SET_PQ_CAM_PARAM:

		pq_cam_param = get_Color_Cam_config();
		if (copy_from_user(pq_cam_param, (void *)arg, sizeof(DISP_PQ_PARAM))) {
			DISP_MSG
			    ("disp driver : DISP_IOCTL_SET_PQ_CAM_PARAM Copy from user failed\n");
			return -EFAULT;
		}

		break;

	case DISP_IOCTL_GET_PQ_CAM_PARAM:

		pq_cam_param = get_Color_Cam_config();
		if (copy_to_user((void *)arg, pq_cam_param, sizeof(DISP_PQ_PARAM))) {
			DISP_MSG("disp driver : DISP_IOCTL_GET_PQ_CAM_PARAM Copy to user failed\n");
			return -EFAULT;
		}

		break;

	case DISP_IOCTL_SET_PQ_GAL_PARAM:

		pq_gal_param = get_Color_Gal_config();
		if (copy_from_user(pq_gal_param, (void *)arg, sizeof(DISP_PQ_PARAM))) {
			DISP_MSG
			    ("disp driver : DISP_IOCTL_SET_PQ_GAL_PARAM Copy from user failed\n");
			return -EFAULT;
		}

		break;

	case DISP_IOCTL_GET_PQ_GAL_PARAM:

		pq_gal_param = get_Color_Gal_config();
		if (copy_to_user((void *)arg, pq_gal_param, sizeof(DISP_PQ_PARAM))) {
			DISP_MSG("disp driver : DISP_IOCTL_GET_PQ_GAL_PARAM Copy to user failed\n");
			return -EFAULT;
		}

		break;

	case DISP_IOCTL_TEST_PATH:
#ifdef DDP_DBG_DDP_PATH_CONFIG
		if (copy_from_user(&value, (void *)arg, sizeof(value))) {
			DISP_ERR("DISP_IOCTL_MARK_CMQ, copy_from_user failed\n");
			return -EFAULT;
		}

		config.layer = 0;
		config.layer_en = 1;
		config.source = OVL_LAYER_SOURCE_MEM;
		config.addr = virt_to_phys(inAddr);
		config.inFormat = OVL_INPUT_FORMAT_RGB565;
		config.pitch = 480;
		config.srcROI.x = 0;	/* ROI */
		config.srcROI.y = 0;
		config.srcROI.width = 480;
		config.srcROI.height = 800;
		config.bgROI.x = config.srcROI.x;
		config.bgROI.y = config.srcROI.y;
		config.bgROI.width = config.srcROI.width;
		config.bgROI.height = config.srcROI.height;
		config.bgColor = 0xff;	/* background color */
		config.key = 0xff;	/* color key */
		config.aen = 0;	/* alpha enable */
		config.alpha = 0;
		DISP_MSG("value=%d\n", value);
		if (value == 0) {	/* mem->ovl->rdma0->dpi0 */
			config.srcModule = DISP_MODULE_OVL;
			config.outFormat = RDMA_OUTPUT_FORMAT_ARGB;
			config.dstModule = DISP_MODULE_DPI0;
			config.dstAddr = 0;
		} else if (value == 1) {	/* mem->ovl-> wdma1->mem */
			config.srcModule = DISP_MODULE_OVL;
			config.outFormat = WDMA_OUTPUT_FORMAT_RGB888;
			config.dstModule = DISP_MODULE_WDMA;
			config.dstAddr = virt_to_phys(outAddr);
		} else if (value == 2) {	/* mem->rdma0 -> dpi0 */
			config.srcModule = DISP_MODULE_RDMA0;
			config.outFormat = RDMA_OUTPUT_FORMAT_ARGB;
			config.dstModule = DISP_MODULE_DPI0;
			config.dstAddr = 0;
		}
		disp_path_config(&config);
		disp_path_enable();
#endif
		break;
	case DISP_SECURE_MVA_MAP:
#ifdef CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT
		{
			struct disp_mva_map mva_map_struct;

			if (copy_from_user
			    (&mva_map_struct, (void *)arg, sizeof(struct disp_mva_map))) {
				DISP_ERR("DISP_SECURE_MVA_MAP, copy_from_user failed\n");
				return -EFAULT;
			}
			DISP_DBG("map_mva, module=%d, cache=%d, addr=0x%x, size=0x%x\n",
				 mva_map_struct.module,
				 mva_map_struct.cache_coherent,
				 mva_map_struct.addr, mva_map_struct.size);

			if (mva_map_struct.addr == 0) {
				DISP_ERR("invalid parameter for DISP_SECURE_MVA_MAP!\n");
			} else {
				MTEEC_PARAM param[4];
				unsigned int paramTypes;
				TZ_RESULT ret;

				param[0].value.a = mva_map_struct.module;
				param[1].value.a = mva_map_struct.cache_coherent;
				param[2].value.a = mva_map_struct.addr;
				param[3].value.a = mva_map_struct.size;
				paramTypes =
				    TZ_ParamTypes4(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT,
						   TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
				ret =
				    KREE_TeeServiceCall(ddp_session_handle(),
							TZCMD_DDP_SECURE_MVA_MAP, paramTypes,
							param);
				if (ret != TZ_RESULT_SUCCESS) {
					DISP_ERR
					    ("KREE_TeeServiceCall(TZCMD_DDP_SECURE_MVA_MAP) fail, ret=%d\n",
					     ret);
				}
			}
		}
#endif
		break;
	case DISP_SECURE_MVA_UNMAP:
#ifdef CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT
		{
			struct disp_mva_map mva_map_struct;

			if (copy_from_user
			    (&mva_map_struct, (void *)arg, sizeof(struct disp_mva_map))) {
				DISP_ERR("DISP_SECURE_MVA_UNMAP, copy_from_user failed\n");
				return -EFAULT;
			}
			DISP_DBG("unmap_mva, module=%d, cache=%d, addr=0x%x, size=0x%x\n",
				 mva_map_struct.module,
				 mva_map_struct.cache_coherent,
				 mva_map_struct.addr, mva_map_struct.size);

			if (mva_map_struct.addr == 0) {
				DISP_ERR("invalid parameter for DISP_SECURE_MVA_UNMAP!\n");
			} else {
				MTEEC_PARAM param[4];
				unsigned int paramTypes;
				TZ_RESULT ret;

				param[0].value.a = mva_map_struct.module;
				param[1].value.a = mva_map_struct.cache_coherent;
				param[2].value.a = mva_map_struct.addr;
				param[3].value.a = mva_map_struct.size;
				paramTypes =
				    TZ_ParamTypes4(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT,
						   TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
				ret =
				    KREE_TeeServiceCall(ddp_session_handle(),
							TZCMD_DDP_SECURE_MVA_UNMAP, paramTypes,
							param);
				if (ret != TZ_RESULT_SUCCESS) {
					DISP_ERR
					    ("KREE_TeeServiceCall(TZCMD_DDP_SECURE_MVA_UNMAP) fail, ret=%d\n",
					     ret);
				}
			}
		}
#endif
		break;

	case DISP_SECURE_SET_MODE_BITBLT:
#ifdef CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT
		{
			unsigned int secure;

			if (copy_from_user(&secure, (void *)arg, sizeof(unsigned int))) {
				pr_info
				    ("disp driver : DISP_SECURE_SET_MODE_BITBLT Copy from user failed\n");
				return -EFAULT;
			}

			if (secure != gBitbltSecure) {
				if (disp_secure_irq_task == NULL) {
					disp_secure_irq_task =
					    kthread_create(disp_secure_intr_callback, NULL,
							   "disp_secure_irq_task");
					if (IS_ERR(disp_secure_irq_task)) {
						DISP_ERR
						    ("DISP_InitVSYNC(): Cannot create disp_irq_log_task kthread\n");
					}
					wake_up_process(disp_secure_irq_task);
				}

				disp_register_intr(disp_dev.irq[DISP_REG_MDP_WROT], secure);
				disp_register_intr(disp_dev.irq[DISP_REG_MDP_WDMA], secure);
				gBitbltSecure = secure;
#if 0
				{
					MTEEC_PARAM param[4];
					unsigned int paramTypes;
					TZ_RESULT ret;

					param[0].value.a = M4U_PORT_ROT_EXT;
					param[1].value.a = gBitbltSecure;
					paramTypes =
					    TZ_ParamTypes2(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
					ret =
					    KREE_TeeServiceCall(ddp_session_handle(),
								TZCMD_DDP_SET_SECURE_MODE,
								paramTypes, param);
					if (ret != TZ_RESULT_SUCCESS) {
						DISP_ERR
						    ("KREE_TeeServiceCall(TZCMD_DDP_SET_SECURE_MODE) fail, ret=%d\n",
						     ret);
					}

					param[0].value.a = M4U_PORT_WDMA0;
					param[1].value.a = gBitbltSecure;
					paramTypes =
					    TZ_ParamTypes2(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
					ret =
					    KREE_TeeServiceCall(ddp_session_handle(),
								TZCMD_DDP_SET_SECURE_MODE,
								paramTypes, param);
					if (ret != TZ_RESULT_SUCCESS) {
						DISP_ERR
						    ("KREE_TeeServiceCall(TZCMD_DDP_SET_SECURE_MODE) fail, ret=%d\n",
						     ret);
					}
				}
#endif
			}
		}
#endif
		break;

	case DISP_SECURE_SET_MODE_OVL_MEM_OUT:
#ifdef CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT
		{
			unsigned int secure;

			if (copy_from_user(&secure, (void *)arg, sizeof(unsigned int))) {
				pr_info
				    ("disp driver : DISP_SECURE_SET_MODE_BITBLT Copy from user failed\n");
				return -EFAULT;
			}

			if (secure != gMemOutSecure) {
				if (disp_secure_irq_task == NULL) {
					disp_secure_irq_task =
					    kthread_create(disp_secure_intr_callback, NULL,
							   "disp_secure_irq_task");
					if (IS_ERR(disp_secure_irq_task)) {
						DISP_ERR
						    ("DISP_InitVSYNC(): Cannot create disp_irq_log_task kthread\n");
					}
					wake_up_process(disp_secure_irq_task);
				}

				disp_register_intr(disp_dev.irq[DISP_REG_MDP_WDMA], secure);
				gMemOutSecure = secure;
			}
		}
#endif
		break;

	default:
		DISP_ERR("Ddp drv dose not have such command : 0x%x\n", cmd);
		break;
	}

	return ret;
}

static int disp_open(struct inode *inode, struct file *file)
{
	disp_node_struct *pNode = NULL;

	DISP_DBG("enter disp_open() process:%s\n", current->comm);

	/* Allocate and initialize private data */
	file->private_data = kmalloc(sizeof(disp_node_struct), GFP_ATOMIC);
	if (NULL == file->private_data) {
		DISP_MSG("Not enough entry for DDP open operation\n");
		return -ENOMEM;
	}

	pNode = (disp_node_struct *) file->private_data;
	pNode->open_pid = current->pid;
	pNode->open_tgid = current->tgid;
	INIT_LIST_HEAD(&(pNode->testList));
	pNode->u4LockedResource = 0;
	pNode->u4Clock = 0;
	spin_lock_init(&pNode->node_lock);

	return 0;

}

static ssize_t disp_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
	return 0;
}

static int disp_release(struct inode *inode, struct file *file)
{
	disp_node_struct *pNode = NULL;
	unsigned int index = 0;

	DISP_DBG("enter disp_release() process:%s\n", current->comm);

	pNode = (disp_node_struct *) file->private_data;

	spin_lock(&pNode->node_lock);

	if (pNode->u4LockedResource) {
		DISP_ERR("Proccess terminated[REsource] ! :%s , resource:%d\n", current->comm,
			 pNode->u4LockedResource);
		spin_lock(&gResourceLock);
		gLockedResource = 0;
		spin_unlock(&gResourceLock);
	}

	if (pNode->u4Clock) {
		DISP_ERR("Process safely terminated [Clock] !:%s , clock:%u\n", current->comm,
			 pNode->u4Clock);

		for (index = 0; index < DISP_MODULE_MAX; index += 1) {
			if ((1 << index) & pNode->u4Clock)
				disp_power_off((DISP_MODULE_ENUM) index, &pNode->u4Clock);
		}
	}

	spin_unlock(&pNode->node_lock);

	if (NULL != file->private_data) {
		kfree(file->private_data);
		file->private_data = NULL;
	}

	return 0;
}

static int disp_flush(struct file *file, fl_owner_t a_id)
{
	return 0;
}

/* remap register to user space */
static int disp_mmap(struct file *file, struct vm_area_struct *a_pstVMArea)
{

	a_pstVMArea->vm_page_prot = pgprot_noncached(a_pstVMArea->vm_page_prot);
	if (remap_pfn_range(a_pstVMArea,
			    a_pstVMArea->vm_start,
			    a_pstVMArea->vm_pgoff,
			    (a_pstVMArea->vm_end - a_pstVMArea->vm_start),
			    a_pstVMArea->vm_page_prot)) {
		DISP_MSG("MMAP failed!!\n");
		return -1;
	}


	return 0;
}


/* Kernel interface */
static const struct file_operations disp_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = disp_unlocked_ioctl,
	.open = disp_open,
	.release = disp_release,
	.flush = disp_flush,
	.read = disp_read,
	.mmap = disp_mmap
};

static int disp_probe(struct platform_device *pdev)
{
	int ret, i;
	static unsigned int disp_probe_cnt;
	struct class_device *class_dev = NULL;
	unsigned int id, reg_va, reg_pa, irq_map;
	unsigned int clk_num = 0;
	struct clk *clk[MAX_CLK_NUM_OF_ONE_MODULE];
#ifdef CONFIG_MTK_IOMMU
	struct device_node *np;
	struct dma_iommu_mapping *dma_mapping;
#endif

	if (disp_probe_cnt == 0)
		memset(&disp_dev, 0, sizeof(disp_dev));

	/* find device id */
	for (i = 0; i < DISP_REG_NUM; i++) {
		ret = of_device_is_compatible(pdev->dev.of_node, disp_reg_name_spy(i));
		if (ret)
			break;
	}
	if (i == DISP_REG_NUM) {
		DISP_ERR("dispsys compatible \"%s\" match failed\n", disp_reg_name_spy(i));
		return -ENODEV;
	}
	id = i;

	if (of_property_read_u32_index(pdev->dev.of_node, "reg", 1, &reg_pa)) {
		DISP_ERR("dispsys %s get reg_pa failed\n", disp_reg_name_spy(id));
		return -ENODEV;
	}

	reg_va = (unsigned int)of_iomap(pdev->dev.of_node, 0);
	if (!reg_va) {
		DISP_ERR("dispsys %s of_iomap failed\n", disp_reg_name_spy(id));
		return -ENODEV;
	}

	irq_map = irq_of_parse_and_map(pdev->dev.of_node, 0);

	/*For PM domain*/
#ifdef CONFIG_PM_RUNTIME
	if (id == DISP_REG_CONFIG)
		pm_runtime_enable(&pdev->dev);
#endif
	/*For CCF*/
	for (i = 0; i < MAX_CLK_NUM_OF_ONE_MODULE; i++) {
		clk[i] = devm_clk_get(&pdev->dev, disp_clk_name_spy(id, i));
		if (IS_ERR(clk[i])) {
			/* DISP_ERR("dispsys %s devm_clk_get failed\n", disp_clk_name_spy(id, i)); */
			break;
		}
		clk_num++;
	}

#ifdef CONFIG_MTK_IOMMU
	/*For IOMMU*/
	if (NULL == disp_dev.pimudev) {
		np = of_parse_phandle(pdev->dev.of_node, "iommus", 0);
		if (np) {
			disp_dev.pimudev = of_find_device_by_node(np);
			if (WARN_ON(!disp_dev.pimudev)) {
				of_node_put(np);
				return -EINVAL;
			}
			disp_dev.dev_use_imu = &pdev->dev;
			dma_mapping = disp_dev.pimudev->dev.archdata.iommu;
			arm_iommu_attach_device(disp_dev.dev_use_imu, dma_mapping);
			/*DISP_MSG("DT, i=%d, module=%s iommus %p\n",
				id, disp_reg_name_spy(id), disp_dev.dev);*/
		}
	}
	if (NULL == disp_dev.psmidev) {
		np = of_parse_phandle(pdev->dev.of_node, "larbs", 0);
		if (np) {
			disp_dev.psmidev = of_find_device_by_node(np);
			/* DISP_MSG("DT, i=%d, module=%s smi larbs\n",
				id, disp_reg_name_spy(id)); */
			/* mtk_smi_config_port(&disp_dev.psmidev->dev, M4U_PORT_DISP_OVL_0, true); */
		}
	}
#endif

	disp_dev.dev[id] = &pdev->dev;
	disp_dev.regs_pa[id] = reg_pa;
	disp_dev.regs_va[id] = reg_va;
	disp_dev.irq[id] = irq_map;
	for (i = 0; i < clk_num; i++)
		disp_dev.clk_map[id][i] = clk[i];

	DPI_InitRegbase();
#if defined(CONFIG_MTK_HDMI_SUPPORT)
	DPI1_InitRegbase();
#endif
	DSI_InitRegbase();

	if (id == DISP_REG_CMDQ) {		/* we have already get all MDP HW addr */
#ifdef MTK_ENABLE_CMDQ
		/* initial wait queue for cmdq */
		for (i = 0; i < CMDQ_THREAD_NUM; i++) {
			init_waitqueue_head(&cmq_wait_queue[i]);
			/* enable CMDQ interrupt */
			DISP_REG_SET(DISP_REG_CMDQ_THRx_IRQ_FLAG_EN(i), 0x13);	/* SL TEST CMDQ time out */
		}
		cmdqInitialize(disp_dev.dev[DISP_REG_CMDQ]);
#endif
	}


	DISP_MSG("DT, i=%d, module=%s, reg_pa=0x%x, map_addr=0x%x, map_irq=%d, clk_num=%d\n",
		 id, disp_reg_name_spy(id), disp_dev.regs_pa[id], disp_dev.regs_va[id],
		 disp_dev.irq[id], clk_num);

	if (disp_probe_cnt != 0)
		return 0;
	disp_probe_cnt = 1;

	DISP_MSG("disp driver probe...\n");
	ret = alloc_chrdev_region(&disp_devno, 0, 1, DISP_DEVNAME);
	if (ret)
		DISP_ERR("Error: Can't Get Major number for DISP Device\n");
	else
		DISP_MSG("Get DISP Device Major number (%d)\n", disp_devno);

	disp_cdev = cdev_alloc();
	disp_cdev->owner = THIS_MODULE;
	disp_cdev->ops = &disp_fops;
	ret = cdev_add(disp_cdev, disp_devno, 1);
	disp_class = class_create(THIS_MODULE, DISP_DEVNAME);
	class_dev =
	    (struct class_device *)device_create(disp_class, NULL, disp_devno, NULL, DISP_DEVNAME);

	spin_lock_init(&gCmdqLock);
	spin_lock_init(&gResourceLock);
	spin_lock_init(&gOvlLock);
	spin_lock_init(&gRegisterUpdateLock);
	spin_lock_init(&gPowerOperateLock);
	spin_lock_init(&g_disp_irq.irq_lock);

	pRegBackup = kmalloc_array(DDP_BACKUP_REG_NUM, sizeof(int), GFP_KERNEL);
	*pRegBackup = DDP_UNBACKED_REG_MEM;

#if 0
	/* initial wait queue for cmdq */
	for (i = 0; i < CMDQ_THREAD_NUM; i++) {
		init_waitqueue_head(&cmq_wait_queue[i]);
		/* enable CMDQ interrupt */
		DISP_REG_SET(DISP_REG_CMDQ_THRx_IRQ_FLAG_EN(i), 0x13);	/* SL TEST CMDQ time out */
	}

	cmdqInitialize();
#endif

	init_waitqueue_head(&disp_irq_log_wq);
	disp_irq_log_task =
	    kthread_create(disp_irq_log_kthread_func, NULL, "disp_config_update_kthread");
	if (IS_ERR(disp_irq_log_task))
		DISP_ERR("DISP_InitVSYNC(): Cannot create disp_irq_log_task kthread\n");
	wake_up_process(disp_irq_log_task);

#ifdef CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT
	/* TEE IRQ Callback deal */
	if (disp_secure_irq_task == NULL) {
		disp_secure_irq_task =
		    kthread_create(disp_secure_intr_callback, NULL, "disp_secure_irq_task");
		if (IS_ERR(disp_secure_irq_task))
			DISP_ERR("Cannot create disp_irq_log_task kthread\n");
		wake_up_process(disp_secure_irq_task);
	}
#endif

	/* init error log timer */
	init_timer(&disp_irq_err_timer);
	disp_irq_err_timer.expires = jiffies + 5 * HZ;
	disp_irq_err_timer.function = disp_irq_err_timer_handler;
	add_timer(&disp_irq_err_timer);

	DISP_MSG("DISP Probe Done\n");
	/*NOT_REFERENCED(class_dev);*/
	return 0;
}

static int disp_remove(struct platform_device *pdev)
{
#ifdef CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT
	/* .TEE. close SVP DDP TEE serivice session */
	/* the sessions are created and accessed using [ddp_session_handle()] / */
	/* [ddp_mem_session_handle()] API, and closed here. */
	{
		TZ_RESULT ret;

		ret = KREE_CloseSession(ddp_session);
		if (ret != TZ_RESULT_SUCCESS) {
			DISP_ERR("DDP close ddp_session fail ret=%d\n", ret);
			return -1;
		}

		ret = KREE_CloseSession(ddp_mem_session);
		if (ret != TZ_RESULT_SUCCESS) {
			DISP_ERR("DDP close ddp_mem_session fail ret=%d\n", ret);
			return -1;
		}
	}
#endif

#ifdef CONFIG_PM_RUNTIME
	if (&pdev->dev == disp_dev.dev[DISP_REG_CONFIG])
		pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}

static void disp_shutdown(struct platform_device *pdev)
{
	/* Nothing yet */
}


/* PM suspend */
static int disp_suspend(struct platform_device *pdev, pm_message_t mesg)
{
#ifdef MTK_ENABLE_CMDQ
	return cmdqSuspendTask();
#else
	return 0;
#endif
}

/* PM resume */
static int disp_resume(struct platform_device *pdev)
{
#ifdef MTK_ENABLE_CMDQ
	return cmdqResumeTask();
#else
	return 0;
#endif
}

static const struct of_device_id dispsys_of_ids[] = {
	{.compatible = "mediatek,mt8127-mmsys",},
	{.compatible = "mediatek,mt8127-disp-ovl",},
	{.compatible = "mediatek,mt8127-disp-rdma0",},
	{.compatible = "mediatek,mt8127-disp-wdma",},
	{.compatible = "mediatek,mt8127-disp-bls",},
	{.compatible = "mediatek,mt8127-disp-color",},
	{.compatible = "mediatek,mt8127-disp-dsi",},
	{.compatible = "mediatek,mt8127-disp-dpi0",},
	{.compatible = "mediatek,mt8127-mm-mutex",},
	{.compatible = "mediatek,mt8127-mm-cmdq",},
	{.compatible = "mediatek,mt8127-disp-rdma1",},
	{.compatible = "mediatek,mt8127-disp-ufoe",},
	{.compatible = "mediatek,mt8127-disp-dpi1",},
	{.compatible = "mediatek,mt8127-disp-hdmi",},
	{.compatible = "mediatek,mt8127-disp-lvds",},
	{.compatible = "mediatek,mt8127-disp-tve",},
	{.compatible = "mediatek,mt8127-disp-mipi",},
	{.compatible = "mediatek,mt8127-mdp-rdma",},
	{.compatible = "mediatek,mt8127-mdp-rsz0",},
	{.compatible = "mediatek,mt8127-mdp-rsz1",},
	{.compatible = "mediatek,mt8127-mdp-wdma",},
	{.compatible = "mediatek,mt8127-mdp-wrot",},
	{.compatible = "mediatek,mt8127-mdp-tdshp"},
	{}
};

MODULE_DEVICE_TABLE(of, dispsys_of_ids);

static struct platform_driver disp_driver = {
	.driver = {
		.name = DISP_DEVNAME,
		.owner = THIS_MODULE,
		.of_match_table = dispsys_of_ids,
	},
	.probe = disp_probe,
	.remove = disp_remove,
	.shutdown = disp_shutdown,
	.suspend = disp_suspend,
	.resume = disp_resume,
};

static int __init disp_init(void)
{
	int ret;

	DISP_MSG("Register the disp driver\n");
	if (platform_driver_register(&disp_driver)) {
		DISP_ERR("failed to register disp driver\n");
		ret = -ENODEV;
		return ret;
	}
	ddp_debug_init();

	return 0;
}

static void __exit disp_exit(void)
{
#ifdef MTK_ENABLE_CMDQ
	cmdqDeInitialize();
#endif

	cdev_del(disp_cdev);
	unregister_chrdev_region(disp_devno, 1);
	platform_driver_unregister(&disp_driver);
	device_destroy(disp_class, disp_devno);
	class_destroy(disp_class);

	ddp_debug_exit();
	DISP_MSG("disp_exit Done\n");
}

module_init(disp_init);
module_exit(disp_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("Display subsystem Driver");
MODULE_LICENSE("GPL");
