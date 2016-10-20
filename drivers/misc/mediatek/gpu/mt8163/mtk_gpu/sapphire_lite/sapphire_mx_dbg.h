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

#ifndef _SAPPHIRE_MX_DBG_H_
#define _SAPPHIRE_MX_DBG_H_

/* Last updated on 2014/08/20, 14:32. The version : 459 */




/**********************************************************************************************************/
/* mx debug raddr port */
#define REG_MX_DBG_RADDR               0x0000	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX_DBG_RADDR        0xffffffff	/* [default:32'b0][perf:] MX debug read address */
						  /* [31:24] MX submodule selection */
						  /* 0: MB */
						  /* 1: LT */
						  /* 2: LD */
						  /* 3: VC */
						  /* 4: CU */
						  /* 5: AF */
						  /* 6: AM */
						  /* [23:0] submodule internal address */
#define SFT_MX_DBG_MX_DBG_RADDR        0

/**********************************************************************************************************/
/* mx debug waddr port */
#define REG_MX_DBG_WADDR               0x0004	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX_DBG_WADDR        0xffffffff	/* [default:32'b0][perf:] MX debug write address */
						  /* [31:24] MX submodule selection */
						  /* 0: MB */
						  /* 1: LT */
						  /* 2: LD */
						  /* 3: VC */
						  /* 4: CU */
						  /* 5: AF */
						  /* 6: AM */
						  /* [23:0] submodule internal address */
#define SFT_MX_DBG_MX_DBG_WADDR        0

/**********************************************************************************************************/
/* mx debug rdata port */
#define REG_MX_DBG_RDATA               0x0008	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX_DBG_RDATA        0xffffffff	/* [default:][perf:] MX debug read data */
#define SFT_MX_DBG_MX_DBG_RDATA        0

/**********************************************************************************************************/
/* mx debug wdata port */
#define REG_MX_DBG_WDATA               0x000c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX_DBG_WDATA        0xffffffff	/* [default:32'b0][perf:] MX debug write data */
#define SFT_MX_DBG_MX_DBG_WDATA        0

/**********************************************************************************************************/
/* monitor address */
#define REG_MX_DBG_MON_ADDR            0x0100	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MON_ADDR            0xfffffff0	/* [default:28'bx][perf:] monitor address */
#define SFT_MX_DBG_MON_ADDR            4

/**********************************************************************************************************/
/* debug frame id */
#define REG_MX_DBG_DEBUG_FRAME_ID      0x0104	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_DEBUG_FRAME_ID      0xffffffff	/* [default:32'bx][perf:] debug frame id */
#define SFT_MX_DBG_DEBUG_FRAME_ID      0

/**********************************************************************************************************/
/* monitor and break control */
#define REG_MX_DBG_MON_BRK_CTRL        0x0108	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MON_BRK_STAR_FRM    0x0000ffff	/* [default:16'bx][perf:] monitor and break
								enable start sub frame count */
#define SFT_MX_DBG_MON_BRK_STAR_FRM    0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MON_BRK_START_MODE  0x00070000	/* [default:3'bx][perf:] 000: immediately */
						  /* 001: break before Nth mx_invalidate pulse */
						  /* 010: break before Nth mx_l2_flush pulse */
						  /* 011: break before Nth mx_l3_flush pulse */
						  /* 100: break before Nth pa2mx_cu_clr pulse */
						  /* others: reserved */
#define SFT_MX_DBG_MON_BRK_START_MODE  16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MON_BRK_VP_EN       0x01000000	/* [default:1'b0][perf:] enable mx_frame_id/mx_debug_flag
								compare at vp phase */
#define SFT_MX_DBG_MON_BRK_VP_EN       24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MON_BRK_PP_EN       0x02000000	/* [default:1'b0][perf:] enable mx_frame_id/mx_debug_flag
								compare at pp phase */
#define SFT_MX_DBG_MON_BRK_PP_EN       25
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MON_BRK_FRAME_ID_EN 0x04000000	/* [default:1'b0][perf:] mx_frame_id compare enable */
#define SFT_MX_DBG_MON_BRK_FRAME_ID_EN 26
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MON_BRK_DEBUG_FLAG_EN 0x08000000	/* [default:1'b0][perf:] mx_debug_flag compare enable */
#define SFT_MX_DBG_MON_BRK_DEBUG_FLAG_EN 27
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_BRK_RELEASE         0x10000000	/* [default:1'b0][perf:] release previously
									matched break condition */
