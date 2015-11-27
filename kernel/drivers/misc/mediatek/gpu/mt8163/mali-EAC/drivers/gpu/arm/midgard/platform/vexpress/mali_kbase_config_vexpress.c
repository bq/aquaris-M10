/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */


#include <linux/ioport.h>

#include <linux/workqueue.h>
#include <linux/proc_fs.h>

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include <mali_kbase_pm.h>
#include "mali_kbase_cpu_vexpress.h"

#include <mach/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mali_kbase_pm.h>

/* MTK clock modified */
#include "mach/mt_clkmgr.h"
#include "mach/mt_gpufreq.h"


/* Versatile Express (VE) configuration defaults shared between config_attributes[]
 * and config_attributes_hw_issue_8408[]. Settings are not shared for
 * JS_HARD_STOP_TICKS_SS and JS_RESET_TICKS_SS.
 */
#define KBASE_VE_JS_SCHEDULING_TICK_NS_DEBUG	15000000u	   /* 15ms, an agressive tick for testing purposes. This will reduce performance significantly */
#define KBASE_VE_JS_SOFT_STOP_TICKS_DEBUG		1	/* between 15ms and 30ms before soft-stop a job */
#define KBASE_VE_JS_SOFT_STOP_TICKS_CL_DEBUG	1	/* between 15ms and 30ms before soft-stop a CL job */
#define KBASE_VE_JS_HARD_STOP_TICKS_SS_DEBUG	333 /* 5s before hard-stop */
#define KBASE_VE_JS_HARD_STOP_TICKS_SS_8401_DEBUG 2000	/* 30s before hard-stop, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) - for issue 8401 */
#define KBASE_VE_JS_HARD_STOP_TICKS_CL_DEBUG	166 /* 2.5s before hard-stop */
#define KBASE_VE_JS_HARD_STOP_TICKS_NSS_DEBUG	100000	/* 1500s (25mins) before NSS hard-stop */
#define KBASE_VE_JS_RESET_TICKS_SS_DEBUG		500 /* 45s before resetting GPU, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) */
#define KBASE_VE_JS_RESET_TICKS_SS_8401_DEBUG	3000	/* 7.5s before resetting GPU - for issue 8401 */
#define KBASE_VE_JS_RESET_TICKS_CL_DEBUG		500 /* 45s before resetting GPU */
#define KBASE_VE_JS_RESET_TICKS_NSS_DEBUG		100166	/* 1502s before resetting GPU */

#define KBASE_VE_JS_SCHEDULING_TICK_NS			1250000000u /* 1.25s */
#define KBASE_VE_JS_SOFT_STOP_TICKS 			2	/* 2.5s before soft-stop a job */
#define KBASE_VE_JS_SOFT_STOP_TICKS_CL			1	/* 1.25s before soft-stop a CL job */
#define KBASE_VE_JS_HARD_STOP_TICKS_SS			4	/* 5s before hard-stop */
#define KBASE_VE_JS_HARD_STOP_TICKS_SS_8401 	24	/* 30s before hard-stop, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) - for issue 8401 */
#define KBASE_VE_JS_HARD_STOP_TICKS_CL			2	/* 2.5s before hard-stop */
#define KBASE_VE_JS_HARD_STOP_TICKS_NSS 		1200	/* 1500s before NSS hard-stop */
#define KBASE_VE_JS_RESET_TICKS_SS				6	/* 7.5s before resetting GPU */
#define KBASE_VE_JS_RESET_TICKS_SS_8401 		36	/* 45s before resetting GPU, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) - for issue 8401 */
#define KBASE_VE_JS_RESET_TICKS_CL				3	/* 7.5s before resetting GPU */
#define KBASE_VE_JS_RESET_TICKS_NSS 			1201	/* 1502s before resetting GPU */

