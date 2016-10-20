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

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/slab.h> /* for memory allocation */
#include <linux/timer.h>
#include <linux/random.h>
#include "gpad_common.h"
#include "gpad_log.h"
#include "gpad_dvfs.h"
#include "mt_gpufreq.h"
#include "gs_gpufreq.h"

#define GET_DVFS_INFO(_dev) \
			(&((_dev)->dvfs_info))

/* FIXME, fill up the correct used power in each entry */
/* TODO: take talbe apart to save memory. */
struct mtk_gpufreq_info gs_dvfs_tables[][4] = {
	{
		{ .freq_khz = GPU_DVFS_F380, .volt = GPU_POWER_VGPU_1_15V, .power_mW = 0, .lower_bound =  0, .upper_bound = 100 }
	},
	{
		{ .freq_khz = GPU_DVFS_F480, .volt = GPU_POWER_VGPU_1_25V, .power_mW = 0, .lower_bound = 40, .upper_bound = 100 }, { .freq_khz = GPU_DVFS_F380, .volt = GPU_POWER_VGPU_1_15V, .power_mW = 0, .lower_bound =  0, .upper_bound =  40 }
	},
	{
		{ .freq_khz = GPU_DVFS_F550, .volt = GPU_POWER_VGPU_1_25V, .power_mW = 0, .lower_bound = 70, .upper_bound = 100 }, { .freq_khz = GPU_DVFS_F480, .volt = GPU_POWER_VGPU_1_25V, .power_mW = 0, .lower_bound = 30, .upper_bound =  70 }, { .freq_khz = GPU_DVFS_F380, .volt = GPU_POWER_VGPU_1_15V, .power_mW = 0, .lower_bound =  0, .upper_bound =  30 }
	}, };

unsigned int gs_dvfs_table_sizes[] = {1, 2, 3};




static void gpad_dvfs_timer_callback(unsigned long data);

