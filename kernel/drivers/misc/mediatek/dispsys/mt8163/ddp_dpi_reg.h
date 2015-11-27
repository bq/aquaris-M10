#ifndef __DDP_DPI_REG_H__
#define __DDP_DPI_REG_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned EN:1;
	unsigned rsv_1:31;
} DPI_REG_EN, *PDPI_REG_EN;

typedef struct {
	unsigned RST:1;
	unsigned rsv_1:31;
} DPI_REG_RST, *PDPI_REG_RST;

typedef struct {
	unsigned VSYNC:1;
	unsigned VDE:1;
	unsigned UNDERFLOW:1;
	unsigned rsv_3:29;
} DPI_REG_INTERRUPT, *PDPI_REG_INTERRUPT;

typedef struct {
	unsigned BG_EN:1;
	unsigned RGB_SWAP:1;
	unsigned INTL_EN:1;
	unsigned TDFP_EN:1;
	unsigned CLPF_EN:1;
	unsigned YUV422_EN:1;
	unsigned RGB2YUV_EN:1;
	unsigned R601_SEL:1;
	unsigned EMBSYNC_EN:1;
	unsigned rsv_9:3;
	unsigned PIXREP:4;	/* new */
	unsigned VS_LODD_EN:1;
	unsigned VS_LEVEN_EN:1;
	unsigned VS_RODD_EN:1;
	unsigned VS_REVEN_EN:1;
	unsigned FAKE_DE_LODD:1;
	unsigned FAKE_DE_LEVEN:1;
	unsigned FAKE_DE_RODD:1;
	unsigned FAKE_DE_REVEN:1;
	unsigned rsv_24:8;
} DPI_REG_CNTL, *PDPI_REG_CNTL;

typedef struct {
	unsigned CH_SWAP:3;
	unsigned BIT_SWAP:1;
	unsigned B_MASK:1;
	unsigned G_MASK:1;
	unsigned R_MASK:1;
	unsigned rsv_7:1;
	unsigned DE_MASK:1;
	unsigned HS_MASK:1;
	unsigned VS_MASK:1;
	unsigned rsv_11:1;
	unsigned DE_POL:1;
	unsigned HSYNC_POL:1;
	unsigned VSYNC_POL:1;
	unsigned CLK_POL:1;
	unsigned DPI_O_EN:1;
	unsigned DUAL_EDGE_SEL:1;
	unsigned OUT_BIT:2;
	unsigned YC_MAP:3;
	unsigned rsv_23:9;
} DPI_REG_OUTPUT_SETTING, *PDPI_REG_OUTPUT_SETTING;

typedef struct {
	unsigned WIDTH:13;
	unsigned rsv_13:3;
	unsigned HEIGHT:13;
	unsigned rsv_28:3;
} DPI_REG_SIZE, *PDPI_REG_SIZE;

typedef struct {
	unsigned DDR_EN:1;
	unsigned DDR_SEL:1;
	unsigned DDR_4PHASE:1;
	unsigned DATA_THROT:1;	/* new */
	unsigned DDR_WIDTH:2;
	unsigned rsv_6:2;
	unsigned DDR_PAD_MODE:1;
	unsigned rsv_9:23;
} DPI_REG_DDR_SETTING, *PDPI_REG_DDR_SETTING;

typedef struct {
	unsigned HBP:12;
	unsigned rsv_12:4;
	unsigned HFP:12;
	unsigned rsv_28:4;
} DPI_REG_TGEN_HPORCH, *PDPI_REG_TGEN_HPORCH;

typedef struct {
	unsigned VPW_LODD:12;
	unsigned rsv_12:4;
	unsigned VPW_HALF_LODD:1;
	unsigned rsv_17:15;
} DPI_REG_TGEN_VWIDTH_LODD, *PDPI_REG_TGEN_VWIDTH_LODD;

typedef struct {
	unsigned VBP_LODD:12;
	unsigned rsv_12:4;
	unsigned VFP_LODD:12;
	unsigned rsv_28:4;
} DPI_REG_TGEN_VPORCH_LODD, *PDPI_REG_TGEN_VPORCH_LODD;

typedef struct {
	unsigned BG_RIGHT:13;
	unsigned rsv_13:3;
	unsigned BG_LEFT:13;
	unsigned rsv_29:3;
} DPI_REG_BG_HCNTL, *PDPI_REG_BG_HCNTL;

typedef struct {
	unsigned BG_BOT:13;
	unsigned rsv_13:3;
	unsigned BG_TOP:13;
	unsigned rsv_29:3;
} DPI_REG_BG_VCNTL, *PDPI_REG_BG_VCNTL;


