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

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include <linux/clk-provider.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/module.h>
#include <linux/version.h>

#define DUMMY_REG_TEST		0
#define DUMP_INIT_STATE		0
#define CLKDBG_PM_DOMAIN	0
#define CLKDBG_8163		1

#define TAG	"[clkdbg] "

#define clk_err(fmt, args...)	pr_err(TAG fmt, ##args)
#define clk_warn(fmt, args...)	pr_warn(TAG fmt, ##args)
#define clk_info(fmt, args...)	pr_debug(TAG fmt, ##args)
#define clk_dbg(fmt, args...)	pr_debug(TAG fmt, ##args)
#define clk_ver(fmt, args...)	pr_debug(TAG fmt, ##args)

/************************************************
 **********      register access       **********
 ************************************************/

#ifndef BIT
#define BIT(_bit_)		(u32)(1U << (_bit_))
#endif

#ifndef GENMASK
#define GENMASK(h, l)	(((1U << ((h) - (l) + 1)) - 1) << (l))
#endif

#define ALT_BITS(o, h, l, v) \
	(((o) & ~GENMASK(h, l)) | (((v) << (l)) & GENMASK(h, l)))

#define clk_readl(addr)		readl(addr)
#define clk_writel(addr, val)	\
	do { writel(val, addr); wmb(); } while (0) /* sync write */
#define clk_setl(addr, val)	clk_writel(addr, clk_readl(addr) | (val))
#define clk_clrl(addr, val)	clk_writel(addr, clk_readl(addr) & ~(val))
#define clk_writel_mask(addr, h, l, v)	\
	clk_writel(addr, (clk_readl(addr) & ~GENMASK(h, l)) | ((v) << (l)))

#define ABS_DIFF(a, b)	((a) > (b) ? (a) - (b) : (b) - (a))

/************************************************
 **********      struct definition     **********
 ************************************************/

#if CLKDBG_8163

static void __iomem *topckgen_base;	/* 0x10000000 */
static void __iomem *infrasys_base;	/* 0x10001000 */
static void __iomem *apmixedsys_base;	/* 0x1000c000 */
static void __iomem *audiosys_base;	/* 0x11220000 */
static void __iomem *mfgsys_base;	/* 0x13000000 */
static void __iomem *mmsys_base;	/* 0x14000000 */
static void __iomem *imgsys_base;	/* 0x15000000 */
static void __iomem *vdecsys_base;	/* 0x16000000 */
static void __iomem *vencsys_base;	/* 0x17000000 */
static void __iomem *scpsys_base;	/* 0x10006000 */

#define TOPCKGEN_REG(ofs)	(topckgen_base + ofs)
#define INFRA_REG(ofs)		(infrasys_base + ofs)
#define APMIXED_REG(ofs)	(apmixedsys_base + ofs)
#define AUDIO_REG(ofs)		(audiosys_base + ofs)
#define MFG_REG(ofs)		(mfgsys_base + ofs)
#define MM_REG(ofs)		(mmsys_base + ofs)
#define IMG_REG(ofs)		(imgsys_base + ofs)
#define VDEC_REG(ofs)		(vdecsys_base + ofs)
#define VENC_REG(ofs)		(vencsys_base + ofs)
#define SCP_REG(ofs)		(scpsys_base + ofs)

#define CLK26CALI_0		TOPCKGEN_REG(0x220)
#define CLK26CALI_1		TOPCKGEN_REG(0x224)

#define CLK_CFG_0		TOPCKGEN_REG(0x040)
#define CLK_CFG_1		TOPCKGEN_REG(0x050)
#define CLK_CFG_2		TOPCKGEN_REG(0x060)
#define CLK_CFG_3		TOPCKGEN_REG(0x070)
#define CLK_CFG_4		TOPCKGEN_REG(0x080)
#define CLK_CFG_5		TOPCKGEN_REG(0x090)
#define CLK_CFG_6		TOPCKGEN_REG(0x0A0)
#define CLK_CFG_7		TOPCKGEN_REG(0x0B0)
#define CLK_CFG_8		TOPCKGEN_REG(0x0C0)
#define CLK_MISC_CFG_0		TOPCKGEN_REG(0x104)
#define CLK_DBG_CFG		TOPCKGEN_REG(0x10C)

#define PLL_HP_CON0		APMIXED_REG(0xF00)

/* APMIXEDSYS Register */

#define ARMPLL_CON0		APMIXED_REG(0x210)
#define ARMPLL_CON1		APMIXED_REG(0x214)
#define ARMPLL_PWR_CON0		APMIXED_REG(0x21C)

#define MAINPLL_CON0		APMIXED_REG(0x220)
#define MAINPLL_CON1		APMIXED_REG(0x224)
#define MAINPLL_PWR_CON0	APMIXED_REG(0x22C)

#define UNIVPLL_CON0		APMIXED_REG(0x230)
#define UNIVPLL_CON1		APMIXED_REG(0x234)
#define UNIVPLL_PWR_CON0	APMIXED_REG(0x23C)

#define MMPLL_CON0		APMIXED_REG(0x240)
#define MMPLL_CON1		APMIXED_REG(0x244)
#define MMPLL_PWR_CON0		APMIXED_REG(0x24C)

#define MSDCPLL_CON0		APMIXED_REG(0x250)
#define MSDCPLL_CON1		APMIXED_REG(0x254)
#define MSDCPLL_PWR_CON0	APMIXED_REG(0x25C)

#define VENCPLL_CON0		APMIXED_REG(0x260)
#define VENCPLL_CON1		APMIXED_REG(0x264)
#define VENCPLL_PWR_CON0	APMIXED_REG(0x26C)

#define TVDPLL_CON0		APMIXED_REG(0x270)
#define TVDPLL_CON1		APMIXED_REG(0x274)
#define TVDPLL_PWR_CON0		APMIXED_REG(0x27C)

#define MPLL_CON0		APMIXED_REG(0x280)
#define MPLL_CON1		APMIXED_REG(0x284)
#define MPLL_PWR_CON0		APMIXED_REG(0x28C)

#define AUD1PLL_CON0		APMIXED_REG(0x2A0)
#define AUD1PLL_CON1		APMIXED_REG(0x2A4)
#define AUD1PLL_PWR_CON0	APMIXED_REG(0x2AC)

#define AUD2PLL_CON0		APMIXED_REG(0x2B4)
#define AUD2PLL_CON1		APMIXED_REG(0x2B8)
#define AUD2PLL_PWR_CON0	APMIXED_REG(0x2B4)

#define LVDSPLL_CON0		APMIXED_REG(0x2C0)
#define LVDSPLL_CON1		APMIXED_REG(0x2C4)
#define LVDSPLL_PWR_CON0	APMIXED_REG(0x2CC)

/* INFRASYS Register */

#define INFRA_TOPCKGEN_DCMCTL	INFRA_REG(0x0010)
#define INFRA0_PDN_SET		INFRA_REG(0x0080)
#define INFRA0_PDN_CLR		INFRA_REG(0x0084)
#define INFRA0_PDN_STA		INFRA_REG(0x0090)
#define INFRA1_PDN_SET		INFRA_REG(0x0088)
#define INFRA1_PDN_CLR		INFRA_REG(0x008c)
#define INFRA1_PDN_STA		INFRA_REG(0x0094)

/* Audio Register */

#define AUDIO_TOP_CON0		AUDIO_REG(0x0000)

/* MFGCFG Register */

#define MFG_CG_CON		MFG_REG(0)
#define MFG_CG_SET		MFG_REG(4)
#define MFG_CG_CLR		MFG_REG(8)

/* MMSYS Register */

#define MMSYS_CG_CON0		MM_REG(0x100)
#define MMSYS_CG_SET0		MM_REG(0x104)
#define MMSYS_CG_CLR0		MM_REG(0x108)
#define MMSYS_CG_CON1		MM_REG(0x110)
#define MMSYS_CG_SET1		MM_REG(0x114)
#define MMSYS_CG_CLR1		MM_REG(0x118)

/* IMGSYS Register */

