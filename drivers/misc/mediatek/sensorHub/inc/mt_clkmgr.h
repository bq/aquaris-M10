#ifndef _MT_CLKMGR_H
#define _MT_CLKMGR_H

#include <linux/list.h>
#include "mt_typedefs.h"

#define CLKMGR_ALIAS		1

#define CONFIG_CLKMGR_STAT

#ifdef CONFIG_OF

extern void __iomem *clk_apmixed_base;
extern void __iomem *clk_cksys_base;
extern void __iomem *clk_infracfg_ao_base;
extern void __iomem *clk_audio_base;
extern void __iomem *clk_mfgcfg_base;
extern void __iomem *clk_mmsys_config_base;
extern void __iomem *clk_imgsys_base;
extern void __iomem *clk_vdec_gcon_base;
extern void __iomem *clk_venc_gcon_base;

#endif /* CONFIG_OF */

/* APMIXEDSYS Register */
#define AP_PLL_CON0             (clk_apmixed_base + 0x00)
#define AP_PLL_CON1             (clk_apmixed_base + 0x04)
#define AP_PLL_CON2             (clk_apmixed_base + 0x08)
#define AP_PLL_CON7             (clk_apmixed_base + 0x1C)

#define ARMPLL_CON0             (clk_apmixed_base + 0x210)
#define ARMPLL_CON1             (clk_apmixed_base + 0x214)
#define ARMPLL_CON2             (clk_apmixed_base + 0x218)
#define ARMPLL_PWR_CON0         (clk_apmixed_base + 0x21C)

#define ARMCA7PLL_CON0          ARMPLL_CON0
#define ARMCA7PLL_CON1          ARMPLL_CON1
#define ARMCA7PLL_CON2          ARMPLL_CON2
#define ARMCA7PLL_PWR_CON0      ARMPLL_PWR_CON0

#define MAINPLL_CON0            (clk_apmixed_base + 0x220)
#define MAINPLL_CON1            (clk_apmixed_base + 0x224)
#define MAINPLL_PWR_CON0        (clk_apmixed_base + 0x22C)

#define UNIVPLL_CON0            (clk_apmixed_base + 0x230)
#define UNIVPLL_CON1            (clk_apmixed_base + 0x234)
#define UNIVPLL_PWR_CON0        (clk_apmixed_base + 0x23C)

#define MMPLL_CON0              (clk_apmixed_base + 0x240)
#define MMPLL_CON1              (clk_apmixed_base + 0x244)
#define MMPLL_CON2              (clk_apmixed_base + 0x248)
#define MMPLL_PWR_CON0          (clk_apmixed_base + 0x24C)

#define MSDCPLL_CON0            (clk_apmixed_base + 0x250)
#define MSDCPLL_CON1            (clk_apmixed_base + 0x254)
#define MSDCPLL_PWR_CON0        (clk_apmixed_base + 0x25C)

#define VENCPLL_CON0            (clk_apmixed_base + 0x260)
#define VENCPLL_CON1            (clk_apmixed_base + 0x264)
#define VENCPLL_PWR_CON0        (clk_apmixed_base + 0x26C)

#define TVDPLL_CON0             (clk_apmixed_base + 0x270)
#define TVDPLL_CON1             (clk_apmixed_base + 0x274)
#define TVDPLL_PWR_CON0         (clk_apmixed_base + 0x27C)

#define MPLL_CON0               (clk_apmixed_base + 0x280)
#define MPLL_CON1               (clk_apmixed_base + 0x284)
#define MPLL_PWR_CON0           (clk_apmixed_base + 0x28C)

#define AUD1PLL_CON0            (clk_apmixed_base + 0x2A0)
#define AUD1PLL_CON1            (clk_apmixed_base + 0x2A4)
#define AUD1PLL_CON2            (clk_apmixed_base + 0x2A8)
#define AUD1PLL_CON3            (clk_apmixed_base + 0x2AC)
#define AUD1PLL_PWR_CON0        (clk_apmixed_base + 0x2B0)

