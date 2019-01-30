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

#include <linux/string.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/wait.h>
#include <linux/types.h>
#include "disp_hal.h"
#include "disp_drv_log.h"
#include "mtkfb.h"
#include "disp_debug.h"
#include "lcm_drv.h"
#include "ddp_ovl.h"
#include "ddp_path.h"
#include "disp_drv.h"


#define MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT

#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
struct dentry *mtkfb_layer_dbgfs[DDP_OVL_LAYER_MUN];
typedef struct {
	uint32_t layer_index;
	uint32_t working_buf;
	uint32_t working_size;
} MTKFB_LAYER_DBG_OPTIONS;
MTKFB_LAYER_DBG_OPTIONS mtkfb_layer_dbg_opt[DDP_OVL_LAYER_MUN];
#endif

typedef struct {
	unsigned int en_fps_log;
	unsigned int en_touch_latency_log;
	unsigned int log_fps_wnd_size;
	unsigned int force_dis_layers;
} DBG_OPTIONS;

static char debug_buffer[2048];
struct MTKFB_MMP_Events_t MTKFB_MMP_Events;
static const long int DEFAULT_LOG_FPS_WND_SIZE = 30;
static DBG_OPTIONS dbg_opt = { 0 };

static char STR_HELP[] =
	"\n"
	"USAGE\n"
	"        echo [ACTION]... > /d/mtkfb\n"
	"\n"
	"ACTION\n"
	"        mtkfblog:[on|off]\n"
	"             enable/disable [MTKFB] log\n"
	"\n"
	"        displog:[on|off]\n"
	"             enable/disable [DISP] log\n"
	"\n"
	"        mtkfb_vsynclog:[on|off]\n"
	"             enable/disable [VSYNC] log\n"
	"\n"
	"        log:[on|off]\n"
	"             enable/disable above all log\n"
	"\n"
	"        fps:[on|off]\n"
	"             enable fps and lcd update time log\n"
	"\n"
	"        tl:[on|off]\n"
	"             enable touch latency log\n"
	"\n"
	"        layer\n"
	"             dump lcd layer information\n"
	"\n"
	"        suspend\n"
	"             enter suspend mode\n"
	"\n"
	"        resume\n"
	"             leave suspend mode\n"
	"\n"
	"        lcm:[on|off|init]\n"
	"             power on/off lcm\n"
	"\n"
	"        cabc:[ui|mov|still]\n"
	"             cabc mode, UI/Moving picture/Still picture\n"
	"\n"
	"        lcd:[on|off]\n"
	"             power on/off display engine\n"
	"\n"
	"        te:[on|off]\n"
	"             turn on/off tearing-free control\n"
	"\n"
	"        tv:[on|off]\n"
	"             turn on/off tv-out\n"
	"\n"
	"        tvsys:[ntsc|pal]\n"
	"             switch tv system\n"
	"\n"
	"        reg:[lcd|dpi|dsi|tvc|tve]\n"
	"             dump hw register values\n"
	"\n"
	"        regw:addr=val\n"
	"             write hw register\n"
	"\n"
	"        regr:addr\n"
	"             read hw register\n"
	"\n"
	"       cpfbonly:[on|off]\n"
	"             capture UI layer only on/off\n"
	"\n"
	"       esd:[on|off]\n"
	"             esd kthread on/off\n"
	"       HQA:[NormalToFactory|FactoryToNormal]\n"
	"             for HQA requirement\n"
	"\n"
	"       mmp\n"
	"             Register MMProfile events\n"
	"\n"
	"       dump_fb:[on|off[,down_sample_x[,down_sample_y,[delay]]]]\n"
	"             Start/end to capture framebuffer every delay(ms)\n"
	"\n"
	"       dsc:rd:Addr           ex. dsc:rd:0x11        read  lcm addr=0x11\n"
	"       dsc:wt:addr:val       ex. dsc:wt:0x11:0x22   write lcm addr|cmd =0x11 value|paramer = 0x22\n"
	"       dsc:rw:addr:val:loop  ex. dsc:wt:0x11:0x22   write lcm addr =0x11 value = 0x22,and then read back\n"
	"                                                           compare the value and the value read back\n"
	"                                                           if different, break,and print the current times.\n"
	"\n"
	"       dump_ovl:[on|off[,down_sample_x[,down_sample_y]]]\n"
	"             Start to capture OVL only once\n"
	"\n"
	"       dump_layer:[on|off[,down_sample_x[,down_sample_y]]]\n"
	"             Start/end to capture current enabled OVL layer every frame\n";


