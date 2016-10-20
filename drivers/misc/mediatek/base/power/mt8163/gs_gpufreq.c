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

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/bitops.h>
#include <asm/atomic.h>
#include <linux/input.h>
#include <linux/clk.h>
#include <linux/clk-private.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>

#define MET_USER_EVENT_SUPPORT 1

#ifdef MET_USER_EVENT_SUPPORT	/* MET is built-in. */
#include <mt-plat/met_drv.h>
#else				/* dummy */
#define met_tag_oneshot(id, name, value) (0)
#endif

#include "mt_gpufreq.h"
#include "gs_gpufreq.h"

#define GPAD_IPEM_DEBUG_PROC 0

#define GS_GPUFREQ_PATHB_FAKE 0
#define GS_GPUFREQ_PATHB_POWER 0
#define GS_GPUFREQ_PATHB_FREQ 1
#define GS_GPUFREQ_PATHB_VOLT 1
#define GS_GPUFREQ_PATHB_MTCMOS 1
#define GS_GPUFREQ_THERMAL_FAKE 0

#define GPUFREQ_PTPOD 1
#define GPUFREQ_PTPOD_DEBUG 0

/*#define GS_GPUFREQ_BRING_UP */

#ifdef GS_GPUFREQ_BRING_UP
#warning GPUFREQ BRING_UP on
#endif

#if 0
#if GS_GPUFREQ_PATHB_FAKE
#include "gs_gpufreq_fake_hw.h"
#endif

#if GS_GPUFREQ_PATHB_MTCMOS
#include "mach/mt_clkmgr.h"
#endif

#if GS_GPUFREQ_PATHB_VOLT
#include "mach/pmic_mt6323_sw.h"
#include "mach/upmu_common.h"
#include "mach/upmu_hw.h"
#endif
#endif

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

#if GS_GPUFREQ_PATHB_VOLT


static volatile int *noncache_dummy_src;
static volatile int *noncache_dummy_dst;
static int late_init_done;
#define NONCACHE_DUMMY_SIZE 0x1000
#endif

#if defined(GS_GPUFREQ_BRING_UP)
#include <linux/of.h>
#include <linux/of_address.h>
#include "mt_typedefs.h"
#include "sync_write.h"
#include "mach/pmic_mt6323_sw.h"
#include "mach/upmu_common.h"
#include "mach/upmu_hw.h"
#include "mach/mt_spm_mtcmos.h"
static void __iomem *spm_cpu_base;
static void __iomem *clk_apmixed_base;
static void __iomem *clk_cksys_base;
#define SPM_MFG_PWR_CON         (spm_cpu_base + 0x214)
#define SPM_MFG_ASYNC_PWR_CON   (spm_cpu_base + 0x2c4)
#define SPM_PWR_STATUS          (spm_cpu_base + 0x60c)
#define SPM_PWR_STATUS_2ND      (spm_cpu_base + 0x610)
#define PWR_CLK_DIS             (1U << 4)
#define PWR_ON_2ND              (1U << 3)
#define PWR_ON                  (1U << 2)
#define PWR_ISO                 (1U << 1)
#define PWR_RST_B               (1U << 0)
#define MFG_SRAM_PDN            (0x3f << 8)
#define MFG_ASYNC_PROT_MASK     0xA00000	/*bit 21,23 */
#define TOPAXI_PROT_EN          (clk_infracfg_ao_base + 0x0220)
#define TOPAXI_PROT_STA1        (clk_infracfg_ao_base + 0x0228)
#define MMPLL_CON0              (clk_apmixed_base + 0x240)
#define MMPLL_CON1              (clk_apmixed_base + 0x244)
#define MMPLL_CON2              (clk_apmixed_base + 0x248)
#define MMPLL_PWR_CON0          (clk_apmixed_base + 0x24C)
#define CLK_CFG_UPDATE          (clk_cksys_base + 0x004)
#define CLK_CFG_1               (clk_cksys_base + 0x050)

#define gs_readl(addr) \
	DRV_Reg32(addr)

#define gs_writel(addr, val) \
	mt_reg_sync_writel(val, addr)

#endif

