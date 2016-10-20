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

#include "gpad_sdbg_dump.h"
#include "gpad_log.h"

#define REG_SIZE (32)
/*
 * internal function declaration
 */
static void _dump_irq_raw_data(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);
static void _dump_pa_idle_status(struct seq_file *m, int idx, unsigned int value);
static void _dump_g3d_stall_bus(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);
static void _dump_instruction_single_context(struct seq_file *m, unsigned int idx, struct gpad_sdbg_info *sdbg_info);
static void _dbg_ff_regset_select(struct seq_file *m, unsigned int top_sel);
static void _dbg_cmn_regset_select(struct seq_file *m, unsigned int top_sel);

void gpad_sdbg_dump_areg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int value;

	seq_printf(m, "==== AREG Debug Status ====\n");

	_dump_irq_raw_data(m, sdbg_info);

	/* dump pa_idle_status0 and pa_idle_status1 */
	seq_printf(m, "**pa idle status:\n");

	value = sdbg_info->areg.pa_idle_status0;
	seq_printf(m, "    %-20s (0x%08X): 0x%08X\n", "pa_idle_status0", SDBG_AREG_PA_IDLE_STATUS0, value);

	if (0xFFFFFFFF != value) {
		/* 0xFFFFFFFF is default value */
		_dump_pa_idle_status(m, 0, value);
	}

	value = sdbg_info->areg.pa_idle_status1;
	seq_printf(m, "    %-20s (0x%08X): 0x%08X\n", "pa_idle_status1", SDBG_AREG_PA_IDLE_STATUS1, value);

	if (0xFFFFFFFF != value) {
		/* 0xFFFFFFFF is default value */
		_dump_pa_idle_status(m, 1, value);
	}

	_dump_g3d_stall_bus(m, sdbg_info);

}

void _dump_irq_raw_data(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	unsigned value, i, size = REG_SIZE;
	const char *irq_raw_str[] = {
		"cq preempt finish", "cq resume finish", "pa frame base data queue not empty", "pa frame swap", "pa frame end", "pa sync sw", "g3d hang", "g3d mmce frame flush", "g3d mmce frame end", "cq error code check", "mx debug", "pwc snap", "DFD internal dump triggered", "pm debug mode", "vl negative index happen", "areg apb timeout happen", "ux_cmn response", "ux_dwp0 response", "ux_dwp1 response", NULL
	};
	/* dump irq raw data */
	seq_printf(m, "**irq raw data:\n");
	value = sdbg_info->areg.irq_raw_data;

	seq_printf(m, "    %-20s (0x%08X): 0x%08X\n", "irq_raw", SDBG_AREG_IRQ_RAW, value);

	if (0 != value) {
		seq_printf(m, "      Details:\n");
		for (i = 0; i < size && irq_raw_str[i] != NULL; i++) {
			if (0 != (value & (0x1 << i)))
				seq_printf(m, "        %s (bit %d)\n", irq_raw_str[i], i);

		}
	}
	return;
}

void _dump_pa_idle_status(struct seq_file *m, int idx, unsigned int value)
{
	int i;
	int status_size = REG_SIZE;
	const char *pa_idle_status_str[2][REG_SIZE] = {
		{
			"g3d_idle_timer_expire", "g3d_all_idle", "g3d_idle", "pa_idle",          /*  0-3  */
			"sf2pa_vp0_idle", "cf2pa_vp0_idle", "vs2pa_vp0_idle", "vl2pa_vp0_idle",  /*  4-7  */
			"vx2pa_vp1_idle", "bw2pa_vp1_idle", "vs2pa_vp1_idle", "vl2pa_vp1_idle",  /*  8-11 */
			"tx2pa_pp_idle", "zp2pa_pp_idle", "cp2pa_pp_idle", "fs2pa_pp_idle",      /* 12-15 */
			"xy2pa_pp_idle", "sb2pa_pp_idle", "lv2pa_pp_idle", "cb2pa_pp_idle",      /* 16-19 */
			"vc2pa_pp_idle", "vx2pa_pp_idle", NULL, NULL,                            /* 20-23 */
			"cq2pa_wr_idle", "cq2pa_rd_idle", "gl2pa_pp_idle", "gl2pa_vp_idle",      /* 24-27 */
			NULL, NULL, NULL, NULL                                                   /* 28-31 */
		}, {
			"cc2pa_pp_l1_idle", "cz2pa_pp_l1_idle", "zc2pa_pp_l1_idle", "ux2pa_vp_l1_idle", /*  0-3  */
			"hc2pa_vp1_l1_idle", "cz2pa_vp1_l1_idle", "mx2pa_l2_idle", NULL,                /*  4-7  */
			"opencl2pa_vp_idle", "clin2pa_vp_idle", "vl2pa_vp1_idle", "vx2pa_vp1_idle",     /*  8-11 */
			NULL, NULL, NULL, NULL,                                 /* 12-15 */
			NULL, NULL, NULL, NULL,                                 /* 16-19 */
			NULL, NULL, NULL, NULL,                                 /* 20-23 */
			NULL, NULL, NULL, NULL,                                 /* 24-27 */
			NULL, NULL, NULL, NULL                                  /* 28-31 */
		}
	};

	seq_printf(m, "      Module not idle:\n");
	for (i = 0; i < status_size; i++) {
		if (NULL == pa_idle_status_str[idx][i])
			continue;
		else if (0 == (value & (0x1 << i)))
			seq_printf(m, "%33s (bit %d)\n", pa_idle_status_str[idx][i], i);
	}
}

