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
#include "gpad_ipem_dpm.h"


#define GET_QUEUE_INFO() (&g_gpad_ipem_queue_info)

/*
 * *******************************************************************
 *  function prototype
 * *******************************************************************
 */
static void gpad_ipem_dpm_disable_init(struct gpad_device *dev);
static void gpad_ipem_dpm_disable_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dpm_disable_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dpm_disable_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dpm_immediate_init(struct gpad_device *dev);
static void gpad_ipem_dpm_immediate_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dpm_immediate_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dpm_immediate_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dpm_timeout_init(struct gpad_device *dev);
static void gpad_ipem_dpm_timeout_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dpm_timeout_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dpm_timeout_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dpm_predict_init(struct gpad_device *dev);
static void gpad_ipem_dpm_predict_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dpm_predict_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dpm_predict_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dpm_qlearn_init(struct gpad_device *dev);
static void gpad_ipem_dpm_qlearn_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dpm_qlearn_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dpm_qlearn_update(struct gpad_ipem_work_info *work_info);

static void gpad_ipem_dpm_sarsa_init(struct gpad_device *dev);
static void gpad_ipem_dpm_sarsa_exit(struct gpad_device *dev);
static gpad_ipem_action gpad_ipem_dpm_sarsa_do(struct gpad_ipem_work_info *work_info);
static void gpad_ipem_dpm_sarsa_update(struct gpad_ipem_work_info *work_info);




/*
 * *******************************************************************
 *  globle variable
 * *******************************************************************
 */
struct gpad_ipem_dpm_policy_info g_ipem_dpm_policies[] = {
	{.name =   "disable", .init_func =   gpad_ipem_dpm_disable_init, .exit_func =   gpad_ipem_dpm_disable_exit, .do_func =   gpad_ipem_dpm_disable_do, .update_func =     gpad_ipem_dpm_disable_update}, {.name = "immediate", .init_func = gpad_ipem_dpm_immediate_init, .exit_func = gpad_ipem_dpm_immediate_exit, .do_func = gpad_ipem_dpm_immediate_do, .update_func =   gpad_ipem_dpm_immediate_update}, {.name =   "timeout", .init_func =   gpad_ipem_dpm_timeout_init, .exit_func =   gpad_ipem_dpm_timeout_exit, .do_func =   gpad_ipem_dpm_timeout_do, .update_func =     gpad_ipem_dpm_timeout_update}, {.name =   "predict", .init_func =   gpad_ipem_dpm_predict_init, .exit_func =   gpad_ipem_dpm_predict_exit, .do_func =   gpad_ipem_dpm_predict_do, .update_func =     gpad_ipem_dpm_predict_update}, {.name =    "qlearn", .init_func =    gpad_ipem_dpm_qlearn_init, .exit_func =     gpad_ipem_dpm_qlearn_exit, .do_func =	gpad_ipem_dpm_qlearn_do, .update_func =      gpad_ipem_dpm_qlearn_update}, {.name =     "sarsa", .init_func =     gpad_ipem_dpm_sarsa_init, .exit_func =      gpad_ipem_dpm_sarsa_exit, .do_func =	gpad_ipem_dpm_sarsa_do, .update_func =       gpad_ipem_dpm_sarsa_update}, };




/*
 * *******************************************************************
 *  dpm disable related
 * *******************************************************************
 */
static void gpad_ipem_dpm_disable_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dpm_disable_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dpm_disable_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Disable Do here */
	return ipem_info->tracker_info.action_prev;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dpm_disable_update(struct gpad_ipem_work_info *work_info)
{
	/* implement Disable Update here */
}




/*
 * *******************************************************************
 *  dpm immediate related
 * *******************************************************************
 */
static void gpad_ipem_dpm_immediate_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dpm_immediate_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dpm_immediate_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Immediate Do here */
	struct mtk_gpufreq_info *table;
	u32 table_size;

	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;

	gpad_ipem_action action_next;

	table = ipem_info->table_info.table;
	table_size = ipem_info->table_info.table_size;

	/* TODO: not to use table_size as the number of actions */
	action_llimit = table_size - 1;
	action_ulimit = table_size - 1; /* TODO ??? */
	WARN_ON(action_llimit > action_ulimit);

	action_next = table_size - 1;

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dpm_immediate_update(struct gpad_ipem_work_info *work_info)
{
	/* implement Immediate Update here */
}




/*
 * *******************************************************************
 *  dpm timeout related
 * *******************************************************************
 */
static void gpad_ipem_dpm_timeout_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dpm_timeout_exit(struct gpad_device *dev)
{
}

