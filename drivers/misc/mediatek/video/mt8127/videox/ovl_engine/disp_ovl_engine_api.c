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

#include <asm/atomic.h>
#include <linux/file.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include "disp_ovl_engine_api.h"
#include "disp_ovl_engine_core.h"
#include "disp_ovl_engine_hw.h"
#include "ddp_ovl.h"
#include "disp_debug.h"
#include "ddp_debug.h"

int Disp_Ovl_Engine_GetInstance(DISP_OVL_ENGINE_INSTANCE_HANDLE *pHandle, OVL_INSTANCE_MODE mode)
{
	unsigned int i = 0;
	unsigned int j = 0;

	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_GetInstance\n");
	MMProfileLogEx(MTKFB_MMP_Events.GetInstance, MMProfileFlagStart, *pHandle, mode);

	if (down_interruptible(&disp_ovl_engine_semaphore))
		return -1;

	for (i = 0; i < OVL_ENGINE_INSTANCE_MAX_INDEX; i++) {
		if (!disp_ovl_engine.Instance[i].bUsed) {
			*pHandle = i;
			disp_ovl_engine.Instance[i].bUsed = true;
			disp_ovl_engine.Instance[i].mode = mode;
			disp_ovl_engine.Instance[i].fgNeedConfigM4U = true;
			for (j = 0; j < DDP_OVL_LAYER_MUN; j++) {
				memset(&disp_ovl_engine.Instance[i].cached_layer_config[j], 0,
				       sizeof(OVL_CONFIG_STRUCT));
				disp_ovl_engine.Instance[i].cached_layer_config[j].layer = j;
			}

			break;
		}
	}

	up(&disp_ovl_engine_semaphore);

	if (OVL_ENGINE_INSTANCE_MAX_INDEX == i) {
		*pHandle = 0xFF;
		MMProfileLogEx(MTKFB_MMP_Events.GetInstance, MMProfileFlagEnd, *pHandle,
			       disp_ovl_engine.bModeSwitch);
		return OVL_ERROR;	/* invalid instance id */
	}

	/* disp_ovl_engine_hw_change_mode(mode); */
	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_GetInstance success %d\n", *pHandle);

	MMProfileLogEx(MTKFB_MMP_Events.GetInstance, MMProfileFlagEnd, *pHandle,
		       disp_ovl_engine.bModeSwitch);
	return OVL_OK;
}


int Disp_Ovl_Engine_ReleaseInstance(DISP_OVL_ENGINE_INSTANCE_HANDLE handle)
{
	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_ReleaseInstance %d\n", handle);
	MMProfileLogEx(MTKFB_MMP_Events.ReleaseInstance, MMProfileFlagStart, handle, 0);

	/* invalid handle */
	if (handle >= OVL_ENGINE_INSTANCE_MAX_INDEX)
		goto ReleaseInstanceFail;
	if (down_interruptible(&disp_ovl_engine_semaphore))
		goto ReleaseInstanceFail;

	if (OVERLAY_STATUS_IDLE != disp_ovl_engine.Instance[handle].status) {
		up(&disp_ovl_engine_semaphore);

		DISP_OVL_ENGINE_ERR
		    ("Disp_Ovl_Engine_ReleaseInstance try to free a working instance\n");
		goto ReleaseInstanceFail;
	}

	memset(&disp_ovl_engine.Instance[handle], 0, sizeof(DISP_OVL_ENGINE_INSTANCE));
	disp_ovl_engine.Instance[handle].bUsed = false;

	up(&disp_ovl_engine_semaphore);

	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_ReleaseInstance finish, bModeSwitch: %d\n",
			     disp_ovl_engine.bModeSwitch);
	MMProfileLogEx(MTKFB_MMP_Events.ReleaseInstance, MMProfileFlagEnd, handle,
		       disp_ovl_engine.bModeSwitch);

	return 0;

ReleaseInstanceFail:
	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_ReleaseInstance fail, bModeSwitch: %d\n",
			     disp_ovl_engine.bModeSwitch);
	MMProfileLogEx(MTKFB_MMP_Events.ReleaseInstance, MMProfileFlagEnd, handle, OVL_ERROR);
	return OVL_ERROR;
}

int Disp_Ovl_Engine_Check_WFD_Instance(void)
{
	int i, ret = 0;

	if (down_interruptible(&disp_ovl_engine_semaphore))
		return -1;

	for (i = 0; i < OVL_ENGINE_INSTANCE_MAX_INDEX; i++) {
		if (disp_ovl_engine.Instance[i].bUsed &&
			disp_ovl_engine.Instance[i].mode == DECOUPLE_MODE
#ifdef CONFIG_MTK_HDMI_SUPPORT
			&& !is_hdmi_active()
#endif
			) {
			DISP_OVL_ENGINE_ERR("wfd is connect, suspend will not stop display\n");
			ret = 1;
		}
	}

	up(&disp_ovl_engine_semaphore);

	return ret;
}

