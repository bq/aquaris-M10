/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
/*#include <linux/earlysuspend.h>*/
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/clk.h>

#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/input.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

#include <asm/uaccess.h>
#include <asm/io.h>

/*#include "mach/mt_clkmgr.h"*/
#include "mach/mt_freqhopping.h"
#include "mt_cpufreq.h"
#include "mt_gpufreq.h"
#include "mt-plat/sync_write.h"

#include "mt_static_power.h"
/*Remove this after thermal is ready */
#undef CONFIG_THERMAL
#ifdef CONFIG_THERMAL
#include "mach/mt_thermal.h"
#endif

#if 1
#include "mt-plat/upmu_common.h"
#include "mach/upmu_sw.h"
#include "mach/upmu_hw.h"
#endif
#include "mt-plat/mt_devinfo.h"
/*
 * CONFIG
 */

/**************************************************
 * GPU DVFS input boost feature
 ***************************************************/
/* TODO: */
#define MT_GPUFREQ_INPUT_BOOST

/***************************
 * Define for dynamic power table update
 ****************************/
#define MT_GPUFREQ_DYNAMIC_POWER_TABLE_UPDATE

/***************************
 * Define for SRAM debugging
 ****************************/
#if 0
#ifdef CONFIG_MTK_RAM_CONSOLE
#define MT_GPUFREQ_AEE_RR_REC
#endif
#endif

/**************************************************
 * Define register write function
 ***************************************************/
#define mt_gpufreq_reg_write(val, addr)	   mt_reg_sync_writel((val), ((void *)addr))


/***************************
 * Operate Point Definition
 ****************************/
#define GPUOP(khz, volt)	   \
{							\
	.gpufreq_khz = khz,	\
	.gpufreq_volt = volt,	\
}

/**************************
 * GPU DVFS OPP table setting
 ***************************/
#define GPU_DVFS_FREQT	   (598000)	/* KHz */
#define GPU_DVFS_FREQ0	   (520000)	/* KHz */
#define GPU_DVFS_FREQ1	   (416000)	/* KHz */
#define GPU_DVFS_FREQ2	   (299000)	/* KHz */
#define GPUFREQ_LAST_FREQ_LEVEL    (GPU_DVFS_FREQ2)

#define GPU_DVFS_VOLT0	   (125000)	/* mV x 100 */
#define GPU_DVFS_VOLT1	   (115000)	/* mV x 100 */
#define GPU_DVFS_VOLT2	   (111000)	/* mV x 100 */
#define GPU_DVFS_VOLT3	   (105000)	/* mV x 100 */

/*
 * LOG
 */
#define HEX_FMT "0x%08x"
#undef TAG

#define TAG	"[Power/gpufreq] "

#define gpufreq_err(fmt, args...)		\
	pr_err(TAG"[ERROR]"fmt, ##args)
#define gpufreq_warn(fmt, args...)		\
	pr_warn(TAG"[WARNING]"fmt, ##args)
#define gpufreq_info(fmt, args...)		\
	pr_info(fmt, ##args)
#define gpufreq_dbg(fmt, args...)		\
	do {								\
		if (mt_gpufreq_debug)			\
			pr_debug(fmt, ##args);	 \
	} while (0)
#define gpufreq_ver(fmt, args...)		\
	do {								\
		if (mt_gpufreq_debug)			\
			pr_debug(fmt, ##args);	 \
	} while (0)

static sampler_func g_pFreqSampler;
static sampler_func g_pVoltSampler;

static gpufreq_power_limit_notify g_pGpufreq_power_limit_notify;
#ifdef MT_GPUFREQ_INPUT_BOOST
static gpufreq_input_boost_notify g_pGpufreq_input_boost_notify;
#endif

struct mt_gpufreq_table_info {
	unsigned int gpufreq_khz;
	unsigned int gpufreq_volt;
};

struct mt_gpufreq_power_table_info {
	unsigned int gpufreq_khz;
	unsigned int gpufreq_volt;
	unsigned int gpufreq_power;
};


/**************************
 * enable GPU DVFS count
 ***************************/
static int g_gpufreq_dvfs_disable_count;

static unsigned int g_cur_gpu_OPPidx = 0xFF;

static unsigned int mt_gpufreqs_num;
static struct mt_gpufreq_table_info *mt_gpufreqs;
static struct mt_gpufreq_table_info *mt_gpufreqs_default;

/***************************
 * GPU DVFS OPP Table
 ****************************/
/* Default , */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_0[] = {
	GPUOP(GPU_DVFS_FREQ0, GPU_DVFS_VOLT1),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT1),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT1),
};

/* V/A */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_1[] = {
	GPUOP(GPU_DVFS_FREQT, GPU_DVFS_VOLT1),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT1),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT1),
};

/* V/D */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_2[] = {
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT1),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT1),
};

/* efuse index */
enum GPUFREQ_TABLE_TYPE {
	GPUFREQ_TABLE_TYPE_DEF = 0,
	GPUFREQ_TABLE_TYPE_A,
	GPUFREQ_TABLE_TYPE_B,
	GPUFREQ_TABLE_TYPE_D
};

static struct mt_gpufreq_table_info *gpufreq_table_list[] = {
	mt_gpufreq_opp_tbl_0,
	mt_gpufreq_opp_tbl_1,
	mt_gpufreq_opp_tbl_0,
	mt_gpufreq_opp_tbl_2,
};

static int gpufreq_table_num[] = {
	ARRAY_SIZE(mt_gpufreq_opp_tbl_0),
	ARRAY_SIZE(mt_gpufreq_opp_tbl_1),
	ARRAY_SIZE(mt_gpufreq_opp_tbl_0),
	ARRAY_SIZE(mt_gpufreq_opp_tbl_2)
};

#define GPUFREQ_GET_TABLE(type) gpufreq_table_list[type]
#define GPUFREQ_GET_TABLE_LEN(type) gpufreq_table_num[type]


#define GPU_DEFAULT_MAX_FREQ_MHZ GPU_DVFS_FREQ0


static bool mt_gpufreq_ready;

/* In default settiing, freq_table[0] is max frequency, freq_table[num-1] is min frequency,*/
static const unsigned int g_gpufreq_max_id;

static int g_last_gpu_dvs_result = 0x7F;

/* If not limited, it should be set to freq_table[0] (MAX frequency) */
static unsigned int g_limited_max_id;
static unsigned int g_limited_min_id;

static bool mt_gpufreq_debug;
static bool mt_gpufreq_pause;
static bool mt_gpufreq_keep_max_frequency_state;
static bool mt_gpufreq_keep_opp_frequency_state;
static unsigned int mt_gpufreq_keep_opp_frequency;
static unsigned int mt_gpufreq_keep_opp_index;
static bool mt_gpufreq_fixed_freq_state;
static unsigned int mt_gpufreq_fixed_frequency;

static unsigned int mt_gpufreq_volt_enable_state;
#ifdef MT_GPUFREQ_INPUT_BOOST
static unsigned int mt_gpufreq_input_boost_state = 1;
#endif


/********************************************
 *		 POWER LIMIT RELATED
 ********************************************/
enum {
	IDX_THERMAL_LIMITED,
	IDX_LOW_BATT_VOLT_LIMITED,
	IDX_LOW_BATT_VOLUME_LIMITED,
	IDX_OC_LIMITED,

	NR_IDX_POWER_LIMITED,
};


/* static bool g_limited_power_ignore_state = false; */
static bool g_limited_thermal_ignore_state;
static unsigned int mt_gpufreq_thermal_limited_gpu_power;	/* thermal limit power */

static unsigned int mt_gpufreq_power_limited_index_array[NR_IDX_POWER_LIMITED] = { 0 };

