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

#ifndef __LCD_DRV_H__
#define __LCD_DRV_H__

#include "disp_intr.h"
#include "lcm_drv.h"
#include <disp_drv_platform.h>
#include <linux/proc_fs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------- */

#define LCD_CHECK_RET(expr)             \
	do {                                \
		LCD_STATUS ret = (expr);        \
		ASSERT(LCD_STATUS_OK == ret);   \
	} while (0)

/* --------------------------------------------------------------------------- */
	extern OVL_CONFIG_STRUCT cached_layer_config[DDP_OVL_LAYER_MUN];
	extern OVL_CONFIG_STRUCT *realtime_layer_config;

	extern LCM_PARAMS *lcm_params;
	extern unsigned int isAEEEnabled;
	extern struct mutex OverlaySettingMutex;
/* --------------------------------------------------------------------------- */

	enum LCD_STATUS {
		LCD_STATUS_OK = 0,

		LCD_STATUS_ERROR,
	};
#define LCD_STATUS    enum LCD_STATUS

	enum LCD_STATE {
		LCD_STATE_IDLE = 0,
		LCD_STATE_BUSY,
		LCD_STATE_POWER_OFF,
	};
#define LCD_STATE    enum LCD_STATE

	enum LCD_IF_ID {
		LCD_IF_PARALLEL_0 = 0,
		LCD_IF_PARALLEL_1 = 1,
		LCD_IF_PARALLEL_2 = 2,
		LCD_IF_SERIAL_0 = 3,
		LCD_IF_SERIAL_1 = 4,

		LCD_IF_ALL = 0xFF,
	};
#define LCD_IF_ID    enum LCD_IF_ID


	enum LCD_IF_PARALLEL_BITS {
		LCD_IF_PARALLEL_8BITS = 0,
		LCD_IF_PARALLEL_9BITS = 1,
		LCD_IF_PARALLEL_16BITS = 2,
		LCD_IF_PARALLEL_18BITS = 3,
		LCD_IF_PARALLEL_24BITS = 4,
	};
#define LCD_IF_PARALLEL_BITS    enum LCD_IF_PARALLEL_BITS


	enum LCD_IF_SERIAL_BITS {
		LCD_IF_SERIAL_8BITS = 0,
		LCD_IF_SERIAL_9BITS = 1,
		LCD_IF_SERIAL_16BITS = 2,
		LCD_IF_SERIAL_18BITS = 3,
		LCD_IF_SERIAL_24BITS = 4,
		LCD_IF_SERIAL_32BITS = 5,
	};
#define LCD_IF_SERIAL_BITS    enum LCD_IF_SERIAL_BITS

	enum LCD_IF_PARALLEL_CLK_DIV {
		LCD_IF_PARALLEL_CLK_DIV_1 = 0,
		LCD_IF_PARALLEL_CLK_DIV_2,
		LCD_IF_PARALLEL_CLK_DIV_4,
	};
#define LCD_IF_PARALLEL_CLK_DIV    enum LCD_IF_PARALLEL_CLK_DIV

/*
typedef enum
{
    LCD_IF_SERIAL_CLK_DIV_1 = 0,
    LCD_IF_SERIAL_CLK_DIV_2,
    LCD_IF_SERIAL_CLK_DIV_4,
    LCD_IF_SERIAL_CLK_DIV_8,
    LCD_IF_SERIAL_CLK_DIV_16,
} LCD_IF_SERIAL_CLK_DIV;
*/

	enum LCD_IF_A0_MODE {
		LCD_IF_A0_LOW = 0,
		LCD_IF_A0_HIGH = 1,
	};
#define LCD_IF_A0_MODE    enum LCD_IF_A0_MODE


	enum LCD_IF_MCU_WRITE_BITS {
		LCD_IF_MCU_WRITE_8BIT = 8,
		LCD_IF_MCU_WRITE_16BIT = 16,
		LCD_IF_MCU_WRITE_32BIT = 32,
	};