typedef struct {
	unsigned BG_B:8;
	unsigned BG_G:8;
	unsigned BG_R:8;
	unsigned rsv_24:8;
} DPI_REG_BG_COLOR, *PDPI_REG_BG_COLOR;

typedef struct {
	unsigned FIFO_VALID_SET:5;
	unsigned rsv_5:3;
	unsigned FIFO_RST_SEL:1;
	unsigned rsv_9:23;
} DPI_REG_FIFO_CTL, *PDPI_REG_FIFO_CTL;

typedef struct {
	unsigned V_CNT:13;
	unsigned rsv_13:3;
	unsigned DPI_BUSY:1;
	unsigned OUT_EN:1;
	unsigned rsv_18:2;
	unsigned FIELD:1;
	unsigned TDLR:1;
	unsigned rsv_22:10;
} DPI_REG_STATUS, *PDPI_REG_STATUS;

typedef struct {
	unsigned OEN_EN:1;
	unsigned rsv_1:31;
} DPI_REG_TMODE, *PDPI_REG_TMODE;

typedef struct {
	unsigned CHKSUM:24;
	unsigned rsv_24:6;
	unsigned CHKSUM_RDY:1;
	unsigned CHKSUM_EN:1;
} DPI_REG_CHKSUM, *PDPI_REG_CHKSUM;

typedef struct {
	unsigned VPW_LEVEN:12;
	unsigned rsv_12:4;
	unsigned VPW_HALF_LEVEN:1;
	unsigned rsv_17:15;
} DPI_REG_TGEN_VWIDTH_LEVEN, *PDPI_REG_TGEN_VWIDTH_LEVEN;

typedef struct {
	unsigned VBP_LEVEN:12;
	unsigned rsv_12:4;
	unsigned VFP_LEVEN:12;
	unsigned rsv_28:4;
} DPI_REG_TGEN_VPORCH_LEVEN, *PDPI_REG_TGEN_VPORCH_LEVEN;

typedef struct {
	unsigned VPW_RODD:12;
	unsigned rsv_12:4;
	unsigned VPW_HALF_RODD:1;
	unsigned rsv_17:15;
} DPI_REG_TGEN_VWIDTH_RODD, *PDPI_REG_TGEN_VWIDTH_RODD;

typedef struct {
	unsigned VBP_RODD:12;
	unsigned rsv_12:4;
	unsigned VFP_RODD:12;
	unsigned rsv_28:4;
} DPI_REG_TGEN_VPORCH_RODD, *PDPI_REG_TGEN_VPORCH_RODD;

typedef struct {
	unsigned VPW_REVEN:12;
	unsigned rsv_12:4;
	unsigned VPW_HALF_REVEN:1;
	unsigned rsv_17:15;
} DPI_REG_TGEN_VWIDTH_REVEN, *PDPI_REG_TGEN_VWIDTH_REVEN;

typedef struct {
	unsigned VBP_REVEN:12;
	unsigned rsv_12:4;
	unsigned VFP_REVEN:12;
	unsigned rsv_28:4;
} DPI_REG_TGEN_VPORCH_REVEN, *PDPI_REG_TGEN_VPORCH_REVEN;

typedef struct {
	unsigned ESAV_VOFST_LODD:12;
	unsigned rsv_12:4;
	unsigned ESAV_VWID_LODD:12;
	unsigned rsv_28:4;
} DPI_REG_ESAV_VTIM_LOAD, *PDPI_REG_ESAV_VTIM_LOAD;

typedef struct {
	unsigned ESAV_VVOFST_LEVEN:12;
	unsigned rsv_12:4;
	unsigned ESAV_VWID_LEVEN:12;
	unsigned rsv_28:4;
} DPI_REG_ESAV_VTIM_LEVEN, *PDPI_REG_ESAV_VTIM_LEVEN;

typedef struct {
	unsigned ESAV_VOFST_RODD:12;
	unsigned rsv_12:4;
	unsigned ESAV_VWID_RODD:12;
	unsigned rsv_28:4;
} DPI_REG_ESAV_VTIM_ROAD, *PDPI_REG_ESAV_VTIM_ROAD;

typedef struct {
	unsigned ESAV_VOFST_REVEN:12;
	unsigned rsv_12:4;
	unsigned ESAV_VWID_REVEN:12;
	unsigned rsv_28:4;
} DPI_REG_ESAV_VTIM_REVEN, *PDPI_REG_ESAV_VTIM_REVEN;


