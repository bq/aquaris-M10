/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clk-private.h>
#include <linux/clkdev.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/ktime.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/regulator/consumer.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/syscore_ops.h>

#include <mach/mt_thermal.h>
#include <mach/mt_rtc_hw.h>
#include <mach/mtk_rtc_hal.h>
#include <mach/mt_dcm.h>
#include "mt_ptp.h"
#include <mach/mt_freqhopping.h>

#include <mt-plat/mt_pmic_wrap.h>
#include <mt-plat/sync_write.h>
#include "mt_cpufreq.h"
#ifdef NOT_PORTING
#include "mt_spm_idle.h"
#endif

#define EN_ISR_LOG			(0)
#define TAG				"[PTP] "

#define ptp_error(fmt, args...)		pr_err(TAG fmt, ##args)
#define ptp_warning(fmt, args...)	pr_warn(TAG fmt, ##args)
#define ptp_notice(fmt, args...)	pr_notice(TAG fmt, ##args)
#define ptp_info(fmt, args...)		pr_debug(TAG fmt, ##args)
#define ptp_debug(fmt, args...)		pr_debug(TAG fmt, ##args)

#if EN_ISR_LOG
#define ptp_isr_info(fmt, args...)	pr_info(TAG fmt, ##args)
#else
#define ptp_isr_info(fmt, args...)	pr_debug(TAG fmt, ##args)
#endif

#define PTP_VOLT_TO_PMIC_VAL(volt)	(((volt) - 70000 + 625 - 1) / 625)

/* Global variable */
static int ptp_log_en;

#if EN_PTP_OD
unsigned int ptp_data[3] = { 0xffffffff, 0, 0 };
#else
unsigned int ptp_data[3] = { 0, 0, 0 };
#endif

static unsigned int val_0 = 0x00000000;
static unsigned int val_1 = 0x00000000;
static unsigned int val_2 = 0x00000000;
static unsigned int val_3 = 0x00000000;

#define ptp_attr(_name) \
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = __stringify(_name),	\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

static void PTP_set_ptp_volt(void);

static unsigned char freq_0, freq_1, freq_2, freq_3;
static unsigned char freq_4, freq_5, freq_6, freq_7;

static unsigned int ptp_level;

static unsigned int ptp_array_size;

static unsigned int ptp_volt_0, ptp_volt_1;
static unsigned int ptp_volt_2, ptp_volt_3;
static unsigned int ptp_volt_4, ptp_volt_5;
static unsigned int ptp_volt_6, ptp_volt_7;

static unsigned int ptp_init2_volt_0;
static unsigned int ptp_init2_volt_1;
static unsigned int ptp_init2_volt_2;
static unsigned int ptp_init2_volt_3;
static unsigned int ptp_init2_volt_4;
static unsigned int ptp_init2_volt_5;
static unsigned int ptp_init2_volt_6;
static unsigned int ptp_init2_volt_7;
static unsigned int ptp_dcvoffset;
static unsigned int ptp_agevoffset;

static unsigned int ptpod_pmic_volt[8] = { 0x30, 0x30, 0x30, 0x30, 0x30, 0,
					  0, 0 };

static struct hrtimer mt_ptp_log_timer;
struct task_struct *mt_ptp_log_thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(mt_ptp_log_timer_waiter);

static int mt_ptp_log_timer_flag;
static int mt_ptp_log_period_s = 2;
static int mt_ptp_log_period_ns;

static struct hrtimer mt_ptp_volt_timer;
struct task_struct *mt_ptp_volt_thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(mt_ptp_volt_timer_waiter);

static int mt_ptp_volt_timer_flag;
static int mt_ptp_volt_period_s;
static int mt_ptp_volt_period_ns = 10000;

static int mt_ptp_offset;

#if EN_PTP_OD
static int mt_ptp_enable = 1;
#else
static int mt_ptp_enable;
#endif

enum ptp_phase {
	PTP_PHASE_INIT01,
	PTP_PHASE_INIT02,
	PTP_PHASE_MON,

	NR_PTP_PHASE,
};

/*
 * CONFIG (CHIP related)
 * PTPCORESEL.APBSEL
 */
unsigned int reg_dump_addr_off[] = {
	0x0000,
	0x0004,
	0x0008,
	0x000C,
	0x0010,
	0x0014,
	0x0018,
	0x001c,
	0x0024,
	0x0028,
	0x002c,
	0x0030,
	0x0034,
	0x0038,
	0x003c,
	0x0040,
	0x0044,
	0x0048,
	0x004c,
	0x0050,
	0x0054,
	0x0058,
	0x005c,
	0x0060,
	0x0064,
	0x0068,
	0x006c,
	0x0070,
	0x0074,
	0x0078,
	0x007c,
	0x0080,
	0x0084,
	0x0088,
	0x008c,
	0x0090,
	0x0094,
	0x0098,
	0x00a0,
	0x00a4,
	0x00a8,
	0x00B0,
	0x00B4,
	0x00B8,
	0x00BC,
	0x00C0,
	0x00C4,
	0x00C8,
	0x00CC,
	0x00F0,
	0x00F4,
	0x00F8,
	0x00FC,
	0x0200,
	0x0204,
	0x0208,
	0x020C,
	0x0210,
	0x0214,
	0x0218,
	0x021C,
	0x0220,
	0x0224,
	0x0228,
	0x022C,
	0x0230,
	0x0234,
	0x0238,
	0x023C,
	0x0240,
	0x0244,
	0x0248,
	0x024C,
	0x0250,
	0x0254,
	0x0258,
	0x025C,
	0x0260,
	0x0264,
	0x0268,
	0x026C,
	0x0270,
	0x0274,
	0x0278,
	0x027C,
	0x0280,
	0x0284,
	0x0400,
	0x0404,
	0x0408,
	0x040C,
	0x0410,
	0x0414,
	0x0418,
	0x041C,
	0x0420,
	0x0424,
	0x0428,
	0x042C,
	0x0430,
};

unsigned int reg_dump_data[ARRAY_SIZE(reg_dump_addr_off)][NR_PTP_PHASE];

#ifdef CONFIG_MTK_RAM_CONSOLE
enum ptp_state {
	PTP_CPU_BIG_IS_SET_VOLT = 0,    /* B */
	PTP_GPU_IS_SET_VOLT = 1,        /* G */
	PTP_CPU_LITTLE_IS_SET_VOLT = 2, /* L */
};
#endif

/* For fake functions */
void __attribute__ ((weak)) bus_dcm_disable(void)
{
	ptp_error("%s not implement...\n", __func__);
}

u32 __attribute__ ((weak)) mt_cpufreq_max_frequency_by_DVS(unsigned int num)
{
	ptp_error("%s not implement...\n", __func__);
	return 0;
}

void __attribute__ ((weak)) mt_cpufreq_return_default_DVS_by_ptpod(void)
{
	ptp_error("%s not implement...\n", __func__);
}

u32 __attribute__ ((weak)) mt_cpufreq_voltage_set_by_ptpod(unsigned int pmic_volt[],
						    unsigned int array_size)
{
	ptp_error("%s not implement...\n", __func__);
	return 0;
}

unsigned int __attribute__ ((weak)) mt_cpufreq_disable_by_ptpod(void)
{
	ptp_error("%s not implement...\n", __func__);
	return 0;
}

void  __attribute__ ((weak)) mt_cpufreq_enable_by_ptpod(void)
{
	ptp_error("%s not implement...\n", __func__);
}

u32 __attribute__ ((weak)) mt_cpufreq_cur_vproc(void)
{
	ptp_error("%s not implement...\n", __func__);
	return 0;
}

void __attribute__ ((weak)) mt_fh_popod_save(void)
{
	ptp_error("%s not implement...\n", __func__);
}

void __attribute__ ((weak)) mt_fh_popod_restore(void)
{
	ptp_error("%s not implement...\n", __func__);
}

static unsigned int ptp_trasnfer_to_volt(unsigned int value)
{
	return (((value * 625) / 100) + 700);	/* (700mv + n * 6.25mv) */
}

enum hrtimer_restart mt_ptp_log_timer_func(struct hrtimer *timer)
{
	mt_ptp_log_timer_flag = 1;
	wake_up_interruptible(&mt_ptp_log_timer_waiter);
	return HRTIMER_NORESTART;
}

int mt_ptp_log_thread_handler(void *unused)
{
	do {
		ktime_t ktime = ktime_set(mt_ptp_log_period_s,
					  mt_ptp_log_period_ns);

		wait_event_interruptible(mt_ptp_log_timer_waiter,
					 mt_ptp_log_timer_flag != 0);
		mt_ptp_log_timer_flag = 0;

		pr_notice
		    ("PTP_LOG: (%d) - (%d, %d, %d, %d, %d, %d, %d, %d) - ",
		     tscpu_get_cpu_temp(),
		     ptp_trasnfer_to_volt(ptpod_pmic_volt[0]),
		     ptp_trasnfer_to_volt(ptpod_pmic_volt[1]),
		     ptp_trasnfer_to_volt(ptpod_pmic_volt[2]),
		     ptp_trasnfer_to_volt(ptpod_pmic_volt[3]),
		     ptp_trasnfer_to_volt(ptpod_pmic_volt[4]),
		     ptp_trasnfer_to_volt(ptpod_pmic_volt[5]),
		     ptp_trasnfer_to_volt(ptpod_pmic_volt[6]),
		     ptp_trasnfer_to_volt(ptpod_pmic_volt[7]));

		pr_notice
		    ("(%d, %d, %d, %d, %d, %d, %d, %d)\n",
		     mt_cpufreq_max_frequency_by_DVS(0),
		     mt_cpufreq_max_frequency_by_DVS(1),
		     mt_cpufreq_max_frequency_by_DVS(2),
		     mt_cpufreq_max_frequency_by_DVS(3),
		     mt_cpufreq_max_frequency_by_DVS(4),
		     mt_cpufreq_max_frequency_by_DVS(5),
		     mt_cpufreq_max_frequency_by_DVS(6),
		     mt_cpufreq_max_frequency_by_DVS(7));
		hrtimer_start(&mt_ptp_log_timer, ktime, HRTIMER_MODE_REL);
	} while (!kthread_should_stop());

	return 0;
}