#define LCD_IF_MCU_WRITE_BITS    enum LCD_IF_MCU_WRITE_BITS


	enum LCD_IF_FORMAT {
		LCD_IF_FORMAT_RGB332 = 0,
		LCD_IF_FORMAT_RGB444 = 1,
		LCD_IF_FORMAT_RGB565 = 2,
		LCD_IF_FORMAT_RGB666 = 3,
		LCD_IF_FORMAT_RGB888 = 4,
	};
#define LCD_IF_FORMAT    enum LCD_IF_FORMAT


	enum LCD_IF_WIDTH {
		LCD_IF_WIDTH_8_BITS = 0,
		LCD_IF_WIDTH_9_BITS = 2,
		LCD_IF_WIDTH_16_BITS = 1,
		LCD_IF_WIDTH_18_BITS = 3,
		LCD_IF_WIDTH_24_BITS = 4,
		LCD_IF_WIDTH_32_BITS = 5,
	};
#define LCD_IF_WIDTH    enum LCD_IF_WIDTH

/*
typedef enum
{
    LCD_CMDQ_0 = 0,
    LCD_CMDQ_1 = 1,
} LCD_CMDQ_ID;
*/

	enum LCD_FB_ID {
		LCD_FB_0 = 0,
		LCD_FB_1 = 1,
		LCD_FB_2 = 2,
		LCD_FB_NUM,
	};
#define LCD_FB_ID    enum LCD_FB_ID


	enum LCD_FB_FORMAT {
		LCD_FB_FORMAT_RGB565 = 0,
		LCD_FB_FORMAT_RGB888 = 1,
		LCD_FB_FORMAT_ARGB8888 = 2,
		LCD_FB_FORMAT_NUM,
	};
#define LCD_FB_FORMAT    enum LCD_FB_FORMAT


	enum LCD_OUTPUT_MODE {
		LCD_OUTPUT_TO_LCM = (1 << 0),
		LCD_OUTPUT_TO_MEM = (1 << 1),
		LCD_OUTPUT_TO_TVROT = (1 << 2),
	};
#define LCD_OUTPUT_MODE    enum LCD_OUTPUT_MODE


	enum LCD_LAYER_ID {
		LCD_LAYER_0 = 0,
		LCD_LAYER_1 = 1,
		LCD_LAYER_2 = 2,
		LCD_LAYER_3 = 3,
		LCD_LAYER_NUM,
		LCD_LAYER_ALL = 0xFFFFFFFF,
	};
#define LCD_LAYER_ID    enum LCD_LAYER_ID

	enum LCD_LAYER_FORMAT {
		LCD_LAYER_FORMAT_RGB888 = 0,
		LCD_LAYER_FORMAT_RGB565 = 1,
		LCD_LAYER_FORMAT_ARGB8888 = 2,
		LCD_LAYER_FORMAT_PARGB8888 = 3,
		LCD_LAYER_FORMAT_xRGB8888 = 4,
		LCD_LAYER_FORMAT_YUYV422 = 8,
		LCD_LAYER_FORMAT_UYVY = 9,
		LCD_LAYER_FORMAT_YVYU = 10,
		LCD_LAYER_FORMAT_VYUY = 11,
		LCD_LAYER_FORMAT_YUV444 = 15,
		LCD_LAYER_FORMAT_NUM,
	};
#define LCD_LAYER_FORMAT    enum LCD_LAYER_FORMAT


	enum LCD_LAYER_ROTATION {
		LCD_LAYER_ROTATE_NONE = 0,
		LCD_LAYER_ROTATE_0 = 0,
		LCD_LAYER_ROTATE_90 = 1,
		LCD_LAYER_ROTATE_180 = 2,
		LCD_LAYER_ROTATE_270 = 3,
		LCD_LAYER_ROTATE_MIRROR_0 = 4,
		LCD_LAYER_ROTATE_MIRROR_90 = 5,
		LCD_LAYER_ROTATE_MIRROR_180 = 6,
		LCD_LAYER_ROTATE_MIRROR_270 = 7,
	};
