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

#ifndef _SAPPHIRE_QREG_H_
#define _SAPPHIRE_QREG_H_

/* Last updated on 2014/08/20, 14:32. The version : 459 */




/**********************************************************************************************************/
/*  */
#define REG_QREG_CQ_RSM                0x0000	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_CQ_RSM                0x00000001	/* [default:1'b0][perf:] command queue resume token,
								SW fill 0 , hw fill 1 */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_CQ_RSM                0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_FRAME_END             0x00000002	/* [default:1'b0][perf:] if sw preempt and g3d finish
								with frame end , set this bit 1, sw init 0 */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_FRAME_END             1
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_PMT_FM_IDX            0x000003fc	/* [default:8'b0][perf:] record the frame id in
								the current head/tail pointer */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_PMT_FM_IDX            2
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_PMT_HTRQ_VLD          0x00000400	/* [default:1'b0][perf:] preempt status, htrq pipe valid */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_PMT_HTRQ_VLD          10
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_PMT_FMRQ_VLD          0x00000800	/* [default:1'b0][perf:] preempt status, fmrq pipe valid */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_PMT_FMRQ_VLD          11
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_PMT_REGCMP_VLD        0x00001000	/* [default:1'b0][perf:] preempt status, regcmp pipe valid */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_PMT_REGCMP_VLD        12
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_PMT_VP_VLD            0x00002000	/* [default:1'b0][perf:] preempt status, vp pipe valid */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_PMT_VP_VLD            13
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_PMT_PP_VLD            0x00004000	/* [default:1'b0][perf:] preempt status, pp pipe valid */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_PMT_PP_VLD            14
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_PMT_VP0_VLD           0x00008000	/* [default:1'b0][perf:] preempt status, vp0 pipe valid */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_PMT_VP0_VLD           15
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_PMT_VP1_VLD           0x00010000	/* [default:1'b0][perf:] preempt status, vp1 pipe valid */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_PMT_VP1_VLD           16
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_PMT_OPENCL_EN         0x00020000	/* [default:1'b0][perf:] preempt status, opencl engine enable */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_PMT_OPENCL_EN         17

/**********************************************************************************************************/
/*  */
#define REG_QREG_CQ_HTBUF_RPTR         0x0004	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_CQ_HTBUF_RPTR         0xffffffff	/* [default:32'b0][perf:] cmdq (ring buffer )head pointer,
								the head = tail at sw init */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_CQ_HTBUF_RPTR         0

/**********************************************************************************************************/
/*  */
#define REG_QREG_CQ_HTBUF_WPTR         0x0008	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_CQ_HTBUF_WPTR         0xffffffff	/* [default:32'b0][perf:] cmdq (ring buffer )tail pointer,
								the head = tail at sw init */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_CQ_HTBUF_WPTR         0

/**********************************************************************************************************/
/*  */
#define REG_QREG_CQ_FM_HEAD            0x000c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_CQ_FM_HEAD            0xffffffff	/* [default:32'h0][perf:] frame command buffer head point */
						  /* SAVE/RESTORE INFO */
#define SFT_QREG_CQ_FM_HEAD            0

/**********************************************************************************************************/
/*  */
#define REG_QREG_CQ_HTBUF_START_ADDR   0x0010	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_CQ_HTBUF_START_ADDR   0xffffffff	/* [default:32'h0][perf:] cmdq(HT buffer) start address,
								must be 16 bytes alignment */
#define SFT_QREG_CQ_HTBUF_START_ADDR   0

/**********************************************************************************************************/
/*  */
#define REG_QREG_CQ_HTBUF_END_ADDR     0x0014	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_CQ_HTBUF_END_ADDR     0xffffffff	/* [default:32'b0][perf:] cmdq(HT buffer) end address,
								must be 16bytes alignment */
#define SFT_QREG_CQ_HTBUF_END_ADDR     0

/**********************************************************************************************************/
/*  */
#define REG_QREG_CQ_RSM_START_ADDR     0x0018	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_CQ_RSM_START_ADDR     0xffffffff	/* [default:32'b0][perf:] cq resume buffer base start addr,
								must be 16bytes alignment */
#define SFT_QREG_CQ_RSM_START_ADDR     0

/**********************************************************************************************************/
/*  */
#define REG_QREG_CQ_RSM_END_ADDR       0x001c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_CQ_RSM_END_ADDR       0xffffffff	/* [default:32'b0][perf:] cq resume buffer base end addr,
								must be 16bytes alignment */
