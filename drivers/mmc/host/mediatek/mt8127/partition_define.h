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

#ifndef __PARTITION_DEFINE_H__
#define __PARTITION_DEFINE_H__

extern int PART_NUM;

#define PART_MAX_COUNT			 40

#define WRITE_SIZE_Byte		512
enum dev_type {
	EMMC = 1,
	NAND = 2,
};

enum Region {
	EMMC_PART_UNKNOWN =
		0, EMMC_PART_BOOT1, EMMC_PART_BOOT2, EMMC_PART_RPMB, EMMC_PART_GP1, EMMC_PART_GP2,
	EMMC_PART_GP3, EMMC_PART_GP4, EMMC_PART_USER, EMMC_PART_END
};

struct excel_info {
	char *name;
	unsigned long long size;
	unsigned long long start_address;
	enum dev_type type;
	unsigned int partition_idx;
	enum Region region;
};

extern struct excel_info PartInfo[PART_MAX_COUNT];

#endif
