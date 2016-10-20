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

#ifndef _SAPPHIRE_UX_CMN_H_
#define _SAPPHIRE_UX_CMN_H_

/* Last updated on 2014/08/20, 14:32. The version : 459 */




/**********************************************************************************************************/
/* Debug Mode Enable Settings */
#define REG_UX_CMN_DEBUG_ENABLE        0x0000	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DET_ADDR_EN     0x00000001	/* [default:1'b0][perf:] isc detect address enable */
#define SFT_UX_CMN_ISC_DET_ADDR_EN     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_COR_ADDR_EN     0x00000002	/* [default:1'b0][perf:] isc correct address enable */
#define SFT_UX_CMN_ISC_COR_ADDR_EN     1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CPL1_DET_ADDR_EN    0x00000004	/* [default:1'b0][perf:] cpl1 detect address enable */
#define SFT_UX_CMN_CPL1_DET_ADDR_EN    2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CPL1_COR_ADDR_EN    0x00000008	/* [default:1'b0][perf:] cpl1 correct address enable */
#define SFT_UX_CMN_CPL1_COR_ADDR_EN    3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_LSU_DET_ADDR_EN     0x00000010	/* [default:1'b0][perf:] lsu detect address enable */
#define SFT_UX_CMN_LSU_DET_ADDR_EN     4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_LSU_COR_ADDR_EN     0x00000020	/* [default:1'b0][perf:] lsu correct address enable */
#define SFT_UX_CMN_LSU_COR_ADDR_EN     5
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_UX_DET_ACTIVITY_EN  0x00000040	/* [default:1'b0][perf:] ux detect activity enable */
#define SFT_UX_CMN_UX_DET_ACTIVITY_EN  6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_UX_DET_RETIRE_EN    0x00000080	/* [default:1'b0][perf:] ux detect retire enable */
#define SFT_UX_CMN_UX_DET_RETIRE_EN    7
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CPRF_DET_ADDR_EN    0x00000100 /* [default:1'b0][perf:] cprf detect wrong address access enable */
#define SFT_UX_CMN_CPRF_DET_ADDR_EN    8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CPRF_COR_ADDR_EN    0x00000200 /* [default:1'b0][perf:] cprf correct wrong address write enable */
#define SFT_UX_CMN_CPRF_COR_ADDR_EN    9
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_IN_STAGE_SID_EN     0x00010000	/* [default:1'b0][perf:] input stage sequence id
								enable (enable sid count) */
#define SFT_UX_CMN_IN_STAGE_SID_EN     16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_IN_STAGE_SID_RSV0   0x00020000	/* [default:1'b0][perf:] reserved */
#define SFT_UX_CMN_IN_STAGE_SID_RSV0   17
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_IN_STAGE_SID_RST_SEL 0x00040000	/* [default:1'b0][perf:] input stage sequence id reset select
								([0]: pp_frame_flush [1]: vp_frame_rst) */
#define SFT_UX_CMN_IN_STAGE_SID_RST_SEL 18
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_IFBSTOP_FORCE_EN    0x01000000	/* [default:1'b0][perf:] IFB Engine Stop Force
								(All IFB Engine stop issuing instructions) */
#define SFT_UX_CMN_IFBSTOP_FORCE_EN    24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_IFBSTOP_VPUERR_EN   0x02000000	/* [default:1'b0][perf:] IFB Engine Stop on VPU Error */
#define SFT_UX_CMN_IFBSTOP_VPUERR_EN   25
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_IFBSTOP_MEMERR_EN   0x04000000 /* [default:1'b0][perf:] IFB Engine Stop on Memory Check Error */
#define SFT_UX_CMN_IFBSTOP_MEMERR_EN   26
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_IFBSTOP_ACTERR_EN   0x08000000 /* [default:1'b0][perf:] IFB Engine Stop on Activity Check Error */
#define SFT_UX_CMN_IFBSTOP_ACTERR_EN   27
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_IFBSTOP_RELEASE     0x40000000	/* [default:1'b0][perf:] IFB Engine Release
								(All Stopped Engine are released) */
#define SFT_UX_CMN_IFBSTOP_RELEASE     30
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_DEBUG_EN            0x80000000	/* [default:1'b0][perf:] Debug Enable for data latch
								(read/dump eu_status_data should active this bit) */
#define SFT_UX_CMN_DEBUG_EN            31

/**********************************************************************************************************/
/* Report of interrupt status */
#define REG_UX_CMN_INTP_STATUS         0x0004	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_UX_INTP_STATUS      0x0000ffff	/* [default:][perf:] ux interrupt status([ 0] isc_det_addr */
						  /* [ 1] cpl1_det_addr */
						  /* [ 2] lsu_det_addr */
						  /* [ 3] ux_det_activity */
						  /* [ 4] ux_det_retire */
						  /* [ 5] cprf_det_addr */
						  /* [ 8] wp0_vpu_error */
						  /* [ 9] wp1_vpu_error */
						  /* [10] wp2_vpu_error */
						  /* [11] wp3_vpu_error */
						  /* [12] wp0_ifb_wait */
						  /* [13] wp1_ifb_wait */
						  /* [14] wp2_ifb_wait */
						  /* [15] wp3_ifb_wait */
#define SFT_UX_CMN_UX_INTP_STATUS      0

/**********************************************************************************************************/
/* Activity detection setting */
#define REG_UX_CMN_ACTIVITY_DETECTION_0 0x0008	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ACT_OSTAGE_CHK_EN   0x00000001	/* [default:1'b1][perf:] Activity output stage check enable */
#define SFT_UX_CMN_ACT_OSTAGE_CHK_EN   0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ACT_ISTAGE_CHK_EN   0x00000002	/* [default:1'b1][perf:] Activity input stage check enable */
#define SFT_UX_CMN_ACT_ISTAGE_CHK_EN   1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ACT_ISC_CHK_EN      0x00000004 /* [default:1'b1][perf:] Activity instruction read check enable */
#define SFT_UX_CMN_ACT_ISC_CHK_EN      2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ACT_MEM_CHK_EN      0x00000008 /* [default:1'b1][perf:] Activity memory request check enable */
#define SFT_UX_CMN_ACT_MEM_CHK_EN      3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ACT_ENG_CHK_EN      0x00000010 /* [default:1'b1][perf:] Activity engine request check enable */
#define SFT_UX_CMN_ACT_ENG_CHK_EN      4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ACT_SYS_CHK_EN      0x00000020	/* [default:1'b1][perf:] Activity syscall check enable */
#define SFT_UX_CMN_ACT_SYS_CHK_EN      5

/**********************************************************************************************************/
/* Activity detection setting */
#define REG_UX_CMN_ACTIVITY_DETECTION_1 0x000c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_UX_ACTIVITY_TIMEOUT 0xffffffff	/* [default:32'hffff_ffff][perf:] ux activity timeout
								threshold (cycles) */
#define SFT_UX_CMN_UX_ACTIVITY_TIMEOUT 0

/**********************************************************************************************************/
/* Activity detection setting */
#define REG_UX_CMN_ACTIVITY_DETECTION_2 0x0010	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_UX_RETIRE_TIMEOUT   0xffffffff	/* [default:32'hffff_ffff][perf:] ux retire timeout
								threshold (cycles) */
#define SFT_UX_CMN_UX_RETIRE_TIMEOUT   0

/**********************************************************************************************************/
/* Advanced Debug Mode */
#define REG_UX_CMN_ADV_DEBUG_0         0x0014	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ADVDBG_EN           0x00000001	/* [default:1'b0][perf:] advanced debug enable
								(stop in specific condition) */
#define SFT_UX_CMN_ADVDBG_EN           0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ADVDBG_TYPE         0x00000030	/* [default:2'd0][perf:] advanced debug type
								(0~2: VS0/VS1/FS/CL) */