enum hrtimer_restart mt_ptp_volt_timer_func(struct hrtimer *timer)
{
	mt_ptp_volt_timer_flag = 1;
	wake_up_interruptible(&mt_ptp_volt_timer_waiter);
	return HRTIMER_NORESTART;
}

int mt_ptp_volt_thread_handler(void *unused)
{
	do {
		ktime_set(mt_ptp_volt_period_s, mt_ptp_volt_period_ns);

		wait_event_interruptible(mt_ptp_volt_timer_waiter,
		mt_ptp_volt_timer_flag != 0);
		mt_ptp_volt_timer_flag = 0;

		PTP_set_ptp_volt();

#ifdef CONFIG_MTK_RAM_CONSOLE
		/* update set volt status for this bank */
		aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
				      (1 << PTP_CPU_LITTLE_IS_SET_VOLT));
#endif
		if (is_cpufreq_disabled_by_ptpod()) {
			mt_cpufreq_enable_by_ptpod();   /* enable DVFS */
		}

		mt_cpufreq_voltage_set_by_ptpod(ptpod_pmic_volt,
						ptp_array_size);
#ifdef CONFIG_MTK_RAM_CONSOLE
		/* clear out set volt status for this bank */
		aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() &
				      ~(1 << PTP_CPU_LITTLE_IS_SET_VOLT));
#endif
	} while (!kthread_should_stop());

	return 0;
}

unsigned int PTP_get_ptp_level(void)
{
#if defined(CONFIG_MTK_FORCE_CPU_27T)
	return 3;		/* 1.5G */
#else
	unsigned int spd_bin_resv = 0, segment = 0, ret = 0;

	segment = (get_devinfo_with_index(15) >> 31) & 0x1;	/* M_HW_RES4 */
	if (segment == 0)
		spd_bin_resv = get_devinfo_with_index(15) & 0xFFFF;
	else
		spd_bin_resv = (get_devinfo_with_index(15) >> 16) & 0x7FFF;

	ptp_debug("spd_bin_resv(%d), M_HW_RES4(0x%x), M_HW2_RES2(0x%x)\n",
		   spd_bin_resv, get_devinfo_with_index(15),
		   get_devinfo_with_index(24));

	if (segment == 1) {
		if (spd_bin_resv == 0x1000)
			ret = 0;	/* 1.3G */
		else if (spd_bin_resv == 0x1001)
			ret = 5;	/* 1.222G */
		else if (spd_bin_resv == 0x1003)
			ret = 6;	/* 1.118G */
		else
			ret = 0;	/* 1.3G */
	} else if (spd_bin_resv != 0) {
		if (spd_bin_resv == 0x1000)
			ret = 3;	/* 1.5G, 8127T */
		else if (spd_bin_resv == 0x1001)
			ret = 0;	/* 1.3G, 8127 */
		else if (spd_bin_resv == 0x1003)
			ret = 0;	/* 1.3G, 8117 */
		else
			ret = 0;	/* 1.3G, */
	} else {		/* free */

		spd_bin_resv = get_devinfo_with_index(24) & 0x7;
		switch (spd_bin_resv) {
		case 0:
			ret = 0;	/* 1.3G */
			break;
		case 1:
			ret = 3;	/* 1.5G */
			break;
		case 2:
			ret = 0;	/* 1.4G */
			break;
		case 3:
			ret = 0;	/* 1.3G */
			break;
		case 4:
			ret = 0;	/* 1.2G */
			break;
		case 5:
			ret = 0;	/* 1.1G */
			break;
		case 6:
			ret = 0;	/* 1.0G */
			break;
		case 7:
			ret = 0;	/* 1.0G */
			break;
		default:
			ret = 0;	/* 1.3G */
			break;
		}
	}
	return ret;
#endif
}

static void PTP_Initialization_01(struct ptp_init_t *ptp_init_val)
{
	unsigned int temp_i, temp_filter, temp_value;

	/* config PTP register */
	ptp_write(PTP_DESCHAR,
		  ((((ptp_init_val->BDES) << 8) & 0xff00) |
		  ((ptp_init_val->MDES) & 0xff)));
	ptp_write(PTP_TEMPCHAR,
		  ((((ptp_init_val->VCO) << 16) & 0xff0000) |
		   (((ptp_init_val->MTDES) << 8) & 0xff00) |
		   ((ptp_init_val->DVTFIXED) & 0xff)));
	ptp_write(PTP_DETCHAR,
		  ((((ptp_init_val->DCBDET) << 8) & 0xff00) |
		  ((ptp_init_val->DCMDET) & 0xff)));
	ptp_write(PTP_AGECHAR,
		  ((((ptp_init_val->AGEDELTA) << 8) & 0xff00) |
		  ((ptp_init_val->AGEM) & 0xff)));
	ptp_write(PTP_DCCONFIG, ((ptp_init_val->DCCONFIG)));
	ptp_write(PTP_AGECONFIG, ((ptp_init_val->AGECONFIG)));

	if (ptp_init_val->AGEM == 0x0) {
		ptp_write(PTP_RUNCONFIG, 0x80000000);
	} else {
		temp_value = 0x0;

		for (temp_i = 0; temp_i < 24; temp_i += 2) {
			temp_filter = 0x3 << temp_i;

			if (((ptp_init_val->AGECONFIG) & temp_filter) == 0x0) {
				temp_value |= (0x1 << temp_i);
			} else {
				temp_value |= ((ptp_init_val->AGECONFIG) &
				temp_filter);
			}
		}
		ptp_write(PTP_RUNCONFIG, temp_value);
	}

	ptp_write(PTP_FREQPCT30,
		  ((((ptp_init_val->FREQPCT3) << 24) & 0xff000000) |
		   (((ptp_init_val->FREQPCT2) << 16) & 0xff0000) |
		   (((ptp_init_val->FREQPCT1) << 8) & 0xff00) |
		   ((ptp_init_val->FREQPCT0) & 0xff)));
	ptp_write(PTP_FREQPCT74,
		  ((((ptp_init_val->FREQPCT7) << 24) & 0xff000000) |
		   (((ptp_init_val->FREQPCT6) << 16) & 0xff0000) |
		   (((ptp_init_val->FREQPCT5) << 8) & 0xff00) |
		   ((ptp_init_val->FREQPCT4) & 0xff)));

	ptp_write(PTP_LIMITVALS,
		  ((((ptp_init_val->VMAX) << 24) & 0xff000000) |
		   (((ptp_init_val->VMIN) << 16) & 0xff0000) |
		   (((ptp_init_val->DTHI) << 8) & 0xff00) |
		   ((ptp_init_val->DTLO) & 0xff)));
	ptp_write(PTP_VBOOT, (((ptp_init_val->VBOOT) & 0xff)));
	ptp_write(PTP_DETWINDOW, (((ptp_init_val->DETWINDOW) & 0xffff)));
	ptp_write(PTP_PTPCONFIG, (((ptp_init_val->DETMAX) & 0xffff)));

	/* clear all pending PTP interrupt & config PTPINTEN */
	ptp_write(PTP_PTPINTSTS, 0xffffffff);

	ptp_write(PTP_PTPINTEN, 0x00005f01);

	/* enable PTP INIT measurement */
	ptp_write(PTP_PTPEN, 0x00000001);
}

static void PTP_Initialization_02(struct ptp_init_t *ptp_init_val)
{
	unsigned int temp_i, temp_filter, temp_value;

	/* config PTP register */
	ptp_write(PTP_DESCHAR,
		  ((((ptp_init_val->BDES) << 8) & 0xff00) |
		  ((ptp_init_val->MDES) & 0xff)));
	ptp_write(PTP_TEMPCHAR,
		  ((((ptp_init_val->VCO) << 16) & 0xff0000) |
		   (((ptp_init_val->MTDES) << 8) & 0xff00) |
		   ((ptp_init_val->DVTFIXED) & 0xff)));
	ptp_write(PTP_DETCHAR,
		  ((((ptp_init_val->DCBDET) << 8) & 0xff00) |
		  ((ptp_init_val->DCMDET) & 0xff)));
	ptp_write(PTP_AGECHAR,
		  ((((ptp_init_val->AGEDELTA) << 8) & 0xff00) |
		  ((ptp_init_val->AGEM) & 0xff)));
	ptp_write(PTP_DCCONFIG, ((ptp_init_val->DCCONFIG)));
	ptp_write(PTP_AGECONFIG, ((ptp_init_val->AGECONFIG)));

	if (ptp_init_val->AGEM == 0x0) {
		ptp_write(PTP_RUNCONFIG, 0x80000000);
	} else {
		temp_value = 0x0;

		for (temp_i = 0; temp_i < 24; temp_i += 2) {
			temp_filter = 0x3 << temp_i;

			if (((ptp_init_val->AGECONFIG) & temp_filter) == 0x0) {
				temp_value |= (0x1 << temp_i);
			} else {
				temp_value |= ((ptp_init_val->AGECONFIG) &
				temp_filter);
			}
		}

		ptp_write(PTP_RUNCONFIG, temp_value);
	}

	ptp_write(PTP_FREQPCT30,
		  ((((ptp_init_val->FREQPCT3) << 24) & 0xff000000) |
		   (((ptp_init_val->FREQPCT2) << 16) & 0xff0000) |
		   (((ptp_init_val->FREQPCT1) << 8) & 0xff00) |
		   ((ptp_init_val->FREQPCT0) & 0xff)));
	ptp_write(PTP_FREQPCT74,
		  ((((ptp_init_val->FREQPCT7) << 24) & 0xff000000) |
		   (((ptp_init_val->FREQPCT6) << 16) & 0xff0000) |
		   (((ptp_init_val->FREQPCT5) << 8) & 0xff00) |
		   ((ptp_init_val->FREQPCT4) & 0xff)));

	ptp_write(PTP_LIMITVALS,
		  ((((ptp_init_val->VMAX) << 24) & 0xff000000) |
		   (((ptp_init_val->VMIN) << 16) & 0xff0000) |
		   (((ptp_init_val->DTHI) << 8) & 0xff00) |
		   ((ptp_init_val->DTLO) & 0xff)));
	ptp_write(PTP_VBOOT, (((ptp_init_val->VBOOT) & 0xff)));
	ptp_write(PTP_DETWINDOW, (((ptp_init_val->DETWINDOW) & 0xffff)));
	ptp_write(PTP_PTPCONFIG, (((ptp_init_val->DETMAX) & 0xffff)));

	/* clear all pending PTP interrupt & config PTPINTEN */
	ptp_write(PTP_PTPINTSTS, 0xffffffff);

	ptp_write(PTP_PTPINTEN, 0x00005f01);

	ptp_write(PTP_INIT2VALS,
		  ((((ptp_init_val->AGEVOFFSETIN) << 16) & 0xffff0000) |
		   ((ptp_init_val->DCVOFFSETIN) & 0xffff)));

	/* enable PTP INIT measurement */
	ptp_write(PTP_PTPEN, 0x00000005);
}