#define LCD_LAYER_ROTATION    enum LCD_LAYER_ROTATION


	enum LCD_LAYER_TRIGGER_MODE {
		LCD_SW_TRIGGER = 0,
		LCD_HW_TRIGGER_BUFFERING,
		LCD_HW_TRIGGER_DIRECT_COUPLE,
	};
#define LCD_LAYER_TRIGGER_MODE    enum LCD_LAYER_TRIGGER_MODE


	enum LCD_HW_TRIGGER_SRC {
		LCD_HW_TRIGGER_SRC_IRT1 = 0,
		LCD_HW_TRIGGER_SRC_IBW1 = 1,
		LCD_HW_TRIGGER_SRC_IBW2 = 3,
	};
#define LCD_HW_TRIGGER_SRC    enum LCD_HW_TRIGGER_SRC


	enum LCD_TE_MODE {
		LCD_TE_MODE_VSYNC_ONLY = 0,
		LCD_TE_MODE_VSYNC_OR_HSYNC = 1,
	};
#define LCD_TE_MODE    enum LCD_TE_MODE


	enum LCD_TE_VS_WIDTH_CNT_DIV {
		LCD_TE_VS_WIDTH_CNT_DIV_8 = 0,
		LCD_TE_VS_WIDTH_CNT_DIV_16 = 1,
		LCD_TE_VS_WIDTH_CNT_DIV_32 = 2,
		LCD_TE_VS_WIDTH_CNT_DIV_64 = 3,
	};
#define LCD_TE_VS_WIDTH_CNT_DIV    enum LCD_TE_VS_WIDTH_CNT_DIV


	enum LCD_IF_FMT_COLOR_ORDER {
		LCD_IF_FMT_COLOR_ORDER_RGB = 0,
		LCD_IF_FMT_COLOR_ORDER_BGR = 1,
	};
#define LCD_IF_FMT_COLOR_ORDER    enum LCD_IF_FMT_COLOR_ORDER


	enum LCD_IF_FMT_TRANS_SEQ {
		LCD_IF_FMT_TRANS_SEQ_MSB_FIRST = 0,
		LCD_IF_FMT_TRANS_SEQ_LSB_FIRST = 1,
	};
#define LCD_IF_FMT_TRANS_SEQ    enum LCD_IF_FMT_TRANS_SEQ

	enum LCD_IF_FMT_PADDING {
		LCD_IF_FMT_PADDING_ON_LSB = 0,
		LCD_IF_FMT_PADDING_ON_MSB = 1,
	};
#define LCD_IF_FMT_PADDING    enum LCD_IF_FMT_PADDING


	enum LCD_DSI_IF_FMT_COLOR_ORDER {
		LCD_DSI_IF_FMT_COLOR_ORDER_RGB = 0,
		LCD_DSI_IF_FMT_COLOR_ORDER_BGR = 1,
	};
#define LCD_DSI_IF_FMT_COLOR_ORDER    enum LCD_DSI_IF_FMT_COLOR_ORDER


	enum LCD_DSI_IF_FMT_TRANS_SEQ {
		LCD_DSI_IF_FMT_TRANS_SEQ_MSB_FIRST = 0,
		LCD_DSI_IF_FMT_TRANS_SEQ_LSB_FIRST = 1,
	};
#define LCD_DSI_IF_FMT_TRANS_SEQ    enum LCD_DSI_IF_FMT_TRANS_SEQ


	enum LCD_DSI_IF_FMT_PADDING {
		LCD_DSI_IF_FMT_PADDING_ON_LSB = 0,
		LCD_DSI_IF_FMT_PADDING_ON_MSB = 1,
	};
