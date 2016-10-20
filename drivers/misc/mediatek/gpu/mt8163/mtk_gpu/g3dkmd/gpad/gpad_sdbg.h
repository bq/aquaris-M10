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

#ifndef _GPAD_SDBG_H
#define _GPAD_SDBG_H


#define SDBG_VERSION "0.2.0"

/**
 * shader dump areg info
 */
struct gpad_sdbg_areg {
	unsigned int irq_raw_data;
	unsigned int pa_idle_status0;
	unsigned int pa_idle_status1;
	unsigned int g3d_stall_bus;
};

/*
 * shader dump common register info
 */
struct gpad_sdbg_cmnreg {
	unsigned int lsu_err_maddr;
	unsigned int cpl1_err_maddr;
	unsigned int isc_err_maddr;
	unsigned int cprf_err_info;
	unsigned int stage_io;
	unsigned int tx_ltc_io;
	unsigned int isc_io;
	unsigned int idle_status;
	unsigned int type_wave_cnt;
	unsigned int wave_stop_info;
	unsigned int vs0i_sid;
	unsigned int vs1i_sid;
	unsigned int fsi_sid;
	unsigned int wp_wave_cnt;
	unsigned int wp0_lsm_rem;
	unsigned int wp0_vrf_crf_rem;
	unsigned int wp2_lsm_rem;
	unsigned int wp2_vrf_crf_rem;
};

/**
 * shader dump dualpool register info
 */
struct gpad_sdbg_dwpreg {
	unsigned int status_report[4];
};

/**
 * shader dump global register register info
 */
struct gpad_sdbg_greg {
	unsigned int inst_base[6];
	unsigned int cnst_dsc_mem_base[6];
	unsigned int cnst_mem_base[6];
	unsigned int grp_mem_base[4];
	unsigned int prv_mem_base[4];
	unsigned int inst_last_pc[6];
	unsigned int fs_out_z_base[4];
	unsigned int data1[4];
	unsigned int data2[4];
	unsigned int inst_mem_limit[6];
	unsigned int cnst_mem_limit[6];
	unsigned int grp_mem_limit[4];
	unsigned int prv_mem_limit[4];
	unsigned int freg_data[10];
};

/**
 * shader dump instruction info
 */
struct gpad_sdbg_inst {
	unsigned int *data;
	unsigned int  size;
};

/**
 * shader dump wave status
 */
struct gpad_sdbg_wave {
	unsigned int data[11][2];
};

/**
 * dump debug register
 */

struct gpad_sdbg_debug_reg {
	unsigned int data[32][2];
};


struct gpad_dfd_areg_dump {
	unsigned int mfg[1024];
	unsigned int areg[1024];
	unsigned int ux_cmn[1024];
	unsigned int ux_dwp[1024];
	unsigned int mx_dbg[1024];
	unsigned int mmu[1024];
};
struct gpad_dfd_freg_dump {
	unsigned int qreg[26];
	unsigned int rsminfo[69];
	unsigned int vpfmreg[254];
	unsigned int ppfmreg[254];
};
struct gpad_dfd_ff_debug_dump {
	unsigned int data[27][49];
};

struct gpad_cmnreg_dump {
	unsigned int data[2][8];
};

struct gpad_sdbg_info {
	struct gpad_sdbg_areg         areg;
	struct gpad_sdbg_cmnreg       cmnreg;
	struct gpad_sdbg_dwpreg       dwpreg;
	struct gpad_sdbg_greg         greg;
	struct gpad_sdbg_inst         inst[6];
	struct gpad_sdbg_wave         wave_stat[24];
	struct gpad_sdbg_wave         debug_wave_info[24];
	struct gpad_sdbg_debug_reg    debug_reg[14];
	struct gpad_dfd_areg_dump     areg_dump;
	struct gpad_dfd_freg_dump     freg_dump;
	struct gpad_dfd_ff_debug_dump ff_debug_dump;
	struct gpad_cmnreg_dump       cmnreg_dump[13];
	unsigned int                  wave_readable_flag;
	unsigned int                  level;
	unsigned long long            dump_timestamp_start;
	unsigned long long            dump_timestamp_end;
};

int gpad_sdbg_init(void *dev_ptr);
void gpad_sdbg_exit(void *dev_ptr);

/**
 * dump "mini","core","verbose" level information
 */
void gpad_sdbg_save_sdbg_info(unsigned int);
/**
 * dump "dfd" level information
 */
void gpad_dfd_save_debug_info(void);

int gpad_sdbg_is_wave_info_readable(void);

#endif /* #define _GPAD_SDBG_H */