static void PTP_Monitor_Mode(struct ptp_init_t *ptp_init_val)
{
	unsigned int temp_i, temp_filter, temp_value;

	/* config PTP register */
	ptp_write(PTP_DESCHAR,
		  ((((ptp_init_val->BDES) << 8) & 0xff00) |
		  ((ptp_init_val->MDES) & 0xff)));
	ptp_write(PTP_TEMPCHAR,
		  ((((ptp_init_val->VCO) << 16) & 0xff0000) |
		   (((ptp_init_val->MTDES) << 8) & 0xff00) |
		   ((ptp_init_val->DVTFIXED) & 0xff)));
	ptp_write(PTP_DETCHAR,
		  ((((ptp_init_val->DCBDET) << 8) & 0xff00) |
		  ((ptp_init_val->DCMDET) & 0xff)));
	ptp_write(PTP_AGECHAR,
		  ((((ptp_init_val->AGEDELTA) << 8) & 0xff00) |
		  ((ptp_init_val->AGEM) & 0xff)));
	ptp_write(PTP_DCCONFIG, ((ptp_init_val->DCCONFIG)));
	ptp_write(PTP_AGECONFIG, ((ptp_init_val->AGECONFIG)));
	ptp_write(PTP_TSCALCS,
		  ((((ptp_init_val->BTS) << 12) & 0xfff000) |
		  ((ptp_init_val->MTS) & 0xfff)));

	if (ptp_init_val->AGEM == 0x0) {
		ptp_write(PTP_RUNCONFIG, 0x80000000);
	} else {
		temp_value = 0x0;

		for (temp_i = 0; temp_i < 24; temp_i += 2) {
			temp_filter = 0x3 << temp_i;

			if (((ptp_init_val->AGECONFIG) & temp_filter) == 0x0)
				temp_value |= (0x1 << temp_i);
			else
				temp_value |= ((ptp_init_val->AGECONFIG) & temp_filter);
		}

		ptp_write(PTP_RUNCONFIG, temp_value);
	}

	ptp_write(PTP_FREQPCT30,
		  ((((ptp_init_val->FREQPCT3) << 24) & 0xff000000) |
		   (((ptp_init_val->FREQPCT2) << 16) & 0xff0000) |
		   (((ptp_init_val->FREQPCT1) << 8) & 0xff00) |
		   ((ptp_init_val->FREQPCT0) & 0xff)));
	ptp_write(PTP_FREQPCT74,
		  ((((ptp_init_val->FREQPCT7) << 24) & 0xff000000) |
		   (((ptp_init_val->FREQPCT6) << 16) & 0xff0000) |
		   (((ptp_init_val->FREQPCT5) << 8) & 0xff00) |
		   ((ptp_init_val->FREQPCT4) & 0xff)));

	ptp_write(PTP_LIMITVALS,
		  ((((ptp_init_val->VMAX) << 24) & 0xff000000) |
		   (((ptp_init_val->VMIN) << 16) & 0xff0000) |
		   (((ptp_init_val->DTHI) << 8) & 0xff00) |
		   ((ptp_init_val->DTLO) & 0xff)));
	ptp_write(PTP_VBOOT, (((ptp_init_val->VBOOT) & 0xff)));
	ptp_write(PTP_DETWINDOW, (((ptp_init_val->DETWINDOW) & 0xffff)));
	ptp_write(PTP_PTPCONFIG, (((ptp_init_val->DETMAX) & 0xffff)));

	/* clear all pending PTP interrupt & config PTPINTEN */
	ptp_write(PTP_PTPINTSTS, 0xffffffff);

	ptp_write(PTP_PTPINTEN, 0x00FF0000);

	/* enable PTP monitor mode */
	ptp_write(PTP_PTPEN, 0x00000002);
}

unsigned int PTP_INIT_01(void)
{
	struct ptp_init_t ptp_init_value;
	unsigned int vboot;

	/* disable slow idle */
	ptp_data[0] = 0xffffffff;

	ptp_debug("PTP_INIT_01() start (ptp_level = 0x%x).\n", ptp_level);

	ptp_init_value.PTPINITEN = (val_0) & 0x1;
	ptp_init_value.PTPMONEN = (val_0 >> 1) & 0x1;
	ptp_init_value.MDES = (val_0 >> 8) & 0xff;
	ptp_init_value.BDES = (val_0 >> 16) & 0xff;
	ptp_init_value.DCMDET = (val_0 >> 24) & 0xff;

	ptp_init_value.DCCONFIG = (val_1) & 0xffffff;
	ptp_init_value.DCBDET = (val_1 >> 24) & 0xff;

	ptp_init_value.AGECONFIG = (val_2) & 0xffffff;
	ptp_init_value.AGEM = (val_2 >> 24) & 0xff;

	ptp_init_value.AGEDELTA = (val_3) & 0xff;
	ptp_init_value.DVTFIXED = (val_3 >> 8) & 0xff;
	ptp_init_value.MTDES = (val_3 >> 16) & 0xff;
	ptp_init_value.VCO = (val_3 >> 24) & 0xff;

	ptp_init_value.FREQPCT0 = freq_0;
	ptp_init_value.FREQPCT1 = freq_1;
	ptp_init_value.FREQPCT2 = freq_2;
	ptp_init_value.FREQPCT3 = freq_3;
	ptp_init_value.FREQPCT4 = freq_4;
	ptp_init_value.FREQPCT5 = freq_5;
	ptp_init_value.FREQPCT6 = freq_6;
	ptp_init_value.FREQPCT7 = freq_7;
	/* 100 us, This is the PTP Detector sampling time as represented
	   in cycles of bclk_ck during INIT. 52 MHz */
	ptp_init_value.DETWINDOW = 0xa28;
	ptp_init_value.VMAX = 0x60;	/* 1.3v (700mv + n * 6.25mv) */

#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
	ptp_init_value.VMIN = 0x38;	/* 1.05v (700mv + n * 6.25mv) */
#else
	ptp_init_value.VMIN = 0x48;	/* 1.15v (700mv + n * 6.25mv) */
#endif

	ptp_init_value.DTHI = 0x01;	/* positive */
	ptp_init_value.DTLO = 0xfe;	/* negative (2¡¦s compliment) */
	ptp_init_value.VBOOT = 0x48;	/* 1.15v  (700mv + n * 6.25mv) */
	/* This timeout value is in cycles of bclk_ck. */
	ptp_init_value.DETMAX = 0xffff;

	if (ptp_init_value.PTPINITEN == 0x0) {
		ptp_error("PTPINITEN = 0x%x\n", ptp_init_value.PTPINITEN);
		/* enable slow idle */
		ptp_data[0] = 0;

		return 1;
	}

	ptp_debug("PTPINITEN   = 0x%x\n", ptp_init_value.PTPINITEN);
	ptp_debug("PTPMONEN    = 0x%x\n", ptp_init_value.PTPMONEN);
	ptp_debug("MDES        = 0x%x\n", ptp_init_value.MDES);
	ptp_debug("BDES        = 0x%x\n", ptp_init_value.BDES);
	ptp_debug("DCMDET      = 0x%x\n", ptp_init_value.DCMDET);
	ptp_debug("DCCONFIG    = 0x%x\n", ptp_init_value.DCCONFIG);
	ptp_debug("DCBDET      = 0x%x\n", ptp_init_value.DCBDET);
	ptp_debug("AGECONFIG   = 0x%x\n", ptp_init_value.AGECONFIG);
	ptp_debug("AGEM        = 0x%x\n", ptp_init_value.AGEM);
	ptp_debug("AGEDELTA    = 0x%x\n", ptp_init_value.AGEDELTA);
	ptp_debug("DVTFIXED    = 0x%x\n", ptp_init_value.DVTFIXED);
	ptp_debug("MTDES       = 0x%x\n", ptp_init_value.MTDES);
	ptp_debug("VCO         = 0x%x\n", ptp_init_value.VCO);
	ptp_debug("FREQPCT0    = %d\n", ptp_init_value.FREQPCT0);
	ptp_debug("FREQPCT1    = %d\n", ptp_init_value.FREQPCT1);
	ptp_debug("FREQPCT2    = %d\n", ptp_init_value.FREQPCT2);
	ptp_debug("FREQPCT3    = %d\n", ptp_init_value.FREQPCT3);
	ptp_debug("FREQPCT4    = %d\n", ptp_init_value.FREQPCT4);
	ptp_debug("FREQPCT5    = %d\n", ptp_init_value.FREQPCT5);
	ptp_debug("FREQPCT6    = %d\n", ptp_init_value.FREQPCT6);
	ptp_debug("FREQPCT7    = %d\n", ptp_init_value.FREQPCT7);

	bus_dcm_disable();	/* make sure bus dcm disable for PTP init 01 */
	mt_fh_popod_save();	/* disable frequency hopping (main PLL) */
	/* disable DVFS and set vproc = 1.15v (1 GHz) */
	mt_cpufreq_disable_by_ptpod();

	vboot = PTP_VOLT_TO_PMIC_VAL(mt_cpufreq_cur_vproc());
	if (vboot != ptp_init_value.VBOOT)
		ptp_error("buck(vboot): 0x%x & VBOOT: 0x%x matching fail\n",
			  vboot, ptp_init_value.VBOOT);
#ifdef CONFIG_MTK_RAM_CONSOLE
	/* record vboot of this bank */
	aee_rr_rec_ptp_vboot(((unsigned long long)(vboot) << 0) |
			     (aee_rr_curr_ptp_vboot() &
			     ~((unsigned long long)(0xFF) << 0)));
#endif

	PTP_Initialization_01(&ptp_init_value);

	return 0;
}