#define SFT_UX_CMN_ADVDBG_TYPE         4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ADVDBG_CHK_MODE     0x00000040	/* [default:1'b0][perf:] advanced debug check mode
								(0: one condition meets  1: all conditions meet) */
#define SFT_UX_CMN_ADVDBG_CHK_MODE     6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ADVDBG_CHK_SID_EN   0x00000100	/* [default:1'b0][perf:] advanced debug check sid enable */
#define SFT_UX_CMN_ADVDBG_CHK_SID_EN   8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ADVDBG_CHK_IBASE_EN 0x00000200	/* [default:1'b0][perf:] advanced debug check
								instruction base enable */
#define SFT_UX_CMN_ADVDBG_CHK_IBASE_EN 9
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ADVDBG_CHK_LASTPC_EN 0x00000400	/* [default:1'b0][perf:] advanced debug check lastpc enable */
#define SFT_UX_CMN_ADVDBG_CHK_LASTPC_EN 10

/**********************************************************************************************************/
/* Advanced Debug Mode */
#define REG_UX_CMN_ADV_DEBUG_1         0x0018	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ADVDBG_SID          0xffffffff	/* [default:32'hffff_ffff][perf:] advanced debug sid
								(debug would stop when cur_sid>=reg_sid) */
#define SFT_UX_CMN_ADVDBG_SID          0

/**********************************************************************************************************/
/* Advanced Debug Mode */
#define REG_UX_CMN_ADV_DEBUG_2         0x001c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ADVDBG_IBASE        0xffffffff	/* [default:32'hffff_ffff][perf:] advanced debug ibase
								(debug would stop when cur_ibase==reg_ibase) */
#define SFT_UX_CMN_ADVDBG_IBASE        0

/**********************************************************************************************************/
/* Advanced Debug Mode */
#define REG_UX_CMN_ADV_DEBUG_3         0x0020	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ADVDBG_LASTPC       0x000fffff	/* [default:20'hf_ffff][perf:] advanced debug sid
								(debug would stop when cur_lastpc>=reg_lastpc) */
#define SFT_UX_CMN_ADVDBG_LASTPC       0

/**********************************************************************************************************/
/* CPRF Access Control */
#define REG_UX_CMN_CPRF_ACCESS_CONTROL_0 0x0024	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_EN          0x00000001	/* [default:1'b0][perf:] cprf debug enable */
#define SFT_UX_CMN_CRF_DBG_EN          0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_FIRE        0x00000002	/* [default:1'b0][perf:] cprf debug fire (1T pulse) */
#define SFT_UX_CMN_CRF_DBG_FIRE        1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_RW          0x00000004	/* [default:1'b0][perf:] cprf debug r/w (0:read 1:write) */
#define SFT_UX_CMN_CRF_DBG_RW          2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_RSV0        0x00000008	/* [default:1'b0][perf:] cprf debug reserve */
#define SFT_UX_CMN_CRF_DBG_RSV0        3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_WPSEL       0x00000030	/* [default:2'd0][perf:] cprf debug wave pool
								select (0/1/2/3) */
#define SFT_UX_CMN_CRF_DBG_WPSEL       4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_RSV1        0x000000c0	/* [default:2'd0][perf:] cprf debug reserve */
#define SFT_UX_CMN_CRF_DBG_RSV1        6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_WID         0x00000f00	/* [default:4'd0][perf:] cprf debug wave id (0~11) */
#define SFT_UX_CMN_CRF_DBG_WID         8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_RSV2        0x0000f000	/* [default:4'd0][perf:] cprf debug reserve */
#define SFT_UX_CMN_CRF_DBG_RSV2        12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_CPADR       0x00ff0000	/* [default:8'd0][perf:] cprf debug cp address */
#define SFT_UX_CMN_CRF_DBG_CPADR       16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_RSV3        0x0f000000	/* [default:4'd0][perf:] cprf debug reserve */
#define SFT_UX_CMN_CRF_DBG_RSV3        24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_LANESEL     0x30000000	/* [default:2'd0][perf:] cprf debug lane select */
#define SFT_UX_CMN_CRF_DBG_LANESEL     28
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_RDVLD       0x80000000	/* [default:][perf:] cprf debug read valid */
#define SFT_UX_CMN_CRF_DBG_RDVLD       31

/**********************************************************************************************************/
/* CPRF Access Control */
#define REG_UX_CMN_CPRF_ACCESS_CONTROL_1 0x0028	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_WRDAT       0xffffffff	/* [default:32'd0][perf:] cprf debug write data */
#define SFT_UX_CMN_CRF_DBG_WRDAT       0

/**********************************************************************************************************/
/* CPRF Access Control */
#define REG_UX_CMN_CPRF_ACCESS_CONTROL_2 0x002c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CRF_DBG_RDDAT       0xffffffff	/* [default:][perf:] cprf debug read data */
#define SFT_UX_CMN_CRF_DBG_RDDAT       0

/**********************************************************************************************************/
/* L1 Cache Reset */
#define REG_UX_CMN_L1_CACHE_CONTROL    0x0030	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_VPL1_RESET          0x00000001	/* [default:1'b0][perf:] vp l1 reset */
#define SFT_UX_CMN_VPL1_RESET          0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CPL1_RESET          0x00000002	/* [default:1'b0][perf:] cp l1 reset */
#define SFT_UX_CMN_CPL1_RESET          1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ICL1_RESET          0x00000004	/* [default:1'b0][perf:] ic l1 reset */
#define SFT_UX_CMN_ICL1_RESET          2

/**********************************************************************************************************/
/* Instruction Cache Prefetch Control */
#define REG_UX_CMN_ISC_ACCESS_CONTROL_0 0x0034	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_EN          0x00000001	/* [default:1'b0][perf:] isc debug prefetch enable */
#define SFT_UX_CMN_ISC_DBG_EN          0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_PF_FIRE     0x00000002	/* [default:1'b0][perf:] isc debug prefetch fire (1T pulse) */
#define SFT_UX_CMN_ISC_DBG_PF_FIRE     1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_PF_RSV0     0x0000000c	/* [default:2'd0][perf:] isc debug prefetch reserve */
#define SFT_UX_CMN_ISC_DBG_PF_RSV0     2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_PF_PCBLOCK  0x000ffff0	/* [default:16'd0][perf:] isc debug prefetch pc block */
#define SFT_UX_CMN_ISC_DBG_PF_PCBLOCK  4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_PF_CID      0x00700000	/* [default:3'd0][perf:] isc debug prefetch cid */
#define SFT_UX_CMN_ISC_DBG_PF_CID      20
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_PF_RDVLD    0x00800000	/* [default:][perf:] isc debug prefetch read valid */
#define SFT_UX_CMN_ISC_DBG_PF_RDVLD    23
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_PF_REQADR   0xff000000	/* [default:][perf:] isc debug prefetch request address */
#define SFT_UX_CMN_ISC_DBG_PF_REQADR   24

/**********************************************************************************************************/
/* Instruction Cache Release Control */
#define REG_UX_CMN_ISC_ACCESS_CONTROL_1 0x0038	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RL_RSV0     0x00000001	/* [default:1'b0][perf:] isc debug release reserve */
#define SFT_UX_CMN_ISC_DBG_RL_RSV0     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RL_FIRE     0x00000002	/* [default:1'b0][perf:] isc debug release fire (1T pulse) */
#define SFT_UX_CMN_ISC_DBG_RL_FIRE     1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RL_RSV1     0x000000fc	/* [default:6'd0][perf:] isc debug release reserve */
#define SFT_UX_CMN_ISC_DBG_RL_RSV1     2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RL_CACADR   0x0000ff00	/* [default:8'd0][perf:] isc debug release address */
#define SFT_UX_CMN_ISC_DBG_RL_CACADR   8

