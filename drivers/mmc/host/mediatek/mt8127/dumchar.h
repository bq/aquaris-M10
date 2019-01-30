/*
 * Copyright (c) 2014-2015 MediaTek Inc.
 * Author: Chaotian.Jing <chaotian.jing@mediatek.com>
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

#ifndef __DUMCHAR_H__
#define __DUMCHAR_H__


#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/mtd/mtd.h>
#include <linux/semaphore.h>
#include "sd_misc.h"

/*
 * Macros to help debugging
 */
#define DUMCHAR_DEBUG

#ifdef DUMCHAR_DEBUG
#define DDEBUG(fmt, args...) pr_dbg("dumchar_debug: " fmt, ## args)
#else
#define DDEBUG(fmt, args...)
#endif


#define DUMCHAR_MAJOR        0	/* dynamic major by default */
#define MAX_SD_BUFFER		(512)
#define ALIE_LEN		512

struct dumchar_dev {
	char *dumname;		/* nvram boot userdata */
	char actname[64];	/* full act name /dev/mt6573_sd0 /dev/mtd/mtd1    */
	struct semaphore sem;	/* Mutual exclusion */
	enum dev_type type;		/* nand device or emmc device */
	unsigned long long size;	/* partition size */
	struct cdev cdev;
	enum Region region;		/* for emmc */
	unsigned long long start_address;	/* for emmc */
	unsigned int mtd_index;	/* for nand */
};

struct Region_Info {
	enum Region region;
	unsigned long long size_Byte;
};

struct file_obj {
	struct file *act_filp;
	int index;		/* index in dumchar_dev arry */
};

#define REGION_NUM	8

#define	MSDC_RAW_DEVICE					"/dev/misc-sd"

#ifdef CONFIG_MTK_EMMC_SUPPORT

extern int simple_sd_ioctl_rw(struct msdc_ioctl *msdc_ctl);
extern int init_pmt(void);
#ifdef CONFIG_MTK_EMMC_CACHE
extern void msdc_get_cache_region(void);
#endif
#endif

#ifdef CONFIG_MTK_MTD_NAND
extern struct mtd_info *__mtd_next_device(int i);
#endif


#ifdef CONFIG_MTK_MTD_NAND
extern u64 mtd_partition_start_address(struct mtd_info *mtd);
#endif

#define mtd_for_each_device(mtd)			\
	for ((mtd) = __mtd_next_device(0);		\
	     (mtd) != NULL;				\
	     (mtd) = __mtd_next_device(mtd->index + 1))

#endif /*__DUMCHAR_H__ */
