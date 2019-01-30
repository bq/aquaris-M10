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

#include <linux/clk-provider.h>
#include <linux/io.h>

#include "clkdbg.h"

#define ALL_CLK_ON		0
#define DUMP_INIT_STATE		0

/*
 * clkdbg dump_regs
 */

enum {
	topckgen,
	infracfg,
	pericfg,
	scpsys,
	ddrphy,
	apmixed,
	audiosys,
	mfgsys,
	mmsys,
	imgsys,
	vdecsys,
};

#define REGBASE_V(_phys, _id_name) { .phys = _phys, .name = #_id_name }

/*
 * checkpatch.pl ERROR:COMPLEX_MACRO
 *
 * #define REGBASE(_phys, _id_name) [_id_name] = REGBASE_V(_phys, _id_name)
 */

static struct regbase rb[] = {
	[topckgen] = REGBASE_V(0x10000000, topckgen),
	[infracfg] = REGBASE_V(0x10001000, infracfg),
	[pericfg]  = REGBASE_V(0x10003000, pericfg),
	[scpsys]   = REGBASE_V(0x10006000, scpsys),
	[ddrphy]   = REGBASE_V(0x1000f000, ddrphy),
	[apmixed]  = REGBASE_V(0x10209000, apmixed),
	[audiosys] = REGBASE_V(0x11220000, audiosys),
	[mfgsys]   = REGBASE_V(0x13000000, mfgsys),
	[mmsys]    = REGBASE_V(0x14000000, mmsys),
	[imgsys]   = REGBASE_V(0x15000000, imgsys),
	[vdecsys]  = REGBASE_V(0x16000000, vdecsys),
};

