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

#ifndef _SAPPHIRE_UX_DWP_H_
#define _SAPPHIRE_UX_DWP_H_

/* Last updated on 2014/08/20, 14:32. The version : 459 */




/**********************************************************************************************************/
/* syscall interrupt info */
#define REG_UX_DWP_SYSCALL_INFO_0      0x0000	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SYSCALL_WP0_INFO    0x00000fff	/* [default:][perf:] syscall wp0 info
								(active is in syscall_wait state) */
#define SFT_UX_DWP_SYSCALL_WP0_INFO    0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SYSCALL_WP2_INFO    0x0fff0000	/* [default:][perf:] syscall wp2 info
								(active is in syscall_wait state) */
#define SFT_UX_DWP_SYSCALL_WP2_INFO    16

/**********************************************************************************************************/
/* syscall interrupt id info[0] */
#define REG_UX_DWP_SYSCALL_INFO_1      0x0004	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SYSCALL_BIT0_INFO_ID 0x00ffffff	/* [default:][perf:] syscall bit0 id info(indicate more syscall
								information, cid or instruction sys_id) */
#define SFT_UX_DWP_SYSCALL_BIT0_INFO_ID 0

/**********************************************************************************************************/
/* syscall interrupt id info[1] */
#define REG_UX_DWP_SYSCALL_INFO_2      0x0008	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SYSCALL_BIT1_INFO_ID 0x00ffffff	/* [default:][perf:] syscall bit1 id info (indicate more syscall
								information, cid or instruction sys_id) */
#define SFT_UX_DWP_SYSCALL_BIT1_INFO_ID 0

/**********************************************************************************************************/
/* syscall interrupt id info[2] */
#define REG_UX_DWP_SYSCALL_INFO_3      0x000c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SYSCALL_BIT2_INFO_ID 0x00ffffff	/* [default:][perf:] syscall bit2 id info (indicate more syscall
								information, cid or instruction sys_id) */
#define SFT_UX_DWP_SYSCALL_BIT2_INFO_ID 0

/**********************************************************************************************************/
/* barrier interrupt info */
#define REG_UX_DWP_BARRIER_INFO_3      0x0010	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_BARRIER_WP0_INFO    0x00000fff	/* [default:][perf:] barrier wp0 info */
#define SFT_UX_DWP_BARRIER_WP0_INFO    0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_BARRIER_WP2_INFO    0x0fff0000	/* [default:][perf:] barrier wp2 info */
#define SFT_UX_DWP_BARRIER_WP2_INFO    16

/**********************************************************************************************************/
/* Error detection for VPU */
#define REG_UX_DWP_ERROR_DETECTION     0x0014	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VPU_DET_OVERFLOW_EN 0x00000001	/* [default:1'b0][perf:] vpu detect overflow enable */
#define SFT_UX_DWP_VPU_DET_OVERFLOW_EN 0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VPU_DET_UNDERFLOW_EN 0x00000002	/* [default:1'b0][perf:] vpu detect underflow enable */
#define SFT_UX_DWP_VPU_DET_UNDERFLOW_EN 1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VPU_DET_NAN_EN      0x00000004	/* [default:1'b0][perf:] vpu detect nan enable */
#define SFT_UX_DWP_VPU_DET_NAN_EN      2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VPU_DET_SUBNORMAL_EN 0x00000008	/* [default:1'b0][perf:] vpu detect subnormal enable */
#define SFT_UX_DWP_VPU_DET_SUBNORMAL_EN 3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VPRF_DET_ADDR_EN    0x00000010 /* [default:1'b0][perf:] vprf detect wrong address access enable */
#define SFT_UX_DWP_VPRF_DET_ADDR_EN    4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VPRF_COR_ADDR_EN    0x00000020 /* [default:1'b0][perf:] vprf correct wrong address write enable */
#define SFT_UX_DWP_VPRF_COR_ADDR_EN    5
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VPU_DET_CLEAR       0x00000100	/* [default:1'b0][perf:] vpu/vprf detect clear status */
#define SFT_UX_DWP_VPU_DET_CLEAR       8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_FTZ_RND_ORDER       0x00010000 /* [default:1'b0][perf:] vp alu FTZ/RND operation order
							([0]: RND->FTZ(for INTEL) [1]: FTZ->RND(for OpenCL & ARM) ) */
