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

#ifndef __DISP_DRV_H__
#define __DISP_DRV_H__

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include "disp_drv_log.h"
#include "ddp_path.h"
#include "lcm_drv.h"
#include "disp_hal.h"
#include "mtkfb_info.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------- */

#define DISP_CHECK_RET(expr)                                                \
	do {                                                                    \
		DISP_STATUS ret = (expr);                                           \
		if (DISP_STATUS_OK != ret)	\
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "COMMON",						\
					"[ERROR][mtkfb] DISP API return error code: 0x%x\n"      \
					"  file : %s, line : %d\n"                               \
					"  expr : %s\n", ret, __FILE__, __LINE__, #expr);        \
	} while (0)


/* --------------------------------------------------------------------------- */
#define ASSERT_LAYER    (DDP_OVL_LAYER_MUN-1)
#define DISP_DEFAULT_UI_LAYER_ID (DDP_OVL_LAYER_MUN-1)
#define DISP_CHANGED_UI_LAYER_ID (DDP_OVL_LAYER_MUN-2)
#define HW_OVERLAY_COUNT                 (4)
#define RESERVED_LAYER_COUNT             (2)
#define VIDEO_LAYER_COUNT                (HW_OVERLAY_COUNT - RESERVED_LAYER_COUNT)

typedef struct {
	unsigned int id;
	unsigned int curr_en;
	unsigned int next_en;
	unsigned int hw_en;
	int curr_idx;
	int next_idx;
	int hw_idx;
	int curr_identity;
	int next_identity;
	int hw_identity;
	int curr_conn_type;
	int next_conn_type;
	int hw_conn_type;
} DISP_LAYER_INFO;

extern unsigned int FB_LAYER;	/* default LCD layer */
extern unsigned int lcd_fps;

extern struct semaphore sem_early_suspend;
extern unsigned int EnableVSyncLog;

extern LCM_PARAMS *lcm_params;
extern atomic_t OverlaySettingDirtyFlag;
extern atomic_t OverlaySettingApplied;
extern unsigned int PanDispSettingPending;
extern unsigned int PanDispSettingDirty;
extern unsigned int PanDispSettingApplied;
extern unsigned int fb_mva;
extern bool is_ipoh_bootup;
extern struct mutex OverlaySettingMutex;
extern wait_queue_head_t disp_reg_update_wq;
extern unsigned int need_esd_check;
extern wait_queue_head_t esd_check_wq;
extern bool esd_kthread_pause;
extern struct fb_info *mtkfb_fbi;

extern LCM_DRIVER *lcm_driver_list[];
extern unsigned int lcm_count;
extern mtk_dispif_info_t dispif_info[MTKFB_MAX_DISPLAY_COUNT];

extern OVL_CONFIG_STRUCT cached_layer_config[DDP_OVL_LAYER_MUN];
extern bool is_early_suspended;
extern const FBCONFIG_DISP_IF *disphal_fbconfig_get_def_if(void);

#include "disp_ovl_engine_api.h"

