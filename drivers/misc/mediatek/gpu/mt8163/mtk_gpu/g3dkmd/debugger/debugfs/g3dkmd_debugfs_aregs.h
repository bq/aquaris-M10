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

#ifndef _G3DKMD_DEBUGFS_AREGS_H_
#define _G3DKMD_DEBUGFS_AREGS_H_
#include <linux/debugfs.h>



/* register to debugfs creation function. */
void g3dKmd_debugfs_Areg_create(struct dentry *reg_dir);

/* clean up function. */
void g3dKmd_debugfs_Areg_destroy(void);

#endif /* _G3DKMD_DEBUGFS_AREGS_H_ */
