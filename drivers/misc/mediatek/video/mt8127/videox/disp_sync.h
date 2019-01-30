/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __DISP_SYNC_H
#define __DISP_SYNC_H

#define DISP_INVALID_FENCE_FD       -1
#define DISP_INVALID_ION_FD         -1

#define DDP_OVL_LAYER_COUNT         (4)
/* how many counters prior to current timeline real-time counter */
#define FENCE_STEP_COUNTER          1
#define MAX_BUFFER_COUNT            3


typedef unsigned int UINT;
typedef unsigned char bool;

#ifndef NULL
#define NULL                      0x0
#endif

#ifndef ASSERT
#define ASSERT(expr, fmt, arg...)   \
	do {                            \
		if (!(expr)) \
			aee_kernel_warning("[DISP DRV] ASSERT FAILED(%s:%d):"fmt, __func__, __LINE__, ##arg); \
	} while (0)
#endif

#define XLOG_INFO(fmt, arg...) \
	pr_info("DISP/"LOG_TAG""fmt, ##arg)
#define XLOG_DBG(fmt, arg...)  \
	pr_debug("DISP/"LOG_TAG""fmt, ##arg)
#define XLOG_WARN(fmt, arg...) \
	pr_warn("DISP/"LOG_TAG""fmt, ##arg)
#define XLOG_ERR(fmt, arg...) \
	pr_err("DISP/"LOG_TAG""fmt, ##arg)

/* /============================================================================= */
/* forward declare external structures */
/* /=========================== */
struct fb_overlay_buffer;


/* /============================================================================= */
/* structure declarations */
/* /=========================== */
typedef enum {
	SYNC_STATUS_OK = 0,

	SYNC_STATUS_NOT_IMPLEMENTED,
	SYNC_STATUS_ALREADY_SET,
	SYNC_STATUS_INVALID_PARAM,
	SYNC_STATUS_ERROR,
} SYNC_STATUS;

/* /============================================================================= */
/* external variables declarations */
/* /========================== */
extern bool isAEEEnabled;

/* /============================================================================= */
/* function declarations */
/* /=========================== */
SYNC_STATUS disp_sync_ion_alloc_buffer(struct ion_client *client, int ion_fd, UINT *mva,
				       struct ion_handle **hnd);
void disp_sync_ion_cache_flush(struct ion_client *client, struct ion_handle *handle);

/**
 * Release all buffer mva of whose fence is signaled.
 * @param layer, specify which timeline (OVL layer)
 */
void disp_sync_release_buffer(UINT layer);
/**
 * Get the current buffer index to be released
 * @param layer, specify which timeline
 * @param cur_idx, which is sent from client(HWC)
 */
int disp_sync_handle_next(UINT layer, UINT cur_idx);
/**
 * Signal all fences of this timeline to given index
 * @param layer, specify which timeline
 * @param idx, all behind this will be signaled
 */
void disp_sync_signal_fence(UINT layer, UINT idx);
SYNC_STATUS disp_sync_prepare_buffer(struct fb_overlay_buffer *buf);
UINT disp_sync_query_buffer_mva(UINT layer, UINT idx);
/**
 * Query the last signaled fence index
 * @param layer, specify which timeline
 */
UINT disp_sync_query_index(UINT layer);
/**
 * Release all resource(buffer,fence) on this timeline
 * @param layer, specify which timeline
 */
void disp_sync_release(UINT layer);
void disp_sync_init(void);
void disp_sync_deinit(void);

#endif				/* __DISP_SYNC_H */