/**********************************************************************************************************/
/* Instruction Cache Read Control */
#define REG_UX_CMN_ISC_ACCESS_CONTROL_2 0x003c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RD_RSV0     0x00000001	/* [default:1'b0][perf:] isc debug read reserve */
#define SFT_UX_CMN_ISC_DBG_RD_RSV0     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RD_FIRE     0x00000002	/* [default:1'b0][perf:] isc debug read fire (1T pulse) */
#define SFT_UX_CMN_ISC_DBG_RD_FIRE     1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RD_RDSEL    0x0000000c	/* [default:2'd0][perf:] isc debug read select
								(bit0:low/high select bit1: partial enable) */
#define SFT_UX_CMN_ISC_DBG_RD_RDSEL    2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RD_CACBK    0x00000030	/* [default:2'd0][perf:] isc debug read cache bank */
#define SFT_UX_CMN_ISC_DBG_RD_CACBK    4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RD_RSV1     0x000000c0	/* [default:2'd0][perf:] isc debug read reserve */
#define SFT_UX_CMN_ISC_DBG_RD_RSV1     6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RD_CACADR   0x0000ff00	/* [default:8'd0][perf:] isc debug read cache address */
#define SFT_UX_CMN_ISC_DBG_RD_CACADR   8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RD_RDVLD    0x80000000	/* [default:][perf:] isc debug read read valid */
#define SFT_UX_CMN_ISC_DBG_RD_RDVLD    31

/**********************************************************************************************************/
/* Instruction Cache Control: Read Data */
#define REG_UX_CMN_ISC_ACCESS_CONTROL_3 0x0040	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RD_DATA_LOW 0xffffffff	/* [default:][perf:] isc debug read data low */
#define SFT_UX_CMN_ISC_DBG_RD_DATA_LOW 0

/**********************************************************************************************************/
/* Instruction Cache Control: Read Data */
#define REG_UX_CMN_ISC_ACCESS_CONTROL_4 0x0044	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_DBG_RD_DATA_HIGH 0xffffffff	/* [default:][perf:] isc debug read data high */
#define SFT_UX_CMN_ISC_DBG_RD_DATA_HIGH 0

/**********************************************************************************************************/
/* EU Status Report A */
#define REG_UX_CMN_EU_STATUS_REPORT_0  0x0048	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_EU_STATUS_ID_A      0x0000000f	/* [default:4'd0][perf:] eu status id
							(0:cxm 1:tks 2:cpu 3:lsu 4:cpl1 5:vpl1 6:isc 7:fsi 8:vs0i
							9:vs1i 10:fsrr 11:vs0r 12:vs1r) */
#define SFT_UX_CMN_EU_STATUS_ID_A      0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_EU_STATUS_SEL_A     0xffff0000	/* [default:16'd0][perf:] eu status select */
#define SFT_UX_CMN_EU_STATUS_SEL_A     16

/**********************************************************************************************************/
/* EU Status Report A: Read Data */
#define REG_UX_CMN_EU_STATUS_REPORT_1  0x004c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_EU_STATUS_DATA_A    0xffffffff	/* [default:][perf:] eu status data */
#define SFT_UX_CMN_EU_STATUS_DATA_A    0

/**********************************************************************************************************/
/* EU Status Report B */
#define REG_UX_CMN_EU_STATUS_REPORT_2  0x0050	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_EU_STATUS_ID_B      0x0000000f	/* [default:4'd0][perf:] eu status id
							(0:cxm 1:tks 2:cpu 3:lsu 4:cpl1 5:vpl1 6:isc 7:fsi 8:vs0i
							9:vs1i 10:fsrr 11:vs0r 12:vs1r) */
#define SFT_UX_CMN_EU_STATUS_ID_B      0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_EU_STATUS_SEL_B     0xffff0000	/* [default:16'd0][perf:] eu status select */
#define SFT_UX_CMN_EU_STATUS_SEL_B     16

/**********************************************************************************************************/
/* EU Status Report B: Read Data */
#define REG_UX_CMN_EU_STATUS_REPORT_3  0x0054	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_EU_STATUS_DATA_B    0xffffffff	/* [default:][perf:] eu status data */
#define SFT_UX_CMN_EU_STATUS_DATA_B    0

/**********************************************************************************************************/
/* Context Global Read */
#define REG_UX_CMN_CX_GLOBAL_READ_0    0x0058	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CXM_GLOBAL_CID      0x0000000f	/* [default:4'd0][perf:] context global cid
								(0~3: FS/CL 6:VS0 7:VS1) */
#define SFT_UX_CMN_CXM_GLOBAL_CID      0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CXM_GLOBAL_SEL      0x000001f0	/* [default:5'd0][perf:] context/frame global select
								(0~15: context global 16~31: frame global) */
#define SFT_UX_CMN_CXM_GLOBAL_SEL      4

/**********************************************************************************************************/
/* Context Global Read */
#define REG_UX_CMN_CX_GLOBAL_READ_1    0x005c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CXM_GLOBAL_DATA     0xffffffff	/* [default:][perf:] context/frame global data */
#define SFT_UX_CMN_CXM_GLOBAL_DATA     0

/**********************************************************************************************************/
/* Error Memory Address */
#define REG_UX_CMN_ERROR_MEM_ADDR_0    0x0060	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_LSU_ERROR_MADDR     0xffffffff	/* [default:][perf:] error memory address sent by lsu */
#define SFT_UX_CMN_LSU_ERROR_MADDR     0

/**********************************************************************************************************/
/* Error Memory Address */
#define REG_UX_CMN_ERROR_MEM_ADDR_1    0x0064	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CPL1_ERROR_MADDR    0xffffffff	/* [default:][perf:] error memory address sent by cpl1 */
#define SFT_UX_CMN_CPL1_ERROR_MADDR    0

/**********************************************************************************************************/
/* Error Memory Address */
#define REG_UX_CMN_ERROR_MEM_ADDR_2    0x0068	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_ERROR_MADDR     0xffffffff	/* [default:][perf:] error memory address sent by isc */
#define SFT_UX_CMN_ISC_ERROR_MADDR     0

/**********************************************************************************************************/
/* Error CPRF Address */
#define REG_UX_CMN_ERROR_CPRF_ADDR     0x006c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CPRF_ERROR_FID      0x0000003f	/* [default:][perf:] cprf error fid */
#define SFT_UX_CMN_CPRF_ERROR_FID      0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CPRF_ERROR_ADDR     0x0000ff00	/* [default:][perf:] cprf error address */
#define SFT_UX_CMN_CPRF_ERROR_ADDR     8

/**********************************************************************************************************/
/* in/out io */
#define REG_UX_CMN_UX_STATUS_REPORT_0  0x0070	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_INPUT_STAGE_IO      0x0003ffff	/* [default:][perf:] {vs0i_rdy,vs0i_ack,vs0i2tks_rdy,
							tks2vs0i_ack, vs0i2smm_rdy,smm2vs0i_ack, */
						  /* vs1i_rdy,vs1i_ack,vs1i2tks_rdy, tks2vs1i_ack,
							vs1i2smm_rdy,smm2vs1i_ack, */
						  /* fsi_rdy,   fsi_ack,   fsi2tks_rdy,
							tks2fsi_ack,   fsi2smm_rdy,  smm2fsi_ack} */
#define SFT_UX_CMN_INPUT_STAGE_IO      0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_OUTPUT_STAGE_IO     0xff000000	/* [default:][perf:] {ux2vl_vs0_rdy,vl2ux_vs0_ack,
							ux2vl_vs1_rdy,vl2ux_vs1_ack,ux2vx_rdy,vx2ux_ack, */
						  /* ux2cp_rdy,cp2ux_ack} */