#define AUD2PLL_CON0            (clk_apmixed_base + 0x2B4)
#define AUD2PLL_CON1            (clk_apmixed_base + 0x2B8)
#define AUD2PLL_CON2            (clk_apmixed_base + 0x2BC)
#define AUD2PLL_CON3            (clk_apmixed_base + 0x2C0)
#define AUD2PLL_PWR_CON0        (clk_apmixed_base + 0x2C4)

#define APLL1_CON0              AUD1PLL_CON0
#define APLL1_CON1              AUD1PLL_CON1
#define APLL1_CON2              AUD1PLL_CON2
#define APLL1_CON3              AUD1PLL_CON3
#define APLL1_PWR_CON0          AUD1PLL_PWR_CON0

#define APLL2_CON0              AUD2PLL_CON0
#define APLL2_CON1              AUD2PLL_CON1
#define APLL2_CON2              AUD2PLL_CON2
#define APLL2_CON3              AUD2PLL_CON3
#define APLL2_PWR_CON0          AUD2PLL_PWR_CON0

#define LVDSPLL_CON0            (clk_apmixed_base + 0x2C0)
#define LVDSPLL_CON1            (clk_apmixed_base + 0x2C4)
#define LVDSPLL_CON2            (clk_apmixed_base + 0x2C8)
#define LVDSPLL_PWR_CON0        (clk_apmixed_base + 0x2CC)

/* TOPCKGEN Register */
#define CLK_MODE                (clk_cksys_base + 0x000)
#define CLK_CFG_UPDATE          (clk_cksys_base + 0x004)
#define CLK_CFG_UPDATE_1        (clk_cksys_base + 0x008)
#define TST_SEL_0               (clk_cksys_base + 0x020)
#define TST_SEL_1               (clk_cksys_base + 0x024)
#define TST_SEL_2               (clk_cksys_base + 0x028)
#define CLK_CFG_0               (clk_cksys_base + 0x040)
#define CLK_CFG_1               (clk_cksys_base + 0x050)
#define CLK_CFG_2               (clk_cksys_base + 0x060)
#define CLK_CFG_3               (clk_cksys_base + 0x070)
#define CLK_CFG_4               (clk_cksys_base + 0x080)
#define CLK_CFG_5               (clk_cksys_base + 0x090)
#define CLK_CFG_6               (clk_cksys_base + 0x0A0)
#define CLK_CFG_7               (clk_cksys_base + 0x0B0)
#define CLK_CFG_8               (clk_cksys_base + 0x0C0)
#define CLK_MISC_CFG_0          (clk_cksys_base + 0x104)
#define CLK_DBG_CFG             (clk_cksys_base + 0x10C)
#define CLK_SCP_CFG_0           (clk_cksys_base + 0x200)
#define CLK_SCP_CFG_1           (clk_cksys_base + 0x204)
#define CLK26CALI_0             (clk_cksys_base + 0x220)
#define CLK26CALI_1             (clk_cksys_base + 0x224)
#define CKSTA_REG               (clk_cksys_base + 0x22C)

/* INFRASYS Register */
#define TOP_CKMUXSEL            (clk_infracfg_ao_base + 0x00)
#define TOP_CKDIV1              (clk_infracfg_ao_base + 0x08)

#define INFRA_PDN_SET0          (clk_infracfg_ao_base + 0x0080)
#define INFRA_PDN_CLR0          (clk_infracfg_ao_base + 0x0084)
#define INFRA_PDN_SET1          (clk_infracfg_ao_base + 0x0088)
#define INFRA_PDN_CLR1          (clk_infracfg_ao_base + 0x008C)
#define INFRA_PDN_STA0          (clk_infracfg_ao_base + 0x0090)
#define INFRA_PDN_STA1          (clk_infracfg_ao_base + 0x0094)