unsigned int PTP_INIT_02(void)
{
	struct ptp_init_t ptp_init_value;

	/* disable slow idle */
	ptp_data[0] = 0xffffffff;

	if (ptp_log_en)
		ptp_notice("PTP_INIT_02() start (ptp_level = 0x%x).\n", ptp_level);

	ptp_init_value.PTPINITEN = (val_0) & 0x1;
	ptp_init_value.PTPMONEN = (val_0 >> 1) & 0x1;
	ptp_init_value.MDES = (val_0 >> 8) & 0xff;
	ptp_init_value.BDES = (val_0 >> 16) & 0xff;
	ptp_init_value.DCMDET = (val_0 >> 24) & 0xff;

	ptp_init_value.DCCONFIG = (val_1) & 0xffffff;
	ptp_init_value.DCBDET = (val_1 >> 24) & 0xff;

	ptp_init_value.AGECONFIG = (val_2) & 0xffffff;
	ptp_init_value.AGEM = (val_2 >> 24) & 0xff;

	ptp_init_value.AGEDELTA = (val_3) & 0xff;
	ptp_init_value.DVTFIXED = (val_3 >> 8) & 0xff;
	ptp_init_value.MTDES = (val_3 >> 16) & 0xff;
	ptp_init_value.VCO = (val_3 >> 24) & 0xff;

	ptp_init_value.FREQPCT0 = freq_0;
	ptp_init_value.FREQPCT1 = freq_1;
	ptp_init_value.FREQPCT2 = freq_2;
	ptp_init_value.FREQPCT3 = freq_3;
	ptp_init_value.FREQPCT4 = freq_4;
	ptp_init_value.FREQPCT5 = freq_5;
	ptp_init_value.FREQPCT6 = freq_6;
	ptp_init_value.FREQPCT7 = freq_7;

	/* 100 us, This is the PTP Detector sampling time as
	   represented in cycles of bclk_ck during INIT. 52 MHz */
	ptp_init_value.DETWINDOW = 0xa28;
	ptp_init_value.VMAX = 0x60;	/* 1.3v (700mv + n * 6.25mv) */

#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
	ptp_init_value.VMIN = 0x38;	/* 1.05v (700mv + n * 6.25mv) */
#else
	ptp_init_value.VMIN = 0x48;	/* 1.15v (700mv + n * 6.25mv) */
#endif

	ptp_init_value.DTHI = 0x01;	/* positive */
	ptp_init_value.DTLO = 0xfe;	/* negative (2¡¦s compliment) */
	ptp_init_value.VBOOT = 0x48;	/* 1.15v  (700mv + n * 6.25mv) */
	/* This timeout value is in cycles of bclk_ck. */
	ptp_init_value.DETMAX = 0xffff;

	ptp_init_value.DCVOFFSETIN = ptp_dcvoffset;
	ptp_init_value.AGEVOFFSETIN = ptp_agevoffset;

	if (ptp_log_en) {
		ptp_notice("DCVOFFSETIN = 0x%x\n", ptp_init_value.DCVOFFSETIN);
		ptp_notice("AGEVOFFSETIN = 0x%x\n", ptp_init_value.AGEVOFFSETIN);
	}

	if (ptp_init_value.PTPINITEN == 0x0) {
		ptp_error("PTPINITEN = 0x%x\n", ptp_init_value.PTPINITEN);
		/* enable slow idle */
		ptp_data[0] = 0;

		return 1;
	}

	if (ptp_log_en) {
		ptp_isr_info("PTPINITEN = 0x%x\n", ptp_init_value.PTPINITEN);
		ptp_isr_info("PTPMONEN = 0x%x\n", ptp_init_value.PTPMONEN);
		ptp_isr_info("MDES = 0x%x\n", ptp_init_value.MDES);
		ptp_isr_info("BDES = 0x%x\n", ptp_init_value.BDES);
		ptp_isr_info("DCMDET = 0x%x\n", ptp_init_value.DCMDET);
		ptp_isr_info("DCCONFIG = 0x%x\n", ptp_init_value.DCCONFIG);
		ptp_isr_info("DCBDET = 0x%x\n", ptp_init_value.DCBDET);
		ptp_isr_info("AGECONFIG = 0x%x\n", ptp_init_value.AGECONFIG);
		ptp_isr_info("AGEM = 0x%x\n", ptp_init_value.AGEM);
		ptp_isr_info("AGEDELTA = 0x%x\n", ptp_init_value.AGEDELTA);
		ptp_isr_info("DVTFIXED = 0x%x\n", ptp_init_value.DVTFIXED);
		ptp_isr_info("MTDES = 0x%x\n", ptp_init_value.MTDES);
		ptp_isr_info("VCO = 0x%x\n", ptp_init_value.VCO);
		ptp_isr_info("DCVOFFSETIN = 0x%x\n", ptp_init_value.DCVOFFSETIN);
		ptp_isr_info("AGEVOFFSETIN = 0x%x\n", ptp_init_value.AGEVOFFSETIN);
		ptp_isr_info("FREQPCT0 = %d\n", ptp_init_value.FREQPCT0);
		ptp_isr_info("FREQPCT1 = %d\n", ptp_init_value.FREQPCT1);
		ptp_isr_info("FREQPCT2 = %d\n", ptp_init_value.FREQPCT2);
		ptp_isr_info("FREQPCT3 = %d\n", ptp_init_value.FREQPCT3);
		ptp_isr_info("FREQPCT4 = %d\n", ptp_init_value.FREQPCT4);
		ptp_isr_info("FREQPCT5 = %d\n", ptp_init_value.FREQPCT5);
		ptp_isr_info("FREQPCT6 = %d\n", ptp_init_value.FREQPCT6);
		ptp_isr_info("FREQPCT7 = %d\n", ptp_init_value.FREQPCT7);
	}

	PTP_Initialization_02(&ptp_init_value);

	return 0;
}

unsigned int PTP_MON_MODE(void)
{
	struct ptp_init_t ptp_init_value;
	struct TS_PTPOD ts_info;

	if (ptp_log_en)
		ptp_notice("PTP_MON_MODE() start (ptp_level = 0x%x).\n", ptp_level);

	ptp_init_value.PTPINITEN = (val_0) & 0x1;
	ptp_init_value.PTPMONEN = (val_0 >> 1) & 0x1;
	ptp_init_value.ADC_CALI_EN = (val_0 >> 2) & 0x1;
	ptp_init_value.MDES = (val_0 >> 8) & 0xff;
	ptp_init_value.BDES = (val_0 >> 16) & 0xff;
	ptp_init_value.DCMDET = (val_0 >> 24) & 0xff;

	ptp_init_value.DCCONFIG = (val_1) & 0xffffff;
	ptp_init_value.DCBDET = (val_1 >> 24) & 0xff;

	ptp_init_value.AGECONFIG = (val_2) & 0xffffff;
	ptp_init_value.AGEM = (val_2 >> 24) & 0xff;

	ptp_init_value.AGEDELTA = (val_3) & 0xff;
	ptp_init_value.DVTFIXED = (val_3 >> 8) & 0xff;
	ptp_init_value.MTDES = (val_3 >> 16) & 0xff;
	ptp_init_value.VCO = (val_3 >> 24) & 0xff;

	get_thermal_slope_intercept(&ts_info);
	ptp_init_value.MTS = ts_info.ts_MTS;
	ptp_init_value.BTS = ts_info.ts_BTS;

	ptp_init_value.FREQPCT0 = freq_0;
	ptp_init_value.FREQPCT1 = freq_1;
	ptp_init_value.FREQPCT2 = freq_2;
	ptp_init_value.FREQPCT3 = freq_3;
	ptp_init_value.FREQPCT4 = freq_4;
	ptp_init_value.FREQPCT5 = freq_5;
	ptp_init_value.FREQPCT6 = freq_6;
	ptp_init_value.FREQPCT7 = freq_7;
	/* 100 us, This is the PTP Detector sampling time as
	   represented in cycles of bclk_ck during INIT. 52 MHz */
	ptp_init_value.DETWINDOW = 0xa28;
	ptp_init_value.VMAX = 0x60;	/* 1.3v (700mv + n * 6.25mv) */

#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
	ptp_init_value.VMIN = 0x38;	/* 1.05v (700mv + n * 6.25mv) */
#else
	ptp_init_value.VMIN = 0x48;	/* 1.15v (700mv + n * 6.25mv) */
#endif

	ptp_init_value.DTHI = 0x01;	/* positive */
	ptp_init_value.DTLO = 0xfe;	/* negative (2¡¦s compliment) */
	ptp_init_value.VBOOT = 0x48;	/* 1.15v  (700mv + n * 6.25mv) */
	/* This timeout value is in cycles of bclk_ck. */
	ptp_init_value.DETMAX = 0xffff;

	if ((ptp_init_value.PTPINITEN == 0x0) || (ptp_init_value.PTPMONEN == 0x0)) {
		ptp_error("PTPINITEN = %x, PTPMONEN = %x, ADC_CALI_EN = %x\n",
			   ptp_init_value.PTPINITEN, ptp_init_value.PTPMONEN,
			   ptp_init_value.ADC_CALI_EN);
		return 1;
	}

	if (ptp_log_en) {
		ptp_isr_info("PTPINITEN = 0x%x\n", ptp_init_value.PTPINITEN);
		ptp_isr_info("PTPMONEN = 0x%x\n", ptp_init_value.PTPMONEN);
		ptp_isr_info("MDES = 0x%x\n", ptp_init_value.MDES);
		ptp_isr_info("BDES = 0x%x\n", ptp_init_value.BDES);
		ptp_isr_info("DCMDET = 0x%x\n", ptp_init_value.DCMDET);
		ptp_isr_info("DCCONFIG = 0x%x\n", ptp_init_value.DCCONFIG);
		ptp_isr_info("DCBDET = 0x%x\n", ptp_init_value.DCBDET);
		ptp_isr_info("AGECONFIG = 0x%x\n", ptp_init_value.AGECONFIG);
		ptp_isr_info("AGEM = 0x%x\n", ptp_init_value.AGEM);
		ptp_isr_info("AGEDELTA = 0x%x\n", ptp_init_value.AGEDELTA);
		ptp_isr_info("DVTFIXED = 0x%x\n", ptp_init_value.DVTFIXED);
		ptp_isr_info("MTDES = 0x%x\n", ptp_init_value.MTDES);
		ptp_isr_info("VCO = 0x%x\n", ptp_init_value.VCO);
		ptp_isr_info("MTS = 0x%x\n", ptp_init_value.MTS);
		ptp_isr_info("BTS = 0x%x\n", ptp_init_value.BTS);
		ptp_isr_info("FREQPCT0 = %d\n", ptp_init_value.FREQPCT0);
		ptp_isr_info("FREQPCT1 = %d\n", ptp_init_value.FREQPCT1);
		ptp_isr_info("FREQPCT2 = %d\n", ptp_init_value.FREQPCT2);
		ptp_isr_info("FREQPCT3 = %d\n", ptp_init_value.FREQPCT3);
		ptp_isr_info("FREQPCT4 = %d\n", ptp_init_value.FREQPCT4);
		ptp_isr_info("FREQPCT5 = %d\n", ptp_init_value.FREQPCT5);
		ptp_isr_info("FREQPCT6 = %d\n", ptp_init_value.FREQPCT6);
		ptp_isr_info("FREQPCT7 = %d\n", ptp_init_value.FREQPCT7);
	}

	PTP_Monitor_Mode(&ptp_init_value);

	return 0;
}

