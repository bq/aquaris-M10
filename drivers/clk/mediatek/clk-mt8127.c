/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Shunli Wang <shunli.wang@mediatek.com>
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

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/mfd/syscon.h>

#include "clk-mtk.h"
#include "clk-gate.h"
#include "clk-cpumux.h"

#include <dt-bindings/clock/mt8127-clk.h>

static DEFINE_SPINLOCK(lock);

/* This root clock derived from:
 * 1. some PADs
 * 2. AD parts from PLLGP
 * 3. Reference clock in PLLGP
 */
static const struct mtk_fixed_factor root_clk_alias[] __initconst = {
	FACTOR(CLK_TOP_CLKPH_MCK_O, "clkph_mck_o", "clk_null", 1, 1),
	FACTOR(CLK_TOP_MEMPLL2, "mempll2", "clk_null", 1, 1),
	FACTOR(CLK_TOP_FPC, "fpc_ck", "clk_null", 1, 1),
	FACTOR(CLK_TOP_HDMITX_CLKDIG, "hdmitx_clkdig", "clk_null", 1, 1),
	FACTOR(CLK_TOP_LVDSTX_CLKDIG, "lvdstx_clkdig", "clk_null", 1, 1),
	FACTOR(CLK_TOP_LVDSPLL_ETH, "lvdspll_eth", "clk_null", 1, 1),
	FACTOR(CLK_TOP_DPI, "dpi_ck", "clk_null", 1, 1),
	FACTOR(CLK_TOP_VENCPLL, "vencpll_ck", "clk_null", 1, 1),
};

static const struct mtk_fixed_factor top_fixed_divs[] __initconst = {
	FACTOR(CLK_TOP_SYSPLL_D2, "syspll_d2", "mainpll", 1, 2),
	FACTOR(CLK_TOP_SYSPLL_D3, "syspll_d3", "mainpll", 1, 3),
	FACTOR(CLK_TOP_SYSPLL_D5, "syspll_d5", "mainpll", 1, 5),
	FACTOR(CLK_TOP_SYSPLL_D7, "6syspll_d7", "mainpll", 1, 7),
	FACTOR(CLK_TOP_SYSPLL1_D2, "syspll1_d2", "syspll_d2", 1, 2),
	FACTOR(CLK_TOP_SYSPLL1_D4, "syspll1_d4", "syspll_d2", 1, 4),
	FACTOR(CLK_TOP_SYSPLL1_D8, "syspll1_d8", "syspll_d2", 1, 8),
	FACTOR(CLK_TOP_SYSPLL1_D16, "syspll1_d16", "syspll_d2", 1, 16),
	FACTOR(CLK_TOP_SYSPLL2_D2, "syspll2_d2", "syspll_d3", 1, 2),
	FACTOR(CLK_TOP_SYSPLL2_D4, "syspll2_d4", "syspll_d3", 1, 4),
	FACTOR(CLK_TOP_SYSPLL2_D8, "syspll2_d8", "syspll_d3", 1, 8),
	FACTOR(CLK_TOP_SYSPLL3_D2, "syspll3_d2", "syspll_d5", 1, 2),
	FACTOR(CLK_TOP_SYSPLL3_D4, "syspll3_d4", "syspll_d5", 1, 4),
	FACTOR(CLK_TOP_SYSPLL4_D2, "syspll4_d2", "syspll_d7", 1, 2),
	FACTOR(CLK_TOP_SYSPLL4_D4, "syspll4_d4", "syspll_d7", 1, 4),

	FACTOR(CLK_TOP_UNIVPLL_D2, "univpll_d2", "univpll", 1, 1),
	FACTOR(CLK_TOP_UNIVPLL_D3, "univpll_d3", "univpll", 1, 1),
	FACTOR(CLK_TOP_UNIVPLL_D5, "univpll_d5", "univpll", 1, 1),
	FACTOR(CLK_TOP_UNIVPLL_D26, "univpll_d26", "univpll", 1, 26),
	FACTOR(CLK_TOP_UNIVPLL1_D2, "univpll1_d2", "univpll_d2", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL1_D4, "univpll1_d4", "univpll_d2", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL1_D8, "univpll1_d8", "univpll_d2", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL2_D2, "univpll2_d2", "univpll_d3", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL2_D4, "univpll2_d4", "univpll_d3", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL2_D8, "univpll2_d8", "univpll_d3", 1, 8),
	FACTOR(CLK_TOP_UNIVPLL3_D2, "univpll3_d2", "univpll_d5", 1, 2),
	FACTOR(CLK_TOP_UNIVPLL3_D4, "univpll3_d4", "univpll_d5", 1, 4),
	FACTOR(CLK_TOP_UNIVPLL3_D8, "univpll3_d8", "univpll_d5", 1, 8),

	FACTOR(CLK_TOP_MSDCPLL, "msdcpll_ck", "msdcpll", 1, 1),
	FACTOR(CLK_TOP_MSDCPLL_D2, "msdcpll_d2", "msdcpll", 1, 2),

	FACTOR(CLK_TOP_MMPLL, "mmpll_ck", "mmpll", 1, 1),
	FACTOR(CLK_TOP_MMPLL_D2, "mmpll_d2", "mmpll", 1, 2),

	FACTOR(CLK_TOP_DMPLL, "dmpll_ck", "clk_mck_o", 1, 1),
	FACTOR(CLK_TOP_DMPLL_D2, "dmpll_d2", "clk_mck_o", 1, 2),
	FACTOR(CLK_TOP_DMPLL_D4, "dmpll_d4", "clk_mck_o", 1, 4),
	FACTOR(CLK_TOP_DMPLL_X2, "dmpll_x2", "dmpll_ck", 1, 1),

	FACTOR(CLK_TOP_TVDPLL, "tvdpll_ck", "tvdpll", 1, 1),
	FACTOR(CLK_TOP_TVDPLL_D2, "tvdpll_d2", "tvdpll", 1, 2),
	FACTOR(CLK_TOP_TVDPLL_D4, "tvdpll_d4", "tvdpll", 1, 4),

	FACTOR(CLK_TOP_MIPIPLL, "mipipll_ck", "dpi_ck", 1, 1),
	FACTOR(CLK_TOP_MIPIPLL_D2, "mipipll_d2", "dpi_ck", 1, 2),
	FACTOR(CLK_TOP_MIPIPLL_D4, "mipipll_d4", "dpi_ck", 1, 4),

	FACTOR(CLK_TOP_HDMIPLL, "hdmipll_ck", "hdmitx_clkdig", 1, 1),
	FACTOR(CLK_TOP_HDMIPLL_D2, "hdmipll_d2", "hdmitx_clkdig", 1, 2),
	FACTOR(CLK_TOP_HDMIPLL_D3, "hdmipll_d3", "hdmitx_clkdig", 1, 3),

	FACTOR(CLK_TOP_LVDSPLL, "lvdspll_ck", "lvdspll", 1, 1),
	FACTOR(CLK_TOP_LVDSPLL_D2, "lvdspll_d2", "lvdspll", 1, 2),
	FACTOR(CLK_TOP_LVDSPLL_D4, "lvdspll_d4", "lvdspll", 1, 4),
	FACTOR(CLK_TOP_LVDSPLL_D8, "lvdspll_d8", "lvdspll", 1, 8),

	FACTOR(CLK_TOP_AUDPLL, "audpll_ck", "audpll", 1, 1),
	FACTOR(CLK_TOP_AUDPLL_D4, "audpll_d4", "audpll", 1, 4),
	FACTOR(CLK_TOP_AUDPLL_D8, "audpll_d8", "audpll", 1, 8),
	FACTOR(CLK_TOP_AUDPLL_D16, "audpll_d16", "audpll", 1, 16),
	FACTOR(CLK_TOP_AUDPLL_D24, "audpll_d24", "audpll", 1, 24),

	FACTOR(CLK_TOP_32K_INTERNAL, "32k_internal", "clk26m", 1, 793),
	FACTOR(CLK_TOP_32K_EXTERNAL, "32k_external", "rtc32k", 1, 1),
};