static bool mt_gpufreq_opp_max_frequency_state;
static unsigned int mt_gpufreq_opp_max_frequency;
static unsigned int mt_gpufreq_opp_max_index;

static enum GPUFREQ_TABLE_TYPE mt_gpufreq_dvfs_table_type = GPUFREQ_TABLE_TYPE_DEF;
static unsigned int mt_gpufreq_dvfs_mmpll_spd_bond;

/* static DEFINE_SPINLOCK(mt_gpufreq_lock); */
static DEFINE_MUTEX(mt_gpufreq_lock);
static DEFINE_MUTEX(mt_gpufreq_power_lock);

static struct mt_gpufreq_power_table_info *mt_gpufreqs_power;
static unsigned int mt_gpufreqs_power_num;	/* run-time decided */

/* Currently MMPLL rate is not flaged with noncached, use this to get 'real' rate */
#define GPUFREQ_MAP_APMIXEDSYS

#ifdef GPUFREQ_MAP_APMIXEDSYS
static void __iomem *ap_start;
#else
static struct clk *clk_pll;
#endif


/******************************
 * Extern Function Declaration
 *******************************/
/*extern int mtk_gpufreq_register(struct mt_gpufreq_power_table_info *freqs, int num);
extern u32 get_devinfo_with_index(u32 index);*/


/**************************************
 * AEE (SRAM debug)
 ***************************************/
#ifdef MT_GPUFREQ_AEE_RR_REC
enum gpu_dvfs_state {
	GPU_DVFS_IS_DOING_DVFS = 0,
	GPU_DVFS_IS_VGPU_ENABLED,	/* always on */
};

static void _mt_gpufreq_aee_init(void)
{
	aee_rr_rec_gpu_dvfs_vgpu(0xFF);	/* no used */
	aee_rr_rec_gpu_dvfs_oppidx(0xFF);
	aee_rr_rec_gpu_dvfs_status(0xFF);
}
#endif

/**************************************
 * Efuse
 ***************************************/
static enum GPUFREQ_TABLE_TYPE _mt_gpufreq_get_dvfs_table_type(void)
{
	enum GPUFREQ_TABLE_TYPE type = GPUFREQ_TABLE_TYPE_DEF;

	mt_gpufreq_dvfs_mmpll_spd_bond = (get_devinfo_with_index(4) >> 20) & 0xf;

	gpufreq_info("@%s: mmpll_spd_bond = 0x%x\n", __func__, mt_gpufreq_dvfs_mmpll_spd_bond);

	/* No efuse,  use clock-frequency from device tree to determine GPU table type! */
	if (mt_gpufreq_dvfs_mmpll_spd_bond == 0) {
#ifdef CONFIG_OF
		static const struct of_device_id gpu_ids[] = {
			{.compatible = "arm,malit6xx"},
			{.compatible = "arm,mali-midgard"},
			{ /* sentinel */ }
		};

		struct device_node *node;
		unsigned int gpu_speed = 0;

		node = of_find_matching_node(NULL, gpu_ids);
		if (!node) {
			gpufreq_err("@%s: find GPU node failed\n", __func__);
			gpu_speed = GPU_DEFAULT_MAX_FREQ_MHZ;	/* default speed */
		} else {
			if (!of_property_read_u32(node, "clock-frequency", &gpu_speed)) {
				gpu_speed = gpu_speed / 1000 / 1000;	/* MHz */
			} else {
				gpufreq_err
				    ("@%s: missing clock-frequency property, use default GPU level\n",
				     __func__);
				gpu_speed = GPU_DEFAULT_MAX_FREQ_MHZ;	/* default speed */
			}
		}
		gpufreq_info("GPU clock-frequency from DT = %d MHz\n", gpu_speed);


		if (gpu_speed > GPU_DEFAULT_MAX_FREQ_MHZ)
			type = GPUFREQ_TABLE_TYPE_A;	/* 600M */
		else
			type = GPUFREQ_TABLE_TYPE_DEF;
#else
		gpufreq_err("@%s: Cannot get GPU speed from DT!\n", __func__);
		type = GPUFREQ_TABLE_TYPE_DEF;
#endif
		return type;
	}

	switch (mt_gpufreq_dvfs_mmpll_spd_bond) {
	case 0x4:
		type = GPUFREQ_TABLE_TYPE_A;
		break;
	case 0x5:
		type = GPUFREQ_TABLE_TYPE_B;
		break;
	case 0x7:
		type = GPUFREQ_TABLE_TYPE_D;
		break;
	default:
		type = GPUFREQ_TABLE_TYPE_DEF;
		break;
	}
	return type;
}

#ifdef MT_GPUFREQ_INPUT_BOOST
static struct task_struct *mt_gpufreq_up_task;

static int _mt_gpufreq_input_boost_task(void *data)
{
	while (1) {
		gpufreq_dbg("@%s: begin\n", __func__);

		if (NULL != g_pGpufreq_input_boost_notify) {
			gpufreq_dbg("@%s: g_pGpufreq_input_boost_notify\n", __func__);
			g_pGpufreq_input_boost_notify(g_gpufreq_max_id);
		}

		gpufreq_dbg("@%s: end\n", __func__);

		set_current_state(TASK_INTERRUPTIBLE);
		schedule();

		if (kthread_should_stop())
			break;
	}

	return 0;
}


/**************************************
 * Input boost
 ***************************************/
static void _mt_gpufreq_input_event(struct input_handle *handle, unsigned int type,
				    unsigned int code, int value)
{
	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

	if ((type == EV_KEY) && (code == BTN_TOUCH) && (value == 1)
	    && (mt_gpufreq_input_boost_state == 1)) {
		gpufreq_dbg("@%s: accept.\n", __func__);

		wake_up_process(mt_gpufreq_up_task);
	}
}

