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

#ifndef _GPAD_DVFS_UTS_H
#define _GPAD_DVFS_UTS_H

#define DVFS_UTS_RAND_LOOP     4

/*!
 * DVFS policy unit test entry.
 * @param dev    Handle of the device.
 * @param param  Test parameter from user space.
 *
 * @return 0 if succeeded, error code otherwise.
 */
long gpad_dvfs_utst(struct gpad_device *dev, int param);
#endif /* _GPAD_DVFS_UTS_H */
