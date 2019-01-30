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

#include <linux/delay.h>
#include "disp_ovl_engine_hw.h"
#include "ddp_reg.h"
#include "disp_debug.h"
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include "disp_ovl_engine_core.h"
#include "ddp_hal.h"
#include "ddp_drv.h"
/* for MTK_HDMI_MAIN_PATH */

/* Parameter */
static DISP_OVL_ENGINE_INSTANCE disp_ovl_engine_params;

/* Irq callback */
void (*disp_ovl_engine_hw_irq_callback)(unsigned int param) = NULL;
static void disp_ovl_engine_hw_ovl_wdma_irq_handler(unsigned int param);
static void disp_ovl_engine_hw_ovl_rdma_irq_handler(unsigned int param);

#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
static void *disp_ovl_engine_hw_allocate_secure_memory(int size);
static void disp_ovl_engine_hw_free_secure_memory(void *mem_handle);
#endif

void disp_ovl_engine_hw_init(void)
{
	memset(&disp_ovl_engine_params, 0, sizeof(DISP_OVL_ENGINE_INSTANCE));

	disp_path_register_ovl_wdma_callback(disp_ovl_engine_hw_ovl_wdma_irq_handler, 0);
	/* disp_path_register_ovl_rdma_callback(disp_ovl_engine_hw_ovl_rdma_irq_handler,0); */
}


void disp_ovl_engine_hw_set_params(DISP_OVL_ENGINE_INSTANCE *params)
{
	MMProfileLogEx(MTKFB_MMP_Events.hw_set_params, MMProfileFlagPulse, 0, 0);

	memcpy(&disp_ovl_engine_params, params, sizeof(DISP_OVL_ENGINE_INSTANCE));
	atomic_set(&params->OverlaySettingDirtyFlag, 0);
	atomic_set(&params->OverlaySettingApplied, 1);
	memset(&params->MemOutConfig, 0, sizeof(params->MemOutConfig));
}


int g_ovl_wdma_irq_ignore = 0;
void disp_ovl_engine_hw_ovl_wdma_irq_handler(unsigned int param)
{
	DISP_OVL_ENGINE_DBG("disp_ovl_engine_hw_ovl_wdma_irq_handler\n");
	MMProfileLogEx(MTKFB_MMP_Events.hw_ovl_wdma_irq_handler, MMProfileFlagPulse, param, 0);

	if (g_ovl_wdma_irq_ignore) {
		g_ovl_wdma_irq_ignore = 0;
		return;
	}

	if (disp_ovl_engine_hw_irq_callback != NULL)
		disp_ovl_engine_hw_irq_callback(param);
}

void disp_ovl_engine_hw_ovl_rdma_irq_handler(unsigned int param)
{
	DISP_OVL_ENGINE_DBG("disp_ovl_engine_hw_ovl_rdma_irq_handler\n");
	MMProfileLogEx(MTKFB_MMP_Events.hw_ovl_rdma_irq_handler, MMProfileFlagPulse, param, 0);

	if (disp_ovl_engine_hw_irq_callback != NULL)
		disp_ovl_engine_hw_irq_callback(param);
}


static void _rdma0_irq_handler(unsigned int param)
{
	/* DISP_OVL_ENGINE_DBG("rdma0 irq interrupt, param: %d\n", param); */
	MMProfileLogEx(MTKFB_MMP_Events.rdma0_irq_handler, MMProfileFlagPulse, param, 0);
	/* disp_ovl_engine.RdmaRdIdx = disp_ovl_engine.OvlWrIdx; */

	if (param & 0x1)	/* rdma0 reg update */
		MMProfileLogEx(MTKFB_MMP_Events.OVLEngine_rdma_status, MMProfileFlagPulse, 0x1111,
			       0x1111);
	if (param & 0x2)	/* rdma0 frame start */
		MMProfileLogEx(MTKFB_MMP_Events.OVLEngine_rdma_status, MMProfileFlagStart, 0x2222,
			       0x2222);
	if (param & 0x4)	/* rdma0 frame end */
		MMProfileLogEx(MTKFB_MMP_Events.OVLEngine_rdma_status, MMProfileFlagEnd, 0x4444,
			       0x4444);
	if (param & 0x8)	/* rdma0 EOF abnormal */
		MMProfileLogEx(MTKFB_MMP_Events.OVLEngine_rdma_status, MMProfileFlagEnd, 0x8888,
			       0x8888);
	if (param & 0x20) {
		MMProfileLogEx(MTKFB_MMP_Events.OVLEngine_rdma_status, MMProfileFlagPulse, 0x20,
			       0x20);
		disp_ovl_engine_wake_up_rdma0_update_thread();
	}
}

static unsigned int gOvlWdmaMutexID = 3;
static int OvlSecure;		/* Todo, this suggest that only one HW overlay. */

