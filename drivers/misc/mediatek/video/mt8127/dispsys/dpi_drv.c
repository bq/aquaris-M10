/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifdef BUILD_UBOOT
#define ENABLE_DPI_INTERRUPT        0
#define ENABLE_DPI_REFRESH_RATE_LOG 0

#include <asm/arch/disp_drv_platform.h>
#else

#define ENABLE_DPI_INTERRUPT        1
#define ENABLE_DPI_REFRESH_RATE_LOG 0

#if ENABLE_DPI_REFRESH_RATE_LOG && !ENABLE_DPI_INTERRUPT
#error "ENABLE_DPI_REFRESH_RATE_LOG should be also ENABLE_DPI_INTERRUPT"
#endif

#if defined(CONFIG_MTK_HDMI_SUPPORT) && !ENABLE_DPI_INTERRUPT
/* #error "enable CONFIG_MTK_HDMI_SUPPORT should be also ENABLE_DPI_INTERRUPT" */
#endif

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/hrtimer.h>
#include <asm/io.h>
#include <disp_drv_log.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include "disp_drv_platform.h"

#include "dpi_reg.h"
#include "dsi_reg.h"
#include "dpi_drv.h"
#include "lcd_drv.h"
#include "disp_debug.h"

#if ENABLE_DPI_INTERRUPT
/* #include <linux/interrupt.h> */
/* #include <linux/wait.h> */

#include "mtkfb.h"
#endif
static wait_queue_head_t _vsync_wait_queue_dpi;
static bool dpi_vsync;
static bool wait_dpi_vsync;
static struct hrtimer hrtimer_vsync_dpi;
#include <linux/module.h>
#endif

static PDPI_REGS DPI_REG;
static PLVDS_TX_REGS LVDS_TX_REG;
static PLVDS_ANA_REGS LVDS_ANA_REG;
static PDSI_PHY_REGS DSI_PHY_REG_DPI;
/* static uint32_t const PLL_SOURCE = APMIXEDSYS_BASE + 0x44; */
static bool s_isDpiPowerOn;
static DPI_REGS regBackup;
static void (*dpiIntCallback)(DISP_INTERRUPT_EVENTS);

#define DPI_REG_OFFSET(r)       offsetof(DPI_REGS, r)
#define REG_ADDR(base, offset)  (((uint8_t *)(base)) + (offset))

const uint32_t BACKUP_DPI_REG_OFFSETS[] = {
	DPI_REG_OFFSET(INT_ENABLE),
	DPI_REG_OFFSET(SIZE),
	DPI_REG_OFFSET(OUTPUT_SETTING),

	DPI_REG_OFFSET(TGEN_HWIDTH),
	DPI_REG_OFFSET(TGEN_HPORCH),

	DPI_REG_OFFSET(TGEN_VWIDTH_LODD),
	DPI_REG_OFFSET(TGEN_VPORCH_LODD),

	DPI_REG_OFFSET(TGEN_VWIDTH_LEVEN),
	DPI_REG_OFFSET(TGEN_VPORCH_LEVEN),
	DPI_REG_OFFSET(TGEN_VWIDTH_RODD),

	DPI_REG_OFFSET(TGEN_VPORCH_RODD),
	DPI_REG_OFFSET(TGEN_VWIDTH_REVEN),

	DPI_REG_OFFSET(TGEN_VPORCH_REVEN),
	DPI_REG_OFFSET(ESAV_VTIM_LODD),
	DPI_REG_OFFSET(ESAV_VTIM_LEVEN),

	DPI_REG_OFFSET(ESAV_VTIM_RODD),
	DPI_REG_OFFSET(ESAV_VTIM_REVEN),


	DPI_REG_OFFSET(ESAV_FTIM),
	DPI_REG_OFFSET(BG_HCNTL),

	DPI_REG_OFFSET(BG_VCNTL),
	DPI_REG_OFFSET(BG_COLOR),
	/* DPI_REG_OFFSET(TGEN_POL), */
	DPI_REG_OFFSET(EMBSYNC_SETTING),

	DPI_REG_OFFSET(CNTL),
	DPI_REG_OFFSET(DPI_CLKCON),
};

static void _BackupDPIRegisters(void)
{
	DPI_REGS *reg = &regBackup;
	uint32_t i;

	for (i = 0; i < ARY_SIZE(BACKUP_DPI_REG_OFFSETS); ++i) {
		OUTREG32(REG_ADDR(reg, BACKUP_DPI_REG_OFFSETS[i]),
			 AS_UINT32(REG_ADDR(DDP_REG_BASE_DPI0, BACKUP_DPI_REG_OFFSETS[i])));
	}
}

static void _RestoreDPIRegisters(void)
{
	DPI_REGS *reg = &regBackup;
	uint32_t i;

	for (i = 0; i < ARY_SIZE(BACKUP_DPI_REG_OFFSETS); ++i) {
		OUTREG32(REG_ADDR(DDP_REG_BASE_DPI0, BACKUP_DPI_REG_OFFSETS[i]),
			 AS_UINT32(REG_ADDR(reg, BACKUP_DPI_REG_OFFSETS[i])));
	}
}

static void _ResetBackupedDPIRegisterValues(void)
{
	DPI_REGS *regs = &regBackup;

	memset((void *)regs, 0, sizeof(DPI_REGS));

}


#if ENABLE_DPI_REFRESH_RATE_LOG
static void _DPI_LogRefreshRate(DPI_REG_INTERRUPT status)
{
	static unsigned long prevUs = 0xFFFFFFFF;

	if (status.VSYNC) {
		struct timeval curr;

		do_gettimeofday(&curr);

		if (prevUs < curr.tv_usec) {
			DISP_LOG_PRINT(ANDROID_LOG_INFO, "DPI", "Receive 1 vsync in %lu us\n",
				       curr.tv_usec - prevUs);
		}
		prevUs = curr.tv_usec;
	}
}
#else
#define _DPI_LogRefreshRate(x)  do {} while (0)
#endif

void DPI_DisableIrq(void)
{
#if ENABLE_DPI_INTERRUPT
	DPI_REG_INTERRUPT enInt = DPI_REG->INT_ENABLE;
	/* enInt.FIFO_EMPTY = 0; */
	/* enInt.FIFO_FULL = 0; */
	/* enInt.OUT_EMPTY = 0; */
	/* enInt.CNT_OVERFLOW = 0; */
	/* enInt.LINE_ERR = 0; */
	enInt.VSYNC = 0;
	OUTREG32(&DPI_REG->INT_ENABLE, AS_UINT32(&enInt));
#endif
}

void DPI_EnableIrq(void)
{
#if ENABLE_DPI_INTERRUPT
	DPI_REG_INTERRUPT enInt = DPI_REG->INT_ENABLE;
	/* enInt.FIFO_EMPTY = 1; */
	/* enInt.FIFO_FULL = 0; */
	/* enInt.OUT_EMPTY = 0; */
	/* enInt.CNT_OVERFLOW = 0; */
	/* enInt.LINE_ERR = 0; */
	enInt.VSYNC = 1;
	OUTREG32(&DPI_REG->INT_ENABLE, AS_UINT32(&enInt));
#endif
}