/* --------------------------------------------------------------------------- */
/* Information Dump Routines */
/* --------------------------------------------------------------------------- */

void init_mtkfb_mmp_events(void)
{
	if (MTKFB_MMP_Events.MTKFB == 0) {
		MTKFB_MMP_Events.MTKFB = MMProfileRegisterEvent(MMP_RootEvent, "MTKFB");
		MTKFB_MMP_Events.PanDisplay =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "PanDisplay");
		MTKFB_MMP_Events.SetOverlayLayer =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "SetOverlayLayer");
		MTKFB_MMP_Events.SetOverlayLayers =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "SetOverlayLayers");
		MTKFB_MMP_Events.TrigOverlayOut =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "TrigOverlayOut");
		MTKFB_MMP_Events.UpdateScreenImpl =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "UpdateScreenImpl");
		MTKFB_MMP_Events.VSync = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "VSync");
		MTKFB_MMP_Events.UpdateConfig =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "UpdateConfig");
		MTKFB_MMP_Events.EsdCheck =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "EsdCheck");
		MTKFB_MMP_Events.ConfigOVL =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "ConfigOVL");
		MTKFB_MMP_Events.ConfigAAL =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "ConfigAAL");
		MTKFB_MMP_Events.ConfigMemOut =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "ConfigMemOut");
		MTKFB_MMP_Events.ScreenUpdate =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "ScreenUpdate");
		MTKFB_MMP_Events.CaptureFramebuffer =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "CaptureFB");
		MTKFB_MMP_Events.RegUpdate =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "RegUpdate");
		MTKFB_MMP_Events.EarlySuspend =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "EarlySuspend");
		MTKFB_MMP_Events.DispDone =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DispDone");
		MTKFB_MMP_Events.DSICmd = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSICmd");
		MTKFB_MMP_Events.DSIIRQ = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSIIrq");
		MTKFB_MMP_Events.WaitVSync =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "WaitVSync");
		MTKFB_MMP_Events.LayerDump =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "LayerDump");
		MTKFB_MMP_Events.Layer[0] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer0");
		MTKFB_MMP_Events.Layer[1] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer1");
		MTKFB_MMP_Events.Layer[2] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer2");
		MTKFB_MMP_Events.Layer[3] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer3");
		MTKFB_MMP_Events.OvlDump =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "OvlDump");
		MTKFB_MMP_Events.FBDump = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "FBDump");
		MTKFB_MMP_Events.DSIRead =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSIRead");
		MTKFB_MMP_Events.GetLayerInfo =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "GetLayerInfo");
		MTKFB_MMP_Events.LayerInfo[0] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo0");
		MTKFB_MMP_Events.LayerInfo[1] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo1");
		MTKFB_MMP_Events.LayerInfo[2] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo2");
		MTKFB_MMP_Events.LayerInfo[3] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo3");
		MTKFB_MMP_Events.IOCtrl = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "IOCtrl");
		MTKFB_MMP_Events.Debug = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "Debug");

		MTKFB_MMP_Events.QueueWork =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "QueueWork");
		MTKFB_MMP_Events.WrokHandler =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "WrokHandler");
		MTKFB_MMP_Events.FenceContrl =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "FenceContrl");
		MTKFB_MMP_Events.HWCFence[0] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.FenceContrl, "HWCFence0");
		MTKFB_MMP_Events.HWCFence[1] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.FenceContrl, "HWCFence1");
		MTKFB_MMP_Events.HWCFence[2] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.FenceContrl, "HWCFence2");
		MTKFB_MMP_Events.HWCFence[3] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.FenceContrl, "HWCFence3");
		MTKFB_MMP_Events.FBFence =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.FenceContrl, "FBFence");
		MTKFB_MMP_Events.FBTimeline =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.FenceContrl, "FBTimeline");

		MTKFB_MMP_Events.OVLEngine =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "OVLEngine");
		MTKFB_MMP_Events.Instance_status[0] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "Instance0_status");
		MTKFB_MMP_Events.Instance_status[1] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "Instance1_status");
		MTKFB_MMP_Events.Instance_status[2] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "Instance2_status");
		MTKFB_MMP_Events.OVLEngine_ovlwdma_status =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "OVLEngine_ovlwdma_status");
		MTKFB_MMP_Events.OVLEngine_rdma_status =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "OVLEngine_rdma_status");
		MTKFB_MMP_Events.OVLEngine_rdma_hang =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "OVLEngine_rdma_hang");
		MTKFB_MMP_Events.OVLEngine_fence[0] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "Instance0_fence");
		MTKFB_MMP_Events.OVLEngine_fence[1] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "Instance1_fence");
		MTKFB_MMP_Events.OVLEngine_fence[2] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "Instance2_fence");
		MTKFB_MMP_Events.OVLEngine_timeline[0] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "Instance0_timeline");
		MTKFB_MMP_Events.OVLEngine_timeline[1] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "Instance1_timeline");
		MTKFB_MMP_Events.OVLEngine_timeline[2] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "Instance2_timeline");
		MTKFB_MMP_Events.OVLEngine_API =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "OVLEngine_API");
		MTKFB_MMP_Events.OVLEngine_CORE =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "OVLEngine_CORE");
		MTKFB_MMP_Events.OVLEngine_HW =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "OVLEngine_HW");
		MTKFB_MMP_Events.OVLEngine_IOCTL =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine, "OVLEngine_IOCTL");

		MTKFB_MMP_Events.GetInstance =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "GetInstance");
		MTKFB_MMP_Events.ReleaseInstance =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "ReleaseInstance");
		MTKFB_MMP_Events.Get_layer_info =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "Get_layer_info");
		MTKFB_MMP_Events.Set_layer_info =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "Set_layer_info");
		MTKFB_MMP_Events.Get_Ovl_layer_info =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "Get_Ovl_layer_info");
		MTKFB_MMP_Events.Set_Overlayed_Buffer =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "Set_Overlayed_Buffer");
		MTKFB_MMP_Events.Trigger_Overlay =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "Trigger_Overlay");
		MTKFB_MMP_Events.Get_Dirty_info =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "Get_Dirty_info");
		MTKFB_MMP_Events.Set_Path_Info =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "Set_Path_Info");
		MTKFB_MMP_Events.Sync_Captured_layer_info =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API,
					   "Sync_Captured_layer_info");
		MTKFB_MMP_Events.Dump_layer_info =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "Dump_layer_info");
		MTKFB_MMP_Events.Trigger_Overlay_Handler =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API,
					   "Trigger_Overlay_Handler");
		MTKFB_MMP_Events.Trigger_Overlay_Fence =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "Trigger_Overlay_Fence");
		MTKFB_MMP_Events.Get_Fence_Fd =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_API, "Get_Fence_Fd");

		MTKFB_MMP_Events.Update_kthread =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_CORE, "Update_kthread");
		MTKFB_MMP_Events.wake_up_ovl_engine_thread =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_CORE,
					   "wake_up_ovl_engine_thread");
		MTKFB_MMP_Events.interrupt_handler =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_CORE, "interrupt_handler");

		MTKFB_MMP_Events.hw_set_params =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW, "hw_set_params");
		MTKFB_MMP_Events.hw_ovl_wdma_irq_handler =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW,
					   "hw_ovl_wdma_irq_handler");
		MTKFB_MMP_Events.hw_ovl_rdma_irq_handler =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW,
					   "hw_ovl_rdma_irq_handler");
		MTKFB_MMP_Events.rdma0_irq_handler =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW, "rdma0_irq_handler");
		MTKFB_MMP_Events.trigger_hw_overlay =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW, "trigger_hw_overlay");
		MTKFB_MMP_Events.trigger_hw_overlay_decouple =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW,
					   "trigger_hw_overlay_decouple");
		MTKFB_MMP_Events.trigger_hw_overlay_couple =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW,
					   "trigger_hw_overlay_couple");
		MTKFB_MMP_Events.config_overlay =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW, "config_overlay");
		MTKFB_MMP_Events.direct_link_overlay =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW, "direct_link_overlay");
		MTKFB_MMP_Events.indirect_link_overlay =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW, "indirect_link_overlay");
		MTKFB_MMP_Events.set_overlay_to_buffer =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW, "set_overlay_to_buffer");
		MTKFB_MMP_Events.dumpallinfo =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW, "dumpallinfo");
		MTKFB_MMP_Events.hw_mva_map =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW, "hw_mva_map");
		MTKFB_MMP_Events.hw_mva_unmap =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW, "hw_mva_unmap");
		MTKFB_MMP_Events.hw_reset =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.OVLEngine_HW, "hw_reset");

		MTKFB_MMP_Events.Svp = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "Svp");
		MTKFB_MMP_Events.OVL_WDMA_IRQ_InTEE =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.Svp, "OVL_WDMA_IRQ_InTEE");
		MTKFB_MMP_Events.OVL_WDMA_M4U_InTEE =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.Svp, "OVL_WDMA_M4U_InTEE");
		MTKFB_MMP_Events.OVL_WDMA_ADDR_InTEE =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.Svp, "OVL_WDMA_ADDR_InTEE");
		MTKFB_MMP_Events.RDMA0_IRQ_InTEE =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.Svp, "RDMA0_IRQ_InTEE");
		MTKFB_MMP_Events.RDMA0_M4U_InTEE =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.Svp, "RDMA0_M4U_InTEE");
		MTKFB_MMP_Events.RDMA0_ADDR_InTEE =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.Svp, "RDMA0_ADDR_InTEE");
		MTKFB_MMP_Events.RDMA1_IRQ_InTEE =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.Svp, "RDMA1_IRQ_InTEE");
		MTKFB_MMP_Events.RDMA1_M4U_InTEE =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.Svp, "RDMA1_M4U_InTEE");
		MTKFB_MMP_Events.RDMA1_ADDR_InTEE =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.Svp, "RDMA1_ADDR_InTEE");

		MMProfileEnableEventRecursive(MTKFB_MMP_Events.MTKFB, 1);
	}
}

