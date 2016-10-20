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
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/random.h>
#include "gpad_common.h"
#include "gs_gpufreq.h"
#include "gpad_ipem.h"
#include "gpad_ipem_dvfs.h"




/*
 * *******************************************************************
 *  function prototype
 * *******************************************************************
 */
static void gpad_ipem_dvfs_disable_init(struct gpad_device *dev);
static void gpad_ipem_dvfs_disable_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dvfs_disable_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dvfs_disable_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dvfs_lookup_init(struct gpad_device *dev);
static void gpad_ipem_dvfs_lookup_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dvfs_lookup_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dvfs_lookup_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dvfs_thermal_init(struct gpad_device *dev);
static void gpad_ipem_dvfs_thermal_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dvfs_thermal_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dvfs_thermal_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dvfs_lookahead_init(struct gpad_device *dev);
static void gpad_ipem_dvfs_lookahead_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dvfs_lookahead_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dvfs_lookahead_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dvfs_qlearn_init(struct gpad_device *dev);
static void gpad_ipem_dvfs_qlearn_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dvfs_qlearn_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dvfs_qlearn_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dvfs_sarsa_init(struct gpad_device *dev);
static void gpad_ipem_dvfs_sarsa_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dvfs_sarsa_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dvfs_sarsa_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dvfs_type1_init(struct gpad_device *dev);
static void gpad_ipem_dvfs_type1_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dvfs_type1_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dvfs_type1_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dvfs_type2_init(struct gpad_device *dev);
static void gpad_ipem_dvfs_type2_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dvfs_type2_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dvfs_type2_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dvfs_type4_init(struct gpad_device *dev);
static void gpad_ipem_dvfs_type4_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dvfs_type4_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dvfs_type4_update(struct gpad_ipem_work_info *work_info);




/*
 * *******************************************************************
 *  globle variable
 * *******************************************************************
 */
struct gpad_ipem_dvfs_policy_info g_ipem_dvfs_policies[] = {
	{.name =   "disable", .init_func =   gpad_ipem_dvfs_disable_init, .exit_func =   gpad_ipem_dvfs_disable_exit, .do_func =   gpad_ipem_dvfs_disable_do, .update_func =   gpad_ipem_dvfs_disable_update}, {.name =    "lookup", .init_func =    gpad_ipem_dvfs_lookup_init, .exit_func =    gpad_ipem_dvfs_lookup_exit, .do_func =    gpad_ipem_dvfs_lookup_do, .update_func =	gpad_ipem_dvfs_lookup_update}, {.name =   "thermal", .init_func =   gpad_ipem_dvfs_thermal_init, .exit_func =   gpad_ipem_dvfs_thermal_exit, .do_func =   gpad_ipem_dvfs_thermal_do, .update_func =   gpad_ipem_dvfs_thermal_update}, {.name = "lookahead", .init_func = gpad_ipem_dvfs_lookahead_init, .exit_func = gpad_ipem_dvfs_lookahead_exit, .do_func = gpad_ipem_dvfs_lookahead_do, .update_func = gpad_ipem_dvfs_lookahead_update}, {.name =    "qlearn", .init_func =    gpad_ipem_dvfs_qlearn_init, .exit_func =    gpad_ipem_dvfs_qlearn_exit, .do_func =    gpad_ipem_dvfs_qlearn_do, .update_func =	gpad_ipem_dvfs_qlearn_update}, {.name =     "sarsa", .init_func =     gpad_ipem_dvfs_sarsa_init, .exit_func =     gpad_ipem_dvfs_sarsa_exit, .do_func =     gpad_ipem_dvfs_sarsa_do, .update_func =	 gpad_ipem_dvfs_sarsa_update}, {.name =     "type1", .init_func =     gpad_ipem_dvfs_type1_init, .exit_func =     gpad_ipem_dvfs_type1_exit, .do_func =     gpad_ipem_dvfs_type1_do, .update_func =	 gpad_ipem_dvfs_type1_update}, {.name =     "type2", .init_func =     gpad_ipem_dvfs_type2_init, .exit_func =     gpad_ipem_dvfs_type2_exit, .do_func =     gpad_ipem_dvfs_type2_do, .update_func =	 gpad_ipem_dvfs_type2_update}, {.name =     "type4", .init_func =     gpad_ipem_dvfs_type4_init, .exit_func =     gpad_ipem_dvfs_type4_exit, .do_func =     gpad_ipem_dvfs_type4_do, .update_func =	 gpad_ipem_dvfs_type4_update}, };


