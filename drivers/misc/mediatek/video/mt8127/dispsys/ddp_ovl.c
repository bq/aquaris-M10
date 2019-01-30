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
#include <asm/io.h>
#include <linux/types.h>

#include "ddp_reg.h"
#include "ddp_ovl.h"
#include "ddp_matrix_para.h"
#include "ddp_hal.h"
#include "ddp_drv.h"
#include "ddp_debug.h"

/*for MTK_HDMI_MAIN_PATH*/
#include "disp_drv_platform.h"
#if MTK_HDMI_MAIN_PATH
#include "hdmitx.h"
#endif

enum OVL_COLOR_SPACE {
	OVL_COLOR_SPACE_RGB = 0,
	OVL_COLOR_SPACE_YUV,
};

#define OVL_COLOR_BASE 30
enum OVL_INPUT_FORMAT {
	OVL_INPUT_FORMAT_RGB888 = 0,
	OVL_INPUT_FORMAT_RGB565 = 1,
	OVL_INPUT_FORMAT_ARGB8888 = 2,
	OVL_INPUT_FORMAT_PARGB8888 = 3,
	OVL_INPUT_FORMAT_xRGB8888 = 4,
	OVL_INPUT_FORMAT_YUYV = 8,
	OVL_INPUT_FORMAT_UYVY = 9,
	OVL_INPUT_FORMAT_YVYU = 10,
	OVL_INPUT_FORMAT_VYUY = 11,
	OVL_INPUT_FORMAT_YUV444 = 15,
	OVL_INPUT_FORMAT_UNKNOWN = 16,

	OVL_INPUT_FORMAT_ABGR8888 = OVL_INPUT_FORMAT_ARGB8888 + OVL_COLOR_BASE,
	OVL_INPUT_FORMAT_BGR888 = OVL_INPUT_FORMAT_RGB888 + OVL_COLOR_BASE,
	OVL_INPUT_FORMAT_BGR565 = OVL_INPUT_FORMAT_RGB565 + OVL_COLOR_BASE,
	OVL_INPUT_FORMAT_PABGR8888 = OVL_INPUT_FORMAT_PARGB8888 + OVL_COLOR_BASE,
	OVL_INPUT_FORMAT_xBGR8888 = OVL_INPUT_FORMAT_xRGB8888 + OVL_COLOR_BASE,
};

enum OVL_INPUT_FORMAT ovl_fmt_convert(DpColorFormat fmt)
{
	enum OVL_INPUT_FORMAT ovl_fmt = OVL_INPUT_FORMAT_UNKNOWN;

	/* pr_notice("BOOT ovl_fmt_convert 0 fmt=%d, ovl_fmt=%d\n", fmt, ovl_fmt); */
	switch (fmt) {
	case eYUY2:
		ovl_fmt = OVL_INPUT_FORMAT_YUYV;
		break;
	case eUYVY:
		ovl_fmt = OVL_INPUT_FORMAT_UYVY;
		break;
	case eYVYU:
		ovl_fmt = OVL_INPUT_FORMAT_YVYU;
		break;
	case eVYUY:
		ovl_fmt = OVL_INPUT_FORMAT_VYUY;
		break;
		/* case eYUV_444_1P       : */
		/* ovl_fmt = OVL_INPUT_FORMAT_YUV444; break; */
	case eRGB565:
		ovl_fmt = OVL_INPUT_FORMAT_RGB565;
		break;
	case eRGB888:
		ovl_fmt = OVL_INPUT_FORMAT_RGB888;
		break;
	case eBGR888:
		ovl_fmt = OVL_INPUT_FORMAT_BGR888;
		break;
	case eARGB8888:
		ovl_fmt = OVL_INPUT_FORMAT_ARGB8888;
		break;
	case eABGR8888:
		ovl_fmt = OVL_INPUT_FORMAT_ABGR8888;
		break;
	case ePARGB8888:
		ovl_fmt = OVL_INPUT_FORMAT_PARGB8888;
		break;
	case ePABGR8888:
		ovl_fmt = OVL_INPUT_FORMAT_PABGR8888;
		break;
	case eXARGB8888:
		ovl_fmt = OVL_INPUT_FORMAT_xRGB8888;
		break;

	default:
		DISP_ERR("error: DDP, unknown ovl input format = %d\n", fmt);
	}

	/* pr_notice("BOOT ovl_fmt_convert 1 fmt=%d, ovl_fmt=%d\n", fmt, ovl_fmt); */

	return ovl_fmt;
}

int OVLStart(void)
{

	DISP_REG_SET(DISP_REG_OVL_INTEN, 0x0f);
	DISP_REG_SET(DISP_REG_OVL_EN, 0x01);

	DISP_REG_SET(DISP_REG_OVL_RDMA0_FIFO_CTRL, 0x400000);
	DISP_REG_SET(DISP_REG_OVL_RDMA1_FIFO_CTRL, 0x400000);
	DISP_REG_SET(DISP_REG_OVL_RDMA2_FIFO_CTRL, 0x400000);
	DISP_REG_SET(DISP_REG_OVL_RDMA3_FIFO_CTRL, 0x400000);

	return 0;
}

int OVLStop(void)
{
	DISP_REG_SET(DISP_REG_OVL_INTEN, 0x00);
	DISP_REG_SET(DISP_REG_OVL_EN, 0x00);
	DISP_REG_SET(DISP_REG_OVL_INTSTA, 0x00);

	return 0;
}