static inline int is_layer_enable(unsigned int roi_ctl, unsigned int layer)
{
	return (roi_ctl >> (31 - layer)) & 0x1;
}

static void dump_layer_info(void)
{
	unsigned int i;

	for (i = 0; i < 4; i++) {
		pr_notice("LayerInfo, layer=%d,layer_en=%d, source=%d, fmt=%d, addr=0x%x, x=%d, y=%d\n",
			cached_layer_config[i].layer,	/* layer */
			cached_layer_config[i].layer_en,
			cached_layer_config[i].source,	/* data source (0=memory) */
			cached_layer_config[i].fmt, cached_layer_config[i].addr,	/* addr */
			cached_layer_config[i].dst_x,	/* x */
			cached_layer_config[i].dst_y);
		pr_notice("w=%d, h=%d, pitch=%d, keyEn=%d, key=%d, aen=%d, alpha=%d\n",
			cached_layer_config[i].dst_w,	/* width */
			cached_layer_config[i].dst_h,	/* height */
			cached_layer_config[i].src_pitch,	/* pitch, pixel number */
			cached_layer_config[i].keyEn,	/* color key */
			cached_layer_config[i].key,	/* color key */
			cached_layer_config[i].aen,	/* alpha enable */
			cached_layer_config[i].alpha);
	}
}