#define SFT_UX_DWP_FTZ_RND_ORDER       16

/**********************************************************************************************************/
/* interrupt status A */
#define REG_UX_DWP_INTP_STATUS_0       0x0018	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP0_INTP_TYPE       0x0000000f	/* [default:][perf:] wave pool 0 interrupt type
								{subnormal,nan,underflow,overflow} */
#define SFT_UX_DWP_WP0_INTP_TYPE       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP0_INTP_WID        0x000000f0	/* [default:][perf:] wave pool 0 interrupt wid */
#define SFT_UX_DWP_WP0_INTP_WID        4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP0_INTP_SPC        0x00000f00	/* [default:][perf:] wave pool 0 interrupt pc[3:0] */
#define SFT_UX_DWP_WP0_INTP_SPC        8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP0_INTP_ERR_FLAG   0x0000f000	/* [default:][perf:] wave pool 0 interrupt error flag */
#define SFT_UX_DWP_WP0_INTP_ERR_FLAG   12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP2_INTP_TYPE       0x000f0000	/* [default:][perf:] wave pool 2 interrupt type
								{subnormal,nan,underflow,overflow} */
#define SFT_UX_DWP_WP2_INTP_TYPE       16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP2_INTP_WID        0x00f00000	/* [default:][perf:] wave pool 2 interrupt wid */
#define SFT_UX_DWP_WP2_INTP_WID        20
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP2_INTP_SPC        0x0f000000	/* [default:][perf:] wave pool 2 interrupt pc[3:0] */
#define SFT_UX_DWP_WP2_INTP_SPC        24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP2_INTP_ERR_FLAG   0xf0000000	/* [default:][perf:] wave pool 2 interrupt error flag */
#define SFT_UX_DWP_WP2_INTP_ERR_FLAG   28

/**********************************************************************************************************/
/* interrupt status B */
#define REG_UX_DWP_INTP_STATUS_1       0x001c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP0_INTP_VPRF_ERR   0x00000001	/* [default:][perf:] wave pool 0 interrupt vprf error */
#define SFT_UX_DWP_WP0_INTP_VPRF_ERR   0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP0_INTP_VPRF_WID   0x000000f0	/* [default:][perf:] wave pool 0 interrupt vprf wid */
#define SFT_UX_DWP_WP0_INTP_VPRF_WID   4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP0_INTP_VPRF_ADR   0x0000ff00	/* [default:][perf:] wave pool 0 interrupt vprf adr */
#define SFT_UX_DWP_WP0_INTP_VPRF_ADR   8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP2_INTP_VPRF_ERR   0x00010000	/* [default:][perf:] wave pool 2 interrupt vprf error */
#define SFT_UX_DWP_WP2_INTP_VPRF_ERR   16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP2_INTP_VPRF_WID   0x00f00000	/* [default:][perf:] wave pool 2 interrupt vprf wid */
#define SFT_UX_DWP_WP2_INTP_VPRF_WID   20
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP2_INTP_VPRF_ADR   0xff000000	/* [default:][perf:] wave pool 2 interrupt vprf adr */
#define SFT_UX_DWP_WP2_INTP_VPRF_ADR   24

/**********************************************************************************************************/
/* advanced debug enable */
#define REG_UX_DWP_ADV_DEBUG           0x0020	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_ADVDBG_EN           0x00000001	/* [default:1'b0][perf:] advanced debug enable
								(stop in specific pc condition) */
#define SFT_UX_DWP_ADVDBG_EN           0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_ADVDBG_CLEAR        0x00000002	/* [default:1'b0][perf:] advanced debug clear stop status */
#define SFT_UX_DWP_ADVDBG_CLEAR        1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_ADVDBG_PC_NEXT      0x00000004	/* [default:1'b0][perf:] advanced debug stop for next pc
								value by one pulse (only valid after stop condition) */
#define SFT_UX_DWP_ADVDBG_PC_NEXT      2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_ADVDBG_PC_TYPE      0x00000010	/* [default:1'b0][perf:] advanced debug pc type
								(0: pc 1: issued pc number) */
#define SFT_UX_DWP_ADVDBG_PC_TYPE      4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_DEBUG_EN            0x00000080	/* [default:1'b0][perf:] Debug Enable for data latch
								(read/dump eu_status_data should active this bit) */