#define TOPAXI_PROT_EN          (clk_infracfg_ao_base + 0x0220)
#define TOPAXI_PROT_STA1        (clk_infracfg_ao_base + 0x0228)

/* Audio Register*/
#define AUDIO_TOP_CON0          (clk_audio_base + 0x0000)

/* MFGCFG Register*/
#define MFG_CG_CON              (clk_mfgcfg_base + 0)
#define MFG_CG_SET              (clk_mfgcfg_base + 4)
#define MFG_CG_CLR              (clk_mfgcfg_base + 8)

/* MMSYS Register*/
#define DISP_CG_CON0            (clk_mmsys_config_base + 0x100)
#define DISP_CG_SET0            (clk_mmsys_config_base + 0x104)
#define DISP_CG_CLR0            (clk_mmsys_config_base + 0x108)
#define DISP_CG_CON1            (clk_mmsys_config_base + 0x110)
#define DISP_CG_SET1            (clk_mmsys_config_base + 0x114)
#define DISP_CG_CLR1            (clk_mmsys_config_base + 0x118)

#define MMSYS_DUMMY             (clk_mmsys_config_base + 0x890)
#define	SMI_LARB_BWL_EN_REG     (clk_mmsys_config_base + 0x21050)

/* IMGSYS Register */
#define IMG_CG_CON              (clk_imgsys_base + 0x0000)
#define IMG_CG_SET              (clk_imgsys_base + 0x0004)
#define IMG_CG_CLR              (clk_imgsys_base + 0x0008)

/* VDEC Register */
#define VDEC_CKEN_SET           (clk_vdec_gcon_base + 0x0000)
#define VDEC_CKEN_CLR           (clk_vdec_gcon_base + 0x0004)
#define LARB_CKEN_SET           (clk_vdec_gcon_base + 0x0008)
#define LARB_CKEN_CLR           (clk_vdec_gcon_base + 0x000C)

/* VENC Register*/
#define VENC_CG_CON             (clk_venc_gcon_base + 0x0)
#define VENC_CG_SET             (clk_venc_gcon_base + 0x4)
#define VENC_CG_CLR             (clk_venc_gcon_base + 0x8)

enum {
	CG_INFRA0	= 0,
	CG_INFRA1	= 1,
	CG_DISP0	= 2,
	CG_DISP1	= 3,
	CG_IMAGE	= 4,
	CG_MFG		= 5,
	CG_AUDIO	= 6,
	CG_VDEC0	= 7,
	CG_VDEC1	= 8,
	CG_VENC		= 9,
	NR_GRPS		= 10,
};

#define CGID(grp, bit)	((grp) * 32 + bit)

enum cg_clk_id {
	MT_CG_INFRA_PMIC_TMR		= CGID(CG_INFRA0, 0),
	MT_CG_INFRA_PMIC_AP		= CGID(CG_INFRA0, 1),
	MT_CG_INFRA_PMIC_MD		= CGID(CG_INFRA0, 2),
	MT_CG_INFRA_PMIC_CONN		= CGID(CG_INFRA0, 3),
	MT_CG_INFRA_SCPSYS		= CGID(CG_INFRA0, 4),
	MT_CG_INFRA_SEJ			= CGID(CG_INFRA0, 5),
	MT_CG_INFRA_APXGPT		= CGID(CG_INFRA0, 6),
	MT_CG_INFRA_USB			= CGID(CG_INFRA0, 7),
	MT_CG_INFRA_ICUSB		= CGID(CG_INFRA0, 8),
	MT_CG_INFRA_GCE			= CGID(CG_INFRA0, 9),
	MT_CG_INFRA_THERM		= CGID(CG_INFRA0, 10),
	MT_CG_INFRA_I2C0		= CGID(CG_INFRA0, 11),
	MT_CG_INFRA_I2C1		= CGID(CG_INFRA0, 12),
	MT_CG_INFRA_I2C2		= CGID(CG_INFRA0, 13),
	MT_CG_INFRA_PWM_HCLK		= CGID(CG_INFRA0, 15),
	MT_CG_INFRA_PWM1		= CGID(CG_INFRA0, 16),
	MT_CG_INFRA_PWM2		= CGID(CG_INFRA0, 17),
	MT_CG_INFRA_PWM3		= CGID(CG_INFRA0, 18),
	MT_CG_INFRA_PWM			= CGID(CG_INFRA0, 21),
	MT_CG_INFRA_UART0		= CGID(CG_INFRA0, 22),
	MT_CG_INFRA_UART1		= CGID(CG_INFRA0, 23),
	MT_CG_INFRA_UART2		= CGID(CG_INFRA0, 24),
	MT_CG_INFRA_UART3		= CGID(CG_INFRA0, 25),
	MT_CG_INFRA_USB_MCU		= CGID(CG_INFRA0, 26),
	MT_CG_NFRA_NFI_ECC_66M		= CGID(CG_INFRA0, 27),
	MT_CG_NFRA_NFI_66M		= CGID(CG_INFRA0, 28),
	MT_CG_INFRA_BTIF		= CGID(CG_INFRA0, 31),

