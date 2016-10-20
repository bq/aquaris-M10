#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <asm/string.h>
#include "gpad_common.h"
#include "gpad_hal.h"
#include "gpad_ipem.h"


#define GPAD_IPEM_DEBUG_PROC 0
#define GPAD_IPEM_NEW_GPULOAD 1

#define GPAD_IPEM_UPPER_BOUND_MAX        100
#define GPAD_IPEM_DEF_TABLE_ID           0
#define GPAD_IPEM_TABLE_ID_MT8163A_SD    2
#define GPAD_IPEM_TABLE_ID_MT8163A_SB    0
#define GPAD_IPEM_TABLE_ID_MT8163A_SA    1

/*
 * *******************************************************************
 *  globle variable
 * *******************************************************************
 */
/* FIXME: fill up the correct used power in each entry */
/* TODO: take table apart to save memory */
/* "lower_bound < gpuload <= upper_bound", except "0 < gpuload <= 0" */
/* Increase upper bound to fit special value */
/* 101 --> total-equal-previous condition. */
struct mtk_gpufreq_info g_ipem_tables[][4] = {
	{ /* 0 */
		{.freq_khz = GPU_DVFS_F480, .volt = GPU_POWER_VGPU_1_25V, .power_mW = 1075, .lower_bound = 60, .upper_bound =  GPAD_IPEM_UPPER_BOUND_MAX}, {.freq_khz = GPU_DVFS_F380, .volt = GPU_POWER_VGPU_1_15V, .power_mW =  750, .lower_bound = 30, .upper_bound =  60}, {.freq_khz = GPU_DVFS_F250, .volt = GPU_POWER_VGPU_1_15V, .power_mW =  495, .lower_bound =  0, .upper_bound =  30}, },
	{ /* 1 */
		{.freq_khz = GPU_DVFS_F550, .volt = GPU_POWER_VGPU_1_25V, .power_mW = 1155, .lower_bound = 80, .upper_bound =  GPAD_IPEM_UPPER_BOUND_MAX}, {.freq_khz = GPU_DVFS_F480, .volt = GPU_POWER_VGPU_1_25V, .power_mW = 1075, .lower_bound = 60, .upper_bound =  80}, {.freq_khz = GPU_DVFS_F380, .volt = GPU_POWER_VGPU_1_15V, .power_mW =  750, .lower_bound = 30, .upper_bound =  60}, {.freq_khz = GPU_DVFS_F250, .volt = GPU_POWER_VGPU_1_15V, .power_mW =  495, .lower_bound =  0, .upper_bound =  30}, },
	{ /* 2 */
		{.freq_khz = GPU_DVFS_F380, .volt = GPU_POWER_VGPU_1_15V, .power_mW =  750, .lower_bound = 30, .upper_bound =  GPAD_IPEM_UPPER_BOUND_MAX}, {.freq_khz = GPU_DVFS_F250, .volt = GPU_POWER_VGPU_1_15V, .power_mW =  495, .lower_bound =  0, .upper_bound =  30}, },
	{ /* 3 */
		{.freq_khz = GPU_DVFS_F250, .volt = GPU_POWER_VGPU_1_15V, .power_mW =  495, .lower_bound =  0, .upper_bound =  GPAD_IPEM_UPPER_BOUND_MAX}, },
	{ /* 4 */
		{.freq_khz = GPU_DVFS_F380, .volt = GPU_POWER_VGPU_1_15V, .power_mW =  750, .lower_bound =  0, .upper_bound = GPAD_IPEM_UPPER_BOUND_MAX}, },
	{ /* 5 */
		{.freq_khz = GPU_DVFS_F480, .volt = GPU_POWER_VGPU_1_25V, .power_mW = 1075, .lower_bound =  0, .upper_bound = GPAD_IPEM_UPPER_BOUND_MAX}, },
	{ /* 6 */
		{.freq_khz = GPU_DVFS_F550, .volt = GPU_POWER_VGPU_1_25V, .power_mW = 1155, .lower_bound =  0, .upper_bound = GPAD_IPEM_UPPER_BOUND_MAX}, },
	{ /* 7 */
		{.freq_khz = GPU_DVFS_F380, .volt = GPU_POWER_VGPU_1_20V, .power_mW =  800, .lower_bound =  0, .upper_bound = GPAD_IPEM_UPPER_BOUND_MAX}, }, };

unsigned int g_ipem_table_sizes[] = {3, 4, 2, 1, 1, 1, 1, 1};
#if GPAD_IPEM_DEBUG_PROC
static struct proc_dir_entry *g_proc_dir;
#endif
static bool s_is_command_queue_busy;

/* Counters  */
static u32 g_debug;
static u32 g_called[GPAD_IPEM_EVENT_LENGTH];
static u32 g_recursive_called[GPAD_IPEM_EVENT_LENGTH];
static u32 g_recursive_called_max[GPAD_IPEM_EVENT_LENGTH];
static u32 g_in_touch;
static atomic_t g_in_performance_mode = ATOMIC_INIT(0);

/* MET */
static u32 g_gpuload;
static u32 g_idle    = 100;


struct gpad_ipem_queue_info g_gpad_ipem_queue_info;
#define GET_QUEUE_INFO() (&g_gpad_ipem_queue_info)
struct gpad_ipem_timer_info g_gpad_ipem_timer_info;
#define GET_TIMER_INFO() (&g_gpad_ipem_timer_info)

/* around 70 bytes in 64bit environment */
static struct gpad_ipem_work_info s_work_info_allocation[GPAD_IPEM_EVENT_LENGTH];

/*static struct gpad_ipem_info g__ipem_info; */
/*
 * *******************************************************************
 *  Locks
 * *******************************************************************
 */
#define USE_TIMER_LOCK \
	unsigned long timer_lock_flags
#define LOCK_TIMER() \
	spin_lock_irqsave(&g_gpad_ipem_timer_lock, timer_lock_flags)
#define UNLOCK_TIMER() \
	spin_unlock_irqrestore(&g_gpad_ipem_timer_lock, timer_lock_flags)
DEFINE_SPINLOCK(g_gpad_ipem_timer_lock);

#define USE_WORK_LOCK \
	unsigned long work_lock_flags
#define LOCK_WORK_INFO() \
	spin_lock_irqsave(&g_gpad_ipem_work_info_lock, work_lock_flags)
#define UNLOCK_WORK_INFO() \
	spin_unlock_irqrestore(&g_gpad_ipem_work_info_lock, work_lock_flags)
DEFINE_SPINLOCK(g_gpad_ipem_work_info_lock);

#define USE_WQ_LOCK \
	unsigned long wq_lock_flags
#define LOCK_WQ() \
	spin_lock_irqsave(&g_gpad_ipem_wq_lock, wq_lock_flags)
#define UNLOCK_WQ() \
	spin_unlock_irqrestore(&g_gpad_ipem_wq_lock, wq_lock_flags)
DEFINE_SPINLOCK(g_gpad_ipem_wq_lock);

/*
 * *******************************************************************
 *  function prototype
 * *******************************************************************
 */
static void gpad_ipem_do(struct gpad_device *dev, enum gpad_ipem_event event);
static s32 gpad_ipem_initialize_proc_(void);