/* FIXME: ASSUMPTION: ipem's lock had been held? */
static gpad_ipem_action gpad_ipem_dpm_timeout_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;
	struct gpad_ipem_queue_info *queue_info = GET_QUEUE_INFO();

	/* implement Timeout Do here */
	struct mtk_gpufreq_info *table;
	u32 table_size;
	ktime_t timeout;
	s32 condition = 0;

	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;

	gpad_ipem_action action_next;

	table = ipem_info->table_info.table;
	table_size = ipem_info->table_info.table_size;

	/* TODO: not to use table_size as the number of actions */
	action_llimit = table_size - 1;
	action_ulimit = table_size - 1;
	WARN_ON(action_llimit > action_ulimit);

	/* wait timeout or event POWERON */
	timeout = ktime_set(0, GPAD_IPEM_TIMEOUT * 1000);
	/* ipem_info->event = GPAD_IPEM_EVENT_INVALID; */
	/* UNLOCK_IPEM(); // we can't call this directly since flag is not yet initialized when calling UNLOCK first. */
	condition = wait_event_hrtimeout(queue_info->waitqueue,  gpad_ipem_is_queuing_power_on(),  /* work_info->event == GPAD_IPEM_EVENT_POWERON, */
									 timeout);
	/* LOCK_IPEM(); */

	/* TODO: do something when not to go to power-off ? */
	if (condition == 0)
		action_next = ipem_info->tracker_info.action_prev;
	else
		action_next = table_size - 1;

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, condition = %d, action_next = %u\n", __func__, action_llimit, action_ulimit, condition, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dpm_timeout_update(struct gpad_ipem_work_info *work_info)
{
	/* implement Timeout Update here */
}




/*
 * *******************************************************************
 *  dpm predict related
 * *******************************************************************
 */
static void gpad_ipem_dpm_predict_init(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);
	USE_IPEM_LOCK;

	LOCK_IPEM();
	ipem_info->dpm_info.predict_info.idle = 0;
	UNLOCK_IPEM();
}

static void gpad_ipem_dpm_predict_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dpm_predict_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Predict Do here */
	struct mtk_gpufreq_info *table;
	u32 table_size;
	u32 idle;

	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;

	gpad_ipem_action action_next;

	table = ipem_info->table_info.table;
	table_size = ipem_info->table_info.table_size;

	/* TODO: not to use table_size as the number of actions */
	action_llimit = table_size - 1;
	action_ulimit = table_size - 1;
	WARN_ON(action_llimit > action_ulimit);

	/* predict the idle */
	idle = ipem_info->dpm_info.predict_info.idle;
	if (ipem_info->statistic_info->idle > 0) {
		/* FIXME: handle the overflow case, suggest to have 64bits fixed point operation*/
		idle = gpad_ipem_fixedpt_mul(GPAD_IPEM_BETA, gpad_ipem_fixedpt_fromint(idle)) + gpad_ipem_fixedpt_mul((GPAD_IPEM_FIXEDPT_ONE - GPAD_IPEM_BETA), gpad_ipem_fixedpt_fromint(ipem_info->statistic_info->idle));
		idle = gpad_ipem_fixedpt_toint(idle);
		ipem_info->dpm_info.predict_info.idle = idle;
	}

	/* TODO: do something when not to go to power-off ? */
	if (idle > (GPAD_IPEM_BREAKEVENT_POFF + GPAD_IPEM_BREAKEVENT_PON)) {
		action_next = table_size - 1;
	} else if (0) {
	} else if (0) {
	} else if (0) {
	} else {
		action_next = ipem_info->tracker_info.action_prev;
	}

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, idle = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, idle, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dpm_predict_update(struct gpad_ipem_work_info *work_info)
{
	/* implement Predict Update here */
}




/*
 * *******************************************************************
 *  dpm qlearn related
 * *******************************************************************
 */
static void gpad_ipem_dpm_qlearn_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dpm_qlearn_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dpm_qlearn_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	/* implement Qlearn Do here */
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
	WARN_ON(action_llimit > action_ulimit);

	epsilon = gpad_ipem_fixedpt_rconst(GPAD_IPEM_EPSILON);
	epsilons = ipem_info->scoreboard_info.epsilons;

	tau = gpad_ipem_fixedpt_rconst(GPAD_IPEM_TAU);

	qs = ipem_info->scoreboard_info.qs;

	gpad_ipem_stateaction_enc(&start, state_next, action_llimit);
	gpad_ipem_stateaction_enc(&end, state_next, action_ulimit);

	/* TODO: put an option for them */
	/* action_next = gpad_ipem_qmax_do(qs, start, end); */
	/* action_next = gpad_ipem_egreedy_do(epsilon, qs, start, end); */
	/* action_next = gpad_ipem_softmax_do(tau, qs, start, end); */
	action_next = gpad_ipem_vdbe_do(epsilons, tau, qs, start, end);

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, start = %u, end = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, start, end, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dpm_qlearn_update(struct gpad_ipem_work_info *work_info)
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
 *  dpm sarsa related
 * *******************************************************************
 */