int Disp_Ovl_Engine_Get_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
				   struct fb_overlay_layer *layerInfo)
{
	unsigned int layerpitch;
	OVL_CONFIG_STRUCT *ovl_config_tmp =
	    &disp_ovl_engine.Instance[handle].cached_layer_config[layerInfo->layer_id];

	if (handle >= OVL_ENGINE_INSTANCE_MAX_INDEX)
		return OVL_ERROR;

	DISP_OVL_ENGINE_DBG("Disp_Ovl_Engine_Get_layer_info\n");
	MMProfileLog(MTKFB_MMP_Events.Get_layer_info, MMProfileFlagStart);

	/* Todo, check layer id is valid. */

	if (down_interruptible(&disp_ovl_engine_semaphore))
		return -1;

	layerInfo->layer_enable = ovl_config_tmp->layer_en;
	layerInfo->connected_type = ovl_config_tmp->connected_type;
	layerInfo->identity = ovl_config_tmp->identity;
	layerInfo->isTdshp = ovl_config_tmp->isTdshp;
	layerInfo->layer_rotation = MTK_FB_ORIENTATION_0;
	layerInfo->layer_type = LAYER_2D;
	layerInfo->next_buff_idx = ovl_config_tmp->buff_idx;
	layerInfo->security = ovl_config_tmp->security;
	layerInfo->src_base_addr = (void *)(ovl_config_tmp->vaddr);
	layerInfo->src_color_key = ovl_config_tmp->key;
	layerInfo->src_direct_link = 0;
	switch (ovl_config_tmp->fmt) {
	case eYUY2:
		layerInfo->src_fmt = MTK_FB_FORMAT_YUV422;
		layerpitch = 2;
		break;
	case eRGB565:
		layerInfo->src_fmt = MTK_FB_FORMAT_RGB565;
		layerpitch = 2;
		break;
	case eRGB888:
		layerInfo->src_fmt = MTK_FB_FORMAT_RGB888;
		layerpitch = 3;
		break;
	case eBGR888:
		layerInfo->src_fmt = MTK_FB_FORMAT_BGR888;
		layerpitch = 3;
		break;
	case ePARGB8888:
		layerInfo->src_fmt = MTK_FB_FORMAT_ARGB8888;
		layerpitch = 4;
		break;
	case ePABGR8888:
		layerInfo->src_fmt = MTK_FB_FORMAT_ABGR8888;
		layerpitch = 4;
		break;
	case eARGB8888:
		layerInfo->src_fmt = MTK_FB_FORMAT_XRGB8888;
		layerpitch = 4;
		break;
	case eABGR8888:
		layerInfo->src_fmt = MTK_FB_FORMAT_XBGR8888;
		layerpitch = 4;
		break;
	default:
		layerInfo->src_fmt = MTK_FB_FORMAT_UNKNOWN;
		layerpitch = 1;
		break;
	}

	layerInfo->src_width = ovl_config_tmp->src_w;
	layerInfo->src_height = ovl_config_tmp->src_h;
	layerInfo->src_offset_x = ovl_config_tmp->src_x;
	layerInfo->src_offset_y = ovl_config_tmp->src_y;
	layerInfo->src_phy_addr = (void *)(ovl_config_tmp->addr);
	layerInfo->src_pitch = ovl_config_tmp->src_pitch / layerpitch;
	layerInfo->src_use_color_key = ovl_config_tmp->keyEn;
	layerInfo->tgt_offset_x = ovl_config_tmp->dst_x;
	layerInfo->tgt_offset_y = ovl_config_tmp->dst_y;
	layerInfo->tgt_width = ovl_config_tmp->dst_w;
	layerInfo->tgt_height = ovl_config_tmp->dst_h;
	layerInfo->video_rotation = MTK_FB_ORIENTATION_0;
	up(&disp_ovl_engine_semaphore);

	MMProfileLogStructure(MTKFB_MMP_Events.Get_layer_info, MMProfileFlagEnd, layerInfo,
			      struct fb_overlay_layer);

	return 0;
}


int Disp_Ovl_Engine_Set_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
				   struct fb_overlay_layer *layerInfo)
{
	/* Todo, check layer id is valid. */
	unsigned int fmt;
	unsigned int layerpitch;
	unsigned int layerbpp;
	OVL_CONFIG_STRUCT *ovl_config_tmp =
	    &disp_ovl_engine.Instance[handle].cached_layer_config[layerInfo->layer_id];

	if (handle >= OVL_ENGINE_INSTANCE_MAX_INDEX)
		return OVL_ERROR;

	DISP_OVL_ENGINE_DBG("Disp_Ovl_Engine_Set_layer_info %d\n", handle);
	MMProfileLogStructure(MTKFB_MMP_Events.Set_layer_info, MMProfileFlagStart, layerInfo,
			      struct fb_overlay_layer);

