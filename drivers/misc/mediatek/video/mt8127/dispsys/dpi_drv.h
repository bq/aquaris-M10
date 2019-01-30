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

#ifndef __DPI_DRV_H__
#define __DPI_DRV_H__

#include "lcm_drv.h"
#include "disp_intr.h"
#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------- */

#define DPI_CHECK_RET(expr)             \
	do {                                \
		DPI_STATUS ret = (expr);        \
		ASSERT(DPI_STATUS_OK == ret);   \
	} while (0)

/* --------------------------------------------------------------------------- */

	enum DPI_STATUS {
		DPI_STATUS_OK = 0,

		DPI_STATUS_ERROR,
	};
#define DPI_STATUS    enum DPI_STATUS

	enum DPI_FB_FORMAT {
		DPI_FB_FORMAT_RGB565 = 0,
		DPI_FB_FORMAT_RGB888 = 1,
		DPI_FB_FORMAT_XRGB888 = 1,
		DPI_FB_FORMAT_RGBX888 = 1,
		DPI_FB_FORMAT_NUM,
	};
#define DPI_FB_FORMAT    enum DPI_FB_FORMAT

	enum DPI_RGB_ORDER {
		DPI_RGB_ORDER_RGB = 0,
		DPI_RGB_ORDER_BGR = 1,
	};
#define DPI_RGB_ORDER    enum DPI_RGB_ORDER


	enum DPI_FB_ID {
		DPI_FB_0 = 0,
		DPI_FB_1 = 1,
		DPI_FB_2 = 2,
		DPI_FB_NUM,
	};
#define DPI_FB_ID    enum DPI_FB_ID


	enum DPI_POLARITY {
		DPI_POLARITY_RISING = 0,
		DPI_POLARITY_FALLING = 1
	};
#define DPI_POLARITY    enum DPI_POLARITY



/*
typedef enum
{
    DPI_OUTPUT_BIT_NUM_8BITS = 0,
    DPI_OUTPUT_BIT_NUM_10BITS = 1,
    DPI_OUTPUT_BIT_NUM_12BITS = 2
} DPI_OUTPUT_BIT_NUM;

typedef enum
{
    DPI_OUTPUT_CHANNEL_SWAP_RGB = 0,
    DPI_OUTPUT_CHANNEL_SWAP_GBR = 1,
    DPI_OUTPUT_CHANNEL_SWAP_BRG = 2,
    DPI_OUTPUT_CHANNEL_SWAP_RBG = 3,
    DPI_OUTPUT_CHANNEL_SWAP_GRB = 4,
    DPI_OUTPUT_CHANNEL_SWAP_BGR = 5
} DPI_OUTPUT_CHANNEL_SWAP;

typedef enum
{
    DPI_OUTPUT_YC_MAP_RGB_OR_CrYCb = 0, // {R[7:4],G[7:4],B[7:4]} or {Cr[7:4],Y[7:4],Cb[7:4]}
    DPI_OUTPUT_YC_MAP_CYCY         = 4, // {C[11:4],Y[11:4],C[3:0],Y[3:0]}
    DPI_OUTPUT_YC_MAP_YCYC         = 5, // {Y[11:4],C[11:4],Y[3:0],C[3:0]}
    DPI_OUTPUT_YC_MAP_CY           = 6, // {C[11:0],Y[11:0]}
    DPI_OUTPUT_YC_MAP_YC           = 7  // {Y[11:0],C[11:0]}
} DPI_OUTPUT_YC_MAP;

typedef  enum
{
   RGB = 0,
   RGB_FULL,
   YCBCR_444,
   YCBCR_422,
   XV_YCC,
   YCBCR_444_FULL,
   YCBCR_422_FULL

} COLOR_SPACE_T;
*/

	enum DPI0_OUTPUT_CHANNEL_SWAP {
		DPI0_OUTPUT_CHANNEL_SWAP_RGB = 0,
		DPI0_OUTPUT_CHANNEL_SWAP_GBR = 1,
		DPI0_OUTPUT_CHANNEL_SWAP_BRG = 2,
		DPI0_OUTPUT_CHANNEL_SWAP_RBG = 3,
		DPI0_OUTPUT_CHANNEL_SWAP_GRB = 4,
		DPI0_OUTPUT_CHANNEL_SWAP_BGR = 5
	};

	enum DPI0_INPUT_COLOR_FORMAT {
		DPI0_INPUT_YUV = 0,
		DPI0_INPUT_RGB = 1,
	};

	enum DPI0_OUTPUT_COLOR_FORMAT {
		DPI0_OUTPUT_YUV = 0,
		DPI0_OUTPUT_RGB = 1,
		DPI0_OUTPUT_GBR = 2,
		DPI0_OUTPUT_BRG = 3,
		DPI0_OUTPUT_RBG = 4,
		DPI0_OUTPUT_GRB = 5,
		DPI0_OUTPUT_BGR = 6
	};

