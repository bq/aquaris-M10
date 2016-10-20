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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h> /* for memory allocation */
#include <linux/sched.h> /* for cpu_clock */
#include "gpad_common.h"
#include "gpad_hal.h"
#include "gpad_sdbg.h"
#include "gpad_sdbg_reg.h"
#ifdef GPAD_SDBG_ITST
#include "gpad_pm.h"
#endif

#define MAX_INST_SIZE           (100)
#define SDBG_CONTEXT_NUM        (6)

static struct gpad_sdbg_info *s_sdbg_info;
static void _save_instruction_single_context(unsigned int i, unsigned int max_size);
static void _save_greg(void);
static void _save_areg_dump(void);
static void _save_freg_dump(void);
static void _save_ff_debug_dump(void);
static void _save_cmnreg_dump(void);
static void _save_ux_debug_register(void);
static void _save_inst(void);
static void _save_wave_info(void);
static int _prefetch_instruction_block(unsigned int cid, unsigned int pc_block);
static int _is_data_ready(unsigned int addr, unsigned int bit);
static int _toggle_register(unsigned int offset, unsigned int bit);

int gpad_sdbg_init(void *dev_ptr)
{
	struct gpad_device *dev = (struct gpad_device *)dev_ptr;
	int idx;

	s_sdbg_info = &(dev->sdbg_info);

	for (idx = 0; idx < SDBG_CONTEXT_NUM; idx++) {
		s_sdbg_info->inst[idx].size = 0;
		s_sdbg_info->inst[idx].data = (unsigned int *)kzalloc(sizeof(unsigned int) * MAX_INST_SIZE, GFP_KERNEL);
		if (unlikely(NULL == s_sdbg_info->inst[idx].data)) {
			GPAD_LOG_ERR("Allocate memory failed: sdbg_info.inst[%d]\n", idx);
			return 1;
		}
	}
	return 0;
}

void gpad_sdbg_exit(void *dev_ptr)
{
	struct gpad_device *dev = (struct gpad_device *)dev_ptr;
	struct gpad_sdbg_info  *sdbg_info = &(dev->sdbg_info);
	int idx;

	if (NULL == sdbg_info)
		return;

	for (idx = 0; idx < SDBG_CONTEXT_NUM; idx++) {
		if (NULL == sdbg_info->inst[idx].data)
			continue;
		kfree(sdbg_info->inst[idx].data);
	}
}

void gpad_dfd_save_debug_info(void)
{
	if (NULL == s_sdbg_info)
		return;


	_save_areg_dump();
	_save_freg_dump();
	_save_ff_debug_dump();
	_save_cmnreg_dump();
	_save_ux_debug_register();

}