void disp_ovl_engine_trigger_hw_overlay_decouple(void)
{
	int layer_id;
	int OvlSecureNew = 0;

	DISP_OVL_ENGINE_DBG("disp_ovl_engine_trigger_hw_overlay_decouple\n");
	MMProfileLogEx(MTKFB_MMP_Events.trigger_hw_overlay_decouple, MMProfileFlagStart, 0, 0);

	/* OVLReset(); */
	/* WDMAReset(1); */

	disp_path_config_OVL_WDMA_path(gOvlWdmaMutexID);

	disp_path_get_mutex_(gOvlWdmaMutexID);

	for (layer_id = 0; layer_id < DDP_OVL_LAYER_MUN; layer_id++)
		if ((disp_ovl_engine_params.cached_layer_config[layer_id].layer_en) &&
		    (disp_ovl_engine_params.cached_layer_config[layer_id].security ==
		     OVL_LAYER_SECURE_BUFFER))
			OvlSecureNew = 1;

	if (OvlSecure != OvlSecureNew)
		if (OvlSecureNew) {
			OvlSecure = OvlSecureNew;
			disp_register_intr(disp_dev.irq[DISP_REG_WDMA], OvlSecure);
		}

	disp_path_config_OVL_WDMA(&(disp_ovl_engine_params.MemOutConfig), OvlSecure);

	for (layer_id = 0; layer_id < DDP_OVL_LAYER_MUN; layer_id++) {
		disp_ovl_engine_params.cached_layer_config[layer_id].layer = layer_id;

		disp_path_config_layer_ovl_engine(&
						  (disp_ovl_engine_params.cached_layer_config
						   [layer_id]), OvlSecure);
	}

	/* disp_dump_reg(DISP_MODULE_WDMA1); */
	/* disp_dump_reg(DISP_MODULE_OVL); */

	disp_path_release_mutex_(gOvlWdmaMutexID);
	disp_path_wait_reg_update(gOvlWdmaMutexID);

	MMProfileLogEx(MTKFB_MMP_Events.OVLEngine_ovlwdma_status, MMProfileFlagStart,
		       disp_ovl_engine.OvlWrIdx,
		       disp_ovl_engine.OvlBufAddr[disp_ovl_engine.OvlWrIdx]);

	if (OvlSecure != OvlSecureNew)
		if (!OvlSecureNew) {
			OvlSecure = OvlSecureNew;
			disp_register_intr(disp_dev.irq[DISP_REG_WDMA], OvlSecure);
		}

	MMProfileLogEx(MTKFB_MMP_Events.trigger_hw_overlay_decouple, MMProfileFlagEnd, 0, 0);
}

void disp_ovl_engine_config_overlay(void)
{
	unsigned int i = 0;
	int dirty;
	int layer_id;
	int OvlSecureNew = 0;

	MMProfileLogEx(MTKFB_MMP_Events.config_overlay, MMProfileFlagStart, 0, 0);

	disp_path_config_layer_ovl_engine_control(true);

	for (layer_id = 0; layer_id < DDP_OVL_LAYER_MUN; layer_id++) {
		/*pr_err("[02420] ovl_ening_hw layer=%d,layer_en=%d,addr=0x%08x,vaddr=0x%08x,src_x=%d, src_y=%d",
		   disp_ovl_engine_params.cached_layer_config[layer_id].layer,
		   disp_ovl_engine_params.cached_layer_config[layer_id].layer_en,
		   disp_ovl_engine_params.cached_layer_config[layer_id].addr,
		   disp_ovl_engine_params.cached_layer_config[layer_id].vaddr,
		   disp_ovl_engine_params.cached_layer_config[layer_id].src_x,
		   disp_ovl_engine_params.cached_layer_config[layer_id].src_y ); */

		if ((disp_ovl_engine_params.cached_layer_config[layer_id].layer_en) &&
		    (disp_ovl_engine_params.cached_layer_config[layer_id].security ==
		     OVL_LAYER_SECURE_BUFFER))
			OvlSecureNew = 1;
	}

	if (OvlSecure != OvlSecureNew) {
		OvlSecure = OvlSecureNew;

		disp_register_intr(disp_dev.irq[DISP_REG_OVL], OvlSecure);
	}


	disp_path_get_mutex();
	for (i = 0; i < DDP_OVL_LAYER_MUN; i++) {
		if (disp_ovl_engine_params.cached_layer_config[i].isDirty) {
			dirty |= 1 << i;
			disp_path_config_layer_ovl_engine
			    (&disp_ovl_engine_params.cached_layer_config[i], OvlSecure);
			disp_ovl_engine_params.cached_layer_config[i].isDirty = false;
		}
	}
	disp_path_release_mutex();

	MMProfileLogEx(MTKFB_MMP_Events.config_overlay, MMProfileFlagEnd, 0, 0);
}

void disp_ovl_engine_direct_link_overlay(void)
{
/* unsigned int i = 0; */
/* unsigned int dirty = 0; */
	int lcm_width = DISP_GetScreenWidth();
	int buffer_bpp = 3;
	static int first_boot = 1;
	/* setup the direct link of overlay - rdma */
	DISP_OVL_ENGINE_INFO("direct link overlay, addr=0x%x\n",
			     disp_ovl_engine.OvlBufAddr[disp_ovl_engine.RdmaRdIdx]);

	DISP_OVL_ENGINE_DBG
	    ("ovl addr: 0x%x srcModule: %d dstModule: %d inFormat: %d outFormat: %d\n",
	     disp_ovl_engine_params.path_info.ovl_config.addr,
	     disp_ovl_engine_params.path_info.srcModule, disp_ovl_engine_params.path_info.dstModule,
	     disp_ovl_engine_params.path_info.inFormat, disp_ovl_engine_params.path_info.outFormat);

	MMProfileLogEx(MTKFB_MMP_Events.direct_link_overlay, MMProfileFlagStart, 0, 0);

	if (1 == first_boot) {
#ifdef CONFIG_MTK_M4U
		M4U_PORT_STRUCT portStruct;

		DISP_OVL_ENGINE_DBG("config m4u start\n\n");
		portStruct.ePortID = DISP_OVL_0;	/* hardware port ID, defined in M4U_PORT_ID_ENUM */
		portStruct.Virtuality = 1;
		portStruct.Security = 0;
		portStruct.domain = 3;	/* domain : 0 1 2 3 */
		portStruct.Distance = 1;
		portStruct.Direction = 0;
		m4u_config_port(&portStruct);
#endif
		first_boot = 0;
	}

	if (disp_ovl_engine.OvlBufAddr[disp_ovl_engine.RdmaRdIdx] != 0) {
		disp_ovl_engine_params.path_info.ovl_config.fmt = eRGB888;
		disp_ovl_engine_params.path_info.ovl_config.src_pitch = lcm_width * buffer_bpp;
		disp_ovl_engine_params.path_info.ovl_config.addr =
		    disp_ovl_engine.OvlBufAddr[disp_ovl_engine.RdmaRdIdx];
		disp_ovl_engine_params.path_info.addr =
		    disp_ovl_engine.OvlBufAddr[disp_ovl_engine.RdmaRdIdx];
	}

	disp_path_register_ovl_rdma_callback(disp_ovl_engine_hw_ovl_rdma_irq_handler, 0);
	disp_unregister_irq(DISP_MODULE_RDMA0, _rdma0_irq_handler);
	disp_path_get_mutex();
	disp_path_config(&(disp_ovl_engine_params.path_info));
	disp_path_release_mutex();

	if (lcm_params->dsi.mode != CMD_MODE)
		disp_path_get_mutex();


	if (lcm_params->dsi.mode != CMD_MODE)
		disp_path_release_mutex();

	/* if(lcm_params->dsi.mode != CMD_MODE) */
	{
		/* disp_path_release_mutex(); */
	}

    /*************************************************/
	/* Ultra config */

	DISP_REG_SET(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING, 0x40402020);
	DISP_REG_SET(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING, 0x40402020);
	DISP_REG_SET(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING, 0x40402020);
	DISP_REG_SET(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING, 0x40402020);

	/* disp_wdma1 ultra */
	DISP_REG_SET(DISP_REG_WDMA_BUF_CON1 + 0x1000, 0x800800ff);
    /*************************************************/

	/* pr_info("DUMP register =============================================\n"); */
	/* disp_dump_reg(DISP_MODULE_OVL); */
	/* disp_dump_reg(DISP_MODULE_RDMA0); */
	/* pr_info("DUMP register end =============================================\n"); */

	MMProfileLogEx(MTKFB_MMP_Events.direct_link_overlay, MMProfileFlagEnd, 0, 0);

}