static unsigned long g_touch_finish_timestamp;


/*
 * *******************************************************************
 *  dvfs disable related
 * *******************************************************************
 */
static void gpad_ipem_dvfs_disable_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dvfs_disable_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dvfs_disable_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Disable Do here */
	return ipem_info->tracker_info.action_prev;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dvfs_disable_update(struct gpad_ipem_work_info *work_info)
{
	/* implement Disable Update here */
}




/*
 * *******************************************************************
 *  dvfs lookup related
 * *******************************************************************
 */
static void gpad_ipem_dvfs_lookup_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dvfs_lookup_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dvfs_lookup_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement LookUp Do here */
	struct mtk_gpufreq_info *table;
	u32 table_size;

	u32 gpuload;

	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;

	gpad_ipem_action action_next;

	table = ipem_info->table_info.table;
	table_size = ipem_info->table_info.table_size;

	/* TODO: not to use table_size as the number of actions */
	action_llimit = 0;
	action_ulimit = table_size - 1;
	if (ipem_info->table_info.table[action_ulimit].volt == 0) {
		action_ulimit--;
	}
	WARN_ON(action_llimit > action_ulimit);

	gpuload = ipem_info->statistic_info->gpuload;

	action_next = gpad_ipem_tsearch_do(table, action_llimit, action_ulimit, gpuload);

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dvfs_lookup_update(struct gpad_ipem_work_info *work_info)
{
	/* implement LookUp Update here */
}




/*
 * *******************************************************************
 *  dvfs thermal related
 * *******************************************************************
 */
static void gpad_ipem_dvfs_thermal_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dvfs_thermal_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dvfs_thermal_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Thermal Do here */
	struct mtk_gpufreq_info *table;
	u32 table_size;

	u32 gpuload;

	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;

	gpad_ipem_action action_next;

	table = ipem_info->table_info.table;
	table_size = ipem_info->table_info.table_size;

	/* TODO: not to use table_size as the number of actions */
	action_llimit = ipem_info->statistic_info->action_llimit;
	action_ulimit = table_size - 1;
	if (ipem_info->table_info.table[action_ulimit].volt == 0) {
		action_ulimit--;
	}
	WARN_ON(action_llimit > action_ulimit);

	gpuload = ipem_info->statistic_info->gpuload;

	action_next = gpad_ipem_tsearch_do(table, action_llimit, action_ulimit, gpuload);

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dvfs_thermal_update(struct gpad_ipem_work_info *work_info)
{
	/* implement Thermal Update here */
}




/*
 * *******************************************************************
 *  dvfs lookahead related
 * *******************************************************************
 */
static void gpad_ipem_dvfs_lookahead_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dvfs_lookahead_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dvfs_lookahead_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement LookAhead Do here */
	struct mtk_gpufreq_info *table;
	struct gpad_ipem_statistic_info *stat;
	u32 table_size;

	u32 gpuload;
	u32 thermal;
	u32 touch;
	unsigned long timestamp;

	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;

	gpad_ipem_action action_next;

	table = ipem_info->table_info.table;
	table_size = ipem_info->table_info.table_size;
	stat = ipem_info->statistic_info;

	/* TODO: not to use table_size as the number of actions */
	action_llimit = stat->action_llimit;
	action_ulimit = stat->action_ulimit;
	timestamp     = stat->timestamp;
	if (ipem_info->table_info.table[action_ulimit].volt == 0) {
		action_ulimit--;
	}
	WARN_ON(action_llimit > action_ulimit);

	if (work_info->performance_mode > 0)
		action_ulimit = action_llimit;

	thermal = ipem_info->statistic_info->gpuload;
	touch   = ipem_info->statistic_info->touch;

	if (touch) {
		g_touch_finish_timestamp = timestamp + 500*1000*1000;

		/* touch effect */
		action_ulimit = action_llimit;

		/* overflow for each 292 years. but no harm */
		if (unlikely(g_touch_finish_timestamp < timestamp)) {
			GPAD_LOG_ERR("[IPEM] timer overflow\n");
		}
	} else if (timestamp < g_touch_finish_timestamp) { /* still in touch effect */

		/* touch effect */
		action_ulimit = action_llimit;
	}

	if (0) {
	}

	if (0) {
	}

	if (0) {
	}

	if (0) {
	}

	gpuload = ipem_info->statistic_info->gpuload;

	action_next = gpad_ipem_tsearch_do(table, action_llimit, action_ulimit, gpuload);

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dvfs_lookahead_update(struct gpad_ipem_work_info *work_info)
{
	/* implement LookAhead Update here */
}