#define LCD_DSI_IF_FMT_PADDING    enum LCD_DSI_IF_FMT_PADDING


	enum LCD_DSI_IF_FORMAT {
		LCD_DSI_IF_FORMAT_RGB565 = 0,
		LCD_DSI_IF_FORMAT_RGB666 = 1,
		LCD_DSI_IF_FORMAT_RGB888 = 2,
	};
#define LCD_DSI_IF_FORMAT    enum LCD_DSI_IF_FORMAT


/* --------------------------------------------------------------------------- */

/* Configurations */
	LCD_STATUS LCD_Init(void);
	LCD_STATUS LCD_Deinit(void);

	LCD_STATUS LCD_Set_DrivingCurrent(LCM_PARAMS *lcm_params);
	LCD_STATUS LCD_PowerOn(void);
	LCD_STATUS LCD_PowerOff(void);
	LCD_STATUS LCD_M4UPowerOn(void);
	LCD_STATUS LCD_M4UPowerOff(void);
	LCD_STATUS LCD_WaitForNotBusy(void);

	LCD_STATUS LCD_EnableInterrupt(DISP_INTERRUPT_EVENTS eventID);
	LCD_STATUS LCD_SetInterruptCallback(void (*pCB) (DISP_INTERRUPT_EVENTS eventID));

/* LCD Controller Interface */
	LCD_STATUS LCD_ConfigParallelIF(LCD_IF_ID id,
					LCD_IF_PARALLEL_BITS ifDataWidth,
					LCD_IF_PARALLEL_CLK_DIV clkDivisor,
					uint32_t writeSetup,
					uint32_t writeHold,
					uint32_t writeWait,
					uint32_t readSetup,
					uint32_t readHold,
					uint32_t readLatency, uint32_t waitPeriod, uint32_t chw);

	LCD_STATUS LCD_ConfigSerialIF(LCD_IF_ID id,
				      LCD_IF_SERIAL_BITS bits,
				      uint32_t three_wire,
				      uint32_t sdi,
				      bool first_pol,
				      bool sck_def,
				      uint32_t div2,
				      uint32_t hw_cs,
				      uint32_t css,
				      uint32_t csh,
				      uint32_t rd_1st, uint32_t rd_2nd, uint32_t wr_1st, uint32_t wr_2nd);


	LCD_STATUS LCD_ConfigIfFormat(LCD_IF_FMT_COLOR_ORDER order,
				      LCD_IF_FMT_TRANS_SEQ transSeq,
				      LCD_IF_FMT_PADDING padding,
				      LCD_IF_FORMAT format, LCD_IF_WIDTH busWidth);

	LCD_STATUS LCD_SetResetSignal(bool high);
	LCD_STATUS LCD_ConfigDSIIfFormat(LCD_DSI_IF_FMT_COLOR_ORDER order,
					 LCD_DSI_IF_FMT_TRANS_SEQ transSeq,
					 LCD_DSI_IF_FMT_PADDING padding,
					 LCD_DSI_IF_FORMAT format, uint32_t packet_size, bool DC_DSI);

/* LCD Command Queue */
	LCD_STATUS LCD_CmdQueueEnable(bool enabled);
	LCD_STATUS LCD_CmdQueueWrite(uint32_t *cmds, uint32_t cmdCount);

/* Layer Configurations */
	LCD_STATUS LCD_LayerEnable(LCD_LAYER_ID id, bool enable);
	bool LCD_IsLayerEnable(LCD_LAYER_ID id);
	LCD_STATUS LCD_LayerSetAddress(LCD_LAYER_ID id, uint32_t address);
	uint32_t LCD_LayerGetAddress(LCD_LAYER_ID id);
	LCD_STATUS LCD_LayerSetSize(LCD_LAYER_ID id, uint32_t width, uint32_t height);