static int dpi_vsync_irq_count;
#if ENABLE_DPI_INTERRUPT
static irqreturn_t _DPI_InterruptHandler(int irq, void *dev_id)
{
	static int counter;
	DPI_REG_INTERRUPT status = DPI_REG->INT_STATUS;
/* if (status.FIFO_EMPTY) ++ counter; */

	if (status.VSYNC) {
		dpi_vsync_irq_count++;
		if (dpi_vsync_irq_count > 120) {
			pr_info("dpi vsync\n");
			dpi_vsync_irq_count = 0;
		}
		if (dpiIntCallback)
			dpiIntCallback(DISP_DPI_VSYNC_INT);
#ifndef BUILD_UBOOT
		if (wait_dpi_vsync) {
			if (-1 != hrtimer_try_to_cancel(&hrtimer_vsync_dpi)) {
				dpi_vsync = true;
/* hrtimer_try_to_cancel(&hrtimer_vsync_dpi); */
				wake_up_interruptible(&_vsync_wait_queue_dpi);
			}
		}
#endif
	}

	if (status.VSYNC && counter) {
		DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI",
			       "[Error] DPI FIFO is empty, received %d times interrupt !!!\n",
			       counter);
		counter = 0;
	}

	_DPI_LogRefreshRate(status);
	OUTREG32(&DPI_REG->INT_STATUS, 0);
	return IRQ_HANDLED;
}
#endif

#define VSYNC_US_TO_NS(x) (x * 1000)
unsigned int vsync_timer_dpi = 0;
void DPI_WaitVSYNC(void)
{
#ifndef BUILD_UBOOT
	wait_dpi_vsync = true;
	hrtimer_start(&hrtimer_vsync_dpi, ktime_set(0, VSYNC_US_TO_NS(vsync_timer_dpi)),
		      HRTIMER_MODE_REL);
	wait_event_interruptible(_vsync_wait_queue_dpi, dpi_vsync);
	dpi_vsync = false;
	wait_dpi_vsync = false;
#endif
}

void DPI_PauseVSYNC(bool enable)
{
}

#ifndef BUILD_UBOOT
enum hrtimer_restart dpi_vsync_hrtimer_func(struct hrtimer *timer)
{
/* long long ret; */
	if (wait_dpi_vsync) {
		dpi_vsync = true;
		wake_up_interruptible(&_vsync_wait_queue_dpi);
/* pr_info("hrtimer Vsync, and wake up\n"); */
	}
/* ret = hrtimer_forward_now(timer, ktime_set(0, VSYNC_US_TO_NS(vsync_timer_dpi))); */
/* pr_info("hrtimer callback\n"); */
	return HRTIMER_NORESTART;
}
#endif
void DPI_InitVSYNC(unsigned int vsync_interval)
{
#ifndef BUILD_UBOOT
	ktime_t ktime;

	vsync_timer_dpi = vsync_interval;
	ktime = ktime_set(0, VSYNC_US_TO_NS(vsync_timer_dpi));
	hrtimer_init(&hrtimer_vsync_dpi, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_vsync_dpi.function = dpi_vsync_hrtimer_func;
/* hrtimer_start(&hrtimer_vsync_dpi, ktime, HRTIMER_MODE_REL); */
#endif
}

#ifndef CONFIG_BOX_FPGA
static void DPI_MIPI_clk_setting(LCM_PARAMS *lcm_params)
{
	unsigned int data_Rate = lcm_params->dpi.PLL_CLOCK * 2 * 7;
	unsigned int txdiv, pcw;
/* unsigned int fmod = 30;//Fmod = 30KHz by default */
	unsigned int delta1 = 5;	/* Delta1 is SSC range, default is 0%~-5% */
	unsigned int pdelta1;

	if (DSI_PHY_REG_DPI == 0)
		DPI_InitRegbase();
	if ((lcm_params->dpi.lvds_tx_en == 0) && (lcm_params->type == LCM_TYPE_DPI)) {
		data_Rate = lcm_params->dpi.PLL_CLOCK * 2 * 8;	/* for DPI TTL */
		OUTREGBIT(MIPITX_DSI0_CON_REG, DSI_PHY_REG_DPI->MIPITX_DSI0_CON,
			  RG_DSI0_CKG_LDOOUT_EN, 1);
	}
	/* OUTREGBIT(MIPITX_DSI_TOP_CON_REG,DSI_PHY_REG_DPI->MIPITX_DSI_TOP_CON,RG_DSI_LNT_IMP_CAL_CODE,8); */
	/* OUTREGBIT(MIPITX_DSI_TOP_CON_REG,DSI_PHY_REG_DPI->MIPITX_DSI_TOP_CON,RG_DSI_LNT_HS_BIAS_EN,1); */

	/* OUTREGBIT(MIPITX_DSI_BG_CON_REG,DSI_PHY_REG_DPI->MIPITX_DSI_BG_CON,RG_DSI_V032_SEL,4); */
	/* OUTREGBIT(MIPITX_DSI_BG_CON_REG,DSI_PHY_REG_DPI->MIPITX_DSI_BG_CON,RG_DSI_V04_SEL,4); */
	/* OUTREGBIT(MIPITX_DSI_BG_CON_REG,DSI_PHY_REG_DPI->MIPITX_DSI_BG_CON,RG_DSI_V072_SEL,4); */
	/* OUTREGBIT(MIPITX_DSI_BG_CON_REG,DSI_PHY_REG_DPI->MIPITX_DSI_BG_CON,RG_DSI_V10_SEL,4); */
	/* OUTREGBIT(MIPITX_DSI_BG_CON_REG,DSI_PHY_REG_DPI->MIPITX_DSI_BG_CON,RG_DSI_V12_SEL,4); */
	/* OUTREGBIT(MIPITX_DSI_BG_CON_REG,DSI_PHY_REG_DPI->MIPITX_DSI_BG_CON,RG_DSI_BG_CKEN,1); */
	/* OUTREGBIT(MIPITX_DSI_BG_CON_REG,DSI_PHY_REG_DPI->MIPITX_DSI_BG_CON,RG_DSI_BG_CORE_EN,1); */
	/* mdelay(10); */

	/* OUTREGBIT(MIPITX_DSI0_CON_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_CON,RG_DSI0_CKG_LDOOUT_EN,1); */
	/* OUTREGBIT(MIPITX_DSI0_CON_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_CON,RG_DSI0_LDOCORE_EN,1); */

	OUTREGBIT(MIPITX_DSI_PLL_PWR_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_PWR,
		  DA_DSI0_MPPLL_SDM_PWR_ON, 1);
	OUTREGBIT(MIPITX_DSI_PLL_PWR_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_PWR,
		  DA_DSI0_MPPLL_SDM_ISO_EN, 1);
	mdelay(10);

	OUTREGBIT(MIPITX_DSI_PLL_PWR_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_PWR,
		  DA_DSI0_MPPLL_SDM_ISO_EN, 0);

	OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
		  RG_DSI0_MPPLL_PREDIV, 0);
	OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
		  RG_DSI0_MPPLL_POSDIV, 0);

	if (0 != data_Rate) {	/* if lcm_params->dpi.PLL_CLOCK=0, use other method */
		if (data_Rate > 1250) {
			pr_info("[dpi_drv.c error]Data Rate exceed limitation\n");
			ASSERT(0);
		} else if (data_Rate >= 500)
			txdiv = 1;
		else if (data_Rate >= 250)
			txdiv = 2;
		else if (data_Rate >= 125)
			txdiv = 4;
		else if (data_Rate > 62)
			txdiv = 8;
		else if (data_Rate >= 50)
			txdiv = 16;
		else {
			pr_info("[dpi_drv.c Error]: dataRate is too low,%d!!!\n", __LINE__);
			ASSERT(0);
		}
		/* PLL txdiv config */
		switch (txdiv) {
		case 1:
			OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
				  RG_DSI0_MPPLL_TXDIV0, 0);
			OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
				  RG_DSI0_MPPLL_TXDIV1, 0);
			break;
		case 2:
			OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
				  RG_DSI0_MPPLL_TXDIV0, 1);
			OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
				  RG_DSI0_MPPLL_TXDIV1, 0);
			break;
		case 4:
			OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
				  RG_DSI0_MPPLL_TXDIV0, 2);
			OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
				  RG_DSI0_MPPLL_TXDIV1, 0);
			break;
		case 8:
			OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
				  RG_DSI0_MPPLL_TXDIV0, 2);
			OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
				  RG_DSI0_MPPLL_TXDIV1, 1);
			break;
		case 16:
			OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
				  RG_DSI0_MPPLL_TXDIV0, 2);
			OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
				  RG_DSI0_MPPLL_TXDIV1, 2);
			break;
		default:
			break;
		}

		/* PLL PCW config */
		/*
		   PCW bit 24~30 = floor(pcw)
		   PCW bit 16~23 = (pcw - floor(pcw))*256
		   PCW bit 8~15 = (pcw*256 - floor(pcw)*256)*256
		   PCW bit 8~15 = (pcw*256*256 - floor(pcw)*256*256)*256
		 */
		/* pcw = data_Rate*4*txdiv/(26*2);//Post DIV =4, so need data_Rate*4 */
		pcw = data_Rate * txdiv / 13;

		OUTREGBIT(MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON2,
			  RG_DSI0_MPPLL_SDM_PCW_H, (pcw & 0x7F));
		OUTREGBIT(MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON2,
			  RG_DSI0_MPPLL_SDM_PCW_16_23,
			  ((256 * (data_Rate * txdiv % 13) / 13) & 0xFF));
		OUTREGBIT(MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON2,
			  RG_DSI0_MPPLL_SDM_PCW_8_15,
			  ((256 * (256 * (data_Rate * txdiv % 13) % 13) / 13) & 0xFF));
		OUTREGBIT(MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON2,
			  RG_DSI0_MPPLL_SDM_PCW_0_7,
			  ((256 * (256 * (256 * (data_Rate * txdiv % 13) % 13) % 13) / 13) & 0xFF));

		/* SSC config */
