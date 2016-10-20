/*
 * Copyright (c) 2014 MediaTek Inc.
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

#ifndef _SAPPHIRE_AREG_H_
#define _SAPPHIRE_AREG_H_

/* Last updated on 2014/08/20, 14:32. The version : 459 */




/**********************************************************************************************************/
/* g3d version IDs */
#define REG_AREG_G3D_ID                0x0000	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_REVISION_ID           0x000000ff	/* [default:][perf:] revision ID */
#define SFT_AREG_REVISION_ID           0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VERSION_ID            0x0000ff00	/* [default:][perf:] version ID */
#define SFT_AREG_VERSION_ID            8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEVICE_ID             0xffff0000	/* [default:][perf:] device ID */
#define SFT_AREG_DEVICE_ID             16

/**********************************************************************************************************/
/* g3d reset */
#define REG_AREG_G3D_SWRST             0x0004	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_G3D_SWRST             0x00000001	/* [default:1'b0][perf:] sw reset  g3d engine */
#define SFT_AREG_G3D_SWRST             0

/**********************************************************************************************************/
/* g3d engin fire */
#define REG_AREG_G3D_ENG_FIRE          0x0008	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_G3D_ENG_FIRE          0x00000001	/* [default:1'b0][perf:] g3d engine filre, auto clear */
#define SFT_AREG_G3D_ENG_FIRE          0

/**********************************************************************************************************/
/* g3d mcu set */
#define REG_AREG_MCU2G3D_SET           0x000c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MCU2G3D_SET           0x00000001	/* [default:1'b0][perf:] mcu to g3d engine set to control the
							fdq read/write pointer and htbuf read pointer, auto clear */
#define SFT_AREG_MCU2G3D_SET           0

/**********************************************************************************************************/
/* g3d idle expire counter */
#define REG_AREG_G3D_IDLE_EXPIRE_CNT   0x0010	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_G3D_IDLE_EXPIRE_CNT   0x0000ffff	/* [default:16'd64][perf:] the g3d idle timer expire credit
								counter */
#define SFT_AREG_G3D_IDLE_EXPIRE_CNT   0

/**********************************************************************************************************/
/* g3d_reg apb time out counter */
#define REG_AREG_G3D_TIME_OUT_CNT      0x0014	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_G3D_TIME_OUT_CNT      0xffffffff	/* [default:32'h00001000][perf:] for g3d hang used */
#define SFT_AREG_G3D_TIME_OUT_CNT      0

/**********************************************************************************************************/
/* g3d_ token 0 */
#define REG_AREG_G3D_TOKEN0            0x0020	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_G3D_TOKEN0            0x00000001	/* [default:1'b1][perf:] token 0 */
#define SFT_AREG_G3D_TOKEN0            0

/**********************************************************************************************************/
/* g3d_ token 1 */
#define REG_AREG_G3D_TOKEN1            0x0024	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_G3D_TOKEN1            0x00000001	/* [default:1'b1][perf:] token 1 */
#define SFT_AREG_G3D_TOKEN1            0

/**********************************************************************************************************/
/* g3d_ token 2 */
#define REG_AREG_G3D_TOKEN2            0x0028	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_G3D_TOKEN2            0x00000001	/* [default:1'b1][perf:] token 2 */
#define SFT_AREG_G3D_TOKEN2            0

/**********************************************************************************************************/
/* g3d_ token 3 */
#define REG_AREG_G3D_TOKEN3            0x002c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_G3D_TOKEN3            0x00000001	/* [default:1'b1][perf:] token 3 */
#define SFT_AREG_G3D_TOKEN3            0

/**********************************************************************************************************/
/* g3d clock gating control */
#define REG_AREG_G3D_CGC_SETTING       0x0080	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_G3D_CGC_EN            0x00000001 /* [default:1'b1][perf:] g3d global clock gating control enable */
#define SFT_AREG_G3D_CGC_EN            0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_GRP_CGC_EN            0x0000003e	/* [default:5'h1f][perf:] eclk group gating control enable */
#define SFT_AREG_GRP_CGC_EN            1

/**********************************************************************************************************/
/* g3d sub-module clock gating control 0 */
#define REG_AREG_G3D_CGC_SUB_SETTING0  0x0084	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC0_EN          0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating control enable 0 */
#define SFT_AREG_ECLK_CGC0_EN          0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 1 */
#define REG_AREG_G3D_CGC_SUB_SETTING1  0x0088	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC1_EN          0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating control enable 1 */
#define SFT_AREG_ECLK_CGC1_EN          0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 2 */
#define REG_AREG_G3D_CGC_SUB_SETTING2  0x008c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC2_EN          0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating control enable 2 */
#define SFT_AREG_ECLK_CGC2_EN          0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 3 */
#define REG_AREG_G3D_CGC_SUB_SETTING3  0x0090	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC3_EN          0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating control enable 3 */
#define SFT_AREG_ECLK_CGC3_EN          0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 4 */
#define REG_AREG_G3D_CGC_SUB_SETTING4  0x0094	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC4_EN          0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating control enable 4 */
#define SFT_AREG_ECLK_CGC4_EN          0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 5 */
#define REG_AREG_G3D_CGC_SUB_SETTING5  0x0098	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC5_EN          0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating control enable 5 */
#define SFT_AREG_ECLK_CGC5_EN          0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 6 */
#define REG_AREG_G3D_CGC_SUB_SETTING6  0x009c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC6_EN          0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating control enable 6 */
#define SFT_AREG_ECLK_CGC6_EN          0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 7 */
#define REG_AREG_G3D_CGC_SUB_SETTING7  0x00a0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC7_EN          0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating control enable 7 */
#define SFT_AREG_ECLK_CGC7_EN          0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 8 */
#define REG_AREG_G3D_CGC_SUB_SETTING8  0x00a4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC8_EN          0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating control enable 8 */
#define SFT_AREG_ECLK_CGC8_EN          0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 9 */
#define REG_AREG_G3D_CGC_SUB_SETTING9  0x00a8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC9_EN          0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating control enable 9 */
#define SFT_AREG_ECLK_CGC9_EN          0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 10 */
#define REG_AREG_G3D_CGC_SUB_SETTING10 0x00ac	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC10_EN         0xffffffff	/* [default:32'hffffffff][perf:] eclk clock gating
								control enable 10 */
#define SFT_AREG_ECLK_CGC10_EN         0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 11 */
#define REG_AREG_G3D_CGC_SUB_SETTING11 0x00b0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC11_EN         0xffffffff	/* [default:32'hffffffff][perf:] eclk clock gating
								control enable 11 */
#define SFT_AREG_ECLK_CGC11_EN         0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 12 */
#define REG_AREG_G3D_CGC_SUB_SETTING12 0x00b4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC12_EN         0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating
								control enable 12 */
#define SFT_AREG_ECLK_CGC12_EN         0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 13 */
#define REG_AREG_G3D_CGC_SUB_SETTING13 0x00b8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC13_EN         0xffffffff /* [default:32'hffffffff][perf:] eclk clock gating
								control enable 13 */
#define SFT_AREG_ECLK_CGC13_EN         0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 14 */
#define REG_AREG_G3D_CGC_SUB_SETTING14 0x00bc	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC14_EN         0xffffffff	/* [default:32'hffffffff][perf:] eclk clock gating
								control enable 14 */
#define SFT_AREG_ECLK_CGC14_EN         0

/**********************************************************************************************************/
/* g3d sub-module clock gating control 15 */
#define REG_AREG_G3D_CGC_SUB_SETTING15 0x00c0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECLK_CGC15_EN         0xffffffff	/* [default:32'hffffffff][perf:] eclk clock gating
							control enable 15 */
#define SFT_AREG_ECLK_CGC15_EN         0

/**********************************************************************************************************/
/* cq ht-buffer  start address */
#define REG_AREG_CQ_HTBUF_START_ADDR   0x0100	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_HTBUF_START_ADDR   0xffffffff	/* [default:32'h0][perf:] cmdq(HT buffer) start address,
								must be 16 bytes alignment */
#define SFT_AREG_CQ_HTBUF_START_ADDR   0

/**********************************************************************************************************/
/* cq ht-buffer end address */
#define REG_AREG_CQ_HTBUF_END_ADDR     0x0104	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_HTBUF_END_ADDR     0xffffffff	/* [default:32'b0][perf:] cmdq(HT buffer) end address,
								must be 16bytes alignment */
#define SFT_AREG_CQ_HTBUF_END_ADDR     0

/**********************************************************************************************************/
/* cq ht-buffer read pointer */
#define REG_AREG_CQ_HTBUF_RPTR         0x0108	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_HTBUF_RPTR         0xffffffff /* [default:32'b0][perf:] cmdq (ring buffer ):
							g3d read head /tail buffer read pointer */
#define SFT_AREG_CQ_HTBUF_RPTR         0

/**********************************************************************************************************/
/* cq ht-buffer write pointer */
#define REG_AREG_CQ_HTBUF_WPTR         0x010c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_HTBUF_WPTR         0xffffffff /* [default:32'b0][perf:] cmdq (ring buffer ):
							sw write head/tail buffer write pointer */
#define SFT_AREG_CQ_HTBUF_WPTR         0

/**********************************************************************************************************/
/* cq queue buffer start address */
#define REG_AREG_CQ_QBUF_START_ADDR    0x0110	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_QBUF_START_ADDR    0xffffffff	/* [default:32'b0][perf:] cq queue buffer base start addr */
#define SFT_AREG_CQ_QBUF_START_ADDR    0

/**********************************************************************************************************/
/* cq queue buffer end address */
#define REG_AREG_CQ_QBUF_END_ADDR      0x0114	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_QBUF_END_ADDR      0xffffffff	/* [default:32'b0][perf:] cq queue buffer base end addr */
#define SFT_AREG_CQ_QBUF_END_ADDR      0

/**********************************************************************************************************/
/* cq control update setting */
#define REG_AREG_CQ_CTRL               0x0118	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_QLD_UPD            0x00000001 /* [default:1'b0][perf:] Update the head address set this trigger */
#define SFT_AREG_CQ_QLD_UPD            0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_HTBUF_RPTR_UPD     0x00000002 /* [default:1'b0][perf:] Update the head address set this trigger */
#define SFT_AREG_CQ_HTBUF_RPTR_UPD     1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_HTBUF_WPTR_UPD     0x00000004 /* [default:1'b0][perf:] Update the tail address set this trigger */
#define SFT_AREG_CQ_HTBUF_WPTR_UPD     2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_RST                0x00000010	/* [default:1'b0][perf:] command queue sw reset */
#define SFT_AREG_CQ_RST                4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_ECC_EN             0x00000020	/* [default:1'b0][perf:] command queue error code check */
#define SFT_AREG_CQ_ECC_EN             5

/**********************************************************************************************************/
/* cq context switch mode  */
#define REG_AREG_CQ_CONTEXT_CTRL       0x011c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_PMT_MD             0x00000001	/* [default:1'b0][perf:] command queue preempt mode */
						  /* 1'b0: normal case */
						  /* 1'b1: preempt at frame boundary */
#define SFT_AREG_CQ_PMT_MD             0

/**********************************************************************************************************/
/* g3d idle status0 */
#define REG_AREG_PA_IDLE_STATUS0       0x0180	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_IDLE_BUS0          0xffffffff	/* [default:][perf:] g3d idle status */
						  /*  */
						  /* [0]:g3d idle timer expire */
						  /* [1]:g3d all idle */
						  /* [2]:g3d idle */
						  /* [3]:pa idle */
						  /* [4]:sf2pa_vp0_idle */
						  /* [5]:cf2pa_vp0_idle */
						  /* [6]:vs2pa_vp0_idle */
						  /* [7]:vl2ap_vp0_idle */
						  /* [8]:vx2pa_vp1_idle */
						  /* [9]:bw2pa_vp1_idle */
						  /* [10]:vs2pa_vp1_idle */
						  /* [11]:vl2pa_vp1_idle */
						  /* [12]:tx2pa_pp_idle */
						  /* [13]:zp2pa_pp_idle */
						  /* [14]:cp2pa_pp_idle */
						  /* [15]:fs2pa_pp_idle */
						  /* [16]:xy2pa_pp_idle */
						  /* [17]:sb2pa_pp_idle */
						  /* [18]:lv2pa_pp_idle */
						  /* [19]:cb2pa_pp_idle */
						  /* [20]:vc2pa_pp_idle */
						  /* [21]:vx2pa_pp_idle */
						  /* [23:22]:reserved */
						  /* [24]:cq2pa_wr_idle */
						  /* [25]:cq2pa_rd_idle */
						  /* [26]:gl2pa_pp_idle */
						  /* [27]:gl2pa_vp_idle */
						  /* [31:28]:reserved */
#define SFT_AREG_PA_IDLE_BUS0          0

#define VAL_PA_IDLE_ST0_G3D_IDLE_TIMER_EXPIRE   (0x1 << 0)
#define VAL_PA_IDLE_ST0_G3D_ALL_IDLE            (0x1 << 1)
#define VAL_PA_IDLE_ST0_G3D_IDLE                (0x1 << 2)
#define VAL_PA_IDLE_ST0_PA_IDLE                 (0x1 << 3)
#define VAL_PA_IDLE_ST0_SF2PA_VP0_IDLE          (0x1 << 4)
#define VAL_PA_IDLE_ST0_CF2PA_VP0_IDLE          (0x1 << 5)
#define VAL_PA_IDLE_ST0_VS2PA_VP0_IDLE          (0x1 << 6)
#define VAL_PA_IDLE_ST0_VL2AP_VP0_IDLE          (0x1 << 7)
#define VAL_PA_IDLE_ST0_VX2PA_VP1_IDLE          (0x1 << 8)
#define VAL_PA_IDLE_ST0_BW2PA_VP1_IDLE          (0x1 << 9)
#define VAL_PA_IDLE_ST0_VS2PA_VP1_IDLE          (0x1 << 10)
#define VAL_PA_IDLE_ST0_VL2PA_VP1_IDLE          (0x1 << 11)
#define VAL_PA_IDLE_ST0_TX2PA_PP_IDLE           (0x1 << 12)
#define VAL_PA_IDLE_ST0_ZP2PA_PP_IDLE           (0x1 << 13)
#define VAL_PA_IDLE_ST0_CP2PA_PP_IDLE           (0x1 << 14)
#define VAL_PA_IDLE_ST0_FS2PA_PP_IDLE           (0x1 << 15)
#define VAL_PA_IDLE_ST0_XY2PA_PP_IDLE           (0x1 << 16)
#define VAL_PA_IDLE_ST0_SB2PA_PP_IDLE           (0x1 << 17)
#define VAL_PA_IDLE_ST0_LV2PA_PP_IDLE           (0x1 << 18)
#define VAL_PA_IDLE_ST0_CB2PA_PP_IDLE           (0x1 << 19)
#define VAL_PA_IDLE_ST0_VC2PA_PP_IDLE           (0x1 << 20)
#define VAL_PA_IDLE_ST0_VX2PA_PP_IDLE           (0x1 << 21)
#define VAL_PA_IDLE_ST0_CQ2PA_WR_IDLE           (0x1 << 24)
#define VAL_PA_IDLE_ST0_CQ2PA_RD_IDLE           (0x1 << 25)
#define VAL_PA_IDLE_ST0_GL2PA_PP_IDLE           (0x1 << 26)
#define VAL_PA_IDLE_ST0_GL2PA_VP_IDLE           (0x1 << 27)

/**********************************************************************************************************/
/* g3d idle status1 */
#define REG_AREG_PA_IDLE_STATUS1       0x0184	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_IDLE_BUS1          0xffffffff	/* [default:][perf:] g3d idle status */
						  /*  */
						  /* [0]:cc2pa_pp_l1_idle */
						  /* [1]:cz2pa_pp_l1_idle */
						  /* [2]:zc2pa_pp_l1_idle */
						  /* [3]:ux2pa_vp_l1_idle */
						  /* [4]:hc2pa_vp1_l1_idle */
						  /* [5]:cz2pa_vp1_l1_idle */
						  /* [6]:mx2pa_l2_idle */
						  /* [7]:reserved */
						  /* [8]:opencl2pa_vp_idle */
						  /* [9]:clin2pa_vp_idle */
						  /* [10]:vl2pa_vp1_bidle */
						  /* [11]:vx2pa_vp1_bidle */
						  /* [31:12]:reserved */
#define SFT_AREG_PA_IDLE_BUS1          0

#define VAL_PA_IDLE_ST1_CC2PA_PP_L1_IDLE    (0x1 << 0)
#define VAL_PA_IDLE_ST1_CZ2PA_PP_L1_IDLE    (0x1 << 1)
#define VAL_PA_IDLE_ST1_ZC2PA_PP_L1_IDLE    (0x1 << 2)
#define VAL_PA_IDLE_ST1_UX2PA_VP_L1_IDLE    (0x1 << 3)
#define VAL_PA_IDLE_ST1_HC2PA_VP1_L1_IDLE   (0x1 << 4)
#define VAL_PA_IDLE_ST1_CZ2PA_VP1_L1_IDLE   (0x1 << 5)
#define VAL_PA_IDLE_ST1_MX2PA_L2_L1_IDLE    (0x1 << 6)
#define VAL_PA_IDLE_ST1_OPENCL2PA_VP_IDLE   (0x1 << 8)
#define VAL_PA_IDLE_ST1_CLIN2PA_VP_IDLE     (0x1 << 9)
#define VAL_PA_IDLE_ST1_VL2PA_VP1_BIDLE     (0x1 << 10)
#define VAL_PA_IDLE_ST1_VX2PA_VP1_BIDLE     (0x1 << 11)

/**********************************************************************************************************/
/* queue status */
#define REG_AREG_QUEUE_STATUS          0x0188	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_IDLE               0x00000001	/* [default:][perf:] cmdq is idle */
#define SFT_AREG_CQ_IDLE               0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_QLDING             0x00000002	/* [default:][perf:] cq is loading  the queue buffer,
								for SW debug mode monitor */
#define SFT_AREG_CQ_QLDING             1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ECC_HTBUF_RPTR        0xfffffff0	/* [default:][perf:] read the error htbuffer read pointer
								when ecc happen */
#define SFT_AREG_ECC_HTBUF_RPTR        4

/**********************************************************************************************************/
/* cq done ht-buffer read pointer */
#define REG_AREG_CQ_DONE_HTBUF_RPTR    0x018c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_DONE_HTBUF_RPTR    0xffffffff /* [default:][perf:] cmdq (ring buffer )head pointer read back */
						  /* If the head_rd not equal the tail, the */
#define SFT_AREG_CQ_DONE_HTBUF_RPTR    0

/**********************************************************************************************************/
/* sw flush ltc set */
#define REG_AREG_PA_FLUSH_LTC_SET      0x0190	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_FLUSH_LTC_EN       0x00000001	/* [default:1'b0][perf:] sw flushes the g3d ltc
								according to apb bus. */
#define SFT_AREG_PA_FLUSH_LTC_EN       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_SYNC_SW_EN         0x00000002	/* [default:1'b0][perf:] sw finish the program
							condition(for DWP), and set this signal tell g3d keep going */
#define SFT_AREG_PA_SYNC_SW_EN         1

/**********************************************************************************************************/
/* hw  flush ltc done */
#define REG_AREG_PA_FLUSH_LTC_DONE     0x0194	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_FLUSH_LTC_DONE     0x00000001	/* [default:][perf:] g3d will set this bit 1, after flush LTC */
#define SFT_AREG_PA_FLUSH_LTC_DONE     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_SYNC_SW_DONE       0x00000002	/* [default:][perf:] g3d stop g3d engine and wait sw program
								"pa_sync_sw_set" to release , and keep going. */
#define SFT_AREG_PA_SYNC_SW_DONE       1

/**********************************************************************************************************/
/* pa frame data queue buffer base */
#define REG_AREG_PA_FDQBUF_BASE        0x0198	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_FDQBUF_BASE        0xffffffff	/* [default:32'b0][perf:] the frame base return data buffer
								base,16 byte alignment */
#define SFT_AREG_PA_FDQBUF_BASE        0

/**********************************************************************************************************/
/* pa frame data queue buffer size */
#define REG_AREG_PA_FDQBUF_SIZE        0x019c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_FDQBUF_SIZE        0x001fffff	/* [default:21'b0][perf:] the frame base return
								data buffer size , max size 32MB,
								the unit is 8 Byte(64bits!!) */
#define SFT_AREG_PA_FDQBUF_SIZE        0

/**********************************************************************************************************/
/* pa frame data queue buffer hw write pointer */
#define REG_AREG_PA_FDQ_HWWPTR         0x01a0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_FDQ_HWWPTR         0x007ffff8	/* [default:][perf:] the frame base return data buffer
								hw write pointer, the unit is 8byte ,
								need set "mcu2g3d_set" before reading */
#define SFT_AREG_PA_FDQ_HWWPTR         3

/**********************************************************************************************************/
/* pa frame data queue buffer sw read pointer */
#define REG_AREG_PA_FDQ_SWRPTR         0x01a4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_FDQ_SWRPTR         0x007ffff8	/* [default:20'b0][perf:] the frame base return data buffer
								sw read pointer,the unit is 8byte */
#define SFT_AREG_PA_FDQ_SWRPTR         3

/**********************************************************************************************************/
/* roll back the pa frame data queue buffer hw write pointer update */
#define REG_AREG_PA_FDQ_HWWPTR_RB      0x01a8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_FDQ_HWWPTR_RB_UPD  0x00000001	/* [default:1'b0][perf:] roll back the pa frame data queue
								buffer hw write pointer update */
#define SFT_AREG_PA_FDQ_HWWPTR_RB_UPD  0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_FDQ_HWWPTR_RB      0x007ffff8	/* [default:20'b0][perf:] roll back the frame base return
								data buffer hw write pointer, the unit is 8byte ,
								need set "mcu2g3d_set" to set */
#define SFT_AREG_PA_FDQ_HWWPTR_RB      3

/**********************************************************************************************************/
/* read command queue task id   */
#define REG_AREG_TASKID                0x01ac	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_TASKID             0x000000ff	/* [default:][perf:] read the command task id */
#define SFT_AREG_CQ_TASKID             0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_VP_TASKID          0x0000ff00	/* [default:][perf:] read the vp frame task id */
#define SFT_AREG_PA_VP_TASKID          8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_PP_TASKID          0x00ff0000	/* [default:][perf:] read the pp frame task id */
#define SFT_AREG_PA_PP_TASKID          16

/**********************************************************************************************************/
/* read command queue frame head */
#define REG_AREG_CQ_FMHEAD             0x01b0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_FMHEAD             0xffffffff	/* [default:][perf:] read the command frame head */
#define SFT_AREG_CQ_FMHEAD             0

/**********************************************************************************************************/
/* read command queue frame tail */
#define REG_AREG_CQ_FMTAIL             0x01b4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_FMTAIL             0xffffffff	/* [default:][perf:] read the command frame tail */
#define SFT_AREG_CQ_FMTAIL             0

/**********************************************************************************************************/
/* read vp pipe frame id  */
#define REG_AREG_PA_VP_FRAMEID         0x01b8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_VP_FRAMEID         0xffffffff	/* [default:][perf:] read vp pipe frame id */
#define SFT_AREG_PA_VP_FRAMEID         0

/**********************************************************************************************************/
/* read pp pipe frame id */
#define REG_AREG_PA_PP_FRAMEID         0x01bc	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PA_PP_FRAMEID         0xffffffff	/* [default:][perf:] read pp pipe frame id */
#define SFT_AREG_PA_PP_FRAMEID         0

/**********************************************************************************************************/
/* pa debug rd */
#define REG_AREG_PA_DEBUG_RD           0x01c0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_LIST_ID         0x00000fff /* [default:][perf:] report the g3d engine list to SW, if hang */
#define SFT_AREG_DEBUG_LIST_ID         0