#define SFT_QREG_CQ_RSM_END_ADDR       0

/**********************************************************************************************************/
/*  */
#define REG_QREG_CQ_RSMVPREG_START_ADDR 0x0020	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_CQ_RSMVPREG_START_ADDR 0xffffffff /* [default:32'b0][perf:] cq resume vp registers buffer start addr,
								must be 16bytes alignment */
#define SFT_QREG_CQ_RSMVPREG_START_ADDR 0

/**********************************************************************************************************/
/*  */
#define REG_QREG_CQ_RSMVPREG_END_ADDR  0x0024	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_CQ_RSMVPREG_END_ADDR  0xffffffff /* [default:32'b0][perf:] cq resume vp registers buffer end addr,
								must be 16bytes alignment */
#define SFT_QREG_CQ_RSMVPREG_END_ADDR  0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_BINHD_BASE0        0x0028	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_BINHD_BASE0        0xffffffff /* [default:32'h1000000][perf:] buf0 bin header base address,
								128 byte align */
#define SFT_QREG_VX_BINHD_BASE0        0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_BINTB_BASE0        0x002c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_BINTB_BASE0        0xffffffff /* [default:32'h2000000][perf:] buf0 bin table base address,
								128 byte align */
#define SFT_QREG_VX_BINTB_BASE0        0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_BINBK_BASE0        0x0030	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_BINBK_BASE0        0xffffffff	/* [default:32'h3000000][perf:] buf0 bin block base address,
								128 byte align */
#define SFT_QREG_VX_BINBK_BASE0        0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_VARY_BASE          0x0034	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_VARY_BASE          0xffffffff	/* [default:32'h4000000][perf:] varying data base address,
								128 byte align */
						  /* (note: it is a ring buffer for buf0, and buf1) */
#define SFT_QREG_VX_VARY_BASE          0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_BINHD_BASE1        0x0038	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_BINHD_BASE1        0xffffffff	/* [default:32'h5000000][perf:] buf1 bin header base address,
								128 byte align */
#define SFT_QREG_VX_BINHD_BASE1        0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_BINTB_BASE1        0x003c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_BINTB_BASE1        0xffffffff	/* [default:32'h6000000][perf:] buf1 bin table base address,
								128 byte align */
#define SFT_QREG_VX_BINTB_BASE1        0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_BINBK_BASE1        0x0040	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_BINBK_BASE1        0xffffffff	/* [default:32'h7000000][perf:] buf1 bin block base address,
								128 byte align */
#define SFT_QREG_VX_BINBK_BASE1        0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_VARY_MSIZE         0x0044	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_VARY_MSIZE         0x0fffffff	/* [default:28'h200000][perf:] memory size of varying data,
								multiple of 4K bytes, max 256M */
						  /* (note: it is a ring buffer size, sum of buf0's, and buf1's size) */
#define SFT_QREG_VX_VARY_MSIZE         0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_TB_MSIZE           0x0048	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_TB_MSIZE           0x0fffffff	/* [default:28'h100000][perf:] memory size of bin table,
								multiple of 16 bytes, max 256M */
#define SFT_QREG_VX_TB_MSIZE           0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_BK_MSIZE           0x004c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_BK_MSIZE           0x0fffffff	/* [default:28'h100000][perf:] memory size of bin block,
								multiple of 128 bytes, max 256M */
#define SFT_QREG_VX_BK_MSIZE           0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_VARY_PREBOFF_MSIZE 0x0050	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_VARY_PREBOFF_MSIZE 0x0fffffff	/* [default:28'h100000][perf:] pre full memory size of
								varying data, multiple of 1K bytes */
						  /* (note, it is the size for only buf0 or buf1) */
						  /* it should be <= vx_vary_msize/2 - vx_vw_buf_mgn */
#define SFT_QREG_VX_VARY_PREBOFF_MSIZE 0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_TB_PREBOFF_MSIZE   0x0054	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_TB_PREBOFF_MSIZE   0x0fffffff	/* [default:28'h100000][perf:] pre full memory size of
								bin table, multiple of 16 bytes */
						  /* it should be <= vx_tb_msize */
#define SFT_QREG_VX_TB_PREBOFF_MSIZE   0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_BK_PREBOFF_MSIZE   0x0058	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_BK_PREBOFF_MSIZE   0x0fffffff	/* [default:28'h100000][perf:] pre full memory size of
								bin block, multiple of 128 bytes */
						  /* it sould be <= vx_bk_msize */
