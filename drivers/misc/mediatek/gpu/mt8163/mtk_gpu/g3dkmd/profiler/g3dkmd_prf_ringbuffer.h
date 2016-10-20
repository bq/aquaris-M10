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

#ifndef _G3DKMD_PRF_RINGBUFFER_H_
#define _G3DKMD_PRF_RINGBUFFER_H_

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dbase_type.h" /* for G3Duint & G3DMAXBUFFER */
#include "g3dkmd_profiler.h"

#if defined(G3D_PROFILER)
#if defined(G3D_PROFILER_RINGBUFFER_STATISTIC)
/* ringbuffer profiler */

/* Types */

/* APIs */
int _g3dKmdProfilerRingBufferDeviceInit(uintptr_t *in_ppRingbuffers);
void _g3dKmdProfilerRingBufferDeviceTerminate(uintptr_t *in_ppRingbuffers);


void g3dKmdProfilerRingBufferPollingCnt(uintptr_t profiler_private_data, const G3Duint pid,
					const G3Duint usage, const G3Duint pollCnt,
					const G3Duint askSize, const G3Duint availableSize,
					const G3Duint stamp);
#else
  /* a fake set api for profiler ringbuffer disable */
#define _g3dKmdProfilerRingBufferDeviceInit(...)
#define _g3dKmdProfilerRingBufferDeviceTerminate(...)
#define g3dKmdProfilerRingBufferPollingCnt(...)
#endif /* (G3D_PROFILER_RINGBUFFER_STATISTIC) */

#endif /* (G3D_PROFILER) */


#endif /* _G3DKMD_PRF_RINGBUFFER_H_ */
