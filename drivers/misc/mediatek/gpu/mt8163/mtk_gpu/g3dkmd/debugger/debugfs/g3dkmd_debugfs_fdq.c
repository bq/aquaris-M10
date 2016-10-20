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

#include "g3dbase_type.h"

#include "g3dkmd_debugfs_fdq.h"
#include "g3dkmd_debugfs_helper.h"
#include "g3dkmd_fdq.h"



/*****************************************************************************
 * Fdq by debugfs
 *****************************************************************************/

static struct debugfs_blob_wrapper fdq_blob = { 0 };

static int __show_fdq_sw_ridx(struct seq_file *m, void *v)
{
	DEBUGFS_PRINT_LOG_OR_SEQ(m, "0x%x", _g3dkmd_getFdqReadIdx());
	return 0;
}

static int __show_fdq_hw_widx(struct seq_file *m, void *v)
{
	DEBUGFS_PRINT_LOG_OR_SEQ(m, "0x%x", _g3dkmd_getFdqWriteIdx());
	return 0;
}

void g3dKmd_debugfs_Fdq_create(struct dentry *reg_dir)
{
	fdq_blob.data = _g3dkmd_getFdqDataPtr();
	fdq_blob.size = _g3dkmd_getFdqDataSize();

	debugfs_create_blob("data", S_IRUSR, reg_dir, &fdq_blob);

	_g3dKmdCreateDebugfsFileNode("swReadIdx", reg_dir, __show_fdq_sw_ridx);
	_g3dKmdCreateDebugfsFileNode("hwWriteIdx", reg_dir, __show_fdq_hw_widx);
}

void g3dKmd_debugfs_Fdq_destroy(void)
{

}
