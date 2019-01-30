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

#ifndef __MTKFB_DEBUG_H
#define __MTKFB_DEBUG_H

#include "ddp_hal.h"
#include "lcm_drv.h"
#include "disp_drv_platform.h"
#include <mmprofile.h>

extern struct MTKFB_MMP_Events_t {
	MMP_Event MTKFB;
	MMP_Event CreateSyncTimeline;
	MMP_Event PanDisplay;
	MMP_Event SetOverlayLayer;
	MMP_Event SetOverlayLayers;
	MMP_Event TrigOverlayOut;
	MMP_Event SetVideoLayers;
	MMP_Event SetMultipleLayers;
	MMP_Event CreateSyncFence;
	MMP_Event IncSyncTimeline;
	MMP_Event SignalSyncFence;
	MMP_Event UpdateScreenImpl;
	MMP_Event VSync;
	MMP_Event UpdateConfig;
	MMP_Event ConfigOVL;
	MMP_Event ConfigAAL;
	MMP_Event ConfigMemOut;
	MMP_Event ScreenUpdate;
	MMP_Event CaptureFramebuffer;
	MMP_Event RegUpdate;
	MMP_Event OverlayOutDone;
	MMP_Event SwitchMode;
	MMP_Event BypassOVL;
	MMP_Event EarlySuspend;
	MMP_Event DispDone;
	MMP_Event DSICmd;
	MMP_Event DSIIRQ;
	MMP_Event EsdCheck;
	MMP_Event WaitVSync;
	MMP_Event LayerDump;
	MMP_Event Layer[4];
	MMP_Event OvlDump;
	MMP_Event FBDump;
	MMP_Event DSIRead;
	MMP_Event GetLayerInfo;
	MMP_Event LayerInfo[4];
	MMP_Event IOCtrl;
	MMP_Event Debug;

	MMP_Event QueueWork;
	MMP_Event WrokHandler;
	MMP_Event FenceContrl;
	MMP_Event HWCFence[4];
	MMP_Event FBFence;
	MMP_Event FBTimeline;

	MMP_Event OVLEngine;
	MMP_Event Instance_status[5];
	MMP_Event OVLEngine_ovlwdma_status;
	MMP_Event OVLEngine_rdma_status;
	MMP_Event OVLEngine_rdma_hang;
	MMP_Event OVLEngine_fence[3];
	MMP_Event OVLEngine_timeline[3];
	MMP_Event OVLEngine_API;
	MMP_Event OVLEngine_CORE;
	MMP_Event OVLEngine_HW;
	MMP_Event OVLEngine_IOCTL;

	MMP_Event GetInstance;
	MMP_Event ReleaseInstance;
	MMP_Event Get_layer_info;
	MMP_Event Set_layer_info;
	MMP_Event Get_Ovl_layer_info;
	MMP_Event Set_Overlayed_Buffer;
	MMP_Event Trigger_Overlay;
	MMP_Event Get_Dirty_info;
	MMP_Event Set_Path_Info;
	MMP_Event Sync_Captured_layer_info;
	MMP_Event Sync_Realtime_layer_info;
	MMP_Event Dump_layer_info;
	MMP_Event Trigger_Overlay_Handler;
	MMP_Event Trigger_Overlay_Fence;
	MMP_Event Get_Fence_Fd;

	MMP_Event Update_kthread;
	MMP_Event wake_up_ovl_engine_thread;
	MMP_Event interrupt_handler;

	MMP_Event hw_set_params;
	MMP_Event hw_ovl_wdma_irq_handler;
	MMP_Event hw_ovl_rdma_irq_handler;
	MMP_Event rdma0_irq_handler;
	MMP_Event trigger_hw_overlay;
	MMP_Event trigger_hw_overlay_decouple;
	MMP_Event trigger_hw_overlay_couple;
	MMP_Event config_overlay;
	MMP_Event direct_link_overlay;
	MMP_Event indirect_link_overlay;
	MMP_Event set_overlay_to_buffer;
	MMP_Event dumpallinfo;
	MMP_Event hw_mva_map;
	MMP_Event hw_mva_unmap;
	MMP_Event hw_reset;

	MMP_Event Svp;
	MMP_Event OVL_WDMA_IRQ_InTEE;
	MMP_Event OVL_WDMA_M4U_InTEE;
	MMP_Event OVL_WDMA_ADDR_InTEE;

	MMP_Event RDMA0_IRQ_InTEE;
	MMP_Event RDMA0_M4U_InTEE;
	MMP_Event RDMA0_ADDR_InTEE;

	MMP_Event RDMA1_IRQ_InTEE;
	MMP_Event RDMA1_M4U_InTEE;
	MMP_Event RDMA1_ADDR_InTEE;


} MTKFB_MMP_Events;

