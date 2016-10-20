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
#include "gpad_dvfs.h"
#include "gpad_dvfs_utst.h"




#define UTLOG(fmt, args...) \
	GPAD_LOG("[DVFS][UTLOG]" fmt, ##args)

#define UTERR(fmt, args...) \
	GPAD_LOG_ERR("[DVFS][UTERR]" fmt, ##args)



typedef int (*ut_test_func)(struct gpad_device *dev);

struct ut_test_item {
	char           *name;
	ut_test_func    func;
};

static int gpad_dvfs_utst_timer(struct gpad_device *dev)
{
	unsigned int loop;
	unsigned int rand;
	unsigned int interval;

	/* interval get/set test */
	for (loop = DVFS_UTS_RAND_LOOP; loop > 0; loop--) {
		get_random_bytes(&rand, sizeof(unsigned int));
		rand = rand % HZ;

		gpad_dvfs_timer_set_interval(dev, rand);
		interval = gpad_dvfs_timer_get_interval(dev);

		if (interval != rand)
			return -1;
	}

	return 0;
}

static int gpad_dvfs_utst_table(struct gpad_device *dev)
{
	unsigned int loop;
	unsigned int rand;
	unsigned int table_id;

	/* table get/set test */
	for (loop = DVFS_UTS_RAND_LOOP; loop > 0; loop--) {
		get_random_bytes(&rand, sizeof(unsigned int));
		rand = rand % 2;

		gpad_dvfs_table_set_table(dev, rand);
		table_id = gpad_dvfs_table_get_table(dev);

		if (table_id != rand)
			return -1;
	}

	return 0;
}

static int gpad_dvfs_utst_policy(struct gpad_device *dev)
{
	unsigned int loop;
	unsigned int rand;
	unsigned int policy_id;

	/* policy get/set test */
	for (loop = DVFS_UTS_RAND_LOOP; loop > 0; loop--) {
		get_random_bytes(&rand, sizeof(unsigned int));
		rand = rand % 4;

		gpad_dvfs_policy_set_policy(dev, rand);
		policy_id = gpad_dvfs_policy_get_policy(dev);

		if (policy_id != rand)
			return -1;
	}

	return 0;
}

static int gpad_dvfs_utst_policy_lookup(struct gpad_device *dev)
{
	struct gpad_dvfs_info *dvfs_info = &dev->dvfs_info;
	unsigned int gpuload;
	struct mtk_gpufreq_info *table;
	unsigned int action_next;

	/* policy "lookup" test */
	gpad_dvfs_policy_set_policy(dev, 0);
	for (gpuload = 0; gpuload <= 100; gpuload++) {
		dvfs_info->ut_gpuload = gpuload; /* fake GPU loading */

		gpad_dvfs_statistic_update_statistic(dvfs_info);

		table = dvfs_info->table_info.table;
		action_next = dvfs_info->statistic_info.action_next;

		if (!((table[action_next].lower_bound <= gpuload) && (gpuload <= table[action_next].upper_bound)))
			return -1;
	}

	return 0;
}

long gpad_dvfs_utst(struct gpad_device *dev, int param)
{
	/* return 0 if succeeded, error code otherwise. */

#define UT_TEST_ITEM(_name) { #_name, gpad_dvfs_utst_ ## _name }

	static struct ut_test_item test_items[] = {
		UT_TEST_ITEM(timer), UT_TEST_ITEM(table), UT_TEST_ITEM(policy), UT_TEST_ITEM(policy_lookup), };
	unsigned int cnt = sizeof(test_items) / sizeof(test_items[0]);
	unsigned int idx;
	struct ut_test_item *test_item;
	int ret;

	for (idx = 0; idx < cnt; idx++) {
		test_item = &test_items[idx];

		ret = test_item->func(dev);
		if (0 == ret) {
			UTLOG("%s: PASSED...\n", test_item->name);
		} else {
			UTERR("%s: FAILED with %d!!!\n", test_item->name, ret);
			return -1;
		}
	}

	UTLOG("all passed...\n");

	return 0;
}