	if (down_interruptible(&disp_ovl_engine_semaphore))
		return OVL_ERROR;

	if (layerInfo->layer_enable) {

		switch (layerInfo->src_fmt) {
		case MTK_FB_FORMAT_YUV422:
			fmt = eYUY2;
			layerpitch = 2;
			layerbpp = 24;
			break;

		case MTK_FB_FORMAT_RGB565:
			fmt = eRGB565;
			layerpitch = 2;
			layerbpp = 16;
			break;

		case MTK_FB_FORMAT_RGB888:
			fmt = eRGB888;
			layerpitch = 3;
			layerbpp = 24;
			break;
		case MTK_FB_FORMAT_BGR888:
			fmt = eBGR888;
			layerpitch = 3;
			layerbpp = 24;
			break;

		case MTK_FB_FORMAT_ARGB8888:
			fmt = ePARGB8888;
			layerpitch = 4;
			layerbpp = 32;
			break;
		case MTK_FB_FORMAT_ABGR8888:
			fmt = ePABGR8888;
			layerpitch = 4;
			layerbpp = 32;
			break;
		case MTK_FB_FORMAT_XRGB8888:
			fmt = eARGB8888;
			layerpitch = 4;
			layerbpp = 32;
			break;
		case MTK_FB_FORMAT_XBGR8888:
			fmt = eABGR8888;
			layerpitch = 4;
			layerbpp = 32;
			break;
		default:
			DISP_OVL_ENGINE_INFO
			    ("Disp_Ovl_Engine_Set_layer_info %d, src_fmt %d does not support\n",
			     handle, layerInfo->src_fmt);
			up(&disp_ovl_engine_semaphore);
			return -1;
		}

		ovl_config_tmp->fmt = fmt;

		ovl_config_tmp->vaddr = (unsigned int)layerInfo->src_base_addr;
		ovl_config_tmp->security = layerInfo->security;
		ovl_config_tmp->addr = (unsigned int)layerInfo->src_phy_addr;
		ovl_config_tmp->isTdshp = layerInfo->isTdshp;
		ovl_config_tmp->buff_idx = layerInfo->next_buff_idx;
		ovl_config_tmp->identity = layerInfo->identity;
		ovl_config_tmp->connected_type = layerInfo->connected_type;

		/* set Alpha blending */
		ovl_config_tmp->alpha = 0xFF;
		if (layerInfo->alpha_enable) {
			ovl_config_tmp->aen = true;
			ovl_config_tmp->alpha = layerInfo->alpha;
		} else
			ovl_config_tmp->aen = false;

		if (MTK_FB_FORMAT_ARGB8888 == layerInfo->src_fmt
		    || MTK_FB_FORMAT_ABGR8888 == layerInfo->src_fmt)
			ovl_config_tmp->aen = true;

		/* after suspend/resume, should not clear assert _layer alpha setting. */
		if (isAEEEnabled && (layerInfo->layer_id == ASSERT_LAYER)) {
			ovl_config_tmp->alpha = 0x80;
			ovl_config_tmp->aen = true;
		}

		/* set src width, src height */
		ovl_config_tmp->src_x = layerInfo->src_offset_x;
		ovl_config_tmp->src_y = layerInfo->src_offset_y;
		ovl_config_tmp->src_w = layerInfo->src_width;
		ovl_config_tmp->src_h = layerInfo->src_height;
		ovl_config_tmp->dst_x = layerInfo->tgt_offset_x;
		ovl_config_tmp->dst_y = layerInfo->tgt_offset_y;
		ovl_config_tmp->dst_w = layerInfo->tgt_width;
		ovl_config_tmp->dst_h = layerInfo->tgt_height;
		if (ovl_config_tmp->dst_w > ovl_config_tmp->src_w)
			ovl_config_tmp->dst_w = ovl_config_tmp->src_w;
		if (ovl_config_tmp->dst_h > ovl_config_tmp->src_h)
			ovl_config_tmp->dst_h = ovl_config_tmp->src_h;

		ovl_config_tmp->src_pitch = layerInfo->src_pitch * layerpitch;

		/* set color key */
		ovl_config_tmp->key = layerInfo->src_color_key;
		ovl_config_tmp->keyEn = layerInfo->src_use_color_key;

	}

	ovl_config_tmp->layer_en = layerInfo->layer_enable;
	ovl_config_tmp->isDirty = true;

