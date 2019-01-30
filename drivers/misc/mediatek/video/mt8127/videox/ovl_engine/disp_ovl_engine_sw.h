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

#ifndef __DISP_OVL_ENGINE_SW_H__
#define __DISP_OVL_ENGINE_SW_H__

#include "disp_ovl_engine_core.h"

/* Ovl_Engine SW */
/* #define DISP_OVL_ENGINE_SW_SUPPORT */

#ifdef DISP_OVL_ENGINE_SW_SUPPORT
void disp_ovl_engine_sw_init(void);
void disp_ovl_engine_sw_set_params(DISP_OVL_ENGINE_INSTANCE *params);
void disp_ovl_engine_trigger_sw_overlay(void);
void disp_ovl_engine_sw_register_irq(void (*irq_callback) (unsigned int param));
#endif

#endif