#define SFT_UX_DWP_DEBUG_EN            7
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_ADVDBG_PC_VAL       0xffffff00	/* [default:24'd0][perf:] advanced debug pc value
						(stop when cur_pc==reg_pc(pc_type=0)/cur_pc>=reg_pc(pc_type=1)) */
#define SFT_UX_DWP_ADVDBG_PC_VAL       8

/**********************************************************************************************************/
/* wave information control */
#define REG_UX_DWP_WAVE_INFO_CONTROL   0x0024	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WAVE_INFO_EN        0x00000001	/* [default:1'b0][perf:] wave info wave-pool enable
								(some debug counter would be active) */
#define SFT_UX_DWP_WAVE_INFO_EN        0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WAVE_INFO_RSV       0xfffffffe	/* [default:31'd0][perf:] wave info wave-pool reserve */
#define SFT_UX_DWP_WAVE_INFO_RSV       1

/**********************************************************************************************************/
/* vprf access control */
#define REG_UX_DWP_VPRF_ACCESS_CONTROL_0 0x0028	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_EN          0x00000001	/* [default:1'b0][perf:] vprf debug enable */
#define SFT_UX_DWP_VRF_DBG_EN          0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_FIRE        0x00000002	/* [default:1'b0][perf:] vprf debug fire (1T pulse) */
#define SFT_UX_DWP_VRF_DBG_FIRE        1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_RW          0x00000004	/* [default:1'b0][perf:] vprf debug r/w (0:read 1:write) */
#define SFT_UX_DWP_VRF_DBG_RW          2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_RSV0        0x00000008	/* [default:1'b0][perf:] vprf debug reserve */
#define SFT_UX_DWP_VRF_DBG_RSV0        3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_WPSEL       0x00000030	/* [default:2'd0][perf:] vprf debug wave pool
								select (0:1st/2:2nd) */
#define SFT_UX_DWP_VRF_DBG_WPSEL       4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_VEC         0x000000c0	/* [default:2'd0][perf:] vprf debug vector mode */
#define SFT_UX_DWP_VRF_DBG_VEC         6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_WID         0x00000f00	/* [default:4'd0][perf:] vprf debug wave id (0~11) */
#define SFT_UX_DWP_VRF_DBG_WID         8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_RSV1        0x0000f000	/* [default:4'd0][perf:] vprf debug reserve */
#define SFT_UX_DWP_VRF_DBG_RSV1        12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_VPADR       0x00ff0000	/* [default:8'd0][perf:] vprf debug vp address */
#define SFT_UX_DWP_VRF_DBG_VPADR       16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_SRAMSEL     0x0f000000	/* [default:4'd0][perf:] vprf debug sram
								select ([0~3]bk0 [4~7]bk1 [8~11]bk2 [12~15]bk3) */
#define SFT_UX_DWP_VRF_DBG_SRAMSEL     24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_LANESEL     0x30000000	/* [default:2'd0][perf:] vprf debug lane select */
#define SFT_UX_DWP_VRF_DBG_LANESEL     28
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_RDVLD       0x80000000	/* [default:][perf:] vprf debug read valid */
#define SFT_UX_DWP_VRF_DBG_RDVLD       31

/**********************************************************************************************************/
/* vprf access control */
#define REG_UX_DWP_VPRF_ACCESS_CONTROL_1 0x002c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_WRDAT       0xffffffff	/* [default:32'd0][perf:] vprf debug write data */
#define SFT_UX_DWP_VRF_DBG_WRDAT       0

/**********************************************************************************************************/
/* vprf access control */
#define REG_UX_DWP_VPRF_ACCESS_CONTROL_2 0x0030	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VRF_DBG_RDDAT       0xffffffff	/* [default:][perf:] vprf debug read data */
#define SFT_UX_DWP_VRF_DBG_RDDAT       0

/**********************************************************************************************************/
/* lsm access control */
#define REG_UX_DWP_LSM_ACCESS_CONTROL_0 0x0034	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_EN          0x00000001	/* [default:1'b0][perf:] smm debug enable */
#define SFT_UX_DWP_SMM_DBG_EN          0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_FIRE        0x00000002	/* [default:1'b0][perf:] smm debug fire (1T pulse) */
#define SFT_UX_DWP_SMM_DBG_FIRE        1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_DUMP_MD     0x0000001c	/* [default:3'd0][perf:] smm debug dump mode
								(0: private segment, 1: group segment,
								2: private base of wave, 3: group base of wave,
								4: share memory raw data dump) */