	/* ion handle to mva */
	do {
		ion_mm_data_t data;
		size_t _unused;
		ion_phys_addr_t mva;
		struct ion_handle *ion_handles;

		if (!layerInfo->layer_enable)
			continue;

		/* If mva has already exist, do not need process ion handle to mva */
		if (ovl_config_tmp->addr > 0)
			continue;

		/* Import ion handles so userspace (hwc) doesn't need to have a ref to them */
		if (layerInfo->ion_fd < 0) {
			ion_handles = NULL;
			continue;	/* worker will use mva from userspace */
		}
		ion_handles = ion_import_dma_buf(disp_ovl_engine.ion_client, layerInfo->ion_fd);
		if (IS_ERR(ion_handles)) {
			DISP_OVL_ENGINE_ERR("failed to import ion fd, disable ovl %d\n",
					    layerInfo->layer_id);
			ion_handles = NULL;
			continue;
		}

		if (!ion_handles)
			continue;	/* Use mva from userspace */

		/* configure buffer */
		memset(&data, 0, sizeof(ion_mm_data_t));
		data.mm_cmd = ION_MM_CONFIG_BUFFER;
		data.config_buffer_param.kernel_handle = ion_handles;
		data.config_buffer_param.eModuleID = DISP_OVL_0;
		ion_kernel_ioctl(disp_ovl_engine.ion_client, ION_CMD_MULTIMEDIA,
				 (unsigned long)&data);

		/* Get "physical" address (mva) */
		if (ion_phys(disp_ovl_engine.ion_client, ion_handles, &mva, &_unused)) {
			DISP_OVL_ENGINE_ERR("ion_phys failed, disable ovl %d\n",
					    layerInfo->layer_id);
			ovl_config_tmp->layer_en = 0;
			ovl_config_tmp->addr = 0;
			ion_free(disp_ovl_engine.ion_client, ion_handles);
			ion_handles = NULL;
			continue;
		}

		ovl_config_tmp->fgIonHandleImport = true;
		ovl_config_tmp->ion_handles  = ion_handles;
		ovl_config_tmp->addr = (unsigned int)mva;
	} while (0);

	if (layerInfo->layer_enable && (ovl_config_tmp->addr == 0)) {
		DISP_OVL_ENGINE_ERR
		    ("Invalid address for enable overlay, Instance %d, layer%d, addr%x, addr%x, ion_fd=0x%x\n",
		     handle, layerInfo->layer_id, (unsigned int)layerInfo->src_base_addr,
		     (unsigned int)layerInfo->src_phy_addr, layerInfo->ion_fd);
		ovl_config_tmp->layer_en = 0;
	}

	atomic_set(&disp_ovl_engine.Instance[handle].OverlaySettingDirtyFlag, 1);
	atomic_set(&disp_ovl_engine.Instance[handle].OverlaySettingApplied, 0);
	atomic_set(&disp_ovl_engine.Instance[handle].fgCompleted, 0);
	up(&disp_ovl_engine_semaphore);

	if (layerInfo->layer_enable) {
		DISP_OVL_ENGINE_INFO
		    ("Instance%d, layer%d en%d, next_idx=0x%x, vaddr=0x%x, paddr=0x%x, fmt=%u\n",
		     handle, layerInfo->layer_id, layerInfo->layer_enable, layerInfo->next_buff_idx,
		     (unsigned int)(layerInfo->src_base_addr), (unsigned int)(layerInfo->src_phy_addr),
		     layerInfo->src_fmt);
		DISP_OVL_ENGINE_INFO
		    ("            d-link=%u, pitch=%u, xoff=%u, yoff=%u, w=%u, h=%u\n",
		     (unsigned int)(layerInfo->src_direct_link), layerInfo->src_pitch,
		     layerInfo->src_offset_x, layerInfo->src_offset_y, layerInfo->src_width,
		     layerInfo->src_height);
		DISP_OVL_ENGINE_INFO
		    ("            alpha_en=%d, alpha=%d, fence_fd=%d, ion_fd=%d, security=%d\n",
		     layerInfo->alpha_enable, layerInfo->alpha, layerInfo->fence_fd,
		     layerInfo->ion_fd, layerInfo->security);
	}

	MMProfileLog(MTKFB_MMP_Events.Set_layer_info, MMProfileFlagEnd);
	return OVL_OK;
}