static void gpad_dvfs_statistic_init(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_statistic_exit(struct gpad_dvfs_info *dvfs_info);

static void gpad_dvfs_policy_lookup_init(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_lookup_exit(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_lookahead_init(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_lookahead_exit(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_qlearn_init(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_qlearn_exit(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_sarsa_init(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_sarsa_exit(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_type1_init(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_type1_exit(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_type2_init(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_type2_exit(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_type4_init(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_policy_type4_exit(struct gpad_dvfs_info *dvfs_info);

static void gpad_dvfs_recorder_init(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_recorder_update_record(struct gpad_dvfs_info *dvfs_info);
static void gpad_dvfs_recorder_exit(struct gpad_dvfs_info *dvfs_info);

static void gpad_dvfs_event_thread(struct work_struct *work);
static void gpad_dvfs_schedule_work(struct gpad_dvfs_info *dvfs_info);


unsigned int gpad_dvfs_get_gpuload(struct gpad_dvfs_info *dvfs_info)
{
	/*
	unsigned int rand;

	if (dvfs_info->ut_gpuload >= 0 && dvfs_info->ut_gpuload <= 100) {
	    rand = dvfs_info->ut_gpuload;
	    dvfs_info->ut_gpuload = -1;
	    return rand;
	}

	get_random_bytes(&rand, sizeof(unsigned int));
	return rand%100;
	*/

	struct gpad_device *dev = gpad_get_default_dev();
	struct gpad_pm_info *pm_info = &(dev->pm_info);
	unsigned int gpuload;

	gpuload = (100 * (pm_info->stat.busy_time)) / (pm_info->stat.total_time);

	return gpuload;
}

static void gpad_dvfs_thermal_callback(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);

	gpad_dvfs_schedule_work(dvfs_info);
}

static void gpad_dvfs_touch_callback(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);

	gpad_dvfs_schedule_work(dvfs_info);
}

static void gpad_dvfs_timer_init(struct gpad_dvfs_info *dvfs_info)
{
	init_timer(&(dvfs_info->timer_info.timer));
	dvfs_info->timer_info.timer_interval = DVFS_TIMER_INTERVAL;
	dvfs_info->timer_info.timer.data = (uintptr_t)dvfs_info;
	dvfs_info->timer_info.timer.function = gpad_dvfs_timer_callback;
}

unsigned long gpad_dvfs_timer_get_interval(struct gpad_device *dev)
{
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);

	return dvfs_info->timer_info.timer_interval;
}

void gpad_dvfs_timer_set_interval(struct gpad_device *dev, unsigned long interval)
{
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);

	spin_lock_bh(&(dvfs_info->lock));
	if (interval != dvfs_info->timer_info.timer_interval)
		dvfs_info->timer_info.timer_interval = interval;
	spin_unlock_bh(&(dvfs_info->lock));

	GPAD_LOG_INFO("[DVFS] interval = %lu\n", interval);
}

static void gpad_dvfs_timer_callback(unsigned long data)
{
	struct gpad_dvfs_info *dvfs_info = (struct gpad_dvfs_info *)data;
	unsigned int action_next;
	unsigned int action_prev;

	spin_lock_bh(&(dvfs_info->lock));
	if (dvfs_info->enable == 0) {
		spin_unlock_bh(&(dvfs_info->lock));
		return;
	} else {
		spin_unlock_bh(&(dvfs_info->lock));
	}

	GPAD_LOG_INFO("[DVFS] gpad_dvfs_timer_callback: jiffies = %lu\n", jiffies);

	gpad_dvfs_statistic_update_statistic(dvfs_info);
	gpad_dvfs_recorder_update_record(dvfs_info);

	spin_lock_bh(&(dvfs_info->lock));
	action_next = dvfs_info->statistic_info.action_next;
	action_prev = dvfs_info->statistic_info.action_prev;
	spin_unlock_bh(&(dvfs_info->lock));

	if (action_next != action_prev) {
		gs_gpufreq_set_cur_index(action_next);
		gs_gpufreq_switch_handler();
	}

	spin_lock_bh(&(dvfs_info->lock));
	if (dvfs_info->enable == 1) {
		dvfs_info->timer_info.timer.expires = jiffies + dvfs_info->timer_info.timer_interval;
		spin_unlock_bh(&(dvfs_info->lock));

		add_timer(&(dvfs_info->timer_info.timer));
	} else {
		spin_unlock_bh(&(dvfs_info->lock));
	}

	GPAD_LOG_INFO("[DVFS] gpad_dvfs_timer_callback: action_prev = %u,  action_next = %u\n", action_prev, action_next);
}

static void gpad_dvfs_timer_exit(struct gpad_dvfs_info *dvfs_info)
{
	WARN_ON(dvfs_info->enable);
	del_timer_sync(&(dvfs_info->timer_info.timer));
}

static void gpad_dvfs_table_init(struct gpad_dvfs_info *dvfs_info)
{
	struct mtk_gpufreq_info *table;
	unsigned int table_id;
	unsigned int table_size;

	table = gs_dvfs_tables[0];
	table_size = gs_dvfs_table_sizes[0];
	table_id = 0;

	gs_gpufreq_register(table, table_size);

	spin_lock_bh(&(dvfs_info->lock));
	dvfs_info->table_info.table = table;
	dvfs_info->table_info.table_id = table_id;
	spin_unlock_bh(&(dvfs_info->lock));

	GPAD_LOG_INFO("[DVFS] table_id = %u, table_size = %u\n", table_id, table_size);
}

unsigned int gpad_dvfs_table_get_table(struct gpad_device *dev)
{
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);

	return dvfs_info->table_info.table_id;
}

void gpad_dvfs_table_set_table(struct gpad_device *dev, unsigned int table_id)
{
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);

	spin_lock_bh(&(dvfs_info->lock));
	if (table_id != dvfs_info->table_info.table_id) {
		dvfs_info->table_info.table = gs_dvfs_tables[table_id];
		dvfs_info->table_info.table_id = table_id;
		spin_unlock_bh(&(dvfs_info->lock));

		/* unregister the previous table to free memory */
		gs_gpufreq_unregister();
		gs_gpufreq_register(gs_dvfs_tables[table_id], gs_dvfs_table_sizes[table_id]);

		GPAD_LOG_INFO("[DVFS] table_id = %u\n", table_id);
	} else {
		spin_unlock_bh(&(dvfs_info->lock));
	}
}

static void gpad_dvfs_table_exit(struct gpad_dvfs_info *dvfs_info)
{
	gs_gpufreq_unregister();

	spin_lock_bh(&(dvfs_info->lock));
	dvfs_info->table_info.table = NULL;
	spin_unlock_bh(&(dvfs_info->lock));
}

static void gpad_dvfs_statistic_init(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

void gpad_dvfs_statistic_update_statistic(struct gpad_dvfs_info *dvfs_info)
{
	unsigned long timestamp;
	unsigned int gpuload;
	unsigned int action_llimit;
	unsigned int action_ulimit;
	unsigned int action_prev;

	/* get timestamp */
	timestamp = cpu_clock(0);

	/* get statistic from [gpad][pm] */
	gpuload = gpad_dvfs_get_gpuload(dvfs_info);

	/* get statistic from [gpufreq] */
	gs_gpufreq_get_cur_indices(&action_ulimit, &action_llimit, &action_prev);

	/* update statistic */
	spin_lock_bh(&(dvfs_info->lock));
	dvfs_info->statistic_info.timestamp = timestamp;
	dvfs_info->statistic_info.gpuload = gpuload;
	dvfs_info->statistic_info.action_llimit = action_llimit;
	dvfs_info->statistic_info.action_prev = action_prev;
	dvfs_info->statistic_info.action_ulimit = action_ulimit;

	/* get action_next from policy */
	dvfs_info->policy_info.policy(dvfs_info);
	spin_unlock_bh(&(dvfs_info->lock));

	GPAD_LOG_INFO("[DVFS] gpad_dvfs_statistic_update_statistic: gpuload = %u,  action_llimit = %u, action_prev = %u, action_ulimit = %u\n", gpuload, action_llimit, action_prev, action_ulimit);
}

static void gpad_dvfs_statistic_exit(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

static void gpad_dvfs_policy_init(struct gpad_dvfs_info *dvfs_info)
{
	gpad_dvfs_policy_lookup_init(dvfs_info);
	gpad_dvfs_policy_lookahead_init(dvfs_info);
	gpad_dvfs_policy_qlearn_init(dvfs_info);
	gpad_dvfs_policy_sarsa_init(dvfs_info);
	gpad_dvfs_policy_type1_init(dvfs_info);
	gpad_dvfs_policy_type2_init(dvfs_info);
	gpad_dvfs_policy_type4_init(dvfs_info);

	spin_lock_bh(&(dvfs_info->lock));
	dvfs_info->policy_info.policy = gpad_dvfs_policy_lookup;
	dvfs_info->policy_info.policy_id = DVFS_POLICY_LOOKUP;
	spin_unlock_bh(&(dvfs_info->lock));
}

unsigned int gpad_dvfs_policy_get_policy(struct gpad_device *dev)
{
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);

	return dvfs_info->policy_info.policy_id;
}

void gpad_dvfs_policy_set_policy(struct gpad_device *dev, unsigned int policy_id)
{
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);
	gpad_dvfs_policy_func policy;

	spin_lock_bh(&(dvfs_info->lock));
	if (policy_id != dvfs_info->policy_info.policy_id) {
		switch (policy_id) {
		default:
			policy_id = DVFS_POLICY_LOOKUP;
		case DVFS_POLICY_LOOKUP:
			policy = gpad_dvfs_policy_lookup;
			break;
		case DVFS_POLICY_LOOKAHEAD:
			policy = gpad_dvfs_policy_lookahead;
			break;
		case DVFS_POLICY_QLEARN:
			policy = gpad_dvfs_policy_qlearn;
			break;
		case DVFS_POLICY_SARSA:
			policy = gpad_dvfs_policy_sarsa;
			break;
		case DVFS_POLICY_TYPE1:
			policy = gpad_dvfs_policy_type1;
			break;
		case DVFS_POLICY_TYPE2:
			policy = gpad_dvfs_policy_type2;
			break;
		case DVFS_POLICY_TYPE4:
			policy = gpad_dvfs_policy_type4;
			break;
		}

		dvfs_info->policy_info.policy = policy;
		dvfs_info->policy_info.policy_id = policy_id;
	}
	spin_unlock_bh(&(dvfs_info->lock));

	GPAD_LOG_INFO("[DVFS] policy_id = %u\n", policy_id);
}

static void gpad_dvfs_policy_exit(struct gpad_dvfs_info *dvfs_info)
{
	gpad_dvfs_policy_lookup_exit(dvfs_info);
	gpad_dvfs_policy_lookahead_exit(dvfs_info);
	gpad_dvfs_policy_qlearn_exit(dvfs_info);
	gpad_dvfs_policy_sarsa_exit(dvfs_info);
	gpad_dvfs_policy_type1_exit(dvfs_info);
	gpad_dvfs_policy_type2_exit(dvfs_info);
	gpad_dvfs_policy_type4_exit(dvfs_info);

	spin_lock_bh(&(dvfs_info->lock));
	dvfs_info->policy_info.policy = NULL;
	spin_unlock_bh(&(dvfs_info->lock));
}

static void gpad_dvfs_policy_lookup_init(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

/*
 * Assumption: DVFS's lock has been held.
 */
void gpad_dvfs_policy_lookup(void *param)
{
	struct gpad_dvfs_info *dvfs_info = (struct gpad_dvfs_info *)param;
	struct mtk_gpufreq_info *table;
	unsigned int gpuload;
	unsigned int action_llimit;
	unsigned int action_ulimit;
	unsigned int action_next;

	table = dvfs_info->table_info.table;
	gpuload = dvfs_info->statistic_info.gpuload;
	action_llimit = dvfs_info->statistic_info.action_llimit;
	action_ulimit = dvfs_info->statistic_info.action_ulimit;

	WARN_ON(action_llimit > action_ulimit);
	/* binary search for action_next */
	while (action_llimit < action_ulimit) {
		action_next = (action_ulimit + action_llimit) / 2;

		if (gpuload < table[action_next].lower_bound)
			action_llimit = action_next + 1;
		else
			action_ulimit = action_next;
	}
	action_next = action_llimit;

	dvfs_info->statistic_info.action_next = action_next;
}

static void gpad_dvfs_policy_lookup_exit(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

static void gpad_dvfs_policy_lookahead_init(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

/*
 * Assumption: DVFS's lock has been held.
 */
void gpad_dvfs_policy_lookahead(void *param)
{
	GPAD_UNUSED(param);
	/* implement LookAhead here*/
	/*
	struct mtk_gpufreq_info *table;

	unsigned int gpuload;

	unsigned int action_llimit;
	unsigned int action_ulimit;

	unsigned int action_next;

	WARN_ON(action_llimit > action_ulimit);
	*/
	/* FIXME, handle events and the corresponding action here */
	/*
	if () {
	    return;
	} else if () {
	    return;
	} else if () {
	    return;
	}
	*/

	/* fall back to "lookup" if no event matched */
	/* gpad_dvfs_policy_lookup(); */
}

static void gpad_dvfs_policy_lookahead_exit(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

static void gpad_dvfs_policy_qlearn_init(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

/*
 * Assumption: DVFS's lock has been held.
 */
void gpad_dvfs_policy_qlearn(void *param)
{
	GPAD_UNUSED(param);
	/* implement Q-Learning here */
}

static void gpad_dvfs_policy_qlearn_exit(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

static void gpad_dvfs_policy_sarsa_init(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

/*
 * Assumption: DVFS's lock has been held.
 */
void gpad_dvfs_policy_sarsa(void *param)
{
	GPAD_UNUSED(param);
	/* implement SARSA here */
}

static void gpad_dvfs_policy_sarsa_exit(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

static void gpad_dvfs_policy_type1_init(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

/*
 * Assumption: DVFS's lock has been held.
 */
void gpad_dvfs_policy_type1(void *param)
{
	struct gpad_dvfs_info *dvfs_info = (struct gpad_dvfs_info *)param;

	/* implement Type1 here */
	struct mtk_gpufreq_info *table;
	unsigned int table_id;
	unsigned int table_size;

	unsigned int gpuload;

	unsigned int action_llimit;
	unsigned int action_ulimit;

	unsigned int action_prev;
	unsigned int action_next;

	table = dvfs_info->table_info.table;
	table_id = dvfs_info->table_info.table_id;
	table_size = gs_dvfs_table_sizes[table_id];

	action_llimit = dvfs_info->statistic_info.action_llimit;
	action_ulimit = dvfs_info->statistic_info.action_ulimit;
	WARN_ON(action_llimit > action_ulimit);

	gpuload = dvfs_info->statistic_info.gpuload;

	action_prev = dvfs_info->statistic_info.action_prev;

	action_next = action_prev;
	if (80 < gpuload)
		action_next -= 1;
	else if (gpuload < 50)
		action_next += 1;

	/* TODO: a better way to do underflow protect */
	if ((int)action_ulimit < (int)action_next)
		action_next = action_ulimit;
	else if ((int)action_next < (int)action_llimit)
		action_next = action_llimit;

	dvfs_info->statistic_info.action_next = action_next;
}

static void gpad_dvfs_policy_type1_exit(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

static void gpad_dvfs_policy_type2_init(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

/*
 * Assumption: DVFS's lock has been held.
 */
void gpad_dvfs_policy_type2(void *param)
{
	struct gpad_dvfs_info *dvfs_info = (struct gpad_dvfs_info *)param;

	/* implement Type2 here */
	struct mtk_gpufreq_info *table;
	unsigned int table_id;
	unsigned int table_size;

	unsigned int gpuload;

	unsigned int action_llimit;
	unsigned int action_ulimit;

	unsigned int action_prev;
	unsigned int action_next;

	table = dvfs_info->table_info.table;
	table_id = dvfs_info->table_info.table_id;
	table_size = gs_dvfs_table_sizes[table_id];

	action_llimit = dvfs_info->statistic_info.action_llimit;
	action_ulimit = dvfs_info->statistic_info.action_ulimit;
	WARN_ON(action_llimit > action_ulimit);

	gpuload = dvfs_info->statistic_info.gpuload;

	action_prev = dvfs_info->statistic_info.action_prev;

	action_next = action_prev;
	if (98 < gpuload && gpuload <= 100)
		action_next = action_llimit;
	else if (84 < gpuload && gpuload <= 98)
		action_next -= 2;
	else if (69 < gpuload && gpuload <= 84)
		action_next -= 1;
	else if (30 < gpuload && gpuload <= 50)
		action_next += 1;
	else if (1 < gpuload && gpuload <= 30)
		action_next += 2;
	else if (0 < gpuload && gpuload <= 1)
		action_next = action_ulimit;

	/* TODO: a better way to do underflow protect */
	if ((int)action_ulimit < (int)action_next)
		action_next = action_ulimit;
	else if ((int)action_next < (int)action_llimit)
		action_next = action_llimit;

	dvfs_info->statistic_info.action_next = action_next;
}

static void gpad_dvfs_policy_type2_exit(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

static void gpad_dvfs_policy_type4_init(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

/*
 * Assumption: DVFS's lock has been held.
 */
void gpad_dvfs_policy_type4(void *param)
{
	struct gpad_dvfs_info *dvfs_info = (struct gpad_dvfs_info *)param;

	/* implement Type4 here */
	struct mtk_gpufreq_info *table;
	unsigned int table_id;
	unsigned int table_size;

	unsigned int gpuload_prev;
	unsigned int gpuload_next;

	unsigned int action_llimit;
	unsigned int action_ulimit;

	unsigned int action_prev;
	unsigned int action_next;

	table = dvfs_info->table_info.table;
	table_id = dvfs_info->table_info.table_id;
	table_size = gs_dvfs_table_sizes[table_id];

	action_llimit = dvfs_info->statistic_info.action_llimit;
	action_ulimit = dvfs_info->statistic_info.action_ulimit;
	WARN_ON(action_llimit > action_ulimit);

	gpuload_prev = dvfs_info->policy_info.type4_info.gpuload_next;
	gpuload_next = dvfs_info->statistic_info.gpuload;

	action_prev = dvfs_info->statistic_info.action_prev;

	action_next = action_prev;
	if (98 < gpuload_next && gpuload_next <= 100)
		action_next = action_llimit;
	else if (84 < gpuload_next && gpuload_next <= 98) {
		action_next -= 2;
		if (((gpuload_prev * 170) / 100) < gpuload_next)
			action_next -= 1;
	} else if (69 < gpuload_next && gpuload_next <= 84) {
		action_next -= 1;
		if (((gpuload_prev * 170) / 100) < gpuload_next)
			action_next -= 1;
	} else if (30 < gpuload_next && gpuload_next <= 50) {
		action_next += 1;
		if (((gpuload_next * 170) / 100) < gpuload_prev)
			action_next += 1;
	} else if (1 < gpuload_next && gpuload_next <= 30) {
		action_next += 2;
		if (((gpuload_next * 170) / 100) < gpuload_prev)
			action_next += 1;
	} else if (0 < gpuload_next && gpuload_next <= 1)
		action_next = action_ulimit;

	/* TODO: a better way to do underflow protect */
	if ((int)action_ulimit < (int)action_next)
		action_next = action_ulimit;
	else if ((int)action_next < (int)action_llimit)
		action_next = action_llimit;

	dvfs_info->policy_info.type4_info.gpuload_prev = gpuload_prev;
	dvfs_info->policy_info.type4_info.gpuload_next = gpuload_next;

	dvfs_info->statistic_info.action_next = action_next;
}

static void gpad_dvfs_policy_type4_exit(struct gpad_dvfs_info *dvfs_info)
{
	GPAD_UNUSED(dvfs_info);
}

static void gpad_dvfs_recorder_init(struct gpad_dvfs_info *dvfs_info)
{
	spin_lock_bh(&(dvfs_info->lock));
	dvfs_info->recorder_info.records = kzalloc(DVFS_RECORDER_SIZE, GFP_KERNEL);
	dvfs_info->recorder_info.recordid = 0;
	spin_unlock_bh(&(dvfs_info->lock));
}

static void gpad_dvfs_recorder_update_record(struct gpad_dvfs_info *dvfs_info)
{
	unsigned int recordid;

	spin_lock_bh(&(dvfs_info->lock));
	recordid = dvfs_info->recorder_info.recordid;

	memcpy(&(dvfs_info->recorder_info.records[recordid]), &(dvfs_info->statistic_info), sizeof(struct gpad_dvfs_statistic_info));

	recordid++;
	if (recordid >= (DVFS_RECORDER_SIZE / sizeof(struct gpad_dvfs_statistic_info)))
		recordid = 0;

	dvfs_info->recorder_info.recordid = recordid;
	spin_unlock_bh(&(dvfs_info->lock));
}

static void gpad_dvfs_recorder_exit(struct gpad_dvfs_info *dvfs_info)
{
	kfree(dvfs_info->recorder_info.records);
}

int gpad_dvfs_init(struct gpad_device *dev)
{
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);
	struct mtk_gpufreq_kmif kmif_ops = {
		.notify_thermal_event = gpad_dvfs_thermal_callback, .notify_touch_event = gpad_dvfs_touch_callback
	};

	GPAD_LOG_INFO("[DVFS] gpad_dvfs_init...\n");

	spin_lock_init(&(dvfs_info->lock));
	dvfs_info->enable = 0;
	dvfs_info->ut_gpuload = -1;

	gpad_dvfs_timer_init(dvfs_info);
	gpad_dvfs_table_init(dvfs_info);
	gpad_dvfs_statistic_init(dvfs_info);
	gpad_dvfs_policy_init(dvfs_info);
	gpad_dvfs_recorder_init(dvfs_info);

	INIT_WORK(&(dvfs_info->work), gpad_dvfs_event_thread);
	spin_lock_bh(&(dvfs_info->lock));
	dvfs_info->work_ref_cnt = 1;
	spin_unlock_bh(&(dvfs_info->lock));

	gs_gpufreq_kmif_register(&kmif_ops);

	GPAD_LOG_INFO("[DVFS] gpad_dvfs_init...done\n");
	return 0;
}

static void gpad_dvfs_event_thread(struct work_struct *work)
{
	struct gpad_dvfs_info *dvfs_info = container_of(work, struct gpad_dvfs_info, work);

	spin_lock_bh(&(dvfs_info->lock));
	if (dvfs_info->enable == 1) {
		spin_unlock_bh(&(dvfs_info->lock));

		del_timer_sync(&(dvfs_info->timer_info.timer));
	} else {
		spin_unlock_bh(&(dvfs_info->lock));
	}

	gpad_dvfs_timer_callback((uintptr_t)dvfs_info);

	spin_lock_bh(&(dvfs_info->lock));
	dvfs_info->work_ref_cnt--;
	spin_unlock_bh(&(dvfs_info->lock));
}

static void gpad_dvfs_schedule_work(struct gpad_dvfs_info *dvfs_info)
{
	spin_lock_bh(&(dvfs_info->lock));
	if (likely(dvfs_info->work_ref_cnt > 0)) {
		dvfs_info->work_ref_cnt++;
		spin_unlock_bh(&(dvfs_info->lock));

		schedule_work(&(dvfs_info->work));
	} else {
		spin_unlock_bh(&(dvfs_info->lock));
	}
}

int gpad_dvfs_exit(struct gpad_device *dev)
{
	struct gpad_dvfs_info   *dvfs_info = &(dev->dvfs_info);

	GPAD_LOG_INFO("[DVFS] gpad_dvfs_exit...\n");

	gs_gpufreq_kmif_unregister();

	gpad_dvfs_disable(dev);

	spin_lock_bh(&(dvfs_info->lock));
	dvfs_info->work_ref_cnt--;
	if (dvfs_info->work_ref_cnt > 0) {
		spin_unlock_bh(&(dvfs_info->lock));

		cancel_work_sync(&dvfs_info->work);
	} else {
		spin_unlock_bh(&(dvfs_info->lock));
	}

	gpad_dvfs_timer_exit(dvfs_info);
	gpad_dvfs_table_exit(dvfs_info);
	gpad_dvfs_statistic_exit(dvfs_info);
	gpad_dvfs_policy_exit(dvfs_info);
	gpad_dvfs_recorder_exit(dvfs_info);

	gs_gpufreq_unregister();

	GPAD_LOG_INFO("[DVFS] gpad_dvfs_exit...done\n");
	return 0;
}

int gpad_dvfs_enable(struct gpad_device *dev)
{
	/* WARNING, use external dvfs */
	/* mtk_gpufreq_dvfs_disable(); */
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);

	spin_lock_bh(&(dvfs_info->lock));
	if (dvfs_info->enable == 0) {
		dvfs_info->timer_info.timer.expires = jiffies + dvfs_info->timer_info.timer_interval;
		dvfs_info->enable = 1;
		spin_unlock_bh(&(dvfs_info->lock));

		add_timer(&(dvfs_info->timer_info.timer));
	} else {
		spin_unlock_bh(&(dvfs_info->lock));
	}

	GPAD_LOG_INFO("[DVFS] dvfs is enabled\n");
	return 0;
}

unsigned int gpad_dvfs_get_enable(struct gpad_device *dev)
{
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);

	return dvfs_info->enable;
}

int gpad_dvfs_disable(struct gpad_device *dev)
{
	struct gpad_dvfs_info *dvfs_info = GET_DVFS_INFO(dev);

	spin_lock_bh(&(dvfs_info->lock));
	if (dvfs_info->enable == 1) {
		dvfs_info->enable = 0;
		spin_unlock_bh(&(dvfs_info->lock));

		del_timer_sync(&(dvfs_info->timer_info.timer));
	} else {
		spin_unlock_bh(&(dvfs_info->lock));
	}


	/* WARNING, use built-in dvfs in gpufreq */
	/* mtk_gpufreq_dvfs_enable(); */

	GPAD_LOG_INFO("[DVFS] dvfs is disabled\n");
	return 0;
}