static void PTP_set_ptp_volt(void)
{
#if SET_PMIC_VOLT
	int cur_temp, low_temp_offset;
#ifdef CONFIG_MTK_RAM_CONSOLE
	int dvfs_index;
	unsigned long long temp_long;
	unsigned long long temp_cur =
		(unsigned long long)aee_rr_curr_ptp_temp();
#endif
	cur_temp = tscpu_get_cpu_temp();

	if (cur_temp <= 33000)
		low_temp_offset = 10;
	else
		low_temp_offset = 0;

	ptp_array_size = 0;

	if (freq_0 != 0) {
		ptpod_pmic_volt[0] =
			ptp_volt_0 + mt_ptp_offset + low_temp_offset;
		if (ptpod_pmic_volt[0] > 0x60) {
			ptpod_pmic_volt[0] = 0x60;
		} else {
#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
			if (ptpod_pmic_volt[0] < 0x38)
				ptpod_pmic_volt[0] = 0x38;
#else
			if (ptpod_pmic_volt[0] < 0x48)
				ptpod_pmic_volt[0] = 0x48;
#endif
		}
		ptp_array_size++;
	}

	if (freq_1 != 0) {
		ptpod_pmic_volt[1] =
			ptp_volt_1 + mt_ptp_offset + low_temp_offset;
		if (ptpod_pmic_volt[1] > 0x60) {
			ptpod_pmic_volt[1] = 0x60;
		} else {
#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
			if (ptpod_pmic_volt[1] < 0x38)
				ptpod_pmic_volt[1] = 0x38;
#else
			if (ptpod_pmic_volt[1] < 0x48)
				ptpod_pmic_volt[1] = 0x48;
#endif
		}
		ptp_array_size++;
	}

	if (freq_2 != 0) {
		ptpod_pmic_volt[2] =
			ptp_volt_2 + mt_ptp_offset + low_temp_offset;
		if (ptpod_pmic_volt[2] > 0x60) {
			ptpod_pmic_volt[2] = 0x60;
		} else {
#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
			if (ptpod_pmic_volt[2] < 0x38)
				ptpod_pmic_volt[2] = 0x38;
#else
			if (ptpod_pmic_volt[2] < 0x48)
				ptpod_pmic_volt[2] = 0x48;
#endif
		}
		ptp_array_size++;
	}

	if (freq_3 != 0) {
		ptpod_pmic_volt[3] =
			ptp_volt_3 + mt_ptp_offset + low_temp_offset;
		if (ptpod_pmic_volt[3] > 0x60) {
			ptpod_pmic_volt[3] = 0x60;
		} else {
#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
			if (ptpod_pmic_volt[3] < 0x38)
				ptpod_pmic_volt[3] = 0x38;
#else
			if (ptpod_pmic_volt[3] < 0x48)
				ptpod_pmic_volt[3] = 0x48;
#endif
		}
		ptp_array_size++;
	}

	if (freq_4 != 0) {
		ptpod_pmic_volt[4] =
			ptp_volt_4 + mt_ptp_offset + low_temp_offset;
		if (ptpod_pmic_volt[4] > 0x60) {
			ptpod_pmic_volt[4] = 0x60;
		} else {
#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
			if (ptpod_pmic_volt[4] < 0x38)
				ptpod_pmic_volt[4] = 0x38;
#else
			if (ptpod_pmic_volt[4] < 0x48)
				ptpod_pmic_volt[4] = 0x48;
#endif
		}
		ptp_array_size++;
	}

	if (freq_5 != 0) {
		ptpod_pmic_volt[5] =
			ptp_volt_5 + mt_ptp_offset + low_temp_offset;
		if (ptpod_pmic_volt[5] > 0x60) {
			ptpod_pmic_volt[5] = 0x60;
		} else {
#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
			if (ptpod_pmic_volt[5] < 0x38)
				ptpod_pmic_volt[5] = 0x38;
#else
			if (ptpod_pmic_volt[5] < 0x48)
				ptpod_pmic_volt[5] = 0x48;
#endif
		}
		ptp_array_size++;
	}

	if (freq_6 != 0) {
		ptpod_pmic_volt[6] =
			ptp_volt_6 + mt_ptp_offset + low_temp_offset;
		if (ptpod_pmic_volt[6] > 0x60) {
			ptpod_pmic_volt[6] = 0x60;
		} else {
#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
			if (ptpod_pmic_volt[6] < 0x38)
				ptpod_pmic_volt[6] = 0x38;
#else
			if (ptpod_pmic_volt[6] < 0x48)
				ptpod_pmic_volt[6] = 0x48;
#endif
		}
		ptp_array_size++;
	}

	if (freq_7 != 0) {
		ptpod_pmic_volt[7] =
			ptp_volt_7 + mt_ptp_offset + low_temp_offset;
		if (ptpod_pmic_volt[7] > 0x60) {
			ptpod_pmic_volt[7] = 0x60;
		} else {
#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
			if (ptpod_pmic_volt[7] < 0x38)
				ptpod_pmic_volt[7] = 0x38;
#else
			if (ptpod_pmic_volt[7] < 0x48)
				ptpod_pmic_volt[7] = 0x48;
#endif
		}
		ptp_array_size++;
	}
#ifdef CONFIG_MTK_RAM_CONSOLE
	/* update AEE temperature */
	temp_long = (unsigned long long) (cur_temp / 1000);
	if (temp_long != 0) {
		aee_rr_rec_ptp_temp(temp_long <<
				    (8 * PTP_CPU_LITTLE_IS_SET_VOLT) |
				    (temp_cur & ~((unsigned long long)0xFF <<
				    (8 * PTP_CPU_LITTLE_IS_SET_VOLT))));
	}

	/* update AEE voltage */
	for (dvfs_index = 0; dvfs_index < ptp_array_size; dvfs_index++) {
		aee_rr_rec_ptp_cpu_little_volt(
			((unsigned long long)(ptpod_pmic_volt[dvfs_index]) <<
			(8 * dvfs_index)) |
			(aee_rr_curr_ptp_cpu_little_volt() &
			~((unsigned long long)(0xFF) << (8 * dvfs_index))));
	}
#endif
#endif
}