/* 3D Layer Configurations */
	LCD_STATUS LCD_Layer3D_Enable(LCD_LAYER_ID id, bool enable);
	LCD_STATUS LCD_Layer3D_R1st(LCD_LAYER_ID id, bool r_first);
	LCD_STATUS LCD_Layer3D_landscape(LCD_LAYER_ID id, bool landscape);
	LCD_STATUS LCD_LayerSet3D(LCD_LAYER_ID id, bool enable, bool r_first, bool landscape);
	bool LCD_Is3DEnabled(void);
	bool LCD_Is3DLandscapeMode(void);


/* MT6573 added */
	LCD_STATUS LCD_LayerSetPitch(LCD_LAYER_ID id, uint32_t pitch);
	LCD_STATUS LCD_LayerSetOffset(LCD_LAYER_ID id, uint32_t x, uint32_t y);
	LCD_STATUS LCD_LayerSetWindowOffset(LCD_LAYER_ID id, uint32_t x, uint32_t y);
	LCD_STATUS LCD_LayerSetFormat(LCD_LAYER_ID id, unsigned int format);
	LCD_STATUS LCD_LayerEnableTdshp(LCD_LAYER_ID id, uint32_t en);
	LCD_STATUS LCD_LayerSetNextBuffIdx(LCD_LAYER_ID id, int32_t idx);
	LCD_STATUS LCD_LayerGetInfo(LCD_LAYER_ID id, uint32_t *enabled, int32_t *curr_idx,
				    int32_t *next_idx);

	LCD_STATUS LCD_LayerEnableByteSwap(LCD_LAYER_ID id, bool enable);
	LCD_STATUS LCD_LayerSetRotation(LCD_LAYER_ID id, LCD_LAYER_ROTATION rotation);
	LCD_STATUS LCD_LayerSetAlphaBlending(LCD_LAYER_ID id, bool enable, uint8_t alpha);
	LCD_STATUS LCD_LayerSetSourceColorKey(LCD_LAYER_ID id, bool enable, uint32_t colorKey);

/* HW Trigger Configurations */
	LCD_STATUS LCD_LayerSetTriggerMode(LCD_LAYER_ID id, LCD_LAYER_TRIGGER_MODE mode);
	LCD_STATUS LCD_LayerSetHwTriggerSrc(LCD_LAYER_ID id, LCD_HW_TRIGGER_SRC src);
	LCD_STATUS LCD_EnableHwTrigger(bool enable);

/* ROI Window Configurations */
	LCD_STATUS LCD_SetBackgroundColor(uint32_t bgColor);
	LCD_STATUS LCD_SetRoiWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

/* DSI Related Configurations */
	LCD_STATUS LCD_EnableDCtoDsi(bool dc_dsi_en);

/* Output to Memory Configurations */
	LCD_STATUS LCD_SetOutputMode(LCD_OUTPUT_MODE mode);
	LCD_STATUS LCD_WaitDPIIndication(bool enable);
	LCD_STATUS LCD_FBSetFormat(LCD_FB_FORMAT format);
	LCD_STATUS LCD_FBSetPitch(uint32_t pitchInByte);
	LCD_STATUS LCD_FBReset(void);
	LCD_STATUS LCD_FBEnable(LCD_FB_ID id, bool enable);
	LCD_STATUS LCD_FBSetAddress(LCD_FB_ID id, uint32_t address);
	LCD_STATUS LCD_FBSetStartCoord(uint32_t x, uint32_t y);

/* Color Matrix */
	LCD_STATUS LCD_EnableColorMatrix(LCD_IF_ID id, bool enable);
	LCD_STATUS LCD_SetColorMatrix(const unsigned int mat[9]);

/* Tearing Control */
	LCD_STATUS LCD_TE_Enable(bool enable);
	LCD_STATUS LCD_TE_SetMode(LCD_TE_MODE mode);
	LCD_STATUS LCD_TE_SetEdgePolarity(bool polarity);
	LCD_STATUS LCD_TE_ConfigVHSyncMode(uint32_t hsDelayCnt,
					   uint32_t vsWidthCnt,
					   LCD_TE_VS_WIDTH_CNT_DIV vsWidthCntDiv);