static const struct mtk_fixed_factor infra_fixed_divs[] __initconst = {
	FACTOR(CLK_INFRA_CLK_13M, "clk13m", "clk26m", 1, 2),
};

static const char * const axi_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"syspll_d5",
	"syspll1_d4",
	"univpll_d5",
	"univpll2_d2",
	"dmpll_ck",
	"dmpll_d2"
};

static const char * const mem_parents[] __initconst = {
	"clk26m",
	"dmpll_ck"
};

static const char * const ddrphycfg_parents[] __initconst = {
	"clk26m",
	"syspll1_d8"
};

static const char * const mm_parents[] __initconst = {
	"clk26m",
	"vencpll_ck",
	"syspll1_d2",
	"syspll_d5",
	"syspll1_d4",
	"univpll_d5",
	"univpll2_d2",
	"dmpll_ck"
};

static const char * const pwm_parents[] __initconst = {
	"clk26m",
	"univpll2_d4",
	"univpll3_d2",
	"univpll1_d4",
};

static const char * const vdec_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"syspll_d5",
	"syspll1_d4",
	"univpll_d5",
	"univpll2_d2",
	"univpll2_d4",
	"msdcpll_d2",
	"mmpll_d2"
};

static const char * const mfg_parents[] __initconst = {
	"clk26m",
	"mmpll_ck",
	"dmpll_x2_ck",
	"msdcpll_ck",
	"clk26m",
	"syspll_d3",
	"univpll_d3",
	"univpll1_d2"
};

static const char * const camtg_parents[] __initconst = {
	"clk26m",
	"univpll_d26",
	"univpll2_d2",
	"syspll3_d2",
	"syspll3_d4",
	"msdcpll_d2",
	"mmpll_d2"
};

static const char * const uart_parents[] __initconst = {
	"clk26m",
	"univpll2_d8"
};

static const char * const spi_parents[] __initconst = {
	"clk26m",
	"syspll3_d2",
	"syspll4_d2",
	"univpll2_d4",
	"univpll1_d8"
};

static const char * const usb20_parents[] __initconst = {
	"clk26m",
	"univpll1_d8",
	"univpll3_d4"
};

static const char * const msdc30_parents[] __initconst = {
	"clk26m",
	"msdcpll_d2",
	"syspll2_d2",
	"syspll1_d4",
	"univpll1_d4",
	"univpll2_d4"
};

static const char * const audio_parents[] __initconst = {
	"clk26m",
	"syspll1_d16"
};