#define REGNAME(_base, _ofs, _name)	\
	{ .base = &rb[_base], .ofs = _ofs, .name = #_name }

static struct regname rn[] = {
	REGNAME(topckgen, 0x040, CLK_CFG_0),
	REGNAME(topckgen, 0x050, CLK_CFG_1),
	REGNAME(topckgen, 0x060, CLK_CFG_2),
	REGNAME(topckgen, 0x070, CLK_CFG_3),
	REGNAME(topckgen, 0x080, CLK_CFG_4),
	REGNAME(topckgen, 0x090, CLK_CFG_5),
	REGNAME(topckgen, 0x0a0, CLK_CFG_6),
	REGNAME(topckgen, 0x100, CLK_CFG_8),
	REGNAME(topckgen, 0x104, CLK_CFG_9),
	REGNAME(topckgen, 0x108, CLK_CFG_10),
	REGNAME(topckgen, 0x10c, CLK_CFG_11),
	REGNAME(topckgen, 0x214, CLK_MISC_CFG_1),
	REGNAME(topckgen, 0x220, CLK26CALI_0),
	REGNAME(topckgen, 0x224, CLK26CALI_1),
	REGNAME(topckgen, 0x228, CLK26CALI_2),
	REGNAME(infracfg, 0x040, INFRA_PDN_SET),
	REGNAME(infracfg, 0x044, INFRA_PDN_CLR),
	REGNAME(infracfg, 0x048, INFRA_PDN_STA),
	REGNAME(infracfg, 0x220, TOPAXI_PROT_EN),
	REGNAME(infracfg, 0x228, TOPAXI_PROT_STA1),
	REGNAME(pericfg, 0x008, PERI_PDN0_SET),
	REGNAME(pericfg, 0x010, PERI_PDN0_CLR),
	REGNAME(pericfg, 0x018, PERI_PDN0_STA),
	REGNAME(pericfg, 0x00c, PERI_PDN1_SET),
	REGNAME(pericfg, 0x014, PERI_PDN1_CLR),
	REGNAME(pericfg, 0x01c, PERI_PDN1_STA),
	REGNAME(scpsys, 0x210, SPM_VDE_PWR_CON),
	REGNAME(scpsys, 0x214, SPM_MFG_PWR_CON),
	REGNAME(scpsys, 0x234, SPM_IFR_PWR_CON),
	REGNAME(scpsys, 0x238, SPM_ISP_PWR_CON),
	REGNAME(scpsys, 0x23c, SPM_DIS_PWR_CON),
	REGNAME(scpsys, 0x280, SPM_CONN_PWR_CON),
	REGNAME(scpsys, 0x60c, SPM_PWR_STATUS),
	REGNAME(scpsys, 0x610, SPM_PWR_STATUS_S),
	REGNAME(ddrphy, 0x800, VENCPLL_CON0),
	REGNAME(ddrphy, 0x804, VENCPLL_CON1),
	REGNAME(ddrphy, 0x80c, VENCPLL_PWR_CON0),
	REGNAME(apmixed, 0x000, AP_PLL_CON0),
	REGNAME(apmixed, 0x004, AP_PLL_CON1),
	REGNAME(apmixed, 0x008, AP_PLL_CON2),
	REGNAME(apmixed, 0x014, PLL_HP_CON0),
	REGNAME(apmixed, 0x200, ARMPLL_CON0),
	REGNAME(apmixed, 0x204, ARMPLL_CON1),
	REGNAME(apmixed, 0x20c, ARMPLL_PWR_CON0),
	REGNAME(apmixed, 0x210, MAINPLL_CON0),
	REGNAME(apmixed, 0x214, MAINPLL_CON1),
	REGNAME(apmixed, 0x21c, MAINPLL_PWR_CON0),
	REGNAME(apmixed, 0x220, UNIVPLL_CON0),
	REGNAME(apmixed, 0x224, UNIVPLL_CON1),
	REGNAME(apmixed, 0x22c, UNIVPLL_PWR_CON0),
	REGNAME(apmixed, 0x230, MMPLL_CON0),
	REGNAME(apmixed, 0x234, MMPLL_CON1),
	REGNAME(apmixed, 0x23c, MMPLL_PWR_CON0),
	REGNAME(apmixed, 0x240, MSDCPLL_CON0),
	REGNAME(apmixed, 0x244, MSDCPLL_CON1),
	REGNAME(apmixed, 0x24c, MSDCPLL_PWR_CON0),
	REGNAME(apmixed, 0x250, AUDPLL_CON0),
	REGNAME(apmixed, 0x254, AUDPLL_CON1),
	REGNAME(apmixed, 0x25c, AUDPLL_PWR_CON0),
	REGNAME(apmixed, 0x260, TVDPLL_CON0),
	REGNAME(apmixed, 0x264, TVDPLL_CON1),
	REGNAME(apmixed, 0x26c, TVDPLL_PWR_CON0),
	REGNAME(apmixed, 0x270, LVDSPLL_CON0),
	REGNAME(apmixed, 0x274, LVDSPLL_CON1),
	REGNAME(apmixed, 0x27c, LVDSPLL_PWR_CON0),
	REGNAME(audiosys, 0x000, AUDIO_TOP_CON0),
	REGNAME(mfgsys, 0x000,  MFG_CG_CON),
	REGNAME(mfgsys, 0x004,  MFG_CG_SET),
	REGNAME(mfgsys, 0x008,  MFG_CG_CLR),
	REGNAME(mmsys, 0x100, DISP_CG_CON0),
	REGNAME(mmsys, 0x104, DISP_CG_SET0),
	REGNAME(mmsys, 0x108, DISP_CG_CLR0),
	REGNAME(mmsys, 0x110, DISP_CG_CON1),
	REGNAME(mmsys, 0x114, DISP_CG_SET1),
	REGNAME(mmsys, 0x118, DISP_CG_CLR1),
	REGNAME(imgsys, 0x000, IMG_CG_CON),
	REGNAME(imgsys, 0x004, IMG_CG_SET),
	REGNAME(imgsys, 0x008, IMG_CG_CLR),
	REGNAME(vdecsys, 0x000, VDEC_CKEN_SET),
	REGNAME(vdecsys, 0x004, VDEC_CKEN_CLR),
	REGNAME(vdecsys, 0x008, LARB_CKEN_SET),
	REGNAME(vdecsys, 0x00c, LARB_CKEN_CLR),
	{}
};

static const struct regname *get_all_regnames(void)
{
	return rn;
}

static void __init init_regbase(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rb); i++)
		rb[i].virt = ioremap(rb[i].phys, PAGE_SIZE);
}

/*
 * clkdbg fmeter
 */

#include <linux/delay.h>

#ifndef GENMASK
#define GENMASK(h, l)	(((1U << ((h) - (l) + 1)) - 1) << (l))
#endif

#define ALT_BITS(o, h, l, v) \
	(((o) & ~GENMASK(h, l)) | (((v) << (l)) & GENMASK(h, l)))