#define IMG_CG_CON		IMG_REG(0x0000)
#define IMG_CG_SET		IMG_REG(0x0004)
#define IMG_CG_CLR		IMG_REG(0x0008)

/* VDEC Register */

#define VDEC_CKEN_SET		VDEC_REG(0x0000)
#define VDEC_CKEN_CLR		VDEC_REG(0x0004)
#define LARB_CKEN_SET		VDEC_REG(0x0008)
#define LARB_CKEN_CLR		VDEC_REG(0x000C)

/* VENC Register */

#define VENC_CG_CON		VENC_REG(0x0)
#define VENC_CG_SET		VENC_REG(0x4)
#define VENC_CG_CLR		VENC_REG(0x8)

#endif /* CLKDBG_SOC */

/* SCPSYS Register */

#define SPM_VDE_PWR_CON		SCP_REG(0x210)
#define SPM_MFG_PWR_CON		SCP_REG(0x214)
#define SPM_VEN_PWR_CON		SCP_REG(0x230)
#define SPM_ISP_PWR_CON		SCP_REG(0x238)
#define SPM_DIS_PWR_CON		SCP_REG(0x23c)
#define SPM_CONN_PWR_CON	SCP_REG(0x280)
#define SPM_VEN2_PWR_CON	SCP_REG(0x298)
#define SPM_AUDIO_PWR_CON	SCP_REG(0x29c)
#define SPM_MFG_2D_PWR_CON	SCP_REG(0x2c0)
#define SPM_MFG_ASYNC_PWR_CON	SCP_REG(0x2c4)
#define SPM_USB_PWR_CON		SCP_REG(0x2cc)
#define SPM_PWR_STATUS		SCP_REG(0x60c)
#define SPM_PWR_STATUS_2ND	SCP_REG(0x610)

static bool is_valid_reg(void __iomem *addr)
{
	return ((u64)addr & 0xf0000000) || (((u64)addr >> 32) & 0xf0000000);
}

enum FMETER_TYPE {
	ABIST,
	CKGEN
};

enum ABIST_CLK {
	ABIST_CLK_NULL,

	AD_ARMCA7PLL_754M_CORE_CK	=  2,
	AD_OSC_CK			=  3,
	AD_MAIN_H546M_CK		=  4,
	AD_MAIN_H364M_CK		=  5,
	AD_MAIN_H218P4M_CK		=  6,
	AD_MAIN_H156M_CK		=  7,
	AD_UNIV_178P3M_CK		=  8,
	AD_UNIV_48M_CK			=  9,
	AD_UNIV_624M_CK			= 10,
	AD_UNIV_416M_CK			= 11,
	AD_UNIV_249P6M_CK		= 12,
	AD_APLL1_180P6336M_CK		= 13,
	AD_APLL2_196P608M_CK		= 14,
	AD_LTEPLL_FS26M_CK		= 15,
	RTC32K_CK_I			= 16,
	AD_MMPLL_700M_CK		= 17,
	AD_VENCPLL_410M_CK		= 18,
	AD_TVDPLL_594M_CK		= 19,
	AD_MPLL_208M_CK			= 20,
	AD_MSDCPLL_806M_CK		= 21,
	AD_USB_48M_CK			= 22,
	MIPI_DSI_TST_CK			= 24,
	AD_PLLGP_TST_CK			= 25,
	AD_LVDSTX_MONCLK		= 26,
	AD_DSI0_LNTC_DSICLK		= 27,
	AD_DSI0_MPPLL_TST_CK		= 28,
	AD_CSI0_DELAY_TSTCLK		= 29,
	AD_CSI1_DELAY_TSTCLK		= 30,
	AD_HDMITX_MON_CK		= 31,
	AD_ARMPLL_1508M_CK		= 32,
	AD_MAINPLL_1092M_CK		= 33,
	AD_MAINPLL_PRO_CK		= 34,
	AD_UNIVPLL_1248M_CK		= 35,
	AD_LVDSPLL_150M_CK		= 36,
	AD_LVDSPLL_ETH_CK		= 37,
	AD_SSUSB_48M_CK			= 38,
	AD_MIPI_26M_CK			= 39,
	AD_MEM_26M_CK			= 40,
	AD_MEMPLL_MONCLK		= 41,
	AD_MEMPLL2_MONCLK		= 42,
	AD_MEMPLL3_MONCLK		= 43,
	AD_MEMPLL4_MONCLK		= 44,
	AD_MEMPLL_REFCLK_BUF		= 45,
	AD_MEMPLL_FBCLK_BUF		= 46,
	AD_MEMPLL2_REFCLK_BUF		= 47,
	AD_MEMPLL2_FBCLK_BUF		= 48,
	AD_MEMPLL3_REFCLK_BUF		= 49,
	AD_MEMPLL3_FBCLK_BUF		= 50,
	AD_MEMPLL4_REFCLK_BUF		= 51,
	AD_MEMPLL4_FBCLK_BUF		= 52,
	AD_USB20_MONCLK			= 53,
	AD_USB20_MONCLK_1P		= 54,
	AD_MONREF_CK			= 55,
	AD_MONFBK_CK			= 56,

	ABIST_CLK_END,
};

static const char * const ABIST_CLK_NAME[] = {
	[AD_ARMCA7PLL_754M_CORE_CK]	= "AD_ARMCA7PLL_754M_CORE_CK",
	[AD_OSC_CK]			= "AD_OSC_CK",
	[AD_MAIN_H546M_CK]		= "AD_MAIN_H546M_CK",
	[AD_MAIN_H364M_CK]		= "AD_MAIN_H364M_CK",
	[AD_MAIN_H218P4M_CK]		= "AD_MAIN_H218P4M_CK",
	[AD_MAIN_H156M_CK]		= "AD_MAIN_H156M_CK",
	[AD_UNIV_178P3M_CK]		= "AD_UNIV_178P3M_CK",
	[AD_UNIV_48M_CK]		= "AD_UNIV_48M_CK",
	[AD_UNIV_624M_CK]		= "AD_UNIV_624M_CK",
	[AD_UNIV_416M_CK]		= "AD_UNIV_416M_CK",
	[AD_UNIV_249P6M_CK]		= "AD_UNIV_249P6M_CK",
	[AD_APLL1_180P6336M_CK]		= "AD_APLL1_180P6336M_CK",
	[AD_APLL2_196P608M_CK]		= "AD_APLL2_196P608M_CK",
	[AD_LTEPLL_FS26M_CK]		= "AD_LTEPLL_FS26M_CK",
	[RTC32K_CK_I]			= "rtc32k_ck_i",
	[AD_MMPLL_700M_CK]		= "AD_MMPLL_700M_CK",
	[AD_VENCPLL_410M_CK]		= "AD_VENCPLL_410M_CK",
	[AD_TVDPLL_594M_CK]		= "AD_TVDPLL_594M_CK",
	[AD_MPLL_208M_CK]		= "AD_MPLL_208M_CK",
	[AD_MSDCPLL_806M_CK]		= "AD_MSDCPLL_806M_CK",
	[AD_USB_48M_CK]			= "AD_USB_48M_CK",
	[MIPI_DSI_TST_CK]		= "MIPI_DSI_TST_CK",
	[AD_PLLGP_TST_CK]		= "AD_PLLGP_TST_CK",
	[AD_LVDSTX_MONCLK]		= "AD_LVDSTX_MONCLK",
	[AD_DSI0_LNTC_DSICLK]		= "AD_DSI0_LNTC_DSICLK",
	[AD_DSI0_MPPLL_TST_CK]		= "AD_DSI0_MPPLL_TST_CK",
	[AD_CSI0_DELAY_TSTCLK]		= "AD_CSI0_DELAY_TSTCLK",
	[AD_CSI1_DELAY_TSTCLK]		= "AD_CSI1_DELAY_TSTCLK",
	[AD_HDMITX_MON_CK]		= "AD_HDMITX_MON_CK",
	[AD_ARMPLL_1508M_CK]		= "AD_ARMPLL_1508M_CK",
	[AD_MAINPLL_1092M_CK]		= "AD_MAINPLL_1092M_CK",
	[AD_MAINPLL_PRO_CK]		= "AD_MAINPLL_PRO_CK",
	[AD_UNIVPLL_1248M_CK]		= "AD_UNIVPLL_1248M_CK",
	[AD_LVDSPLL_150M_CK]		= "AD_LVDSPLL_150M_CK",
	[AD_LVDSPLL_ETH_CK]		= "AD_LVDSPLL_ETH_CK",
	[AD_SSUSB_48M_CK]		= "AD_SSUSB_48M_CK",
	[AD_MIPI_26M_CK]		= "AD_MIPI_26M_CK",
	[AD_MEM_26M_CK]			= "AD_MEM_26M_CK",
	[AD_MEMPLL_MONCLK]		= "AD_MEMPLL_MONCLK",
	[AD_MEMPLL2_MONCLK]		= "AD_MEMPLL2_MONCLK",
	[AD_MEMPLL3_MONCLK]		= "AD_MEMPLL3_MONCLK",
	[AD_MEMPLL4_MONCLK]		= "AD_MEMPLL4_MONCLK",
	[AD_MEMPLL_REFCLK_BUF]		= "AD_MEMPLL_REFCLK_BUF",
	[AD_MEMPLL_FBCLK_BUF]		= "AD_MEMPLL_FBCLK_BUF",
	[AD_MEMPLL2_REFCLK_BUF]		= "AD_MEMPLL2_REFCLK_BUF",
	[AD_MEMPLL2_FBCLK_BUF]		= "AD_MEMPLL2_FBCLK_BUF",
	[AD_MEMPLL3_REFCLK_BUF]		= "AD_MEMPLL3_REFCLK_BUF",
	[AD_MEMPLL3_FBCLK_BUF]		= "AD_MEMPLL3_FBCLK_BUF",
	[AD_MEMPLL4_REFCLK_BUF]		= "AD_MEMPLL4_REFCLK_BUF",
	[AD_MEMPLL4_FBCLK_BUF]		= "AD_MEMPLL4_FBCLK_BUF",
	[AD_USB20_MONCLK]		= "AD_USB20_MONCLK",
	[AD_USB20_MONCLK_1P]		= "AD_USB20_MONCLK_1P",
	[AD_MONREF_CK]			= "AD_MONREF_CK",
	[AD_MONFBK_CK]			= "AD_MONFBK_CK",
};