/* pmod = ROUND(1000*26MHz/fmod/2);fmod default is 30Khz, and this value not be changed */
/* pmod = 433.33; */
		if (1 != lcm_params->dpi.ssc_disable) {
			OUTREGBIT(MIPITX_DSI_PLL_CON1_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON1,
				  RG_DSI0_MPPLL_SDM_SSC_PH_INIT, 1);
			OUTREGBIT(MIPITX_DSI_PLL_CON1_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON1,
				  RG_DSI0_MPPLL_SDM_SSC_PRD, 0x1B1);
			if (0 != lcm_params->dpi.ssc_range)
				delta1 = lcm_params->dpi.ssc_range;

			ASSERT(delta1 <= 8);
			pdelta1 = (delta1 * data_Rate * txdiv * 262144 + 281664) / 563329;
			OUTREGBIT(MIPITX_DSI_PLL_CON3_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON3,
				  RG_DSI0_MPPLL_SDM_SSC_DELTA, pdelta1);
			OUTREGBIT(MIPITX_DSI_PLL_CON3_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON3,
				  RG_DSI0_MPPLL_SDM_SSC_DELTA1, pdelta1);
			/* OUTREGBIT(MIPITX_DSI_PLL_CON1_REG,DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON1,
			   RG_DSI0_MPPLL_SDM_FRA_EN,1); */
			pr_info
			    ("[dpi_drv.c] PLL config:data_rate=%d,txdiv=%d,pcw=%d,delta1=%d,pdelta1=0x%x\n",
			     data_Rate, txdiv, DISP_REG_GET(&DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON2),
			     delta1, pdelta1);
		}
	} else {

		OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
			  RG_DSI0_MPPLL_TXDIV0, lcm_params->dpi.mipi_pll_clk_div1);
		OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
			  RG_DSI0_MPPLL_TXDIV1, lcm_params->dpi.mipi_pll_clk_div2);

		OUTREGBIT(MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON2,
			  RG_DSI0_MPPLL_SDM_PCW_H, ((lcm_params->dpi.mipi_pll_clk_fbk_div) << 2));
		OUTREGBIT(MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON2,
			  RG_DSI0_MPPLL_SDM_PCW_16_23, 0);
		OUTREGBIT(MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON2,
			  RG_DSI0_MPPLL_SDM_PCW_8_15, 0);
		OUTREGBIT(MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON2,
			  RG_DSI0_MPPLL_SDM_PCW_0_7, 0);

		/* OUTREGBIT(MIPITX_DSI_PLL_CON1_REG,DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON1,
		   RG_DSI0_MPPLL_SDM_FRA_EN,0); */
	}
	OUTREGBIT(MIPITX_DSI_PLL_CON1_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON1,
		  RG_DSI0_MPPLL_SDM_FRA_EN, 1);