static const char * const aud_intbus_parents[] __initconst = {
	"clk26m",
	"syspll1_d4",
	"syspll3_d2",
	"syspll4_d2",
	"univpll3_d2",
	"univpll2_d4"
};

static const char * const pmicspi_parents[] __initconst = {
	"clk26m",
	"syspll1_d8",
	"syspll2_d4",
	"syspll4_d2",
	"syspll3_d4",
	"syspll2_d8",
	"syspll1_d16",
	"univpll3_d4",
	"univpll_d26",
	"dmpll_d2",
	"dmpll_d4"
};

static const char * const scp_parents[] __initconst = {
	"clk26m",
	"syspll1_d8",
	"dmpll_d2",
	"dmpll_d4"
};

static const char * const dpi0_parents[] __initconst = {
	"clk26m",
	"mipipll_ck",
	"mipipll_d2",
	"mipipll_d4",
	"lvdspll_ck",
	"lvdspll_d2",
	"lvdspll_d4",
	"lvdspll_d8"
};

static const char * const dpi1_parents[] __initconst = {
	"clk26m",
	"tvdpll_ck",
	"tvdpll_d2",
	"tvdpll_d4"
};

static const char * const tve_parents[] __initconst = {
	"clk26m",
	"mipipll_ck",
	"mipipll_d2",
	"mipipll_d4",
	"clk26m",
	"tvdpll_ck",
	"tvdpll_d2",
	"tvdpll_d4"
};

static const char * const hdmi_parents[] __initconst = {
	"clk26m",
	"hdmipll",
	"hdmipll_d2",
	"hdmipll_d3"
};

static const char * const apll_parents[] __initconst = {
	"clk26m",
	"audpll_ck",
	"audpll_d4",
	"audpll_d8",
	"audpll_d16",
	"audpll_d24",
	"clk26m",
	"clk26m"
};

static const char * const dpilvds_parents[] __initconst = {
	"clk26m",
	"lvdspll_ck",
	"lvdspll_d2",
	"lvdspll_d4",
	"lvdspll_d8",
	"fpc_ck",
	"clk26m",
	"clk26m"
};

static const char * const rtc_parents[] __initconst = {
	"32k_internal",
	"32k_external",
	"clk26m",
	"univpll3_d8"
};

static const char * const nfi2x_parents[] __initconst = {
	"clk26m",
	"syspll2_d2",
	"syspll_d7",
	"univpll3_d2",
	"syspll2_d4",
	"univpll3_d4",
	"syspll4_d4",
	"clk26m"
};

static const char * const eth_50m_parents[] __initconst = {
	"clk26m",
	"syspll3_d4",
	"univpll2_d8",
	"lvdspll_eth",
	"univpll_d26",
	"syspll2_d8",
	"syspll4_d4",
	"univpll3_d8"
};

static const char * const cpu_parents[] __initconst = {
	"clk26m",
	"armpll",
	"mainpll",
	"mmpll"
};

static const struct mtk_composite top_muxes[] __initconst = {
	MUX_GATE(CLK_TOP_AXI_SEL, "axi_sel", axi_parents,
				0x0040, 0, 3, 7),
	MUX_GATE(CLK_TOP_MEM_SEL, "mem_sel", mem_parents, 0x0040, 8, 1, 15),
	MUX_GATE(CLK_TOP_DDRPHYCFG_SEL, "ddrphycfg_sel", ddrphycfg_parents,
				0x0040, 16, 1, 23),
	MUX_GATE(CLK_TOP_MM_SEL, "mm_sel", mm_parents, 0x0040, 24, 3, 31),

	MUX_GATE(CLK_TOP_PWM_SEL, "pwm_sel", pwm_parents, 0x0050, 0, 2, 7),
	MUX_GATE(CLK_TOP_VDEC_SEL, "vdec_sel", vdec_parents,
				0x0050, 8, 4, 15),
	MUX_GATE(CLK_TOP_MFG_SEL, "mfg_sel", mfg_parents, 0x0050, 16, 3, 23),
	MUX_GATE(CLK_TOP_CAMTG_SEL, "camtg_sel", camtg_parents,
				0x0050, 24, 3, 31),

	MUX_GATE(CLK_TOP_UART_SEL, "uart_sel", uart_parents, 0x0060, 0, 1, 7),
	MUX_GATE(CLK_TOP_SPI0_SEL, "spi0_sel", spi_parents, 0x0060, 8, 3, 15),
	MUX_GATE(CLK_TOP_USB20_SEL, "usb20_sel", usb20_parents,
				0x0060, 16, 2, 23),
	MUX_GATE(CLK_TOP_MSDC30_0_SEL, "msdc30_0_sel", msdc30_parents,
				0x0060, 24, 3, 31),

	MUX_GATE(CLK_TOP_MSDC30_1_SEL, "msdc30_1_sel", msdc30_parents,
				0x0070, 0, 3, 7),
	MUX_GATE(CLK_TOP_MSDC30_2_SEL, "msdc30_2_sel", msdc30_parents,
				0x0070, 8, 3, 15),
	MUX_GATE(CLK_TOP_AUDIO_SEL, "audio_sel", audio_parents,
				0x0070, 16, 1, 23),
	MUX_GATE(CLK_TOP_AUDINTBUS_SEL, "aud_intbus_sel", aud_intbus_parents,
				0x0070, 24, 3, 31),

	MUX_GATE(CLK_TOP_PMICSPI_SEL, "pmicspi_sel", pmicspi_parents,
				0x0080, 0, 4, 7),
	MUX_GATE(CLK_TOP_SCP_SEL, "scp_sel", scp_parents, 0x0080, 8, 2, 15),
	MUX_GATE(CLK_TOP_DPI0_SEL, "dpi0_sel", dpi0_parents, 0x0080, 16, 3, 23),
	MUX_GATE(CLK_TOP_DPI1_SEL, "dpi1_sel", dpi1_parents, 0x0080, 24, 2, 31),

	MUX_GATE(CLK_TOP_TVE_SEL, "tve_sel", tve_parents, 0x0090, 0, 3, 7),
	MUX_GATE(CLK_TOP_HDMI_SEL, "hdmi_sel", hdmi_parents, 0x0090, 8, 2, 15),
	MUX_GATE(CLK_TOP_APLL_SEL, "apll_sel", apll_parents, 0x0090, 16, 3, 23),
	MUX_GATE(CLK_TOP_DPILVDS_SEL, "dpilvds_sel", dpilvds_parents,
				0x0090, 24, 3, 31),

	MUX_GATE(CLK_TOP_RTC_SEL, "rtc_sel", rtc_parents, 0x00A0, 0, 2, 7),
	MUX_GATE(CLK_TOP_NFI2X_SEL, "nfi2x_sel", nfi2x_parents,
				0x00A0, 8, 3, 15),
	MUX_GATE(CLK_TOP_ETH_50M_SEL, "eth_50m_sel", eth_50m_parents,
				0x00A0, 16, 3, 23),
};