void _dump_g3d_stall_bus(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int value, i;
	const char *g3d_stall_bus[] = {
		/* pp l1 */
		"mx2pa_l2", "cc2pa_pp_l1", "zc2pa_pp_l1", "cz2pa_pp_l1",    /*  0-3  */
		/* pp */
		"cp2pa_pp", "zp2pa_pp", "tx2pa_pp", "fs2pa_pp", "lv2pa_pp", /*  4-8  */
		"xy2pa_pp", "sb2pa_pp", "cb2pa_pp", "vc2pa_pp", "vx2pa_pp", /*  9-13 */
		/* vp l1 */
		"ux2pa_vp_l1", "hc2pa_vp1_l1", "cz2pa_vp1_l1", NULL,        /* 14-17 */
		/* vp1 */
		"vx2pa_vp1", "bw2pa_vp1", "vs2pa_vp1", "vl2pa_vp1",         /* 18-21 */
		/* vp0 */
		"vl2pa_vp0", "vs2pa_vp0", "cf2pa_vp0", "sf2pa_vp0",         /* 22-25 */
		/* others */
		"pe2pa", "mx2pa_axi", NULL, NULL, NULL, NULL                /* 26-31 */
	};

	value = sdbg_info->areg.g3d_stall_bus;

	if (0 == value)
		return;


	seq_printf(m, "** stall staus:\n");
	seq_printf(m, "    g3d_stall_bus  (0x%08X): 0x%08X\n", SDBG_AREG_DEBUG_BUS, value);

	for (i = 0; i < REG_SIZE; i++) {
		if (NULL == g3d_stall_bus[i])
			continue;

		if (0 != (value & (0x1 << i)))
			seq_printf(m, "%27s_stall (bit %d)\n", g3d_stall_bus[i], i);

	}
}