/*
 * *******************************************************************
 *  Performance mode related
 * *******************************************************************
 */
void gpad_ipem_performance_mode(unsigned int enabled)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_PERFORMANCE_OFF;
	s32 count = 0;

	switch (enabled) {
	case 1:
		atomic_inc(&g_in_performance_mode);
		event = GPAD_IPEM_EVENT_PERFORMANCE_ON;
		gpad_ipem_do(dev, event);
		break;
	case 0:
		atomic_dec(&g_in_performance_mode);
		event = GPAD_IPEM_EVENT_PERFORMANCE_OFF;
		gpad_ipem_do(dev, event);
		break;
	default:
		GPAD_LOG_ERR("gpad_ipem_performance_mode: mode must be 0 or 1!\n");
		break;
	}


	count = atomic_read(&g_in_performance_mode);
	if (unlikely(count < 0)) {
		GPAD_LOG_ERR("gpad_ipem_performance_mode: disable more than enable!\n");
	}
}

/*
 * *******************************************************************
 *  ipem queue related
 * *******************************************************************
 */
static void gpad_ipem_queue_init(struct gpad_device *dev)
{
	u32 i;
	/*struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev); */
	struct gpad_ipem_queue_info *queue_info = GET_QUEUE_INFO();
	USE_WORK_LOCK;

	GPAD_LOG_INFO("[IPEM] %s: queue_init...\n", __func__);
	/* FIXME: release lock before calling */
	/* TODO: use the appropriate workqueue types */
	queue_info->workid = 0;
	queue_info->workcnt = 0;
	queue_info->workqueue = create_singlethread_workqueue("ipem_workq");
	init_waitqueue_head(&(queue_info->waitqueue));

	LOCK_WORK_INFO();
	memset(s_work_info_allocation, 0, sizeof(s_work_info_allocation));
	for (i = 0; i < GPAD_IPEM_EVENT_LENGTH; ++i) {
		atomic_set(&s_work_info_allocation[i].in_use, 0);
	}
	UNLOCK_WORK_INFO();
}

static void gpad_ipem_queue_exit(struct gpad_device *dev)
{
	/*struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev); */
	struct gpad_ipem_queue_info *queue_info = GET_QUEUE_INFO();

	flush_workqueue(queue_info->workqueue);
	destroy_workqueue(queue_info->workqueue);

	g_gpuload = 0;
	g_idle    = 100;
}





/*
 * *******************************************************************
 *  ipem timer related
 * *******************************************************************
 */
static void gpad_ipem_timer_init(struct gpad_device *dev)
{
	/*struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev); */
	struct gpad_ipem_timer_info *timer_info = GET_TIMER_INFO();

	unsigned long interval;
	USE_TIMER_LOCK;

	GPAD_LOG_INFO("[IPEM] %s: timer_init...\n", __func__);

	interval = GPAD_IPEM_TIMER_INTERVAL;

	init_timer(&(timer_info->timer));

	LOCK_TIMER();
	timer_info->timer_interval = interval;
	timer_info->timer.function = gpad_ipem_callback_timer;
	UNLOCK_TIMER();

	GPAD_LOG_INFO("[IPEM] %s: interval = %lu\n", __func__, interval);
}

static void gpad_ipem_timer_exit(struct gpad_device *dev)
{
	/*struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev); */
	struct gpad_ipem_timer_info *timer_info = GET_TIMER_INFO();

	del_timer_sync(&(timer_info->timer));
}

int gpad_ipem_timer_enable(struct gpad_device *dev)
{
	/*struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev); */
	struct gpad_ipem_timer_info *timer_info = GET_TIMER_INFO();

	unsigned long expires;
	USE_TIMER_LOCK;

	LOCK_TIMER();
	expires = jiffies + timer_info->timer_interval;
	UNLOCK_TIMER();

	mod_timer(&(timer_info->timer), expires);

	GPAD_LOG_INFO("[IPEM] %s: timer is enabled\n", __func__);

	return 0;
}

int gpad_ipem_timer_disable(struct gpad_device *dev)
{
	/*struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev); */
	struct gpad_ipem_timer_info *timer_info = GET_TIMER_INFO();

	del_timer(&(timer_info->timer));

	GPAD_LOG_INFO("[IPEM] %s: timer is disabled\n", __func__);

	return 0;
}

unsigned long gpad_ipem_timer_get(struct gpad_device *dev)
{
	/*struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev); */
	struct gpad_ipem_timer_info *timer_info = GET_TIMER_INFO();
	unsigned long interval;
	USE_TIMER_LOCK;

	LOCK_TIMER();
	interval = timer_info->timer_interval;
	UNLOCK_TIMER();

	return interval;
}

void gpad_ipem_timer_set(struct gpad_device *dev, unsigned long interval)
{
	/*struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev); */
	struct gpad_ipem_timer_info *timer_info = GET_TIMER_INFO();
	USE_TIMER_LOCK;

	LOCK_TIMER();
	if (interval != timer_info->timer_interval) {
		timer_info->timer_interval = interval;

		GPAD_LOG_INFO("[IPEM] %s: interval = %lu\n", __func__, interval);
	}
	UNLOCK_TIMER();
}




/*
 * *******************************************************************
 *  ipem table related
 * *******************************************************************
 */
static void gpad_ipem_table_init(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);
	USE_IPEM_LOCK;

	struct mtk_gpufreq_info *table;
	unsigned int table_id;
	unsigned int table_size;
	unsigned int efuse_tag;

	efuse_tag = get_devinfo_with_index(4);
	efuse_tag = (efuse_tag & 0x00f00000) >> 20;
	switch (efuse_tag) {
	case 0x8:
		table_id = GPAD_IPEM_TABLE_ID_MT8163A_SD;
		break;
	case 0x6:
		table_id = GPAD_IPEM_TABLE_ID_MT8163A_SB;
		break;
	case 0x5:
		table_id = GPAD_IPEM_TABLE_ID_MT8163A_SA;
		break;
	default:
		table_id = GPAD_IPEM_DEF_TABLE_ID;
	}

	table = g_ipem_tables[table_id];
	table_size = g_ipem_table_sizes[table_id];

	gs_gpufreq_register(table, table_size);

	LOCK_IPEM();
	ipem_info->table_info.table_id = table_id;
	ipem_info->table_info.table = table;
	ipem_info->table_info.table_size = table_size;
	UNLOCK_IPEM();

	GPAD_LOG("[IPEM] %s: table_id = %u, table = %p, table_size = %u\n", __func__, table_id, table, table_size);
}

static void gpad_ipem_table_exit(struct gpad_device *dev)
{
	gs_gpufreq_unregister();
}

unsigned int gpad_ipem_table_get(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	return ipem_info->table_info.table_id;
}