irqreturn_t mt_ptp_isr(int irq, void *dev_id)
{
	unsigned int i, PTPINTSTS, temp, temp_0, temp_ptpen;
	ktime_t ktime = ktime_set(mt_ptp_volt_period_s, mt_ptp_volt_period_ns);

	PTPINTSTS = ptp_read(PTP_PTPINTSTS);
	temp_ptpen = ptp_read(PTP_PTPEN);

	if (ptp_log_en) {
		ptp_isr_info("%s start.\n", __func__);
		ptp_isr_info("PTPINTSTS = 0x%x\n", PTPINTSTS);
		ptp_isr_info("PTP_PTPEN = 0x%x\n", temp_ptpen);
	}

	ptp_data[1] = ptp_read(PTP_DCVALUES);
	ptp_data[2] = ptp_read(PTP_AGECOUNT);

	if (ptp_log_en) {
		ptp_isr_info("PTP_DCVALUES = 0x%x\n", ptp_data[1]);
		ptp_isr_info("PTP_AGECOUNT = 0x%x\n", ptp_data[2]);
	}

	/* enable slow idle */
	ptp_data[0] = 0;

	if (PTPINTSTS == 0x1) {	/* PTP init1 or init2 */
		if ((temp_ptpen & 0x7) == 0x1) {	/* PTP init1 */
			for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++)
				reg_dump_data[i][PTP_PHASE_INIT01] =
					ptp_read(PTP_BASEADDR + reg_dump_addr_off[i]);

			ptp_notice("cpu: %s 01: VDN74:0x%08x, VDN30:0x%08x, DCVALUES:0x%08x\n",
				   __func__, ptp_read(PTP_VDESIGN74),
				   ptp_read(PTP_VDESIGN30), ptp_read(PTP_DCVALUES));

			/* Read & store 16 bit values DCVALUES.DCVOFFSET
			   and AGEVALUES.AGEVOFFSET for later use in INIT2
			   procedure */
			ptp_dcvoffset = ~(ptp_read(PTP_DCVALUES) & 0xffff) + 1;
			ptp_agevoffset = ptp_read(PTP_AGEVALUES) & 0xffff;

			/* Set PTPEN.PTPINITEN/PTPEN.PTPINIT2EN = 0x0 &
			   Clear PTP INIT interrupt PTPINTSTS = 0x00000001 */
			ptp_write(PTP_PTPEN, 0x0);
			ptp_write(PTP_PTPINTSTS, 0x1);

			/* enable frequency hopping (main PLL) */
			mt_fh_popod_restore();

			PTP_INIT_02();
		} else if ((temp_ptpen & 0x7) == 0x5) {	/* PTP init2 */
			for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++)
				reg_dump_data[i][PTP_PHASE_INIT02] =
					ptp_read(PTP_BASEADDR + reg_dump_addr_off[i]);

			ptp_notice("cpu: %s 02: VOP74:0x%08x, VOP30:0x%08x, DCVALUES:0x%08x\n",
				   __func__, ptp_read(PTP_VOP74),
				   ptp_read(PTP_VOP30), ptp_read(PTP_DCVALUES));

			/* read ptp_volt_0 ~ ptp_volt_3 */
			temp = ptp_read(PTP_VOP30);
			ptp_volt_0 = temp & 0xff;
			ptp_volt_1 = (temp >> 8) & 0xff;
			ptp_volt_2 = (temp >> 16) & 0xff;
			ptp_volt_3 = (temp >> 24) & 0xff;

			/* read ptp_volt_4 ~ ptp_volt_7 */
			temp = ptp_read(PTP_VOP74);
			ptp_volt_4 = temp & 0xff;
			ptp_volt_5 = (temp >> 8) & 0xff;
			ptp_volt_6 = (temp >> 16) & 0xff;
			ptp_volt_7 = (temp >> 24) & 0xff;

			/* save ptp_init2_volt_0 ~ ptp_init2_volt_7 */
			ptp_init2_volt_0 = ptp_volt_0;
			ptp_init2_volt_1 = ptp_volt_1;
			ptp_init2_volt_2 = ptp_volt_2;
			ptp_init2_volt_3 = ptp_volt_3;
			ptp_init2_volt_4 = ptp_volt_4;
			ptp_init2_volt_5 = ptp_volt_5;
			ptp_init2_volt_6 = ptp_volt_6;
			ptp_init2_volt_7 = ptp_volt_7;

			if (ptp_log_en) {
				ptp_isr_info("ptp_volt_0 = 0x%x\n", ptp_volt_0);
				ptp_isr_info("ptp_volt_1 = 0x%x\n", ptp_volt_1);
				ptp_isr_info("ptp_volt_2 = 0x%x\n", ptp_volt_2);
				ptp_isr_info("ptp_volt_3 = 0x%x\n", ptp_volt_3);
				ptp_isr_info("ptp_volt_4 = 0x%x\n", ptp_volt_4);
				ptp_isr_info("ptp_volt_5 = 0x%x\n", ptp_volt_5);
				ptp_isr_info("ptp_volt_6 = 0x%x\n", ptp_volt_6);
				ptp_isr_info("ptp_volt_7 = 0x%x\n", ptp_volt_7);
				ptp_isr_info("ptp_level = 0x%x\n", ptp_level);
			}

			hrtimer_start(&mt_ptp_volt_timer, ktime, HRTIMER_MODE_REL);

			/* Set PTPEN.PTPINITEN/PTPEN.PTPINIT2EN = 0x0 &
			   Clear PTP INIT interrupt PTPINTSTS = 0x00000001 */
			ptp_write(PTP_PTPEN, 0x0);
			ptp_write(PTP_PTPINTSTS, 0x1);
			PTP_MON_MODE();
		} else {
			/* error: init1 or init2, but enable setting is wrong. */
			ptp_error("error: init1 or init2, but enable setting is wrong.\n");
			ptp_error("PTP_PTPEN = 0x%x : PTPINTSTS = 0x%x\n",
				     temp_ptpen, PTPINTSTS);
			ptp_error("PTP_SMSTATE0 = 0x%x\n",
				     ptp_read(PTP_SMSTATE0));
			ptp_error("PTP_SMSTATE1 = 0x%x\n",
				     ptp_read(PTP_SMSTATE1));

			/* disable PTP */
			ptp_write(PTP_PTPEN, 0x0);

			/* Clear PTP interrupt PTPINTSTS */
			ptp_write(PTP_PTPINTSTS, 0x00ffffff);

			/* restore default DVFS table (PMIC) */
			mt_cpufreq_return_default_DVS_by_ptpod();
		}
	} else if ((PTPINTSTS & 0x00ff0000) != 0x0) {	/* PTP Monitor mode */
		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++)
			reg_dump_data[i][PTP_PHASE_MON] =
				ptp_read(PTP_BASEADDR + reg_dump_addr_off[i]);

		/* check if thermal sensor init completed? */
		temp_0 = (ptp_read(PTP_TEMP) & 0xff);

		if ((temp_0 > 0x4b) && (temp_0 < 0xd3)) {
			ptp_error("thermal has not been completed. PTP_TEMP = %x\n", temp_0);
		} else {
			/* read ptp_volt_0 ~ ptp_volt_3 */
			temp = ptp_read(PTP_VOP30);
			ptp_volt_0 = temp & 0xff;
			ptp_volt_1 = (temp >> 8) & 0xff;
			ptp_volt_2 = (temp >> 16) & 0xff;
			ptp_volt_3 = (temp >> 24) & 0xff;
			/* read ptp_volt_3 ~ ptp_volt_7 */
			temp = ptp_read(PTP_VOP74);
			ptp_volt_4 = temp & 0xff;
			ptp_volt_5 = (temp >> 8) & 0xff;
			ptp_volt_6 = (temp >> 16) & 0xff;
			ptp_volt_7 = (temp >> 24) & 0xff;

			if (ptp_log_en) {
				ptp_isr_info("ptp_volt_0 = 0x%x\n", ptp_volt_0);
				ptp_isr_info("ptp_volt_1 = 0x%x\n", ptp_volt_1);
				ptp_isr_info("ptp_volt_2 = 0x%x\n", ptp_volt_2);
				ptp_isr_info("ptp_volt_3 = 0x%x\n", ptp_volt_3);
				ptp_isr_info("ptp_volt_4 = 0x%x\n", ptp_volt_4);
				ptp_isr_info("ptp_volt_5 = 0x%x\n", ptp_volt_5);
				ptp_isr_info("ptp_volt_6 = 0x%x\n", ptp_volt_6);
				ptp_isr_info("ptp_volt_7 = 0x%x\n", ptp_volt_7);
				ptp_isr_info("ptp_level = 0x%x\n", ptp_level);
				ptp_isr_info("ISR : TEMPSPARE1 = 0x%x\n", ptp_read(TEMPSPARE1));
			}

			hrtimer_start(&mt_ptp_volt_timer, ktime, HRTIMER_MODE_REL);
		}

		/* Clear PTP INIT interrupt PTPINTSTS = 0x00ff0000 */
		ptp_write(PTP_PTPINTSTS, 0x00ff0000);
	} else {		/* PTP error handler */

		if (((temp_ptpen & 0x7) == 0x1) || ((temp_ptpen & 0x7) == 0x5)) {
			/* init 1 or init 2 error handler */
			ptp_error("init 1 or init 2 error handler\n");
			ptp_error("PTP_PTPEN = 0x%x : PTPINTSTS = 0x%x\n",
				     temp_ptpen, PTPINTSTS);
			ptp_error("PTP_SMSTATE0 %x\n", ptp_read(PTP_SMSTATE0));
			ptp_error("PTP_SMSTATE1 %x\n", ptp_read(PTP_SMSTATE1));

			/* disable PTP */
			ptp_write(PTP_PTPEN, 0x0);

			/* Clear PTP interrupt PTPINTSTS */
			ptp_write(PTP_PTPINTSTS, 0x00ffffff);

			/* restore default DVFS table (PMIC) */
			mt_cpufreq_return_default_DVS_by_ptpod();
		} else {	/* PTP Monitor mode error handler */
			ptp_error("PTP Monitor mode error handler\n");
			ptp_error("PTP_PTPEN = 0x%x : PTPINTSTS = 0x%x\n",
				     temp_ptpen, PTPINTSTS);
			ptp_error("PTP_SMSTATE0 0x%x\n",
				     ptp_read(PTP_SMSTATE0));
			ptp_error("PTP_SMSTATE1 0x%x\n",
				     ptp_read(PTP_SMSTATE1));
			ptp_error("PTP_TEMP = 0x%x\n", ptp_read(PTP_TEMP));
			ptp_error("PTP_TEMPMSR0 0x%x\n",
				     ptp_read(PTP_TEMPMSR0));
			ptp_error("PTP_TEMPMSR1 0x%x\n",
				     ptp_read(PTP_TEMPMSR1));
			ptp_error("PTP_TEMPMSR2 0x%x\n",
				     ptp_read(PTP_TEMPMSR2));
			ptp_error("PTP_TEMPMONCTL0 0x%x\n",
				     ptp_read(PTP_TEMPMONCTL0));
			ptp_error("PTP_TEMPMSRCTL1 0x%x\n",
				     ptp_read(PTP_TEMPMSRCTL1));

			/* disable PTP */
			ptp_write(PTP_PTPEN, 0x0);

			/* Clear PTP interrupt PTPINTSTS */
			ptp_write(PTP_PTPINTSTS, 0x00ffffff);

			/* set init2 value to DVFS table (PMIC) */
			ptp_volt_0 = ptp_init2_volt_0;
			ptp_volt_1 = ptp_init2_volt_1;
			ptp_volt_2 = ptp_init2_volt_2;
			ptp_volt_3 = ptp_init2_volt_3;
			ptp_volt_4 = ptp_init2_volt_4;
			ptp_volt_5 = ptp_init2_volt_5;
			ptp_volt_6 = ptp_init2_volt_6;
			ptp_volt_7 = ptp_init2_volt_7;
			hrtimer_start(&mt_ptp_volt_timer, ktime, HRTIMER_MODE_REL);
		}
	}

	if (ptp_log_en)
		ptp_isr_info("%s done.\n", __func__);

	return IRQ_HANDLED;
}