void gpad_sdbg_dump_common_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int value;
	int i;
	const char *input_stage_io_arr[] = {
		"vs0i_rdy", "vs0i_ack", "vs0i2tks_rdy", "tks2vs0i_ack", "vs0i2smm_rdy", "smm2vs0i_ack", "vs1i_rdy", "vs1i_ack", "vs1i2tks_rdy", "tks2vs1i_ack", "vs1i2smm_rdy", "smm2vs1i_ack", "fsi_rdy", "fsi_ack", "fsi2tks_rdy", "tks2fsi_ack", "fsi2smm_rdy", "smm2fsi_ack", NULL
	};

	const char *output_stage_io_arr[] = {
		"ux2vl_vs0_rdy", "vl2ux_vs0_ack", "ux2vl_vs1_rdy", "vl2ux_vs1_ack", "ux2vx_rdy", "vx2ux_ack", "ux2cp_rdy", "cp2ux_ack", NULL
	};

	const char *tx_io_arr[] = {
		"ux2tx_rdy", "tx2ux_ack", "ux2tx_idx_rdy", "tx2ux_idx_ack", "ux2tx_pix_rdy", "tx2ux_pix_ack", "ux2tx_glbini_rdy", "tx2ux_glbini_ack", "ux2tx_glbclr_rdy", "tx2ux_glbclr_ack", NULL
	};

	const char *ltc_io_arr[] = {
		"ux2mx_cp_rd_req", "mx2ux_cp_rd_ack", "ux2mx_ic_rd_req", "mx2ux_ic_rd_ack", "ux2mx_vp_rd_req", "mx2ux_vp_rd_ack", "ux2mx_vp_wr_req", "mx2ux_vp_wr_ack", NULL
	};

	const char *isc_pf_io_arr[] = {
		"wp02isc_pf_rdy", "isc2wp0_pf_ack", "wp22isc_pf_rdy", "isc2wp2_pf_ack", NULL
	};

	const char *isc_rl_io_arr[] = {
		"wp02isc_rl_rdy", "isc2wp0_rl_ack", "wp22isc_rl_rdy", "isc2wp2_rl_ack", NULL
	};

	const char *isc_rd_io_arr[] = {
		"wp02isc_rd_rdy", "isc2wp0_rd_ack", "wp22isc_rd_rdy", "isc2wp2_rd_ack", NULL
	};

	const char *idle_status_arr[] = {
		"cpu", "lsu", "isc", "vpl1", "cpl1", "vs0i", "vs1i", "fsi", "vs0r", "vs1r", "fsr", NULL
	};

	seq_printf(m, "==== CMNREG Status ====\n");

	value = sdbg_info->cmnreg.lsu_err_maddr;
	seq_printf(m, "Error memory address sent by LSU   : 0x%08X\n", value);

	value = sdbg_info->cmnreg.cpl1_err_maddr;
	seq_printf(m, "Error memory address sent by CPL1  : 0x%08X\n", value);

	value = sdbg_info->cmnreg.isc_err_maddr;
	seq_printf(m, "Error memory address sent by isc             : 0x%08X\n", value);

	value = sdbg_info->cmnreg.cprf_err_info;
	seq_printf(m, "CPRF error wave pool id                      : %d\n", (value >> 4) & 0x3);
	seq_printf(m, "CPRF error wave id                           : %d\n", (value & 0xF));
	seq_printf(m, "CPRF error address                           : 0x%X\n", (value >> 8) & 0xFF);

	value = sdbg_info->cmnreg.stage_io;
	seq_printf(m, "Input stage I/O:\n");
	for (i = 0; NULL != input_stage_io_arr[i]; i++)
		seq_printf(m, "  %-33s: %d\n", input_stage_io_arr[i], (value >> i) & 0x1);

	seq_printf(m, "Output stage I/O:\n");
	for (i = 0; NULL != output_stage_io_arr[i]; i++)
		seq_printf(m, "  %-33s: %d\n", output_stage_io_arr[i], (value >> (i + 24)) & 0x1);


	value = sdbg_info->cmnreg.tx_ltc_io;
	seq_printf(m, "TX I/O:\n");
	for (i = 0; NULL != tx_io_arr[i]; i++)
		seq_printf(m, "  %-33s: %d\n", tx_io_arr[i], (value >> i) & 0x1);


	seq_printf(m, "LTC I/O:\n");
	for (i = 0; NULL != ltc_io_arr[i]; i++)
		seq_printf(m, "  %-33s: %d\n", ltc_io_arr[i], (value >> (i + 16)) & 0x1);


	value = sdbg_info->cmnreg.isc_io;
	seq_printf(m, "ISC PF I/O:\n");
	for (i = 0; NULL != isc_pf_io_arr[i]; i++)
		seq_printf(m, "  %-33s: %d\n", isc_pf_io_arr[i], (value >> i) & 0x1);


	seq_printf(m, "ISC RL I/O:\n");
	for (i = 0; NULL != isc_rl_io_arr[i]; i++)
		seq_printf(m, "  %-33s: %d\n", isc_rl_io_arr[i], (value >> (i + 8)) & 0x1);

	seq_printf(m, "ISC RD I/O:\n");
	for (i = 0; NULL != isc_rd_io_arr[i]; i++)
		seq_printf(m, "  %-33s: %d\n", isc_rd_io_arr[i], (value >> (i + 16)) & 0x1);


	value = sdbg_info->cmnreg.idle_status;
	seq_printf(m, "Idle status:\n");
	for (i = 0; NULL != idle_status_arr[i]; i++)
		seq_printf(m, "  %-33s: %d\n", idle_status_arr[i], (value >> i) & 0x1);


	value = sdbg_info->cmnreg.type_wave_cnt;
	seq_printf(m, "Wave count of each shader type:\n");
	seq_printf(m, "vs0 wave count                     : %d\n", value & 0x3F);
	seq_printf(m, "vs1 wave count                     : %d\n", (value >> 8) & 0x3F);
	seq_printf(m, "fs wave count                      : %d\n", (value >> 16) & 0x3F);
	seq_printf(m, "context number                     : %d\n", (value >> 24) & 0x3D);

	value = sdbg_info->cmnreg.wave_stop_info;
	seq_printf(m, "vs0i stop wave pool id             : %d\n", (value >> 4) & 0x3);
	seq_printf(m, "vs0i stop wave id                  : %d\n", value & 0xF);
	seq_printf(m, "vs1i stop wave pool id             : %d\n", (value >> 4) & 0x3);
	seq_printf(m, "vs1i stop wave id                  : %d\n", value & 0xF);
	seq_printf(m, "fsi stop wave pool id              : %d\n", (value >> 4) & 0x3);
	seq_printf(m, "fsi stop wave id                   : %d\n", value & 0xF);

	value = sdbg_info->cmnreg.vs0i_sid;
	seq_printf(m, "vs0i sequence id                   : %d\n", value);
	value = sdbg_info->cmnreg.vs1i_sid;
	seq_printf(m, "vs1i sequence id                   : %d\n", value);
	value = sdbg_info->cmnreg.fsi_sid;
	seq_printf(m, "fsi sequence id                    : %d\n", value);
	value = sdbg_info->cmnreg.wp_wave_cnt;
	seq_printf(m, "Wave pool 0 wave count             : %d\n", value & 0xF);
#ifdef SAPPHIRE
	seq_printf(m, "Wave pool 1 wave count             : %d\n", (value >> 4)  & 0xF);
