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

#ifndef _SAPPHIRE_RSM_H_
#define _SAPPHIRE_RSM_H_

/* Last updated on 2014/08/20, 14:32. The version : 459 */




/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_INFO                0x0000	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_LIST_ID             0x00000fff	/* [default:12'b0][perf:] [11:0] list ide */
						  /*  */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_LIST_ID             0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_FRAME_EMPTY         0x00001000	/* [default:1'b0][perf:] preempt and
								this frame is empty; (debug) */
						  /*  */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_FRAME_EMPTY         12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_VL_BCKWRD           0x00002000	/* [default:1'b0][perf:] vl bckwrd(debug) */
						  /*  */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_VL_BCKWRD           13
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_VL_EMPTY            0x00004000	/* [default:1'b0][perf:] vl empty(debug) */
						  /*  */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_VL_EMPTY            14
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_VX_EMPTY            0x00008000	/* [default:1'b0][perf:] vx empty(debug) */
						  /*  */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_VX_EMPTY            15
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_BOFF_ABORT          0x00010000	/* [default:1'b0][perf:] boff abort status */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_BOFF_ABORT          16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_DEFER_EN            0x00020000 /* [default:1'b0][perf:] preempt at defer enable mode(debug) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_DEFER_EN            17
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_OPENCL_EN           0x00040000	/* [default:1'b0][perf:] preempt at opencl enable mode */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_OPENCL_EN           18
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_VPENG_VLD           0x00080000	/* [default:1'b0][perf:] preempt after vp engine fire */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_VPENG_VLD           19
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_PPENG_VLD           0x00100000	/* [default:1'b0][perf:] preempt after pp engine fire */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_PPENG_VLD           20
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_VPENG_INIT          0x00200000	/* [default:1'b0][perf:] preempt after vp engine init */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_VPENG_INIT          21
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_PPENG_INIT          0x00400000	/* [default:1'b0][perf:] preempt after pp engine init */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_PPENG_INIT          22
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_VP_FRAME_FLUSH      0x00800000	/* [default:1'b0][perf:] defer: vp frame flush */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_VP_FRAME_FLUSH      23
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_BIN_FLUSH           0x01000000	/* [default:1'b0][perf:] preempt after vp bin flush */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_BIN_FLUSH           24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_ENG_STATUS          0xfe000000	/* [default:7'b0][perf:] vp pp engine status */
						  /*  */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_ENG_STATUS          25