static void disp_ovl_engine_565_to_888(void *src_va, void *dst_va)
{
	unsigned int xres = DISP_GetScreenWidth();
	unsigned int yres = DISP_GetScreenHeight();

	unsigned short *s = (unsigned short *)(src_va);
	unsigned char *d = (unsigned char *)(dst_va);
	unsigned short src_rgb565 = 0;
	int j = 0;
	int k = 0;

	pr_notice("disp ovl engine 555_to_888, s = 0x%p, d=0x%p\n", s, d);
	for (j = 0; j < yres; ++j) {
		for (k = 0; k < xres; ++k) {
			src_rgb565 = *s++;
			*d++ = ((src_rgb565 & 0x1F) << 3);
			*d++ = ((src_rgb565 & 0x7E0) >> 3);
			*d++ = ((src_rgb565 & 0xF800) >> 8);
		}
		s += (ALIGN_TO(xres, disphal_get_fb_alignment()) - xres);
	}
}

static int fill_buffer_rgb(void *addr, unsigned int size)
{
	int i;
	char *buf_addr;

	return 0;

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


int disp_ovl_engine_indirect_link_overlay(void *fb_va, void *fb_pa)
{
	/* Steps for reconfig display path
	   1. allocate internal buffer
	   2. config m4u port
	   3. config rdma read from memory and change mutex setting
	   4. config overlay to wdma
	 */
	int lcm_width, lcm_height;
	int buffer_bpp;
	int tmpBufferSize;
	int i, temp_va = 0;
	static int internal_buffer_init;
	struct disp_path_config_struct config = { 0 };
#ifdef CONFIG_MTK_M4U
	M4U_PORT_STRUCT portStruct;
#endif

	pr_notice("indirect link overlay entry\n");

	MMProfileLogEx(MTKFB_MMP_Events.indirect_link_overlay, MMProfileFlagStart, 0, 0);

	/* step 1, alloc resource */
	lcm_width = DISP_GetScreenWidth();
	lcm_height = DISP_GetScreenHeight();
	buffer_bpp = 3;		/* (DISP_GetScreenBpp() + 7) >> 3; */
	tmpBufferSize = lcm_width * lcm_height * buffer_bpp * OVL_ENGINE_OVL_BUFFER_NUMBER;
	DISP_OVL_ENGINE_DBG("lcm_width: %d, lcm_height: %d, buffer_bpp: %d\n",
			    lcm_width, lcm_height, buffer_bpp);

	if (0 == internal_buffer_init) {
		internal_buffer_init = 1;

		DISP_OVL_ENGINE_DBG("indirect link alloc internal buffer\n");

#ifdef CONFIG_MTK_IOMMU
		temp_va = (unsigned int)dma_alloc_coherent(disp_dev.dev_use_imu,
						tmpBufferSize,
						(dma_addr_t *)&disp_ovl_engine.Ovlmva,
						GFP_KERNEL);
		DISP_MSG("dma_alloc_coherent:0x%x va:0x%x size:0x%x\n",
				disp_ovl_engine.Ovlmva, temp_va, tmpBufferSize);
#else
		temp_va = (uint32_t)fb_va + DISP_GetFBRamSize() + DAL_GetLayerSize();
		disp_ovl_engine.Ovlmva = (uint32_t)fb_pa + DISP_GetFBRamSize() + DAL_GetLayerSize();
#endif
		for (i = 0; i < OVL_ENGINE_OVL_BUFFER_NUMBER; i++) {
			disp_ovl_engine.OvlBufAddr[i] =
			    disp_ovl_engine.Ovlmva + lcm_width * lcm_height * buffer_bpp * i;

			disp_ovl_engine.OvlBufAddr_va[i] =
			    temp_va + lcm_width * lcm_height * buffer_bpp * i;

			disp_ovl_engine_565_to_888(fb_va, (void *)disp_ovl_engine.OvlBufAddr_va[i]);

			fill_buffer_rgb((void *)disp_ovl_engine.OvlBufAddr_va[i], lcm_width * lcm_height * buffer_bpp);

			disp_ovl_engine.OvlBufSecurity[i] = false;
		}

		DISP_OVL_ENGINE_DBG("M4U alloc mva: 0x%x va: 0x%x size: 0x%x\n",
				    disp_ovl_engine.Ovlmva, temp_va, tmpBufferSize);
	}
#ifdef CONFIG_MTK_M4U
	/* config m4u port */
	DISP_OVL_ENGINE_DBG("config m4u start\n\n");
	portStruct.ePortID = DISP_OVL_0;
	portStruct.Virtuality = 1;
	portStruct.Security = 0;
	portStruct.domain = 3;
	portStruct.Distance = 1;
	portStruct.Direction = 0;
	m4u_config_port(&portStruct);

	portStruct.ePortID = DISP_WDMA;
	portStruct.Virtuality = 1;
	portStruct.Security = 0;
	portStruct.domain = 1;
	portStruct.Distance = 1;
	portStruct.Direction = 0;
	m4u_config_port(&portStruct);

	portStruct.ePortID = DISP_RDMA;
	portStruct.Virtuality = 1;
	portStruct.Security = 0;
	portStruct.domain = 1;
	portStruct.Distance = 1;
	portStruct.Direction = 0;
	m4u_config_port(&portStruct);

#elif defined(CONFIG_MTK_IOMMU)

	mtk_smi_config_port(&disp_dev.psmidev->dev, M4U_PORT_DISP_RDMA, true);
	mtk_smi_config_port(&disp_dev.psmidev->dev, M4U_PORT_DISP_WDMA, true);

#endif

	/* config rdma->color->bls->dpi0 path */
	config.srcModule = DISP_MODULE_RDMA0;
	config.inFormat = eRGB888;
	config.addr = disp_ovl_engine.OvlBufAddr[0];	/* *(volatile unsigned int *)(0xF40030A0); */
	config.pitch = lcm_width * buffer_bpp;
	config.srcHeight = lcm_height;
	config.srcWidth = lcm_width;
#if MTK_HDMI_MAIN_PATH
	config.dstModule = DISP_MODULE_DPI1;
#else
	if (LCM_TYPE_DSI == lcm_params->type) {
		if (lcm_params->dsi.mode == CMD_MODE)
			config.dstModule = DISP_MODULE_DSI_CMD;	/* DISP_MODULE_WDMA1 */
		else
			config.dstModule = DISP_MODULE_DSI_VDO;	/* DISP_MODULE_WDMA1 */
	} else if (LCM_TYPE_DPI == lcm_params->type)
		config.dstModule = DISP_MODULE_DPI0;
	else
		config.dstModule = DISP_MODULE_DBI;
#endif
	config.outFormat = RDMA_OUTPUT_FORMAT_ARGB;
	disp_register_irq(DISP_MODULE_RDMA0, _rdma0_irq_handler);
	DISP_WaitVSYNC();
	disp_path_get_mutex();
	RDMASetTargetLine(0, lcm_height * 4 / 5);
	disp_path_config(&config);
	disp_path_release_mutex();
	disp_path_wait_reg_update(0);

	/* config ovl->wdma path */
	DISP_REG_SET(DISP_REG_CONFIG_MUTEX_MOD(gOvlWdmaMutexID), 0x48);
	DISP_REG_SET(DISP_REG_CONFIG_MUTEX_SOF(gOvlWdmaMutexID), 0);
	/* disp_dump_reg(DISP_MODULE_MUTEX); */
	MMProfileLogEx(MTKFB_MMP_Events.indirect_link_overlay, MMProfileFlagEnd, 0, 0);

	pr_notice("indirect link overlay leave\n");

	return 0;
}

void disp_ovl_engine_set_overlay_to_buffer(void)
{
	MMProfileLogEx(MTKFB_MMP_Events.set_overlay_to_buffer, MMProfileFlagPulse, 0, 0);

	if (disp_ovl_engine.bCouple)
		disp_path_config_mem_out(&disp_ovl_engine_params.MemOutConfig);
}

int disp_ovl_engine_trigger_hw_overlay_couple(void)
{
	struct disp_path_config_mem_out_struct rMemOutConfig = { 0 };
	unsigned int temp_va = 0;
	unsigned int width, height, bpp;
	unsigned int layer_id;
	int tmp;
	int OvlSecureNew = 0;

	MMProfileLogEx(MTKFB_MMP_Events.trigger_hw_overlay_couple, MMProfileFlagPulse, 0, 0);
	DISP_OVL_ENGINE_DBG("disp_ovl_engine_trigger_hw_overlay_couple entry\n");

	/* overlay output to internal buffer */
	if (atomic_read(&disp_ovl_engine_params.OverlaySettingDirtyFlag)) {
		/* update OvlWrIdx except screen capture case */
		if (disp_ovl_engine_params.MemOutConfig.dirty == 0) {
			tmp = disp_ovl_engine.OvlWrIdx + 1;
			tmp %= OVL_ENGINE_OVL_BUFFER_NUMBER;
			if (tmp == disp_ovl_engine.RdmaRdIdx) {
				DISP_OVL_ENGINE_ERR
				    ("OVL BuffCtl RDMA hang (%d), stop write (ovlWrIdx: %d)\n",
				     disp_ovl_engine.RdmaRdIdx, disp_ovl_engine.OvlWrIdx);
				disp_ovl_engine.OvlWrIdx =
				    (disp_ovl_engine.OvlWrIdx + OVL_ENGINE_OVL_BUFFER_NUMBER - 1)
				    % OVL_ENGINE_OVL_BUFFER_NUMBER;
				DISP_OVL_ENGINE_ERR("OVL BuffCtl new WrIdx: %d\n",
						    disp_ovl_engine.OvlWrIdx);

				MMProfileLogEx(MTKFB_MMP_Events.OVLEngine_rdma_hang,
					       MMProfileFlagPulse, disp_ovl_engine.OvlWrIdx,
					       disp_ovl_engine.RdmaRdIdx);
			} else
				disp_ovl_engine.OvlWrIdx = tmp;

			DISP_OVL_ENGINE_DBG("OVL BuffCtl disp_ovl_engine.OvlWrIdx: %d\n",
					    disp_ovl_engine.OvlWrIdx);
		}

		width = DISP_GetScreenWidth();
		height = DISP_GetScreenHeight();
		bpp = 3;	/* (DISP_GetScreenBpp() + 7) >> 3; */

		/* OVLReset(); */
		/* WDMAReset(1); */

		for (layer_id = 0; layer_id < DDP_OVL_LAYER_MUN; layer_id++)
			if ((disp_ovl_engine_params.cached_layer_config[layer_id].layer_en) &&
			    (disp_ovl_engine_params.cached_layer_config[layer_id].security ==
			     OVL_LAYER_SECURE_BUFFER))
				OvlSecureNew = 1;

		if (OvlSecure != OvlSecureNew)
			if (OvlSecureNew) {
				OvlSecure = OvlSecureNew;

				DISP_OVL_ENGINE_ERR
				    ("disp_ovl_engine_trigger_hw_overlay_couple, OvlSecure=0x%x\n",
				     OvlSecure);

				disp_register_intr(disp_dev.irq[DISP_REG_WDMA], OvlSecure);
			}
#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
		/* Allocate or free secure buffer */
		if (disp_ovl_engine.OvlBufSecurity[disp_ovl_engine.OvlWrIdx] != OvlSecureNew) {
			if (OvlSecureNew) {
				/* Allocate secure buffer */
				disp_ovl_engine.OvlBufAddr[disp_ovl_engine.OvlWrIdx] =
				    (unsigned int)disp_ovl_engine_hw_allocate_secure_memory(width *
											    height *
											    bpp);
				disp_ovl_engine.OvlBufSecurity[disp_ovl_engine.OvlWrIdx] = true;
			} else {
				/* Free secure buffer */
				disp_ovl_engine_hw_free_secure_memory((void
								       *)(disp_ovl_engine.OvlBufAddr
									  [disp_ovl_engine.OvlWrIdx]));
				disp_ovl_engine.OvlBufAddr[disp_ovl_engine.OvlWrIdx] =
				    disp_ovl_engine.Ovlmva +
				    (width * height * bpp) * disp_ovl_engine.OvlWrIdx;
				disp_ovl_engine.OvlBufSecurity[disp_ovl_engine.OvlWrIdx] = false;
			}

			DISP_OVL_ENGINE_ERR("OvlBufSecurity[%d] = %d\n", disp_ovl_engine.OvlWrIdx,
					    OvlSecureNew);
		}
#endif

		disp_path_get_mutex_(gOvlWdmaMutexID);
		disp_path_config_OVL_WDMA_path(gOvlWdmaMutexID);

		if (disp_ovl_engine_params.MemOutConfig.dirty)
			disp_path_config_mem_out(&disp_ovl_engine_params.MemOutConfig);
		else {
			temp_va = disp_ovl_engine.OvlBufAddr[disp_ovl_engine.OvlWrIdx];
			rMemOutConfig.dirty = true;
			rMemOutConfig.dstAddr = temp_va;
			rMemOutConfig.enable = true;
			rMemOutConfig.outFormat = eRGB888;
			rMemOutConfig.srcROI.x = 0;
			rMemOutConfig.srcROI.y = 0;
			rMemOutConfig.srcROI.width = width;
			rMemOutConfig.srcROI.height = height;
			rMemOutConfig.security = OvlSecureNew;
			disp_path_config_OVL_WDMA(&rMemOutConfig, OvlSecure);
			DISP_OVL_ENGINE_DBG
			    ("OVL BuffCtl disp_ovl_engine.OvlWrIdx: %d; ovl output addr=0x%08x\n",
			     disp_ovl_engine.OvlWrIdx, temp_va);
		}

		for (layer_id = 0; layer_id < DDP_OVL_LAYER_MUN; layer_id++) {
			disp_ovl_engine_params.cached_layer_config[layer_id].layer = layer_id;
			disp_path_config_layer_ovl_engine(&
							  (disp_ovl_engine_params.cached_layer_config
							   [layer_id]), OvlSecure);
		}

		disp_path_release_mutex_(gOvlWdmaMutexID);
		disp_path_wait_reg_update(gOvlWdmaMutexID);
		MMProfileLogEx(MTKFB_MMP_Events.OVLEngine_ovlwdma_status, MMProfileFlagStart,
			       disp_ovl_engine.OvlWrIdx,
			       disp_ovl_engine.OvlBufAddr[disp_ovl_engine.OvlWrIdx]);

		if (OvlSecure != OvlSecureNew) {
			if (OvlSecureNew)
				MMProfileLogEx(MTKFB_MMP_Events.OVL_WDMA_ADDR_InTEE,
					       MMProfileFlagStart, 0, 0);
			else
				MMProfileLogEx(MTKFB_MMP_Events.OVL_WDMA_ADDR_InTEE,
					       MMProfileFlagEnd, 0, 0);

			if (!OvlSecureNew) {
				OvlSecure = OvlSecureNew;

				DISP_OVL_ENGINE_ERR
				    ("disp_ovl_engine_trigger_hw_overlay_couple, OvlSecure=0x%x\n",
				     OvlSecure);

				disp_register_intr(disp_dev.irq[DISP_REG_WDMA], OvlSecure);
			}
		}
	}

	DISP_OVL_ENGINE_DBG("disp_ovl_engine_trigger_hw_overlay_couple leave\n");

	return 0;
}

#ifdef MTK_SEC_VIDEO_PATH_SUPPORT	/* fix warning */
static int RdmaSecure;
#endif

int disp_ovl_engine_update_rdma0(void)
{
	/* Update RDMA */
	unsigned int rdma_buffer_addr = disp_ovl_engine.OvlBufAddr[disp_ovl_engine.RdmaRdIdx];
	static unsigned int cur_rdma_idx;

	/* DISP_OVL_ENGINE_DBG("RDMA BuffCtl disp_ovl_engine.RdmaRdIdx=%d  rdma_buffer_addr=0x%08x", */
	/* cur_rdma_idx,rdma_buffer_addr); */

	/* skip to config repeat buffer */
	if (cur_rdma_idx == disp_ovl_engine.RdmaRdIdx)
		return 0;

	cur_rdma_idx = disp_ovl_engine.RdmaRdIdx;

#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
	/* Switch REE to TEE */
	int rdma_cur_secure = disp_ovl_engine.OvlBufSecurity[disp_ovl_engine.RdmaRdIdx];

	if (rdma_cur_secure != RdmaSecure)
		if (rdma_cur_secure) {
			RdmaSecure = rdma_cur_secure;

			disp_register_intr(disp_dev.irq[DISP_REG_RDMA0], RdmaSecure);

			DISP_OVL_ENGINE_ERR("RdmaSecure = %d, switch to TEE\n", RdmaSecure);
		}
#endif

	disp_path_get_mutex();
	DISP_OVL_ENGINE_INFO("rdma0 config: addr0x%x\n", rdma_buffer_addr);
#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
	if (RdmaSecure) {
		MTEEC_PARAM param[4];
		unsigned int paramTypes;
		TZ_RESULT ret;

		param[0].value.a = (uint32_t) rdma_buffer_addr;
		param[1].value.a = rdma_cur_secure;
		param[2].value.a = DISP_GetScreenWidth() * DISP_GetScreenHeight() * 3;
		paramTypes = TZ_ParamTypes3(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
		DISP_OVL_ENGINE_DBG("Rdma config handle=0x%x\n", param[0].value.a);

		ret =
		    KREE_TeeServiceCall(ddp_session_handle(), TZCMD_DDP_RDMA_ADDR_CONFIG,
					paramTypes, param);
		if (ret != TZ_RESULT_SUCCESS)
			DISP_OVL_ENGINE_ERR("TZCMD_DDP_RDMA_ADDR_CONFIG fail, ret=%d\n", ret);
	} else
#endif
	{
		DISP_REG_SET(DISP_REG_RDMA_MEM_START_ADDR, rdma_buffer_addr);
	}

	disp_path_release_mutex();

#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
	/* Switch TEE to REE */
	if (rdma_cur_secure != RdmaSecure) {
		if (!rdma_cur_secure) {
			RdmaSecure = rdma_cur_secure;
			disp_register_intr(disp_dev.irq[DISP_REG_RDMA0], RdmaSecure);

			DISP_OVL_ENGINE_ERR("RdmaSecure = %d, switch to REE\n", RdmaSecure);
		}
	}
#endif

	/* DISP_OVL_ENGINE_DBG("OVL BuffCtl RdmaRdIdx: 0x%x Addr: 0x%x\n", cur_rdma_idx, rdma_buffer_addr); */

	return 0;
}

int dump_all_info(void)
{
	int i;

	DISP_OVL_ENGINE_INFO
	    ("dump_all_info ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

	DISP_OVL_ENGINE_INFO
	    ("disp_ovl_engine:\nbInit %d bCouple %d bModeSwitch %d Ovlmva 0x%x OvlWrIdx %d RdmaRdIdx %d\n",
	     disp_ovl_engine.bInit, disp_ovl_engine.bCouple, disp_ovl_engine.bModeSwitch,
	     disp_ovl_engine.Ovlmva, disp_ovl_engine.OvlWrIdx, disp_ovl_engine.RdmaRdIdx);

	MMProfileLogEx(MTKFB_MMP_Events.dumpallinfo, MMProfileFlagPulse, 0, 0);

	for (i = 0; i < 2; i++) {
		DISP_OVL_ENGINE_INFO
		    ("disp_ovl_engine.Instance[%d]:\nindex %d bUsed %d mode %d status %d\n",
		     i, disp_ovl_engine.Instance[i].index, disp_ovl_engine.Instance[i].bUsed,
		     disp_ovl_engine.Instance[i].mode, disp_ovl_engine.Instance[i].status);
		DISP_OVL_ENGINE_INFO
		    ("OverlaySettingDirtyFlag %d OverlaySettingApplied %d fgNeedConfigM4U %d\n",
		     *(unsigned int *)(&disp_ovl_engine.Instance[i].OverlaySettingDirtyFlag),
		     *(unsigned int *)(&disp_ovl_engine.Instance[i].OverlaySettingApplied),
		     (unsigned int)disp_ovl_engine.Instance[i].fgNeedConfigM4U);

		DISP_OVL_ENGINE_INFO("disp_ovl_engine.Instance[%d].cached_layer_config:\n", i);
		DISP_OVL_ENGINE_INFO
		    ("	layer %d layer_en %d source %d addr 0x%x vaddr 0x%x fmt %d\n",
		     disp_ovl_engine.Instance[i].cached_layer_config[3].layer,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].layer_en,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].source,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].addr,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].vaddr,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].fmt);

		DISP_OVL_ENGINE_INFO
		    ("	src_x %d src_y %d src_w %d src_h %d src_pitch %d\n",
		     disp_ovl_engine.Instance[i].cached_layer_config[3].src_x,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].src_y,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].src_w,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].src_h,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].src_pitch);
		DISP_OVL_ENGINE_INFO
		    ("	dst_x %d dst_y %d dst_w %d dst_h %d isDirty %d security %d\n",
		     disp_ovl_engine.Instance[i].cached_layer_config[3].dst_x,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].dst_y,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].dst_w,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].dst_h,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].isDirty,
		     disp_ovl_engine.Instance[i].cached_layer_config[3].security);
	}



	/* disp_dump_reg(DISP_MODULE_OVL); */
	/* disp_dump_reg(DISP_MODULE_MUTEX); */
	/* disp_dump_reg(DISP_MODULE_WDMA1); */
	/* disp_dump_reg(DISP_MODULE_RDMA0); */

	DISP_OVL_ENGINE_INFO
	    ("dump_all_info end --------------------------------------------------------\n");

	return 0;
}