	MT_CG_INFRA_SPI			= CGID(CG_INFRA1, 1),
	MT_CG_INFRA_MSDC_0		= CGID(CG_INFRA1, 2),
	MT_CG_INFRA_MSDC_1		= CGID(CG_INFRA1, 4),
	MT_CG_INFRA_MSDC_2		= CGID(CG_INFRA1, 5),
	MT_CG_INFRA_MSDC_3		= CGID(CG_INFRA1, 6),
	MT_CG_INFRA_GCPU		= CGID(CG_INFRA1, 8),
	MT_CG_INFRA_AUXADC		= CGID(CG_INFRA1, 10),
	MT_CG_INFRA_CPUM		= CGID(CG_INFRA1, 11),
	MT_CG_INFRA_IRRX		= CGID(CG_INFRA1, 12),
	MT_CG_INFRA_UFO			= CGID(CG_INFRA1, 13),
	MT_CG_INFRA_CEC			= CGID(CG_INFRA1, 14),
	MT_CG_INFRA_CEC_26M		= CGID(CG_INFRA1, 15),
	MT_CG_INFRA_NFI_BCLK		= CGID(CG_INFRA1, 16),
	MT_CG_INFRA_NFI_ECC		= CGID(CG_INFRA1, 17),
	MT_CG_INFRA_APDMA		= CGID(CG_INFRA1, 18),
	MT_CG_INFRA_DEVICE_APC		= CGID(CG_INFRA1, 20),
	MT_CG_INFRA_XIU2AHB		= CGID(CG_INFRA1, 21),
	MT_CG_INFRA_L2C_SRAM		= CGID(CG_INFRA1, 22),
	MT_CG_INFRA_ETH_50M		= CGID(CG_INFRA1, 23),
	MT_CG_INFRA_DEBUGSYS		= CGID(CG_INFRA1, 24),
	MT_CG_INFRA_AUDIO		= CGID(CG_INFRA1, 25),
	MT_CG_INFRA_ETH_25M		= CGID(CG_INFRA1, 26),
	MT_CG_INFRA_NFI			= CGID(CG_INFRA1, 27),
	MT_CG_INFRA_ONFI		= CGID(CG_INFRA1, 28),
	MT_CG_INFRA_SNFI		= CGID(CG_INFRA1, 29),
	MT_CG_INFRA_ETH_HCLK		= CGID(CG_INFRA1, 30),
	MT_CG_INFRA_DRAMC_F26M		= CGID(CG_INFRA1, 31),