#define KBASE_VE_JS_RESET_TIMEOUT_MS			3000	/* 3s before cancelling stuck jobs */
#define KBASE_VE_JS_CTX_TIMESLICE_NS			1000000 /* 1ms - an agressive timeslice for testing purposes (causes lots of scheduling out for >4 ctxs) */
#define KBASE_VE_POWER_MANAGEMENT_CALLBACKS 	((uintptr_t)&pm_callbacks)
#define KBASE_VE_CPU_SPEED_FUNC 				((uintptr_t)&kbase_get_vexpress_cpu_clock_speed)
#define KBASE_VE_PLATFORM_FUNCS 				((uintptr_t)&platform_func)

#define HARD_RESET_AT_POWER_OFF 1

#ifndef CONFIG_OF
static kbase_io_resources io_resources = {
	.job_irq_number = 68,
	.mmu_irq_number = 69,
	.gpu_irq_number = 70,
	.io_memory_region = {
	.start = 0xFC010000,
	.end = 0xFC010000 + (4096 * 4) - 1
	}
};
#endif /* CONFIG_OF */

/**
 * DVFS implementation 
 */
typedef enum{
	MTK_DVFS_STATUS_NORMAL = 0,
	MTK_DVFS_STATUS_BOOST,
	MTK_DVFS_STATUS_DISABLE
}  mtk_dvfs_status ;

static const char * status_str[] = {
	"Normal", 
	"Boosting", 
	"Disabled"
};

#define STATUS_STR(s) (status_str[(s)])

//mtk dvfs action, externed mali actions
typedef enum {
	MTK_DVFS_NOP 		= KBASE_PM_DVFS_NOP,	    /**< No change in clock frequency is requested */
	MTK_DVFS_CLOCK_UP 	= KBASE_PM_DVFS_CLOCK_UP,	    /**< The clock frequency should be increased if possible */
	MTK_DVFS_CLOCK_DOWN = KBASE_PM_DVFS_CLOCK_DOWN,    /**< The clock frequency should be decreased if possible */
	MTK_DVFS_BOOST,    
	MTK_DVFS_POWER_ON,
	MTK_DVFS_POWER_OFF,
	MTK_DVFS_ENABLE,        
	MTK_DVFS_DISABLE        
} mtk_dvfs_action ;
static const char * action_str[] = {
	"Nop", 
	"Up", 
	"Down",
	"Boost",
	"PowerOn",
	"PowerOff",
	"Enable",
	"Disable"
};
#define ACTION_STR(a) (action_str[(a)])

struct mtk_dvfs_info {
	struct work_struct work;

	int min_freq_idx;
	int max_freq_idx;

	int cur_freq_idx;  /* index before powered off */


	
	//struct kbase_device * kbdev;
	int boosted;
	mtk_dvfs_status status;
	//enum kbase_pm_dvfs_action action;
	int power_on;	
	
	struct mutex mlock; /* dvfs lock*/

	
	mtk_dvfs_action last_action; /*debug */
	
	enum kbase_pm_dvfs_action mali_action; /* pending mali action from metrics */
	spinlock_t lock; /* mali action lock */


} ;
static struct mtk_dvfs_info mtk_dvfs;


#define TOP_IDX(info) ((info)->min_freq_idx)
#define BOTTOM_IDX(info) ((info)->max_freq_idx)
#define LOWER_IDX(info, idx) ((idx) >= (info)->max_freq_idx)?(idx):((idx) + 1)
#define UPPER_IDX(info, idx) ((idx) <= (info)->min_freq_idx)?(idx):((idx) - 1)

#define IS_UPPER(idx1, idx2) ((idx1) < (idx2))


static void mtk_mali_create_proc(struct kbase_device *kbdev);


