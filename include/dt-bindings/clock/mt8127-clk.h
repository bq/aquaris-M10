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

#ifndef _DT_BINDINGS_CLK_MT8127_H
#define _DT_BINDINGS_CLK_MT8127_H

/* TOPCKGEN */

#define CLK_TOP_CLKPH_MCK_O			1
#define CLK_TOP_MEMPLL2				2
#define CLK_TOP_FPC				3
#define CLK_TOP_HDMITX_CLKDIG			4
#define CLK_TOP_LVDSTX_CLKDIG			5
#define CLK_TOP_LVDSPLL_ETH			6
#define CLK_TOP_DPI				7
#define CLK_TOP_VENCPLL				8
#define CLK_TOP_SYSPLL_D2			9
#define CLK_TOP_SYSPLL_D3			10
#define CLK_TOP_SYSPLL_D5			11
#define CLK_TOP_SYSPLL_D7			12
#define CLK_TOP_SYSPLL1_D2			13
#define CLK_TOP_SYSPLL1_D4			14
#define CLK_TOP_SYSPLL1_D8			15
#define CLK_TOP_SYSPLL1_D16			16
#define CLK_TOP_SYSPLL2_D2			17
#define CLK_TOP_SYSPLL2_D4			18
#define CLK_TOP_SYSPLL2_D8			19
#define CLK_TOP_SYSPLL3_D2			20
#define CLK_TOP_SYSPLL3_D4			21
#define CLK_TOP_SYSPLL4_D2			22
#define CLK_TOP_SYSPLL4_D4			23
#define CLK_TOP_UNIVPLL_D2			24
#define CLK_TOP_UNIVPLL_D3			25
#define CLK_TOP_UNIVPLL_D5			26
#define CLK_TOP_UNIVPLL_D26			27
#define CLK_TOP_UNIVPLL1_D2			28
#define CLK_TOP_UNIVPLL1_D4			29
#define CLK_TOP_UNIVPLL1_D8			30
#define CLK_TOP_UNIVPLL2_D2			31
#define CLK_TOP_UNIVPLL2_D4			32
#define CLK_TOP_UNIVPLL2_D8			33
#define CLK_TOP_UNIVPLL3_D2			34
#define CLK_TOP_UNIVPLL3_D4			35
#define CLK_TOP_UNIVPLL3_D8			36
#define CLK_TOP_MSDCPLL				37
#define CLK_TOP_MSDCPLL_D2			38
#define CLK_TOP_MMPLL				39
#define CLK_TOP_MMPLL_D2			40
#define CLK_TOP_DMPLL				41
#define CLK_TOP_DMPLL_D2			42
#define CLK_TOP_DMPLL_D4			43
#define CLK_TOP_DMPLL_X2			44
#define CLK_TOP_TVDPLL				45
#define CLK_TOP_TVDPLL_D2			46
#define CLK_TOP_TVDPLL_D4			47
#define CLK_TOP_MIPIPLL				48
#define CLK_TOP_MIPIPLL_D2			49
#define CLK_TOP_MIPIPLL_D4			50
#define CLK_TOP_HDMIPLL				51
#define CLK_TOP_HDMIPLL_D2			52
#define CLK_TOP_HDMIPLL_D3			53
#define CLK_TOP_LVDSPLL				54
#define CLK_TOP_LVDSPLL_D2			55
#define CLK_TOP_LVDSPLL_D4			56
#define CLK_TOP_LVDSPLL_D8			57
#define CLK_TOP_AUDPLL				58
#define CLK_TOP_AUDPLL_D4			59
#define CLK_TOP_AUDPLL_D8			60
#define CLK_TOP_AUDPLL_D16			61
#define CLK_TOP_AUDPLL_D24			62
#define CLK_TOP_32K_INTERNAL			63
#define CLK_TOP_32K_EXTERNAL			64

#define CLK_TOP_MM_SEL				65
#define CLK_TOP_DDRPHYCFG_SEL			66
#define CLK_TOP_MEM_SEL				67
#define CLK_TOP_AXI_SEL				68
#define CLK_TOP_CAMTG_SEL			69
#define CLK_TOP_MFG_SEL				70
#define CLK_TOP_VDEC_SEL			71
#define CLK_TOP_PWM_SEL				72
#define CLK_TOP_MSDC30_0_SEL			73
#define CLK_TOP_USB20_SEL			74
#define CLK_TOP_SPI0_SEL			75
#define CLK_TOP_UART_SEL			76
#define CLK_TOP_AUDINTBUS_SEL			77
#define CLK_TOP_AUDIO_SEL			78
#define CLK_TOP_MSDC30_2_SEL			79
#define CLK_TOP_MSDC30_1_SEL			80
#define CLK_TOP_DPI1_SEL			81
#define CLK_TOP_DPI0_SEL			82
#define CLK_TOP_SCP_SEL				83
#define CLK_TOP_PMICSPI_SEL			84
#define CLK_TOP_DPILVDS_SEL			85
#define CLK_TOP_APLL_SEL			86
#define CLK_TOP_HDMI_SEL			87
#define CLK_TOP_TVE_SEL				88
#define CLK_TOP_ETH_50M_SEL			89
#define CLK_TOP_NFI2X_SEL			90
#define CLK_TOP_RTC_SEL				91
#define CLK_TOP_NR				92

/* APMIXEDSYS */

