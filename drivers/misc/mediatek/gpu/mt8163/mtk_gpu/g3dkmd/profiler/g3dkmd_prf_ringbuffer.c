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


#include <linux/slab.h> /* for kmalloc */
#include <linux/types.h>
#include <linux/sched.h> /* for current */

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dbase_type.h" /* for G3Duint & G3DMAXBUFFER */
#include "g3dkmd_util.h" /* for G3DKMDMALLOC */
#include "g3dkmd_file_io.h" /* for KMDDPF */
#include "g3dkmd_profiler.h"
#include "profiler/g3dkmd_prf_ringbuffer.h"


/* debug message printer */

#define L_MSG_NONE      0x0
#define L_MSG_INTERFACE 0x1
#define L_MSG_ERROR     0x2
#define L_MSG_WARNING   0x4
#define L_MSG_DEBUG     0x8


#define USE_LOCAL_PRINTK  0
#define RDPF_TAG          "(Prf_Ring)"
#define L_MSG_FLAGS       (L_MSG_NONE)
/* #define L_MSG_FLAGS       (L_MSG_INTERFACE | L_MSG_ERROR) */
/* #define L_MSG_FLAGS       (L_MSG_ERROR | L_MSG_WARNING | L_MSG_DEBUG) */
/* #define L_MSG_FLAGS       (L_MSG_INTERFACE | L_MSG_ERROR | L_MSG_WARNING | L_MSG_DEBUG) */

#define L_CHECKFLAG(f)  ((L_MSG_FLAGS) & (f))

#if USE_LOCAL_PRINTK
#define LPRINTER(g3dflag, flag, tag, ...) do { if (L_CHECKFLAG(flag))\
							pr_debug(RDPF_TAG #tag __VA_ARGS__);\
					  } while (0)
#else
#define LPRINTER(g3dflag, flag, tag, ...) do { if (L_CHECKFLAG(flag))\
							KMDDPF(g3dflag, KERN_INFO RDPF_TAG #tag __VA_ARGS__);\
					  } while (0)
#endif
#undef USE_LOCAL_PRINTK

#define RDPF_E(...) LPRINTER(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PROFILER, L_MSG_ERROR,     "[E] ", __VA_ARGS__)
#define RDPF_I(...) LPRINTER(G3DKMD_LLVL_INFO  | G3DKMD_MSG_PROFILER, L_MSG_INTERFACE, "[I] ", __VA_ARGS__)
#define RDPF_D(...) LPRINTER(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_PROFILER, L_MSG_DEBUG,     "[D] ", __VA_ARGS__)


/* Implementation */


#if defined(G3D_PROFILER_RINGBUFFER_STATISTIC)
/* ringbuffer profiler */


struct G3d_profiler_ringbuffer_data {
	G3Duint maxPollCnt;
	G3Duint askSize;
	G3Duint availableSize;
	G3Duint stmp;
};

struct G3d_profiler_ringbuffers {
	struct G3d_profiler_ringbuffer_data buffers[G3DMAXBUFFER];
};

#endif /* (G3D_PROFILER_RINGBUFFER_STATISTIC) */


#if defined(G3D_PROFILER_RINGBUFFER_STATISTIC)
int _g3dKmdProfilerRingBufferDeviceInit(uintptr_t *in_ppRingbuffers)
{
	struct G3d_profiler_ringbuffers **ppRingbuffers;

	RDPF_I("(%s) ppRingbuffers:%p\n", __func__, in_ppRingbuffers);

	ppRingbuffers = (struct G3d_profiler_ringbuffers **) in_ppRingbuffers;

	if (ppRingbuffers) {
		int i;
		struct G3d_profiler_ringbuffers *pRingbuffers;

		/* alloc memory for device */
		pRingbuffers = (struct G3d_profiler_ringbuffers *)
				G3DKMDMALLOC(sizeof(struct G3d_profiler_ringbuffers));
		if (!pRingbuffers) {
			RDPF_E("(%s)alloc memory fail\n", __func__);
			goto fail;
		}


		for (i = 0; i < G3DMAXBUFFER; ++i) {
			pRingbuffers->buffers[i].maxPollCnt = 0;
			pRingbuffers->buffers[i].askSize = 0;
			pRingbuffers->buffers[i].availableSize = 0;
			pRingbuffers->buffers[i].stmp = 0;
		}
		*ppRingbuffers = pRingbuffers;
		RDPF_D("(%s)  *in_ppRingbuffers:%p\n", __func__, (void *)*in_ppRingbuffers);
		RDPF_D("(%s)  *ppRingbuffers   :%p\n", __func__, (void *)*ppRingbuffers);
		RDPF_D("(%s)  pRingbuffers     :%p\n", __func__, pRingbuffers);

		return 0;
	}

fail:
	*ppRingbuffers = NULL;
	return 1;
}