static void mtk_process_dvfs(struct mtk_dvfs_info * info, mtk_dvfs_action action) {
	unsigned int request_freq_idx;
	unsigned int cur_freq_idx;
    mtk_dvfs_status last_status;
	
	mutex_lock(&info->mlock);

	request_freq_idx = cur_freq_idx = mt_gpufreq_get_cur_freq_index();

	last_status = info->status;

	switch(last_status) {
		case MTK_DVFS_STATUS_DISABLE:
			if(action == MTK_DVFS_ENABLE) {
				info->status = MTK_DVFS_STATUS_NORMAL;
			}
			/*ignore any actions*/
			request_freq_idx = TOP_IDX(info);
			break;
		case MTK_DVFS_STATUS_BOOST:
			if(action == MTK_DVFS_DISABLE) {
				info->status = MTK_DVFS_STATUS_DISABLE;
				info->boosted = 0;
				request_freq_idx = TOP_IDX(info);
				
			} else if(action == MTK_DVFS_POWER_OFF){
				//status not changed..
				//info->status = MTK_DVFS_STATUS_POWER_OFF;
				request_freq_idx = BOTTOM_IDX(info);		
				
		    } else if(action  == MTK_DVFS_POWER_ON) {
				request_freq_idx = TOP_IDX(info);
				
			} else {
				//should be up/down also nop.
				info->boosted ++;
				if(info->boosted >= 4) {					
					info->status = MTK_DVFS_STATUS_NORMAL;					
				}
				request_freq_idx = TOP_IDX(info);
			}

			break;
		case MTK_DVFS_STATUS_NORMAL:
			if(action == MTK_DVFS_CLOCK_UP) {
				request_freq_idx = UPPER_IDX(info, cur_freq_idx);
				
			} else if(action == MTK_DVFS_CLOCK_DOWN) {
				request_freq_idx = LOWER_IDX(info, cur_freq_idx);
				
			} else if(action  == MTK_DVFS_POWER_OFF) {
				info->cur_freq_idx = cur_freq_idx;
				request_freq_idx = BOTTOM_IDX(info);
				
			} else if(action  == MTK_DVFS_POWER_ON) {
				request_freq_idx = info->cur_freq_idx;
				
			} else if(action == MTK_DVFS_BOOST) {
				info->status = MTK_DVFS_STATUS_BOOST;
				info->boosted = 0;
				request_freq_idx = TOP_IDX(info);
				
			} else if(action == MTK_DVFS_DISABLE) {
				info->status = MTK_DVFS_STATUS_DISABLE;
				request_freq_idx = TOP_IDX(info);
				
			}
			break;
		default:
			break;
	}

	if(action != MTK_DVFS_NOP) {
			//log status change
		pr_warn("GPU action: %s, status: %s -> %s, idx: %d -> %d\n", 
			ACTION_STR(action), 
			STATUS_STR(last_status), 
			STATUS_STR(info->status), 
			cur_freq_idx, 
			request_freq_idx);
	}


	if(cur_freq_idx != request_freq_idx){	
		pr_warn("Try set gpu freq %d , cur idx is %d ", request_freq_idx, cur_freq_idx);
		//may fail for thermal protecting 
		mt_gpufreq_target(request_freq_idx);		
		cur_freq_idx = mt_gpufreq_get_cur_freq_index();		
		pr_warn("After set gpu freq, cur idx is %d ", cur_freq_idx);
	}

	//common part :
	if(action == MTK_DVFS_POWER_ON) {
		info->power_on = 1;
	} else if(action == MTK_DVFS_POWER_OFF) {
		info->power_on = 0;	
	}
	info->last_action = action;
	
	mutex_unlock(&info->mlock);	
}


static void mtk_process_mali_action(struct work_struct *work) {
	struct mtk_dvfs_info * info = container_of(work, struct mtk_dvfs_info, work);

	unsigned long flags;
	mtk_dvfs_action action;

	spin_lock_irqsave(&info->lock, flags);
	action = (mtk_dvfs_action)info->mali_action;	
	spin_unlock_irqrestore(&info->lock, flags);
	
	mtk_process_dvfs(info, action);
}

static void input_boost_notify(unsigned int idx ) {
	/*Idx is assumed to be TOP */
	mtk_process_dvfs(&mtk_dvfs, MTK_DVFS_BOOST);
}

static void power_limit_notify(unsigned int idx) {
	unsigned int cur_idx;
	mutex_lock(&mtk_dvfs.mlock);	
	cur_idx = mt_gpufreq_get_cur_freq_index();
	if(IS_UPPER(cur_idx, idx)) {		
		mt_gpufreq_target(idx);
	}
	mutex_unlock(&mtk_dvfs.mlock);
}

