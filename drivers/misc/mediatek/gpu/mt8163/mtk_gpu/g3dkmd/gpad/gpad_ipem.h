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

#ifndef _GPAD_IPEM_H
#define _GPAD_IPEM_H
#include <linux/types.h>
#include "gs_gpufreq.h"
#include "gpad_common.h"
#include "gpad_ipem_lib.h"
#include "gpad_ipem_dvfs.h"
#include "gpad_ipem_dpm.h"



/*
 * *******************************************************************
 *  macro
 * *******************************************************************
 */
#define GPAD_IPEM_VERSION					"0.8"

/* Default timer interval = 100ms */
#define GPAD_IPEM_TIMER_INTERVAL				((HZ * 100) / 1000)

/* Default qs size = 64kbytes */
#define GPAD_IPEM_SCOREBOARD_QS_SIZE				(64 * 1024)
/* Default epsilons size = 16kbytes */
#define GPAD_IPEM_SCOREBOARD_EPSILONS_SIZE			(16 * 1024)

/* TODO: make it configurable */
#define GPAD_IPEM_ALPHA						0.9
#define GPAD_IPEM_GAMMA						0.9
#define GPAD_IPEM_EPSILON					0.9
#define GPAD_IPEM_TAU						80
#define GPAD_IPEM_DELTA						0.25
#define GPAD_IPEM_SIGMA						10
#define GPAD_IPEM_BETA						0.6

/* Breakevent time (us) for voltage and frequency scaling, power-off and power-on */
/* TODO: is it constant? tableup? */
#define GPAD_IPEM_BREAKEVENT_VOLT				100
#define GPAD_IPEM_BREAKEVENT_FREQ				100
#define GPAD_IPEM_BREAKEVENT_POFF				100
#define GPAD_IPEM_BREAKEVENT_PON				100

/* Timeout time (us) */
#define GPAD_IPEM_TIMEOUT					100

/* Default records size = 1mbytes */
#define GPAD_IPEM_RECORDER_RECORDS_SIZE	(1024 * 1024)

/* Lock helpers */
#define USE_IPEM_LOCK \
	unsigned long ipem_lock_flags
#define LOCK_IPEM() \
	spin_lock_irqsave(&ipem_info->lock, ipem_lock_flags)
#define UNLOCK_IPEM() \
	spin_unlock_irqrestore(&ipem_info->lock, ipem_lock_flags)


/*
 * *******************************************************************
 *  enumeration
 * *******************************************************************
 */
enum gpad_ipem_callback {
	GPAD_IPEM_CALLBACK_TIMER           = 0, GPAD_IPEM_CALLBACK_THERMAL         = 1, GPAD_IPEM_CALLBACK_TOUCH           = 2, GPAD_IPEM_CALLBACK_POWERLOW        = 3, GPAD_IPEM_CALLBACK_POWERHIGH       = 4, GPAD_IPEM_CALLBACK_POWEROFF        = 5, GPAD_IPEM_CALLBACK_POWERON         = 6, GPAD_IPEM_CALLBACK_POWEROFF_BEGIN  = 7, GPAD_IPEM_CALLBACK_POWEROFF_END    = 8, GPAD_IPEM_CALLBACK_POWERON_BEGIN   = 9, GPAD_IPEM_CALLBACK_POWERON_END     = 10, GPAD_IPEM_CALLBACK_PERFORMANCE_ON  = 11, GPAD_IPEM_CALLBACK_PERFORMANCE_OFF = 12, GPAD_IPEM_CALLBACK_LENGTH          = 13, GPAD_IPEM_CALLBACK_INVALID         = 13
};

enum gpad_ipem_event {
	GPAD_IPEM_EVENT_TIMER           = 0, GPAD_IPEM_EVENT_THERMAL         = 1, GPAD_IPEM_EVENT_TOUCH           = 2, GPAD_IPEM_EVENT_POWERLOW        = 3, GPAD_IPEM_EVENT_POWERHIGH       = 4, GPAD_IPEM_EVENT_POWEROFF        = 5, GPAD_IPEM_EVENT_POWERON         = 6, GPAD_IPEM_EVENT_POWEROFF_BEGIN  = 7, GPAD_IPEM_EVENT_POWEROFF_END    = 8, GPAD_IPEM_EVENT_POWERON_BEGIN   = 9, GPAD_IPEM_EVENT_POWERON_END     = 10, GPAD_IPEM_EVENT_PERFORMANCE_ON  = 11, GPAD_IPEM_EVENT_PERFORMANCE_OFF = 12, GPAD_IPEM_EVENT_LENGTH          = 13, GPAD_IPEM_EVENT_INVALID         = 13
};




/*
 * *******************************************************************
 *  data structure
 * *******************************************************************
 */
struct gpad_ipem_queue_info {
	struct workqueue_struct            *workqueue;

	wait_queue_head_t					waitqueue;

	unsigned int						workid;
	unsigned int						workcnt;
};

struct gpad_ipem_timer_info {
	struct timer_list					timer;
	unsigned long						timer_interval;
};

struct gpad_ipem_table_info {
	unsigned int						table_id;
	struct mtk_gpufreq_info					*table;
	unsigned int						table_size;
};