enum CKGEN_CLK {
	CKGEN_CLK_NULL,

	HD_FAXI_CK		=  1,
	HF_FDDRPHYCFG_CK	=  2,
	F_FPWM_CK		=  3,
	HF_FVDEC_CK		=  4,
	HF_FMM_CK		=  5,
	HF_FCAMTG_CK		=  6,
	F_FUART_CK		=  7,
	HF_FSPI_CK		=  8,
	HF_FMSDC_30_0_CK	=  9,
	HF_FMSDC_30_1_CK	= 10,
	HF_FMSDC_30_2_CK	= 11,
	HF_FMSDC_50_3_CK	= 12,
	HF_FMSDC_50_3_HCLK_CK	= 13,
	HF_FAUDIO_CK		= 14,
	HF_FAUD_INTBUS_CK	= 15,
	HF_FPMICSPI_CK		= 16,
	HF_FSCP_CK		= 17,
	HF_FATB_CK		= 18,
	HF_FSNFI_CK		= 19,
	HF_FDPI0_CK		= 20,
	HF_FAUD_1_CK_PRE	= 21,
	HF_FAUD_2_CK_PRE	= 22,
	HF_FSCAM_CK		= 23,
	HF_FMFG_CK		= 24,
	ECO_AO12_MEM_CLKMUX_CK	= 25,
	ECO_A012_MEM_DCM_CK	= 26,
	HF_FDPI1_CK		= 27,
	HF_FUFOENC_CK		= 28,
	HF_FUFODEC_CK		= 29,
	HF_FETH_CK		= 30,
	HF_FONFI_CK		= 31,

	CKGEN_CLK_END,
};

static const char * const CKGEN_CLK_NAME[] = {
	[HD_FAXI_CK]			= "hd_faxi_ck",
	[HF_FDDRPHYCFG_CK]		= "hf_fddrphycfg_ck",
	[F_FPWM_CK]			= "f_fpwm_ck",
	[HF_FVDEC_CK]			= "hf_fvdec_ck",
	[HF_FMM_CK]			= "hf_fmm_ck",
	[HF_FCAMTG_CK]			= "hf_fcamtg_ck",
	[F_FUART_CK]			= "f_fuart_ck",
	[HF_FSPI_CK]			= "hf_fspi_ck",
	[HF_FMSDC_30_0_CK]		= "hf_fmsdc_30_0_ck",
	[HF_FMSDC_30_1_CK]		= "hf_fmsdc_30_1_ck",
	[HF_FMSDC_30_2_CK]		= "hf_fmsdc_30_2_ck",
	[HF_FMSDC_50_3_CK]		= "hf_fmsdc_50_3_ck",
	[HF_FMSDC_50_3_HCLK_CK]		= "hf_fmsdc_50_3_hclk_ck",
	[HF_FAUDIO_CK]			= "hf_faudio_ck",
	[HF_FAUD_INTBUS_CK]		= "hf_faud_intbus_ck",
	[HF_FPMICSPI_CK]		= "hf_fpmicspi_ck",
	[HF_FSCP_CK]			= "hf_fscp_ck",
	[HF_FATB_CK]			= "hf_fatb_ck",
	[HF_FSNFI_CK]			= "hf_fsnfi_ck",
	[HF_FDPI0_CK]			= "hf_fdpi0_ck",
	[HF_FAUD_1_CK_PRE]		= "hf_faud_1_ck_pre",
	[HF_FAUD_2_CK_PRE]		= "hf_faud_2_ck_pre",
	[HF_FSCAM_CK]			= "hf_fscam_ck",
	[HF_FMFG_CK]			= "hf_fmfg_ck",
	[ECO_AO12_MEM_CLKMUX_CK]	= "eco_ao12_mem_clkmux_ck",
	[ECO_A012_MEM_DCM_CK]		= "eco_a012_mem_dcm_ck",
	[HF_FDPI1_CK]			= "hf_fdpi1_ck",
	[HF_FUFOENC_CK]			= "hf_fufoenc_ck",
	[HF_FUFODEC_CK]			= "hf_fufodec_ck",
	[HF_FETH_CK]			= "hf_feth_ck",
	[HF_FONFI_CK]			= "hf_fonfi_ck",
};

#if CLKDBG_8163

static void set_fmeter_divider_ca7(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_0);

	v = ALT_BITS(v, 23, 16, k1);
	clk_writel(CLK_MISC_CFG_0, v);
}

static void set_fmeter_divider(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_0);

	v = ALT_BITS(v, 31, 24, k1);
	clk_writel(CLK_MISC_CFG_0, v);
}

#endif /* CLKDBG_8163 */

#if CLKDBG_8163

static bool wait_fmeter_done(u32 tri_bit)
{
	static int max_wait_count;
	int wait_count = (max_wait_count > 0) ? (max_wait_count * 2 + 2) : 100;
	int i;

	/* wait fmeter */
	for (i = 0; i < wait_count && (clk_readl(CLK26CALI_0) & tri_bit); i++)
		udelay(20);

	if (!(clk_readl(CLK26CALI_0) & tri_bit)) {
		max_wait_count = max(max_wait_count, i);
		return true;
	}

	return false;
}

#endif /* CLKDBG_8163 */