void kbase_platform_dvfs_action(struct kbase_device *kbdev, enum kbase_pm_dvfs_action action) {
	unsigned long flags;
	struct mtk_dvfs_info * info = &mtk_dvfs;

	
	spin_lock_irqsave(&info->lock, flags);
	
	info->mali_action = action;
	schedule_work(&info->work);
#if 0
	
#endif	
	spin_unlock_irqrestore(&info->lock, flags);
}


//For MTK MET,
//Fixme: Access without pm metrics lock, so it is better to be atomic...
static u32 snap_loading = 0;
static u32 snap_cl0 = 0;
static u32 snap_cl1 = 0;

//Implement this to connect MIDGARD dvfs
int kbase_platform_dvfs_event(
	struct kbase_device *kbdev, 
	u32 utilisation,
	u32 util_gl_share, 
	u32 util_cl_share[2]) {

	snap_loading = utilisation;
	snap_cl0 = util_cl_share[0];
	snap_cl1 = util_cl_share[1];	

	return 0;
}

static u32 mtk_met_get_mali_loading(void)
{
	u32 loading = snap_loading;
	return loading;
}

KBASE_EXPORT_SYMBOL(mtk_met_get_mali_loading);
extern unsigned int (*mtk_get_gpu_loading_fp)(void);

static mali_bool mtk_platform_init(struct kbase_device *kbdev) {	
	mtk_dvfs.cur_freq_idx = mt_gpufreq_get_cur_freq_index();
	mtk_dvfs.max_freq_idx = mt_gpufreq_get_dvfs_table_num() - 1;
	mtk_dvfs.min_freq_idx = 0;
	mtk_dvfs.status = MTK_DVFS_STATUS_NORMAL;
	mtk_dvfs.power_on = 0;
	mtk_dvfs.last_action = MTK_DVFS_NOP;
	mutex_init(&mtk_dvfs.mlock);
	
	mtk_dvfs.mali_action = KBASE_PM_DVFS_NOP;
	spin_lock_init(&mtk_dvfs.lock);

	INIT_WORK(&mtk_dvfs.work, mtk_process_mali_action);

	mt_gpufreq_input_boost_notify_registerCB(input_boost_notify);
	mt_gpufreq_power_limit_notify_registerCB(power_limit_notify);

	/* MTK MET */
	mtk_get_gpu_loading_fp = mtk_met_get_mali_loading;		

	/* Memory report ... */
	/* mtk_get_gpu_memory_usage_fp = kbase_report_gpu_memory_usage; */
#ifdef CONFIG_PROC_FS
	mtk_mali_create_proc(kbdev);
#endif
	return MALI_TRUE;
}
static void mtk_platform_term(struct kbase_device *kbdev) {
	mt_gpufreq_power_limit_notify_registerCB(NULL);
	mt_gpufreq_input_boost_notify_registerCB(NULL);
}
static struct kbase_platform_funcs_conf platform_func = {
	.platform_init_func = mtk_platform_init,
	.platform_term_func = mtk_platform_term
};

//clock :
#if 0
//for mtk gpufreq update
static void set_gpu_freq_power_on(struct kbase_device *kbdev) {

	mutex_lock(&mtk_dvfs.mlock);
	
	mtk_set_gpu_freq_index(&mtk_dvfs, mtk_dvfs.request_freq_idx);

	mtk_dvfs.power_on = 1;	 
	
	mutex_unlock(&mtk_dvfs.mlock);
}
//for mtk gpufreq update
static void set_gpu_freq_power_off(struct kbase_device *kbdev) {
	int cur_idx;
	
	mutex_lock(&mtk_dvfs.mlock);
	
	mtk_set_gpu_freq_index(&mtk_dvfs, BOTTOM_IDX(&mtk_dvfs));

	mtk_dvfs.power_on = 0;	 
	
	mutex_unlock(&mtk_dvfs.mlock);
}

#endif
static int pm_callback_power_on(struct kbase_device *kbdev)
{
	mt_gpufreq_voltage_enable_set(1);
	
	enable_clock( MT_CG_DISP0_SMI_COMMON, "GPU");
	enable_clock( MT_CG_MFG_BG3D, "GPU");

	mtk_process_dvfs(&mtk_dvfs, MTK_DVFS_POWER_ON);
	/* Nothing is needed on VExpress, but we may have destroyed GPU state (if the below HARD_RESET code is active) */
	return 1;
}