/* platform dependent */
#define print_error(fmt, args...) \
	pr_err(fmt, ##args)


#define print_notice(fmt, args...) \
	pr_notice(fmt, ##args)


#define print_info(fmt, args...) \
	pr_info(fmt, ##args)


#define print_debug(fmt, args...) \
	pr_debug(fmt, ##args)


#define LOCK_TABLE() \
	spin_lock_bh(&g_table_lock)
#define UNLOCK_TABLE() \
	spin_unlock_bh(&g_table_lock)

#define LOCK_FREQ_VOLT() \
	spin_lock_bh(&g_freq_volt_lock)
#define UNLOCK_FREQ_VOLT() \
	spin_unlock_bh(&g_freq_volt_lock)

#define DEFAULT_ERROR 1

#ifndef MT_GPUFREQ_INPUT_BOOST
#define MT_GPUFREQ_INPUT_BOOST
#endif

static struct platform_device *mfg_dev;
static struct platform_device *g3d_dev;
static void __iomem *mfg_start;
static struct clk *clk_mm;
static struct clk *clk_pll;
static struct clk *clk_topck;

static const struct of_device_id mfg_dt_ids[] = {
	{.compatible = "mediatek,mt8163-mfg"}
};

MODULE_DEVICE_TABLE(of, mfg_dt_ids);

#define MFG_CG_CON 0x0
#define MFG_CG_SET 0x4
#define MFG_CG_CLR 0x8
#define MFG_DEBUG_SEL 0x180
#define MFG_DEBUG_STAT 0x184

#define MFG_READ32(r) __raw_readl((void __iomem *)((unsigned long)mfg_start + (r)))
#define MFG_WRITE32(v, r) __raw_writel((v), (void __iomem *)((unsigned long)mfg_start + (r)))


/*****************************************************************************
 * Fake HW values
 *****************************************************************************/
static u32 g_fake_freq_khz = GPU_DVFS_F380;
static u32 g_fake_volt = GPU_POWER_VGPU_1_15V;
static s32 g_fake_vgpu_sel_default_called;
static s32 g_fake_vgpu_disable_called;
static bool g_touch_boost_switch = 1;
static bool g_thermal_switch = 1;
static bool g_low_battery_volt_switch = 1;
static bool g_low_battery_volume_switch = 1;
static atomic_t g_touch_boost_register_count = ATOMIC_INIT(0);
static atomic_t g_power_limit_register_count = ATOMIC_INIT(0);
static atomic_t g_freq_register_count = ATOMIC_INIT(0);
static atomic_t g_volt_register_count = ATOMIC_INIT(0);
static atomic_t g_table_register_count = ATOMIC_INIT(0);
static atomic_t g_table_unregister_count = ATOMIC_INIT(0);
static atomic_t g_update_volt_count = ATOMIC_INIT(0);
static s64 g_set_load_call_count;
static s64 g_set_index_call_count;
static s64 g_touch_boost_call_count;
static s64 g_thermal_call_count;
static s64 g_low_battery_volt_call_count;
static s64 g_low_battery_volume_call_count;

/*****************************************************************************
 * Fake external values
 *****************************************************************************/
static u32 g_fake_freq_to_thermal;
static u32 g_fake_volt_to_thermal;

/*****************************************************************************
 * Global
 *****************************************************************************/
static struct proc_dir_entry *g_proc_dir;

static u32 g_dvs_delay = 100;
static u32 g_dummy_read = 256;
static u32 g_debug;
static bool g_reg_finished = true;
static atomic_t g_power_switch = ATOMIC_INIT(1);	/* GPU power on or not */
static atomic_t g_dvfs_enable_count = ATOMIC_INIT(0);	/* GPU DVFS enabled or disabled */
static u32 g_freq_khz = GPU_DVFS_F480;	/* Current GPU frequency */
static u32 g_volt = GPU_POWER_VGPU_1_25V;	/* Current GPU voltage */
static u32 g_old_freq_khz = GPU_DVFS_F380;	/* Current GPU frequency */
static u32 g_old_volt = GPU_POWER_VGPU_1_15V;	/* Current GPU voltage */
static u32 g_load;		/* Current GPU loading. */
static u32 g_limited_power_mW;	/* Max power allowed, by thermal module. */
static u32 g_bottom_freq;	/* Lowest frequest expected */
static u32 g_ptpod_switch = 1;	/* When false, the dvfs is disabled */
static atomic_t g_mtcmos_enable_count = ATOMIC_INIT(1);	/* GPU MTCMOS enabled or disabled */


#ifdef CONFIG_THERMAL

static struct mtk_gpu_power_info g_power_table[10];
#endif

/* aligned to 64 */
struct freq_table {
	u32 capacity;		/* number of rows allocated */
	u32 size;		/* number of rows used */
	u32 input_index;	/* last index intend to set */
	u32 current_index;	/* current index selected */
	u32 thermal_max_index;	/* index to highest frequest under thermal limit */
	u32 low_battery_volt_max_index;	/* index to highest frequest under low battery voltage */
	u32 low_battery_volume_max_index;	/* index to highest frequest under low battery volume */
	u32 max_index;		/* index to highest frequest under all limit */
	u32 bottom_index;	/* index to lowest frequest frequency */
	u32 ptpod_disable_idx;

	struct mtk_gpufreq_info *rows;	/* table of freq/voltage/power/loading boundary */
#if BITS_PER_LONG == 32
	void *padding;		/* to keep structure size multiple of 64-bit. */
#endif
};


static struct mtk_gpufreq_info g_default_row = {
	.freq_khz = GPU_DVFS_F480,
	.volt = GPU_POWER_VGPU_1_25V,
	.power_mW = 1025,
	.lower_bound = 0,
	.upper_bound = 100,
};

static struct freq_table g_default_table = {
	.rows = &g_default_row,
	.capacity = 1,
	.size = 1,
	.input_index = 0,
	.current_index = 0,
	.thermal_max_index = 0,
	.low_battery_volt_max_index = 0,
	.low_battery_volume_max_index = 0,
	.max_index = 0,
	.bottom_index = 0,
	.ptpod_disable_idx = 0,
};

#if GS_GPUFREQ_PATHB_FAKE

#define PMIC_WRAP_PHASE_NORMAL 0
#define IDX_NM_VGPU 0

/* void pmic_config_interface(u32, u32, u32, u32); */
/* void pmic_read_interface(u32, u32 *, u32, u32); */
/* void mt_dfs_mmpll(u32); */
/* void mt_cpufreq_set_pmic_cmd(u32, u32, u32); */
/* void mt_cpufreq_apply_pmic_cmd(u32); */

#endif

#if GS_GPUFREQ_PATHB_FREQ
/* void mt_dfs_mmpll(u32); */
#endif




#if GS_GPUFREQ_THERMAL_FAKE
static void FAKE_thermal_control(u32 const index)
{
	static bool entered;
	static u32 powers[] = { 1050, 950, 800 };
	s64 time1 = 10;
	s64 time2 = 20;
	static s64 counter1;
	static s64 counter2;
	u32 power_index = 0;

	/* recursion guard */
	if (entered)
		return;

	entered = true;

	if (index >= 0)
		++counter1;

	if (index >= 1)
		++counter2;

	if (counter1 >= time1) {
		power_index = 1;
		counter1 = 0;
	}

	if (counter2 >= time2) {
		power_index = 2;
		counter2 = 0;
	}

	udelay(23);

	mt_gpufreq_thermal_protect(powers[power_index]);

	entered = false;
}

#endif

#if GS_GPUFREQ_PATHB_VOLT
#define PMIC_CMD_DELAY_TIME     5
#define MIN_PMIC_SETTLE_TIME    25

#define PMIC_ADDR_VGPU_EN             0x0612	/* [0]     */
#endif


#if GS_GPUFREQ_PATHB_FREQ
/* L1 */
static unsigned int _mt_gpufreq_dds_calc(unsigned int khz)
{
	unsigned int dds = 0;

	if ((khz >= 250250) && (khz <= 747500))
		dds = 0x0209A000 + ((khz - 250250) * 4 / 13000) * 0x2000;
	else
		print_error("[GPUFREQ][PROC][ERROR] @%s: target khz(%d) out of range!\n", __func__,
			    khz);

	return dds;
}

static void _mt_gpufreq_set_cur_freq(unsigned int freq_new)
{
	unsigned int dds = _mt_gpufreq_dds_calc(freq_new);

	mt_dfs_mmpll(dds);

	print_debug("@%s: freq_new = %d, dds = 0x%x\n", __func__, freq_new, dds);
}
#endif









#if GS_GPUFREQ_PATHB_POWER
/*  OLD */
static unsigned int mt_gpufreq_calc_pmic_settle_time(unsigned int volt_old, unsigned int volt_new)
{
	unsigned int delay = 100;

	if (volt_new == volt_old)
		return 0;
	else if (volt_new > volt_old) {
		/* need to find new formula. Can't compile now */
		delay = PMIC_VOLT_UP_SETTLE_TIME(volt_old, volt_new);
	} else {
		/* need to find new formula. Can't compile now */
		delay = PMIC_VOLT_DOWN_SETTLE_TIME(volt_old, volt_new);
	}

	if (delay < MIN_PMIC_SETTLE_TIME)
		delay = MIN_PMIC_SETTLE_TIME;

	return delay;
}
#endif

#define GPU_DVFS_PTPOD_DISABLE_FREQ GPU_DVFS_F380
#define GPU_DVFS_PTPOD_DISABLE_VOLT GPU_POWER_VGPU_1_15V

static struct freq_table *g_freq_table = &g_default_table;
static struct freq_table *g_backup_table = &g_default_table;

static struct mtk_gpufreq_kmif g_kmif_ops = {
	.notify_thermal_event = NULL,
	.notify_touch_event = NULL,

	.get_loading_stat = NULL,
	.get_block_stat = NULL,
	.get_idle_stat = NULL,
	.get_power_loading_stat = NULL
};

/* Locks  */
DEFINE_SPINLOCK(g_table_lock);
DEFINE_SPINLOCK(g_freq_volt_lock);

/*******************
* Callback
********************/
static sampler_func g_pFreqSampler;
static sampler_func g_pVoltSampler;

/*
static gpufreq_power_limit_notify g_power_limit_notify;
#ifdef MT_GPUFREQ_INPUT_BOOST
static gpufreq_input_boost_notify g_input_boost_notify;
#endif

void input_boost_CB(unsigned int id) {

    ++g_touch_boost_call_count;
}
*/


static unsigned int pmic_wrap_to_volt_(unsigned int pmic_wrap_value)
{
	unsigned int volt = 0;

	volt = (pmic_wrap_value * 625) + 70000;

	print_debug("@%s: volt = %d\n", __func__, volt);

	/* 1.37500V */
	if (unlikely(volt > 137500)) {
		print_error("@%s: volt > 1.37500v!\n", __func__);
		return 137500;
	}

	return volt;
}

static unsigned int volt_to_pmic_warp_(unsigned int volt)
{
	unsigned int reg_val = 0;

	reg_val = (volt - 70000) / 625;

	print_debug("@%s: reg_val = %d\n", __func__, reg_val);

	if (unlikely(reg_val > 0x7F)) {
		print_error("@%s: reg_val > 0x7F!\n", __func__);
		return 0x7F;
	}

	return reg_val;
}

/************************************************
 * register / unregister GPU input boost notifiction CB
 *************************************************/
void mt_gpufreq_input_boost_notify_registerCB(gpufreq_input_boost_notify pCB)
{
	atomic_inc(&g_touch_boost_register_count);

	/* not in use     */
	/*#ifdef MT_GPUFREQ_INPUT_BOOST */
	/*    g_input_boost_notify = pCB; */
	/*#endif */
}
EXPORT_SYMBOL(mt_gpufreq_input_boost_notify_registerCB);

void mt_gpufreq_power_limit_notify_registerCB(gpufreq_power_limit_notify pCB)
{
	atomic_inc(&g_power_limit_register_count);

	/* not in use     */
	/*TODO connect */
	/*    g_power_limit_notify = pCB; */
}
EXPORT_SYMBOL(mt_gpufreq_power_limit_notify_registerCB);

/************************************************
 * register / unregister set GPU freq CB
 *************************************************/
/* MET */
void mt_gpufreq_setfreq_registerCB(sampler_func pCB)
{
	atomic_inc(&g_freq_register_count);
	g_pFreqSampler = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_setfreq_registerCB);

/************************************************
 * register / unregister set GPU volt CB
 *************************************************/
/* MET */
void mt_gpufreq_setvolt_registerCB(sampler_func pCB)
{
	atomic_inc(&g_volt_register_count);
	g_pVoltSampler = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_setvolt_registerCB);

/*******************
* internal function
********************/
static void copy_row_(struct mtk_gpufreq_info *out, struct mtk_gpufreq_info const *in)
{
	out->freq_khz = in->freq_khz;
	out->volt = in->volt;
	out->power_mW = in->power_mW;
	out->lower_bound = in->lower_bound;
	out->upper_bound = in->upper_bound;
}

/* will call kzalloc() */
/* num must > 0 */
static s32 alloc_table_(struct freq_table **out, u32 const num)
{
	struct freq_table *tmp = NULL;
	size_t table_size = sizeof(*tmp);
	size_t content_size = sizeof(struct mtk_gpufreq_info) * num;

	*out = NULL;

	/* may sleep */
	tmp = kzalloc(table_size + content_size, GFP_KERNEL);

	if (!tmp)
		return -DEFAULT_ERROR;

	tmp->rows = (struct mtk_gpufreq_info *)(((char *)tmp) + table_size);
	tmp->capacity = num;
	tmp->size = 0;
	tmp->input_index = 0;
	tmp->current_index = 0;
	tmp->thermal_max_index = 0;
	tmp->low_battery_volt_max_index = 0;
	tmp->low_battery_volume_max_index = 0;
	tmp->max_index = 0;
	tmp->bottom_index = 0;
	tmp->ptpod_disable_idx = 0;

	print_notice("[GPUFREQ] alloc_table size %zd+%zd = %zd addr:%p %p.\n",
		     table_size, content_size, table_size + content_size, tmp, tmp->rows);

	*out = tmp;

	return 0;
}

static s32 init_table_(struct freq_table *out, struct mtk_gpufreq_info const *freqs, u32 const num)
{
	s32 i = 0;

	if (num > out->capacity)
		return -DEFAULT_ERROR;

	for (i = 0; i < num; ++i)
		copy_row_(&out->rows[i], &freqs[i]);

	out->size = num;
	out->input_index = 0;
	out->current_index = 0;
	out->thermal_max_index = 0;
	out->low_battery_volt_max_index = 0;
	out->low_battery_volume_max_index = 0;
	out->max_index = 0;
	out->bottom_index = num - 1;
	out->ptpod_disable_idx = 0;

	return 0;
}

#if 0
static struct freq_table *swap_table_(struct freq_table *new_table)
{
	struct freq_table *old = NULL;

	LOCK_TABLE();
	old = g_freq_table;
	g_freq_table = new_table;
	UNLOCK_TABLE();

	return old;
}
#endif

static void swap_2_table_(struct freq_table *new_table,
			  struct freq_table *new_backup, struct freq_table **old,
			  struct freq_table **old_backup)
{
	*old = NULL;
	*old_backup = NULL;

	LOCK_TABLE();
	*old = g_freq_table;
	*old_backup = g_backup_table;
	g_freq_table = new_table;
	g_backup_table = new_backup;
	UNLOCK_TABLE();
}

static s32 release_table_(struct freq_table *out)
{
	if ((out == &g_default_table) || (!out))
		return 0;


	kfree(out);
	return 0;
}

static u32 calc_max_index_(bool const thermal_switch, u32 const thermal_index,
			   bool const volt_switch, u32 const volt_index,
			   bool const volume_switch, u32 const volume_index)
{
	u32 max_index = 0;

	if ((thermal_switch) && (max_index < thermal_index))
		max_index = thermal_index;


	if ((volt_switch) && (max_index < volt_index))
		max_index = volt_index;


	if ((volume_switch) && (max_index < volume_index))
		max_index = volume_index;


	return max_index;
}

/* will set current_index */
static void get_row_by_load_(u32 const load, struct mtk_gpufreq_info *out_row)
{
	s32 i = 0;
	u32 input_index = 0;
	u32 current_index = 0;
	u32 size = 0;
	bool thermal_switch = 0;
	bool volt_switch = 0;
	bool volume_switch = 0;
	u32 thermal_max_index = 0;
	u32 volt_max_index = 0;
	u32 volume_max_index = 0;
	u32 max_index = 0;
	u32 bottom_index = 0;
	struct mtk_gpufreq_info const *rows = NULL;
	struct mtk_gpufreq_info const *row;

	LOCK_TABLE();
	rows = g_freq_table->rows;
	size = g_freq_table->size;
	thermal_switch = g_thermal_switch;
	volt_switch = g_low_battery_volt_switch;
	volume_switch = g_low_battery_volume_switch;
	thermal_max_index = g_freq_table->thermal_max_index;
	volt_max_index = g_freq_table->low_battery_volt_max_index;
	volume_max_index = g_freq_table->low_battery_volume_max_index;
	bottom_index = g_freq_table->bottom_index;


	/* Decide max index begin */
	max_index = calc_max_index_(thermal_switch, thermal_max_index,
				    volt_switch, volt_max_index, volume_switch, volume_max_index);

	g_freq_table->max_index = max_index;
	/* Decide max index end */

	/* max_index is from thermal. It has higher priority. */
	if (bottom_index < max_index)
		bottom_index = max_index;


	input_index = size - 1;
	for (i = 0; i < size; ++i) {
		row = &rows[i];
		if (row->lower_bound <= load) {
			input_index = i;
			break;
		}
	}

	current_index = input_index;

	if (current_index < max_index)
		current_index = max_index;


	if (current_index > bottom_index)
		current_index = bottom_index;


	g_freq_table->input_index = input_index;
	g_freq_table->current_index = current_index;
	copy_row_(out_row, &rows[current_index]);
	UNLOCK_TABLE();
}

/* will set current_index */
static void get_row_by_index_(u32 index, struct mtk_gpufreq_info *out_row)
{
	u32 size = 0;
	bool thermal_switch = 0;
	bool volt_switch = 0;
	bool volume_switch = 0;
	u32 thermal_max_index = 0;
	u32 volt_max_index = 0;
	u32 volume_max_index = 0;
	u32 max_index = 0;
	u32 bottom_index = 0;
	struct mtk_gpufreq_info const *rows = NULL;

	LOCK_TABLE();
	rows = g_freq_table->rows;
	size = g_freq_table->size;
	thermal_switch = g_thermal_switch;
	volt_switch = g_low_battery_volt_switch;
	volume_switch = g_low_battery_volume_switch;
	thermal_max_index = g_freq_table->thermal_max_index;
	volt_max_index = g_freq_table->low_battery_volt_max_index;
	volume_max_index = g_freq_table->low_battery_volume_max_index;
	bottom_index = g_freq_table->bottom_index;

	/* Decide max index begin */
	max_index = calc_max_index_(thermal_switch, thermal_max_index,
				    volt_switch, volt_max_index, volume_switch, volume_max_index);

	g_freq_table->max_index = max_index;
	/* Decide max index end */

	/* max_index is from thermal. It has higher priority. */
	if (bottom_index < max_index)
		bottom_index = max_index;


	if (index >= size)
		index = size - 1;


	if (index < max_index)
		index = max_index;


	if (index > bottom_index)
		index = bottom_index;


	g_freq_table->input_index = index;
	g_freq_table->current_index = index;
	copy_row_(out_row, &rows[index]);
	UNLOCK_TABLE();
}

/* Will lock freq volt */
static void set_freq_volt_(u32 const freq_khz, u32 const volt)
{
	LOCK_FREQ_VOLT();
	g_freq_khz = freq_khz;
	g_volt = volt;
	UNLOCK_FREQ_VOLT();
}

static void set_cur_load_(u32 const load)
{
	struct mtk_gpufreq_info row;
	u32 enable_count = atomic_read(&g_dvfs_enable_count);
	++g_set_load_call_count;

	g_load = load;

	/* will lock  */
	get_row_by_load_(load, &row);

#if GPUFREQ_PTPOD
	/* ptpod override here */
	if (g_ptpod_switch == false) {
		row.freq_khz = GPU_DVFS_PTPOD_DISABLE_FREQ;
		row.volt = GPU_DVFS_PTPOD_DISABLE_VOLT;
	}
#endif

	if (enable_count > 0)
		set_freq_volt_(row.freq_khz, row.volt);

}

static void set_by_cur_index_(u32 const index)
{
	struct mtk_gpufreq_info row;
	u32 enable_count = atomic_read(&g_dvfs_enable_count);
	++g_set_index_call_count;

	/* will lock  */
	get_row_by_index_(index, &row);

#if GPUFREQ_PTPOD
	/* ptpod override here */
	if (g_ptpod_switch == false) {
		row.freq_khz = GPU_DVFS_PTPOD_DISABLE_FREQ;
		row.volt = GPU_DVFS_PTPOD_DISABLE_VOLT;
	}
#endif

	if (enable_count > 0)
		set_freq_volt_(row.freq_khz, row.volt);

}

/* By isolating this function, we have chance to use other information to determine row */
/* return limit_index  */
static u32 find_thermal_index_(struct freq_table const *table, u32 const limited_power)
{
	int i = 0;
	u32 index = 0;
	struct mtk_gpufreq_info *rows = table->rows;
	u32 size = table->size;

	/* not set -> no limit */
	if (limited_power == 0) {
		index = 0;
	} else {
		index = size - 1;

		/* determine row  */
		for (i = 0; i < size; ++i) {
			if (rows[i].power_mW <= limited_power) {
				index = i;
				break;
			}
		}
	}

	return index;
}

/* voltage upper bound */
u32 find_bottom_index_(struct freq_table const *table, u32 const bottom_freq)
{
	u32 i = 0;
	struct mtk_gpufreq_info *rows = table->rows;
	u32 const size = table->size;
	u32 index = size - 1;

	/* determine row */
	for (i = 0; i < size; ++i) {
		if (rows[i].freq_khz <= bottom_freq) {
			index = i;
			break;
		}
	}

	return index;
}

/* voltage upper bound, min_freq */
u32 find_ptpod_disable_index_(struct freq_table const *table, u32 const freq)
{
	s32 i = 0;
	struct mtk_gpufreq_info *rows = table->rows;
	u32 const size = table->size;
	u32 index = size - 1;
	/*u32 freqFound = 0; */

	/* determine row */
	for (i = 0; i < size; ++i) {
		if (rows[i].freq_khz <= freq) {
			index = i;
			break;
		}
	}


	return index;
}

static void thermal_table_register_(void)
{
#ifdef CONFIG_THERMAL
	u32 size = 0;
	u32 i = 0;
	struct mtk_gpufreq_info *rows = 0;

	LOCK_TABLE();
	size = g_freq_table->size;
	size = (size <= 10) ? size : 10;
	rows = g_freq_table->rows;
	for (i = 0; i < size; ++i) {
		g_power_table[i].gpufreq_khz = rows[i].freq_khz;
		g_power_table[i].gpufreq_power = rows[i].power_mW;
	}
	UNLOCK_TABLE();

	mtk_gpufreq_register(g_power_table, (int)size);
#endif
}

/* reserved for future need  */
static void notify_thermal_module_(u32 const freq_khz, u32 const volt)
{
	g_fake_freq_to_thermal = freq_khz;
	g_fake_volt_to_thermal = volt;

	/* write to thermal module if needed  */
}

static void update_max_index_(u32 *max_index, u32 sub_max_index)
{
	if (*max_index < sub_max_index)
		*max_index = sub_max_index;
}


/*****************
* extern function
******************/
static int mtk_gpufreq_initialize_proc_(void);
static void mtk_gpufreq_initialize_met_(void);
#ifdef MT_GPUFREQ_INPUT_BOOST
static struct input_handler input_handler_;
#endif

int mtk_gpufreq_initialize(void)
{
	int error = 0;
#ifdef MT_GPUFREQ_INPUT_BOOST
	int rc = 0;
#endif

#if defined(GS_GPUFREQ_BRING_UP)
	struct device_node *node;

	/*spm_cpu */
	node = of_find_compatible_node(NULL, NULL, "mediatek,SLEEP");
	if (!node)
		print_notice("[GPUFREQ] SLEEP find node failed\n");

	spm_cpu_base = of_iomap(node, 0);
	if (!spm_cpu_base)
		print_notice("[GPUFREQ] spm_spu_base base failed\n");
	else
		print_notice("[GPUFREQ] spm_spu_base base: %p\n", spm_cpu_base);


	/*infracfg_ao         */
	node = of_find_compatible_node(NULL, NULL, "mediatek,INFRACFG_AO");
	if (!node)
		print_notice("[GPUFREQ] CLK_INFRACFG_AO find node failed\n");

	clk_infracfg_ao_base = of_iomap(node, 0);
	if (!clk_infracfg_ao_base)
		print_notice("[GPUFREQ] CLK_INFRACFG_AO base failed\n");
	else
		print_notice("[GPUFREQ] CLK_INFRACFG_AO base: %p\n", clk_infracfg_ao_base);


	/*apmixed */
	node = of_find_compatible_node(NULL, NULL, "mediatek,APMIXED");
	if (!node)
		print_notice("[GPUFREQ] CLK_APMIXED find node failed\n");

	clk_apmixed_base = of_iomap(node, 0);
	if (!clk_apmixed_base)
		print_notice("[GPUFREQ] CLK_APMIXED base failed\n");
	else
		print_notice("[GPUFREQ] CLK_APMIXED base: %p\n", clk_apmixed_base);


	/*cksys_base */
	node = of_find_compatible_node(NULL, NULL, "mediatek,TOPCKGEN");
	if (!node)
		print_notice("[CLK_CKSYS] find node failed\n");

	clk_cksys_base = of_iomap(node, 0);
	if (!clk_cksys_base)
		print_notice("[GPUFREQ] CLK_CKSYS base failed\n");
	else
		print_notice("[GPUFREQ] CLK_CKSYS base: %p\n", clk_cksys_base);

#endif

	mtk_gpufreq_initialize_met_();
	error = mtk_gpufreq_initialize_proc_();

#ifdef MT_GPUFREQ_INPUT_BOOST
	rc = input_register_handler(&input_handler_);
#endif

	return error;
}
EXPORT_SYMBOL(mtk_gpufreq_initialize);

s32 gs_gpufreq_register(struct mtk_gpufreq_info *rows, u32 const num)
{
	s32 error = 0;
	s32 error2 = 0;
	struct freq_table *new_backup = NULL;
	struct freq_table *new_table = NULL;
	struct mtk_gpufreq_info *new_rows = NULL;
	struct freq_table *old = NULL;
	struct freq_table *old_backup = NULL;
	struct mtk_gpufreq_info row;
	u32 thermal_max_index = 0;
	u32 max_index = 0;
	u32 bottom_freq = 0;
	u32 bottom_index = 0;
	/*u32 ptpod_index  = 0; */
	u32 limited_power_mW = 0;

	atomic_inc(&g_table_register_count);

	if (!num) {
		error = -DEFAULT_ERROR;
		print_error("[GPUFREQ] register failed invalid num.\n");
		return error;
	}

	error = alloc_table_(&new_table, num);
	error2 = alloc_table_(&new_backup, num);

	if (error || error2) {
		if (error) {
			kfree(new_table);
			new_table = NULL;
		}
		if (error2) {
			kfree(new_backup);
			new_backup = NULL;
		}
		print_error("[GPUFREQ] register failed in allocation.\n");
		return error | error2;
	}

	error = init_table_(new_table, rows, num);
	error2 = init_table_(new_backup, rows, num);
	if (error || error2) {
		kfree(new_table);
		kfree(new_backup);
		new_table = NULL;
		new_backup = NULL;
		print_error("[GPUFREQ] register failed in initization.\n");
		return error | error2;
	}

	new_rows = new_backup->rows;

	/* max index */
	/* (1) thermal  */
	limited_power_mW = g_limited_power_mW;
	thermal_max_index = find_thermal_index_(new_backup, limited_power_mW);
	copy_row_(&row, &new_rows[thermal_max_index]);
	new_table->thermal_max_index = thermal_max_index;
	update_max_index_(&max_index, thermal_max_index);

	/* (2) low battery volt */
	/* TODO */
	new_table->low_battery_volt_max_index = 0;

	/* (3) low battery volume */
	/* TODO */
	new_table->low_battery_volume_max_index = 0;

	/* bottom_index */
	bottom_freq = g_bottom_freq;
	bottom_index = find_bottom_index_(new_backup, bottom_freq);
	new_table->bottom_index = bottom_index;

	/* ptpod_disable_idx */
#if 0
	ptpod_index = find_ptpod_disable_index_(new_backup, GPU_DVFS_PTPOD_DISABLE_FREQ);
	new_table->ptpod_disable_idx = ptpod_index;
#endif

	g_reg_finished = false;

	swap_2_table_(new_table, new_backup, &old, &old_backup);

	release_table_(old);
	release_table_(old_backup);
	old = NULL;
	old_backup = NULL;

	set_freq_volt_(row.freq_khz, row.volt);

	thermal_table_register_();

	g_reg_finished = true;

	notify_thermal_module_(row.freq_khz, row.volt);

	return 0;
}
EXPORT_SYMBOL(gs_gpufreq_register);

void gs_gpufreq_unregister(void)
{
	struct freq_table *old = NULL;
	struct freq_table *old_backup = NULL;
	struct mtk_gpufreq_info *default_row = NULL;
	int rejected;

	g_reg_finished = false;
	atomic_inc(&g_table_unregister_count);

	LOCK_TABLE();
	rejected = (g_freq_table == &g_default_table);
	old = g_freq_table;
	old_backup = g_backup_table;
	g_freq_table = &g_default_table;
	g_backup_table = &g_default_table;
	UNLOCK_TABLE();

	if (!rejected) {
		default_row = &g_default_table.rows[0];

		/* no need to update min max freq. since only one row */

		thermal_table_register_();

		g_reg_finished = true;

		notify_thermal_module_(default_row->freq_khz, default_row->volt);

		release_table_(old);
		release_table_(old_backup);
	}
}
EXPORT_SYMBOL(gs_gpufreq_unregister);

s32 gs_gpufreq_kmif_register(struct mtk_gpufreq_kmif *ops)
{
	atomic_inc(&g_touch_boost_register_count);
	atomic_inc(&g_power_limit_register_count);
	memcpy(&g_kmif_ops, ops, sizeof(*ops));
	return 0;
}
EXPORT_SYMBOL(gs_gpufreq_kmif_register);

s32 gs_gpufreq_kmif_unregister(void)
{
	atomic_dec(&g_touch_boost_register_count);
	atomic_dec(&g_power_limit_register_count);
	memset(&g_kmif_ops, 0, sizeof(g_kmif_ops));
	return 0;
}
EXPORT_SYMBOL(gs_gpufreq_kmif_unregister);

/************************************************
 * Update enable counter only
 ************************************************/
void gs_gpufreq_dvfs_enable(void)
{
	s32 enable_count = 0;

	/*atomic_inc(&g_dvfs_enable_count); */
	atomic_set(&g_dvfs_enable_count, 1);
	enable_count = atomic_read(&g_dvfs_enable_count);

	if (enable_count > 0)
		set_by_cur_index_(0);


	print_notice("[GPUFREQ] enable DVFS... enable_count:%d\n", enable_count);
}
EXPORT_SYMBOL(gs_gpufreq_dvfs_enable);

void gs_gpufreq_dvfs_disable(void)
{
	s32 enable_count = 0;

	/*atomic_dec(&g_dvfs_enable_count); */
	atomic_set(&g_dvfs_enable_count, 0);
	enable_count = atomic_read(&g_dvfs_enable_count);

	if (enable_count <= 0)
		set_by_cur_index_(0);


	print_notice("[GPUFREQ] disable DVFS... enable_count:%d\n", enable_count);
}
EXPORT_SYMBOL(gs_gpufreq_dvfs_disable);

int mt_gpufreq_state_set(int enabled)
{
	print_notice("[GPUFREQ] state_set(%d)\n", enabled);

	if (enabled)
		gs_gpufreq_dvfs_enable();
	else
		gs_gpufreq_dvfs_disable();

	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_state_set);

static void gs_gpufreq_ptpod_fake_volt(int index)
{
	u32 volts2[][2] = {
		{74, 73},	/* 116250 115625 */
		{81, 77},	/* 120625 118125 */
		{82, 78}	/* 121250 118750 */
	};
	u32 volts3[][3] = {
		{89, 74, 73},	/* 125625 116250 115625 */
		{83, 82, 78}	/* 121875 121250 118750 */
	};
	u32 num = mt_gpufreq_get_dvfs_table_num();

	if ((index < 0) || (index >= 2))
		return;

	switch (num) {
	case 2:
		mt_gpufreq_update_volt(volts2[index], num);
		break;
	case 3:
		mt_gpufreq_update_volt(volts3[index], num);
		break;
	default:
		break;
	};
}

void mt_gpufreq_enable_by_ptpod(void)
{
#if GPUFREQ_PTPOD
	g_ptpod_switch = true;

	print_notice("[GPUFREQ] ptpod switch on\n");
#endif

}
EXPORT_SYMBOL(mt_gpufreq_enable_by_ptpod);

/************************************************
 * disable gpufreq immediately.
 * And replaced by ptp-od voltage
 ************************************************/
void mt_gpufreq_disable_by_ptpod(void)
{
#if GPUFREQ_PTPOD
	/* u32 target_idx = 0; */

	g_ptpod_switch = false;
	print_notice("[GPUFREQ] ptpod switch off - gpufreq disabled.\n");

	set_by_cur_index_(0);
	gs_gpufreq_switch_handler();

	/*target_idx = find_ptpod_disable_index_(g_backup_table, GPU_DVFS_PTPOD_DISABLE_FREQ); */
	/*LOCK_TABLE(); */
	/*g_freq_table->ptpod_disable_idx = target_idx; */
	/*UNLOCK_TABLE(); */

	/*mt_gpufreq_voltage_enable_set(1); */
	/*mt_gpufreq_target(target_idx); */
#endif

}
EXPORT_SYMBOL(mt_gpufreq_disable_by_ptpod);



/* Set internal state for next change. */
extern void gs_gpufreq_set_cur_load(u32 const load)
{
	print_debug("[GPUFREQ] set load %d.\n", load);

	set_cur_load_(load);
}
EXPORT_SYMBOL(gs_gpufreq_set_cur_load);

extern void gs_gpufreq_set_cur_index(u32 const index)
{
	/* print_debug("[GPUFREQ] set index %d.\n", index); */

	set_by_cur_index_(index);

#if GS_GPUFREQ_THERMAL_FAKE
	FAKE_thermal_control(index);
#endif

}
EXPORT_SYMBOL(gs_gpufreq_set_cur_index);

static void gs_gpufreq_volt_switch_(unsigned int volt_old, unsigned int volt_new);
static void gs_gpufreq_clock_switch_(unsigned int freq_new);

/* Executing real change based on internal states. */
extern void gs_gpufreq_switch_handler(void)
{
	u32 freq_khz = 0;
	u32 volt = 0;
	u32 old_freq = 0;
	u32 old_volt = 0;

	s32 power_enabled = atomic_read(&g_power_switch);

	/* print_debug("[GPUFREQ] switch handler\n"); */

	if (!power_enabled)
		return;

	LOCK_FREQ_VOLT();
	freq_khz = g_freq_khz;
	volt = g_volt;
	old_freq = g_old_freq_khz;
	old_volt = g_old_volt;
	UNLOCK_FREQ_VOLT();

	/*freq_khz = 549250; */
	/*    volt = 1250; */

	notify_thermal_module_(freq_khz, volt);


	/* volt rise before freq */
	if (volt > old_volt)
		gs_gpufreq_volt_switch_(old_volt, volt);

	if (freq_khz != old_freq)
		gs_gpufreq_clock_switch_(freq_khz);

	/* volt fall before freq */
	if (volt < old_volt)
		gs_gpufreq_volt_switch_(old_volt, volt);

	LOCK_FREQ_VOLT();
	g_old_freq_khz = freq_khz;
	g_old_volt = volt;
	g_freq_khz = freq_khz;
	g_volt = volt;
	g_fake_freq_khz = freq_khz;
	g_fake_volt = volt;
	UNLOCK_FREQ_VOLT();

	/*
	   print_error("[GPUFREQ] switch_handler : (freq, volt) = (%d, %d)", g_fake_freq_khz, g_fake_volt);
	 */
}
EXPORT_SYMBOL(gs_gpufreq_switch_handler);

int mt_gpufreq_target(unsigned int idx)
{
	print_notice("[GPUFREQ] target(%d)\n", idx);

	gs_gpufreq_set_cur_index((u32) idx);
	gs_gpufreq_switch_handler();
	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_target);

extern void gs_gpufreq_mtcmos_on(void)
{
#if GS_GPUFREQ_PATHB_MTCMOS
	if (!mfg_dev || !g3d_dev)
		return;
	pm_runtime_get_sync(&g3d_dev->dev);
	pm_runtime_get_sync(&mfg_dev->dev);
	clk_prepare_enable(clk_mm);
	MFG_WRITE32(0x1, MFG_CG_CLR);
	atomic_set(&g_mtcmos_enable_count, 1);
#endif

#if defined(GS_GPUFREQ_BRING_UP) && !GS_GPUFREQ_PATHB_MTCMOS
	spm_mtcmos_ctrl_mfg_ASYNC(STA_POWER_ON);
	spm_mtcmos_ctrl_mfg(STA_POWER_ON);
	atomic_set(&g_mtcmos_enable_count, 1);
#endif

}
EXPORT_SYMBOL(gs_gpufreq_mtcmos_on);

extern void gs_gpufreq_mtcmos_off(void)
{
#if GS_GPUFREQ_PATHB_MTCMOS
	if (!mfg_dev || !g3d_dev)
		return;
	MFG_WRITE32(0x1, MFG_CG_SET);
	clk_disable_unprepare(clk_mm);
	pm_runtime_put_sync(&mfg_dev->dev);
	pm_runtime_put_sync(&g3d_dev->dev);
	atomic_set(&g_mtcmos_enable_count, 0);
#endif

#if defined(GS_GPUFREQ_BRING_UP) && !GS_GPUFREQ_PATHB_MTCMOS
	spm_mtcmos_ctrl_mfg(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mfg_ASYNC(STA_POWER_DOWN);
	atomic_set(&g_mtcmos_enable_count, 0);
#endif

}
EXPORT_SYMBOL(gs_gpufreq_mtcmos_off);

/* NOT in use */
/* disable power */
extern void gs_gpufreq_vgpu_disable(void)
{
	u32 volt = 0;

#if GS_GPUFREQ_PATHB_POWER
	u32 delay = 0;
	u32 reg_val = 0;
#endif

	print_notice("[GPUFREQ] disable vgpu\n");

	atomic_dec(&g_power_switch);

	++g_fake_vgpu_disable_called;

	LOCK_FREQ_VOLT();
	volt = g_volt;
	UNLOCK_FREQ_VOLT();

#if defined(GS_GPUFREQ_BRING_UP)
	/* do nothing */
#endif

#if GS_GPUFREQ_PATHB_POWER
	pmic_config_interface(PMIC_ADDR_VGPU_EN, 0x0, 0x1, 0x0);	/* Set VGPU_EN[0] to 0 */
	delay = mt_gpufreq_calc_pmic_settle_time(0, g_volt);	/*(g_volt / 1250) + 26; */
	udelay(delay);
	pmic_read_interface(PMIC_ADDR_VGPU_EN, &reg_val, 0x1, 0x0);
#endif

	LOCK_FREQ_VOLT();
	g_old_volt = 0;
	g_volt = 0;
	g_fake_volt = 0;
	UNLOCK_FREQ_VOLT();
}
EXPORT_SYMBOL(gs_gpufreq_vgpu_disable);

/* NOT in use */
/* enable power */
extern void gs_gpufreq_vgpu_sel_default(void)
{
#if GS_GPUFREQ_PATHB_POWER
	u32 delay = 0;
	u32 reg_val = 0;
#endif

	print_notice("[GPUFREQ] vgpu sel default\n");

	atomic_inc(&g_power_switch);

	++g_fake_vgpu_sel_default_called;


#if defined(GS_GPUFREQ_BRING_UP)
	/* do nothing */
#endif

#if GS_GPUFREQ_PATHB_POWER
	pmic_config_interface(PMIC_ADDR_VGPU_EN, 0x1, 0x1, 0x0);	/* Set VGPU_EN[0] to 1 */
	delay = mt_gpufreq_calc_pmic_settle_time(0, g_volt);	/*(g_volt / 1150) + 26; */
	udelay(delay);
	pmic_read_interface(PMIC_ADDR_VGPU_EN, &reg_val, 0x1, 0x0);
#endif

	LOCK_FREQ_VOLT();
	g_old_volt = GPU_POWER_VGPU_1_15V;
	g_volt = GPU_POWER_VGPU_1_15V;
	g_fake_volt = GPU_POWER_VGPU_1_15V;
	UNLOCK_FREQ_VOLT();
}
EXPORT_SYMBOL(gs_gpufreq_vgpu_sel_default);

int mt_gpufreq_voltage_enable_set(unsigned int enable)
{
	print_notice("[GPUFREQ] voltage_enable_set(%d)\n", enable);

	if (enable)
		gs_gpufreq_vgpu_sel_default();
	else
		gs_gpufreq_vgpu_disable();

	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_voltage_enable_set);


/************************************************
 * Modify voltage column in the table
 ************************************************/
unsigned int mt_gpufreq_update_volt(unsigned int pmic_volt[], unsigned int array_size)
{
#if GPUFREQ_PTPOD
	u32 i = 0;
	u32 reject = 0;
	u32 error = 0;
	u32 volt = 0;
	u32 num_rows = 0;

	atomic_inc(&g_update_volt_count);
	print_notice("[GPUFREQ] update_volt.\n");

	LOCK_TABLE();
	num_rows = g_freq_table->size;
	if (array_size > num_rows) {
		error |= 1;
		reject = 1;
	}
	if (g_freq_table == &g_default_table) {
		error |= 2;
		reject = 1;
	}

	if (!reject) {
		for (i = 0; i < array_size; ++i) {
			volt = pmic_wrap_to_volt_(pmic_volt[i]);
			g_freq_table->rows[i].volt = volt;
			print_debug("@%s: mt_gpufreqs[%d].volt = %x\n", __func__, i, volt);
#if GPUFREQ_PTPOD_DEBUG
			print_error("[GPUFREQ] update_volt:%d, %d\n", i, volt);
#endif
		}
	}
	UNLOCK_TABLE();

	if ((error & 1) > 0)
		print_error
		    ("[GPUFREQ] update volt : input size (%d) is different from current size (%d.\n",
		     array_size, num_rows);


	if ((error & 2) > 0)
		print_error("[GPUFREQ] update volt : Try to update default table.\n");


	/*
	   if (!error) {
	   gs_gpufreq_switch_handler();
	   }
	 */

#endif

	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_update_volt);

/************************************************
 * Unmodify voltage column in the table
 ************************************************/
void mt_gpufreq_restore_default_volt(void)
{
	u32 error = 0;
	u32 reject = 0;
	u32 i = 0;
	u32 size = 0;

	print_notice("[GPUFREQ] restore volt.\n");

	LOCK_TABLE();
	if (g_freq_table->size != g_backup_table->size) {
		error = 1;
		reject = 1;
	}
	if (g_freq_table == &g_default_table)
		reject = 1;

	if (!reject) {
		size = g_freq_table->size;
		for (i = 0; i < size; ++i)
			g_freq_table->rows[i].volt = g_backup_table->rows[i].volt;
	}
	UNLOCK_TABLE();

	if (error == 1)
		print_error("[GPUFREQ] restore volt : data inconsistent!\n");

	/*
	   if (!error) {
	   gs_gpufreq_switch_handler();
	   }
	 */

}
EXPORT_SYMBOL(mt_gpufreq_restore_default_volt);

/* THERMAL */
/* Set the max power which shouldn't exceed. */
/* call when thermal or table is changed. */
void mt_gpufreq_thermal_protect(unsigned int const limited_power)
{
	u32 limit_index = 0;
	u32 input_index = 0;
	s32 dvfs_enable_count = atomic_read(&g_dvfs_enable_count);

	++g_thermal_call_count;

	g_limited_power_mW = limited_power;

	/* disable all sw / hw function first. */
	/* TODO : turn off hw only. */
	if (!g_thermal_switch)
		return;

	if (!g_reg_finished)
		return;

	LOCK_TABLE();
	limit_index = find_thermal_index_(g_freq_table, limited_power);
	g_freq_table->thermal_max_index = limit_index;
	input_index = g_freq_table->input_index;
	UNLOCK_TABLE();

	gs_gpufreq_set_cur_index(input_index);
	if (dvfs_enable_count > 0)
		gs_gpufreq_switch_handler();

	if (g_kmif_ops.notify_thermal_event != NULL)
		g_kmif_ops.notify_thermal_event();

}
EXPORT_SYMBOL(mt_gpufreq_thermal_protect);

void gs_gpufreq_bottom_speed(u32 const bottom_freq)
{
	u32 bottom_index = 0;
	u32 input_index = 0;
	s32 dvfs_enable_count = atomic_read(&g_dvfs_enable_count);

	g_bottom_freq = bottom_freq;

	LOCK_TABLE();
	bottom_index = find_bottom_index_(g_freq_table, bottom_freq);
	g_freq_table->bottom_index = bottom_index;
	input_index = g_freq_table->input_index;
	UNLOCK_TABLE();

	gs_gpufreq_set_cur_index(input_index);
	if (dvfs_enable_count > 0)
		gs_gpufreq_switch_handler();

}
EXPORT_SYMBOL(gs_gpufreq_bottom_speed);

/* bool */
u32 gs_gpufreq_is_registered(void)
{
	return g_freq_table != &g_default_table;
}
EXPORT_SYMBOL(gs_gpufreq_is_registered);

void gs_gpufreq_get_cur_setting(u32 *freq_khz, u32 *volt)
{
	LOCK_FREQ_VOLT();
	*freq_khz = g_freq_khz;
	*volt = g_volt;
	UNLOCK_FREQ_VOLT();
}
EXPORT_SYMBOL(gs_gpufreq_get_cur_setting);

void gs_gpufreq_get_cur_indices(u32 *bottom_index, u32 *limit_index, u32 *current_index)
{
	static struct freq_table *table;

	LOCK_TABLE();
	table = g_freq_table;
	*bottom_index = table->bottom_index;
	*limit_index = table->max_index;
	*current_index = table->current_index;
	UNLOCK_TABLE();
}
EXPORT_SYMBOL(gs_gpufreq_get_cur_indices);

u32 gs_gpufreq_get_cur_load(void)
{
	return g_load;
}
EXPORT_SYMBOL(gs_gpufreq_get_cur_load);

/* No need to lock, since no use for locking */
s32 gs_gpufreq_get_state(void)
{				/* bool */
	s32 enable_count = atomic_read(&g_dvfs_enable_count);
	return enable_count;
}
EXPORT_SYMBOL(gs_gpufreq_get_state);

struct proc_dir_entry *gs_gpufreq_get_proc_dir(void)
{
	return g_proc_dir;
}
EXPORT_SYMBOL(gs_gpufreq_get_proc_dir);

unsigned int mt_gpufreq_get_thermal_limit_index(void)
{
	u32 thermal_max_index = 0;

	LOCK_TABLE();
	thermal_max_index = g_freq_table->thermal_max_index;
	UNLOCK_TABLE();

	print_debug("current GPU thermal limit index is %d\n", thermal_max_index);
	return thermal_max_index;
}
EXPORT_SYMBOL(mt_gpufreq_get_thermal_limit_index);

unsigned int mt_gpufreq_get_thermal_limit_freq(void)
{
	u32 index = 0;
	u32 thermal_max_freq = 0;

	LOCK_TABLE();
	index = g_freq_table->thermal_max_index;
	thermal_max_freq = g_freq_table->rows[index].freq_khz;
	UNLOCK_TABLE();

	print_debug("current GPU thermal limit freq is %d\n", thermal_max_freq);
	return thermal_max_freq;
}
EXPORT_SYMBOL(mt_gpufreq_get_thermal_limit_freq);

unsigned int mt_gpufreq_get_cur_freq_index(void)
{
	u32 index = 0;

	LOCK_TABLE();
	index = g_freq_table->current_index;
	UNLOCK_TABLE();

	print_debug("current GPU frequency OPP index is %d\n", index);
	return index;
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq_index);

/* MET */
unsigned int mt_gpufreq_get_cur_freq(void)
{
	u32 freq = g_fake_freq_khz;

	print_debug("current GPU frequency is %d kHz\n", freq);

	return freq;
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq);

/* MET */
unsigned int mt_gpufreq_get_cur_volt(void)
{

#if 0
	u32 volt = g_fake_volt;

	print_debug("current GPU voltage is %d\n", volt);

	if (unlikely(volt < 50))
		return 0;

	return volt * 100 - 50;
#else
	return pmic_wrap_to_volt_(upmu_get_ni_vproc_vosel());
#endif
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_volt);

unsigned int mt_gpufreq_get_dvfs_table_num(void)
{
	u32 num = 0;

	LOCK_FREQ_VOLT();
	num = g_freq_table->size;
	UNLOCK_FREQ_VOLT();

	return num;
}
EXPORT_SYMBOL(mt_gpufreq_get_dvfs_table_num);

unsigned int mt_gpufreq_get_freq_by_idx(unsigned int idx)
{
	u32 size = 0;
	u32 freq = 0;

	LOCK_TABLE();
	size = g_freq_table->size;
	if (idx < size)
		freq = g_freq_table->rows[idx].freq_khz;

	UNLOCK_TABLE();

	if (idx < size) {
		print_debug("@%s: idx = %d, frequency= %d\n", __func__, idx, freq);
		return freq;
	}

	print_debug("@%s: idx = %d, NOT found! return 0!\n", __func__, idx);
	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_get_freq_by_idx);




#ifdef MT_GPUFREQ_INPUT_BOOST
/*****************************************************************************
 * Input Boost                                                               *
 *****************************************************************************/
static void _mt_gpufreq_input_event(struct input_handle *handle, unsigned int type,
				    unsigned int code, int value)
{
	if ((type == EV_KEY) && (code == BTN_TOUCH) && (value == 1)) {
		print_notice("@%s: accept.\n", __func__);

		++g_touch_boost_call_count;

		/* 0x8163 is the same as g3d */
		met_tag_oneshot(0x8163, "Touch", 1);
		met_tag_oneshot(0x8163, "Touch", 0);

		if (g_kmif_ops.notify_touch_event != NULL)
			g_kmif_ops.notify_touch_event();

		/*        wake_up_process(mt_gpufreq_up_task); */
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

static struct input_handler input_handler_ = {
	.event = _mt_gpufreq_input_event,
	.connect = _mt_gpufreq_input_connect,
	.disconnect = _mt_gpufreq_input_disconnect,
	.name = "gpufreq_ib",
	.id_table = mt_gpufreq_ids,
};
#endif








/*****************************************************************************
 * MET                                                                       *
 *****************************************************************************/
static unsigned int get_gpu_loading_met_(void)
{
	u32 ret = 42;
	/* guard for concurrency */
	u32 (*fp)(void) = g_kmif_ops.get_loading_stat;

	if (fp)
		ret = fp();

	return ret;
};

static unsigned int get_gpu_block_met_(void)
{
	u32 ret = 43;
	u32 (*fp)(void) = g_kmif_ops.get_block_stat;

	if (fp)
		ret = fp();

	return ret;
};

static unsigned int get_gpu_idle_met_(void)
{
	u32 ret = 44;
	u32 (*fp)(void) = g_kmif_ops.get_idle_stat;

	if (fp)
		ret = fp();

	return ret;
};

static unsigned int get_gpu_power_loading_met_(void)
{
	u32 ret = 45;
	u32 (*fp)(void) = g_kmif_ops.get_power_loading_stat;

	if (fp)
		ret = fp();

	return ret;
};

extern unsigned int (*mtk_get_gpu_loading_fp)(void);
extern unsigned int (*mtk_get_gpu_block_fp)(void);
extern unsigned int (*mtk_get_gpu_idle_fp)(void);
extern unsigned int (*mtk_get_gpu_power_loading_fp)(void);

static void mtk_gpufreq_initialize_met_(void)
{
	if (1) {
		mtk_get_gpu_loading_fp = get_gpu_loading_met_;
		mtk_get_gpu_block_fp = get_gpu_block_met_;
		mtk_get_gpu_idle_fp = get_gpu_idle_met_;
		mtk_get_gpu_power_loading_fp = get_gpu_power_loading_met_;
	}
}


/*****************************************************************************
 * HW written                                                                *
 *****************************************************************************/
static void gs_gpufreq_clock_switch_(unsigned int freq_new)
{
#if GS_GPUFREQ_PATHB_FREQ
	_mt_gpufreq_set_cur_freq(freq_new);
#endif

	/* write to hw from g_freq_khz, g_volt */
#if defined(GS_GPUFREQ_BRING_UP) && !GS_GPUFREQ_PATHB_FREQ
	{
		unsigned int mmpll_n_info_chg;
		unsigned int mmpll_n_info;
		unsigned int mmpll_pdiv;

#if 0
		/*mux enable */
		gs_writel(CLK_CFG_1 + 8, 0x80000000);	/*write clr reg */
		gs_writel(CLK_CFG_UPDATE, 0x00000080);	/*update */
		/*mux select */
		gs_writel(CLK_CFG_1 + 8, 0x03000000);	/*write clr reg */
		gs_writel(CLK_CFG_1 + 4, 0x01000000);	/*write set reg */
		gs_writel(CLK_CFG_UPDATE, 0x00000080);	/*update */

		/*pll power-on */
		gs_writel(MMPLL_PWR_CON0, 0x00000003);	/*[0]MMPLL_PWR_ON=1 */
		udelay(1);
		gs_writel(MMPLL_PWR_CON0, 0x00000001);	/*[1]MMPLL_ISO_EN=0 */
#endif

		mmpll_n_info_chg = 0x80000000;
		mmpll_pdiv = 0x02000000;
		mmpll_n_info = 0x000EA000;	/*default 380.25mhz */
		/*pll frequency select */
		if ((freq_new >= 279500) && (freq_new <= 578500)) {
			mmpll_n_info = 0x000AC000 + ((freq_new - 279500) * 4 / 13000) * 0x2000;

		} else {
			print_error("@%s: target freq_new(%d) out of range!\n", __func__, freq_new);
			BUG();
		}
		print_debug("@%s: freq_new = %d, mmpll_n_info = 0x%x\n", __func__, freq_new,
			    mmpll_n_info);

		/*set MMPLL_N_INFO_CHG, MMPLL_PDIV, MMPLL_N_INFO */
		gs_writel(MMPLL_CON1, mmpll_n_info_chg | mmpll_pdiv | mmpll_n_info);
		gs_writel(MMPLL_CON0, 0x00000101);	/*[0]MMPLL_EN=1 */

		udelay(20);
	}
#else
	udelay(20);
#endif

	LOCK_FREQ_VOLT();
	g_fake_freq_khz = freq_new;
	UNLOCK_FREQ_VOLT();

	if (g_pFreqSampler)
		g_pFreqSampler(mt_gpufreq_get_cur_freq());
}

static void gs_gpufreq_noncache_dummy_read(void)
{
	print_info("@%s: [Before] noncache_dummy_src[0] = %d, noncache_dummy_dst[0] = %d\n",
		   __func__, noncache_dummy_src[0], noncache_dummy_dst[0]);
	memcpy(((void *)noncache_dummy_dst), ((const void *)noncache_dummy_src), g_dummy_read);
	print_info("@%s: [After] noncache_dummy_src[0] = %d, noncache_dummy_dst[0] = %d\n",
		   __func__, noncache_dummy_src[0], noncache_dummy_dst[0]);
}

static void gs_gpufreq_volt_switch_(unsigned int volt_old, unsigned int volt_new)
{
	u32 delay = 1000;
#if GS_GPUFREQ_PATHB_VOLT
	u32 val = 0;
#endif

	print_debug("@%s: volt_new = %d\n", __func__, volt_new);

#if GS_GPUFREQ_PATHB_VOLT
	val = 0x00000048;	/*default 1.15mv */
	/*pmic voltage select */
	if (likely((volt_new >= GPU_POWER_VGPU_MIN) && (volt_new <= GPU_POWER_VGPU_MAX))) {
		if (volt_new > volt_old) {
			while (volt_new - volt_old > 1250) {
				volt_old += 1250;
				val = volt_to_pmic_warp_(volt_old);
				upmu_set_vproc_vosel(val);
				upmu_set_vproc_vosel_on(val);
				udelay(g_dvs_delay);
				if (late_init_done == 1 && g_dummy_read > 0)
					gs_gpufreq_noncache_dummy_read();
			}
		} else {
			while (volt_old - volt_new > 1250) {
				volt_old -= 1250;
				val = volt_to_pmic_warp_(volt_old);
				upmu_set_vproc_vosel(val);
				upmu_set_vproc_vosel_on(val);
				udelay(g_dvs_delay);
				if (late_init_done == 1 && g_dummy_read > 0)
					gs_gpufreq_noncache_dummy_read();
			}
		}
		val = volt_to_pmic_warp_(volt_new);
	} else {
		print_error("@%s: target volt_new(%d) out of range!\n", __func__, volt_new);
		return;
	}
	print_debug("@%s: volt_new = %d, pmic_vproc_con9 = 0x%x\n", __func__, volt_new, val);

	upmu_set_vproc_vosel(val);
	upmu_set_vproc_vosel_on(val);
	udelay(g_dvs_delay);
	if (late_init_done == 1 && g_dummy_read > 0)
		gs_gpufreq_noncache_dummy_read();

#if GPUFREQ_PTPOD_DEBUG
	print_error("[GPUFREQ] volt_switch : %d, 0x%x\n", volt_new, val);
#endif

#endif

#if defined(GS_GPUFREQ_BRING_UP) && !GS_GPUFREQ_PATHB_VOLT
	{
		unsigned int val;
		u32 reg_val = 0;

		val = 0x00000048;	/*default 1.15mv */
		/*pmic voltage select */
		if ((volt_new >= GPU_POWER_VGPU_MIN) && (volt_new <= GPU_POWER_VGPU_MAX))
			val = volt_to_pmic_warp_(volt_new);
		else
			print_error("@%s: target volt_new(%d) out of range!\n", __func__, volt_new);

		print_debug("@%s: volt_new = %d, pmic_vproc_con9 = 0x%x\n", __func__, volt_new,
			    val);

		upmu_set_vproc_vosel(val);
		upmu_set_vproc_vosel_on(val);

		pmic_config_interface(0x21e, pmic_vproc_con9, 0x7f, 0x0);
		pmic_read_interface(0x21e, &reg_val, 0x7f, 0x0);
		print_debug("[PMIC_VPROC] VPROC_CON9 setting 0x%x, should be 0x%x\n", reg_val,
			    pmic_vproc_con9);
		pmic_config_interface(0x220, pmic_vproc_con9, 0x7f, 0x0);
		pmic_read_interface(0x220, &reg_val, 0x7f, 0x0);
		print_debug("[PMIC_VPROC] VPROC_CON10 setting 0x%x, should be 0x%x\n", reg_val,
			    pmic_vproc_con9);

		delay = 1000;
	}
#endif

	if (volt_new > volt_old) {
		/*delay = mt_gpufreq_calc_pmic_settle_time(volt_old, volt_new); */
		print_debug("@%s: delay = %d\n", __func__, delay);
		udelay(delay);
	}

	LOCK_FREQ_VOLT();
	g_fake_volt = volt_new;
	UNLOCK_FREQ_VOLT();

	if (g_pVoltSampler)
		g_pVoltSampler(mt_gpufreq_get_cur_volt());
}

void gs_gpufreq_fake_get_cur_setting(u32 *freq_khz, u32 *volt)
{
	*freq_khz = g_fake_freq_khz;
	*volt = g_fake_volt;
}
EXPORT_SYMBOL(gs_gpufreq_fake_get_cur_setting);

/*****************************************************************************
 * procs write                                                               *
 *****************************************************************************/

/***********************
 * enable debug message
 ***********************/
static ssize_t proc_dvs_delay_write_(struct file *file, const char *buffer, size_t count,
				     loff_t *data)
{
	u32 dvs_delay = 0;
	int i;
	char proc_buffer[20] = {0};

	if (count > (sizeof(proc_buffer) - 1)) {
		print_error("[GPUFREQ][PROC][ERROR]: dvs_delay_write : input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < count; ++i)
		proc_buffer[i] = buffer[i];

	proc_buffer[i] = 0;


	if (!kstrtouint((const char *) proc_buffer, 10, &dvs_delay)) {
		if (dvs_delay) {
			g_dvs_delay = dvs_delay;
			return count;
		}
		g_dvs_delay = 0;
		return count;
	}

	print_error("[GPUFREQ][PROC][ERROR] dvs_delay_write : should be a number\n");

	return count;
}

static ssize_t proc_dummy_read_write_(struct file *file, const char *buffer, size_t count,
				      loff_t *data)
{
	u32 dummy_read = 0;
	int i;
	char proc_buffer[20] = {0};

	if (count > (sizeof(proc_buffer) - 1)) {
		print_error("[GPUFREQ][PROC][ERROR]: dummy_read_write : input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < count; ++i)
		proc_buffer[i] = buffer[i];

	proc_buffer[i] = 0;

	if (!kstrtouint((const char *) proc_buffer, 10, &dummy_read)) {
		if (dummy_read) {
			g_dummy_read = dummy_read;
			return count;
		}
		g_dummy_read = 0;
		return count;
	}

	print_error("[GPUFREQ][PROC][ERROR] dummy_read_write : should be a number\n");

	return count;
}

static ssize_t proc_debug_write_(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	s32 debug = 0;
	int i;
	char proc_buffer[20] = {0};

	if (count > (sizeof(proc_buffer) - 1)) {
		print_error("[GPUFREQ][PROC][ERROR]: debug_write : input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < count; ++i)
		proc_buffer[i] = buffer[i];

	proc_buffer[i] = 0;

	if (kstrtoint(proc_buffer, 10, &debug) == 0) {
		if (debug) {
			g_debug = 1;
			return count;
		}

		g_debug = 0;
		return count;
	}

	print_error("[GPUFREQ][PROC][ERROR] debug_write : should be 0 or 1\n");

	return count;
}

static ssize_t proc_power_switch_write_(struct file *file, const char *buffer, size_t count,
					loff_t *data)
{
	s32 power_switch = 0;
	int i;
	char proc_buffer[20] = {0};

	if (count > (sizeof(proc_buffer) - 1)) {
		print_error("[GPUFREQ][PROC][ERROR]: power_switch_write : input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < count; ++i)
		proc_buffer[i] = buffer[i];

	proc_buffer[i] = 0;

	if (kstrtoint(proc_buffer, 10, &power_switch) == 0) {
		if (power_switch > 0) {
			gs_gpufreq_vgpu_sel_default();
			return count;
		}

		gs_gpufreq_vgpu_disable();
		return count;

	}

	print_error("[GPUFREQ][PROC][ERROR] power_swtich_write : should be -1 or 1\n");

	return count;
}

/**********************
 * set GPU DVFS stauts
 **********************/
static ssize_t proc_dvfs_state_write_(struct file *file, const char *buffer, size_t count,
				      loff_t *data)
{
	s32 enabled = 0;
	int i;
	char proc_buffer[20] = {0};

	if (count > (sizeof(proc_buffer) - 1)) {
		print_error("[GPUFREQ][PROC][ERROR]: dvfs_state_write : input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < count; ++i)
		proc_buffer[i] = buffer[i];

	proc_buffer[i] = 0;

	if (kstrtoint(proc_buffer, 10, &enabled) == 0) {
		if (enabled > 0) {
			gs_gpufreq_dvfs_enable();
			return count;
		}

		gs_gpufreq_dvfs_disable();
		return count;

	}

	print_error("[GPUFREQ][PROC][ERROR] dvfs_state_write : input 1 or -1\n");

	return count;
}

static ssize_t proc_limited_power_write_(struct file *file, const char *buffer, size_t count,
					 loff_t *data)
{
	u32 power = 0;
	int i;
	char proc_buffer[20] = {0};

	if (count > (sizeof(proc_buffer) - 1)) {
		print_error("[GPUFREQ][PROC][ERROR]: limited_power_write : input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < count; ++i)
		proc_buffer[i] = buffer[i];

	proc_buffer[i] = 0;

	if (kstrtouint(proc_buffer, 10, &power) == 0) {
		mt_gpufreq_thermal_protect(power);
		return count;
	}

	print_error("[GPUFREQ][PROC][ERROR] limited_power_write : input maximum limited power\n");

	return count;
}

static ssize_t proc_bottom_freq_write_(struct file *file, const char *buffer, size_t count,
				       loff_t *data)
{
	u32 bottom_freq = 0;
	int i;
	char proc_buffer[20] = {0};

	if (count > (sizeof(proc_buffer) - 1)) {
		print_error("[GPUFREQ][PROC][ERROR]: bottom_freq_write : input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < count; ++i)
		proc_buffer[i] = buffer[i];

	proc_buffer[i] = 0;

	if (kstrtouint(proc_buffer, 10, &bottom_freq) == 0) {
		gs_gpufreq_bottom_speed(bottom_freq);
		return count;
	}

	print_error("[GPUFREQ][PROC][ERROR] bottom_freq_write : input bottom freq.\n");

	return count;
}

static ssize_t proc_index_write_(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	u32 size = 0;
	u32 index = 0;
	struct mtk_gpufreq_info row;
	int i;
	char proc_buffer[20] = {0};

	if (count > (sizeof(proc_buffer) - 1)) {
		print_error("[GPUFREQ][PROC][ERROR]: index_write : input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < count; ++i)
		proc_buffer[i] = buffer[i];

	proc_buffer[i] = 0;

	if (kstrtouint(proc_buffer, 10, &index) == 0) {

		LOCK_TABLE();
		size = g_freq_table->size;

		if (index < size) {
			row = g_freq_table->rows[index];
			UNLOCK_TABLE();

			LOCK_FREQ_VOLT();
			g_freq_khz = row.freq_khz;
			g_volt = row.volt;
			UNLOCK_FREQ_VOLT();

			gs_gpufreq_switch_handler();

			print_notice("[GPUFREQ][PROC] set index = %d\n", index);
			return count;

		} else {
			UNLOCK_TABLE();
		}
	}

	print_error("[GPUFREQ][PROC][ERROR] index_write : value\n");

	return count;
}


static ssize_t proc_freq_volt_write_(struct file *file, const char *buffer, size_t count,
				     loff_t *data)
{

	u32 freq = 0;
	u32 volt = 0;

	if (sscanf(buffer, "%u %u", &freq, &volt) == 2) {
		if ((freq > 0) && ((volt >= GPU_POWER_VGPU_MIN) && (volt <= GPU_POWER_VGPU_MAX))) {
			LOCK_FREQ_VOLT();
			g_freq_khz = freq;
			g_volt = volt;
			UNLOCK_FREQ_VOLT();

			gs_gpufreq_switch_handler();
			print_notice("[GPUFREQ][PROC] set freq write (freq, volt) = (%d, %d)\n",
				     freq, volt);
			return count;
		}
	}

	print_error("[GPUFREQ][PROC][ERROR] freq_volt write : freq volt\n");

	return count;
}

/* 0, 1, 2 */
static ssize_t proc_ptpod_write_(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	u32 cmd = 0;
	int i;
	char proc_buffer[20] = {0};

	if (count > (sizeof(proc_buffer) - 1)) {
		print_error("[GPUFREQ][PROC][ERROR]: ptpod_write : input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < count; ++i)
		proc_buffer[i] = buffer[i];

	proc_buffer[i] = 0;

	if (kstrtouint(proc_buffer, 10, &cmd) == 0) {
		switch (cmd) {

		case 0:
			mt_gpufreq_disable_by_ptpod();
			break;
		case 1:
			mt_gpufreq_enable_by_ptpod();
			break;
		case 2:
			mt_gpufreq_restore_default_volt();
			break;
		case 10:
		case 11:
		case 12:
			gs_gpufreq_ptpod_fake_volt(cmd - 10);
			break;
		}
		return count;
	}

	print_error("[GPUFREQ][PROC][ERROR] ptpod_write\n");

	return count;
}

static ssize_t proc_input_switch_write_(struct file *file, const char *buffer, size_t count,
					loff_t *data)
{

	u32 touch_boost = 0;
	u32 thermal = 0;
	u32 low_battery_volt = 0;
	u32 low_battery_volume = 0;

	if (sscanf
	    (buffer, "%u %u %u %u", &touch_boost, &thermal, &low_battery_volt,
	     &low_battery_volume) == 4) {
		g_touch_boost_switch = !!touch_boost;
		g_thermal_switch = !!thermal;
		g_low_battery_volt_switch = !!low_battery_volt;
		g_low_battery_volume_switch = !!low_battery_volume;
		return count;
	}

	print_error("[GPUFREQ][PROC][ERROR] freq_volt write : freq volt\n");

	return count;
}

/**********************
 * set GPU MTCMOS stauts
 **********************/
static ssize_t proc_mtcmos_state_write_(struct file *file, const char *buffer, size_t count,
					loff_t *data)
{
	s32 enabled = 0;
	int i;
	char proc_buffer[20] = {0};

	if (count > (sizeof(proc_buffer) - 1)) {
		print_error("[GPUFREQ][PROC][ERROR]: mtcmos_state_write : input too large\n");
		return -EINVAL;
	}

	for (i = 0; i < count; ++i)
		proc_buffer[i] = buffer[i];

	proc_buffer[i] = 0;

	if (kstrtoint(proc_buffer, 10, &enabled) == 0) {
		if (enabled > 0) {
			gs_gpufreq_mtcmos_on();
			return count;
		}

		gs_gpufreq_mtcmos_off();
		return count;
	}

	print_error("[GPUFREQ][PROC][ERROR] mtcmos_state_write : input 1 or -1\n");

	return count;
}

/*****************************************************************************
 * procs open / read                                                         *
 *****************************************************************************/

static int proc_dvs_delay_show_(struct seq_file *s, void *v)
{
	seq_printf(s, "gpufreq dvs delay = %d\n", g_dvs_delay);

	return 0;
}

static int proc_dummy_read_show_(struct seq_file *s, void *v)
{
	seq_printf(s, "gpufreq dvs dummy read = %d\n", g_dummy_read);

	return 0;
}


static int proc_debug_show_(struct seq_file *s, void *v)
{
	if (g_debug)
		seq_puts(s, "gpufreq debug enabled\n");
	else
		seq_puts(s, "gpufreq debug disabled\n");

	return 0;
}

static int proc_dvs_delay_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_dvs_delay_show_, NULL);
}

static int proc_dummy_read_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_dummy_read_show_, NULL);
}

static int proc_debug_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_debug_show_, NULL);
}

static int proc_power_switch_show_(struct seq_file *s, void *v)
{
	s32 power_switch = atomic_read(&g_power_switch);

	if (power_switch > 0)
		seq_puts(s, "power_switch enabled\n");
	else
		seq_puts(s, "power_switch disabled\n");

	return 0;
}

static int proc_power_switch_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_power_switch_show_, NULL);
}

/********************************
 * show current limited GPU freq
 ********************************/
static int proc_limited_power_show_(struct seq_file *s, void *v)
{
	u32 power = g_limited_power_mW;

	seq_printf(s, "g_limited_power_mW = %d\n", power);

	return 0;
}

static int proc_limited_power_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_limited_power_show_, NULL);
}

/***************************
 * show current bottom freq
 ***************************/
static int proc_bottom_freq_show_(struct seq_file *s, void *v)
{
	u32 bottom_freq = g_bottom_freq;

	seq_printf(s, "g_bottom_freq = %d\n", bottom_freq);

	return 0;
}

static int proc_bottom_freq_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_bottom_freq_show_, NULL);
}

/*******************************
 * show current GPU DVFS stauts
 *******************************/
static int proc_dvfs_state_show_(struct seq_file *s, void *v)
{
	s32 dvfs_enable_count = atomic_read(&g_dvfs_enable_count);

	if (dvfs_enable_count > 0)
		seq_puts(s, "GPU DVFS enabled\n");
	else
		seq_puts(s, "GPU DVFS disabled\n");

	return 0;
}

static int proc_dvfs_state_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_dvfs_state_show_, NULL);
}

/*********************
 * show GPU OPP table
 *********************/
static int proc_opp_dump_show_(struct seq_file *s, void *v)
{
	u32 size;
	u32 i;
	struct mtk_gpufreq_info *rows;
	struct freq_table *current_table;

	LOCK_TABLE();
	current_table = g_freq_table;
	size = current_table->size;
	rows = current_table->rows;

	seq_printf(s, "[table] size:%d ", current_table->size);
	seq_printf(s, "cur:%d ", current_table->current_index);
	seq_printf(s, "max:%d ", current_table->max_index);
	seq_printf(s, "bottom:%d\n", current_table->bottom_index);

	for (i = 0; i < current_table->size; ++i) {
		seq_printf(s, "[%d] ", i);
		seq_printf(s, "freq = %d, ", rows[i].freq_khz);
		seq_printf(s, "volt = %2d, ", rows[i].volt);
		seq_printf(s, "lower_bound = %3d, ", rows[i].lower_bound);
		seq_printf(s, "upper_bound = %3d, ", rows[i].upper_bound);
		seq_printf(s, "power_mW = %5d\n", rows[i].power_mW);
	}
	UNLOCK_TABLE();

	return 0;
}

static int proc_opp_dump_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_opp_dump_show_, NULL);
}

/*********************
 * show variable dump
 *********************/

static int proc_var_dump_show_(struct seq_file *s, void *v)
{
	s32 power_switch = atomic_read(&g_power_switch);
	s32 dvfs_enable_count = atomic_read(&g_dvfs_enable_count);
	s32 touch_boost_register_count = atomic_read(&g_touch_boost_register_count);
	s32 power_limit_register_count = atomic_read(&g_power_limit_register_count);
	s32 freq_register_count = atomic_read(&g_freq_register_count);
	s32 volt_register_count = atomic_read(&g_volt_register_count);
	s32 table_register_count = atomic_read(&g_table_register_count);
	s32 table_unregister_count = atomic_read(&g_table_unregister_count);
	s32 update_volt_count = atomic_read(&g_update_volt_count);

	seq_puts(s, "#### SW values ####\n");
	seq_printf(s, "g_power_switch = %d\n", power_switch);
	seq_printf(s, "g_dvfs_enable_count = %d\n", dvfs_enable_count);
	seq_printf(s, "g_freq_khz = %d\n", g_freq_khz);
	seq_printf(s, "g_volt = %d\n", g_volt);
	seq_printf(s, "g_old_freq_khz = %d\n", g_old_freq_khz);
	seq_printf(s, "g_old_volt = %d\n", g_old_volt);
	seq_printf(s, "g_limited_power_mW = %d\n", g_limited_power_mW);
	seq_printf(s, "g_bottom_freq = %d\n", g_bottom_freq);

	seq_puts(s, "#### Set to HW ####\n");
	seq_printf(s, "g_fake_freq_khz = %d\n", g_fake_freq_khz);
	seq_printf(s, "g_fake_volt = %d\n", g_fake_volt);
	seq_printf(s, "g_fake_vgpu_sel_default_called = %d\n", g_fake_vgpu_sel_default_called);
	seq_printf(s, "g_fake_vgpu_disable_called = %d\n", g_fake_vgpu_disable_called);
	seq_printf(s, "g_fake_freq_to_thermal = %d\n", g_fake_freq_to_thermal);
	seq_printf(s, "g_fake_volt_to_thermal = %d\n", g_fake_volt_to_thermal);

	seq_puts(s, "#### Callback register ####\n");
	seq_printf(s, "g_touch_boost_register_count = %d\n", touch_boost_register_count);
	seq_printf(s, "g_power_limit_register_count = %d\n", power_limit_register_count);
	seq_printf(s, "g_MET_freq_register_count = %d\n", freq_register_count);
	seq_printf(s, "g_MET_volt_register_count = %d\n", volt_register_count);

	seq_puts(s, "#### Call counts ####\n");
	seq_printf(s, "g_table_register_count= %d\n", table_register_count);
	seq_printf(s, "g_table_unregister_count= %d\n", table_unregister_count);
	seq_printf(s, "g_update_volt_count= %d\n", update_volt_count);
	seq_printf(s, "g_set_load_call_count = %llu\n", g_set_load_call_count);
	seq_printf(s, "g_set_load_call_count = %llu\n", g_set_load_call_count);
	seq_printf(s, "g_set_index_call_count = %llu\n", g_set_index_call_count);
	seq_printf(s, "g_touch_boost_call_count = %llu\n", g_touch_boost_call_count);
	seq_printf(s, "g_thermal_call_count = %llu\n", g_thermal_call_count);
	seq_printf(s, "g_low_battery_volt_call_count = %llu\n", g_low_battery_volt_call_count);
	seq_printf(s, "g_low_battery_volume_call_count = %llu\n", g_low_battery_volume_call_count);

	return 0;
}

static int proc_var_dump_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_var_dump_show_, NULL);
}

static int proc_index_show_(struct seq_file *s, void *v)
{
	u32 cur = 0;
	/*u32 u = 0; */
	/*u32 l = 0; */

	LOCK_TABLE();
	cur = g_freq_table->current_index;
	UNLOCK_TABLE();

	seq_printf(s, "current index= %d\n", cur);
	return 0;
}

static int proc_freq_volt_show_(struct seq_file *s, void *v)
{
	u32 freq = 0;
	u32 volt = 0;

	LOCK_FREQ_VOLT();
	freq = g_freq_khz;
	volt = g_volt;
	UNLOCK_FREQ_VOLT();

	seq_printf(s, "g_freq_khz = %d\n", g_freq_khz);
	seq_printf(s, "g_volt = %d\n", g_volt);
	return 0;
}

static int proc_ptpod_show_(struct seq_file *s, void *v)
{
	u32 ptpod_switch = g_ptpod_switch;
	u32 cur_volt = 0;

	cur_volt = mt_gpufreq_get_cur_volt();


	seq_puts(s, "usage : 0:disable, 1:enable 2:restore 10~11:fake value\n");
	seq_printf(s, "g_ptpod_switch = %d cur_volt = %d\n", ptpod_switch, cur_volt);
	return 0;
}

static int proc_index_open_(struct inode *inode, struct file *file)
{
	print_notice("GPUFREQ set index open\n");
	return single_open(file, proc_index_show_, NULL);
}

static int proc_freq_volt_open_(struct inode *inode, struct file *file)
{
	print_notice("GPUFREQ set freq open\n");
	return single_open(file, proc_freq_volt_show_, NULL);
}

static int proc_ptpod_open_(struct inode *inode, struct file *file)
{
	print_notice("GPUFREQ PROC ptpod open\n");
	return single_open(file, proc_ptpod_show_, NULL);
}

static int proc_input_switch_show_(struct seq_file *s, void *v)
{
	bool touch_boost = g_touch_boost_switch;
	bool thermal = g_thermal_switch;
	bool low_battery_volt = g_low_battery_volt_switch;
	bool low_battery_volume = g_low_battery_volume_switch;

	seq_printf(s,
		   "touch boost / thermal / low battery volt / low battery volume =\n %d %d %d %d\n",
		   touch_boost, thermal, low_battery_volt, low_battery_volume);

	return 0;
}

static int proc_input_switch_open_(struct inode *inode, struct file *file)
{
	print_notice("GPUFREQ input enable open\n");
	return single_open(file, proc_input_switch_show_, NULL);
}

/*******************************
 * show current GPU MTCMOS stauts
 *******************************/
static int proc_mtcmos_state_show_(struct seq_file *s, void *v)
{
	s32 mtcmos_enable_count = atomic_read(&g_mtcmos_enable_count);

	if (mtcmos_enable_count > 0)
		seq_puts(s, "GPU MTCMOS enabled\n");
	else
		seq_puts(s, "GPU MTCMOS disabled\n");

	return 0;
}

static int proc_mtcmos_state_open_(struct inode *inode, struct file *file)
{
	return single_open(file, proc_mtcmos_state_show_, NULL);
}

static const struct file_operations dvs_delay_fops = {
	.owner = THIS_MODULE,
	.write = proc_dvs_delay_write_,
	.open = proc_dvs_delay_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations dummy_read_fops = {
	.owner = THIS_MODULE,
	.write = proc_dummy_read_write_,
	.open = proc_dummy_read_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations debug_fops = {
	.owner = THIS_MODULE,
	.write = proc_debug_write_,
	.open = proc_debug_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations power_switch_fops = {
	.owner = THIS_MODULE,
	.write = proc_power_switch_write_,
	.open = proc_power_switch_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations limited_power_fops = {
	.owner = THIS_MODULE,
	.write = proc_limited_power_write_,
	.open = proc_limited_power_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations bottom_freq_fops = {
	.owner = THIS_MODULE,
	.write = proc_bottom_freq_write_,
	.open = proc_bottom_freq_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations dvfs_state_fops = {
	.owner = THIS_MODULE,
	.write = proc_dvfs_state_write_,
	.open = proc_dvfs_state_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


static const struct file_operations opp_dump_fops = {
	.owner = THIS_MODULE,
	.write = NULL,
	.open = proc_opp_dump_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations index_fops = {
	.owner = THIS_MODULE,
	.write = proc_index_write_,
	.open = proc_index_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations freq_volt_fops = {
	.owner = THIS_MODULE,
	.write = proc_freq_volt_write_,
	.open = proc_freq_volt_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations ptpod_fops = {
	.owner = THIS_MODULE,
	.write = proc_ptpod_write_,
	.open = proc_ptpod_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations input_switch_fops = {
	.owner = THIS_MODULE,
	.write = proc_input_switch_write_,
	.open = proc_input_switch_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations var_dump_fops = {
	.owner = THIS_MODULE,
	.write = NULL,
	.open = proc_var_dump_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations mtcmos_state_fops = {
	.owner = THIS_MODULE,
	.write = proc_mtcmos_state_write_,
	.open = proc_mtcmos_state_open_,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mtk_gpufreq_initialize_proc_(void)
{
	struct proc_dir_entry *dir = (void *)NULL;
	struct proc_dir_entry *entry = (void *)NULL;

	print_notice("[GPUFREQ][PROC] init\n");

	dir = proc_mkdir("gpufreq", NULL);
	if (!dir) {
		print_error("[GPUFREQ][PROC] Error proc_mkdir(gpufreq)\n");
		return -1;
	}

	g_proc_dir = dir;

	/* dvs_delay : RW [s32] */
	entry = proc_create("dvs_delay", S_IRUGO | S_IWUSR | S_IWGRP, dir, &dvs_delay_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/dvs_delay failed\n", __func__);


	/* dummy_read : RW [s32] */
	entry = proc_create("dummy_read", S_IRUGO | S_IWUSR | S_IWGRP, dir, &dummy_read_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/dummy_read failed\n", __func__);


	/* debug enabled : RW [0,1] */
	entry = proc_create("debug", S_IRUGO | S_IWUSR | S_IWGRP, dir, &debug_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/debug failed\n", __func__);


	/* freq_volt : RW (freq,volt) = (u32,u32) */
	entry = proc_create("index", S_IRUGO | S_IWUSR | S_IWGRP, dir, &index_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/index failed\n", __func__);

#if GPAD_IPEM_DEBUG_PROC

	/* power switch : RW [-1,1] */
	entry = proc_create("power_switch", S_IRUGO | S_IWUSR | S_IWGRP, dir, &power_switch_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/power_switch failed\n", __func__);

	/* dvfs_state : RW reference counted [-1,1] */
	entry = proc_create("dvfs_state", S_IRUGO | S_IWUSR | S_IWGRP, dir, &dvfs_state_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/dvfs_state failed\n", __func__);

	/* limited_power : RW u32 */
	entry = proc_create("limited_power", S_IRUGO | S_IWUSR | S_IWGRP, dir, &limited_power_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/limited_power failed\n", __func__);

	/* bottom freq : RW u32 */
	entry = proc_create("bottom_freq", S_IRUGO | S_IWUSR | S_IWGRP, dir, &bottom_freq_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/bottom_freq failed\n", __func__);

	/* freq_volt : RW (freq,volt) = (u32,u32) */
	entry = proc_create("freq_volt", S_IRUGO | S_IWUSR | S_IWGRP, dir, &freq_volt_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/freq_volt failed\n", __func__);

	/* */
	entry = proc_create("ptpod", S_IRUGO | S_IWUSR | S_IWGRP, dir, &ptpod_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/ptpod failed\n", __func__);

	/* input switch */
	entry = proc_create("input_switch", S_IRUGO | S_IWUSR | S_IWGRP, dir, &input_switch_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/input_switch failed\n", __func__);
#endif

	/* opp_dump : R */
	entry = proc_create("opp_dump", S_IRUGO | S_IWUSR | S_IWGRP, dir, &opp_dump_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/opp_dump failed\n", __func__);

	/* var_dump : R */
	entry = proc_create("var_dump", S_IRUGO | S_IWUSR | S_IWGRP, dir, &var_dump_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/var_dump failed\n", __func__);

	/* mtcmos_state : RW state [-1,1] */
	entry = proc_create("mtcmos_state", S_IRUGO | S_IWUSR | S_IWGRP, dir, &mtcmos_state_fops);
	if (!entry)
		print_error("[%s]: mkdir /proc/gpufreq/mtcmos_state failed\n", __func__);
	return 0;
}

static int mfg_device_probe(struct platform_device *pdev)
{
	struct clk *parent;

	print_info("[%s]: MFG device probed\n", __func__);


	mfg_start = of_iomap(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(mfg_start)) {
		mfg_start = NULL;
		goto error_out;
	}
	print_debug("[%s]: MFG start is mapped %p\n", __func__, mfg_start);

	clk_mm = devm_clk_get(&pdev->dev, "mm");
	if (IS_ERR(clk_mm)) {
		print_error("[%s]: MFG cannot get mm clock\n", __func__);
		clk_mm = NULL;
		goto error_out;
	}

	clk_pll = devm_clk_get(&pdev->dev, "pll");
	if (IS_ERR(clk_pll)) {
		print_error("[%s]: MFG cannot get pll clock\n", __func__);
		clk_pll = NULL;
		goto error_out;
	}

	clk_topck = devm_clk_get(&pdev->dev, "topck");
	if (IS_ERR(clk_topck)) {
		print_error("[%s]: MFG cannot get topck clock\n", __func__);
		clk_topck = NULL;
		goto error_out;
	}

	clk_prepare_enable(clk_topck);
	clk_set_parent(clk_topck, clk_pll);
	parent = clk_get_parent(clk_topck);
	if (!IS_ERR_OR_NULL(parent)) {
		print_info("[%s]: MFG is now selected to %s\n", __func__, parent->name);
	} else {
		print_error("[%s]: Failed to select mfg\n", __func__);
		BUG();
	}

	pm_runtime_enable(&pdev->dev);

	mfg_dev = pdev;

	return 0;

error_out:
	if (mfg_start)
		iounmap(mfg_start);
	if (clk_mm)
		devm_clk_put(&pdev->dev, clk_mm);
	if (clk_pll)
		devm_clk_put(&pdev->dev, clk_pll);
	if (clk_topck)
		devm_clk_put(&pdev->dev, clk_topck);

	return -1;
}

static int mfg_device_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);

	if (mfg_start)
		iounmap(mfg_start);
	if (clk_mm)
		devm_clk_put(&pdev->dev, clk_mm);
	if (clk_pll)
		devm_clk_put(&pdev->dev, clk_pll);
	if (clk_topck) {
		clk_disable_unprepare(clk_topck);
		devm_clk_put(&pdev->dev, clk_topck);
	}

	return 0;
}

static struct platform_driver mtk_mfg_driver = {
	.probe = mfg_device_probe,
	.remove = mfg_device_remove,
	.driver = {
		   .name = "mfg",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(mfg_dt_ids),
		   },
};

/*
 * TODO: built-in
 */
static int __init mtk_gpufreq_install(void)
{
	int ret;

	print_notice("[GPUFREQ] after module installed\n");

	ret = mtk_gpufreq_initialize();
	if (ret)
		return ret;

	ret = platform_driver_register(&mtk_mfg_driver);
	if (!ret)
		return ret;

	return 0;
}

static void __exit mtk_gpufreq_exit(void)
{
	print_notice("GPUFREQ] before module exit\n");
}

static int mtk_gpufreq_initialize_noncache_dummy(void)
{
#if GS_GPUFREQ_PATHB_VOLT
	dma_addr_t dma_handle;
	struct platform_device *g3dpdev = (struct platform_device *)g3dGetPlatformDevice();
	struct device *g3ddev = (NULL != g3dpdev) ? &(g3dpdev->dev) : NULL;

	noncache_dummy_src =
	    dma_alloc_coherent(g3ddev, NONCACHE_DUMMY_SIZE, &dma_handle, GFP_KERNEL | GFP_DMA);
	noncache_dummy_dst =
	    dma_alloc_coherent(g3ddev, NONCACHE_DUMMY_SIZE, &dma_handle, GFP_KERNEL | GFP_DMA);
	memset(((void *)noncache_dummy_src), 0x5a, NONCACHE_DUMMY_SIZE);
	late_init_done = 1;
	return 0;
#endif
}

static int __init mtk_gpufreq_lateinit(void)
{
	int ret;

	print_notice("[GPUFREQ] after module lateinit\n");
	g3d_dev = (struct platform_device *)g3dGetPlatformDevice();

	gs_gpufreq_mtcmos_on();
	gs_gpufreq_switch_handler();

	ret = mtk_gpufreq_initialize_noncache_dummy();

	return ret;
}


/* TODO */


subsys_initcall(mtk_gpufreq_install);
late_initcall(mtk_gpufreq_lateinit);
module_exit(mtk_gpufreq_exit);

MODULE_DESCRIPTION("MediaTek GPU Frequency Scaling driver");
MODULE_LICENSE("GPL");
