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
#include <asm/uaccess.h>
#include <linux/slab.h>
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif
#include "cmdq_record.h"
#include "ddp_drv.h"
#include "ddp_reg.h"
#include "ddp_path.h"
#include "ddp_gamma.h"


static DEFINE_MUTEX(g_gamma_global_lock);


/* ======================================================================== */
/*  GAMMA                                                                   */
/* ======================================================================== */

static DISP_GAMMA_LUT_T *g_disp_gamma_lut[DISP_GAMMA_TOTAL] = { NULL };

static ddp_module_notify g_gamma_ddp_notify;


static int disp_gamma_write_lut_reg(cmdqRecHandle cmdq, disp_gamma_id_t id, int lock);


static void disp_gamma_init(disp_gamma_id_t id, unsigned int width, unsigned int height, void *cmdq)
{
	DISP_REG_SET(cmdq, DISP_REG_GAMMA_SIZE, (width << 16) | height);

	disp_gamma_write_lut_reg(cmdq, id, 1);
}


static int disp_gamma_config(DISP_MODULE_ENUM module, disp_ddp_path_config *pConfig, void *cmdq)
{
	if (pConfig->dst_dirty)
		disp_gamma_init(DISP_GAMMA0, pConfig->dst_w, pConfig->dst_h, cmdq);
	return 0;
}


static void disp_gamma_trigger_refresh(disp_gamma_id_t id)
{
	if (g_gamma_ddp_notify != NULL)
		g_gamma_ddp_notify(DISP_MODULE_GAMMA, DISP_PATH_EVENT_TRIGGER);
}


static int disp_gamma_write_lut_reg(cmdqRecHandle cmdq, disp_gamma_id_t id, int lock)
{
	unsigned long lut_base = 0;
	DISP_GAMMA_LUT_T *gamma_lut;
	int i;
	int ret = 0;

	if (id >= DISP_GAMMA_TOTAL) {
		pr_err("[GAMMA] disp_gamma_write_lut_reg: invalid ID = %d\n", id);
		return -EFAULT;
	}

	if (lock)
		mutex_lock(&g_gamma_global_lock);

	gamma_lut = g_disp_gamma_lut[id];
	if (gamma_lut == NULL) {
		pr_debug("[GAMMA] disp_gamma_write_lut_reg: gamma table [%d] not initialized\n", id);
		ret = -EFAULT;
		goto gamma_write_lut_unlock;
	}

	if (id == DISP_GAMMA0) {
		DISP_REG_MASK(cmdq, DISP_REG_GAMMA_EN, 0x1, 0x1);
		DISP_REG_MASK(cmdq, DISP_REG_GAMMA_CFG, 0x2, 0x2);
		lut_base = DISP_REG_GAMMA_LUT;
	} else {
		ret = -EFAULT;
		goto gamma_write_lut_unlock;
	}

	for (i = 0; i < DISP_GAMMA_LUT_SIZE; i++) {
		DISP_REG_MASK(cmdq, (lut_base + i * 4), gamma_lut->lut[i], ~0);

		if ((i & 0x3f) == 0) {
			pr_debug("[GAMMA] [0x%08lx](%d) = 0x%x\n", (lut_base + i * 4), i,
			       gamma_lut->lut[i]);
		}
	}
	i--;
	pr_debug("[GAMMA] [0x%08lx](%d) = 0x%x\n", (lut_base + i * 4), i,
	       gamma_lut->lut[i]);

gamma_write_lut_unlock:

	if (lock)
		mutex_unlock(&g_gamma_global_lock);

	return ret;
}


static int disp_gamma_set_lut(const DISP_GAMMA_LUT_T __user *user_gamma_lut, void *cmdq)
{
	int ret = 0;
	disp_gamma_id_t id;
	DISP_GAMMA_LUT_T *gamma_lut, *old_lut;

	pr_debug("[GAMMA] disp_gamma_set_lut(cmdq = %d)", (cmdq != NULL ? 1 : 0));

	gamma_lut = kmalloc(sizeof(DISP_GAMMA_LUT_T), GFP_KERNEL);
	if (gamma_lut == NULL) {
		/*pr_err("[GAMMA] disp_gamma_set_lut: no memory\n");*/
		return -EFAULT;
	}

	if (copy_from_user(gamma_lut, user_gamma_lut, sizeof(DISP_GAMMA_LUT_T)) != 0) {
		ret = -EFAULT;
		kfree(gamma_lut);
	} else {
		id = gamma_lut->hw_id;
		if (0 <= id && id < DISP_GAMMA_TOTAL) {
			mutex_lock(&g_gamma_global_lock);

			old_lut = g_disp_gamma_lut[id];
			g_disp_gamma_lut[id] = gamma_lut;

			ret = disp_gamma_write_lut_reg(cmdq, id, 0);

			mutex_unlock(&g_gamma_global_lock);

			if (old_lut != NULL)
				kfree(old_lut);

			disp_gamma_trigger_refresh(id);
		} else {
			pr_err("[GAMMA] disp_gamma_set_lut: invalid ID = %d\n", id);
			ret = -EFAULT;
		}
	}

	return ret;
}