void gpad_sdbg_save_sdbg_info(unsigned int level)
{
#ifdef GPAD_SDBG_ITST
# define GPAD_SDBG_INFO_SECTION_NUM  (7)
	unsigned int ts[GPAD_SDBG_INFO_SECTION_NUM];
	int i = 0;

	ts[i++] = gpad_pm_get_timestamp();
#endif
	if (NULL == s_sdbg_info)
		return;

	s_sdbg_info->dump_timestamp_start = cpu_clock(0);

	/* areg stat */
	GPAD_LOG_LOUD("%s: save areg stat\n", __func__);
	s_sdbg_info->areg.irq_raw_data = gpad_hal_read32(SDBG_AREG_IRQ_RAW);
	s_sdbg_info->areg.pa_idle_status0 = gpad_hal_read32(SDBG_AREG_PA_IDLE_STATUS0);
	s_sdbg_info->areg.pa_idle_status1 = gpad_hal_read32(SDBG_AREG_PA_IDLE_STATUS1);
	gpad_hal_write8(SDBG_AREG_DEBUG_MODE_SEL, 0x1);
	s_sdbg_info->areg.g3d_stall_bus = gpad_hal_read32(SDBG_AREG_DEBUG_BUS);
#ifdef GPAD_SDBG_ITST
	ts[i++] = gpad_pm_get_timestamp();
#endif

	/* cmnreg */
	GPAD_LOG_LOUD("%s: save cmnreg stat\n", __func__);
	s_sdbg_info->cmnreg.lsu_err_maddr   = gpad_hal_read32(SDBG_REG_CMN_LSU_ERR_MADDR);
	s_sdbg_info->cmnreg.cpl1_err_maddr  = gpad_hal_read32(SDBG_REG_CMN_CPL1_ERR_MADDR);
	s_sdbg_info->cmnreg.isc_err_maddr   = gpad_hal_read32(SDBG_REG_CMN_ISC_ERR_MADDR);
	s_sdbg_info->cmnreg.cprf_err_info   = gpad_hal_read32(SDBG_REG_CMN_CPRF_ERR_INFO);
	s_sdbg_info->cmnreg.stage_io        = gpad_hal_read32(SDBG_REG_CMN_STAGE_IO);
	s_sdbg_info->cmnreg.tx_ltc_io       = gpad_hal_read32(SDBG_REG_CMN_TX_LTC_IO);
	s_sdbg_info->cmnreg.isc_io          = gpad_hal_read32(SDBG_REG_CMN_ISC_IO);
	s_sdbg_info->cmnreg.idle_status     = gpad_hal_read32(SDBG_REG_CMN_IDLE_STATUS);
	s_sdbg_info->cmnreg.type_wave_cnt   = gpad_hal_read32(SDBG_REG_CMN_TYPE_WAVE_CNT);
	s_sdbg_info->cmnreg.wave_stop_info  = gpad_hal_read32(SDBG_REG_CMN_STOP_WAVE);
	s_sdbg_info->cmnreg.vs0i_sid        = gpad_hal_read32(SDBG_REG_CMN_VS0i_SID);
	s_sdbg_info->cmnreg.vs1i_sid        = gpad_hal_read32(SDBG_REG_CMN_VS1i_SID);
	s_sdbg_info->cmnreg.fsi_sid         = gpad_hal_read32(SDBG_REG_CMN_FSi_SID);
	s_sdbg_info->cmnreg.wp_wave_cnt     = gpad_hal_read32(SDBG_REG_CMN_WP_WAVE_CNT);
	s_sdbg_info->cmnreg.wp0_lsm_rem     = gpad_hal_read32(SDBG_REG_CMN_WP0_LSM_REM);
	s_sdbg_info->cmnreg.wp0_vrf_crf_rem = gpad_hal_read32(SDBG_REG_CMN_WP0_VRF_CRF_REM);
	s_sdbg_info->cmnreg.wp2_lsm_rem     = gpad_hal_read32(SDBG_REG_CMN_WP2_LSM_REM);
	s_sdbg_info->cmnreg.wp2_vrf_crf_rem = gpad_hal_read32(SDBG_REG_CMN_WP2_VRF_CRF_REM);
#ifdef GPAD_SDBG_ITST
	ts[i++] = gpad_pm_get_timestamp();
#endif

	/* dwpreg */
	GPAD_LOG_LOUD("%s: save dwpreg stat\n", __func__);
	s_sdbg_info->dwpreg.status_report[0] = gpad_hal_read32(SDBG_REG_DWP_STATUS_REPORT0);
	s_sdbg_info->dwpreg.status_report[1] = gpad_hal_read32(SDBG_REG_DWP_STATUS_REPORT1);
	s_sdbg_info->dwpreg.status_report[2] = gpad_hal_read32(SDBG_REG_DWP_STATUS_REPORT2);
	s_sdbg_info->dwpreg.status_report[3] = gpad_hal_read32(SDBG_REG_DWP_STATUS_REPORT3);
#ifdef GPAD_SDBG_ITST
	ts[i++] = gpad_pm_get_timestamp();
#endif

	/* global register */
	GPAD_LOG_LOUD("%s: save greg stat\n", __func__);
	_save_greg();
#ifdef GPAD_SDBG_ITST
	ts[i++] = gpad_pm_get_timestamp();
#endif
	/* debug register */
	if (level == DUMP_CORE_LEVEL || level == DUMP_VERBOSE_LEVEL)
		_save_ux_debug_register();
	if (level == DUMP_VERBOSE_LEVEL) {
		/* instruction */
		GPAD_LOG_LOUD("%s: save instruction\n", __func__);
		_save_inst();
#ifdef GPAD_SDBG_ITST
		ts[i++] = gpad_pm_get_timestamp();
#endif

		/* Dump wave information if read wave is enabled */
		s_sdbg_info->wave_readable_flag = gpad_sdbg_is_wave_info_readable();

		if (unlikely(s_sdbg_info->wave_readable_flag != 0)) {
			GPAD_LOG_LOUD("%s: save wave info\n", __func__);
			_save_wave_info();
		}
#ifdef GPAD_SDBG_ITST
		ts[i++] = gpad_pm_get_timestamp();
#endif

#ifdef GPAD_SDBG_ITST
		GPAD_LOG_INFO("ts start: %u\n", ts[0]);
		for (i = 1; i < GPAD_SDBG_INFO_SECTION_NUM; i++) {
			const char *sec[GPAD_SDBG_INFO_SECTION_NUM-1]
				= {"areg", "cmn", "dwp", "greg", "inst", "wave"};
			GPAD_LOG_INFO("ts %s: %u\n", sec[i-1], ts[i]);
		}
#endif
	}
	s_sdbg_info->dump_timestamp_end = cpu_clock(0);
}

