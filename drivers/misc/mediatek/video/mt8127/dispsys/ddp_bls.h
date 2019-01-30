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

#ifndef __DDP_BLS_H__
#define __DDP_BLS_H__

#include "ddp_drv.h"

void disp_bls_init(unsigned int srcWidth, unsigned int srcHeight);
int disp_bls_config(void);
void disp_bls_config_full(unsigned int width, unsigned int height);
int disp_bls_set_backlight(unsigned int level);
int disp_bls_set_max_backlight_(unsigned int level);
int disp_bls_get_max_backlight(void);

DISPLAY_GAMMA_T *get_gamma_index(void);
DISPLAY_PWM_T *get_pwm_lut(void);

/*
 * Called by ioctl to config sysram
 */
void disp_bls_update_gamma_lut(void);
void disp_bls_update_pwm_lut(void);

/*
 * Called by tasklet to config registers
 */
void disp_onConfig_bls(DISP_AAL_PARAM *param);

#endif