static void __init mtk_topckgen_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(CLK_TOP_NR);

	mtk_clk_register_factors(root_clk_alias, ARRAY_SIZE(root_clk_alias),
				clk_data);
	mtk_clk_register_factors(top_fixed_divs, ARRAY_SIZE(top_fixed_divs),
				clk_data);
	mtk_clk_register_composites(top_muxes, ARRAY_SIZE(top_muxes),
				base, &lock, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
				      __func__, r);
}
CLK_OF_DECLARE(mtk_topckgen, "mediatek,mt8127-topckgen", mtk_topckgen_init);

static const struct mtk_gate_regs infra_cg_regs __initconst = {
	.set_ofs = 0x0040,
	.clr_ofs = 0x0044,
	.sta_ofs = 0x0048,
};

#define GATE_ICG(_id, _name, _parent, _shift) {	\
	.id = _id,					\
	.name = _name,					\
	.parent_name = _parent,				\
	.regs = &infra_cg_regs,				\
	.shift = _shift,				\
	.ops = &mtk_clk_gate_ops_setclr,		\
}

static const struct mtk_gate infra_clks[] __initconst = {
	GATE_ICG(CLK_INFRA_PMICWRAP, "pmicwrap_ck", "axi_sel", 23),
	GATE_ICG(CLK_INFRA_PMICSPI, "pmicspi_ck", "pmicspi_sel", 22),
	GATE_ICG(CLK_INFRA_IRRX, "irrx_ck", "axi_sel", 19),
	GATE_ICG(CLK_INFRA_CEC, "cec_ck", "rtc_sel", 18),
	GATE_ICG(CLK_INFRA_KP, "kp_ck", "axi_sel", 16),
	GATE_ICG(CLK_INFRA_CPUM, "cpum_ck", "cpum_tck_in", 15),
	GATE_ICG(CLK_INFRA_TRNG, "trng_ck", "axi_sel", 13),
	GATE_ICG(CLK_INFRA_CONNMCU, "connsys_bus", "conn2ap_hclk", 12),
	GATE_ICG(CLK_INFRA_M4U, "m4u_ck", "mem_sel", 8),
	GATE_ICG(CLK_INFRA_L2C_SRAM, "l2c_sram_ck", "mm_sel", 7),
	GATE_ICG(CLK_INFRA_EFUSE, "efuse_ck", "clk26m", 6),
	GATE_ICG(CLK_INFRA_AUDIO, "audio_ck", "audio_sel", 5),
	GATE_ICG(CLK_INFRA_SMI, "smi_ck", "mm_sel", 1),
	GATE_ICG(CLK_INFRA_DBG, "dbgclk", "axi_sel", 0),
};

static const struct mtk_composite cpu_muxes[] __initconst = {
	MUX(CLK_INFRA_CKMUXSEL, "ckmux_sel", cpu_parents, 0x0000, 2, 2),
};

struct mtk_clk_divider {
	int id;
	const char *name;
	const char *parent_name;
	unsigned long flags;

	uint32_t div_reg;
	unsigned char div_shift;
	unsigned char div_width;
	unsigned char clk_divider_flags;
	const struct clk_div_table *clk_div_table;
};

#define DIV_TBL(_id, _name, _parent, _reg, _shift, _width,	\
		_table) {					\
		.id = _id,					\
		.name = _name,					\
		.parent_name = _parent,				\
		.flags = CLK_SET_RATE_PARENT,			\
		.div_reg = _reg,				\
		.div_shift = _shift,				\
		.div_width = _width,				\
		.clk_div_table = _table,			\
}