/*
    OUTREGBIT(MIPITX_DSI0_CLOCK_LANE_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_CLOCK_LANE,RG_DSI0_LNTC_RT_CODE,0x8);
    OUTREGBIT(MIPITX_DSI0_CLOCK_LANE_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_CLOCK_LANE,RG_DSI0_LNTC_PHI_SEL,0x1);
    OUTREGBIT(MIPITX_DSI0_CLOCK_LANE_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_CLOCK_LANE,RG_DSI0_LNTC_LDOOUT_EN,1);
    if(lcm_params->dpi.LANE_NUM > 0)
    {
	OUTREGBIT(MIPITX_DSI0_DATA_LANE0_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_DATA_LANE0,RG_DSI0_LNT0_RT_CODE,0x8);
	OUTREGBIT(MIPITX_DSI0_DATA_LANE0_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_DATA_LANE0,RG_DSI0_LNT0_LDOOUT_EN,1);
    }

    if(lcm_params->dsi.LANE_NUM > 1)
    {
	OUTREGBIT(MIPITX_DSI0_DATA_LANE1_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_DATA_LANE1,RG_DSI0_LNT1_RT_CODE,0x8);
	OUTREGBIT(MIPITX_DSI0_DATA_LANE1_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_DATA_LANE1,RG_DSI0_LNT1_LDOOUT_EN,1);
    }

    if(lcm_params->dsi.LANE_NUM > 2)
    {
	OUTREGBIT(MIPITX_DSI0_DATA_LANE2_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_DATA_LANE2,RG_DSI0_LNT2_RT_CODE,0x8);
	OUTREGBIT(MIPITX_DSI0_DATA_LANE2_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_DATA_LANE2,RG_DSI0_LNT2_LDOOUT_EN,1);
    }

    if(lcm_params->dsi.LANE_NUM > 3)
    {
	OUTREGBIT(MIPITX_DSI0_DATA_LANE3_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_DATA_LANE3,RG_DSI0_LNT3_RT_CODE,0x8);
	OUTREGBIT(MIPITX_DSI0_DATA_LANE3_REG,DSI_PHY_REG_DPI->MIPITX_DSI0_DATA_LANE3,RG_DSI0_LNT3_LDOOUT_EN,1);
    }
*/
	OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
		  RG_DSI0_MPPLL_PLL_EN, 1);
	mdelay(1);
	if ((0 != data_Rate) && (1 != lcm_params->dpi.ssc_disable))
		OUTREGBIT(MIPITX_DSI_PLL_CON1_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON1,
			  RG_DSI0_MPPLL_SDM_SSC_EN, 1);
	else
		OUTREGBIT(MIPITX_DSI_PLL_CON1_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON1,
			  RG_DSI0_MPPLL_SDM_SSC_EN, 0);

	/* default POSDIV by 4 */
	OUTREGBIT(MIPITX_DSI_PLL_TOP_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_TOP, RG_MPPLL_PRESERVE_L,
		  3);
	OUTREGBIT(MIPITX_DSI_PLL_TOP_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_TOP, RG_MPPLL_PRESERVE_H,
		  1);

	OUTREGBIT(MIPITX_DSI_TOP_CON_REG, DSI_PHY_REG_DPI->MIPITX_DSI_TOP_CON,
		  RG_DSI_PAD_TIE_LOW_EN, 0);
}
#endif

void DPI_InitRegbase(void)
{
	DPI_REG = (PDPI_REGS) (DDP_REG_BASE_DPI0);
	LVDS_TX_REG = (PLVDS_TX_REGS) (DDP_REG_BASE_LVDS + 0x200);
	LVDS_ANA_REG = (PLVDS_ANA_REGS) (DDP_REG_BASE_MIPI + 0x400);
	DSI_PHY_REG_DPI = (PDSI_PHY_REGS) (DDP_REG_BASE_MIPI + 0x000);
}

DPI_STATUS DPI_Init(bool isDpiPoweredOn)
{
	if (isDpiPoweredOn)
		_BackupDPIRegisters();
	else
		_ResetBackupedDPIRegisterValues();

	DPI_PowerOn();

#if ENABLE_DPI_INTERRUPT
	if (request_irq(disp_dev.irq[DISP_REG_DPI0],
			_DPI_InterruptHandler, IRQF_TRIGGER_LOW, "mtkdpi", NULL) < 0) {
		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DPI", "[ERROR] fail to request DPI irq\n");
		return DPI_STATUS_ERROR;
	}

	{
		DPI_REG_INTERRUPT enInt = DPI_REG->INT_ENABLE;

		enInt.VSYNC = 1;
		OUTREG32(&DPI_REG->INT_ENABLE, AS_UINT32(&enInt));
	}
#endif
	LCD_W2M_NeedLimiteSpeed(true);
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_Init);

DPI_STATUS DPI_FreeIRQ(void)
{
#if ENABLE_DPI_INTERRUPT
	free_irq(disp_dev.irq[DISP_REG_DPI0], NULL);
#endif
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FreeIRQ);

DPI_STATUS DPI_Deinit(void)
{
	DPI_DisableClk();
	DPI_PowerOff();

	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_Deinit);

void DPI_mipi_switch(bool on)
{
	if (on) {
		/* may call enable_mipi(), but do this in DPI_Init_PLL */
	} else {
		OUTREGBIT(MIPITX_DSI_SW_CTRL_REG, DSI_PHY_REG_DPI->MIPITX_DSI_SW_CTRL, SW_CTRL_EN,
			  1);

		/* disable mipi clock */
		OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
			  RG_DSI0_MPPLL_PLL_EN, 0);
		mdelay(1);
		OUTREGBIT(MIPITX_DSI_PLL_TOP_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_TOP,
			  RG_MPPLL_PRESERVE_L, 0);

		mdelay(1);

		OUTREGBIT(MIPITX_DSI_PLL_PWR_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_PWR,
			  DA_DSI0_MPPLL_SDM_ISO_EN, 1);
		OUTREGBIT(MIPITX_DSI_PLL_PWR_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_PWR,
			  DA_DSI0_MPPLL_SDM_PWR_ON, 0);


		OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
			  RG_DSI0_MPPLL_PREDIV, 0);
		OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
			  RG_DSI0_MPPLL_TXDIV0, 0);
		OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
			  RG_DSI0_MPPLL_TXDIV1, 0);
		OUTREGBIT(MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON0,
			  RG_DSI0_MPPLL_POSDIV, 0);

		OUTREGBIT(MIPITX_DSI0_CON_REG, DSI_PHY_REG_DPI->MIPITX_DSI0_CON,
			  RG_DSI0_CKG_LDOOUT_EN, 0);


		OUTREG32(&DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON1, 0x00000000);
		OUTREG32(&DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON2, 0x50000000);

		OUTREGBIT(MIPITX_DSI_SW_CTRL_REG, DSI_PHY_REG_DPI->MIPITX_DSI_SW_CTRL, SW_CTRL_EN,
			  0);
		mdelay(1);
	}
}

DPI_STATUS DPI_Init_PLL(void)
{
	/* unsigned int reg_value = 0; */


#ifndef CONFIG_BOX_FPGA
	DPI_MIPI_clk_setting(lcm_params);
#endif
	/* MASKREG32(0xf0000080, 0x70000, 0x10000);  // CLK_CFG_5[10] rg_lvds_tv_sel */
	/* MASKREG32(0xf0000080, 0x70000, 0x40000);  // CLK_CFG_5[10] rg_lvds_tv_sel */
	/* 0:dpi0_ck from tvhdmi pll */
	/* 1:dpi0_ck from lvds pll */

/* MASKREG32(0xf0000090, 0x07000000, 0x01000000); // CLK_CFG_7[26:24] lvdspll clock divider selection */
	/* 0: from 26M */
	/* 1: lvds_pll_ck */
	/* 2: lvds_pll_ck/2 */
	/* 3: lvds_pll_ck/4 */
	/* 4: lvds_pll_ck/8 */
	/* CLK_CFG_7[31] 0: clock on, 1: clock off */
/* / set PLL as 75MHz for bringup  start */

/* DRV_SetReg32(0xf020927c , (0x1 << 0)); //PLL_PWR_ON */
/* udelay(2); */
/* DRV_ClrReg32(0xf020927c, (0x1 << 1));   //PLL_ISO_EN */

/* OUTREG32(0xf0209274, 0x800A6000); */
/* OUTREG32(0xf0209270, 0x00000141); */
/* udelay(20); */

	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_Init_PLL);