typedef struct {
	unsigned ESAV_FOFST_ODD:12;
	unsigned rsv_12:4;
	unsigned ESAV_FOFST_EVEN:12;
	unsigned rsv_28:4;
} DPI_REG_ESAV_FTIM, *PDPI_REG_ESAV_FTIM;

typedef struct {
	unsigned CLPF_TYPE:2;
	unsigned rsv2:2;
	unsigned ROUND_EN:1;
	unsigned rsv5:27;
} DPI_REG_CLPF_SETTING, *PDPI_REG_CLPF_SETTING;

typedef struct {
	unsigned Y_LIMIT_BOT:12;
	unsigned rsv12:4;
	unsigned Y_LIMIT_TOP:12;
	unsigned rsv28:4;
} DPI_REG_Y_LIMIT, *PDPI_REG_Y_LIMIT;

typedef struct {
	unsigned C_LIMIT_BOT:12;
	unsigned rsv12:4;
	unsigned C_LIMIT_TOP:12;
	unsigned rsv28:4;
} DPI_REG_C_LIMIT, *PDPI_REG_C_LIMIT;

typedef struct {
	unsigned UV_SWAP:1;
	unsigned rsv1:3;
	unsigned CR_DELSEL:1;
	unsigned CB_DELSEL:1;
	unsigned Y_DELSEL:1;
	unsigned DE_DELSEL:1;
	unsigned rsv8:24;
} DPI_REG_YUV422_SETTING, *PDPI_REG_YUV422_SETTING;

typedef struct {
	unsigned EMBVSYNC_R_CR:1;
	unsigned EMBVSYNC_G_Y:1;
	unsigned EMBVSYNC_B_CB:1;
	unsigned rsv_3:1;
	unsigned ESAV_F_INV:1;
	unsigned ESAV_V_INV:1;
	unsigned ESAV_H_INV:1;
	unsigned rsv_7:1;
	unsigned ESAV_CODE_MAN:1;
	unsigned rsv_9:3;
	unsigned VS_OUT_SEL:3;
	unsigned rsv_15:1;
	unsigned EMBSYNC_OPT:1;	/* new */
	unsigned rsv_17:15;
} DPI_REG_EMBSYNC_SETTING;

typedef struct {
	unsigned ESAV_CODE0:12;
	unsigned rsv_12:4;
	unsigned ESAV_CODE1:12;
	unsigned rsv_28:4;
} DPI_REG_ESAV_CODE_SET0, *PDPI_REG_ESAV_CODE_SET0;

typedef struct {
	unsigned ESAV_CODE2:12;
	unsigned rsv_12:4;
	unsigned ESAV_CODE3_MSB:1;
	unsigned rsv_17:15;
} DPI_REG_ESAV_CODE_SET1, *PDPI_REG_ESAV_CODE_SET1;

typedef struct {
	unsigned PAT_EN:1;
	unsigned rsv_1:3;
	unsigned PAT_SEL:3;
	unsigned rsv_6:1;
	unsigned PAT_B_MAN:8;
	unsigned PAT_G_MAN:8;
	unsigned PAT_R_MAN:8;
} DPI_REG_PATTERN;

/* <--not be used */
typedef struct {
	unsigned HPW:8;
	unsigned HBP:8;
	unsigned HFP:8;
	unsigned HSYNC_POL:1;
	unsigned DE_POL:1;
	unsigned rsv_26:6;
} DPI_REG_TGEN_HCNTL, *PDPI_REG_TGEN_HCNTL;

typedef struct {
	unsigned VPW:8;
	unsigned VBP:8;
	unsigned VFP:8;
	unsigned VSYNC_POL:1;
	unsigned rsv_25:7;
} DPI_REG_TGEN_VCNTL, *PDPI_REG_TGEN_VCNTL;
/* --> */

typedef struct {
	unsigned MATRIX_C00:13;
	unsigned rsv_13:3;
	unsigned MATRIX_C01:13;
	unsigned rsv_29:3;
} DPI_REG_MATRIX_COEFF_SET0;

typedef struct {
	unsigned MATRIX_C02:13;
	unsigned rsv_13:3;
	unsigned MATRIX_C10:13;
	unsigned rsv_29:3;
} DPI_REG_MATRIX_COEFF_SET1;

typedef struct {
	unsigned MATRIX_C11:13;
	unsigned rsv_13:3;
	unsigned MATRIX_C12:13;
	unsigned rsv_29:3;
} DPI_REG_MATRIX_COEFF_SET2;