void _save_greg(void)
{
	unsigned int idx;
	const unsigned int ctx[6] = {0, 1, 2, 3, 6, 7};

	for (idx = 0; idx < 6; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, ctx[idx]);
		s_sdbg_info->greg.inst_base[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 6; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0x10 | ctx[idx]);
		s_sdbg_info->greg.cnst_dsc_mem_base[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 6; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0x20 | ctx[idx]);
		s_sdbg_info->greg.cnst_mem_base[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 4; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0x30 | ctx[idx]);
		s_sdbg_info->greg.grp_mem_base[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 4; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0x40 | ctx[idx]);
		s_sdbg_info->greg.prv_mem_base[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 6; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0x50 | ctx[idx]);
		s_sdbg_info->greg.inst_last_pc[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 4; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0x60 | ctx[idx]);
		s_sdbg_info->greg.fs_out_z_base[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 4; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0x70 | ctx[idx]);
		s_sdbg_info->greg.data1[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 4; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0x80 | ctx[idx]);
		s_sdbg_info->greg.data2[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 6; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0x90 | ctx[idx]);
		s_sdbg_info->greg.inst_mem_limit[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 6; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0xa0 | ctx[idx]);
		s_sdbg_info->greg.cnst_mem_limit[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 4; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0xb0 | ctx[idx]);
		s_sdbg_info->greg.grp_mem_limit[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 4; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0xc0 | ctx[idx]);
		s_sdbg_info->greg.prv_mem_limit[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}

	for (idx = 0; idx < 10; idx++) {
		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0x100 | (idx << 4));
		s_sdbg_info->greg.freg_data[idx] = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
	}
}

void _save_inst(void)
{
	unsigned int i;
	const unsigned int ctx[6] = {0, 1, 2, 3, 6, 7};

	for (i = 0; i < 6; i++) {
		unsigned int size;
		unsigned int cid = ctx[i];

		if (unlikely(NULL == s_sdbg_info->inst[i].data))
			return;

		gpad_hal_write32(SDBG_REG_CMN_CXM_GLOBAL_CTRL, 0x50 | cid);
		size = gpad_hal_read32(SDBG_REG_CMN_CXM_GLOBAL_DATA);
		if (unlikely(size > MAX_INST_SIZE))
			size = MAX_INST_SIZE;
		_save_instruction_single_context(i, size);
	}
}