/*
 * *******************************************************************
 *  dvfs qlearn related
 * *******************************************************************
 */
static void gpad_ipem_dvfs_qlearn_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dvfs_qlearn_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dvfs_qlearn_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	gpad_ipem_state state_next;
	gpad_ipem_action action_next;
	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;
	u32 table_size;
	gpad_ipem_fixedpt epsilon;
	gpad_ipem_fixedpt *epsilons;
	gpad_ipem_fixedpt tau;
	gpad_ipem_fixedpt *qs;
	gpad_ipem_stateaction start;
	gpad_ipem_stateaction end;

	state_next = ipem_info->tracker_info.state_next;

	table_size = ipem_info->table_info.table_size;

	/* TODO: not to use table_size as the number of actions */
	/* TODO: relax the termal constraint? */
	action_llimit = ipem_info->statistic_info->action_llimit;
	action_ulimit = table_size - 1;
	if (ipem_info->table_info.table[action_ulimit].volt == 0) {
		action_ulimit--;
	}
	WARN_ON(action_llimit > action_ulimit);

	epsilon = gpad_ipem_fixedpt_rconst(GPAD_IPEM_EPSILON);
	epsilons = ipem_info->scoreboard_info.epsilons;

	tau = gpad_ipem_fixedpt_rconst(GPAD_IPEM_TAU);

	qs = ipem_info->scoreboard_info.qs;

	gpad_ipem_stateaction_enc(&start, state_next, action_llimit);
	gpad_ipem_stateaction_enc(&end, state_next, action_ulimit);

	/* TODO: put an option for them */
	/*action_next = gpad_ipem_qmax_do(qs, start, end); */
	/*action_next = gpad_ipem_egreedy_do(epsilon, qs, start, end); */
	/*action_next = gpad_ipem_softmax_do(tau, qs, start, end); */
	action_next = gpad_ipem_vdbe_do(epsilons, tau, qs, start, end);

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, start = %u, end = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, start, end, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dvfs_qlearn_update(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Qlearn Update here */
	gpad_ipem_fixedpt *qs;
	gpad_ipem_state state_prev;
	gpad_ipem_action action_prev;
	gpad_ipem_stateaction stateaction_prev;
	gpad_ipem_fixedpt q_prev;
	gpad_ipem_state state_next;
	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;
	u32 table_size;
	gpad_ipem_stateaction start;
	gpad_ipem_stateaction end;
	gpad_ipem_action action_max;
	gpad_ipem_stateaction stateaction_max;
	gpad_ipem_fixedpt q_max;
	gpad_ipem_fixedpt q;
	gpad_ipem_fixedpt q_delta;
	gpad_ipem_fixedpt alpha;
	gpad_ipem_fixedpt gamma;
	gpad_ipem_fixedpt delta;
	gpad_ipem_fixedpt sigma;
	gpad_ipem_fixedpt *epsilons;
	gpad_ipem_fixedpt reward;

	qs = ipem_info->scoreboard_info.qs;

	state_prev = ipem_info->tracker_info.state_prev;
	action_prev = ipem_info->tracker_info.action_prev;
	gpad_ipem_stateaction_enc(&stateaction_prev, state_prev, action_prev);
	q_prev = qs[stateaction_prev];

	state_next = ipem_info->tracker_info.state_next;
	table_size = ipem_info->table_info.table_size;

	/* TODO: not to use table_size as the number of actions */
	/* TODO: relax the termal constraint? */
	action_llimit = 0;
	action_ulimit = table_size - 1;
	if (ipem_info->table_info.table[action_ulimit].volt == 0) {
		action_ulimit--;
	}
	WARN_ON(action_llimit > action_ulimit);
	gpad_ipem_stateaction_enc(&start, state_next, action_llimit);
	gpad_ipem_stateaction_enc(&end, state_next, action_ulimit);
	action_max = gpad_ipem_qmax_do(qs, start, end);
	gpad_ipem_stateaction_enc(&stateaction_max, state_next, action_max);
	q_max = qs[stateaction_max];

	reward = ipem_info->statistic_info->reward;

	alpha = gpad_ipem_fixedpt_rconst(GPAD_IPEM_ALPHA);
	gamma = gpad_ipem_fixedpt_rconst(GPAD_IPEM_GAMMA);

	q_delta = gpad_ipem_fixedpt_mul(alpha, (reward + gpad_ipem_fixedpt_mul(gamma, q_max) - q_prev));
	q = q_prev + q_delta;

	qs[stateaction_prev] = q;

	epsilons = ipem_info->scoreboard_info.epsilons;
	delta = gpad_ipem_fixedpt_rconst(GPAD_IPEM_DELTA);
	sigma = gpad_ipem_fixedpt_rconst(GPAD_IPEM_SIGMA);

	gpad_ipem_vdbe_update(epsilons, delta, sigma, q_delta, state_prev);

	GPAD_LOG_INFO("[IPEM] %s: state_prev = %u, action_prev = %u, stateaction_prev = %u, q_prev = %s\n", __func__, state_prev, action_prev, stateaction_prev, gpad_ipem_fixedpt_cstr(q_prev));
	GPAD_LOG_INFO("[IPEM] %s: state_next = %u,  action_max = %u,  stateaction_max = %u,  q_max = %s\n", __func__, state_next, action_max, stateaction_max, gpad_ipem_fixedpt_cstr(q_max));
	GPAD_LOG_INFO("[IPEM] %s: reward = %s\n", __func__, gpad_ipem_fixedpt_cstr(reward));
	GPAD_LOG_INFO("[IPEM] %s: q_delta = %s\n", __func__, gpad_ipem_fixedpt_cstr(q_delta));
	GPAD_LOG_INFO("[IPEM] %s: q = q_prev + q_delta = %s\n", __func__, gpad_ipem_fixedpt_cstr(q));
}