#define TBL_VAL(_val, _div)	{				\
			.val = _val,				\
			.div = _div,				\
		}						\

static const struct clk_div_table infra_clkdiv1_tbl[] __initconst = {
	[0] = TBL_VAL(0x00, 1),
	[1] = TBL_VAL(0x0A, 2),
	[2] = TBL_VAL(0x0B, 4),
	[3] = TBL_VAL(0x10, 1),
	[4] = TBL_VAL(0x14, 5),
	[5] = TBL_VAL(0x18, 1),
	[6] = TBL_VAL(0x1B, 2),
	[7] = TBL_VAL(0x1C, 3),
	[8] = TBL_VAL(0x1D, 6),
};

static const struct mtk_clk_divider infra_tbl_divs[] __initconst = {
	DIV_TBL(CLK_INFRA_CKDIV1, "ckdiv1", "ckmux_sel",
				0x0008, 0, 5, infra_clkdiv1_tbl),
};

static void mtk_clk_register_divider_tables(const struct mtk_clk_divider *mcds,
			int num, void __iomem *base, spinlock_t *lock,
			struct clk_onecell_data *clk_data)
{
	struct clk *clk;
	int i;

	for (i = 0; i <  num; i++) {
		const struct mtk_clk_divider *mcd = &mcds[i];

		clk = clk_register_divider_table(NULL, mcd->name,
			mcd->parent_name,
			mcd->flags, base +  mcd->div_reg,
			mcd->div_shift,
			mcd->div_width, mcd->clk_divider_flags,
			mcd->clk_div_table,
			lock);

		if (IS_ERR(clk)) {
			pr_err("Failed to register clk %s: %ld\n",
				mcd->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[mcd->id] = clk;
	}
}

static void mtk_clk_register_divtbls_by_node(struct device_node *node,
			const struct mtk_clk_divider *mcds, int num,
			spinlock_t *lock, struct clk_onecell_data *clk_data)
{
	void __iomem *base = NULL;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	mtk_clk_register_divider_tables(mcds, num, base, lock, clk_data);
}

static void __init mtk_infrasys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_INFRA_NR);

	mtk_clk_register_gates(node, infra_clks, ARRAY_SIZE(infra_clks),
				clk_data);
	mtk_clk_register_factors(infra_fixed_divs, ARRAY_SIZE(infra_fixed_divs),
				clk_data);

	mtk_clk_register_cpumuxes(node, cpu_muxes, ARRAY_SIZE(cpu_muxes),
						clk_data);

	mtk_clk_register_divtbls_by_node(node, infra_tbl_divs,
				ARRAY_SIZE(infra_tbl_divs),
				&lock, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
				      __func__, r);

	mtk_register_reset_controller(node, 2, 0x30);
}
CLK_OF_DECLARE(mtk_infrasys, "mediatek,mt8127-infracfg", mtk_infrasys_init);

static const struct mtk_gate_regs peri0_cg_regs __initconst = {
	.set_ofs = 0x0008,
	.clr_ofs = 0x0010,
	.sta_ofs = 0x0018,
};

static const struct mtk_gate_regs peri1_cg_regs __initconst = {
	.set_ofs = 0x000c,
	.clr_ofs = 0x0014,
	.sta_ofs = 0x001c,
};

#define GATE_PERI0(_id, _name, _parent, _shift) {	\
	.id = _id,					\
	.name = _name,					\
	.parent_name = _parent,				\
	.regs = &peri0_cg_regs,				\
	.shift = _shift,				\
	.ops = &mtk_clk_gate_ops_setclr,		\
}

#define GATE_PERI1(_id, _name, _parent, _shift) {	\
	.id = _id,					\
	.name = _name,					\
	.parent_name = _parent,				\
	.regs = &peri1_cg_regs,				\
	.shift = _shift,				\
	.ops = &mtk_clk_gate_ops_setclr,		\
}

