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

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include "disp_assert_layer.h"
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/file.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include "mt-plat/dma.h"
#include <linux/dma-mapping.h>
#include "mt-plat/mt_boot.h"
#include "disp_debug.h"
#include "disp_drv.h"
#include "ddp_hal.h"
#include "disp_drv_log.h"
#include "disp_hal.h"
#include "mtkfb.h"
#include "mtkfb_console.h"
#include "mtkfb_info.h"
#include "ddp_ovl.h"
#include "ddp_drv.h"
#include "mtkfb_priv.h"
#include "disp_drv_platform.h"

#if defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
#include "disp_ovl_engine_api.h"
#include "disp_ovl_engine_core.h"
#endif

static int fill_buffer_rgb(void *addr, unsigned int size)
{
	int i;
	char *buf_addr;

	buf_addr = (char *)addr;
	for (i = 0; i < size / 3; i += 3) {
		buf_addr[i] = 0xff;
		buf_addr[i + 1] = 0x0;
		buf_addr[i + 2] = 0x0;
	}
	for (; i < size * 2 / 3; i += 3) {
		buf_addr[i] = 0x0;
		buf_addr[i + 1] = 0xff;
		buf_addr[i + 2] = 0x0;
	}
	for (; i < size; i += 3) {
		buf_addr[i] = 0x0;
		buf_addr[i + 1] = 0x0;
		buf_addr[i + 2] = 0xff;
	}

	return 0;
}

static int alloc_buffer(unsigned int size, unsigned int *pVa, unsigned int *pMva)
{
#ifdef CONFIG_MTK_M4U
	M4U_PORT_STRUCT m4uport;
#endif
	unsigned int mva, va;

	if (size == 0)
		return -1;

	va = (unsigned int)vmalloc(size);
	if (((void *)va) == NULL) {
		pr_notice("vmalloc %d bytes fail!!!\n", size);
		return -1;
	}

	memset((void *)va, 0, size);

	fill_buffer_rgb((void *)va, size);

#ifdef CONFIG_MTK_M4U
	if (m4u_alloc_mva(DISP_OVL_0, va, size, 0, 0, &mva)) {
		pr_notice("m4u_alloc_mva for hdmi_mva_r fail\n");
		vfree((void *)va);
		return -1;
	}

	memset((void *)&m4uport, 0, sizeof(M4U_PORT_STRUCT));
	m4uport.ePortID = DISP_OVL_0;
	m4uport.Virtuality = 1;
	m4uport.domain = 3;
	m4uport.Security = 0;
	m4uport.Distance = 1;
	m4uport.Direction = 0;
	m4u_config_port(&m4uport);
#else
	mva = __pa(va);
#endif

	*pMva = mva;
	*pVa = va;

	pr_notice("[Pattern] alloc_buffer va=0x%08x, mva=0x%08x\n", va, mva);

	return 0;
}

int free_buffer(unsigned int size, unsigned int va, unsigned int mva)
{
	pr_notice("[Pattern] free_buffer va=0x%08x, mva=0x%08x\n", va, mva);

#ifdef CONFIG_MTK_M4U
	if (mva) {
		M4U_PORT_STRUCT m4uport;

		m4uport.ePortID = DISP_OVL_0;
		m4uport.Virtuality = 1;
		m4uport.domain = 3;
		m4uport.Security = 0;
		m4uport.Distance = 1;
		m4uport.Direction = 0;
		m4u_config_port(&m4uport);

		m4u_dealloc_mva(DISP_OVL_0, va, size, mva);
		mva = 0;
	}
#endif

	if (va) {
		vfree((void *)va);
		va = 0;
	}

	return 0;
}

#ifdef MTK_SEC_VIDEO_PATH_SUPPORT

KREE_SESSION_HANDLE fb_pattern_secure_memory_session_handle(void)
{
	static KREE_SESSION_HANDLE fb_pattern_secure_memory_session;

	pr_notice("fb_pattern_secure_memory_session_handle() acquire TEE session\n");
	if (NULL == fb_pattern_secure_memory_session) {
		TZ_RESULT ret;

		pr_notice("fb_pattern_secure_memory_session_handle() create session\n");
		ret = KREE_CreateSession(TZ_TA_MEM_UUID, &fb_pattern_secure_memory_session);
		if (ret != TZ_RESULT_SUCCESS) {
			pr_notice("KREE_CreateSession fail, ret=%d\n", ret);
			return NULL;
		}
	}

	pr_notice("fb_pattern_secure_memory_session_handle() session=%x\n",
		  (unsigned int)fb_pattern_secure_memory_session);
	return fb_pattern_secure_memory_session;
}