/* --------------------------------------------------------------------------- */
/* FPS Log */
/* --------------------------------------------------------------------------- */

typedef struct {
	long int current_lcd_time_us;
	long int current_te_delay_time_us;
	long int total_lcd_time_us;
	long int total_te_delay_time_us;
	long int start_time_us;
	long int trigger_lcd_time_us;
	unsigned int trigger_lcd_count;

	long int current_hdmi_time_us;
	long int total_hdmi_time_us;
	long int hdmi_start_time_us;
	long int trigger_hdmi_time_us;
	unsigned int trigger_hdmi_count;
} FPS_LOGGER;

static FPS_LOGGER fps = { 0 };
static FPS_LOGGER hdmi_fps = { 0 };

static long int get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return (t.tv_sec & 0xFFF) * 1000000 + t.tv_usec;
}


static void reset_fps_logger(void)
{
	memset(&fps, 0, sizeof(fps));
}

static void reset_hdmi_fps_logger(void)
{
	memset(&hdmi_fps, 0, sizeof(hdmi_fps));
}

void DBG_OnTriggerLcd(void)
{
	if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log)
		return;

	fps.trigger_lcd_time_us = get_current_time_us();
	if (fps.trigger_lcd_count == 0)
		fps.start_time_us = fps.trigger_lcd_time_us;
}

void DBG_OnTriggerHDMI(void)
{
	if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log)
		return;

	hdmi_fps.trigger_hdmi_time_us = get_current_time_us();
	if (hdmi_fps.trigger_hdmi_count == 0)
		hdmi_fps.hdmi_start_time_us = hdmi_fps.trigger_hdmi_time_us;
}

