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

#ifndef _GPAD_GPUFREQ_UTS_H
#define _GPAD_GPUFREQ_UTS_H

#include "mt_gpufreq.h"
#include "gs_gpufreq.h"

struct gpad_device;

/*!
 * GPUFREQ unit test entry.
 * @param dev    Handle of the device.
 * @param param  Test parameter from user space.
 *
 * @return 0 if succeeded, error code otherwise.
 */
long gpad_gpufreq_utst(struct gpad_device *dev, int param);

#endif /* _GPAD_DVFS_UTS_H */