DPI_STATUS DPI_Set_DrivingCurrent(LCM_PARAMS *lcm_params)
{
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "DPI_Set_DrivingCurrent not implement for 6575");
	return DPI_STATUS_OK;
}

#ifdef BUILD_UBOOT
DPI_STATUS DPI_PowerOn(void)
{
	if (!s_isDpiPowerOn) {
#if 1				/* FIXME */
		MASKREG32(0x14000110, 0x40, 0x0);	/* dpi0 clock gate clear */
#endif
		_RestoreDPIRegisters();
		s_isDpiPowerOn = true;
	}

	return DPI_STATUS_OK;
}


DPI_STATUS DPI_PowerOff(void)
{
	if (s_isDpiPowerOn) {
		bool ret = true;

		_BackupDPIRegisters();
#if 1				/* FIXME */
		MASKREG32(0x14000110, 0x40, 0x40);	/* dpi0 clock gate setting */
#endif
		ASSERT(ret);
		s_isDpiPowerOn = false;
	}

	return DPI_STATUS_OK;
}

#else

DPI_STATUS DPI_PowerOn(void)
{
	if (!s_isDpiPowerOn) {
#ifdef DDP_USE_CLOCK_API
		int ret = 0;

		clk_prepare_enable(disp_dev.clk_map[DISP_REG_DPI0][0]);
		clk_prepare_enable(disp_dev.clk_map[DISP_REG_DPI0][1]);
		clk_prepare_enable(disp_dev.clk_map[DISP_REG_LVDS][0]);
		clk_prepare_enable(disp_dev.clk_map[DISP_REG_LVDS][1]);
		if (ret > 0) {
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI",
				       "power manager API return false\n");
		}
		/* enable_pll(LVDSPLL, "dpi0"); */
#endif
		_RestoreDPIRegisters();
		s_isDpiPowerOn = true;
	}
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_PowerOn);


DPI_STATUS DPI_PowerOff(void)
{
	int ret = DPI_STATUS_OK;

	if (s_isDpiPowerOn) {
		_BackupDPIRegisters();
#ifdef DDP_USE_CLOCK_API
		clk_disable_unprepare(disp_dev.clk_map[DISP_REG_DPI0][0]);
		clk_disable_unprepare(disp_dev.clk_map[DISP_REG_DPI0][1]);
		clk_disable_unprepare(disp_dev.clk_map[DISP_REG_LVDS][0]);
		clk_disable_unprepare(disp_dev.clk_map[DISP_REG_LVDS][1]);
		if (ret > 0) {
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI",
				       "power manager API return false\n");
		}
#endif
		s_isDpiPowerOn = false;
	}

	return ret;
}
#endif
EXPORT_SYMBOL(DPI_PowerOff);

DPI_STATUS DPI_EnableClk(void)
{
	DPI_REG_EN en = DPI_REG->DPI_EN;

	en.EN = 1;
	OUTREG32(&DPI_REG->DPI_EN, AS_UINT32(&en));

	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_EnableClk);

DPI_STATUS DPI_DisableClk(void)
{
	DPI_REG_EN en = DPI_REG->DPI_EN;

	en.EN = 0;
	OUTREG32(&DPI_REG->DPI_EN, AS_UINT32(&en));

	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_DisableClk);

DPI_STATUS DPI_EnableSeqOutput(bool enable)
{
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_EnableSeqOutput);

DPI_STATUS DPI_SetRGBOrder(DPI_RGB_ORDER input, DPI_RGB_ORDER output)
{
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_SetRGBOrder);

DPI_STATUS DPI_ConfigPixelClk(DPI_POLARITY polarity, uint32_t divisor, uint32_t duty)
{
	DPI_REG_OUTPUT_SETTING ctrl = DPI_REG->OUTPUT_SETTING;
	DPI_REG_CLKCNTL clkctrl = DPI_REG->DPI_CLKCON;
/*
    ASSERT(divisor >= 2);
    ASSERT(duty > 0 && duty < divisor);

    ctrl.POLARITY = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    ctrl.DIVISOR = divisor - 1;
    ctrl.DUTY = duty;
*/

	ctrl.CLK_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	if (lcm_params->dpi.lvds_tx_en == 1)
		clkctrl.DPI_CKOUT_DIV = 1;
	else
		clkctrl.DPI_CKOUT_DIV = 0;
	OUTREG32(&DPI_REG->OUTPUT_SETTING, AS_UINT32(&ctrl));
	OUTREG32(&DPI_REG->DPI_CLKCON, AS_UINT32(&clkctrl));

	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigPixelClk);

DPI_STATUS DPI_ConfigLVDS(LCM_PARAMS *lcm_params)
{

	LVDS_VPLL_REG_CTL2 lvds_vpll_ctl2 = LVDS_ANA_REG->LVDS_VPLL_CTL2;
	LVDS_REG_FMTCNTL lvds_fmtctl = LVDS_TX_REG->LVDS_FMTCTRL;

	pr_info("enter DPI_ConfigLVDS!\n");

	if (lcm_params->dpi.lvds_tx_en == 1) {
		pr_info("LVDS Setting....................!\n");
		lvds_vpll_ctl2.LVDS_VPLL_CLK_SEL = 1;	/* From MIPI PLL */
		lvds_vpll_ctl2.LVDS_VPLL_EN = 1;	/* enable LVDS VPLL */

		OUTREG32(&LVDS_ANA_REG->LVDS_VPLL_CTL2, AS_UINT32(&lvds_vpll_ctl2));
		udelay(30);

		/* from DE setting */
		OUTREG32(&LVDS_ANA_REG->LVDSTX_ANA_CTL3, 0x00007fe0);
		udelay(5);
		OUTREG32(&LVDS_ANA_REG->LVDSTX_ANA_CTL2, 0x00c10fb3);
		udelay(5);
		OUTREG32(&LVDS_TX_REG->LVDS_CLK_CTRL, 0x7);
		OUTREG32(&LVDS_TX_REG->LVDS_OUT_CTRL, 0x1);
		udelay(5);
		/* MASKREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x005c, 0x2, 0x2); */
		MASKREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x090c, 0x10000, 0x10000);
		/* MASKREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x0118, 0x40, 0x40); */
		if (lcm_params->dpi.format == LCM_DPI_FORMAT_RGB666)
			lvds_fmtctl.DATA_FMT = 1;
		else
			lvds_fmtctl.DATA_FMT = 0;

		OUTREG32(&LVDS_TX_REG->LVDS_FMTCTRL, AS_UINT32(&lvds_fmtctl));

	}

	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigLVDS);


