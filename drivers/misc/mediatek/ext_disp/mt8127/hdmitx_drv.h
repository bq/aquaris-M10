/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

/* --------------------------------------------------------------------------- */
#ifndef HDMITX_DRV_H
#define HDMITX_DRV_H

#include <generated/autoconf.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <disp_assert_layer.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/switch.h>
#if defined(CONFIG_MTK_SMARTBOOK_SUPPORT)
#include <linux/sbsuspend.h>
#endif
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include "mt-plat/dma.h"
#include <linux/of_irq.h>
#include <asm/tlbflush.h>
#include <asm/page.h>

#include <m4u.h>
/* #include <mach/mt_typedefs.h> */
/* #include <mach/mt_reg_base.h> */
#include <linux/clk.h>
#include "mt-plat/mt_boot.h"
#include <linux/suspend.h>

/* #include "linux/hdmitx.h" */
#include "mtkfb_info.h"

#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif

#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)
#include "internal_hdmi_drv.h"
#elif defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
#include "inter_mhl_drv.h"
#else
#include "hdmi_drv.h"
#endif

/* #include "hdmi_utils.h" */

#include "dpi_reg.h"
/* #include "mach/eint.h" */
#include <linux/of_irq.h>
#include "disp_drv_platform.h"
#include "ddp_reg.h"
#include "mtkfb.h"
#include "dpi_drv.h"
#include "dpi1_drv.h"
#if defined(CONFIG_MTK_SMARTBOOK_SUPPORT)
#include "smartbook.h"
#endif

#ifdef I2C_DBG
#include "tmbslHdmiTx_types.h"
#include "tmbslTDA9989_local.h"
#endif

#if MTK_HDMI_MAIN_PATH
#include "ddp_ovl.h"
#include "ddp_rdma.h"
#include "ddp_bls.h"
#include "ddp_color.h"
#endif

#if defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
#include "tz_cross/trustzone.h"
#include "tz_cross/ta_mem.h"
#include <tz_cross/tz_ddp.h>
#include "trustzone/kree/system.h"
#include "trustzone/kree/mem.h"
#endif
#include "mt-plat/mt_smi.h"
#include <mmprofile.h>


