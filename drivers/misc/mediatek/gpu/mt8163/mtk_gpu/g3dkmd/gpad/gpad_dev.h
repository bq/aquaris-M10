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

#ifndef _GPAD_DEV_H
#define _GPAD_DEV_H

#include <linux/device.h>   /* for device */
#include <linux/cdev.h>     /* for cdev */
#include <linux/fs.h>       /* for file_operations, file */

#include "gpad_pm.h"
#include "gpad_sdbg_dump.h"
#if defined(GPAD_SUPPORT_DVFS)
#include "gpad_ipem.h"
#endif /* defined(GPAD_SUPPORT_DVFS) */
#if defined(GPAD_SUPPORT_DFD)
#include "gpad_dfd.h"
#endif /* defined(GPAD_SUPPORT_DFD) */

#define GPAD_DEV_OPEN           0
#define GPAD_PM_INIT            1
#define GPAD_PM_ENABLE          2
#define GPAD_PM_EN_PREV         3
#define GPAD_IPEM_EN_PREV       4

#define GPAD_CHECK_DEV_STATUS(_dev, _bit) \
			test_bit(_bit, &((_dev)->status))

#define GPAD_SET_DEV_STATUS(_dev, _bit) \
			set_bit(_bit, &((_dev)->status))

#define GPAD_CLEAR_DEV_STATUS(_dev, _bit) \
			clear_bit(_bit, &((_dev)->status))

/*!
 * GPU device context.
 */
struct gpad_device {
	unsigned long       status;
	struct device      *device;
	struct cdev         cdev;

	struct gpad_pm_info     pm_info; /**< Context of GPU performance monitor. */
	struct gpad_sdbg_info   sdbg_info;
#if defined(GPAD_SUPPORT_DVFS)
	struct gpad_ipem_info   ipem_info; /**< Context of GPU integrated power and energy management. */
#endif /* defined(GPAD_SUPPORT_DVFS) */
#if defined(GPAD_SUPPORT_DFD)
	struct gpad_dfd_info    dfd_info; /**< Context of GPU DVFS. */
#endif /* defined(GPAD_SUPPORT_DFD) */
	unsigned int        ext_id_reg;
};

int gpad_dev_init(void);
void gpad_dev_exit(void);
struct gpad_device *gpad_dev_create(void);
void gpad_dev_destroy(struct gpad_device *dev);
int gpad_dev_open(struct inode *inode, struct file *file);
int gpad_dev_release(struct inode *inode, struct file *file);
long gpad_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif /* _GPAD_DEV_H */