static const struct mtk_gate peri_clks[] __initconst = {
	GATE_PERI0(CLK_PERI_USB_SLV, "usbslv_ck", "axi_sel", 31),
	GATE_PERI0(CLK_PERI_USB1_MCU, "usb1_mcu_ck", "axi_sel", 30),
	GATE_PERI0(CLK_PERI_USB0_MCU, "usb0_mcu_ck", "axi_sel", 29),
	GATE_PERI0(CLK_PERI_ETH, "eth_ck", "eth_50m_sel", 28),
	GATE_PERI0(CLK_PERI_SPI0, "spi0_ck", "spi0_sel", 27),
	GATE_PERI0(CLK_PERI_AUXADC, "auxadc_ck", "clk26m", 26),
	GATE_PERI0(CLK_PERI_I2C3, "i2c3_ck", "clk26m", 25),
	GATE_PERI0(CLK_PERI_I2C2, "i2c2_ck", "axi_sel", 24),
	GATE_PERI0(CLK_PERI_I2C1, "i2c1_ck", "axi_sel", 23),
	GATE_PERI0(CLK_PERI_I2C0, "i2c0_ck", "axi_sel", 22),
	GATE_PERI0(CLK_PERI_BTIF, "bitif_ck", "axi_sel", 21),
	GATE_PERI0(CLK_PERI_UART3, "uart3_ck", "uart_sel", 20),
	GATE_PERI0(CLK_PERI_UART2, "uart2_ck", "uart_sel", 19),
	GATE_PERI0(CLK_PERI_UART1, "uart1_ck", "uart_sel", 18),
	GATE_PERI0(CLK_PERI_UART0, "uart0_ck", "uart_sel", 17),
	GATE_PERI0(CLK_PERI_NLI, "nli_ck", "axi_sel", 16),
	GATE_PERI0(CLK_PERI_MSDC30_2, "msdc30_2_ck", "msdc30_2_sel", 15),
	GATE_PERI0(CLK_PERI_MSDC30_1, "msdc30_1_ck", "msdc30_1_sel", 14),
	GATE_PERI0(CLK_PERI_MSDC30_0, "msdc30_0_ck", "msdc30_0_sel", 13),
	GATE_PERI0(CLK_PERI_AP_DMA, "ap_dma_ck", "axi_sel", 12),
	GATE_PERI0(CLK_PERI_USB1, "usb1_ck", "usb20_sel", 11),
	GATE_PERI0(CLK_PERI_USB0, "usb0_ck", "usb20_sel", 10),
	GATE_PERI0(CLK_PERI_PWM, "pwm_ck", "axi_sel", 9),
	GATE_PERI0(CLK_PERI_PWM7, "pwm7_ck", "axi_sel", 8),
	GATE_PERI0(CLK_PERI_PWM6, "pwm6_ck", "axi_sel", 7),
	GATE_PERI0(CLK_PERI_PWM5, "pwm5_ck", "axi_sel", 6),
	GATE_PERI0(CLK_PERI_PWM4, "pwm4_ck", "axi_sel", 5),
	GATE_PERI0(CLK_PERI_PWM3, "pwm3_ck", "axi_sel", 4),
	GATE_PERI0(CLK_PERI_PWM2, "pwm2_ck", "axi_sel", 3),
	GATE_PERI0(CLK_PERI_PWM1, "pwm1_ck", "axi_sel", 2),
	GATE_PERI0(CLK_PERI_THERM, "therm_ck", "axi_sel", 1),
	GATE_PERI0(CLK_PERI_NFI, "nfi_ck", "axi_sel", 0),

	GATE_PERI1(CLK_PERI_NFI_PAD, "nfi_pad_ck", "nfi2x_sel", 2),
	GATE_PERI1(CLK_PERI_NFI_ECC, "nfi_ecc_ck", "nfi2x_sel", 1),
	GATE_PERI1(CLK_PERI_GCPU, "gcpu_ck", "axi_sel", 0),
};

static void __init mtk_pericfg_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_PERI_NR);

	mtk_clk_register_gates(node, peri_clks, ARRAY_SIZE(peri_clks),
				clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
				      __func__, r);

	mtk_register_reset_controller(node, 2, 0x0);
}
CLK_OF_DECLARE(mtk_pericfg, "mediatek,mt8127-pericfg", mtk_pericfg_init);

static const struct mtk_gate_regs mfg_cg_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_MFG(_id, _name, _parent, _shift) {	\
	.id = _id,					\
	.name = _name,					\
	.parent_name = _parent,				\
	.regs = &mfg_cg_regs,				\
	.shift = _shift,				\
	.ops = &mtk_clk_gate_ops_setclr,		\
}

static const struct mtk_gate mfg_clks[] __initconst = {
	GATE_MFG(CLK_MFG_BG3D, "mfg_bg3d", "mfg_sel", 0),
};

static void __init mtk_mfgsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_MFG_NR);

	mtk_clk_register_gates(node, mfg_clks, ARRAY_SIZE(mfg_clks),
				clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
				      __func__, r);
}
CLK_OF_DECLARE(mtk_mfgsys, "mediatek,mt8127-mfgsys", mtk_mfgsys_init);

static const struct mtk_gate_regs disp0_cg_regs __initconst = {
	.set_ofs = 0x0104,
	.clr_ofs = 0x0108,
	.sta_ofs = 0x0100,
};

static const struct mtk_gate_regs disp1_cg_regs __initconst = {
	.set_ofs = 0x0114,
	.clr_ofs = 0x0118,
	.sta_ofs = 0x0110,
};

#define GATE_DISP0(_id, _name, _parent, _shift) {	\
	.id = _id,					\
	.name = _name,					\
	.parent_name = _parent,				\
	.regs = &disp0_cg_regs,				\
	.shift = _shift,				\
	.ops = &mtk_clk_gate_ops_setclr,		\
}

#define GATE_DISP1(_id, _name, _parent, _shift) {	\
	.id = _id,					\
	.name = _name,					\
	.parent_name = _parent,				\
	.regs = &disp1_cg_regs,				\
	.shift = _shift,				\
	.ops = &mtk_clk_gate_ops_setclr,		\
}