int Disp_Ovl_Engine_Get_Ovl_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
				       DISP_LAYER_INFO *layerInfo)
{

	if (handle >= OVL_ENGINE_INSTANCE_MAX_INDEX)
		return OVL_ERROR;
	/* DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_Get_Ovl_layer_info\n"); */
	MMProfileLog(MTKFB_MMP_Events.Get_Ovl_layer_info, MMProfileFlagStart);
	if (down_interruptible(&disp_ovl_engine_semaphore))
		return OVL_ERROR;

	layerInfo->curr_en = disp_ovl_engine.captured_layer_config[layerInfo->id].layer_en;
	layerInfo->next_en =
	    disp_ovl_engine.Instance[handle].cached_layer_config[layerInfo->id].layer_en;
	layerInfo->hw_en = disp_ovl_engine.realtime_layer_config[layerInfo->id].layer_en;
	layerInfo->curr_idx = disp_ovl_engine.captured_layer_config[layerInfo->id].buff_idx;
	layerInfo->next_idx =
	    disp_ovl_engine.Instance[handle].cached_layer_config[layerInfo->id].buff_idx;
	layerInfo->hw_idx = disp_ovl_engine.realtime_layer_config[layerInfo->id].buff_idx;
	layerInfo->curr_identity = disp_ovl_engine.captured_layer_config[layerInfo->id].identity;
	layerInfo->next_identity =
	    disp_ovl_engine.Instance[handle].cached_layer_config[layerInfo->id].identity;
	layerInfo->hw_identity = disp_ovl_engine.realtime_layer_config[layerInfo->id].identity;
	layerInfo->curr_conn_type =
	    disp_ovl_engine.captured_layer_config[layerInfo->id].connected_type;
	layerInfo->next_conn_type =
	    disp_ovl_engine.Instance[handle].cached_layer_config[layerInfo->id].connected_type;
	layerInfo->hw_conn_type =
	    disp_ovl_engine.realtime_layer_config[layerInfo->id].connected_type;
	up(&disp_ovl_engine_semaphore);

	MMProfileLogStructure(MTKFB_MMP_Events.Get_Ovl_layer_info, MMProfileFlagEnd, layerInfo,
			      DISP_LAYER_INFO);

	return OVL_OK;
}

int Disp_Ovl_Engine_Set_Overlayed_Buffer(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
		struct disp_ovl_engine_config_mem_out_struct *overlayedBufInfo)
{
	/* DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_Set_Overlayed_Buffer %d\n",handle); */
	/* MMProfileLogStructure(MTKFB_MMP_Events.Set_Overlayed_Buffer, MMProfileFlagStart,
	   overlayedBufInfo, struct disp_ovl_engine_config_mem_out_struct); */

	if (down_interruptible(&disp_ovl_engine_semaphore))
		return OVL_ERROR;

#ifdef CONFIG_MTK_ION
	do {
		ion_mm_data_t data;
		size_t _unused;
		ion_phys_addr_t mva;
		struct ion_handle *ion_handles;

		/* pr_err("[layeredbuf]dstAddr=0x%08x ion_fd=%d H=%d W=%d mva=0x%x\n",
		   overlayedBufInfo->dstAddr,overlayedBufInfo->ion_fd,
		   overlayedBufInfo->srcROI.height,overlayedBufInfo->srcROI.height,
		   (unsigned int)mva); */

		if (overlayedBufInfo->dstAddr > 0)
			continue;
		if (overlayedBufInfo->ion_fd < 0)
			continue;
		ion_handles =
		    ion_import_dma_buf(disp_ovl_engine.ion_client, overlayedBufInfo->ion_fd);
		if (IS_ERR(ion_handles)) {
			DISP_OVL_ENGINE_ERR("failed to import ion fd\n");
			ion_handles = NULL;
			continue;
		}

		if (!ion_handles) {
			pr_err("Use mva from userspace");
			continue;	/* Use mva from userspace */
		}

		/* configure buffer */
		memset(&data, 0, sizeof(ion_mm_data_t));
		data.mm_cmd = ION_MM_CONFIG_BUFFER;
		data.config_buffer_param.handle = (ion_user_handle_t) ion_handles;
		data.config_buffer_param.eModuleID = DISP_OVL_0;
		ion_kernel_ioctl(disp_ovl_engine.ion_client, ION_CMD_MULTIMEDIA,
				 (unsigned long)&data);

		/* Get "physical" address (mva) */
		if (ion_phys(disp_ovl_engine.ion_client, ion_handles, &mva, &_unused)) {
			DISP_OVL_ENGINE_ERR("ion_phys failed\n");
			ion_free(disp_ovl_engine.ion_client, ion_handles);
			ion_handles = NULL;
			continue;
		}
		overlayedBufInfo->dstAddr = (unsigned int)mva;
	} while (0);
#endif

	memcpy(&(disp_ovl_engine.Instance[handle].MemOutConfig), overlayedBufInfo,
	       sizeof(struct disp_ovl_engine_config_mem_out_struct));

	switch (overlayedBufInfo->outFormat) {
	case MTK_FB_FORMAT_ARGB8888:
		disp_ovl_engine.Instance[handle].MemOutConfig.outFormat = eARGB8888;
		break;
	case MTK_FB_FORMAT_ABGR8888:
		disp_ovl_engine.Instance[handle].MemOutConfig.outFormat = eABGR8888;
		break;
	case MTK_FB_FORMAT_RGB888:
		disp_ovl_engine.Instance[handle].MemOutConfig.outFormat = eRGB888;
		break;
	case MTK_FB_FORMAT_YUV422:
		disp_ovl_engine.Instance[handle].MemOutConfig.outFormat = eUYVY;
		break;
	case MTK_FB_FORMAT_YUV420_P:
		disp_ovl_engine.Instance[handle].MemOutConfig.outFormat = eYUV_420_3P;
		break;
	default:
		DISP_OVL_ENGINE_ERR("unsupport wdma format: 0x%x\n", overlayedBufInfo->outFormat);
		disp_ovl_engine.Instance[handle].MemOutConfig.outFormat = eARGB8888;
		break;
	}