static u32 fmeter_freq(enum FMETER_TYPE type, int k1, int clk)
{
#if CLKDBG_8163
	u32 cksw_ckgen[] =	{15, 8, clk};
	u32 cksw_abist[] =	{23, 16, clk};
	u32 fqmtr_ck =		(type == CKGEN) ? 1 : 0;
	void __iomem *clk_cfg_reg =
				(type == CKGEN) ? CLK_DBG_CFG	: CLK_DBG_CFG;
	void __iomem *cnt_reg =
				(type == CKGEN) ? CLK26CALI_1	: CLK26CALI_1;
	u32 *cksw_hlv =		(type == CKGEN) ? cksw_ckgen	: cksw_abist;
	u32 tri_bit =		(type == CKGEN) ? BIT(4)	: BIT(4);

	u32 clk_misc_cfg_0;
	u32 clk_cfg_val;
	u32 freq;

	/* setup fmeter */
	clk_writel_mask(CLK_DBG_CFG, 1, 0, fqmtr_ck);	/* fqmtr_ck_sel */
	clk_setl(CLK26CALI_0, BIT(12));			/* enable fmeter_en */

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);	/* backup reg value */
	clk_cfg_val = clk_readl(clk_cfg_reg);

	set_fmeter_divider(k1);				/* set div (0 = /1) */
	set_fmeter_divider_ca7(k1);
	clk_writel_mask(clk_cfg_reg, cksw_hlv[0], cksw_hlv[1], cksw_hlv[2]);

	clk_setl(CLK26CALI_0, tri_bit);			/* start fmeter */

	freq = 0;

	if (wait_fmeter_done(tri_bit)) {
		u32 cnt = clk_readl(cnt_reg) & 0xFFFF;

		/* freq = counter * 26M / 1024 (KHz) */
		freq = (cnt * 26000) * (k1 + 1) / 1024;
	}

	/* restore register settings */
	clk_writel(clk_cfg_reg, clk_cfg_val);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);

	return freq;
#else
	return 0;
#endif /* CLKDBG_8163 */
}

static u32 measure_stable_fmeter_freq(enum FMETER_TYPE type, int k1, int clk)
{
	u32 last_freq = 0;
	u32 freq;
	u32 maxfreq;

	freq = fmeter_freq(type, k1, clk);
	maxfreq = max(freq, last_freq);

	while (maxfreq > 0 && ABS_DIFF(freq, last_freq) * 100 / maxfreq > 10) {
		last_freq = freq;
		freq = fmeter_freq(type, k1, clk);
		maxfreq = max(freq, last_freq);
	}

	return freq;
}

static u32 measure_abist_freq(enum ABIST_CLK clk)
{
	return measure_stable_fmeter_freq(ABIST, 0, clk);
}

static u32 measure_ckgen_freq(enum CKGEN_CLK clk)
{
	return measure_stable_fmeter_freq(CKGEN, 0, clk);
}

#if CLKDBG_8163

static enum ABIST_CLK abist_clk[] = {
	AD_ARMCA7PLL_754M_CORE_CK,
	AD_OSC_CK,
	AD_MAIN_H546M_CK,
	AD_MAIN_H364M_CK,
	AD_MAIN_H218P4M_CK,
	AD_MAIN_H156M_CK,
	AD_UNIV_178P3M_CK,
	AD_UNIV_48M_CK,
	AD_UNIV_624M_CK,
	AD_UNIV_416M_CK,
	AD_UNIV_249P6M_CK,
	AD_APLL1_180P6336M_CK,
	AD_APLL2_196P608M_CK,
	AD_LTEPLL_FS26M_CK,
	RTC32K_CK_I,
	AD_MMPLL_700M_CK,
	AD_VENCPLL_410M_CK,
	AD_TVDPLL_594M_CK,
	AD_MPLL_208M_CK,
	AD_MSDCPLL_806M_CK,
	AD_USB_48M_CK,
	MIPI_DSI_TST_CK,
	AD_PLLGP_TST_CK,
	AD_LVDSTX_MONCLK,
	AD_DSI0_LNTC_DSICLK,
	AD_DSI0_MPPLL_TST_CK,
	AD_CSI0_DELAY_TSTCLK,
	AD_CSI1_DELAY_TSTCLK,
	AD_HDMITX_MON_CK,
	AD_ARMPLL_1508M_CK,
	AD_MAINPLL_1092M_CK,
	AD_MAINPLL_PRO_CK,
	AD_UNIVPLL_1248M_CK,
	AD_LVDSPLL_150M_CK,
	AD_LVDSPLL_ETH_CK,
	AD_SSUSB_48M_CK,
	AD_MIPI_26M_CK,
	AD_MEM_26M_CK,
	AD_MEMPLL_MONCLK,
	AD_MEMPLL2_MONCLK,
	AD_MEMPLL3_MONCLK,
	AD_MEMPLL4_MONCLK,
	AD_MEMPLL_REFCLK_BUF,
	AD_MEMPLL_FBCLK_BUF,
	AD_MEMPLL2_REFCLK_BUF,
	AD_MEMPLL2_FBCLK_BUF,
	AD_MEMPLL3_REFCLK_BUF,
	AD_MEMPLL3_FBCLK_BUF,
	AD_MEMPLL4_REFCLK_BUF,
	AD_MEMPLL4_FBCLK_BUF,
	AD_USB20_MONCLK,
	AD_USB20_MONCLK_1P,
	AD_MONREF_CK,
	AD_MONFBK_CK,
};

static enum CKGEN_CLK ckgen_clk[] = {
	HD_FAXI_CK,
	HF_FDDRPHYCFG_CK,
	F_FPWM_CK,
	HF_FVDEC_CK,
	HF_FMM_CK,
	HF_FCAMTG_CK,
	F_FUART_CK,
	HF_FSPI_CK,
	HF_FMSDC_30_0_CK,
	HF_FMSDC_30_1_CK,
	HF_FMSDC_30_2_CK,
	HF_FMSDC_50_3_CK,
	HF_FMSDC_50_3_HCLK_CK,
	HF_FAUDIO_CK,
	HF_FAUD_INTBUS_CK,
	HF_FPMICSPI_CK,
	HF_FSCP_CK,
	HF_FATB_CK,
	HF_FSNFI_CK,
	HF_FDPI0_CK,
	HF_FAUD_1_CK_PRE,
	HF_FAUD_2_CK_PRE,
	HF_FSCAM_CK,
	HF_FMFG_CK,
	ECO_AO12_MEM_CLKMUX_CK,
	ECO_A012_MEM_DCM_CK,
	HF_FDPI1_CK,
	HF_FUFOENC_CK,
	HF_FUFODEC_CK,
	HF_FETH_CK,
	HF_FONFI_CK,
};

#endif /* CLKDBG_8163 */

#if DUMP_INIT_STATE

static void print_fmeter_all(void)
{
#if CLKDBG_8163
	size_t i;
	u32 old_pll_hp_con0;

	if (!is_valid_reg(apmixedsys_base))
		return;

	old_pll_hp_con0 = clk_readl(PLL_HP_CON0);

	clk_writel(PLL_HP_CON0, 0x0);		/* disable PLL hopping */
	udelay(10);

	for (i = 0; i < ARRAY_SIZE(abist_clk); i++)
		print_abist_clock(abist_clk[i]);

	for (i = 0; i < ARRAY_SIZE(ckgen_clk); i++)
		print_ckgen_clock(ckgen_clk[i]);

	/* restore old setting */
	clk_writel(PLL_HP_CON0, old_pll_hp_con0);
#endif /* CLKDBG_8163 */
}

#endif /* DUMP_INIT_STATE */

static void seq_print_abist_clock(enum ABIST_CLK clk,
		struct seq_file *s, void *v)
{
	u32 freq = measure_abist_freq(clk);

	seq_printf(s, "%2d: %-29s: %u\n", clk, ABIST_CLK_NAME[clk], freq);
}

static void seq_print_ckgen_clock(enum CKGEN_CLK clk,
		struct seq_file *s, void *v)
{
	u32 freq = measure_ckgen_freq(clk);

	seq_printf(s, "%2d: %-29s: %u\n", clk, CKGEN_CLK_NAME[clk], freq);
}

static int seq_print_fmeter_all(struct seq_file *s, void *v)
{
#if CLKDBG_8163

	size_t i;
	u32 old_pll_hp_con0;

	if (!is_valid_reg(apmixedsys_base))
		return 0;

	old_pll_hp_con0 = clk_readl(PLL_HP_CON0);

	clk_writel(PLL_HP_CON0, 0x0);		/* disable PLL hopping */
	udelay(10);

	for (i = 0; i < ARRAY_SIZE(abist_clk); i++)
		seq_print_abist_clock(abist_clk[i], s, v);

	for (i = 0; i < ARRAY_SIZE(ckgen_clk); i++)
		seq_print_ckgen_clock(ckgen_clk[i], s, v);

	/* restore old setting */
	clk_writel(PLL_HP_CON0, old_pll_hp_con0);

#endif /* CLKDBG_8163 */

	return 0;
}

