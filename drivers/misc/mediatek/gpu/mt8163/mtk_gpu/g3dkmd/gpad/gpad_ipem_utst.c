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
#include <linux/random.h>
#include <linux/delay.h>

#include "gpad_common.h"
#include "gpad_ipem.h"
#include "gpad_ipem_utst.h"




#define UTLOG(fmt, args...) \
	GPAD_LOG("[IPEM][UTLOG]" fmt, ##args)

#define UTERR(fmt, args...) \
	GPAD_LOG_ERR("[IPEM][UTERR]" fmt, ##args)




typedef s64 (*ut_test_func)(struct gpad_device *dev, s32 param);

struct ut_test_item {
	char         *name;
	ut_test_func  func;
};

static s64 gpad_ipem_utst_timer(struct gpad_device *dev, s32 param)
{
	u32 loop;
	u32 rand;
	u32 interval;

	/* interval get and set test*/
	for (loop = IPEM_UTS_RAND_LOOP; loop > 0; loop--) {
		get_random_bytes(&rand, sizeof(rand));
		rand = rand % HZ;

		gpad_ipem_timer_set(dev, rand);
		interval = gpad_ipem_timer_get(dev);

		if (interval != rand)
			return -1001;
	}

	return 0;
}

static s64 gpad_ipem_utst_table(struct gpad_device *dev, s32 param)
{
	u32 loop;
	u32 rand;
	u32 table_id;

	/* table get and set test */
	for (loop = IPEM_UTS_RAND_LOOP; loop > 0; loop--) {
		get_random_bytes(&rand, sizeof(rand));
		rand = rand % 2;

		gpad_ipem_table_set(dev, rand);
		table_id = gpad_ipem_table_get(dev);

		if (table_id != rand)
			return -2001;
	}

	return 0;
}

static s64 gpad_ipem_utst_policy(struct gpad_device *dev, s32 param)
{
	u32 loop;
	u32 rand;
	u32 policy_id;

	/* dvfs policy get and set test */
	for (loop = IPEM_UTS_RAND_LOOP; loop > 0; loop--) {
		get_random_bytes(&rand, sizeof(rand));
		rand = rand % 9;

		gpad_ipem_dvfs_policy_set(dev, rand);
		policy_id = gpad_ipem_dvfs_policy_get(dev);

		if (policy_id != rand)
			return -3001;
	}

	/* dpm policy get and set test */
	for (loop = IPEM_UTS_RAND_LOOP; loop > 0; loop--) {
		get_random_bytes(&rand, sizeof(rand));
		rand = rand % 6;

		gpad_ipem_dpm_policy_set(dev, rand);
		policy_id = gpad_ipem_dpm_policy_get(dev);

		if (policy_id != rand)
			return -3002;
	}

	return 0;
}

static s64 gpad_ipem_utst_dvfs_lookup(struct gpad_device *dev, s32 param)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	u32 loop;
	u32 gpuload;
	struct mtk_gpufreq_info *table;
	u32 action_next;

	if ((param <= 0) || (param > IPEM_UTS_RAND_LOOP))
		return 0;

	loop = param;

	/* dvfs policy "lookup" test */
	for (loop = IPEM_UTS_RAND_LOOP; loop > 0; --loop) {
		UTLOG("%s: loop=%d...\n", __func__, loop);
		gpad_ipem_dvfs_policy_set(dev, 1);

		gpad_ipem_enable(dev);

		msleep(1000);

		gpad_ipem_disable(dev);

		gpuload = ipem_info->statistic_info->gpuload;
		table = ipem_info->table_info.table;
		action_next = ipem_info->statistic_info->action;

		if (!((table[action_next].lower_bound <= gpuload) && (gpuload <= table[action_next].upper_bound))) {
			UTLOG("ERROR: %d %d %d\n", table[action_next].lower_bound,  table[action_next].upper_bound, gpuload);
			return -4001;
		}
	}

	return 0;
}

long gpad_ipem_utst(struct gpad_device *dev, int param)
{
	/* return 0 if succeeded, error code otherwise. */

#define UT_TEST_ITEM(_name) { #_name, gpad_ipem_utst_ ## _name }

	static struct ut_test_item test_items[] = {
		UT_TEST_ITEM(timer), UT_TEST_ITEM(table), UT_TEST_ITEM(policy), };

	static struct ut_test_item test_long_items[] = {
		UT_TEST_ITEM(dvfs_lookup), };

	u32 cnt = sizeof(test_items) / sizeof(test_items[0]);
	u32 idx;
	struct ut_test_item *test_item;
	s64 ret;

	UTERR("gpad_ipem_utst() input : %d\n", param);

	if (param == 0) {
		for (idx = 0; idx < cnt; idx++) {
			test_item = &test_items[idx];

			ret = test_item->func(dev, param);
			if (0 == ret) {
				UTLOG("gpad_ipem_utst() %s:: PASSED...\n", test_item->name);
			} else {
				UTERR("gpad_ipem_utst() %s:: FAILED with %lld!!!\n", test_item->name, ret);
				return ret;
			}
		}
	}

	/* if (param > 0) { */
	test_item = &test_long_items[0];

	ret = test_item->func(dev, param);
	if (0 == ret) {
		UTLOG("gpad_ipem_utst() %s:: LONG PASSED...\n", test_item->name);
	} else {
		UTERR("gpad_ipem_utst() %s:: LONG FAILED with %lld!!!\n", test_item->name, ret);
		return ret;
	}
	/* } */



	UTLOG("gpad_ipem_utst() :: all passed...\n");

	return 0;
}

