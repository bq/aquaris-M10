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

#ifndef _PMT_H
#define _PMT_H

#include "partition_define.h"
#define MAX_PARTITION_NAME_LEN 64
/*64bit*/
struct pt_resident {
	char name[MAX_PARTITION_NAME_LEN];	/* partition name */
	unsigned long long size;	/* partition size */
	unsigned long long part_id;	/* partition region */
	unsigned long long offset;	/* partition start */
	unsigned long long mask_flags;	/* partition flags */

};
/*32bit*/
struct pt_resident32 {
	char name[MAX_PARTITION_NAME_LEN];	/* partition name */
	unsigned long size;	/* partition size */
	unsigned long offset;	/* partition start */
	unsigned long mask_flags;	/* partition flags */

};

#define DM_ERR_OK 0
#define DM_ERR_NO_VALID_TABLE 9
#define DM_ERR_NO_SPACE_FOUND 10
#define ERR_NO_EXIST  1


#define PT_SIG      0x50547633	/* "PTv3" */
#define MPT_SIG    0x4D505433	/* "MPT3" */
#define PT_SIG_SIZE 4
#define is_valid_pt(buf) (!memcmp(buf, "3vTP", 4))
#define is_valid_mpt(buf) (!memcmp(buf, "3TPM", 4))
#define RETRY_TIMES 5

struct MBR_EBR_struct {
	char xbr_name[8];
	int part_index[4];
};

#define XBR_COUNT 8

struct DM_PARTITION_INFO_x {
	char part_name[MAX_PARTITION_NAME_LEN];	/* the name of partition */
	unsigned long long part_id;	/* the region of partition */
	unsigned long long start_addr;	/* the start address of partition */
	unsigned long long part_len;	/* the length of partition */
	unsigned char visible;	/* part_visibility is 0: this partition is hidden and CANNOT download */
	/* part_visibility is 1: this partition is visible and can download */
	unsigned char dl_selected;	/* dl_selected is 0: this partition is NOT selected to download */
	/* dl_selected is 1: this partition is selected to download */
};

struct DM_PARTITION_INFO_PACKET_x {
	unsigned int pattern;
	unsigned int part_num;	/* The actual number of partitions */
	struct DM_PARTITION_INFO_x part_info[PART_MAX_COUNT];
};

struct pt_info {
	int sequencenumber:8;
	int tool_or_sd_update:8;
	int mirror_pt_dl:4;	/* mirror download OK */
	int mirror_pt_has_space:4;
	int pt_changed:4;
	int pt_has_space:4;
};

extern int eMMC_rw_x(loff_t addr, u32 *buffer, int host_num, int iswrite, u32 totalsize,
		     int transtype, enum Region part);
extern void msdc_check_init_done(void);

#endif