	MT_CG_DISP0_SMI_COMMON		= CGID(CG_DISP0, 0),
	MT_CG_DISP0_SMI_LARB0		= CGID(CG_DISP0, 1),
	MT_CG_DISP0_CAM_MDP		= CGID(CG_DISP0, 2),
	MT_CG_DISP0_MDP_RDMA		= CGID(CG_DISP0, 3),
	MT_CG_DISP0_MDP_RSZ0		= CGID(CG_DISP0, 4),
	MT_CG_DISP0_MDP_RSZ1		= CGID(CG_DISP0, 5),
	MT_CG_DISP0_MDP_TDSHP		= CGID(CG_DISP0, 6),
	MT_CG_DISP0_MDP_WDMA		= CGID(CG_DISP0, 7),
	MT_CG_DISP0_MDP_WROT		= CGID(CG_DISP0, 8),
	MT_CG_DISP0_FAKE_ENG		= CGID(CG_DISP0, 9),
	MT_CG_DISP0_DISP_OVL0		= CGID(CG_DISP0, 10),
	MT_CG_DISP0_DISP_OVL1		= CGID(CG_DISP0, 11),
	MT_CG_DISP0_DISP_RDMA0		= CGID(CG_DISP0, 12),
	MT_CG_DISP0_DISP_RDMA1		= CGID(CG_DISP0, 13),
	MT_CG_DISP0_DISP_WDMA0		= CGID(CG_DISP0, 14),
	MT_CG_DISP0_DISP_COLOR		= CGID(CG_DISP0, 15),
	MT_CG_DISP0_DISP_CCORR		= CGID(CG_DISP0, 16),
	MT_CG_DISP0_DISP_AAL		= CGID(CG_DISP0, 17),
	MT_CG_DISP0_DISP_GAMMA		= CGID(CG_DISP0, 18),
	MT_CG_DISP0_DISP_DITHER		= CGID(CG_DISP0, 19),
	MT_CG_DISP0_DISP_UFOE		= CGID(CG_DISP0, 20),
	MT_CG_DISP0_LARB4_AXI_ASIF_MM	= CGID(CG_DISP0, 21),
	MT_CG_DISP0_LARB4_AXI_ASIF_MJC	= CGID(CG_DISP0, 22),
	MT_CG_DISP0_DISP_WDMA1		= CGID(CG_DISP0, 23),
	MT_CG_DISP0_UFOD_RDMA0_L0	= CGID(CG_DISP0, 24),
	MT_CG_DISP0_UFOD_RDMA0_L1	= CGID(CG_DISP0, 25),
	MT_CG_DISP0_UFOD_RDMA0_L2	= CGID(CG_DISP0, 26),
	MT_CG_DISP0_UFOD_RDMA0_L3	= CGID(CG_DISP0, 27),
	MT_CG_DISP0_UFOD_RDMA1_L0	= CGID(CG_DISP0, 28),
	MT_CG_DISP0_UFOD_RDMA1_L1	= CGID(CG_DISP0, 29),
	MT_CG_DISP0_UFOD_RDMA1_L2	= CGID(CG_DISP0, 30),
	MT_CG_DISP0_UFOD_RDMA1_L3	= CGID(CG_DISP0, 31),

	MT_CG_DISP1_DISP_PWM_MM		= CGID(CG_DISP1, 0),
	MT_CG_DISP1_DISP_PWM_26M	= CGID(CG_DISP1, 1),
	MT_CG_DISP1_DSI_ENGINE		= CGID(CG_DISP1, 2),
	MT_CG_DISP1_DSI_DIGITAL		= CGID(CG_DISP1, 3),
	MT_CG_DISP1_DPI_PIXEL		= CGID(CG_DISP1, 4),
	MT_CG_DISP1_DPI_ENGINE		= CGID(CG_DISP1, 5),
	MT_CG_DISP1_LVDS_PIXEL		= CGID(CG_DISP1, 6),
	MT_CG_DISP1_LVDS_CTS		= CGID(CG_DISP1, 7),
	MT_CG_DISP1_DPI1_PIXEL		= CGID(CG_DISP1, 8),
	MT_CG_DISP1_DPI1_ENGINE		= CGID(CG_DISP1, 9),
	MT_CG_DISP1_HDMI_PIXEL		= CGID(CG_DISP1, 10),
	MT_CG_DISP1_HDMI_SPDIF		= CGID(CG_DISP1, 11),
	MT_CG_DISP1_HDMI_ADSP		= CGID(CG_DISP1, 12),
	MT_CG_DISP1_HDMI_PLLCK		= CGID(CG_DISP1, 13),
	MT_CG_DISP1_DISP_DSC_ENGINE	= CGID(CG_DISP1, 14),
	MT_CG_DISP1_DISP_DSC_MEM	= CGID(CG_DISP1, 15),