/**********************************************************************************************************/
/* debug mode select */
#define REG_AREG_DEBUG_MOD_SEL         0x01c4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_EN                0x00000001	/* [default:1'b0][perf:] g3d debug enable */
#define SFT_AREG_DBG_EN                0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_TOP_SEL           0x00000ff0	/* [default:8'h0][perf:] g3d debug module select */
						  /* 0:g3d  module stall status */
						  /* 1:reserved */
						  /* 2:reserved */
						  /* 3:vl_vp0 */
						  /* 4:reserved */
						  /* 5:cf_vp0 */
						  /* 6:sf_vp0 */
						  /* 7:reserved */
						  /* 8:vl_vp1 */
						  /* 9:reserved */
						  /* 10:bw_vp1 */
						  /* 11:vx_vp1 */
						  /* 12:reserve */
						  /* 13:vx_pp */
						  /* 14:cb_pp */
						  /* 15:sb_pp */
						  /* 16:xy_pp */
						  /* 17:reserved */
						  /* 18:cp_pp */
						  /* 19:zp_pp */
						  /* 20:tx_ pp */
						  /* 21:reserve */
						  /* 22:cc_pp */
						  /* 23:zc_pp */
						  /* 24:mx_eng */
						  /* 25:mx_cac */
						  /* 26:pm */
						  /* others:reserved */
#define SFT_AREG_DBG_TOP_SEL           4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_SUB_SEL           0x000ff000	/* [default:8'h0][perf:] 8 bits for per module debug select */
#define SFT_AREG_DBG_SUB_SEL           12

/**********************************************************************************************************/
/* debug bus */
#define REG_AREG_DEBUG_BUS             0x01c8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS               0xffffffff	/* [default:][perf:] 32 bits debug bus */
#define SFT_AREG_DBG_BUS               0

/**********************************************************************************************************/
/* cq/mx bypass/mx dcm */
#define REG_AREG_BYPASS_SET            0x01cc	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_FIRE_BY_LIST          0x00000001	/* [default:1'b0][perf:] g3d fire list by list */
#define SFT_AREG_FIRE_BY_LIST          0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_SNGBL_EN           0x00000002	/* [default:1'b0][perf:] command queue set
							single mode request , no burst */
#define SFT_AREG_CQ_SNGBL_EN           1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_PSEUDOBYPASS_EN    0x00000004	/* [default:1'b0][perf:] pseudo-bypass enable */
						  /* 0:normal mode; */
						  /* 1:pseudo-bypass mode */
#define SFT_AREG_MX_PSEUDOBYPASS_EN    2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_AXI_INTERLEAVE_EN  0x00000008	/* [default:1'b0][perf:] axi interleave enable */
						  /* 0:axi data in-order mode; */
						  /* 1:axi data interleaving mode */
#define SFT_AREG_MX_AXI_INTERLEAVE_EN  3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_DCM_LEVEL          0x00000030	/* [default:2'b0][perf:] mx clk gating level setting */
						  /* 00: no performance loss, clk gating */
						  /* 01:  little performance loss, clk gating */
#define SFT_AREG_MX_DCM_LEVEL          4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_DATAPRERDY_DISABLE 0x00000040	/* [default:1'b0][perf:] mx clk gating safe mode setting */
						  /* 0: have clk gating */
						  /* 1: always dataprerdy = 1, no clk gating */
#define SFT_AREG_MX_DATAPRERDY_DISABLE 6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_PREACK_DISABLE     0x00000080	/* [default:1'b0][perf:] mx clk gating safe mode setting */
						  /* 0: have clk gating */
						  /* 1: always preack = 1, no clk gating */
#define SFT_AREG_MX_PREACK_DISABLE     7
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_AXIID_SET          0x00000300	/* [default:2'b00][perf:] mfg axi id number from 32 to 4 */
						  /* 00: 32 AXI ID, 01: 16 AXI ID */
						  /* 10:  8 AXI ID, 11: 4 AXI ID */
#define SFT_AREG_MX_AXIID_SET          8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_AXI_2CH_GRAIN      0x0003fc00	/* [default:8'h10][perf:] axi 2ch use */
						  /* [6:4] granule setting, 0:64B, 1:128B, ..7:8KB */
						  /* [2:0] 2 ch weighting, ch1 has N/4 weighting */
#define SFT_AREG_MX_AXI_2CH_GRAIN      10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_AXI_EVEN_WBL       0x00040000	/* [default:1'h0][perf:] DDR4 AXI use */
						  /* 0 : normal, 1: wr burst length is even for DDR4 */
#define SFT_AREG_MX_AXI_EVEN_WBL       18
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_CTRL_RVD           0x00380000	/* [default:3'h0][perf:] mx ctrl space register if need */
						  /* [0]: disable length cache clock off by freg enable bits */
#define SFT_AREG_MX_CTRL_RVD           19

/**********************************************************************************************************/
/* g3d interrupt status result (after mask) */
#define REG_AREG_IRQ_STATUS_MASKED     0x0200	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_IRQ_MASKED            0xffffffff	/* [default:][perf:] interrupt status (masked) */
#define SFT_AREG_IRQ_MASKED            0

#define VAL_IRQ_CQ_PREMPT_FINISH            (1<<0)
#define VAL_IRQ_CQ_RESUME_FINISH            (1<<1)
#define VAL_IRQ_PA_DATA_QUEUE_NOT_EMPTY     (1<<2)
#define VAL_IRQ_PA_FRAME_SWAP               (1<<3)
#define VAL_IRQ_PA_FRAME_END                (1<<4)
#define VAL_IRQ_PA_SYNC_SW                  (1<<5)
#define VAL_IRQ_G3D_HANG                    (1<<6)
#define VAL_IRQ_G3D_MMCE_FRAME_FLUSH        (1<<7)
#define VAL_IRQ_G3D_MMCE_FRAME_END          (1<<8)
#define VAL_IRQ_CQ_ECC                                         (1<<9)
#define VAL_IRQ_MX_DBG                                        (1<<10)
#define VAL_IRQ_PWC_SNAP                                        (1<<11)
#define VAL_IRQ_DFD_DUMP_TRIGGERD                   (1<<12)
#define VAL_IRQ_AREG_ABP_TIMEOUT_HAPPEN     (1<<15)
#define VAL_IRQ_UX_CMN_RESPONSE             (1<<16)
#define VAL_IRQ_UX_DWP0_RESPONSE            (1<<20)
#define VAL_IRQ_UX_DWP1_RESPONSE            (1<<21)

/**********************************************************************************************************/
/* g3d interrupt status mask */
#define REG_AREG_IRQ_STATUS_MASK       0x0204	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_IRQ_MASK              0xffffffff	/* [default:32'hffffffff][perf:] interrupt mask */
						  /* 0: enable interrupt */
						  /* 1: disable interrupt */
#define SFT_AREG_IRQ_MASK              0

/**********************************************************************************************************/
/* g3d interrupt status raw data (before mask), and write one clear */
#define REG_AREG_IRQ_STATUS_RAW        0x0208	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_IRQ_RAW               0xffffffff	/* [default:32'h0][perf:] interrupt status (raw data) */
						  /* [0]: cq preempt finish */
						  /* [1]: cq resume finish */
						  /* [2]: pa frame base data queue not empty */
						  /* [3]: pa frame swap */
						  /* [4]: pa frame end */
						  /* [5]: pa sync sw */
						  /* [6]: g3d hang */
						  /* [7]: g3d mmce frame flush */
						  /* [8]: g3d mmce frame end */
						  /* [9]: cq error code check */
						  /* [10]: mx debug */
						  /* [11]: pwc snap */
						  /* [12]: DFD Internal dump triggered */
						  /* [13]: pm debug mode */
						  /* [14]: vl negative index happen */
						  /* [15]: areg apb timeout happen */
						  /* [16]: ux_cmn response */
						  /* [20]: ux_dwp0 response */
						  /* [21]: ux_dwp1 response */
#define SFT_AREG_IRQ_RAW               0

/**********************************************************************************************************/
/* vx vertex related control setting */
#define REG_AREG_VX_VTX_SETTING        0x0300	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_VW_BUF_MGN         0x000000ff	/* [default:8'ha][perf:] varying ring buffer margin */
						  /* margin = (vx_vary_buf_mgn) K byte */
#define SFT_AREG_VX_VW_BUF_MGN         0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_BS_HD_TEST         0x00000100	/* [default:1'b0][perf:] test mode for header no. execeed */
#define SFT_AREG_VX_BS_HD_TEST         8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_LONGSTAY_EN        0x00000200	/* [default:1'b1][perf:] longstay/release enable for imm mode */
#define SFT_AREG_VX_LONGSTAY_EN        9
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_BKJMP_EN           0x00000c00	/* [default:2'h0][perf:] defer bank jump enable */
						  /* 0x : disable */
						  /* 10 : jump with 32 primitves */
						  /* 11 : jump with 16 primitives */
#define SFT_AREG_VX_BKJMP_EN           10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_BS_BOFF_MODE       0x00007000	/* [default:3'h0][perf:] boff mode */
						  /* [0] : for header */
						  /* [1] : for table */
						  /* [2] : for block */
						  /* 0 : stop gradurally */
						  /* 1 : stop immediately */
#define SFT_AREG_VX_BS_BOFF_MODE       12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_DUMMY0             0xffff8000	/* [default:17'h0][perf:] dummy registers */
#define SFT_AREG_VX_DUMMY0             15

/**********************************************************************************************************/
/* vx defer related control setting */
#define REG_AREG_VX_DEFER_SETTING      0x0304	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_BINTBXR_4X         0x00000001	/* [default:1'b0][perf:] bin table extend
								always read 4 times for mbin */
						  /* 0: backward  if possible */
						  /* 1: always 4 times */
#define SFT_AREG_VX_BINTBXR_4X         0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_BINBKR_4X          0x00000002	/* [default:1'b0][perf:] bin block
								always read 4 times for mbin */
						  /* 0: backward  if possible */
						  /* 1: always 4 times */
#define SFT_AREG_VX_BINBKR_4X          1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_DUMMY1             0x000000fc	/* [default:6'h0][perf:] dummy registers */
#define SFT_AREG_VX_DUMMY1             2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_BINHDR_THR         0x00001f00	/* [default:5'h8][perf:] bin head buffer threshold
								(when lower than this threshold will
								generate reads to let buffer full) */
#define SFT_AREG_VX_BINHDR_THR         8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_BINTBR_THR         0x001f0000	/* [default:5'h8][perf:] bin table buffer threshold */
#define SFT_AREG_VX_BINTBR_THR         16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VX_BINBKR_THR         0x7f000000	/* [default:7'h10][perf:] bin block buffer threshold */
#define SFT_AREG_VX_BINBKR_THR         24

/**********************************************************************************************************/
/* vl misc setting */
#define REG_AREG_VL_SETTING            0x0380	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VL_IDXGEN_WATERLV     0x0000003f	/* [default:6'h20][perf:] vertex index read
								data buffer water level */
#define SFT_AREG_VL_IDXGEN_WATERLV     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VL_INDIRECT_WRAP_MD   0x00000040	/* [default:1'b0][perf:] DrawElementIndirect
								wrap on 32 bits or type bits */
						  /* 0: wrap on 32 bits */
						  /* 1: wrap by type */
#define SFT_AREG_VL_INDIRECT_WRAP_MD   6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VL_TFB_SAFE_MODE      0x00000080	/* [default:1'b0][perf:] TransformFeedback safe mode,
								one 128bit write per component. */
#define SFT_AREG_VL_TFB_SAFE_MODE      7
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VL1_VXPBUF_LOWTH      0x0000ff00	/* [default:8'd48][perf:] VX pbuf low threshold,
								for VL1 flush end condition */
#define SFT_AREG_VL1_VXPBUF_LOWTH      8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VL1_VXPBUF_HIGHTH     0x00ff0000	/* [default:8'd144][perf:] VX pbuf high threshold,
								for VL1 flush start condition */
#define SFT_AREG_VL1_VXPBUF_HIGHTH     16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VL_DUMMY0             0xff000000	/* [default:8'd0][perf:] dummy registers */
#define SFT_AREG_VL_DUMMY0             24

/**********************************************************************************************************/
/* vl0_flush_timer */
#define REG_AREG_VL_FLUSH              0x0384	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VL0_FLUSH_TIMER       0x0000ffff	/* [default:16'h0001][perf:] vl0_flush_timer */
#define SFT_AREG_VL0_FLUSH_TIMER       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VL1_FLUSH_TIMER       0xffff0000	/* [default:16'h0fff][perf:] vl1_flush_timer */
#define SFT_AREG_VL1_FLUSH_TIMER       16

/**********************************************************************************************************/
/* vl_flush_th water level */
#define REG_AREG_VL_FLUSH_TH           0x0388	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VL0_PRIMFIFO_LOWTH    0x000003ff	/* [default:10'd256][perf:] VL primfifo low threshold,
								for VL0 flush end condition */
#define SFT_AREG_VL0_PRIMFIFO_LOWTH    0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VL0_PRIMFIFO_HIGHTH   0x000ffc00	/* [default:10'd510][perf:] VL primfifo high threshold,
								for VL0 flush start condition */
#define SFT_AREG_VL0_PRIMFIFO_HIGHTH   10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VL_DUMMY1             0xfff00000	/* [default:12'd0][perf:] dummy registers */
#define SFT_AREG_VL_DUMMY1             20

/**********************************************************************************************************/
/*  */
#define REG_AREG_CQ_SECURE_CTRL        0x0700	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CQ_SECURITY           0x00000001	/* [default:1'b0][perf:] allocate a security queue */
#define SFT_AREG_CQ_SECURITY           0

/**********************************************************************************************************/
/* mx secure0 start addr */
#define REG_AREG_MX_SECURE0_BASE_ADDR  0x0704	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_SECURE0_BASE_ADDR  0x000fffff /* [default:20'b0][perf:] base address of secure range for Q buffer */
#define SFT_AREG_MX_SECURE0_BASE_ADDR  0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_SECURE0_ENABLE     0x80000000	/* [default:1'b0][perf:] enable this range checking */
#define SFT_AREG_MX_SECURE0_ENABLE     31

/**********************************************************************************************************/
/* mx secure0 end addr */
#define REG_AREG_MX_SECURE0_TOP_ADDR   0x0708	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MX_SECURE0_TOP_ADDR   0x000fffff	/* [default:20'b0][perf:] base address of
								secure range for Q buffer */
#define SFT_AREG_MX_SECURE0_TOP_ADDR   0

/**********************************************************************************************************/
/* read register enable */
#define REG_AREG_RD_REG_EN             0x0e00	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_QREG_EN            0x00000001 /* [default:1'b0][perf:] read queue base registers enable(debug) */
#define SFT_AREG_RD_QREG_EN            0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_RSMINFO_EN         0x00000002	/* [default:1'b0][perf:] read resume info enable */
#define SFT_AREG_RD_RSMINFO_EN         1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_VPFMREG_EN         0x00000004 /* [default:1'b0][perf:] read pp frame base registers enable */
#define SFT_AREG_RD_VPFMREG_EN         2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_PPFMREG_EN         0x00000008 /* [default:1'b0][perf:] read pp frame base registers enable */
#define SFT_AREG_RD_PPFMREG_EN         3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_VP0REG_EN          0x00000010 /* [default:1'b0][perf:] read vp0 list base registers enable */
#define SFT_AREG_RD_VP0REG_EN          4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_VP1REG_EN          0x00000020 /* [default:1'b0][perf:] read vp1 list base registers enable */
#define SFT_AREG_RD_VP1REG_EN          5
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_CBREG_EN           0x00000040 /* [default:1'b0][perf:] read pp cb  list base registers enable */
#define SFT_AREG_RD_CBREG_EN           6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_XYREG_EN           0x00000080 /* [default:1'b0][perf:] read pp xy list base registers enable */
#define SFT_AREG_RD_XYREG_EN           7
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_FSREG_EN           0x00000100 /* [default:1'b0][perf:] read pp fs list base registers enable */
#define SFT_AREG_RD_FSREG_EN           8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_ZPFREG_EN          0x00000200 /* [default:1'b0][perf:] read pp zpf list base registers enable */
#define SFT_AREG_RD_ZPFREG_EN          9
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_ZPBREG_EN          0x00000400 /* [default:1'b0][perf:] read pp zpb list base registers enable */
#define SFT_AREG_RD_ZPBREG_EN          10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_CPFREG_EN          0x00000800 /* [default:1'b0][perf:] read pp cpf  list base registers enable */
#define SFT_AREG_RD_CPFREG_EN          11
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_CPBREG_EN          0x00001000 /* [default:1'b0][perf:] read pp cpb list base registers enable */
#define SFT_AREG_RD_CPBREG_EN          12

/**********************************************************************************************************/
/* read offset */
#define REG_AREG_RD_OFFSET             0x0e04	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RD_OFFSET             0x00000fff	/* [default:12'b0][perf:] read offset for
								queue/rsm/ frame / vp list base registers */
#define SFT_AREG_RD_OFFSET             0

/**********************************************************************************************************/
/* qreg read data */
#define REG_AREG_QREG_RD               0x0e08	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_QREG_RD               0xffffffff	/* [default:][perf:] queue base registers read */
#define SFT_AREG_QREG_RD               0

/**********************************************************************************************************/
/* resumed read data */
#define REG_AREG_RSMINFO_RD            0x0e0c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_RSMINFO_RD            0xffffffff	/* [default:][perf:] resume information read */
#define SFT_AREG_RSMINFO_RD            0

/**********************************************************************************************************/
/* vp fmreg read data */
#define REG_AREG_VPFMREG_RD            0x0e10	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VPFMREG_RD            0xffffffff	/* [default:][perf:] vp frame base registers read */
#define SFT_AREG_VPFMREG_RD            0

/**********************************************************************************************************/
/* pp fmreg read data */
#define REG_AREG_PPFMREG_RD            0x0e14	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PPFMREG_RD            0xffffffff	/* [default:][perf:] pp frame base registers read */
#define SFT_AREG_PPFMREG_RD            0

/**********************************************************************************************************/
/* vp0reg read data */
#define REG_AREG_VP0REG_RD             0x0e18	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VP0REG_RD             0xffffffff	/* [default:][perf:] vp0 list base registers read */
#define SFT_AREG_VP0REG_RD             0

/**********************************************************************************************************/
/* vp1reg read data */
#define REG_AREG_VP1REG_RD             0x0e1c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_VP1REG_RD             0xffffffff	/* [default:][perf:] vp1 list base registers read */
#define SFT_AREG_VP1REG_RD             0

/**********************************************************************************************************/
/* cbreg read data */
#define REG_AREG_CBREG_RD              0x0e20	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CBREG_RD              0xffffffff	/* [default:][perf:] pp cb list base registers read */
#define SFT_AREG_CBREG_RD              0

/**********************************************************************************************************/
/* xyreg read data */
#define REG_AREG_XYREG_RD              0x0e24	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_XYREG_RD              0xffffffff	/* [default:][perf:] pp xy list base registers read */
#define SFT_AREG_XYREG_RD              0

/**********************************************************************************************************/
/* fsreg read data */
#define REG_AREG_FSREG_RD              0x0e28	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_FSREG_RD              0xffffffff	/* [default:][perf:] pp fs list base registers read */
#define SFT_AREG_FSREG_RD              0

/**********************************************************************************************************/
/* zpfreg read data */
#define REG_AREG_ZPFREG_RD             0x0e2c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ZPFREG_RD             0xffffffff	/* [default:][perf:] pp zpf list base registers read */
#define SFT_AREG_ZPFREG_RD             0

/**********************************************************************************************************/
/* zpbreg read data */
#define REG_AREG_ZPBREG_RD             0x0e30	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_ZPBREG_RD             0xffffffff	/* [default:][perf:] pp zpb list base registers read */
#define SFT_AREG_ZPBREG_RD             0

/**********************************************************************************************************/
/* cpfreg read data */
#define REG_AREG_CPFREG_RD             0x0e34	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CPFREG_RD             0xffffffff	/* [default:][perf:] pp cpf list base registers read */
#define SFT_AREG_CPFREG_RD             0

/**********************************************************************************************************/
/* cpbreg read data */
#define REG_AREG_CPBREG_RD             0x0e38	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_CPBREG_RD             0xffffffff	/* [default:][perf:] pp cpb list base registers read */
#define SFT_AREG_CPBREG_RD             0

/**********************************************************************************************************/
/* mxreg read data */
#define REG_AREG_MXREG_RD              0x0e3c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_MXREG_RD              0xffffffff	/* [default:][perf:] mx frame base registers read */
#define SFT_AREG_MXREG_RD              0

/**********************************************************************************************************/
/* pwc related control setting */
#define REG_AREG_PWC_CTRL              0x0d00	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_CTRL_EN           0x00000001	/* [default:1'b0][perf:] enable PWC */
#define SFT_AREG_PWC_CTRL_EN           0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_CTRL_TIMER_EN     0x00000002	/* [default:1'b0][perf:] enable timer */
#define SFT_AREG_PWC_CTRL_TIMER_EN     1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_CTRL_STMP_EN      0x00000004	/* [default:1'b0][perf:] enable timestamp */
#define SFT_AREG_PWC_CTRL_STMP_EN      2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_CTRL_IRQ_EN       0x00000008	/* [default:1'b0][perf:] enable interrupt */
#define SFT_AREG_PWC_CTRL_IRQ_EN       3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_CTRL_SNAP         0x00000010	/* [default:1'b0][perf:] snap all counters and timestamp
								(also clear all internal counters) */
#define SFT_AREG_PWC_CTRL_SNAP         4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_CTRL_CLR          0x00000020	/* [default:1'b0][perf:] clear all counters and timestamp */
#define SFT_AREG_PWC_CTRL_CLR          5
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_CTRL_CGC_EN       0x80000000	/* [default:1'b0][perf:] enable clock gating cell */
#define SFT_AREG_PWC_CTRL_CGC_EN       31

/**********************************************************************************************************/
/* pwc sel */
#define REG_AREG_PWC_SEL               0x0d04	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_DW_VPPIPE     0x00000001	/* [default:1'b0][perf:] dw vppipe mux selection 0 for
								dw pwc counter1, dw pwc counter4 */
#define SFT_AREG_PWC_SEL_DW_VPPIPE     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_MX_CU         0x00000002 /* [default:1'b0][perf:] mx cu mux selection for mx pwc counter2 */
#define SFT_AREG_PWC_SEL_MX_CU         1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_MX_AF_0       0x00000004 /* [default:1'b0][perf:] mx af mux selection 0 for mx pwc counter4 */
#define SFT_AREG_PWC_SEL_MX_AF_0       2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_MX_AF_1       0x00000008 /* [default:1'b0][perf:] mx af mux selection 1 for mx pwc counter5 */
#define SFT_AREG_PWC_SEL_MX_AF_1       3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_CE         0x00000030 /* [default:2'b0][perf:] ff ce mux selection for ff pwc counter6 */
#define SFT_AREG_PWC_SEL_FF_CE         4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_VL         0x000000c0 /* [default:2'b0][perf:] ff vl mux selection for ff pwc counter7 */
#define SFT_AREG_PWC_SEL_FF_VL         6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_VX         0x00000300 /* [default:2'b0][perf:] ff vx mux selection for ff pwc counter8 */
#define SFT_AREG_PWC_SEL_FF_VX         8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_SF         0x00000c00 /* [default:2'b0][perf:] ff sf mux selection for ff pwc counter9 */
#define SFT_AREG_PWC_SEL_FF_SF         10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_CF         0x00003000 /* [default:2'b0][perf:] ff cf mux selection for ff pwc counter10 */
#define SFT_AREG_PWC_SEL_FF_CF         12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_BW         0x0000c000 /* [default:2'b0][perf:] ff bw mux selection for ff pwc counter11 */
#define SFT_AREG_PWC_SEL_FF_BW         14
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_SB         0x00030000 /* [default:2'b0][perf:] ff sb mux selection for ff pwc counter12 */
#define SFT_AREG_PWC_SEL_FF_SB         16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_PE         0x000c0000 /* [default:2'b0][perf:] ff pe mux selection for ff pwc counter13 */
#define SFT_AREG_PWC_SEL_FF_PE         18
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_CB_0       0x00300000 /* [default:2'b0][perf:] ff cb mux selection 0 for ff pwc counter14 */
#define SFT_AREG_PWC_SEL_FF_CB_0       20
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_CB_1       0x00c00000 /* [default:2'b0][perf:] ff cb mux selection 1 for ff pwc counter15 */
#define SFT_AREG_PWC_SEL_FF_CB_1       22
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_XY_0       0x03000000 /* [default:2'b0][perf:] ff xy mux selection 0 for ff pwc counter16 */
#define SFT_AREG_PWC_SEL_FF_XY_0       24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_XY_1       0x0c000000 /* [default:2'b0][perf:] ff xy mux selection 1 for ff pwc counter17 */
#define SFT_AREG_PWC_SEL_FF_XY_1       26
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_PO_0       0x30000000 /* [default:2'b0][perf:] ff po mux selection 0 for ff pwc counter18 */
#define SFT_AREG_PWC_SEL_FF_PO_0       28
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_SEL_FF_PO_1       0xc0000000 /* [default:2'b0][perf:] ff po mux selection 1 for ff pwc counter19 */
#define SFT_AREG_PWC_SEL_FF_PO_1       30