DPI_STATUS DPI_LVDS_Enable(void)
{

	LVDS_VPLL_REG_CTL2 lvds_vpll_ctl2 = LVDS_ANA_REG->LVDS_VPLL_CTL2;

	if (lcm_params->dpi.lvds_tx_en == 1) {
		pr_info("enter DPI_LVDS_Enable!\n");
		/* from DE setting */
		lvds_vpll_ctl2.LVDS_VPLL_CLK_SEL = 1;	/* From MIPI PLL */
		lvds_vpll_ctl2.LVDS_VPLL_EN = 1;	/* enable LVDS VPLL */

		OUTREG32(&LVDS_ANA_REG->LVDS_VPLL_CTL2, AS_UINT32(&lvds_vpll_ctl2));
		udelay(30);

		OUTREG32(&LVDS_ANA_REG->LVDSTX_ANA_CTL3, 0x00007fe0);
		udelay(5);
		OUTREG32(&LVDS_ANA_REG->LVDSTX_ANA_CTL2, 0x00c10fb3);
		udelay(5);
		OUTREG32(&LVDS_TX_REG->LVDS_CLK_CTRL, 0x7);
		OUTREG32(&LVDS_TX_REG->LVDS_OUT_CTRL, 0x1);
		udelay(5);

		MASKREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x090c, 0x10000, 0x10000);	/* enable LVDS out */
	} else {
		MASKREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x0058, 1, 1);
		MASKREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x0074, 2, 2);
	}
	return DPI_STATUS_OK;
}

DPI_STATUS DPI_LVDS_Disable(void)
{

	LVDS_VPLL_REG_CTL2 lvds_vpll_ctl2 = LVDS_ANA_REG->LVDS_VPLL_CTL2;


	if (lcm_params->dpi.lvds_tx_en == 1) {
		pr_info("enter DPI_LVDS_Disable!\n");
		/* from DE setting */
		OUTREG32(&LVDS_ANA_REG->LVDSTX_ANA_CTL3, 0);
		udelay(5);
		OUTREG32(&LVDS_ANA_REG->LVDSTX_ANA_CTL2, 0);
		udelay(5);
		OUTREG32(&LVDS_TX_REG->LVDS_CLK_CTRL, 0x0);
		OUTREG32(&LVDS_TX_REG->LVDS_OUT_CTRL, 0x0);
		udelay(5);

		lvds_vpll_ctl2.LVDS_VPLL_EN = 0;	/* enable LVDS VPLL */
		OUTREG32(&LVDS_ANA_REG->LVDS_VPLL_CTL2, AS_UINT32(&lvds_vpll_ctl2));
		/* MASKREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x090c, 0x10000, 0x10000);  // enable LVDS out */

	}

	return DPI_STATUS_OK;
}


DPI_STATUS DPI_ConfigHDMI(void)
{

	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigHDMI);

DPI_STATUS DPI_ConfigDataEnable(DPI_POLARITY polarity)
{

	DPI_REG_OUTPUT_SETTING pol = DPI_REG->OUTPUT_SETTING;

	pol.DE_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	OUTREG32(&DPI_REG->OUTPUT_SETTING, AS_UINT32(&pol));
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigDataEnable);

DPI_STATUS DPI_ConfigVsync(DPI_POLARITY polarity, uint32_t pulseWidth, uint32_t backPorch,
			   uint32_t frontPorch)
{
	DPI_REG_TGEN_VWIDTH_LODD vwidth_lodd = DPI_REG->TGEN_VWIDTH_LODD;
	DPI_REG_TGEN_VPORCH_LODD vporch_lodd = DPI_REG->TGEN_VPORCH_LODD;
	DPI_REG_OUTPUT_SETTING pol = DPI_REG->OUTPUT_SETTING;
	DPI_REG_CNTL cntl = DPI_REG->CNTL;

	pol.VSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	vwidth_lodd.VPW_LODD = pulseWidth;
	vporch_lodd.VBP_LODD = backPorch;
	vporch_lodd.VFP_LODD = frontPorch;

	if (lcm_params->dpi.lvds_tx_en == 0)
		cntl.VS_LODD_EN = 1;


	OUTREG32(&DPI_REG->OUTPUT_SETTING, AS_UINT32(&pol));
	OUTREG32(&DPI_REG->TGEN_VWIDTH_LODD, AS_UINT32(&vwidth_lodd));
	OUTREG32(&DPI_REG->TGEN_VPORCH_LODD, AS_UINT32(&vporch_lodd));
	OUTREG32(&DPI_REG->CNTL, AS_UINT32(&cntl));

	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigVsync);


DPI_STATUS DPI_ConfigHsync(DPI_POLARITY polarity, uint32_t pulseWidth, uint32_t backPorch,
			   uint32_t frontPorch)
{
	DPI_REG_TGEN_HPORCH hporch = DPI_REG->TGEN_HPORCH;
	DPI_REG_OUTPUT_SETTING pol = DPI_REG->OUTPUT_SETTING;

	pol.HSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	DPI_REG->TGEN_HWIDTH = pulseWidth;
	hporch.HBP = backPorch;
	hporch.HFP = frontPorch;

	pol.HSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	/* DPI_REG->TGEN_HWIDTH = pulseWidth; */
	OUTREG32(&DPI_REG->TGEN_HWIDTH, pulseWidth);
	hporch.HBP = backPorch;
	hporch.HFP = frontPorch;
	OUTREG32(&DPI_REG->OUTPUT_SETTING, AS_UINT32(&pol));
	OUTREG32(&DPI_REG->TGEN_HPORCH, AS_UINT32(&hporch));

	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigHsync);

DPI_STATUS DPI_FBEnable(DPI_FB_ID id, bool enable)
{
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBEnable);

DPI_STATUS DPI_FBSyncFlipWithLCD(bool enable)
{
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBSyncFlipWithLCD);

DPI_STATUS DPI_SetDSIMode(bool enable)
{
	return DPI_STATUS_OK;
}


bool DPI_IsDSIMode(void)
{
/* return DPI_REG->CNTL.DSI_MODE ? true : false; */
	return false;
}


DPI_STATUS DPI_FBSetFormat(DPI_FB_FORMAT format)
{
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBSetFormat);

DPI_FB_FORMAT DPI_FBGetFormat(void)
{
	return 0;
}
EXPORT_SYMBOL(DPI_FBGetFormat);


DPI_STATUS DPI_FBSetSize(uint32_t width, uint32_t height)
{
	DPI_REG_SIZE size;

	size.WIDTH = width;
	size.HEIGHT = height;

	OUTREG32(&DPI_REG->SIZE, AS_UINT32(&size));

	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBSetSize);

DPI_STATUS DPI_FBSetAddress(DPI_FB_ID id, uint32_t address)
{
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBSetAddress);

DPI_STATUS DPI_FBSetPitch(DPI_FB_ID id, uint32_t pitchInByte)
{
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBSetPitch);

DPI_STATUS DPI_SetFifoThreshold(uint32_t low, uint32_t high)
{
	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_SetFifoThreshold);


DPI_STATUS DPI_DumpRegisters(void)
{
	uint32_t i;

	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "---------- Start dump DPI registers ----------\n");

	for (i = 0; i < sizeof(DPI_REGS); i += 4) {
		DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "DPI+%04x : 0x%08x\n", i,
			       DISP_REG_GET(DDP_REG_BASE_DPI0 + i));
	}

	return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_DumpRegisters);

uint32_t DPI_GetCurrentFB(void)
{
	return 0;
}
EXPORT_SYMBOL(DPI_GetCurrentFB);