void gpad_ipem_table_set(struct gpad_device *dev, unsigned int table_id)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	struct mtk_gpufreq_info *table;
	unsigned int table_size;
	USE_IPEM_LOCK;

	if (table_id >= (unsigned int)ARRAY_SIZE(g_ipem_table_sizes)) {
		GPAD_LOG_ERR("Setting an invalid IPEM table id: %u!\n", table_id);
		return;
	}

	LOCK_IPEM();
	if (table_id != ipem_info->table_info.table_id) {
		table = g_ipem_tables[table_id];
		table_size = g_ipem_table_sizes[table_id];

		ipem_info->table_info.table_id = table_id;
		ipem_info->table_info.table = table;
		ipem_info->table_info.table_size = table_size;
		UNLOCK_IPEM();

		gs_gpufreq_unregister();
		gs_gpufreq_register(table, table_size);

		GPAD_LOG("[IPEM] %s: table_id = %u, table = %p, table_size = %u\n", __func__, table_id, table, table_size);
	} else {
		UNLOCK_IPEM();
	}
}




/*
 * *******************************************************************
 *  ipem tracker related
 * *******************************************************************
 */
/* TODO: refine the tracker infrastructure */
static void gpad_ipem_tracker_init(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);
	USE_IPEM_LOCK;

	LOCK_IPEM();
	ipem_info->tracker_info.event_prev = GPAD_IPEM_EVENT_INVALID;
	ipem_info->tracker_info.event_next = GPAD_IPEM_EVENT_INVALID;
	ipem_info->tracker_info.timestamp_prev = 0;
	ipem_info->tracker_info.timestamp_next = 0;
	ipem_info->tracker_info.timestamp_off2on = 0;
	ipem_info->tracker_info.timebusy_prev = 0;
	ipem_info->tracker_info.timebusy_next = 0;
	ipem_info->tracker_info.timetotal_prev = 0;
	ipem_info->tracker_info.timetotal_next = 0;

	ipem_info->tracker_info.policy_do_prev = NULL;
	ipem_info->tracker_info.policy_do_next = NULL;
	ipem_info->tracker_info.policy_update_prev = NULL;
	ipem_info->tracker_info.policy_update_next = NULL;
	ipem_info->tracker_info.state_prev = 0;
	ipem_info->tracker_info.state_next = 0;
	ipem_info->tracker_info.action_prev = 0;
	ipem_info->tracker_info.action_next = 0;
	UNLOCK_IPEM();
}

static void gpad_ipem_tracker_exit(struct gpad_device *dev)
{
}

static void gpad_ipem_tracker_update_top(struct gpad_ipem_info *ipem_info, enum gpad_ipem_event event, unsigned long timestamp, u32 timebusy, u32 timetotal)
{
	enum gpad_ipem_event event_prev;
	enum gpad_ipem_event event_next;
	unsigned long timestamp_prev;
	unsigned long timestamp_next;
	unsigned long timestamp_off2on;
	unsigned long timestamp_diff;
	u32 timebusy_prev;
	u32 timebusy_next;
	u32 timetotal_prev;
	u32 timetotal_next;
	USE_IPEM_LOCK;

	LOCK_IPEM();
	event_prev = ipem_info->tracker_info.event_next;
	event_next = event;
	timestamp_prev = ipem_info->tracker_info.timestamp_next;
	timestamp_next = timestamp;

	timestamp_diff = timestamp_next - timestamp_prev;
	if (event_next == GPAD_IPEM_EVENT_POWEROFF) {
		timestamp_off2on = 0;
	} else {
		timestamp_off2on += timestamp_diff;
	}

	timebusy_prev = ipem_info->tracker_info.timebusy_next;
	timebusy_next = timebusy;
	timetotal_prev = ipem_info->tracker_info.timetotal_next;
	timetotal_next = timetotal;

	ipem_info->tracker_info.event_prev = event_prev;
	ipem_info->tracker_info.event_next = event_next;
	ipem_info->tracker_info.timestamp_prev = timestamp_prev;
	ipem_info->tracker_info.timestamp_next = timestamp_next;
	ipem_info->tracker_info.timestamp_off2on = timestamp_off2on;
	ipem_info->tracker_info.timebusy_prev = timebusy_prev;
	ipem_info->tracker_info.timebusy_next = timebusy_next;
	ipem_info->tracker_info.timetotal_prev = timetotal_prev;
	ipem_info->tracker_info.timetotal_next = timetotal_next;
	UNLOCK_IPEM();

	GPAD_LOG_INFO("[IPEM] %s: callback_prev = %u, callback_next = %u, timestamp_prev = %lu, timestamp_next = %lu, timestamp_off2on = %lu, timebusy_prev = %u, timebusy_next = %u, timetotal_prev = %u, timetotal_next = %u\n", __func__, event_prev, event_next, timestamp_prev, timestamp_next, timestamp_off2on, timebusy_prev, timebusy_next, timetotal_prev, timetotal_next);
}

static void gpad_ipem_tracker_update_middle(struct gpad_ipem_work_info *work_info, gpad_ipem_policy_do_func policy_do, gpad_ipem_policy_update_func policy_update, gpad_ipem_state state)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	gpad_ipem_policy_do_func policy_do_prev;
	gpad_ipem_policy_do_func policy_do_next;
	gpad_ipem_policy_update_func policy_update_prev;
	gpad_ipem_policy_update_func policy_update_next;
	gpad_ipem_state state_prev;
	gpad_ipem_state state_next;
	USE_IPEM_LOCK;

	LOCK_IPEM();
	policy_do_prev = ipem_info->tracker_info.policy_do_next;
	policy_do_next = policy_do;
	policy_update_prev = ipem_info->tracker_info.policy_update_next;
	policy_update_next = policy_update;

	state_prev = ipem_info->tracker_info.state_next;
	state_next = state;

	ipem_info->tracker_info.policy_do_prev = policy_do_prev;
	ipem_info->tracker_info.policy_do_next = policy_do_next;
	ipem_info->tracker_info.policy_update_prev = policy_update_prev;
	ipem_info->tracker_info.policy_update_next = policy_update_next;
	ipem_info->tracker_info.state_prev = state_prev;
	ipem_info->tracker_info.state_next = state_next;
	UNLOCK_IPEM();

	GPAD_LOG_INFO("[IPEM] %s: policy_do_prev = %p, policy_do_next = %p, policy_update_prev = %p, policy_update_next = %p, state_prev = %u, state_next = %u\n", __func__, policy_do_prev, policy_do_next, policy_update_prev, policy_update_next, state_prev, state_next);
}

static void gpad_ipem_tracker_update_bottom_front(struct gpad_ipem_work_info *work_info)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	gpad_ipem_action action_prev;
	USE_IPEM_LOCK;

	LOCK_IPEM();
	action_prev = ipem_info->tracker_info.action_next;

	ipem_info->tracker_info.action_prev = action_prev;
	UNLOCK_IPEM();

	GPAD_LOG_INFO("[IPEM] %s: action_prev = %u\n", __func__, action_prev);
}

static void gpad_ipem_tracker_update_bottom_back(struct gpad_ipem_work_info *work_info, gpad_ipem_action action)
{
	struct gpad_ipem_info *ipem_info = work_info->ipem_info;

	gpad_ipem_action action_next;
	USE_IPEM_LOCK;

	LOCK_IPEM();
	action_next = action;

	ipem_info->tracker_info.action_next = action_next;
	UNLOCK_IPEM();

	GPAD_LOG_INFO("[IPEM] %s: action_next = %u\n", __func__, action_next);
}




/*
 * *******************************************************************
 *  ipem statistic related
 * *******************************************************************
 */

