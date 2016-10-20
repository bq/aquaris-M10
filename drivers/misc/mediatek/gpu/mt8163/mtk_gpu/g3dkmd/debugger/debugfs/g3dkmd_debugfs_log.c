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

#include <linux/debugfs.h>


#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#include "g3dkmd_debugfs_log.h"
#include "g3dkmd_file_io.h"	/* for KMDDPF() */




/*****************************************************************************
 * Areg Dump by debugfs
 *****************************************************************************/

void g3dKmd_debugfs_Log_create(struct dentry *klog_dir)
{
	struct dentry *kmddpf_dir;

	kmddpf_dir = debugfs_create_dir("KMDDPF", klog_dir);

	debugfs_create_x32("logfilter", (S_IWUGO | S_IRUGO | S_IWUSR),
			   kmddpf_dir, g3dKmd_GetKMDDebugFlagsPtr());
	debugfs_create_u32("loglevel", (S_IWUGO | S_IRUGO | S_IWUSR),
			   kmddpf_dir, g3dKmd_GetKMDDebugLogLevelPtr());
}

void g3dKmd_debugfs_Log_destroy(void)
{

}
