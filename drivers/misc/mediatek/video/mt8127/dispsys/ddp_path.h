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

#ifndef __DDP_PATH_H__
#define __DDP_PATH_H__

#include "ddp_ovl.h"
#include "ddp_rdma.h"
#include "ddp_wdma.h"
#include "ddp_bls.h"
#include "ddp_drv.h"
#include "ddp_hal.h"

#define DDP_OVL_LAYER_MUN 4
#define DDP_BACKUP_REG_NUM 0x1000
#define DDP_UNBACKED_REG_MEM 0xdeadbeef

extern unsigned int fb_width;
extern unsigned int fb_height;

int disp_check_engine_status(int mutexID);
int disp_path_release_mutex_(int mutexID);
void disp_path_stop_access_memory(void);
int disp_path_config_layer_ovl_engine_control(int enable);
void disp_path_register_ovl_rdma_callback(void (*callback) (unsigned int param), unsigned int param);
void disp_path_register_ovl_wdma_callback(void (*callback) (unsigned int param), unsigned int param);
int disp_path_config_layer_ovl_engine(OVL_CONFIG_STRUCT *pOvlConfig, int OvlSecure);
int disp_path_config_OVL_WDMA(struct disp_path_config_mem_out_struct *pConfig, int OvlSecure);
int disp_path_config_OVL_WDMA_path(int mutex_id);


#endif