/* FIXME: need hack for unit test? */
/* assumption : LOCK_IEPM */
unsigned int gpad_ipem_statistic_gpuload_old(struct gpad_ipem_info *ipem_info, struct gpad_pm_info *pm_info, bool isCalc)
{
#if 0
	/* return random load */
	unsigned int rand;

	get_random_bytes(&rand, sizeof(unsigned int));

	return rand%100;
#else

	/* assumption : atomic rw */
	static unsigned int s_pre_gpuload;

	unsigned int timebusy_prev;
	unsigned int timebusy_next;
	unsigned int timetotal_prev;
	unsigned int timetotal_next;
	unsigned int gpuload;
	unsigned int timetotal_diff;

	if (!isCalc) {

		return s_pre_gpuload;
	}

	/* get the timestamp of gpu through pm */
	timebusy_prev = ipem_info->tracker_info.timebusy_prev;
	timebusy_next = pm_info->stat.busy_time;

	timetotal_prev = ipem_info->tracker_info.timetotal_prev;
	timetotal_next = pm_info->stat.total_time;
	/*WARN_ON(timetotal_next < timetotal_prev);  FIXME: this assumption is not valid */

		timetotal_diff = (timetotal_next - timetotal_prev)/100;

	/* nothing runing for gpu  */
	if (timebusy_next == 0) {
		gpuload = 3;
		s_pre_gpuload = gpuload;
	} else if (timetotal_diff && (timetotal_next > timetotal_prev)) {
		/* TODO: double check if gpuload is overflow or underflow */
		gpuload = (timebusy_next - timebusy_prev) / timetotal_diff;
		s_pre_gpuload = gpuload;
	} else {
		/* very short call, use previous */
		gpuload = s_pre_gpuload;
	}

	ipem_info->tracker_info.timebusy_prev = timebusy_next;
	ipem_info->tracker_info.timetotal_prev = timetotal_next;

	return gpuload;
#endif
}

static gpad_ipem_fixedpt gpad_ipem_statistic_reward(struct gpad_ipem_info *ipem_info)
{
	/*struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev); */

	/* implement IPEM Reward here */
	return gpad_ipem_fixedpt_rconst(1);
}

static unsigned long gpad_ipem_statistic_idle(struct gpad_ipem_info *ipem_info)
{
	/* implement IPEM Idle here */
	enum gpad_ipem_event event_next;
	unsigned long timestamp_off2on;
	unsigned long idle_period;
	USE_IPEM_LOCK;

	LOCK_IPEM();
	event_next = ipem_info->tracker_info.event_next;
	timestamp_off2on = ipem_info->tracker_info.timestamp_off2on;
	UNLOCK_IPEM();

	/* the resolution (us) */
	if (event_next == GPAD_IPEM_EVENT_POWERON) {
		idle_period = do_div(timestamp_off2on, 1000);
	} else {
		idle_period = 0;
	}

	return idle_period;
}

/* assumption : LOCK_WORK_INFO, GPAD_IPEM_CALLBACK_LENGTH  */
static void gpad_ipem_statistic_update_top(struct gpad_ipem_info *ipem_info, struct gpad_pm_info *pm_info, struct gpad_ipem_statistic_info *statistic_info)
{
	/*struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev); */

	enum gpad_ipem_event event;
	unsigned long timestamp;
	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;
	gpad_ipem_action action_prev;
	unsigned int gpuload;
	unsigned int thermal;
	unsigned int touch;
	gpad_ipem_fixedpt flush = 0; /* TODO: from g3dkmd */
	gpad_ipem_fixedpt ht = 0; /* TODO: from g3dkmd */
	gpad_ipem_fixedpt reward;
	unsigned long idle;
	USE_IPEM_LOCK;

	/* get callback */
	event = ipem_info->tracker_info.event_next;

	/* get timestamp */
	timestamp = ipem_info->tracker_info.timestamp_next;

	/* TODO: can it be the one to replace "thermal" and "touch" ? */
	/* get statistic from [gpufreq] */
	gs_gpufreq_get_cur_indices(&action_ulimit, &action_llimit, &action_prev);

	/* get statistic from [gpad][pm] */
#if GPAD_IPEM_NEW_GPULOAD
	gpuload = g_gpuload;
	/*gpad_ipem_statistic_gpuload_old(ipem_info, pm_info, event == GPAD_IPEM_EVENT_TIMER ); */
#else
	gpuload = gpad_ipem_statistic_gpuload_old(ipem_info, pm_info, event == GPAD_IPEM_EVENT_TIMER);
	g_gpuload = gpuload;
	g_idle = GPAD_IPEM_UPPER_BOUND_MAX - gpuload;
#endif


	/* get thermal */
	thermal = ipem_info->tracker_info.event_next == GPAD_IPEM_EVENT_THERMAL ? 1 : 0;

	/* get touch */
	touch = ipem_info->tracker_info.event_next == GPAD_IPEM_EVENT_TOUCH ? 1 : 0;

	/* get statistic from [g3dkmd] */
	/* TODO: calculate rate of the statistic */

	/* get reward */
	reward = gpad_ipem_statistic_reward(ipem_info);

	/* get idle */
	idle = gpad_ipem_statistic_idle(ipem_info);

	/* update statistic*/
	LOCK_IPEM();
	statistic_info->event = event;
	statistic_info->timestamp = timestamp;
	statistic_info->action_ulimit = action_ulimit;
	statistic_info->action_llimit = action_llimit;
	statistic_info->action_prev = action_prev;
	statistic_info->gpuload = gpuload;
	statistic_info->thermal = thermal;
	statistic_info->touch = touch;
	statistic_info->flush = flush;
	statistic_info->ht = ht;
	statistic_info->reward = reward;
	statistic_info->idle = idle;
	UNLOCK_IPEM();

	GPAD_LOG_INFO("[IPEM] %s: action_prev = %u, action_llimit = %u, action_ulimit = %u\n", __func__, action_prev, action_llimit, action_ulimit);
	GPAD_LOG_INFO("[IPEM] %s: gpuload = %u\n", __func__, gpuload);
	GPAD_LOG_INFO("[IPEM] %s: thermal = %u\n", __func__, thermal);
	GPAD_LOG_INFO("[IPEM] %s: touch = %u\n", __func__, touch);
	GPAD_LOG_INFO("[IPEM] %s: flush = %s\n", __func__, gpad_ipem_fixedpt_cstr(flush));
	GPAD_LOG_INFO("[IPEM] %s: ht = %s\n", __func__, gpad_ipem_fixedpt_cstr(ht));
	GPAD_LOG_INFO("[IPEM] %s: reward = %s\n", __func__, gpad_ipem_fixedpt_cstr(reward));
	GPAD_LOG_INFO("[IPEM] %s: idle = %lu\n", __func__, idle);
}