/**********************************************************************************************************/
/* pwc timer */
#define REG_AREG_PWC_TIMER             0x0d08	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_TIMER             0xffffffff	/* [default:32'h00ffffff][perf:] timer to trigger snap,
								timestamp and interrupt (unit: cycle) */
#define SFT_AREG_PWC_TIMER             0

/**********************************************************************************************************/
/* pwc stmp */
#define REG_AREG_PWC_STMP              0x0d0c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_STMP              0xffffffff	/* [default:32'h0][perf:] timestamp, increased by one when
								(snap == 1) or (timer timeout) */
#define SFT_AREG_PWC_STMP              0

/**********************************************************************************************************/
/* dw pwc counter0 */
#define REG_AREG_PWC_DW_CNT_0          0x0d10	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_0          0xffffffff	/* [default:32'h0][perf:] dw pwc counter0 */
#define SFT_AREG_PWC_DW_CNT_0          0

/**********************************************************************************************************/
/* dw pwc counter1 */
#define REG_AREG_PWC_DW_CNT_1          0x0d14	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_1          0xffffffff	/* [default:32'h0][perf:] dw pwc counter1 */
#define SFT_AREG_PWC_DW_CNT_1          0

/**********************************************************************************************************/
/* dw pwc counter2 */
#define REG_AREG_PWC_DW_CNT_2          0x0d18	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_2          0xffffffff	/* [default:32'h0][perf:] dw pwc counter2 */
#define SFT_AREG_PWC_DW_CNT_2          0

/**********************************************************************************************************/
/* dw pwc counter3 */
#define REG_AREG_PWC_DW_CNT_3          0x0d1c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_3          0xffffffff	/* [default:32'h0][perf:] dw pwc counter3 */
#define SFT_AREG_PWC_DW_CNT_3          0

/**********************************************************************************************************/
/* dw pwc counter4 */
#define REG_AREG_PWC_DW_CNT_4          0x0d20	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_4          0xffffffff	/* [default:32'h0][perf:] dw pwc counter4 */
#define SFT_AREG_PWC_DW_CNT_4          0

/**********************************************************************************************************/
/* dw pwc counter5 */
#define REG_AREG_PWC_DW_CNT_5          0x0d24	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_5          0xffffffff	/* [default:32'h0][perf:] dw pwc counter5 */
#define SFT_AREG_PWC_DW_CNT_5          0

/**********************************************************************************************************/
/* dw pwc counter6 */
#define REG_AREG_PWC_DW_CNT_6          0x0d28	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_6          0xffffffff	/* [default:32'h0][perf:] dw pwc counter6 */
#define SFT_AREG_PWC_DW_CNT_6          0

/**********************************************************************************************************/
/* dw pwc counter7 */
#define REG_AREG_PWC_DW_CNT_7          0x0d2c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_7          0xffffffff	/* [default:32'h0][perf:] dw pwc counter7 */
#define SFT_AREG_PWC_DW_CNT_7          0

/**********************************************************************************************************/
/* dw pwc counter8 */
#define REG_AREG_PWC_DW_CNT_8          0x0d30	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_8          0xffffffff	/* [default:32'h0][perf:] dw pwc counter8 */
#define SFT_AREG_PWC_DW_CNT_8          0

/**********************************************************************************************************/
/* dw pwc counter9 */
#define REG_AREG_PWC_DW_CNT_9          0x0d34	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_9          0xffffffff	/* [default:32'h0][perf:] dw pwc counter9 */
#define SFT_AREG_PWC_DW_CNT_9          0

/**********************************************************************************************************/
/* dw pwc counter10 */
#define REG_AREG_PWC_DW_CNT_10         0x0d38	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_10         0xffffffff	/* [default:32'h0][perf:] dw pwc counter10 */
#define SFT_AREG_PWC_DW_CNT_10         0

/**********************************************************************************************************/
/* dw pwc counter11 */
#define REG_AREG_PWC_DW_CNT_11         0x0d3c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_11         0xffffffff	/* [default:32'h0][perf:] dw pwc counter11 */
#define SFT_AREG_PWC_DW_CNT_11         0

/**********************************************************************************************************/
/* dw pwc counter12 */
#define REG_AREG_PWC_DW_CNT_12         0x0d40	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_12         0xffffffff	/* [default:32'h0][perf:] dw pwc counter12 */
#define SFT_AREG_PWC_DW_CNT_12         0

/**********************************************************************************************************/
/* dw pwc counter13 */
#define REG_AREG_PWC_DW_CNT_13         0x0d44	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_13         0xffffffff	/* [default:32'h0][perf:] dw pwc counter13 */
#define SFT_AREG_PWC_DW_CNT_13         0

/**********************************************************************************************************/
/* dw pwc counter14 */
#define REG_AREG_PWC_DW_CNT_14         0x0d48	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_14         0xffffffff	/* [default:32'h0][perf:] dw pwc counter14 */
#define SFT_AREG_PWC_DW_CNT_14         0

/**********************************************************************************************************/
/* dw pwc counter15 */
#define REG_AREG_PWC_DW_CNT_15         0x0d4c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_DW_CNT_15         0xffffffff	/* [default:32'h0][perf:] dw pwc counter15 */
#define SFT_AREG_PWC_DW_CNT_15         0

/**********************************************************************************************************/
/* mx pwc counter0 */
#define REG_AREG_PWC_MX_CNT_0          0x0d50	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_MX_CNT_0          0xffffffff	/* [default:32'h0][perf:] mx pwc counter0 */
#define SFT_AREG_PWC_MX_CNT_0          0

/**********************************************************************************************************/
/* mx pwc counter1 */
#define REG_AREG_PWC_MX_CNT_1          0x0d54	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_MX_CNT_1          0xffffffff	/* [default:32'h0][perf:] mx pwc counter1 */
#define SFT_AREG_PWC_MX_CNT_1          0

/**********************************************************************************************************/
/* mx pwc counter2 */
#define REG_AREG_PWC_MX_CNT_2          0x0d58	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_MX_CNT_2          0xffffffff	/* [default:32'h0][perf:] mx pwc counter2 */
#define SFT_AREG_PWC_MX_CNT_2          0

/**********************************************************************************************************/
/* mx pwc counter3 */
#define REG_AREG_PWC_MX_CNT_3          0x0d5c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_MX_CNT_3          0xffffffff	/* [default:32'h0][perf:] mx pwc counter3 */
#define SFT_AREG_PWC_MX_CNT_3          0

/**********************************************************************************************************/
/* mx pwc counter4 */
#define REG_AREG_PWC_MX_CNT_4          0x0d60	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_MX_CNT_4          0xffffffff	/* [default:32'h0][perf:] mx pwc counter4 */
#define SFT_AREG_PWC_MX_CNT_4          0

/**********************************************************************************************************/
/* mx pwc counter5 */
#define REG_AREG_PWC_MX_CNT_5          0x0d64	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_MX_CNT_5          0xffffffff	/* [default:32'h0][perf:] mx pwc counter5 */
#define SFT_AREG_PWC_MX_CNT_5          0

/**********************************************************************************************************/
/* mx pwc counter6 */
#define REG_AREG_PWC_MX_CNT_6          0x0d68	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_MX_CNT_6          0xffffffff	/* [default:32'h0][perf:] mx pwc counter6 */
#define SFT_AREG_PWC_MX_CNT_6          0

/**********************************************************************************************************/
/* mx pwc counter7 */
#define REG_AREG_PWC_MX_CNT_7          0x0d6c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_MX_CNT_7          0xffffffff	/* [default:32'h0][perf:] mx pwc counter7 */
#define SFT_AREG_PWC_MX_CNT_7          0

/**********************************************************************************************************/
/* tx pwc counter0 */
#define REG_AREG_PWC_TX_CNT_0          0x0d70	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_TX_CNT_0          0xffffffff	/* [default:32'h0][perf:] tx pwc counter0 */
#define SFT_AREG_PWC_TX_CNT_0          0

/**********************************************************************************************************/
/* tx pwc counter1 */
#define REG_AREG_PWC_TX_CNT_1          0x0d74	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_TX_CNT_1          0xffffffff	/* [default:32'h0][perf:] tx pwc counter1 */
#define SFT_AREG_PWC_TX_CNT_1          0

/**********************************************************************************************************/
/* tx pwc counter2 */
#define REG_AREG_PWC_TX_CNT_2          0x0d78	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_TX_CNT_2          0xffffffff	/* [default:32'h0][perf:] tx pwc counter2 */
#define SFT_AREG_PWC_TX_CNT_2          0

/**********************************************************************************************************/
/* tx pwc counter3 */
#define REG_AREG_PWC_TX_CNT_3          0x0d7c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_TX_CNT_3          0xffffffff	/* [default:32'h0][perf:] tx pwc counter3 */
#define SFT_AREG_PWC_TX_CNT_3          0

/**********************************************************************************************************/
/* ff pwc counter0 */
#define REG_AREG_PWC_FF_CNT_0          0x0d80	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_0          0xffffffff	/* [default:32'h0][perf:] ff pwc counter0 */
#define SFT_AREG_PWC_FF_CNT_0          0

/**********************************************************************************************************/
/* ff pwc counter1 */
#define REG_AREG_PWC_FF_CNT_1          0x0d84	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_1          0xffffffff	/* [default:32'h0][perf:] ff pwc counter1 */
#define SFT_AREG_PWC_FF_CNT_1          0

/**********************************************************************************************************/
/* ff pwc counter2 */
#define REG_AREG_PWC_FF_CNT_2          0x0d88	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_2          0xffffffff	/* [default:32'h0][perf:] ff pwc counter2 */
#define SFT_AREG_PWC_FF_CNT_2          0

/**********************************************************************************************************/
/* ff pwc counter3 */
#define REG_AREG_PWC_FF_CNT_3          0x0d8c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_3          0xffffffff	/* [default:32'h0][perf:] ff pwc counter3 */
#define SFT_AREG_PWC_FF_CNT_3          0

/**********************************************************************************************************/
/* ff pwc counter4 */
#define REG_AREG_PWC_FF_CNT_4          0x0d90	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_4          0xffffffff	/* [default:32'h0][perf:] ff pwc counter4 */
#define SFT_AREG_PWC_FF_CNT_4          0

/**********************************************************************************************************/
/* ff pwc counter5 */
#define REG_AREG_PWC_FF_CNT_5          0x0d94	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_5          0xffffffff	/* [default:32'h0][perf:] ff pwc counter5 */
#define SFT_AREG_PWC_FF_CNT_5          0

/**********************************************************************************************************/
/* ff pwc counter6 */
#define REG_AREG_PWC_FF_CNT_6          0x0d98	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_6          0xffffffff	/* [default:32'h0][perf:] ff pwc counter6 */
#define SFT_AREG_PWC_FF_CNT_6          0

/**********************************************************************************************************/
/* ff pwc counter7 */
#define REG_AREG_PWC_FF_CNT_7          0x0d9c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_7          0xffffffff	/* [default:32'h0][perf:] ff pwc counter7 */
#define SFT_AREG_PWC_FF_CNT_7          0

/**********************************************************************************************************/
/* ff pwc counter8 */
#define REG_AREG_PWC_FF_CNT_8          0x0da0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_8          0xffffffff	/* [default:32'h0][perf:] ff pwc counter8 */
#define SFT_AREG_PWC_FF_CNT_8          0

/**********************************************************************************************************/
/* ff pwc counter9 */
#define REG_AREG_PWC_FF_CNT_9          0x0da4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_9          0xffffffff	/* [default:32'h0][perf:] ff pwc counter9 */
#define SFT_AREG_PWC_FF_CNT_9          0

/**********************************************************************************************************/
/* ff pwc counter10 */
#define REG_AREG_PWC_FF_CNT_10         0x0da8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_10         0xffffffff	/* [default:32'h0][perf:] ff pwc counter10 */
#define SFT_AREG_PWC_FF_CNT_10         0

/**********************************************************************************************************/
/* ff pwc counter11 */
#define REG_AREG_PWC_FF_CNT_11         0x0dac	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_11         0xffffffff	/* [default:32'h0][perf:] ff pwc counter11 */
#define SFT_AREG_PWC_FF_CNT_11         0

/**********************************************************************************************************/
/* ff pwc counter12 */
#define REG_AREG_PWC_FF_CNT_12         0x0db0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_12         0xffffffff	/* [default:32'h0][perf:] ff pwc counter12 */
#define SFT_AREG_PWC_FF_CNT_12         0

/**********************************************************************************************************/
/* ff pwc counter13 */
#define REG_AREG_PWC_FF_CNT_13         0x0db4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_13         0xffffffff	/* [default:32'h0][perf:] ff pwc counter13 */
#define SFT_AREG_PWC_FF_CNT_13         0

/**********************************************************************************************************/
/* ff pwc counter14 */
#define REG_AREG_PWC_FF_CNT_14         0x0db8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_14         0xffffffff	/* [default:32'h0][perf:] ff pwc counter14 */
#define SFT_AREG_PWC_FF_CNT_14         0

/**********************************************************************************************************/
/* ff pwc counter15 */
#define REG_AREG_PWC_FF_CNT_15         0x0dbc	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_15         0xffffffff	/* [default:32'h0][perf:] ff pwc counter15 */
#define SFT_AREG_PWC_FF_CNT_15         0

/**********************************************************************************************************/
/* ff pwc counter16 */
#define REG_AREG_PWC_FF_CNT_16         0x0dc0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_16         0xffffffff	/* [default:32'h0][perf:] ff pwc counter16 */
#define SFT_AREG_PWC_FF_CNT_16         0

/**********************************************************************************************************/
/* ff pwc counter17 */
#define REG_AREG_PWC_FF_CNT_17         0x0dc4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_17         0xffffffff	/* [default:32'h0][perf:] ff pwc counter17 */
#define SFT_AREG_PWC_FF_CNT_17         0

/**********************************************************************************************************/
/* ff pwc counter18 */
#define REG_AREG_PWC_FF_CNT_18         0x0dc8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_18         0xffffffff	/* [default:32'h0][perf:] ff pwc counter18 */
#define SFT_AREG_PWC_FF_CNT_18         0

/**********************************************************************************************************/
/* ff pwc counter19 */
#define REG_AREG_PWC_FF_CNT_19         0x0dcc	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_FF_CNT_19         0xffffffff	/* [default:32'h0][perf:] ff pwc counter19 */
#define SFT_AREG_PWC_FF_CNT_19         0

/**********************************************************************************************************/
/* counter enable 0 */
#define REG_AREG_PWC_COUNTER_DIS_0     0x0dd0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_COUNTER_DIS_0     0xffffffff	/* [default:32'h0][perf:] control separately 48 power
								counters in g3d_pwc */
						  /* 0: enable  1: disable */
						  /* [0] enable/disabel for pwc_dw_cnt_0 */
						  /* [1] enable/disabel for pwc_dw_cnt_1 */
						  /* [2] enable/disabel for pwc_dw_cnt_2 */
						  /* [3] enable/disabel for pwc_dw_cnt_3 */
						  /* [4] enable/disabel for pwc_dw_cnt_4 */
						  /* [5] enable/disabel for pwc_dw_cnt_5 */
						  /* [6] enable/disabel for pwc_dw_cnt_6 */
						  /* [7] enable/disabel for pwc_dw_cnt_7 */
						  /* [8] enable/disabel for pwc_dw_cnt_8 */
						  /* [9] enable/disabel for pwc_dw_cnt_9 */
						  /* [10] enable/disabel for pwc_dw_cnt_10 */
						  /* [11] enable/disabel for pwc_dw_cnt_11 */
						  /* [12] enable/disabel for pwc_dw_cnt_12 */
						  /* [13] enable/disabel for pwc_dw_cnt_13 */
						  /* [14] enable/disabel for pwc_dw_cnt_14 */
						  /* [15] enable/disabel for pwc_dw_cnt_15 */
						  /* [16] enable/disabel for pwc_mx_cnt_0 */
						  /* [17] enable/disabel for pwc_mx_cnt_1 */
						  /* [18] enable/disabel for pwc_mx_cnt_2 */
						  /* [19] enable/disabel for pwc_mx_cnt_3 */
						  /* [20] enable/disabel for pwc_mx_cnt_4 */
						  /* [21] enable/disabel for pwc_mx_cnt_5 */
						  /* [22] enable/disabel for pwc_mx_cnt_6 */
						  /* [23] enable/disabel for pwc_mx_cnt_7 */
						  /* [24] enable/disabel for pwc_tx_cnt_0 */
						  /* [25] enable/disabel for pwc_tx_cnt_1 */
						  /* [26] enable/disabel for pwc_tx_cnt_2 */
						  /* [27] enable/disabel for pwc_tx_cnt_3 */
						  /* [28] enable/disabel for pwc_ff_cnt_0 */
						  /* [29] enable/disabel for pwc_ff_cnt_1 */
						  /* [30] enable/disabel for pwc_ff_cnt_2 */
						  /* [31] enable/disabel for pwc_ff_cnt_3 */
#define SFT_AREG_PWC_COUNTER_DIS_0     0

/**********************************************************************************************************/
/* counter enable 1 */
#define REG_AREG_PWC_COUNTER_DIS_1     0x0dd4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PWC_COUNTER_DIS_1     0x0000ffff	/* [default:16'h0][perf:] control separately 48 power
								counters in g3d_pwc */
						  /* 0: enable  1: disable */
						  /* [0] enable/disabel for pwc_ff_cnt_4 */
						  /* [1] enable/disabel for pwc_ff_cnt_5 */
						  /* [2] enable/disabel for pwc_ff_cnt_6 */
						  /* [3] enable/disabel for pwc_ff_cnt_7 */
						  /* [4] enable/disabel for pwc_ff_cnt_8 */
						  /* [5] enable/disabel for pwc_ff_cnt_9 */
						  /* [6] enable/disabel for pwc_ff_cnt_10 */
						  /* [7] enable/disabel for pwc_ff_cnt_11 */
						  /* [8] enable/disabel for pwc_ff_cnt_12 */
						  /* [9] enable/disabel for pwc_ff_cnt_13 */
						  /* [10] enable/disabel for pwc_ff_cnt_14 */
						  /* [11] enable/disabel for pwc_ff_cnt_15 */
						  /* [12] enable/disabel for pwc_ff_cnt_16 */
						  /* [13] enable/disabel for pwc_ff_cnt_17 */
						  /* [14] enable/disabel for pwc_ff_cnt_18 */
						  /* [15] enable/disabel for pwc_ff_cnt_19 */
#define SFT_AREG_PWC_COUNTER_DIS_1     0

/**********************************************************************************************************/
/* pm flow control_config */
#define REG_AREG_PM_REG0               0x0f00	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_PM_EN           0x00000001	/* [default:1'b0][perf:] enable PM */
#define SFT_AREG_DEBUG_PM_EN           0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_RST_ALL_CNTR       0x00000002	/* [default:1'b0][perf:] reset all counter */
#define SFT_AREG_PM_RST_ALL_CNTR       1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_DEBUG_MODE         0x00000004	/* [default:1'b0][perf:] into debug mode */
#define SFT_AREG_PM_DEBUG_MODE         2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_FORCE_IDLE         0x00000008	/* [default:1'b0][perf:] force into idle mode */
#define SFT_AREG_PM_FORCE_IDLE         3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_CLEAR_REG          0x00000010	/* [default:1'b0][perf:] pm SW reset */
#define SFT_AREG_PM_CLEAR_REG          4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_FORCE_WRITE        0x00000020	/* [default:1'b0][perf:] force into write mode */
#define SFT_AREG_PM_FORCE_WRITE        5
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_SAMPLE_LEVEL       0x000000c0	/* [default:2'b0][perf:] define sample level */
						  /* 00: HW sub-frame, */
						  /* 01: SW sub-frame, */
						  /* 10: Frame */
#define SFT_AREG_PM_SAMPLE_LEVEL       6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_BAK_UP_REG         0xffffff00	/* [default:24'h0][perf:] bak up reg */
						  /* [8]: stop time stamp */
						  /* [9]: 0xF1C/areg_pm_sturation[0]=freg_opencl_en */
						  /* [27:10]:0xF1C/areg_pm_sturation[27:10]=unit counter[17:0] */
						  /* [31:28]:0xF1C/areg_pm_sturation[31:28]=state[3:0] */
						  /* [31:30]:option for reset fifo utilization/stall starve counter */
#define SFT_AREG_PM_BAK_UP_REG         8

/**********************************************************************************************************/
/* start to count from the m-th unit */
#define REG_AREG_PM_M_RUN              0x0f04	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_M_RUN              0xffffffff	/* [default:32'h0][perf:] ignore m-1 unit, start to count
								from the m-th unit,
								unit is defined by 0xF00: pm_sample_level */
#define SFT_AREG_PM_M_RUN              0

/**********************************************************************************************************/
/* sample n units in one run */
#define REG_AREG_PM_N_UNITS            0x0f08	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_N_UNITS            0xffffffff	/* [default:32'h0][perf:] sample n units in one run */
#define SFT_AREG_PM_N_UNITS            0

/**********************************************************************************************************/
/* dram start address to save counter value */
#define REG_AREG_PM_DRAM_START_ADDR    0x0f0c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_DRAM_START_ADDR    0xffffffff	/* [default:32'h0][perf:] DRAM write start address */
#define SFT_AREG_PM_DRAM_START_ADDR    0

/**********************************************************************************************************/
/* select which counter showed in areg_dbg_bus */
#define REG_AREG_PM_DBG_BUS_SEL        0x0f10	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_DBG_BUS_SEL        0xffffffff	/* [default:32'h0][perf:] select which counter shown
								in Areg debug port */
#define SFT_AREG_PM_DBG_BUS_SEL        0

/**********************************************************************************************************/
/* module mx, select MX rd channel into AXI to count */
#define REG_AREG_PM_MXAXI_CHSEL_0      0x0f14	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_MXAXIR_CHSEL       0x001fffff	/* [default:21'h0][perf:] module mx, select MX rd channel
								into AXI for counting */
						  /* * Rd/Wr need pairs with pm_mxaxiw_chsel
							[0]: cp_c_rd * [1]: ux_vp_rd * [2]: zp_z_rd * */
						  /* [3]: vx_hc_rd* [4]: xy_cac_rd* [5]:vx_vtx_rd* */
						  /* [6]: tx_ch0_rd [7]:tx_glb_rd [8]:vl_idx_rd */
						  /* [9]:vl_att_rd [10]:ux_ic_rd [11]:pe_pe_rd */
						  /* [12]:xy_buf_rd [13]:vx_bl_rd [14]:tx_ch1_rd */
						  /* [15]:gl_gl_rd [16]:ux_cp_rd [17]:cq_cq_rp */
						  /* [18]:vl_idx_rp [19]:vx_vtx_rp [20]: reserved */
#define SFT_AREG_PM_MXAXIR_CHSEL       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_MXAXIR_CHSEL_LC    0x00200000	/* [default:1'b0][perf:] module mx, select
							MX rd lc (length cache) channel into AXI for counting */
						  /*  */
						  /*  */
#define SFT_AREG_PM_MXAXIR_CHSEL_LC    21
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_MXAXI_CHSEL_EN     0x80000000	/* [default:1'b0][perf:] enable MX channel
								count from AXI interface */
						  /* 0: disable MX channel count from AXI */
						  /* 1: enable MX channel count from AXI */
#define SFT_AREG_PM_MXAXI_CHSEL_EN     31

/**********************************************************************************************************/
/* module mx, select MX wr channel into AXI to count */
#define REG_AREG_PM_MXAXI_CHSEL_1      0x0f18	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_MXAXIW_CHSEL       0x0000ffff	/* [default:16'h0][perf:] module mx,
								select MX wr channel into AXI for counting */
						  /* * Rd/Wr need pairs with pm_mxaxir_chsel
							[0]: cp_c_wr* [1]: ux_vp_wr* [2]: zp_z_wr* */
						  /* [3]:vx_hc_wr*[4]:xy_cac_wr*[5]:vx_vtx_wr * */
						  /* [6]: vl_tfb_wr [7]: vx_bs_wr [8]:cq_cq_wp */
						  /* [9]:cp_c_wp [10]:cp_ebf_wp [11]:pe_pe_wp */
						  /* [12]:vx_vtx_wp [13]:zp_z_wp[14]:xy_cac_wp */
						  /* [15]:pm_pm_wp */
