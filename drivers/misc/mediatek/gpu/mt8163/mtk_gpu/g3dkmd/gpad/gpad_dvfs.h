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

#ifndef _GPAD_DVFS_H
#define _GPAD_DVFS_H
#include <linux/types.h>
#include "mt_gpufreq.h"
#include "gs_gpufreq.h"

struct gpad_device;

unsigned long gpad_dvfs_timer_get_interval(struct gpad_device *dev);
void gpad_dvfs_timer_set_interval(struct gpad_device *dev, unsigned long interval);

unsigned int gpad_dvfs_table_get_table(struct gpad_device *dev);
void gpad_dvfs_table_set_table(struct gpad_device *dev, unsigned int table_id);

unsigned int gpad_dvfs_policy_get_policy(struct gpad_device *dev);
void gpad_dvfs_policy_set_policy(struct gpad_device *dev, unsigned int policy_id);

int gpad_dvfs_init(struct gpad_device *dev);
int gpad_dvfs_exit(struct gpad_device *dev);
int gpad_dvfs_enable(struct gpad_device *dev);
unsigned int gpad_dvfs_get_enable(struct gpad_device *dev);
int gpad_dvfs_disable(struct gpad_device *dev);
void gpad_dvfs_statistic_update_statistic(struct gpad_dvfs_info *dvfs_info);
void gpad_dvfs_policy_lookup(void *param);
void gpad_dvfs_policy_lookahead(void *param);
void gpad_dvfs_policy_qlearn(void *param);
void gpad_dvfs_policy_sarsa(void *param);
void gpad_dvfs_policy_type1(void *param);
void gpad_dvfs_policy_type2(void *param);
void gpad_dvfs_policy_type4(void *param);

#define DVFS_VERSION                            "0.2.5"

#define DVFS_POLICY_LOOKUP                      0
#define DVFS_POLICY_LOOKAHEAD                   1
#define DVFS_POLICY_QLEARN                      2
#define DVFS_POLICY_SARSA                       3
#define DVFS_POLICY_TYPE1                       4
#define DVFS_POLICY_TYPE2                       5
#define DVFS_POLICY_TYPE4                       6

/* Default timer_interval = 100ms */
#define DVFS_TIMER_INTERVAL                     ((HZ * 100) / 1000)

/* Default recorder_size = 1mb */
#define DVFS_RECORDER_SIZE                      (1024 * 1024)

struct gpad_dvfs_timer_info {
	struct timer_list                           timer;
	unsigned long                               timer_interval;
};

struct gpad_dvfs_table_info {
	struct mtk_gpufreq_info                     *table;
	unsigned int                                table_id;
};

struct gpad_dvfs_statistic_info {
	unsigned long                               timestamp;

	unsigned int                                gpuload;

	unsigned int                                action_next;
	unsigned int                                action_prev;

	/* The action id for thermal limit */
	unsigned int                                action_llimit;

	/* The action id for touch limit */
	unsigned int                                action_ulimit;
};

struct gpad_dvfs_policy_lookup_info {
};

struct gpad_dvfs_policy_lookahead_info {
};

struct gpad_dvfs_policy_qlearn_info {
};

struct gpad_dvfs_policy_sarsa_info {
};

struct gpad_dvfs_policy_type1_info {
};

struct gpad_dvfs_policy_type2_info {
};

struct gpad_dvfs_policy_type4_info {
	unsigned int gpuload_prev;
	unsigned int gpuload_next;
};

typedef void (*gpad_dvfs_policy_func)(void *);

struct gpad_dvfs_policy_info {
	gpad_dvfs_policy_func                       policy;

	unsigned int                                policy_id;

	struct gpad_dvfs_policy_lookup_info         lookup_info;
	struct gpad_dvfs_policy_lookahead_info      lookahead_info;
	struct gpad_dvfs_policy_qlearn_info         qlearn_info;
	struct gpad_dvfs_policy_sarsa_info          sarsa_info;
	struct gpad_dvfs_policy_type1_info          type1_info;
	struct gpad_dvfs_policy_type2_info          type2_info;
	struct gpad_dvfs_policy_type4_info          type4_info;
};

struct gpad_dvfs_recorder_info {
	struct gpad_dvfs_statistic_info         *records;
	unsigned int                            recordid;
};

struct gpad_dvfs_info {
	spinlock_t                                  lock;

	struct work_struct                          work;
	int                                         work_ref_cnt; /* 0: N/A, 1: ready, 2+: queuing */

	unsigned int                                enable;
	unsigned int                                ut_gpuload;

	struct gpad_dvfs_timer_info                 timer_info;

	struct gpad_dvfs_table_info                 table_info;

	struct gpad_dvfs_statistic_info             statistic_info;

	struct gpad_dvfs_policy_info                policy_info;

	struct gpad_dvfs_recorder_info              recorder_info;
};

#endif /* #define _GPAD_DVFS_H */
