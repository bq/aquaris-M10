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

#include "g3dkmd_debugfs.h"
#include "g3dkmd_debugfs_aregs.h"
#include "g3dkmd_debugfs_task.h"
#include "g3dkmd_debugfs_profiler.h"
#include "g3dkmd_debugfs_log.h"
#include "g3dkmd_debugfs_fdq.h"

static struct dentry *g3dkmd_root_dir;
static struct dentry *reg_dir;
static struct dentry *tasks_dir;
static struct dentry *kprofiler_dir;
static struct dentry *klog_dir;
static struct dentry *fdq_dir;


void g3dKmd_debugfs_create(void)
{
	/* 0. create root dir */
	g3dkmd_root_dir = debugfs_create_dir("yolig3d", NULL);

	/* 1. Regs */
	reg_dir = debugfs_create_dir("Regs", g3dkmd_root_dir);
	{
		g3dKmd_debugfs_Areg_create(reg_dir);
	}

	/* 2. Tasks/Execution Instances */
	tasks_dir = debugfs_create_dir("Tasks", g3dkmd_root_dir);
	{
		g3dKmd_debugfs_task_create(tasks_dir);
	}

	/* 3. Profilers */
	kprofiler_dir = debugfs_create_dir("Profiler", g3dkmd_root_dir);
	{
		g3dKmd_debugfs_profiler_create(kprofiler_dir);
	}

	/* 4. Kernel KMDDPF log control */
	klog_dir = debugfs_create_dir("Log", g3dkmd_root_dir);
	{
		g3dKmd_debugfs_Log_create(klog_dir);
	}

	/* 5. Fdq */
	fdq_dir = debugfs_create_dir("Fdq", g3dkmd_root_dir);
	{
		g3dKmd_debugfs_Fdq_create(fdq_dir);
	}
}

void g3dKmd_debugfs_destroy(void)
{
	g3dKmd_debugfs_Areg_destroy();
	g3dKmd_debugfs_profiler_destroy();
	g3dKmd_debugfs_task_destroy();
	g3dKmd_debugfs_Log_destroy();
	g3dKmd_debugfs_Fdq_destroy();

	debugfs_remove_recursive(g3dkmd_root_dir);
}