#endif
	seq_printf(m, "Wave pool 2 wave count             : %d\n", (value >> 8)  & 0xF);
#ifdef SAPPHIRE
	seq_printf(m, "Wave pool 3 wave count             : %d\n", (value >> 12) & 0xF);
#endif
	value = sdbg_info->cmnreg.wp0_lsm_rem;
	seq_printf(m, "Wave pool 0 LSM private rem        : %d\n", value & 0x7F);
	seq_printf(m, "Wave pool 0 LSM group rem          : %d\n", (value >> 10) & 0x7F);
	seq_printf(m, "Wave pool 0 LSM global rem         : %d\n", (value >> 20) & 0x7F);
	value = sdbg_info->cmnreg.wp0_vrf_crf_rem;
	seq_printf(m, "Wave pool 0 VRF rem                : %d\n", value & 0x7F);
	seq_printf(m, "Wave pool 0 CRF rem                : %d\n", (value >> 10) & 0x7F);
#ifdef SAPPHIRE
	value = sdbg_info->cmnreg.wp0_lsm_rem;
	seq_printf(m, "Wave pool 1 LSM private rem        : %d\n", value & 0x7F);
	seq_printf(m, "Wave pool 1 LSM group rem          : %d\n", (value >> 10) & 0x7F);
	seq_printf(m, "Wave pool 1 LSM global rem         : %d\n", (value >> 20) & 0x7F);
	value = sdbg_info->cmnreg.wp0_vrf_crf_rem;
	seq_printf(m, "Wave pool 1 VRF rem                : %d\n", value & 0x7F);
	seq_printf(m, "Wave pool 1 CRF rem                : %d\n", (value >> 10) & 0x7F);
#endif
	value = sdbg_info->cmnreg.wp2_lsm_rem;
	seq_printf(m, "Wave pool 2 LSM private rem        : %d\n", value & 0x7F);
	seq_printf(m, "Wave pool 2 LSM group rem          : %d\n", (value >> 10) & 0x7F);
	seq_printf(m, "Wave pool 2 LSM global rem         : %d\n", (value >> 20) & 0x7F);
	value = sdbg_info->cmnreg.wp2_vrf_crf_rem;
	seq_printf(m, "Wave pool 2 VRF rem                : %d\n", value & 0x7F);
	seq_printf(m, "Wave pool 2 CRF rem                : %d\n", (value >> 10) & 0x7F);
#ifdef SAPPHIRE
	value = sdbg_info->cmnreg.wp2_lsm_rem;
	seq_printf(m, "Wave pool 3 LSM private rem        : %d\n", value & 0x7F);
	seq_printf(m, "Wave pool 3 LSM group rem          : %d\n", (value >> 10) & 0x7F);
	seq_printf(m, "Wave pool 3 LSM global rem         : %d\n", (value >> 20) & 0x7F);
	value = sdbg_info->cmnreg.wp2_vrf_crf_rem;
	seq_printf(m, "Wave pool 3 VRF rem                : %d\n", value & 0x7F);
	seq_printf(m, "Wave pool 3 CRF rem                : %d\n", (value >> 10) & 0x7F);
#endif
}

void gpad_sdbg_dump_dualwavepool_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int value;
	int i;
	const char *idle_status_str[] = {
		"wp0_vpu", "wp0_lsm", "wp1_vpu", "wp1_lsm", "sfu", "vs0r", "vs1r", "fsr", NULL
	};

	seq_printf(m, "==== DWPREG Status ====\n");
	value = sdbg_info->dwpreg.status_report[0];
	seq_printf(m, "vs0r_curr_fid: %X\n", value & 0x3F);
	seq_printf(m, "vs1r_curr_fid: %X\n", (value >> 8) & 0x3F);
	seq_printf(m, " fsr_curr_fid: %X\n", (value >> 16) & 0x3F);

	value = sdbg_info->dwpreg.status_report[1];
	seq_printf(m, "vs0r_next_fid: %X\n", value & 0x3F);
	seq_printf(m, "vs1r_next_fid: %X\n", (value >> 8) & 0x3F);
	seq_printf(m, " fsr_next_fid: %X\n", (value >> 16) & 0x3F);

	value = sdbg_info->dwpreg.status_report[2];
	seq_printf(m, "     wp0_busy: %X\n", value & 0x3FF);
	seq_printf(m, "     wp1_busy: %X\n", (value >> 16) & 0x3FF);

	value = sdbg_info->dwpreg.status_report[3];
	for (i = 0; idle_status_str[i] != NULL; i++) {
		int bit = (value & (0x1 << i)) != 0 ? 1 : 0;
		seq_printf(m, "%8s idle: %d\n", idle_status_str[i], bit);
	}
}