static int _mt_gpufreq_input_connect(struct input_handler *handler,
				     struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "gpufreq_ib";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void _mt_gpufreq_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id mt_gpufreq_ids[] = {
	{.driver_info = 1},
	{},
};

static struct input_handler mt_gpufreq_input_handler = {
	.event = _mt_gpufreq_input_event,
	.connect = _mt_gpufreq_input_connect,
	.disconnect = _mt_gpufreq_input_disconnect,
	.name = "gpufreq_ib",
	.id_table = mt_gpufreq_ids,
};
#endif

/**************************************
 * Convert voltage to pmic wrap register
 ***************************************/
#if defined(MT_GPUFREQ_AEE_RR_REC) || defined(MT_GPUFREQ_PERFORMANCE_TEST)
static unsigned int _mt_gpufreq_volt_to_pmic_wrap(unsigned int volt)
{
	unsigned int reg_val = 0;

	reg_val = (volt - 70000) / 625;

	gpufreq_dbg("@%s: reg_val = %d\n", __func__, reg_val);

	if (reg_val > 0x7F) {
		gpufreq_err("@%s: reg_val > 0x7F!\n", __func__);
		return 0x7F;
	}

	return reg_val;
}
#endif

static unsigned int _mt_gpufreq_pmic_wrap_to_volt(unsigned int pmic_wrap_value)
{
	unsigned int volt = 0;

	volt = (pmic_wrap_value * 625) + 70000;

	gpufreq_dbg("@%s: volt = %d\n", __func__, volt);

	if (volt > 139375) {
		gpufreq_err("@%s: volt > 1.39375v!\n", __func__);
		return 139375;
	}

	return volt;
}


/**************************************
 * Power table calculation
 ***************************************/
static void _mt_gpufreq_power_calculation(unsigned int idx, unsigned int freq, unsigned int volt,
					  unsigned int temp)
{

/* TODO: CHECK This */
#define GPU_ACT_REF_POWER		720	/* mW  */
#define GPU_ACT_REF_FREQ		450000	/* KHz */
#define GPU_ACT_REF_VOLT		115000	/* mV x 100 */

	unsigned int p_total = 0, p_dynamic = 0, p_leakage = 0, ref_freq = 0, ref_volt = 0;

	p_dynamic = GPU_ACT_REF_POWER;
	ref_freq = GPU_ACT_REF_FREQ;
	ref_volt = GPU_ACT_REF_VOLT;

	p_dynamic = p_dynamic *
	    ((freq * 100) / ref_freq) *
	    ((volt * 100) / ref_volt) * ((volt * 100) / ref_volt) / (100 * 100 * 100);

#if 0
	p_leakage =  mt_spower_get_leakage(MT_SPOWER_GPU, (volt / 100), temp);
#endif
	p_leakage = 0;
	p_total = p_dynamic + p_leakage;

	gpufreq_dbg("%d: p_dynamic = %d, p_leakage = %d, p_total = %d\n", idx, p_dynamic, p_leakage,
		    p_total);

	mt_gpufreqs_power[idx].gpufreq_power = p_total;
}


#ifdef MT_GPUFREQ_DYNAMIC_POWER_TABLE_UPDATE
static void _mt_update_gpufreqs_power_table(void)
{
	int i = 0, temp = 0;
	unsigned int freq, volt;

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready\n", __func__);
		return;
	}
#ifdef CONFIG_THERMAL
	temp = get_immediate_gpu_wrap() / 1000;
#else
	temp = 40;
#endif
	gpufreq_dbg("@%s, temp = %d\n", __func__, temp);

	mutex_lock(&mt_gpufreq_lock);

	if ((temp > 0) && (temp < 125)) {
		for (i = 0; i < mt_gpufreqs_power_num; i++) {
			freq = mt_gpufreqs_power[i].gpufreq_khz;
			volt = mt_gpufreqs_power[i].gpufreq_volt;

			_mt_gpufreq_power_calculation(i, freq, volt, temp);

			gpufreq_dbg("update mt_gpufreqs_power[%d].gpufreq_khz = %d\n", i,
				    mt_gpufreqs_power[i].gpufreq_khz);
			gpufreq_dbg("update mt_gpufreqs_power[%d].gpufreq_volt = %d\n", i,
				    mt_gpufreqs_power[i].gpufreq_volt);
			gpufreq_dbg("update mt_gpufreqs_power[%d].gpufreq_power = %d\n", i,
				    mt_gpufreqs_power[i].gpufreq_power);
		}
	} else
		gpufreq_err("@%s: temp < 0 or temp > 125, NOT update power table!\n", __func__);

	mutex_unlock(&mt_gpufreq_lock);
}
#endif

static void _mt_setup_gpufreqs_power_table(int num)
{
	int i, j, k, temp = 0;
	unsigned int freq, volt;

	mt_gpufreqs_power = kzalloc((num) * sizeof(struct mt_gpufreq_power_table_info), GFP_KERNEL);
	if (mt_gpufreqs_power == NULL) {
		/*gpufreq_err("GPU power table memory allocation fail\n"); */
		return;
	}
#ifdef CONFIG_THERMAL
	temp = get_immediate_gpu_wrap() / 1000;
#else
	temp = 40;
#endif
	gpufreq_info("@%s: temp = %d\n", __func__, temp);

	if ((temp < 0) || (temp > 125)) {
		gpufreq_dbg("@%s: temp < 0 or temp > 125!\n", __func__);
		temp = 65;
	}

	/* fill-in freq and volt in power table */
	for (i = 0, k = 0; i < mt_gpufreqs_num; i++) {	/* freq */
		for (j = 0; j <= i; j++) {	/* volt */
			mt_gpufreqs_power[k].gpufreq_khz = mt_gpufreqs[i].gpufreq_khz;
			mt_gpufreqs_power[k].gpufreq_volt = mt_gpufreqs[j].gpufreq_volt;
			k++;

			if (k == num)
				break;	/* should not happen? */
		}

		if (k == num)
			break;	/* should not happen? */
	}

	for (i = 0; i < num; i++) {
		freq = mt_gpufreqs_power[i].gpufreq_khz;
		volt = mt_gpufreqs_power[i].gpufreq_volt;

		_mt_gpufreq_power_calculation(i, freq, volt, temp);

		gpufreq_info("mt_gpufreqs_power[%d].gpufreq_khz = %u\n", i, freq);
		gpufreq_info("mt_gpufreqs_power[%d].gpufreq_volt = %u\n", i, volt);
		gpufreq_info("mt_gpufreqs_power[%d].gpufreq_power = %u\n", i,
			     mt_gpufreqs_power[i].gpufreq_power);
	}

#ifdef CONFIG_THERMAL
	mtk_gpufreq_register(mt_gpufreqs_power, num);
#endif
}

/***********************************************
 * register frequency table to gpufreq subsystem
 ************************************************/
static int _mt_setup_gpufreqs_table(struct mt_gpufreq_table_info *freqs, int num)
{
	int i = 0;

	mt_gpufreqs = kzalloc((num) * sizeof(*freqs), GFP_KERNEL);
	mt_gpufreqs_default = kzalloc((num) * sizeof(*freqs), GFP_KERNEL);
	if (mt_gpufreqs == NULL)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		mt_gpufreqs[i].gpufreq_khz = freqs[i].gpufreq_khz;
		mt_gpufreqs[i].gpufreq_volt = freqs[i].gpufreq_volt;

		mt_gpufreqs_default[i].gpufreq_khz = freqs[i].gpufreq_khz;
		mt_gpufreqs_default[i].gpufreq_volt = freqs[i].gpufreq_volt;

		gpufreq_dbg("mt_gpufreqs[%d].gpufreq_khz = %u\n", i, mt_gpufreqs[i].gpufreq_khz);
		gpufreq_dbg("mt_gpufreqs[%d].gpufreq_volt = %u\n", i, mt_gpufreqs[i].gpufreq_volt);
	}


	mt_gpufreqs_num = num;

	g_limited_max_id = 0;
	g_limited_min_id = mt_gpufreqs_num - 1;

	/* setup power table */
	mt_gpufreqs_power_num = (num + 1) * num / 2;

	_mt_setup_gpufreqs_power_table(mt_gpufreqs_power_num);

	return 0;
}

/*****************************
 * set GPU DVFS status
 ******************************/
static int _mt_gpufreq_state_set(int enabled)
{
	if (enabled) {
		if (!mt_gpufreq_pause) {
			gpufreq_dbg("gpufreq already enabled\n");
			return 0;
		}

		/*****************
		 * enable GPU DVFS
		 ******************/
		g_gpufreq_dvfs_disable_count--;
		gpufreq_dbg("enable GPU DVFS: g_gpufreq_dvfs_disable_count = %d\n",
			    g_gpufreq_dvfs_disable_count);

		/***********************************************
		 * enable DVFS if no any module still disable it
		 ************************************************/
		if (g_gpufreq_dvfs_disable_count <= 0)
			mt_gpufreq_pause = false;
		else
			gpufreq_warn("someone still disable gpufreq, cannot enable it\n");
	} else {
		/******************
		 * disable GPU DVFS
		 *******************/
		g_gpufreq_dvfs_disable_count++;
		gpufreq_dbg("disable GPU DVFS: g_gpufreq_dvfs_disable_count = %d\n",
			    g_gpufreq_dvfs_disable_count);

		if (mt_gpufreq_pause) {
			gpufreq_dbg("gpufreq already disabled\n");
			return 0;
		}

		mt_gpufreq_pause = true;
	}

	return 0;
}