#define DELAY_LOOP_COUNT	100000
#define MFG_DEBUG_SEL		 0x3
#define MFG_BUS_IDLE_BIT	(1 << 2)
							
#define MFG_DEBUG_CTRL_REG	(clk_mfgcfg_base + 0x180)
#define MFG_DEBUG_STAT_REG	(clk_mfgcfg_base + 0x184)

#define MFG_WRITE32(value, addr) writel(value, addr)
#define MFG_READ32(addr)		 readl(addr)

static void wait_gpu_hw_stop(void) {
	volatile int polling_count = 100000;

	/// 2. Polling the MFG_DEBUG_REG for checking GPU IDLE before MTCMOS power off (0.1ms)
	MFG_WRITE32(0x3, MFG_DEBUG_CTRL_REG);
	do {
		/// 0x13000184[2]
		/// 1'b1: bus idle
		/// 1'b0: bus busy
		if (MFG_READ32(MFG_DEBUG_STAT_REG) & MFG_BUS_IDLE_BIT)
		{
			//OK,
			return;
		}
	} while (polling_count --);

	if (polling_count <=0)
	{
		printk("[MALI]!!!!MFG(GPU) subsys is still BUSY!!!!!, polling_count=%d\n", polling_count);
	}
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	/// 1. Delay 0.01ms before power off	
	volatile int i;
	for (i = 0; i < 100000; i++);

	wait_gpu_hw_stop();
#if HARD_RESET_AT_POWER_OFF
	/* Cause a GPU hard reset to test whether we have actually idled the GPU
	 * and that we properly reconfigure the GPU on power up.
	 * Usually this would be dangerous, but if the GPU is working correctly it should
	 * be completely safe as the GPU should not be active at this point.
	 * However this is disabled normally because it will most likely interfere with
	 * bus logging etc.
	 */
	//KBASE_TRACE_ADD(kbdev, CORE_GPU_HARD_RESET, NULL, NULL, 0u, 0);
	kbase_os_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND), GPU_COMMAND_HARD_RESET);
#endif

	wait_gpu_hw_stop();

	mtk_process_dvfs(&mtk_dvfs, MTK_DVFS_POWER_OFF);

	disable_clock( MT_CG_MFG_BG3D, "GPU");
	disable_clock( MT_CG_DISP0_SMI_COMMON, "GPU");

	mt_gpufreq_voltage_enable_set(0);

}

static struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = NULL,
	.power_resume_callback = NULL
};


/* Please keep table config_attributes in sync with config_attributes_hw_issue_8408 */
static struct kbase_attribute config_attributes[] = {
#ifdef CONFIG_MALI_DEBUG
/* Use more aggressive scheduling timeouts in debug builds for testing purposes */
	{KBASE_CONFIG_ATTR_JS_SCHEDULING_TICK_NS, KBASE_VE_JS_SCHEDULING_TICK_NS_DEBUG},

	{KBASE_CONFIG_ATTR_JS_SOFT_STOP_TICKS, KBASE_VE_JS_SOFT_STOP_TICKS_DEBUG},

	{KBASE_CONFIG_ATTR_JS_SOFT_STOP_TICKS_CL, KBASE_VE_JS_SOFT_STOP_TICKS_CL_DEBUG},

	{KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_SS, KBASE_VE_JS_HARD_STOP_TICKS_SS_DEBUG},

	{KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_CL, KBASE_VE_JS_HARD_STOP_TICKS_CL_DEBUG},

	{KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_NSS, KBASE_VE_JS_HARD_STOP_TICKS_NSS_DEBUG},

	{KBASE_CONFIG_ATTR_JS_RESET_TICKS_SS, KBASE_VE_JS_RESET_TICKS_SS_DEBUG},

	{KBASE_CONFIG_ATTR_JS_RESET_TICKS_CL, KBASE_VE_JS_RESET_TICKS_CL_DEBUG},