#define SFT_UX_CMN_OUTPUT_STAGE_IO     24

/**********************************************************************************************************/
/* cache io */
#define REG_UX_CMN_UX_STATUS_REPORT_1  0x0074	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_TX_IO               0x000003ff	/* [default:][perf:] {ux2tx_rdy,tx2ux_ack,ux2tx_idx_rdy,
								tx2ux_idx_ack,ux2tx_pix_rdy,tx2ux_pix_ack, */
					/* ux2tx_glbini_rdy,tx2ux_glbini_ack,ux2tx_glbclr_rdy,tx2ux_glbclr_ack} */
#define SFT_UX_CMN_TX_IO               0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_LTC_IO              0x00ff0000	/* [default:][perf:] {ux2mx_cp_rd_req,mx2ux_cp_rd_ack,
								ux2mx_ic_rd_req,mx2ux_ic_rd_ack, */
						  /* ux2mx_vp_rd_req,mx2ux_vp_rd_ack,ux2mx_vp_wr_req,
							mx2ux_vp_wr_ack} */
#define SFT_UX_CMN_LTC_IO              16

/**********************************************************************************************************/
/* isc io */
#define REG_UX_CMN_UX_STATUS_REPORT_2  0x0078	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_PF_IO           0x000000ff /* [default:][perf:] {...,isc2dbg_wp0_pf_rdy,isc2dbg_wp0_pf_ack} */
#define SFT_UX_CMN_ISC_PF_IO           0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_RL_IO           0x0000ff00 /* [default:][perf:] {...,isc2dbg_wp0_rl_rdy,isc2dbg_wp0_rl_ack} */
#define SFT_UX_CMN_ISC_RL_IO           8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_RD_IO           0x00ff0000 /* [default:][perf:] {...,isc2dbg_wp0_rd_rdy,isc2dbg_wp0_rd_ack} */
#define SFT_UX_CMN_ISC_RD_IO           16

/**********************************************************************************************************/
/* idle */
#define REG_UX_CMN_UX_STATUS_REPORT_3  0x007c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_IDLE_STATUS         0x000007ff	/* [default:][perf:] {cpu,lsu,isc,vpl1,cpl1,vs0i,
								vs1i,fsi,vs0r,vs1r,fsr} */
#define SFT_UX_CMN_IDLE_STATUS         0

/**********************************************************************************************************/
/* tkx/cxm */
#define REG_UX_CMN_UX_STATUS_REPORT_4  0x0080	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_VS0_WV_CNT          0x0000003f	/* [default:][perf:] vs0_wv_cnt */
#define SFT_UX_CMN_VS0_WV_CNT          0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_VS1_WV_CNT          0x00003f00	/* [default:][perf:] vs1_wv_cnt */
#define SFT_UX_CMN_VS1_WV_CNT          8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_FS_WV_CNT           0x003f0000	/* [default:][perf:] fs_wv_cnt */
#define SFT_UX_CMN_FS_WV_CNT           16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_CTX_NUM             0x07000000	/* [default:][perf:] ctx_num */
#define SFT_UX_CMN_CTX_NUM             24

/**********************************************************************************************************/
/* stop fid */
#define REG_UX_CMN_UX_STATUS_REPORT_5  0x0084	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_VS0I_STOP_FID       0x0000003f	/* [default:][perf:] vs0i stop fid */
#define SFT_UX_CMN_VS0I_STOP_FID       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_VS1I_STOP_FID       0x00003f00	/* [default:][perf:] vs1i stop fid */
#define SFT_UX_CMN_VS1I_STOP_FID       8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_FSI_STOP_FID        0x003f0000	/* [default:][perf:] fsi stop fid */
#define SFT_UX_CMN_FSI_STOP_FID        16

/**********************************************************************************************************/
/* vs0i sequence id */
#define REG_UX_CMN_UX_STATUS_REPORT_6  0x0088	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_VS0I_SID            0xffffffff	/* [default:][perf:] vs0i sequence id */
#define SFT_UX_CMN_VS0I_SID            0

/**********************************************************************************************************/
/* vs1i sequence id */
#define REG_UX_CMN_UX_STATUS_REPORT_7  0x008c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_VS1I_SID            0xffffffff	/* [default:][perf:] vs1i sequence id */
#define SFT_UX_CMN_VS1I_SID            0

/**********************************************************************************************************/
/* fsi sequence id */
#define REG_UX_CMN_UX_STATUS_REPORT_8  0x0090	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_FSI_SID             0xffffffff	/* [default:][perf:] fsi sequence id */
#define SFT_UX_CMN_FSI_SID             0

/**********************************************************************************************************/
/* wave count */
#define REG_UX_CMN_UX_STATUS_REPORT_9  0x0094	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP0_WV_CNT          0x0000000f	/* [default:][perf:] wp0_wv_cnt */
#define SFT_UX_CMN_WP0_WV_CNT          0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP1_WV_CNT          0x000000f0	/* [default:][perf:] wp1_wv_cnt */
#define SFT_UX_CMN_WP1_WV_CNT          4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP2_WV_CNT          0x00000f00	/* [default:][perf:] wp2_wv_cnt */
#define SFT_UX_CMN_WP2_WV_CNT          8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP3_WV_CNT          0x0000f000	/* [default:][perf:] wp3_wv_cnt */
#define SFT_UX_CMN_WP3_WV_CNT          12

/**********************************************************************************************************/
/* wp0 lsm remainder size */
#define REG_UX_CMN_UX_STATUS_REPORT_10 0x0098	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP0_LSM_PRV_REM     0x000001ff	/* [default:9'd64][perf:] wp0_lsm_prv_rem */
#define SFT_UX_CMN_WP0_LSM_PRV_REM     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP0_LSM_GRP_REM     0x0007fc00	/* [default:9'd64][perf:] wp0_lsm_grp_rem */
#define SFT_UX_CMN_WP0_LSM_GRP_REM     10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP0_LSM_GLB_REM     0x1ff00000	/* [default:9'd0][perf:] wp0_lsm_glb_rem */
#define SFT_UX_CMN_WP0_LSM_GLB_REM     20

/**********************************************************************************************************/
/* wp0 vrf/crf remainder size */
#define REG_UX_CMN_UX_STATUS_REPORT_11 0x009c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP0_VRF_REM         0x000001ff	/* [default:9'd256][perf:] wp0_vrf_rem */
#define SFT_UX_CMN_WP0_VRF_REM         0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP0_CRF_REM         0x0007fc00	/* [default:9'd128][perf:] wp0_crf_rem */
#define SFT_UX_CMN_WP0_CRF_REM         10

/**********************************************************************************************************/
/* wp1 lsm remainder size */
#define REG_UX_CMN_UX_STATUS_REPORT_12 0x00a0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP1_LSM_PRV_REM     0x000001ff	/* [default:9'd64][perf:] wp1_lsm_prv_rem */
#define SFT_UX_CMN_WP1_LSM_PRV_REM     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP1_LSM_GRP_REM     0x0007fc00	/* [default:9'd64][perf:] wp1_lsm_grp_rem */
#define SFT_UX_CMN_WP1_LSM_GRP_REM     10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP1_LSM_GLB_REM     0x1ff00000	/* [default:9'd0][perf:] wp1_lsm_glb_rem */
#define SFT_UX_CMN_WP1_LSM_GLB_REM     20

/**********************************************************************************************************/
/* wp1 vrf/crf remainder size */
#define REG_UX_CMN_UX_STATUS_REPORT_13 0x00a4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP1_VRF_REM         0x000001ff	/* [default:9'd256][perf:] wp1_vrf_rem */
#define SFT_UX_CMN_WP1_VRF_REM         0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP1_CRF_REM         0x0007fc00	/* [default:9'd128][perf:] wp1_crf_rem */
#define SFT_UX_CMN_WP1_CRF_REM         10