#define SFT_MX_DBG_BRK_RELEASE         28
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_BRK_ENABLE          0x40000000	/* [default:1'b0][perf:] break enable */
#define SFT_MX_DBG_BRK_ENABLE          30
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MON_ENABLE          0x80000000	/* [default:1'b0][perf:] monitor enable */
#define SFT_MX_DBG_MON_ENABLE          31

/**********************************************************************************************************/
/* command burst length */
#define REG_MX_DBG_CMD_ADDR            0x010c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_LEN             0x00000007	/* [default:3'bx][perf:] command burst length */
#define SFT_MX_DBG_CMD_LEN             0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_SEC             0x00000008	/* [default:1'bx][perf:] command section */
#define SFT_MX_DBG_CMD_SEC             3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_ADDR            0xfffffff0	/* [default:28'bx][perf:] command address */
#define SFT_MX_DBG_CMD_ADDR            4

/**********************************************************************************************************/
/* command write data[31:0] */
#define REG_MX_DBG_CMD_WDATA_0         0x0110	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_WDATA_0         0xffffffff	/* [default:32'bx][perf:] command write data[31:0] */
#define SFT_MX_DBG_CMD_WDATA_0         0

/**********************************************************************************************************/
/* command write data[63:32] */
#define REG_MX_DBG_CMD_WDATA_1         0x0114	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_WDATA_1         0xffffffff	/* [default:32'bx][perf:] command write data[63:32] */
#define SFT_MX_DBG_CMD_WDATA_1         0

/**********************************************************************************************************/
/* command write data[95:64] */
#define REG_MX_DBG_CMD_WDATA_2         0x0118	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_WDATA_2         0xffffffff	/* [default:32'bx][perf:] command write data[95:64] */
#define SFT_MX_DBG_CMD_WDATA_2         0

/**********************************************************************************************************/
/* command write data[127:96] */
#define REG_MX_DBG_CMD_WDATA_3         0x011c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_WDATA_3         0xffffffff	/* [default:32'bx][perf:] command write data[127:96] */
#define SFT_MX_DBG_CMD_WDATA_3         0

/**********************************************************************************************************/
/* command write byte enable */
#define REG_MX_DBG_CMD_WBE             0x0120	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_WBE             0x0000ffff	/* [default:16'bx][perf:] command write byte enable */
#define SFT_MX_DBG_CMD_WBE             0

/**********************************************************************************************************/
/* command control */
#define REG_MX_DBG_CMD_CTRL            0x0124	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_TYPE            0x00000003	/* [default:2'bx][perf:] command type */
						  /* 00: bypass read command from MB */
						  /* 01: bypass write command from MB */
						  /* 10: LTC read command from MB */
						  /* 11: LTC write command from MB */
#define SFT_MX_DBG_CMD_TYPE            0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_RDATA_SEL       0x00000070	/* [default:3'b0][perf:] read command return
								data selection in read burst */
						  /* 0: 1st 128-bit data */
						  /* 1: 2nd 128-bit data */
						  /* 2: 3rd 128-bit data */
						  /* 3: 4th 128-bit data */
						  /* 4: 5th 128-bit data */
						  /* 5: 6th 128-bit data */
						  /* 6: 7th 128-bit data */
						  /* 7: 8th 128-bit data */
#define SFT_MX_DBG_CMD_RDATA_SEL       4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_REQ_TRIGGER         0x80000000	/* [default:1'b0][perf:] request trigger */
						  /* 0: NOP */
						  /* 1: trigger request */
#define SFT_MX_DBG_REQ_TRIGGER         31

/**********************************************************************************************************/
/* command read data[31:0] */
#define REG_MX_DBG_CMD_RDATA_0         0x0128	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_RDATA_0         0xffffffff	/* [default:32'b0][perf:] command read data[31:0] */
#define SFT_MX_DBG_CMD_RDATA_0         0

/**********************************************************************************************************/
/* command read data[63:32] */
#define REG_MX_DBG_CMD_RDATA_1         0x012c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_RDATA_1         0xffffffff	/* [default:32'b0][perf:] command read data[63:32] */
#define SFT_MX_DBG_CMD_RDATA_1         0

