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

#ifndef __DPI1_DRV_H__
#define __DPI1_DRV_H__

#include "dpi_drv.h"
#include "lcm_drv.h"
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>

#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)
#include "internal_hdmi_drv.h"
#elif defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
#include "inter_mhl_drv.h"
#else
#include "hdmi_drv.h"
#endif
#include "disp_intr.h"
#ifdef __cplusplus
extern "C" {
#endif
/* --------------------------------------------------------------------------- */
#if 0 /* ndef BULID_UBOOT */
extern uint32_t FB_Addr;
#endif

extern void dsi_handle_esd_recovery(void);
/* --------------------------------------------------------------------------- */
#if 0
	typedef enum {
		DPI_OUTPUT_BIT_NUM_8BITS = 0,
		DPI_OUTPUT_BIT_NUM_10BITS = 1,
		DPI_OUTPUT_BIT_NUM_12BITS = 2
	} DPI_OUTPUT_BIT_NUM;

	typedef enum {
		DPI_OUTPUT_CHANNEL_SWAP_RGB = 0,
		DPI_OUTPUT_CHANNEL_SWAP_GBR = 1,
		DPI_OUTPUT_CHANNEL_SWAP_BRG = 2,
		DPI_OUTPUT_CHANNEL_SWAP_RBG = 3,
		DPI_OUTPUT_CHANNEL_SWAP_GRB = 4,
		DPI_OUTPUT_CHANNEL_SWAP_BGR = 5
	} DPI_OUTPUT_CHANNEL_SWAP;

	typedef enum {
		DPI_OUTPUT_YC_MAP_RGB_OR_CrYCb = 0,	/* {R[7:4],G[7:4],B[7:4]} or {Cr[7:4],Y[7:4],Cb[7:4]} */
		DPI_OUTPUT_YC_MAP_CYCY = 4,	/* {C[11:4],Y[11:4],C[3:0],Y[3:0]} */
		DPI_OUTPUT_YC_MAP_YCYC = 5,	/* {Y[11:4],C[11:4],Y[3:0],C[3:0]} */
		DPI_OUTPUT_YC_MAP_CY = 6,	/* {C[11:0],Y[11:0]} */
		DPI_OUTPUT_YC_MAP_YC = 7	/* {Y[11:0],C[11:0]} */
	} DPI_OUTPUT_YC_MAP;
#endif
	typedef enum {
		RGB = 0,
		RGB_FULL,
		YCBCR_444,
		YCBCR_422,
		XV_YCC,
		YCBCR_444_FULL,
		YCBCR_422_FULL
	} COLOR_SPACE_T;

	DPI_STATUS DPI1_Init_PLL(HDMI_VIDEO_RESOLUTION resolution);
	void DPI1_InitRegbase(void);
	DPI_STATUS DPI1_Init(bool isDpiPoweredOn);
	DPI_STATUS DPI1_Deinit(void);

	DPI_STATUS DPI1_Set_DrivingCurrent(LCM_PARAMS *lcm_params);

	DPI_STATUS DPI1_PowerOn(void);
	DPI_STATUS DPI1_PowerOff(void);

	DPI_STATUS DPI1_EnableClk(void);
	DPI_STATUS DPI1_DisableClk(void);

	DPI_STATUS DPI1_EnableSeqOutput(bool enable);
	DPI_STATUS DPI1_SetRGBOrder(DPI_RGB_ORDER input, DPI_RGB_ORDER output);

	DPI_STATUS DPI1_ConfigPixelClk(DPI_POLARITY polarity, uint32_t divisor, uint32_t duty);
	DPI_STATUS DPI1_ConfigDataEnable(DPI_POLARITY polarity);
	DPI_STATUS DPI1_ConfigVsync(DPI_POLARITY polarity,
				    uint32_t pulseWidth, uint32_t backPorch, uint32_t frontPorch);
	DPI_STATUS DPI1_ConfigHsync(DPI_POLARITY polarity,
				    uint32_t pulseWidth, uint32_t backPorch, uint32_t frontPorch);
	DPI_STATUS DPI1_ConfigVsync_LEVEN(uint32_t pulseWidth, uint32_t backPorch, uint32_t frontPorch,
					  bool fgInterlace);
	DPI_STATUS DPI1_ConfigVsync_REVEN(uint32_t pulseWidth, uint32_t backPorch, uint32_t frontPorch,
					  bool fgInterlace);

	DPI_STATUS DPI1_ConfigVsync_RODD(uint32_t pulseWidth, uint32_t backPorch, uint32_t frontPorch);
	DPI_STATUS DPI1_SW_Reset(uint32_t Reset);
	DPI_STATUS DPI1_ColorSpace_Enable(uint8_t ColorSpace);

	DPI_STATUS DPI1_Config_Ctrl(bool fg3DFrame, bool fgInterlace);
	DPI_STATUS DPI1_Config_ColorSpace(uint8_t ColorSpace, uint8_t HDMI_Res);


	DPI_STATUS DPI1_FBSyncFlipWithLCD(bool enable);
	DPI_STATUS DPI1_SetDSIMode(bool enable);
	bool DPI1_IsDSIMode(void);
	DPI_STATUS DPI1_FBSetFormat(DPI_FB_FORMAT format);
	DPI_FB_FORMAT DPI1_FBGetFormat(void);
	DPI_STATUS DPI1_FBSetSize(uint32_t width, uint32_t height);
	DPI_STATUS DPI1_FBEnable(DPI_FB_ID id, bool enable);
	DPI_STATUS DPI1_FBSetAddress(DPI_FB_ID id, uint32_t address);
	DPI_STATUS DPI1_FBSetPitch(DPI_FB_ID id, uint32_t pitchInByte);

	DPI_STATUS DPI1_SetFifoThreshold(uint32_t low, uint32_t high);

/* Debug */
	DPI_STATUS DPI1_DumpRegisters(void);

	DPI_STATUS DPI1_Capture_Framebuffer(unsigned int pvbuf, unsigned int bpp);

/* FM De-sense */
	DPI_STATUS DPI1_FMDesense_Query(void);
	DPI_STATUS DPI1_FM_Desense(unsigned long freq);
	DPI_STATUS DPI1_Get_Default_CLK(unsigned int *clk);
	DPI_STATUS DPI1_Get_Current_CLK(unsigned int *clk);
	DPI_STATUS DPI1_Change_CLK(unsigned int clk);
	DPI_STATUS DPI1_Reset_CLK(void);

	void DPI1_mipi_switch(bool on);
	void DPI1_DisableIrq(void);
	void DPI1_EnableIrq(void);
	DPI_STATUS DPI1_FreeIRQ(void);

	DPI_STATUS DPI1_EnableInterrupt(DISP_INTERRUPT_EVENTS eventID);
	DPI_STATUS DPI1_SetInterruptCallback(void (*pCB) (DISP_INTERRUPT_EVENTS eventID));
	void DPI1_WaitVSYNC(void);
	void DPI1_InitVSYNC(unsigned int vsync_interval);
	void DPI1_PauseVSYNC(bool enable);

	DPI_STATUS DPI1_ConfigHDMI(void);
	DPI_STATUS DPI1_EnableColorBar(void);
	DPI_STATUS DPI1_EnableBlackScreen(void);
	DPI_STATUS DPI1_DisableInternalPattern(void);
	DPI_STATUS DPI1_ESAVVTimingControlLeft(uint16_t offsetOdd, uint16_t widthOdd, uint16_t offsetEven,
					       uint16_t widthEven);
	DPI_STATUS DPI1_ESAVVTimingControlRight(uint16_t offsetOdd, uint16_t widthOdd,
						uint16_t offsetEven, uint16_t widthEven);
	DPI_STATUS DPI1_MatrixCoef(uint16_t c00, uint16_t c01, uint16_t c02, uint16_t c10, uint16_t c11,
				   uint16_t c12, uint16_t c20, uint16_t c21, uint16_t c22);
	DPI_STATUS DPI1_MatrixPreOffset(uint16_t preAdd0, uint16_t preAdd1, uint16_t preAdd2);
	DPI_STATUS DPI1_MatrixPostOffset(uint16_t postAdd0, uint16_t postAdd1, uint16_t postAdd2);
	DPI_STATUS DPI1_SetChannelLimit(uint16_t yBottom, uint16_t yTop, uint16_t cBottom, uint16_t cTop);
	DPI_STATUS DPI1_CLPFSetting(uint8_t clpfType, bool roundingEnable);
	DPI_STATUS DPI1_EmbeddedSyncSetting(bool embSync_R_Cr, bool embSync_G_Y, bool embSync_B_Cb,
					    bool esavFInv, bool esavVInv, bool esavHInv,
					    bool esavCodeMan);
	DPI_STATUS DPI1_OutputSetting(unsigned int outBitNum, bool outBitSwap,
				      unsigned int outChSwap, unsigned int outYCMap);
	/*DPI_STATUS DPI1_OutputSetting(DPI_OUTPUT__BIT_NUM outBitNum, BOOL outBitSwap,
	   DPI_OUTPUT_CHANNEL__SWAP outChSwap,
	   DPI_OUT_PUT_YC_MAP outYCMap); */
	bool DPI1_IS_TOP_FIELD(void);

/* --------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif				/* __DPI1_DRV_H__ */