static void gpad_ipem_statistic_update_bottom(struct gpad_device *dev, struct gpad_ipem_statistic_info *statistic_info)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	gpad_ipem_policy_do_func policy_do;
	gpad_ipem_policy_update_func policy_update;
	gpad_ipem_state state;
	gpad_ipem_action action;
	USE_IPEM_LOCK;

	/* get policy_do */
	policy_do = ipem_info->tracker_info.policy_do_next;

	/* get policy_update */
	policy_update = ipem_info->tracker_info.policy_update_next;

	/* get state */
	state = ipem_info->tracker_info.state_next;

	/* get action */
	action = ipem_info->tracker_info.action_next;

	/* update statistic*/
	LOCK_IPEM();
	statistic_info->policy_do = policy_do;
	statistic_info->policy_update = policy_update;
	statistic_info->state = state;
	statistic_info->action = action;
	UNLOCK_IPEM();

	GPAD_LOG_INFO("[IPEM] %s: policy_do = %p, policy_update = %p, state = %u, action = %u\n", __func__, policy_do, policy_update, state, action);
}




/*
 * *******************************************************************
 *  ipem scoreboard related
 * *******************************************************************
 */
static void gpad_ipem_scoreboard_init(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);
	gpad_ipem_fixedpt     *qs;
	gpad_ipem_fixedpt     *epsilons;
	USE_IPEM_LOCK;

	/* TODO: dynamic size of scoreboard */

	qs = kzalloc(GPAD_IPEM_SCOREBOARD_QS_SIZE, GFP_KERNEL);
	epsilons = kzalloc(GPAD_IPEM_SCOREBOARD_EPSILONS_SIZE, GFP_KERNEL);

	LOCK_IPEM();
	ipem_info->scoreboard_info.qs = qs;
	ipem_info->scoreboard_info.epsilons = epsilons;
	UNLOCK_IPEM();
}

static void gpad_ipem_scoreboard_exit(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	kfree(ipem_info->scoreboard_info.qs);
	kfree(ipem_info->scoreboard_info.epsilons);
}




/*
 * *******************************************************************
 *  ipem state related
 * *******************************************************************
 */
static void gpad_ipem_state_enc(gpad_ipem_state *state, struct gpad_ipem_statistic_info *statistic_info)
{
	/* TODO: survey the state partitioning studies, give it a better approach*/
	/* FIXME: refine the implementation and state encoding method */
	gpad_ipem_action action_prev;
	gpad_ipem_action action_llimit;
	gpad_ipem_action action_ulimit;
	gpad_ipem_fixedpt gpuload;
	gpad_ipem_fixedpt flush;
	gpad_ipem_fixedpt ht;
	unsigned int bmask = 0x3;

	/*state aggregation = 1 */
	action_prev = statistic_info->action_prev;
	action_llimit = statistic_info->action_llimit;
	action_ulimit = statistic_info->action_ulimit;

	/*state aggregation = 25 */
	gpuload = gpad_ipem_fixedpt_toint(statistic_info->gpuload / 25);

	/*state aggregation = 4 */
	flush = gpad_ipem_fixedpt_toint(statistic_info->flush / 4);

	/*state aggregation = 2 */
	ht = gpad_ipem_fixedpt_toint(statistic_info->ht / 2);

	*state = 0;

	*state = *state << 2;
	*state = *state | (action_prev & bmask);

	*state = *state << 2;
	*state = *state | (action_llimit & bmask);

	*state = *state << 2;
	*state = *state | (action_ulimit & bmask);

	*state = *state << 2;
	*state = *state | (gpuload & bmask);

	*state = *state << 2;
	*state = *state | (flush & bmask);

	*state = *state << 2;
	*state = *state | (ht & bmask);
}

/*static void gpad_ipem_state_dec(gpad_ipem_state state, struct gpad_ipem_statistic_info *statistic_info) */
/*{ */
/*} */




/*
 * *******************************************************************
 *  ipem recorder related
 * *******************************************************************
 */
static void gpad_ipem_recorder_init(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);
	struct gpad_ipem_record_info *records;
	USE_IPEM_LOCK;

	records = kzalloc(GPAD_IPEM_RECORDER_RECORDS_SIZE, GFP_KERNEL);

	LOCK_IPEM();
	ipem_info->recorder_info.records = records;
	ipem_info->recorder_info.recordid = 0;
	UNLOCK_IPEM();
}

static void gpad_ipem_recorder_exit(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	kfree(ipem_info->recorder_info.records);
}

/* TODO: a better naming and a function for getting the records */
static void gpad_ipem_recorder_do(struct gpad_device *dev, unsigned int workid, struct gpad_ipem_statistic_info *statistic_info)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	unsigned int recordid;
	USE_IPEM_LOCK;

	LOCK_IPEM();
	recordid = ipem_info->recorder_info.recordid;

	ipem_info->recorder_info.records[recordid].workid = workid;
	memcpy(&(ipem_info->recorder_info.records[recordid].statistic_info), statistic_info, sizeof(*statistic_info));
	ipem_info->recorder_info.records[recordid].workid = workid;

	recordid++;
	if (recordid >= (GPAD_IPEM_RECORDER_RECORDS_SIZE / sizeof(struct gpad_ipem_record_info))) {
		recordid = 0;
	}

	ipem_info->recorder_info.recordid = recordid;
	UNLOCK_IPEM();
}




/*
 * *******************************************************************
 *  ipem callback related
 * *******************************************************************
 */
void gpad_ipem_callback_timer(unsigned long data)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_TIMER;

	++g_called[event];
	++g_recursive_called[event];

	if (g_recursive_called_max[event] < g_recursive_called[event]) {
		g_recursive_called_max[event] = g_recursive_called[event];
	}

	gpad_ipem_do(dev, event);
	--g_recursive_called[event];
}

void gpad_ipem_callback_thermal(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_THERMAL;

	++g_called[event];
	++g_recursive_called[event];

	if (g_recursive_called_max[event] < g_recursive_called[event]) {
		g_recursive_called_max[event] = g_recursive_called[event];
	}

	gpad_ipem_do(dev, event);
	--g_recursive_called[event];
}

void gpad_ipem_callback_touch(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_TOUCH;

	++g_called[event];
	++g_recursive_called[event];

	if (g_recursive_called_max[event] < g_recursive_called[event]) {
		g_recursive_called_max[event] = g_recursive_called[event];
	}

	gpad_ipem_do(dev, event);
	--g_recursive_called[event];
}

void gpad_ipem_callback_powerlow(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_POWERLOW;

	++g_called[event];
	++g_recursive_called[event];

	if (g_recursive_called_max[event] < g_recursive_called[event]) {
		g_recursive_called_max[event] = g_recursive_called[event];
	}

	s_is_command_queue_busy = false;

	gpad_ipem_do(dev, event);
	--g_recursive_called[event];
}

void gpad_ipem_callback_powerhigh(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_POWERHIGH;

	++g_called[event];
	++g_recursive_called[event];

	if (g_recursive_called_max[event] < g_recursive_called[event]) {
		g_recursive_called_max[event] = g_recursive_called[event];
	}

	s_is_command_queue_busy = true;

	gpad_ipem_do(dev, event);
	--g_recursive_called[event];
}

void gpad_ipem_callback_poweroff(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_POWEROFF;

	++g_called[event];
	++g_recursive_called[event];

	if (g_recursive_called_max[event] < g_recursive_called[event]) {
		g_recursive_called_max[event] = g_recursive_called[event];
	}
	/* TODO: extract this to be a common function ? */


	/* FIXME: integrate the mtcmos function into gpad_ipem_do_work */
	gpad_ipem_do(dev, event);
	--g_recursive_called[event];
	gs_gpufreq_mtcmos_off();
}