void *fb_pattern_allocate_secure_memory(int size)
{
	KREE_SECUREMEM_HANDLE mem_handle;
	TZ_RESULT ret;
	struct disp_mva_map mva_map_struct;
	MTEEC_PARAM param[4];
	unsigned int paramTypes;

	/* Allocate */
	ret =
	    KREE_AllocSecurechunkmemWithTag(fb_pattern_secure_memory_session_handle(), &mem_handle, 4096,
				     size, "main_path");
	if (ret != TZ_RESULT_SUCCESS) {
		pr_notice("KREE_AllocSecurechunkmemWithTag fail, ret=%d\n", ret);
		return NULL;
	}

	pr_notice("KREE_AllocSecurechunkmemWithTag handle=0x%x\n", mem_handle);

	param[0].value.a = DISP_OVL_0;
	param[1].value.a = 0;
	param[2].value.a = (unsigned int)mem_handle;
	param[3].value.a = size;
	paramTypes =
	    TZ_ParamTypes4(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
	ret =
	    KREE_TeeServiceCall(ddp_session_handle(), TZCMD_DDP_SECURE_MVA_MAP, paramTypes, param);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_OVL_ENGINE_ERR("KREE_TeeServiceCall(TZCMD_DDP_SECURE_MVA_MAP) fail, ret=%d\n",
				    ret);

		return -1;
	}

	return (void *)mem_handle;
}
#endif

unsigned int g_fb_pattern_en = 0;
int fb_pattern_en(int enable)
{
	g_fb_pattern_en = enable;

	return 0;
}

int fb_pattern(struct mtkfb_device *fbdev, struct fb_overlay_config *fb_config)
{
	static unsigned int mem_init = 0, pattern_va = 0, pattern_mva = 0, pattern_mva_sec;
	unsigned int pattern_size = DISP_GetScreenWidth() * DISP_GetScreenHeight() * 3;
	int ret, i;

	pr_notice("================================ New frame ===============================\n");
	pr_notice("[Pattern] fb_pattern\n");

	pr_notice("fb_pattern orgi input: fence:%d, time: 0x%x\n", fb_config->fence,
		  fb_config->time);
	for (i = 0; i < 4; i++) {
		pr_notice("fb_pattern orgi input:\n");
		pr_notice("    layer%d en%d, next_idx=0x%x, vaddr=0x%x, paddr=0x%x, fmt=%u\n",
			  fb_config->layers[i].layer_id,
			  fb_config->layers[i].layer_enable, fb_config->layers[i].next_buff_idx,
			  (unsigned int)(fb_config->layers[i].src_base_addr),
			  (unsigned int)(fb_config->layers[i].src_phy_addr),
			  fb_config->layers[i].src_fmt);
		pr_notice
		    ("    d-link=%u, pitch=%u, xoff=%u, yoff=%u, w=%u, h=%u, alpha_en=%d, alpha=%d\n",
		     (unsigned int)(fb_config->layers[i].src_direct_link),
		     fb_config->layers[i].src_pitch, fb_config->layers[i].src_offset_x,
		     fb_config->layers[i].src_offset_y, fb_config->layers[i].src_width,
		     fb_config->layers[i].src_height, fb_config->layers[i].alpha_enable,
		     fb_config->layers[i].alpha);
		pr_notice("    fence_fd=%d, ion_fd=%d, security=%d\n",
			  fb_config->layers[i].fence_fd, fb_config->layers[i].ion_fd,
			  fb_config->layers[i].security);
	}
	pr_notice("\n");

	if (mem_init == 0) {
		if (pattern_mva == 0) {
			ret = alloc_buffer(pattern_size, &pattern_va, &pattern_mva);
			if (ret == -1)
				return -1;
		}
#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
		if (pattern_mva_sec == 0)
			pattern_mva_sec =
			    (unsigned int)fb_pattern_allocate_secure_memory(pattern_size);
#endif

		mem_init = 1;
	}

	fb_config->layers[0].layer_id = 0;
	fb_config->layers[0].layer_enable = 1;
	fb_config->layers[0].src_width = DISP_GetScreenWidth();
	fb_config->layers[0].src_height = DISP_GetScreenHeight();
	fb_config->layers[0].src_pitch = DISP_GetScreenWidth();
	fb_config->layers[0].tgt_width = DISP_GetScreenWidth();
	fb_config->layers[0].tgt_height = DISP_GetScreenHeight();
	fb_config->layers[0].src_fmt = MTK_FB_FORMAT_RGB888;
	if (g_fb_pattern_en == 1) {
		fb_config->layers[0].src_phy_addr = (void *)pattern_mva;
		fb_config->layers[0].security = 0;
	} else if (g_fb_pattern_en == 2) {
		fb_config->layers[0].src_phy_addr = (void *)pattern_mva_sec;
		fb_config->layers[0].security = 1;
	}

	fb_config->layers[1].layer_id = 1;
	fb_config->layers[1].layer_enable = 0;

	fb_config->layers[2].layer_id = 2;
	fb_config->layers[2].layer_enable = 0;

	fb_config->layers[3].layer_id = 3;
	fb_config->layers[3].layer_enable = 0;

	mtkfb_queue_overlay_config(fbdev, fb_config);

	/* free_buffer(DISP_OVL_0, pattern_size, pattern_va, pattern_mva); */

	return 0;

}