void gpad_sdbg_dump_global_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int idx;
	const char *desc[] = {"cx0_fs", "cx1_fs", "cx2_fs", "cx3_fs", "vs0", "vs1"};
	seq_printf(m, "==== Context/Frame Global Registser ====\n");

	/* instruction base address. cl/fs: 0~3, vs0: 6, vs1: 7 */
	for (idx = 0; idx < 6; idx++)
		seq_printf(m, "%20s_inst_base: 0x%08X\n", desc[idx], sdbg_info->greg.inst_base[idx]);


	/* constant descriptor memory base address */
	for (idx = 0; idx < 6; idx++) {
		seq_printf(m, "%12s_cnst_dsc_mem_base: 0x%08X\n",    desc[idx], sdbg_info->greg.cnst_dsc_mem_base[idx]);
	}

	/* constant memory base address */
	for (idx = 0; idx < 6; idx++) {
		seq_printf(m, "%16s_cnst_mem_base: 0x%08X\n",    desc[idx], sdbg_info->greg.cnst_mem_base[idx]);
	}

	/* group memory base address (cl only) */
	for (idx = 0; idx < 4; idx++) {
		seq_printf(m, "%14s_cl_grp_mem_base: 0x%08X\n",    desc[idx], sdbg_info->greg.grp_mem_base[idx]);
	}

	/* constant memory base address (cl only) */
	for (idx = 0; idx < 4; idx++) {
		seq_printf(m, "%16s_cl_prv_mem_base: 0x%08X\n",    desc[idx], sdbg_info->greg.prv_mem_base[idx]);
	}

	/* instruction last pc */
	for (idx = 0; idx < 6; idx++) {
		seq_printf(m, "%17s_inst_last_pc: 0x%08X\n",    desc[idx], sdbg_info->greg.inst_last_pc[idx]);
	}

	/* fs out rt0~3 / z base */
	for (idx = 0; idx < 4; idx++) {
		seq_printf(m, "%10s_fs_out_rt0~3/z_base: 0x%08X\n",    desc[idx], sdbg_info->greg.fs_out_z_base[idx]);
	}

	/* misc data */
	/*    seq_printf(m, "Misc data : [2'd0/cl_barrier_en/cl_wkg_config/7'd0/cl_wg_type_sel/fs_primtype/fsin_en/5'd0/fs_out_rt_en]:\n"); */
	for (idx = 0; idx < 4; idx++) {
		seq_printf(m, "%11s_data (select 0x7%u): 0x%08X\n",    desc[idx], idx, sdbg_info->greg.data1[idx]);
	}

	/* cl context sequence id/ cl proc sequence id */
	for (idx = 0; idx < 4; idx++) {
		seq_printf(m, "%11s_data (select 0x8%u): 0x%08X\n",    desc[idx], idx, sdbg_info->greg.data2[idx]);
	}

	for (idx = 0; idx < 6; idx++) {
		seq_printf(m, "%15s_inst_mem_limit: 0x%08X\n",    desc[idx], sdbg_info->greg.inst_mem_limit[idx]);
	}

	for (idx = 0; idx < 6; idx++) {
		seq_printf(m, "%15s_cnst_mem_limit: 0x%08X\n",    desc[idx], sdbg_info->greg.cnst_mem_limit[idx]);
	}

	for (idx = 0; idx < 4; idx++) {
		seq_printf(m, "%16s_grp_mem_limit: 0x%08X\n",    desc[idx], sdbg_info->greg.grp_mem_limit[idx]);
	}

	for (idx = 0; idx < 4; idx++) {
		seq_printf(m, "%17s_prv_mem_limit: 0x%08X\n",    desc[idx], sdbg_info->greg.prv_mem_limit[idx]);
	}

	/* other data */
	for (idx = 0; idx < 10; idx++)
		seq_printf(m, "       freg_data (select 0x1%u0): 0x%08X\n", idx, sdbg_info->greg.freg_data[0]);

}

void gpad_sdbg_dump_instruction_cache(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	const unsigned int ctx[6] = {0, 1, 2, 3, 6, 7};
	int i;

	/* check debug level */
	seq_printf(m, "==== Instruction cache ====\n");

	for (i = 0; i < 6; i++) {
		unsigned int cid = ctx[i];
		seq_printf(m, "*Context %d: %u byte(s)\n", cid, sdbg_info->inst[i].size);
		GPAD_LOG_INFO("*Context %d: %u byte(s)\n", cid, sdbg_info->inst[i].size);
		if (unlikely(NULL == sdbg_info->inst[i].data)) {
			GPAD_LOG_ERR("instruction data ctx %u is null\n", cid);
			continue;
		}
		_dump_instruction_single_context(m, i, sdbg_info);
	}
}

void _dump_instruction_single_context(struct seq_file *m, unsigned int idx, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int i, size, old_value;
	unsigned int *data;
	unsigned int is64bit;

	size = sdbg_info->inst[idx].size;
	data = sdbg_info->inst[idx].data;

	is64bit = old_value = 0;

	for (i = 0; i < size; i++) {

		if (0 == is64bit && 0 != (data[i] & 0x80000000)) {
			is64bit = 1;
			old_value = data[i];
			continue;
		}

		if (likely(is64bit)) {
			seq_printf(m, "%08X %08X\n", old_value, data[i]);
			is64bit = 0;
		} else {
			seq_printf(m, "%08X\n", data[i]);
		}
	}
}

