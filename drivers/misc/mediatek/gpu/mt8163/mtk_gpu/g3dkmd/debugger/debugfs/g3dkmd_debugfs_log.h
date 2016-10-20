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

#ifndef _G3DKMD_DEBUGFS_LOG_H_
#define _G3DKMD_DEBUGFS_LOG_H_
#include <linux/debugfs.h>

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dbase_type.h"
#include "g3dkmd_define.h"

#if 0
/* Log level controller callbacks
 * members
 *   _getFlagsPtr    : get a pointer to DPF type flags control variable
 *   _getLogLevelPtr : get a pointer to DPF Log level control variable
 */
/*
typedef struct _debugfs_log_callbacks {
	G3Duint *(*_getFlagsPtr) (void);
	G3Duint *(*_getLogLevelPtr) (void);
} G3d_debugfs_log_callbacks_t, *pG3d_debugfs_log_callbacks_t;
*/
#endif

/* G3dKmd_debugfs_Log_create()
 * Description:
 *   register to debugfs creation function.
 *
 * Inputs:
 *   p_dir      : the node of parrent folder
 */
void g3dKmd_debugfs_Log_create(struct dentry *reg_dir);

/* g3dKmd_debugfs_Log_destroy()
 * Description:
 *   the clean-up function.
 */
void g3dKmd_debugfs_Log_destroy(void);

#endif /* _G3DKMD_DEBUGFS_Log_H_ */