	up(&disp_ovl_engine_semaphore);


	/* MMProfileLog(MTKFB_MMP_Events.Set_Overlayed_Buffer, MMProfileFlagEnd); */

	return OVL_OK;
}

int Disp_Ovl_Engine_Trigger_Overlay(DISP_OVL_ENGINE_INSTANCE_HANDLE handle)
{
	if ((handle >= OVL_ENGINE_INSTANCE_MAX_INDEX) || !disp_ovl_engine.Instance[handle].bUsed)
		return OVL_ERROR;

	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_Trigger_Overlay %d\n", handle);

	if (down_interruptible(&disp_ovl_engine_semaphore))
		return OVL_ERROR;

	MMProfileLogEx(MTKFB_MMP_Events.Trigger_Overlay, MMProfileFlagPulse, handle, 0);

	if (disp_ovl_engine.Instance[handle].status == OVERLAY_STATUS_IDLE) {
		disp_ovl_engine.Instance[handle].status = OVERLAY_STATUS_TRIGGER;
		atomic_set(&disp_ovl_engine.Instance[handle].fgCompleted, 0);
		disp_ovl_engine_wake_up_ovl_engine_thread();
		DISP_OVL_ENGINE_INFO("disp_ovl_engine_wake_up_ovl_engine_thread %d\n", handle);
	} else {
		disp_ovl_engine_wake_up_ovl_engine_thread();
		up(&disp_ovl_engine_semaphore);
		return OVL_ERROR;
	}
	up(&disp_ovl_engine_semaphore);
	return OVL_OK;
}


int Disp_Ovl_Engine_Wait_Overlay_Complete(DISP_OVL_ENGINE_INSTANCE_HANDLE handle, int timeout)
{
#define DISP_OVL_ENGINE_WAIT_COMPLETE_TIME_LOOP 10
	int wait_ret = 1;
	int loopNum = DISP_OVL_ENGINE_WAIT_COMPLETE_TIME_LOOP;	/*loop 10 times */
	int subTimeout = timeout / loopNum;

	DISP_OVL_ENGINE_INFO("Instance%d start wait overlay complete\n", handle);

	while (loopNum) {
		/*no need to wait when suspend state */
		if (mtkfb_is_suspend() || mtkfb_work_is_skip())
			break;

		wait_ret = wait_event_interruptible_timeout(disp_ovl_complete_wq,
					    atomic_read(&disp_ovl_engine.Instance[handle].fgCompleted),
					    msecs_to_jiffies(subTimeout));
		if (wait_ret != 0)
			break;
		loopNum--;
	}

	if (loopNum == 0 && wait_ret == 0) {
		DISP_OVL_ENGINE_ERR("Instance%d Error: wait overlay complete timeout (>%ds)\n",
				    handle, timeout / 1000);
		disp_dump_reg(DISP_MODULE_MUTEX);
		disp_dump_reg(DISP_MODULE_OVL);
		disp_dump_reg(DISP_MODULE_WDMA);
		disp_dump_reg(DISP_MODULE_CONFIG);
		return OVL_ERROR;
	}

	DISP_OVL_ENGINE_INFO("Instance%d: wait overlay complete success\n", handle);
	atomic_set(&disp_ovl_engine.Instance[handle].fgCompleted, 0);

	return OVL_OK;
}


int Disp_Ovl_Engine_Get_Dirty_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle, int *OverlayDirty,
				   int *MemOutDirty)
{
	if (handle >= OVL_ENGINE_INSTANCE_MAX_INDEX)
		return OVL_ERROR;

	if (down_interruptible(&disp_ovl_engine_semaphore))
		return OVL_ERROR;

	MMProfileLogEx(MTKFB_MMP_Events.Get_Dirty_info, MMProfileFlagStart, handle, 0);

	*OverlayDirty = atomic_read(&disp_ovl_engine.Instance[handle].OverlaySettingDirtyFlag);
	*MemOutDirty = disp_ovl_engine.Instance[handle].MemOutConfig.dirty;

	up(&disp_ovl_engine_semaphore);

	MMProfileLogEx(MTKFB_MMP_Events.Get_Dirty_info, MMProfileFlagEnd, *OverlayDirty,
		       *MemOutDirty);
	return OVL_OK;
}