	{KBASE_CONFIG_ATTR_JS_RESET_TICKS_NSS, KBASE_VE_JS_RESET_TICKS_NSS_DEBUG},
	
#else				/* CONFIG_MALI_DEBUG */
/* In release builds same as the defaults but scaled for 5MHz FPGA */
	{KBASE_CONFIG_ATTR_JS_SCHEDULING_TICK_NS, KBASE_VE_JS_SCHEDULING_TICK_NS},

	{KBASE_CONFIG_ATTR_JS_SOFT_STOP_TICKS, KBASE_VE_JS_SOFT_STOP_TICKS},

	{KBASE_CONFIG_ATTR_JS_SOFT_STOP_TICKS_CL, KBASE_VE_JS_SOFT_STOP_TICKS_CL},

	{KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_SS, KBASE_VE_JS_HARD_STOP_TICKS_SS},

	{KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_CL, KBASE_VE_JS_HARD_STOP_TICKS_CL},

	{KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_NSS, KBASE_VE_JS_HARD_STOP_TICKS_NSS},

	{KBASE_CONFIG_ATTR_JS_RESET_TICKS_SS, KBASE_VE_JS_RESET_TICKS_SS},

	{KBASE_CONFIG_ATTR_JS_RESET_TICKS_CL, KBASE_VE_JS_RESET_TICKS_CL},

	{KBASE_CONFIG_ATTR_JS_RESET_TICKS_NSS, KBASE_VE_JS_RESET_TICKS_NSS},
#endif				/* CONFIG_MALI_DEBUG */

	{KBASE_CONFIG_ATTR_JS_RESET_TIMEOUT_MS, KBASE_VE_JS_RESET_TIMEOUT_MS},

	{KBASE_CONFIG_ATTR_JS_CTX_TIMESLICE_NS, KBASE_VE_JS_CTX_TIMESLICE_NS},

	{KBASE_CONFIG_ATTR_POWER_MANAGEMENT_CALLBACKS, KBASE_VE_POWER_MANAGEMENT_CALLBACKS},

	{KBASE_CONFIG_ATTR_PLATFORM_FUNCS, KBASE_VE_PLATFORM_FUNCS},

	{KBASE_CONFIG_ATTR_CPU_SPEED_FUNC, KBASE_VE_CPU_SPEED_FUNC},

	{KBASE_CONFIG_ATTR_POWER_MANAGEMENT_DVFS_FREQ, 200},

	{KBASE_CONFIG_ATTR_END, 0}
};

/* as config_attributes array above except with different settings for
 * JS_HARD_STOP_TICKS_SS, JS_RESET_TICKS_SS that
 * are needed for BASE_HW_ISSUE_8408.
 */
struct kbase_attribute config_attributes_hw_issue_8408[] = {
#ifdef CONFIG_MALI_DEBUG
/* Use more aggressive scheduling timeouts in debug builds for testing purposes */
	{KBASE_CONFIG_ATTR_JS_SCHEDULING_TICK_NS, KBASE_VE_JS_SCHEDULING_TICK_NS_DEBUG},

	{KBASE_CONFIG_ATTR_JS_SOFT_STOP_TICKS, KBASE_VE_JS_SOFT_STOP_TICKS_DEBUG},

	{KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_SS, KBASE_VE_JS_HARD_STOP_TICKS_SS_8401_DEBUG},

	{KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_NSS, KBASE_VE_JS_HARD_STOP_TICKS_NSS_DEBUG},

	{KBASE_CONFIG_ATTR_JS_RESET_TICKS_SS, KBASE_VE_JS_RESET_TICKS_SS_8401_DEBUG},

	{KBASE_CONFIG_ATTR_JS_RESET_TICKS_NSS, KBASE_VE_JS_RESET_TICKS_NSS_DEBUG},
#else				/* CONFIG_MALI_DEBUG */
/* In release builds same as the defaults but scaled for 5MHz FPGA */
	{KBASE_CONFIG_ATTR_JS_SCHEDULING_TICK_NS, KBASE_VE_JS_SCHEDULING_TICK_NS},