void gpad_ipem_callback_poweron(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_POWERON;

	++g_called[event];
	++g_recursive_called[event];

	if (g_recursive_called_max[event] < g_recursive_called[event]) {
		g_recursive_called_max[event] = g_recursive_called[event];
	}

	/* FIXME: integrate the mtcmos function into gpad_ipem_do_work */
	gs_gpufreq_mtcmos_on();
	gpad_ipem_do(dev, event);
	--g_recursive_called[event];
}

void gpad_ipem_callback_poweroff_begin(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_POWEROFF_BEGIN;

	++g_called[event];
	++g_recursive_called[event];

	if (g_recursive_called_max[event] < g_recursive_called[event]) {
		g_recursive_called_max[event] = g_recursive_called[event];
	}

	gpad_ipem_do(dev, event);
	--g_recursive_called[event];
}

void gpad_ipem_callback_poweroff_end(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_POWEROFF_END;

	++g_called[event];
	++g_recursive_called[event];

	if (g_recursive_called_max[event] < g_recursive_called[event]) {
		g_recursive_called_max[event] = g_recursive_called[event];
	}

	gpad_ipem_do(dev, event);
	--g_recursive_called[event];
}

void gpad_ipem_callback_poweron_begin(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_POWERON_BEGIN;

	++g_called[event];
	++g_recursive_called[event];

	if (g_recursive_called_max[event] < g_recursive_called[event]) {
		g_recursive_called_max[event] = g_recursive_called[event];
	}

	gpad_ipem_do(dev, event);
	--g_recursive_called[event];
}

void gpad_ipem_callback_poweron_end(void)
{
	struct gpad_device *dev = gpad_get_default_dev();
	enum gpad_ipem_event event = GPAD_IPEM_EVENT_POWERON_END;

	++g_called[event];
	++g_recursive_called[event];

	if (g_recursive_called_max[event] < g_recursive_called[event]) {
		g_recursive_called_max[event] = g_recursive_called[event];
	}

	gpad_ipem_do(dev, event);
	--g_recursive_called[event];
}

void gpad_ipem_callback_pm(struct gpad_device *dev, struct gpad_ipem_pm_data *data)
{
	static unsigned long long s_acc_busy_ns_prev;
	static unsigned long long s_acc_total_ns_prev;

	unsigned long long acc_busy_ns  = data->acc_busy_ns;
	unsigned long long acc_total_ns = data->acc_total_ns;

	long long busy_diff_64 = 0;
	long long total_diff_64 = 0;

	u32 gpuload = 0;
	s32 busy_diff = 0;
	s32 total_diff = 0;

	if (acc_total_ns == s_acc_total_ns_prev) {
		/* keep current */
		/* TODO : fill it */
		if (s_is_command_queue_busy) {
			g_gpuload = 97; /* 101 ? */
			g_idle    = 0;
		} else {
			g_gpuload = 3;
			g_idle    = 100;
		}
	} else {
		busy_diff_64  = (long long)acc_busy_ns  - (long long)s_acc_busy_ns_prev;
		total_diff_64 = (long long)acc_total_ns - (long long)s_acc_total_ns_prev;

		if (unlikely((total_diff_64 < 0) || (busy_diff_64 < 0))) {
			GPAD_LOG_ERR("Logic Error : (total_diff_64<0) || (busy_diff_64<0)");
			busy_diff_64  = 0;
			total_diff_64 = (1<<10);
		}

		total_diff = (s32)(total_diff_64 >> 10);
		busy_diff  = (s32)(busy_diff_64 >> 10);

		if (likely(total_diff)) {
			gpuload = (busy_diff*100) / total_diff; /* << 7 */
		} else {
			gpuload = 4;
		}
		g_gpuload = gpuload;
		g_idle    = 100 - gpuload;

	}

	s_acc_busy_ns_prev  = acc_busy_ns;
	s_acc_total_ns_prev = acc_total_ns;
}



/*
 * *******************************************************************
 *  ipem do related
 * *******************************************************************
 */
/* Assumption : Job is sequencial */
static void gpad_ipem_do_work(struct work_struct *work)
{
	struct gpad_ipem_work_info *work_info;
	struct gpad_device *dev;
	struct gpad_ipem_info *ipem_info;
	/* TODO: make me much more compact */
	unsigned int workid;
	/*enum gpad_ipem_callback callback; */
	gpad_ipem_state state;
	gpad_ipem_action action;
	gpad_ipem_policy_do_func policy_do;
	gpad_ipem_policy_update_func policy_update;
	struct gpad_ipem_timer_info *timer_info = GET_TIMER_INFO();
	enum gpad_ipem_event                event;
	struct gpad_pm_info *pm_info;
	unsigned long timestamp;
	unsigned int timebusy;
	unsigned int timetotal;
	unsigned long expires;
	USE_TIMER_LOCK;
	USE_WORK_LOCK;

	work_info = container_of(work, struct gpad_ipem_work_info, work);
	dev = work_info->dev;
	ipem_info = GPAD_IPEM_INFO_GET(dev);
	pm_info = &dev->pm_info;
	event = work_info->event;

	if (ipem_info->enable == 0) {
		LOCK_WORK_INFO();
		atomic_set(&work_info->in_use, 0);
		UNLOCK_WORK_INFO();
		return;
	}

	workid = work_info->workid;

	/* TODO : lock pm_info */
	timestamp = cpu_clock(0);
	timebusy = pm_info->stat.busy_time;
	timetotal = pm_info->stat.total_time;
	gpad_ipem_tracker_update_top(ipem_info, event, timestamp, timebusy, timetotal);

	/*gpad_ipem_statistic_update_top(dev, &(work_info->statistic_info));    <workaround> */
		gpad_ipem_statistic_update_top(ipem_info, pm_info, &(ipem_info->_statistic_info));


	GPAD_LOG_INFO("[IPEM] %s: workid = %u, jiffies = %lu\n", __func__, workid, jiffies);

	/*ipem_info->statistic_info = &(work_info->statistic_info);    <workaround> */
		ipem_info->statistic_info = &(ipem_info->_statistic_info);

	/*callback = ipem_info->statistic_info->callback; */
	switch (event) {
	case GPAD_IPEM_EVENT_POWERLOW:
	case GPAD_IPEM_EVENT_POWEROFF:
		policy_do = gpad_ipem_dpm_do;
		policy_update = gpad_ipem_dpm_update;
		break;
	case GPAD_IPEM_EVENT_POWERHIGH:
	case GPAD_IPEM_EVENT_POWERON:
	case GPAD_IPEM_EVENT_TOUCH:
	case GPAD_IPEM_EVENT_THERMAL:
	case GPAD_IPEM_EVENT_TIMER:
		policy_do = gpad_ipem_dvfs_do;
		policy_update = gpad_ipem_dvfs_update;
		break;
	case GPAD_IPEM_EVENT_POWEROFF_BEGIN:
	case GPAD_IPEM_EVENT_POWEROFF_END:
	case GPAD_IPEM_EVENT_POWERON_BEGIN:
	case GPAD_IPEM_EVENT_POWERON_END:
		policy_do = NULL;
		policy_update = NULL;
		break;
	default:
		policy_do = NULL;
		policy_update = NULL;
		GPAD_LOG_ERR("[IPEM] %s: unknown callback = %u\n", __func__, event);
		break;
	}

	if (event == GPAD_IPEM_EVENT_TOUCH) {
		g_in_touch = 1;
	}

	gpad_ipem_state_enc(&state, ipem_info->statistic_info);
	gpad_ipem_tracker_update_middle(work_info, policy_do, policy_update, state);
	gpad_ipem_tracker_update_bottom_front(work_info);

	if (policy_do != NULL) {
		action = policy_do(work_info);
	} else {
		action = ipem_info->tracker_info.action_next;
	}

	gpad_ipem_tracker_update_bottom_back(work_info, action);

	if (ipem_info->tracker_info.policy_update_prev != NULL) {
		ipem_info->tracker_info.policy_update_prev(work_info);
	}

	gpad_ipem_statistic_update_bottom(dev, ipem_info->statistic_info);
	gpad_ipem_recorder_do(dev, workid, ipem_info->statistic_info);

	/* FIXME: handle power state transition */
	/* TODO: handle the situation that not all the actions go through ipem */
	/*if (action != ipem_info->tracker_info.action_prev) { */
	/*    if (ipem_info->table_info.table[action].volt > 0) { */

	gpad_pm_change_freq(dev, ipem_info->table_info.table[action].freq_khz * 1000);
	gs_gpufreq_set_cur_index(action);
	gs_gpufreq_switch_handler();
	/*    } else { */
	/* FIXME: turn off power */
	/*    } */
	/*} */

	/* TODO: refine this loosely written codes */
	if (ipem_info->enable == 1 && event == GPAD_IPEM_EVENT_TIMER) {
		LOCK_TIMER();
		expires = jiffies + timer_info->timer_interval;
		UNLOCK_TIMER();

		mod_timer(&(timer_info->timer), expires);
	}

	LOCK_WORK_INFO();
	atomic_set(&work_info->in_use, 0);
	UNLOCK_WORK_INFO();

	GPAD_LOG_INFO("[IPEM] %s: event = %u, state = %u, action = %u\n", __func__, event, state, action);
}