typedef struct {
	unsigned MATRIX_C20:13;
	unsigned rsv_13:3;
	unsigned MATRIX_C21:13;
	unsigned rsv_29:3;
} DPI_REG_MATRIX_COEFF_SET3;

typedef struct {
	unsigned MATRIX_C22:13;
	unsigned rsv_13:19;
} DPI_REG_MATRIX_COEFF_SET4;

typedef struct {
	unsigned MATRIX_PRE_ADD_0:9;
	unsigned rsv_9:7;
	unsigned MATRIX_PRE_ADD_1:9;
	unsigned rsv_24:7;
} DPI_REG_MATRIX_PREADD_SET0;

typedef struct {
	unsigned MATRIX_PRE_ADD_2:9;
	unsigned rsv_9:23;
} DPI_REG_MATRIX_PREADD_SET1;

typedef struct {
	unsigned MATRIX_POST_ADD_0:13;
	unsigned rsv_13:3;
	unsigned MATRIX_POST_ADD_1:13;
	unsigned rsv_24:3;
} DPI_REG_MATRIX_POSTADD_SET0;

typedef struct {
	unsigned MATRIX_POST_ADD_2:13;
	unsigned rsv_13:19;
} DPI_REG_MATRIX_POSTADD_SET1;

typedef struct {
	unsigned DPI_CK_DIV:5;
	unsigned EDGE_SEL_EN:1;
	unsigned rsv_6:2;
	unsigned DPI_CK_DUT:5;
	unsigned rsv_13:12;
	unsigned DPI_CKOUT_DIV:1;
	unsigned rsv_26:5;
	unsigned DPI_CK_POL:1;
} DPI_REG_CLKCNTL, *PDPI_REG_CLKCNTL;

typedef struct {
	DPI_REG_EN DPI_EN;	/* 0000 */
	DPI_REG_RST DPI_RST;	/* 0004 */
	DPI_REG_INTERRUPT INT_ENABLE;	/* 0008 */
	DPI_REG_INTERRUPT INT_STATUS;	/* 000C */
	DPI_REG_CNTL CNTL;	/* 0010 */
	DPI_REG_OUTPUT_SETTING OUTPUT_SETTING;	/* 0014 */
	DPI_REG_SIZE SIZE;	/* 0018 */
	DPI_REG_DDR_SETTING DDR_SETTING;	/* 001c */
	UINT32 TGEN_HWIDTH;	/* 0020 */
	DPI_REG_TGEN_HPORCH TGEN_HPORCH;	/* 0024 */
	DPI_REG_TGEN_VWIDTH_LODD TGEN_VWIDTH_LODD;	/* 0028 */
	DPI_REG_TGEN_VPORCH_LODD TGEN_VPORCH_LODD;	/* 002C */
	DPI_REG_BG_HCNTL BG_HCNTL;	/* 0030 */
	DPI_REG_BG_VCNTL BG_VCNTL;	/* 0034 */
	DPI_REG_BG_COLOR BG_COLOR;	/* 0038 */
	DPI_REG_FIFO_CTL FIFO_CTL;	/* 003C */

	DPI_REG_STATUS STATUS;	/* 0040 */
	DPI_REG_TMODE TMODE;	/* 0044 */
	DPI_REG_CHKSUM CHKSUM;	/* 0048 */
	UINT32 rsv_4C;
	UINT32 DUMMY;	/* 0050 */
	UINT32 rsv_54[5];
	DPI_REG_TGEN_VWIDTH_LEVEN TGEN_VWIDTH_LEVEN;	/* 0068 */
	DPI_REG_TGEN_VPORCH_LEVEN TGEN_VPORCH_LEVEN;	/* 006C */
	DPI_REG_TGEN_VWIDTH_RODD TGEN_VWIDTH_RODD;	/* 0070 */
	DPI_REG_TGEN_VPORCH_RODD TGEN_VPORCH_RODD;	/* 0074 */
	DPI_REG_TGEN_VWIDTH_REVEN TGEN_VWIDTH_REVEN;	/* 0078 */
	DPI_REG_TGEN_VPORCH_REVEN TGEN_VPORCH_REVEN;	/* 007C */
	DPI_REG_ESAV_VTIM_LOAD ESAV_VTIM_LOAD;	/* 0080 */
	DPI_REG_ESAV_VTIM_LEVEN ESAV_VTIM_LEVEN;	/* 0084 */
	DPI_REG_ESAV_VTIM_ROAD ESAV_VTIM_ROAD;	/* 0088 */
	DPI_REG_ESAV_VTIM_REVEN ESAV_VTIM_REVEN;	/* 008C */
	DPI_REG_ESAV_FTIM ESAV_FTIM;	/* 0090 */
	DPI_REG_CLPF_SETTING CLPF_SETTING;	/* 0094 */
	DPI_REG_Y_LIMIT Y_LIMIT;	/* 0098 */
	DPI_REG_C_LIMIT C_LIMIT;	/* 009C */
	DPI_REG_YUV422_SETTING YUV422_SETTING;	/* 00A0 */
	DPI_REG_EMBSYNC_SETTING EMBSYNC_SETTING;	/* 00A4 */
	DPI_REG_ESAV_CODE_SET0 ESAV_CODE_SET0;	/* 00A8 */
	DPI_REG_ESAV_CODE_SET1 ESAV_CODE_SET1;	/* 00AC */
	UINT32 rsv_b0[12];
	DPI_REG_CLKCNTL DPI_CLKCON;	/* 00E0 */
} volatile DPI_REGS, *PDPI_REGS;