	{KBASE_CONFIG_ATTR_JS_SOFT_STOP_TICKS, KBASE_VE_JS_SOFT_STOP_TICKS},

	{KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_SS, KBASE_VE_JS_HARD_STOP_TICKS_SS_8401},

	{KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_NSS, KBASE_VE_JS_HARD_STOP_TICKS_NSS},

	{KBASE_CONFIG_ATTR_JS_RESET_TICKS_SS, KBASE_VE_JS_RESET_TICKS_SS_8401},

	{KBASE_CONFIG_ATTR_JS_RESET_TICKS_NSS, KBASE_VE_JS_RESET_TICKS_NSS},
#endif				/* CONFIG_MALI_DEBUG */
	{KBASE_CONFIG_ATTR_JS_RESET_TIMEOUT_MS, KBASE_VE_JS_RESET_TIMEOUT_MS},

	{KBASE_CONFIG_ATTR_JS_CTX_TIMESLICE_NS, KBASE_VE_JS_CTX_TIMESLICE_NS},

	{KBASE_CONFIG_ATTR_POWER_MANAGEMENT_CALLBACKS, KBASE_VE_POWER_MANAGEMENT_CALLBACKS},

	{KBASE_CONFIG_ATTR_PLATFORM_FUNCS, KBASE_VE_PLATFORM_FUNCS},

	{KBASE_CONFIG_ATTR_CPU_SPEED_FUNC, KBASE_VE_CPU_SPEED_FUNC},

	{KBASE_CONFIG_ATTR_POWER_MANAGEMENT_DVFS_FREQ, 200},

	{KBASE_CONFIG_ATTR_END, 0}
};

static struct kbase_platform_config versatile_platform_config = {
	.attributes = config_attributes,
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &versatile_platform_config;
}

int kbase_platform_early_init(void)
{
	/* Nothing needed at this stage */
	return 0;
}


#ifdef CONFIG_PROC_FS
static int utilization_seq_show(struct seq_file *m, void *v)
{
	unsigned int freq = mt_gpufreq_get_cur_freq();
	if(snap_loading > 0)
		seq_printf(m, "gpu/cljs0/cljs1=%u/%u/%u [Freq : %d]\n",
				snap_loading, snap_cl0, snap_cl1, freq);
	else
		seq_printf(m, "gpu=%u [Freq : %d]\n", 0, freq);
	return 0;
}

static int utilization_seq_open(struct inode *in, struct file *file)
{
//	struct kbase_device *kbdev = PDE_DATA(file_inode(file));
	return single_open(file, utilization_seq_show , NULL);
}

static const struct file_operations utilization_proc_fops = {
	.open	 = utilization_seq_open,
	.read	 = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int dvfs_seq_show(struct seq_file *m, void *v)
{
	mutex_lock(&mtk_dvfs.mlock);
	seq_printf(m, " Status : %s \n Last action : %s\n Index : %d [%d-%d] \n Power : %d \n Boosted count : %d\n echo 0 can disable\n",
		STATUS_STR(mtk_dvfs.status),
		ACTION_STR(mtk_dvfs.last_action),
		mtk_dvfs.cur_freq_idx,
		mtk_dvfs.min_freq_idx,
		mtk_dvfs.max_freq_idx,
		mtk_dvfs.power_on,
		mtk_dvfs.boosted);

	mutex_unlock(&mtk_dvfs.mlock);
	

	return 0;
}

static int dvfs_seq_open(struct inode *in, struct file *file)
{
	return single_open(file, dvfs_seq_show , NULL);
}

static ssize_t dvfs_seq_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len)) {
		return 0;
	}
	desc[len] = '\0';

	if(!strncmp(desc, "1", 1)) {
		mtk_process_dvfs(&mtk_dvfs, MTK_DVFS_ENABLE);
	}
	else if(!strncmp(desc, "0", 1)) {
		mtk_process_dvfs(&mtk_dvfs, MTK_DVFS_DISABLE);
	}
	return count;
}

static const struct file_operations dvfs_proc_fops = {
		.open	 = dvfs_seq_open,
		.read	 = seq_read,
		.write	 = dvfs_seq_write,
		.release = single_release,
};


