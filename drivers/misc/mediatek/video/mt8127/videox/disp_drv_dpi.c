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

#ifdef BUILD_UBOOT
#include <asm/arch/disp_drv_platform.h>
#else

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/semaphore.h>

#include "disp_drv.h"
#include "disp_drv_platform.h"
#include "disp_drv_log.h"

#include "lcd_drv.h"
#include "dpi_drv.h"
#include "dsi_drv.h"

#include "lcm_drv.h"
#include "disp_hal.h"
#endif
/* ------------------------------------------------------- */
/* Private Variables */
/* ------------------------------------------------------- */

static DPI_FB_FORMAT dpiTmpBufFormat = DPI_FB_FORMAT_RGB888;
static LCD_FB_FORMAT lcdTmpBufFormat = LCD_FB_FORMAT_RGB888;
static uint32_t dpiTmpBufBpp;
struct TempBuffer {
	uint32_t pa;
	uint32_t pitchInBytes;
};
#define TempBuffer	struct TempBuffer

static TempBuffer s_tmpBuffers[3];
static bool reinitializing;
/* --------------------------------------------------------------- */
/* Private Functions */
/* --------------------------------------------------------------- */

static inline DPI_FB_FORMAT get_tmp_buffer_dpi_format(void)
{
	switch (lcm_params->dpi.format) {
	case LCM_DPI_FORMAT_RGB565:
		return DPI_FB_FORMAT_RGB565;
	case LCM_DPI_FORMAT_RGB666:
	case LCM_DPI_FORMAT_RGB888:
		return DPI_FB_FORMAT_RGB888;
	default:
		ASSERT(0);
	}
	return DPI_FB_FORMAT_RGB888;
}


static inline uint32_t get_tmp_buffer_bpp(void)
{
	static const uint32_t TO_BPP[] = { 2, 3 };
	return TO_BPP[dpiTmpBufFormat];
}


static inline LCD_FB_FORMAT get_tmp_buffer_lcd_format(void)
{
	static const uint32_t TO_LCD_FORMAT[] = {
		LCD_FB_FORMAT_RGB565,
		LCD_FB_FORMAT_RGB888
	};

	return TO_LCD_FORMAT[dpiTmpBufFormat];
}


static bool disp_drv_dpi_init_context(void)
{
	dpiTmpBufFormat = get_tmp_buffer_dpi_format();
	lcdTmpBufFormat = get_tmp_buffer_lcd_format();
	dpiTmpBufBpp = get_tmp_buffer_bpp();

	if (lcm_drv != NULL && lcm_params != NULL)
		return true;

	pr_info("%s, lcm_drv=0x%08x, lcm_params=0x%08x\n",
		__func__, (unsigned int)lcm_drv, (unsigned int)lcm_params);

	if (NULL == lcm_drv) {
		pr_info("%s, lcm_drv is NULL\n", __func__);

		return false;
	}

	return true;
}

#ifndef MT65XX_NEW_DISP
static void init_intermediate_buffers(uint32_t fbPhysAddr)
{
	uint32_t tmpFbStartPA = fbPhysAddr;

	uint32_t tmpFbPitchInBytes = DISP_GetScreenWidth() * dpiTmpBufBpp;
	uint32_t tmpFbSizeInBytes = tmpFbPitchInBytes * DISP_GetScreenHeight();

	uint32_t i;

	for (i = 0; i < lcm_params->dpi.intermediat_buffer_num; ++i) {
		TempBuffer *b = &s_tmpBuffers[i];

#ifdef BUILD_UBOOT
		/* clean the intermediate buffers
		   as black to prevent from noise display */
		memset(tmpFbStartPA, 0, tmpFbSizeInBytes);
#endif
		b->pitchInBytes = tmpFbPitchInBytes;
		b->pa = tmpFbStartPA;
		/* check if 8-uint8_t-aligned */
		ASSERT((tmpFbStartPA & 0x7) == 0);
		tmpFbStartPA += tmpFbSizeInBytes;
	}

}
#endif

static void init_mipi_pll(void)
{
	DPI_CHECK_RET(DPI_Init_PLL());
}


static void init_io_pad(void)
{
	LCD_CHECK_RET(LCD_Init_IO_pad(lcm_params));

}

static void init_io_driving_current(void)
{
	DPI_CHECK_RET(DPI_Set_DrivingCurrent(lcm_params));
}