static int disp_gamma_io(DISP_MODULE_ENUM module, int msg, unsigned long arg, void *cmdq)
{
	switch (msg) {
	case DISP_IOCTL_SET_GAMMALUT:
		if (disp_gamma_set_lut((DISP_GAMMA_LUT_T *) arg, cmdq) < 0) {
			pr_err("DISP_IOCTL_SET_GAMMALUT: failed\n");
			return -EFAULT;
		}
		break;
	}

	return 0;
}


static int disp_gamma_set_listener(DISP_MODULE_ENUM module, ddp_module_notify notify)
{
	g_gamma_ddp_notify = notify;
	return 0;
}


static int disp_gamma_bypass(DISP_MODULE_ENUM module, int bypass)
{
	int relay = 0;

	if (bypass)
		relay = 1;

	DISP_REG_MASK(NULL, DISP_REG_GAMMA_CFG, relay, 0x1);

	pr_err("disp_gamma_bypass(bypass = %d)", bypass);

	return 0;
}


static int disp_gamma_power_on(DISP_MODULE_ENUM module, void *handle)
{
	if (module == DISP_MODULE_GAMMA) {
		ddp_module_clock_enable(MM_CLK_DISP_GAMMA, true);
	}

	return 0;
}

static int disp_gamma_power_off(DISP_MODULE_ENUM module, void *handle)
{
	if (module == DISP_MODULE_GAMMA) {
		ddp_module_clock_enable(MM_CLK_DISP_GAMMA, false);
	}

	return 0;
}


DDP_MODULE_DRIVER ddp_driver_gamma = {
	.config = disp_gamma_config,
	.bypass = disp_gamma_bypass,
	.set_listener = disp_gamma_set_listener,
	.cmd = disp_gamma_io,
	.init = disp_gamma_power_on,
	.deinit = disp_gamma_power_off,
	.power_on = disp_gamma_power_on,
	.power_off = disp_gamma_power_off,
};



/* ======================================================================== */
/*  COLOR CORRECTION                                                        */
/* ======================================================================== */
#define CCORR_CLIP(val, min, max) ((val >= max) ? max : ((val <= min) ? min : val))

static DISP_CCORR_COEF_T *g_disp_ccorr_coef[DISP_CCORR_TOTAL] = { NULL };
static int g_disp_ccorr_color_matrix[3][3] = {
	{1024, 0, 0},
	{0, 1024, 0},
	{0, 0, 1024} };
static DISP_CCORR_COEF_T g_multiply_matrix_coef;
static int g_disp_ccorr_without_gamma;

static ddp_module_notify g_ccorr_ddp_notify;

static int disp_ccorr_write_coef_reg(cmdqRecHandle cmdq, disp_ccorr_id_t id, int lock);


static void disp_ccorr_init(disp_ccorr_id_t id, unsigned int width, unsigned int height, void *cmdq)
{
	DISP_REG_SET(cmdq, DISP_REG_CCORR_SIZE, (width << 16) | height);

	disp_ccorr_write_coef_reg(cmdq, id, 1);
}