/**********************************************************************************************************/
/* command read data[95:64] */
#define REG_MX_DBG_CMD_RDATA_2         0x0130	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_RDATA_2         0xffffffff	/* [default:32'b0][perf:] command read data[95:64] */
#define SFT_MX_DBG_CMD_RDATA_2         0

/**********************************************************************************************************/
/* command read data[127:96] */
#define REG_MX_DBG_CMD_RDATA_3         0x0134	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CMD_RDATA_3         0xffffffff	/* [default:32'b0][perf:] command read data[127:96] */
#define SFT_MX_DBG_CMD_RDATA_3         0

/**********************************************************************************************************/
/* dbg dump ldram */
#define REG_MX_DBG_INTERNAL_DUMP       0x0180	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_DBG_DUMP_LDRAM_ENABLE 0x00000001	/* [default:1'b0][perf:] internal dump enable */
#define SFT_MX_DBG_DBG_DUMP_LDRAM_ENABLE 0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_DBG_DUMP_LDRAM_FLUSH_EN 0x00000002	/* [default:1'b0][perf:] internal dump flush turn-on
								when engine flush & dbg_dump_ldram_flush=1 */
#define SFT_MX_DBG_DBG_DUMP_LDRAM_FLUSH_EN 1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_RESERVED29          0x0000003c	/* [default:][perf:] */
#define SFT_MX_DBG_RESERVED29          2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_DBG_DUMP_LDRAM_FLUSH_BASE 0xffffffc0	/* [default:26'hx][perf:] base address for flush to dram */
#define SFT_MX_DBG_DBG_DUMP_LDRAM_FLUSH_BASE 6

/**********************************************************************************************************/
/* mx debug mux selection */
#define REG_MX_DBG_DEBUG_MX_SEL        0x0184	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_DEBUG_MX_SEL        0xffffffff	/* [default:32'b0][perf:] mx debug mux selection */
#define SFT_MX_DBG_DEBUG_MX_SEL        0

/**********************************************************************************************************/
/* mx pm mux selection */
#define REG_MX_DBG_PM_MX_SEL           0x0188	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_PM_MX_SEL           0xffffffff	/* [default:32'b0][perf:] mx pm mux selection */
#define SFT_MX_DBG_PM_MX_SEL           0

/**********************************************************************************************************/
/* mx debug control */
#define REG_MX_DBG_DEBUG_CTRL          0x0ff0	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MB_DBG_CTRL         0x0000000f	/* [default:4'b0][perf:] MB debug control */
#define SFT_MX_DBG_MB_DBG_CTRL         0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_LT_DBG_CTRL         0x000000f0	/* [default:4'b0][perf:] LT debug control */
#define SFT_MX_DBG_LT_DBG_CTRL         4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_LD_DBG_CTRL         0x00000f00	/* [default:4'b0][perf:] LD debug control */
#define SFT_MX_DBG_LD_DBG_CTRL         8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_VC_DBG_CTRL         0x0000f000	/* [default:4'b0][perf:] VC debug control */
#define SFT_MX_DBG_VC_DBG_CTRL         12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CU_DBG_CTRL         0x000f0000	/* [default:4'b0][perf:] CU debug control */
#define SFT_MX_DBG_CU_DBG_CTRL         16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_AF_DBG_CTRL         0x00f00000	/* [default:4'b0][perf:] AF debug control */
#define SFT_MX_DBG_AF_DBG_CTRL         20
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX_APB_DBG_CK_ON    0x01000000	/* [default:1'b0][perf:] turn on MX APB debug related clock */
#define SFT_MX_DBG_MX_APB_DBG_CK_ON    24
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_RSV_DBG_CTRL        0x02000000	/* [default:1'b0][perf:] Reserved debug control */
#define SFT_MX_DBG_RSV_DBG_CTRL        25
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_PA2MX_CU_CLR        0x04000000	/* [default:1'b0][perf:] debug mode pa2mx_cu_clr trigger
								(write 1 then write 0 to trigger) */
#define SFT_MX_DBG_PA2MX_CU_CLR        26
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX_L3_FLUSH         0x08000000	/* [default:1'b0][perf:] debug mode mx_l3_flush trigger
								(write 1 then write 0 to trigger) */