static unsigned int _mt_gpufreq_dds_calc(unsigned int khz)
{
	unsigned int dds = 0;
	dds = ((khz * 4 / 1000) * 8192) / 13;
	return dds;
}

static void _mt_gpufreq_set_cur_freq(unsigned int freq_new)
{
	unsigned int dds = _mt_gpufreq_dds_calc(freq_new);

	mt_dfs_mmpll(dds);

	pr_debug("@%s: freq_new = %d, dds = 0x%x\n", __func__, freq_new, dds);

	if (NULL != g_pFreqSampler)
		g_pFreqSampler(freq_new);
}

static unsigned int _mt_gpufreq_get_cur_freq(void)
{
#ifdef GPUFREQ_MAP_APMIXEDSYS
	unsigned int mmpll;
	unsigned int freq;
	u32 div;
	u32 n_info;

	mmpll = __raw_readl(ap_start + 0x244) & ~0x80000000;
	div = 1 << ((mmpll  & (0x7 << 24)) >> 24);
	n_info = (mmpll  & 0x1fffff) >> 14;
	freq = n_info * 26000 / div;
	pr_debug(" div %d, mmpll 0x%x, n_info %d", div, mmpll, n_info);

	return freq;
#else

	if (clk_pll) {
		unsigned int freq_khz;

		freq_khz = clk_get_rate(clk_pll)/1000;
		return freq_khz;
	}

	return 0;
#endif


}

static unsigned int _mt_gpufreq_get_cur_volt(void)
{
	return _mt_gpufreq_pmic_wrap_to_volt(upmu_get_ni_vproc_vosel());
}


/*****************************************
 * frequency ramp up and ramp down handler
 ******************************************/
/***********************************************************
 * [note]
 * 1. frequency ramp up need to wait voltage settle
 * 2. frequency ramp down do not need to wait voltage settle
 ************************************************************/
static int _mt_gpufreq_set(unsigned int freq_old, unsigned int freq_new, unsigned int new_oppidx)
{
	/* only freq can be changed , */
	/* volt ? */
	_mt_gpufreq_set_cur_freq(freq_new);
	return 0;
}


/* Set frequency and voltage at driver probe function */
static void _mt_gpufreq_set_initial(void)
{
	unsigned int cur_volt = 0, cur_freq = 0;
	int i = 0;

	mutex_lock(&mt_gpufreq_lock);

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	cur_volt = _mt_gpufreq_get_cur_volt();
	cur_freq = _mt_gpufreq_get_cur_freq();

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (cur_volt == mt_gpufreqs[i].gpufreq_volt) {
			_mt_gpufreq_set_cur_freq(mt_gpufreqs[i].gpufreq_khz);
			g_cur_gpu_OPPidx = i;
			gpufreq_dbg("init_idx = %d\n", g_cur_gpu_OPPidx);
			break;
		}
	}

	/* Not found, set to LPM */
	if (i == mt_gpufreqs_num) {
		gpufreq_err
		    ("Set to LPM since GPU idx not found according to current Vcore = %d mV\n",
		     cur_volt / 100);
		g_cur_gpu_OPPidx = mt_gpufreqs_num - 1;
		_mt_gpufreq_set(cur_freq, mt_gpufreqs[mt_gpufreqs_num - 1].gpufreq_khz,
				g_cur_gpu_OPPidx);
	}
#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_oppidx(g_cur_gpu_OPPidx);
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() & ~(1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	mutex_unlock(&mt_gpufreq_lock);
}

/************************************************
 * frequency adjust interface for thermal protect
 *************************************************/
/******************************************************
 * parameter: target power
 *******************************************************/
static int _mt_gpufreq_power_throttle_protect(void)
{
	int ret = 0;
	int i = 0;
	unsigned int limited_index = 0;

	/* Check lowest frequency in all limitation */
	for (i = 0; i < NR_IDX_POWER_LIMITED; i++) {
		if (mt_gpufreq_power_limited_index_array[i] != 0 && limited_index == 0)
			limited_index = mt_gpufreq_power_limited_index_array[i];
		else if (mt_gpufreq_power_limited_index_array[i] != 0 && limited_index != 0) {
			if (mt_gpufreq_power_limited_index_array[i] > limited_index)
				limited_index = mt_gpufreq_power_limited_index_array[i];
		}

		gpufreq_dbg("mt_gpufreq_power_limited_index_array[%d] = %d\n", i,
			    mt_gpufreq_power_limited_index_array[i]);
	}

	g_limited_max_id = limited_index;
	gpufreq_dbg("Final limit frequency upper bound to id = %d, frequency = %d\n",
		    g_limited_max_id, mt_gpufreqs[g_limited_max_id].gpufreq_khz);

	if (NULL != g_pGpufreq_power_limit_notify)
		g_pGpufreq_power_limit_notify(g_limited_max_id);

	return ret;
}


/************************************************
 * frequency adjust interface for thermal protect
 *************************************************/
/******************************************************
 * parameter: target power
 *******************************************************/
static unsigned int _mt_gpufreq_get_limited_freq(unsigned int limited_power)
{
	int i = 0;
	unsigned int limited_freq = 0;
	unsigned int found = 0;

	for (i = 0; i < mt_gpufreqs_power_num; i++) {
		if (mt_gpufreqs_power[i].gpufreq_power <= limited_power) {
			limited_freq = mt_gpufreqs_power[i].gpufreq_khz;
			found = 1;
			break;
		}
	}

	/* not found */
	if (!found)
		limited_freq = mt_gpufreqs_power[mt_gpufreqs_power_num - 1].gpufreq_khz;

	gpufreq_info("@%s: limited_freq = %d\n", __func__, limited_freq);

	return limited_freq;
}