#define SFT_UX_DWP_SMM_DBG_DUMP_MD     2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_WPSEL       0x00000020	/* [default:1'b0][perf:] smm debug wp select */
#define SFT_UX_DWP_SMM_DBG_WPSEL       5
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_RW          0x00000040	/* [default:1'b0][perf:] smm debug r/w */
#define SFT_UX_DWP_SMM_DBG_RW          6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_RSV1        0x00000080	/* [default:1'b0][perf:] smm debug reserve */
#define SFT_UX_DWP_SMM_DBG_RSV1        7
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_WID         0x00000f00	/* [default:4'd0][perf:] smm debug wave id */
#define SFT_UX_DWP_SMM_DBG_WID         8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_ADDR        0x3fff0000	/* [default:14'd0][perf:] smm debug address */
#define SFT_UX_DWP_SMM_DBG_ADDR        16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_RD_DATARDY  0x40000000	/* [default:][perf:] smm debug read data ready */
#define SFT_UX_DWP_SMM_DBG_RD_DATARDY  30
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_WR_DONE     0x80000000	/* [default:][perf:] smm debug write done */
#define SFT_UX_DWP_SMM_DBG_WR_DONE     31

/**********************************************************************************************************/
/* lsm access control */
#define REG_UX_DWP_LSM_ACCESS_CONTROL_1 0x0038	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_WR_DATA     0xffffffff	/* [default:32'd0][perf:] smm debug write data */
#define SFT_UX_DWP_SMM_DBG_WR_DATA     0

/**********************************************************************************************************/
/* lsm access control */
#define REG_UX_DWP_LSM_ACCESS_CONTROL_2 0x003c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_SMM_DBG_RD_DATA     0xffffffff	/* [default:][perf:] smm debug read data */
#define SFT_UX_DWP_SMM_DBG_RD_DATA     0

/**********************************************************************************************************/
/* eu status report A */
#define REG_UX_DWP_EU_STATUS_REPORT_0  0x0040	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_EU_STATUS_ID_A      0x0000000f	/* [default:4'd0][perf:] eu status id
								(0: sfu 1:ifb 2:vs0r 3:vs1r 4:fsr 8:vrf_wp0
								9:vrf_wp2 10:vppipe_wp0 11:vppipe_wp2) */
#define SFT_UX_DWP_EU_STATUS_ID_A      0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_EU_STATUS_SEL_A     0xffff0000	/* [default:16'd0][perf:] eu status select */
#define SFT_UX_DWP_EU_STATUS_SEL_A     16

/**********************************************************************************************************/
/* eu status report A */
#define REG_UX_DWP_EU_STATUS_REPORT_1  0x0044	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_EU_STATUS_DATA_A    0xffffffff	/* [default:][perf:] eu status data */
#define SFT_UX_DWP_EU_STATUS_DATA_A    0

/**********************************************************************************************************/
/* eu status report B */
#define REG_UX_DWP_EU_STATUS_REPORT_2  0x0048	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_EU_STATUS_ID_B      0x0000000f	/* [default:4'd0][perf:] eu status id (0: sfu 1:ifb 2:vs0r
								3:vs1r 4:fsr 8:vrf_wp0 9:vrf_wp2 10:vppipe_wp0
								11:vppipe_wp2) */
#define SFT_UX_DWP_EU_STATUS_ID_B      0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_EU_STATUS_SEL_B     0xffff0000	/* [default:16'd0][perf:] eu status select */
#define SFT_UX_DWP_EU_STATUS_SEL_B     16

/**********************************************************************************************************/
/* eu status report B */
#define REG_UX_DWP_EU_STATUS_REPORT_3  0x004c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_EU_STATUS_DATA_B    0xffffffff	/* [default:][perf:] eu status data */
#define SFT_UX_DWP_EU_STATUS_DATA_B    0

/**********************************************************************************************************/
/* pm control bits */
#define REG_UX_DWP_PM_CONTROL_0        0x0050	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_DWP_PM_CTRL_A       0xffffffff	/* [default:32'd0][perf:] for dwp_pm_sel */
#define SFT_UX_DWP_DWP_PM_CTRL_A       0