static void init_lcd(void)
{
	uint32_t i;

	LCD_CHECK_RET(LCD_LayerEnable(LCD_LAYER_ALL, false));
	LCD_CHECK_RET(LCD_LayerSetTriggerMode(LCD_LAYER_ALL, LCD_SW_TRIGGER));

	LCD_CHECK_RET(LCD_EnableHwTrigger(false));

	LCD_CHECK_RET(LCD_SetBackgroundColor(0));
	LCD_CHECK_RET(LCD_SetRoiWindow(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight()));

	LCD_CHECK_RET(LCD_FBSetFormat(lcdTmpBufFormat));
	LCD_CHECK_RET(LCD_FBSetPitch(s_tmpBuffers[0].pitchInBytes));
	LCD_CHECK_RET(LCD_FBSetStartCoord(0, 0));

	for (i = 0; i < lcm_params->dpi.intermediat_buffer_num; ++i) {
		LCD_CHECK_RET(LCD_FBSetAddress(LCD_FB_0 + i, s_tmpBuffers[i].pa));

		LCD_CHECK_RET(LCD_FBEnable(LCD_FB_0 + i, true));
	}

	LCD_CHECK_RET(LCD_SetOutputMode(LCD_OUTPUT_TO_MEM));
    /**
       "LCD Delay Enable" function should be used when there is only
       single buffer between LCD and DPI.
       Double buffer even triple buffer need not enable it.
    */
	LCD_CHECK_RET(LCD_WaitDPIIndication(true));
}


static void init_dpi(bool isDpiPoweredOn)
{
	const LCM_DPI_PARAMS *dpi = &(lcm_params->dpi);
	uint32_t i;

	if (!reinitializing)
		DPI_CHECK_RET(DPI_Init(isDpiPoweredOn));

	DPI_CHECK_RET(DPI_EnableSeqOutput(false));

	DPI_CHECK_RET(DPI_ConfigPixelClk((DPI_POLARITY) dpi->clk_pol,
					 dpi->dpi_clk_div, dpi->dpi_clk_duty));

	DPI_CHECK_RET(DPI_ConfigDataEnable((DPI_POLARITY) dpi->de_pol));

	DPI_CHECK_RET(DPI_ConfigHsync((DPI_POLARITY) dpi->hsync_pol,
				      dpi->hsync_pulse_width, dpi->hsync_back_porch,
				      dpi->hsync_front_porch));

	DPI_CHECK_RET(DPI_ConfigVsync((DPI_POLARITY) dpi->vsync_pol,
				      dpi->vsync_pulse_width, dpi->vsync_back_porch,
				      dpi->vsync_front_porch));

#ifdef MT65XX_NEW_DISP
	DPI_CHECK_RET(DPI_ConfigLVDS(lcm_params));
#endif

	DPI_CHECK_RET(DPI_FBSetSize(DISP_GetScreenWidth(), DISP_GetScreenHeight()));

	for (i = 0; i < dpi->intermediat_buffer_num; ++i) {
		DPI_CHECK_RET(DPI_FBSetAddress(DPI_FB_0 + i, s_tmpBuffers[i].pa));

		DPI_CHECK_RET(DPI_FBSetPitch(DPI_FB_0 + i, s_tmpBuffers[i].pitchInBytes));

		DPI_CHECK_RET(DPI_FBEnable(DPI_FB_0 + i, true));
	}
	DPI_CHECK_RET(DPI_FBSetFormat(dpiTmpBufFormat));
	DPI_CHECK_RET(DPI_FBSyncFlipWithLCD(true));

	if (LCM_COLOR_ORDER_BGR == dpi->rgb_order)
		DPI_CHECK_RET(DPI_SetRGBOrder(DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_BGR));
	else
		DPI_CHECK_RET(DPI_SetRGBOrder(DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_RGB));


	DPI_CHECK_RET(DPI_EnableClk());
}


/* ------------------------------------------------------------------- */
/* DPI Display Driver Public Functions */
/* ------------------------------------------------------------------- */
static int fbva_stored;
static int fbpa_stored;