void mt_gpufreq_thermal_protect(unsigned int limited_power)
{
	int i = 0;
	unsigned int limited_freq = 0;

	mutex_lock(&mt_gpufreq_power_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	if (mt_gpufreqs_num == 0) {
		gpufreq_warn("@%s: mt_gpufreqs_num == 0!\n", __func__);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	if (g_limited_thermal_ignore_state == true) {
		gpufreq_info("@%s: g_limited_thermal_ignore_state == true!\n", __func__);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	mt_gpufreq_thermal_limited_gpu_power = limited_power;

	gpufreq_dbg("@%s: limited_power = %d\n", __func__, limited_power);

#ifdef MT_GPUFREQ_DYNAMIC_POWER_TABLE_UPDATE
	_mt_update_gpufreqs_power_table();
#endif

	if (limited_power == 0)
		mt_gpufreq_power_limited_index_array[IDX_THERMAL_LIMITED] = 0;
	else {
		limited_freq = _mt_gpufreq_get_limited_freq(limited_power);

		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (mt_gpufreqs[i].gpufreq_khz <= limited_freq) {
				mt_gpufreq_power_limited_index_array[IDX_THERMAL_LIMITED] = i;
				break;
			}
		}
	}

	gpufreq_dbg("Thermal limit frequency upper bound to id = %d\n",
		    mt_gpufreq_power_limited_index_array[IDX_THERMAL_LIMITED]);

	_mt_gpufreq_power_throttle_protect();

	mutex_unlock(&mt_gpufreq_power_lock);

	/*return; */
}
EXPORT_SYMBOL(mt_gpufreq_thermal_protect);

#if 0
unsigned int mt_gpufreq_update_volt(unsigned int pmic_volt[], unsigned int array_size)
{
	return 0;
}

void mt_gpufreq_restore_default_volt(void)
{
}

void mt_gpufreq_enable_by_ptpod(void)
{
}

void mt_gpufreq_disable_by_ptpod(void)
{
}
#endif
unsigned int mt_gpufreq_get_leakage_mw(void)
{
/* TODO */
#if 0
/* NO pbm */
	int temp = 0;
	unsigned int cur_vcore = _mt_gpufreq_get_cur_volt() / 100;

#ifdef CONFIG_THERMAL
	temp = get_immediate_gpu_wrap() / 1000;
#else
	temp = 40;
#endif

	return mt_spower_get_leakage(MT_SPOWER_VCORE, cur_vcore, temp);
#else
	return 0;
#endif
}

/************************************************
 * return current GPU thermal limit index
 *************************************************/
unsigned int mt_gpufreq_get_thermal_limit_index(void)
{
	gpufreq_dbg("current GPU thermal limit index is %d\n", g_limited_max_id);
	return g_limited_max_id;
}
EXPORT_SYMBOL(mt_gpufreq_get_thermal_limit_index);

/************************************************
 * return current GPU thermal limit frequency
 *************************************************/
unsigned int mt_gpufreq_get_thermal_limit_freq(void)
{
	gpufreq_dbg("current GPU thermal limit freq is %d MHz\n",
		    mt_gpufreqs[g_limited_max_id].gpufreq_khz / 1000);
	return mt_gpufreqs[g_limited_max_id].gpufreq_khz;
}
EXPORT_SYMBOL(mt_gpufreq_get_thermal_limit_freq);

/**********************************
 * gpufreq target callback function
 ***********************************/
/*************************************************
 * [note]
 * 1. handle frequency change request
 * 2. call mt_gpufreq_set to set target frequency
 **************************************************/
int mt_gpufreq_target(unsigned int idx)
{
	/* unsigned long flags; */
	unsigned int target_freq, target_volt, target_OPPidx;
	unsigned int cur_freq = _mt_gpufreq_get_cur_freq();
	int ret = 0;

#ifdef MT_GPUFREQ_PERFORMANCE_TEST
	return 0;
#endif

	mutex_lock(&mt_gpufreq_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("GPU DVFS not ready!\n");
		mutex_unlock(&mt_gpufreq_lock);
		return -ENOSYS;
	}

	if (mt_gpufreq_volt_enable_state == 0) {
		gpufreq_info("mt_gpufreq_volt_enable_state == 0! return\n");
		mutex_unlock(&mt_gpufreq_lock);
		return -ENOSYS;
	}

	if (idx > (mt_gpufreqs_num - 1)) {
		mutex_unlock(&mt_gpufreq_lock);
		gpufreq_err("@%s: idx out of range! idx = %d\n", __func__, idx);
		return -1;
	}

	/************************************************
	 * If /proc command fix the frequency.
	 *************************************************/
	if (mt_gpufreq_fixed_freq_state == true) {
		mutex_unlock(&mt_gpufreq_lock);
		gpufreq_info("@%s: GPU freq is fixed by user! fixed_freq = %dKHz\n", __func__,
			     mt_gpufreq_fixed_frequency);
		return 0;
	}
#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	/**********************************
	 * look up for the target GPU OPP
	 ***********************************/
	target_freq = mt_gpufreqs[idx].gpufreq_khz;
	target_volt = mt_gpufreqs[idx].gpufreq_volt;
	target_OPPidx = idx;

	gpufreq_dbg("@%s: begin, receive freq: %d, OPPidx: %d\n", __func__, target_freq,
		    target_OPPidx);

	/**********************************
	 * Check if need to keep max frequency
	 ***********************************/
	if (mt_gpufreq_keep_max_frequency_state == true) {
		target_freq = mt_gpufreqs[g_gpufreq_max_id].gpufreq_khz;
		target_volt = mt_gpufreqs[g_gpufreq_max_id].gpufreq_volt;
		target_OPPidx = g_gpufreq_max_id;
		gpufreq_dbg("Keep MAX frequency %d !\n", target_freq);
	}

	/************************************************
	 * If /proc command keep opp frequency.
	 *************************************************/
	if (mt_gpufreq_keep_opp_frequency_state == true) {
		target_freq = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_khz;
		target_volt = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_volt;
		target_OPPidx = mt_gpufreq_keep_opp_index;
		gpufreq_dbg("Keep opp! opp frequency %d, opp voltage %d, opp idx %d\n", target_freq,
			    target_volt, target_OPPidx);
	}

	/************************************************
	 * If /proc command keep opp max frequency.
	 *************************************************/
	if (mt_gpufreq_opp_max_frequency_state == true) {
		if (target_freq > mt_gpufreq_opp_max_frequency) {
			target_freq = mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz;
			target_volt = mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_volt;
			target_OPPidx = mt_gpufreq_opp_max_index;

			gpufreq_dbg
			    ("opp max freq! opp max frequency %d, opp max voltage %d, opp max idx %d\n",
			     target_freq, target_volt, target_OPPidx);
		}
	}


	/************************************************
	 * Thermal/Power limit
	 *************************************************/
	if (g_limited_max_id != 0) {
		if (target_freq > mt_gpufreqs[g_limited_max_id].gpufreq_khz) {
			/*********************************************
			 * target_freq > limited_freq, need to adjust
			 **********************************************/
			target_freq = mt_gpufreqs[g_limited_max_id].gpufreq_khz;
			target_volt = mt_gpufreqs[g_limited_max_id].gpufreq_volt;
			target_OPPidx = g_limited_max_id;
			gpufreq_dbg("Limit! Thermal/Power limit gpu frequency %d\n",
				    mt_gpufreqs[g_limited_max_id].gpufreq_khz);
		}
	}

	/************************************************
	 * target frequency == current frequency, skip it
	 *************************************************/
	if (cur_freq == target_freq) {
		mutex_unlock(&mt_gpufreq_lock);
		gpufreq_dbg("GPU frequency from %d KHz to %d KHz (skipped) due to same frequency\n",
			    cur_freq, target_freq);
		return 0;
	}

	gpufreq_dbg("GPU current frequency %d KHz, target frequency %d KHz\n", cur_freq,
		    target_freq);

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_oppidx(target_OPPidx);
#endif

	/******************************
	 * set to the target frequency
	 *******************************/
	ret = _mt_gpufreq_set(cur_freq, target_freq, target_OPPidx);
	if (!ret)
		g_cur_gpu_OPPidx = target_OPPidx;

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_oppidx(g_cur_gpu_OPPidx);
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() & ~(1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	mutex_unlock(&mt_gpufreq_lock);

	return ret;
}
EXPORT_SYMBOL(mt_gpufreq_target);

/*
 * There is no real HW operaition because vcore will not be
 * dynamically enabled/disabled.
 * Reserve this.
 */
int mt_gpufreq_voltage_enable_set(unsigned int enable)
{
	int ret = 0;

	gpufreq_dbg("@%s: enable = %x\n", __func__, enable);

	mutex_lock(&mt_gpufreq_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		mutex_unlock(&mt_gpufreq_lock);
		return -ENOSYS;
	}

	if (enable == mt_gpufreq_volt_enable_state) {
		mutex_unlock(&mt_gpufreq_lock);
		return 0;
	}
	mt_gpufreq_volt_enable_state = enable;

	mutex_unlock(&mt_gpufreq_lock);

	return ret;
}
EXPORT_SYMBOL(mt_gpufreq_voltage_enable_set);

unsigned int mt_gpufreq_get_dvfs_table_num(void)
{
	return mt_gpufreqs_num;
}
EXPORT_SYMBOL(mt_gpufreq_get_dvfs_table_num);

unsigned int mt_gpufreq_get_freq_by_idx(unsigned int idx)
{
	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return -ENOSYS;
	}

	if (idx < mt_gpufreqs_num) {
		gpufreq_dbg("@%s: idx = %d, frequency= %d\n", __func__, idx,
			    mt_gpufreqs[idx].gpufreq_khz);
		return mt_gpufreqs[idx].gpufreq_khz;
	}

	gpufreq_dbg("@%s: idx = %d, NOT found! return 0!\n", __func__, idx);
	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_get_freq_by_idx);

/************************************************
 * return current GPU frequency index
 *************************************************/
unsigned int mt_gpufreq_get_cur_freq_index(void)
{
	unsigned int idx;

	mutex_lock(&mt_gpufreq_lock);
	idx = g_cur_gpu_OPPidx;
	mutex_unlock(&mt_gpufreq_lock);
	return idx;
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq_index);

/************************************************
 * return current GPU frequency
 *************************************************/
unsigned int mt_gpufreq_get_cur_freq(void)
{
	return _mt_gpufreq_get_cur_freq();
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq);

/************************************************
 * return current GPU voltage
 *************************************************/
unsigned int mt_gpufreq_get_cur_volt(void)
{
	return _mt_gpufreq_get_cur_volt();
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_volt);

/************************************************
 * register / unregister GPU input boost notifiction CB
 *************************************************/
void mt_gpufreq_input_boost_notify_registerCB(gpufreq_input_boost_notify pCB)
{
#ifdef MT_GPUFREQ_INPUT_BOOST
	g_pGpufreq_input_boost_notify = pCB;
#endif
}
EXPORT_SYMBOL(mt_gpufreq_input_boost_notify_registerCB);

/************************************************
 * register / unregister GPU power limit notifiction CB
 *************************************************/
void mt_gpufreq_power_limit_notify_registerCB(gpufreq_power_limit_notify pCB)
{
	g_pGpufreq_power_limit_notify = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_power_limit_notify_registerCB);

/************************************************
 * register / unregister set GPU freq CB
 *************************************************/
void mt_gpufreq_setfreq_registerCB(sampler_func pCB)
{
	g_pFreqSampler = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_setfreq_registerCB);

/************************************************
 * register / unregister set GPU volt CB
 *************************************************/
void mt_gpufreq_setvolt_registerCB(sampler_func pCB)
{
	g_pVoltSampler = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_setvolt_registerCB);

static int _mt_gpufreq_pm_restore_early(struct device *dev)
{
	int i = 0;
	int found = 0;
	unsigned int cur_freq = _mt_gpufreq_get_cur_freq();

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (cur_freq == mt_gpufreqs[i].gpufreq_khz) {
			g_cur_gpu_OPPidx = i;
			found = 1;
			gpufreq_dbg("match g_cur_gpu_OPPidx: %d\n", g_cur_gpu_OPPidx);
			break;
		}
	}

	if (found == 0) {
		g_cur_gpu_OPPidx = 0;
		gpufreq_err("gpu freq not found, set parameter to max freq\n");
	}

	gpufreq_dbg("GPU freq: %d\n", cur_freq);
	gpufreq_dbg("g_cur_gpu_OPPidx: %d\n", g_cur_gpu_OPPidx);

	return 0;
}

static int _mt_gpufreq_pdrv_probe(struct platform_device *pdev)
{
#ifdef MT_GPUFREQ_INPUT_BOOST
	int rc;
	struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1 };
#endif
	struct device_node *node;

#ifdef GPUFREQ_MAP_APMIXEDSYS
	static const struct of_device_id ap_ids[] = {
		{.compatible = "mediatek,mt8163-apmixedsys"},
		{ /* sentinel */ }
	};
	node = of_find_matching_node(NULL, ap_ids);
	if (node)
		ap_start = of_iomap(node, 0);
#else
	static const struct of_device_id mfg_ids[] = {
		{.compatible = "mediatek,mt8163-mfg"},
		{.compatible = "mediatek,midgard-mfg"},
		{ /* sentinel */ }
	};

	node = of_find_matching_node(NULL, mfg_ids);
	if (node)
		clk_pll = of_clk_get_by_name(node, "pll");
	if (!IS_ERR_OR_NULL(clk_pll)) {
		pr_debug("found pll clk from mfg in gpufreq %p\n", clk_pll);
	} else {
		pr_err("cannot get mmpll clk from mfg\\n");
		clk_pll = NULL;
	}
#endif


	/**********************
	 * Initial leackage power usage
	 ***********************/
/* TODO:    mt_spower_init(); */

	/**********************
	 * Initial SRAM debugging ptr
	 ***********************/
#ifdef MT_GPUFREQ_AEE_RR_REC
	_mt_gpufreq_aee_init();
#endif

	/**********************
	 * setup gpufreq table
	 ***********************/
	gpufreq_info("setup gpufreqs table\n");

	mt_gpufreq_dvfs_table_type = _mt_gpufreq_get_dvfs_table_type();
	_mt_setup_gpufreqs_table(GPUFREQ_GET_TABLE(mt_gpufreq_dvfs_table_type),
				 GPUFREQ_GET_TABLE_LEN(mt_gpufreq_dvfs_table_type));


#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_VGPU_ENABLED));
#endif

	mt_gpufreq_volt_enable_state = 1;

	/**********************
	 * setup initial frequency
	 ***********************/
	_mt_gpufreq_set_initial();

	gpufreq_info("GPU current frequency = %dKHz\n", _mt_gpufreq_get_cur_freq());
	gpufreq_info("Current Vcore = %dmV\n", _mt_gpufreq_get_cur_volt() / 100);
	gpufreq_info("g_cur_gpu_OPPidx = %d\n", g_cur_gpu_OPPidx);

	mt_gpufreq_ready = true;

#ifdef MT_GPUFREQ_INPUT_BOOST
	mt_gpufreq_up_task =
	    kthread_create(_mt_gpufreq_input_boost_task, NULL, "mt_gpufreq_input_boost_task");
	if (IS_ERR(mt_gpufreq_up_task))
		return PTR_ERR(mt_gpufreq_up_task);

	sched_setscheduler_nocheck(mt_gpufreq_up_task, SCHED_FIFO, &param);
	get_task_struct(mt_gpufreq_up_task);

	rc = input_register_handler(&mt_gpufreq_input_handler);
#endif


	return 0;
}

