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

#include <linux/slab.h>		/* for kmalloc */


#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dbase_type.h"	/* for G3DMAXBUFFER */
#include "g3dkmd_util.h"	/* for G3DKMDMALLOC */
#include "g3dkmd_file_io.h"	/* for KMDDPF */
#include "g3dkmd_profiler.h"

#include "profiler/g3dkmd_prf_kmemory.h"
#include "profiler/g3dkmd_prf_ringbuffer.h"



/* debug message printer */

#define L_MSG_NONE      0x0
#define L_MSG_INTERFACE 0x1
#define L_MSG_ERROR     0x2
#define L_MSG_WARNING   0x4
#define L_MSG_DEBUG     0x8


#define USE_LOCAL_PRINTK  0
#define LOCAL_TAG         "(Profiler)"
#define L_MSG_FLAGS       (L_MSG_NONE)
/* #define L_MSG_FLAGS       (L_MSG_INTERFACE | L_MSG_ERROR) */
/* #define L_MSG_FLAGS       (L_MSG_ERROR | L_MSG_WARNING | L_MSG_DEBUG) */
/* #define L_MSG_FLAGS       (L_MSG_INTERFACE | L_MSG_ERROR | L_MSG_WARNING | L_MSG_DEBUG) */

#define L_CHECKFLAG(f)  ((L_MSG_FLAGS) & (f))

#if USE_LOCAL_PRINTK
#define LPRINTER(g3dflag, flag, tag, ...) do { if (L_CHECKFLAG(flag)) \
						pr_debug(LOCAL_TAG #tag __VA_ARGS__);\
					  } while (0)
#else
#define LPRINTER(g3dflag, flag, tag, ...) do { if (L_CHECKFLAG(flag)) \
						KMDDPF(g3dflag, KERN_INFO LOCAL_TAG #tag __VA_ARGS__); \
					  } while (0)
#endif

#define PDPF_E(...) LPRINTER(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PROFILER, L_MSG_ERROR,     "[E] ", __VA_ARGS__)
#define PDPF_I(...) LPRINTER(G3DKMD_LLVL_INFO  | G3DKMD_MSG_PROFILER, L_MSG_INTERFACE, "[I] ", __VA_ARGS__)
#define PDPF_D(...) LPRINTER(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_PROFILER, L_MSG_DEBUG,     "[D] ", __VA_ARGS__)


/* Implementation */

#if defined(G3D_PROFILER)

uintptr_t _g3dKmdGetDevicePrivateDataPtr(uintptr_t in_pPrflr_private_data, int idx)
{
	struct G3d_profiler_private_data *pPrflr_private_data =
		(struct G3d_profiler_private_data *)in_pPrflr_private_data;
	uintptr_t retval = (uintptr_t) NULL;

	PDPF_I("(%s) pPrflr_private_data: %p, idx: %d\n", __func__, pPrflr_private_data, idx);

	if (MAX_PROFILER_PRIVATE_DATA_IDX > 0 && idx >= 0 && idx < MAX_PROFILER_PRIVATE_DATA_IDX)
		retval = (uintptr_t) &(pPrflr_private_data->profiler_private_data[idx]);

	PDPF_I("(%s) retval: %p\n", __func__, (void *)retval);

	return retval;
}

uintptr_t _g3dKmdGetDevicePrivateData(uintptr_t in_pPrflr_private_data, int idx)
{
	struct G3d_profiler_private_data *pPrflr_private_data =
		(struct G3d_profiler_private_data *)in_pPrflr_private_data;
	uintptr_t retval = (uintptr_t) NULL;

	PDPF_I("(%s) pPrflr_private_data: %p, idx: %d\n", __func__, pPrflr_private_data, idx);

	if (MAX_PROFILER_PRIVATE_DATA_IDX > 0 && idx >= 0 && idx < MAX_PROFILER_PRIVATE_DATA_IDX)
		retval = pPrflr_private_data->profiler_private_data[idx];

	PDPF_I("(%s) retval: %p\n", __func__, (void *)retval);

	return retval;
}

void _g3dKmdProfilerModuleInit(void)
{
	PDPF_I("(%s)\n", __func__);
	/* 1. Kernel memory usage initialize */
	_g3dKmdKMemoryUsageModuleInit();
}

void _g3dKmdProfilerModuleTerminate(void)
{
	PDPF_I("(%s)\n", __func__);
	/* 1. Kernel memory usage terminate */
	_g3dKmdKMemoryUsageModuleTerminate();
}

void _g3dKmdProfilerDeviceInit(uintptr_t *ppProfiler_private_data)
{
	struct G3d_profiler_private_data *pPrivate_data;

	PDPF_I("(%s) ppProfiler_private_data: %p\n", __func__, ppProfiler_private_data);

	/* alloc profiler private data */
	pPrivate_data = (struct G3d_profiler_private_data *)
	    G3DKMDMALLOC(sizeof(struct G3d_profiler_private_data));
	if (!pPrivate_data) {
		PDPF_E("(%s) alloc memory fail\n", __func__);
		return;
	}
	/* store to device_private */
	*ppProfiler_private_data = (uintptr_t) pPrivate_data;
	PDPF_D("(%s)  pProfiler_private_data: %p\n", __func__,
	       (void *)*ppProfiler_private_data);

	/* initialized */
	{
		/* 1. ringbuffer: */
#if defined(G3D_PROFILER_RINGBUFFER_STATISTIC)
		{
			uintptr_t *ring_data_ptr =
			    (uintptr_t *) _g3dKmdGetDevicePrivateDataPtr((uintptr_t) pPrivate_data,
									 RINGBUFFER_PROFIELR_PRIVATE_DATA_IDX);

			PDPF_D("(%s) ring_data_ptr : %p\n", __func__, ring_data_ptr);
			PDPF_D("(%s) *ring_data_ptr: %p\n", __func__, (void *)(*ring_data_ptr));

			_g3dKmdProfilerRingBufferDeviceInit(ring_data_ptr);
		}
#endif
		/* 2. kmemory usage initialize */
		_g3dKmdKMemoryUsageDeviceInit(NULL);

	}

}

void _g3dKmdProfilerDeviceTerminate(uintptr_t pProfiler_private_data)
{
	struct G3d_profiler_private_data *pPrivate_data;

	PDPF_I("(%s) pProfiler_private_data: %p\n", __func__, (void *)pProfiler_private_data);

	pPrivate_data = (struct G3d_profiler_private_data *) pProfiler_private_data;

	/* uninit */
	{
		/* 2. kmemory usage initialize */
		_g3dKmdKMemoryUsageDeviceTerminate(NULL);
		/* 1. ringbuffer */
#if defined(G3D_PROFILER_RINGBUFFER_STATISTIC)
		{
			uintptr_t *ring_data_ptr =
			    (uintptr_t *) _g3dKmdGetDevicePrivateDataPtr((uintptr_t) pPrivate_data,
									 RINGBUFFER_PROFIELR_PRIVATE_DATA_IDX);

			PDPF_D("(%s) ring_data_ptr : %p\n", __func__, ring_data_ptr);
			PDPF_D("(%s) *ring_data_ptr: %p\n", __func__, (void *)(*ring_data_ptr));

			_g3dKmdProfilerRingBufferDeviceTerminate(ring_data_ptr);
		}
#endif
	}

	/* free profiler private dta */
	G3DKMDFREE(pPrivate_data);


}
#endif /* (G3D_PROFILER) */
