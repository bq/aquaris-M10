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

#ifndef _MT_IDLE_H
#define _MT_IDLE_H

#define MT_IDLE_EN		1

enum idle_lock_spm_id {
	IDLE_SPM_LOCK_VCORE_DVFS = 0,
};

#if MT_IDLE_EN

extern void idle_lock_spm(enum idle_lock_spm_id id);
extern void idle_unlock_spm(enum idle_lock_spm_id id);

extern void enable_dpidle_by_bit(int id);
extern void disable_dpidle_by_bit(int id);
extern void enable_soidle_by_bit(int id);
extern void disable_soidle_by_bit(int id);

extern void set_slp_spm_deepidle_flags(bool en);

extern void mt_idle_init(void);

#else /* !MT_IDLE_EN */

static inline void idle_lock_spm(enum idle_lock_spm_id id) {}
static inline void idle_unlock_spm(enum idle_lock_spm_id id) {}

static inline void enable_dpidle_by_bit(int id) {}
static inline void disable_dpidle_by_bit(int id) {}
static inline void enable_soidle_by_bit(int id) {}
static inline void disable_soidle_by_bit(int id) {}

static inline void set_slp_spm_deepidle_flags(bool en) {}

static inline void mt_idle_init(void) {}

#endif /* MT_IDLE_EN */

#define CLKMGR_ALIAS		1

enum {
	CG_PERI0	= 0,
	CG_PERI1	= 1,
	CG_INFRA	= 2,
	CG_TOPCK	= 3,
	CG_DISP0	= 4,
	CG_DISP1	= 5,
	CG_IMAGE	= 6,
	CG_MFG		= 7,
	CG_AUDIO	= 8,
	CG_VDEC0	= 9,
	CG_VDEC1	= 10,
	NR_GRPS		= 11,
};

#define CGID(grp, bit)	((grp) * 32 + bit)

enum cg_clk_id {
	MT_CG_PERI_NFI			= CGID(CG_PERI0, 0),
	MT_CG_PERI_THERM		= CGID(CG_PERI0, 1),
	MT_CG_PERI_PWM1			= CGID(CG_PERI0, 2),
	MT_CG_PERI_PWM2			= CGID(CG_PERI0, 3),
	MT_CG_PERI_PWM3			= CGID(CG_PERI0, 4),
	MT_CG_PERI_PWM4			= CGID(CG_PERI0, 5),
	MT_CG_PERI_PWM5			= CGID(CG_PERI0, 6),
	MT_CG_PERI_PWM6			= CGID(CG_PERI0, 7),
	MT_CG_PERI_PWM7			= CGID(CG_PERI0, 8),
	MT_CG_PERI_PWM			= CGID(CG_PERI0, 9),
	MT_CG_PERI_USB0			= CGID(CG_PERI0, 10),
	MT_CG_PERI_USB1			= CGID(CG_PERI0, 11),
	MT_CG_PERI_AP_DMA		= CGID(CG_PERI0, 12),
	MT_CG_PERI_MSDC30_0		= CGID(CG_PERI0, 13),
	MT_CG_PERI_MSDC30_1		= CGID(CG_PERI0, 14),
	MT_CG_PERI_MSDC30_2		= CGID(CG_PERI0, 15),
	MT_CG_PERI_NLI			= CGID(CG_PERI0, 16),
	MT_CG_PERI_UART0		= CGID(CG_PERI0, 17),
	MT_CG_PERI_UART1		= CGID(CG_PERI0, 18),
	MT_CG_PERI_UART2		= CGID(CG_PERI0, 19),
	MT_CG_PERI_UART3		= CGID(CG_PERI0, 20),
	MT_CG_PERI_BTIF			= CGID(CG_PERI0, 21),
	MT_CG_PERI_I2C0			= CGID(CG_PERI0, 22),
	MT_CG_PERI_I2C1			= CGID(CG_PERI0, 23),
	MT_CG_PERI_I2C2			= CGID(CG_PERI0, 24),
	MT_CG_PERI_I2C3			= CGID(CG_PERI0, 25),
	MT_CG_PERI_AUXADC		= CGID(CG_PERI0, 26),
	MT_CG_PERI_SPI0			= CGID(CG_PERI0, 27),
	MT_CG_PERI_ETH			= CGID(CG_PERI0, 28),
	MT_CG_PERI_USB0_MCU		= CGID(CG_PERI0, 29),
	MT_CG_PERI_USB1_MCU		= CGID(CG_PERI0, 30),
	MT_CG_PERI_USB_SLV		= CGID(CG_PERI0, 31),

	MT_CG_PERI_GCPU			= CGID(CG_PERI1, 0),
	MT_CG_PERI_NFI_ECC		= CGID(CG_PERI1, 1),
	MT_CG_PERI_NFIPAD		= CGID(CG_PERI1, 2),

	MT_CG_INFRA_DBGCLK		= CGID(CG_INFRA, 0),
	MT_CG_INFRA_SMI			= CGID(CG_INFRA, 1),
	MT_CG_INFRA_AUDIO		= CGID(CG_INFRA, 5),
	MT_CG_INFRA_EFUSE		= CGID(CG_INFRA, 6),
	MT_CG_INFRA_L2C_SRAM		= CGID(CG_INFRA, 7),
	MT_CG_INFRA_M4U			= CGID(CG_INFRA, 8),
	MT_CG_INFRA_CONNMCU		= CGID(CG_INFRA, 12),
	MT_CG_INFRA_TRNG		= CGID(CG_INFRA, 13),
	MT_CG_INFRA_CPUM		= CGID(CG_INFRA, 15),
	MT_CG_INFRA_KP			= CGID(CG_INFRA, 16),
	MT_CG_INFRA_CEC			= CGID(CG_INFRA, 18),
	MT_CG_INFRA_IRRX		= CGID(CG_INFRA, 19),
	MT_CG_INFRA_PMICSPI_SHARE	= CGID(CG_INFRA, 22),
	MT_CG_INFRA_PMICWRAP		= CGID(CG_INFRA, 23),

