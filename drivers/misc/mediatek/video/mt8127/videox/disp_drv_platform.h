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

#ifndef __DISP_DRV_PLATFORM_H__
#define __DISP_DRV_PLATFORM_H__

#include <linux/dma-mapping.h>
#include <linux/types.h>
#include <linux/clk.h>
#include "disp_assert_layer.h"
#include "ddp_hal.h"
#include "ddp_drv.h"
#include "ddp_path.h"
#include "ddp_rdma.h"
#include "dsi_drv.h"
#include "disp_drv_log.h"
#include "ddp_reg.h"
#include "mt-plat/mt_smi.h"

#ifdef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
#include "disp_ovl_engine_api.h"
#include "disp_ovl_engine_hw.h"
#include "disp_ovl_engine_core.h"
#endif

#ifdef CONFIG_MTK_M4U
#include <m4u.h>
#include <m4u_port.h>
#endif

#ifdef CONFIG_MTK_IOMMU
#include <soc/mediatek/smi.h>
#include <asm/dma-iommu.h>
#endif

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/of_platform.h>
#endif

#ifdef CONFIG_MTK_ION
#include <ion_drv.h>
#endif

#ifdef CONFIG_SYNC
#include <../drivers/staging/android/sw_sync.h>
#endif

#if 0 /* def CONFIG_ANDROID */
#include <mach/leds-mt65xx.h>
/* #include <mach/xlog.h> */
#endif

#define MTK_LCD_HW_SIF_VERSION      2
#define MTKFB_NO_M4U
#define MT65XX_NEW_DISP
#define MTK_FB_ALIGNMENT 32
#define MTK_FB_SYNC_SUPPORT
#define MTK_OVL_DECOUPLE_SUPPORT

#define MTK_HDMI_MAIN_PATH 0
#define MTK_CVBS_SUB_PATH_SUPPORT 0

#define MTK_HDMI_MAIN_PATH_TEST 0
#define MTK_HDMI_MAIN_PATH_TEST_SIZE 0
#define MTK_DISABLE_HDMI_BUFFER_FROM_RDMA1 1


#define MTK_CVBS_FORMAT_YUV 1


#if MTK_CVBS_FORMAT_YUV
/* YUV422 */
/* #define RDMA1_INTPUT_FORMAT eYUYV */

#define RDMA1_INTPUT_FORMAT eUYVY
#define RDMA1_SRC_FORMAT MTK_FB_FORMAT_YUV422
#define RDMA1_OUTPUT_FORMAT RDMA_OUTPUT_FORMAT_YUV444
#define RDMA1_SRC_BPP 2
#define TVE_DISPIF_FORMAT DISPIF_FORMAT_YUV422
#define TVE_DISPIF_TYPE DISPIF_TYPE_CVBS
#define WDMA_INPUT_COLOR_FORMAT  WDMA_INPUT_FORMAT_YUV444
#else
 /* RGB888 */
#define RDMA1_INTPUT_FORMAT eRGB888
#define RDMA1_SRC_FORMAT MTK_FB_FORMAT_RGB888
#define RDMA1_OUTPUT_FORMAT RDMA_OUTPUT_FORMAT_ARGB
#define RDMA1_SRC_BPP 3
#define TVE_DISPIF_FORMAT DISPIF_FORMAT_RGB888
#define TVE_DISPIF_TYPE HDMI
#define WDMA_INPUT_COLOR_FORMAT  WDMA_INPUT_FORMAT_ARGB
#endif



#define TVE_DISP_WIDTH 720
#define TVE_DISP_HEIGHT 576
#define RDMA1_FRAME_BUFFER_NUM 3
#define HDMI_DISP_WIDTH 1920
#define HDMI_DISP_HEIGHT 1080
#define HDMI_DEFAULT_RESOLUTION HDMI_VIDEO_1920x1080p_60Hz

extern unsigned char *disp_module_name[];

#define DDP_USE_CLOCK_API

#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))

#endif				/* __DISP_DRV_PLATFORM_H__ */