void DBG_OnTeDelayDone(void)
{
	long int time;

	if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log)
		return;

	time = get_current_time_us();
	fps.current_te_delay_time_us = (time - fps.trigger_lcd_time_us);
	fps.total_te_delay_time_us += fps.current_te_delay_time_us;
}


void DBG_OnLcdDone(void)
{
	long int time;

	if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log)
		return;

	/* deal with touch latency log */

	time = get_current_time_us();
	fps.current_lcd_time_us = (time - fps.trigger_lcd_time_us);

#if 0				/* FIXME */
	if (dbg_opt.en_touch_latency_log && tpd_start_profiling) {

		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "Touch Latency: %ld ms\n",
			       (time - tpd_last_down_time) / 1000);

		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG",
				"LCD update time %ld ms (TE delay %ld ms + LCD %ld ms)\n",
			       fps.current_lcd_time_us / 1000, fps.current_te_delay_time_us / 1000,
			       (fps.current_lcd_time_us - fps.current_te_delay_time_us) / 1000);

		tpd_start_profiling = 0;
	}
#endif

	if (!dbg_opt.en_fps_log)
		return;

	/* deal with fps log */

	fps.total_lcd_time_us += fps.current_lcd_time_us;
	++fps.trigger_lcd_count;

	if (fps.trigger_lcd_count >= dbg_opt.log_fps_wnd_size) {

		long int f = fps.trigger_lcd_count * 100 * 1000 * 1000 / (time - fps.start_time_us);

		long int update = fps.total_lcd_time_us * 100 / (1000 * fps.trigger_lcd_count);

		long int te = fps.total_te_delay_time_us * 100 / (1000 * fps.trigger_lcd_count);

		long int lcd = (fps.total_lcd_time_us - fps.total_te_delay_time_us) * 100
		    / (1000 * fps.trigger_lcd_count);

		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG",
"MTKFB FPS: %ld.%02ld, Avg. update time: %ld.%02ld ms (TE delay %ld.%02ld ms, LCD %ld.%02ld ms)\n",
			       f / 100, f % 100,
			       update / 100, update % 100, te / 100, te % 100, lcd / 100,
			       lcd % 100);
		reset_fps_logger();
	}
}

void DBG_OnHDMIDone(void)
{
	long int time;

	if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log)
		return;

	/* deal with touch latency log */

	time = get_current_time_us();
	hdmi_fps.current_hdmi_time_us = (time - hdmi_fps.trigger_hdmi_time_us);


	if (!dbg_opt.en_fps_log)
		return;

	/* deal with fps log */

	hdmi_fps.total_hdmi_time_us += hdmi_fps.current_hdmi_time_us;
	++hdmi_fps.trigger_hdmi_count;

	if (hdmi_fps.trigger_hdmi_count >= dbg_opt.log_fps_wnd_size) {

		long int f = hdmi_fps.trigger_hdmi_count * 100 * 1000 * 1000
		    / (time - hdmi_fps.hdmi_start_time_us);

		long int update = hdmi_fps.total_hdmi_time_us * 100
		    / (1000 * hdmi_fps.trigger_hdmi_count);

		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG",
				"[HDMI] FPS: %ld.%02ld, Avg. update time: %ld.%02ld ms\n", f / 100,
				f % 100, update / 100, update % 100);

		reset_hdmi_fps_logger();
	}
}

/* --------------------------------------------------------------------------- */
/* Command Processor */
/* --------------------------------------------------------------------------- */