#define SFT_AREG_PM_MXAXIW_CHSEL       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_MXAXIW_CHSEL_LC    0x00010000	/* [default:1'b0][perf:] module mx,
							select MX wr lc (length cache) channel into AXI for counting */
						  /*  */
						  /*  */
#define SFT_AREG_PM_MXAXIW_CHSEL_LC    16

/**********************************************************************************************************/
/* indicate saturation */
#define REG_AREG_PM_SATURATION         0x0f1c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_SATURATION         0xffffffff	/* [default:][perf:] saturation, stop to count */
						  /* 1: means saturation occurred in the counter */
#define SFT_AREG_PM_SATURATION         0

/**********************************************************************************************************/
/* size of data written to dram */
#define REG_AREG_PM_DRAM_SIZE          0x0f20	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_DRAM_SIZE          0xffffffff /* [default:32'h0][perf:] sw set the size of data written to dram */
#define SFT_AREG_PM_DRAM_SIZE          0

/**********************************************************************************************************/
/* select debug signal from which module under partition FF */
#define REG_AREG_DEBUG_FF_MODULE_SEL   0x0f24	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FF_MODULE_SEL_0 0x0000001f	/* [default:5'h1f][perf:] Select module for Debug Group 0 */
						  /* For each debug group */
						  /* 0: BW */
						  /* 1: CE */
						  /* 2: CB */
						  /* 3: CF */
						  /* 4: PO */
						  /* 5: SB */
						  /* 6: SF */
						  /* 7: VL */
						  /* 8: VX */
						  /* 9: XY */
						  /* 10: UX Common */
						  /* 11: MF */
						  /* 12: PE */
						  /* 13:PM_TYPICAL */
						  /* 31:Perf Monitor */
#define SFT_AREG_DEBUG_FF_MODULE_SEL_0 0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FF_MODULE_SEL_1 0x00001f00	/* [default:5'h1f][perf:] Select module for Debug Group 1 */
						  /* As above */
#define SFT_AREG_DEBUG_FF_MODULE_SEL_1 8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FF_MODULE_SEL_2 0x001f0000	/* [default:5'h1f][perf:] Select module for Debug Group 2 */
						  /* As above */
#define SFT_AREG_DEBUG_FF_MODULE_SEL_2 16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FF_MODULE_SEL_3 0x1f000000	/* [default:5'h1f][perf:] Select module for Debug Group 3 */
						  /* As above */
#define SFT_AREG_DEBUG_FF_MODULE_SEL_3 24

/**********************************************************************************************************/
/* select pm signal from which module under partition FF */
#define REG_AREG_PM_FF_MODULE_SEL      0x0f28	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_FF_MODULE_SEL_0    0x0000001f	/* [default:5'h0][perf:] Select module for PM Group 0 */
						  /* PM_TYPICAL is the module collecting typical pm signal */
						  /* For each debug group */
						  /* 0: BW */
						  /* 1: CE */
						  /* 2: CB */
						  /* 3: CF */
						  /* 4: PO */
						  /* 5: SB */
						  /* 6: SF */
						  /* 7: VL */
						  /* 8: VX */
						  /* 9: XY */
						  /* 10: UX Common */
						  /* 11: MF */
						  /* 12: PE */
						  /* 13:PM_TYPICAL */
#define SFT_AREG_PM_FF_MODULE_SEL_0    0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_FF_MODULE_SEL_1    0x00001f00	/* [default:5'h0][perf:] Select module for PM Group 1 */
						  /* As above */
#define SFT_AREG_PM_FF_MODULE_SEL_1    8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_FF_MODULE_SEL_2    0x001f0000	/* [default:5'h0][perf:] Select module for PM Group 2 */
						  /* As above */
#define SFT_AREG_PM_FF_MODULE_SEL_2    16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_FF_MODULE_SEL_3    0x1f000000	/* [default:5'h0][perf:] Select module for PM Group 3 */
						  /* As above */
#define SFT_AREG_PM_FF_MODULE_SEL_3    24

/**********************************************************************************************************/
/* indicate which counter reset to 0 while signal equal to 0 */
#define REG_AREG_PM_COUNTER_RESET      0x0f2c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_COUNTER_RESET      0xffffffff	/* [default:32'h0][perf:] for shader pm,
							some counter value need reset while signal equal 0 */
#define SFT_AREG_PM_COUNTER_RESET      0

/**********************************************************************************************************/
/* select debug group to engine output */
#define REG_AREG_DEBUG_GROUP_SEL       0x0f30	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_GROUP_SEL       0xffffffff /* [default:32'h0][perf:] all engine shared same
							debug mux select pin */
						  /* Bit [7:0]: Select debug group for output debug group 0 */
						  /* Bit [15:8]: Select debug group for output debug group 1 */
						  /* Bit [23:16]: Select debug group for output debug group 2 */
						  /* Bit [31:24]: Select debug group for output debug group 3 */
						  /*  */
						  /* For each output group */
						  /* 0: PM Group 0 and 1 */
						  /* 1: PM Group 2 and 3 */
						  /* 2:Debug ID */
						  /* 3: Inverse of Debug ID */
						  /* 4~255: design dependent */
#define SFT_AREG_DEBUG_GROUP_SEL       0

/**********************************************************************************************************/
/* select pm group in engine */
#define REG_AREG_PM_GROUP_SEL          0x0f34	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_GROUP_SEL          0xffffffff /* [default:32'h0][perf:] all engine shared same pm mux select pin */
						  /* Bit [7:0]: Select group for pm signal 07-00 */
						  /* Bit [15:8]: Select group for pm signal 15-08 */
						  /* Bit [23:16]: Select group for pm signal 23-16 */
						  /* Bit [31:24]: Select group for pm signal 31-24 */
						  /* Fir each pm group */
						  /* 0: PM group 0 */
						  /* 1: PM group 1 */
						  /* 2: PM group 2 */
#define SFT_AREG_PM_GROUP_SEL          0

/**********************************************************************************************************/
/* central pm select signal from which partition */
#define REG_AREG_PM_DEBUG_CENTRAL_SEL  0x0f38	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_CENTRAL_SEL_0   0x00000007	/* [default:3'b0][perf:] select debug
								signal Bit15~Bit00 from which partition */
						  /* 0: DWP */
						  /* 1: MX */
						  /* 2: TX */
						  /* 3: FF */
#define SFT_AREG_DEBUG_CENTRAL_SEL_0   0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_CENTRAL_SEL_1   0x00000070	/* [default:3'b0][perf:] select debug
								signal Bit31~Bit16 from which partition */
						  /* As above */
#define SFT_AREG_DEBUG_CENTRAL_SEL_1   4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_CENTRAL_SEL_2   0x00000700	/* [default:3'b0][perf:] select debug
								signal Bit47~Bit32 from which partition */
						  /* As above */
#define SFT_AREG_DEBUG_CENTRAL_SEL_2   8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_CENTRAL_SEL_3   0x00007000	/* [default:3'b0][perf:] select debug
								signal Bit63~Bit48 from which partition */
						  /* As above */
#define SFT_AREG_DEBUG_CENTRAL_SEL_3   12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_CENTRAL_SEL_0      0x00070000	/* [default:3'b0][perf:] select pm
								signal Bit7~Bit0 from which partition */
						  /* 0: DWP */
						  /* 1: MX */
						  /* 2: TX */
						  /* 3: FF */
#define SFT_AREG_PM_CENTRAL_SEL_0      16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_CENTRAL_SEL_1      0x00700000	/* [default:3'b0][perf:] select pm
								signal Bit15~Bit8 from which partition */
						  /* As above */
#define SFT_AREG_PM_CENTRAL_SEL_1      20
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_CENTRAL_SEL_2      0x07000000	/* [default:3'b0][perf:] select pm
								signal Bit23~Bit16 from which partition */
						  /* As above */
#define SFT_AREG_PM_CENTRAL_SEL_2      24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_CENTRAL_SEL_3      0x70000000	/* [default:3'b0][perf:] select pm
								signal Bit31~Bit24 from which partition */
						  /* As above */
#define SFT_AREG_PM_CENTRAL_SEL_3      28

/**********************************************************************************************************/
/* Debug Bus final mux */
#define REG_AREG_DEBUG_FINAL_MUX       0x0f3c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FINAL_BUS_B0_SEL 0x00000007	/* [default:3'h0][perf:] select
								debug io bit7 ~ bit0 from which byte */
#define SFT_AREG_DEBUG_FINAL_BUS_B0_SEL 0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FINAL_BUS_B1_SEL 0x00000070	/* [default:3'h1][perf:] select
								debug io bit15 ~ bit8 from which byte */
#define SFT_AREG_DEBUG_FINAL_BUS_B1_SEL 4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FINAL_BUS_B2_SEL 0x00000700	/* [default:3'h2][perf:] select
								debug io bit23 ~ bit16 from which byte */
#define SFT_AREG_DEBUG_FINAL_BUS_B2_SEL 8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FINAL_BUS_B3_SEL 0x00007000	/* [default:3'h3][perf:] select
								debug io bit31 ~ bit24 from which byte */
#define SFT_AREG_DEBUG_FINAL_BUS_B3_SEL 12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FINAL_TRIG_B0_SEL 0x00070000	/* [default:3'h4][perf:] select
								debug trigger bit7 ~ bit0 from which byte */
#define SFT_AREG_DEBUG_FINAL_TRIG_B0_SEL 16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FINAL_TRIG_B1_SEL 0x00700000	/* [default:3'h5][perf:] select
								debug trigger bit15 ~ bit8 from which byte */
#define SFT_AREG_DEBUG_FINAL_TRIG_B1_SEL 20
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FINAL_TRIG_B2_SEL 0x07000000	/* [default:3'h6][perf:] select
								debug trigger bit23 ~ bit16 from which byte */
#define SFT_AREG_DEBUG_FINAL_TRIG_B2_SEL 24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DEBUG_FINAL_TRIG_B3_SEL 0x70000000	/* [default:3'h7][perf:] select
								debug trigger bit31 ~ bit24 from which byte */
#define SFT_AREG_DEBUG_FINAL_TRIG_B3_SEL 28

/**********************************************************************************************************/
/* time stamp for APB read */
#define REG_AREG_PM_TIME_STAMP         0x0f40	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_TIME_STAMP         0xffffffff	/* [default:32'h0][perf:] time stamp counts between
								pm_en rising and  falling. Time stamp will also be
								record to DRAM while flush */
#define SFT_AREG_PM_TIME_STAMP         0

/**********************************************************************************************************/
/* enable individual counter */
#define REG_AREG_PM_COUNTER_EN         0x0f44	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_PM_COUNTER_EN         0xffffffff	/* [default:32'h0][perf:] turn off/on
								individual counters by the register */
						  /* 0: disable, counter won't accumulate */
						  /* 1. enable, counter accumulate with active signal */
#define SFT_AREG_PM_COUNTER_EN         0

/**********************************************************************************************************/
/* enable debug control logic */
#define REG_AREG_DBG_CTRL              0x0f60	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_CTRL_ENABLE       0x00000001	/* [default:1'b0][perf:] Enable debug control logic */
#define SFT_AREG_DBG_CTRL_ENABLE       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_CTRL_FORCE_FRAME_LOAD 0x00000002	/* [default:1'b0][perf:] Debug purpose.
							Override the frame_loaded indicator inside g3d_dbg_ctrl to 1 */
						  /* 0: normal */
						  /* 1: force frame_loaded = 1 */
#define SFT_AREG_DBG_CTRL_FORCE_FRAME_LOAD 1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_TARGET_FRAME_BY   0x0000000c	/* [default:2'b00][perf:] Debug target frame is set by */
						  /*  */
						  /* 0: all frames */
						  /* 1: freg_debug_flag */
						  /* 2: freg_frame_id */
						  /* 3: all frames after freg_frame_id */
#define SFT_AREG_DBG_TARGET_FRAME_BY   2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_TARGET_FRAME_PHASE 0x00000030 /* [default:2'b11][perf:] Debug target frame at VP/PP phase */
						  /* [0]: VP phase */
						  /* [1]: PP phasae */
#define SFT_AREG_DBG_TARGET_FRAME_PHASE 4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_CTRL_FRAME_LOADED 0x000000c0 /* [default:2'b00][perf:] Indicate that frame is loaded in g3d */
						  /* [0]: VP phase */
						  /* [1]: PP phasae */
#define SFT_AREG_DBG_CTRL_FRAME_LOADED 6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_TRIGGER_EN    0x00000100	/* [default:1'b0][perf:] Dedicate trigger pin enable */
						  /* 0: Disable */
						  /* 1: Enable */
#define SFT_AREG_DBG_BUS_TRIGGER_EN    8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMMY_0           0x00000200	/* [default:1'b0][perf:] dummy register 0 for eco */
#define SFT_AREG_DBG_DUMMY_0           9
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_TRIGGER_FRAME 0x00000c00 /* [default:2'h0][perf:] trigger condition qualificateion by frame */
						  /* 0: Qualifiedy by target frame */
						  /* 1: for each frame */
						  /* 2: trigger on target frame regardless the condition */
						  /* 3: force trigger high */
#define SFT_AREG_DBG_BUS_TRIGGER_FRAME 10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_TRIGGER_PIN_NUM 0x0001f000	/* [default:5'h0][perf:] The pin number that
								trigger pin will replace with */
#define SFT_AREG_DBG_BUS_TRIGGER_PIN_NUM 12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMMY_1           0x00020000	/* [default:1'b0][perf:] dummy register 1 for eco */
#define SFT_AREG_DBG_DUMMY_1           17
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMMY_2           0x00040000	/* [default:1'b0][perf:] dummy register 2 for eco */
#define SFT_AREG_DBG_DUMMY_2           18
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_TRIGGER_PIN_EN 0x00080000	/* [default:1'b0][perf:] Enable replacement with trigger pin */
#define SFT_AREG_DBG_BUS_TRIGGER_PIN_EN 19
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_TRIGGER_MODE  0x00300000	/* [default:2'h0][perf:] Dedicate trigger state mode */
						  /* 0: Either condition 0 or condition 1 is hit */
						  /* 1: Both condition 0 and condition 1 are hit */
						  /* 2: condition 0 than condition 1 sequence */
						  /* 3: condition 0 to condition 1 transition */
#define SFT_AREG_DBG_BUS_TRIGGER_MODE  20
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMMY_3           0x00400000	/* [default:1'b0][perf:] dummy register 3 for eco */
#define SFT_AREG_DBG_DUMMY_3           22
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMMY_4           0x00800000	/* [default:1'b0][perf:] dummy register 4 for eco */
#define SFT_AREG_DBG_DUMMY_4           23
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_ENABLE       0x01000000	/* [default:1'b0][perf:] Enable Internal Dump */
						  /* if dbg_ctrl_enable == 0, the it will target all frames */
						  /* 0: Disable Internal Dump */
						  /* 1: Enable internal Dump */
#define SFT_AREG_DBG_DUMP_ENABLE       24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_FLUSH_INFO_EN 0x02000000	/* [default:1'b0][perf:] When set, it will flush SRAM data
								to system memory at frame end */
						  /* 0: Will not flush */
						  /* 1: Will flush */
#define SFT_AREG_DBG_DUMP_FLUSH_INFO_EN 25
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_INTO_REGISTER_EN 0x04000000	/* [default:1'b0][perf:] Use register
								dbg_dump_data_dw* as storage */
						  /* 0: Use SRAM as storage */
						  /* 1: Use dbg_dump_data_dw* */
#define SFT_AREG_DBG_DUMP_INTO_REGISTER_EN 26
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_STOP_WHEN_FULL 0x08000000	/* [default:1'b1][perf:] Stop capturing data
								when the storage is full */
						  /* 0: Ring buffer mode */
						  /* 1: Stop capturing when storage is full */
#define SFT_AREG_DBG_DUMP_STOP_WHEN_FULL 27
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_32K_MODE     0x10000000	/* [default:1'b1][perf:] When set, Internal Dump
								will use only 32KB SRAM. */
						  /* This register is only valid when dbg_dump_into_register_en == 0 */
						  /* 0: 64KB SRAM mode */
						  /* 1: 32KB SRAM mode */
#define SFT_AREG_DBG_DUMP_32K_MODE     28
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_INTR_EN      0x20000000	/* [default:1'b0][perf:] When set, Internal Dump
								will request interrupt when trigger condition hit. */
						  /* 0: enable interrupt */
						  /* 1: disable interrupt */
#define SFT_AREG_DBG_DUMP_INTR_EN      29
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMMY_5           0x40000000	/* [default:1'b0][perf:] dummy register 5 for eco */
#define SFT_AREG_DBG_DUMMY_5           30
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_CLK_EN            0x80000000	/* [default:1'b0][perf:] enable clock for DFD engine */
#define SFT_AREG_DBG_CLK_EN            31

/**********************************************************************************************************/
/* Target Frame ID */
#define REG_AREG_DBG_TARGET_FRAME_ID   0x0f64	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_TARGET_FRAME_ID   0xffffffff	/* [default:32'h0][perf:] Target frame ID
								if dbg_target_frame_by == 2 or 3 */
#define SFT_AREG_DBG_TARGET_FRAME_ID   0

/**********************************************************************************************************/
/* Target HW subframe offset */
#define REG_AREG_DBG_TARGET_FRAME_OFFSET 0x0f68	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_TARGET_FRAME_OFFSET 0x0000ffff	/* [default:16'h0][perf:] Target frame offset. */
						  /* Begin DFD engine after count numbers of HW subframe
							marked as target frame. */
						  /* 0x0 means begin DFD engine regardless the offset. */
						  /* 0x1 means the first target hardware subframe hit */
#define SFT_AREG_DBG_TARGET_FRAME_OFFSET 0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_TARGET_FRAME_CURR_OFFSET 0xffff0000	/* [default:16'h0][perf:] Current hardware
									subframe offset count */
#define SFT_AREG_DBG_TARGET_FRAME_CURR_OFFSET 16

/**********************************************************************************************************/
/* debug bus control */
#define REG_AREG_DBG_BUS_CTRL          0x0f6c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_C0_INVERSED   0x00000001	/* [default:1'b0][perf:] Inverse condition 0 */
#define SFT_AREG_DBG_BUS_C0_INVERSED   0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_C0_SOURCE     0x00000002	/* [default:1'b0][perf:] condition 0 source */
						  /* 0: from bus[63:32] */
						  /* 1: from bus[31:0] */
#define SFT_AREG_DBG_BUS_C0_SOURCE     1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_C0_CHANGE     0x00000004	/* [default:1'b0][perf:] Ignore the dbg_c0_pattern,
							c0hit if the bits selected by dbg_c0_mask are changed */
#define SFT_AREG_DBG_BUS_C0_CHANGE     2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_C1_INVERSED   0x00000010	/* [default:1'b0][perf:] Inverse condition 1 */
#define SFT_AREG_DBG_BUS_C1_INVERSED   4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_C1_SOURCE     0x00000020	/* [default:1'b0][perf:] condition 1 source */
						  /* 0: from bus[63:32] */
						  /* 1: from bus[31:0] */
#define SFT_AREG_DBG_BUS_C1_SOURCE     5
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_C1_CHANGE     0x00000040	/* [default:1'b0][perf:] Ignore the dbg_c1_pattern,
							c1hit if the bits selected by dbg_c1_mask are changed */
#define SFT_AREG_DBG_BUS_C1_CHANGE     6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_TRIGGERED     0x80000000	/* [default:1'b0][perf:] The trigger condition has ever hit. */
						  /* 0: Not trigger */
						  /* 1: Triggered */
#define SFT_AREG_DBG_BUS_TRIGGERED     31

/**********************************************************************************************************/
/* Condition 0 mask of dedicate trigger */
#define REG_AREG_DBG_BUS_C0_MASK       0x0f70	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_C0_MASK       0xffffffff	/* [default:32'h0][perf:] Condition 0 mask of
								debug bus dedicate trigger */
						  /* 0: condition 0 will never be hit */
#define SFT_AREG_DBG_BUS_C0_MASK       0

/**********************************************************************************************************/
/* Condition 0 of dedicate trigger */
#define REG_AREG_DBG_BUS_C0_PATTERN    0x0f74	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_C0_PATTERN    0xffffffff	/* [default:32'h0][perf:] Condition 0 match pattern of
								debug bus dedicate trigger */
						  /* It is useless when dbg_bus_c0_change = 1 */
#define SFT_AREG_DBG_BUS_C0_PATTERN    0

/**********************************************************************************************************/
/* Condition 0 mask of dedicate trigger */
#define REG_AREG_DBG_BUS_C1_MASK       0x0f78	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_C1_MASK       0xffffffff	/* [default:32'h0][perf:] Condition 1 mask of
								debug bus dedicate trigger */
						  /* 0: condition 1 will never be hit */
#define SFT_AREG_DBG_BUS_C1_MASK       0

/**********************************************************************************************************/
/* Condition 0 of dedicate trigger */
#define REG_AREG_DBG_BUS_C1_PATTERN    0x0f7c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_C1_PATTERN    0xffffffff	/* [default:32'h0][perf:] Condition 1 match pattern of
								debug bus dedicate trigger */
						  /* It is useless when dbg_bus_c1_change = 1 */
#define SFT_AREG_DBG_BUS_C1_PATTERN    0

/**********************************************************************************************************/
/* Debug Bus value dword 0 */
#define REG_AREG_DBG_BUS_VALUE_DW0     0x0f80	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_VALUE_DW0     0xffffffff	/* [default:32'h0][perf:] The bit[31:0] value of debug bus.
								AKA value of debug out to pad */
#define SFT_AREG_DBG_BUS_VALUE_DW0     0

/**********************************************************************************************************/
/* Debug Bus value dword 1 */
#define REG_AREG_DBG_BUS_VALUE_DW1     0x0f84	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_VALUE_DW1     0xffffffff	/* [default:32'h0][perf:] The bit[63:32] value of debug bus.
								AKA value of debug trigger */
#define SFT_AREG_DBG_BUS_VALUE_DW1     0

/**********************************************************************************************************/
/* Debug Bus bit Occurred */
#define REG_AREG_DBG_BUS_OCCURRED      0x0f88	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_BUS_OCCURRED      0xffffffff	/* [default:32'h0][perf:] The bit[31:0] of debug bus
								has ever gone to high */
#define SFT_AREG_DBG_BUS_OCCURRED      0

/**********************************************************************************************************/
/* Internal Dump Mode */
#define REG_AREG_DBG_DUMP_MODE         0x0f8c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_FLUSH_MODE   0x00000003	/* [default:2'h0][perf:] Internal Dump Fluh Mode */
						  /* 0: No flush */
						  /* 1:flush at the end of frame whose freg_debug_flush is set */
						  /* 2: flush at the end of every target frame */
						  /* 3: reserved */
#define SFT_AREG_DBG_DUMP_FLUSH_MODE   0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_FLUSH_CONTEXT_SWITCH 0x00000004	/* [default:1'b0][perf:] Internal Dump flush
									when context switch happened */
						  /* 0: Normal mode */
						  /* 1: see context switch as frame end */
#define SFT_AREG_DBG_DUMP_FLUSH_CONTEXT_SWITCH 2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_FLUSH_BACKOFF 0x00000008	/* [default:1'b0][perf:] Internal Dump flush
								when backoff happened */
						  /* 0: Normal mode */
						  /* 1: see backoff as frame end */
#define SFT_AREG_DBG_DUMP_FLUSH_BACKOFF 3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_SRC_SEL      0x00000030	/* [default:2'h0][perf:] Input source for Internal Dump */
						  /* 0: debug bus */
						  /* 1: Reserved Customized Information */
						  /* 2: Internal probe */
						  /* 3: Reserved */
#define SFT_AREG_DBG_DUMP_SRC_SEL      4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_PROBE_SEL    0x0000ff00	/* [default:8'h0][perf:] internal probe select.
								Only valid when dbg_dump_src_sel == 1. */
						  /* bit[7:4] means module select */
						  /* bit[3:0] means the bus select in the module */
						  /* For bit [7:4] */
						  /* 0: VL */
						  /* 1: CF */
						  /* 2: SF */
						  /* 3: BW */
						  /* 4: UX */
						  /* 5: TX */
						  /* 6: VX */
						  /* 7: CB */
						  /* 8: SB */
						  /* 9: XY */
						  /* 10: ZP */