	MT_CG_TOPCK_PMICSPI		= CGID(CG_TOPCK, 7),

	MT_CG_DISP0_SMI_COMMON		= CGID(CG_DISP0, 0),
	MT_CG_DISP0_SMI_LARB0		= CGID(CG_DISP0, 1),
	MT_CG_DISP0_MM_CMDQ		= CGID(CG_DISP0, 2),
	MT_CG_DISP0_MUTEX		= CGID(CG_DISP0, 3),
	MT_CG_DISP0_DISP_COLOR		= CGID(CG_DISP0, 4),
	MT_CG_DISP0_DISP_BLS		= CGID(CG_DISP0, 5),
	MT_CG_DISP0_DISP_WDMA		= CGID(CG_DISP0, 6),
	MT_CG_DISP0_DISP_RDMA		= CGID(CG_DISP0, 7),
	MT_CG_DISP0_DISP_OVL		= CGID(CG_DISP0, 8),
	MT_CG_DISP0_MDP_TDSHP		= CGID(CG_DISP0, 9),
	MT_CG_DISP0_MDP_WROT		= CGID(CG_DISP0, 10),
	MT_CG_DISP0_MDP_WDMA		= CGID(CG_DISP0, 11),
	MT_CG_DISP0_MDP_RSZ1		= CGID(CG_DISP0, 12),
	MT_CG_DISP0_MDP_RSZ0		= CGID(CG_DISP0, 13),
	MT_CG_DISP0_MDP_RDMA		= CGID(CG_DISP0, 14),
	MT_CG_DISP0_MDP_BLS_26M		= CGID(CG_DISP0, 15),
	MT_CG_DISP0_CAM_MDP		= CGID(CG_DISP0, 16),
	MT_CG_DISP0_FAKE_ENG		= CGID(CG_DISP0, 17),
	MT_CG_DISP0_MUTEX_32K		= CGID(CG_DISP0, 18),
	MT_CG_DISP0_DISP_RMDA1		= CGID(CG_DISP0, 19),
	MT_CG_DISP0_DISP_UFOE		= CGID(CG_DISP0, 20),

	MT_CG_DISP1_DSI_ENGINE		= CGID(CG_DISP1, 0),
	MT_CG_DISP1_DSI_DIGITAL		= CGID(CG_DISP1, 1),
	MT_CG_DISP1_DPI_DIGITAL_LANE	= CGID(CG_DISP1, 2),
	MT_CG_DISP1_DPI_ENGINE		= CGID(CG_DISP1, 3),
	MT_CG_DISP1_DPI1_DIGITAL_LANE	= CGID(CG_DISP1, 4),
	MT_CG_DISP1_DPI1_ENGINE		= CGID(CG_DISP1, 5),
	MT_CG_DISP1_TVE_OUTPUT_CLOCK	= CGID(CG_DISP1, 6),
	MT_CG_DISP1_TVE_INPUT_CLOCK	= CGID(CG_DISP1, 7),
	MT_CG_DISP1_HDMI_PIXEL_CLOCK	= CGID(CG_DISP1, 8),
	MT_CG_DISP1_HDMI_PLL_CLOCK	= CGID(CG_DISP1, 9),
	MT_CG_DISP1_HDMI_AUDIO_CLOCK	= CGID(CG_DISP1, 10),
	MT_CG_DISP1_HDMI_SPDIF_CLOCK	= CGID(CG_DISP1, 11),
	MT_CG_DISP1_LVDS_PIXEL_CLOCK	= CGID(CG_DISP1, 12),
	MT_CG_DISP1_LVDS_CTS_CLOCK	= CGID(CG_DISP1, 13),

	MT_CG_IMAGE_LARB2_SMI		= CGID(CG_IMAGE, 0),
	MT_CG_IMAGE_CAM_SMI		= CGID(CG_IMAGE, 5),
	MT_CG_IMAGE_CAM_CAM		= CGID(CG_IMAGE, 6),
	MT_CG_IMAGE_SEN_TG		= CGID(CG_IMAGE, 7),
	MT_CG_IMAGE_SEN_CAM		= CGID(CG_IMAGE, 8),
	MT_CG_IMAGE_VENC_JPENC		= CGID(CG_IMAGE, 9),

	MT_CG_MFG_G3D			= CGID(CG_MFG, 0),

	MT_CG_AUDIO_AFE			= CGID(CG_AUDIO, 2),
	MT_CG_AUDIO_I2S			= CGID(CG_AUDIO, 6),
	MT_CG_AUDIO_APLL_TUNER_CK	= CGID(CG_AUDIO, 19),
	MT_CG_AUDIO_HDMI_CK		= CGID(CG_AUDIO, 20),
	MT_CG_AUDIO_SPDF_CK		= CGID(CG_AUDIO, 21),
	MT_CG_AUDIO_SPDF2_CK		= CGID(CG_AUDIO, 22),

	MT_CG_VDEC0_VDEC		= CGID(CG_VDEC0, 0),

	MT_CG_VDEC1_LARB		= CGID(CG_VDEC1, 0),

	NR_CLKS,
};

#ifdef _MT_IDLE_C

/* TODO: remove it */

extern unsigned int g_SPM_MCDI_Abnormal_WakeUp;

#if defined(EN_PTP_OD) && EN_PTP_OD
extern u32 ptp_data[3];
#endif

extern struct kobject *power_kobj;

#endif /* _MT_IDLE_C */

#endif
