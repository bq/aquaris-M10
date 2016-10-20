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

#ifndef _GPAD_IOCTL_H
#define _GPAD_IOCTL_H

#include <linux/ioctl.h>
#include "gpad_pm_sample.h"

#define GPAD_IOC_CMN         0xe0   /**< General purpose category. */
#define GPAD_IOC_PM          0xe1   /**< Performnace Monitor related. */
#define GPAD_IOC_SDBG        0xe2   /**< Shader Debugger related. */
#define GPAD_IOC_GPUFREQ     0xe3   /**< GPU Frequency framework related. */
#define GPAD_IOC_IPEM        0xe4   /**< Integrated Power and Energy Management related. */
#define GPAD_IOC_DFD         0xe5   /**< DFD related. */
#define GPAD_IOC_G3DSHELL    0xe6   /**< g3dshell related. */

/*!
 * Query information about this module.
 */
#define GPAD_IOC_GET_MOD_INFO \
	_IOW(GPAD_IOC_CMN, 0, struct gpad_mod_info)

/*!
 * Read value from specified register.
 */
#define GPAD_IOC_READ_REG \
	_IOWR(GPAD_IOC_CMN, 1, struct gpad_reg_rw)

/*!
 * Write value to specifed register.
 */
#define GPAD_IOC_WRITE_REG \
	_IOWR(GPAD_IOC_CMN, 2, struct gpad_reg_rw)

/*!
 * Toggle one bit of specified register.
 */
#define GPAD_IOC_TOGGLE_REG \
	_IOR(GPAD_IOC_CMN, 3, struct gpad_reg_toggle)

/*!
 * Query GPU Performance Monitor current state.
 */
#define GPAD_IOC_PM_GET_STATE \
	_IOW(GPAD_IOC_PM, 0, enum gpad_pm_state)

/*!
 * Change GPU Performance Monitor current state.
 */
#define GPAD_IOC_PM_SET_STATE \
	_IOR(GPAD_IOC_PM, 1, enum gpad_pm_state)

/*!
 * Query current timestamp.
 */
#define GPAD_IOC_PM_GET_TIMESTAMP \
	_IOW(GPAD_IOC_PM, 2, unsigned int)

/*!
 * Query GPU Performance Monitor configuration.
 */
#define GPAD_IOC_PM_GET_CONFIG \
	_IOW(GPAD_IOC_PM, 3, unsigned int)

/*!
 * Change GPU Performance Monitor configuration.
 */
#define GPAD_IOC_PM_SET_CONFIG \
	_IOR(GPAD_IOC_PM, 4, unsigned int)

/*!
 * Query GPU Performance Monitor counters polling interval, in unit of ms.
 */
#define GPAD_IOC_GET_INTERVAL \
	_IOW(GPAD_IOC_PM, 5, unsigned int)

/*!
 * Change GPU Performance Monitor counters polling interval, in unit of ms.
 */
#define GPAD_IOC_SET_INTERVAL \
	_IOR(GPAD_IOC_PM, 6, unsigned int)

/*!
 * Read and revmove GPU Performance Monitor counters polled.
 */
#define GPAD_IOC_PM_READ_COUNTERS \
	_IOWR(GPAD_IOC_PM, 7, struct gpad_buf_read)

/*!
 * Change output pin.
 */
#define GPAD_IOC_CHANGE_OUTPUT_PIN \
	_IOWR(GPAD_IOC_PM, 8, struct gpad_outpin_req)

/*!
 * Dump debug information with specific level.
 */
#define GPAD_IOC_DUMP_INFO \
	_IOWR(GPAD_IOC_SDBG, 0, unsigned int)

#define GPAD_IOC_SDBG_UPDATE_KEY \
	_IOWR(GPAD_IOC_SDBG, 1, unsigned int)

#define GPAD_IOC_SDBG_READ_MEM \
	_IOWR(GPAD_IOC_SDBG, 2, unsigned int)

#define GPAD_IOC_SDBG_WRITE_MEM \
	_IOWR(GPAD_IOC_SDBG, 3, unsigned int)

/*!
 * Trigger gpu frequncy unit test.
 */
#define GPAD_IOC_GPUFREQ_UTST \
	_IOWR(GPAD_IOC_GPUFREQ, 0, unsigned int)

/*!
 * Trigger IPEM unit test.
 */