struct regname {
	void __iomem *reg;
	const char *name;
};

static size_t get_regnames(struct regname *regnames, size_t size)
{
	struct regname rn[] = {
		{SPM_VDE_PWR_CON, "SPM_VDE_PWR_CON"},
		{SPM_MFG_PWR_CON, "SPM_MFG_PWR_CON"},
		{SPM_VEN_PWR_CON, "SPM_VEN_PWR_CON"},
		{SPM_ISP_PWR_CON, "SPM_ISP_PWR_CON"},
		{SPM_DIS_PWR_CON, "SPM_DIS_PWR_CON"},
		{SPM_VEN2_PWR_CON, "SPM_VEN2_PWR_CON"},
		{SPM_CONN_PWR_CON, "SPM_CONN_PWR_CON"},
		{SPM_AUDIO_PWR_CON, "SPM_AUDIO_PWR_CON"},
		{SPM_MFG_2D_PWR_CON, "SPM_MFG_2D_PWR_CON"},
		{SPM_MFG_ASYNC_PWR_CON, "SPM_MFG_ASYNC_PWR_CON"},
		{SPM_USB_PWR_CON, "SPM_USB_PWR_CON"},
		{SPM_PWR_STATUS, "SPM_PWR_STATUS"},
		{SPM_PWR_STATUS_2ND, "SPM_PWR_STATUS_2ND"},
#if CLKDBG_8163
		{CLK26CALI_0, "CLK26CALI_0"},
		{CLK26CALI_1, "CLK26CALI_1"},
		{CLK_CFG_0, "CLK_CFG_0"},
		{CLK_CFG_1, "CLK_CFG_1"},
		{CLK_CFG_2, "CLK_CFG_2"},
		{CLK_CFG_3, "CLK_CFG_3"},
		{CLK_CFG_4, "CLK_CFG_4"},
		{CLK_CFG_5, "CLK_CFG_5"},
		{CLK_CFG_6, "CLK_CFG_6"},
		{CLK_CFG_7, "CLK_CFG_7"},
		{CLK_CFG_8, "CLK_CFG_8"},
		{CLK_MISC_CFG_0, "CLK_MISC_CFG_0"},
		{CLK_DBG_CFG, "CLK_DBG_CFG"},
		{PLL_HP_CON0, "PLL_HP_CON0"},
		{ARMPLL_CON0, "ARMPLL_CON0"},
		{ARMPLL_CON1, "ARMPLL_CON1"},
		{ARMPLL_PWR_CON0, "ARMPLL_PWR_CON0"},
		{MAINPLL_CON0, "MAINPLL_CON0"},
		{MAINPLL_CON1, "MAINPLL_CON1"},
		{MAINPLL_PWR_CON0, "MAINPLL_PWR_CON0"},
		{UNIVPLL_CON0, "UNIVPLL_CON0"},
		{UNIVPLL_CON1, "UNIVPLL_CON1"},
		{UNIVPLL_PWR_CON0, "UNIVPLL_PWR_CON0"},
		{MMPLL_CON0, "MMPLL_CON0"},
		{MMPLL_CON1, "MMPLL_CON1"},
		{MMPLL_PWR_CON0, "MMPLL_PWR_CON0"},
		{MSDCPLL_CON0, "MSDCPLL_CON0"},
		{MSDCPLL_CON1, "MSDCPLL_CON1"},
		{MSDCPLL_PWR_CON0, "MSDCPLL_PWR_CON0"},
		{VENCPLL_CON0, "VENCPLL_CON0"},
		{VENCPLL_CON1, "VENCPLL_CON1"},
		{VENCPLL_PWR_CON0, "VENCPLL_PWR_CON0"},
		{TVDPLL_CON0, "TVDPLL_CON0"},
		{TVDPLL_CON1, "TVDPLL_CON1"},
		{TVDPLL_PWR_CON0, "TVDPLL_PWR_CON0"},
		{MPLL_CON0, "MPLL_CON0"},
		{MPLL_CON1, "MPLL_CON1"},
		{MPLL_PWR_CON0, "MPLL_PWR_CON0"},
		{AUD1PLL_CON0, "AUD1PLL_CON0"},
		{AUD1PLL_CON1, "AUD1PLL_CON1"},
		{AUD1PLL_PWR_CON0, "AUD1PLL_PWR_CON0"},
		{AUD2PLL_CON0, "AUD2PLL_CON0"},
		{AUD2PLL_CON1, "AUD2PLL_CON1"},
		{AUD2PLL_PWR_CON0, "AUD2PLL_PWR_CON0"},
		{LVDSPLL_CON0, "LVDSPLL_CON0"},
		{LVDSPLL_CON1, "LVDSPLL_CON1"},
		{LVDSPLL_PWR_CON0, "LVDSPLL_PWR_CON0"},
		{INFRA_TOPCKGEN_DCMCTL, "INFRA_TOPCKGEN_DCMCTL"},
		{INFRA0_PDN_SET, "INFRA0_PDN_SET"},
		{INFRA0_PDN_CLR, "INFRA0_PDN_CLR"},
		{INFRA0_PDN_STA, "INFRA0_PDN_STA"},
		{INFRA1_PDN_SET, "INFRA1_PDN_SET"},
		{INFRA1_PDN_CLR, "INFRA1_PDN_CLR"},
		{INFRA1_PDN_STA, "INFRA1_PDN_STA"},
		{AUDIO_TOP_CON0, "AUDIO_TOP_CON0"},
		{MFG_CG_CON, "MFG_CG_CON"},
		{MFG_CG_SET, "MFG_CG_SET"},
		{MFG_CG_CLR, "MFG_CG_CLR"},
		{MMSYS_CG_CON0, "MMSYS_CG_CON0"},
		{MMSYS_CG_SET0, "MMSYS_CG_SET0"},
		{MMSYS_CG_CLR0, "MMSYS_CG_CLR0"},
		{MMSYS_CG_CON1, "MMSYS_CG_CON1"},
		{MMSYS_CG_SET1, "MMSYS_CG_SET1"},
		{MMSYS_CG_CLR1, "MMSYS_CG_CLR1"},
		{IMG_CG_CON, "IMG_CG_CON"},
		{IMG_CG_SET, "IMG_CG_SET"},
		{IMG_CG_CLR, "IMG_CG_CLR"},
		{VDEC_CKEN_SET, "VDEC_CKEN_SET"},
		{VDEC_CKEN_CLR, "VDEC_CKEN_CLR"},
		{LARB_CKEN_SET, "LARB_CKEN_SET"},
		{LARB_CKEN_CLR, "LARB_CKEN_CLR"},
		{VENC_CG_CON, "VENC_CG_CON"},
		{VENC_CG_SET, "VENC_CG_SET"},
		{VENC_CG_CLR, "VENC_CG_CLR"},

#endif /* CLKDBG_SOC */
	};

	size_t n = min(ARRAY_SIZE(rn), size);

	memcpy(regnames, rn, sizeof(rn[0]) * n);
	return n;
}

#if DUMP_INIT_STATE

static void print_reg(void __iomem *reg, const char *name)
{
	if (!is_valid_reg(reg))
		return;

	clk_info("%-21s: [0x%p] = 0x%08x\n", name, reg, clk_readl(reg));
}

static void print_regs(void)
{
	static struct regname rn[128];
	int i, n;

	n = get_regnames(rn, ARRAY_SIZE(rn));

	for (i = 0; i < n; i++)
		print_reg(rn[i].reg, rn[i].name);
}

#endif /* DUMP_INIT_STATE */

static void seq_print_reg(void __iomem *reg, const char *name,
		struct seq_file *s, void *v)
{
	u32 val;

	if (!is_valid_reg(reg))
		return;

	val = clk_readl(reg);