/* lock hierarchy */
/* LOCK_WORK_INFO -> LOCK_IPEM */
/* Assumption : same type won't goes in at the same time */
/* modify only work queue info */
/* and in queue */
static void gpad_ipem_do(struct gpad_device *dev, enum gpad_ipem_event event)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);
	struct gpad_ipem_queue_info *queue_info = GET_QUEUE_INFO();
	s32 performance_mode = atomic_read(&g_in_performance_mode);

	struct gpad_ipem_work_info *work_info;/* = &s_work_info_allocation; */
	unsigned int workid;
	/*s32 in_use; */
	USE_WORK_LOCK;
	USE_WQ_LOCK;

	/* invalid argument */
	if (event >= GPAD_IPEM_EVENT_LENGTH) {
		GPAD_LOG_ERR("[IPEM] %s: unknown callback = %u\n", __func__, event);
		return;
	}

	/* No need to do the following works. */
	if (ipem_info->enable == 0) {
		return;
	}

	LOCK_WORK_INFO();
	work_info = &s_work_info_allocation[event];
	if (atomic_read(&work_info->in_use)) {
		/* TODO increase skip count */
		UNLOCK_WORK_INFO();
		return;
	} else {
		/*memset(work_info, 0, sizeof(*work_info)); */
		atomic_inc(&work_info->in_use);
		UNLOCK_WORK_INFO();
	}

	/*work_info = kzalloc(sizeof(*work_info), GFP_KERNEL); */

	work_info->dev = dev;
	work_info->ipem_info = ipem_info;
	work_info->event = event;

	INIT_WORK(&(work_info->work), gpad_ipem_do_work);

	LOCK_WQ();
	workid = queue_info->workcnt;
	queue_info->workcnt++;
	UNLOCK_WQ();
	work_info->workid = workid;
	work_info->performance_mode = performance_mode;

	queue_work(queue_info->workqueue, &(work_info->work));
	/*schedule_work(&(work_info->work)); */

	GPAD_LOG_INFO("[IPEM] %s: event = %u, workid = %u\n", __func__, event, workid);
}

/*
 * *******************************************************************
 *  ipem Power Related
 * *******************************************************************
 */
u32 gpad_ipem_met_loading_(void)
{
	return g_gpuload;
}

u32 gpad_ipem_met_block_(void)
{
	return 443;  /* no meaning, just place holder */
}

u32 gpad_ipem_met_idle_(void)
{
	return g_idle;
}

u32 gpad_ipem_met_power_loading_(void)
{
#if 0
	unsigned int gpuload = 0;
	gpuload = gpad_ipem_statistic_gpuload_old(0, 0, false);
	return gpuload;
#else
	return 445;  /* no meaning, just place holder */
#endif
}

/*
 * *******************************************************************
 *  ipem Power Related
 * *******************************************************************
 */
/* Assumption : The do_work() will execute sequencially. */
/* When dpm_timeout_do waits, no other work will run.  */
/* Works will keep Queuing, until (A) Queue(the power turns on) or (B) timeout */
bool gpad_ipem_is_queuing_power_on(void)
{

	struct gpad_ipem_work_info *p = &s_work_info_allocation[GPAD_IPEM_EVENT_POWERON];

	return atomic_read(&p->in_use);
}

/*
 * *******************************************************************
 *  ipem related
 * *******************************************************************
 */
/*static struct timer_list s_init_timer; */

/*void gpad_ipem_init_delayed_(unsigned long data); */

int gpad_ipem_init(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	struct mtk_gpufreq_kmif kmif_ops = {
		.notify_thermal_event   = gpad_ipem_callback_thermal, .notify_touch_event     = gpad_ipem_callback_touch, .get_loading_stat       = gpad_ipem_met_loading_, .get_block_stat         = gpad_ipem_met_block_, .get_idle_stat          = gpad_ipem_met_idle_, .get_power_loading_stat = gpad_ipem_met_power_loading_
	};
	USE_IPEM_LOCK;

	GPAD_LOG_INFO("[IPEM] %s: gpad_ipem_init...\n", __func__);

	spin_lock_init(&ipem_info->lock);

	LOCK_IPEM();
	ipem_info->enable = 0;
	/*ipem_info->event = GPAD_IPEM_EVENT_INVALID; */
	UNLOCK_IPEM();

	gpad_ipem_initialize_proc_();
	gpad_ipem_queue_init(dev);
	gpad_ipem_timer_init(dev);
	gpad_ipem_table_init(dev);
	gpad_ipem_tracker_init(dev);
	gpad_ipem_scoreboard_init(dev);
	gpad_ipem_dvfs_init(dev);
	gpad_ipem_dpm_init(dev);
	gpad_ipem_recorder_init(dev);

	gpad_ipem_enable(dev);
	gs_gpufreq_kmif_register(&kmif_ops);

	GPAD_LOG_INFO("[IPEM] %s: gpad_ipem_init...done\n", __func__);

	return 0;
}