/**********************************************************************************************************/
/*  */
#define REG_RSM_GL_VPREG_HEAD          0x0004	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_GL_VPREG_HEAD          0xffffffff /* [default:32'h0][perf:] Current local working group position X. */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_GL_VPREG_HEAD          0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_VX_BOFF_CNT         0x0008	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_BOFF_HD_OVF_CNT     0x000000ff	/* [default:8'h0][perf:] vx back off condition */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_BOFF_HD_OVF_CNT     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_BOFF_TB_OVF_CNT     0x0000ff00	/* [default:8'h0][perf:] vx back off condition */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_BOFF_TB_OVF_CNT     8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_BOFF_BK_OVF_CNT     0x00ff0000	/* [default:8'h0][perf:] vx back off condition */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_BOFF_BK_OVF_CNT     16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_BOFF_VA_OVF_CNT     0xff000000	/* [default:8'h0][perf:] vx back off condition */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_BOFF_VA_OVF_CNT     24

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_RSRV_0              0x000c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_RSRV_0              0xffffffff	/* [default:32'h0][perf:] parser reserved 0 */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_RSRV_0              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_RSRV_1              0x0010	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_RSRV_1              0xffffffff	/* [default:32'h0][perf:] parser reserved 1 */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_RSRV_1              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_ACCU_WG_X           0x0014	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_ACCU_WG_X           0xffffffff /* [default:32'h0][perf:] Current local working group position X. */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_ACCU_WG_X           0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_ACCU_WG_Y           0x0018	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_ACCU_WG_Y           0xffffffff /* [default:32'h0][perf:]Current local working group position Y */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_ACCU_WG_Y           0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_ACCU_WG_Z           0x001c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_ACCU_WG_Z           0xffffffff	/* [default:32'h0][perf:] Current local
								working group position Z */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_ACCU_WG_Z           0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_POINTER             0x0020	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_WR_PTR              0x00000003	/* [default:2'h0][perf:] VL write pointer in primgen */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_WR_PTR              0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_LOOP_PTR            0x0000001c	/* [default:3'h0][perf:] VL loop pointer in primgen */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_LOOP_PTR            2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVGEN_NOADD         0x00000020	/* [default:1'h0][perf:] Divisor gen no add */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVGEN_NOADD         5
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_GL_PMT_EMPTY        0x00000040	/* [default:1'h0][perf:] GL mode preempt empty */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_GL_PMT_EMPTY        6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_WG_PMT_EMPTY        0x00000080	/* [default:1'h0][perf:] CL model preempt empty */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_WG_PMT_EMPTY        7

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_ELEMENT_CNT         0x0024	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_ELEMENT_CNT         0xffffffff	/* [default:32'h0][perf:] VL element count */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_ELEMENT_CNT         0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_PRIM_DIFF_CNT       0x0028	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_PRIM_DIFF_CNT       0xffffffff	/* [default:32'h0][perf:] VL and VX primitive
								difference count, due to backoff */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_PRIM_DIFF_CNT       0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_INST_CNT            0x002c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_INST_CNT            0xffffffff	/* [default:32'h0][perf:] VL instance count */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_INST_CNT            0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_RESTART_BASE        0x0030	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_RESTART_BASE        0xffffffff	/* [default:32'h0][perf:] VL restart base for triangle-fan */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_RESTART_BASE        0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT0              0x0034	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT0              0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT0              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT1              0x0038	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT1              0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT1              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT2              0x003c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT2              0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT2              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT3              0x0040	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT3              0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT3              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT4              0x0044	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT4              0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT4              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT5              0x0048	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT5              0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT5              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT6              0x004c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT6              0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT6              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT7              0x0050	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT7              0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT7              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT8              0x0054	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT8              0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT8              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT9              0x0058	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT9              0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT9              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT10             0x005c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT10             0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT10             0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT11             0x0060	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT11             0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT11             0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT12             0x0064	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT12             0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT12             0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT13             0x0068	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT13             0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT13             0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT14             0x006c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT14             0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT14             0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_DVCNT15             0x0070	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_DVCNT15             0xffffffff	/* [default:32'h0][perf:]
								Divisor count = floor(instance_id/divisor) */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_DVCNT15             0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_LV1_DVCNTB0         0x0074	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_LV1_DVCNTB0         0x0000ffff	/* [default:16'h0][perf:] Leve1 counter bit 0 */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_LV1_DVCNTB0         0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_TFB_POINTER0        0x0078	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_TFB_POINTER0        0x3fffffff	/* [default:30'h0][perf:] Current TFB write dram pointer0,
								4 byte alignment. */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_TFB_POINTER0        0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_TFB_POINTER1        0x007c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_TFB_POINTER1        0x3fffffff	/* [default:30'h0][perf:] Current TFB write dram pointer1,
								4 byte alignment. */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_TFB_POINTER1        0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_TFB_POINTER2        0x0080	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_TFB_POINTER2        0x3fffffff	/* [default:30'h0][perf:] Current TFB write dram pointer2,
								4 byte alignment. */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_TFB_POINTER2        0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_TFB_POINTER3        0x0084	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_TFB_POINTER3        0x3fffffff	/* [default:30'h0][perf:] Current TFB write dram pointer3,
								4 byte alignment. */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_TFB_POINTER3        0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_WG_XID              0x0088	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_WG_XID              0xffffffff	/* [default:32'h0][perf:] workgroup xid */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_WG_XID              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_WG_YID              0x008c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_WG_YID              0xffffffff	/* [default:32'h0][perf:] workgroup yid */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_WG_YID              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_WG_ZID              0x0090	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_WG_ZID              0xffffffff	/* [default:32'h0][perf:] workgroup zid */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_WG_ZID              0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VL_CTX_RESERVED        0x0094	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VL_CTX_RESERVED        0xffffffff	/* [default:32'h0][perf:] reserved for ctx switch */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VL_CTX_RESERVED        0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_DB_INFO0            0x0098	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_DB_HD_CNT           0x000fffff	/* [default:20'h0][perf:] drawbin header count */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_DB_HD_CNT           0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_DB_INFO1            0x009c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_DB_HDX_CNT          0x000003ff	/* [default:10'h0][perf:] drawbin header horizental count */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_DB_HDX_CNT          0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_DB_HDY_CNT          0x000ffc00	/* [default:10'h0][perf:] drawbin header vertical count */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_DB_HDY_CNT          10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_DB_BIN_CNT          0x00300000	/* [default:2'b0][perf:] drawbin bin count of a macro bin */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_DB_BIN_CNT          20

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_DB_INFO2            0x00a0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_DB_PRIM_CNT         0x0000ffff	/* [default:16'b0][perf:] drawbin primitive count of a bin */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_DB_PRIM_CNT         0

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_BS_INFO0            0x00a4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BS_NEW_TB_PTR       0x00ffffff	/* [default:24'h0][perf:] bin store new table pointer */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BS_NEW_TB_PTR       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BS_BASE_SEL         0x01000000	/* [default:1'b0][perf:] bin table baseaddress select */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BS_BASE_SEL         24

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_BS_INFO1            0x00a8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BS_NEW_BK_PTR       0x00ffffff	/* [default:24'h0][perf:] bin store new block pointer */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BS_NEW_BK_PTR       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BS_HD_MAX_L_NO      0xff000000	/* [default:8'h0][perf:] bin store table header
								max number L part */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BS_HD_MAX_L_NO      24

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_BS_INFO2            0x00ac	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BS_TB_BND_NO        0x000fffff	/* [default:20'h0][perf:] bin store table boundary number */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BS_TB_BND_NO        0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BS_HD_MAX_H_NO      0xff000000	/* [default:8'h0][perf:] bin store table header
								max number H part */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BS_HD_MAX_H_NO      24

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_BS_INFO3            0x00b0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BS_BK_BND_NO        0x000fffff	/* [default:20'h0][perf:] bin store block boundary number */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BS_BK_BND_NO        0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_DUMMY0              0xfff00000	/* [default:12'h0][perf:] vx dummy0 */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_DUMMY0              20

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_BS_INFO4            0x00b4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BS_TB_CBND_NO       0x000fffff	/* [default:20'h0][perf:] bin store table boundary number
								for clipping part */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BS_TB_CBND_NO       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_DUMMY1              0xfff00000	/* [default:12'h0][perf:] vx dummy1 */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_DUMMY1              20

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_BS_INFO5            0x00b8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BS_BK_CBND_NO       0x000fffff	/* [default:20'h0][perf:] bin store block boundary number
								for clipping part */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BS_BK_CBND_NO       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_DUMMY2              0xfff00000	/* [default:12'h0][perf:] vx dummy2 */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_DUMMY2              20

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_VW_INFO0            0x00bc	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_VW_VADR             0x00ffffff	/* [default:24'b0][perf:] vertex write vertex address */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_VW_VADR             0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_DUMMY3              0xff000000	/* [default:8'h0][perf:] vx dummy3 */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_DUMMY3              24

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_DI_INFO0            0x00c0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_DI_BPTR             0x00ffffff	/* [default:24'b0][perf:] data in vertex base pointer */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_DI_BPTR             0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BOFF_HD_OVF         0x01000000	/* [default:1'b0][perf:] header mbin number overflow */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BOFF_HD_OVF         24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BOFF_TB_OVF         0x02000000	/* [default:1'b0][perf:] table entry overflow */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BOFF_TB_OVF         25
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BOFF_BK_OVF         0x04000000	/* [default:1'b0][perf:] block entry overflow */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BOFF_BK_OVF         26
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BOFF_VA_OVF         0x08000000	/* [default:1'b0][perf:] varying entry overflow */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BOFF_VA_OVF         27
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BOFF_HD_PREOVF      0x10000000	/* [default:1'b0][perf:] header mbin number pre overflow */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BOFF_HD_PREOVF      28
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BOFF_TB_PREOVF      0x20000000	/* [default:1'b0][perf:] table entry pre overflow */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BOFF_TB_PREOVF      29
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BOFF_BK_PREOVF      0x40000000	/* [default:1'b0][perf:] block entry pre overflow */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BOFF_BK_PREOVF      30
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BOFF_VA_PREOVF      0x80000000	/* [default:1'b0][perf:] varying entry pre overflow */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BOFF_VA_PREOVF      31