/*
 * *******************************************************************
 *  dvfs sarsa related
 * *******************************************************************
 */
static void gpad_ipem_dvfs_sarsa_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dvfs_sarsa_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dvfs_sarsa_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Sarsa Do here */
	gpad_ipem_state state_next;
	gpad_ipem_action action_next;
	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;
	u32 table_size;
	gpad_ipem_fixedpt epsilon;
	gpad_ipem_fixedpt *epsilons;
	gpad_ipem_fixedpt tau;
	gpad_ipem_fixedpt *qs;
	gpad_ipem_stateaction start;
	gpad_ipem_stateaction end;

	state_next = ipem_info->tracker_info.state_next;

	table_size = ipem_info->table_info.table_size;

	/* TODO: not to use table_size as the number of actions */
	/* TODO: relax the termal constraint? */
	action_llimit = ipem_info->statistic_info->action_llimit;
	action_ulimit = table_size - 1;
	if (ipem_info->table_info.table[action_ulimit].volt == 0) {
		action_ulimit--;
	}
	WARN_ON(action_llimit > action_ulimit);

	epsilon = gpad_ipem_fixedpt_rconst(GPAD_IPEM_EPSILON);
	epsilons = ipem_info->scoreboard_info.epsilons;

	tau = gpad_ipem_fixedpt_rconst(GPAD_IPEM_TAU);

	qs = ipem_info->scoreboard_info.qs;

	gpad_ipem_stateaction_enc(&start, state_next, action_llimit);
	gpad_ipem_stateaction_enc(&end, state_next, action_ulimit);

	/* TODO: put an option for them */
	/*action_next = gpad_ipem_qmax_do(qs, start, end); */
	/*action_next = gpad_ipem_egreedy_do(epsilon, qs, start, end); */
	/*action_next = gpad_ipem_softmax_do(tau, qs, start, end); */
	action_next = gpad_ipem_vdbe_do(epsilons, tau, qs, start, end);

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, start = %u, end = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, start, end, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dvfs_sarsa_update(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Sarsa Update here */
	gpad_ipem_fixedpt *qs;
	gpad_ipem_state state_prev;
	gpad_ipem_action action_prev;
	gpad_ipem_stateaction stateaction_prev;
	gpad_ipem_fixedpt q_prev;
	gpad_ipem_state state_next;
	gpad_ipem_action action_next;
	gpad_ipem_stateaction stateaction_next;
	gpad_ipem_fixedpt q_next;
	gpad_ipem_fixedpt q;
	gpad_ipem_fixedpt q_delta;
	gpad_ipem_fixedpt alpha;
	gpad_ipem_fixedpt gamma;
	gpad_ipem_fixedpt delta;
	gpad_ipem_fixedpt sigma;
	gpad_ipem_fixedpt *epsilons;
	gpad_ipem_fixedpt reward;

	qs = ipem_info->scoreboard_info.qs;

	state_prev = ipem_info->tracker_info.state_prev;
	action_prev = ipem_info->tracker_info.action_prev;
	gpad_ipem_stateaction_enc(&stateaction_prev, state_prev, action_prev);
	q_prev = qs[stateaction_prev];

	state_next = ipem_info->tracker_info.state_next;
	action_next = ipem_info->tracker_info.action_next;
	gpad_ipem_stateaction_enc(&stateaction_next, state_next, action_next);
	q_next = qs[stateaction_next];

	reward = ipem_info->statistic_info->reward;

	alpha = gpad_ipem_fixedpt_rconst(GPAD_IPEM_ALPHA);
	gamma = gpad_ipem_fixedpt_rconst(GPAD_IPEM_GAMMA);

	q_delta = gpad_ipem_fixedpt_mul(alpha, (reward + gpad_ipem_fixedpt_mul(gamma, q_next) - q_prev));
	q = q_prev + q_delta;

	qs[stateaction_prev] = q;

	epsilons = ipem_info->scoreboard_info.epsilons;
	delta = gpad_ipem_fixedpt_rconst(GPAD_IPEM_DELTA);
	sigma = gpad_ipem_fixedpt_rconst(GPAD_IPEM_SIGMA);

	gpad_ipem_vdbe_update(epsilons, delta, sigma, q_delta, state_prev);

	GPAD_LOG_INFO("[IPEM] %s: state_prev = %u, action_prev = %u, stateaction_prev = %u, q_prev = %s\n", __func__, state_prev, action_prev, stateaction_prev, gpad_ipem_fixedpt_cstr(q_prev));
	GPAD_LOG_INFO("[IPEM] %s: state_next = %u, action_next = %u, stateaction_next = %u, q_next = %s\n", __func__, state_next, action_next, stateaction_next, gpad_ipem_fixedpt_cstr(q_next));
	GPAD_LOG_INFO("[IPEM] %s: reward = %s\n", __func__, gpad_ipem_fixedpt_cstr(reward));
	GPAD_LOG_INFO("[IPEM] %s: q_delta = %s\n", __func__, gpad_ipem_fixedpt_cstr(q_delta));
	GPAD_LOG_INFO("[IPEM] %s: q = q_prev + q_delta = %s\n", __func__, gpad_ipem_fixedpt_cstr(q));
}