unsigned int PTP_INIT_01_API(void)
{
	/* only for CPU stress */

	struct ptp_init_t ptp_init_value;

	unsigned int ptp_counter = 0;

	ptp_debug("PTP_INIT_01_API() start.\n");

	/* disable slow idle */
	ptp_data[0] = 0xffffffff;

	ptp_data[1] = 0xffffffff;
	ptp_data[2] = 0xffffffff;

	/* disable PTP */
	ptp_write(PTP_PTPEN, 0x0);

	ptp_init_value.PTPINITEN = (val_0) & 0x1;
	ptp_init_value.PTPMONEN = (val_0 >> 1) & 0x1;
	ptp_init_value.MDES = (val_0 >> 8) & 0xff;
	ptp_init_value.BDES = (val_0 >> 16) & 0xff;
	ptp_init_value.DCMDET = (val_0 >> 24) & 0xff;

	ptp_init_value.DCCONFIG = (val_1) & 0xffffff;
	ptp_init_value.DCBDET = (val_1 >> 24) & 0xff;

	ptp_init_value.AGECONFIG = (val_2) & 0xffffff;
	ptp_init_value.AGEM = (val_2 >> 24) & 0xff;

	/* ptp_init_value.AGEDELTA = (val_3) & 0xff; */
	ptp_init_value.AGEDELTA = 0x88;
	ptp_init_value.DVTFIXED = (val_3 >> 8) & 0xff;
	ptp_init_value.MTDES = (val_3 >> 16) & 0xff;
	ptp_init_value.VCO = (val_3 >> 24) & 0xff;

	/* Get DVFS frequency table */
	freq_0 = (u8) (mt_cpufreq_max_frequency_by_DVS(0) / 13000);
	/* max freq 1300 x 100% */
	freq_1 = (u8) (mt_cpufreq_max_frequency_by_DVS(1) / 13000);
	freq_2 = (u8) (mt_cpufreq_max_frequency_by_DVS(2) / 13000);
	freq_3 = (u8) (mt_cpufreq_max_frequency_by_DVS(3) / 13000);
	freq_4 = (u8) (mt_cpufreq_max_frequency_by_DVS(4) / 13000);
	freq_5 = (u8) (mt_cpufreq_max_frequency_by_DVS(5) / 13000);
	freq_6 = (u8) (mt_cpufreq_max_frequency_by_DVS(6) / 13000);
	freq_7 = (u8) (mt_cpufreq_max_frequency_by_DVS(7) / 13000);

	ptp_init_value.FREQPCT0 = freq_0;
	ptp_init_value.FREQPCT1 = freq_1;
	ptp_init_value.FREQPCT2 = freq_2;
	ptp_init_value.FREQPCT3 = freq_3;
	ptp_init_value.FREQPCT4 = freq_4;
	ptp_init_value.FREQPCT5 = freq_5;
	ptp_init_value.FREQPCT6 = freq_6;
	ptp_init_value.FREQPCT7 = freq_7;
	/* 100 us, This is the PTP Detector sampling time as represented in
	   cycles of bclk_ck during INIT. 52 MHz */
	ptp_init_value.DETWINDOW = 0xa28;
	ptp_init_value.VMAX = 0x60;	/* 1.3v (700mv + n * 6.25mv) */

#if (defined(IS_VCORE_USE_6333VCORE) || defined(CONFIG_MTK_PMIC_MT6397)) && \
	!defined(MTK_DVFS_DISABLE_LOW_VOLTAGE_SUPPORT)
	ptp_init_value.VMIN = 0x38;	/* 1.05v (700mv + n * 6.25mv) */
#else
	ptp_init_value.VMIN = 0x48;	/* 1.15v (700mv + n * 6.25mv) */
#endif

	ptp_init_value.DTHI = 0x01;	/* positive */
	ptp_init_value.DTLO = 0xfe;	/* negative (2¡¦s compliment) */
	ptp_init_value.VBOOT = 0x48;	/* 1.15v  (700mv + n * 6.25mv) */
	/* This timeout value is in cycles of bclk_ck. */
	ptp_init_value.DETMAX = 0xffff;
	if (ptp_init_value.PTPINITEN == 0x0) {
		ptp_error("PTPINITEN = 0x%x\n", ptp_init_value.PTPINITEN);
		/* enable slow idle */
		ptp_data[0] = 0;

		return 0;
	}

	PTP_Initialization_01(&ptp_init_value);

	while (1) {
		ptp_counter++;

		if (ptp_counter >= 0xffffff) {
			ptp_debug("ptp_counter = 0x%x\n", ptp_counter);
			return 0;
		}

		if (ptp_data[0] == 0)
			break;
	}

	return ((unsigned int)(&ptp_data[1]));
}

static const struct of_device_id ptp_of_match[] = {
	{.compatible = "mediatek,mt8127-ptp_od",},
	{},
};

MODULE_DEVICE_TABLE(of, ptp_of_match);

static int ptp_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int ptp_resume(struct platform_device *pdev)
{
	if (mt_ptp_enable) {
		if ((val_0 & 0x1) == 0x0) {
			ptp_error("PTPINITEN = 0x%x\n", (val_0 & 0x1));
			/* enable slow idle */
			ptp_data[0] = 0;

			return 0;
		}
		PTP_INIT_02();
	}
	return 0;
}

int ptp_opp_num(void)
{
	int num = 0;

	while (1) {
		if (mt_cpufreq_max_frequency_by_DVS(num) == 0)
			break;
		num++;
	}

	return num;
}
EXPORT_SYMBOL(ptp_opp_num);

void ptp_opp_freq(unsigned int *freq)
{
	int i = 0;

	while (1) {
		if (mt_cpufreq_max_frequency_by_DVS(i) == 0)
			break;
		freq[i] = mt_cpufreq_max_frequency_by_DVS(i);
		i++;
	}
}
EXPORT_SYMBOL(ptp_opp_freq);

void ptp_opp_status(unsigned int *temp, unsigned int *volt)
{
	int i = 0;

	*temp = tscpu_get_cpu_temp();

	while (1) {
		if (mt_cpufreq_max_frequency_by_DVS(i) == 0)
			break;
		volt[i] = ptp_trasnfer_to_volt(ptpod_pmic_volt[i]);
		i++;
	}
}
EXPORT_SYMBOL(ptp_opp_status);

void ptp_disable(void)
{
	unsigned long flags;

	/* Mask ARM i bit */
	local_irq_save(flags);

	/* disable PTP */
	ptp_write(PTP_PTPEN, 0x0);

	/* Clear PTP interrupt PTPINTSTS */
	ptp_write(PTP_PTPINTSTS, 0x00ffffff);

	/* restore default DVFS table (PMIC) */
	mt_cpufreq_return_default_DVS_by_ptpod();

	mt_ptp_enable = 0;

	ptp_debug("Disable PTP-OD done.\n");

	/* Un-Mask ARM i bit */
	local_irq_restore(flags);
}

/***************************
* return current PTP stauts
****************************/
int ptp_status(void)
{
	int ret = 0;

	if (ptp_read(PTP_PTPEN) != 0)
		ret = 1;
	else
		ret = 0;

	return ret;
}

static int ptp_offset_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "mt_ptp_offset = %d\n", mt_ptp_offset);

	return 0;
}

static ssize_t ptp_offset_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret, offset;
	char *buf = (char *) __get_free_page(GFP_USER);
	ktime_t ktime = ktime_set(mt_ptp_volt_period_s, mt_ptp_volt_period_ns);

	if (!buf)
		return -ENOMEM;

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (!kstrtoint(buf, 10, &offset)) {
		ret = 0;
		mt_ptp_offset = offset;
		hrtimer_start(&mt_ptp_volt_timer, ktime, HRTIMER_MODE_REL);
	} else {
		ptp_warning("bad argument_1!! argument should be \"0\"\n");
	}

out:
	free_page((unsigned long)buf);

	return (ret < 0) ? ret : count;
}

static int ptp_debug_proc_show(struct seq_file *m, void *v)
{
	if (ptp_read(PTP_PTPEN) != 0)
		seq_printf(m, "PTP enabled (ptp_level = 0x%x)\n", ptp_level);
	else
		seq_printf(m, "PTP disabled (ptp_level = 0x%x)\n", ptp_level);

	return 0;
}

static ssize_t ptp_debug_proc_write(struct file *file,
				    const char __user *buffer, size_t count, loff_t *pos)
{
	int enabled = 0, ret;
	char *buf = (char *) __get_free_page(GFP_USER);

	if (!buf)
		return -ENOMEM;

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (!kstrtoint(buf, 10, &enabled)) {
		ret = 0;

		if (enabled == 0)
			/* Disable PTP and restore default DVFS table (PMIC) */
			ptp_disable();
		else
			ptp_warning("bad argument. argument should be \"0\"\n");
	} else {
		ptp_warning("bad argument!! argument should be \"0\"\n");
	}

out:
	free_page((unsigned long)buf);

	return (ret < 0) ? ret : count;
}

static int ptp_cur_volt_proc_show(struct seq_file *m, void *v)
{
	u32 rdata = 0;

	rdata = mt_cpufreq_cur_vproc(); /* mask till dvfs driver ready */

	if (rdata != 0)
		seq_printf(m, "current voltage: (%d)\n", rdata);
	else
		seq_puts(m, "Not support\n");

	return 0;
}

static int ptp_status_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "PTP_LOG: (%d) - (%d, %d, %d, %d, %d, %d, %d, %d) - ",
		   tscpu_get_cpu_temp(),
		   ptp_trasnfer_to_volt(ptpod_pmic_volt[0]),
		   ptp_trasnfer_to_volt(ptpod_pmic_volt[1]),
		   ptp_trasnfer_to_volt(ptpod_pmic_volt[2]),
		   ptp_trasnfer_to_volt(ptpod_pmic_volt[3]),
		   ptp_trasnfer_to_volt(ptpod_pmic_volt[4]),
		   ptp_trasnfer_to_volt(ptpod_pmic_volt[5]),
		   ptp_trasnfer_to_volt(ptpod_pmic_volt[6]),
		   ptp_trasnfer_to_volt(ptpod_pmic_volt[7]));
	seq_printf(m, "(%d, %d, %d, %d, %d, %d, %d, %d)\n",
		   mt_cpufreq_max_frequency_by_DVS(0),
		   mt_cpufreq_max_frequency_by_DVS(1),
		   mt_cpufreq_max_frequency_by_DVS(2),
		   mt_cpufreq_max_frequency_by_DVS(3),
		   mt_cpufreq_max_frequency_by_DVS(4),
		   mt_cpufreq_max_frequency_by_DVS(5),
		   mt_cpufreq_max_frequency_by_DVS(6),
		   mt_cpufreq_max_frequency_by_DVS(7));

	return 0;
}