/**********************************************************************************************************/
/*  */
#define REG_RSM_VX_INFO0               0x00c4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_MBINX               0x000003ff	/* [default:10'h0][perf:] mbin horizental count */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_MBINX               0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_MBINY               0x000ffc00	/* [default:10'h0][perf:] mbin vertical count */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_MBINY               10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_SUB                 0x00300000	/* [default:2'b0][perf:] bin count of a macro bin */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_SUB                 20
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_VX_BIN_UNFINISH        0x00400000	/* [default:1'b0][perf:] bin unfinished */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_VX_BIN_UNFINISH        22

/**********************************************************************************************************/
/*  */
#define REG_RSM_ZP_QRY_INFO0           0x00c8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_ZP_OCC_QRY_STATE0      0xffffffff	/* [default:32'h0][perf:] zpost occlusion query state [31:0] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_ZP_OCC_QRY_STATE0      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_ZP_QRY_INFO1           0x00cc	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_ZP_OCC_QRY_STATE1      0xffffffff /* [default:32'h0][perf:] zpost occlusion query state [63:32] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_ZP_OCC_QRY_STATE1      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_ZP_QRY_INFO2           0x00d0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_ZP_OCC_QRY_STATE2      0xffffffff /* [default:32'h0][perf:] zpost occlusion query state [95:64] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_ZP_OCC_QRY_STATE2      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_ZP_QRY_INFO3           0x00d4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_ZP_OCC_QRY_STATE3      0xffffffff /* [default:32'h0][perf:] zpost occlusion query state [127:96] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_ZP_OCC_QRY_STATE3      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT_STATUS    0x00d8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT0_BUSY     0x00000001 /* [default:1'b0][perf:] pa cl run counter 0 busy when preempt */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT0_BUSY     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT1_BUSY     0x00000002 /* [default:1'b0][perf:] pa cl run counter 1 busy when preempt */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT1_BUSY     1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT2_BUSY     0x00000004 /* [default:1'b0][perf:] pa cl run counter 2 busy when preempt */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT2_BUSY     2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT3_BUSY     0x00000008 /* [default:1'b0][perf:] pa cl run counter 3 busy when preempt */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT3_BUSY     3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT4_BUSY     0x00000010 /* [default:1'b0][perf:] pa cl run counter 4 busy when preempt */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT4_BUSY     4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT5_BUSY     0x00000020 /* [default:1'b0][perf:] pa cl run counter 5 busy when preempt */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT5_BUSY     5
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT6_BUSY     0x00000040 /* [default:1'b0][perf:] pa cl run counter 6 busy when preempt */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT6_BUSY     6

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT0_L32      0x00dc	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT0_L32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 0 [31:0] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT0_L32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT0_H32      0x00e0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT0_H32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 0 [63:32] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT0_H32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT1_L32      0x00e4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT1_L32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 1 [31:0] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT1_L32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT1_H32      0x00e8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT1_H32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 1 [63:32] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT1_H32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT2_L32      0x00ec	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT2_L32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 2 [31:0] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT2_L32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT2_H32      0x00f0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT2_H32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 2 [63:32] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT2_H32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT3_L32      0x00f4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT3_L32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 3 [31:0] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT3_L32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT3_H32      0x00f8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT3_H32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 3 [63:32] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT3_H32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT4_L32      0x00fc	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT4_L32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 4 [31:0] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT4_L32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT4_H32      0x0100	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT4_H32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 4 [63:32] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT4_H32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT5_L32      0x0104	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT5_L32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 5 [31:0] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT5_L32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT5_H32      0x0108	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT5_H32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 5 [63:32] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT5_H32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT6_L32      0x010c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT6_L32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 6 [31:0] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT6_L32      0