	MT_CG_IMAGE_LARB2_SMI		= CGID(CG_IMAGE, 0),
	MT_CG_IMAGE_JPGENC		= CGID(CG_IMAGE, 4),
	MT_CG_IMAGE_CAM_SMI		= CGID(CG_IMAGE, 5),
	MT_CG_IMAGE_CAM_CAM		= CGID(CG_IMAGE, 6),
	MT_CG_IMAGE_SEN_TG		= CGID(CG_IMAGE, 7),
	MT_CG_IMAGE_SEN_CAM		= CGID(CG_IMAGE, 8),
	MT_CG_IMAGE_CAM_SV		= CGID(CG_IMAGE, 9),

#if CLKMGR_ALIAS
	MT_CG_MFG_BG3D			= CGID(CG_MFG, 0),
#endif

	MT_CG_G3D_FF			= CGID(CG_MFG, 0),
	MT_CG_G3D_DW			= CGID(CG_MFG, 1),
	MT_CG_G3D_TX			= CGID(CG_MFG, 2),
	MT_CG_G3D_MX			= CGID(CG_MFG, 3),

	MT_CG_AUDIO_AFE			= CGID(CG_AUDIO, 2),
	MT_CG_AUDIO_I2S			= CGID(CG_AUDIO, 6),
	MT_CG_AUDIO_22M			= CGID(CG_AUDIO, 8),
	MT_CG_AUDIO_24M			= CGID(CG_AUDIO, 9),
	MT_CG_AUDIO_SPDF2		= CGID(CG_AUDIO, 11),
	MT_CG_AUDIO_APLL2_TUNER		= CGID(CG_AUDIO, 18),
	MT_CG_AUDIO_APLL_TUNER		= CGID(CG_AUDIO, 19),
	MT_CG_AUDIO_HDMI		= CGID(CG_AUDIO, 20),
	MT_CG_AUDIO_SPDF		= CGID(CG_AUDIO, 21),
	MT_CG_AUDIO_ADC			= CGID(CG_AUDIO, 24),
	MT_CG_AUDIO_DAC			= CGID(CG_AUDIO, 25),
	MT_CG_AUDIO_DAC_PREDIS		= CGID(CG_AUDIO, 26),
	MT_CG_AUDIO_TML			= CGID(CG_AUDIO, 27),
	MT_CG_AUDIO_IDLE_EN_EXT		= CGID(CG_AUDIO, 29),
	MT_CG_AUDIO_IDLE_EN_INT		= CGID(CG_AUDIO, 30),

	MT_CG_VDEC0_VDEC		= CGID(CG_VDEC0, 0),

	MT_CG_VDEC1_LARB		= CGID(CG_VDEC1, 0),

#if CLKMGR_ALIAS
	MT_CG_VENC_LARB			= CGID(CG_VENC, 0),
	MT_CG_VENC_VENC			= CGID(CG_VENC, 4),
	MT_CG_VENC_JPGENC		= CGID(CG_VENC, 8),
	MT_CG_VENC_JPGDEC		= CGID(CG_VENC, 12),
#endif

	MT_CG_VENC_CKE0			= CGID(CG_VENC, 0),
	MT_CG_VENC_CKE1			= CGID(CG_VENC, 4),
	MT_CG_VENC_CKE2			= CGID(CG_VENC, 8),
	MT_CG_VENC_CKE3			= CGID(CG_VENC, 12),