static const struct mtk_gate mm_clks[] __initconst = {
	GATE_DISP0(CLK_MM_SMI_COMMON, "mm_smi_comm", "mm_sel", 0),
	GATE_DISP0(CLK_MM_SMI_LARB0, "mm_smi_larb0", "mm_sel", 1),
	GATE_DISP0(CLK_MM_CMDQ, "mm_cmdq", "mm_sel", 2),
	GATE_DISP0(CLK_MM_MUTEX	, "mm_mutex", "mm_sel", 3),
	GATE_DISP0(CLK_MM_DISP_COLOR, "mm_disp_color", "mm_sel", 4),
	GATE_DISP0(CLK_MM_DISP_BLS, "mm_disp_bls", "mm_sel", 5),
	GATE_DISP0(CLK_MM_DISP_WDMA, "mm_disp_wdma", "mm_sel", 6),
	GATE_DISP0(CLK_MM_DISP_RDMA, "mm_disp_rdma", "mm_sel", 7),
	GATE_DISP0(CLK_MM_DISP_OVL, "mm_disp_ovl", "mm_sel", 8),
	GATE_DISP0(CLK_MM_MDP_TDSHP, "mm_mdp_tdshp", "mm_sel", 9),
	GATE_DISP0(CLK_MM_MDP_WROT, "mm_mdp_wrot", "mm_sel", 10),
	GATE_DISP0(CLK_MM_MDP_WDMA, "mm_mdp_wdma", "mm_sel", 11),
	GATE_DISP0(CLK_MM_MDP_RSZ1, "mm_mdp_rsz1", "mm_sel", 12),
	GATE_DISP0(CLK_MM_MDP_RSZ0, "mm_mdp_rsz0", "mm_sel", 13),
	GATE_DISP0(CLK_MM_MDP_RDMA, "mm_mdp_rdma", "mm_sel", 14),
	GATE_DISP0(CLK_MM_MDP_BLS_26M, "mm_mdp_bls_26m", "clk26m", 15),
	GATE_DISP0(CLK_MM_CAM_MDP, "mm_cam_mdp", "mm_sel", 16),
	GATE_DISP0(CLK_MM_FAKE_ENG, "mm_fake_eng", "mm_sel", 17),
	GATE_DISP0(CLK_MM_MUTEX_32K, "mm_mutex_32k", "rtc_sel", 18),
	GATE_DISP0(CLK_MM_DISP_RDMA1, "mm_disp_rdma1", "mm_sel", 19),
	GATE_DISP0(CLK_MM_DISP_UFOE, "mm_disp_ufoe", "mm_sel", 20),

	GATE_DISP1(CLK_MM_DSI_ENGINE, "mm_dsi_eng", "mm_sel", 0),
	GATE_DISP1(CLK_MM_DSI_DIG, "mm_dsi_dig", "dsio_lntc_dsiclk", 1),
	GATE_DISP1(CLK_MM_DPI_DIGL, "mm_dpi_digl", "dpi0_sel", 2),
	GATE_DISP1(CLK_MM_DPI_ENGINE, "mm_dpi_eng", "mm_sel", 3),
	GATE_DISP1(CLK_MM_DPI1_DIGL, "mm_dpi1_digl", "dpi1_sel", 4),
	GATE_DISP1(CLK_MM_DPI1_ENGINE, "mm_dpi1_eng", "mm_sel", 5),
	GATE_DISP1(CLK_MM_TVE_OUTPUT, "mm_tve_output", "tve_sel", 6),
	GATE_DISP1(CLK_MM_TVE_INPUT, "mm_tve_input", "dpi0_sel", 7),
	GATE_DISP1(CLK_MM_HDMI_PIXEL, "mm_hdmi_pixel", "dpi1_sel", 8),
	GATE_DISP1(CLK_MM_HDMI_PLL, "mm_hdmi_pll", "hdmi_sel", 9),
	GATE_DISP1(CLK_MM_HDMI_AUDIO, "mm_hdmi_audio", "apll_sel", 10),
	GATE_DISP1(CLK_MM_HDMI_SPDIF, "mm_hdmi_spdif", "apll_sel", 11),
	GATE_DISP1(CLK_MM_LVDS_PIXEL, "mm_lvds_pixel", "dpilvds_sel", 12),
	GATE_DISP1(CLK_MM_LVDS_CTS, "mm_lvds_cts", "dpilvds_sel", 13),
};

static void __init mtk_mmsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_MM_NR);

	mtk_clk_register_gates(node, mm_clks, ARRAY_SIZE(mm_clks),
				clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
				      __func__, r);
}
CLK_OF_DECLARE(mtk_mmsys, "mediatek,mt8127-mmsys", mtk_mmsys_init);

static const struct mtk_gate_regs img_cg_regs __initconst = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

#define GATE_IMG(_id, _name, _parent, _shift) {	\
	.id = _id,					\
	.name = _name,					\
	.parent_name = _parent,				\
	.regs = &img_cg_regs,				\
	.shift = _shift,				\
	.ops = &mtk_clk_gate_ops_setclr,		\
}

static const struct mtk_gate img_clks[] __initconst = {
	GATE_IMG(CLK_IMG_LARB2_SMI, "larb2_smi", "mm_sel", 0),
	GATE_IMG(CLK_IMG_CAM_SMI, "cam_smi", "mm_sel", 5),
	GATE_IMG(CLK_IMG_CAM_CAM, "cam_cam", "mm_sel", 6),
	GATE_IMG(CLK_IMG_SEN_TG, "img_sen_tg", "mm_sel", 7),
	GATE_IMG(CLK_IMG_SEN_CAM, "img_sen_cam", "mm_sel", 8),
	GATE_IMG(CLK_IMG_VENC_JPGENC, "img_venc_jpgenc", "mm_sel", 9),
};