/**********************************************************************************************************/
/*  */
#define REG_RSM_PA_CL_RUNCNT6_H32      0x0110	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_RSM_PA_CL_RUNCNT6_H32      0xffffffff	/* [default:32'h0][perf:] pa cl run counter 6 [63:32] */
						  /* SAVE/RESTORE INFO */
#define SFT_RSM_PA_CL_RUNCNT6_H32      0

/**********************************************************************************************************/


typedef struct {
	union			/* 0x0000, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_list_id:12;	/* [11:0] list ide */
			/*  */
			/* SAVE/RESTORE INFO */
			/* default: 12'b0 */
			unsigned int pa_frame_empty:1;	/* preempt and this frame is empty; (debug) */
			/*  */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_vl_bckwrd:1;	/* vl bckwrd(debug) */
			/*  */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_vl_empty:1;	/* vl empty(debug) */
			/*  */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_vx_empty:1;	/* vx empty(debug) */
			/*  */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_boff_abort:1;	/* boff abort status */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_defer_en:1;	/* preempt at defer enable mode(debug) */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_opencl_en:1;	/* preempt at opencl enable mode */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_vpeng_vld:1;	/* preempt after vp engine fire */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_ppeng_vld:1;	/* preempt after pp engine fire */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_vpeng_init:1;	/* preempt after vp engine init */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_ppeng_init:1;	/* preempt after pp engine init */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_vp_frame_flush:1;	/* defer: vp frame flush */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_bin_flush:1;	/* preempt after vp bin flush */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_eng_status:7;	/* vp pp engine status */
			/*  */
			/* SAVE/RESTORE INFO */
			/* default: 7'b0 */
		} s;
	} reg_pa_info;

	union			/* 0x0004, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int gl_vpreg_head:32;	/* Current local working group position X. */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_gl_vpreg_head;

	union			/* 0x0008, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_boff_hd_ovf_cnt:8;	/* vx back off condition */
			/* SAVE/RESTORE INFO */
			/* default: 8'h0 */
			unsigned int pa_boff_tb_ovf_cnt:8;	/* vx back off condition */
			/* SAVE/RESTORE INFO */
			/* default: 8'h0 */
			unsigned int pa_boff_bk_ovf_cnt:8;	/* vx back off condition */
			/* SAVE/RESTORE INFO */
			/* default: 8'h0 */
			unsigned int pa_boff_va_ovf_cnt:8;	/* vx back off condition */
			/* SAVE/RESTORE INFO */
			/* default: 8'h0 */
		} s;
	} reg_pa_vx_boff_cnt;

	union			/* 0x000c, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_rsrv_0:32;	/* parser reserved 0 */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_rsrv_0;

	union			/* 0x0010, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_rsrv_1:32;	/* parser reserved 1 */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_rsrv_1;

	union			/* 0x0014, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_accu_wg_x:32;	/* Current local working group position X. */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_accu_wg_x;

	union			/* 0x0018, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_accu_wg_y:32;	/* Current local working group position Y */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_accu_wg_y;

	union			/* 0x001c, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_accu_wg_z:32;	/* Current local working group position Z */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_accu_wg_z;

	union			/* 0x0020, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_wr_ptr:2;	/* VL write pointer in primgen */
			/* SAVE/RESTORE INFO */
			/* default: 2'h0 */
			unsigned int vl_loop_ptr:3;	/* VL loop pointer in primgen */
			/* SAVE/RESTORE INFO */
			/* default: 3'h0 */
			unsigned int vl_dvgen_noadd:1;	/* Divisor gen no add */
			/* SAVE/RESTORE INFO */
			/* default: 1'h0 */
			unsigned int vl_gl_pmt_empty:1;	/* GL mode preempt empty */
			/* SAVE/RESTORE INFO */
			/* default: 1'h0 */
			unsigned int vl_wg_pmt_empty:1;	/* CL model preempt empty */
			/* SAVE/RESTORE INFO */
			/* default: 1'h0 */
		} s;
	} reg_vl_pointer;

	union			/* 0x0024, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_element_cnt:32;	/* VL element count */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_element_cnt;

	union			/* 0x0028, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_prim_diff_cnt:32;	/* VL and VX primitive difference count,
									due to backoff */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_prim_diff_cnt;

	union			/* 0x002c, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_inst_cnt:32;	/* VL instance count */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_inst_cnt;

	union			/* 0x0030, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_restart_base:32;	/* VL restart base for triangle-fan */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_restart_base;

	union			/* 0x0034, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt0:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt0;

	union			/* 0x0038, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt1:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt1;

	union			/* 0x003c, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt2:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt2;

	union			/* 0x0040, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt3:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt3;

	union			/* 0x0044, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt4:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt4;

	union			/* 0x0048, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt5:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt5;

	union			/* 0x004c, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt6:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt6;

	union			/* 0x0050, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt7:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt7;

	union			/* 0x0054, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt8:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt8;

	union			/* 0x0058, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt9:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt9;

	union			/* 0x005c, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt10:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt10;

	union			/* 0x0060, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt11:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt11;

	union			/* 0x0064, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt12:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt12;

	union			/* 0x0068, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt13:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt13;

	union			/* 0x006c, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt14:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt14;

	union			/* 0x0070, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_dvcnt15:32;	/* Divisor count = floor(instance_id/divisor) */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_dvcnt15;

	union			/* 0x0074, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_lv1_dvcntb0:16;	/* Leve1 counter bit 0 */
			/* SAVE/RESTORE INFO */
			/* default: 16'h0 */
		} s;
	} reg_vl_lv1_dvcntb0;

	union			/* 0x0078, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_tfb_pointer0:30;	/* Current TFB write dram pointer0,
									4 byte alignment. */
			/* SAVE/RESTORE INFO */
			/* default: 30'h0 */
		} s;
	} reg_vl_tfb_pointer0;

	union			/* 0x007c, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_tfb_pointer1:30;	/* Current TFB write dram pointer1,
									4 byte alignment. */
			/* SAVE/RESTORE INFO */
			/* default: 30'h0 */
		} s;
	} reg_vl_tfb_pointer1;

	union			/* 0x0080, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_tfb_pointer2:30;	/* Current TFB write dram pointer2,
									4 byte alignment. */
			/* SAVE/RESTORE INFO */
			/* default: 30'h0 */
		} s;
	} reg_vl_tfb_pointer2;

	union			/* 0x0084, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_tfb_pointer3:30;	/* Current TFB write dram pointer3,
									4 byte alignment. */
			/* SAVE/RESTORE INFO */
			/* default: 30'h0 */
		} s;
	} reg_vl_tfb_pointer3;

	union			/* 0x0088, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_wg_xid:32;	/* workgroup xid */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_wg_xid;

	union			/* 0x008c, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_wg_yid:32;	/* workgroup yid */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_wg_yid;

	union			/* 0x0090, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_wg_zid:32;	/* workgroup zid */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_wg_zid;

	union			/* 0x0094, VL */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vl_ctx_reserved:32;	/* reserved for ctx switch */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_vl_ctx_reserved;

	union			/* 0x0098, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_db_hd_cnt:20;	/* drawbin header count */
			/* SAVE/RESTORE INFO */
			/* default: 20'h0 */
		} s;
	} reg_vx_db_info0;

	union			/* 0x009c, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_db_hdx_cnt:10;	/* drawbin header horizental count */
			/* SAVE/RESTORE INFO */
			/* default: 10'h0 */
			unsigned int vx_db_hdy_cnt:10;	/* drawbin header vertical count */
			/* SAVE/RESTORE INFO */
			/* default: 10'h0 */
			unsigned int vx_db_bin_cnt:2;	/* drawbin bin count of a macro bin */
			/* SAVE/RESTORE INFO */
			/* default: 2'b0 */
		} s;
	} reg_vx_db_info1;

	union			/* 0x00a0, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_db_prim_cnt:16;	/* drawbin primitive count of a bin */
			/* SAVE/RESTORE INFO */
			/* default: 16'b0 */
		} s;
	} reg_vx_db_info2;

	union			/* 0x00a4, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_bs_new_tb_ptr:24;	/* bin store new table pointer */
			/* SAVE/RESTORE INFO */
			/* default: 24'h0 */
			unsigned int vx_bs_base_sel:1;	/* bin table baseaddress select */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
		} s;
	} reg_vx_bs_info0;

	union			/* 0x00a8, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_bs_new_bk_ptr:24;	/* bin store new block pointer */
			/* SAVE/RESTORE INFO */
			/* default: 24'h0 */
			unsigned int vx_bs_hd_max_l_no:8;	/* bin store table header max number L part */
			/* SAVE/RESTORE INFO */
			/* default: 8'h0 */
		} s;
	} reg_vx_bs_info1;

	union			/* 0x00ac, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_bs_tb_bnd_no:20;	/* bin store table boundary number */
			/* SAVE/RESTORE INFO */
			/* default: 20'h0 */
			unsigned int reserved0:4;
			unsigned int vx_bs_hd_max_h_no:8;	/* bin store table header max number H part */
			/* SAVE/RESTORE INFO */
			/* default: 8'h0 */
		} s;
	} reg_vx_bs_info2;

	union			/* 0x00b0, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_bs_bk_bnd_no:20;	/* bin store block boundary number */
			/* SAVE/RESTORE INFO */
			/* default: 20'h0 */
			unsigned int vx_dummy0:12;	/* vx dummy0 */
			/* SAVE/RESTORE INFO */
			/* default: 12'h0 */
		} s;
	} reg_vx_bs_info3;

	union			/* 0x00b4, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_bs_tb_cbnd_no:20;	/* bin store table boundary number for
									clipping part */
			/* SAVE/RESTORE INFO */
			/* default: 20'h0 */
			unsigned int vx_dummy1:12;	/* vx dummy1 */
			/* SAVE/RESTORE INFO */
			/* default: 12'h0 */
		} s;
	} reg_vx_bs_info4;

	union			/* 0x00b8, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_bs_bk_cbnd_no:20;	/* bin store block boundary number for
									clipping part */
			/* SAVE/RESTORE INFO */
			/* default: 20'h0 */
			unsigned int vx_dummy2:12;	/* vx dummy2 */
			/* SAVE/RESTORE INFO */
			/* default: 12'h0 */
		} s;
	} reg_vx_bs_info5;

	union			/* 0x00bc, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_vw_vadr:24;	/* vertex write vertex address */
			/* SAVE/RESTORE INFO */
			/* default: 24'b0 */
			unsigned int vx_dummy3:8;	/* vx dummy3 */
			/* SAVE/RESTORE INFO */
			/* default: 8'h0 */
		} s;
	} reg_vx_vw_info0;

	union			/* 0x00c0, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_di_bptr:24;	/* data in vertex base pointer */
			/* SAVE/RESTORE INFO */
			/* default: 24'b0 */
			unsigned int vx_boff_hd_ovf:1;	/* header mbin number overflow */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int vx_boff_tb_ovf:1;	/* table entry overflow */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int vx_boff_bk_ovf:1;	/* block entry overflow */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int vx_boff_va_ovf:1;	/* varying entry overflow */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int vx_boff_hd_preovf:1;	/* header mbin number pre overflow */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int vx_boff_tb_preovf:1;	/* table entry pre overflow */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int vx_boff_bk_preovf:1;	/* block entry pre overflow */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int vx_boff_va_preovf:1;	/* varying entry pre overflow */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
		} s;
	} reg_vx_di_info0;

	union			/* 0x00c4, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_mbinx:10;	/* mbin horizental count */
			/* SAVE/RESTORE INFO */
			/* default: 10'h0 */
			unsigned int vx_mbiny:10;	/* mbin vertical count */
			/* SAVE/RESTORE INFO */
			/* default: 10'h0 */
			unsigned int vx_sub:2;	/* bin count of a macro bin */
			/* SAVE/RESTORE INFO */
			/* default: 2'b0 */
			unsigned int vx_bin_unfinish:1;	/* bin unfinished */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
		} s;
	} reg_vx_info0;

	union			/* 0x00c8, ZP */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int zp_occ_qry_state0:32;	/* zpost occlusion query state [31:0] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_zp_qry_info0;

	union			/* 0x00cc, ZP */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int zp_occ_qry_state1:32;	/* zpost occlusion query state [63:32] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_zp_qry_info1;

	union			/* 0x00d0, ZP */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int zp_occ_qry_state2:32;	/* zpost occlusion query state [95:64] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_zp_qry_info2;

	union			/* 0x00d4, ZP */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int zp_occ_qry_state3:32;	/* zpost occlusion query state [127:96] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_zp_qry_info3;

	union			/* 0x00d8, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt0_busy:1;	/* pa cl run counter 0 busy when preempt */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_cl_runcnt1_busy:1;	/* pa cl run counter 1 busy when preempt */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_cl_runcnt2_busy:1;	/* pa cl run counter 2 busy when preempt */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_cl_runcnt3_busy:1;	/* pa cl run counter 3 busy when preempt */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_cl_runcnt4_busy:1;	/* pa cl run counter 4 busy when preempt */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_cl_runcnt5_busy:1;	/* pa cl run counter 5 busy when preempt */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pa_cl_runcnt6_busy:1;	/* pa cl run counter 6 busy when preempt */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
		} s;
	} reg_pa_cl_runcnt_status;

	union			/* 0x00dc, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt0_l32:32;	/* pa cl run counter 0 [31:0] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt0_l32;

	union			/* 0x00e0, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt0_h32:32;	/* pa cl run counter 0 [63:32] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt0_h32;

	union			/* 0x00e4, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt1_l32:32;	/* pa cl run counter 1 [31:0] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt1_l32;

	union			/* 0x00e8, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt1_h32:32;	/* pa cl run counter 1 [63:32] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt1_h32;

	union			/* 0x00ec, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt2_l32:32;	/* pa cl run counter 2 [31:0] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt2_l32;

	union			/* 0x00f0, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt2_h32:32;	/* pa cl run counter 2 [63:32] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt2_h32;

	union			/* 0x00f4, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt3_l32:32;	/* pa cl run counter 3 [31:0] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt3_l32;

	union			/* 0x00f8, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt3_h32:32;	/* pa cl run counter 3 [63:32] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt3_h32;

	union			/* 0x00fc, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt4_l32:32;	/* pa cl run counter 4 [31:0] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt4_l32;

	union			/* 0x0100, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt4_h32:32;	/* pa cl run counter 4 [63:32] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt4_h32;

	union			/* 0x0104, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt5_l32:32;	/* pa cl run counter 5 [31:0] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt5_l32;

	union			/* 0x0108, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt5_h32:32;	/* pa cl run counter 5 [63:32] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt5_h32;

	union			/* 0x010c, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt6_l32:32;	/* pa cl run counter 6 [31:0] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt6_l32;

	union			/* 0x0110, PA */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int pa_cl_runcnt6_h32:32;	/* pa cl run counter 6 [63:32] */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_pa_cl_runcnt6_h32;

} SapphireRsm;


#endif /* _SAPPHIRE_RSM_H_ */