/*
 * *******************************************************************
 *  dvfs type1 related
 * *******************************************************************
 */
static void gpad_ipem_dvfs_type1_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dvfs_type1_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dvfs_type1_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Type1 Do here */
	struct mtk_gpufreq_info *table;
	u32 table_size;

	u32 gpuload;

	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;

	gpad_ipem_action action_prev;
	gpad_ipem_action action_next;

	table = ipem_info->table_info.table;
	table_size = ipem_info->table_info.table_size;

	action_llimit = ipem_info->statistic_info->action_llimit;
	action_ulimit = ipem_info->statistic_info->action_ulimit;
	if (ipem_info->table_info.table[action_ulimit].volt == 0) {
		action_ulimit--;
	}
	WARN_ON(action_llimit > action_ulimit);

	gpuload = ipem_info->statistic_info->gpuload;

	action_prev = ipem_info->tracker_info.action_prev;

	action_next = action_prev;
	if (80 < gpuload) {
		action_next -= 1;
	} else if (gpuload < 50) {
		action_next += 1;
	}

	/* TODO: a better way to do underflow protect */
	if ((int)action_ulimit < (int)action_next) {
		action_next = action_ulimit;
	} else if ((int)action_next < (int)action_llimit) {
		action_next = action_llimit;
	}

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dvfs_type1_update(struct gpad_ipem_work_info *work_info)
{
	/* implement Type1 Update here */
}