#define SFT_AREG_DBG_DUMP_PROBE_SEL    8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DUMMY_0      0x00010000	/* [default:1'b0][perf:] Dump dummy register 0 for eco */
#define SFT_AREG_DBG_DUMP_DUMMY_0      16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DUMMY_1      0x00020000	/* [default:1'b0][perf:] Dump dummy register 1 for eco */
#define SFT_AREG_DBG_DUMP_DUMMY_1      17
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DUMMY_2      0x00040000	/* [default:1'b0][perf:] Dump dummy register 2 for eco */
#define SFT_AREG_DBG_DUMP_DUMMY_2      18
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DUMMY_3      0x00080000	/* [default:1'b0][perf:] Dump dummy register 3 for eco */
#define SFT_AREG_DBG_DUMP_DUMMY_3      19
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_OVERRIDE     0x00300000	/* [default:2'h3][perf:] Internal Dump width override */
						  /* 0: Override width = 32bits */
						  /* 1: Override width = 64bits */
						  /* 2: Override width = 128bits */
						  /* 3: No override. */
#define SFT_AREG_DBG_DUMP_OVERRIDE     20
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DW0_SEL      0x03000000	/* [default:2'h0][perf:] Internal Dump Double Word 0 Data
								select if width overide */
						  /* 0: dump data[31:0] = Input Data[31:0] */
						  /* 1: dump data[31:0] = input Data[63:32] */
						  /* 2: dump data[31:0] = input Data[95:64] */
						  /* 3: dump data[31:0] = input Data[127:96] */
#define SFT_AREG_DBG_DUMP_DW0_SEL      24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DW1_SEL      0x0c000000	/* [default:2'h1][perf:] Internal Dump Double Word 1 Data
								select if width overide */
						  /* 0: dump data[63:32] = Input Data[31:0] */
						  /* 1: dump data[63:32] = input Data[63:32] */
						  /* 2: dump data[63:32] = input Data[95:64] */
						  /* 3: dump data[63:32] = input Data[127:96] */
#define SFT_AREG_DBG_DUMP_DW1_SEL      26
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DW2_SEL      0x30000000	/* [default:2'h2][perf:] Internal Dump Double Word 2 Data
								select if width overide */
						  /* 0: dump data[95:64] = Input Data[31:0] */
						  /* 1: dump data[95:64] = input Data[63:32] */
						  /* 2: dump data[95:64] = input Data[95:64] */
						  /* 3: dump data[95:64] = input Data[127:96] */
#define SFT_AREG_DBG_DUMP_DW2_SEL      28
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DW3_SEL      0xc0000000	/* [default:2'h3][perf:] Internal Dump Double Word 3 Data
								select if width overide */
						  /* 0: dump data[127:96] = Input Data[31:0] */
						  /* 1: dump data[127:96] = input Data[63:32] */
						  /* 2: dump data[127:96] = input Data[95:64] */
						  /* 3: dump data[127:96] = input Data[127:96] */
#define SFT_AREG_DBG_DUMP_DW3_SEL      30

/**********************************************************************************************************/
/* Internal dump begin count */
#define REG_AREG_DBG_DUMP_BEGIN_COUNT  0x0f90	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_BEGIN_COUNT  0xffffffff	/* [default:32'h0][perf:] Internal dump begin dump at the
								number of trigger hit count */
						  /* 0: means begin capture when frame begins */
#define SFT_AREG_DBG_DUMP_BEGIN_COUNT  0

/**********************************************************************************************************/
/* Internal dump end count */
#define REG_AREG_DBG_DUMP_END_COUNT    0x0f94	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_END_COUNT    0xffffffff	/* [default:32'h0][perf:] Internal dump begin dump at the
								number of trigger hit count */
						  /* 0: means no stop capturing */
#define SFT_AREG_DBG_DUMP_END_COUNT    0

/**********************************************************************************************************/
/* Internal dump end count */
#define REG_AREG_DBG_DUMP_CAPTURE_COUNT 0x0f98	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_CAPTURE_COUNT 0xffffffff	/* [default:32'h0][perf:] Internal dump begin dump at the
								number of trigger hit count */
						  /* 0: means capture every cycle after triggered */
#define SFT_AREG_DBG_DUMP_CAPTURE_COUNT 0

/**********************************************************************************************************/
/* Internal Dump Status */
#define REG_AREG_DBG_DUMP_STATUS       0x0f9c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_STATUS_LAST_ADDR 0x00003fff	/* [default:14'h0][perf:] Last write address to SRAM */
#define SFT_AREG_DBG_DUMP_STATUS_LAST_ADDR 0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_STATUS_DATA_WIDTH 0x0000c000	/* [default:2'h0][perf:] Internal Dump captured data width */
						  /* 0: 32bit width */
						  /* 1: 64bits width */
						  /* 2: 128bit width */
						  /* 3: reserved */
#define SFT_AREG_DBG_DUMP_STATUS_DATA_WIDTH 14
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_STATUS_FLUSH_COUNT 0x00ff0000	/* [default:8'h0][perf:] Internal Dump flush count */
#define SFT_AREG_DBG_DUMP_STATUS_FLUSH_COUNT 16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_STATUS_FLUSH_BY_CS 0x01000000	/* [default:1'b0][perf:] Indicate that last flush is
								caused by context switch */
#define SFT_AREG_DBG_DUMP_STATUS_FLUSH_BY_CS 24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_STATUS_FLUSH_BY_BO 0x02000000	/* [default:1'b0][perf:] Indicate that last flush is
								caused by back off */
#define SFT_AREG_DBG_DUMP_STATUS_FLUSH_BY_BO 25
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_STATUS_CS_OCCURRED 0x04000000	/* [default:1'b0][perf:] Indicate that context switch
								has ever happened before last flush */
#define SFT_AREG_DBG_DUMP_STATUS_CS_OCCURRED 26
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_STATUS_BO_OCCURRED 0x08000000	/* [default:1'b0][perf:] Indicate that back off
									has ever happened before last flush */
#define SFT_AREG_DBG_DUMP_STATUS_BO_OCCURRED 27
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_STATE        0x30000000	/* [default:2'h0][perf:] Current state of internal dump */
#define SFT_AREG_DBG_DUMP_STATE        28
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_STATUS_CAPTURED 0x40000000	/* [default:1'b0][perf:] Captured indicator */
						  /* It means something has captured in the dump */
						  /* 0:  No captured */
						  /* 1: Captured */
#define SFT_AREG_DBG_DUMP_STATUS_CAPTURED 30
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_STATUS_OVERFLOW 0x80000000	/* [default:1'b0][perf:] Overflow indicator */
						  /* 0:  No overflow */
						  /* 1: Overflow occurred */
#define SFT_AREG_DBG_DUMP_STATUS_OVERFLOW 31

/**********************************************************************************************************/
/* Dump Data [31:0] Register */
#define REG_AREG_DBG_DUMP_DATA_DW0     0x0fa0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DATA_DW0     0xffffffff	/* [default:32'h0][perf:] The dumpped data[31:0]
								when dbg_dmp_into_register_en == 1 */
#define SFT_AREG_DBG_DUMP_DATA_DW0     0

/**********************************************************************************************************/
/* Dump Data [63:32] Register */
#define REG_AREG_DBG_DUMP_DATA_DW1     0x0fa4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DATA_DW1     0xffffffff	/* [default:32'h0][perf:] The dumpped data[63:32]
								when dbg_dmp_into_register_en == 1 */
#define SFT_AREG_DBG_DUMP_DATA_DW1     0

/**********************************************************************************************************/
/* Dump Data [95:64] Register */
#define REG_AREG_DBG_DUMP_DATA_DW2     0x0fa8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DATA_DW2     0xffffffff	/* [default:32'h0][perf:] The dumpped data[95:64]
								when dbg_dmp_into_register_en == 1 */
#define SFT_AREG_DBG_DUMP_DATA_DW2     0

/**********************************************************************************************************/
/* Dump Data [127:96] Register */
#define REG_AREG_DBG_DUMP_DATA_DW3     0x0fac	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_DUMP_DATA_DW3     0xffffffff	/* [default:32'h0][perf:] The dumpped data[127:96]
								when dbg_dmp_into_register_en == 1 */
#define SFT_AREG_DBG_DUMP_DATA_DW3     0

/**********************************************************************************************************/
/* Internal Bus Signature Check Control */
#define REG_AREG_DBG_IBSC_CTRL         0x0fb0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_IBSC_ENABLE       0x00000001	/* [default:1'b0][perf:] Enable Internal Bus Signature Check */
						  /* 0: Disable */
						  /* 1: Enable */
#define SFT_AREG_DBG_IBSC_ENABLE       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_IBSC_BY_TGT_FRAME 0x00000002	/* [default:1'b0][perf:] If set, it will generate signature with
								the next target frame controlled by dbg_ctrl */
						  /* otherwise, it will generate with tne next frame */
						  /* 0: Next frame */
						  /* 1: Next target frame */
#define SFT_AREG_DBG_IBSC_BY_TGT_FRAME 1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_IBSC_XY2ZP_CHECK_FFACE 0x00000004	/* [default:1'b0][perf:] If set, IBSC will generate signature
								with xy2zp_fface */
#define SFT_AREG_DBG_IBSC_XY2ZP_CHECK_FFACE 2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_IBSC_VL2UX_IGNORE_END 0x00000008	/* [default:1'b0][perf:] If set, IBSC will generate signature
								without vl2ux_vs0_instend, vl2ux_vs0_listend,
								vl2ux_vs0_ndrend, vl2ux_vs0_wgend,
								vl2ux_vs1_instend and vl2ux_vs1_listend */
#define SFT_AREG_DBG_IBSC_VL2UX_IGNORE_END 3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_ISBC_DUMMY_0      0x00000010	/* [default:1'b0][perf:] IBSC dummy register 0 for eco */
#define SFT_AREG_DBG_ISBC_DUMMY_0      4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_ISBC_DUMMY_1      0x00000020	/* [default:1'b0][perf:] IBSC dummy register 1 for eco */
#define SFT_AREG_DBG_ISBC_DUMMY_1      5
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_ISBC_DUMMY_2      0x00000040	/* [default:1'b0][perf:] IBSC dummy register 2 for eco */
#define SFT_AREG_DBG_ISBC_DUMMY_2      6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_ISBC_DUMMY_3      0x00000080	/* [default:1'b0][perf:] IBSC dummy register 3 for eco */
#define SFT_AREG_DBG_ISBC_DUMMY_3      7
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_IBSC_READ_SEL     0x0000ff00	/* [default:8'h0][perf:] Select which internal bus to put
								its signature at dbg_ibsc_signature register */
						  /* bit[15:12] means module select */
						  /* bit[11:8] means the bus select in the module */
						  /* For bit [15:12] */
						  /* 0: VL */
						  /* 1: CF */
						  /* 2: SF */
						  /* 3: BW */
						  /* 4: UX */
						  /* 5: TX */
						  /* 6: VX */
						  /* 7: CB */
						  /* 8: SB */
						  /* 9: XY */
						  /* 10: GL */
#define SFT_AREG_DBG_IBSC_READ_SEL     8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_IBSC_VP_READY     0x01000000	/* [default:1'b0][perf:] Indicate that
								VP phase bus signature is generating */
#define SFT_AREG_DBG_IBSC_VP_READY     24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_IBSC_PP_READY     0x02000000	/* [default:1'b0][perf:] Indicate that
								pP phase bus signature is generating */
#define SFT_AREG_DBG_IBSC_PP_READY     25

/**********************************************************************************************************/
/* Signature Value */
#define REG_AREG_DBG_IBSC_SIGNATURE    0x0fb4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_IBSC_SIGNATURE    0xffffffff /* [default:32'h0][perf:] The signature value of selected bus */
#define SFT_AREG_DBG_IBSC_SIGNATURE    0

/**********************************************************************************************************/
/* Specia value for vl2vx */
#define REG_AREG_DBG_IBSC_VL2VX        0x0fb8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_AREG_DBG_IBSC_VL2VX_ADDR_OFFSET 0xffffffff	/* [default:32'h0][perf:] The very first vl2vx qualified
								value when ibsc_enable asserted */
						  /* [31:0] = {vl2vx_v2_offset[3:0], vl2vx_v1_offset[3:0],
							vl2vx_v0_addr[23:0]} */
#define SFT_AREG_DBG_IBSC_VL2VX_ADDR_OFFSET 0

/**********************************************************************************************************/