int Disp_Ovl_Engine_Set_Path_Info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
				  struct disp_path_config_struct *info)
{
	if ((handle >= OVL_ENGINE_INSTANCE_MAX_INDEX) || !disp_ovl_engine.Instance[handle].bUsed)
		return OVL_ERROR;

	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_Set_Path_Info handle %d srcModule %d dstModule %d\n",
			     handle, info->srcModule, info->dstModule);
	MMProfileLogStructure(MTKFB_MMP_Events.Set_Path_Info, MMProfileFlagStart, info,
			      struct disp_path_config_struct);

	if (down_interruptible(&disp_ovl_engine_semaphore))
		return OVL_ERROR;
	memcpy(&disp_ovl_engine.Instance[handle].path_info, info,
	       sizeof(struct disp_path_config_struct));

	up(&disp_ovl_engine_semaphore);

	MMProfileLog(MTKFB_MMP_Events.Set_Path_Info, MMProfileFlagEnd);
	return OVL_OK;
}

int Disp_Ovl_Engine_Sync_Captured_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle)
{
	if ((handle >= OVL_ENGINE_INSTANCE_MAX_INDEX) || !disp_ovl_engine.Instance[handle].bUsed)
		return OVL_ERROR;

	MMProfileLogEx(MTKFB_MMP_Events.Sync_Captured_layer_info, MMProfileFlagStart, handle, 0);

	if (down_interruptible(&disp_ovl_engine_semaphore))
		return OVL_ERROR;
	disp_ovl_engine.layer_config_index = 1 - disp_ovl_engine.layer_config_index;
	disp_ovl_engine.captured_layer_config =
	    disp_ovl_engine.layer_config[disp_ovl_engine.layer_config_index];
	/* DISP_OVL_ENGINE_INFO("========= cached --> captured ===========\n"); */
	memcpy(disp_ovl_engine.captured_layer_config,
	       disp_ovl_engine.Instance[handle].cached_layer_config,
	       sizeof(OVL_CONFIG_STRUCT) * DDP_OVL_LAYER_MUN);
	up(&disp_ovl_engine_semaphore);

	MMProfileLogEx(MTKFB_MMP_Events.Sync_Captured_layer_info, MMProfileFlagEnd, handle, 0);
	return OVL_OK;
}

int Disp_Ovl_Engine_Sync_Realtime_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle)
{
	if ((handle >= OVL_ENGINE_INSTANCE_MAX_INDEX) || !disp_ovl_engine.Instance[handle].bUsed)
		return OVL_ERROR;

	MMProfileLogEx(MTKFB_MMP_Events.Sync_Realtime_layer_info, MMProfileFlagStart, handle, 0);

	disp_ovl_engine.realtime_layer_config = disp_ovl_engine.captured_layer_config;

	MMProfileLogEx(MTKFB_MMP_Events.Sync_Realtime_layer_info, MMProfileFlagEnd, handle, 0);
	return OVL_OK;
}

int Disp_Ovl_Engine_Dump_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
				    OVL_CONFIG_STRUCT *cached_layer,
				    OVL_CONFIG_STRUCT *captured_layer,
				    OVL_CONFIG_STRUCT *realtime_layer)
{
	if ((handle >= OVL_ENGINE_INSTANCE_MAX_INDEX) || !disp_ovl_engine.Instance[handle].bUsed)
		return OVL_ERROR;

	MMProfileLogEx(MTKFB_MMP_Events.Dump_layer_info, MMProfileFlagStart, handle, 0);

	if (down_interruptible(&disp_ovl_engine_semaphore))
		return OVL_ERROR;

	if (cached_layer != NULL)
		memcpy(cached_layer, disp_ovl_engine.Instance[handle].cached_layer_config,
		       sizeof(OVL_CONFIG_STRUCT) * DDP_OVL_LAYER_MUN);
	if (captured_layer != NULL)
		memcpy(captured_layer, disp_ovl_engine.captured_layer_config,
		       sizeof(OVL_CONFIG_STRUCT) * DDP_OVL_LAYER_MUN);
	if (realtime_layer != NULL)
		memcpy(realtime_layer, disp_ovl_engine.realtime_layer_config,
		       sizeof(OVL_CONFIG_STRUCT) * DDP_OVL_LAYER_MUN);
	up(&disp_ovl_engine_semaphore);

	MMProfileLogEx(MTKFB_MMP_Events.Dump_layer_info, MMProfileFlagEnd, handle, 0);
	return OVL_OK;
}