/* --------------------------------------------------------------------------- */

	extern LCM_PARAMS *lcm_params;

	extern LCM_DRIVER *lcm_drv;

	extern void dsi_handle_esd_recovery(void);
#ifndef BULID_UBOOT
	extern uint32_t FB_Addr;
#endif

	void DPI_InitRegbase(void);
	DPI_STATUS DPI_Init(bool isDpiPoweredOn);
	DPI_STATUS DPI_Deinit(void);

	DPI_STATUS DPI_Init_PLL(void);
	DPI_STATUS DPI_Set_DrivingCurrent(LCM_PARAMS *lcm_params);

	DPI_STATUS DPI_PowerOn(void);
	DPI_STATUS DPI_PowerOff(void);

	DPI_STATUS DPI_EnableClk(void);
	DPI_STATUS DPI_DisableClk(void);

	DPI_STATUS DPI_EnableSeqOutput(bool enable);
	DPI_STATUS DPI_SetRGBOrder(DPI_RGB_ORDER input, DPI_RGB_ORDER output);

	DPI_STATUS DPI_ConfigPixelClk(DPI_POLARITY polarity, uint32_t divisor, uint32_t duty);
	DPI_STATUS DPI_ConfigDataEnable(DPI_POLARITY polarity);
	DPI_STATUS DPI_ConfigVsync(DPI_POLARITY polarity,
				   uint32_t pulseWidth, uint32_t backPorch, uint32_t frontPorch);
	DPI_STATUS DPI_ConfigHsync(DPI_POLARITY polarity,
				   uint32_t pulseWidth, uint32_t backPorch, uint32_t frontPorch);

	DPI_STATUS DPI_FBSyncFlipWithLCD(bool enable);
	DPI_STATUS DPI_SetDSIMode(bool enable);
	bool DPI_IsDSIMode(void);
	DPI_STATUS DPI_FBSetFormat(DPI_FB_FORMAT format);
	DPI_FB_FORMAT DPI_FBGetFormat(void);
	DPI_STATUS DPI_FBSetSize(uint32_t width, uint32_t height);
	DPI_STATUS DPI_FBEnable(DPI_FB_ID id, bool enable);
	DPI_STATUS DPI_FBSetAddress(DPI_FB_ID id, uint32_t address);
	DPI_STATUS DPI_FBSetPitch(DPI_FB_ID id, uint32_t pitchInByte);

	DPI_STATUS DPI_SetFifoThreshold(uint32_t low, uint32_t high);

/* Debug */
	DPI_STATUS DPI_DumpRegisters(void);

	DPI_STATUS DPI_Capture_Framebuffer(unsigned int pvbuf, unsigned int bpp);

/* FM De-sense */
	DPI_STATUS DPI_FMDesense_Query(void);
	DPI_STATUS DPI_FM_Desense(unsigned long freq);
	DPI_STATUS DPI_Get_Default_CLK(unsigned int *clk);
	DPI_STATUS DPI_Get_Current_CLK(unsigned int *clk);
	DPI_STATUS DPI_Change_CLK(unsigned int clk);
	DPI_STATUS DPI_Reset_CLK(void);

	void DPI_mipi_switch(bool on);
	void DPI_DisableIrq(void);
	void DPI_EnableIrq(void);
	DPI_STATUS DPI_FreeIRQ(void);

	DPI_STATUS DPI_EnableInterrupt(DISP_INTERRUPT_EVENTS eventID);
	DPI_STATUS DPI_SetInterruptCallback(void (*pCB) (DISP_INTERRUPT_EVENTS eventID));
	void DPI_WaitVSYNC(void);
	void DPI_InitVSYNC(unsigned int vsync_interval);
	void DPI_PauseVSYNC(bool enable);

	DPI_STATUS DPI_ConfigLVDS(LCM_PARAMS *lcm_params);
	DPI_STATUS DPI_LVDS_Enable(void);
	DPI_STATUS DPI_LVDS_Disable(void);

	DPI_STATUS DPI_ConfigHDMI(void);

	unsigned int DPI_Check_LCM(void);

	DPI_STATUS DPI0_ConfigColorFormat(enum DPI0_INPUT_COLOR_FORMAT input_fmt,
					  enum DPI0_OUTPUT_COLOR_FORMAT output_fmt);

/* --------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif				/* __DPI_DRV_H__ */