static DISP_STATUS dpi_init(uint32_t fbVA, uint32_t fbPA, bool isLcmInited)
{
#if !defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
	struct disp_path_config_struct config = { 0 };
#endif
#if MTK_HDMI_MAIN_PATH
	unsigned disp_width = HDMI_DISP_WIDTH;
	unsigned disp_height = HDMI_DISP_HEIGHT;
#endif

	if (!disp_drv_dpi_init_context())
		return DISP_STATUS_NOT_IMPLEMENTED;

	fbva_stored = fbVA;
	fbpa_stored = fbPA;

	pr_info("[DPI]fbVA=0x%x, fbPA=0x%x\n", fbVA, fbPA);

	init_mipi_pll();
	init_io_pad();
	init_io_driving_current();

	init_lcd();
	init_dpi(isLcmInited);

	if (NULL != lcm_drv->init && !isLcmInited)
		lcm_drv->init();

	DSI_PowerOn();
	DSI_PowerOff();

#ifndef MT65XX_NEW_DISP
	init_intermediate_buffers(fbPA);
#else

#if !defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)

	config.bgROI.x = 0;
	config.bgROI.y = 0;
	config.srcROI.x = 0;
	config.srcROI.y = 0;

#if MTK_HDMI_MAIN_PATH
	config.bgROI.width = disp_width;
	config.bgROI.height = disp_height;
	config.srcROI.height = disp_height;
	config.srcROI.width = disp_width;
	/* background color(alpha,r,g,b)(this is blue) */
	config.bgColor = 0;
#else
	config.bgROI.width = DISP_GetScreenWidth();
	config.bgROI.height = DISP_GetScreenHeight();
	config.srcROI.height = DISP_GetScreenHeight();
	config.srcROI.width = DISP_GetScreenWidth();
	config.bgColor = 0x0;	/* background color */
#endif

	config.ovl_config.source = OVL_LAYER_SOURCE_MEM;
	config.ovl_config.layer = FB_LAYER;
	config.ovl_config.layer_en = 1;
	config.ovl_config.fmt = eRGB565;
	config.ovl_config.addr = fbPA;
	config.ovl_config.source = OVL_LAYER_SOURCE_MEM;
	config.ovl_config.src_x = 0;
	config.ovl_config.src_y = 0;
	config.ovl_config.dst_x = 0;	/* ROI */
	config.ovl_config.dst_y = 0;
	config.ovl_config.keyEn = 0;
	config.ovl_config.key = 0xFF;	/* color key */
	config.ovl_config.aen = 0;	/* alpha enable */
	config.ovl_config.alpha = 0;
	LCD_LayerSetAddress(FB_LAYER, fbPA);
	LCD_LayerSetFormat(FB_LAYER, LCD_LAYER_FORMAT_RGB565);
	LCD_LayerSetOffset(FB_LAYER, 0, 0);
	LCD_LayerEnable(FB_LAYER, true);

#if MTK_HDMI_MAIN_PATH
	config.ovl_config.dst_w = disp_width;
	config.ovl_config.dst_h = disp_height;

	/* pixel number */
	config.ovl_config.src_pitch = ALIGN_TO(disp_width, 32) * 2;
	LCD_LayerSetSize(FB_LAYER, disp_width, disp_height);
	LCD_LayerSetPitch(FB_LAYER, config.ovl_config.src_pitch);
#else
	config.ovl_config.dst_w = DISP_GetScreenWidth();
	config.ovl_config.dst_h = DISP_GetScreenHeight();
	/* pixel number */
	config.ovl_config.src_pitch = ALIGN_TO(DISP_GetScreenWidth(), 32) * 2;
	LCD_LayerSetSize(FB_LAYER, DISP_GetScreenWidth(), DISP_GetScreenHeight());
	LCD_LayerSetPitch(FB_LAYER, ALIGN_TO(DISP_GetScreenWidth(), 32) * 2);
#endif

#if MTK_HDMI_MAIN_PATH
	/*hdmi path ovl->rdma0->color->bls->dpi1->hdmi */
	config.srcModule = DISP_MODULE_OVL;
	config.dstModule = DISP_MODULE_DPI1;	/*  */
#else
	config.srcModule = DISP_MODULE_OVL;
	config.dstModule = DISP_MODULE_DPI0;	/* DISP_MODULE_WDMA1 */
#endif

	config.outFormat = RDMA_OUTPUT_FORMAT_ARGB;

#ifdef CONFIG_MTK_M4U
	/* Because PA = MVA, config port can be at any time */
	{
		M4U_PORT_STRUCT portStruct;
		/* hardware port ID, defined in M4U_PORT_ID_ENUM */
		portStruct.ePortID = DISP_OVL_0;
		portStruct.Virtuality = 1;
		portStruct.Security = 0;
		portStruct.domain = 3;	/* domain : 0 1 2 3 */
		portStruct.Distance = 1;
		portStruct.Direction = 0;
		m4u_config_port(&portStruct);
	}
#endif

	disp_path_get_mutex();
	RDMASetTargetLine(0, DISP_GetScreenHeight() * 4 / 5);
	disp_path_config(&config);
	/* Disable lk boot mode layer. */
	config.ovl_config.layer = FB_LAYER;
	config.ovl_config.layer_en = 0;
	disp_path_config_layer(&config.ovl_config);
	disp_path_release_mutex();
	disp_path_wait_reg_update(0);

#endif
#endif

	return DISP_STATUS_OK;
}