#define clk_readl(addr)		readl(addr)
#define clk_writel(addr, val)	\
	do { writel(val, addr); wmb(); } while (0) /* sync write */
#define clk_writel_mask(addr, h, l, v)	\
	clk_writel(addr, (clk_readl(addr) & ~GENMASK(h, l)) | ((v) << (l)))

#define ABS_DIFF(a, b)	((a) > (b) ? (a) - (b) : (b) - (a))

#define CLK_CFG_8	(rb[topckgen].virt + 0x100)
#define CLK_CFG_9	(rb[topckgen].virt + 0x104)
#define CLK_MISC_CFG_1	(rb[topckgen].virt + 0x214)
#define CLK26CALI_0	(rb[topckgen].virt + 0x220)
#define CLK26CALI_1	(rb[topckgen].virt + 0x224)
#define CLK26CALI_2	(rb[topckgen].virt + 0x228)
#define PLL_HP_CON0	(rb[apmixed].virt + 0x014)

enum FMETER_TYPE {
	FT_NULL,
	ABIST,
	CKGEN
};

#define FMCLK(_t, _i, _n) { .type = _t, .id = _i, .name = _n }

static const struct fmeter_clk fclks[] = {
	FMCLK(ABIST,  1, "AD_MAIN_H546M_CK"),
	FMCLK(ABIST,  2, "AD_MAIN_H364M_CK"),
	FMCLK(ABIST,  3, "AD_MAIN_H218P4M_CK"),
	FMCLK(ABIST,  4, "AD_MAIN_H156M_CK"),
	FMCLK(ABIST,  5, "AD_UNIV_624M_CK"),
	FMCLK(ABIST,  6, "AD_UNIV_416M_CK"),
	FMCLK(ABIST,  7, "AD_UNIV_249P6M_CK"),
	FMCLK(ABIST,  8, "AD_UNIV_178P3M_CK"),
	FMCLK(ABIST,  9, "AD_UNIV_48M_CK"),
	FMCLK(ABIST, 10, "AD_USB_48M_CK"),
	FMCLK(ABIST, 11, "AD_MMPLL_CK"),
	FMCLK(ABIST, 12, "AD_MSDCPLL_CK"),
	FMCLK(ABIST, 13, "AD_DPICLK"),
	FMCLK(ABIST, 14, "clkph_mck_o"),
	FMCLK(ABIST, 15, "AD_MEMPLL2_CKOUT0_PRE_ISO"),
	FMCLK(ABIST, 16, "AD_MCUPLL1_H481M_CK"),
	FMCLK(ABIST, 17, "AD_MDPLL1_416M_CK"),
	FMCLK(ABIST, 18, "AD_WPLL_CK"),
	FMCLK(ABIST, 19, "AD_WHPLL_CK"),
	FMCLK(ABIST, 20, "rtc32k_ck_i"),
	FMCLK(ABIST, 21, "AD_SYS_26M_CK"),
	FMCLK(ABIST, 22, "AD_VENCPLL_CK"),
	FMCLK(ABIST, 33, "AD_MIPI_26M_CK"),
	FMCLK(ABIST, 35, "AD_MEM_26M_CK"),
	FMCLK(ABIST, 36, "AD_PLLGP_TST_CK"),
	FMCLK(ABIST, 37, "AD_DSI0_LNTC_DSICLK"),
	FMCLK(ABIST, 38, "AD_MPPLL_TST_CK"),
	FMCLK(ABIST, 39, "armpll_occ_mon"),
	FMCLK(ABIST, 40, "AD_MEM2MIPI_26M_CK"),
	FMCLK(ABIST, 41, "AD_MEMPLL_MONCLK"),
	FMCLK(ABIST, 42, "AD_MEMPLL2_MONCLK"),
	FMCLK(ABIST, 43, "AD_MEMPLL3_MONCLK"),
	FMCLK(ABIST, 44, "AD_MEMPLL4_MONCLK"),
	FMCLK(ABIST, 45, "AD_MEMPLL_REFCLK"),
	FMCLK(ABIST, 46, "AD_MEMPLL_FBCLK"),
	FMCLK(ABIST, 47, "AD_MEMPLL2_REFCLK"),
	FMCLK(ABIST, 48, "AD_MEMPLL2_FBCLK"),
	FMCLK(ABIST, 49, "AD_MEMPLL3_REFCLK"),
	FMCLK(ABIST, 50, "AD_MEMPLL3_FBCLK"),
	FMCLK(ABIST, 51, "AD_MEMPLL4_REFCLK"),
	FMCLK(ABIST, 52, "AD_MEMPLL4_FBCLK"),
	FMCLK(ABIST, 53, "AD_MEMPLL_TSTDIV2_CK"),
	FMCLK(ABIST, 54, "AD_LVDSPLL_CK"),
	FMCLK(ABIST, 55, "AD_LVDSTX_MONCLK"),
	FMCLK(ABIST, 56, "AD_HDMITX_MONCLK"),
	FMCLK(ABIST, 57, "AD_USB20_C240M"),
	FMCLK(ABIST, 58, "AD_USB20_C240M_1P"),
	FMCLK(ABIST, 59, "AD_MONREF_CK"),
	FMCLK(ABIST, 60, "AD_MONFBK_CK"),
	FMCLK(ABIST, 61, "AD_TVDPLL_CK"),
	FMCLK(ABIST, 62, "AD_AUDPLL_CK"),
	FMCLK(ABIST, 63, "AD_LVDSPLL_ETH_CK"),
	FMCLK(CKGEN,  1, "hf_faxi_ck"),
	FMCLK(CKGEN,  2, "hd_faxi_ck"),
	FMCLK(CKGEN,  3, "hf_fnfi2x_ck"),
	FMCLK(CKGEN,  4, "hf_fddrphycfg_ck"),
	FMCLK(CKGEN,  5, "hf_fmm_ck"),
	FMCLK(CKGEN,  6, "f_fpwm_ck"),
	FMCLK(CKGEN,  7, "hf_fvdec_ck"),
	FMCLK(CKGEN,  8, "hf_fmfg_ck"),
	FMCLK(CKGEN,  9, "hf_fcamtg_ck"),
	FMCLK(CKGEN, 10, "f_fuart_ck"),
	FMCLK(CKGEN, 11, "hf_fspi_ck"),
	FMCLK(CKGEN, 12, "f_fusb20_ck"),
	FMCLK(CKGEN, 13, "hf_fmsdc30_0_ck"),
	FMCLK(CKGEN, 14, "hf_fmsdc30_1_ck"),
	FMCLK(CKGEN, 15, "hf_fmsdc30_2_ck"),
	FMCLK(CKGEN, 16, "hf_faudio_ck"),
	FMCLK(CKGEN, 17, "hf_faud_intbus_ck"),
	FMCLK(CKGEN, 18, "hf_fpmicspi_ck"),
	FMCLK(CKGEN, 19, "f_frtc_ck"),
	FMCLK(CKGEN, 20, "f_f26m_ck"),
	FMCLK(CKGEN, 21, "f_f32k_md1_ck"),
	FMCLK(CKGEN, 22, "f_frtc_conn_ck"),
	FMCLK(CKGEN, 23, "hf_feth_50m_ck"),
	FMCLK(CKGEN, 25, "hd_haxi_nli_ck"),
	FMCLK(CKGEN, 26, "hd_qaxidcm_ck"),
	FMCLK(CKGEN, 27, "f_ffpc_ck"),
	FMCLK(CKGEN, 28, "hf_fdpi0_ck"),
	FMCLK(CKGEN, 29, "f_fckbus_ck_scan"),
	FMCLK(CKGEN, 30, "f_fckrtc_ck_scan"),
	FMCLK(CKGEN, 31, "hf_fdpilvds_ck"),
	{}
};