#define SFT_MX_DBG_MX_L3_FLUSH         27
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX_L2_FLUSH         0x10000000	/* [default:1'b0][perf:] debug mode mx_l2_flush trigger
								(write 1 then write 0 to trigger) */
#define SFT_MX_DBG_MX_L2_FLUSH         28
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX_INVALIDATE       0x20000000	/* [default:1'b0][perf:] debug mode mx_invalidate trigger
								(write 1 then write 0 to trigger) */
#define SFT_MX_DBG_MX_INVALIDATE       29
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_RADDR_AUTO_INC      0x40000000	/* [default:1'b0][perf:] raddr auto increase mode enable */
#define SFT_MX_DBG_RADDR_AUTO_INC      30
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_WADDR_AUTO_INC      0x80000000	/* [default:1'b0][perf:] waddr auto increase mode enable */
#define SFT_MX_DBG_WADDR_AUTO_INC      31

/**********************************************************************************************************/
/* MX idle status */
#define REG_MX_DBG_MX_IDLE             0x0ff4	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MB_IDLE             0x00000001	/* [default:1'b1][perf:] MX and sub module idle status */
#define SFT_MX_DBG_MB_IDLE             0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_LT_IDLE             0x00000002	/* [default:1'b1][perf:] MX and sub module idle status */
#define SFT_MX_DBG_LT_IDLE             1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_LD_IDLE             0x00000004	/* [default:1'b1][perf:] MX and sub module idle status */
#define SFT_MX_DBG_LD_IDLE             2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_VC_IDLE             0x00000008	/* [default:1'b1][perf:] MX and sub module idle status */
#define SFT_MX_DBG_VC_IDLE             3
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_CU_IDLE             0x00000010	/* [default:1'b1][perf:] MX and sub module idle status */
#define SFT_MX_DBG_CU_IDLE             4
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_AF_IDLE             0x00000020	/* [default:1'b1][perf:] MX and sub module idle status */
#define SFT_MX_DBG_AF_IDLE             5
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX2MF_IDLE          0x00000040	/* [default:1'b1][perf:] MX and sub module idle status */
#define SFT_MX_DBG_MX2MF_IDLE          6
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX2MF_L2_ENGIDLE    0x00000080	/* [default:1'b1][perf:] MX and sub module idle status */
#define SFT_MX_DBG_MX2MF_L2_ENGIDLE    7
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX2MF_STALL         0x00000100	/* [default:1'b0][perf:] MX and sub module idle status */
#define SFT_MX_DBG_MX2MF_STALL         8
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_MX2MF_ALIVE         0x00000200	/* [default:1'b0][perf:] MX and sub module idle status */
#define SFT_MX_DBG_MX2MF_ALIVE         9
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_BREAK_MATCH         0x00000400	/* [default:1'b0][perf:] break conditiion matched status */
#define SFT_MX_DBG_BREAK_MATCH         10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_APB_CMD_BUSY        0x00000800	/* [default:1'b0][perf:] apb command busy status */
#define SFT_MX_DBG_APB_CMD_BUSY        11
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_HW_SUBFRAME_MATCH   0x00001000	/* [default:1'b0][perf:] monitor/break hw subframe
								count match status */
#define SFT_MX_DBG_HW_SUBFRAME_MATCH   12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_SW_FRAME_ID_MATCH   0x00002000	/* [default:1'b1][perf:] monitor/break sw frame_id/debug
								flag match status */
#define SFT_MX_DBG_SW_FRAME_ID_MATCH   13

/**********************************************************************************************************/
/* read error count clear */
#define REG_MX_DBG_R_ERR_CNT_CLR       0x0ff8	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_R_ERR_CNT_CLR       0x00000001	/* [default:][perf:] read error count clear */
#define SFT_MX_DBG_R_ERR_CNT_CLR       0

/**********************************************************************************************************/
/* write error count clear */
#define REG_MX_DBG_W_ERR_CNT_CLR       0x0ffc	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_MX_DBG_W_ERR_CNT_CLR       0x00000001	/* [default:][perf:] write error count clear */
#define SFT_MX_DBG_W_ERR_CNT_CLR       0

/**********************************************************************************************************/