/*LVDS TX REG defines*/
typedef struct
{
    unsigned HSYNC_INV  : 1;
    unsigned VSYNC_INV    : 1;
    unsigned DE_INV     : 1;
    unsigned rsv_3     : 1;
    unsigned DATA_FMT     : 3;
    unsigned rsv_7        : 25;	
} LVDS_REG_FMTCNTL, *PLVDS_REG_FMTCNTL;


typedef struct
{
    unsigned R_SEL  : 2;
    unsigned G_SEL    : 2;
    unsigned B_SEL     : 2;
    unsigned rsv_6     : 2;
    unsigned R_SRC_VAL     : 8;
    unsigned G_SRC_VAL        : 8;	
    unsigned B_SRC_VAL        : 8;	
} LVDS_REG_DATA_SRC, *PLVDS_REG_DATA_SRC;




typedef struct
{
    unsigned HSYNC_SEL  : 2;
    unsigned VSYNC_SEL    : 2;
    unsigned DE_SEL     : 2;
    unsigned SRC_FOR_HSYNC     : 1;
    unsigned SRC_FOR_VSYNC     : 1;
    unsigned SRC_FOR_DE     : 1;	
    unsigned rsv_7        : 23;	
} LVDS_REG_CNTL, *PLVDS_REG_CNTL;



typedef struct
{
    unsigned R0_SEL  : 3;
    unsigned R1_SEL  : 3;
    unsigned R2_SEL  : 3;
    unsigned R3_SEL  : 3;
    unsigned R4_SEL  : 3;
    unsigned R5_SEL  : 3;
    unsigned R6_SEL  : 3;
    unsigned R7_SEL  : 3;
    unsigned rsv_24     : 8;	
} LVDS_REG_R_SEL, *PLVDS_REG_R_SEL;

typedef struct
{
    unsigned G0_SEL  : 3;
    unsigned G1_SEL  : 3;
    unsigned G2_SEL  : 3;
    unsigned G3_SEL  : 3;
    unsigned G4_SEL  : 3;
    unsigned G5_SEL  : 3;
    unsigned G6_SEL  : 3;
    unsigned G7_SEL  : 3;
    unsigned rsv_24     : 8;	
} LVDS_REG_G_SEL, *PLVDS_REG_G_SEL;


typedef struct
{
    unsigned B0_SEL  : 3;
    unsigned B1_SEL  : 3;
    unsigned B2_SEL  : 3;
    unsigned B3_SEL  : 3;
    unsigned B4_SEL  : 3;
    unsigned B5_SEL  : 3;
    unsigned B6_SEL  : 3;
    unsigned B7_SEL  : 3;
    unsigned rsv_24     : 8;	
} LVDS_REG_B_SEL, *PLVDS_REG_B_SEL;


typedef struct
{
    unsigned LVDS_EN  : 1;
    unsigned LVDS_OUT_FIFO_EN  : 1;
    unsigned DPMODE  : 1;
    unsigned rsv_4     : 28;	
    unsigned LVDSRX_FIFO_EN  : 1;	
} LVDS_REG_OUT_CTRL, *PLVDS_REG_OUT_CTRL;


typedef struct
{
    unsigned CH0_SEL  : 3;
    unsigned CH1_SEL  : 3;
    unsigned CH2_SEL  : 3;
    unsigned CH3_SEL  : 3;
    unsigned CLK_SEL  : 3;
    unsigned rsv_15  : 1;
    unsigned ML_SWAP  : 5;
    unsigned rsv_21  : 3;
    unsigned PN_SWAP     : 5;	
    unsigned rsv_29  : 2;
    unsigned BIT_SWAP     : 1;		
} LVDS_REG_CH_SWAP, *PLVDS_REG_CH_SWAP;