#define SFT_QREG_VX_BK_PREBOFF_MSIZE   0

/**********************************************************************************************************/
/*  */
#define REG_QREG_VX_HD_PREBOFF_NUM     0x005c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_VX_HD_PREBOFF_NUM     0x0000ffff	/* [default:16'hff00][perf:] pre full primitive number of
								bin header */
						  /* it should be <= 16'ff00 */
#define SFT_QREG_VX_HD_PREBOFF_NUM     0

/**********************************************************************************************************/
/*  */
#define REG_QREG_MX_SECURE1_BASE_ADDR  0x0060	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_MX_SECURE1_BASE_ADDR  0x000fffff	/* [default:20'd0][perf:] base address of secure range for
								frame cmd buffer */
#define SFT_QREG_MX_SECURE1_BASE_ADDR  0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_MX_SECURE1_ENABLE     0x80000000	/* [default:1'b0][perf:] enable this range checking */
#define SFT_QREG_MX_SECURE1_ENABLE     31

/**********************************************************************************************************/
/*  */
#define REG_QREG_MX_SECURE1_TOP_ADDR   0x0064	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_QREG_MX_SECURE1_TOP_ADDR   0x000fffff	/* [default:20'd0][perf:] top address of secure range for
								frame cmd buffer */
#define SFT_QREG_MX_SECURE1_TOP_ADDR   0

/**********************************************************************************************************/