	clk_info("%-21s: [0x%p] = 0x%08x\n", name, reg, val);
	seq_printf(s, "%-21s: [0x%p] = 0x%08x\n", name, reg, val);
}

static int seq_print_regs(struct seq_file *s, void *v)
{
	static struct regname rn[128];
	int i, n;

	n = get_regnames(rn, ARRAY_SIZE(rn));

	for (i = 0; i < n; i++)
		seq_print_reg(rn[i].reg, rn[i].name, s, v);

	return 0;
}

static const char * const *get_all_clk_names(size_t *num)
{
	static const char * const clks[] = {
#if CLKDBG_8163
		/* ROOT */
		"clk_null",
		"clk26m",
		"clk32k",
		"hdmitx_dig_cts",
		/* APMIXEDSYS */
		"armpll",
		"mainpll",
		"univpll",
		"mmpll",
		"msdcpll",
		"vencpll",
		"tvdpll",
		"mpll",
		"aud1pll",
		"aud2pll",
		"lvdspll",
		/* TOP_DIVS */
		"main_h546m",
		"main_h364m",
		"main_h218p4m",
		"main_h156m",
		"univ_624m",
		"univ_416m",
		"univ_249p6m",
		"univ_178p3m",
		"univ_48m",
		"apll1_ck",
		"apll2_ck",
		"clkrtc_ext",
		"clkrtc_int",
		"dmpll_ck",
		"f26m_mem_ckgen",
		"hdmi_cts_ck",
		"hdmi_cts_d2",
		"hdmi_cts_d3",
		"lvdspll_ck",
		"lvdspll_d2",
		"lvdspll_d4",
		"lvdspll_d8",
		"lvdspll_eth_ck",
		"mmpll_ck",
		"mpll_208m_ck",
		"msdcpll_ck",
		"msdcpll_d2",
		"msdcpll_d4",
		"syspll_d2",
		"syspll_d2p5",
		"syspll_d3",
		"syspll_d5",
		"syspll_d7",
		"syspll1_d2",
		"syspll1_d4",
		"syspll1_d8",
		"syspll1_d16",
		"syspll2_d2",
		"syspll2_d4",
		"syspll2_d8",
		"syspll3_d2",
		"syspll3_d4",
		"syspll4_d2",
		"syspll4_d4",
		"tvdpll_d2",
		"tvdpll_d4",
		"tvdpll_d8",
		"tvdpll_d16",
		"univpll_d2",
		"univpll_d2p5",
		"univpll_d3",
		"univpll_d5",
		"univpll_d7",
		"univpll_d26",
		"univpll1_d2",
		"univpll1_d4",
		"univpll2_d2",
		"univpll2_d4",
		"univpll2_d8",
		"univpll3_d2",
		"univpll3_d4",
		"univpll3_d8",
		"vencpll_ck",
		/* TOP */
		"axi_sel",
		"mem_sel",
		"ddrphycfg_sel",
		"mm_sel",
		"pwm_sel",
		"vdec_sel",
		"mfg_sel",
		"camtg_sel",
		"uart_sel",
		"spi_sel",
		"msdc30_0_sel",
		"msdc30_1_sel",
		"msdc30_2_sel",
		"msdc50_3_hsel",
		"msdc50_3_sel",
		"audio_sel",
		"aud_intbus_sel",
		"pmicspi_sel",
		"scp_sel",
		"atb_sel",
		"mjc_sel",
		"dpi0_sel",
		"scam_sel",
		"aud_1_sel",
		"aud_2_sel",
		"dpi1_sel",
		"ufoenc_sel",
		"ufodec_sel",
		"eth_sel",
		"onfi_sel",
		"snfi_sel",
		"hdmi_sel",
		"rtc_sel",
		/* INFRA */
		"infra_pmic_tmr",
		"infra_pmic_ap",
		"infra_pmic_md",
		"infra_pmic_conn",
		"infra_scpsys",
		"infra_sej",
		"infra_apxgpt",
		"infra_usb",
		"infra_icusb",
		"infra_gce",
		"infra_therm",
		"infra_i2c0",
		"infra_i2c1",
		"infra_i2c2",
		"infra_pwm_hclk",
		"infra_pwm1",
		"infra_pwm2",
		"infra_pwm3",
		"infra_pwm",
		"infra_uart0",
		"infra_uart1",
		"infra_uart2",
		"infra_uart3",
		"infra_usb_mcu",
		"infra_nfi_ecc66",
		"infra_nfi_66m",
		"infra_btif",
		"infra_spi",
		"infra_msdc3",
		"infra_msdc1",
		"infra_msdc2",
		"infra_msdc0",
		"infra_gcpu",
		"infra_auxadc",
		"infra_cpum",
		"infra_irrx",
		"infra_ufo",
		"infra_cec",
		"infra_cec_26m",
		"infra_nfi_bclk",
		"infra_nfi_ecc",
		"infra_ap_dma",
		"infra_xiu",
		"infra_devapc",
		"infra_xiu2ahb",
		"infra_l2c_sram",
		"infra_eth_50m",
		"infra_debugsys",
		"infra_audio",
		"infra_eth_25m",
		"infra_nfi",
		"infra_onfi",
		"infra_snfi",
		"infra_eth",
		"infra_dramc_26m",
		/* MM */
		"mm_smi_comoon",
		"mm_smi_larb0",
		"mm_cam_mdp",
		"mm_mdp_rdma",
		"mm_mdp_rsz0",
		"mm_mdp_rsz1",
		"mm_mdp_tdshp",
		"mm_mdp_wdma",
		"mm_mdp_wrot",
		"mm_fake_eng",
		"mm_disp_ovl0",
		"mm_disp_ovl1",
		"mm_disp_rdma0",
		"mm_disp_rdma1",
		"mm_disp_wdma0",
		"mm_disp_color",
		"mm_disp_ccorr",
		"mm_disp_aal",
		"mm_disp_gamma",
		"mm_disp_dither",
		"mm_disp_ufoe",
		"mm_larb4_mm",
		"mm_larb4_mjc",
		"mm_disp_wdma1",
		"mm_ufodrdma0_l0",
		"mm_ufodrdma0_l1",
		"mm_ufodrdma0_l2",
		"mm_ufodrdma0_l3",
		"mm_ufodrdma1_l0",
		"mm_ufodrdma1_l1",
		"mm_ufodrdma1_l2",
		"mm_ufodrdma1_l3",
		"mm_disp_pwm0mm",
		"mm_disp_pwm026m",
		"mm_dsi0_engine",
		"mm_dsi0_digital",
		"mm_dpi_pixel",
		"mm_dpi_engine",
		"mm_lvds_pixel",
		"mm_lvds_cts",
		"mm_dpi1_pixel",
		"mm_dpi1_engine",
		"mm_hdmi_pixel",
		"mm_hdmi_spdif",
		"mm_hdmi_audio",
		"mm_hdmi_pllck",
		"mm_disp_dsc_eng",
		"mm_disp_dsc_mem",
		/* IMG */
		"img_larb2_smi",
		"img_jpgenc",
		"img_cam_smi",
		"img_cam_cam",
		"img_sen_tg",
		"img_sen_cam",
		"img_cam_sv",
		/* VDEC */
		"vdec_cken",
		"vdec_larb_cken",
		/* VENC */
		"venc_cke0",
		"venc_cke1",
		"venc_cke2",
		"venc_cke3",
		/* MFG */
		"mfg_ff",
		"mfg_dw",
		"mfg_tx",
		"mfg_mx",
		/* AUDIO */
		"aud_afe",
		"aud_i2s",
		"aud_22m",
		"aud_24m",
		"aud_spdf2",
		"aud_apll2_tnr",
		"aud_apll_tnr",
		"aud_hdmi",
		"aud_spdf",
		"aud_adc",
		"aud_dac",
		"aud_dac_predis",
		"aud_tml",
		"aud_ahb_idl_ex",
		"aud_ahb_idl_in",
#endif /* CLKDBG_SOC */
	};

	*num = ARRAY_SIZE(clks);

	return clks;
}