#define HDMI_CHECK_RET(expr)\
	do {\
		HDMI_STATUS ret = (expr);\
		if (HDMI_STATUS_OK != ret) {\
			pr_err("[ERROR][mtkfb] HDMI API return error code: 0x%x\n"\
				"  file : %s, line : %d\n"\
				"  expr : %s\n", ret, __FILE__, __LINE__, #expr);\
		} \
	} while (0)

/*#define MTK_AUDIO_MULTI_CHANNEL_SUPPORT*/

extern unsigned int mtkfb_get_fb_phys_addr(void);
extern unsigned int mtkfb_get_fb_size(void);
extern unsigned int mtkfb_get_fb_va(void);

struct ext_memory_info {
	unsigned int buffer_num;
	unsigned int width;
	unsigned int height;
	unsigned int bpp;
};

struct ext_buffer {
	unsigned int id;
	unsigned int ts_sec;
	unsigned int ts_nsec;
};
extern struct HDMI_MMP_Events_t {
	MMP_Event HDMI;
	MMP_Event DDPKBitblt;
	MMP_Event OverlayDone;
	MMP_Event SwitchRDMABuffer;
	MMP_Event SwitchOverlayBuffer;
	MMP_Event StopOverlayBuffer;
	MMP_Event RDMA1RegisterUpdated;
	MMP_Event WDMA1RegisterUpdated;
	MMP_Event WaitVSync;
	MMP_Event BufferPost;
	MMP_Event BufferInsert;
	MMP_Event BufferAdd;
	MMP_Event BufferUsed;
	MMP_Event BufferRemove;
	MMP_Event FenceCreate;
	MMP_Event FenceSignal;
	MMP_Event HDMIState;
	MMP_Event GetDevInfo;
	MMP_Event ErrorInfo;
	MMP_Event MutexErr;
	MMP_Event BufferCfg;
	MMP_Event BufferUpdate;
} HDMI_MMP_Events;

/* #define MTK_EXT_DISPLAY_ENTER                   HDMI_IO(40)
#define MTK_EXT_DISPLAY_LEAVE                   HDMI_IO(41)
#define MTK_EXT_DISPLAY_START                   HDMI_IO(42)
#define MTK_EXT_DISPLAY_STOP                    HDMI_IO(43)
#define MTK_EXT_DISPLAY_SET_MEMORY_INFO         HDMI_IOW(44, struct ext_memory_info)
#define MTK_EXT_DISPLAY_GET_MEMORY_INFO         HDMI_IOW(45, struct ext_memory_info)
#define MTK_EXT_DISPLAY_GET_BUFFER              HDMI_IOW(46, struct ext_buffer)
#define MTK_EXT_DISPLAY_FREE_BUFFER             HDMI_IOW(47, struct ext_buffer)

#define MTK_HDMI_ENTER_VIDEO_MODE               HDMI_IO(17)
#define MTK_HDMI_LEAVE_VIDEO_MODE               HDMI_IO(18)
#define MTK_HDMI_REGISTER_VIDEO_BUFFER          HDMI_IOW(19, hdmi_video_buffer_info) */

enum HDMI_report_state {
	NO_DEVICE = 0,
	HDMI_PLUGIN = 1,
};

typedef enum {
	HDMI_CHARGE_CURRENT,

} HDMI_QUERY_TYPE;

int get_hdmi_dev_info(HDMI_QUERY_TYPE type);
bool is_hdmi_enable(void);
void hdmi_setorientation(int orientation);
void hdmi_suspend(void);
void hdmi_resume(void);
void hdmi_power_on(void);
void hdmi_power_off(void);
void hdmi_update_buffer_switch(void);
void hdmi_update(void);
void hdmi_dpi_config_clock(void);
void hdmi_dpi_power_switch(bool enable);
int hdmi_audio_config(int samplerate);
int hdmi_video_enable(bool enable);
int hdmi_audio_enable(bool enable);
int hdmi_audio_delay_mute(int latency);
void hdmi_set_mode(unsigned char ucMode);
void hdmi_reg_dump(void);

#if defined(CONFIG_MTK_MT8193_HDMI_SUPPORT)
void hdmi_read_reg(unsigned char u8Reg, unsigned int *p4Data);
#else
void hdmi_read_reg(unsigned char u8Reg);
#endif
void hdmi_write_reg(unsigned char u8Reg, unsigned char u8Data);
#if defined(CONFIG_MTK_SMARTBOOK_SUPPORT)
void smartbook_state_callback(void);
#endif

extern const struct HDMI_DRIVER *HDMI_GetDriver(void);
extern void HDMI_DBG_Init(void);

extern uint32_t DISP_GetScreenHeight(void);
extern uint32_t DISP_GetScreenWidth(void);
extern bool DISP_IsVideoMode(void);
extern int disp_lock_mutex(void);
extern int disp_unlock_mutex(int id);
extern int disp_module_clock_on(DISP_MODULE_ENUM module, char *caller_name);
extern int disp_module_clock_off(DISP_MODULE_ENUM module, char *caller_name);
extern int m4u_do_mva_map_kernel(unsigned int mva, unsigned int size, int sec,
					unsigned int *map_va, unsigned int *map_size);
extern int m4u_do_mva_unmap_kernel(unsigned int mva, unsigned int size, unsigned int va);
int hdmi_allocate_hdmi_buffer(void);
int hdmi_free_hdmi_buffer(void);
int hdmi_rdma_address_config(bool enable, hdmi_video_buffer_info buffer_info);

extern unsigned int fb_mva;
extern phys_addr_t fb_base_pa;


#if defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
extern void disp_register_intr(unsigned int irq, unsigned int secure);
extern KREE_SESSION_HANDLE ddp_session_handle(void);
unsigned int gRDMASecure = 0;
unsigned int gRDMASecureSwitch = 0;
#endif

#ifdef CONFIG_PM
extern int hdmi_drv_pm_restore_noirq(struct device *device);
extern void hdmi_module_init(void);
#endif
extern bool is_early_suspended;
extern void DBG_OnTriggerHDMI(void);
extern void DBG_OnHDMIDone(void);
void smartbook_state_callback(void);
/* extern void smi_dynamic_adj_hint_mhl(int mhl_enable); */


#endif