typedef struct
{
    unsigned TX_CK_EN  : 1;
    unsigned RX_CK_EN  : 1;  // used for out crc
    unsigned TEST_CK_EN  : 1;
    unsigned rsv_3  : 5;
    unsigned TEST_CK_SEL0  : 1;
    unsigned TEST_CK_SEL1  : 1;
    unsigned rsv_10  : 22;
} LVDS_REG_CLK_CTRL, *PLVDS_REG_CLK_CTRL;

typedef struct
{
    unsigned RG_CRC_START  : 1;
    unsigned RG_CRC_CLR  : 1;  
    unsigned rsv_2  : 6;
    unsigned RG_LVDSRX_CRC_START1  : 1;
	unsigned rsv_9  : 1;
    unsigned RG_LVDSRX_CRC_CLR1  : 1;
    unsigned rsv_11  : 21;
} LVDS_REG_CRC_CTRL, *PLVDS_REG_CRC_CTRL;

typedef struct
{
    unsigned RG_CLK_CTRL  : 7;
    unsigned RG_CLKEN  : 1;  // used for out crc
    unsigned RG_RES_BIT  : 1;
    unsigned rsv_9  : 23;
} LVDS_REG_RG_CLK, *PLVDS_REG_RG_CLKL;

typedef struct
{
    unsigned RG_MON_SEL  : 6;
	unsigned rsv_6  : 2;
    unsigned RG_MON_EN  : 1;  // used for out crc
    unsigned rsv_9  : 7;
    unsigned RG_TEST_EN  : 1;
    unsigned rsv_17  : 15;
} LVDS_REG_RG_MON, *PLVDS_REG_RG_MON;

typedef struct
{
    unsigned RG_PCLK_SW_RST  : 1;
    unsigned RG_CTSCLK_SW_RST  : 1;
    unsigned rsv_2  : 30;
} LVDS_REG_SOFT_RESET, *PLVDS_REG_SOFT_RESET;

typedef struct /*10BIT LVDS*/
{
    unsigned RG_LVDS_CH0  : 10;
    unsigned RG_LVDS_CH1  : 10;
	unsigned RG_LVDS_CH2  : 10;
    unsigned rsv_30  : 2;
} LVDS_REG_RGCHL_SRC0, *PLVDS_REG_RGCHL_SRC0;

typedef struct /*10BIT LVDS*/
{
    unsigned RG_LVDS_CH3  : 10;
    unsigned RG_LVDS_CLK: 10;
    unsigned rsv_20  : 11;
	unsigned RG_PAT_EN  : 1;
} LVDS_REG_RGCHL_SRC1, *PLVDS_REG_RGCHL_SRC1;

typedef struct
{
    unsigned RG_PTGEN_HTOTAL  : 13;
    unsigned rsv_13  : 3;
	unsigned RG_PTGEN_VTOTAL  : 12;
	unsigned rsv_28  : 4;
} LVDS_REG_PATN_TOTAL, *PLVDS_REG_PATN_TOTAL;

typedef struct
{
    unsigned RG_PTGEN_HWIDTH  : 13;
    unsigned rsv_13  : 3;
	unsigned RG_PTGEN_VWIDTH  : 12;
	unsigned rsv_28  : 4;
} LVDS_REG_PATN_WIDTH, *PLVDS_REG_PATN_WIDTH;

typedef struct
{
    unsigned RG_PTGEN_HSTART  : 13;
    unsigned rsv_13  : 3;
	unsigned RG_PTGEN_VSTART  : 12;
	unsigned rsv_28  : 4;
} LVDS_REG_PATN_START, *PLVDS_REG_PATN_START;

typedef struct
{
    unsigned RG_PTGEN_HACTIVE  : 13;
    unsigned rsv_13  : 3;
	unsigned RG_PTGEN_VACTIVE  : 12;
	unsigned rsv_28  : 4;
} LVDS_REG_PATN_ACTIVE, *PLVDS_REG_PATN_ACTIVE;

typedef struct
{
    unsigned RG_TST_PATN_EN  : 1;
    unsigned rsv_1  : 7;
	unsigned RG_TST_PATN_TYPE  : 8;	
	unsigned RG_PTGEN_CLOR_BAR_TH  : 8;
	unsigned rsv_28  : 4;
} LVDS_REG_RGTST_PAT, *PLVDS_REG_RGTST_PAT;

