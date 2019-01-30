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

#ifndef __DISP_OVL_ENGINE_API_H__
#define __DISP_OVL_ENGINE_API_H__

#include <linux/types.h>
#include "disp_ovl_engine_dev.h"
#include "disp_drv.h"
#include "mtkfb.h"


typedef enum {
	COUPLE_MODE,
	DECOUPLE_MODE,
} OVL_INSTANCE_MODE;

typedef struct {
	struct disp_path_config_struct couple_config;
	struct disp_path_config_struct decouple_config;
} OVL_CONFIG_PATH_INFO_T;


typedef int DISP_OVL_ENGINE_INSTANCE_HANDLE;

typedef struct {
	struct file *mFile;
	DISP_OVL_ENGINE_INSTANCE_HANDLE mHandle;
} DISP_OVL_ENGINE_FILE_HANDLE_MAP;

#define OVL_OK         0
#define OVL_ERROR      -1

int Disp_Ovl_Engine_GetInstance(DISP_OVL_ENGINE_INSTANCE_HANDLE *pHandle, OVL_INSTANCE_MODE mode);
int Disp_Ovl_Engine_ReleaseInstance(DISP_OVL_ENGINE_INSTANCE_HANDLE handle);
int Disp_Ovl_Engine_Get_Ovl_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
				       DISP_LAYER_INFO *layerInfo);
int Disp_Ovl_Engine_Get_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
				   struct fb_overlay_layer *layerInfo);
int Disp_Ovl_Engine_Set_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
				   struct fb_overlay_layer *layerInfo);
int Disp_Ovl_Engine_Set_Overlayed_Buffer(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
					 struct disp_ovl_engine_config_mem_out_struct
					 *overlayedBufInfo);
int Disp_Ovl_Engine_Trigger_Overlay(DISP_OVL_ENGINE_INSTANCE_HANDLE handle);
int Disp_Ovl_Engine_Wait_Overlay_Complete(DISP_OVL_ENGINE_INSTANCE_HANDLE handle, int timeout);
int Disp_Ovl_Engine_Get_Dirty_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle, int *OverlayDirty,
				   int *MemOutDirty);
int Disp_Ovl_Engine_Set_Path_Info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
				  struct disp_path_config_struct *info);
int Disp_Ovl_Engine_Sync_Captured_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle);
int Disp_Ovl_Engine_Sync_Realtime_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle);
int Disp_Ovl_Engine_Dump_layer_info(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
				    OVL_CONFIG_STRUCT *cached_layer,
				    OVL_CONFIG_STRUCT *captured_layer,
				    OVL_CONFIG_STRUCT *realtime_layer);
int Disp_Ovl_Engine_Trigger_Overlay_Fence(DISP_OVL_ENGINE_INSTANCE_HANDLE handle,
					  struct disp_ovl_engine_setting *setting);
int Disp_Ovl_Engine_Get_Fence_Fd(DISP_OVL_ENGINE_INSTANCE_HANDLE handle, int *fence_fd);
int Disp_Ovl_Engine_Check_WFD_Instance(void);

extern DISP_OVL_ENGINE_INSTANCE_HANDLE mtkfb_instance;
extern bool mtkfb_work_is_skip(void);

#endif
