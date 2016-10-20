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

#ifndef _GPAD_PM_H
#define _GPAD_PM_H

#include "gpad_pm_sample.h"
#include "gpad_ioctl.h"

#define GPAD_PM_DEF_CONFIG  0
#define GPAD_PM_CUST_CONFIG 10000
#define GPAD_PM_CONFIG_REG_CNT  8

struct gpad_pm_sample_list {
	struct list_head        node;
	struct gpad_pm_sample   sample;
	unsigned long long      timestamp_ns;
	unsigned int            id;
	int                     freq;
};

/*!
 * GPU performance statistics.
 */
struct gpad_pm_stat {
	/*
	 * Counters to be reset while refreshing statistics.
	 */
	unsigned int            passed_intvl;
	unsigned int            busy_time;
	unsigned int            total_time;
	unsigned int            vertex_cnt;
	unsigned int            triangle_cnt;
	unsigned int            pixel_cnt;
	unsigned int            frame_cnt;

	/*
	 * Counters to be kept even statistics refresh.
	 */
	unsigned int            read_samples;
	unsigned int            last_frame_id;
	unsigned long long      acc_busy_ns;
	unsigned long long      acc_total_ns;
};

/*!
 * Register to configure GPU performance monitor.
 */
struct gpad_pm_reg {
	unsigned int offset;
	unsigned int mask;
	unsigned int value;
};

/*!
 * GPU performance monitor data structure.
 */
struct gpad_pm_info {
	spinlock_t              free_sample_lock; /**< Lock for free_sample_list. Note that, the locking sequence is free_sample_lock -> used_sample_lock. */
	struct list_head        free_sample_list; /**< List of invalid PM samples. */

	spinlock_t              used_sample_lock; /**< Lock for used_sample_list. */
	struct list_head        used_sample_list; /**< List of valid PM samples. */

	spinlock_t              hw_buffer_lock;   /**< Lock for hw buffer related members. */
	void                   *hw_buffer_desc;   /**< Descriptor to the HW buffer. */
	unsigned char          *hw_buffer_addr;   /**< Kernel logical address of the HW buffer. */
	unsigned int            hw_buffer_idx;    /**< Index to the sample to check and read from H/W. */
	unsigned int            interval;         /**< Polling interval, in unit of ms. */
	unsigned long           intvl_j;          /**< Polling interval, in unit of jiffies. */
	unsigned int            samples_alloc;    /**< Number of PM samples allocated. */
	struct timer_list       hw_buffer_timer;  /**< Timer to poll HW buffer. */
	unsigned long long      last_cpu_ts;      /**< CPU timestamp of last sample polled, in unit of ns. */
	unsigned int            last_gpu_ts;      /**< GPU timestamp of last sample polled, in unt if 1/GPU core frequency. */
	int                     unsync_intvl;     /**< Time passed since last sample polled, in unit of ms. */
	int                     to_restart_ts;    /**< 1 to use next sample's timestamp as base. */
	int                     gpu_freq;         /**< GPU clock rate in HZ. */

	unsigned int            config;           /**< Index to a set of PM counters. */
	struct gpad_pm_stat     stat;             /**< Statistics. */
	unsigned int            bak_reg_cnt;      /**< Number of elements used in bak_reg[]. */
	struct gpad_pm_reg      bak_reg[GPAD_PM_CONFIG_REG_CNT]; /**< Registers to configure PM, only for customized configuration. */
};

struct gpad_device;

int gpad_pm_init(struct gpad_device *dev);
void gpad_pm_exit(struct gpad_device *dev);
void gpad_pm_enable(struct gpad_device *dev);
void gpad_pm_force_enable(struct gpad_device *dev, int reset_to_default);
void gpad_pm_disable(struct gpad_device *dev);
enum gpad_pm_state gpad_pm_get_state(struct gpad_device *dev);
unsigned int gpad_pm_get_config(struct gpad_device *dev);
int gpad_pm_set_config(struct gpad_device *dev, unsigned int config);
unsigned int gpad_pm_get_timestamp(void);

struct gpad_pm_sample_list *gpad_pm_read_sample_list(struct gpad_device *dev);
void gpad_pm_complete_sample_list(struct gpad_device *dev, struct gpad_pm_sample_list *sample_list);
void gpad_pm_change_freq(struct gpad_device *dev, int gpu_freq);

/*
 * @param cmdStr          command
 * @param cmdStrLen       command string length
 * @param buffer          buffer
 * @param bufferLen       total size of buffer
 * @param bufferDataLen   size of input data in the buffer
 * @retval size of output data in the buffer, last byte '\0' included
 */
int g3dkmd_kQueryAPI(const char *cmdStr, const unsigned int cmdStrLen, char *buffer, const unsigned int bufferLen, const unsigned int bufferDataLen);
#endif /* _GPAD_PM_H */