void _save_instruction_single_context(unsigned int idx, unsigned int max_size)
{
	unsigned int req, pc_block, cache_addr, count;
	int eop = 0, bk;
	const unsigned int ctx[6] = {0, 1, 2, 3, 6, 7};
	unsigned int cid = ctx[idx];

	count = pc_block = 0;

	/* get last pc value */
	s_sdbg_info->inst[idx].size = 0;

	while (unlikely(0 == eop && count < max_size)) {
		if (unlikely(0 != _prefetch_instruction_block(cid, pc_block))) {
			GPAD_LOG_ERR("ctx %d prefetch instruction timeout\n", cid);
			break;
		}

		/* 2 x 4 64-bit data are prefetched. start from cache_addr. */
		cache_addr = gpad_hal_read32(SDBG_REG_CMN_INST_PREFETCH) >> 24;

		/* read instruction data (4 x 128 bit for each prefetch) */
		for (bk = 0; bk < 4 && eop == 0; bk++) {
			int sel;

			/* 128-bit data in each bank */
			for (sel = 0; sel < 2 && eop == 0; sel++) {
				/* get 64-bit data at a time */
				req  = (sel << 2) | (bk << 4) | (cache_addr << 8);
				gpad_hal_write32(SDBG_REG_CMN_INST_READ, req);

				/* toggle bit 1 */
				if (unlikely(_toggle_register(SDBG_REG_CMN_INST_READ, 1))) {
					eop = 1;
					break;
				}

				if (unlikely(1 != _is_data_ready(SDBG_REG_CMN_INST_READ, 31))) {
					eop = 1;
					break;
				}

				s_sdbg_info->inst[idx].data[count++] = gpad_hal_read32(SDBG_REG_CMN_INST_DATA_LOW);

				if (unlikely(count >= max_size)) {
					eop = 1;
					break;
				}

				s_sdbg_info->inst[idx].data[count++] = gpad_hal_read32(SDBG_REG_CMN_INST_DATA_HIGH);

				if (unlikely(count >= max_size)) {
					eop = 1;
					break;
				}
			}
		}

		/* release instruction*/
		req = gpad_hal_read32(SDBG_REG_CMN_INST_RELEASE);
		req |= (cache_addr << 8);
		gpad_hal_write32(SDBG_REG_CMN_INST_RELEASE, req);

		/* toggle bit 0 */
		_toggle_register(SDBG_REG_CMN_INST_RELEASE, 1);

		pc_block++;
	}
	s_sdbg_info->inst[idx].size = count;
}

int _prefetch_instruction_block(unsigned int cid, unsigned int pc_block)
{
	unsigned int value;

	value = 0x1 | (pc_block << 4) | (cid << 20);
	gpad_hal_write32(SDBG_REG_CMN_INST_PREFETCH, value);

	/* toggle bit 0 */
	if (unlikely(_toggle_register(SDBG_REG_CMN_INST_PREFETCH, 1)))
		return -1;

	if (unlikely(1 != _is_data_ready(SDBG_REG_CMN_INST_PREFETCH, 23)))
		return -1;
	else
		return 0;
}

int _toggle_register(unsigned int offset, unsigned int bit)
{
# define GPAD_SDBG_TOGGLE_REGISTER_TIMEOUT (10)
	int count;
	unsigned int value, dummy;

	count = 0;
	value = gpad_hal_read32(offset);
	gpad_hal_write32(offset, value ^ (0x1 << bit));

	do {
		dummy = gpad_hal_read32(offset);
		count++;

		if (value != dummy) {
			gpad_hal_write32(offset, value);
			/* GPAD_LOG_INFO("%s: read offset 0x%08X bit %2u for %d times.\n"
			 * , __FUNCTION__, offset, bit, count);
			 */
			return 0;
		}

	} while (count <= GPAD_SDBG_TOGGLE_REGISTER_TIMEOUT);

	GPAD_LOG_INFO("%s: toggle offset 0x%08X bit %u timeout.\n", __func__, offset, bit);
	return -1;
}