/*
   void gpad_ipem_init_delayed_(unsigned long data)
   {
   struct mtk_gpufreq_kmif kmif_ops = {
   .notify_thermal_event   = gpad_ipem_callback_thermal,    .notify_touch_event     = gpad_ipem_callback_touch,    .get_loading_stat       = gpad_ipem_met_loading_,    .get_block_stat         = gpad_ipem_met_block_,    .get_idle_stat          = gpad_ipem_met_idle_,    .get_power_loading_stat = gpad_ipem_met_power_loading_
   };
   struct gpad_device *dev = gpad_get_default_dev();

	gpad_ipem_table_init(dev);
	gpad_ipem_enable(dev, 0 * HZ);
gpad_ipem_enable(dev);

gs_gpufreq_kmif_register(&kmif_ops);

GPAD_LOG_INFO("[IPEM] %s: gpad_ipem_delay_init...done\n", __FUNCTION__);
}
*/

int gpad_ipem_exit(struct gpad_device *dev)
{
	GPAD_LOG_INFO("[IPEM] %s: gpad_ipem_exit...\n", __func__);

	/* FIXME: check the exit process and its sequence */
	/* exit all the sources of works called from */
	gs_gpufreq_kmif_unregister();
	gpad_ipem_timer_exit(dev);

	/* exit all the sub-system */
	gpad_ipem_queue_exit(dev);
	gpad_ipem_table_exit(dev);
	gpad_ipem_tracker_exit(dev);
	gpad_ipem_scoreboard_exit(dev);
	gpad_ipem_dvfs_exit(dev);
	gpad_ipem_dpm_exit(dev);
	gpad_ipem_recorder_exit(dev);
	gs_gpufreq_unregister();

	GPAD_LOG_INFO("[IPEM] %s: gpad_ipem_exit...done\n", __func__);

	return 0;
}

/*int gpad_ipem_enable(struct gpad_device *dev, u32 delay) */
int gpad_ipem_enable(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);
	struct gpad_ipem_timer_info *timer_info = GET_TIMER_INFO();
	unsigned long expires;
	USE_IPEM_LOCK;
	USE_TIMER_LOCK;

	LOCK_IPEM();
	if (ipem_info->enable == 0) {
		ipem_info->enable = 1;

		UNLOCK_IPEM();

		/* enable dvfs mode in gpufreq */
		gs_gpufreq_dvfs_enable();

		LOCK_TIMER();
		expires = jiffies + timer_info->timer_interval;
		UNLOCK_TIMER();

		mod_timer(&(timer_info->timer), expires);

	} else {
		UNLOCK_IPEM();
	}

	GPAD_LOG_INFO("[IPEM] %s: ipem is enabled\n", __func__);

	return 0;
}

int gpad_ipem_disable(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);
	struct gpad_ipem_timer_info *timer_info = GET_TIMER_INFO();
	USE_IPEM_LOCK;

	LOCK_IPEM();
	if (ipem_info->enable == 1) {
		ipem_info->enable = 0;
		UNLOCK_IPEM();

		del_timer(&(timer_info->timer));

		/* disable dvfs mode in gpufreq */
		gs_gpufreq_dvfs_disable();
	} else {
		UNLOCK_IPEM();
	}

	GPAD_LOG_INFO("[IPEM] %s: ipem is disabled\n", __func__);

	return 0;
}

unsigned int gpad_ipem_enable_get(struct gpad_device *dev)
{
	struct gpad_ipem_info *ipem_info = GPAD_IPEM_INFO_GET(dev);

	return ipem_info->enable;
}


/*
 * *******************************************************************
 *  ipem proc struct procs
 * *******************************************************************
 */

/* debug */
static ssize_t proc_debug_write_(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	s32 debug = 0;
	char proc_buffer[20] = {0};
	int i;

	if (count > (sizeof(proc_buffer) - 1)) {
		GPAD_LOG_ERR("[IPEM][PROC][ERROR] debug write : input too long\n");
		return -EINVAL;
	}

	for (i = 0; i < count; ++i) {
		proc_buffer[i] = buffer[i];
	}
	proc_buffer[i] = 0;

	if (kstrtoint(proc_buffer, 10, &debug) == 0) {
		if (debug) {
			g_debug = 1;
			return count;
		} else {
			g_debug = 0;
			return count;
		}
	}

	GPAD_LOG_ERR("[IPEM][PROC][ERROR] debug write : should be 0 or 1\n");

	return count;
}

static int proc_debug_show_(struct seq_file *s, void *v)
{
	if (g_debug)
		seq_printf(s, "ipem debug enabled\n");
	else
		seq_printf(s, "ipem debug disabled\n");

	return 0;
}

static int proc_debug_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_debug_show_, NULL);
}



/* called */
static ssize_t proc_called_write_(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	return count;
}

static int proc_called_show_(struct seq_file *s, void *v)
{
	seq_printf(s, "CALLED / MAX CALLED\nTIMER THERMAL TOUCH POWERLOW POWERHIGH POWEROFF POWERON\n");

	seq_printf(s, "%6d %6d %6d %6d %6d %6d %6d\n", g_called[0], g_called[1], g_called[2], g_called[3], g_called[4], g_called[5], g_called[6]
			);

	seq_printf(s, "%6d %6d %6d %6d %6d %6d %6d\n", g_recursive_called_max[0], g_recursive_called_max[1], g_recursive_called_max[2], g_recursive_called_max[3], g_recursive_called_max[4], g_recursive_called_max[5], g_recursive_called_max[6]
			);
	return 0;
}

static int proc_called_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_called_show_, NULL);
}













/*
 * *******************************************************************
 *  ipem proc structs
 * *******************************************************************
 */
static const struct file_operations debug_fops = {
	.owner      = THIS_MODULE, .write      = proc_debug_write_, .open       = proc_debug_open_, .read       = seq_read, .llseek     = seq_lseek, .release    = single_release, };

static const struct file_operations called_fops = {
	.owner      = THIS_MODULE, .write      = proc_called_write_, .open       = proc_called_open_, .read       = seq_read, .llseek     = seq_lseek, .release    = single_release, };

/*
 * *******************************************************************
 *  ipem procs
 * *******************************************************************
 */


static s32 gpad_ipem_initialize_proc_(void)
{
#if GPAD_IPEM_DEBUG_PROC
	struct proc_dir_entry *dir   = (void *)NULL;
	struct proc_dir_entry *entry = (void *)NULL;

	GPAD_LOG_INFO("[IPEM][PROC] %s\n", __func__);

	dir = proc_mkdir("ipem", NULL);
	if (!dir) {
		GPAD_LOG_ERR("[IPEM][PROC] Error proc_mkdir(ipem)\n");
		return -1;
	}

	g_proc_dir = dir;

	/* debug enabled : RW [0,1] */
	entry = proc_create("debug", S_IRUGO | S_IWUSR | S_IWGRP, dir, &debug_fops);
	if (!entry) {
		GPAD_LOG_ERR("[%s]: mkdir /proc/ipem/debug failed\n", __func__);
	}

	/* debug enabled : RW [0,1] */
	entry = proc_create("called", S_IRUGO | S_IWUSR | S_IWGRP, dir, &called_fops);
	if (!entry) {
		GPAD_LOG_ERR("[%s]: mkdir /proc/ipem/called failed\n", __func__);
	}
#endif

	return 0;
}