unsigned int gDumpOVLEnable = 0;
void disp_ovl_engine_dump_ovl(int idx)
{
	MMP_MetaDataBitmap_t Bitmap;
	MMP_MetaData_t meta;

	if (gDumpOVLEnable) {
		pr_notice("[OVL] start to dump OVL->WDMA output, instance%d\n", idx);
		if (idx == 0) {
			Bitmap.data1 = idx;
			Bitmap.data2 = disp_ovl_engine.OvlWrIdx;
			Bitmap.width = DISP_GetScreenWidth();
			Bitmap.height = DISP_GetScreenHeight();
			Bitmap.format = MMProfileBitmapBGR888;
			Bitmap.start_pos = 0;
			Bitmap.bpp = 24;
			Bitmap.pitch = DISP_GetScreenWidth()*3;
			Bitmap.data_size = Bitmap.pitch * Bitmap.height;
			Bitmap.down_sample_x = gCaptureOvlDownX;
			Bitmap.down_sample_y = gCaptureOvlDownY;
			Bitmap.pData = (void *)disp_ovl_engine.OvlBufAddr_va[disp_ovl_engine.RdmaRdIdx];

			MMProfileLogMetaBitmap(MTKFB_MMP_Events.OvlDump, MMProfileFlagPulse, &Bitmap);
		} else {
			unsigned int buffer_size;
			unsigned int raw = 0;

			Bitmap.data1 = idx;
			Bitmap.data2 = disp_ovl_engine.OvlWrIdx;
			Bitmap.width = disp_ovl_engine_params.MemOutConfig.srcROI.width;
			Bitmap.height = disp_ovl_engine_params.MemOutConfig.srcROI.height;
			switch (disp_ovl_engine_params.MemOutConfig.outFormat) {
			case eRGB565:
			case eBGR565:
				Bitmap.format = MMProfileBitmapRGB565; Bitmap.bpp = 16; break;
			case eRGB888:
				Bitmap.format = MMProfileBitmapBGR888; Bitmap.bpp = 24; break;
			case eARGB8888:
			case ePARGB8888:
			  Bitmap.format = MMProfileBitmapBGRA8888; Bitmap.bpp = 32; break;
			case eBGR888:
				Bitmap.format = MMProfileBitmapRGB888; Bitmap.bpp = 24; break;
			case eABGR8888:
			case ePABGR8888:
			  Bitmap.format = MMProfileBitmapRGBA8888; Bitmap.bpp = 32; break;
			case eUYVY:
			  raw = 1; Bitmap.bpp = 16; break;
			case eYUV_420_3P:
			  raw = 1; Bitmap.bpp = 12; break;
			default:
				pr_err("error: _DISP_DumpLayer(), unknown format=0x%x, dump raw\n",
					disp_ovl_engine_params.MemOutConfig.outFormat);
				raw = 1;
			}
			Bitmap.start_pos = 0;
			Bitmap.pitch = Bitmap.width*Bitmap.bpp/8;
			Bitmap.data_size = Bitmap.pitch * Bitmap.height;
			Bitmap.down_sample_x = gCaptureOvlDownX;
			Bitmap.down_sample_y = gCaptureOvlDownY;
			Bitmap.pData = (void *)disp_ovl_engine_params.MemOutConfig.dstAddr;

			buffer_size = Bitmap.data_size;
			if (buffer_size != 0) {
				disphal_dma_map_kernel(disp_ovl_engine_params.MemOutConfig.dstAddr,
					buffer_size, (unsigned int *)&Bitmap.pData, &Bitmap.data_size);

				if (Bitmap.data_size != 0 && !raw)
					MMProfileLogMetaBitmap(MTKFB_MMP_Events.OvlDump, MMProfileFlagPulse, &Bitmap);
				else if (Bitmap.data_size != 0) {
					meta.data1 = idx;
					meta.data2 = disp_ovl_engine_params.MemOutConfig.outFormat;
					meta.data_type = MMProfileMetaRaw;
					meta.pData = (void *)Bitmap.pData;
					meta.size = Bitmap.data_size;
					MMProfileLogMeta(MTKFB_MMP_Events.OvlDump, MMProfileFlagPulse, &meta);
				}

				disphal_dma_unmap_kernel(disp_ovl_engine_params.MemOutConfig.dstAddr,
					Bitmap.data_size, (unsigned int)Bitmap.pData);
			}
		}
		pr_notice("[OVL] dump end\n");
	}
}