static void process_dbg_opt(const char *opt)
{
	if (0 == strncmp(opt, "fps:", 4)) {
		if (0 == strncmp(opt + 4, "on", 2))
			dbg_opt.en_fps_log = 1;
		else if (0 == strncmp(opt + 4, "off", 3))
			dbg_opt.en_fps_log = 0;
		else
			goto Error;
		reset_fps_logger();
	} else if (0 == strncmp(opt, "tl:", 3)) {
		if (0 == strncmp(opt + 3, "on", 2))
			dbg_opt.en_touch_latency_log = 1;
		else if (0 == strncmp(opt + 3, "off", 3))
			dbg_opt.en_touch_latency_log = 0;
		else
			goto Error;
	} else if (0 == strncmp(opt, "black", 5))
		mtkfb_clear_lcm();
	else if (0 == strncmp(opt, "suspend", 4)) {
		DISP_PanelEnable(false);
		DISP_PowerEnable(false);
	} else if (0 == strncmp(opt, "resume", 4)) {
		DISP_PowerEnable(true);
		DISP_PanelEnable(true);
	} else if (0 == strncmp(opt, "lcm:", 4)) {
		if (0 == strncmp(opt + 4, "on", 2))
			DISP_PanelEnable(true);
		else if (0 == strncmp(opt + 4, "off", 3))
			DISP_PanelEnable(false);
		else if (0 == strncmp(opt + 4, "init", 4)) {
			if (NULL != lcm_drv && NULL != lcm_drv->init)
				lcm_drv->init();
		} else
			goto Error;
	} else if (0 == strncmp(opt, "cabc:", 5)) {
		if (0 == strncmp(opt + 5, "ui", 2))
			mtkfb_set_backlight_mode(1);
		else if (0 == strncmp(opt + 5, "mov", 3))
			mtkfb_set_backlight_mode(3);
		else if (0 == strncmp(opt + 5, "still", 5))
			mtkfb_set_backlight_mode(2);
		else
			goto Error;
	} else if (0 == strncmp(opt, "lcd:", 4)) {
		if (0 == strncmp(opt + 4, "on", 2))
			DISP_PowerEnable(true);
		else if (0 == strncmp(opt + 4, "off", 3))
			DISP_PowerEnable(false);
		else
			goto Error;
	} else if (0 == strncmp(opt, "vsynclog:", 9)) {
		if (0 == strncmp(opt + 9, "on", 2))
			EnableVSyncLog = 1;
		else if (0 == strncmp(opt + 9, "off", 3))
			EnableVSyncLog = 0;
		else
			goto Error;
	} else if (0 == strncmp(opt, "layer", 5))
		dump_layer_info();
	else if (0 == strncmp(opt, "regw:", 5)) {
		char *p = (char *)opt + 5;
		unsigned int addr;
		unsigned int val;

		STR_CONVERT(&p, &addr, uint, goto Error);
		STR_CONVERT(&p, &val, uint, goto Error);

		if (addr)
			DISP_REG_SET(addr, val);
		else
			goto Error;
	} else if (0 == strncmp(opt, "regr:", 5)) {
		char *p = (char *)opt + 5;
		unsigned int addr;

		STR_CONVERT(&p, &addr, uint, goto Error);

		if (addr)
			DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "Read register 0x%08x: 0x%08x\n",
				       addr, DISP_REG_GET(addr));
		else
			goto Error;
	} else if (0 == strncmp(opt, "bkl:", 4)) {
		char *p = (char *)opt + 4;
		unsigned int level;

		STR_CONVERT(&p, &level, uint, goto Error);

		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG",
			       "process_dbg_opt(), set backlight level = %d\n", level);
		DISP_SetBacklight(level);
	} else if (0 == strncmp(opt, "dither:", 7)) {
		unsigned lrs, lgs, lbs, dbr, dbg, dbb;
		char *p = (char *)opt + 7;

		STR_CONVERT(&p, &lrs, uint, goto Error);
		p++;
		STR_CONVERT(&p, &lgs, uint, goto Error);
		p++;
		STR_CONVERT(&p, &lbs, uint, goto Error);
		p++;
		STR_CONVERT(&p, &dbr, uint, goto Error);
		p++;
		STR_CONVERT(&p, &dbg, uint, goto Error);
		p++;
		STR_CONVERT(&p, &dbb, uint, goto Error);

		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "process_dbg_opt(), %d %d %d %d %d %d\n",
			       lrs, lgs, lbs, dbr, dbg, dbb);
	}
#if defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
	else if (0 == strncmp(opt, "ovlengine_log:", 14)) {

		char *p = (char *)opt + 14;
		unsigned int level;

		STR_CONVERT(&p, &level, uint, goto Error);

		set_ovlengine_debug_level(level);
	}