/* --------------------------------------------------------------------------- */
/* Public Functions */
/* --------------------------------------------------------------------------- */

	DISP_STATUS DISP_Init(uint32_t fbVA, uint32_t fbPA, bool isLcmInited);
	DISP_STATUS DISP_Deinit(void);
	DISP_STATUS DISP_PowerEnable(bool enable);
	DISP_STATUS DISP_PanelEnable(bool enable);
	DISP_STATUS DISP_SetFrameBufferAddr(uint32_t fbPhysAddr);
	DISP_STATUS DISP_EnterOverlayMode(void);
	DISP_STATUS DISP_LeaveOverlayMode(void);
	DISP_STATUS DISP_UpdateScreen(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	DISP_STATUS DISP_WaitForLCDNotBusy(void);
	DISP_STATUS DISP_PrepareSuspend(void);
	DISP_STATUS DISP_GetLayerInfo(DISP_LAYER_INFO *pLayer);

/* Register extra trigger source */
	typedef int (*DISP_EXTRA_CHECKUPDATE_PTR) (int);
	typedef int (*DISP_EXTRA_CONFIG_PTR) (int);
	int DISP_RegisterExTriggerSource(DISP_EXTRA_CHECKUPDATE_PTR pCheckUpdateFunc,
					 DISP_EXTRA_CONFIG_PTR pConfFunc);
	void DISP_UnRegisterExTriggerSource(int u4ID);
	void GetUpdateMutex(void);
	void ReleaseUpdateMutex(void);

	DISP_STATUS DISP_ConfigDither(int lrs, int lgs, int lbs, int dbr, int dbg, int dbb);


/* Retrieve Information */
	bool DISP_IsVideoMode(void);
	uint32_t DISP_GetScreenWidth(void);
	uint32_t DISP_GetScreenHeight(void);
	uint32_t DISP_GetActiveWidth(void);
	uint32_t DISP_GetActiveHeight(void);
	uint32_t DISP_GetDensity(void);
	uint32_t DISP_GetScreenBpp(void);
	uint32_t DISP_GetPages(void);
	DISP_STATUS DISP_SetScreenBpp(uint32_t);	/* /config how many bits for each pixel of framebuffer */
	DISP_STATUS DISP_SetPages(uint32_t);	/* /config how many framebuffer will be used */
/* /above information is used to determine the vRAM size */

	bool DISP_IsDirectLinkMode(void);
	bool DISP_IsInOverlayMode(void);
	uint32_t DISP_GetFBRamSize(void);	/* /get FB buffer size */
	uint32_t DISP_GetVRamSize(void);	/* / get total RAM size (FB+working buffer+DAL buffer) */
	PANEL_COLOR_FORMAT DISP_GetPanelColorFormat(void);
	uint32_t DISP_GetPanelBPP(void);
	bool DISP_IsLcmFound(void);
	bool DISP_IsImmediateUpdate(void);
	DISP_STATUS DISP_ConfigImmediateUpdate(bool enable);

	DISP_STATUS DISP_SetBacklight(uint32_t level);
	DISP_STATUS DISP_SetPWM(uint32_t divider);
	DISP_STATUS DISP_GetPWM(uint32_t divider, unsigned int *freq);
	DISP_STATUS DISP_SetBacklight_mode(uint32_t mode);

	DISP_STATUS DISP_Set3DPWM(bool enable, bool landscape);

/* FM De-sense */

	DISP_STATUS DISP_Get_Default_UpdateSpeed(unsigned int *speed);
	DISP_STATUS DISP_Get_Current_UpdateSpeed(unsigned int *speed);
	DISP_STATUS DISP_Change_Update(unsigned int);

/* ///////////// */

/* --------------------------------------------------------------------------- */
/* Private Functions */
/* --------------------------------------------------------------------------- */

	bool DISP_SelectDevice(const char *lcm_name);
	bool DISP_DetectDevice(void);
	bool DISP_SelectDeviceBoot(const char *lcm_name);
	uint32_t DISP_GetVRamSizeBoot(char *cmdline);
	DISP_STATUS DISP_Capture_Framebuffer(unsigned int pvbuf, unsigned int bpp,
					     unsigned int is_early_suspended);
	bool DISP_IsContextInited(void);

	uint32_t DISP_GetOutputBPPforDithering(void);
	const char *DISP_GetLCMId(void);

	bool DISP_EsdRecoverCapbility(void);
	bool DISP_EsdCheck(void);
	bool DISP_EsdRecover(void);
	void DISP_WaitVSYNC(void);
	void DISP_InitVSYNC(unsigned int vsync_interval);
	DISP_STATUS DISP_Config_Overlay_to_Memory(unsigned int mva, int enable);
	void DISP_StartConfigUpdate(void);

	unsigned long DISP_GetLCMIndex(void);
/* --------------------------------------------------------------------------- */
	void DISP_Change_LCM_Resolution(unsigned int width, unsigned int height);

	void fbconfig_disp_set_mipi_clk(unsigned int clk);
	void fbconfig_disp_set_mipi_ssc(unsigned int ssc);
	bool fbconfig_dsi_vdo_prepare(void);
	bool fbconfig_rest_lcm_setting_prepare(void);
	void fbconfig_disp_set_mipi_lane_num(unsigned int lane_num);
	int fbconfig_get_esd_check(void);
	void _DISP_DumpLayer(OVL_CONFIG_STRUCT *pLayer);

#ifdef __cplusplus
}
#endif
#endif				/* __DISP_DRV_H__ */
