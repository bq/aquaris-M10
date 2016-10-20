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

#include "gpad_common.h"
#include "gpad_proc.h"
#include "gpad_hal.h"
#include "gpad_pm.h"
#include "gpad_sdbg_dump.h"

#if defined(GPAD_SUPPORT_DVFS)
#include "gpad_ipem.h"
#endif /* defined(GPAD_SUPPORT_DVFS) */

static struct gpad_device *gpad_dev0_s; /* GPAD device instance. */

static int __init gpad_init(void)
{
	int                 err;
	int                 dev_init = 0;
	struct gpad_device *dev = NULL;
	int                 proc_init = 0;
	int                 hal_open = 0;
	int                 pm_init = 0;
	int                 sdbg_init = 0;
#if defined(GPAD_SUPPORT_DVFS)
	int                 ipem_init = 0;
#endif /* GPAD_SUPPORT_DVFS */
#if defined(GPAD_SUPPORT_DFD)
	int                 dfd_init = 0;
#endif /* GPAD_SUPPORT_DFD */

	GPAD_LOG_LOUD("GPAD module initializing...%p\n", THIS_MODULE);

	/*
	 * Do char device initialization, such as class registration and char device region allocation.
	 */
	err = gpad_dev_init();
	if (unlikely(err < 0)) {
		GPAD_LOG_ERR("gpad_dev_init() failed with %d!\n", err);
		goto gpad_init_error;
	}
	dev_init = 1;

	/*
	 * Create first device.
	 */
	dev = gpad_dev_create();
	if (unlikely(IS_ERR(dev))) {
		err =  PTR_ERR(dev);
		GPAD_LOG_ERR("gpad_dev_create() failed with %d\n", err);
		goto gpad_init_error;
	}
	gpad_dev0_s = NULL;
	gpad_dev0_s = dev;

	/*
	 * Create proc files.
	 */
	gpad_proc_init();
	proc_init = 1;

	/*
	 * Register with graphic kernel module.
	 */
	err = gpad_hal_open();
	if (unlikely(err)) {
		GPAD_LOG_ERR("gpad_hal_open() failed with %d\n", err);
		goto gpad_init_error;
	}
	hal_open = 1;

	/*
	 * Initialize functional components.
	 */
	err = gpad_pm_init(dev);
	if (unlikely(err)) {
		GPAD_LOG_ERR("gpad_pm_init() failed with %d\n", err);
		goto gpad_init_error;
	}
	pm_init = 1;


	err = gpad_sdbg_init(dev);
	if (unlikely(err)) {
		GPAD_LOG_ERR("gpad_sdbg_init() failed with %d\n", err);
		goto gpad_init_error;
	}
	sdbg_init = 1;

#if defined(GPAD_SUPPORT_DVFS)
	err = gpad_ipem_init(dev);
	if (unlikely(err)) {
		GPAD_LOG_ERR("gpad_ipem_init() failed with %d\n", err);
		goto gpad_init_error;
	}
	ipem_init = 1;
#endif /* GPAD_SUPPORT_DVFS */

#if defined(GPAD_SUPPORT_DFD)
	err = gpad_dfd_init(dev);
	if (unlikely(err)) {
		GPAD_LOG_ERR("gpad_dfd_init() failed with %d\n", err);
		goto gpad_init_error;
	}
	dfd_init = 1;
#endif /* GPAD_SUPPORT_DFD */

	GPAD_LOG_INFO("GPAD module initialized\n");
	return 0;

gpad_init_error:
#if defined(GPAD_SUPPORT_DVFS)
	if (ipem_init)
		gpad_ipem_exit(dev);

#endif /* GPAD_SUPPORT_DVFS */

#if defined(GPAD_SUPPORT_DFD)
	if (dfd_init)
		gpad_dfd_exit(dev);

#endif /* GPAD_SUPPORT_DVFS */

	if (sdbg_init)
		gpad_sdbg_exit(dev);


	if (pm_init)
		gpad_pm_exit(dev);


	if (hal_open)
		gpad_hal_close();


	if (proc_init)
		gpad_proc_exit();


	if (dev) {
		gpad_dev0_s = NULL;
		gpad_dev_destroy(dev);
	}

	if (dev_init)
		gpad_dev_exit();


	return err;
}

static void __exit gpad_exit(void)
{
	struct gpad_device *dev;

	GPAD_LOG_LOUD("GPAD module exiting...%p\n", THIS_MODULE);

	if (gpad_dev0_s) {
		dev = gpad_dev0_s;
		gpad_dev0_s = NULL;

#if defined(GPAD_SUPPORT_DFD)
		gpad_dfd_exit(dev);
#endif /* GPAD_SUPPORT_DFD */

#if defined(GPAD_SUPPORT_DVFS)
		gpad_ipem_exit(dev);
#endif /* GPAD_SUPPORT_DVFS */

		gpad_sdbg_exit(dev);
		gpad_pm_exit(dev);

		gpad_hal_close();
		gpad_proc_exit();

		gpad_dev_destroy(dev);
	}

	gpad_dev_exit();

	GPAD_LOG_INFO("GPAD module existed\n");
}

struct gpad_device *gpad_get_default_dev(void)
{
	return gpad_dev0_s;
}

module_init(gpad_init);
module_exit(gpad_exit);

MODULE_DESCRIPTION("GPU Performance Analysis and Debugging Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MediaTek Inc.");