typedef struct
{
    unsigned RG_BD_R  : 8;
    unsigned RG_BD_G  : 8;
	unsigned RG_BD_B  : 8;	
	unsigned rsv_24  : 8;
} LVDS_REG_RGPAT_EDGE, *PLVDS_REG_RGPAT_EDGE;

typedef struct
{
    unsigned RG_PTGEN_R  : 8;
    unsigned RG_PTGEN_G  : 8;
	unsigned RG_PTGEN_B  : 8;	
	unsigned rsv_24  : 8;
} LVDS_REG_RGPAT_SRC, *PLVDS_REG_RGPAT_SRC;

typedef struct
{
    unsigned RG_OUT_CRC_RDY  : 1;
    unsigned RG_OUT_CRC_VALUE  : 16;	
	unsigned rsv_17  : 15;
} LVDS_REG_RGCRC_STATU, *PLVDS_REG_RGCRC_STATU;

typedef struct
{
    LVDS_REG_FMTCNTL        LVDS_FMTCTRL;           // 0000
    LVDS_REG_DATA_SRC   		  LVDS_DATA_SRC;			// 0004
    LVDS_REG_CNTL     LVDS_CTRL;       // 0008
    LVDS_REG_R_SEL     LVDS_R_SEL;       // 000C
    LVDS_REG_G_SEL      LVDS_G_SEL;             // 0010
    LVDS_REG_B_SEL   LVDS_B_SEL; 		// 0014
    LVDS_REG_OUT_CTRL   LVDS_OUT_CTRL; // 0018
    LVDS_REG_CH_SWAP      LVDS_CH_SWAP;             // 001C
    LVDS_REG_CLK_CTRL			  LVDS_CLK_CTRL;			// 0020
    LVDS_REG_CRC_CTRL			LVDS_CRC_CTRL;
	LVDS_REG_RG_CLK				LVDS_RG_CLK;
	LVDS_REG_RG_MON				LVDS_RG_MON;
	LVDS_REG_SOFT_RESET			LVDS_SOFT_RESET;
	LVDS_REG_RGCHL_SRC0			LVDS_RGCHL_SCR0;
	LVDS_REG_RGCHL_SRC1			LVDS_RGCHL_SCR1;
	LVDS_REG_PATN_TOTAL			LVDS_PATN_TOTAL;
	LVDS_REG_PATN_WIDTH			LVDS_PATN_WIDTH;
	LVDS_REG_PATN_START			LVDS_PATN_START;
	LVDS_REG_PATN_ACTIVE		LVDS_PATN_ACTIVE;
	LVDS_REG_RGTST_PAT			LVDS_RGTST_PAT;
	LVDS_REG_RGPAT_EDGE			LVDS_RGPAT_EDGE;
	LVDS_REG_RGPAT_SRC			LVDS_RGPAT_SRC;
	LVDS_REG_RGCRC_STATU		LVDS_RGCRC_STATU;
} volatile LVDS_TX_REGS, *PLVDS_TX_REGS;


/* LVDS ANA REG defines*/
typedef struct
{
    unsigned LVDSTX_ANA_TVO  : 4;
    unsigned LVDSTX_ANA_TVCM    : 4;
    unsigned LVDSTX_ANA_NSRC     : 4;
    unsigned LVDSTX_ANA_PSRC     : 4;
    unsigned LVDSTX_ANA_BIAS_SEL     : 2;	
    unsigned LVDSTX_ANA_R_TERM     : 2;
    unsigned LVDSTX_ANA_SEL_CKTST     : 1;	
    unsigned LVDSTX_ANA_SEL_MERGE     : 1;
    unsigned LVDSTX_ANA_LDO_EN     : 1;
    unsigned LVDSTX_ANA_BIAS_EN     : 1;	
    unsigned LVDSTX_ANA_SER_ABIST_EN     : 1;	
    unsigned LVDSTX_ANA_SER_ABEDG_EN     : 1;
    unsigned LVDSTX_ANA_SER_BIST_TOG     : 1;

    unsigned rsv_27        : 5;	
} LVDS_ANA_REG_CTL2, *PLVDS_ANA_REG_CTL2;


typedef struct
{
    unsigned LVDSTX_ANA_IMP_TEST_EN  : 5;
    unsigned LVDSTX_ANA_EXT_EN    : 5;
    unsigned LVDSTX_ANA_DRV_EN     : 5;
    unsigned rsv_15        : 1;	
    unsigned LVDSTX_ANA_SER_DIN_SEL     : 1;
    unsigned LVDSTX_ANA_SER_CLKDIG_INV     : 1;
    unsigned rsv_18        : 2;	
    unsigned LVDSTX_ANA_SER_DIN     : 10;
    unsigned rsv_30        : 2;	
} LVDS_ANA_REG_CTL3, *PLVDS_ANA_REG_CTL3;