static void dump_clk_state(const char *clkname, struct seq_file *s)
{
	struct clk *c = __clk_lookup(clkname);
	struct clk *p = IS_ERR_OR_NULL(c) ? NULL : __clk_get_parent(c);

	if (IS_ERR_OR_NULL(c)) {
		seq_printf(s, "[%17s: NULL]\n", clkname);
		return;
	}

	seq_printf(s, "[%-17s: %3s, %3d, %3d, %10ld, %17s]\n",
		__clk_get_name(c),
		(__clk_is_enabled(c) || __clk_is_prepared(c)) ? "ON" : "off",
		__clk_is_prepared(c),
		__clk_get_enable_count(c),
		__clk_get_rate(c),
		p ? __clk_get_name(p) : "");
}

static int clkdbg_dump_state_all(struct seq_file *s, void *v)
{
	int i;
	size_t num;

	const char * const *clks = get_all_clk_names(&num);

	pr_debug("\n");
	for (i = 0; i < num; i++)
		dump_clk_state(clks[i], s);

	return 0;
}

static char last_cmd[128] = "null";

static int clkdbg_prepare_enable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)];
	char *c = cmd;
	char *ign;
	char *clk_name;
	struct clk *clk;
	int r;

	strcpy(cmd, last_cmd);

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");

	seq_printf(s, "clk_prepare_enable(%s): ", clk_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	r = clk_prepare_enable(clk);
	seq_printf(s, "%d\n", r);

	return r;
}

static int clkdbg_disable_unprepare(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)];
	char *c = cmd;
	char *ign;
	char *clk_name;
	struct clk *clk;

	strcpy(cmd, last_cmd);

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");

	seq_printf(s, "clk_disable_unprepare(%s): ", clk_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	clk_disable_unprepare(clk);
	seq_puts(s, "0\n");

	return 0;
}

static int clkdbg_set_parent(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)];
	char *c = cmd;
	char *ign;
	char *clk_name;
	char *parent_name;
	struct clk *clk;
	struct clk *parent;
	int r;

	strcpy(cmd, last_cmd);

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");
	parent_name = strsep(&c, " ");

	seq_printf(s, "clk_set_parent(%s, %s): ", clk_name, parent_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	parent = __clk_lookup(parent_name);
	if (IS_ERR_OR_NULL(parent)) {
		seq_printf(s, "clk_get(): 0x%p\n", parent);
		return PTR_ERR(parent);
	}

	clk_prepare_enable(clk);
	r = clk_set_parent(clk, parent);
	clk_disable_unprepare(clk);
	seq_printf(s, "%d\n", r);

	return r;
}

static int clkdbg_set_rate(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)];
	char *c = cmd;
	char *ign;
	char *clk_name;
	char *rate_str;
	struct clk *clk;
	unsigned long rate;
	int r;

	strcpy(cmd, last_cmd);

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");
	rate_str = strsep(&c, " ");
	r = kstrtoul(rate_str, 0, &rate);

	seq_printf(s, "clk_set_rate(%s, %lu): %d: ", clk_name, rate, r);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	r = clk_set_rate(clk, rate);
	seq_printf(s, "%d\n", r);

	return r;
}