int _is_data_ready(unsigned int addr, unsigned int bit)
{
# define GPAD_SDBG_DUMP_READ_TIMEOUT (1000)
	int count;
	unsigned int value, mask;

#if defined(GPAD_SHIM_ON)
	return 1;
#endif /* defined(GPAD_SHIM_ON) */

	value = 0;
	mask = 0x1 << bit;
	count  = 0;

	do {
		value = gpad_hal_read32(addr);
		count++;
		if (0 != (value & mask)) {
			/* GPAD_LOG_INFO("is_data_ready: addr 0x%08X bit %2u: read %d time(s)\n", addr, bit, count); */
			return 1;
		}

	} while (count <= GPAD_SDBG_DUMP_READ_TIMEOUT);

	GPAD_LOG_INFO("is_data_ready: addr 0x%08X bit %u: read timeout\n", addr, bit);
	return 0;
}

int gpad_sdbg_is_wave_info_readable(void)
{
	unsigned int value;

	value = gpad_hal_read32(SDBG_REG_DWP_WAVE_INFO);
	if (likely((value & 0x1) == 0))
		return 0;

	value = gpad_hal_read32(SDBG_REG_DWP_ADVDBG);
	if (likely((value & 0x80) == 0))
		return 0;

	value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_A_SET);
	if (likely((value & 0x1) == 0))
		return 0;

	value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_B_SET);
	if (likely((value & 0x1) == 0))
		return 0;

	return 1;
}

void _save_ux_debug_register(void)
{
	unsigned int req, value, sel, advdbg_value;
	unsigned int id, wid, wpcnt, wpid;
	bool reset_advdbg = false;
	advdbg_value = gpad_hal_read32(SDBG_REG_DWP_ADVDBG);
	if (likely((advdbg_value & 0x80) == 0)) {
		reset_advdbg = true;
		gpad_hal_write32(SDBG_REG_DWP_ADVDBG, 0x80);
	}
	/* SFU */
	for (sel = 0; sel < 8; sel++) {
		req = 0x0 | (sel << 16);
		/* Port A */
		gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_A_SET, req);
		value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_A_GET);
		s_sdbg_info->debug_reg[0].data[sel][0] = value;

		/* Port B */
		gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_B_SET, req);
		value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_B_GET);
		s_sdbg_info->debug_reg[0].data[sel][1] = value;
	}

	/* IFB */
	for (wpcnt = 0; wpcnt < 2; wpcnt++) {
		wpid = wpcnt * 2;
		for (wid = 0; wid < 12; wid++) {
			for (sel = 0; sel < 10; sel++) {
				req = 0x1 | (wid << 24) | (wpid << 28) | (sel << 16);

				gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_A_SET, req);
				value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_A_GET);
				s_sdbg_info->debug_wave_info[wpcnt*12+wid].data[sel][0] = value;

				gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_B_SET, req);
				value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_B_GET);
				s_sdbg_info->debug_wave_info[wpcnt*12+wid].data[sel][1] = value;
			}
			/* Sel 10 */
			/* Port A */
			req = 0x1 | (wid << 24) | (wpid << 28) | 0x100000;
			gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_A_SET, req);
			value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_A_GET);
			s_sdbg_info->debug_wave_info[wpcnt*12+wid].data[sel][0] = value;
			/* Port B */
			gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_B_SET, req);
			value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_B_GET);
			s_sdbg_info->debug_wave_info[wpcnt*12+wid].data[sel][1] = value;
		}
	}

	/* VS0R, VS1R, FSR */
	for (id = 2; id < 6; id++) {
		for (sel = 0; sel < 8; sel++) {
			if (sel == 4) /* 0x04:none */
				continue;
			req = (sel << 16) | id;
			/* Port A */
			gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_A_SET, req);
			value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_A_GET);
			s_sdbg_info->debug_reg[id].data[sel][0] = value;
			/* Port B */
			gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_B_SET, req);
			value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_B_GET);
			s_sdbg_info->debug_reg[id].data[sel][1] = value;
		}
	}

	/* VRF, VPPIPE, SMU in both WP0 and WP2 */
	for (id = 8; id < 14; id++) {
		for (sel = 0; sel < 32; sel++) {
			req = (sel << 16) | id;
			/* Port A */
			gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_A_SET, req);
			value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_A_GET);
			s_sdbg_info->debug_reg[id].data[sel][0] = value;
			/* Port B */
			gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_B_SET, req);
			value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_B_GET);
			s_sdbg_info->debug_reg[id].data[sel][1] = value;
		}
	}
	if (reset_advdbg)
		gpad_hal_write32(SDBG_REG_DWP_ADVDBG, advdbg_value);



}