void disp_ovl_engine_trigger_hw_overlay(void)
{
	/* unsigned int layer_en_backup[DDP_OVL_LAYER_MUN] = {0}; //[mo] */
	MMProfileLogEx(MTKFB_MMP_Events.trigger_hw_overlay, MMProfileFlagStart, 0, 0);

	disp_path_config_layer_ovl_engine_control(true);

	/* decouple mode */
	DISP_OVL_ENGINE_DBG(" decouple mode\n");
	if (COUPLE_MODE == disp_ovl_engine_params.mode) {
		DISP_OVL_ENGINE_DBG(" couple instance\n");
		disp_ovl_engine_trigger_hw_overlay_couple();
	} else {
		DISP_OVL_ENGINE_DBG(" decouple instance\n");
		disp_ovl_engine_trigger_hw_overlay_decouple();
	}

	MMProfileLogEx(MTKFB_MMP_Events.trigger_hw_overlay, MMProfileFlagEnd, 0, 0);

}

void disp_ovl_engine_hw_register_irq(void (*irq_callback) (unsigned int param))
{
	disp_ovl_engine_hw_irq_callback = irq_callback;
}


int disp_ovl_engine_hw_mva_map(struct disp_mva_map *mva_map_struct)
{
	MMProfileLogEx(MTKFB_MMP_Events.hw_mva_map, MMProfileFlagPulse, 0, 0);
#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
	MTEEC_PARAM param[4];
	unsigned int paramTypes;
	TZ_RESULT ret;

	param[0].value.a = mva_map_struct->module;
	param[1].value.a = mva_map_struct->cache_coherent;
	param[2].value.a = mva_map_struct->addr;
	param[3].value.a = mva_map_struct->size;
	paramTypes =
	    TZ_ParamTypes4(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
	ret =
	    KREE_TeeServiceCall(ddp_session_handle(), TZCMD_DDP_SECURE_MVA_MAP, paramTypes, param);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_OVL_ENGINE_ERR("KREE_TeeServiceCall(TZCMD_DDP_SECURE_MVA_MAP) fail, ret=%d\n",
				    ret);

		return -1;
	}
#endif
	return 0;
}