/* Operations */
	LCD_STATUS LCD_SelectWriteIF(LCD_IF_ID id);
	LCD_STATUS LCD_WriteIF(LCD_IF_ID id, LCD_IF_A0_MODE a0, uint32_t value,
			       LCD_IF_MCU_WRITE_BITS bits);
	LCD_STATUS LCD_ReadIF(LCD_IF_ID id, LCD_IF_A0_MODE a0, uint32_t *value,
			      LCD_IF_MCU_WRITE_BITS bits);
	LCD_STATUS LCD_StartTransfer(bool blocking, bool isMutexLocked);

	LCD_STATUS LCD_Dump_Layer_Info(void);
	LCD_STATUS LCD_Dynamic_Change_FB_Layer(unsigned int isAEEEnabled);

/* Retrieve Information */
	LCD_STATE LCD_GetState(void);
	LCD_OUTPUT_MODE LCD_GetOutputMode(void);

	LCD_STATUS LCD_ConfigOVL(void);

	bool LCD_IsBusy(void);
/* Debug */
	LCD_STATUS LCD_ConfigDither(int lrs, int lgs, int lbs, int dbr, int dbg, int dbb);
	LCD_STATUS LCD_LayerEnableDither(LCD_LAYER_ID id, uint32_t enable);
	LCD_STATUS LCD_SetOutputAlpha(unsigned int alpha);
	LCD_STATUS LCD_DumpRegisters(void);

	LCD_STATUS LCD_Capture_Framebuffer(unsigned int pvbuf, unsigned int bpp);
	LCD_STATUS LCD_Capture_Layerbuffer(unsigned int layer_id, unsigned int pvbuf,
					   unsigned int bpp);
	LCD_STATUS LCD_Init_IO_pad(LCM_PARAMS *lcm_params);

/* FM De-sense */
	LCD_STATUS LCD_FMDesense_Query(void);
	LCD_STATUS LCD_FM_Desense(LCD_IF_ID id, unsigned long freq);
	LCD_STATUS LCD_Get_Default_WriteCycle(LCD_IF_ID id, unsigned int *write_cycle);
	LCD_STATUS LCD_Get_Current_WriteCycle(LCD_IF_ID id, unsigned int *write_cycle);
	LCD_STATUS LCD_Change_WriteCycle(LCD_IF_ID id, unsigned int write_cycle);
	LCD_STATUS LCD_Reset_WriteCycle(LCD_IF_ID id);

	LCD_STATUS LCD_Get_VideoLayerSize(unsigned int id, unsigned int *width,
					  unsigned int *height);
	LCD_STATUS LCD_Capture_Videobuffer(unsigned int pvbuf, unsigned int bpp,
					   unsigned int video_rotation);

	LCD_STATUS LCD_InitM4U(void);
	LCD_STATUS LCD_AllocUIMva(unsigned int va, unsigned int *mva, unsigned int size);
	LCD_STATUS LCD_AllocOverlayMva(unsigned int va, unsigned int *mva, unsigned int size);
	LCD_STATUS LCD_DeallocMva(unsigned int va, unsigned int mva, unsigned int size);

	LCD_STATUS LCD_W2M_NeedLimiteSpeed(bool enable);
	LCD_STATUS LCD_W2TVR_NeedLimiteSpeed(bool enable);
	LCD_STATUS LCD_SetGMCThrottle(void);


	LCD_STATUS LCD_DumpM4U(void);
	LCD_STATUS LCD_M4U_On(bool enable);

	bool LCD_esd_check(void);
	void LCD_WaitTE(void);
	void LCD_InitVSYNC(unsigned int vsync_interval);
	void LCD_PauseVSYNC(bool enable);
	void LCD_GetVsyncCnt(void);
	void LCD_DumpLayer(void);
/* --------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif				/* __LCD_DRV_H__ */