/**********************************************************************************************************/
/* pm control bits */
#define REG_UX_DWP_PM_CONTROL_1        0x0054	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_DWP_PM_CTRL_B       0xffffffff	/* [default:32'd0][perf:] for dwp_debug_sel */
#define SFT_UX_DWP_DWP_PM_CTRL_B       0

/**********************************************************************************************************/
/* ux status report: retire fid */
#define REG_UX_DWP_UX_STATUS_REPORT_0  0x0058	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VS0R_CURR_FID       0x0000003f	/* [default:][perf:] vs0r_fid */
#define SFT_UX_DWP_VS0R_CURR_FID       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VS1R_CURR_FID       0x00003f00	/* [default:][perf:] vs1r_fid */
#define SFT_UX_DWP_VS1R_CURR_FID       8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_FSR_CURR_FID        0x003f0000	/* [default:][perf:] fsr_fid */
#define SFT_UX_DWP_FSR_CURR_FID        16

/**********************************************************************************************************/
/* ux status report: retire fid */
#define REG_UX_DWP_UX_STATUS_REPORT_1  0x005c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VS0R_NEXT_FID       0x0000003f	/* [default:][perf:] vs0r_fid */
#define SFT_UX_DWP_VS0R_NEXT_FID       0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_VS1R_NEXT_FID       0x00003f00	/* [default:][perf:] vs1r_fid */
#define SFT_UX_DWP_VS1R_NEXT_FID       8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_FSR_NEXT_FID        0x003f0000	/* [default:][perf:] fsr_fid */
#define SFT_UX_DWP_FSR_NEXT_FID        16

/**********************************************************************************************************/
/* ux status report: busy status */
#define REG_UX_DWP_UX_STATUS_REPORT_2  0x0060	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP0_BUSY            0x00000fff	/* [default:][perf:] wp0_busy */
#define SFT_UX_DWP_WP0_BUSY            0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_WP2_BUSY            0x0fff0000	/* [default:][perf:] wp2_busy */
#define SFT_UX_DWP_WP2_BUSY            16

/**********************************************************************************************************/
/* ux status report: idle status */
#define REG_UX_DWP_UX_STATUS_REPORT_3  0x0064	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_UX_DWP_IDLE_STATUS         0x000000ff	/* [default:8'hff][perf:] {wp0_vpu,wp0_lsm,vp1_vpu,
								wp1_lsm,sfu,vs0r,vs1r,fsr} */
#define SFT_UX_DWP_IDLE_STATUS         0

/**********************************************************************************************************/