#define CLK_APMIXED_ARMPLL			1
#define CLK_APMIXED_MAINPLL			2
#define CLK_APMIXED_UNIVPLL			3
#define CLK_APMIXED_MMPLL			4
#define CLK_APMIXED_MSDCPLL			5
#define CLK_APMIXED_AUDPLL			6
#define CLK_APMIXED_TVDPLL			7
#define CLK_APMIXED_LVDSPLL			8
#define CLK_APMIXED_NR				9

/* INFRACFG */

#define CLK_INFRA_DBG				1
#define CLK_INFRA_SMI				2
#define CLK_INFRA_AUDIO				3
#define CLK_INFRA_EFUSE				4
#define CLK_INFRA_L2C_SRAM			5
#define CLK_INFRA_M4U				6
#define CLK_INFRA_CONNMCU			7
#define CLK_INFRA_TRNG				8
#define CLK_INFRA_CPUM				9
#define CLK_INFRA_KP				10
#define CLK_INFRA_CEC				11
#define CLK_INFRA_IRRX				12
#define CLK_INFRA_PMICSPI			13
#define CLK_INFRA_PMICWRAP			14
#define CLK_INFRA_CLK_13M			15
#define CLK_INFRA_CKMUXSEL			16
#define CLK_INFRA_CKDIV1			17
#define CLK_INFRA_NR				18

/* PERICFG */

#define CLK_PERI_NFI				1
#define CLK_PERI_THERM				2
#define CLK_PERI_PWM1				3
#define CLK_PERI_PWM2				4
#define CLK_PERI_PWM3				5
#define CLK_PERI_PWM4				6
#define CLK_PERI_PWM5				7
#define CLK_PERI_PWM6				8
#define CLK_PERI_PWM7				9
#define CLK_PERI_PWM				10
#define CLK_PERI_USB0				11
#define CLK_PERI_USB1				12
#define CLK_PERI_AP_DMA				13
#define CLK_PERI_MSDC30_0			14
#define CLK_PERI_MSDC30_1			15
#define CLK_PERI_MSDC30_2			16
#define CLK_PERI_NLI				17
#define CLK_PERI_UART0				18
#define CLK_PERI_UART1				19
#define CLK_PERI_UART2				20
#define CLK_PERI_UART3				21
#define CLK_PERI_BTIF				22
#define CLK_PERI_I2C0				23
#define CLK_PERI_I2C1				24
#define CLK_PERI_I2C2				25
#define CLK_PERI_I2C3				26
#define CLK_PERI_AUXADC				27
#define CLK_PERI_SPI0				28
#define CLK_PERI_ETH				29
#define CLK_PERI_USB0_MCU			30
#define CLK_PERI_USB1_MCU			31
#define CLK_PERI_USB_SLV			32

#define CLK_PERI_GCPU				33
#define CLK_PERI_NFI_ECC			34
#define CLK_PERI_NFI_PAD			35
#define CLK_PERI_NR				36

/* MFGSYS */

#define CLK_MFG_BG3D				1
#define CLK_MFG_NR				2

/* MMSYS */

#define CLK_MM_SMI_COMMON			1
#define CLK_MM_SMI_LARB0			2
#define CLK_MM_CMDQ				3
#define CLK_MM_MUTEX				4
#define CLK_MM_DISP_COLOR			5
#define CLK_MM_DISP_BLS				6
#define CLK_MM_DISP_WDMA			7
#define CLK_MM_DISP_RDMA			8
#define CLK_MM_DISP_OVL				9
#define CLK_MM_MDP_TDSHP			10
#define CLK_MM_MDP_WROT				11
#define CLK_MM_MDP_WDMA				12
#define CLK_MM_MDP_RSZ1				13
#define CLK_MM_MDP_RSZ0				14
#define CLK_MM_MDP_RDMA				15
#define CLK_MM_MDP_BLS_26M			16
#define CLK_MM_CAM_MDP				17
#define CLK_MM_FAKE_ENG				18
#define CLK_MM_MUTEX_32K			19
#define CLK_MM_DISP_RDMA1			20
#define CLK_MM_DISP_UFOE			21

#define CLK_MM_DSI_ENGINE			22
#define CLK_MM_DSI_DIG				23
#define CLK_MM_DPI_DIGL				24
#define CLK_MM_DPI_ENGINE			25
#define CLK_MM_DPI1_DIGL			26
#define CLK_MM_DPI1_ENGINE			27
#define CLK_MM_TVE_OUTPUT			28
#define CLK_MM_TVE_INPUT			29
#define CLK_MM_HDMI_PIXEL			30
#define CLK_MM_HDMI_PLL				31
#define CLK_MM_HDMI_AUDIO			32
#define CLK_MM_HDMI_SPDIF			33
#define CLK_MM_LVDS_PIXEL			34
#define CLK_MM_LVDS_CTS				35
#define CLK_MM_NR				36

/* IMGSYS */

#define CLK_IMG_LARB2_SMI			1
#define CLK_IMG_CAM_SMI				2
#define CLK_IMG_CAM_CAM				3
#define CLK_IMG_SEN_TG				4
#define CLK_IMG_SEN_CAM				5
#define CLK_IMG_VENC_JPGENC			6
#define CLK_IMG_NR				7

/* VDECSYS */

#define CLK_VDEC_CKGEN				1
#define CLK_VDEC_LARB				2
#define CLK_VDEC_NR				3

#endif /* _DT_BINDINGS_CLK_MT8127_H */
