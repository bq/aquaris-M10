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

#ifndef _GPAD_DEF_H
#define _GPAD_DEF_H

/*
 * This file defines configuable settings of the kernel modules.
 */

#define GPAD_VERSION        0x01000300  /**< Version of this module. */
#define GPAD_NAME           "gpad"      /**< Name of GPAD device/class/driver */
#define GPAD_BASE_DEV_MINOR 0           /**< First of the requested range of minor number. */
#define GPAD_MAX_DEV_CNT    1           /**< Max number of GPAD devices */

#define GPAD_PM_DEFAULT_POOL_INTERVAL   100   /**< Default PM polling interval, in unit of ms. */
#define GPAD_PM_MAX_SAMPLE_PER_SEC      28800 /**< Max PM samples per seconds. */
#define GPAD_PM_MAX_UNSYNC_INTERVAL     1000  /**< Max interval of unsync period, in unit of ms. */
#define GPAD_PM_STAT_REFRESH_INTERVAL   4000  /**< Interval to refresh statistics, in unit of ms. */

#define DUMP_MINI_LEVEL                 0     /**Define mini dump level value*/
#define DUMP_CORE_LEVEL                 1     /**Define core dump level value */
/*#define DUMP_INST_LEVEL               2 */
#define DUMP_VERBOSE_LEVEL              3     /**Define verbose level value */
#define DUMP_DBGREG_LEVEL               10    /**Define debug dump level value */


#define GPAD_GPU_VER                    0x0801
#define GPAD_ON_FPGA                    0

#if GPAD_ON_FPGA
#define GPAD_GPU_DEF_FREQ           6000000
#define GPAD_CHECKING_HW            0
#else
#define GPAD_GPU_DEF_FREQ           380250000
#define GPAD_CHECKING_HW            0
#endif

#endif /* _GPAD_DEF_H */