/*
 * *******************************************************************
 *  dvfs type2 related
 * *******************************************************************
 */
static void gpad_ipem_dvfs_type2_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dvfs_type2_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dvfs_type2_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Type2 Do here */
	struct mtk_gpufreq_info *table;
	u32 table_size;

	u32 gpuload;

	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;

	gpad_ipem_action action_prev;
	gpad_ipem_action action_next;

	table = ipem_info->table_info.table;
	table_size = ipem_info->table_info.table_size;

	action_llimit = ipem_info->statistic_info->action_llimit;
	action_ulimit = ipem_info->statistic_info->action_ulimit;
	if (ipem_info->table_info.table[action_ulimit].volt == 0) {
		action_ulimit--;
	}
	WARN_ON(action_llimit > action_ulimit);

	if (work_info->performance_mode > 0)
		action_ulimit = action_llimit;

	gpuload = ipem_info->statistic_info->gpuload;

	action_prev = ipem_info->tracker_info.action_prev;

	action_next = action_prev;
	if (98 < gpuload && gpuload <= 100) {
		action_next = action_llimit;
	} else if (84 < gpuload && gpuload <= 98) {
		action_next -= 2;
	} else if (69 < gpuload && gpuload <= 84) {
		action_next -= 1;
	} else if (30 < gpuload && gpuload <= 50) {
		action_next += 1;
	} else if (1 < gpuload && gpuload <= 30) {
		action_next += 2;
	} else if (0 < gpuload && gpuload <= 1) {
		action_next = action_ulimit;
	}

	/* TODO: a better way to do underflow protect */
	if ((int)action_ulimit < (int)action_next) {
		action_next = action_ulimit;
	} else if ((int)action_next < (int)action_llimit) {
		action_next = action_llimit;
	}

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dvfs_type2_update(struct gpad_ipem_work_info *work_info)
{
	/* implement Type2 Update here */
}




/*
 * *******************************************************************
 *  dvfs type4 related
 * *******************************************************************
 */
static void gpad_ipem_dvfs_type4_init(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);
	USE_IPEM_LOCK;

	LOCK_IPEM();
	ipem_info->dvfs_info.type4_info.gpuload_prev = 0;
	ipem_info->dvfs_info.type4_info.gpuload_next = 0;
	UNLOCK_IPEM();
}

static void gpad_ipem_dvfs_type4_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dvfs_type4_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Type4 Do here */
	struct mtk_gpufreq_info *table;
	u32 table_size;

	u32 gpuload_next;
	u32 gpuload_prev;

	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;

	gpad_ipem_action action_prev;
	gpad_ipem_action action_next;

	table = ipem_info->table_info.table;
	table_size = ipem_info->table_info.table_size;

	action_llimit = ipem_info->statistic_info->action_llimit;
	action_ulimit = ipem_info->statistic_info->action_ulimit;
	if (ipem_info->table_info.table[action_ulimit].volt == 0) {
		action_ulimit--;
	}
	WARN_ON(action_llimit > action_ulimit);

	if (work_info->performance_mode > 0)
		action_ulimit = action_llimit;

	gpuload_prev = ipem_info->dvfs_info.type4_info.gpuload_next;
	gpuload_next = ipem_info->statistic_info->gpuload;

	action_prev = ipem_info->tracker_info.action_prev;

	action_next = action_prev;
	if (98 < gpuload_next && gpuload_next <= 100) {
		action_next = action_llimit;
	} else if (84 < gpuload_next && gpuload_next <= 98) {
		action_next -= 2;
		if (((gpuload_prev * 170) / 100) < gpuload_next) {
			action_next -= 1;
		}
	} else if (69 < gpuload_next && gpuload_next <= 84) {
		action_next -= 1;
		if (((gpuload_prev * 170) / 100) < gpuload_next) {
			action_next -= 1;
		}
	} else if (30 < gpuload_next && gpuload_next <= 50) {
		action_next += 1;
		if (((gpuload_next * 170) / 100) < gpuload_prev) {
			action_next += 1;
		}
	} else if (1 < gpuload_next && gpuload_next <= 30) {
		action_next += 2;
		if (((gpuload_next * 170) / 100) < gpuload_prev) {
			action_next += 1;
		}
	} else if (0 < gpuload_next && gpuload_next <= 1) {
		action_next = action_ulimit;
	}

	/* TODO: a better way to do underflow protect */
	if ((int)action_ulimit < (int)action_next) {
		action_next = action_ulimit;
	} else if ((int)action_next < (int)action_llimit) {
		action_next = action_llimit;
	}

	ipem_info->dvfs_info.type4_info.gpuload_prev = gpuload_prev;
	ipem_info->dvfs_info.type4_info.gpuload_next = gpuload_next;

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dvfs_type4_update(struct gpad_ipem_work_info *work_info)
{
	/* implement Type4 Update here */
}




