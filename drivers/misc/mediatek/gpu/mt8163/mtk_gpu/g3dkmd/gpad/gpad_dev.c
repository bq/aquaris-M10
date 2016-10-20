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
#include <linux/err.h>  /* for IS_ERR, PTR_ERR */
#include <linux/slab.h> /* for memory allocation */

#include "gpad_common.h"

static struct class *gpad_class_s;    /* GPAD class */
static dev_t         gpad_first_devno_s; /* First number assigned by alloc_chrdev_region. */

const struct file_operations  gpad_fops = {
	.owner          = THIS_MODULE, .open           = gpad_dev_open, .release        = gpad_dev_release, .unlocked_ioctl = gpad_dev_ioctl, };

int __init gpad_dev_init(void)
{
	int err;
	gpad_class_s = NULL;    /* GPAD class */
	gpad_first_devno_s = 0; /* First number assigned by alloc_chrdev_region. */

	/*
	 * Create and register a class (check /sys/class/gpad/).
	 */
	gpad_class_s = class_create(
					   THIS_MODULE, /* owner */
					   GPAD_NAME);  /* name */
	if (unlikely(IS_ERR(gpad_class_s))) {
		err =  PTR_ERR(gpad_class_s);
		GPAD_LOG_ERR("Failed to create GPAD class! error=%d\n", err);
		gpad_class_s = NULL;
		return err;
	}
	GPAD_LOG_INFO("GPAD class created: %p\n", gpad_class_s);

	/*
	 * Create and register a cdev cccupying a range of minors (check /proc/devices/).
	 */
	err = alloc_chrdev_region(
			  &gpad_first_devno_s, /* output parameter for first assigned number. */
			  GPAD_BASE_DEV_MINOR, /* baseminor */
			  GPAD_MAX_DEV_CNT,    /* count */
			  GPAD_NAME);          /* name */
	if (unlikely(err < 0)) {
		GPAD_LOG_ERR("Failed to allocate char dev region for GPAD! err=%d\n", err);
		gpad_first_devno_s = 0;
		gpad_dev_exit();
		return err;
	}
	GPAD_LOG_INFO("Allocated char dev region for %d GPAD device(s). first devno: 0x%X\n",   GPAD_MAX_DEV_CNT, gpad_first_devno_s);
	return 0;
}

void gpad_dev_exit(void)
{
	if (likely(gpad_first_devno_s)) {
		dev_t devno = gpad_first_devno_s;
		gpad_first_devno_s = 0;

		unregister_chrdev_region(gpad_first_devno_s, GPAD_MAX_DEV_CNT);
		GPAD_LOG("Unregistered chr dev region for %d GPAD device(s). first devno: 0x%X\n",  GPAD_MAX_DEV_CNT, devno);
	}

	if (likely(gpad_class_s)) {
		class_destroy(gpad_class_s);
		gpad_class_s = NULL;
		GPAD_LOG("Destroyed GPAD class\n");
	}
}

struct gpad_device *gpad_dev_create(void)
{
	static int          id; /* only support on device */
	struct gpad_device *dev;
	dev_t               devno;
	int                 err;
	struct device      *device;
	id = 0;
	/*
	 * Allocate memory for a gpad device.
	 */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (unlikely(!dev)) {
		GPAD_LOG_ERR("Failed to allocate %zd-byte gpad_dev for device-%d!\n", sizeof(*dev), id);
		err = -ENOMEM;
		goto gpad_dev_create_error;
	}
	GPAD_LOG_INFO("Allocated %zd-byte gpad_dev for a device-%d: %p\n", sizeof(*dev), id, dev);

	/*
	 * Fill in the char dev structure and add it to system.
	 */
	cdev_init(&dev->cdev,  /* struct cdev *cdev */
			  &gpad_fops); /* struct file_operations */
	dev->cdev.owner = THIS_MODULE;

	devno = MKDEV(MAJOR(gpad_first_devno_s), id);
	err = cdev_add(
			  &dev->cdev, /* the cdev structure for the device */
			  devno,      /* the first device number for which this device is responsible */
			  1);         /* the number of consecutive minor numbers corresponding to this device */
	if (unlikely(err)) {
		GPAD_LOG_ERR("Failed to add a cdev! error=%d, devno=%X. major=%d, minor=%d\n",  err, devno, MAJOR(gpad_first_devno_s), id);
		goto gpad_dev_create_error;
	}
	GPAD_LOG_INFO("Added a cdev (%p). devno=%X. major=%d, minor=%d\n",   &dev->cdev, devno, MAJOR(gpad_first_devno_s), id);

	/*
	 * Create and register a device.
	 */
	device = device_create(
				 gpad_class_s, /* class  */
				 NULL,       /* parent */
				 devno,      /* the dev_t for the char device to be added */
				 NULL,       /* the data to be added to the device for callbacks */
				 "%s%d", GPAD_NAME, id); /* string for the device's name */
	if (unlikely(IS_ERR(device))) {
		err = PTR_ERR(device);
		GPAD_LOG_ERR("Failed to create gpad_dev for device-%d, error=%d!\n", id, err);
		goto gpad_dev_create_error;
	}
	dev->device = device;

	GPAD_LOG_INFO("Created gpad_dev (%p) for device-%d\n", device, id);
	return dev;

gpad_dev_create_error:
	gpad_dev_destroy(dev);
	return ERR_PTR(err);
}

void gpad_dev_destroy(struct gpad_device *dev)
{
	if (likely(dev && !IS_ERR(dev))) {
		int devno = dev->cdev.dev;

		if (dev->device) {
			device_destroy(gpad_class_s, devno);
			dev->device = NULL;
		}

		if (devno) {
			cdev_del(&dev->cdev);
			dev->cdev.dev = 0;
		}

		kfree(dev);

		GPAD_LOG_INFO("Destroyed gpad_dev(%p) for device-%d\n", dev, MINOR(devno));
	}
}

int gpad_dev_open(struct inode *inode, struct file *file)
{
	int                 id = iminor(inode);
	struct gpad_device *dev = gpad_find_dev(id);

	if (unlikely((!dev || IS_ERR(dev)))) {
		GPAD_LOG_ERR("Failed to device-%d (%p)!\n", id, dev);
		return -ENODEV;
	}

	if (unlikely(test_and_set_bit(GPAD_DEV_OPEN, &dev->status))) {
		GPAD_LOG_LOUD("device-%d has been opened.\n", id);
		return -EBUSY;
	}

	if (unlikely(!try_module_get(THIS_MODULE))) {
		GPAD_LOG_ERR("Failed in try_module_get(), stop opening device-%d!\n", id);
		clear_bit(GPAD_DEV_OPEN, &dev->status);
		return -ENODEV;
	}

	file->private_data = (void *)dev;

	/*GPAD_LOG_INFO("device-%d is opened\n", id); */
	return nonseekable_open(inode, file);
}

int gpad_dev_release(struct inode *inode, struct file *file)
{
	/*int                 id = iminor(inode); */
	struct gpad_device *dev = (struct gpad_device *)file->private_data;

	file->private_data = NULL;
	clear_bit(GPAD_DEV_OPEN, &dev->status);
	module_put(THIS_MODULE);

	/*GPAD_LOG_INFO("device-%d is closed.\n", id); */
	return 0;
}