/***************************************
 * this function should never be called
 ****************************************/
static int _mt_gpufreq_pdrv_remove(struct platform_device *pdev)
{
#ifdef MT_GPUFREQ_INPUT_BOOST
	input_unregister_handler(&mt_gpufreq_input_handler);

	kthread_stop(mt_gpufreq_up_task);
	put_task_struct(mt_gpufreq_up_task);
#endif

#ifdef GPUFREQ_MAP_APMIXEDSYS
	if (ap_start)
		iounmap(ap_start);
#else
	if (clk_pll)
		clk_put(clk_pll);
#endif

	return 0;
}


const struct dev_pm_ops mt_gpufreq_pm_ops = {
	.suspend = NULL,
	.resume = NULL,
	.restore_early = _mt_gpufreq_pm_restore_early,
};

struct platform_device mt_gpufreq_pdev = {
	.name = "mt-gpufreq",
	.id = -1,
};

static struct platform_driver mt_gpufreq_pdrv = {
	.probe = _mt_gpufreq_pdrv_probe,
	.remove = _mt_gpufreq_pdrv_remove,
	.driver = {
		   .name = "mt-gpufreq",
		   .pm = &mt_gpufreq_pm_ops,
		   .owner = THIS_MODULE,
		   },
};


#ifdef CONFIG_PROC_FS
/*
 * PROC
 */