void gpad_sdbg_dump_wave_info(struct seq_file *m, unsigned int wpid, unsigned int wid, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int value1, value2, wcnt;
	const char *wf_state[] = {
		"Idle", "Init", "Execution", "Stop", "Wait TX Init",                /*  0-4 */
		"Issue CTP", "Wait Set PC", "Wait End Program", "Wait System Call", /*  5-8 */
		"Wait MSync Exe", "Wait MSync Counter", "Wait Barrier", "Wait COP", /*  9-12 */
		"Wait VOP", "End1(wait dependency counter)", "End2"                 /* 13-15 */
	};

	seq_printf(m, "wave pool id = %u, wave id = %u\n", wpid, wid);
	wcnt = (wpid / 2) * 12 + wid;

	value1 = sdbg_info->wave_stat[wcnt].data[0][0];
	seq_printf(m, "sequence id         : %u\n", value1 & 0x3FFF);

	value1 = sdbg_info->wave_stat[wcnt].data[1][0];
	seq_printf(m, "current pc          : %u\n", value1 & 0xFFFFF);

	value1 = sdbg_info->wave_stat[wcnt].data[1][1];
	seq_printf(m, "current issued pc   : %u\n", value1 & 0xFFFFFF);

	value1 = sdbg_info->wave_stat[wcnt].data[2][0];
	seq_printf(m, "wavefront state     : 0x%08X (%s)\n", value1 & 0xF, wf_state[value1&0xF]);
	seq_printf(m, "context id          : %u\n", (value1 >> 12) & 0x7);

	value1 = sdbg_info->wave_stat[wcnt].data[3][0];
	value2 = sdbg_info->wave_stat[wcnt].data[3][1];
	seq_printf(m, "last instruction    : 0x%08X 0x%08X\n", value1, value2);

	value1 = sdbg_info->wave_stat[wcnt].data[8][0];
	value2 = sdbg_info->wave_stat[wcnt].data[8][1];
	seq_printf(m, "current instruction : 0x%08X 0x%08X\n", value1, value2);

}