void disp_ccorr_multiply_3x3(unsigned int ccorrCoef[3][3], int color_matrix[3][3],
		unsigned int resultCoef[3][3])
{
	int temp_Result;

	temp_Result = (int)((ccorrCoef[0][0]*color_matrix[0][0] + ccorrCoef[0][1]*color_matrix[1][0] +
		ccorrCoef[0][2]*color_matrix[2][0]) / 1024);
	resultCoef[0][0] = CCORR_CLIP(temp_Result, -2048, 2047) & 0xFFF;

	temp_Result = (int)((ccorrCoef[0][0]*color_matrix[0][1] + ccorrCoef[0][1]*color_matrix[1][1] +
		ccorrCoef[0][2]*color_matrix[2][1]) / 1024);
	resultCoef[0][1] = CCORR_CLIP(temp_Result, -2048, 2047) & 0xFFF;

	temp_Result = (int)((ccorrCoef[0][0]*color_matrix[0][2] + ccorrCoef[0][1]*color_matrix[1][2] +
		ccorrCoef[0][2]*color_matrix[2][2]) / 1024);
	resultCoef[0][2] = CCORR_CLIP(temp_Result, -2048, 2047) & 0xFFF;


	temp_Result = (int)((ccorrCoef[1][0]*color_matrix[0][0] + ccorrCoef[1][1]*color_matrix[1][0] +
		ccorrCoef[1][2]*color_matrix[2][0]) / 1024);
	resultCoef[1][0] = CCORR_CLIP(temp_Result, -2048, 2047) & 0xFFF;

	temp_Result = (int)((ccorrCoef[1][0]*color_matrix[0][1] + ccorrCoef[1][1]*color_matrix[1][1] +
		ccorrCoef[1][2]*color_matrix[2][1]) / 1024);
	resultCoef[1][1] = CCORR_CLIP(temp_Result, -2048, 2047) & 0xFFF;

	temp_Result = (int)((ccorrCoef[1][0]*color_matrix[0][2] + ccorrCoef[1][1]*color_matrix[1][2] +
		ccorrCoef[1][2]*color_matrix[2][2]) / 1024);
	resultCoef[1][2] = CCORR_CLIP(temp_Result, -2048, 2047) & 0xFFF;


	temp_Result = (int)((ccorrCoef[2][0]*color_matrix[0][0] + ccorrCoef[2][1]*color_matrix[1][0] +
		ccorrCoef[2][2]*color_matrix[2][0]) / 1024);
	resultCoef[2][0] = CCORR_CLIP(temp_Result, -2048, 2047) & 0xFFF;

	temp_Result = (int)((ccorrCoef[2][0]*color_matrix[0][1] + ccorrCoef[2][1]*color_matrix[1][1] +
		ccorrCoef[2][2]*color_matrix[2][1]) / 1024);
	resultCoef[2][0] = CCORR_CLIP(temp_Result, -2048, 2047) & 0xFFF;

	temp_Result = (int)((ccorrCoef[2][0]*color_matrix[0][2] + ccorrCoef[2][1]*color_matrix[1][2] +
		ccorrCoef[2][2]*color_matrix[2][2]) / 1024);
	resultCoef[2][2] = CCORR_CLIP(temp_Result, -2048, 2047) & 0xFFF;
}

#define CCORR_REG(base, idx) (base + (idx) * 4)

static int disp_ccorr_write_coef_reg(cmdqRecHandle cmdq, disp_ccorr_id_t id, int lock)
{
	const unsigned long ccorr_base = DISPSYS_CCORR_BASE + 0x80;
	int ret = 0;
	DISP_CCORR_COEF_T *ccorr, *multiply_matrix;

	if (lock)
		mutex_lock(&g_gamma_global_lock);

	ccorr = g_disp_ccorr_coef[id];
	if (ccorr == NULL) {
		pr_debug("[GAMMA] disp_ccorr_write_coef_reg: [%d] not initialized\n", id);
		ret = -EFAULT;
		goto ccorr_write_coef_unlock;
	}

	if (id == 0) {
		multiply_matrix = &g_multiply_matrix_coef;
		disp_ccorr_multiply_3x3(ccorr->coef, g_disp_ccorr_color_matrix, multiply_matrix->coef);
		ccorr = multiply_matrix;
	}

	DISP_REG_SET(cmdq, DISP_REG_CCORR_EN, 1);
#if 0
	/*
	 * enable ccorr engine
	 */
	DISP_REG_MASK(cmdq, DISP_REG_CCORR_CFG, 0x2, 0x2);
#else
	/*
	 * 1) enanle ccorr engine
	 * 2) disable gamma in ccorr
	 * 3) set relay mode to 0
	 */
	DISP_REG_MASK(cmdq, DISP_REG_CCORR_CFG, 0x2 | (g_disp_ccorr_without_gamma << 2), 0x7);
#endif

	DISP_REG_SET(cmdq, CCORR_REG(ccorr_base, 0),
		     ((ccorr->coef[0][0] << 16) | (ccorr->coef[0][1])));
	DISP_REG_SET(cmdq, CCORR_REG(ccorr_base, 1),
		     ((ccorr->coef[0][2] << 16) | (ccorr->coef[1][0])));
	DISP_REG_SET(cmdq, CCORR_REG(ccorr_base, 2),
		     ((ccorr->coef[1][1] << 16) | (ccorr->coef[1][2])));
	DISP_REG_SET(cmdq, CCORR_REG(ccorr_base, 3),
		     ((ccorr->coef[2][0] << 16) | (ccorr->coef[2][1])));
	DISP_REG_SET(cmdq, CCORR_REG(ccorr_base, 4), (ccorr->coef[2][2] << 16));

ccorr_write_coef_unlock:

	if (lock)
		mutex_unlock(&g_gamma_global_lock);

	return ret;
}


static void disp_ccorr_trigger_refresh(disp_ccorr_id_t id)
{
	if (g_ccorr_ddp_notify != NULL)
		g_ccorr_ddp_notify(DISP_MODULE_CCORR, DISP_PATH_EVENT_TRIGGER);
}


