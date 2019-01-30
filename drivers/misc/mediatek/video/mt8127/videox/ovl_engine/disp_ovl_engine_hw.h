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

#ifndef __DISP_OVL_ENGINE_HW_H__
#define __DISP_OVL_ENGINE_HW_H__

#include "disp_ovl_engine_core.h"

extern LCM_PARAMS *lcm_params;

/* Ovl_Engine SW */
#define DISP_OVL_ENGINE_HW_SUPPORT

#ifdef DISP_OVL_ENGINE_HW_SUPPORT
void disp_ovl_engine_hw_init(void);
void disp_ovl_engine_hw_set_params(DISP_OVL_ENGINE_INSTANCE *params);
void disp_ovl_engine_trigger_hw_overlay(void);
int disp_ovl_engine_indirect_link_overlay(void *fb_va, void *fb_pa);
void disp_ovl_engine_hw_register_irq(void (*irq_callback) (unsigned int param));
int disp_ovl_engine_hw_mva_map(struct disp_mva_map *mva_map_struct);
int disp_ovl_engine_hw_mva_unmap(struct disp_mva_map *mva_map_struct);
int disp_ovl_engine_hw_reset(void);
int disp_ovl_engine_update_rdma0(void);
void disp_ovl_engine_dump_ovl(int idx);
#endif

#endif