static int memory_usage_seq_show(struct seq_file *m, void *v)
{
	//just use debugfs impl to calculate
	const struct list_head *kbdev_list;
	struct list_head *entry;
	unsigned long usage = 0;
	
	kbdev_list = kbase_dev_list_get();

	list_for_each(entry, kbdev_list) {
		struct kbase_device *kbdev = NULL;
		kbdev = list_entry(entry, struct kbase_device, entry);
		usage += PAGE_SIZE * atomic_read(&(kbdev->memdev.used_pages));
	}	
	kbase_dev_list_put(kbdev_list);

	seq_printf(m, "%ld\n", usage);
	
	return 0;
}
	
static int memory_usage_seq_open(struct inode *in, struct file *file)
{
	return single_open(file, memory_usage_seq_show , NULL);
}


static const struct file_operations memory_usage_proc_fops = {
		.open	 = memory_usage_seq_open,
		.read	 = seq_read,
		.release = single_release,
};


static int dvfs_threshhold_seq_show(struct seq_file *m, void *v)
{
	u32 min_loading;
	u32 max_loading;
	kbase_pm_metrics_get_threshold(&min_loading, &max_loading);
	seq_printf(m, "min:%d, max:%d\n", min_loading, max_loading);
	return 0;
}

static int dvfs_threshhold_seq_open(struct inode *in, struct file *file)
{
	return single_open(file, dvfs_threshhold_seq_show , NULL);
}

static ssize_t dvfs_threshhold_seq_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	u32 min_loading;
	u32 max_loading;
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len)) {
		return 0;
	}
	desc[len] = '\0';
	
	if(sscanf(desc, "%d %d", &min_loading, &max_loading) == 2) {		
		kbase_pm_metrics_set_threshold(min_loading, max_loading);
	}
	return count;
}

static const struct file_operations dvfs_threshhold_proc_fops = {
		.open	 = dvfs_threshhold_seq_open,
		.read	 = seq_read,
		.write	 = dvfs_threshhold_seq_write,
		.release = single_release,
};

static int dvfs_timer_seq_show(struct seq_file *m, void *v)
{
	u32 time;
	struct kbase_device *kbdev = (struct kbase_device *)m->private;
	time = kbase_pm_metrics_get_timer(kbdev);	
	seq_printf(m, "%d\n", time);
	return 0;
}

static int dvfs_timer_seq_open(struct inode *in, struct file *file)
{
	struct kbase_device *kbdev = PDE_DATA(file_inode(file));
	return single_open(file, dvfs_timer_seq_show , kbdev);
}

static ssize_t dvfs_timer_seq_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	u32 time;
	
	struct kbase_device *kbdev = PDE_DATA(file_inode(file));

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len)) {
		return 0;
	}
	desc[len] = '\0';
	
	if(sscanf(desc, "%d", &time) == 1) {		
		kbase_pm_metrics_set_timer(kbdev, time);
	}
	return count;
}

static const struct file_operations dvfs_timer_proc_fops = {
		.open	 = dvfs_timer_seq_open,
		.read	 = seq_read,
		.write	 = dvfs_timer_seq_write,
		.release = single_release,
};


static void mtk_mali_create_proc(struct kbase_device *kbdev) {
	struct proc_dir_entry * mali_pentry = proc_mkdir("mali", NULL);
	if(mali_pentry) {
		printk(KERN_EMERG"create proc fs for  %p", kbdev);
		proc_create_data("utilization", 0, mali_pentry, &utilization_proc_fops, kbdev);
		proc_create_data("dvfs", 0, mali_pentry, &dvfs_proc_fops, kbdev);
		proc_create_data("memory_usage", 0, mali_pentry, &memory_usage_proc_fops, kbdev);
		proc_create_data("dvfs_threshold", 0, mali_pentry, &dvfs_threshhold_proc_fops, kbdev);
		proc_create_data("dvfs_timer", 0, mali_pentry, &dvfs_timer_proc_fops, kbdev);
	}
}
#endif /*CONFIG_PROC_FS*/