void gpad_dfd_dump_dwp_debug_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int i, wid, wpid;
	/* SFU */
	seq_printf(m, "     SFU:\n");
	seq_printf(m, "         Port A (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[0].data[i][0]);

	seq_printf(m, "\n         Port B (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[0].data[i][1]);

	seq_printf(m, "\n");

	/* IFB */
	seq_printf(m, "     IFB:\n");
	for (wpid = 0; wpid < 2; wpid++) {
		seq_printf(m, "     WP%d Port A:", wpid*2);
		for (wid = 0; wid < 12; wid++) {
			for (i = 0; i < 11; i++)
				seq_printf(m, " WID%dSEL%d %08X, ", wid, i, sdbg_info->debug_wave_info[wpid*12+wid].data[i][0]);
			seq_printf(m, "\n                ");
		}
		seq_printf(m, "\n");
	}
	for (wpid = 0; wpid < 2; wpid++) {
		seq_printf(m, "     WP%d Port B:", wpid*2);
		for (wid = 0; wid < 12; wid++) {
			for (i = 0; i < 11; i++)
				seq_printf(m, " WID%dSEL%d %08X, ", wid, i, sdbg_info->debug_wave_info[wpid*12+wid].data[i][1]);
			seq_printf(m, "\n                ");
		}
		seq_printf(m, "\n");
	}

	/* VS0R */
	seq_printf(m, "     VSR0:\n");
	seq_printf(m, "         Port A (sel 0~31):");
	for (i = 0; i < 32; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[2].data[i][0]);

	seq_printf(m, "\n         Port B (sel 0~31):");
	for (i = 0; i < 32; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[2].data[i][1]);

	seq_printf(m, "\n");

	/* VS1R */
	seq_printf(m, "     VSR1:\n");
	seq_printf(m, "         Port A (sel 0~31):");
	for (i = 0; i < 32; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[3].data[i][0]);

	seq_printf(m, "\n         Port B (sel 0~31):");
	for (i = 0; i < 32; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[3].data[i][1]);

	seq_printf(m, "\n");

	/* FSR */
	seq_printf(m, "     FSR:\n");
	seq_printf(m, "         Port A (sel 0~31):");
	for (i = 0; i < 32; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[5].data[i][0]);

	seq_printf(m, "\n         Port B (sel 0~31):");
	for (i = 0; i < 32; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[5].data[i][1]);

	seq_printf(m, "\n");

	/* VRF WP0 */
	seq_printf(m, "     VRF WP0:\n");
	seq_printf(m, "         Port A (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[8].data[i][0]);

	seq_printf(m, "\n         Port B (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[8].data[i][1]);

	seq_printf(m, "\n");

	/* VRF WP2 */
	seq_printf(m, "     VRF WP2:\n");
	seq_printf(m, "         Port A (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[9].data[i][0]);

	seq_printf(m, "\n         Port B (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[9].data[i][1]);

	seq_printf(m, "\n");

	/* VPPIPE WP0 */
	seq_printf(m, "     VPPIPE WP0:\n");
	seq_printf(m, "         Port A (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[10].data[i][0]);

	seq_printf(m, "\n         Port B (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[10].data[i][1]);

	seq_printf(m, "\n");

	/* VPPIPE WP2 */
	seq_printf(m, "     VPPIPE WP2:\n");
	seq_printf(m, "         Port A (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[11].data[i][0]);

	seq_printf(m, "\n         Port B (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[11].data[i][1]);

	seq_printf(m, "\n");

	/* SMU WP0 */
	seq_printf(m, "     SMU WP0:\n");
	seq_printf(m, "         Port A (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[12].data[i][0]);

	seq_printf(m, "\n         Port B (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[12].data[i][1]);

	seq_printf(m, "\n");

	/* SMU WP2 */
	seq_printf(m, "     SMU WP2:\n");
	seq_printf(m, "         Port A (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[13].data[i][0]);

	seq_printf(m, "\n         Port B (sel 0~7):");
	for (i = 0; i < 8; i++)
		seq_printf(m, " %08X", sdbg_info->debug_reg[13].data[i][1]);

	seq_printf(m, "\n");

}

void gpad_dfd_dump_areg_all_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int row, col, index;
	index = 0x0000;
	seq_printf(m, "==== Module MFG    ( 0x0000 ~ 0x0FFF ) ====\n");
	for (row = 0; row < 256; row++) {
		seq_printf(m, "0x%04X ", index);
		for (col = 0; col < 4; col++)
			seq_printf(m, "%08X ", sdbg_info->areg_dump.mfg[row*4+col]);

		index += 0x010;
		seq_printf(m, "\n");
	}

	seq_printf(m, "==== Module AREG   ( 0x1000 ~ 0x1FFF ) ====\n");
	for (row = 0; row < 256; row++) {
		seq_printf(m, "0x%04X ", index);
		for (col = 0; col < 4; col++)
			seq_printf(m, "%08X ", sdbg_info->areg_dump.areg[row*4+col]);

		index += 0x010;
		seq_printf(m, "\n");
	}
	seq_printf(m, "==== ModuleUX_CMN ( 0x2000 ~ 0x2FFF ) ====\n");
	for (row = 0; row < 256; row++) {
		seq_printf(m, "0x%04X ", index);
		for (col = 0; col < 4; col++)
			seq_printf(m, "%08X ", sdbg_info->areg_dump.ux_cmn[row*4+col]);

		index += 0x010;
		seq_printf(m, "\n");
	}
	seq_printf(m, "==== Module UX_DWP ( 0x3000 ~ 0x3FFF ) ====\n");
	for (row = 0; row < 256; row++) {
		seq_printf(m, "0x%04X ", index);
		for (col = 0; col < 4; col++)
			seq_printf(m, "%08X ", sdbg_info->areg_dump.ux_dwp[row*4+col]);

		index += 0x010;
		seq_printf(m, "\n");
	}
	seq_printf(m, "==== Module MX_DBG ( 0x4000 ~ 0x4FFF ) ====\n");
	for (row = 0; row < 256; row++) {
		seq_printf(m, "0x%04X ", index);
		for (col = 0; col < 4; col++)
			seq_printf(m, "%08X ", sdbg_info->areg_dump.mx_dbg[row*4+col]);

		index += 0x010;
		seq_printf(m, "\n");
	}
	seq_printf(m, "==== Module MMU ( 0x5000 ~ 0x5FFF ) ====\n");
	for (row = 0; row < 256; row++) {
		seq_printf(m, "0x%04X ", index);
		for (col = 0; col < 4; col++)
			seq_printf(m, "%08X ", sdbg_info->areg_dump.mmu[row*4+col]);

		index += 0x010;
		seq_printf(m, "\n");
	}

}

void gpad_dfd_dump_freg_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int row;

	seq_printf(m, "==== QREG (0x1E08) ====\n");
	for (row = 0; row < 26; row++) {
		seq_printf(m, "rd_offset(0x%04X) => ", row << 2);
		seq_printf(m, "%08x\n", sdbg_info->freg_dump.qreg[row]);
	}
	seq_printf(m, "==== RSMINFO (0x1E0C) ====\n");
	for (row = 0; row < 69; row++) {
		seq_printf(m, "rd_offset(0x%04X) => ", row << 2);
		seq_printf(m, "%08X\n", sdbg_info->freg_dump.rsminfo[row]);
	}
	seq_printf(m, "==== VPFMREG (0x1E10) ====\n");
	for (row = 0; row < 254; row++) {
		seq_printf(m, "rd_offset(0x%04X) => ", row << 2);
		seq_printf(m, "%08X\n", sdbg_info->freg_dump.vpfmreg[row]);
	}
	seq_printf(m, "==== PPFMREG (0x1E14) ====\n");
	for (row = 0; row < 254; row++) {
		seq_printf(m, "rd_offset(0x%04X) => ", row << 2);
		seq_printf(m, "%08X\n", sdbg_info->freg_dump.ppfmreg[row]);
	}

}

void _dbg_ff_regset_select(struct seq_file *m, unsigned int top_sel)
{
	switch (top_sel) {
	case 0:
		seq_printf(m, "==== G3D_STALL ====\n");
		break;
	case 1:
		seq_printf(m, "==== PA ====\n");
		break;
	case 3:
		seq_printf(m, "==== VL2PA_VP0 ====\n");
		break;
	case 5:
		seq_printf(m, "==== CF2PA_VP0 ====\n");
		break;
	case 6:
		seq_printf(m, "==== SF2PA_VP0 ====\n");
		break;
	case 8:
		seq_printf(m, "==== VL2PA_VP1 ====\n");
		break;
	case 10:
		seq_printf(m, "==== BW2PA_VP1 ====\n");
		break;
	case 11:
		seq_printf(m, "==== VX2PA_VP1 ====\n");
		break;
	case 13:
		seq_printf(m, "==== VX2PA_PP ====\n");
		break;
	case 14:
		seq_printf(m, "==== CB2PA_PP ====\n");
		break;
	case 15:
		seq_printf(m, "==== SB2PA_PP ====\n");
		break;
	case 16:
		seq_printf(m, "==== XY2PA_PP ====\n");
		break;
	case 18:
		seq_printf(m, "==== CP2PA_PP ====\n");
		break;
	case 19:
		seq_printf(m, "==== ZP2PA_PP ====\n");
		break;
	case 20:
		seq_printf(m, "==== TX2PA_PP ====\n");
		break;
	case 22:
		seq_printf(m, "==== CC2PA_PP ====\n");
		break;
	case 23:
		seq_printf(m, "==== ZC2PA_PP ====\n");
		break;
	case 24:
		seq_printf(m, "==== MX2PA_ENG ====\n");
		break;
	case 25:
		seq_printf(m, "==== MX2PA_CAC ====\n");
		break;
	case 26:
		seq_printf(m, "==== PM2PA ====\n");
		break;
	}
}

void _dbg_cmn_regset_select(struct seq_file *m, unsigned int top_sel)
{
	switch (top_sel) {
	case 0:
		seq_printf(m, "==== CXM ====\n");
		break;
	case 1:
		seq_printf(m, "==== TKS ====\n");
		break;
	case 2:
		seq_printf(m, "==== CP ====\n");
		break;
	case 3:
		seq_printf(m, "==== LSU ====\n");
		break;
	case 5:
		seq_printf(m, "==== VPL1 ====\n");
		break;
	case 6:
		seq_printf(m, "==== ISC ====\n");
		break;
	case 7:
		seq_printf(m, "==== FSI ====\n");
		break;
	case 8:
		seq_printf(m, "==== VS0I ====\n");
		break;
	case 9:
		seq_printf(m, "==== VS1I ====\n");
		break;
	case 10:
		seq_printf(m, "==== FSR ====\n");
		break;
	case 11:
		seq_printf(m, "==== VS0R ====\n");
		break;
	case 12:
		seq_printf(m, "==== VS1R ====\n");
		break;
	}
}

void gpad_dfd_dump_ff_debug_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int top, sub;

	for (top = 0; top < 27; top++) {
		/*Skip null register set*/
		if (top == 2 || top == 4 || top == 7 || top == 9 ||
			top == 12 || top == 17 || top ==  21)
			continue;
		_dbg_ff_regset_select(m, top);
		for (sub = 0; sub < 49; sub++) {
			seq_printf(m, "0x11C4(0x%08X) => ", (sub << 12) | (top << 4) | 0x1);
			seq_printf(m, "%08X\n", sdbg_info->ff_debug_dump.data[top][sub]);
		}
	}
}

void gpad_dfd_dump_ux_cmn_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info)
{
	unsigned int id, sel;

	for (id = 0; id < 13; id++) {
		/*Skip null register set*/
		if (id == 4)
			continue;

		_dbg_cmn_regset_select(m, id);
		seq_printf(m, "PortA:\n");
		for (sel = 0; sel < 8; sel++) {
			seq_printf(m, "     0x2048(0x%08X) => ", (sel << 16) | id);
			seq_printf(m, "%08X\n", sdbg_info->cmnreg_dump[id].data[0][sel]);
		}
		seq_printf(m, "\n");

		seq_printf(m, "PortB:\n");
		for (sel = 0; sel < 8; sel++) {
			seq_printf(m, "     0x2050(0x%08X) => ", (sel << 16) | id);
			seq_printf(m, "%08X\n", sdbg_info->cmnreg_dump[id].data[1][sel]);
		}
		seq_printf(m, "\n");
	}
}