void hdmi_reinit_dpi(void)
{
	pr_info("[DPI]hdmi_reinit_dpi.\n");

	LCD_WaitForNotBusy();
	reinitializing = true;
	dpi_init(fbva_stored, fbpa_stored, 1);
	reinitializing = false;
}

int dpi_get_immediate_buffer_num(void)
{
	return lcm_params->dpi.intermediat_buffer_num;
}

static DISP_STATUS dpi_enable_power(bool enable)
{
	if (enable) {
		DPI_CHECK_RET(DPI_PowerOn());

		/* for MT6573 and later chip, Must re-init mipi pll for dpi,
		   because pll register have located in */
		init_mipi_pll();
		/* MMSYS1 except MT6516 */
		init_io_pad();
		LCD_CHECK_RET(LCD_PowerOn());
		DPI_CHECK_RET(DPI_EnableClk());
		LCD_CHECK_RET(DPI_LVDS_Enable());
		DPI_EnableIrq();
	} else {
		DPI_DisableIrq();
		DPI_CHECK_RET(DPI_DisableClk());
		DPI_CHECK_RET(DPI_PowerOff());
		LCD_CHECK_RET(LCD_PowerOff());
		LCD_CHECK_RET(DPI_LVDS_Disable());
		DPI_mipi_switch(false);
	}
	return DISP_STATUS_OK;
}


static DISP_STATUS dpi_update_screen(bool isMuextLocked)
{
#ifndef MT65XX_NEW_DISP
	LCD_CHECK_RET(LCD_StartTransfer(false));
#else
	if (!isMuextLocked) {
		disp_path_get_mutex();
		LCD_CHECK_RET(LCD_ConfigOVL());
		disp_path_release_mutex();
	}
#endif
	return DISP_STATUS_OK;
}

static uint32_t dpi_get_working_buffer_size(void)
{
	uint32_t size = 0;
	uint32_t framePixels = 0;

	disp_drv_dpi_init_context();

	framePixels = DISP_GetScreenWidth() * DISP_GetScreenHeight();

	size = framePixels * dpiTmpBufBpp * lcm_params->dpi.intermediat_buffer_num;

	return size;
}

static uint32_t dpi_get_working_buffer_bpp(void)
{
	disp_drv_dpi_init_context();

	return dpiTmpBufBpp;
}

static PANEL_COLOR_FORMAT dpi_get_panel_color_format(void)
{
	disp_drv_dpi_init_context();

	switch (lcm_params->dpi.format) {
	case LCM_DPI_FORMAT_RGB565:
		return PANEL_COLOR_FORMAT_RGB565;
	case LCM_DPI_FORMAT_RGB666:
		return PANEL_COLOR_FORMAT_RGB666;
	case LCM_DPI_FORMAT_RGB888:
		return PANEL_COLOR_FORMAT_RGB888;
	default:
		ASSERT(0);
	}
	return PANEL_COLOR_FORMAT_RGB888;
}



DISP_STATUS dpi_capture_framebuffer(uint32_t pvbuf, uint32_t bpp)
{
	LCD_CHECK_RET(DPI_Capture_Framebuffer(pvbuf, bpp));

	return DISP_STATUS_OK;
}

#undef MIN
#define MIN(x, y) ((x) > (y)?(y):(x))
static uint32_t dpi_get_dithering_bpp(void)
{
	return MIN(get_tmp_buffer_bpp() * 8,
		   PANEL_COLOR_FORMAT_TO_BPP(dpi_get_panel_color_format()));
}

const DISP_IF_DRIVER *DISP_GetDriverDPI(void)
{
	static const DISP_IF_DRIVER DPI_DISP_DRV = {
		.init = dpi_init,
		.enable_power = dpi_enable_power,
		.update_screen = dpi_update_screen,

		.get_working_buffer_size = dpi_get_working_buffer_size,
		.get_working_buffer_bpp = dpi_get_working_buffer_bpp,
		.get_panel_color_format = dpi_get_panel_color_format,
		.get_dithering_bpp = dpi_get_dithering_bpp,
		.capture_framebuffer = dpi_capture_framebuffer,
	};

	return &DPI_DISP_DRV;
}