void _save_wave_info(void)
{
	unsigned int wpcnt, wpid, wid;
	unsigned int req, value, i;
	/* IFB */
	for (wpcnt = 0; wpcnt < 2; wpcnt++) {
		wpid = wpcnt * 2;
		for (wid = 0; wid < 12; wid++) {
			for (i = 0; i < 10; i++) {
				req = 0x1 | (wid << 24) | (wpid << 28) | (i << 16);

				gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_A_SET, req);
				value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_A_GET);
				s_sdbg_info->wave_stat[wpcnt*12+wid].data[i][0] = value;

				gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_B_SET, req);
				value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_B_GET);
				s_sdbg_info->wave_stat[wpcnt*12+wid].data[i][1] = value;
			}
			/* Sel 10 */
			/* Port A */
			req = 0x1 | (wid << 24) | (wpid << 28) | 0x100000;
			gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_A_SET, req);
			value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_A_GET);
			s_sdbg_info->wave_stat[wpcnt*12+wid].data[i][0] = value;
			/* Port B */
			gpad_hal_write32(SDBG_REG_DWP_EU_STATUS_B_SET, req);
			value = gpad_hal_read32(SDBG_REG_DWP_EU_STATUS_B_GET);
			s_sdbg_info->wave_stat[wpcnt*12+wid].data[i][1] = value;
		}
	}
}

void _save_areg_dump(void)
{
	unsigned int i, areg_en_value, cmn_dbg_value, adv_value;
	bool areg_en_reset = false;
	bool cmn_dbg_reset = false;
	bool adv_reset = false;
	areg_en_value = gpad_hal_read32(DFD_AREG_REG_EN);
	if (likely((areg_en_value & 0XF) == 0)) {
		gpad_hal_write32(DFD_AREG_REG_EN, 0XF);
		areg_en_reset = true;
	}

	cmn_dbg_value = gpad_hal_read32(SDBG_REG_CMN_DEBUG_SET);
	if (likely((cmn_dbg_value & 0X80000000) == 0)) {
		gpad_hal_write32(SDBG_REG_CMN_DEBUG_SET, 0X80000000);
		cmn_dbg_reset = true;
	}

	adv_value = gpad_hal_read32(SDBG_REG_DWP_ADVDBG);
	if (likely((adv_value & 0X80) == 0)) {
		gpad_hal_write32(SDBG_REG_DWP_ADVDBG, 0X80);
		adv_reset = true;
	}

	/*MFG*/
	for (i = 0; i < 1024; i++)
		s_sdbg_info->areg_dump.mfg[i] =  gpad_hal_read32(SDBG_MFG_BASE | (i*4));

	/*AREG*/
	for (i = 0; i < 1024; i++)
		s_sdbg_info->areg_dump.areg[i] =  gpad_hal_read32(SDBG_AREG_BASE | (i*4));

	/*UC_CMN*/
	for (i = 0; i < 1024; i++)
		s_sdbg_info->areg_dump.ux_cmn[i] =  gpad_hal_read32(SDBG_REG_CMN_BASE | (i*4));

	/*UX_DWP*/
	for (i = 0; i < 1024; i++)
		s_sdbg_info->areg_dump.ux_dwp[i] =  gpad_hal_read32(SDBG_REG_DWP_BASE | (i*4));

	/*MX_DBG*/
	for (i = 0; i < 1024; i++)
		s_sdbg_info->areg_dump.mx_dbg[i] =  gpad_hal_read32(SDBG_MX_DBG_BASE | (i*4));

	/*MMU*/
	for (i = 0; i < 1024; i++)
		s_sdbg_info->areg_dump.mmu[i] =  gpad_hal_read32(SDBG_MMU_BASE | (i*4));


	if (areg_en_reset)
		gpad_hal_write32(DFD_AREG_REG_EN, areg_en_value);


	if (cmn_dbg_reset)
		gpad_hal_write32(SDBG_REG_CMN_DEBUG_SET, cmn_dbg_value);


	if (adv_reset)
		gpad_hal_write32(SDBG_REG_DWP_ADVDBG, adv_value);

}