typedef struct
{
    unsigned LVDS_VPLL_RESERVE  : 2;
    unsigned LVDS_VPLL_LVROD_EN   : 1;
    unsigned rsv_3        : 1;		
    unsigned LVDS_VPLL_RST_DLY     : 2;
    unsigned LVDS_VPLL_FBKSEL     : 2;
    unsigned LVDS_VPLL_DDSFBK_EN     : 1;
    unsigned rsv_9        : 3;	
    unsigned LVDS_VPLL_FBKDIV     : 7;
    unsigned rsv_19        : 1;		
    unsigned LVDS_VPLL_PREDIV     : 2;
    unsigned LVDS_VPLL_POSDIV     : 2;
    unsigned LVDS_VPLL_VCO_DIV_SEL     : 1;
    unsigned LVDS_VPLL_BLP     : 1;
    unsigned LVDS_VPLL_BP    : 1;
    unsigned LVDS_VPLL_BR     : 1;
    unsigned rsv_28        : 4;
} LVDS_VPLL_REG_CTL1, *PLVDS_VPLL_REG_CTL1;


typedef struct
{
    unsigned LVDS_VPLL_DIVEN  : 3;
    unsigned rsv_3        : 1;		
    unsigned LVDS_VPLL_MONCK_EN     : 1;
    unsigned LVDS_VPLL_MONVC_EN     : 1;
    unsigned LVDS_VPLL_MONREF_EN     : 1;
    unsigned LVDS_VPLL_EN     : 1;
    unsigned LVDS_VPLL_TXDIV1     : 2;
    unsigned LVDS_VPLL_TXDIV2     : 2;
    unsigned LVDS_VPLL_LVDS_EN     : 1;
    unsigned LVDS_VPLL_LVDS_DPIX_DIV2     : 1;
    unsigned LVDS_VPLL_TTL_EN     : 1;
    unsigned rsv_15        : 1;		
    unsigned LVDS_VPLL_TTLDIV     : 2;
    unsigned LVDS_VPLL_TXSEL     : 1;
    unsigned rsv_19        : 1;		
    unsigned LVDS_VPLL_RESERVE1     : 3;
    unsigned LVDS_VPLL_CLK_SEL     : 1;
    unsigned rsv_24        : 8;	
} LVDS_VPLL_REG_CTL2, *PLVDS_VPLL_REG_CTL2;

typedef struct
{
	unsigned LVDS_TVE_BGREF_PWD	: 1;
	unsigned LVDS_TVE_TRIM_VAL	: 5;
	unsigned rsv_6	: 2;
	unsigned LVDS_TVE_VDAC_REV	: 8;
	unsigned LVDS_TVE_VDAC_PWD	: 1;
	unsigned LVDS_TVE_PBS_TST_EN	: 1;
	unsigned LVDS_TVE_VDAC_TST_EN	: 1;
	unsigned rsv_19: 13;
} LVDS_TVE_REG_CTL0, *PLVDS_TVE_REG_CTL0;

typedef struct
{
    UINT32        LVDSTX_ANA_CTL1;           // 0000
    LVDS_ANA_REG_CTL2   		  LVDSTX_ANA_CTL2;			// 0004
    LVDS_ANA_REG_CTL3     LVDSTX_ANA_CTL3;       // 0008
    UINT32     LVDSTX_ANA_CTL4;       // 000C
    UINT32      LVDSTX_ANA_CTL5;             // 0010
    LVDS_VPLL_REG_CTL1   LVDS_VPLL_CTL1; 		// 0014
    LVDS_VPLL_REG_CTL2   LVDS_VPLL_CTL2; // 0018
    LVDS_TVE_REG_CTL0			LVDS_TVE_CTL0;
} volatile LVDS_ANA_REGS, *PLVDS_ANA_REGS;


#ifndef BUILD_UBOOT
STATIC_ASSERT(0x0018 == offsetof(DPI_REGS, SIZE));
STATIC_ASSERT(0x0038 == offsetof(DPI_REGS, BG_COLOR));
STATIC_ASSERT(0x0070 == offsetof(DPI_REGS, TGEN_VWIDTH_RODD));
STATIC_ASSERT(0x00AC == offsetof(DPI_REGS, ESAV_CODE_SET1));
#endif
#ifdef __cplusplus
}
#endif
#endif				/* __DPI_REG_H__ */