	NR_CLKS,

	CG_INFRA0_FROM			= MT_CG_INFRA_PMIC_TMR,
	CG_INFRA0_TO			= MT_CG_INFRA_BTIF,

	CG_INFRA1_FROM			= MT_CG_INFRA_SPI,
	CG_INFRA1_TO			= MT_CG_INFRA_DRAMC_F26M,

	CG_DISP0_FROM			= MT_CG_DISP0_SMI_COMMON,
	CG_DISP0_TO			= MT_CG_DISP0_UFOD_RDMA1_L3,

	CG_DISP1_FROM			= MT_CG_DISP1_DISP_PWM_MM,
	CG_DISP1_TO			= MT_CG_DISP1_DISP_DSC_MEM,

	CG_IMAGE_FROM			= MT_CG_IMAGE_LARB2_SMI,
	CG_IMAGE_TO			= MT_CG_IMAGE_CAM_SV,

	CG_MFG_FROM			= MT_CG_G3D_FF,
	CG_MFG_TO			= MT_CG_G3D_MX,

	CG_AUDIO_FROM			= MT_CG_AUDIO_AFE,
	CG_AUDIO_TO			= MT_CG_AUDIO_IDLE_EN_INT,

	CG_VDEC0_FROM			= MT_CG_VDEC0_VDEC,
	CG_VDEC0_TO			= MT_CG_VDEC0_VDEC,

	CG_VDEC1_FROM			= MT_CG_VDEC1_LARB,
	CG_VDEC1_TO			= MT_CG_VDEC1_LARB,

	CG_VENC_FROM			= MT_CG_VENC_CKE0,
	CG_VENC_TO			= MT_CG_VENC_CKE3,
};

enum {
	/* CLK_CFG_0 */
	MT_MUX_AXI,
	MT_MUX_MEM,
	MT_MUX_DDRPHY,
	MT_MUX_MM,

	/* CLK_CFG_1 */
	MT_MUX_PWM,
	MT_MUX_VDEC,
	MT_MUX_MFG,

	/* CLK_CFG_2 */
	MT_MUX_CAMTG,
	MT_MUX_UART,
	MT_MUX_SPI,

	/* CLK_CFG_3 */
	MT_MUX_MSDC30_0,
	MT_MUX_MSDC30_1,
	MT_MUX_MSDC30_2,

	/* CLK_CFG_4 */
	MT_MUX_MSDC50_3_HCLK,
	MT_MUX_MSDC50_3,
	MT_MUX_AUDIO,
	MT_MUX_AUDINTBUS,

	/* CLK_CFG_5 */
	MT_MUX_PMICSPI,
	MT_MUX_SCP,
	MT_MUX_ATB,
	MT_MUX_MJC,

	/* CLK_CFG_6 */
	MT_MUX_DPI0,
	MT_MUX_SCAM,
	MT_MUX_AUD1,
	MT_MUX_AUD2,

	/* CLK_CFG_7 */
	MT_MUX_DPI1,
	MT_MUX_UFOENC,
	MT_MUX_UFODEC,
	MT_MUX_ETH,

	/* CLK_CFG_8 */
	MT_MUX_ONFI,
	MT_MUX_SNFI,
	MT_MUX_HDMI,
	MT_MUX_RTC,

	NR_MUXS,
};

enum {
	ARMPLL		= 0,
	MAINPLL		= 1,
	MSDCPLL		= 2,
	UNIVPLL		= 3,
	MMPLL		= 4,
	VENCPLL		= 5,
	TVDPLL		= 6,
	MPLL		= 7,
#if CLKMGR_ALIAS
	APLL1		= 8,
	APLL2		= 9,
#endif
	AUD1PLL		= 8,
	AUD2PLL		= 9,

	LVDSPLL		= 10,
	NR_PLLS		= 11,
};