/**********************************************************************************************************/
/* wp2 lsm remainder size */
#define REG_UX_CMN_UX_STATUS_REPORT_14 0x00a8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP2_LSM_PRV_REM     0x000001ff	/* [default:9'd64][perf:] wp2_lsm_prv_rem */
#define SFT_UX_CMN_WP2_LSM_PRV_REM     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP2_LSM_GRP_REM     0x0007fc00	/* [default:9'd64][perf:] wp2_lsm_grp_rem */
#define SFT_UX_CMN_WP2_LSM_GRP_REM     10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP2_LSM_GLB_REM     0x1ff00000	/* [default:9'd0][perf:] wp2_lsm_glb_rem */
#define SFT_UX_CMN_WP2_LSM_GLB_REM     20

/**********************************************************************************************************/
/* wp2 vrf/crf remainder size */
#define REG_UX_CMN_UX_STATUS_REPORT_15 0x00ac	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP2_VRF_REM         0x000001ff	/* [default:9'd256][perf:] wp2_vrf_rem */
#define SFT_UX_CMN_WP2_VRF_REM         0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP2_CRF_REM         0x0007fc00	/* [default:9'd128][perf:] wp2_crf_rem */
#define SFT_UX_CMN_WP2_CRF_REM         10

/**********************************************************************************************************/
/* wp3 lsm remainder size */
#define REG_UX_CMN_UX_STATUS_REPORT_16 0x00b0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP3_LSM_PRV_REM     0x000001ff	/* [default:9'd64][perf:] wp3_lsm_prv_rem */
#define SFT_UX_CMN_WP3_LSM_PRV_REM     0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP3_LSM_GRP_REM     0x0007fc00	/* [default:9'd64][perf:] wp3_lsm_grp_rem */
#define SFT_UX_CMN_WP3_LSM_GRP_REM     10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP3_LSM_GLB_REM     0x1ff00000	/* [default:9'd0][perf:] wp3_lsm_glb_rem */
#define SFT_UX_CMN_WP3_LSM_GLB_REM     20

/**********************************************************************************************************/
/* wp3 vrf/crf remainder size */
#define REG_UX_CMN_UX_STATUS_REPORT_17 0x00b4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP3_VRF_REM         0x000001ff	/* [default:9'd256][perf:] wp3_vrf_rem */
#define SFT_UX_CMN_WP3_VRF_REM         0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_WP3_CRF_REM         0x0007fc00	/* [default:9'd128][perf:] wp3_crf_rem */
#define SFT_UX_CMN_WP3_CRF_REM         10

/**********************************************************************************************************/
/* isc frame mask */
#define REG_UX_CMN_ISC_FRAME_CONTROL_0 0x00b8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_GMASK0          0xffffffff	/* [default:32'd0][perf:] ISC frame mask 0 */
#define SFT_UX_CMN_ISC_GMASK0          0

/**********************************************************************************************************/
/* isc frame mask */
#define REG_UX_CMN_ISC_FRAME_CONTROL_1 0x00bc	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_GMASK1          0xffffffff	/* [default:32'd0][perf:] ISC frame mask 1 */
#define SFT_UX_CMN_ISC_GMASK1          0

/**********************************************************************************************************/
/* isc frame mask */
#define REG_UX_CMN_ISC_FRAME_CONTROL_2 0x00c0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_GMASK2          0xffffffff	/* [default:32'd0][perf:] ISC frame mask 2 */
#define SFT_UX_CMN_ISC_GMASK2          0

/**********************************************************************************************************/
/* isc frame mask */
#define REG_UX_CMN_ISC_FRAME_CONTROL_3 0x00c4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_CMN_ISC_GMASK3          0xffffffff	/* [default:32'd0][perf:] ISC frame mask 3 */
#define SFT_UX_CMN_ISC_GMASK3          0

/**********************************************************************************************************/