void _save_freg_dump(void)
{
	unsigned int value, i, offset;
	bool reset = false;
	offset = 0;
	/*Enable FREQ DUMP*/
	value = gpad_hal_read32(DFD_AREG_REG_EN);
	if (likely((value & 0XF) == 0)) {
		gpad_hal_write32(DFD_AREG_REG_EN, 0XF);
		reset = true;
	}

	/*Read QREG*/
	for (i = 0; i < 26; i++) {
		offset = i << 2;
		gpad_hal_write32(DFD_AREG_RD_OFFSET, offset);
		value = gpad_hal_read32(DFD_AREG_QREG_RD);
		s_sdbg_info->freg_dump.qreg[i] = value;
	}
	/*Read RSMINFO*/
	for (i = 0; i < 69; i++) {
		offset = i << 2;
		gpad_hal_write32(DFD_AREG_RD_OFFSET, offset);
		value = gpad_hal_read32(DFD_AREG_RSMINFO_RD);
		s_sdbg_info->freg_dump.rsminfo[i] = value;
	}
	/*Read VPFMREG*/
	for (i = 0; i < 254; i++) {
		offset = i << 2;
		gpad_hal_write32(DFD_AREG_RD_OFFSET, offset);
		value = gpad_hal_read32(DFD_AREG_VPFMREG_RD);
		s_sdbg_info->freg_dump.vpfmreg[i] = value;
	}
	/*Read PPFMREG*/
	for (i = 0; i < 254; i++) {
		offset = i << 2;
		gpad_hal_write32(DFD_AREG_RD_OFFSET, offset);
		value = gpad_hal_read32(DFD_AREG_PPFMREG_RD);
		s_sdbg_info->freg_dump.ppfmreg[i] = value;
	}
	/*Disable FREG Dump*/
	if (reset)
		gpad_hal_write32(DFD_AREG_REG_EN, value);

}

void _save_ff_debug_dump(void)
{
	unsigned int value, req, top, sub;
	for (top = 0; top < 27; top++) {
		for (sub = 0; sub < 49; sub++) {
			req = (sub << 12) | (top << 4) | 0x1;
			gpad_hal_write32(SDBG_AREG_DEBUG_MODE_SEL , req);
			value = gpad_hal_read32(SDBG_AREG_DEBUG_BUS);
			s_sdbg_info->ff_debug_dump.data[top][sub] = value;
		}
	}
}

void _save_cmnreg_dump(void)
{
	unsigned int value, id, sel, req;
	bool reset = false;
	value = gpad_hal_read32(SDBG_REG_CMN_DEBUG_SET);
	if (likely((value & 0X80000000) == 0)) {
		gpad_hal_write32(SDBG_REG_CMN_DEBUG_SET, 0X80000000);
		reset = true;
	}

	for (id = 0; id < 13; id++) {
		for (sel = 0; sel < 8; sel++) {
			req = (sel << 16) | id;
			/* Port A */
			gpad_hal_write32(SDBG_REG_CMN_STATUS_REPORT0 , req);
			value = gpad_hal_read32(SDBG_REG_CMN_STATUS_REPORT1);
			s_sdbg_info->cmnreg_dump[id].data[0][sel] = value;
			/* Port B */
			gpad_hal_write32(SDBG_REG_CMN_STATUS_REPORT2 , req);
			value = gpad_hal_read32(SDBG_REG_CMN_STATUS_REPORT3);
			s_sdbg_info->cmnreg_dump[id].data[1][sel] = value;

		}
	}
	if (reset)
		gpad_hal_write32(SDBG_REG_CMN_DEBUG_SET, value);


}
