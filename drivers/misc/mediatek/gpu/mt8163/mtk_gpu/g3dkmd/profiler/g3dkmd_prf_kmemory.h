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

#ifndef _G3DKMD_PRF_KMEMORY_H_
#define _G3DKMD_PRF_KMEMORY_H_


#if defined(linux) && !defined(linux_user_mode)	/* linux kernel */
#include <linux/seq_file.h>
#endif
#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dbase_type.h"	/* for G3Duint */
#include "g3dkmd_profiler.h"


/* kernel memory statistic */
#if defined(G3D_KMD_MEMORY_STATISTIC)

struct G3d_profiler_memory_usage_data {
	G3Duint log_level;
	/* G3Duint pid; */
	G3Duint max_usage;
	G3Duint current_usage;
};


/* APIs */
void _g3dKmdKMemoryUsageDeviceInit(struct G3d_profiler_memory_usage_data *pMemoryUsage);
void _g3dKmdKMemoryUsageDeviceTerminate(struct G3d_profiler_memory_usage_data *pMemoryUsage);
void _g3dKmdKMemoryUsageModuleInit(void);
void _g3dKmdKMemoryUsageModuleTerminate(void);


G3Duint *_g3dKmdProfilerKMemoryUsageGetLogLevelVariablePointer(void);
void _g3dKmdProfilerKMemoryUsageReport(struct seq_file *seq);

#define _g3dKmdProfilerKMemoryUsageAlloc(size, resType, pid) \
		__g3dKmdProfilerKMemoryUsageAlloc(size, resType, pid, __func__)
#define _g3dKmdProfilerKMemoryUsageFree(size, resType, pid) \
		__g3dKmdProfilerKMemoryUsageFree(size, resType, pid, __func__)

/* don't use the __g3dKmd* */
void __g3dKmdProfilerKMemoryUsageAlloc(const G3Duint size, const G3Denum resType, const G3Duint pid,
							const G3Dchar *caller_name);
void __g3dKmdProfilerKMemoryUsageFree(const G3Duint size, const G3Denum resType, const G3Duint pid,
							const G3Dchar *caller_name);

G3Duint _g3dKmdProfilerKMemoryGetCurrentUsage(void);

#else

#define _g3dKmdKMemoryUsageDeviceInit(...)
#define _g3dKmdKMemoryUsageDeviceTerminate(...)
#define _g3dKmdKMemoryUsageModuleInit()
#define _g3dKmdKMemoryUsageModuleTerminate()

#define _setLogLevelKMemoryUsage(...)
#define _g3dKmdProfilerKMemoryUsageAlloc(...)
#define _g3dKmdProfilerKMemoryUsageFree(...)

#define _g3dKmdProfilerKMemoryUsageReport(...)
#define _g3dKmdProfilerKMemoryUsageGetLogLevelVariablePointer()

#define _g3dKmdProfilerKMemoryGetCurrentUsage() 0
#endif

#endif