DPI_STATUS DPI_Capture_Framebuffer(unsigned int pvbuf, unsigned int bpp)
{
#if 0
	unsigned int i = 0;
	unsigned char *fbv;
	unsigned int fbsize = 0;
	unsigned int dpi_fb_bpp = 0;
	unsigned int w, h;
	bool dpi_needPowerOff = false;

	if (!s_isDpiPowerOn) {
		DPI_PowerOn();
		dpi_needPowerOff = true;
		LCD_WaitForNotBusy();
		LCD_WaitDPIIndication(false);
		LCD_FBReset();
		LCD_StartTransfer(true);
		LCD_WaitDPIIndication(true);
	}

	if (pvbuf == 0 || bpp == 0) {
		DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI",
			       "DPI_Capture_Framebuffer, ERROR, parameters wrong: pvbuf=0x%08x, bpp=%d\n",
			       pvbuf, bpp);
		return DPI_STATUS_OK;
	}

	if (DPI_FBGetFormat() == DPI_FB_FORMAT_RGB565) {
		dpi_fb_bpp = 16;
	} else if (DPI_FBGetFormat() == DPI_FB_FORMAT_RGB888) {
		dpi_fb_bpp = 24;
	} else {
		DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI",
			       "DPI_Capture_Framebuffer, ERROR, dpi_fb_bpp is wrong: %d\n",
			       dpi_fb_bpp);
		return DPI_STATUS_OK;
	}

	w = lcm_params->width;
	h = lcm_params->height;
	fbsize = w * h * dpi_fb_bpp / 8;
	if (dpi_needPowerOff)
		fbv = (unsigned char *)ioremap_cached((unsigned int)DPI_REG->FB[0].ADDR, fbsize);
	else
		fbv =
		    (unsigned char *)ioremap_cached((unsigned int)DPI_REG->FB[DPI_GetCurrentFB()].
						    ADDR, fbsize);

	DISP_LOG_PRINT(ANDROID_LOG_INFO, "DPI", "current fb count is %d\n", DPI_GetCurrentFB());

	if (bpp == 32 && dpi_fb_bpp == 24) {
		if (0 == strncmp(CONFIG_MTK_LCM_PHYSICAL_ROTATION, "180", 3)) {
			unsigned int pix_count = w * h - 1;

			for (i = 0; i < w * h; i++) {
				*(unsigned int *)(pvbuf + (pix_count - i) * 4) =
				    0xff000000 | fbv[i *
						     3] | (fbv[i * 3 + 1] << 8) | (fbv[i * 3 +
										       2] << 16);
			}
		} else {
			for (i = 0; i < w * h; i++) {
				*(unsigned int *)(pvbuf + i * 4) =
				    0xff000000 | fbv[i *
						     3] | (fbv[i * 3 + 1] << 8) | (fbv[i * 3 +
										       2] << 16);
			}
		}
	} else if (bpp == 32 && dpi_fb_bpp == 16) {
		unsigned int t;
		unsigned short *fbvt = (unsigned short *)fbv;

		if (0 == strncmp(CONFIG_MTK_LCM_PHYSICAL_ROTATION, "180", 3)) {
			unsigned int pix_count = w * h - 1;

			for (i = 0; i < w * h; i++) {
				t = fbvt[i];
				*(unsigned int *)(pvbuf + (pix_count - i) * 4) =
				    0xff000000 | ((t & 0x001F) << 3) | ((t & 0x07E0) << 5) |
				    ((t & 0xF800) << 8);
			}
		} else {
			for (i = 0; i < w * h; i++) {
				t = fbvt[i];
				*(unsigned int *)(pvbuf + i * 4) =
				    0xff000000 | ((t & 0x001F) << 3) | ((t & 0x07E0) << 5) |
				    ((t & 0xF800) << 8);
			}
		}
	} else if (bpp == 16 && dpi_fb_bpp == 16) {
		if (0 == strncmp(CONFIG_MTK_LCM_PHYSICAL_ROTATION, "180", 3)) {
			unsigned int pix_count = w * h - 1;
			unsigned short *fbvt = (unsigned short *)fbv;

			for (i = 0; i < w * h; i++)
				*(unsigned short *)(pvbuf + (pix_count - i) * 2) = fbvt[i];

		} else
			memcpy((void *)pvbuf, (void *)fbv, fbsize);
	} else if (bpp == 16 && dpi_fb_bpp == 24) {
		if (0 == strncmp(CONFIG_MTK_LCM_PHYSICAL_ROTATION, "180", 3)) {
			unsigned int pix_count = w * h - 1;

			for (i = 0; i < w * h; i++) {
				*(unsigned short *)(pvbuf + (pix_count - i) * 2) =
				    ((fbv[i * 3 + 0] & 0xF8) >> 3) | ((fbv[i * 3 + 1] & 0xFC) << 3)
				    | ((fbv[i * 3 + 2] & 0xF8) << 8);
			}
		} else {
			for (i = 0; i < w * h; i++) {
				*(unsigned short *)(pvbuf + i * 2) =
				    ((fbv[i * 3 + 0] & 0xF8) >> 3) | ((fbv[i * 3 + 1] & 0xFC) << 3)
				    | ((fbv[i * 3 + 2] & 0xF8) << 8);
			}
		}
	} else {
		DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI",
			       "DPI_Capture_Framebuffer, bpp:%d & dpi_fb_bpp:%d is not supported now\n",
			       bpp, dpi_fb_bpp);
	}

	iounmap(fbv);

	if (dpi_needPowerOff)
		DPI_PowerOff();

#else
	unsigned int mva;
#ifdef CONFIG_MTK_M4U
	unsigned int ret = 0;
	M4U_PORT_STRUCT portStruct;
#endif


	struct disp_path_config_mem_out_struct mem_out = { 0 };

	pr_info("enter DPI_Capture_FB!\n");

	if (bpp == 32)
		mem_out.outFormat = eARGB8888;
	else if (bpp == 16)
		mem_out.outFormat = eRGB565;
	else if (bpp == 24)
		mem_out.outFormat = eRGB888;
	else
		pr_info("DPI_Capture_FB, fb color format not support\n");

	pr_info("before alloc MVA: va = 0x%x, size = %d\n", pvbuf,
		lcm_params->height * lcm_params->width * bpp / 8);

#ifdef CONFIG_MTK_M4U
	ret =
	    m4u_alloc_mva(DISP_WDMA, pvbuf, lcm_params->height * lcm_params->width * bpp / 8, 0, 0,
			  &mva);
	if (ret != 0) {
		pr_info("m4u_alloc_mva() fail!\n");
		return DPI_STATUS_OK;
	}
	pr_info("addr=0x%x, format=%d\n", mva, mem_out.outFormat);

	m4u_dma_cache_maint(DISP_WDMA,
			    (void *)pvbuf,
			    lcm_params->height * lcm_params->width * bpp / 8, DMA_BIDIRECTIONAL);

	portStruct.ePortID = DISP_WDMA;	/* hardware port ID, defined in M4U_PORT_ID_ENUM */
	portStruct.Virtuality = 1;
	portStruct.Security = 0;
	portStruct.domain = 0;	/* domain : 0 1 2 3 */
	portStruct.Distance = 1;
	portStruct.Direction = 0;
	m4u_config_port(&portStruct);
#else
	mva = __pa(pvbuf);