/***************************
 * show current debug status
 ****************************/
static int mt_gpufreq_debug_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_debug)
		seq_puts(m, "gpufreq debug enabled\n");
	else
		seq_puts(m, "gpufreq debug disabled\n");

	return 0;
}

/***********************
 * enable debug message
 ************************/
static ssize_t mt_gpufreq_debug_proc_write(struct file *file, const char __user *buffer,
					   size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int debug = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (!kstrtoint(desc, 10, &debug)) {
		if (debug == 0)
			mt_gpufreq_debug = 0;
		else if (debug == 1)
			mt_gpufreq_debug = 1;
		else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");

	return count;
}

/****************************
 * show current limited by thermal
 *****************************/
static int mt_gpufreq_limited_thermal_ignore_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "g_limited_max_id = %d, g_limited_thermal_ignore_state = %d\n",
		   g_limited_max_id, g_limited_thermal_ignore_state);

	return 0;
}

/**********************************
 * limited for thermal protect
 ***********************************/
static ssize_t mt_gpufreq_limited_thermal_ignore_proc_write(struct file *file,
							    const char __user *buffer,
							    size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int ignore = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (!kstrtouint(desc, 10, &ignore)) {
		if (ignore == 1)
			g_limited_thermal_ignore_state = true;
		else if (ignore == 0)
			g_limited_thermal_ignore_state = false;
		else
			gpufreq_warn
			    ("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");

	return count;
}

/****************************
 * show current limited power
 *****************************/
static int mt_gpufreq_limited_power_proc_show(struct seq_file *m, void *v)
{

	seq_printf(m, "g_limited_max_id = %d, limit frequency = %d\n", g_limited_max_id,
		   mt_gpufreqs[g_limited_max_id].gpufreq_khz);

	return 0;
}

/**********************************
 * limited power for thermal protect
 ***********************************/
static ssize_t mt_gpufreq_limited_power_proc_write(struct file *file, const char __user *buffer,
						   size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int power = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (!kstrtouint(desc, 10, &power))
		mt_gpufreq_thermal_protect(power);
	else
		gpufreq_warn("bad argument!! please provide the maximum limited power\n");

	return count;
}

/******************************
 * show current GPU DVFS stauts
 *******************************/
static int mt_gpufreq_state_proc_show(struct seq_file *m, void *v)
{
	if (!mt_gpufreq_pause)
		seq_puts(m, "GPU DVFS enabled\n");
	else
		seq_puts(m, "GPU DVFS disabled\n");

	return 0;
}

/****************************************
 * set GPU DVFS stauts by sysfs interface
 *****************************************/
static ssize_t mt_gpufreq_state_proc_write(struct file *file, const char __user *buffer,
					   size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int enabled = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (!kstrtoint(desc, 10, &enabled)) {
		if (enabled == 1) {
			mt_gpufreq_keep_max_frequency_state = false;
			_mt_gpufreq_state_set(1);
		} else if (enabled == 0) {
			/* Keep MAX frequency when GPU DVFS disabled. */
			mt_gpufreq_keep_max_frequency_state = true;
			mt_gpufreq_voltage_enable_set(1);
			mt_gpufreq_target(g_gpufreq_max_id);
			_mt_gpufreq_state_set(0);
		} else
			gpufreq_warn("bad argument!! argument should be \"1\" or \"0\"\n");
	} else
		gpufreq_warn("bad argument!! argument should be \"1\" or \"0\"\n");

	return count;
}

/********************
 * show GPU OPP table
 *********************/
static int mt_gpufreq_opp_dump_proc_show(struct seq_file *m, void *v)
{
	int i = 0;

	for (i = 0; i < mt_gpufreqs_num; i++) {
		seq_printf(m, "[%d] ", i);
		seq_printf(m, "freq = %d, ", mt_gpufreqs[i].gpufreq_khz);
		seq_printf(m, "volt = %d\n", mt_gpufreqs[i].gpufreq_volt);
	}

	return 0;
}

/********************
 * show GPU power table
 *********************/
static int mt_gpufreq_power_dump_proc_show(struct seq_file *m, void *v)
{
	int i = 0;

	for (i = 0; i < mt_gpufreqs_power_num; i++) {
		seq_printf(m, "mt_gpufreqs_power[%d].gpufreq_khz = %d\n", i,
			   mt_gpufreqs_power[i].gpufreq_khz);
		seq_printf(m, "mt_gpufreqs_power[%d].gpufreq_volt = %d\n", i,
			   mt_gpufreqs_power[i].gpufreq_volt);
		seq_printf(m, "mt_gpufreqs_power[%d].gpufreq_power = %d\n", i,
			   mt_gpufreqs_power[i].gpufreq_power);
	}

	return 0;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_opp_freq_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_keep_opp_frequency_state) {
		seq_puts(m, "gpufreq keep opp frequency enabled\n");
		seq_printf(m, "freq = %d\n", mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_khz);
		seq_printf(m, "volt = %d\n", mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_volt);
	} else
		seq_puts(m, "gpufreq keep opp frequency disabled\n");

	return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_opp_freq_proc_write(struct file *file, const char __user *buffer,
					      size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int i = 0;
	int fixed_freq = 0;
	int found = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (!kstrtoint(desc, 10, &fixed_freq)) {
		if (fixed_freq == 0) {
			mt_gpufreq_keep_opp_frequency_state = false;
		} else {
			for (i = 0; i < mt_gpufreqs_num; i++) {
				if (fixed_freq == mt_gpufreqs[i].gpufreq_khz) {
					mt_gpufreq_keep_opp_index = i;
					found = 1;
					break;
				}
			}

			if (found == 1) {
				mt_gpufreq_keep_opp_frequency_state = true;
				mt_gpufreq_keep_opp_frequency = fixed_freq;

				mt_gpufreq_voltage_enable_set(1);
				mt_gpufreq_target(mt_gpufreq_keep_opp_index);
			}

		}
	} else
		gpufreq_warn("bad argument!! please provide the fixed frequency\n");

	return count;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_opp_max_freq_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_opp_max_frequency_state) {
		seq_puts(m, "gpufreq opp max frequency enabled\n");
		seq_printf(m, "freq = %d\n", mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz);
		seq_printf(m, "volt = %d\n", mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_volt);
	} else
		seq_puts(m, "gpufreq opp max frequency disabled\n");

	return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_opp_max_freq_proc_write(struct file *file, const char __user *buffer,
						  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int i = 0;
	int max_freq = 0;
	int found = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtoint(desc, 10, &max_freq)) {
		if (max_freq == 0) {
			mt_gpufreq_opp_max_frequency_state = false;
		} else {
			for (i = 0; i < mt_gpufreqs_num; i++) {
				if (mt_gpufreqs[i].gpufreq_khz <= max_freq) {
					mt_gpufreq_opp_max_index = i;
					found = 1;
					break;
				}
			}

			if (found == 1) {
				mt_gpufreq_opp_max_frequency_state = true;
				mt_gpufreq_opp_max_frequency =
				    mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz;

				mt_gpufreq_voltage_enable_set(1);
				mt_gpufreq_target(mt_gpufreq_opp_max_index);
			}
		}
	} else
		gpufreq_warn("bad argument!! please provide the maximum limited frequency\n");

	return count;
}

/********************
 * show variable dump
 *********************/