#endif
	else if (0 == strncmp(opt, "update", 6))
		DISP_UpdateScreen(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight());
	else if (0 == strncmp(opt, "pan_disp", 8))
		mtkfb_pan_disp_test();
	else if (0 == strncmp(opt, "sem_cnt", 7))
		mtkfb_show_sem_cnt();
	else if (0 == strncmp(opt, "hang:", 5)) {
		if (0 == strncmp(opt + 5, "on", 2))
			mtkfb_hang_test(true);
		else if (0 == strncmp(opt + 5, "off", 3))
			mtkfb_hang_test(false);
		else
			goto Error;
	} else if (0 == strncmp(opt, "cpfbonly:", 9)) {
		if (0 == strncmp(opt + 9, "on", 2))
			mtkfb_capture_fb_only(true);
		else if (0 == strncmp(opt + 9, "off", 3))
			mtkfb_capture_fb_only(false);
	} else if (0 == strncmp(opt, "esd:", 4)) {
		if (0 == strncmp(opt + 4, "on", 2))
			esd_recovery_pause(false);
		else if (0 == strncmp(opt + 4, "off", 3))
			esd_recovery_pause(true);
	} else if (0 == strncmp(opt, "mmp", 3))
		init_mtkfb_mmp_events();
	else if (0 == strncmp(opt, "dump_layer:", 11)) {
		if (0 == strncmp(opt + 11, "on", 2)) {
			char *p = (char *)opt + 14;

			STR_CONVERT(&p, &gCaptureLayerDownX, uint, goto Error);
			STR_CONVERT(&p, &gCaptureLayerDownY, uint, goto Error);

			if (gCaptureLayerDownX <= 0 || gCaptureLayerDownX > 100
			    || gCaptureLayerDownY <= 0 || gCaptureLayerDownY > 100) {
				gCaptureLayerDownX = 10;
				gCaptureLayerDownY = 10;
			}

			gCaptureLayerEnable = 1;
		} else if (0 == strncmp(opt + 11, "off", 3))
			gCaptureLayerEnable = 0;
	} else if (0 == strncmp(opt, "dump_ovl:", 9)) {
		if (0 == strncmp(opt + 9, "on", 2)) {
			char *p = (char *)opt + 12;

			STR_CONVERT(&p, &gCaptureOvlDownX, uint, goto Error);
			STR_CONVERT(&p, &gCaptureOvlDownY, uint, goto Error);

			if (gCaptureOvlDownX <= 0 || gCaptureOvlDownX > 100
			    || gCaptureOvlDownY <= 0 || gCaptureOvlDownY > 100) {
				gCaptureOvlDownX = 10;
				gCaptureOvlDownY = 10;
			}
#ifndef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
			gCaptureOvlThreadEnable = 1;
			wake_up_process(captureovl_task);
#else
			gDumpOVLEnable = 1;
#endif
		} else if (0 == strncmp(opt + 9, "off", 3)) {
#ifndef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
			gCaptureOvlThreadEnable = 0;
#else
			gDumpOVLEnable = 0;
#endif
		}
	} else if (0 == strncmp(opt, "dump_fb:", 8)) {
		if (0 == strncmp(opt + 8, "on", 2)) {
			char *p = (char *)opt + 11;

			STR_CONVERT(&p, &gCaptureFBDownX, uint, goto Error);
			STR_CONVERT(&p, &gCaptureFBDownY, uint, goto Error);
			STR_CONVERT(&p, &gCaptureFBPeriod, uint, goto Error);
			gCaptureFBEnable = 1;
			wake_up_interruptible(&gCaptureFBWQ);
		} else if (0 == strncmp(opt + 8, "off", 3))
			gCaptureFBEnable = 0;
	} else if (0 == strncmp(opt, "fb_pattern:", 11)) {
		if (0 == strncmp(opt + 11, "on", 2))
			fb_pattern_en(1);
		else if (0 == strncmp(opt + 11, "off", 3))
			fb_pattern_en(0);
		else if (0 == strncmp(opt + 11, "svp", 3))
			fb_pattern_en(2);
	} else if (0 == strncmp(opt, "dal:", 4)) {
		if (0 == strncmp(opt + 4, "test", 4)) {
			DAL_Printf("display driver DAL test\n");
			DAL_Printf("use \"echo dal:clean > /d/mtkfb\" to clean it\n");
		} else if (0 == strncmp(opt + 4, "clean", 5))
			DAL_Clean();
	} else {
		if (disphal_process_dbg_opt(opt))
			goto Error;
	}

	return;

Error:
	DISP_LOG_PRINT(ANDROID_LOG_INFO, "ERROR", "parse command error!\n\n%s", STR_HELP);
}


static void process_dbg_cmd(char *cmd)
{
	char *tok;

	DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "[mtkfb_dbg] %s\n", cmd);
	memset(debug_buffer, 0, sizeof(debug_buffer));
	while ((tok = strsep(&cmd, " ")) != NULL)
		process_dbg_opt(tok);
}


/* --------------------------------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------------------------------- */

struct dentry *mtkfb_dbgfs = NULL;


static ssize_t debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}