/*
 * set PTP log enable by procfs interface
 */
static int ptp_log_en_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", ptp_log_en);

	return 0;
}

static ssize_t ptp_log_en_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int enabled = 0, ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	ktime_t ktime = ktime_set(mt_ptp_log_period_s, mt_ptp_log_period_ns);

	if (!buf)
		return -ENOMEM;

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (!kstrtoint(buf, 10, &enabled)) {
		ret = 0;

		if (enabled == 1) {
			ptp_debug("ptp log enabled.\n");
			mt_ptp_log_thread =
				kthread_run(mt_ptp_log_thread_handler, 0,
					    "ptp logging");

			if (IS_ERR(mt_ptp_log_thread))
				ptp_error("failed create ptp logging thread\n");

			hrtimer_start(&mt_ptp_log_timer, ktime, HRTIMER_MODE_REL);
			ptp_log_en = 1;
		} else if (enabled == 0) {
			kthread_stop(mt_ptp_log_thread);
			hrtimer_cancel(&mt_ptp_log_timer);
			ptp_log_en = 0;
		} else {
			ptp_warning("bad argument!! Should be \"0\" or \"1\"\n");
		}
	} else {
		ptp_warning("bad argument!! Should be \"0\" or \"1\"\n");
	}

out:
	free_page((unsigned long)buf);

	return (ret < 0) ? ret : count;
}

static int ptp_dump_proc_show(struct seq_file *m, void *v)
{
	int i, j;

	seq_printf(m, "M_HW_RES0\t= 0x%08X\n", val_0);
	seq_printf(m, "M_HW_RES1\t= 0x%08X\n", val_1);
	seq_printf(m, "M_HW_RES2\t= 0x%08X\n", val_2);
	seq_printf(m, "M_HW_RES3\t= 0x%08X\n", val_3);

	for (i = PTP_PHASE_INIT01; i < NR_PTP_PHASE; i++) {
		seq_puts(m, "Bank_number = 0\n");

		if (i < PTP_PHASE_MON)
			seq_printf(m, "mode = init%d\n", i + 1);
		else
			seq_puts(m, "mode = mon\n");

		for (j = 0; j < ARRAY_SIZE(reg_dump_addr_off); j++)
			seq_printf(m, "0x%p = 0x%08X\n",
				   PTP_BASEADDR + reg_dump_addr_off[j],
				   reg_dump_data[j][i]);
	}

	return 0;
}

#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
		.write          = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RO(ptp_dump);
PROC_FOPS_RW(ptp_debug);
PROC_FOPS_RW(ptp_log_en);
PROC_FOPS_RO(ptp_status);
PROC_FOPS_RO(ptp_cur_volt);
PROC_FOPS_RW(ptp_offset);

static int create_procfs(void)
{
	int i;
	struct proc_dir_entry *ptp_dir = NULL;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry ptp_entries[] = {
		PROC_ENTRY(ptp_dump),
		PROC_ENTRY(ptp_log_en),
		PROC_ENTRY(ptp_cur_volt),
		PROC_ENTRY(ptp_status),
		PROC_ENTRY(ptp_debug),
		PROC_ENTRY(ptp_offset),
	};

	ptp_dir = proc_mkdir("ptp", NULL);

	if (!ptp_dir) {
		ptp_error("[%s]: mkdir /proc/ptp failed\n", __func__);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(ptp_entries); i++) {
		if (!proc_create(ptp_entries[i].name,
				 S_IRUGO | S_IWUSR | S_IWGRP,
				 ptp_dir, ptp_entries[i].fops)) {
			ptp_error("[%s]: create /proc/ptp/%s failed\n",
				  __func__, ptp_entries[i].name);
			return -3;
		}
	}

	return 0;
}

void __iomem *ptpod_base;

#ifdef CONFIG_MTK_RAM_CONSOLE
static void _mt_ptp_aee_init(void)
{
	aee_rr_rec_ptp_vboot(0x0);
	aee_rr_rec_ptp_cpu_little_volt(0x0);
	aee_rr_rec_ptp_temp(0x0);

	/* 0xFF default value. 1 means writing new voltage, 0 means finish */
	aee_rr_rec_ptp_status(0x0);
}
#endif

#if EN_PTP_OD
static int ptp_probe(struct platform_device *pdev)
{
	int ret;
	u32 rdata = 0, test_mode = 0, irq;
	struct device_node *node;
	struct clk *clk_therm;

#ifdef CONFIG_MTK_RAM_CONSOLE
	_mt_ptp_aee_init();
#endif
	/* enable slow idle */
	ptp_data[0] = 0;

	hrtimer_init(&mt_ptp_log_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mt_ptp_log_timer.function = mt_ptp_log_timer_func;

	hrtimer_init(&mt_ptp_volt_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mt_ptp_volt_timer.function = mt_ptp_volt_timer_func;

	if (pwrap_read((u32) RTC_AL_DOM, &rdata) == 0) {
		rdata = (rdata >> 8) & 0x7;
		ptp_info("rdata = 0x%x\n", rdata);

		/* don't care */
		test_mode = 0;
	}
#if PTP_GET_REAL_VAL
	if (!test_mode) {
		val_0 = get_devinfo_with_index(16);
		val_1 = get_devinfo_with_index(17);
		val_2 = get_devinfo_with_index(18);
		val_3 = get_devinfo_with_index(19);
#ifdef CONFIG_MTK_RAM_CONSOLE
		aee_rr_rec_ptp_60((unsigned int)val_0);
		aee_rr_rec_ptp_64((unsigned int)val_1);
		aee_rr_rec_ptp_68((unsigned int)val_2);
		aee_rr_rec_ptp_6C((unsigned int)val_3);
#endif
	}
#else
	val_0 = 0x14105803;
	val_1 = 0xDC555555;
	val_2 = 0x00000000;
	val_3 = 0x28580600;
#endif
	ptp_notice("M_HW_RES0 = 0x%08X\n", val_0);
	ptp_notice("M_HW_RES1 = 0x%08X\n", val_1);
	ptp_notice("M_HW_RES2 = 0x%08X\n", val_2);
	ptp_notice("M_HW_RES3 = 0x%08X\n", val_3);

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt8127-ptp_od");
	if (node == NULL) {
		ptp_error("cannot find mediatek,mt8127-ptp_od\n");
		return -ENODEV;
	}

	/* Setup IO addresses */
	ptpod_base = of_iomap(node, 0);
	if (!ptpod_base) {
		ptp_error("ptpod_base of_iomap error\n");
		return -ENOMEM;
	}

	/*get ptpod irq num */
	irq = irq_of_parse_and_map(node, 0);
	if (!irq) {
		ptp_error("get irqnr failed = 0x%x\n", irq);
		return 0;
	}

	ptp_info("ptpod_irq_number = 0x%x\n", irq);
	ptp_info("ptpod_base=0x%16p\n", (void *)ptpod_base);

	if ((val_0 & 0x1) == 0x0) {
		ptp_error("PTPINITEN = 0x%x\n", (val_0 & 0x1));
		return 0;
	}

	clk_therm = devm_clk_get(&pdev->dev, "ptp_peri_therm");
	BUG_ON(IS_ERR(clk_therm));
	clk_prepare_enable(clk_therm);
	ptp_info("clk_therm = %ldHz\n", __clk_get_rate(clk_therm));

	/* set PTP IRQ */
	ret = request_irq(irq, mt_ptp_isr, IRQF_TRIGGER_LOW, "ptp", NULL);
	if (ret) {
		ptp_error("PTP IRQ register failed (%d)\n", ret);
		WARN_ON(1);
	}

	ptp_info("Set PTP IRQ OK.\n");

	/* get DVFS frequency table */
	/* max freq 1300 x 100% */
	freq_0 = (unsigned char)(mt_cpufreq_max_frequency_by_DVS(0) / 13000);
	if (freq_0 != 0)
		freq_0 += 1;

	freq_1 = (unsigned char)(mt_cpufreq_max_frequency_by_DVS(1) / 13000);
	if (freq_1 != 0)
		freq_1 += 1;

	freq_2 = (unsigned char)(mt_cpufreq_max_frequency_by_DVS(2) / 13000);
	if (freq_2 != 0)
		freq_2 += 1;

	freq_3 = (unsigned char)(mt_cpufreq_max_frequency_by_DVS(3) / 13000);
	if (freq_3 != 0)
		freq_3 += 1;

	freq_4 = (unsigned char)(mt_cpufreq_max_frequency_by_DVS(4) / 13000);
	if (freq_4 != 0)
		freq_4 += 1;

	freq_5 = (unsigned char)(mt_cpufreq_max_frequency_by_DVS(5) / 13000);
	if (freq_5 != 0)
		freq_5 += 1;

	freq_6 = (unsigned char)(mt_cpufreq_max_frequency_by_DVS(6) / 13000);
	if (freq_6 != 0)
		freq_6 += 1;

	freq_7 = (unsigned char)(mt_cpufreq_max_frequency_by_DVS(7) / 13000);
	if (freq_7 != 0)
		freq_7 += 1;

	ptp_level = PTP_get_ptp_level();

	mt_ptp_volt_thread = kthread_run(mt_ptp_volt_thread_handler, 0, "ptp volt");

	if (IS_ERR(mt_ptp_volt_thread))
		ptp_error("[%s]: failed to create ptp volt thread\n", __func__);

	PTP_INIT_01();

	create_procfs();

	return 0;
}

static struct platform_driver ptp_driver = {
	.remove = NULL,
	.shutdown = NULL,
	.probe = ptp_probe,
	.suspend = ptp_suspend,
	.resume = ptp_resume,
	.driver = {
		   .name = "mt-ptp",
		   .of_match_table = of_match_ptr(ptp_of_match),
		   },
};

static int __init ptp_init(void)
{
	int err = 0;

	err = platform_driver_register(&ptp_driver);

	if (err) {
		ptp_error("PTP driver callback register failed..\n");
		return err;
	}

	return 0;
}

static void __exit ptp_exit(void)
{
	ptp_debug("PTP de-initialization\n");
}
late_initcall(ptp_init);
#endif

MODULE_DESCRIPTION("MediaTek PTPOD Driver v0.2");
MODULE_LICENSE("GPL");