#ifndef STR_CONVERT
#define STR_CONVERT(p, val, base, action)\
	do {			\
		int ret = 0;	\
		const char *tmp;	\
		tmp = strsep(p, ","); \
		if (tmp == NULL) \
			break; \
		if (strcmp(#base, "int") == 0)\
			ret = kstrtoint(tmp, 0, (int *)val); \
		else if (strcmp(#base, "uint") == 0)\
			ret = kstrtouint(tmp, 0, (unsigned int *)val); \
		else if (strcmp(#base, "ul") == 0)\
			ret = kstrtoul(tmp, 0, (unsigned long *)val); \
		if (0 != ret) {\
			DISP_ERR("kstrtoint/kstrtouint/kstrtoul return error: %d\n" \
				"  file : %s, line : %d\n",		\
				ret, __FILE__, __LINE__);\
			action; \
		} \
	} while (0)
#endif


#ifdef MTKFB_DBG
#include "disp_drv_log.h"

#define DBG_BUF_SIZE		    2048
#define MAX_DBG_INDENT_LEVEL	5
#define DBG_INDENT_SIZE		    3
#define MAX_DBG_MESSAGES	    0

static int dbg_indent;
static int dbg_cnt;
static char dbg_buf[DBG_BUF_SIZE];
static spinlock_t dbg_spinlock = SPIN_LOCK_UNLOCKED;

static inline void dbg_print(int level, const char *fmt, ...)
{
	if (level <= MTKFB_DBG) {
		if (!MAX_DBG_MESSAGES || dbg_cnt < MAX_DBG_MESSAGES) {
			va_list args;
			int ind = dbg_indent;
			unsigned long flags;

			spin_lock_irqsave(&dbg_spinlock, flags);
			dbg_cnt++;
			if (ind > MAX_DBG_INDENT_LEVEL)
				ind = MAX_DBG_INDENT_LEVEL;

			DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "%*s", ind * DBG_INDENT_SIZE, "");
			va_start(args, fmt);
			vsnprintf(dbg_buf, sizeof(dbg_buf), fmt, args);
			DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", dbg_buf);
			va_end(args);
			spin_unlock_irqrestore(&dbg_spinlock, flags);
		}
	}
}

#define DBGPRINT	dbg_print

#define DBGENTER(level)	do { \
		dbg_print(level, "%s: Enter\n", __func__); \
		dbg_indent++; \
	} while (0)

#define DBGLEAVE(level)	do { \
		dbg_indent--; \
		dbg_print(level, "%s: Leave\n", __func__); \
	} while (0)

/* Debug Macros */

#define MTKFB_DBG_EVT_NONE    0x00000000
#define MTKFB_DBG_EVT_FUNC    0x00000001	/* Function Entry     */
#define MTKFB_DBG_EVT_ARGU    0x00000002	/* Function Arguments */
#define MTKFB_DBG_EVT_INFO    0x00000003	/* Information        */

#define MTKFB_DBG_EVT_MASK    (MTKFB_DBG_EVT_NONE)

#define MSG(evt, fmt, args...)                              \
	do {                                                    \
		if ((MTKFB_DBG_EVT_##evt) & MTKFB_DBG_EVT_MASK)     \
			DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", fmt, ##args);\
	} while (0)

#define MSG_FUNC_ENTER(f)   MSG(FUNC, "<FB_ENTER>: %s\n", __func__)
#define MSG_FUNC_LEAVE(f)   MSG(FUNC, "<FB_LEAVE>: %s\n", __func__)


#else				/* MTKFB_DBG */

#define DBGPRINT(level, format, ...)
#define DBGENTER(level)
#define DBGLEAVE(level)

/* Debug Macros */

#define MSG(evt, fmt, args...)
#define MSG_FUNC_ENTER() DISP_FB("Enter %s\n", __func__)
#define MSG_FUNC_LEAVE() DISP_FB("Leave %s\n", __func__)

#endif				/* MTKFB_DBG */

/* --------------------------------------------------------------------------- */
/* External variable declarations */
/* --------------------------------------------------------------------------- */
extern LCM_DRIVER *lcm_drv;
extern unsigned int EnableVSyncLog;
extern long tpd_last_down_time;
extern int tpd_start_profiling;
extern unsigned int gCaptureLayerEnable;
extern unsigned int gCaptureLayerDownX;
extern unsigned int gCaptureLayerDownY;
extern unsigned int gCaptureOvlThreadEnable;
extern unsigned int gCaptureOvlDownX;
extern unsigned int gCaptureOvlDownY;
extern struct task_struct *captureovl_task;
extern unsigned int gCaptureFBEnable;
extern unsigned int gCaptureFBDownX;
extern unsigned int gCaptureFBDownY;
extern unsigned int gCaptureFBPeriod;
extern struct task_struct *capturefb_task;
extern wait_queue_head_t gCaptureFBWQ;
extern OVL_CONFIG_STRUCT cached_layer_config[4];
extern LCM_DRIVER *lcm_drv;
extern unsigned int gDumpOVLEnable;

extern void disp_log_enable(int enable);
extern void mtkfb_vsync_log_enable(int enable);
extern void mtkfb_capture_fb_only(bool enable);
extern void esd_recovery_pause(bool en);
extern int mtkfb_set_backlight_mode(unsigned int mode);
extern void mtkfb_pan_disp_test(void);
extern void mtkfb_show_sem_cnt(void);
extern void mtkfb_hang_test(bool en);
extern void set_ovlengine_debug_level(int level);
extern int fb_pattern_en(int enable);
extern void mtkfb_clear_lcm(void);

void DBG_Init(void);
void DBG_Deinit(void);

void DBG_OnTriggerLcd(void);
void DBG_OnTeDelayDone(void);
void DBG_OnLcdDone(void);

#endif				/* __MTKFB_DEBUG_H */