static ssize_t debug_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	/* const int debug_bufmax = sizeof(debug_buffer) - 1; */
	/* int n = 0; */

	/* n += scnprintf(debug_buffer + n, debug_bufmax - n, STR_HELP); */
	/* debug_buffer[n++] = 0; */

	/* return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n); */


	if (strlen(debug_buffer))
		return simple_read_from_buffer(ubuf, count, ppos, debug_buffer,
					       strlen(debug_buffer));
	else
		return simple_read_from_buffer(ubuf, count, ppos, STR_HELP, strlen(STR_HELP));


}


static char dis_cmd_buf[512];
static ssize_t debug_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(dis_cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&dis_cmd_buf, ubuf, count))
		return -EFAULT;

	dis_cmd_buf[count] = 0;

	process_dbg_cmd(dis_cmd_buf);

	return ret;
}


static const struct file_operations debug_fops = {
	.read = debug_read,
	.write = debug_write,
	.open = debug_open,
};

#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT

static ssize_t layer_debug_open(struct inode *inode, struct file *file)
{
	MTKFB_LAYER_DBG_OPTIONS *dbgopt;
	/* /record the private data */
	file->private_data = inode->i_private;
	dbgopt = (MTKFB_LAYER_DBG_OPTIONS *) file->private_data;

	dbgopt->working_size = DISP_GetScreenWidth() * DISP_GetScreenHeight() * 2 + 32;
	dbgopt->working_buf = (uint32_t) vmalloc(dbgopt->working_size);
	if (dbgopt->working_buf == 0)
		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "Vmalloc to get temp buffer failed\n");

	return 0;
}


static ssize_t layer_debug_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	return 0;
}


static ssize_t layer_debug_write(struct file *file,
				 const char __user *ubuf, size_t count, loff_t *ppos)
{
	MTKFB_LAYER_DBG_OPTIONS *dbgopt = (MTKFB_LAYER_DBG_OPTIONS *) file->private_data;

	DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "mtkfb_layer%d write is not implemented yet\n",
		       dbgopt->layer_index);

	return count;
}

static int layer_debug_release(struct inode *inode, struct file *file)
{
	MTKFB_LAYER_DBG_OPTIONS *dbgopt;

	dbgopt = (MTKFB_LAYER_DBG_OPTIONS *) file->private_data;

	if (dbgopt->working_buf != 0)
		vfree((void *)dbgopt->working_buf);

	dbgopt->working_buf = 0;

	return 0;
}


static const struct file_operations layer_debug_fops = {
	.read = layer_debug_read,
	.write = layer_debug_write,
	.open = layer_debug_open,
	.release = layer_debug_release,
};

#endif

void DBG_Init(void)
{
	mtkfb_dbgfs = debugfs_create_file("mtkfb", S_IFREG | S_IRUGO, NULL, (void *)0, &debug_fops);

	memset(&dbg_opt, 0, sizeof(dbg_opt));
	memset(&fps, 0, sizeof(fps));
	dbg_opt.log_fps_wnd_size = DEFAULT_LOG_FPS_WND_SIZE;

#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
	{
		unsigned int i;
		unsigned char a[13];

		a[0] = 'm';
		a[1] = 't';
		a[2] = 'k';
		a[3] = 'f';
		a[4] = 'b';
		a[5] = '_';
		a[6] = 'l';
		a[7] = 'a';
		a[8] = 'y';
		a[9] = 'e';
		a[10] = 'r';
		a[11] = '0';
		a[12] = '\0';

		for (i = 0; i < DDP_OVL_LAYER_MUN; i++) {
			a[11] = '0' + i;
			mtkfb_layer_dbg_opt[i].layer_index = i;
			mtkfb_layer_dbgfs[i] = debugfs_create_file(a,
								   S_IFREG | S_IRUGO, NULL,
								   (void *)&mtkfb_layer_dbg_opt[i],
								   &layer_debug_fops);
		}
	}
#endif

#ifdef CONFIG_MTK_MMPROFILE_SUPPORT
	MMProfileEnable(1);
	init_mtkfb_mmp_events();
	init_ddp_mmp_events();
	MMProfileStart(0);
	MMProfileStart(1);
#endif
}


void DBG_Deinit(void)
{
	debugfs_remove(mtkfb_dbgfs);
#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
	{
		unsigned int i;

		for (i = 0; i < DDP_OVL_LAYER_MUN; i++)
			debugfs_remove(mtkfb_layer_dbgfs[i]);
	}
#endif
}