static void __init mtk_imgsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_IMG_NR);

	mtk_clk_register_gates(node, img_clks, ARRAY_SIZE(img_clks),
				clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
				      __func__, r);
}
CLK_OF_DECLARE(mtk_imgsys, "mediatek,mt8127-imgsys", mtk_imgsys_init);

static const struct mtk_gate_regs vdec0_cg_regs __initconst = {
	.set_ofs = 0x0000,
	.clr_ofs = 0x0004,
	.sta_ofs = 0x0000,
};

static const struct mtk_gate_regs vdec1_cg_regs __initconst = {
	.set_ofs = 0x0008,
	.clr_ofs = 0x000c,
	.sta_ofs = 0x0008,
};

#define GATE_VDEC0(_id, _name, _parent, _shift) {	\
	.id = _id,					\
	.name = _name,					\
	.parent_name = _parent,				\
	.regs = &vdec0_cg_regs,				\
	.shift = _shift,				\
	.ops = &mtk_clk_gate_ops_setclr_inv,		\
}

#define GATE_VDEC1(_id, _name, _parent, _shift) {	\
	.id = _id,					\
	.name = _name,					\
	.parent_name = _parent,				\
	.regs = &vdec1_cg_regs,				\
	.shift = _shift,				\
	.ops = &mtk_clk_gate_ops_setclr_inv,		\
}

static const struct mtk_gate vdec_clks[] __initconst = {
	GATE_VDEC0(CLK_VDEC_CKGEN, "vdec_cken", "vdec_sel", 0),
	GATE_VDEC1(CLK_VDEC_LARB, "vdec_larb_cken", "mm_sel", 0),
};

static void __init mtk_vdecsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_VDEC_NR);

	mtk_clk_register_gates(node, vdec_clks, ARRAY_SIZE(vdec_clks),
				clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
				      __func__, r);
}
CLK_OF_DECLARE(mtk_vdecsys, "mediatek,mt8127-vdecsys", mtk_vdecsys_init);

#define MT8127_PLL_FMAX		(2000 * MHZ)
#define CON0_MT8127_RST_BAR	BIT(27)

#define PLL_A(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits,	\
			_pd_reg, _pd_shift, _pcw_reg, _pcw_shift,	\
			_tuner_reg, _tuner_en_reg, _tuner_en_shift) {	\
		.id = _id,						\
		.name = _name,						\
		.reg = _reg,						\
		.pwr_reg = _pwr_reg,					\
		.en_mask = _en_mask,					\
		.flags = _flags,					\
		.rst_bar_mask = CON0_MT8127_RST_BAR,			\
		.fmax = MT8127_PLL_FMAX,				\
		.pcwbits = _pcwbits,					\
		.pd_reg = _pd_reg,					\
		.pd_shift = _pd_shift,					\
		.tuner_reg = _tuner_reg,				\
		.tuner_en_reg = _tuner_en_reg,				\
		.tuner_en_shift = _tuner_en_shift,			\
		.pcw_reg = _pcw_reg,					\
		.pcw_shift = _pcw_shift,				\
	}

#define PLL(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits,	\
			_pd_reg, _pd_shift, _pcw_reg, _pcw_shift)	\
		PLL_A(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits, \
			_pd_reg, _pd_shift, _pcw_reg, _pcw_shift,	\
			0, 0, 0)

static const struct mtk_pll_data apmixed_plls[] = {
	PLL(CLK_APMIXED_ARMPLL, "armpll", 0x200, 0x20c, 0x00000001,
				0, 21, 0x204, 24, 0x204, 0),
	PLL(CLK_APMIXED_MAINPLL, "mainpll", 0x210, 0x21c, 0x78000001,
				HAVE_RST_BAR, 21, 0x210, 4, 0x214, 0),
	PLL(CLK_APMIXED_UNIVPLL, "univpll", 0x220, 0x22c, 0xf3000001,
				HAVE_RST_BAR, 7, 0x220, 4, 0x224, 14),
	PLL(CLK_APMIXED_MMPLL, "mmpll", 0x230, 0x23c, 0x00000001,
				0, 21, 0x230, 4, 0x234, 0),
	PLL(CLK_APMIXED_MSDCPLL, "msdcpll", 0x240, 0x24c, 0x00000001,
				0, 21, 0x240, 4, 0x244, 0),
	PLL_A(CLK_APMIXED_AUDPLL, "audpll", 0x250, 0x25c, 0x00000001,
				0, 31, 0x250, 4, 0x254, 0, 0x408, 0x408, 31),
	PLL(CLK_APMIXED_TVDPLL, "tvdpll", 0x260, 0x26c, 0x00000001,
				0, 21, 0x260, 4, 0x264, 0),
	PLL(CLK_APMIXED_LVDSPLL, "lvdspll", 0x270, 0x27c, 0x00000001,
				0, 21, 0x270, 4, 0x274, 0),
};

static void __init mtk_apmixedsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;

	clk_data = mtk_alloc_clk_data(ARRAY_SIZE(apmixed_plls));
	if (!clk_data)
		return;

	mtk_clk_register_plls(node, apmixed_plls, ARRAY_SIZE(apmixed_plls),
				clk_data);
}
CLK_OF_DECLARE(mtk_apmixedsys, "mediatek,mt8127-apmixedsys",
			mtk_apmixedsys_init);