void *reg_from_str(const char *str)
{
	if (sizeof(void *) == sizeof(unsigned long)) {
		unsigned long v;

		if (kstrtoul(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	} else if (sizeof(void *) == sizeof(unsigned long long)) {
		unsigned long long v;

		if (kstrtoull(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	} else {
		unsigned long long v;

		clk_warn("unexpected pointer size: sizeof(void *): %zu\n",
			sizeof(void *));

		if (kstrtoull(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	}

	clk_warn("%s(): parsing error: %s\n", __func__, str);

	return NULL;
}

static int parse_reg_val_from_cmd(void __iomem **preg, unsigned long *pval)
{
	char cmd[sizeof(last_cmd)];
	char *c = cmd;
	char *ign;
	char *reg_str;
	char *val_str;
	int r = 0;

	strcpy(cmd, last_cmd);

	ign = strsep(&c, " ");
	reg_str = strsep(&c, " ");
	val_str = strsep(&c, " ");

	if (preg)
		*preg = reg_from_str(reg_str);

	if (pval)
		r = kstrtoul(val_str, 0, pval);

	return r;
}

static int clkdbg_reg_read(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, NULL);
	seq_printf(s, "readl(0x%p): ", reg);

	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_write(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_writel(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_set(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_setl(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_clr(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_clrl(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

#if CLKDBG_PM_DOMAIN

/*
 * pm_domain support
 */

static struct generic_pm_domain **get_all_pm_domain(int *numpd)
{
	static struct generic_pm_domain *pds[10];
	const int maxpd = ARRAY_SIZE(pds);
#if CLKDBG_8163
	const char *cmp = "mediatek,mt8167-scpsys";
#endif

	struct device_node *node;
	int i;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		clk_err("node '%s' not found!\n", cmp);
		return NULL;
	}

	for (i = 0; i < maxpd; i++) {
		struct of_phandle_args pa;

		pa.np = node;
		pa.args[0] = i;
		pa.args_count = 1;
		pds[i] = of_genpd_get_from_provider(&pa);

		if (IS_ERR(pds[i]))
			break;
	}

	if (numpd)
		*numpd = i;

	return pds;
}

static struct generic_pm_domain *genpd_from_name(const char *name)
{
	int i;
	int numpd;
	struct generic_pm_domain **pds = get_all_pm_domain(&numpd);

	for (i = 0; i < numpd; i++) {
		struct generic_pm_domain *pd = pds[i];

		if (IS_ERR_OR_NULL(pd))
			continue;

		if (strcmp(name, pd->name) == 0)
			return pd;
	}

	return NULL;
}

static struct platform_device *pdev_from_name(const char *name)
{
	int i;
	int numpd;
	struct generic_pm_domain **pds = get_all_pm_domain(&numpd);

	for (i = 0; i < numpd; i++) {
		struct pm_domain_data *pdd;
		struct generic_pm_domain *pd = pds[i];

		if (IS_ERR_OR_NULL(pd))
			continue;

		list_for_each_entry(pdd, &pd->dev_list, list_node) {
			struct device *dev = pdd->dev;
			struct platform_device *pdev = to_platform_device(dev);

			if (strcmp(name, pdev->name) == 0)
				return pdev;
		}
	}

	return NULL;
}

static void seq_print_all_genpd(struct seq_file *s)
{
	static const char * const gpd_status_name[] = {
		"ACTIVE",
		"WAIT_MASTER",
		"BUSY",
		"REPEAT",
		"POWER_OFF",
	};

	static const char * const prm_status_name[] = {
		"active",
		"resuming",
		"suspended",
		"suspending",
	};

	int i;
	int numpd;
	struct generic_pm_domain **pds = get_all_pm_domain(&numpd);

	seq_puts(s, "domain_on [pmd_name  status]\n");
	seq_puts(s, "\tdev_on (dev_name usage_count, disable, status)\n");
	seq_puts(s, "------------------------------------------------------\n");

	for (i = 0; i < numpd; i++) {
		struct pm_domain_data *pdd;
		struct generic_pm_domain *pd = pds[i];

		if (IS_ERR_OR_NULL(pd)) {
			seq_printf(s, "pd[%d]: 0x%p\n", i, pd);
			continue;
		}

		seq_printf(s, "%c [%-9s %11s]\n",
			(pd->status == GPD_STATE_ACTIVE) ? '+' : '-',
			pd->name, gpd_status_name[pd->status]);

		list_for_each_entry(pdd, &pd->dev_list, list_node) {
			struct device *dev = pdd->dev;
			struct platform_device *pdev = to_platform_device(dev);

			seq_printf(s, "\t%c (%-16s %3d, %d, %10s)\n",
				pm_runtime_active(dev) ? '+' : '-',
				pdev->name,
				atomic_read(&dev->power.usage_count),
				dev->power.disable_depth,
				prm_status_name[dev->power.runtime_status]);
		}
	}
}

static int clkdbg_dump_genpd(struct seq_file *s, void *v)
{
	seq_print_all_genpd(s);

	return 0;
}

static int clkdbg_pm_genpd_poweron(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)];
	char *c = cmd;
	char *ign;
	char *genpd_name;
	struct generic_pm_domain *pd;

	strcpy(cmd, last_cmd);

	ign = strsep(&c, " ");
	genpd_name = strsep(&c, " ");

	seq_printf(s, "pm_genpd_poweron(%s): ", genpd_name);

	pd = genpd_from_name(genpd_name);
	if (pd) {
		int r = pm_genpd_poweron(pd);

		seq_printf(s, "%d\n", r);
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_genpd_poweroff_unused(struct seq_file *s, void *v)
{
	seq_puts(s, "pm_genpd_poweroff_unused()\n");
	pm_genpd_poweroff_unused();

	return 0;
}

static int clkdbg_pm_runtime_enable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strcpy(cmd, last_cmd);

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_enable(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		pm_runtime_enable(&pdev->dev);
		seq_puts(s, "\n");
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_disable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strcpy(cmd, last_cmd);

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_disable(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		pm_runtime_disable(&pdev->dev);
		seq_puts(s, "\n");
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_get_sync(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strcpy(cmd, last_cmd);

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_get_sync(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		int r = pm_runtime_get_sync(&pdev->dev);

		seq_printf(s, "%d\n", r);
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_put_sync(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strcpy(cmd, last_cmd);

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_put_sync(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		int r = pm_runtime_put_sync(&pdev->dev);

		seq_printf(s, "%d\n", r);
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

#endif /* CLKDBG_PM_DOMAIN */

#define CMDFN(_cmd, _fn) {	\
	.cmd = _cmd,		\
	.fn = _fn,		\
}

static int clkdbg_show(struct seq_file *s, void *v)
{
	static const struct {
		const char	*cmd;
		int (*fn)(struct seq_file *, void *);
	} cmds[] = {
		CMDFN("dump_regs", seq_print_regs),
		CMDFN("dump_state", clkdbg_dump_state_all),
		CMDFN("fmeter", seq_print_fmeter_all),
		CMDFN("prepare_enable", clkdbg_prepare_enable),
		CMDFN("disable_unprepare", clkdbg_disable_unprepare),
		CMDFN("set_parent", clkdbg_set_parent),
		CMDFN("set_rate", clkdbg_set_rate),
		CMDFN("reg_read", clkdbg_reg_read),
		CMDFN("reg_write", clkdbg_reg_write),
		CMDFN("reg_set", clkdbg_reg_set),
		CMDFN("reg_clr", clkdbg_reg_clr),
#if CLKDBG_PM_DOMAIN
		CMDFN("dump_genpd", clkdbg_dump_genpd),
		CMDFN("pm_genpd_poweron", clkdbg_pm_genpd_poweron),
		CMDFN("pm_genpd_poweroff_unused",
			clkdbg_pm_genpd_poweroff_unused),
		CMDFN("pm_runtime_enable", clkdbg_pm_runtime_enable),
		CMDFN("pm_runtime_disable", clkdbg_pm_runtime_disable),
		CMDFN("pm_runtime_get_sync", clkdbg_pm_runtime_get_sync),
		CMDFN("pm_runtime_put_sync", clkdbg_pm_runtime_put_sync),
#endif /* CLKDBG_PM_DOMAIN */
	};

	int i;
	char cmd[sizeof(last_cmd)];

	pr_debug("last_cmd: %s\n", last_cmd);

	strcpy(cmd, last_cmd);

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		char *c = cmd;
		char *token = strsep(&c, " ");

		if (strcmp(cmds[i].cmd, token) == 0)
			return cmds[i].fn(s, v);
	}

	return 0;
}

static int clkdbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, clkdbg_show, NULL);
}

static ssize_t clkdbg_write(
		struct file *file,
		const char __user *buffer,
		size_t count,
		loff_t *data)
{
	char desc[sizeof(last_cmd)];
	int len = 0;

	pr_debug("count: %zu\n", count);
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';
	strcpy(last_cmd, desc);
	if (last_cmd[len - 1] == '\n')
		last_cmd[len - 1] = 0;

	return count;
}

static const struct file_operations clkdbg_fops = {
	.owner		= THIS_MODULE,
	.open		= clkdbg_open,
	.read		= seq_read,
	.write		= clkdbg_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void __iomem *get_reg(struct device_node *np, int index)
{
#if DUMMY_REG_TEST
	return kzalloc(PAGE_SIZE, GFP_KERNEL);
#else
	return of_iomap(np, index);
#endif
}

static int __init get_base_from_node(
			const char *cmp, int idx, void __iomem **pbase)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		clk_warn("node '%s' not found!\n", cmp);
		return -1;
	}

	*pbase = get_reg(node, idx);
	clk_info("%s base: 0x%p\n", cmp, *pbase);

	return 0;
}

static void __init init_iomap(void)
{
#if CLKDBG_8163
	get_base_from_node("mediatek,mt8167-topckgen", 0, &topckgen_base);
	get_base_from_node("mediatek,mt8167-infracfg", 0, &infrasys_base);
	get_base_from_node("mediatek,mt8167-apmixedsys", 0, &apmixedsys_base);
	get_base_from_node("mediatek,mt8167-audiosys", 0, &audiosys_base);
	get_base_from_node("mediatek,mt8167-mfgsys", 0, &mfgsys_base);
	get_base_from_node("mediatek,mt8167-mmsys", 0, &mmsys_base);
	get_base_from_node("mediatek,mt8167-imgsys", 0, &imgsys_base);
	get_base_from_node("mediatek,mt8167-vdecsys", 0, &vdecsys_base);
	get_base_from_node("mediatek,mt8167-vencsys", 0, &vencsys_base);
	get_base_from_node("mediatek,mt8167-scpsys", 0, &scpsys_base);
#endif /* CLKDBG_SOC */
}

/*
 * clkdbg pm_domain support
 */

static const struct of_device_id clkdbg_id_table[] = {
	{ .compatible = "mediatek,clkdbg-pd",},
	{ },
};
MODULE_DEVICE_TABLE(of, clkdbg_id_table);

static int clkdbg_probe(struct platform_device *pdev)
{
	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static int clkdbg_remove(struct platform_device *pdev)
{
	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static int clkdbg_pd_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static int clkdbg_pd_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static const struct dev_pm_ops clkdbg_pd_pm_ops = {
	.runtime_suspend = clkdbg_pd_runtime_suspend,
	.runtime_resume = clkdbg_pd_runtime_resume,
};

static struct platform_driver clkdbg_driver = {
	.probe		= clkdbg_probe,
	.remove		= clkdbg_remove,
	.driver		= {
		.name	= "clkdbg",
		.owner	= THIS_MODULE,
		.of_match_table = clkdbg_id_table,
		.pm = &clkdbg_pd_pm_ops,
	},
};

module_platform_driver(clkdbg_driver);

/*
 * pm_domain sample code
 */

static struct platform_device *my_pdev;

int power_on_before_work(void)
{
	return pm_runtime_get_sync(&my_pdev->dev);
}

int power_off_after_work(void)
{
	return pm_runtime_put_sync(&my_pdev->dev);
}

static const struct of_device_id my_id_table[] = {
	{ .compatible = "mediatek,my-device",},
	{ },
};
MODULE_DEVICE_TABLE(of, my_id_table);

static int my_probe(struct platform_device *pdev)
{
	pm_runtime_enable(&pdev->dev);

	my_pdev = pdev;

	return 0;
}

static int my_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static struct platform_driver my_driver = {
	.probe		= my_probe,
	.remove		= my_remove,
	.driver		= {
		.name	= "my_driver",
		.owner	= THIS_MODULE,
		.of_match_table = my_id_table,
	},
};

module_platform_driver(my_driver);

/*
 * init functions
 */

int __init mt_clkdbg_init(void)
{
	init_iomap();

#if DUMP_INIT_STATE
	print_regs();
	print_fmeter_all();
#endif /* DUMP_INIT_STATE */

	return 0;
}
arch_initcall(mt_clkdbg_init);

int __init mt_clkdbg_debug_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("clkdbg", 0, 0, &clkdbg_fops);
	if (!entry)
		return -ENOMEM;

	return 0;
}
module_init(mt_clkdbg_debug_init);
