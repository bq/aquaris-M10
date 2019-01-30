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

#ifndef _MT_PTP_
#define _MT_PTP_

#include <linux/kernel.h>
#include <mt-plat/sync_write.h>

extern void __iomem *ptpod_base;

#define PTP_BASEADDR        ptpod_base
/* PTP Register Definition */
#define PTP_DESCHAR         (PTP_BASEADDR + 0x200)
#define PTP_TEMPCHAR        (PTP_BASEADDR + 0x204)
#define PTP_DETCHAR         (PTP_BASEADDR + 0x208)
#define PTP_AGECHAR         (PTP_BASEADDR + 0x20C)
#define PTP_DCCONFIG        (PTP_BASEADDR + 0x210)
#define PTP_AGECONFIG       (PTP_BASEADDR + 0x214)
#define PTP_FREQPCT30       (PTP_BASEADDR + 0x218)
#define PTP_FREQPCT74       (PTP_BASEADDR + 0x21C)
#define PTP_LIMITVALS       (PTP_BASEADDR + 0x220)
#define PTP_VBOOT           (PTP_BASEADDR + 0x224)
#define PTP_DETWINDOW       (PTP_BASEADDR + 0x228)
#define PTP_PTPCONFIG       (PTP_BASEADDR + 0x22C)
#define PTP_TSCALCS         (PTP_BASEADDR + 0x230)
#define PTP_RUNCONFIG       (PTP_BASEADDR + 0x234)
#define PTP_PTPEN           (PTP_BASEADDR + 0x238)
#define PTP_INIT2VALS       (PTP_BASEADDR + 0x23C)
#define PTP_DCVALUES        (PTP_BASEADDR + 0x240)
#define PTP_AGEVALUES       (PTP_BASEADDR + 0x244)
#define PTP_VOP30           (PTP_BASEADDR + 0x248)
#define PTP_VOP74           (PTP_BASEADDR + 0x24C)
#define PTP_TEMP            (PTP_BASEADDR + 0x250)
#define PTP_PTPINTSTS       (PTP_BASEADDR + 0x254)
#define PTP_PTPINTSTSRAW    (PTP_BASEADDR + 0x258)
#define PTP_PTPINTEN        (PTP_BASEADDR + 0x25C)
#define PTP_PTPCHKSHIFT     (PTP_BASEADDR + 0x264)
#define PTP_VDESIGN30       (PTP_BASEADDR + 0x26C)
#define PTP_VDESIGN74       (PTP_BASEADDR + 0x270)
#define PTP_AGECOUNT        (PTP_BASEADDR + 0x27C)
#define PTP_SMSTATE0        (PTP_BASEADDR + 0x280)
#define PTP_SMSTATE1        (PTP_BASEADDR + 0x284)

#define THERMAL_BASE            ptpod_base
/* Thermal Register Definition */
#define PTP_TEMPMONCTL0         (THERMAL_BASE + 0x000)
#define PTP_TEMPMONCTL1         (THERMAL_BASE + 0x004)
#define PTP_TEMPMONCTL2         (THERMAL_BASE + 0x008)
#define PTP_TEMPMONINT          (THERMAL_BASE + 0x00C)
#define PTP_TEMPMONINTSTS       (THERMAL_BASE + 0x010)
#define PTP_TEMPMONIDET0        (THERMAL_BASE + 0x014)
#define PTP_TEMPMONIDET1        (THERMAL_BASE + 0x018)
#define PTP_TEMPMONIDET2        (THERMAL_BASE + 0x01C)
#define PTP_TEMPH2NTHRE         (THERMAL_BASE + 0x024)
#define PTP_TEMPHTHRE           (THERMAL_BASE + 0x028)
#define PTP_TEMPCTHRE           (THERMAL_BASE + 0x02C)
#define PTP_TEMPOFFSETH         (THERMAL_BASE + 0x030)
#define PTP_TEMPOFFSETL         (THERMAL_BASE + 0x034)
#define PTP_TEMPMSRCTL0         (THERMAL_BASE + 0x038)
#define PTP_TEMPMSRCTL1         (THERMAL_BASE + 0x03C)
#define PTP_TEMPAHBPOLL         (THERMAL_BASE + 0x040)
#define PTP_TEMPAHBTO           (THERMAL_BASE + 0x044)
#define PTP_TEMPADCPNP0         (THERMAL_BASE + 0x048)
#define PTP_TEMPADCPNP1         (THERMAL_BASE + 0x04C)
#define PTP_TEMPADCPNP2         (THERMAL_BASE + 0x050)
#define PTP_TEMPADCMUX          (THERMAL_BASE + 0x054)
#define PTP_TEMPADCEXT          (THERMAL_BASE + 0x058)
#define PTP_TEMPADCEXT1         (THERMAL_BASE + 0x05C)
#define PTP_TEMPADCEN           (THERMAL_BASE + 0x060)
#define PTP_TEMPPNPMUXADDR      (THERMAL_BASE + 0x064)
#define PTP_TEMPADCMUXADDR      (THERMAL_BASE + 0x068)
#define PTP_TEMPADCEXTADDR      (THERMAL_BASE + 0x06C)
#define PTP_TEMPADCEXT1ADDR     (THERMAL_BASE + 0x070)
#define PTP_TEMPADCENADDR       (THERMAL_BASE + 0x074)
#define PTP_TEMPADCVALIDADDR    (THERMAL_BASE + 0x078)
#define PTP_TEMPADCVOLTADDR     (THERMAL_BASE + 0x07C)
#define PTP_TEMPRDCTRL          (THERMAL_BASE + 0x080)
#define PTP_TEMPADCVALIDMASK    (THERMAL_BASE + 0x084)
#define PTP_TEMPADCVOLTAGESHIFT (THERMAL_BASE + 0x088)
#define PTP_TEMPADCWRITECTRL    (THERMAL_BASE + 0x08C)
#define PTP_TEMPMSR0            (THERMAL_BASE + 0x090)
#define PTP_TEMPMSR1            (THERMAL_BASE + 0x094)
#define PTP_TEMPMSR2            (THERMAL_BASE + 0x098)
#define PTP_TEMPIMMD0           (THERMAL_BASE + 0x0A0)
#define PTP_TEMPIMMD1           (THERMAL_BASE + 0x0A4)
#define PTP_TEMPIMMD2           (THERMAL_BASE + 0x0A8)
#define PTP_TEMPPROTCTL         (THERMAL_BASE + 0x0C0)
#define PTP_TEMPPROTTA          (THERMAL_BASE + 0x0C4)
#define PTP_TEMPPROTTB          (THERMAL_BASE + 0x0C8)
#define PTP_TEMPPROTTC          (THERMAL_BASE + 0x0CC)
#define PTP_TEMPSPARE0          (THERMAL_BASE + 0x0F0)
#define PTP_TEMPSPARE1          (THERMAL_BASE + 0x0F4)
#define PTP_TEMPSPARE2          (THERMAL_BASE + 0x0F8)
#define PTP_TEMPSPARE3          (THERMAL_BASE + 0x0FC)