static int disp_ccorr_set_coef(const DISP_CCORR_COEF_T __user *user_color_corr, void *cmdq)
{
	int ret = 0;
	DISP_CCORR_COEF_T *ccorr, *old_ccorr;
	disp_ccorr_id_t id;

	ccorr = kmalloc(sizeof(DISP_CCORR_COEF_T), GFP_KERNEL);
	if (ccorr == NULL) {
		/*pr_err("[GAMMA] disp_ccorr_set_coef: no memory\n");*/
		return -EFAULT;
	}

	if (copy_from_user(ccorr, user_color_corr, sizeof(DISP_CCORR_COEF_T)) != 0) {
		ret = -EFAULT;
		kfree(ccorr);
	} else {
		id = ccorr->hw_id;
		if (0 <= id && id < DISP_CCORR_TOTAL) {
			mutex_lock(&g_gamma_global_lock);

			old_ccorr = g_disp_ccorr_coef[id];
			g_disp_ccorr_coef[id] = ccorr;

			ret = disp_ccorr_write_coef_reg(cmdq, id, 0);

			mutex_unlock(&g_gamma_global_lock);

			if (old_ccorr != NULL)
				kfree(old_ccorr);

			disp_ccorr_trigger_refresh(id);
		} else {
			pr_err("[GAMMA] disp_ccorr_set_coef: invalid ID = %d\n", id);
			ret = -EFAULT;
		}
	}

	return ret;
}

int disp_ccorr_set_color_matrix(void *cmdq, int32_t matrix[16], int32_t hint)
{
	int ret = 0;
	int i, j;
	int ccorr_without_gamma = 0;

	if (cmdq == NULL) {
		DDPERR("[GAMMA] disp_ccorr_set_color_matrix: cmdq can not be NULL\n");
		return -EFAULT;
	}

	mutex_lock(&g_gamma_global_lock);

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			/* Copy Color Matrix */
			g_disp_ccorr_color_matrix[i][j] = matrix[i*4 + j];

			if (i == j && g_disp_ccorr_color_matrix[i][j] != 1024)
				ccorr_without_gamma = 1;
			else if (i != j && g_disp_ccorr_color_matrix[i][j] != 0)
				ccorr_without_gamma = 1;
		}
	}

	g_disp_ccorr_without_gamma = ccorr_without_gamma;

	disp_ccorr_write_coef_reg(cmdq, 0, 0);

	mutex_unlock(&g_gamma_global_lock);

	return ret;
}

static int disp_ccorr_config(DISP_MODULE_ENUM module, disp_ddp_path_config *pConfig, void *cmdq)
{
	if (pConfig->dst_dirty)
		disp_ccorr_init(DISP_CCORR0, pConfig->dst_w, pConfig->dst_h, cmdq);

	return 0;
}


static int disp_ccorr_io(DISP_MODULE_ENUM module, int msg, unsigned long arg, void *cmdq)
{
	switch (msg) {
	case DISP_IOCTL_SET_CCORR:
		if (disp_ccorr_set_coef((DISP_CCORR_COEF_T *) arg, cmdq) < 0) {
			pr_err("DISP_IOCTL_SET_CCORR: failed\n");
			return -EFAULT;
		}
		break;
	}

	return 0;
}


static int disp_ccorr_set_listener(DISP_MODULE_ENUM module, ddp_module_notify notify)
{
	g_ccorr_ddp_notify = notify;
	return 0;
}


static int disp_ccorr_bypass(DISP_MODULE_ENUM module, int bypass)
{
	int relay = 0;

	if (bypass)
		relay = 1;

	DISP_REG_MASK(NULL, DISP_REG_CCORR_CFG, relay, 0x1);

	pr_debug("disp_ccorr_bypass(bypass = %d)", bypass);

	return 0;
}


static int disp_ccorr_power_on(DISP_MODULE_ENUM module, void *handle)
{
	if (module == DISP_MODULE_CCORR) {
		ddp_module_clock_enable(MM_CLK_DISP_CCORR, true);
	}

	return 0;
}

static int disp_ccorr_power_off(DISP_MODULE_ENUM module, void *handle)
{
	if (module == DISP_MODULE_CCORR) {
		ddp_module_clock_enable(MM_CLK_DISP_CCORR, false);
	}

	return 0;
}



DDP_MODULE_DRIVER ddp_driver_ccorr = {
	.config = disp_ccorr_config,
	.bypass = disp_ccorr_bypass,
	.set_listener = disp_ccorr_set_listener,
	.cmd = disp_ccorr_io,
	.init = disp_ccorr_power_on,
	.deinit = disp_ccorr_power_off,
	.power_on = disp_ccorr_power_on,
	.power_off = disp_ccorr_power_off,
};