typedef struct {
	union			/* 0x0000, */
	{
		unsigned int dwValue;	/* Debug Mode Enable Settings */
		struct {
			unsigned int isc_det_addr_en:1;	/* isc detect address enable */
			/* default: 1'b0 */
			unsigned int isc_cor_addr_en:1;	/* isc correct address enable */
			/* default: 1'b0 */
			unsigned int cpl1_det_addr_en:1;	/* cpl1 detect address enable */
			/* default: 1'b0 */
			unsigned int cpl1_cor_addr_en:1;	/* cpl1 correct address enable */
			/* default: 1'b0 */
			unsigned int lsu_det_addr_en:1;	/* lsu detect address enable */
			/* default: 1'b0 */
			unsigned int lsu_cor_addr_en:1;	/* lsu correct address enable */
			/* default: 1'b0 */
			unsigned int ux_det_activity_en:1;	/* ux detect activity enable */
			/* default: 1'b0 */
			unsigned int ux_det_retire_en:1;	/* ux detect retire enable */
			/* default: 1'b0 */
			unsigned int cprf_det_addr_en:1;	/* cprf detect wrong address access enable */
			/* default: 1'b0 */
			unsigned int cprf_cor_addr_en:1;	/* cprf correct wrong address write enable */
			/* default: 1'b0 */
			unsigned int reserved0:6;
			unsigned int in_stage_sid_en:1;	/* input stage sequence id enable (enable sid count) */
			/* default: 1'b0 */
			unsigned int in_stage_sid_rsv0:1;	/* reserved */
			/* default: 1'b0 */
			unsigned int in_stage_sid_rst_sel:1;	/* input stage sequence id reset select
								([0]: pp_frame_flush [1]: vp_frame_rst) */
			/* default: 1'b0 */
			unsigned int reserved1:5;
			unsigned int ifbstop_force_en:1;	/* IFB Engine Stop Force
								(All IFB Engine stop issuing instructions) */
			/* default: 1'b0 */
			unsigned int ifbstop_vpuerr_en:1;	/* IFB Engine Stop on VPU Error */
			/* default: 1'b0 */
			unsigned int ifbstop_memerr_en:1;	/* IFB Engine Stop on Memory Check Error */
			/* default: 1'b0 */
			unsigned int ifbstop_acterr_en:1;	/* IFB Engine Stop on Activity Check Error */
			/* default: 1'b0 */
			unsigned int reserved2:2;
			unsigned int ifbstop_release:1;	/* IFB Engine Release (All Stopped Engine are released) */
			/* default: 1'b0 */
			unsigned int debug_en:1;	/* Debug Enable for data latch
							(read/dump eu_status_data should active this bit) */
			/* default: 1'b0 */
		} s;
	} reg_debug_enable;

	union			/* 0x0004, */
	{
		unsigned int dwValue;	/* Report of interrupt status */
		struct {
			unsigned int ux_intp_status:16;	/* ux interrupt status([ 0] isc_det_addr */
			/* [ 1] cpl1_det_addr */
			/* [ 2] lsu_det_addr */
			/* [ 3] ux_det_activity */
			/* [ 4] ux_det_retire */
			/* [ 5] cprf_det_addr */
			/* [ 8] wp0_vpu_error */
			/* [ 9] wp1_vpu_error */
			/* [10] wp2_vpu_error */
			/* [11] wp3_vpu_error */
			/* [12] wp0_ifb_wait */
			/* [13] wp1_ifb_wait */
			/* [14] wp2_ifb_wait */
			/* [15] wp3_ifb_wait */
		} s;
	} reg_intp_status;

	union			/* 0x0008, */
	{
		unsigned int dwValue;	/* Activity detection setting */
		struct {
			unsigned int act_ostage_chk_en:1;	/* Activity output stage check enable */
			/* default: 1'b1 */
			unsigned int act_istage_chk_en:1;	/* Activity input stage check enable */
			/* default: 1'b1 */
			unsigned int act_isc_chk_en:1;	/* Activity instruction read check enable */
			/* default: 1'b1 */
			unsigned int act_mem_chk_en:1;	/* Activity memory request check enable */
			/* default: 1'b1 */
			unsigned int act_eng_chk_en:1;	/* Activity engine request check enable */
			/* default: 1'b1 */
			unsigned int act_sys_chk_en:1;	/* Activity syscall check enable */
			/* default: 1'b1 */
		} s;
	} reg_activity_detection_0;

	union			/* 0x000c, */
	{
		unsigned int dwValue;	/* Activity detection setting */
		struct {
			unsigned int ux_activity_timeout:32;	/* ux activity timeout threshold (cycles) */
			/* default: 32'hffff_ffff */
		} s;
	} reg_activity_detection_1;

	union			/* 0x0010, */
	{
		unsigned int dwValue;	/* Activity detection setting */
		struct {
			unsigned int ux_retire_timeout:32;	/* ux retire timeout threshold (cycles) */
			/* default: 32'hffff_ffff */
		} s;
	} reg_activity_detection_2;

	union			/* 0x0014, */
	{
		unsigned int dwValue;	/* Advanced Debug Mode */
		struct {
			unsigned int advdbg_en:1;	/* advanced debug enable (stop in specific condition) */
			/* default: 1'b0 */
			unsigned int reserved3:3;
			unsigned int advdbg_type:2;	/* advanced debug type(0~2: VS0/VS1/FS/CL) */
			/* default: 2'd0 */
			unsigned int advdbg_chk_mode:1;	/* advanced debug check mode
							(0: one condition meets  1: all conditions meet) */
			/* default: 1'b0 */
			unsigned int reserved4:1;
			unsigned int advdbg_chk_sid_en:1;	/* advanced debug check sid enable */
			/* default: 1'b0 */
			unsigned int advdbg_chk_ibase_en:1;	/* advanced debug check instruction base enable */
			/* default: 1'b0 */
			unsigned int advdbg_chk_lastpc_en:1;	/* advanced debug check lastpc enable */
			/* default: 1'b0 */
		} s;
	} reg_adv_debug_0;

	union			/* 0x0018, */
	{
		unsigned int dwValue;	/* Advanced Debug Mode */
		struct {
			unsigned int advdbg_sid:32;	/* advanced debug sid
							(debug would stop when cur_sid>=reg_sid) */
			/* default: 32'hffff_ffff */
		} s;
	} reg_adv_debug_1;

	union			/* 0x001c, */
	{
		unsigned int dwValue;	/* Advanced Debug Mode */
		struct {
			unsigned int advdbg_ibase:32;	/* advanced debug ibase
							(debug would stop when cur_ibase==reg_ibase) */
			/* default: 32'hffff_ffff */
		} s;
	} reg_adv_debug_2;

	union			/* 0x0020, */
	{
		unsigned int dwValue;	/* Advanced Debug Mode */
		struct {
			unsigned int advdbg_lastpc:20;	/* advanced debug sid
							(debug would stop when cur_lastpc>=reg_lastpc) */
			/* default: 20'hf_ffff */
		} s;
	} reg_adv_debug_3;

	union			/* 0x0024, */
	{
		unsigned int dwValue;	/* CPRF Access Control */
		struct {
			unsigned int crf_dbg_en:1;	/* cprf debug enable */
			/* default: 1'b0 */
			unsigned int crf_dbg_fire:1;	/* cprf debug fire (1T pulse) */
			/* default: 1'b0 */
			unsigned int crf_dbg_rw:1;	/* cprf debug r/w (0:read 1:write) */
			/* default: 1'b0 */
			unsigned int crf_dbg_rsv0:1;	/* cprf debug reserve */
			/* default: 1'b0 */
			unsigned int crf_dbg_wpsel:2;	/* cprf debug wave pool select (0/1/2/3) */
			/* default: 2'd0 */
			unsigned int crf_dbg_rsv1:2;	/* cprf debug reserve */
			/* default: 2'd0 */
			unsigned int crf_dbg_wid:4;	/* cprf debug wave id (0~11) */
			/* default: 4'd0 */
			unsigned int crf_dbg_rsv2:4;	/* cprf debug reserve */
			/* default: 4'd0 */
			unsigned int crf_dbg_cpadr:8;	/* cprf debug cp address */
			/* default: 8'd0 */
			unsigned int crf_dbg_rsv3:4;	/* cprf debug reserve */
			/* default: 4'd0 */
			unsigned int crf_dbg_lanesel:2;	/* cprf debug lane select */
			/* default: 2'd0 */
			unsigned int reserved5:1;
			unsigned int crf_dbg_rdvld:1;	/* cprf debug read valid */
		} s;
	} reg_cprf_access_control_0;

	union			/* 0x0028, */
	{
		unsigned int dwValue;	/* CPRF Access Control */
		struct {
			unsigned int crf_dbg_wrdat:32;	/* cprf debug write data */
			/* default: 32'd0 */
		} s;
	} reg_cprf_access_control_1;

	union			/* 0x002c, */
	{
		unsigned int dwValue;	/* CPRF Access Control */
		struct {
			unsigned int crf_dbg_rddat:32;	/* cprf debug read data */
		} s;
	} reg_cprf_access_control_2;

	union			/* 0x0030, */
	{
		unsigned int dwValue;	/* L1 Cache Reset */
		struct {
			unsigned int vpl1_reset:1;	/* vp l1 reset */
			/* default: 1'b0 */
			unsigned int cpl1_reset:1;	/* cp l1 reset */
			/* default: 1'b0 */
			unsigned int icl1_reset:1;	/* ic l1 reset */
			/* default: 1'b0 */
		} s;
	} reg_l1_cache_control;

	union			/* 0x0034, */
	{
		unsigned int dwValue;	/* Instruction Cache Prefetch Control */
		struct {
			unsigned int isc_dbg_en:1;	/* isc debug prefetch enable */
			/* default: 1'b0 */
			unsigned int isc_dbg_pf_fire:1;	/* isc debug prefetch fire (1T pulse) */
			/* default: 1'b0 */
			unsigned int isc_dbg_pf_rsv0:2;	/* isc debug prefetch reserve */
			/* default: 2'd0 */
			unsigned int isc_dbg_pf_pcblock:16;	/* isc debug prefetch pc block */
			/* default: 16'd0 */
			unsigned int isc_dbg_pf_cid:3;	/* isc debug prefetch cid */
			/* default: 3'd0 */
			unsigned int isc_dbg_pf_rdvld:1;	/* isc debug prefetch read valid */
			unsigned int isc_dbg_pf_reqadr:8;	/* isc debug prefetch request address */
		} s;
	} reg_isc_access_control_0;

	union			/* 0x0038, */
	{
		unsigned int dwValue;	/* Instruction Cache Release Control */
		struct {
			unsigned int isc_dbg_rl_rsv0:1;	/* isc debug release reserve */
			/* default: 1'b0 */
			unsigned int isc_dbg_rl_fire:1;	/* isc debug release fire (1T pulse) */
			/* default: 1'b0 */
			unsigned int isc_dbg_rl_rsv1:6;	/* isc debug release reserve */
			/* default: 6'd0 */
			unsigned int isc_dbg_rl_cacadr:8;	/* isc debug release address */
			/* default: 8'd0 */
		} s;
	} reg_isc_access_control_1;

	union			/* 0x003c, */
	{
		unsigned int dwValue;	/* Instruction Cache Read Control */
		struct {
			unsigned int isc_dbg_rd_rsv0:1;	/* isc debug read reserve */
			/* default: 1'b0 */
			unsigned int isc_dbg_rd_fire:1;	/* isc debug read fire (1T pulse) */
			/* default: 1'b0 */
			unsigned int isc_dbg_rd_rdsel:2;	/* isc debug read select (bit0:low/high select
									bit1: partial enable) */
			/* default: 2'd0 */
			unsigned int isc_dbg_rd_cacbk:2;	/* isc debug read cache bank */
			/* default: 2'd0 */
			unsigned int isc_dbg_rd_rsv1:2;	/* isc debug read reserve */
			/* default: 2'd0 */
			unsigned int isc_dbg_rd_cacadr:8;	/* isc debug read cache address */
			/* default: 8'd0 */
			unsigned int reserved6:15;
			unsigned int isc_dbg_rd_rdvld:1;	/* isc debug read read valid */
		} s;
	} reg_isc_access_control_2;

	union			/* 0x0040, */
	{
		unsigned int dwValue;	/* Instruction Cache Control: Read Data */
		struct {
			unsigned int isc_dbg_rd_data_low:32;	/* isc debug read data low */
		} s;
	} reg_isc_access_control_3;

	union			/* 0x0044, */
	{
		unsigned int dwValue;	/* Instruction Cache Control: Read Data */
		struct {
			unsigned int isc_dbg_rd_data_high:32;	/* isc debug read data high */
		} s;
	} reg_isc_access_control_4;

	union			/* 0x0048, */
	{
		unsigned int dwValue;	/* EU Status Report A */
		struct {
			unsigned int eu_status_id_a:4;	/* eu status id (0:cxm 1:tks 2:cpu 3:lsu 4:cpl1 5:vpl1 6:isc
								7:fsi 8:vs0i 9:vs1i 10:fsrr 11:vs0r 12:vs1r) */
			/* default: 4'd0 */
			unsigned int reserved7:12;
			unsigned int eu_status_sel_a:16;	/* eu status select */
			/* default: 16'd0 */
		} s;
	} reg_eu_status_report_0;

	union			/* 0x004c, */
	{
		unsigned int dwValue;	/* EU Status Report A: Read Data */
		struct {
			unsigned int eu_status_data_a:32;	/* eu status data */
		} s;
	} reg_eu_status_report_1;

	union			/* 0x0050, */
	{
		unsigned int dwValue;	/* EU Status Report B */
		struct {
			unsigned int eu_status_id_b:4;	/* eu status id (0:cxm 1:tks 2:cpu 3:lsu 4:cpl1 5:vpl1 6:isc
								7:fsi 8:vs0i 9:vs1i 10:fsrr 11:vs0r 12:vs1r) */
			/* default: 4'd0 */
			unsigned int reserved8:12;
			unsigned int eu_status_sel_b:16;	/* eu status select */
			/* default: 16'd0 */
		} s;
	} reg_eu_status_report_2;

	union			/* 0x0054, */
	{
		unsigned int dwValue;	/* EU Status Report B: Read Data */
		struct {
			unsigned int eu_status_data_b:32;	/* eu status data */
		} s;
	} reg_eu_status_report_3;

	union			/* 0x0058, */
	{
		unsigned int dwValue;	/* Context Global Read */
		struct {
			unsigned int cxm_global_cid:4;	/* context global cid (0~3: FS/CL 6:VS0 7:VS1) */
			/* default: 4'd0 */
			unsigned int cxm_global_sel:5;	/* context/frame global select
								(0~15: context global 16~31: frame global) */
			/* default: 5'd0 */
		} s;
	} reg_cx_global_read_0;

	union			/* 0x005c, */
	{
		unsigned int dwValue;	/* Context Global Read */
		struct {
			unsigned int cxm_global_data:32;	/* context/frame global data */
		} s;
	} reg_cx_global_read_1;

	union			/* 0x0060, */
	{
		unsigned int dwValue;	/* Error Memory Address */
		struct {
			unsigned int lsu_error_maddr:32;	/* error memory address sent by lsu */
		} s;
	} reg_error_mem_addr_0;

	union			/* 0x0064, */
	{
		unsigned int dwValue;	/* Error Memory Address */
		struct {
			unsigned int cpl1_error_maddr:32;	/* error memory address sent by cpl1 */
		} s;
	} reg_error_mem_addr_1;

	union			/* 0x0068, */
	{
		unsigned int dwValue;	/* Error Memory Address */
		struct {
			unsigned int isc_error_maddr:32;	/* error memory address sent by isc */
		} s;
	} reg_error_mem_addr_2;

	union			/* 0x006c, */
	{
		unsigned int dwValue;	/* Error CPRF Address */
		struct {
			unsigned int cprf_error_fid:6;	/* cprf error fid */
			unsigned int reserved9:2;
			unsigned int cprf_error_addr:8;	/* cprf error address */
		} s;
	} reg_error_cprf_addr;

	union			/* 0x0070, */
	{
		unsigned int dwValue;	/* in/out io */
		struct {
			unsigned int input_stage_io:18;	/* {vs0i_rdy,vs0i_ack,vs0i2tks_rdy, tks2vs0i_ack,
								vs0i2smm_rdy,smm2vs0i_ack, */
			/* vs1i_rdy,vs1i_ack,vs1i2tks_rdy, tks2vs1i_ack, vs1i2smm_rdy,smm2vs1i_ack, */
			/* fsi_rdy,   fsi_ack,   fsi2tks_rdy,   tks2fsi_ack,   fsi2smm_rdy,  smm2fsi_ack} */
			unsigned int reserved10:6;
			unsigned int output_stage_io:8;	/* {ux2vl_vs0_rdy,vl2ux_vs0_ack,ux2vl_vs1_rdy,
								vl2ux_vs1_ack,ux2vx_rdy,vx2ux_ack, */
			/* ux2cp_rdy,cp2ux_ack} */
		} s;
	} reg_ux_status_report_0;

	union			/* 0x0074, */
	{
		unsigned int dwValue;	/* cache io */
		struct {
			unsigned int tx_io:10;	/* {ux2tx_rdy,tx2ux_ack,ux2tx_idx_rdy,
							tx2ux_idx_ack,ux2tx_pix_rdy,tx2ux_pix_ack, */
			/* ux2tx_glbini_rdy,tx2ux_glbini_ack,ux2tx_glbclr_rdy,tx2ux_glbclr_ack} */
			unsigned int reserved11:6;
			unsigned int ltc_io:8;	/* {ux2mx_cp_rd_req,mx2ux_cp_rd_ack,
							ux2mx_ic_rd_req,mx2ux_ic_rd_ack, */
			/* ux2mx_vp_rd_req,mx2ux_vp_rd_ack,ux2mx_vp_wr_req,mx2ux_vp_wr_ack} */
		} s;
	} reg_ux_status_report_1;

	union			/* 0x0078, */
	{
		unsigned int dwValue;	/* isc io */
		struct {
			unsigned int isc_pf_io:8;	/* {...,isc2dbg_wp0_pf_rdy,isc2dbg_wp0_pf_ack} */
			unsigned int isc_rl_io:8;	/* {...,isc2dbg_wp0_rl_rdy,isc2dbg_wp0_rl_ack} */
			unsigned int isc_rd_io:8;	/* {...,isc2dbg_wp0_rd_rdy,isc2dbg_wp0_rd_ack} */
		} s;
	} reg_ux_status_report_2;

	union			/* 0x007c, */
	{
		unsigned int dwValue;	/* idle */
		struct {
			unsigned int idle_status:11;	/* {cpu,lsu,isc,vpl1,cpl1,vs0i,vs1i,fsi,vs0r,vs1r,fsr} */
		} s;
	} reg_ux_status_report_3;

	union			/* 0x0080, */
	{
		unsigned int dwValue;	/* tkx/cxm */
		struct {
			unsigned int vs0_wv_cnt:6;	/* vs0_wv_cnt */
			unsigned int reserved12:2;
			unsigned int vs1_wv_cnt:6;	/* vs1_wv_cnt */
			unsigned int reserved13:2;
			unsigned int fs_wv_cnt:6;	/* fs_wv_cnt */
			unsigned int reserved14:2;
			unsigned int ctx_num:3;	/* ctx_num */
		} s;
	} reg_ux_status_report_4;

	union			/* 0x0084, */
	{
		unsigned int dwValue;	/* stop fid */
		struct {
			unsigned int vs0i_stop_fid:6;	/* vs0i stop fid */
			unsigned int reserved15:2;
			unsigned int vs1i_stop_fid:6;	/* vs1i stop fid */
			unsigned int reserved16:2;
			unsigned int fsi_stop_fid:6;	/* fsi stop fid */
		} s;
	} reg_ux_status_report_5;

	union			/* 0x0088, */
	{
		unsigned int dwValue;	/* vs0i sequence id */
		struct {
			unsigned int vs0i_sid:32;	/* vs0i sequence id */
		} s;
	} reg_ux_status_report_6;

	union			/* 0x008c, */
	{
		unsigned int dwValue;	/* vs1i sequence id */
		struct {
			unsigned int vs1i_sid:32;	/* vs1i sequence id */
		} s;
	} reg_ux_status_report_7;

	union			/* 0x0090, */
	{
		unsigned int dwValue;	/* fsi sequence id */
		struct {
			unsigned int fsi_sid:32;	/* fsi sequence id */
		} s;
	} reg_ux_status_report_8;

	union			/* 0x0094, */
	{
		unsigned int dwValue;	/* wave count */
		struct {
			unsigned int wp0_wv_cnt:4;	/* wp0_wv_cnt */
			unsigned int wp1_wv_cnt:4;	/* wp1_wv_cnt */
			unsigned int wp2_wv_cnt:4;	/* wp2_wv_cnt */
			unsigned int wp3_wv_cnt:4;	/* wp3_wv_cnt */
		} s;
	} reg_ux_status_report_9;

	union			/* 0x0098, */
	{
		unsigned int dwValue;	/* wp0 lsm remainder size */
		struct {
			unsigned int wp0_lsm_prv_rem:9;	/* wp0_lsm_prv_rem */
			/* default: 9'd64 */
			unsigned int reserved17:1;
			unsigned int wp0_lsm_grp_rem:9;	/* wp0_lsm_grp_rem */
			/* default: 9'd64 */
			unsigned int reserved18:1;
			unsigned int wp0_lsm_glb_rem:9;	/* wp0_lsm_glb_rem */
			/* default: 9'd0 */
		} s;
	} reg_ux_status_report_10;

	union			/* 0x009c, */
	{
		unsigned int dwValue;	/* wp0 vrf/crf remainder size */
		struct {
			unsigned int wp0_vrf_rem:9;	/* wp0_vrf_rem */
			/* default: 9'd256 */
			unsigned int reserved19:1;
			unsigned int wp0_crf_rem:9;	/* wp0_crf_rem */
			/* default: 9'd128 */
		} s;
	} reg_ux_status_report_11;

	union			/* 0x00a0, */
	{
		unsigned int dwValue;	/* wp1 lsm remainder size */
		struct {
			unsigned int wp1_lsm_prv_rem:9;	/* wp1_lsm_prv_rem */
			/* default: 9'd64 */
			unsigned int reserved20:1;
			unsigned int wp1_lsm_grp_rem:9;	/* wp1_lsm_grp_rem */
			/* default: 9'd64 */
			unsigned int reserved21:1;
			unsigned int wp1_lsm_glb_rem:9;	/* wp1_lsm_glb_rem */
			/* default: 9'd0 */
		} s;
	} reg_ux_status_report_12;

	union			/* 0x00a4, */
	{
		unsigned int dwValue;	/* wp1 vrf/crf remainder size */
		struct {
			unsigned int wp1_vrf_rem:9;	/* wp1_vrf_rem */
			/* default: 9'd256 */
			unsigned int reserved22:1;
			unsigned int wp1_crf_rem:9;	/* wp1_crf_rem */
			/* default: 9'd128 */
		} s;
	} reg_ux_status_report_13;

	union			/* 0x00a8, */
	{
		unsigned int dwValue;	/* wp2 lsm remainder size */
		struct {
			unsigned int wp2_lsm_prv_rem:9;	/* wp2_lsm_prv_rem */
			/* default: 9'd64 */
			unsigned int reserved23:1;
			unsigned int wp2_lsm_grp_rem:9;	/* wp2_lsm_grp_rem */
			/* default: 9'd64 */
			unsigned int reserved24:1;
			unsigned int wp2_lsm_glb_rem:9;	/* wp2_lsm_glb_rem */
			/* default: 9'd0 */
		} s;
	} reg_ux_status_report_14;

	union			/* 0x00ac, */
	{
		unsigned int dwValue;	/* wp2 vrf/crf remainder size */
		struct {
			unsigned int wp2_vrf_rem:9;	/* wp2_vrf_rem */
			/* default: 9'd256 */
			unsigned int reserved25:1;
			unsigned int wp2_crf_rem:9;	/* wp2_crf_rem */
			/* default: 9'd128 */
		} s;
	} reg_ux_status_report_15;

	union			/* 0x00b0, */
	{
		unsigned int dwValue;	/* wp3 lsm remainder size */
		struct {
			unsigned int wp3_lsm_prv_rem:9;	/* wp3_lsm_prv_rem */
			/* default: 9'd64 */
			unsigned int reserved26:1;
			unsigned int wp3_lsm_grp_rem:9;	/* wp3_lsm_grp_rem */
			/* default: 9'd64 */
			unsigned int reserved27:1;
			unsigned int wp3_lsm_glb_rem:9;	/* wp3_lsm_glb_rem */
			/* default: 9'd0 */
		} s;
	} reg_ux_status_report_16;

	union			/* 0x00b4, */
	{
		unsigned int dwValue;	/* wp3 vrf/crf remainder size */
		struct {
			unsigned int wp3_vrf_rem:9;	/* wp3_vrf_rem */
			/* default: 9'd256 */
			unsigned int reserved28:1;
			unsigned int wp3_crf_rem:9;	/* wp3_crf_rem */
			/* default: 9'd128 */
		} s;
	} reg_ux_status_report_17;

	union			/* 0x00b8, */
	{
		unsigned int dwValue;	/* isc frame mask */
		struct {
			unsigned int isc_gmask0:32;	/* ISC frame mask 0 */
			/* default: 32'd0 */
		} s;
	} reg_isc_frame_control_0;

	union			/* 0x00bc, */
	{
		unsigned int dwValue;	/* isc frame mask */
		struct {
			unsigned int isc_gmask1:32;	/* ISC frame mask 1 */
			/* default: 32'd0 */
		} s;
	} reg_isc_frame_control_1;

	union			/* 0x00c0, */
	{
		unsigned int dwValue;	/* isc frame mask */
		struct {
			unsigned int isc_gmask2:32;	/* ISC frame mask 2 */
			/* default: 32'd0 */
		} s;
	} reg_isc_frame_control_2;

	union			/* 0x00c4, */
	{
		unsigned int dwValue;	/* isc frame mask */
		struct {
			unsigned int isc_gmask3:32;	/* ISC frame mask 3 */
			/* default: 32'd0 */
		} s;
	} reg_isc_frame_control_3;

} SapphireUxCmn;


#endif /* _SAPPHIRE_UX_CMN_H_ */
