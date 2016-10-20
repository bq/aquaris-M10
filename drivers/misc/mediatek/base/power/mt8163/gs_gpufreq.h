/*
 * Copyright (c) 2014 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _GS_GPUFREQ_H
#define _GS_GPUFREQ_H

#include <linux/module.h>
#include <linux/types.h>

#define CONFIG_SUPPORT_MET_GPU 1
#if CONFIG_SUPPORT_MET_GPU
#include <linux/export.h>
#endif

/*********************
 * Clock Mux Register
 **********************/
/*#define CLK26CALI       (0xF00001C0) // FIX ME, No this register
#define CLK_MISC_CFG_0  (0xF0000210)
#define CLK_MISC_CFG_1  (0xF0000214)
#define CLK26CALI_0     (0xF0000220)
#define CLK26CALI_1     (0xF0000224)
#define CLK26CALI_2     (0xF0000228)
#define MBIST_CFG_0     (0xF0000308)
#define MBIST_CFG_1     (0xF000030C)
#define MBIST_CFG_2     (0xF0000310)
#define MBIST_CFG_3     (0xF0000314)
*/


#if 0
/****************************
 * PMIC Wrapper DVFS Register
 *****************************/
#define PWRAP_BASE              (0xF000D000)
#define PMIC_WRAP_DVFS_ADR0     (PWRAP_BASE + 0xE8) /**/
#define PMIC_WRAP_DVFS_WDATA0   (PWRAP_BASE + 0xEC) /**/
#define PMIC_WRAP_DVFS_ADR1     (PWRAP_BASE + 0xF0) /**/
#define PMIC_WRAP_DVFS_WDATA1   (PWRAP_BASE + 0xF4) /**/
#define PMIC_WRAP_DVFS_ADR2     (PWRAP_BASE + 0xF8) /**/
#define PMIC_WRAP_DVFS_WDATA2   (PWRAP_BASE + 0xFC) /**/
#define PMIC_WRAP_DVFS_ADR3     (PWRAP_BASE + 0x100) /**/
#define PMIC_WRAP_DVFS_WDATA3   (PWRAP_BASE + 0x104) /**/
#define PMIC_WRAP_DVFS_ADR4     (PWRAP_BASE + 0x108) /**/
#define PMIC_WRAP_DVFS_WDATA4   (PWRAP_BASE + 0x10C) /**/
#define PMIC_WRAP_DVFS_ADR5     (PWRAP_BASE + 0x110) /**/
#define PMIC_WRAP_DVFS_WDATA5   (PWRAP_BASE + 0x114) /**/
#define PMIC_WRAP_DVFS_ADR6     (PWRAP_BASE + 0x118) /**/
#define PMIC_WRAP_DVFS_WDATA6   (PWRAP_BASE + 0x11C) /**/
#define PMIC_WRAP_DVFS_ADR7     (PWRAP_BASE + 0x120) /**/
#define PMIC_WRAP_DVFS_WDATA7   (PWRAP_BASE + 0x124) /**/
#endif
struct proc_dir_entry;
extern const struct platform_device *g3dGetPlatformDevice(void);

#define GPU_DVFS_F550     (549250) /* KHz */
#define GPU_DVFS_F480     (481000) /* KHz */
#define GPU_DVFS_F380     (380250) /* KHz */
#define GPU_DVFS_F250     (250250) /* KHz */

#define GPU_BONDING_000  GPU_DVFS_F550
/* #define GPU_BONDING_001 */
#define GPU_BONDING_010  GPU_DVFS_F480
/* #define GPU_BONDING_011 */
#define GPU_BONDING_100  GPU_DVFS_F380

/* The value has meaning for pmic formula in gpufreq */
#define GPU_POWER_VGPU_MAX     (135000)
#define GPU_POWER_VGPU_1_25V   (125000)
#define GPU_POWER_VGPU_1_20V   (120000)
#define GPU_POWER_VGPU_1_15V   (115000)
#define GPU_POWER_VGPU_MIN     (105000)

