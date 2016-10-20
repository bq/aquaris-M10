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

#include <linux/module.h>
#include <linux/kernel.h>

#include "gpad_common.h"
#include "gpad_gpufreq_utst.h"
#include "mt_gpufreq.h"
#include "gs_gpufreq.h"

#define UTLOG(fmt, args...) \
	GPAD_LOG("[GPUFREQ][UTLOG]" fmt, ##args)

#define UTERR(fmt, args...) \
	GPAD_LOG_ERR("[GPUFREQ][UTERR]" fmt, ##args)

static s32 gpufreq_test_main(void);

long gpad_gpufreq_utst(struct gpad_device *dev, int param)
{
	s32 ret = 0;

	ret = gpufreq_test_main();
	return (long)ret;
}

static struct mtk_gpufreq_info g_ut_table[3][3] = {
	{
		{ .freq_khz = GPU_DVFS_F550, .volt = GPU_POWER_VGPU_1_25V, .power_mW = 1050, .lower_bound = 70, .upper_bound = 100 }, { .freq_khz = GPU_DVFS_F480, .volt = GPU_POWER_VGPU_1_25V, .power_mW =  950, .lower_bound = 30, .upper_bound = 69 }, { .freq_khz = GPU_DVFS_F380, .volt = GPU_POWER_VGPU_1_15V, .power_mW =  800, .lower_bound =  0, .upper_bound = 29 }, },
	{
		{ .freq_khz = GPU_DVFS_F550, .volt = GPU_POWER_VGPU_1_25V, .power_mW = 1050, .lower_bound = 40, .upper_bound = 100 }, { .freq_khz = GPU_DVFS_F380, .volt = GPU_POWER_VGPU_1_15V, .power_mW =  800, .lower_bound =  0, .upper_bound = 39 }, },
	{
		/*  eng mode */
		{ .freq_khz = 50000, .volt = GPU_POWER_VGPU_1_25V, .power_mW =  800, .lower_bound = 0, .upper_bound = 100 }, }, };

static u32 g_num[3] = { 3, 2, 1 };

/*****************************************************************************
 * Helpers
 *****************************************************************************/
static u32 min_(u32 const a, u32 const b)
{
	return (a <= b) ? a : b;
}

static u32 max_(u32 const a, u32 const b)
{
	return (a >= b) ? a : b;
}

static s32 test_freq_(u32 const expected_freq,   u32 const expected_volt)
{
	u32 freq = 0;
	u32 volt = 0;

	gs_gpufreq_get_cur_setting(&freq, &volt);

	if ((freq == expected_freq) &&
		 (volt == expected_volt)) {
		return 0;
	} else {
		UTERR("freq(%d, %d), volt(%d, %d)", freq, expected_freq, volt, expected_volt);
		return -1;
	}

	return 0;
}

static s32 test_indices_(u32 const expected_bottom,  u32 const expected_limit,  u32 const expected_current)
{
	u32 bottom_index  = 0;
	u32 limit_index   = 0;
	u32 current_index = 0;

	gs_gpufreq_get_cur_indices(&bottom_index,    &limit_index,    &current_index);

	if ((bottom_index  == expected_bottom) &&
		 (limit_index   == expected_limit)  &&
		 (current_index == expected_current)) {
		return 0;
	} else {
		return -1;
	}
}

static s32 test_hw_fv_(u32 const expected_freq,    u32 const expected_volt)
{
	u32 freq = 0;
	u32 volt = 0;

	gs_gpufreq_fake_get_cur_setting(&freq, &volt);

	if ((freq == expected_freq) &&
		 (volt == expected_volt)) {
		return 0;
	} else {
		return -1;
	}

	return 0;
}

static s32 gpufreq_test_init_(u64 const flags)
{
	s32 error = 0;
	u32 i;

	UTLOG("init");
	do {
		if ((flags & 0x04))
			gs_gpufreq_unregister();

		if ((flags & 0x10)) {
			i = 0;
			error = gs_gpufreq_register(g_ut_table[i], g_num[i]);
			if (error)
				break;
		}

		if ((flags & 0x20)) {
			i = 1;
			error = gs_gpufreq_register(g_ut_table[i], g_num[i]);
			if (error)
				break;
		}

		if ((flags & 0x40)) {
			i = 2;
			error = gs_gpufreq_register(g_ut_table[i], g_num[i]);
			if (error)
				break;
		}

		if ((flags & 0x100))
			gs_gpufreq_dvfs_enable();
	} while (0);

	return error;
}

static s32 gpufreq_test_uninit_(u64 const type)
{
	GPAD_UNUSED(type);

	if ((type & 0x1)) /* restore to default setting */
		mt_gpufreq_thermal_protect(0);

	UTLOG("uninit");
	return 0;
}

/*****************************************************************************
 * test list
 *****************************************************************************/
typedef s32 (*test_func_type)(u64 const param);

struct test_list_type {
	u32 init;
	u32 uninit;
	u64 param;
	test_func_type func;
	char *name;
};

static s32 gpufreq_test_smoke(u64 const param)
{
	return 0;
}

static s32 gpufreq_test_basic(u64 const param)
{
	s32 ret;
	s32 ret2;

	ret = test_freq_(GPU_DVFS_F550, GPU_POWER_VGPU_1_25V);
	ret2 = test_indices_(2, 0, 0);
	if (ret)
		return -10;
	if (ret2)
		return -20;

	gs_gpufreq_set_cur_load(50);
	ret = test_freq_(GPU_DVFS_F480, GPU_POWER_VGPU_1_25V);
	ret2 = test_indices_(2, 0, 1);
	if (ret)
		return -30;
	if (ret2)
		return -40;

	gs_gpufreq_set_cur_load(20);
	ret = test_freq_(GPU_DVFS_F380, GPU_POWER_VGPU_1_15V);
	ret2 = test_indices_(2, 0, 2);
	if (ret)
		return -50;
	if (ret2)
		return -60;

	gs_gpufreq_set_cur_load(0);
	ret = test_freq_(GPU_DVFS_F380, GPU_POWER_VGPU_1_15V);
	ret2 = test_indices_(2, 0, 2);
	if (ret)
		return -70;
	if (ret2)
		return -80;

	return 0;
}

static s32 gpufreq_test_set_row(u64 const param)
{
	s32 ret;
	s32 ret2;

	ret = test_freq_(GPU_DVFS_F550, GPU_POWER_VGPU_1_25V);
	ret2 = test_indices_(2, 0, 0);
	if (ret)
		return -110;
	if (ret2)
		return -120;

	gs_gpufreq_set_cur_index(1);
	ret = test_freq_(GPU_DVFS_F480, GPU_POWER_VGPU_1_25V);
	ret2 = test_indices_(2, 0, 1);
	if (ret)
		return -130;
	if (ret2)
		return -140;

	gs_gpufreq_set_cur_index(2);
	ret = test_freq_(GPU_DVFS_F380, GPU_POWER_VGPU_1_15V);
	ret2 = test_indices_(2, 0, 2);
	if (ret)
		return -150;
	if (ret2)
		return -160;

	gs_gpufreq_set_cur_index(0);
	ret = test_freq_(GPU_DVFS_F550, GPU_POWER_VGPU_1_25V);
	ret2 = test_indices_(2, 0, 0);
	if (ret)
		return -170;
	if (ret2)
		return -180;

	return 0;
}

static s32 gpufreq_test_unregister(u64 const param)
{
	s32 ret;
	s32 ret2;

	ret = test_freq_(GPU_DVFS_F550, GPU_POWER_VGPU_1_25V);
	ret2 = test_indices_(2, 0, 0);
	if (ret)
		return -210;
	if (ret2)
		return -220;

	gs_gpufreq_unregister();

	ret = test_freq_(GPU_DVFS_F550, GPU_POWER_VGPU_1_25V);
	ret2 = test_indices_(0, 0, 0);
	if (ret)
		return -230;
	if (ret2)
		return -240;

	return 0;
}

static s32 gpufreq_test_basic_hw(u64 const param)
{
	s32 ret;
	s32 ret2;

	ret = test_freq_(GPU_DVFS_F550, GPU_POWER_VGPU_1_25V);
	ret2 = test_indices_(2, 0, 0);
	if (ret)
		return -310;
	if (ret2)
		return -320;

	gs_gpufreq_switch_handler();
	ret = test_hw_fv_(GPU_DVFS_F550, GPU_POWER_VGPU_1_25V);
	if (ret)
		return -330;

	gs_gpufreq_set_cur_index(0);
	gs_gpufreq_switch_handler();
	ret = test_hw_fv_(GPU_DVFS_F550, GPU_POWER_VGPU_1_25V);
	if (ret)
		return -340;

	gs_gpufreq_set_cur_index(1);
	gs_gpufreq_switch_handler();
	ret = test_hw_fv_(GPU_DVFS_F480, GPU_POWER_VGPU_1_25V);
	if (ret)
		return -350;

	gs_gpufreq_set_cur_index(2);
	gs_gpufreq_switch_handler();
	ret = test_hw_fv_(GPU_DVFS_F380, GPU_POWER_VGPU_1_15V);
	if (ret)
		return -360;

	gs_gpufreq_set_cur_index(0);
	gs_gpufreq_switch_handler();
	ret = test_hw_fv_(GPU_DVFS_F550, GPU_POWER_VGPU_1_25V);
	if (ret)
		return -370;

	return 0;
}

static s32 gpufreq_test_thermal(u64 const param)
{
	u32 i = 0;
	u32 powers[]   = { 1200, 1050, 1000, 950, 900, 850, 800, 700 };
	u32 pindices[] = {    0,    0,    1,   1,   2,   2,   2,   2 };
	u32 ptimes = ARRAY_SIZE(powers);

	s32 ret;
	s32 ret2;

	u32 power;
	u32 pindex;
	struct mtk_gpufreq_info *prow;
	u32 pfreq;
	u32 pvolt;

	for (i = 0; i < ptimes; ++i) {

		power = powers[i];
		pindex = pindices[i];
		prow = &g_ut_table[0][pindex];
		pfreq = prow->freq_khz;
		pvolt = prow->volt;

		mt_gpufreq_thermal_protect(power);

		ret = test_freq_(min_(pfreq, GPU_DVFS_F550), min_(pvolt, GPU_POWER_VGPU_1_25V));
		ret2 = test_indices_(2, pindex, max_(pindex, 0));
		if (ret)
			return -9100 - i*16;
		if (ret2)
			return -9200 - i*16;

		gs_gpufreq_set_cur_load(50);
		ret = test_freq_(min_(pfreq, GPU_DVFS_F480), min_(pvolt, GPU_POWER_VGPU_1_25V));
		ret2 = test_indices_(2, pindex, max_(pindex, 1));
		if (ret)
			return -9300 - i*16;
		if (ret2)
			return -9400 - i*16;

		gs_gpufreq_set_cur_load(20);
		ret = test_freq_(min_(pfreq, GPU_DVFS_F380), min_(pvolt, GPU_POWER_VGPU_1_15V));
		ret2 = test_indices_(2, pindex, max_(pindex, 2));
		if (ret)
			return -9500 - i*16;
		if (ret2)
			return -9600 - i*16;

		gs_gpufreq_set_cur_load(0);
		ret = test_freq_(min_(pfreq, GPU_DVFS_F380), min_(pvolt, GPU_POWER_VGPU_1_15V));
		ret2 = test_indices_(2, pindex, max_(pindex, 2));
		if (ret)
			return -9700 - i*16;
		if (ret2)
			return -9800 - i*16;

		gs_gpufreq_set_cur_load(100);
	}

	return 0;
}

/*   bit 0 val   1 = gpufreq init
 *   bit 1 val   2 = dvfs init
 *   bit 2 val   4 = unregister
 *   bit 3 val   8 =
 *   bit 4 val  10 = register g_ut_table 0
 *   bit 0 val   1 = gpufreq init
 *   bit 1 val   2 = dvfs init
 *   bit 2 val   4 = unregister
 *   bit 3 val   8 =
 *   bit 4 val  10 = register g_ut_table 0
 *   bit 5 val  20 = register g_ut_table 1
 *   bit 6 val  40 = register g_ut_table 2
 *   bit 7 val  80 =
 *   bit 8 val 100 = dvfs_enable
 */
static struct test_list_type test_list[] = {
	{ .init = 0x0000, .uninit = 0x0000, .param = 0x0000, .func = gpufreq_test_smoke,           .name = "Smoke 1"   }, { .init = 0x0004, .uninit = 0x0000, .param = 0x0000, .func = gpufreq_test_smoke,           .name = "Smoke 2"   }, { .init = 0x0014, .uninit = 0x0000, .param = 0x0000, .func = gpufreq_test_smoke,           .name = "Smoke 3"   }, { .init = 0x0040, .uninit = 0x0000, .param = 0x0000, .func = gpufreq_test_smoke,           .name = "Table 0"   }, { .init = 0x0020, .uninit = 0x0000, .param = 0x0000, .func = gpufreq_test_smoke,           .name = "Table 1"   }, { .init = 0x0010, .uninit = 0x0000, .param = 0x0000, .func = gpufreq_test_smoke,           .name = "Table 2"   }, { .init = 0x0110, .uninit = 0x0000, .param = 0x0000, .func = gpufreq_test_basic,           .name = "basic 0"   }, { .init = 0x0110, .uninit = 0x0000, .param = 0x0000, .func = gpufreq_test_set_row,         .name = "basic 1" }, { .init = 0x0110, .uninit = 0x0000, .param = 0x0000, .func = gpufreq_test_unregister,      .name = "unregister" }, { .init = 0x0110, .uninit = 0x0000, .param = 0x0000, .func = gpufreq_test_basic_hw,        .name = "Basic HW" }, { .init = 0x0110, .uninit = 0x0001, .param = 0x0000, .func = gpufreq_test_thermal,         .name = "Thermal" }, /*    { .init = 0x0110, .uninit = 0x0000, .param = 0x0000,  *		.func = gpufreq_test_thermal_row,     .name = "Thermal Row" },  */
};

static u32 test_count = ARRAY_SIZE(test_list);

static s32 gpufreq_test_main(void)
{
	u32 i;
	s32 error = 0;
	struct test_list_type *item = NULL;

	UTLOG("test started.\n");

	for (i = 0; i < test_count; ++i) {
		item = &test_list[i];

		UTLOG("%s will init.\n", item->name);
		error = gpufreq_test_init_(item->init);

		if (error) {
			UTERR("%s init failed. ret = %d\n", item->name, error);
			return error;
		}

		UTLOG("%s will start.\n", item->name);
		error = item->func(item->param);
		UTLOG("%s is done.\n", item->name);

		if (error) {
			UTERR("%s failed. ret = %d\n", item->name, error);
			return error;
		}

		gpufreq_test_uninit_(item->uninit);
		UTLOG("%s uninited.\n", item->name);

	}

	UTLOG("test done.\n");

	return error;
}

