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

#include "g3dkmd_debugfs_helper.h"
#include "g3dkmd_debugfs_profiler.h"
#include "g3dkmd_profiler.h"
#include "profiler/g3dkmd_prf_kmemory.h"

#include "g3dkmd_memory_list.h"

#if defined(G3D_KMD_MEMORY_STATISTIC)
static int kmemroy_usage_debugfs_seq_show(struct seq_file *m, void *v)
{
	_g3dKmdProfilerKMemoryUsageReport(m);
	return 0;
}
#endif

static int memroy_list_debugfs_seq_show(struct seq_file *seq, void *v)
{
	_g3dKmdMemList_ListAll(seq);
	return 0;
}


/*****************************************************************************
 * Operate profiler by debugfs
 *****************************************************************************/


void g3dKmd_debugfs_profiler_create(struct dentry *profiler_dir)
{
#if defined(G3D_KMD_MEMORY_STATISTIC) || defined(ENABLE_G3DMEMORY_LINK_LIST)
	struct dentry *memory_dir;
#endif

#if defined(G3D_KMD_MEMORY_STATISTIC) || defined(ENABLE_G3DMEMORY_LINK_LIST)
	/* Memory folder */
	memory_dir = debugfs_create_dir("Memory", profiler_dir);
#endif

#if defined(G3D_KMD_MEMORY_STATISTIC)
	/*log_level controller*/
	debugfs_create_u32("loglevel", (S_IWUGO | S_IRUGO | S_IWUSR), kmemory_dir,
		_g3dKmdProfilerKMemoryUsageGetLogLevelVariablePointer());

	/* ask current status */
	_g3dKmdCreateDebugfsFileNode("KMemUsageCurrent", memory_dir,
				     kmemroy_usage_debugfs_seq_show);
#endif

#if defined(ENABLE_G3DMEMORY_LINK_LIST)
	/* Memory List folder */
	_g3dKmdCreateDebugfsFileNode("MemList", memory_dir, memroy_list_debugfs_seq_show);
#endif


}

void g3dKmd_debugfs_profiler_destroy(void)
{

}