/* PTP Macro Definition */
/* EN_PTP_OD needs to be 1 for enabling PTPOD driver and let
 * slow idle use global variable ptp_data[]
 */
#define EN_PTP_OD                   (1)

#define PTP_GET_REAL_VAL            (1)
#define SET_PMIC_VOLT               (1)

#define ptp_read(addr)		__raw_readl(addr)
#define ptp_write(addr, val)	mt_reg_sync_writel(val, addr)

extern unsigned int get_devinfo_with_index(unsigned int index);

/* PTP Structure */
struct ptp_init_t {
	unsigned int ADC_CALI_EN;
	unsigned int PTPINITEN;
	unsigned int PTPMONEN;

	unsigned int MDES;
	unsigned int BDES;
	unsigned int DCCONFIG;
	unsigned int DCMDET;
	unsigned int DCBDET;
	unsigned int AGECONFIG;
	unsigned int AGEM;
	unsigned int AGEDELTA;
	unsigned int DVTFIXED;
	unsigned int VCO;
	unsigned int MTDES;
	unsigned int MTS;
	unsigned int BTS;

	unsigned char FREQPCT0;
	unsigned char FREQPCT1;
	unsigned char FREQPCT2;
	unsigned char FREQPCT3;
	unsigned char FREQPCT4;
	unsigned char FREQPCT5;
	unsigned char FREQPCT6;
	unsigned char FREQPCT7;

	unsigned int DETWINDOW;
	unsigned int VMAX;
	unsigned int VMIN;
	unsigned int DTHI;
	unsigned int DTLO;
	unsigned int VBOOT;
	unsigned int DETMAX;

	unsigned int PTPCHKSHIFT;

	unsigned int DCVOFFSETIN;
	unsigned int AGEVOFFSETIN;
};

/* PTP Extern Function */
extern unsigned int PTP_get_ptp_level(void);

#ifdef CONFIG_MTK_RAM_CONSOLE
/* aee set function */
extern void aee_rr_rec_ptp_60(u32 val);
extern void aee_rr_rec_ptp_64(u32 val);
extern void aee_rr_rec_ptp_68(u32 val);
extern void aee_rr_rec_ptp_6C(u32 val);
extern void aee_rr_rec_ptp_78(u32 val);
extern void aee_rr_rec_ptp_7C(u32 val);
extern void aee_rr_rec_ptp_vboot(u64 val);
extern void aee_rr_rec_ptp_gpu_volt(u64 val);
extern void aee_rr_rec_ptp_cpu_little_volt(u64 val);
extern void aee_rr_rec_ptp_temp(u64 val);
extern void aee_rr_rec_ptp_status(u8 val);

/* aee get function */
extern u32 aee_rr_curr_ptp_60(void);
extern u32 aee_rr_curr_ptp_64(void);
extern u32 aee_rr_curr_ptp_68(void);
extern u32 aee_rr_curr_ptp_6C(void);
extern u32 aee_rr_curr_ptp_78(void);
extern u32 aee_rr_curr_ptp_7C(void);
extern u64 aee_rr_curr_ptp_vboot(void);
extern u64 aee_rr_curr_ptp_gpu_volt(void);
extern u64 aee_rr_curr_ptp_cpu_little_volt(void);
extern u64 aee_rr_curr_ptp_temp(void);
extern u8 aee_rr_curr_ptp_status(void);
#endif
#endif