struct gpad_ipem_tracker_info {
	/* Top: update every time before "queue_work" */
	enum gpad_ipem_event    event_prev;
	enum gpad_ipem_event    event_next;

	unsigned long						timestamp_prev;
	unsigned long						timestamp_next;
	unsigned long						timestamp_off2on;

	u32             timebusy_prev;
	u32             timebusy_next;

	u32             timetotal_prev;
	u32             timetotal_next;

	u32             timer_timebusy;
	u32             timer_timebusy_prev;

	u32             timer_timetotal;
	u32             timer_timetotal_prev;

	/* Middle: update every time a work is executing */
	gpad_ipem_policy_do_func                                policy_do_prev;
	gpad_ipem_policy_do_func                                policy_do_next;
	gpad_ipem_policy_update_func                            policy_update_prev;
	gpad_ipem_policy_update_func                            policy_update_next;

	gpad_ipem_state                                         state_prev;
	gpad_ipem_state                                         state_next;

	/* Bottom: update every time a work is finished */
	gpad_ipem_action                                        action_prev;
	gpad_ipem_action                                        action_next;
};

struct gpad_ipem_statistic_info {
	/* Top: update every time before "queue_work" */
	enum gpad_ipem_event                event;
	unsigned long                       timestamp;

	gpad_ipem_action					action_llimit;
	gpad_ipem_action					action_ulimit;
	gpad_ipem_action					action_prev;

	unsigned int						gpuload;

	unsigned int						thermal;

	unsigned int						touch;

	gpad_ipem_fixedpt					flush;

	gpad_ipem_fixedpt					ht;

	unsigned int						idle;

	gpad_ipem_fixedpt					reward;

	/* Bottom: update every time a work is finished */
	gpad_ipem_policy_do_func                                policy_do;
	gpad_ipem_policy_update_func                            policy_update;

	gpad_ipem_state                                         state;
	gpad_ipem_action                                        action;
};

struct gpad_ipem_scoreboard_info {
	gpad_ipem_fixedpt					*qs;
	gpad_ipem_fixedpt					*epsilons;
};

struct gpad_ipem_work_info {
	struct gpad_device					*dev;
	struct gpad_ipem_info               *ipem_info;
	atomic_t                            in_use;
	enum gpad_ipem_event				event;
	u32                                 workid;
	s32                                 performance_mode;
	struct work_struct					work;
	/*<workaround> struct gpad_ipem_statistic_info                         statistic_info; */
};

struct gpad_ipem_record_info {
	unsigned int                                            workid;
	struct gpad_ipem_statistic_info                         statistic_info;
};

struct gpad_ipem_recorder_info {
	struct gpad_ipem_record_info       *records;
	unsigned int						recordid;
};

struct gpad_ipem_pm_data {
	unsigned long long                  acc_busy_ns;
	unsigned long long                  acc_total_ns;
};

struct gpad_ipem_info {
	spinlock_t						lock;

	unsigned int						enable;

	struct gpad_ipem_table_info				table_info;

	struct gpad_ipem_tracker_info				tracker_info;

	struct gpad_ipem_statistic_info				*statistic_info;
	struct gpad_ipem_statistic_info				_statistic_info;

	struct gpad_ipem_scoreboard_info			scoreboard_info;

	struct gpad_ipem_dvfs_info				dvfs_info;

	struct gpad_ipem_dpm_info				dpm_info;

	struct gpad_ipem_recorder_info				recorder_info;
};




/*
 * *******************************************************************
 *  function prototype
 * *******************************************************************
 */
int gpad_ipem_init(struct gpad_device *dev);
int gpad_ipem_exit(struct gpad_device *dev);

int gpad_ipem_timer_enable(struct gpad_device *dev);
int gpad_ipem_timer_disable(struct gpad_device *dev);
unsigned long gpad_ipem_timer_get(struct gpad_device *dev);
void gpad_ipem_timer_set(struct gpad_device *dev, unsigned long interval);

unsigned int gpad_ipem_table_get(struct gpad_device *dev);
void gpad_ipem_table_set(struct gpad_device *dev, unsigned int table_id);

int gpad_ipem_enable(struct gpad_device *dev);
int gpad_ipem_disable(struct gpad_device *dev);
unsigned int gpad_ipem_enable_get(struct gpad_device *dev);

/* reference counted performance mode switch */
void gpad_ipem_performance_mode(unsigned int enabled);

void gpad_ipem_callback_timer(unsigned long data);
void gpad_ipem_callback_thermal(void);
void gpad_ipem_callback_touch(void);
void gpad_ipem_callback_powerlow(void);
void gpad_ipem_callback_powerhigh(void);
void gpad_ipem_callback_poweroff(void);
void gpad_ipem_callback_poweron(void);
void gpad_ipem_callback_poweroff_begin(void);
void gpad_ipem_callback_poweroff_end(void);
void gpad_ipem_callback_poweron_begin(void);
void gpad_ipem_callback_poweron_end(void);
void gpad_ipem_callback_pm(struct gpad_device *dev, struct gpad_ipem_pm_data *data);
extern u32 get_devinfo_with_index(u32 index);
bool gpad_ipem_is_queuing_power_on(void);

#endif /* #define _GPAD_IPEM_H */