static void set_fmeter_divider_ca7(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_1);

	v = ALT_BITS(v, 15, 8, k1);
	clk_writel(CLK_MISC_CFG_1, v);
}

static void set_fmeter_divider(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_1);

	v = ALT_BITS(v, 7, 0, k1);
	v = ALT_BITS(v, 31, 24, k1);
	clk_writel(CLK_MISC_CFG_1, v);
}

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

static u32 fmeter_freq(enum FMETER_TYPE type, int k1, int clk)
{
	u32 cksw_ckgen[] =	{20, 16, clk};
	u32 cksw_abist[] =	{13, 8, clk};
	void __iomem *clk_cfg_reg =
				(type == CKGEN) ? CLK_CFG_9	: CLK_CFG_8;
	void __iomem *cnt_reg =
				(type == CKGEN) ? CLK26CALI_2	: CLK26CALI_1;
	u32 *cksw_hlv =		(type == CKGEN) ? cksw_ckgen	: cksw_abist;
	u32 tri_bit =		(type == CKGEN) ? BIT(4)	: BIT(0);

	u32 clk_misc_cfg_1;
	u32 clk_cfg_val;
	u32 freq;

	/* setup fmeter */
	clk_setl(CLK26CALI_0, BIT(7));			/* enable fmeter_en */

	clk_misc_cfg_1 = clk_readl(CLK_MISC_CFG_1);	/* backup reg value */
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
	clk_writel(CLK_MISC_CFG_1, clk_misc_cfg_1);

	return freq;
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

static const struct fmeter_clk *get_all_fmeter_clks(void)
{
	return fclks;
}

static void *prepare_fmeter(void)
{
	static u32 old_pll_hp_con0;

	old_pll_hp_con0 = clk_readl(PLL_HP_CON0);
	clk_writel(PLL_HP_CON0, 0x0);		/* disable PLL hopping */
	udelay(10);

	return &old_pll_hp_con0;
}

static void unprepare_fmeter(void *data)
{
	u32 old_pll_hp_con0 = *(u32 *)data;

	/* restore old setting */
	clk_writel(PLL_HP_CON0, old_pll_hp_con0);
}

static u32 fmeter_freq_op(const struct fmeter_clk *fclk)
{
	if (fclk->type)
		return measure_stable_fmeter_freq(fclk->type, 0, fclk->id);

	return 0;
}

/*
 * clkdbg dump_state
 */

static const char * const *get_all_clk_names(void)
{
	static const char * const clks[] = {
		/* ROOT */
		"clkph_mck_o",
		"mempll2",
		"fpc_ck",
		"hdmitx_clkdig",
		"lvdstx_clkdig",
		"lvdspll_eth",
		"dpi_ck",
		"vencpll_ck",
		/* APMIXEDSYS */
		"armpll",
		"mainpll",
		"univpll",
		"mmpll",
		"msdcpll",
		"audpll",
		"tvdpll",
		"lvdspll",
		/* TOP_DIVS */
		"syspll_d2",
		"syspll_d3",
		"syspll_d5",
		"6syspll_d7",
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
		"univpll_d2",
		"univpll_d3",
		"univpll_d5",
		"univpll_d26",
		"univpll1_d2",
		"univpll1_d4",
		"univpll1_d8",
		"univpll2_d2",
		"univpll2_d4",
		"univpll2_d8",
		"univpll3_d2",
		"univpll3_d4",
		"univpll3_d8",
		"msdcpll_ck",
		"msdcpll_d2",
		"mmpll_ck",
		"mmpll_d2",
		"dmpll_ck",
		"dmpll_d2",
		"dmpll_d4",
		"dmpll_x2",
		"tvdpll_ck",
		"tvdpll_d2",
		"tvdpll_d4",
		"mipipll_ck",
		"mipipll_d2",
		"mipipll_d4",
		"hdmipll_ck",
		"hdmipll_d2",
		"hdmipll_d3",
		"lvdspll_ck",
		"lvdspll_d2",
		"lvdspll_d4",
		"lvdspll_d8",
		"audpll_ck",
		"audpll_d4",
		"audpll_d8",
		"audpll_d16",
		"audpll_d24",
		"32k_internal",
		"32k_external",
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
		"spi0_sel",
		"usb20_sel",
		"msdc30_0_sel",
		"msdc30_1_sel",
		"msdc30_2_sel",
		"audio_sel",
		"aud_intbus_sel",
		"pmicspi_sel",
		"scp_sel",
		"dpi0_sel",
		"dpi1_sel",
		"tve_sel",
		"hdmi_sel",
		"apll_sel",
		"dpilvds_sel",
		"rtc_sel",
		"nfi2x_sel",
		"eth_50m_sel",
		/* INFRA */
		"pmicwrap_ck",
		"pmicspi_ck",
		"irrx_ck",
		"cec_ck",
		"kp_ck",
		"cpum_ck",
		"trng_ck",
		"connsys_bus",
		"m4u_ck",
		"l2c_sram_ck",
		"efuse_ck",
		"audio_ck",
		"smi_ck",
		"dbgclk",
		"clk13m",
		"ckmux_sel",
		"ckdiv1",
		/* PERI */
		"usbslv_ck",
		"usb1_mcu_ck",
		"usb0_mcu_ck",
		"eth_ck",
		"spi0_ck",
		"auxadc_ck",
		"i2c3_ck",
		"i2c2_ck",
		"i2c1_ck",
		"i2c0_ck",
		"bitif_ck",
		"uart3_ck",
		"uart2_ck",
		"uart1_ck",
		"uart0_ck",
		"nli_ck",
		"msdc30_2_ck",
		"msdc30_1_ck",
		"msdc30_0_ck",
		"ap_dma_ck",
		"usb1_ck",
		"usb0_ck",
		"pwm_ck",
		"pwm7_ck",
		"pwm6_ck",
		"pwm5_ck",
		"pwm4_ck",
		"pwm3_ck",
		"pwm2_ck",
		"pwm1_ck",
		"therm_ck",
		"nfi_ck",
		"nfi_pad_ck",
		"nfi_ecc_ck",
		"gcpu_ck",
		/* MM */
		"mm_smi_comm",
		"mm_smi_larb0",
		"mm_cmdq",
		"mm_mutex",
		"mm_disp_color",
		"mm_disp_bls",
		"mm_disp_wdma",
		"mm_disp_rdma",
		"mm_disp_ovl",
		"mm_mdp_tdshp",
		"mm_mdp_wrot",
		"mm_mdp_wdma",
		"mm_mdp_rsz1",
		"mm_mdp_rsz0",
		"mm_mdp_rdma",
		"mm_mdp_bls_26m",
		"mm_cam_mdp",
		"mm_fake_eng",
		"mm_mutex_32k",
		"mm_disp_rdma1",
		"mm_disp_ufoe",
		"mm_dsi_eng",
		"mm_dsi_dig",
		"mm_dpi_digl",
		"mm_dpi_eng",
		"mm_dpi1_digl",
		"mm_dpi1_eng",
		"mm_tve_output",
		"mm_tve_input",
		"mm_hdmi_pixel",
		"mm_hdmi_pll",
		"mm_hdmi_audio",
		"mm_hdmi_spdif",
		"mm_lvds_pixel",
		"mm_lvds_cts",
		/* IMG */
		"larb2_smi",
		"cam_smi",
		"cam_cam",
		"img_sen_tg",
		"img_sen_cam",
		"img_venc_jpgenc",
		/* VDEC */
		"vdec_cken",
		"vdec_larb_cken",
		/* MFG */
		"mfg_bg3d",
		/* end */
		NULL
	};

	return clks;
}

/*
 * clkdbg pwr_status
 */

static const char * const *get_pwr_names(void)
{
	static const char * const pwr_names[] = {
		[0]  = "MD",
		[1]  = "CONN",
		[2]  = "DDRPHY",
		[3]  = "DISP",
		[4]  = "MFG",
		[5]  = "ISP",
		[6]  = "INFRA",
		[7]  = "VDEC",
		[8]  = "CPU",
		[9]  = "FC3",
		[10] = "FC2",
		[11] = "FC1",
		[12] = "FC0",
		[13] = "MCU",
		[14] = "",
		[15] = "",
		[16] = "",
		[17] = "",
		[18] = "",
		[19] = "",
		[20] = "",
		[21] = "",
		[22] = "",
		[23] = "",
		[24] = "",
		[25] = "",
		[26] = "",
		[27] = "",
		[28] = "",
		[29] = "",
		[30] = "",
		[31] = "",
	};

	return pwr_names;
}

/*
 * init functions
 */

static struct clkdbg_ops clkdbg_mt8127_ops = {
	.get_all_fmeter_clks = get_all_fmeter_clks,
	.prepare_fmeter = prepare_fmeter,
	.unprepare_fmeter = unprepare_fmeter,
	.fmeter_freq = fmeter_freq_op,
	.get_all_regnames = get_all_regnames,
	.get_all_clk_names = get_all_clk_names,
	.get_pwr_names = get_pwr_names,
};

static int __init clkdbg_mt8127_init(void)
{
	if (!of_machine_is_compatible("mediatek,mt8127"))
		return -ENODEV;

	init_regbase();

	set_clkdbg_ops(&clkdbg_mt8127_ops);

#if ALL_CLK_ON
	prepare_enable_provider("topckgen");
	reg_pdrv("all");
	prepare_enable_provider("all");
#endif

#if DUMP_INIT_STATE
	print_regs();
	print_fmeter_all();
#endif /* DUMP_INIT_STATE */

	return 0;
}
device_initcall(clkdbg_mt8127_init);