int disp_ovl_engine_hw_mva_unmap(struct disp_mva_map *mva_map_struct)
{
	MMProfileLogEx(MTKFB_MMP_Events.hw_mva_unmap, MMProfileFlagPulse, 0, 0);
#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
	MTEEC_PARAM param[4];
	unsigned int paramTypes;
	TZ_RESULT ret;

	param[0].value.a = mva_map_struct->module;
	param[1].value.a = mva_map_struct->cache_coherent;
	param[2].value.a = mva_map_struct->addr;
	param[3].value.a = mva_map_struct->size;
	paramTypes =
	    TZ_ParamTypes4(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
	ret =
	    KREE_TeeServiceCall(ddp_session_handle(), TZCMD_DDP_SECURE_MVA_UNMAP, paramTypes,
				param);
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_OVL_ENGINE_ERR
		    ("KREE_TeeServiceCall(TZCMD_DDP_SECURE_MVA_UNMAP) fail, ret=%d\n", ret);

		return -1;
	}
#endif
	return 0;
}

int disp_ovl_engine_hw_reset(void)
{
	MMProfileLogEx(MTKFB_MMP_Events.hw_reset, MMProfileFlagPulse, 0, 0);

	if (disp_ovl_engine.bCouple) {
		OVLReset();
		RDMAReset(0);
	} else {
		OVLReset();
		WDMAReset(1);
	}
	return OVL_OK;
}