int OVLReset(void)
{

	unsigned int delay_cnt = 0;
	static unsigned int cnt;

	DISP_DDP("[DDP] OVLReset called %d\n", cnt++);

	DISP_REG_SET(DISP_REG_OVL_RST, 0x1);	/* soft reset */
	DISP_REG_SET(DISP_REG_OVL_RST, 0x0);
	while (((DISP_REG_GET(DISP_REG_OVL_FLOW_CTRL_DBG) & 0x3ff) != 0x1) &&
	       ((DISP_REG_GET(DISP_REG_OVL_FLOW_CTRL_DBG) & 0x3ff) != 0x2)) {
		delay_cnt++;
		if (delay_cnt > 10000) {
			DISP_ERR("error, OVLReset() timeout!\n");
			ddp_dump_info(DISP_MODULE_CONFIG);
			ddp_dump_info(DISP_MODULE_MUTEX);
			ddp_dump_info(DISP_MODULE_OVL);
			ddp_dump_info(DISP_MODULE_RDMA0);
			disp_dump_reg(DISP_MODULE_OVL);
			smi_dumpDebugMsg();
			break;
		}
	}

#if 0
	DISP_REG_SET(DISP_REG_OVL_ROI_SIZE, 0x00);	/* clear regs */
	DISP_REG_SET(DISP_REG_OVL_ROI_BGCLR, 0x00);
	DISP_REG_SET(DISP_REG_OVL_SRC_CON, 0x00);

	DISP_REG_SET(DISP_REG_OVL_L0_CON, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L0_SRCKEY, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L0_SRC_SIZE, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L0_OFFSET, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L0_ADDR, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L0_PITCH, 0x00);

	DISP_REG_SET(DISP_REG_OVL_L1_CON, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L1_SRCKEY, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L1_SRC_SIZE, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L1_OFFSET, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L1_ADDR, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L1_PITCH, 0x00);

	DISP_REG_SET(DISP_REG_OVL_L2_CON, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L2_SRCKEY, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L2_SRC_SIZE, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L2_OFFSET, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L2_ADDR, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L2_PITCH, 0x00);

	DISP_REG_SET(DISP_REG_OVL_L3_CON, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L3_SRCKEY, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L3_SRC_SIZE, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L3_OFFSET, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L3_ADDR, 0x00);
	DISP_REG_SET(DISP_REG_OVL_L3_PITCH, 0x00);
#endif

	return 0;
}

int OVLROI(unsigned int bgW, unsigned int bgH, unsigned int bgColor)
{
	if ((bgW > OVL_MAX_WIDTH) || (bgH > OVL_MAX_HEIGHT)) {
		pr_notice("error: OVLROI(), exceed OVL max size, w=%d, h=%d\n", bgW, bgH);
		BUG_ON((bgW > OVL_MAX_WIDTH) || (bgH > OVL_MAX_HEIGHT));
	}

	DISP_REG_SET(DISP_REG_OVL_ROI_SIZE, bgH << 16 | bgW);
	DISP_REG_SET(DISP_REG_OVL_ROI_BGCLR, bgColor);

	return 0;
}

int OVLLayerSizeConfig(unsigned int layer, unsigned int dst_w,	/* ROT width */
		       unsigned int dst_h)
{
	DISP_REG_SET(DISP_REG_OVL_L0_SRC_SIZE + layer * 0x20, dst_h << 16 | dst_w);

	return 0;
}

int OVLLayerSrcPitch(unsigned int layer, unsigned int src_pitch)
{
	DISP_MSG("set layer pitch %d\n", src_pitch);
	DISP_REG_SET_FIELD(L0_PITCH_FLD_L0_SRC_PITCH, DISP_REG_OVL_L0_PITCH + layer * 0x20,
			   src_pitch);
	DISP_MSG("get layer pitch 0x%x 0x%x\n", (DISP_REG_OVL_L0_PITCH + layer * 0x20),
		 DISP_REG_GET(DISP_REG_OVL_L0_PITCH + layer * 0x20));

	return 0;
}