/*
 * *******************************************************************
 *  dvfs related
 * *******************************************************************
 */
void gpad_ipem_dvfs_init(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	u32 policy_cnt;
	u32 policy_id;

	u32 DEFAULT_POLICY_ID = 7;
	USE_IPEM_LOCK;

	policy_cnt = sizeof(g_ipem_dvfs_policies) / sizeof(g_ipem_dvfs_policies[0]);

	for (policy_id = 0; policy_id < policy_cnt; policy_id++) {
		g_ipem_dvfs_policies[policy_id].init_func(dev);
	}

	LOCK_IPEM();
	ipem_info->dvfs_info.policies = g_ipem_dvfs_policies;
	ipem_info->dvfs_info.policy_id = DEFAULT_POLICY_ID;
	ipem_info->dvfs_info.policy_do = ipem_info->dvfs_info.policies[DEFAULT_POLICY_ID].do_func;
	ipem_info->dvfs_info.policy_update = ipem_info->dvfs_info.policies[DEFAULT_POLICY_ID].update_func;
	UNLOCK_IPEM();
}

void gpad_ipem_dvfs_exit(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	u32 policy_cnt;
	u32 policy_id;

	policy_cnt = sizeof(g_ipem_dvfs_policies) / sizeof(g_ipem_dvfs_policies[0]);

	for (policy_id = 0; policy_id < policy_cnt; policy_id++) {
		ipem_info->dvfs_info.policies[policy_id].exit_func(dev);
	}
}

gpad_ipem_action gpad_ipem_dvfs_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	return ipem_info->dvfs_info.policy_do(work_info);
}

void gpad_ipem_dvfs_update(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	ipem_info->dvfs_info.policy_update(work_info);
}

unsigned int gpad_ipem_dvfs_policy_get(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	return ipem_info->dvfs_info.policy_id;
}

void gpad_ipem_dvfs_policy_set(struct gpad_device *dev, unsigned int policy_id)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);
	USE_IPEM_LOCK;

	if (policy_id >= (unsigned int)ARRAY_SIZE(g_ipem_dvfs_policies)) {
		GPAD_LOG_ERR("Setting an invalid DVFS policy id: %u!\n", policy_id);
		return;
	}

	LOCK_IPEM();
	if (policy_id != ipem_info->dvfs_info.policy_id) {
		ipem_info->dvfs_info.policy_id = policy_id;
		ipem_info->dvfs_info.policy_do = ipem_info->dvfs_info.policies[policy_id].do_func;
		ipem_info->dvfs_info.policy_update = ipem_info->dvfs_info.policies[policy_id].update_func;

		GPAD_LOG_INFO("[IPEM] %s: policy_id = %u, name = %s, init_func = %p, exit_func = %p, do_func = %p, update_func = %p\n", __func__, policy_id, ipem_info->dvfs_info.policies[policy_id].name, (void *)ipem_info->dvfs_info.policies[policy_id].init_func, (void *)ipem_info->dvfs_info.policies[policy_id].exit_func, (void *)ipem_info->dvfs_info.policies[policy_id].do_func, (void *)ipem_info->dvfs_info.policies[policy_id].update_func);
	}
	UNLOCK_IPEM();
}

struct gpad_ipem_dvfs_policy_info *gpad_ipem_dvfs_policies_get(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	return ipem_info->dvfs_info.policies;
}