static void gpad_ipem_dpm_sarsa_init(struct gpad_device *dev)
{
}

static void gpad_ipem_dpm_sarsa_exit(struct gpad_device *dev)
{
}

/* ASSUMPTION: ipem's lock had been held */
static gpad_ipem_action gpad_ipem_dpm_sarsa_do(struct gpad_ipem_work_info *work_info)
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
	WARN_ON(action_llimit > action_ulimit);

	epsilon = gpad_ipem_fixedpt_rconst(GPAD_IPEM_EPSILON);
	epsilons = ipem_info->scoreboard_info.epsilons;

	tau = gpad_ipem_fixedpt_rconst(GPAD_IPEM_TAU);

	qs = ipem_info->scoreboard_info.qs;

	gpad_ipem_stateaction_enc(&start, state_next, action_llimit);
	gpad_ipem_stateaction_enc(&end, state_next, action_ulimit);

	/* TODO: put an option for them */
	/* action_next = gpad_ipem_qmax_do(qs, start, end); */
	/* action_next = gpad_ipem_egreedy_do(epsilon, qs, start, end); */
	/* action_next = gpad_ipem_softmax_do(tau, qs, start, end); */
	action_next = gpad_ipem_vdbe_do(epsilons, tau, qs, start, end);

	GPAD_LOG_INFO("[IPEM] %s: action_llimit = %u, action_ulimit = %u, start = %u, end = %u, action_next = %u\n", __func__, action_llimit, action_ulimit, start, end, action_next);

	return action_next;
}

/* ASSUMPTION: ipem's lock had been held */
static void gpad_ipem_dpm_sarsa_update(struct gpad_ipem_work_info *work_info)
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
 *  dpm related
 * *******************************************************************
 */
void gpad_ipem_dpm_init(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	u32 policy_cnt;
	u32 policy_id;
	USE_IPEM_LOCK;

	policy_cnt = sizeof(g_ipem_dpm_policies) / sizeof(g_ipem_dpm_policies[0]);

	for (policy_id = 0; policy_id < policy_cnt; policy_id++)
		g_ipem_dpm_policies[policy_id].init_func(dev);

	LOCK_IPEM();
	ipem_info->dpm_info.policies = g_ipem_dpm_policies;
	ipem_info->dpm_info.policy_id = 0;
	ipem_info->dpm_info.policy_do = ipem_info->dpm_info.policies[0].do_func;
	ipem_info->dpm_info.policy_update = ipem_info->dpm_info.policies[0].update_func;
	UNLOCK_IPEM();
}

void gpad_ipem_dpm_exit(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	u32 policy_cnt;
	u32 policy_id;

	policy_cnt = sizeof(g_ipem_dpm_policies) / sizeof(g_ipem_dpm_policies[0]);

	for (policy_id = 0; policy_id < policy_cnt; policy_id++)
		ipem_info->dpm_info.policies[policy_id].exit_func(dev);
}

gpad_ipem_action gpad_ipem_dpm_do(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	return ipem_info->dpm_info.policy_do(work_info);
}

void gpad_ipem_dpm_update(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	ipem_info->dpm_info.policy_update(work_info);
}

unsigned int gpad_ipem_dpm_policy_get(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	return ipem_info->dpm_info.policy_id;
}

void gpad_ipem_dpm_policy_set(struct gpad_device *dev, unsigned int policy_id)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);
	USE_IPEM_LOCK;

	if (policy_id >= (unsigned int)ARRAY_SIZE(g_ipem_dpm_policies)) {
		GPAD_LOG_ERR("Setting an invalid DPM policy id: %u!\n", policy_id);
		return;
	}

	LOCK_IPEM();
	if (policy_id != ipem_info->dpm_info.policy_id) {
		ipem_info->dpm_info.policy_id = policy_id;
		ipem_info->dpm_info.policy_do = ipem_info->dpm_info.policies[policy_id].do_func;
		ipem_info->dpm_info.policy_update = ipem_info->dpm_info.policies[policy_id].update_func;

		GPAD_LOG_INFO(
		"[IPEM] %s: policy_id = %u, name = %s, init_func = %p, exit_func = %p, do_func = %p, update_func = %p\n", __func__, policy_id, ipem_info->dpm_info.policies[policy_id].name, (void *)ipem_info->dpm_info.policies[policy_id].init_func, (void *)ipem_info->dpm_info.policies[policy_id].exit_func, (void *)ipem_info->dpm_info.policies[policy_id].do_func, (void *)ipem_info->dpm_info.policies[policy_id].update_func);
	}
	UNLOCK_IPEM();
}

struct gpad_ipem_dpm_policy_info *gpad_ipem_dpm_policies_get(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	return ipem_info->dpm_info.policies;
}