int OVLLayerSwitch(unsigned layer, bool en)
{
	ASSERT(layer <= 3);

	DISP_DDP("OVL: %s layer%d\n", en?"enable":"disable", layer);

	switch (layer) {
	case 0:
		DISP_REG_SET_FIELD(SRC_CON_FLD_L0_EN, DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(DISP_REG_OVL_RDMA0_CTRL, 0);
		break;
	case 1:
		DISP_REG_SET_FIELD(SRC_CON_FLD_L1_EN, DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(DISP_REG_OVL_RDMA1_CTRL, 0);
		break;
	case 2:
		DISP_REG_SET_FIELD(SRC_CON_FLD_L2_EN, DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(DISP_REG_OVL_RDMA2_CTRL, 0);
		break;
	case 3:
		DISP_REG_SET_FIELD(SRC_CON_FLD_L3_EN, DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(DISP_REG_OVL_RDMA3_CTRL, 0);
		break;
	default:
		DISP_ERR("error: invalid layer=%d\n", layer);	/* invalid layer */
		ASSERT(0);
	}

	return 0;
}

int OVL3DConfig(unsigned int layer_id,
		unsigned int en_3d, unsigned int landscape, unsigned int r_first)
{
	ASSERT(layer_id <= 3);

	switch (layer_id) {
	case 0:
		DISP_REG_SET_FIELD(L0_CON_FLD_EN_3D, DISP_REG_OVL_L0_CON, en_3d);
		DISP_REG_SET_FIELD(L0_CON_FLD_LANDSCAPE, DISP_REG_OVL_L0_CON, landscape);
		DISP_REG_SET_FIELD(L0_CON_FLD_R_FIRST, DISP_REG_OVL_L0_CON, r_first);
		break;
	case 1:
		DISP_REG_SET_FIELD(L1_CON_FLD_EN_3D, DISP_REG_OVL_L1_CON, en_3d);
		DISP_REG_SET_FIELD(L1_CON_FLD_LANDSCAPE, DISP_REG_OVL_L1_CON, landscape);
		DISP_REG_SET_FIELD(L1_CON_FLD_R_FIRST, DISP_REG_OVL_L1_CON, r_first);
		break;
	case 2:
		DISP_REG_SET_FIELD(L2_CON_FLD_EN_3D, DISP_REG_OVL_L2_CON, en_3d);
		DISP_REG_SET_FIELD(L2_CON_FLD_LANDSCAPE, DISP_REG_OVL_L2_CON, landscape);
		DISP_REG_SET_FIELD(L2_CON_FLD_R_FIRST, DISP_REG_OVL_L2_CON, r_first);
		break;
	case 3:
		DISP_REG_SET_FIELD(L3_CON_FLD_EN_3D, DISP_REG_OVL_L3_CON, en_3d);
		DISP_REG_SET_FIELD(L3_CON_FLD_LANDSCAPE, DISP_REG_OVL_L3_CON, landscape);
		DISP_REG_SET_FIELD(L3_CON_FLD_R_FIRST, DISP_REG_OVL_L3_CON, r_first);
		break;
	default:
		DISP_ERR("error: OVL3DConfig(), invalid layer=%d\n", layer_id);	/* invalid layer */
		ASSERT(0);
	}

	return 0;
}

int OVLLayerConfig(unsigned int layer, unsigned int source, DpColorFormat format,
		unsigned int addr,
		unsigned int src_x,
		unsigned int src_y,
		unsigned int src_pitch,
		unsigned int dst_x,
		unsigned int dst_y,
		unsigned int dst_w,
		unsigned int dst_h,
		bool keyEn,
		unsigned int key,
		bool aen,
		unsigned char alpha)
{

	unsigned bpp;
	unsigned input_color_space;
	unsigned matrix_transform;
	unsigned mode = (TABLE_NO + 1);	/* 0xdeaddead; yuv to rgb conversion required */
	unsigned int rgb_swap = 0;
	unsigned int address, start, end;
	int width;
	enum OVL_INPUT_FORMAT fmt;

	unsigned int layer_pitch = src_pitch;
	unsigned int dst_height = dst_h;

	DISP_DDP
	    ("layer%d/source%d/fmt0x%x/addr0x%x/src(%d,%d,%d)/dst(%d,%d,%d,%d)/key(%d,0x%x)/alpha(%d,%d)\n",
	     layer, source, format, addr, src_x, src_y, src_pitch,
	     dst_x, dst_y, dst_w, dst_h, keyEn, key, aen, alpha);

	ASSERT((dst_w <= OVL_MAX_WIDTH) && (dst_h <= OVL_MAX_HEIGHT));
	fmt = ovl_fmt_convert(format);


#if MTK_HDMI_MAIN_PATH
	{
		HDMI_VIDEO_RESOLUTION res = hdmi_get_resolution();

		if (res == HDMI_VIDEO_1920x1080i_60Hz || res == HDMI_VIDEO_1920x1080i_50Hz) {
			layer_pitch *= 2;
			dst_height /= 2;
		}
		/* DISP_MSG("[%d] hdmi res %d pitch 0x%x(0x%x) height 0x%x\n",
		   layer,res,layer_pitch,src_pitch,dst_height,dst_h);S */
	}
#endif

	if ((layer == 3) && (isAEEEnabled == 1) && (addr != dal_fb_pa))
		pr_notice("[DDP], config AEE layer! addr=0x%x\n", addr);

	if (fmt == OVL_INPUT_FORMAT_ABGR8888 ||
	    fmt == OVL_INPUT_FORMAT_PABGR8888 ||
	    fmt == OVL_INPUT_FORMAT_xBGR8888 ||
	    fmt == OVL_INPUT_FORMAT_BGR888 || fmt == OVL_INPUT_FORMAT_BGR565) {
		rgb_swap = 1;
		fmt -= OVL_COLOR_BASE;
	} else
		rgb_swap = 0;

	switch (fmt) {
	case OVL_INPUT_FORMAT_ARGB8888:
	case OVL_INPUT_FORMAT_PARGB8888:
	case OVL_INPUT_FORMAT_xRGB8888:
		bpp = 4;
		break;
	case OVL_INPUT_FORMAT_RGB888:
	case OVL_INPUT_FORMAT_YUV444:
		bpp = 3;
		break;
	case OVL_INPUT_FORMAT_RGB565:
	case OVL_INPUT_FORMAT_YUYV:
	case OVL_INPUT_FORMAT_UYVY:
	case OVL_INPUT_FORMAT_YVYU:
	case OVL_INPUT_FORMAT_VYUY:
		bpp = 2;
		break;
	default:
		pr_notice("DDP error, OVLLayerConfig unknown fmt , fmt=%d\n", fmt);
		ASSERT(0);	/* invalid input format */
	}

	if ((source == OVL_LAYER_SOURCE_SCL || source == OVL_LAYER_SOURCE_PQ) &&
	    (fmt != OVL_INPUT_FORMAT_YUV444)) {
		pr_notice("error: direct link to OVL only support YUV444!\n");
		ASSERT(0);	/* direct link support YUV444 only */
	}

	switch (fmt) {
	case OVL_INPUT_FORMAT_ARGB8888:
	case OVL_INPUT_FORMAT_PARGB8888:
	case OVL_INPUT_FORMAT_xRGB8888:
	case OVL_INPUT_FORMAT_RGB888:
	case OVL_INPUT_FORMAT_RGB565:
		input_color_space = OVL_COLOR_SPACE_RGB;
		break;
	case OVL_INPUT_FORMAT_YUV444:
	case OVL_INPUT_FORMAT_YUYV:
	case OVL_INPUT_FORMAT_UYVY:
	case OVL_INPUT_FORMAT_YVYU:
	case OVL_INPUT_FORMAT_VYUY:
		input_color_space = OVL_COLOR_SPACE_YUV;
		break;
	default:
		pr_notice("DDP error, OVLLayerConfig unknown fmt , fmt=%d\n", fmt);
		ASSERT(0);	/* invalid input format */
	}

	switch (fmt) {
	case OVL_INPUT_FORMAT_ARGB8888:
	case OVL_INPUT_FORMAT_PARGB8888:
	case OVL_INPUT_FORMAT_xRGB8888:
	case OVL_INPUT_FORMAT_RGB888:
	case OVL_INPUT_FORMAT_RGB565:
		matrix_transform = 0;
		break;
	case OVL_INPUT_FORMAT_YUV444:
	case OVL_INPUT_FORMAT_YUYV:
	case OVL_INPUT_FORMAT_UYVY:
	case OVL_INPUT_FORMAT_YVYU:
	case OVL_INPUT_FORMAT_VYUY:
		matrix_transform = 0x6;
		break;
	default:
		ASSERT(0);	/* invalid input format */
	}

/*
    if (OVL_COLOR_SPACE_YUV == input_color_space)
	mode = YUV2RGB_601_0_0;
*/
	/* -------------------------------------------------------- */
	/* calculate OVL address alignment for width boundary burst */
	/* 64 uint8_t align: */
	/* start address(first uint8_t)[6:4] <= 3'd4 */
	/* end address(last uint8_t)[6:4] >= 0x3'd4 */
	/* 128 uint8_t align: */
	/* start address(first uint8_t)[6:4] = 3'b0 */
	/* end address(last uint8_t)[6:4] = 3'd7 */
	/* -------------------------------------------------------- */
	address = addr + src_x * bpp + src_y * layer_pitch;	/* First uint8_t of first pixel */
	width = dst_w;
	start = address;
	end = start + bpp * width - 1;	/* Last uint8_t of last pixel of first lline */
#if 0
	if ((width != 0) && (((start & 0x70) > 0x40) || ((end & 0x70) < 0x40))) {
		for (i = 0; start < end;) {
			i += bpp;
			start += bpp;
			if ((start & 0x70) <= 0x40)
				break;
		}
		width -= i / bpp;
		end = start + width * bpp - 1;	/* Last uint8_t of last pixel */
		for (i = 0; end > start;) {
			i += bpp;
			end -= bpp;
			if ((end & 0x70) >= 0x40)
				break;
		}

		width -= i / bpp;
		/* Reset hw start address and layer ROI width */
		address = start;
		if (width <= 0)
			width = dst_w;
		pr_notice("OVL align 64byte:ROI(%d,%d %d,%d), (%d,%d), 0x%08x-->0x%08x, %d-->%d\n",
			  src_x, src_y, dst_w, dst_h, dst_x, dst_w, addr, address, dst_w, width);
	}
#else
	if (layer_pitch
	    && ((layer_pitch & 0x7f) != 0 || (start & 0x70) >= 0x50 || (end & 0x70) < 0x30)) {
		/* pr_notice("[DDP] warning: hw request(pitch:0x%x,star:0x%x,end:0x%x)\n", layer_pitch, start, end); */
	}
#endif
	switch (layer) {
	case 0:
		if ((source == OVL_LAYER_SOURCE_MEM) && (addr != 1))
			DISP_REG_SET(DISP_REG_OVL_RDMA0_CTRL, 0x1);
		else
			DISP_REG_SET(DISP_REG_OVL_RDMA0_CTRL, 0);
		DISP_REG_SET_FIELD(L0_CON_FLD_LAYER_SRC, DISP_REG_OVL_L0_CON, source);
		DISP_REG_SET_FIELD(L0_CON_FLD_CLRFMT, DISP_REG_OVL_L0_CON, fmt);
		DISP_REG_SET_FIELD(L0_CON_FLD_C_CF_SEL, DISP_REG_OVL_L0_CON, matrix_transform);
		DISP_REG_SET_FIELD(L0_CON_FLD_ALPHA_EN, DISP_REG_OVL_L0_CON, aen);
		DISP_REG_SET_FIELD(L0_CON_FLD_ALPHA, DISP_REG_OVL_L0_CON, alpha);
		DISP_REG_SET_FIELD(L0_CON_FLD_SRCKEY_EN, DISP_REG_OVL_L0_CON, keyEn);
		DISP_REG_SET_FIELD(L0_CON_FLD_RGB_SWAP, DISP_REG_OVL_L0_CON, rgb_swap);

		DISP_REG_SET(DISP_REG_OVL_L0_SRC_SIZE, dst_height << 16 | width);

		DISP_REG_SET(DISP_REG_OVL_L0_OFFSET, dst_y << 16 | dst_x);

		DISP_REG_SET(DISP_REG_OVL_L0_ADDR, address);

		DISP_REG_SET_FIELD(L0_PITCH_FLD_L0_SRC_PITCH, DISP_REG_OVL_L0_PITCH, layer_pitch);

		DISP_REG_SET(DISP_REG_OVL_L0_SRCKEY, key);

		if (TABLE_NO > mode) {	/* set up L0 YUV to RGB conversion matrix registers */
			DISP_REG_SET_FIELD(L0_Y2R_PARA_R0_FLD_C_CF_RMY, DISP_REG_OVL_L0_Y2R_PARA_R0,
					   coef[mode][0][0]);
			DISP_REG_SET_FIELD(L0_Y2R_PARA_R0_FLD_C_CF_RMU, DISP_REG_OVL_L0_Y2R_PARA_R0,
					   coef[mode][0][1]);
			DISP_REG_SET_FIELD(L0_Y2R_PARA_R1_FLD_C_CF_RMV, DISP_REG_OVL_L0_Y2R_PARA_R1,
					   coef[mode][0][2]);

			DISP_REG_SET_FIELD(L0_Y2R_PARA_G0_FLD_C_CF_GMY, DISP_REG_OVL_L0_Y2R_PARA_G0,
					   coef[mode][1][0]);
			DISP_REG_SET_FIELD(L0_Y2R_PARA_G0_FLD_C_CF_GMU, DISP_REG_OVL_L0_Y2R_PARA_G0,
					   coef[mode][1][1]);
			DISP_REG_SET_FIELD(L0_Y2R_PARA_G1_FLD_C_CF_GMV, DISP_REG_OVL_L0_Y2R_PARA_G1,
					   coef[mode][1][2]);

			DISP_REG_SET_FIELD(L0_Y2R_PARA_B0_FLD_C_CF_BMY, DISP_REG_OVL_L0_Y2R_PARA_B0,
					   coef[mode][2][0]);
			DISP_REG_SET_FIELD(L0_Y2R_PARA_B0_FLD_C_CF_BMU, DISP_REG_OVL_L0_Y2R_PARA_B0,
					   coef[mode][2][1]);
			DISP_REG_SET_FIELD(L0_Y2R_PARA_B1_FLD_C_CF_BMV, DISP_REG_OVL_L0_Y2R_PARA_B1,
					   coef[mode][2][2]);

			DISP_REG_SET_FIELD(L0_Y2R_PARA_YUV_A_0_FLD_C_CF_YA,
					   DISP_REG_OVL_L0_Y2R_PARA_YUV_A_0, coef[mode][3][0]);
			DISP_REG_SET_FIELD(L0_Y2R_PARA_YUV_A_0_FLD_C_CF_UA,
					   DISP_REG_OVL_L0_Y2R_PARA_YUV_A_0, coef[mode][3][1]);
			DISP_REG_SET_FIELD(L0_Y2R_PARA_YUV_A_1_FLD_C_CF_VA,
					   DISP_REG_OVL_L0_Y2R_PARA_YUV_A_1, coef[mode][3][2]);

			DISP_REG_SET_FIELD(L0_Y2R_PARA_RGB_A_0_FLD_C_CF_RA,
					   DISP_REG_OVL_L0_Y2R_PARA_RGB_A_0, coef[mode][4][0]);
			DISP_REG_SET_FIELD(L0_Y2R_PARA_RGB_A_0_FLD_C_CF_GA,
					   DISP_REG_OVL_L0_Y2R_PARA_RGB_A_0, coef[mode][4][1]);
			DISP_REG_SET_FIELD(L0_Y2R_PARA_RGB_A_1_FLD_C_CF_BA,
					   DISP_REG_OVL_L0_Y2R_PARA_RGB_A_1, coef[mode][4][2]);
			DISP_REG_SET_FIELD(L0_CON_FLD_EXT_MTX_EN, DISP_REG_OVL_L0_CON, 1);
		} else
			DISP_REG_SET_FIELD(L0_CON_FLD_EXT_MTX_EN, DISP_REG_OVL_L0_CON, 0);
		break;

	case 1:
		if ((source == OVL_LAYER_SOURCE_MEM) && (addr != 1))
			DISP_REG_SET(DISP_REG_OVL_RDMA1_CTRL, 0x1);
		else
			DISP_REG_SET(DISP_REG_OVL_RDMA1_CTRL, 0);
		DISP_REG_SET_FIELD(L1_CON_FLD_LAYER_SRC, DISP_REG_OVL_L1_CON, source);
		DISP_REG_SET_FIELD(L1_CON_FLD_CLRFMT, DISP_REG_OVL_L1_CON, fmt);
		DISP_REG_SET_FIELD(L1_CON_FLD_C_CF_SEL, DISP_REG_OVL_L1_CON, matrix_transform);
		DISP_REG_SET_FIELD(L1_CON_FLD_ALPHA_EN, DISP_REG_OVL_L1_CON, aen);
		DISP_REG_SET_FIELD(L1_CON_FLD_ALPHA, DISP_REG_OVL_L1_CON, alpha);
		DISP_REG_SET_FIELD(L1_CON_FLD_SRCKEY_EN, DISP_REG_OVL_L1_CON, keyEn);
		DISP_REG_SET_FIELD(L1_CON_FLD_RGB_SWAP, DISP_REG_OVL_L1_CON, rgb_swap);

		DISP_REG_SET(DISP_REG_OVL_L1_SRC_SIZE, dst_height << 16 | width);

		DISP_REG_SET(DISP_REG_OVL_L1_OFFSET, dst_y << 16 | dst_x);

		DISP_REG_SET(DISP_REG_OVL_L1_ADDR, address);

		DISP_REG_SET_FIELD(L1_PITCH_FLD_L1_SRC_PITCH, DISP_REG_OVL_L1_PITCH, layer_pitch);

		DISP_REG_SET(DISP_REG_OVL_L1_SRCKEY, key);

		if (TABLE_NO > mode) {	/* set up L0 YUV to RGB conversion matrix registers */
			DISP_REG_SET_FIELD(L1_Y2R_PARA_R0_FLD_C_CF_RMY, DISP_REG_OVL_L1_Y2R_PARA_R0,
					   coef[mode][0][0]);
			DISP_REG_SET_FIELD(L1_Y2R_PARA_R0_FLD_C_CF_RMU, DISP_REG_OVL_L1_Y2R_PARA_R0,
					   coef[mode][0][1]);
			DISP_REG_SET_FIELD(L1_Y2R_PARA_R1_FLD_C_CF_RMV, DISP_REG_OVL_L1_Y2R_PARA_R1,
					   coef[mode][0][2]);

			DISP_REG_SET_FIELD(L1_Y2R_PARA_G0_FLD_C_CF_GMY, DISP_REG_OVL_L1_Y2R_PARA_G0,
					   coef[mode][1][0]);
			DISP_REG_SET_FIELD(L1_Y2R_PARA_G0_FLD_C_CF_GMU, DISP_REG_OVL_L1_Y2R_PARA_G0,
					   coef[mode][1][1]);
			DISP_REG_SET_FIELD(L1_Y2R_PARA_G1_FLD_C_CF_GMV, DISP_REG_OVL_L1_Y2R_PARA_G1,
					   coef[mode][1][2]);

			DISP_REG_SET_FIELD(L1_Y2R_PARA_B0_FLD_C_CF_BMY, DISP_REG_OVL_L1_Y2R_PARA_B0,
					   coef[mode][2][0]);
			DISP_REG_SET_FIELD(L1_Y2R_PARA_B0_FLD_C_CF_BMU, DISP_REG_OVL_L1_Y2R_PARA_B0,
					   coef[mode][2][1]);
			DISP_REG_SET_FIELD(L1_Y2R_PARA_B1_FLD_C_CF_BMV, DISP_REG_OVL_L1_Y2R_PARA_B1,
					   coef[mode][2][2]);

			DISP_REG_SET_FIELD(L1_Y2R_PARA_YUV_A_0_FLD_C_CF_YA,
					   DISP_REG_OVL_L1_Y2R_PARA_YUV_A_0, coef[mode][3][0]);
			DISP_REG_SET_FIELD(L1_Y2R_PARA_YUV_A_0_FLD_C_CF_UA,
					   DISP_REG_OVL_L1_Y2R_PARA_YUV_A_0, coef[mode][3][1]);
			DISP_REG_SET_FIELD(L1_Y2R_PARA_YUV_A_1_FLD_C_CF_VA,
					   DISP_REG_OVL_L1_Y2R_PARA_YUV_A_1, coef[mode][3][2]);

			DISP_REG_SET_FIELD(L1_Y2R_PARA_RGB_A_0_FLD_C_CF_RA,
					   DISP_REG_OVL_L1_Y2R_PARA_RGB_A_0, coef[mode][4][0]);
			DISP_REG_SET_FIELD(L1_Y2R_PARA_RGB_A_0_FLD_C_CF_GA,
					   DISP_REG_OVL_L1_Y2R_PARA_RGB_A_0, coef[mode][4][1]);
			DISP_REG_SET_FIELD(L1_Y2R_PARA_RGB_A_1_FLD_C_CF_BA,
					   DISP_REG_OVL_L1_Y2R_PARA_RGB_A_1, coef[mode][4][2]);
			DISP_REG_SET_FIELD(L1_CON_FLD_EXT_MTX_EN, DISP_REG_OVL_L1_CON, 1);
		} else
			DISP_REG_SET_FIELD(L1_CON_FLD_EXT_MTX_EN, DISP_REG_OVL_L1_CON, 0);
		break;
	case 2:
		if ((source == OVL_LAYER_SOURCE_MEM) && (addr != 1))
			DISP_REG_SET(DISP_REG_OVL_RDMA2_CTRL, 0x1);
		else
			DISP_REG_SET(DISP_REG_OVL_RDMA2_CTRL, 0);
		DISP_REG_SET_FIELD(L2_CON_FLD_LAYER_SRC, DISP_REG_OVL_L2_CON, source);
		DISP_REG_SET_FIELD(L2_CON_FLD_CLRFMT, DISP_REG_OVL_L2_CON, fmt);
		DISP_REG_SET_FIELD(L2_CON_FLD_C_CF_SEL, DISP_REG_OVL_L2_CON, matrix_transform);
		DISP_REG_SET_FIELD(L2_CON_FLD_ALPHA_EN, DISP_REG_OVL_L2_CON, aen);
		DISP_REG_SET_FIELD(L2_CON_FLD_ALPHA, DISP_REG_OVL_L2_CON, alpha);
		DISP_REG_SET_FIELD(L2_CON_FLD_SRCKEY_EN, DISP_REG_OVL_L2_CON, keyEn);
		DISP_REG_SET_FIELD(L2_CON_FLD_RGB_SWAP, DISP_REG_OVL_L2_CON, rgb_swap);

		DISP_REG_SET(DISP_REG_OVL_L2_SRC_SIZE, dst_height << 16 | width);

		DISP_REG_SET(DISP_REG_OVL_L2_OFFSET, dst_y << 16 | dst_x);

		DISP_REG_SET(DISP_REG_OVL_L2_ADDR, address);

		DISP_REG_SET_FIELD(L2_PITCH_FLD_L2_SRC_PITCH, DISP_REG_OVL_L2_PITCH, layer_pitch);

		DISP_REG_SET(DISP_REG_OVL_L2_SRCKEY, key);

		if (TABLE_NO > mode) {	/* set up L0 YUV to RGB conversion matrix registers */
			DISP_REG_SET_FIELD(L2_Y2R_PARA_R0_FLD_C_CF_RMY, DISP_REG_OVL_L2_Y2R_PARA_R0,
					   coef[mode][0][0]);
			DISP_REG_SET_FIELD(L2_Y2R_PARA_R0_FLD_C_CF_RMU, DISP_REG_OVL_L2_Y2R_PARA_R0,
					   coef[mode][0][1]);
			DISP_REG_SET_FIELD(L2_Y2R_PARA_R1_FLD_C_CF_RMV, DISP_REG_OVL_L2_Y2R_PARA_R1,
					   coef[mode][0][2]);

			DISP_REG_SET_FIELD(L2_Y2R_PARA_G0_FLD_C_CF_GMY, DISP_REG_OVL_L2_Y2R_PARA_G0,
					   coef[mode][1][0]);
			DISP_REG_SET_FIELD(L2_Y2R_PARA_G0_FLD_C_CF_GMU, DISP_REG_OVL_L2_Y2R_PARA_G0,
					   coef[mode][1][1]);
			DISP_REG_SET_FIELD(L2_Y2R_PARA_G1_FLD_C_CF_GMV, DISP_REG_OVL_L2_Y2R_PARA_G1,
					   coef[mode][1][2]);

			DISP_REG_SET_FIELD(L2_Y2R_PARA_B0_FLD_C_CF_BMY, DISP_REG_OVL_L2_Y2R_PARA_B0,
					   coef[mode][2][0]);
			DISP_REG_SET_FIELD(L2_Y2R_PARA_B0_FLD_C_CF_BMU, DISP_REG_OVL_L2_Y2R_PARA_B0,
					   coef[mode][2][1]);
			DISP_REG_SET_FIELD(L2_Y2R_PARA_B1_FLD_C_CF_BMV, DISP_REG_OVL_L2_Y2R_PARA_B1,
					   coef[mode][2][2]);

			DISP_REG_SET_FIELD(L2_Y2R_PARA_YUV_A_0_FLD_C_CF_YA,
					   DISP_REG_OVL_L2_Y2R_PARA_YUV_A_0, coef[mode][3][0]);
			DISP_REG_SET_FIELD(L2_Y2R_PARA_YUV_A_0_FLD_C_CF_UA,
					   DISP_REG_OVL_L2_Y2R_PARA_YUV_A_0, coef[mode][3][1]);
			DISP_REG_SET_FIELD(L2_Y2R_PARA_YUV_A_1_FLD_C_CF_VA,
					   DISP_REG_OVL_L2_Y2R_PARA_YUV_A_1, coef[mode][3][2]);

			DISP_REG_SET_FIELD(L2_Y2R_PARA_RGB_A_0_FLD_C_CF_RA,
					   DISP_REG_OVL_L2_Y2R_PARA_RGB_A_0, coef[mode][4][0]);
			DISP_REG_SET_FIELD(L2_Y2R_PARA_RGB_A_0_FLD_C_CF_GA,
					   DISP_REG_OVL_L2_Y2R_PARA_RGB_A_0, coef[mode][4][1]);
			DISP_REG_SET_FIELD(L2_Y2R_PARA_RGB_A_1_FLD_C_CF_BA,
					   DISP_REG_OVL_L2_Y2R_PARA_RGB_A_1, coef[mode][4][2]);
			DISP_REG_SET_FIELD(L2_CON_FLD_EXT_MTX_EN, DISP_REG_OVL_L2_CON, 1);
		} else
			DISP_REG_SET_FIELD(L2_CON_FLD_EXT_MTX_EN, DISP_REG_OVL_L2_CON, 0);
		break;

	case 3:
		if ((source == OVL_LAYER_SOURCE_MEM) && (addr != 1))
			DISP_REG_SET(DISP_REG_OVL_RDMA3_CTRL, 0x1);
		else
			DISP_REG_SET(DISP_REG_OVL_RDMA3_CTRL, 0);
		DISP_REG_SET_FIELD(L3_CON_FLD_LAYER_SRC, DISP_REG_OVL_L3_CON, source);
		DISP_REG_SET_FIELD(L3_CON_FLD_CLRFMT, DISP_REG_OVL_L3_CON, fmt);
		DISP_REG_SET_FIELD(L3_CON_FLD_C_CF_SEL, DISP_REG_OVL_L3_CON, matrix_transform);
		DISP_REG_SET_FIELD(L3_CON_FLD_ALPHA_EN, DISP_REG_OVL_L3_CON, aen);
		DISP_REG_SET_FIELD(L3_CON_FLD_ALPHA, DISP_REG_OVL_L3_CON, alpha);
		DISP_REG_SET_FIELD(L3_CON_FLD_SRCKEY_EN, DISP_REG_OVL_L3_CON, keyEn);
		DISP_REG_SET_FIELD(L3_CON_FLD_RGB_SWAP, DISP_REG_OVL_L3_CON, rgb_swap);

		DISP_REG_SET(DISP_REG_OVL_L3_SRC_SIZE, dst_height << 16 | width);

		DISP_REG_SET(DISP_REG_OVL_L3_OFFSET, dst_y << 16 | dst_x);

		DISP_REG_SET(DISP_REG_OVL_L3_ADDR, address);

		DISP_REG_SET_FIELD(L3_PITCH_FLD_L3_SRC_PITCH, DISP_REG_OVL_L3_PITCH, layer_pitch);

		DISP_REG_SET(DISP_REG_OVL_L3_SRCKEY, key);

		if (TABLE_NO > mode) {	/* set up L0 YUV to RGB conversion matrix registers */
			DISP_REG_SET_FIELD(L3_Y2R_PARA_R0_FLD_C_CF_RMY, DISP_REG_OVL_L3_Y2R_PARA_R0,
					   coef[mode][0][0]);
			DISP_REG_SET_FIELD(L3_Y2R_PARA_R0_FLD_C_CF_RMU, DISP_REG_OVL_L3_Y2R_PARA_R0,
					   coef[mode][0][1]);
			DISP_REG_SET_FIELD(L3_Y2R_PARA_R1_FLD_C_CF_RMV, DISP_REG_OVL_L3_Y2R_PARA_R1,
					   coef[mode][0][2]);

			DISP_REG_SET_FIELD(L3_Y2R_PARA_G0_FLD_C_CF_GMY, DISP_REG_OVL_L3_Y2R_PARA_G0,
					   coef[mode][1][0]);
			DISP_REG_SET_FIELD(L3_Y2R_PARA_G0_FLD_C_CF_GMU, DISP_REG_OVL_L3_Y2R_PARA_G0,
					   coef[mode][1][1]);
			DISP_REG_SET_FIELD(L3_Y2R_PARA_G1_FLD_C_CF_GMV, DISP_REG_OVL_L3_Y2R_PARA_G1,
					   coef[mode][1][2]);

			DISP_REG_SET_FIELD(L3_Y2R_PARA_B0_FLD_C_CF_BMY, DISP_REG_OVL_L3_Y2R_PARA_B0,
					   coef[mode][2][0]);
			DISP_REG_SET_FIELD(L3_Y2R_PARA_B0_FLD_C_CF_BMU, DISP_REG_OVL_L3_Y2R_PARA_B0,
					   coef[mode][2][1]);
			DISP_REG_SET_FIELD(L3_Y2R_PARA_B1_FLD_C_CF_BMV, DISP_REG_OVL_L3_Y2R_PARA_B1,
					   coef[mode][2][2]);

			DISP_REG_SET_FIELD(L3_Y2R_PARA_YUV_A_0_FLD_C_CF_YA,
					   DISP_REG_OVL_L3_Y2R_PARA_YUV_A_0, coef[mode][3][0]);
			DISP_REG_SET_FIELD(L3_Y2R_PARA_YUV_A_0_FLD_C_CF_UA,
					   DISP_REG_OVL_L3_Y2R_PARA_YUV_A_0, coef[mode][3][1]);
			DISP_REG_SET_FIELD(L3_Y2R_PARA_YUV_A_1_FLD_C_CF_VA,
					   DISP_REG_OVL_L3_Y2R_PARA_YUV_A_1, coef[mode][3][2]);

			DISP_REG_SET_FIELD(L3_Y2R_PARA_RGB_A_0_FLD_C_CF_RA,
					   DISP_REG_OVL_L3_Y2R_PARA_RGB_A_0, coef[mode][4][0]);
			DISP_REG_SET_FIELD(L3_Y2R_PARA_RGB_A_0_FLD_C_CF_GA,
					   DISP_REG_OVL_L3_Y2R_PARA_RGB_A_0, coef[mode][4][1]);
			DISP_REG_SET_FIELD(L3_Y2R_PARA_RGB_A_1_FLD_C_CF_BA,
					   DISP_REG_OVL_L3_Y2R_PARA_RGB_A_1, coef[mode][4][2]);
			DISP_REG_SET_FIELD(L3_CON_FLD_EXT_MTX_EN, DISP_REG_OVL_L3_CON, 1);
		} else
			DISP_REG_SET_FIELD(L3_CON_FLD_EXT_MTX_EN, DISP_REG_OVL_L3_CON, 0);
		break;

	default:
		pr_notice("DDP error, OVLLayerConfig layer=%d\n", layer);
		ASSERT(0);	/* invalid layer index */
	}

	if (width >= 1080) {
		/* pr_notice("[DDP]set 1080p ultra\n"); */
		DISP_REG_SET(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING, 0x00f00040);
		DISP_REG_SET(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING, 0x00f00040);
		DISP_REG_SET(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING, 0x00f00040);
		DISP_REG_SET(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING, 0x00f00040);
	}

	return 0;
}

void OVLLayerPQControl(unsigned layer, bool pq_en)
{
	if (layer >= 4)
		return;

	if (pq_en)
		DISP_REG_SET(DISP_REG_OVL_DATAPATH_CON, ((1 << (layer + 4)) | layer) << 16);
	else
		DISP_REG_SET(DISP_REG_OVL_DATAPATH_CON, 0);
}
