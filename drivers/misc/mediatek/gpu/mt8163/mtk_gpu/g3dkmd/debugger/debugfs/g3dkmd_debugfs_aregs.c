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

#include "g3dkmd_debugfs_aregs.h"
#include "g3dkmd_engine.h"	/* g3dKmdRegGet...() */
#include "sapphire_reg.h"	/* G3D_REG_MAX */




/*****************************************************************************
 * Areg Dump by debugfs
 *****************************************************************************/


static struct debugfs_blob_wrapper areg_blob = { 0 };

void g3dKmd_debugfs_Areg_create(struct dentry *reg_dir)
{
	areg_blob.data = (void *)g3dKmdRegGetG3DRegister();
	areg_blob.size = G3D_REG_MAX;

	debugfs_create_blob("ARegs", S_IRUSR, reg_dir, &areg_blob);
}

void g3dKmd_debugfs_Areg_destroy(void)
{

}