enum {
	SYS_CONN,
	SYS_DIS,
	SYS_MFG,
	SYS_ISP,
	SYS_VDE,
	SYS_VEN,
	SYS_AUD,
	NR_SYSS,
};

enum {
	MT_LARB_DISP	= 0,
	MT_LARB_VDEC	= 1,
	MT_LARB_IMG	= 2,
	MT_LARB_VENC	= 3,
};

/* larb monitor mechanism definition */
enum {
	LARB_MONITOR_LEVEL_HIGH		= 10,
	LARB_MONITOR_LEVEL_MEDIUM	= 20,
	LARB_MONITOR_LEVEL_LOW		= 30,
};

struct larb_monitor {
	struct list_head link;
	int level;
	void (*backup) (struct larb_monitor *h, int larb_idx);
	void (*restore) (struct larb_monitor *h, int larb_idx);
};

enum monitor_clk_sel_0 {
	no_clk_0		= 0,
	AD_UNIV_624M_CK		= 5,
	AD_UNIV_416M_CK		= 6,
	AD_UNIV_249P6M_CK	= 7,
	AD_UNIV_178P3M_CK_0	= 8,
	AD_UNIV_48M_CK		= 9,
	AD_USB_48M_CK		= 10,
	rtc32k_ck_i_0		= 20,
	AD_SYS_26M_CK_0		= 21,
};
enum monitor_clk_sel {
	no_clk			= 0,
	AD_SYS_26M_CK		= 1,
	rtc32k_ck_i		= 2,
	clkph_MCLK_o		= 7,
	AD_DPICLK		= 8,
	AD_MSDCPLL_CK		= 9,
	AD_MMPLL_CK		= 10,
	AD_UNIV_178P3M_CK	= 11,
	AD_MAIN_H156M_CK	= 12,
	AD_VENCPLL_CK		= 13,
};

enum ckmon_sel {
	clk_ckmon0 = 0,
	clk_ckmon1 = 1,
	clk_ckmon2 = 2,
	clk_ckmon3 = 3,
};

enum idle_mode {
	dpidle = 0,
	soidle = 1,
	slidle = 2,
};

extern void register_larb_monitor(struct larb_monitor *handler);
extern void unregister_larb_monitor(struct larb_monitor *handler);

/* clock API */
extern int enable_clock(enum cg_clk_id id, char *mod_name);
extern int disable_clock(enum cg_clk_id id, char *mod_name);
extern int mt_enable_clock(enum cg_clk_id id, char *mod_name);
extern int mt_disable_clock(enum cg_clk_id id, char *mod_name);

extern int enable_clock_ext_locked(int id, char *mod_name);
extern int disable_clock_ext_locked(int id, char *mod_name);

extern int clock_is_on(int id);

extern int clkmux_sel(int id, unsigned int clksrc, char *name);
extern void enable_mux(int id, char *name);
extern void disable_mux(int id, char *name);

extern void clk_set_force_on(int id);
extern void clk_clr_force_on(int id);
extern int clk_is_force_on(int id);

/* pll API */
extern int enable_pll(int id, char *mod_name);
extern int disable_pll(int id, char *mod_name);

extern int pll_hp_switch_on(int id, int hp_on);
extern int pll_hp_switch_off(int id, int hp_off);

extern int pll_fsel(int id, unsigned int value);
extern int pll_is_on(int id);

/* subsys API */
extern int enable_subsys(int id, char *mod_name);
extern int disable_subsys(int id, char *mod_name);

extern int subsys_is_on(int id);

extern int conn_power_on(void);
extern int conn_power_off(void);

/* other API */

extern void set_mipi26m(int en);
extern void set_ada_ssusb_xtal_ck(int en);

const char *grp_get_name(int id);
extern int clkmgr_is_locked(void);

extern void clk_stat_check(int id);
extern void slp_check_pm_mtcmos_pll(void);
extern void clk_misc_cfg_ops(bool flag);

#endif /* _MT_CLKMGR_H */