typedef struct {
	union			/* 0x0000, */
	{
		unsigned int dwValue;	/* mx debug raddr port */
		struct {
			unsigned int mx_dbg_raddr:32;	/* MX debug read address */
			/* [31:24] MX submodule selection */
			/* 0: MB */
			/* 1: LT */
			/* 2: LD */
			/* 3: VC */
			/* 4: CU */
			/* 5: AF */
			/* 6: AM */
			/* [23:0] submodule internal address */
			/* default: 32'b0 */
		} s;
	} reg_raddr;

	union			/* 0x0004, */
	{
		unsigned int dwValue;	/* mx debug waddr port */
		struct {
			unsigned int mx_dbg_waddr:32;	/* MX debug write address */
			/* [31:24] MX submodule selection */
			/* 0: MB */
			/* 1: LT */
			/* 2: LD */
			/* 3: VC */
			/* 4: CU */
			/* 5: AF */
			/* 6: AM */
			/* [23:0] submodule internal address */
			/* default: 32'b0 */
		} s;
	} reg_waddr;

	union			/* 0x0008, */
	{
		unsigned int dwValue;	/* mx debug rdata port */
		struct {
			unsigned int mx_dbg_rdata:32;	/* MX debug read data */
		} s;
	} reg_rdata;

	union			/* 0x000c, */
	{
		unsigned int dwValue;	/* mx debug wdata port */
		struct {
			unsigned int mx_dbg_wdata:32;	/* MX debug write data */
			/* default: 32'b0 */
		} s;
	} reg_wdata;

	union			/* 0x0100, */
	{
		unsigned int dwValue;	/* monitor address */
		struct {
			unsigned int reserved0:4;
			unsigned int mon_addr:28;	/* monitor address */
			/* default: 28'bx */
		} s;
	} reg_mon_addr;

	union			/* 0x0104, */
	{
		unsigned int dwValue;	/* debug frame id */
		struct {
			unsigned int debug_frame_id:32;	/* debug frame id */
			/* default: 32'bx */
		} s;
	} reg_debug_frame_id;

	union			/* 0x0108, */
	{
		unsigned int dwValue;	/* monitor and break control */
		struct {
			unsigned int mon_brk_star_frm:16;	/* monitor and break enable start sub frame count */
			/* default: 16'bx */
			unsigned int mon_brk_start_mode:3;	/* 000: immediately */
			/* 001: break before Nth mx_invalidate pulse */
			/* 010: break before Nth mx_l2_flush pulse */
			/* 011: break before Nth mx_l3_flush pulse */
			/* 100: break before Nth pa2mx_cu_clr pulse */
			/* others: reserved */
			/* default: 3'bx */
			unsigned int reserved1:5;
			unsigned int mon_brk_vp_en:1;	/* enable mx_frame_id/mx_debug_flag compare at vp phase */
			/* default: 1'b0 */
			unsigned int mon_brk_pp_en:1;	/* enable mx_frame_id/mx_debug_flag compare at pp phase */
			/* default: 1'b0 */
			unsigned int mon_brk_frame_id_en:1;	/* mx_frame_id compare enable */
			/* default: 1'b0 */
			unsigned int mon_brk_debug_flag_en:1;	/* mx_debug_flag compare enable */
			/* default: 1'b0 */
			unsigned int brk_release:1;	/* release previously matched break condition */
			/* default: 1'b0 */
			unsigned int reserved2:1;
			unsigned int brk_enable:1;	/* break enable */
			/* default: 1'b0 */
			unsigned int mon_enable:1;	/* monitor enable */
			/* default: 1'b0 */
		} s;
	} reg_mon_brk_ctrl;

	union			/* 0x010c, */
	{
		unsigned int dwValue;	/* command burst length */
		struct {
			unsigned int cmd_len:3;	/* command burst length */
			/* default: 3'bx */
			unsigned int cmd_sec:1;	/* command section */
			/* default: 1'bx */
			unsigned int cmd_addr:28;	/* command address */
			/* default: 28'bx */
		} s;
	} reg_cmd_addr;

	union			/* 0x0110, */
	{
		unsigned int dwValue;	/* command write data[31:0] */
		struct {
			unsigned int cmd_wdata_0:32;	/* command write data[31:0] */
			/* default: 32'bx */
		} s;
	} reg_cmd_wdata_0;

	union			/* 0x0114, */
	{
		unsigned int dwValue;	/* command write data[63:32] */
		struct {
			unsigned int cmd_wdata_1:32;	/* command write data[63:32] */
			/* default: 32'bx */
		} s;
	} reg_cmd_wdata_1;

	union			/* 0x0118, */
	{
		unsigned int dwValue;	/* command write data[95:64] */
		struct {
			unsigned int cmd_wdata_2:32;	/* command write data[95:64] */
			/* default: 32'bx */
		} s;
	} reg_cmd_wdata_2;

	union			/* 0x011c, */
	{
		unsigned int dwValue;	/* command write data[127:96] */
		struct {
			unsigned int cmd_wdata_3:32;	/* command write data[127:96] */
			/* default: 32'bx */
		} s;
	} reg_cmd_wdata_3;

	union			/* 0x0120, */
	{
		unsigned int dwValue;	/* command write byte enable */
		struct {
			unsigned int cmd_wbe:16;	/* command write byte enable */
			/* default: 16'bx */
		} s;
	} reg_cmd_wbe;

	union			/* 0x0124, */
	{
		unsigned int dwValue;	/* command control */
		struct {
			unsigned int cmd_type:2;	/* command type */
			/* 00: bypass read command from MB */
			/* 01: bypass write command from MB */
			/* 10: LTC read command from MB */
			/* 11: LTC write command from MB */
			/* default: 2'bx */
			unsigned int reserved3:2;
			unsigned int cmd_rdata_sel:3;	/* read command return data selection in read burst */
			/* 0: 1st 128-bit data */
			/* 1: 2nd 128-bit data */
			/* 2: 3rd 128-bit data */
			/* 3: 4th 128-bit data */
			/* 4: 5th 128-bit data */
			/* 5: 6th 128-bit data */
			/* 6: 7th 128-bit data */
			/* 7: 8th 128-bit data */
			/* default: 3'b0 */
			unsigned int reserved4:24;
			unsigned int req_trigger:1;	/* request trigger */
			/* 0: NOP */
			/* 1: trigger request */
			/* default: 1'b0 */
		} s;
	} reg_cmd_ctrl;

	union			/* 0x0128, */
	{
		unsigned int dwValue;	/* command read data[31:0] */
		struct {
			unsigned int cmd_rdata_0:32;	/* command read data[31:0] */
			/* default: 32'b0 */
		} s;
	} reg_cmd_rdata_0;

	union			/* 0x012c, */
	{
		unsigned int dwValue;	/* command read data[63:32] */
		struct {
			unsigned int cmd_rdata_1:32;	/* command read data[63:32] */
			/* default: 32'b0 */
		} s;
	} reg_cmd_rdata_1;

	union			/* 0x0130, */
	{
		unsigned int dwValue;	/* command read data[95:64] */
		struct {
			unsigned int cmd_rdata_2:32;	/* command read data[95:64] */
			/* default: 32'b0 */
		} s;
	} reg_cmd_rdata_2;

	union			/* 0x0134, */
	{
		unsigned int dwValue;	/* command read data[127:96] */
		struct {
			unsigned int cmd_rdata_3:32;	/* command read data[127:96] */
			/* default: 32'b0 */
		} s;
	} reg_cmd_rdata_3;

	union			/* 0x0180, */
	{
		unsigned int dwValue;	/* dbg dump ldram */
		struct {
			unsigned int dbg_dump_ldram_enable:1;	/* internal dump enable */
			/* default: 1'b0 */
			unsigned int dbg_dump_ldram_flush_en:1;	/* internal dump flush turn-on
								when engine flush & dbg_dump_ldram_flush=1 */
			/* default: 1'b0 */
			unsigned int reserved5:4;	/*  */
			unsigned int dbg_dump_ldram_flush_base:26;	/* base address for flush to dram */
			/* default: 26'hx */
		} s;
	} reg_internal_dump;

	union			/* 0x0184, */
	{
		unsigned int dwValue;	/* mx debug mux selection */
		struct {
			unsigned int debug_mx_sel:32;	/* mx debug mux selection */
			/* default: 32'b0 */
		} s;
	} reg_debug_mx_sel;

	union			/* 0x0188, */
	{
		unsigned int dwValue;	/* mx pm mux selection */
		struct {
			unsigned int pm_mx_sel:32;	/* mx pm mux selection */
			/* default: 32'b0 */
		} s;
	} reg_pm_mx_sel;

	union			/* 0x0ff0, */
	{
		unsigned int dwValue;	/* mx debug control */
		struct {
			unsigned int mb_dbg_ctrl:4;	/* MB debug control */
			/* default: 4'b0 */
			unsigned int lt_dbg_ctrl:4;	/* LT debug control */
			/* default: 4'b0 */
			unsigned int ld_dbg_ctrl:4;	/* LD debug control */
			/* default: 4'b0 */
			unsigned int vc_dbg_ctrl:4;	/* VC debug control */
			/* default: 4'b0 */
			unsigned int cu_dbg_ctrl:4;	/* CU debug control */
			/* default: 4'b0 */
			unsigned int af_dbg_ctrl:4;	/* AF debug control */
			/* default: 4'b0 */
			unsigned int mx_apb_dbg_ck_on:1;	/* turn on MX APB debug related clock */
			/* default: 1'b0 */
			unsigned int rsv_dbg_ctrl:1;	/* Reserved debug control */
			/* default: 1'b0 */
			unsigned int pa2mx_cu_clr:1;	/* debug mode pa2mx_cu_clr trigger
								(write 1 then write 0 to trigger) */
			/* default: 1'b0 */
			unsigned int mx_l3_flush:1;	/* debug mode mx_l3_flush trigger
								(write 1 then write 0 to trigger) */
			/* default: 1'b0 */
			unsigned int mx_l2_flush:1;	/* debug mode mx_l2_flush trigger
								(write 1 then write 0 to trigger) */
			/* default: 1'b0 */
			unsigned int mx_invalidate:1;	/* debug mode mx_invalidate trigger
								(write 1 then write 0 to trigger) */
			/* default: 1'b0 */
			unsigned int raddr_auto_inc:1;	/* raddr auto increase mode enable */
			/* default: 1'b0 */
			unsigned int waddr_auto_inc:1;	/* waddr auto increase mode enable */
			/* default: 1'b0 */
		} s;
	} reg_debug_ctrl;

	union			/* 0x0ff4, */
	{
		unsigned int dwValue;	/* MX idle status */
		struct {
			unsigned int mb_idle:1;	/* MX and sub module idle status */
			/* default: 1'b1 */
			unsigned int lt_idle:1;	/* MX and sub module idle status */
			/* default: 1'b1 */
			unsigned int ld_idle:1;	/* MX and sub module idle status */
			/* default: 1'b1 */
			unsigned int vc_idle:1;	/* MX and sub module idle status */
			/* default: 1'b1 */
			unsigned int cu_idle:1;	/* MX and sub module idle status */
			/* default: 1'b1 */
			unsigned int af_idle:1;	/* MX and sub module idle status */
			/* default: 1'b1 */
			unsigned int mx2mf_idle:1;	/* MX and sub module idle status */
			/* default: 1'b1 */
			unsigned int mx2mf_l2_engidle:1;	/* MX and sub module idle status */
			/* default: 1'b1 */
			unsigned int mx2mf_stall:1;	/* MX and sub module idle status */
			/* default: 1'b0 */
			unsigned int mx2mf_alive:1;	/* MX and sub module idle status */
			/* default: 1'b0 */
			unsigned int break_match:1;	/* break conditiion matched status */
			/* default: 1'b0 */
			unsigned int apb_cmd_busy:1;	/* apb command busy status */
			/* default: 1'b0 */
			unsigned int hw_subframe_match:1;	/* monitor/break hw subframe count
									match status */
			/* default: 1'b0 */
			unsigned int sw_frame_id_match:1;	/* monitor/break sw frame_id/debug
									flag match status */
			/* default: 1'b1 */
		} s;
	} reg_mx_idle;

	union			/* 0x0ff8, */
	{
		unsigned int dwValue;	/* read error count clear */
		struct {
			unsigned int r_err_cnt_clr:1;	/* read error count clear */
		} s;
	} reg_r_err_cnt_clr;

	union			/* 0x0ffc, */
	{
		unsigned int dwValue;	/* write error count clear */
		struct {
			unsigned int w_err_cnt_clr:1;	/* write error count clear */
		} s;
	} reg_w_err_cnt_clr;

} SapphireMxDbg;


#endif /* _SAPPHIRE_MX_DBG_H_ */