typedef struct {
	union			/* 0x0000, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int cq_rsm:1;	/* command queue resume token, SW fill 0 , hw fill 1 */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int frame_end:1;	/* if sw preempt and g3d finish with frame end ,
								set this bit 1, sw init 0 */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pmt_fm_idx:8;	/* record the frame id in the current head/tail pointer */
			/* SAVE/RESTORE INFO */
			/* default: 8'b0 */
			unsigned int pmt_htrq_vld:1;	/* preempt status, htrq pipe valid */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pmt_fmrq_vld:1;	/* preempt status, fmrq pipe valid */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pmt_regcmp_vld:1;	/* preempt status, regcmp pipe valid */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pmt_vp_vld:1;	/* preempt status, vp pipe valid */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pmt_pp_vld:1;	/* preempt status, pp pipe valid */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pmt_vp0_vld:1;	/* preempt status, vp0 pipe valid */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pmt_vp1_vld:1;	/* preempt status, vp1 pipe valid */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
			unsigned int pmt_opencl_en:1;	/* preempt status, opencl engine enable */
			/* SAVE/RESTORE INFO */
			/* default: 1'b0 */
		} s;
	} reg_cq_rsm;

	union			/* 0x0004, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int cq_htbuf_rptr:32;	/* cmdq (ring buffer )head pointer,
								the head = tail at sw init */
			/* SAVE/RESTORE INFO */
			/* default: 32'b0 */
		} s;
	} reg_cq_htbuf_rptr;

	union			/* 0x0008, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int cq_htbuf_wptr:32;	/* cmdq (ring buffer )tail pointer,
								the head = tail at sw init */
			/* SAVE/RESTORE INFO */
			/* default: 32'b0 */
		} s;
	} reg_cq_htbuf_wptr;

	union			/* 0x000c, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int cq_fm_head:32;	/* frame command buffer head point */
			/* SAVE/RESTORE INFO */
			/* default: 32'h0 */
		} s;
	} reg_cq_fm_head;

	union			/* 0x0010, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int cq_htbuf_start_addr:32;	/* cmdq(HT buffer) start address,
									must be 16 bytes alignment */
			/* default: 32'h0 */
		} s;
	} reg_cq_htbuf_start_addr;

	union			/* 0x0014, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int cq_htbuf_end_addr:32;	/* cmdq(HT buffer) end address,
									must be 16bytes alignment */
			/* default: 32'b0 */
		} s;
	} reg_cq_htbuf_end_addr;

	union			/* 0x0018, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int cq_rsm_start_addr:32;	/* cq resume buffer base start addr,
									must be 16bytes alignment */
			/* default: 32'b0 */
		} s;
	} reg_cq_rsm_start_addr;

	union			/* 0x001c, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int cq_rsm_end_addr:32;	/* cq resume buffer base end addr,
									must be 16bytes alignment */
			/* default: 32'b0 */
		} s;
	} reg_cq_rsm_end_addr;

	union			/* 0x0020, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int cq_rsmvpreg_start_addr:32;	/* cq resume vp registers buffer start addr,
									must be 16bytes alignment */
			/* default: 32'b0 */
		} s;
	} reg_cq_rsmvpreg_start_addr;

	union			/* 0x0024, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int cq_rsmvpreg_end_addr:32;	/* cq resume vp registers buffer end addr,
									must be 16bytes alignment */
			/* default: 32'b0 */
		} s;
	} reg_cq_rsmvpreg_end_addr;

	union			/* 0x0028, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_binhd_base0:32;	/* buf0 bin header base address, 128 byte align */
			/* default: 32'h1000000 */
		} s;
	} reg_vx_binhd_base0;

	union			/* 0x002c, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_bintb_base0:32;	/* buf0 bin table base address, 128 byte align */
			/* default: 32'h2000000 */
		} s;
	} reg_vx_bintb_base0;

	union			/* 0x0030, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_binbk_base0:32;	/* buf0 bin block base address, 128 byte align */
			/* default: 32'h3000000 */
		} s;
	} reg_vx_binbk_base0;

	union			/* 0x0034, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_vary_base:32;	/* varying data base address, 128 byte align */
			/* (note: it is a ring buffer for buf0, and buf1) */
			/* default: 32'h4000000 */
		} s;
	} reg_vx_vary_base;

	union			/* 0x0038, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_binhd_base1:32;	/* buf1 bin header base address, 128 byte align */
			/* default: 32'h5000000 */
		} s;
	} reg_vx_binhd_base1;

	union			/* 0x003c, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_bintb_base1:32;	/* buf1 bin table base address, 128 byte align */
			/* default: 32'h6000000 */
		} s;
	} reg_vx_bintb_base1;

	union			/* 0x0040, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_binbk_base1:32;	/* buf1 bin block base address, 128 byte align */
			/* default: 32'h7000000 */
		} s;
	} reg_vx_binbk_base1;

	union			/* 0x0044, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_vary_msize:28;	/* memory size of varying data, multiple of
								4K bytes, max 256M */
			/* (note: it is a ring buffer size, sum of buf0's, and buf1's size) */
			/* default: 28'h200000 */
		} s;
	} reg_vx_vary_msize;

	union			/* 0x0048, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_tb_msize:28;	/* memory size of bin table, multiple of 16 bytes, max 256M */
			/* default: 28'h100000 */
		} s;
	} reg_vx_tb_msize;

	union			/* 0x004c, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_bk_msize:28;	/* memory size of bin block, multiple of 128 bytes, max 256M */
			/* default: 28'h100000 */
		} s;
	} reg_vx_bk_msize;

	union			/* 0x0050, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_vary_preboff_msize:28;	/* pre full memory size of varying data,
									multiple of 1K bytes */
			/* (note, it is the size for only buf0 or buf1) */
			/* it should be <= vx_vary_msize/2 - vx_vw_buf_mgn */
			/* default: 28'h100000 */
		} s;
	} reg_vx_vary_preboff_msize;

	union			/* 0x0054, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_tb_preboff_msize:28;	/* pre full memory size of bin table,
									multiple of 16 bytes */
			/* it should be <= vx_tb_msize */
			/* default: 28'h100000 */
		} s;
	} reg_vx_tb_preboff_msize;

	union			/* 0x0058, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_bk_preboff_msize:28;	/* pre full memory size of bin block,
									multiple of 128 bytes */
			/* it sould be <= vx_bk_msize */
			/* default: 28'h100000 */
		} s;
	} reg_vx_bk_preboff_msize;

	union			/* 0x005c, VX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int vx_hd_preboff_num:16;	/* pre full primitive number of bin header */
			/* it should be <= 16'ff00 */
			/* default: 16'hff00 */
		} s;
	} reg_vx_hd_preboff_num;

	union			/* 0x0060, MX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int mx_secure1_base_addr:20;	/* base address of secure range for
									frame cmd buffer */
			/* default: 20'd0 */
			unsigned int reserved0:11;
			unsigned int mx_secure1_enable:1;	/* enable this range checking */
			/* default: 1'b0 */
		} s;
	} reg_mx_secure1_base_addr;

	union			/* 0x0064, MX */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int mx_secure1_top_addr:20;	/* top address of secure range for
									frame cmd buffer */
			/* default: 20'd0 */
		} s;
	} reg_mx_secure1_top_addr;

} SapphireQreg;


#endif /* _SAPPHIRE_QREG_H_ */