typedef struct {
	union			/* 0x0000, */
	{
		unsigned int dwValue;	/* syscall interrupt info */
		struct {
			unsigned int syscall_wp0_info:12;	/* syscall wp0 info (active is in syscall_wait state) */
			unsigned int reserved0:4;
			unsigned int syscall_wp2_info:12;	/* syscall wp2 info (active is in syscall_wait state) */
		} s;
	} reg_syscall_info_0;

	union			/* 0x0004, */
	{
		unsigned int dwValue;	/* syscall interrupt id info[0] */
		struct {
			unsigned int syscall_bit0_info_id:24;	/* syscall bit0 id info (indicate more syscall
									information, cid or instruction sys_id) */
		} s;
	} reg_syscall_info_1;

	union			/* 0x0008, */
	{
		unsigned int dwValue;	/* syscall interrupt id info[1] */
		struct {
			unsigned int syscall_bit1_info_id:24;	/* syscall bit1 id info (indicate more syscall
									information, cid or instruction sys_id) */
		} s;
	} reg_syscall_info_2;

	union			/* 0x000c, */
	{
		unsigned int dwValue;	/* syscall interrupt id info[2] */
		struct {
			unsigned int syscall_bit2_info_id:24;	/* syscall bit2 id info (indicate more syscall
									information, cid or instruction sys_id) */
		} s;
	} reg_syscall_info_3;

	union			/* 0x0010, */
	{
		unsigned int dwValue;	/* barrier interrupt info */
		struct {
			unsigned int barrier_wp0_info:12;	/* barrier wp0 info */
			unsigned int reserved1:4;
			unsigned int barrier_wp2_info:12;	/* barrier wp2 info */
		} s;
	} reg_barrier_info_3;

	union			/* 0x0014, */
	{
		unsigned int dwValue;	/* Error detection for VPU */
		struct {
			unsigned int vpu_det_overflow_en:1;	/* vpu detect overflow enable */
			/* default: 1'b0 */
			unsigned int vpu_det_underflow_en:1;	/* vpu detect underflow enable */
			/* default: 1'b0 */
			unsigned int vpu_det_nan_en:1;	/* vpu detect nan enable */
			/* default: 1'b0 */
			unsigned int vpu_det_subnormal_en:1;	/* vpu detect subnormal enable */
			/* default: 1'b0 */
			unsigned int vprf_det_addr_en:1;	/* vprf detect wrong address access enable */
			/* default: 1'b0 */
			unsigned int vprf_cor_addr_en:1;	/* vprf correct wrong address write enable */
			/* default: 1'b0 */
			unsigned int reserved2:2;
			unsigned int vpu_det_clear:1;	/* vpu/vprf detect clear status */
			/* default: 1'b0 */
			unsigned int reserved3:7;
			unsigned int ftz_rnd_order:1;	/* vp alu FTZ/RND operation order ([0]: RND->FTZ(for INTEL)
								[1]: FTZ->RND(for OpenCL & ARM) ) */
			/* default: 1'b0 */
		} s;
	} reg_error_detection;

	union			/* 0x0018, */
	{
		unsigned int dwValue;	/* interrupt status A */
		struct {
			unsigned int wp0_intp_type:4;	/* wave pool 0 interrupt type
								{subnormal,nan,underflow,overflow} */
			unsigned int wp0_intp_wid:4;	/* wave pool 0 interrupt wid */
			unsigned int wp0_intp_spc:4;	/* wave pool 0 interrupt pc[3:0] */
			unsigned int wp0_intp_err_flag:4;	/* wave pool 0 interrupt error flag */
			unsigned int wp2_intp_type:4;	/* wave pool 2 interrupt type
								{subnormal,nan,underflow,overflow} */
			unsigned int wp2_intp_wid:4;	/* wave pool 2 interrupt wid */
			unsigned int wp2_intp_spc:4;	/* wave pool 2 interrupt pc[3:0] */
			unsigned int wp2_intp_err_flag:4;	/* wave pool 2 interrupt error flag */
		} s;
	} reg_intp_status_0;

	union			/* 0x001c, */
	{
		unsigned int dwValue;	/* interrupt status B */
		struct {
			unsigned int wp0_intp_vprf_err:1;	/* wave pool 0 interrupt vprf error */
			unsigned int reserved4:3;
			unsigned int wp0_intp_vprf_wid:4;	/* wave pool 0 interrupt vprf wid */
			unsigned int wp0_intp_vprf_adr:8;	/* wave pool 0 interrupt vprf adr */
			unsigned int wp2_intp_vprf_err:1;	/* wave pool 2 interrupt vprf error */
			unsigned int reserved5:3;
			unsigned int wp2_intp_vprf_wid:4;	/* wave pool 2 interrupt vprf wid */
			unsigned int wp2_intp_vprf_adr:8;	/* wave pool 2 interrupt vprf adr */
		} s;
	} reg_intp_status_1;

	union			/* 0x0020, */
	{
		unsigned int dwValue;	/* advanced debug enable */
		struct {
			unsigned int advdbg_en:1;	/* advanced debug enable (stop in specific pc condition) */
			/* default: 1'b0 */
			unsigned int advdbg_clear:1;	/* advanced debug clear stop status */
			/* default: 1'b0 */
			unsigned int advdbg_pc_next:1;	/* advanced debug stop for next pc value by one pulse
								(only valid after stop condition) */
			/* default: 1'b0 */
			unsigned int reserved6:1;
			unsigned int advdbg_pc_type:1;	/* advanced debug pc type (0: pc 1: issued pc number) */
			/* default: 1'b0 */
			unsigned int reserved7:2;
			unsigned int debug_en:1;	/* Debug Enable for data latch
								(read/dump eu_status_data should active this bit) */
			/* default: 1'b0 */
			unsigned int advdbg_pc_val:24;	/* advanced debug pc value
						(stop when cur_pc==reg_pc(pc_type=0)/cur_pc>=reg_pc(pc_type=1)) */
			/* default: 24'd0 */
		} s;
	} reg_adv_debug;

	union			/* 0x0024, */
	{
		unsigned int dwValue;	/* wave information control */
		struct {
			unsigned int wave_info_en:1;	/* wave info wave-pool enable
							(some debug counter would be active) */
			/* default: 1'b0 */
			unsigned int wave_info_rsv:31;	/* wave info wave-pool reserve */
			/* default: 31'd0 */
		} s;
	} reg_wave_info_control;

	union			/* 0x0028, */
	{
		unsigned int dwValue;	/* vprf access control */
		struct {
			unsigned int vrf_dbg_en:1;	/* vprf debug enable */
			/* default: 1'b0 */
			unsigned int vrf_dbg_fire:1;	/* vprf debug fire (1T pulse) */
			/* default: 1'b0 */
			unsigned int vrf_dbg_rw:1;	/* vprf debug r/w (0:read 1:write) */
			/* default: 1'b0 */
			unsigned int vrf_dbg_rsv0:1;	/* vprf debug reserve */
			/* default: 1'b0 */
			unsigned int vrf_dbg_wpsel:2;	/* vprf debug wave pool select (0:1st/2:2nd) */
			/* default: 2'd0 */
			unsigned int vrf_dbg_vec:2;	/* vprf debug vector mode */
			/* default: 2'd0 */
			unsigned int vrf_dbg_wid:4;	/* vprf debug wave id (0~11) */
			/* default: 4'd0 */
			unsigned int vrf_dbg_rsv1:4;	/* vprf debug reserve */
			/* default: 4'd0 */
			unsigned int vrf_dbg_vpadr:8;	/* vprf debug vp address */
			/* default: 8'd0 */
			unsigned int vrf_dbg_sramsel:4;	/* vprf debug sram select
								([0~3]bk0 [4~7]bk1 [8~11]bk2 [12~15]bk3) */
			/* default: 4'd0 */
			unsigned int vrf_dbg_lanesel:2;	/* vprf debug lane select */
			/* default: 2'd0 */
			unsigned int reserved8:1;
			unsigned int vrf_dbg_rdvld:1;	/* vprf debug read valid */
		} s;
	} reg_vprf_access_control_0;

	union			/* 0x002c, */
	{
		unsigned int dwValue;	/* vprf access control */
		struct {
			unsigned int vrf_dbg_wrdat:32;	/* vprf debug write data */
			/* default: 32'd0 */
		} s;
	} reg_vprf_access_control_1;

	union			/* 0x0030, */
	{
		unsigned int dwValue;	/* vprf access control */
		struct {
			unsigned int vrf_dbg_rddat:32;	/* vprf debug read data */
		} s;
	} reg_vprf_access_control_2;

	union			/* 0x0034, */
	{
		unsigned int dwValue;	/* lsm access control */
		struct {
			unsigned int smm_dbg_en:1;	/* smm debug enable */
			/* default: 1'b0 */
			unsigned int smm_dbg_fire:1;	/* smm debug fire (1T pulse) */
			/* default: 1'b0 */
			unsigned int smm_dbg_dump_md:3;	/* smm debug dump mode
								(0: private segment, 1: group segment,
								2: private base of wave, 3: group base of wave,
								4: share memory raw data dump) */
			/* default: 3'd0 */
			unsigned int smm_dbg_wpsel:1;	/* smm debug wp select */
			/* default: 1'b0 */
			unsigned int smm_dbg_rw:1;	/* smm debug r/w */
			/* default: 1'b0 */
			unsigned int smm_dbg_rsv1:1;	/* smm debug reserve */
			/* default: 1'b0 */
			unsigned int smm_dbg_wid:4;	/* smm debug wave id */
			/* default: 4'd0 */
			unsigned int reserved9:4;
			unsigned int smm_dbg_addr:14;	/* smm debug address */
			/* default: 14'd0 */
			unsigned int smm_dbg_rd_datardy:1;	/* smm debug read data ready */
			unsigned int smm_dbg_wr_done:1;	/* smm debug write done */
		} s;
	} reg_lsm_access_control_0;

	union			/* 0x0038, */
	{
		unsigned int dwValue;	/* lsm access control */
		struct {
			unsigned int smm_dbg_wr_data:32;	/* smm debug write data */
			/* default: 32'd0 */
		} s;
	} reg_lsm_access_control_1;

	union			/* 0x003c, */
	{
		unsigned int dwValue;	/* lsm access control */
		struct {
			unsigned int smm_dbg_rd_data:32;	/* smm debug read data */
		} s;
	} reg_lsm_access_control_2;

	union			/* 0x0040, */
	{
		unsigned int dwValue;	/* eu status report A */
		struct {
			unsigned int eu_status_id_a:4;	/* eu status id (0: sfu 1:ifb 2:vs0r 3:vs1r 4:fsr
							8:vrf_wp0 9:vrf_wp2 10:vppipe_wp0 11:vppipe_wp2) */
			/* default: 4'd0 */
			unsigned int reserved10:12;
			unsigned int eu_status_sel_a:16;	/* eu status select */
			/* default: 16'd0 */
		} s;
	} reg_eu_status_report_0;

	union			/* 0x0044, */
	{
		unsigned int dwValue;	/* eu status report A */
		struct {
			unsigned int eu_status_data_a:32;	/* eu status data */
		} s;
	} reg_eu_status_report_1;

	union			/* 0x0048, */
	{
		unsigned int dwValue;	/* eu status report B */
		struct {
			unsigned int eu_status_id_b:4;	/* eu status id (0: sfu 1:ifb 2:vs0r 3:vs1r 4:fsr
							8:vrf_wp0 9:vrf_wp2 10:vppipe_wp0 11:vppipe_wp2) */
			/* default: 4'd0 */
			unsigned int reserved11:12;
			unsigned int eu_status_sel_b:16;	/* eu status select */
			/* default: 16'd0 */
		} s;
	} reg_eu_status_report_2;

	union			/* 0x004c, */
	{
		unsigned int dwValue;	/* eu status report B */
		struct {
			unsigned int eu_status_data_b:32;	/* eu status data */
		} s;
	} reg_eu_status_report_3;

	union			/* 0x0050, */
	{
		unsigned int dwValue;	/* pm control bits */
		struct {
			unsigned int dwp_pm_ctrl_a:32;	/* for dwp_pm_sel */
			/* default: 32'd0 */
		} s;
	} reg_pm_control_0;

	union			/* 0x0054, */
	{
		unsigned int dwValue;	/* pm control bits */
		struct {
			unsigned int dwp_pm_ctrl_b:32;	/* for dwp_debug_sel */
			/* default: 32'd0 */
		} s;
	} reg_pm_control_1;

	union			/* 0x0058, */
	{
		unsigned int dwValue;	/* ux status report: retire fid */
		struct {
			unsigned int vs0r_curr_fid:6;	/* vs0r_fid */
			unsigned int reserved12:2;
			unsigned int vs1r_curr_fid:6;	/* vs1r_fid */
			unsigned int reserved13:2;
			unsigned int fsr_curr_fid:6;	/* fsr_fid */
		} s;
	} reg_ux_status_report_0;

	union			/* 0x005c, */
	{
		unsigned int dwValue;	/* ux status report: retire fid */
		struct {
			unsigned int vs0r_next_fid:6;	/* vs0r_fid */
			unsigned int reserved14:2;
			unsigned int vs1r_next_fid:6;	/* vs1r_fid */
			unsigned int reserved15:2;
			unsigned int fsr_next_fid:6;	/* fsr_fid */
		} s;
	} reg_ux_status_report_1;

	union			/* 0x0060, */
	{
		unsigned int dwValue;	/* ux status report: busy status */
		struct {
			unsigned int wp0_busy:12;	/* wp0_busy */
			unsigned int reserved16:4;
			unsigned int wp2_busy:12;	/* wp2_busy */
		} s;
	} reg_ux_status_report_2;

	union			/* 0x0064, */
	{
		unsigned int dwValue;	/* ux status report: idle status */
		struct {
			unsigned int idle_status:8;	/* {wp0_vpu,wp0_lsm,vp1_vpu,wp1_lsm,sfu,vs0r,vs1r,fsr} */
			/* default: 8'hff */
		} s;
	} reg_ux_status_report_3;

} SapphireUxDwp;


#endif/* _SAPPHIRE_UX_DWP_H_ */