#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
static KREE_SESSION_HANDLE disp_ovl_engine_secure_memory_session;
KREE_SESSION_HANDLE disp_ovl_engine_secure_memory_session_handle(void)
{
	DISP_OVL_ENGINE_DBG("disp_ovl_engine_secure_memory_session_handle() acquire TEE session\n");
	/* TODO: the race condition here is not taken into consideration. */
	if (NULL == disp_ovl_engine_secure_memory_session) {
		TZ_RESULT ret;

		DISP_OVL_ENGINE_DBG
		    ("disp_ovl_engine_secure_memory_session_handle() create session\n");
		ret = KREE_CreateSession(TZ_TA_MEM_UUID, &disp_ovl_engine_secure_memory_session);
		if (ret != TZ_RESULT_SUCCESS) {
			DISP_OVL_ENGINE_ERR("KREE_CreateSession fail, ret=%d\n", ret);
			return NULL;
		}
	}

	DISP_OVL_ENGINE_DBG("disp_ovl_engine_secure_memory_session_handle() session=%x\n",
			    (unsigned int)disp_ovl_engine_secure_memory_session);
	return disp_ovl_engine_secure_memory_session;
}


void *disp_ovl_engine_hw_allocate_secure_memory(int size)
{
	KREE_SECUREMEM_HANDLE mem_handle;
	TZ_RESULT ret;
	struct disp_mva_map mva_map_struct;

	/* Allocate */
	ret = KREE_AllocSecurechunkmemWithTag(disp_ovl_engine_secure_memory_session_handle(),
				       &mem_handle, 0, size, "main_path");
	if (ret != TZ_RESULT_SUCCESS) {
		DISP_OVL_ENGINE_ERR("KREE_AllocSecurechunkmemWithTag fail, ret=%d\n", ret);
		return NULL;
	}

	DISP_OVL_ENGINE_DBG("KREE_AllocSecurechunkmemWithTag handle=0x%x\n", mem_handle);

	/* Map mva */
	mva_map_struct.addr = (unsigned int)mem_handle;
	mva_map_struct.size = size;
	mva_map_struct.cache_coherent = 0;
	mva_map_struct.module = DISP_RDMA;
	disp_ovl_engine_hw_mva_map(&mva_map_struct);


	return (void *)mem_handle;
}


void disp_ovl_engine_hw_free_secure_memory(void *mem_handle)
{
	TZ_RESULT ret;
	struct disp_mva_map mva_map_struct;

	/* Unmap mva */
	mva_map_struct.addr = (unsigned int)mem_handle;
	mva_map_struct.size = 0;
	mva_map_struct.cache_coherent = 0;
	mva_map_struct.module = DISP_RDMA;
	disp_ovl_engine_hw_mva_unmap(&mva_map_struct);

	/* Free */
	ret = KREE_UnreferenceSecurechunkmem(disp_ovl_engine_secure_memory_session_handle(),
					     (KREE_SECUREMEM_HANDLE) mem_handle);

	DISP_OVL_ENGINE_DBG("KREE_UnreferenceSecurechunkmem handle=0x%0x\n",
			    (unsigned int)mem_handle);

	if (ret != TZ_RESULT_SUCCESS)
		DISP_OVL_ENGINE_ERR("KREE_UnreferenceSecurechunkmem fail, ret=%d\n", ret);
}
#endif