#endif

	mem_out.enable = 1;
	mem_out.dstAddr = mva;
	mem_out.srcROI.x = 0;
	mem_out.srcROI.y = 0;
	mem_out.srcROI.height = lcm_params->height;
	mem_out.srcROI.width = lcm_params->width;

	disp_path_get_mutex();
	disp_path_config_mem_out(&mem_out);
	pr_info("Wait DPI idle\n");

	disp_path_release_mutex();

	msleep(20);

	disp_path_get_mutex();
	mem_out.enable = 0;
	disp_path_config_mem_out(&mem_out);

	disp_path_release_mutex();

#ifdef CONFIG_MTK_M4U
	portStruct.ePortID = DISP_WDMA;	/* hardware port ID, defined in M4U_PORT_ID_ENUM */
	portStruct.Virtuality = 1;
	portStruct.Security = 0;
	portStruct.domain = 0;	/* domain : 0 1 2 3 */
	portStruct.Distance = 1;
	portStruct.Direction = 0;
	m4u_config_port(&portStruct);

	m4u_dealloc_mva(DISP_WDMA, pvbuf, lcm_params->height * lcm_params->width * bpp / 8, mva);
#endif

#endif

	return DPI_STATUS_OK;
}

static void _DPI_RDMA0_IRQ_Handler(unsigned int param)
{
	if (param & 4) {
		MMProfileLog(MTKFB_MMP_Events.ScreenUpdate, MMProfileFlagEnd);
		dpiIntCallback(DISP_DPI_SCREEN_UPDATE_END_INT);
	}
	if (param & 8)
		MMProfileLog(MTKFB_MMP_Events.ScreenUpdate, MMProfileFlagEnd);

	if (param & 2) {
		MMProfileLog(MTKFB_MMP_Events.ScreenUpdate, MMProfileFlagStart);
		dpiIntCallback(DISP_DPI_SCREEN_UPDATE_START_INT);

#if (ENABLE_DPI_INTERRUPT == 0)
		if (dpiIntCallback)
			dpiIntCallback(DISP_DPI_VSYNC_INT);
#endif
	}
	if (param & 0x20)
		dpiIntCallback(DISP_DPI_TARGET_LINE_INT);

}

static void _DPI_MUTEX_IRQ_Handler(unsigned int param)
{
	if (dpiIntCallback) {
#ifndef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
		if (param & 1)
#endif
			dpiIntCallback(DISP_DPI_REG_UPDATE_INT);

	}
}

DPI_STATUS DPI_EnableInterrupt(DISP_INTERRUPT_EVENTS eventID)
{
#if ENABLE_DPI_INTERRUPT
	switch (eventID) {
	case DISP_DPI_VSYNC_INT:
		/* DPI_REG->INT_ENABLE.VSYNC = 1; */
		OUTREGBIT(DPI_REG_INTERRUPT, DPI_REG->INT_ENABLE, VSYNC, 1);
		break;
	case DISP_DPI_TARGET_LINE_INT:
		disp_register_irq(DISP_MODULE_RDMA0, _DPI_RDMA0_IRQ_Handler);
		break;
	case DISP_DPI_REG_UPDATE_INT:
		disp_register_irq(DISP_MODULE_MUTEX, _DPI_MUTEX_IRQ_Handler);
		break;
	case DISP_DPI_SCREEN_UPDATE_START_INT:
		disp_register_irq(DISP_MODULE_RDMA0, _DPI_RDMA0_IRQ_Handler);
		break;
	case DISP_DPI_SCREEN_UPDATE_END_INT:
		disp_register_irq(DISP_MODULE_RDMA0, _DPI_RDMA0_IRQ_Handler);
		break;
	default:
		/* return DPI_STATUS_ERROR; */
		return DPI_STATUS_OK;
	}

	return DPI_STATUS_OK;
#else
	switch (eventID) {
	case DISP_DPI_VSYNC_INT:
		OUTREGBIT(DPI_REG_INTERRUPT, DPI_REG->INT_ENABLE, VSYNC, 1);
		disp_register_irq(DISP_MODULE_RDMA0, _DPI_RDMA0_IRQ_Handler);
		break;
	case DISP_DPI_TARGET_LINE_INT:
		disp_register_irq(DISP_MODULE_RDMA0, _DPI_RDMA0_IRQ_Handler);
		break;
	case DISP_DPI_REG_UPDATE_INT:
		disp_register_irq(DISP_MODULE_MUTEX, _DPI_MUTEX_IRQ_Handler);
		break;
	default:
		return DPI_STATUS_ERROR;
	}

	return DPI_STATUS_OK;
	/* /TODO: warning log here */
	/* return DPI_STATUS_ERROR; */
#endif
}


DPI_STATUS DPI_SetInterruptCallback(void (*pCB) (DISP_INTERRUPT_EVENTS))
{
	dpiIntCallback = pCB;

	return DPI_STATUS_OK;
}


DPI_STATUS DPI_FMDesense_Query(void)
{
	return DPI_STATUS_ERROR;
}

DPI_STATUS DPI_FM_Desense(unsigned long freq)
{
	return DPI_STATUS_OK;
}

DPI_STATUS DPI_Reset_CLK(void)
{
	return DPI_STATUS_OK;
}

DPI_STATUS DPI_Get_Default_CLK(unsigned int *clk)
{
	return DPI_STATUS_OK;
}

DPI_STATUS DPI_Get_Current_CLK(unsigned int *clk)
{
	return DPI_STATUS_OK;
}

DPI_STATUS DPI_Change_CLK(unsigned int clk)
{
	return DPI_STATUS_OK;
}

unsigned int DPI_Check_LCM(void)
{
	unsigned int ret = 0;

	if (lcm_drv->ata_check)
		ret = lcm_drv->ata_check(NULL);
	return ret;
}

DPI_STATUS DPI0_ConfigColorFormat(enum DPI0_INPUT_COLOR_FORMAT input_fmt,
				  enum DPI0_OUTPUT_COLOR_FORMAT output_fmt)
{
#if 1
	DPI_REG_CNTL cntl = DPI_REG->CNTL;

	DPI_REG_OUTPUT_SETTING output_setting = DPI_REG->OUTPUT_SETTING;


	if (input_fmt == DPI0_INPUT_RGB && DPI0_OUTPUT_YUV == output_fmt)
		cntl.RGB2YUV_EN = 1;
	else
		cntl.RGB2YUV_EN = 0;


	if (input_fmt == DPI0_INPUT_YUV && DPI0_OUTPUT_YUV == output_fmt)
		output_setting.CH_SWAP = DPI0_OUTPUT_CHANNEL_SWAP_BRG;	/* 2 */
	else if (input_fmt == DPI0_INPUT_RGB && DPI0_OUTPUT_YUV == output_fmt)
		output_setting.CH_SWAP = DPI0_OUTPUT_CHANNEL_SWAP_BGR;	/* 5 */
	else
		output_setting.CH_SWAP = 0;



	OUTREG32(&DPI_REG->OUTPUT_SETTING, AS_UINT32(&output_setting));



	OUTREG32(&DPI_REG->CNTL, AS_UINT32(&cntl));
#endif

	return DPI_STATUS_OK;
}