#define GPAD_IOC_IPEM_UTST \
	_IOWR(GPAD_IOC_IPEM, 1, unsigned int)

/*!
 * Trigger DFD DEBUG MODE
 */
#define GPAD_IOC_DFD_DEBUG_MODE \
	_IOWR(GPAD_IOC_DFD, 0, unsigned int)

/*!
 * Trigger DFD DUMP SRAM
 */
#define GPAD_IOC_DFD_DUMP_SRAM \
	_IOWR(GPAD_IOC_DFD, 1, unsigned int)

/*!
 * Polling for debug bus output.
 */
#define GPAD_IOC_DFD_RECEV_POLL \
	_IOWR(GPAD_IOC_DFD, 2, struct gpad_recev_poll)

/*!
 * Set mask which DFD care
 */
#define GPAD_IOC_DFD_SET_MASK \
	_IOWR(GPAD_IOC_DFD, 3, unsigned int)

/*!
 * g3dshell query
 */
#define GPAD_IOC_KMD_QUERY \
	_IOWR(GPAD_IOC_G3DSHELL, 0, struct gpad_kmd_query)

#define GPAD_PA_ADDR_TYPE  (0)
#define GPAD_VA_ADDR_TYPE  (1)

/*!
 * Defines information about this kenel module.
 */
struct gpad_mod_info {
	unsigned int    version; /**< Version of this module. */
};

/*!
 * Defines parameters used for register read/write IOCTL.
 */
struct gpad_reg_rw {
	unsigned int    offset; /**< Offset of the register to read/write. */
	unsigned int    size;   /**< Number bytes to read or write: 1, 2, 4. */
	unsigned int    value;  /**< Value read or value to write. */
};

/*!
 * Defines parameters to toggle one bit of register.
 */
struct gpad_reg_toggle {
	unsigned int    offset; /**< Offset of the register to read/write. */
	unsigned int    bit;    /**< Bit to toggle, from 0 to 31. */
};

/*!
 * GPU Performance Monitor state.
 */
enum gpad_pm_state {
	GPAD_PM_DISABLED = 0, GPAD_PM_ENABLED  = 1, };

/*!
 * Defines GPU Performance Monitor signals selected.
 */
enum gpad_pm_sig_config {
	GPAD_PM_SIGNAL_DEFAULT = 0, };

/*!
 * Defines GPU Performance Monitor sampling point.
 */
enum gpad_pm_sample_type {
	GPAD_PM_SAMPLE_SEGMENT = 0, GPAD_PM_SAMPLE_SUBFRAME = 1, GPAD_PM_SAMPLE_FRAME = 2, };

/*!
 * Defines a block of buffer Performance Monitor.
 */
struct gpad_buf_read {
	unsigned int    bytes_alloc; /**< Size of the buffer immediate after this struct to accommodate data. */
	unsigned int    bytes_read;  /**< Number of bytes read. */
};

/*!
 * Defines operations to an output pin.
 */
enum gpad_outpin_act {
	GPAD_OUTPIN_ACT_NONE = 0, GPAD_OUTPIN_ACT_PULSE = 1, GPAD_OUTPIN_ACT_UP = 2, GPAD_OUTPIN_ACT_DOWN = 3, };

/*!
 * Output pin control request.
 */
struct gpad_outpin_req {
	unsigned int            pin; /**< Index of the output pin. */
	enum gpad_outpin_act    act; /**< Action to take. */
};

/*!
 * Information about a sGPU performance counters sample.
 */
struct gpad_pm_counters {
	unsigned long long      timestamp_ns; /**< Timestamp of the sample, in unit of ns. */
	struct gpad_pm_sample   sample;       /**< Information of read from GPU. */
	int                     freq;         /**< GPU clock rate in HZ. */
};

struct gpad_recev_poll {
	unsigned int    type;
	unsigned int    data[3];
};

struct gpad_kmd_query {
	unsigned int cmd_offset;
	unsigned int buf_offset;
	int   cmd_len; /**< last char '\0' included >*/
	int   buf_len;
	int   arg_len; /**< offset is as same as buf >*/
};

struct gpad_mem_ctx {
	unsigned int    magic;
	unsigned int    addr;
	unsigned int    value;
	unsigned short  size;
	unsigned short  type;
};

#endif /* _GPAD_IOCTL_H */