static void _reportRingBuffer(const G3Duint usage, const G3d_profiler_ringbuffer_data *data)
{
	/* RDPF_I( "(%s) usage: %u, G3d_profiler_ringbuffer_data: %p\n", __FUNCTION__, usage, data); */

	pr_debug("[Profiler] Ringbuffer(usage:%u, maxPollCnt: %u, Size: ask/avaialbe(%u, %u), stamp: %u)\n",
		usage, data->maxPollCnt, data->askSize, data->availableSize, data->stmp);
}

void _g3dKmdProfilerRingBufferDeviceTerminate(uintptr_t *in_ppRingbuffers)
{
	struct G3d_profiler_ringbuffers **ppRingbuffers;

	RDPF_I("(%s) ppRingbuffers:%p\n", __func__, (void *)in_ppRingbuffers);
	RDPF_D("(%s)  pRingbuffers:%p\n", __func__, (void *)*in_ppRingbuffers);


	ppRingbuffers = (struct G3d_profiler_ringbuffers **) in_ppRingbuffers;
	if (ppRingbuffers) {
		struct G3d_profiler_ringbuffers *pRingbuffers;

		pRingbuffers = *ppRingbuffers;

		RDPF_D("(%s)  *in_ppRingbuffers:%p\n", __func__, (void *)*in_ppRingbuffers);
		RDPF_D("(%s)  *ppRingbuffers   :%p\n", __func__, (void *)*ppRingbuffers);
		RDPF_D("(%s)  pRingbuffers     :%p\n", __func__, pRingbuffers);


		if (pRingbuffers) {
			int usage;

			pr_debug("====== [Profielr] Reprot Begin ======\n");
			pr_debug("[Profiler] PID: %5u, name: %s\n", current->pid, current->comm);

			for (usage = 0; usage < G3DMAXBUFFER; ++usage)
				_reportRingBuffer(usage, &(pRingbuffers->buffers[usage]));

			pr_debug("====== [Profielr] Reprot End   ======\n");

			G3DKMDFREE(pRingbuffers);
		} else {
			RDPF_E("(%s)free memory fail: pRingbuffers is NULL\n", __func__);
		}
	}
}


void g3dKmdProfilerRingBufferPollingCnt(uintptr_t profiler_private_data,
					const G3Duint pid, const G3Duint usage,
					const G3Duint pollCnt,
					const G3Duint askSize,
					const G3Duint availableSize, const G3Duint stamp)
{
	RDPF_I("(%s) profiler_private_data: %p, pid: %u, usage: %u, pollCnt: %u, askSize: %u avaibleSz: %u, stamp:%u\n",
		__func__, (void *)profiler_private_data, pid, usage, pollCnt, askSize,
		availableSize, stamp);

	KMD_ASSERT(pid == current->pid);

	if (profiler_private_data) {
		struct G3d_profiler_ringbuffers *pRingbuffers = (struct G3d_profiler_ringbuffers *)
							_g3dKmdGetDevicePrivateDataPtr(profiler_private_data,
							RINGBUFFER_PROFIELR_PRIVATE_DATA_IDX);

		if (pRingbuffers) {
			G3d_profiler_ringbuffer_data *rbuf = &pRingbuffers->buffers[usage];

			if (pollCnt > rbuf->maxPollCnt) {
				rbuf->maxPollCnt = pollCnt;
				rbuf->askSize = askSize;
				rbuf->availableSize = availableSize;
			}
		}
	}
}
#endif /* (G3D_PROFILER_RINGBUFFER_STATISTIC) */