/* #define GET_FENCE_FD_SEPARATE */
int Disp_Ovl_Engine_Trigger_Overlay_Fence(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
					  struct disp_ovl_engine_setting *setting)
{
#ifdef DISP_OVL_ENGINE_FENCE
#ifndef GET_FENCE_FD_SEPARATE
	int fd = -1;
	struct sync_fence *fence;
	struct sync_pt *pt;
#endif
	ovls_work_t *work;

	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_Trigger_Overlay_Fence %d\n", handle);
	MMProfileLogEx(MTKFB_MMP_Events.Trigger_Overlay_Fence, MMProfileFlagStart, handle, 0);

#ifndef GET_FENCE_FD_SEPARATE
	fd = get_unused_fd();
	if (fd < 0) {
		DISP_OVL_ENGINE_ERR("could not get a file descriptor\n");
		goto err;
	}
	/* Create output fence */
	disp_ovl_engine.Instance[handle].timeline_max++;
	pt = sw_sync_pt_create(disp_ovl_engine.Instance[handle].timeline,
			       disp_ovl_engine.Instance[handle].timeline_max);
	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_Trigger_Overlay_Fence %d: fd = %d, time = %d\n",
			     handle, fd, disp_ovl_engine.Instance[handle].timeline_max);
	fence = sync_fence_create("overlay", pt);
	sync_fence_install(fence, fd);
	setting->out_fence_fd = fd;
#endif
	/* Init work */
	work = kzalloc(sizeof(ovls_work_t), GFP_KERNEL);
	if (!work) {
		DISP_OVL_ENGINE_ERR("could not allocate ovls_work_t\n");
		goto err;
	}
	INIT_WORK((struct work_struct *)work, Disp_Ovl_Engine_Trigger_Overlay_Handler);

	/* Import input fences so userspace can close (deref) them after ioctl */
	{
		int layer;

		for (layer = 0; layer < DDP_OVL_LAYER_MUN; layer++)
			if ((setting->overlay_layers[layer].layer_enable) &&
			    (setting->overlay_layers[layer].fence_fd >= 0))
				work->fences[layer] =
				    sync_fence_fdget(disp_ovl_engine.
						     Instance[handle].cached_layer_config[layer].
						     fence_fd);
			else
				work->fences[layer] = NULL;
	}

	work->instance = &(disp_ovl_engine.Instance[handle]);

	/* Copy setting */
	memcpy(&work->setting, setting, sizeof(struct disp_ovl_engine_setting));


	/* Queue work */
	queue_work(disp_ovl_engine.Instance[handle].wq, (struct work_struct *)work);

	MMProfileLogEx(MTKFB_MMP_Events.Trigger_Overlay_Fence, MMProfileFlagEnd, handle, 0);

	goto out;

err:
#ifndef GET_FENCE_FD_SEPARATE
	DISP_OVL_ENGINE_ERR("Disp_Ovl_Engine_Trigger_Overlay_Fence fd=%d failed\n", fd);
	setting->out_fence_fd = -1;
	if (fd >= 0)
		put_unused_fd(fd);
#endif

	return OVL_ERROR;

out:
	return OVL_OK;

#else
	return Disp_Ovl_Engine_Trigger_Overlay(handle);
#endif
}


int Disp_Ovl_Engine_Get_Fence_Fd(DISP_OVL_ENGINE_INSTANCE_HANDLE handle, int *fence_fd)
{
#ifdef DISP_OVL_ENGINE_FENCE
	int fd = -1;
	struct sync_fence *fence;
	struct sync_pt *pt;

	DISP_OVL_ENGINE_DBG("Disp_Ovl_Engine_Get_Fence_Fd %d\n", handle);

	if (down_interruptible(&disp_ovl_engine_semaphore))
		return OVL_ERROR;

	fd = get_unused_fd();
	if (fd < 0) {
		DISP_OVL_ENGINE_DBG("could not get a file descriptor\n");
		goto err;
	}
	/* Create output fence */
	disp_ovl_engine.Instance[handle].timeline_max++;
	pt = sw_sync_pt_create(disp_ovl_engine.Instance[handle].timeline,
			       disp_ovl_engine.Instance[handle].timeline_max);
	DISP_OVL_ENGINE_DBG("Disp_Ovl_Engine_Get_Fence_Fd %d: fd = %d, time = %d\n", handle, fd,
			    disp_ovl_engine.Instance[handle].timeline_max);
	fence = sync_fence_create("overlay", pt);
	sync_fence_install(fence, fd);
	*fence_fd = fd;
	disp_ovl_engine.Instance[handle].outFence = 1;

	MMProfileLogEx(MTKFB_MMP_Events.Get_Fence_Fd, MMProfileFlagPulse, handle, fd);

	/* printk("Disp_Ovl_Engine_Get_Fence_Fd %d: fd = %d, time = %d, timeline->value = %d\n", */
	/* handle,fd,disp_ovl_engine.Instance[handle].timeline_max, */
	/* disp_ovl_engine.Instance[handle].timeline->value); */

	goto out;

err:
	DISP_OVL_ENGINE_ERR("Disp_Ovl_Engine_Get_Fence_Fd fd=%d failed\n", fd);
	*fence_fd = -1;
	if (fd >= 0)
		put_unused_fd(fd);

	up(&disp_ovl_engine_semaphore);

	return OVL_ERROR;

out:
	up(&disp_ovl_engine_semaphore);
#endif
	return OVL_OK;
}