static int mt_gpufreq_var_dump_proc_show(struct seq_file *m, void *v)
{
	int i = 0;

	seq_printf(m, "GPU current frequency = %dKHz\n", _mt_gpufreq_get_cur_freq());
	seq_printf(m, "Current Vcore = %dmV\n", _mt_gpufreq_get_cur_volt() / 100);
	seq_printf(m, "g_cur_gpu_OPPidx = %d\n", g_cur_gpu_OPPidx);
	seq_printf(m, "g_last_gpu_dvs_result = %d (0:success, 127:not enabled, else:error)\n",
		   g_last_gpu_dvs_result);
#if 0
	seq_printf(m, "g_is_vcorefs_on = %d (0:disable, 1:enable, 255:not build-in)\n",
		   g_is_vcorefs_on);
#endif
	seq_printf(m, "g_limited_max_id = %d\n", g_limited_max_id);

	for (i = 0; i < NR_IDX_POWER_LIMITED; i++)
		seq_printf(m, "mt_gpufreq_power_limited_index_array[%d] = %d\n", i,
			   mt_gpufreq_power_limited_index_array[i]);

	seq_printf(m, "mt_gpufreq_volt_enable_state = %d\n", mt_gpufreq_volt_enable_state);
	seq_printf(m, "mt_gpufreq_fixed_freq_state = %d\n", mt_gpufreq_fixed_freq_state);
	seq_printf(m, "mt_gpufreq_dvfs_table_type = %d\n", mt_gpufreq_dvfs_table_type);
	seq_printf(m, "mt_gpufreq_dvfs_mmpll_spd_bond = %d\n", mt_gpufreq_dvfs_mmpll_spd_bond);

	return 0;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_fixed_freq_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_fixed_freq_state) {
		seq_puts(m, "gpufreq fixed frequency enabled\n");
		seq_printf(m, "fixed frequency = %d\n", mt_gpufreq_fixed_frequency);
	} else
		seq_puts(m, "gpufreq fixed frequency disabled\n");

	return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_fixed_freq_proc_write(struct file *file, const char __user *buffer,
						size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	int fixed_freq = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (!kstrtoint(desc, 10, &fixed_freq)) {
		mutex_lock(&mt_gpufreq_lock);
		if (fixed_freq == 0) {
			mt_gpufreq_fixed_freq_state = false;
			mt_gpufreq_fixed_frequency = 0;
		} else {
			_mt_gpufreq_set_cur_freq(fixed_freq);
			mt_gpufreq_fixed_freq_state = true;
			mt_gpufreq_fixed_frequency = fixed_freq;
			g_cur_gpu_OPPidx = 0;	/* keep Max Vcore */
		}
		mutex_unlock(&mt_gpufreq_lock);
	} else
		gpufreq_warn("bad argument!! should be [enable fixed_freq(KHz)]\n");

	return count;
}

#ifdef MT_GPUFREQ_INPUT_BOOST
/*****************************
 * show current input boost status
 ******************************/
static int mt_gpufreq_input_boost_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_input_boost_state == 1)
		seq_puts(m, "gpufreq debug enabled\n");
	else
		seq_puts(m, "gpufreq debug disabled\n");

	return 0;
}

/***************************
 * enable/disable input boost
 ****************************/
static ssize_t mt_gpufreq_input_boost_proc_write(struct file *file, const char __user *buffer,
						 size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int debug = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (!kstrtoint(desc, 10, &debug)) {
		if (debug == 0)
			mt_gpufreq_input_boost_state = 0;
		else if (debug == 1)
			mt_gpufreq_input_boost_state = 1;
		else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");

	return count;
}
#endif

#define PROC_FOPS_RW(name)							\
	static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations mt_ ## name ## _proc_fops = {		\
	.owner			= THIS_MODULE,				\
	.open			= mt_ ## name ## _proc_open,	\
	.read			= seq_read,					\
	.llseek		= seq_lseek,					\
	.release		= single_release,				\
	.write			= mt_ ## name ## _proc_write,				\
}

#define PROC_FOPS_RO(name)							\
	static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations mt_ ## name ## _proc_fops = {		\
	.owner			= THIS_MODULE,				\
	.open			= mt_ ## name ## _proc_open,	\
	.read			= seq_read,					\
	.llseek		= seq_lseek,					\
	.release		= single_release,				\
}

#define PROC_ENTRY(name)	{__stringify(name), &mt_ ## name ## _proc_fops}

PROC_FOPS_RW(gpufreq_debug);
PROC_FOPS_RW(gpufreq_limited_power);
PROC_FOPS_RW(gpufreq_limited_thermal_ignore);
PROC_FOPS_RW(gpufreq_state);
PROC_FOPS_RO(gpufreq_opp_dump);
PROC_FOPS_RO(gpufreq_power_dump);
PROC_FOPS_RW(gpufreq_opp_freq);
PROC_FOPS_RW(gpufreq_opp_max_freq);
PROC_FOPS_RO(gpufreq_var_dump);
/* PROC_FOPS_RW(gpufreq_volt_enable); */
PROC_FOPS_RW(gpufreq_fixed_freq);
#ifdef MT_GPUFREQ_INPUT_BOOST
PROC_FOPS_RW(gpufreq_input_boost);
#endif

static int mt_gpufreq_create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(gpufreq_debug),
		PROC_ENTRY(gpufreq_limited_power),
		PROC_ENTRY(gpufreq_limited_thermal_ignore),
		PROC_ENTRY(gpufreq_state),
		PROC_ENTRY(gpufreq_opp_dump),
		PROC_ENTRY(gpufreq_power_dump),
		PROC_ENTRY(gpufreq_opp_freq),
		PROC_ENTRY(gpufreq_opp_max_freq),
		PROC_ENTRY(gpufreq_var_dump),
		PROC_ENTRY(gpufreq_fixed_freq),
#ifdef MT_GPUFREQ_INPUT_BOOST
		PROC_ENTRY(gpufreq_input_boost),
#endif
	};


	dir = proc_mkdir("gpufreq", NULL);

	if (!dir) {
		gpufreq_err("fail to create /proc/gpufreq @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create
		    (entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			gpufreq_err("@%s: create /proc/gpufreq/%s failed\n", __func__,
				    entries[i].name);
	}

	return 0;
}
#endif				/* CONFIG_PROC_FS */

/* #define GPUFREQ_FOR_BRINGUP */

/**********************************
 * mediatek gpufreq initialization
 ***********************************/
static int __init _mt_gpufreq_init(void)
{
	int ret = 0;

	gpufreq_info("@%s\n", __func__);

#ifdef GPUFREQ_FOR_BRINGUP
	gpufreq_info("@%s for bringup ignore\n", __func__);
	return 0;
#endif

#ifdef CONFIG_PROC_FS

	/* init proc */
	if (mt_gpufreq_create_procfs())
		goto out;

#endif				/* CONFIG_PROC_FS */

	/* register platform device/driver */
	ret = platform_device_register(&mt_gpufreq_pdev);
	if (ret) {
		gpufreq_err("fail to register gpufreq device @ %s()\n", __func__);
		goto out;
	}

	ret = platform_driver_register(&mt_gpufreq_pdrv);
	if (ret) {
		gpufreq_err("fail to register gpufreq driver @ %s()\n", __func__);
		platform_device_unregister(&mt_gpufreq_pdev);
	}

out:
	return ret;

}

static void __exit _mt_gpufreq_exit(void)
{
#ifdef GPUFREQ_FOR_BRINGUP
	gpufreq_info("@%s for bringup ignore\n", __func__);
	return;
#endif

	platform_driver_unregister(&mt_gpufreq_pdrv);
	platform_device_unregister(&mt_gpufreq_pdev);
}
module_init(_mt_gpufreq_init);
module_exit(_mt_gpufreq_exit);

MODULE_DESCRIPTION("MediaTek GPU Frequency Scaling driver");
MODULE_LICENSE("GPL");