/* aligned to 64 */
struct mtk_gpufreq_info {
	u32 freq_khz;
	u32 volt;
	u32 power_mW;
	u32 lower_bound;
	u32 upper_bound;
	u32 padding;
};

/*!
 * Callback functions to register with gpufreq.
 */
struct mtk_gpufreq_kmif {
	void (*notify_thermal_event)(void); /**< Called when thermal event happens. */
	void (*notify_touch_event)(void);/**< Called when touch event happens. */

	u32 (*get_loading_stat)(void);	/**< For MET statistic */
	u32 (*get_block_stat)(void);	/**< For MET statistic */
	u32 (*get_idle_stat)(void);	/**< For MET statistic */
	u32 (*get_power_loading_stat)(void); /**< For MET statistic */
};


struct mtk_gpu_power_info {
	unsigned int gpufreq_khz;
	unsigned int gpufreq_power;
};

extern void __iomem *clk_infracfg_ao_base;

/*****************
 * extern function
 ******************/
extern const struct platform_device *g3dGetPlatformDevice(void);
extern unsigned int mt_gpufreq_update_volt(unsigned int pmic_volt[], unsigned int array_size);

/******************************
 * Extern Function Declaration
 *******************************/
extern int mtk_gpufreq_register(struct mtk_gpu_power_info *freqs, int num);
extern s32 gs_gpufreq_register(struct mtk_gpufreq_info *rows, u32 const num);
extern void gs_gpufreq_unregister(void);
extern int gs_gpufreq_state_set(int enabled);
extern unsigned int gs_gpufreq_get_cur_freq_index(void);
extern unsigned int gs_gpufreq_get_cur_freq(void);
extern unsigned int gs_gpufreq_get_cur_volt(void);
extern unsigned int gs_gpufreq_get_dvfs_table_num(void);
extern unsigned int gs_gpufreq_target(unsigned int idx);
extern void gs_gpufreq_mtcmos_on(void);
extern void gs_gpufreq_mtcmos_off(void);
extern unsigned int gs_gpufreq_voltage_enable_set(unsigned int enable);
extern unsigned int gs_gpufreq_update_volt(unsigned int pmic_volt[], unsigned int array_size);
extern unsigned int gs_gpufreq_get_freq_by_idx(unsigned int idx);
extern void gs_gpufreq_get_cur_setting(u32 *freq_khz, u32 *volt);
extern void gs_gpufreq_restore_default_volt(void);
extern void gs_gpufreq_enable_by_ptpod(void);
extern void gs_gpufreq_disable_by_ptpod(void);
extern unsigned int gs_gpufreq_get_thermal_limit_index(void);
extern unsigned int gs_gpufreq_get_thermal_limit_freq(void);
extern void gs_gpufreq_set_cur_index(u32 const index);
extern void gs_gpufreq_switch_handler(void);
extern void gs_gpufreq_get_cur_indices(u32 *bottom_index, u32 *limit_index, u32 *current_index);

extern void gs_gpufreq_dvfs_enable(void);
extern void gs_gpufreq_dvfs_disable(void);
extern void gs_gpufreq_set_cur_load(u32 const load);

extern void mt_gpufreq_thermal_protect(unsigned int limited_power);

extern void gs_gpufreq_fake_get_cur_setting(u32 *freq_khz, u32 *volt);


extern s32 gs_gpufreq_kmif_register(struct mtk_gpufreq_kmif *ops);
extern s32 gs_gpufreq_kmif_unregister(void);

/*****************
 * power limit notification
 ******************/
typedef void (*gpufreq_power_limit_notify) (unsigned int);
extern void gs_gpufreq_power_limit_notify_registerCB(gpufreq_power_limit_notify pCB);

/*****************
 * input boost notification
 ******************/
typedef void (*gpufreq_input_boost_notify) (unsigned int);
extern void gs_gpufreq_input_boost_notify_registerCB(gpufreq_input_boost_notify pCB);

/*****************
 * profiling purpose
 ******************/
typedef void (*sampler_func) (unsigned int);
extern void gs_gpufreq_setfreq_registerCB(sampler_func pCB);
extern void gs_gpufreq_setvolt_registerCB(sampler_func pCB);

#endif