typedef struct {
	union			/* 0x0000, */
	{
		unsigned int dwValue;	/* g3d version IDs */
		struct {
			unsigned int revision_id:8;	/* revision ID */
			unsigned int version_id:8;	/* version ID */
			unsigned int device_id:16;	/* device ID */
		} s;
	} reg_g3d_id;

	union			/* 0x0004, */
	{
		unsigned int dwValue;	/* g3d reset */
		struct {
			unsigned int g3d_swrst:1;	/* sw reset  g3d engine */
			/* default: 1'b0 */
		} s;
	} reg_g3d_swrst;

	union			/* 0x0008, */
	{
		unsigned int dwValue;	/* g3d engin fire */
		struct {
			unsigned int g3d_eng_fire:1;	/* g3d engine filre, auto clear */
			/* default: 1'b0 */
		} s;
	} reg_g3d_eng_fire;

	union			/* 0x000c, */
	{
		unsigned int dwValue;	/* g3d mcu set */
		struct {
			unsigned int mcu2g3d_set:1;	/* mcu to g3d engine set to control
						the fdq read/write pointer and htbuf read pointer, auto clear */
			/* default: 1'b0 */
		} s;
	} reg_mcu2g3d_set;

	union			/* 0x0010, */
	{
		unsigned int dwValue;	/* g3d idle expire counter */
		struct {
			unsigned int g3d_idle_expire_cnt:16;	/* the g3d idle timer expire credit counter */
			/* default: 16'd64 */
		} s;
	} reg_g3d_idle_expire_cnt;

	union			/* 0x0014, */
	{
		unsigned int dwValue;	/* g3d_reg apb time out counter */
		struct {
			unsigned int g3d_time_out_cnt:32;	/* for g3d hang used */
			/* default: 32'h00001000 */
		} s;
	} reg_g3d_time_out_cnt;

	union			/* 0x0020, */
	{
		unsigned int dwValue;	/* g3d_ token 0 */
		struct {
			unsigned int g3d_token0:1;	/* token 0 */
			/* default: 1'b1 */
		} s;
	} reg_g3d_token0;

	union			/* 0x0024, */
	{
		unsigned int dwValue;	/* g3d_ token 1 */
		struct {
			unsigned int g3d_token1:1;	/* token 1 */
			/* default: 1'b1 */
		} s;
	} reg_g3d_token1;

	union			/* 0x0028, */
	{
		unsigned int dwValue;	/* g3d_ token 2 */
		struct {
			unsigned int g3d_token2:1;	/* token 2 */
			/* default: 1'b1 */
		} s;
	} reg_g3d_token2;

	union			/* 0x002c, */
	{
		unsigned int dwValue;	/* g3d_ token 3 */
		struct {
			unsigned int g3d_token3:1;	/* token 3 */
			/* default: 1'b1 */
		} s;
	} reg_g3d_token3;

	union			/* 0x0080, */
	{
		unsigned int dwValue;	/* g3d clock gating control */
		struct {
			unsigned int g3d_cgc_en:1;	/* g3d global clock gating control enable */
			/* default: 1'b1 */
			unsigned int grp_cgc_en:5;	/* eclk group gating control enable */
			/* default: 5'h1f */
		} s;
	} reg_g3d_cgc_setting;

	union			/* 0x0084, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 0 */
		struct {
			unsigned int eclk_cgc0_en:32;	/* eclk clock gating control enable 0 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting0;

	union			/* 0x0088, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 1 */
		struct {
			unsigned int eclk_cgc1_en:32;	/* eclk clock gating control enable 1 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting1;

	union			/* 0x008c, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 2 */
		struct {
			unsigned int eclk_cgc2_en:32;	/* eclk clock gating control enable 2 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting2;

	union			/* 0x0090, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 3 */
		struct {
			unsigned int eclk_cgc3_en:32;	/* eclk clock gating control enable 3 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting3;

	union			/* 0x0094, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 4 */
		struct {
			unsigned int eclk_cgc4_en:32;	/* eclk clock gating control enable 4 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting4;

	union			/* 0x0098, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 5 */
		struct {
			unsigned int eclk_cgc5_en:32;	/* eclk clock gating control enable 5 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting5;

	union			/* 0x009c, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 6 */
		struct {
			unsigned int eclk_cgc6_en:32;	/* eclk clock gating control enable 6 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting6;

	union			/* 0x00a0, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 7 */
		struct {
			unsigned int eclk_cgc7_en:32;	/* eclk clock gating control enable 7 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting7;

	union			/* 0x00a4, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 8 */
		struct {
			unsigned int eclk_cgc8_en:32;	/* eclk clock gating control enable 8 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting8;

	union			/* 0x00a8, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 9 */
		struct {
			unsigned int eclk_cgc9_en:32;	/* eclk clock gating control enable 9 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting9;

	union			/* 0x00ac, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 10 */
		struct {
			unsigned int eclk_cgc10_en:32;	/* eclk clock gating control enable 10 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting10;

	union			/* 0x00b0, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 11 */
		struct {
			unsigned int eclk_cgc11_en:32;	/* eclk clock gating control enable 11 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting11;

	union			/* 0x00b4, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 12 */
		struct {
			unsigned int eclk_cgc12_en:32;	/* eclk clock gating control enable 12 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting12;

	union			/* 0x00b8, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 13 */
		struct {
			unsigned int eclk_cgc13_en:32;	/* eclk clock gating control enable 13 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting13;

	union			/* 0x00bc, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 14 */
		struct {
			unsigned int eclk_cgc14_en:32;	/* eclk clock gating control enable 14 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting14;

	union			/* 0x00c0, */
	{
		unsigned int dwValue;	/* g3d sub-module clock gating control 15 */
		struct {
			unsigned int eclk_cgc15_en:32;	/* eclk clock gating control enable 15 */
			/* default: 32'hffffffff */
		} s;
	} reg_g3d_cgc_sub_setting15;

	union			/* 0x0100, */
	{
		unsigned int dwValue;	/* cq ht-buffer  start address */
		struct {
			unsigned int cq_htbuf_start_addr:32;	/* cmdq(HT buffer) start address,
								must be 16 bytes alignment */
			/* default: 32'h0 */
		} s;
	} reg_cq_htbuf_start_addr;

	union			/* 0x0104, */
	{
		unsigned int dwValue;	/* cq ht-buffer end address */
		struct {
			unsigned int cq_htbuf_end_addr:32;	/* cmdq(HT buffer) end address,
								must be 16bytes alignment */
			/* default: 32'b0 */
		} s;
	} reg_cq_htbuf_end_addr;

	union			/* 0x0108, */
	{
		unsigned int dwValue;	/* cq ht-buffer read pointer */
		struct {
			unsigned int cq_htbuf_rptr:32;	/* cmdq (ring buffer ):g3d read
							head /tail buffer read pointer */
			/* default: 32'b0 */
		} s;
	} reg_cq_htbuf_rptr;

	union			/* 0x010c, */
	{
		unsigned int dwValue;	/* cq ht-buffer write pointer */
		struct {
			unsigned int cq_htbuf_wptr:32;	/* cmdq (ring buffer ):sw write
							head/tail buffer write pointer */
			/* default: 32'b0 */
		} s;
	} reg_cq_htbuf_wptr;

	union			/* 0x0110, */
	{
		unsigned int dwValue;	/* cq queue buffer start address */
		struct {
			unsigned int cq_qbuf_start_addr:32;	/* cq queue buffer base start addr */
			/* default: 32'b0 */
		} s;
	} reg_cq_qbuf_start_addr;

	union			/* 0x0114, */
	{
		unsigned int dwValue;	/* cq queue buffer end address */
		struct {
			unsigned int cq_qbuf_end_addr:32;	/* cq queue buffer base end addr */
			/* default: 32'b0 */
		} s;
	} reg_cq_qbuf_end_addr;

	union			/* 0x0118, */
	{
		unsigned int dwValue;	/* cq control update setting */
		struct {
			unsigned int cq_qld_upd:1;	/* Update the head address set this trigger */
			/* default: 1'b0 */
			unsigned int cq_htbuf_rptr_upd:1;	/* Update the head address set this trigger */
			/* default: 1'b0 */
			unsigned int cq_htbuf_wptr_upd:1;	/* Update the tail address set this trigger */
			/* default: 1'b0 */
			unsigned int reserved0:1;
			unsigned int cq_rst:1;	/* command queue sw reset */
			/* default: 1'b0 */
			unsigned int cq_ecc_en:1;	/* command queue error code check */
			/* default: 1'b0 */
		} s;
	} reg_cq_ctrl;

	union			/* 0x011c, */
	{
		unsigned int dwValue;	/* cq context switch mode */
		struct {
			unsigned int cq_pmt_md:1;	/* command queue preempt mode */
			/* 1'b0: normal case */
			/* 1'b1: preempt at frame boundary */
			/* default: 1'b0 */
		} s;
	} reg_cq_context_ctrl;

	union			/* 0x0180, */
	{
		unsigned int dwValue;	/* g3d idle status0 */
		struct {
			unsigned int pa_idle_bus0:32;	/* g3d idle status */
			/*  */
			/* [0]:g3d idle timer expire */
			/* [1]:g3d all idle */
			/* [2]:g3d idle */
			/* [3]:pa idle */
			/* [4]:sf2pa_vp0_idle */
			/* [5]:cf2pa_vp0_idle */
			/* [6]:vs2pa_vp0_idle */
			/* [7]:vl2ap_vp0_idle */
			/* [8]:vx2pa_vp1_idle */
			/* [9]:bw2pa_vp1_idle */
			/* [10]:vs2pa_vp1_idle */
			/* [11]:vl2pa_vp1_idle */
			/* [12]:tx2pa_pp_idle */
			/* [13]:zp2pa_pp_idle */
			/* [14]:cp2pa_pp_idle */
			/* [15]:fs2pa_pp_idle */
			/* [16]:xy2pa_pp_idle */
			/* [17]:sb2pa_pp_idle */
			/* [18]:lv2pa_pp_idle */
			/* [19]:cb2pa_pp_idle */
			/* [20]:vc2pa_pp_idle */
			/* [21]:vx2pa_pp_idle */
			/* [23:22]:reserved */
			/* [24]:cq2pa_wr_idle */
			/* [25]:cq2pa_rd_idle */
			/* [26]:gl2pa_pp_idle */
			/* [27]:gl2pa_vp_idle */
			/* [31:28]:reserved */
		} s;
	} reg_pa_idle_status0;

	union			/* 0x0184, */
	{
		unsigned int dwValue;	/* g3d idle status1 */
		struct {
			unsigned int pa_idle_bus1:32;	/* g3d idle status */
			/*  */
			/* [0]:cc2pa_pp_l1_idle */
			/* [1]:cz2pa_pp_l1_idle */
			/* [2]:zc2pa_pp_l1_idle */
			/* [3]:ux2pa_vp_l1_idle */
			/* [4]:hc2pa_vp1_l1_idle */
			/* [5]:cz2pa_vp1_l1_idle */
			/* [6]:mx2pa_l2_idle */
			/* [7]:reserved */
			/* [8]:opencl2pa_vp_idle */
			/* [9]:clin2pa_vp_idle */
			/* [10]:vl2pa_vp1_bidle */
			/* [11]:vx2pa_vp1_bidle */
			/* [31:12]:reserved */
		} s;
	} reg_pa_idle_status1;

	union			/* 0x0188, */
	{
		unsigned int dwValue;	/* queue status */
		struct {
			unsigned int cq_idle:1;	/* cmdq is idle */
			unsigned int cq_qlding:1;	/* cq is loading  the queue buffer,
							for SW debug mode monitor */
			unsigned int reserved1:2;
			unsigned int ecc_htbuf_rptr:28;	/* read the error htbuffer read pointer
							when ecc happen */
		} s;
	} reg_queue_status;

	union			/* 0x018c, */
	{
		unsigned int dwValue;	/* cq done ht-buffer read pointer */
		struct {
			unsigned int cq_done_htbuf_rptr:32;	/* cmdq (ring buffer )head pointer read back */
			/* If the head_rd not equal the tail, the */
		} s;
	} reg_cq_done_htbuf_rptr;

	union			/* 0x0190, */
	{
		unsigned int dwValue;	/* sw flush ltc set */
		struct {
			unsigned int pa_flush_ltc_en:1;	/* sw flushes the g3d ltc according to apb bus. */
			/* default: 1'b0 */
			unsigned int pa_sync_sw_en:1;	/* sw finish the program condition(for DWP),
								and set this signal tell g3d keep going */
			/* default: 1'b0 */
		} s;
	} reg_pa_flush_ltc_set;

	union			/* 0x0194, */
	{
		unsigned int dwValue;	/* hw  flush ltc done */
		struct {
			unsigned int pa_flush_ltc_done:1;	/* g3d will set this bit 1, after flush LTC */
			unsigned int pa_sync_sw_done:1;	/* g3d stop g3d engine and wait sw program
								"pa_sync_sw_set" to release , and keep going. */
		} s;
	} reg_pa_flush_ltc_done;

	union			/* 0x0198, */
	{
		unsigned int dwValue;	/* pa frame data queue buffer base */
		struct {
			unsigned int pa_fdqbuf_base:32;	/* the frame base return data buffer base,16 byte alignment */
			/* default: 32'b0 */
		} s;
	} reg_pa_fdqbuf_base;

	union			/* 0x019c, */
	{
		unsigned int dwValue;	/* pa frame data queue buffer size */
		struct {
			unsigned int pa_fdqbuf_size:21;	/* the frame base return data buffer size ,
								max size 32MB, the unit is 8 Byte(64bits!!) */
			/* default: 21'b0 */
		} s;
	} reg_pa_fdqbuf_size;

	union			/* 0x01a0, */
	{
		unsigned int dwValue;	/* pa frame data queue buffer hw write pointer */
		struct {
			unsigned int reserved2:3;
			unsigned int pa_fdq_hwwptr:20;	/* the frame base return data buffer hw write pointer,
								the unit is 8byte ,
								need set "mcu2g3d_set" before reading */
		} s;
	} reg_pa_fdq_hwwptr;

	union			/* 0x01a4, */
	{
		unsigned int dwValue;	/* pa frame data queue buffer sw read pointer */
		struct {
			unsigned int reserved3:3;
			unsigned int pa_fdq_swrptr:20;	/* the frame base return data buffer
							sw read pointer,the unit is 8byte */
			/* default: 20'b0 */
		} s;
	} reg_pa_fdq_swrptr;

	union			/* 0x01a8, */
	{
		unsigned int dwValue;	/* roll back the pa frame data queue buffer hw write pointer update */
		struct {
			unsigned int pa_fdq_hwwptr_rb_upd:1;	/* roll back the pa frame data
								queue buffer hw write pointer update */
			/* default: 1'b0 */
			unsigned int reserved4:2;
			unsigned int pa_fdq_hwwptr_rb:20;	/* roll back the frame base return
								data buffer hw write pointer, the unit is 8byte ,
								need set "mcu2g3d_set" to set */
			/* default: 20'b0 */
		} s;
	} reg_pa_fdq_hwwptr_rb;

	union			/* 0x01ac, */
	{
		unsigned int dwValue;	/* read command queue task id */
		struct {
			unsigned int cq_taskid:8;	/* read the command task id */
			unsigned int pa_vp_taskid:8;	/* read the vp frame task id */
			unsigned int pa_pp_taskid:8;	/* read the pp frame task id */
		} s;
	} reg_taskid;

	union			/* 0x01b0, */
	{
		unsigned int dwValue;	/* read command queue frame head */
		struct {
			unsigned int cq_fmhead:32;	/* read the command frame head */
		} s;
	} reg_cq_fmhead;

	union			/* 0x01b4, */
	{
		unsigned int dwValue;	/* read command queue frame tail */
		struct {
			unsigned int cq_fmtail:32;	/* read the command frame tail */
		} s;
	} reg_cq_fmtail;

	union			/* 0x01b8, */
	{
		unsigned int dwValue;	/* read vp pipe frame id */
		struct {
			unsigned int pa_vp_frameid:32;	/* read vp pipe frame id */
		} s;
	} reg_pa_vp_frameid;

	union			/* 0x01bc, */
	{
		unsigned int dwValue;	/* read pp pipe frame id */
		struct {
			unsigned int pa_pp_frameid:32;	/* read pp pipe frame id */
		} s;
	} reg_pa_pp_frameid;

	union			/* 0x01c0, */
	{
		unsigned int dwValue;	/* pa debug rd */
		struct {
			unsigned int debug_list_id:12;	/* report the g3d engine list to SW, if hang */
		} s;
	} reg_pa_debug_rd;

	union			/* 0x01c4, */
	{
		unsigned int dwValue;	/* debug mode select */
		struct {
			unsigned int dbg_en:1;	/* g3d debug enable */
			/* default: 1'b0 */
			unsigned int reserved5:3;
			unsigned int dbg_top_sel:8;	/* g3d debug module select */
			/* 0:g3d  module stall status */
			/* 1:reserved */
			/* 2:reserved */
			/* 3:vl_vp0 */
			/* 4:reserved */
			/* 5:cf_vp0 */
			/* 6:sf_vp0 */
			/* 7:reserved */
			/* 8:vl_vp1 */
			/* 9:reserved */
			/* 10:bw_vp1 */
			/* 11:vx_vp1 */
			/* 12:reserve */
			/* 13:vx_pp */
			/* 14:cb_pp */
			/* 15:sb_pp */
			/* 16:xy_pp */
			/* 17:reserved */
			/* 18:cp_pp */
			/* 19:zp_pp */
			/* 20:tx_ pp */
			/* 21:reserve */
			/* 22:cc_pp */
			/* 23:zc_pp */
			/* 24:mx_eng */
			/* 25:mx_cac */
			/* 26:pm */
			/* others:reserved */
			/* default: 8'h0 */
			unsigned int dbg_sub_sel:8;	/* 8 bits for per module debug select */
			/* default: 8'h0 */
		} s;
	} reg_debug_mod_sel;

	union			/* 0x01c8, */
	{
		unsigned int dwValue;	/* debug bus */
		struct {
			unsigned int dbg_bus:32;	/* 32 bits debug bus */
		} s;
	} reg_debug_bus;

	union			/* 0x01cc, */
	{
		unsigned int dwValue;	/* cq/mx bypass/mx dcm */
		struct {
			unsigned int fire_by_list:1;	/* g3d fire list by list */
			/* default: 1'b0 */
			unsigned int cq_sngbl_en:1;	/* command queue set single mode request , no burst */
			/* default: 1'b0 */
			unsigned int mx_pseudobypass_en:1;	/* pseudo-bypass enable */
			/* 0:normal mode; */
			/* 1:pseudo-bypass mode */
			/* default: 1'b0 */
			unsigned int mx_axi_interleave_en:1;	/* axi interleave enable */
			/* 0:axi data in-order mode; */
			/* 1:axi data interleaving mode */
			/* default: 1'b0 */
			unsigned int mx_dcm_level:2;	/* mx clk gating level setting */
			/* 00: no performance loss, clk gating */
			/* 01:  little performance loss, clk gating */
			/* default: 2'b0 */
			unsigned int mx_dataprerdy_disable:1;	/* mx clk gating safe mode setting */
			/* 0: have clk gating */
			/* 1: always dataprerdy = 1, no clk gating */
			/* default: 1'b0 */
			unsigned int mx_preack_disable:1;	/* mx clk gating safe mode setting */
			/* 0: have clk gating */
			/* 1: always preack = 1, no clk gating */
			/* default: 1'b0 */
			unsigned int mx_axiid_set:2;	/* mfg axi id number from 32 to 4 */
			/* 00: 32 AXI ID, 01: 16 AXI ID */
			/* 10:  8 AXI ID, 11: 4 AXI ID */
			/* default: 2'b00 */
			unsigned int mx_axi_2ch_grain:8;	/* axi 2ch use */
			/* [6:4] granule setting, 0:64B, 1:128B, ..7:8KB */
			/* [2:0] 2 ch weighting, ch1 has N/4 weighting */
			/* default: 8'h10 */
			unsigned int mx_axi_even_wbl:1;	/* DDR4 AXI use */
			/* 0 : normal, 1: wr burst length is even for DDR4 */
			/* default: 1'h0 */
			unsigned int mx_ctrl_rvd:3;	/* mx ctrl space register if need */
			/* [0]: disable length cache clock off by freg enable bits */
			/* default: 3'h0 */
		} s;
	} reg_bypass_set;

	union			/* 0x0200, */
	{
		unsigned int dwValue;	/* g3d interrupt status result (after mask) */
		struct {
			unsigned int irq_masked:32;	/* interrupt status (masked) */
		} s;
	} reg_irq_status_masked;

	union			/* 0x0204, */
	{
		unsigned int dwValue;	/* g3d interrupt status mask */
		struct {
			unsigned int irq_mask:32;	/* interrupt mask */
			/* 0: enable interrupt */
			/* 1: disable interrupt */
			/* default: 32'hffffffff */
		} s;
	} reg_irq_status_mask;

	union			/* 0x0208, */
	{
		unsigned int dwValue;	/* g3d interrupt status raw data (before mask), and write one clear */
		struct {
			unsigned int irq_raw:32;	/* interrupt status (raw data) */
			/* [0]: cq preempt finish */
			/* [1]: cq resume finish */
			/* [2]: pa frame base data queue not empty */
			/* [3]: pa frame swap */
			/* [4]: pa frame end */
			/* [5]: pa sync sw */
			/* [6]: g3d hang */
			/* [7]: g3d mmce frame flush */
			/* [8]: g3d mmce frame end */
			/* [9]: cq error code check */
			/* [10]: mx debug */
			/* [11]: pwc snap */
			/* [12]: DFD Internal dump triggered */
			/* [13]: pm debug mode */
			/* [14]: vl negative index happen */
			/* [15]: areg apb timeout happen */
			/* [16]: ux_cmn response */
			/* [20]: ux_dwp0 response */
			/* [21]: ux_dwp1 response */
			/* default: 32'h0 */
		} s;
	} reg_irq_status_raw;

	union			/* 0x0300, */
	{
		unsigned int dwValue;	/* vx vertex related control setting */
		struct {
			unsigned int vx_vw_buf_mgn:8;	/* varying ring buffer margin */
			/* margin = (vx_vary_buf_mgn) K byte */
			/* default: 8'ha */
			unsigned int vx_bs_hd_test:1;	/* test mode for header no. execeed */
			/* default: 1'b0 */
			unsigned int vx_longstay_en:1;	/* longstay/release enable for imm mode */
			/* default: 1'b1 */
			unsigned int vx_bkjmp_en:2;	/* defer bank jump enable */
			/* 0x : disable */
			/* 10 : jump with 32 primitves */
			/* 11 : jump with 16 primitives */
			/* default: 2'h0 */
			unsigned int vx_bs_boff_mode:3;	/* boff mode */
			/* [0] : for header */
			/* [1] : for table */
			/* [2] : for block */
			/* 0 : stop gradurally */
			/* 1 : stop immediately */
			/* default: 3'h0 */
			unsigned int vx_dummy0:17;	/* dummy registers */
			/* default: 17'h0 */
		} s;
	} reg_vx_vtx_setting;

	union			/* 0x0304, */
	{
		unsigned int dwValue;	/* vx defer related control setting */
		struct {
			unsigned int vx_bintbxr_4x:1;	/* bin table extend always read 4 times for mbin */
			/* 0: backward  if possible */
			/* 1: always 4 times */
			/* default: 1'b0 */
			unsigned int vx_binbkr_4x:1;	/* bin block always read 4 times for mbin */
			/* 0: backward  if possible */
			/* 1: always 4 times */
			/* default: 1'b0 */
			unsigned int vx_dummy1:6;	/* dummy registers */
			/* default: 6'h0 */
			unsigned int vx_binhdr_thr:5;	/* bin head buffer threshold (when lower than this threshold
							will generate reads to let buffer full) */
			/* default: 5'h8 */
			unsigned int reserved6:3;
			unsigned int vx_bintbr_thr:5;	/* bin table buffer threshold */
			/* default: 5'h8 */
			unsigned int reserved7:3;
			unsigned int vx_binbkr_thr:7;	/* bin block buffer threshold */
			/* default: 7'h10 */
		} s;
	} reg_vx_defer_setting;

	union			/* 0x0380, */
	{
		unsigned int dwValue;	/* vl misc setting */
		struct {
			unsigned int vl_idxgen_waterlv:6;	/* vertex index read data buffer water level */
			/* default: 6'h20 */
			unsigned int vl_indirect_wrap_md:1;	/* DrawElementIndirect wrap on 32 bits or type bits */
			/* 0: wrap on 32 bits */
			/* 1: wrap by type */
			/* default: 1'b0 */
			unsigned int vl_tfb_safe_mode:1;	/* TransformFeedback safe mode,
								one 128bit write per component. */
			/* default: 1'b0 */
			unsigned int vl1_vxpbuf_lowth:8; /* VX pbuf low threshold, for VL1 flush end condition */
			/* default: 8'd48 */
			unsigned int vl1_vxpbuf_highth:8; /* VX pbuf high threshold, for VL1 flush start condition */
			/* default: 8'd144 */
			unsigned int vl_dummy0:8;	/* dummy registers */
			/* default: 8'd0 */
		} s;
	} reg_vl_setting;

	union			/* 0x0384, */
	{
		unsigned int dwValue;	/* vl0_flush_timer */
		struct {
			unsigned int vl0_flush_timer:16;	/* vl0_flush_timer */
			/* default: 16'h0001 */
			unsigned int vl1_flush_timer:16;	/* vl1_flush_timer */
			/* default: 16'h0fff */
		} s;
	} reg_vl_flush;

	union			/* 0x0388, */
	{
		unsigned int dwValue;	/* vl_flush_th water level */
		struct {
			unsigned int vl0_primfifo_lowth:10;	/* VL primfifo low threshold,
								for VL0 flush end condition */
			/* default: 10'd256 */
			unsigned int vl0_primfifo_highth:10;	/* VL primfifo high threshold,
								for VL0 flush start condition */
			/* default: 10'd510 */
			unsigned int vl_dummy1:12;	/* dummy registers */
			/* default: 12'd0 */
		} s;
	} reg_vl_flush_th;

	union			/* 0x0700, */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int cq_security:1;	/* allocate a security queue */
			/* default: 1'b0 */
		} s;
	} reg_cq_secure_ctrl;

	union			/* 0x0704, */
	{
		unsigned int dwValue;	/* mx secure0 start addr */
		struct {
			unsigned int mx_secure0_base_addr:20;	/* base address of secure range for Q buffer */
			/* default: 20'b0 */
			unsigned int reserved8:11;
			unsigned int mx_secure0_enable:1;	/* enable this range checking */
			/* default: 1'b0 */
		} s;
	} reg_mx_secure0_base_addr;

	union			/* 0x0708, */
	{
		unsigned int dwValue;	/* mx secure0 end addr */
		struct {
			unsigned int mx_secure0_top_addr:20;	/* base address of secure range for Q buffer */
			/* default: 20'b0 */
		} s;
	} reg_mx_secure0_top_addr;

	union			/* 0x0e00, */
	{
		unsigned int dwValue;	/* read register enable */
		struct {
			unsigned int rd_qreg_en:1;	/* read queue base registers enable(debug) */
			/* default: 1'b0 */
			unsigned int rd_rsminfo_en:1;	/* read resume info enable */
			/* default: 1'b0 */
			unsigned int rd_vpfmreg_en:1;	/* read pp frame base registers enable */
			/* default: 1'b0 */
			unsigned int rd_ppfmreg_en:1;	/* read pp frame base registers enable */
			/* default: 1'b0 */
			unsigned int rd_vp0reg_en:1;	/* read vp0 list base registers enable */
			/* default: 1'b0 */
			unsigned int rd_vp1reg_en:1;	/* read vp1 list base registers enable */
			/* default: 1'b0 */
			unsigned int rd_cbreg_en:1;	/* read pp cb  list base registers enable */
			/* default: 1'b0 */
			unsigned int rd_xyreg_en:1;	/* read pp xy list base registers enable */
			/* default: 1'b0 */
			unsigned int rd_fsreg_en:1;	/* read pp fs list base registers enable */
			/* default: 1'b0 */
			unsigned int rd_zpfreg_en:1;	/* read pp zpf list base registers enable */
			/* default: 1'b0 */
			unsigned int rd_zpbreg_en:1;	/* read pp zpb list base registers enable */
			/* default: 1'b0 */
			unsigned int rd_cpfreg_en:1;	/* read pp cpf  list base registers enable */
			/* default: 1'b0 */
			unsigned int rd_cpbreg_en:1;	/* read pp cpb list base registers enable */
			/* default: 1'b0 */
		} s;
	} reg_rd_reg_en;

	union			/* 0x0e04, */
	{
		unsigned int dwValue;	/* read offset */
		struct {
			unsigned int rd_offset:12;	/* read offset for queue/rsm/ frame / vp list base registers */
			/* default: 12'b0 */
		} s;
	} reg_rd_offset;

	union			/* 0x0e08, */
	{
		unsigned int dwValue;	/* qreg read data */
		struct {
			unsigned int qreg_rd:32;	/* queue base registers read */
		} s;
	} reg_qreg_rd;

	union			/* 0x0e0c, */
	{
		unsigned int dwValue;	/* resumed read data */
		struct {
			unsigned int rsminfo_rd:32;	/* resume information read */
		} s;
	} reg_rsminfo_rd;

	union			/* 0x0e10, */
	{
		unsigned int dwValue;	/* vp fmreg read data */
		struct {
			unsigned int vpfmreg_rd:32;	/* vp frame base registers read */
		} s;
	} reg_vpfmreg_rd;

	union			/* 0x0e14, */
	{
		unsigned int dwValue;	/* pp fmreg read data */
		struct {
			unsigned int ppfmreg_rd:32;	/* pp frame base registers read */
		} s;
	} reg_ppfmreg_rd;

	union			/* 0x0e18, */
	{
		unsigned int dwValue;	/* vp0reg read data */
		struct {
			unsigned int vp0reg_rd:32;	/* vp0 list base registers read */
		} s;
	} reg_vp0reg_rd;

	union			/* 0x0e1c, */
	{
		unsigned int dwValue;	/* vp1reg read data */
		struct {
			unsigned int vp1reg_rd:32;	/* vp1 list base registers read */
		} s;
	} reg_vp1reg_rd;

	union			/* 0x0e20, */
	{
		unsigned int dwValue;	/* cbreg read data */
		struct {
			unsigned int cbreg_rd:32;	/* pp cb list base registers read */
		} s;
	} reg_cbreg_rd;

	union			/* 0x0e24, */
	{
		unsigned int dwValue;	/* xyreg read data */
		struct {
			unsigned int xyreg_rd:32;	/* pp xy list base registers read */
		} s;
	} reg_xyreg_rd;

	union			/* 0x0e28, */
	{
		unsigned int dwValue;	/* fsreg read data */
		struct {
			unsigned int fsreg_rd:32;	/* pp fs list base registers read */
		} s;
	} reg_fsreg_rd;

	union			/* 0x0e2c, */
	{
		unsigned int dwValue;	/* zpfreg read data */
		struct {
			unsigned int zpfreg_rd:32;	/* pp zpf list base registers read */
		} s;
	} reg_zpfreg_rd;

	union			/* 0x0e30, */
	{
		unsigned int dwValue;	/* zpbreg read data */
		struct {
			unsigned int zpbreg_rd:32;	/* pp zpb list base registers read */
		} s;
	} reg_zpbreg_rd;

	union			/* 0x0e34, */
	{
		unsigned int dwValue;	/* cpfreg read data */
		struct {
			unsigned int cpfreg_rd:32;	/* pp cpf list base registers read */
		} s;
	} reg_cpfreg_rd;

	union			/* 0x0e38, */
	{
		unsigned int dwValue;	/* cpbreg read data */
		struct {
			unsigned int cpbreg_rd:32;	/* pp cpb list base registers read */
		} s;
	} reg_cpbreg_rd;

	union			/* 0x0e3c, */
	{
		unsigned int dwValue;	/* mxreg read data */
		struct {
			unsigned int mxreg_rd:32;	/* mx frame base registers read */
		} s;
	} reg_mxreg_rd;

	union			/* 0x0d00, */
	{
		unsigned int dwValue;	/* pwc related control setting */
		struct {
			unsigned int pwc_ctrl_en:1;	/* enable PWC */
			/* default: 1'b0 */
			unsigned int pwc_ctrl_timer_en:1;	/* enable timer */
			/* default: 1'b0 */
			unsigned int pwc_ctrl_stmp_en:1;	/* enable timestamp */
			/* default: 1'b0 */
			unsigned int pwc_ctrl_irq_en:1;	/* enable interrupt */
			/* default: 1'b0 */
			unsigned int pwc_ctrl_snap:1;	/* snap all counters and timestamp
								(also clear all internal counters) */
			/* default: 1'b0 */
			unsigned int pwc_ctrl_clr:1;	/* clear all counters and timestamp */
			/* default: 1'b0 */
			unsigned int reserved9:25;
			unsigned int pwc_ctrl_cgc_en:1;	/* enable clock gating cell */
			/* default: 1'b0 */
		} s;
	} reg_pwc_ctrl;

	union			/* 0x0d04, */
	{
		unsigned int dwValue;	/* pwc sel */
		struct {
			unsigned int pwc_sel_dw_vppipe:1;	/* dw vppipe mux selection 0 for
									dw pwc counter1, dw pwc counter4 */
			/* default: 1'b0 */
			unsigned int pwc_sel_mx_cu:1;	/* mx cu mux selection for mx pwc counter2 */
			/* default: 1'b0 */
			unsigned int pwc_sel_mx_af_0:1;	/* mx af mux selection 0 for mx pwc counter4 */
			/* default: 1'b0 */
			unsigned int pwc_sel_mx_af_1:1;	/* mx af mux selection 1 for mx pwc counter5 */
			/* default: 1'b0 */
			unsigned int pwc_sel_ff_ce:2;	/* ff ce mux selection for ff pwc counter6 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_vl:2;	/* ff vl mux selection for ff pwc counter7 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_vx:2;	/* ff vx mux selection for ff pwc counter8 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_sf:2;	/* ff sf mux selection for ff pwc counter9 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_cf:2;	/* ff cf mux selection for ff pwc counter10 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_bw:2;	/* ff bw mux selection for ff pwc counter11 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_sb:2;	/* ff sb mux selection for ff pwc counter12 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_pe:2;	/* ff pe mux selection for ff pwc counter13 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_cb_0:2;	/* ff cb mux selection 0 for ff pwc counter14 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_cb_1:2;	/* ff cb mux selection 1 for ff pwc counter15 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_xy_0:2;	/* ff xy mux selection 0 for ff pwc counter16 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_xy_1:2;	/* ff xy mux selection 1 for ff pwc counter17 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_po_0:2;	/* ff po mux selection 0 for ff pwc counter18 */
			/* default: 2'b0 */
			unsigned int pwc_sel_ff_po_1:2;	/* ff po mux selection 1 for ff pwc counter19 */
			/* default: 2'b0 */
		} s;
	} reg_pwc_sel;

	union			/* 0x0d08, */
	{
		unsigned int dwValue;	/* pwc timer */
		struct {
			unsigned int pwc_timer:32; /* timer to trigger snap, timestamp and interrupt (unit: cycle) */
			/* default: 32'h00ffffff */
		} s;
	} reg_pwc_timer;

	union			/* 0x0d0c, */
	{
		unsigned int dwValue;	/* pwc stmp */
		struct {
			unsigned int pwc_stmp:32;	/* timestamp, increased by one
								when (snap == 1) or (timer timeout) */
			/* default: 32'h0 */
		} s;
	} reg_pwc_stmp;

	union			/* 0x0d10, */
	{
		unsigned int dwValue;	/* dw pwc counter0 */
		struct {
			unsigned int pwc_dw_cnt_0:32;	/* dw pwc counter0 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_0;

	union			/* 0x0d14, */
	{
		unsigned int dwValue;	/* dw pwc counter1 */
		struct {
			unsigned int pwc_dw_cnt_1:32;	/* dw pwc counter1 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_1;

	union			/* 0x0d18, */
	{
		unsigned int dwValue;	/* dw pwc counter2 */
		struct {
			unsigned int pwc_dw_cnt_2:32;	/* dw pwc counter2 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_2;

	union			/* 0x0d1c, */
	{
		unsigned int dwValue;	/* dw pwc counter3 */
		struct {
			unsigned int pwc_dw_cnt_3:32;	/* dw pwc counter3 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_3;

	union			/* 0x0d20, */
	{
		unsigned int dwValue;	/* dw pwc counter4 */
		struct {
			unsigned int pwc_dw_cnt_4:32;	/* dw pwc counter4 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_4;

	union			/* 0x0d24, */
	{
		unsigned int dwValue;	/* dw pwc counter5 */
		struct {
			unsigned int pwc_dw_cnt_5:32;	/* dw pwc counter5 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_5;

	union			/* 0x0d28, */
	{
		unsigned int dwValue;	/* dw pwc counter6 */
		struct {
			unsigned int pwc_dw_cnt_6:32;	/* dw pwc counter6 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_6;

	union			/* 0x0d2c, */
	{
		unsigned int dwValue;	/* dw pwc counter7 */
		struct {
			unsigned int pwc_dw_cnt_7:32;	/* dw pwc counter7 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_7;

	union			/* 0x0d30, */
	{
		unsigned int dwValue;	/* dw pwc counter8 */
		struct {
			unsigned int pwc_dw_cnt_8:32;	/* dw pwc counter8 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_8;

	union			/* 0x0d34, */
	{
		unsigned int dwValue;	/* dw pwc counter9 */
		struct {
			unsigned int pwc_dw_cnt_9:32;	/* dw pwc counter9 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_9;

	union			/* 0x0d38, */
	{
		unsigned int dwValue;	/* dw pwc counter10 */
		struct {
			unsigned int pwc_dw_cnt_10:32;	/* dw pwc counter10 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_10;

	union			/* 0x0d3c, */
	{
		unsigned int dwValue;	/* dw pwc counter11 */
		struct {
			unsigned int pwc_dw_cnt_11:32;	/* dw pwc counter11 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_11;

	union			/* 0x0d40, */
	{
		unsigned int dwValue;	/* dw pwc counter12 */
		struct {
			unsigned int pwc_dw_cnt_12:32;	/* dw pwc counter12 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_12;

	union			/* 0x0d44, */
	{
		unsigned int dwValue;	/* dw pwc counter13 */
		struct {
			unsigned int pwc_dw_cnt_13:32;	/* dw pwc counter13 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_13;

	union			/* 0x0d48, */
	{
		unsigned int dwValue;	/* dw pwc counter14 */
		struct {
			unsigned int pwc_dw_cnt_14:32;	/* dw pwc counter14 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_14;

	union			/* 0x0d4c, */
	{
		unsigned int dwValue;	/* dw pwc counter15 */
		struct {
			unsigned int pwc_dw_cnt_15:32;	/* dw pwc counter15 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_dw_cnt_15;

	union			/* 0x0d50, */
	{
		unsigned int dwValue;	/* mx pwc counter0 */
		struct {
			unsigned int pwc_mx_cnt_0:32;	/* mx pwc counter0 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_mx_cnt_0;

	union			/* 0x0d54, */
	{
		unsigned int dwValue;	/* mx pwc counter1 */
		struct {
			unsigned int pwc_mx_cnt_1:32;	/* mx pwc counter1 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_mx_cnt_1;

	union			/* 0x0d58, */
	{
		unsigned int dwValue;	/* mx pwc counter2 */
		struct {
			unsigned int pwc_mx_cnt_2:32;	/* mx pwc counter2 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_mx_cnt_2;

	union			/* 0x0d5c, */
	{
		unsigned int dwValue;	/* mx pwc counter3 */
		struct {
			unsigned int pwc_mx_cnt_3:32;	/* mx pwc counter3 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_mx_cnt_3;

	union			/* 0x0d60, */
	{
		unsigned int dwValue;	/* mx pwc counter4 */
		struct {
			unsigned int pwc_mx_cnt_4:32;	/* mx pwc counter4 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_mx_cnt_4;

	union			/* 0x0d64, */
	{
		unsigned int dwValue;	/* mx pwc counter5 */
		struct {
			unsigned int pwc_mx_cnt_5:32;	/* mx pwc counter5 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_mx_cnt_5;

	union			/* 0x0d68, */
	{
		unsigned int dwValue;	/* mx pwc counter6 */
		struct {
			unsigned int pwc_mx_cnt_6:32;	/* mx pwc counter6 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_mx_cnt_6;

	union			/* 0x0d6c, */
	{
		unsigned int dwValue;	/* mx pwc counter7 */
		struct {
			unsigned int pwc_mx_cnt_7:32;	/* mx pwc counter7 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_mx_cnt_7;

	union			/* 0x0d70, */
	{
		unsigned int dwValue;	/* tx pwc counter0 */
		struct {
			unsigned int pwc_tx_cnt_0:32;	/* tx pwc counter0 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_tx_cnt_0;

	union			/* 0x0d74, */
	{
		unsigned int dwValue;	/* tx pwc counter1 */
		struct {
			unsigned int pwc_tx_cnt_1:32;	/* tx pwc counter1 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_tx_cnt_1;

	union			/* 0x0d78, */
	{
		unsigned int dwValue;	/* tx pwc counter2 */
		struct {
			unsigned int pwc_tx_cnt_2:32;	/* tx pwc counter2 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_tx_cnt_2;

	union			/* 0x0d7c, */
	{
		unsigned int dwValue;	/* tx pwc counter3 */
		struct {
			unsigned int pwc_tx_cnt_3:32;	/* tx pwc counter3 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_tx_cnt_3;

	union			/* 0x0d80, */
	{
		unsigned int dwValue;	/* ff pwc counter0 */
		struct {
			unsigned int pwc_ff_cnt_0:32;	/* ff pwc counter0 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_0;

	union			/* 0x0d84, */
	{
		unsigned int dwValue;	/* ff pwc counter1 */
		struct {
			unsigned int pwc_ff_cnt_1:32;	/* ff pwc counter1 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_1;

	union			/* 0x0d88, */
	{
		unsigned int dwValue;	/* ff pwc counter2 */
		struct {
			unsigned int pwc_ff_cnt_2:32;	/* ff pwc counter2 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_2;

	union			/* 0x0d8c, */
	{
		unsigned int dwValue;	/* ff pwc counter3 */
		struct {
			unsigned int pwc_ff_cnt_3:32;	/* ff pwc counter3 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_3;

	union			/* 0x0d90, */
	{
		unsigned int dwValue;	/* ff pwc counter4 */
		struct {
			unsigned int pwc_ff_cnt_4:32;	/* ff pwc counter4 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_4;

	union			/* 0x0d94, */
	{
		unsigned int dwValue;	/* ff pwc counter5 */
		struct {
			unsigned int pwc_ff_cnt_5:32;	/* ff pwc counter5 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_5;

	union			/* 0x0d98, */
	{
		unsigned int dwValue;	/* ff pwc counter6 */
		struct {
			unsigned int pwc_ff_cnt_6:32;	/* ff pwc counter6 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_6;

	union			/* 0x0d9c, */
	{
		unsigned int dwValue;	/* ff pwc counter7 */
		struct {
			unsigned int pwc_ff_cnt_7:32;	/* ff pwc counter7 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_7;

	union			/* 0x0da0, */
	{
		unsigned int dwValue;	/* ff pwc counter8 */
		struct {
			unsigned int pwc_ff_cnt_8:32;	/* ff pwc counter8 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_8;

	union			/* 0x0da4, */
	{
		unsigned int dwValue;	/* ff pwc counter9 */
		struct {
			unsigned int pwc_ff_cnt_9:32;	/* ff pwc counter9 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_9;

	union			/* 0x0da8, */
	{
		unsigned int dwValue;	/* ff pwc counter10 */
		struct {
			unsigned int pwc_ff_cnt_10:32;	/* ff pwc counter10 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_10;

	union			/* 0x0dac, */
	{
		unsigned int dwValue;	/* ff pwc counter11 */
		struct {
			unsigned int pwc_ff_cnt_11:32;	/* ff pwc counter11 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_11;

	union			/* 0x0db0, */
	{
		unsigned int dwValue;	/* ff pwc counter12 */
		struct {
			unsigned int pwc_ff_cnt_12:32;	/* ff pwc counter12 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_12;

	union			/* 0x0db4, */
	{
		unsigned int dwValue;	/* ff pwc counter13 */
		struct {
			unsigned int pwc_ff_cnt_13:32;	/* ff pwc counter13 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_13;

	union			/* 0x0db8, */
	{
		unsigned int dwValue;	/* ff pwc counter14 */
		struct {
			unsigned int pwc_ff_cnt_14:32;	/* ff pwc counter14 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_14;

	union			/* 0x0dbc, */
	{
		unsigned int dwValue;	/* ff pwc counter15 */
		struct {
			unsigned int pwc_ff_cnt_15:32;	/* ff pwc counter15 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_15;

	union			/* 0x0dc0, */
	{
		unsigned int dwValue;	/* ff pwc counter16 */
		struct {
			unsigned int pwc_ff_cnt_16:32;	/* ff pwc counter16 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_16;

	union			/* 0x0dc4, */
	{
		unsigned int dwValue;	/* ff pwc counter17 */
		struct {
			unsigned int pwc_ff_cnt_17:32;	/* ff pwc counter17 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_17;

	union			/* 0x0dc8, */
	{
		unsigned int dwValue;	/* ff pwc counter18 */
		struct {
			unsigned int pwc_ff_cnt_18:32;	/* ff pwc counter18 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_18;

	union			/* 0x0dcc, */
	{
		unsigned int dwValue;	/* ff pwc counter19 */
		struct {
			unsigned int pwc_ff_cnt_19:32;	/* ff pwc counter19 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_ff_cnt_19;

	union			/* 0x0dd0, */
	{
		unsigned int dwValue;	/* counter enable 0 */
		struct {
			unsigned int pwc_counter_dis_0:32;	/* control separately 48 power counters in g3d_pwc */
			/* 0: enable  1: disable */
			/* [0] enable/disabel for pwc_dw_cnt_0 */
			/* [1] enable/disabel for pwc_dw_cnt_1 */
			/* [2] enable/disabel for pwc_dw_cnt_2 */
			/* [3] enable/disabel for pwc_dw_cnt_3 */
			/* [4] enable/disabel for pwc_dw_cnt_4 */
			/* [5] enable/disabel for pwc_dw_cnt_5 */
			/* [6] enable/disabel for pwc_dw_cnt_6 */
			/* [7] enable/disabel for pwc_dw_cnt_7 */
			/* [8] enable/disabel for pwc_dw_cnt_8 */
			/* [9] enable/disabel for pwc_dw_cnt_9 */
			/* [10] enable/disabel for pwc_dw_cnt_10 */
			/* [11] enable/disabel for pwc_dw_cnt_11 */
			/* [12] enable/disabel for pwc_dw_cnt_12 */
			/* [13] enable/disabel for pwc_dw_cnt_13 */
			/* [14] enable/disabel for pwc_dw_cnt_14 */
			/* [15] enable/disabel for pwc_dw_cnt_15 */
			/* [16] enable/disabel for pwc_mx_cnt_0 */
			/* [17] enable/disabel for pwc_mx_cnt_1 */
			/* [18] enable/disabel for pwc_mx_cnt_2 */
			/* [19] enable/disabel for pwc_mx_cnt_3 */
			/* [20] enable/disabel for pwc_mx_cnt_4 */
			/* [21] enable/disabel for pwc_mx_cnt_5 */
			/* [22] enable/disabel for pwc_mx_cnt_6 */
			/* [23] enable/disabel for pwc_mx_cnt_7 */
			/* [24] enable/disabel for pwc_tx_cnt_0 */
			/* [25] enable/disabel for pwc_tx_cnt_1 */
			/* [26] enable/disabel for pwc_tx_cnt_2 */
			/* [27] enable/disabel for pwc_tx_cnt_3 */
			/* [28] enable/disabel for pwc_ff_cnt_0 */
			/* [29] enable/disabel for pwc_ff_cnt_1 */
			/* [30] enable/disabel for pwc_ff_cnt_2 */
			/* [31] enable/disabel for pwc_ff_cnt_3 */
			/* default: 32'h0 */
		} s;
	} reg_pwc_counter_dis_0;

	union			/* 0x0dd4, */
	{
		unsigned int dwValue;	/* counter enable 1 */
		struct {
			unsigned int pwc_counter_dis_1:16;	/* control separately 48 power counters in g3d_pwc */
			/* 0: enable  1: disable */
			/* [0] enable/disabel for pwc_ff_cnt_4 */
			/* [1] enable/disabel for pwc_ff_cnt_5 */
			/* [2] enable/disabel for pwc_ff_cnt_6 */
			/* [3] enable/disabel for pwc_ff_cnt_7 */
			/* [4] enable/disabel for pwc_ff_cnt_8 */
			/* [5] enable/disabel for pwc_ff_cnt_9 */
			/* [6] enable/disabel for pwc_ff_cnt_10 */
			/* [7] enable/disabel for pwc_ff_cnt_11 */
			/* [8] enable/disabel for pwc_ff_cnt_12 */
			/* [9] enable/disabel for pwc_ff_cnt_13 */
			/* [10] enable/disabel for pwc_ff_cnt_14 */
			/* [11] enable/disabel for pwc_ff_cnt_15 */
			/* [12] enable/disabel for pwc_ff_cnt_16 */
			/* [13] enable/disabel for pwc_ff_cnt_17 */
			/* [14] enable/disabel for pwc_ff_cnt_18 */
			/* [15] enable/disabel for pwc_ff_cnt_19 */
			/* default: 16'h0 */
		} s;
	} reg_pwc_counter_dis_1;

	union			/* 0x0f00, */
	{
		unsigned int dwValue;	/* pm flow control_config */
		struct {
			unsigned int debug_pm_en:1;	/* enable PM */
			/* default: 1'b0 */
			unsigned int pm_rst_all_cntr:1;	/* reset all counter */
			/* default: 1'b0 */
			unsigned int pm_debug_mode:1;	/* into debug mode */
			/* default: 1'b0 */
			unsigned int pm_force_idle:1;	/* force into idle mode */
			/* default: 1'b0 */
			unsigned int pm_clear_reg:1;	/* pm SW reset */
			/* default: 1'b0 */
			unsigned int pm_force_write:1;	/* force into write mode */
			/* default: 1'b0 */
			unsigned int pm_sample_level:2;	/* define sample level */
			/* 00: HW sub-frame, */
			/* 01: SW sub-frame, */
			/* 10: Frame */
			/* default: 2'b0 */
			unsigned int pm_bak_up_reg:24;	/* bak up reg */
			/* [8]: stop time stamp */
			/* [9]: 0xF1C/areg_pm_sturation[0]=freg_opencl_en */
			/* [27:10]:0xF1C/areg_pm_sturation[27:10]=unit counter[17:0] */
			/* [31:28]:0xF1C/areg_pm_sturation[31:28]=state[3:0] */
			/* [31:30]:option for reset fifo utilization/stall starve counter */
			/* default: 24'h0 */
		} s;
	} reg_pm_reg0;

	union			/* 0x0f04, */
	{
		unsigned int dwValue;	/* start to count from the m-th unit */
		struct {
			unsigned int pm_m_run:32;	/* ignore m-1 unit, start to count from the m-th unit,
							unit is defined by 0xF00: pm_sample_level */
			/* default: 32'h0 */
		} s;
	} reg_pm_m_run;

	union			/* 0x0f08, */
	{
		unsigned int dwValue;	/* sample n units in one run */
		struct {
			unsigned int pm_n_units:32;	/* sample n units in one run */
			/* default: 32'h0 */
		} s;
	} reg_pm_n_units;

	union			/* 0x0f0c, */
	{
		unsigned int dwValue;	/* dram start address to save counter value */
		struct {
			unsigned int pm_dram_start_addr:32;	/* DRAM write start address */
			/* default: 32'h0 */
		} s;
	} reg_pm_dram_start_addr;

	union			/* 0x0f10, */
	{
		unsigned int dwValue;	/* select which counter showed in areg_dbg_bus */
		struct {
			unsigned int pm_dbg_bus_sel:32;	/* select which counter shown in Areg debug port */
			/* default: 32'h0 */
		} s;
	} reg_pm_dbg_bus_sel;

	union			/* 0x0f14, */
	{
		unsigned int dwValue;	/* module mx, select MX rd channel into AXI to count */
		struct {
			unsigned int pm_mxaxir_chsel:21; /* module mx, select MX rd channel into AXI for counting */
			/* * Rd/Wr need pairs with pm_mxaxiw_chsel [0]: cp_c_rd * [1]: ux_vp_rd * [2]: zp_z_rd * */
			/* [3]: vx_hc_rd* [4]: xy_cac_rd* [5]:vx_vtx_rd* */
			/* [6]: tx_ch0_rd [7]:tx_glb_rd [8]:vl_idx_rd */
			/* [9]:vl_att_rd [10]:ux_ic_rd [11]:pe_pe_rd */
			/* [12]:xy_buf_rd [13]:vx_bl_rd [14]:tx_ch1_rd */
			/* [15]:gl_gl_rd [16]:ux_cp_rd [17]:cq_cq_rp */
			/* [18]:vl_idx_rp [19]:vx_vtx_rp [20]: reserved */
			/* default: 21'h0 */
			unsigned int pm_mxaxir_chsel_lc:1;	/* module mx, select MX rd lc (length cache)
									channel into AXI for counting */
			/*  */
			/*  */
			/* default: 1'b0 */
			unsigned int reserved10:9;
			unsigned int pm_mxaxi_chsel_en:1;	/* enable MX channel count from AXI interface */
			/* 0: disable MX channel count from AXI */
			/* 1: enable MX channel count from AXI */
			/* default: 1'b0 */
		} s;
	} reg_pm_mxaxi_chsel_0;

	union			/* 0x0f18, */
	{
		unsigned int dwValue;	/* module mx, select MX wr channel into AXI to count */
		struct {
			unsigned int pm_mxaxiw_chsel:16;	/* module mx, select MX wr channel
									into AXI for counting */
			/* * Rd/Wr need pairs with pm_mxaxir_chsel  [0]: cp_c_wr* [1]: ux_vp_wr* [2]: zp_z_wr* */
			/* [3]:vx_hc_wr*[4]:xy_cac_wr*[5]:vx_vtx_wr * */
			/* [6]: vl_tfb_wr [7]: vx_bs_wr [8]:cq_cq_wp */
			/* [9]:cp_c_wp [10]:cp_ebf_wp [11]:pe_pe_wp */
			/* [12]:vx_vtx_wp [13]:zp_z_wp[14]:xy_cac_wp */
			/* [15]:pm_pm_wp */
			/* default: 16'h0 */
			unsigned int pm_mxaxiw_chsel_lc:1;	/* module mx, select MX wr lc (length cache)
									channel into AXI for counting */
			/*  */
			/*  */
			/* default: 1'b0 */
		} s;
	} reg_pm_mxaxi_chsel_1;

	union			/* 0x0f1c, */
	{
		unsigned int dwValue;	/* indicate saturation */
		struct {
			unsigned int pm_saturation:32;	/* saturation, stop to count */
			/* 1: means saturation occurred in the counter */
		} s;
	} reg_pm_saturation;

	union			/* 0x0f20, */
	{
		unsigned int dwValue;	/* size of data written to dram */
		struct {
			unsigned int pm_dram_size:32;	/* sw set the size of data written to dram */
			/* default: 32'h0 */
		} s;
	} reg_pm_dram_size;

	union			/* 0x0f24, */
	{
		unsigned int dwValue;	/* select debug signal from which module under partition FF */
		struct {
			unsigned int debug_ff_module_sel_0:5;	/* Select module for Debug Group 0 */
			/* For each debug group */
			/* 0: BW */
			/* 1: CE */
			/* 2: CB */
			/* 3: CF */
			/* 4: PO */
			/* 5: SB */
			/* 6: SF */
			/* 7: VL */
			/* 8: VX */
			/* 9: XY */
			/* 10: UX Common */
			/* 11: MF */
			/* 12: PE */
			/* 13:PM_TYPICAL */
			/* 31:Perf Monitor */
			/* default: 5'h1f */
			unsigned int reserved11:3;
			unsigned int debug_ff_module_sel_1:5;	/* Select module for Debug Group 1 */
			/* As above */
			/* default: 5'h1f */
			unsigned int reserved12:3;
			unsigned int debug_ff_module_sel_2:5;	/* Select module for Debug Group 2 */
			/* As above */
			/* default: 5'h1f */
			unsigned int reserved13:3;
			unsigned int debug_ff_module_sel_3:5;	/* Select module for Debug Group 3 */
			/* As above */
			/* default: 5'h1f */
		} s;
	} reg_debug_ff_module_sel;

	union			/* 0x0f28, */
	{
		unsigned int dwValue;	/* select pm signal from which module under partition FF */
		struct {
			unsigned int pm_ff_module_sel_0:5;	/* Select module for PM Group 0 */
			/* PM_TYPICAL is the module collecting typical pm signal */
			/* For each debug group */
			/* 0: BW */
			/* 1: CE */
			/* 2: CB */
			/* 3: CF */
			/* 4: PO */
			/* 5: SB */
			/* 6: SF */
			/* 7: VL */
			/* 8: VX */
			/* 9: XY */
			/* 10: UX Common */
			/* 11: MF */
			/* 12: PE */
			/* 13:PM_TYPICAL */
			/* default: 5'h0 */
			unsigned int reserved14:3;
			unsigned int pm_ff_module_sel_1:5;	/* Select module for PM Group 1 */
			/* As above */
			/* default: 5'h0 */
			unsigned int reserved15:3;
			unsigned int pm_ff_module_sel_2:5;	/* Select module for PM Group 2 */
			/* As above */
			/* default: 5'h0 */
			unsigned int reserved16:3;
			unsigned int pm_ff_module_sel_3:5;	/* Select module for PM Group 3 */
			/* As above */
			/* default: 5'h0 */
		} s;
	} reg_pm_ff_module_sel;

	union			/* 0x0f2c, */
	{
		unsigned int dwValue;	/* indicate which counter reset to 0 while signal equal to 0 */
		struct {
			unsigned int pm_counter_reset:32;	/* for shader pm,  some counter value
									need reset while signal equal 0 */
			/* default: 32'h0 */
		} s;
	} reg_pm_counter_reset;

	union			/* 0x0f30, */
	{
		unsigned int dwValue;	/* select debug group to engine output */
		struct {
			unsigned int debug_group_sel:32;	/* all engine shared same debug mux select pin */
			/* Bit [7:0]: Select debug group for output debug group 0 */
			/* Bit [15:8]: Select debug group for output debug group 1 */
			/* Bit [23:16]: Select debug group for output debug group 2 */
			/* Bit [31:24]: Select debug group for output debug group 3 */
			/*  */
			/* For each output group */
			/* 0: PM Group 0 and 1 */
			/* 1: PM Group 2 and 3 */
			/* 2:Debug ID */
			/* 3: Inverse of Debug ID */
			/* 4~255: design dependent */
			/* default: 32'h0 */
		} s;
	} reg_debug_group_sel;

	union			/* 0x0f34, */
	{
		unsigned int dwValue;	/* select pm group in engine */
		struct {
			unsigned int pm_group_sel:32;	/* all engine shared same pm mux select pin */
			/* Bit [7:0]: Select group for pm signal 07-00 */
			/* Bit [15:8]: Select group for pm signal 15-08 */
			/* Bit [23:16]: Select group for pm signal 23-16 */
			/* Bit [31:24]: Select group for pm signal 31-24 */
			/* Fir each pm group */
			/* 0: PM group 0 */
			/* 1: PM group 1 */
			/* 2: PM group 2 */
			/* default: 32'h0 */
		} s;
	} reg_pm_group_sel;

	union			/* 0x0f38, */
	{
		unsigned int dwValue;	/* central pm select signal from which partition */
		struct {
			unsigned int debug_central_sel_0:3; /* select debug signal Bit15~Bit00 from which partition */
			/* 0: DWP */
			/* 1: MX */
			/* 2: TX */
			/* 3: FF */
			/* default: 3'b0 */
			unsigned int reserved17:1;
			unsigned int debug_central_sel_1:3; /* select debug signal Bit31~Bit16 from which partition */
			/* As above */
			/* default: 3'b0 */
			unsigned int reserved18:1;
			unsigned int debug_central_sel_2:3; /* select debug signal Bit47~Bit32 from which partition */
			/* As above */
			/* default: 3'b0 */
			unsigned int reserved19:1;
			unsigned int debug_central_sel_3:3; /* select debug signal Bit63~Bit48 from which partition */
			/* As above */
			/* default: 3'b0 */
			unsigned int reserved20:1;
			unsigned int pm_central_sel_0:3; /* select pm signal Bit7~Bit0 from which partition */
			/* 0: DWP */
			/* 1: MX */
			/* 2: TX */
			/* 3: FF */
			/* default: 3'b0 */
			unsigned int reserved21:1;
			unsigned int pm_central_sel_1:3;	/* select pm signal Bit15~Bit8 from which partition */
			/* As above */
			/* default: 3'b0 */
			unsigned int reserved22:1;
			unsigned int pm_central_sel_2:3;	/* select pm signal Bit23~Bit16 from which partition */
			/* As above */
			/* default: 3'b0 */
			unsigned int reserved23:1;
			unsigned int pm_central_sel_3:3;	/* select pm signal Bit31~Bit24 from which partition */
			/* As above */
			/* default: 3'b0 */
		} s;
	} reg_pm_debug_central_sel;

	union			/* 0x0f3c, */
	{
		unsigned int dwValue;	/* Debug Bus final mux */
		struct {
			unsigned int debug_final_bus_b0_sel:3;	/* select debug io bit7 ~ bit0 from which byte */
			/* default: 3'h0 */
			unsigned int reserved24:1;
			unsigned int debug_final_bus_b1_sel:3;	/* select debug io bit15 ~ bit8 from which byte */
			/* default: 3'h1 */
			unsigned int reserved25:1;
			unsigned int debug_final_bus_b2_sel:3;	/* select debug io bit23 ~ bit16 from which byte */
			/* default: 3'h2 */
			unsigned int reserved26:1;
			unsigned int debug_final_bus_b3_sel:3;	/* select debug io bit31 ~ bit24 from which byte */
			/* default: 3'h3 */
			unsigned int reserved27:1;
			unsigned int debug_final_trig_b0_sel:3;	/* select debug trigger bit7 ~ bit0 from which byte */
			/* default: 3'h4 */
			unsigned int reserved28:1;
			unsigned int debug_final_trig_b1_sel:3;	/* select debug trigger bit15 ~ bit8 from which byte */
			/* default: 3'h5 */
			unsigned int reserved29:1;
			unsigned int debug_final_trig_b2_sel:3; /* select debug trigger bit23 ~ bit16 from which byte */
			/* default: 3'h6 */
			unsigned int reserved30:1;
			unsigned int debug_final_trig_b3_sel:3;	/* select debug trigger bit31 ~ bit24 from which byte */
			/* default: 3'h7 */
		} s;
	} reg_debug_final_mux;

	union			/* 0x0f40, */
	{
		unsigned int dwValue;	/* time stamp for APB read */
		struct {
			unsigned int pm_time_stamp:32;	/* time stamp counts between pm_en rising and  falling.
							Time stamp will also be record to DRAM while flush */
			/* default: 32'h0 */
		} s;
	} reg_pm_time_stamp;

	union			/* 0x0f44, */
	{
		unsigned int dwValue;	/* enable individual counter */
		struct {
			unsigned int pm_counter_en:32;	/* turn off/on individual counters by the register */
			/* 0: disable, counter won't accumulate */
			/* 1. enable, counter accumulate with active signal */
			/* default: 32'h0 */
		} s;
	} reg_pm_counter_en;

	union			/* 0x0f60, */
	{
		unsigned int dwValue;	/* enable debug control logic */
		struct {
			unsigned int dbg_ctrl_enable:1;	/* Enable debug control logic */
			/* default: 1'b0 */
			unsigned int dbg_ctrl_force_frame_load:1;	/* Debug purpose. Override the
								frame_loaded indicator inside g3d_dbg_ctrl to 1 */
			/* 0: normal */
			/* 1: force frame_loaded = 1 */
			/* default: 1'b0 */
			unsigned int dbg_target_frame_by:2;	/* Debug target frame is set by */
			/*  */
			/* 0: all frames */
			/* 1: freg_debug_flag */
			/* 2: freg_frame_id */
			/* 3: all frames after freg_frame_id */
			/* default: 2'b00 */
			unsigned int dbg_target_frame_phase:2;	/* Debug target frame at VP/PP phase */
			/* [0]: VP phase */
			/* [1]: PP phasae */
			/* default: 2'b11 */
			unsigned int dbg_ctrl_frame_loaded:2;	/* Indicate that frame is loaded in g3d */
			/* [0]: VP phase */
			/* [1]: PP phasae */
			/* default: 2'b00 */
			unsigned int dbg_bus_trigger_en:1;	/* Dedicate trigger pin enable */
			/* 0: Disable */
			/* 1: Enable */
			/* default: 1'b0 */
			unsigned int dbg_dummy_0:1;	/* dummy register 0 for eco */
			/* default: 1'b0 */
			unsigned int dbg_bus_trigger_frame:2;	/* trigger condition qualificateion by frame */
			/* 0: Qualifiedy by target frame */
			/* 1: for each frame */
			/* 2: trigger on target frame regardless the condition */
			/* 3: force trigger high */
			/* default: 2'h0 */
			unsigned int dbg_bus_trigger_pin_num:5;	/* The pin number that trigger pin will replace with */
			/* default: 5'h0 */
			unsigned int dbg_dummy_1:1;	/* dummy register 1 for eco */
			/* default: 1'b0 */
			unsigned int dbg_dummy_2:1;	/* dummy register 2 for eco */
			/* default: 1'b0 */
			unsigned int dbg_bus_trigger_pin_en:1;	/* Enable replacement with trigger pin */
			/* default: 1'b0 */
			unsigned int dbg_bus_trigger_mode:2;	/* Dedicate trigger state mode */
			/* 0: Either condition 0 or condition 1 is hit */
			/* 1: Both condition 0 and condition 1 are hit */
			/* 2: condition 0 than condition 1 sequence */
			/* 3: condition 0 to condition 1 transition */
			/* default: 2'h0 */
			unsigned int dbg_dummy_3:1;	/* dummy register 3 for eco */
			/* default: 1'b0 */
			unsigned int dbg_dummy_4:1;	/* dummy register 4 for eco */
			/* default: 1'b0 */
			unsigned int dbg_dump_enable:1;	/* Enable Internal Dump */
			/* if dbg_ctrl_enable == 0, the it will target all frames */
			/* 0: Disable Internal Dump */
			/* 1: Enable internal Dump */
			/* default: 1'b0 */
			unsigned int dbg_dump_flush_info_en:1;	/* When set, it will flush SRAM data to
									system memory at frame end */
			/* 0: Will not flush */
			/* 1: Will flush */
			/* default: 1'b0 */
			unsigned int dbg_dump_into_register_en:1; /* Use register dbg_dump_data_dw* as storage */
			/* 0: Use SRAM as storage */
			/* 1: Use dbg_dump_data_dw* */
			/* default: 1'b0 */
			unsigned int dbg_dump_stop_when_full:1;	/* Stop capturing data when the storage is full */
			/* 0: Ring buffer mode */
			/* 1: Stop capturing when storage is full */
			/* default: 1'b1 */
			unsigned int dbg_dump_32k_mode:1;	/* When set, Internal Dump will use only 32KB SRAM. */
			/* This register is only valid when dbg_dump_into_register_en == 0 */
			/* 0: 64KB SRAM mode */
			/* 1: 32KB SRAM mode */
			/* default: 1'b1 */
			unsigned int dbg_dump_intr_en:1;	/* When set, Internal Dump will request interrupt
									when trigger condition hit. */
			/* 0: enable interrupt */
			/* 1: disable interrupt */
			/* default: 1'b0 */
			unsigned int dbg_dummy_5:1;	/* dummy register 5 for eco */
			/* default: 1'b0 */
			unsigned int dbg_clk_en:1;	/* enable clock for DFD engine */
			/* default: 1'b0 */
		} s;
	} reg_dbg_ctrl;

	union			/* 0x0f64, */
	{
		unsigned int dwValue;	/* Target Frame ID */
		struct {
			unsigned int dbg_target_frame_id:32;	/* Target frame ID if dbg_target_frame_by == 2 or 3 */
			/* default: 32'h0 */
		} s;
	} reg_dbg_target_frame_id;

	union			/* 0x0f68, */
	{
		unsigned int dwValue;	/* Target HW subframe offset */
		struct {
			unsigned int dbg_target_frame_offset:16;	/* Target frame offset. */
			/* Begin DFD engine after count numbers of HW subframe marked as target frame. */
			/* 0x0 means begin DFD engine regardless the offset. */
			/* 0x1 means the first target hardware subframe hit */
			/* default: 16'h0 */
			unsigned int dbg_target_frame_curr_offset:16;	/* Current hardware subframe offset count */
			/* default: 16'h0 */
		} s;
	} reg_dbg_target_frame_offset;

	union			/* 0x0f6c, */
	{
		unsigned int dwValue;	/* debug bus control */
		struct {
			unsigned int dbg_bus_c0_inversed:1;	/* Inverse condition 0 */
			/* default: 1'b0 */
			unsigned int dbg_bus_c0_source:1;	/* condition 0 source */
			/* 0: from bus[63:32] */
			/* 1: from bus[31:0] */
			/* default: 1'b0 */
			unsigned int dbg_bus_c0_change:1;	/* Ignore the dbg_c0_pattern, c0hit if the bits
									selected by dbg_c0_mask are changed */
			/* default: 1'b0 */
			unsigned int reserved31:1;
			unsigned int dbg_bus_c1_inversed:1;	/* Inverse condition 1 */
			/* default: 1'b0 */
			unsigned int dbg_bus_c1_source:1;	/* condition 1 source */
			/* 0: from bus[63:32] */
			/* 1: from bus[31:0] */
			/* default: 1'b0 */
			unsigned int dbg_bus_c1_change:1;	/* Ignore the dbg_c1_pattern, c1hit if the bits
									selected by dbg_c1_mask are changed */
			/* default: 1'b0 */
			unsigned int reserved32:24;
			unsigned int dbg_bus_triggered:1;	/* The trigger condition has ever hit. */
			/* 0: Not trigger */
			/* 1: Triggered */
			/* default: 1'b0 */
		} s;
	} reg_dbg_bus_ctrl;

	union			/* 0x0f70, */
	{
		unsigned int dwValue;	/* Condition 0 mask of dedicate trigger */
		struct {
			unsigned int dbg_bus_c0_mask:32;	/* Condition 0 mask of debug bus dedicate trigger */
			/* 0: condition 0 will never be hit */
			/* default: 32'h0 */
		} s;
	} reg_dbg_bus_c0_mask;

	union			/* 0x0f74, */
	{
		unsigned int dwValue;	/* Condition 0 of dedicate trigger */
		struct {
			unsigned int dbg_bus_c0_pattern:32;	/* Condition 0 match pattern of
									debug bus dedicate trigger */
			/* It is useless when dbg_bus_c0_change = 1 */
			/* default: 32'h0 */
		} s;
	} reg_dbg_bus_c0_pattern;

	union			/* 0x0f78, */
	{
		unsigned int dwValue;	/* Condition 0 mask of dedicate trigger */
		struct {
			unsigned int dbg_bus_c1_mask:32;	/* Condition 1 mask of
									debug bus dedicate trigger */
			/* 0: condition 1 will never be hit */
			/* default: 32'h0 */
		} s;
	} reg_dbg_bus_c1_mask;

	union			/* 0x0f7c, */
	{
		unsigned int dwValue;	/* Condition 0 of dedicate trigger */
		struct {
			unsigned int dbg_bus_c1_pattern:32;	/* Condition 1 match pattern of
									debug bus dedicate trigger */
			/* It is useless when dbg_bus_c1_change = 1 */
			/* default: 32'h0 */
		} s;
	} reg_dbg_bus_c1_pattern;

	union			/* 0x0f80, */
	{
		unsigned int dwValue;	/* Debug Bus value dword 0 */
		struct {
			unsigned int dbg_bus_value_dw0:32;	/* The bit[31:0] value of debug bus.
									AKA value of debug out to pad */
			/* default: 32'h0 */
		} s;
	} reg_dbg_bus_value_dw0;

	union			/* 0x0f84, */
	{
		unsigned int dwValue;	/* Debug Bus value dword 1 */
		struct {
			unsigned int dbg_bus_value_dw1:32;	/* The bit[63:32] value of debug bus.
									AKA value of debug trigger */
			/* default: 32'h0 */
		} s;
	} reg_dbg_bus_value_dw1;

	union			/* 0x0f88, */
	{
		unsigned int dwValue;	/* Debug Bus bit Occurred */
		struct {
			unsigned int dbg_bus_occurred:32;	/* The bit[31:0] of debug
									bus has ever gone to high */
			/* default: 32'h0 */
		} s;
	} reg_dbg_bus_occurred;

	union			/* 0x0f8c, */
	{
		unsigned int dwValue;	/* Internal Dump Mode */
		struct {
			unsigned int dbg_dump_flush_mode:2;	/* Internal Dump Fluh Mode */
			/* 0: No flush */
			/* 1:flush at the end of frame whose freg_debug_flush is set */
			/* 2: flush at the end of every target frame */
			/* 3: reserved */
			/* default: 2'h0 */
			unsigned int dbg_dump_flush_context_switch:1;	/* Internal Dump flush
									when context switch happened */
			/* 0: Normal mode */
			/* 1: see context switch as frame end */
			/* default: 1'b0 */
			unsigned int dbg_dump_flush_backoff:1;	/* Internal Dump flush when backoff happened */
			/* 0: Normal mode */
			/* 1: see backoff as frame end */
			/* default: 1'b0 */
			unsigned int dbg_dump_src_sel:2;	/* Input source for Internal Dump */
			/* 0: debug bus */
			/* 1: Reserved Customized Information */
			/* 2: Internal probe */
			/* 3: Reserved */
			/* default: 2'h0 */
			unsigned int reserved33:2;
			unsigned int dbg_dump_probe_sel:8;	/* internal probe select.
								Only valid when dbg_dump_src_sel == 1. */
			/* bit[7:4] means module select */
			/* bit[3:0] means the bus select in the module */
			/* For bit [7:4] */
			/* 0: VL */
			/* 1: CF */
			/* 2: SF */
			/* 3: BW */
			/* 4: UX */
			/* 5: TX */
			/* 6: VX */
			/* 7: CB */
			/* 8: SB */
			/* 9: XY */
			/* 10: ZP */
			/* default: 8'h0 */
			unsigned int dbg_dump_dummy_0:1;	/* Dump dummy register 0 for eco */
			/* default: 1'b0 */
			unsigned int dbg_dump_dummy_1:1;	/* Dump dummy register 1 for eco */
			/* default: 1'b0 */
			unsigned int dbg_dump_dummy_2:1;	/* Dump dummy register 2 for eco */
			/* default: 1'b0 */
			unsigned int dbg_dump_dummy_3:1;	/* Dump dummy register 3 for eco */
			/* default: 1'b0 */
			unsigned int dbg_dump_override:2;	/* Internal Dump width override */
			/* 0: Override width = 32bits */
			/* 1: Override width = 64bits */
			/* 2: Override width = 128bits */
			/* 3: No override. */
			/* default: 2'h3 */
			unsigned int reserved34:2;
			unsigned int dbg_dump_dw0_sel:2;	/* Internal Dump Double Word 0 Data
									select if width overide */
			/* 0: dump data[31:0] = Input Data[31:0] */
			/* 1: dump data[31:0] = input Data[63:32] */
			/* 2: dump data[31:0] = input Data[95:64] */
			/* 3: dump data[31:0] = input Data[127:96] */
			/* default: 2'h0 */
			unsigned int dbg_dump_dw1_sel:2;	/* Internal Dump Double Word 1 Data
									select if width overide */
			/* 0: dump data[63:32] = Input Data[31:0] */
			/* 1: dump data[63:32] = input Data[63:32] */
			/* 2: dump data[63:32] = input Data[95:64] */
			/* 3: dump data[63:32] = input Data[127:96] */
			/* default: 2'h1 */
			unsigned int dbg_dump_dw2_sel:2;	/* Internal Dump Double Word 2 Data
									select if width overide */
			/* 0: dump data[95:64] = Input Data[31:0] */
			/* 1: dump data[95:64] = input Data[63:32] */
			/* 2: dump data[95:64] = input Data[95:64] */
			/* 3: dump data[95:64] = input Data[127:96] */
			/* default: 2'h2 */
			unsigned int dbg_dump_dw3_sel:2;	/* Internal Dump Double Word 3 Data
									select if width overide */
			/* 0: dump data[127:96] = Input Data[31:0] */
			/* 1: dump data[127:96] = input Data[63:32] */
			/* 2: dump data[127:96] = input Data[95:64] */
			/* 3: dump data[127:96] = input Data[127:96] */
			/* default: 2'h3 */
		} s;
	} reg_dbg_dump_mode;

	union			/* 0x0f90, */
	{
		unsigned int dwValue;	/* Internal dump begin count */
		struct {
			unsigned int dbg_dump_begin_count:32;	/* Internal dump begin dump at
									the number of trigger hit count */
			/* 0: means begin capture when frame begins */
			/* default: 32'h0 */
		} s;
	} reg_dbg_dump_begin_count;

	union			/* 0x0f94, */
	{
		unsigned int dwValue;	/* Internal dump end count */
		struct {
			unsigned int dbg_dump_end_count:32;	/* Internal dump begin dump at
									the number of trigger hit count */
			/* 0: means no stop capturing */
			/* default: 32'h0 */
		} s;
	} reg_dbg_dump_end_count;

	union			/* 0x0f98, */
	{
		unsigned int dwValue;	/* Internal dump end count */
		struct {
			unsigned int dbg_dump_capture_count:32;	/* Internal dump begin dump at
									the number of trigger hit count */
			/* 0: means capture every cycle after triggered */
			/* default: 32'h0 */
		} s;
	} reg_dbg_dump_capture_count;

	union			/* 0x0f9c, */
	{
		unsigned int dwValue;	/* Internal Dump Status */
		struct {
			unsigned int dbg_dump_status_last_addr:14;	/* Last write address to SRAM */
			/* default: 14'h0 */
			unsigned int dbg_dump_status_data_width:2;	/* Internal Dump captured data width */
			/* 0: 32bit width */
			/* 1: 64bits width */
			/* 2: 128bit width */
			/* 3: reserved */
			/* default: 2'h0 */
			unsigned int dbg_dump_status_flush_count:8;	/* Internal Dump flush count */
			/* default: 8'h0 */
			unsigned int dbg_dump_status_flush_by_cs:1;	/* Indicate that last flush is caused
										by context switch */
			/* default: 1'b0 */
			unsigned int dbg_dump_status_flush_by_bo:1;	/* Indicate that last flush is caused
										by back off */
			/* default: 1'b0 */
			unsigned int dbg_dump_status_cs_occurred:1;	/* Indicate that context switch has ever
										happened before last flush */
			/* default: 1'b0 */
			unsigned int dbg_dump_status_bo_occurred:1;	/* Indicate that back off has ever
										happened before last flush */
			/* default: 1'b0 */
			unsigned int dbg_dump_state:2;	/* Current state of internal dump */
			/* default: 2'h0 */
			unsigned int dbg_dump_status_captured:1;	/* Captured indicator */
			/* It means something has captured in the dump */
			/* 0:  No captured */
			/* 1: Captured */
			/* default: 1'b0 */
			unsigned int dbg_dump_status_overflow:1;	/* Overflow indicator */
			/* 0:  No overflow */
			/* 1: Overflow occurred */
			/* default: 1'b0 */
		} s;
	} reg_dbg_dump_status;

	union			/* 0x0fa0, */
	{
		unsigned int dwValue;	/* Dump Data [31:0] Register */
		struct {
			unsigned int dbg_dump_data_dw0:32;	/* The dumpped data[31:0]
									when dbg_dmp_into_register_en == 1 */
			/* default: 32'h0 */
		} s;
	} reg_dbg_dump_data_dw0;

	union			/* 0x0fa4, */
	{
		unsigned int dwValue;	/* Dump Data [63:32] Register */
		struct {
			unsigned int dbg_dump_data_dw1:32;	/* The dumpped data[63:32]
									when dbg_dmp_into_register_en == 1 */
			/* default: 32'h0 */
		} s;
	} reg_dbg_dump_data_dw1;

	union			/* 0x0fa8, */
	{
		unsigned int dwValue;	/* Dump Data [95:64] Register */
		struct {
			unsigned int dbg_dump_data_dw2:32;	/* The dumpped data[95:64]
									when dbg_dmp_into_register_en == 1 */
			/* default: 32'h0 */
		} s;
	} reg_dbg_dump_data_dw2;

	union			/* 0x0fac, */
	{
		unsigned int dwValue;	/* Dump Data [127:96] Register */
		struct {
			unsigned int dbg_dump_data_dw3:32;	/* The dumpped data[127:96]
									when dbg_dmp_into_register_en == 1 */
			/* default: 32'h0 */
		} s;
	} reg_dbg_dump_data_dw3;

	union			/* 0x0fb0, */
	{
		unsigned int dwValue;	/* Internal Bus Signature Check Control */
		struct {
			unsigned int dbg_ibsc_enable:1;	/* Enable Internal Bus Signature Check */
			/* 0: Disable */
			/* 1: Enable */
			/* default: 1'b0 */
			unsigned int dbg_ibsc_by_tgt_frame:1;	/* If set, it will generate signature with
								the next target frame controlled by dbg_ctrl */
			/* otherwise, it will generate with tne next frame */
			/* 0: Next frame */
			/* 1: Next target frame */
			/* default: 1'b0 */
			unsigned int dbg_ibsc_xy2zp_check_fface:1;	/* If set, IBSC will generate
										signature with xy2zp_fface */
			/* default: 1'b0 */
			unsigned int dbg_ibsc_vl2ux_ignore_end:1;	/* If set, IBSC will generate
										signature without vl2ux_vs0_instend,
										vl2ux_vs0_listend, vl2ux_vs0_ndrend,
										vl2ux_vs0_wgend, vl2ux_vs1_instend
										and vl2ux_vs1_listend */
			/* default: 1'b0 */
			unsigned int dbg_isbc_dummy_0:1;	/* IBSC dummy register 0 for eco */
			/* default: 1'b0 */
			unsigned int dbg_isbc_dummy_1:1;	/* IBSC dummy register 1 for eco */
			/* default: 1'b0 */
			unsigned int dbg_isbc_dummy_2:1;	/* IBSC dummy register 2 for eco */
			/* default: 1'b0 */
			unsigned int dbg_isbc_dummy_3:1;	/* IBSC dummy register 3 for eco */
			/* default: 1'b0 */
			unsigned int dbg_ibsc_read_sel:8;	/* Select which internal bus to put its signature
									at dbg_ibsc_signature register */
			/* bit[15:12] means module select */
			/* bit[11:8] means the bus select in the module */
			/* For bit [15:12] */
			/* 0: VL */
			/* 1: CF */
			/* 2: SF */
			/* 3: BW */
			/* 4: UX */
			/* 5: TX */
			/* 6: VX */
			/* 7: CB */
			/* 8: SB */
			/* 9: XY */
			/* 10: GL */
			/* default: 8'h0 */
			unsigned int reserved35:8;
			unsigned int dbg_ibsc_vp_ready:1;	/* Indicate that VP phase bus signature is generating */
			/* default: 1'b0 */
			unsigned int dbg_ibsc_pp_ready:1;	/* Indicate that pP phase bus signature is generating */
			/* default: 1'b0 */
		} s;
	} reg_dbg_ibsc_ctrl;

	union			/* 0x0fb4, */
	{
		unsigned int dwValue;	/* Signature Value */
		struct {
			unsigned int dbg_ibsc_signature:32;	/* The signature value of selected bus */
			/* default: 32'h0 */
		} s;
	} reg_dbg_ibsc_signature;

	union			/* 0x0fb8, */
	{
		unsigned int dwValue;	/* Specia value for vl2vx */
		struct {
			unsigned int dbg_ibsc_vl2vx_addr_offset:32;	/* The very first vl2vx qualified
									value when ibsc_enable asserted */
			/* [31:0] = {vl2vx_v2_offset[3:0], vl2vx_v1_offset[3:0], vl2vx_v0_addr[23:0]} */
			/* default: 32'h0 */
		} s;
	} reg_dbg_ibsc_vl2vx;

} SapphireAreg;


#endif /* _SAPPHIRE_AREG_H_ */
