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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>  /* for IS_ERR, PTR_ERR */
#include <linux/slab.h> /* for memory allocation */
#include <linux/sched.h> /* for cpu_clock */

#include "gpad_common.h"
#include "gpad_kmif.h"
#include "gpad_hal.h"
#include "gpad_pm.h"
#include "gpad_pm_priv.h"
#include "gpad_pm_config.h"

#if defined(GPAD_SHIM_ON)
#define GPAD_PM_ITST 0
#else
#define GPAD_PM_ITST 0
#endif

#if GPAD_CHECKING_HW
static unsigned long long s_cpu_clock;
static unsigned int s_gpu_timestamp;
static unsigned int s_last_gpu_ts;

#define RESET_CPU_CLOCK(_curr) \
	{ \
	s_cpu_clock = (_curr) \
	}

#define CHECK_CPU_CLOCK(_curr) \
	{ \
	if ((long long)((_curr) - s_cpu_clock) < 0) { \
		GPAD_LOG_ERR("CPU curr: 0x%llx < last: 0x%llx at %s\n", _curr, s_cpu_clock, __func__); \
		GPAD_LOG_ERR("CPU Now: 0x%llx\n", cpu_clock(0)); \
		BUG_ON(1); \
	} \
	s_cpu_clock = (_curr) \
	}

#define RESET_GPU_TIMESTAMP(_curr) \
	{ \
	s_gpu_timestamp = (_curr) \
	}

#define CHECK_GPU_TIMESTAMP(_curr) \
	{ \
	if ((int)((_curr) - s_gpu_timestamp) < 0) { \
		G3DKMD_KMIF_MEM_DESC   *desc = (G3DKMD_KMIF_MEM_DESC *)pm_info->hw_buffer_desc; \
		GPAD_LOG_ERR("GPU curr: 0x%08x < last: 0x%08x at %s\n", _curr, s_gpu_timestamp, __func__); \
		GPAD_LOG_ERR("HA: %08X, VA: %p, Index: %d/%d\n", \
			desc->hw_addr, desc->vaddr, pm_info->hw_buffer_idx, pm_info->samples_alloc); \
		BUG_ON(1); \
	} \
	s_gpu_timestamp = (_curr) \
	}

#define RESET_GPU_TS_REG() \
	{ \
	s_last_gpu_ts = gpad_hal_read32(0x1f40) \
	}

#define CHECK_GPU_TS_REG() \
	do { \
		unsigned int _curr_gpu_ts = gpad_hal_read32(0x1f40); \
		if ((int)(_curr_gpu_ts - s_last_gpu_ts) < 0) { \
			GPAD_LOG_ERR("GPU curr: 0x%08x < last: 0x%08x\n", _curr_gpu_ts, s_last_gpu_ts); \
			BUG_ON(1); \
		} \
		s_last_gpu_ts = _curr_gpu_ts; \
	} while (0)

#define CHECK_SAMPLE_HEADER_TAG(_sample) \
	do { \
		if ((_sample)->header.tag != 0xff) { \
			GPAD_LOG_ERR("header.tag: 0x%x != 0xff!\n", (_sample)->header.tag); \
			BUG_ON(1); \
		} \
	} while (0)

#define CHECK_SAMPLE_HEADER_SATURATION(_sample) \
	do { \
		if ((_sample)->header.saturation != 0x0) { \
			GPAD_LOG_ERR("header.saturation: 0x%x != 0x0!\n", (_sample)->header.saturation); \
			BUG_ON(1); \
		} \
	} while (0)

#define CHECK_SAMPLE_HEADER_TIMESTAMP(_sample) \
	do { \
		unsigned int _curr_gpu_ts = curr_gpu_ts = gpad_hal_read32(0x1f40); \
		int _diff_gpu_ts = (int)_curr_gpu_ts - (int)(_sample)->header.timestamp; \
		if (_diff_gpu_ts < 0 || _diff_gpu_ts > (pm_info->interval * GPU_FREQ / 1000)) { \
			GPAD_LOG_ERR("header.timestamp: 0x%08x is invalid!\n", (_sample)->header.timestamp); \
			BUG_ON(1); \
		} \
	} while (0)
#else
#define RESET_CPU_CLOCK(...)
#define CHECK_CPU_CLOCK(...)
#define RESET_GPU_TIMESTAMP(...)
#define CHECK_GPU_TIMESTAMP(...)
#define RESET_GPU_TS_REG()
#define CHECK_GPU_TS_REG()
#define CHECK_SAMPLE_HEADER_TAG(...)
#define CHECK_SAMPLE_HEADER_SATURATION(...)
#define CHECK_SAMPLE_HEADER_TIMESTAMP(...)
#endif

#if GPAD_PM_ITST
static void gpad_pm_check_used_list(struct gpad_pm_info *pm_info)
{
	struct list_head           *p;
	struct gpad_pm_sample_list *curr;
	unsigned int                last_frame_id;
	unsigned int                idx = 0;
	USE_USED_SAMPLE_LIST_LOCK;

	LOCK_USED_SAMPLE_LIST(pm_info);
	list_for_each(p, &pm_info->used_sample_list) {
		curr = list_entry(p, struct gpad_pm_sample_list, node);
		if (idx)
			BUG_ON(((int)curr->sample.header.frame_id - (int)last_frame_id) != 1);
		last_frame_id = curr->sample.header.frame_id;
		idx++;
	}
	UNLOCK_USED_SAMPLE_LIST(pm_info);
}
#endif /* GPAD_PM_ITST */

#if defined(GPAD_SUPPORT_DVFS)
#include "gpad_ipem.h"

static void gpad_pm_notify_ipem(struct gpad_device *dev)
{
	struct gpad_pm_info *pm_info = GET_PM_INFO(dev);
	struct gpad_pm_stat *stat = &pm_info->stat;
	struct gpad_ipem_pm_data data;
	USE_HW_BUFFER;

	LOCK_HW_BUFFER(pm_info);
	data.acc_busy_ns = stat->acc_busy_ns;
	data.acc_total_ns = stat->acc_total_ns;
	UNLOCK_HW_BUFFER(pm_info);
	gpad_ipem_callback_pm(dev, &data);
}
#else
#define gpad_pm_notify_ipem(...)
#endif /* GPAD_SUPPORT_DVFS */

static int gpad_pm_alloc_sw_buffer(struct gpad_pm_info *pm_info, unsigned int samples_to_alloc)
{
	unsigned int                allocated = 0;
	unsigned int                idx;
	struct gpad_pm_sample_list *curr;
	USE_FREE_SAMPLE_LIST_LOCK;

	/*
	 * Allocate PM samples and add them to free_sample_list.
	 */
	for (idx = samples_to_alloc; idx > 0; --idx) {
		curr = kzalloc(sizeof(struct gpad_pm_sample_list), GFP_KERNEL);
		if (likely(curr)) {
			INIT_LIST_HEAD(&curr->node);
			curr->id = allocated++;
			/*GPAD_LOG_LOUD("gpad_pm_sample_list [%u] = %p\n", curr->id, curr); */

			LOCK_FREE_SAMPLE_LIST(pm_info);
			list_add_tail(&curr->node, &pm_info->free_sample_list);
			UNLOCK_FREE_SAMPLE_LIST(pm_info);
		} else {
			GPAD_LOG_ERR("Failed to allocate sample-%u!\n", allocated);
			break;
		}
	}
	GPAD_LOG_INFO("PM polling interval: %d ms, samples allocated: %d\n", pm_info->interval, allocated);

	return (allocated > 0) ? 0 : -ENOMEM;
}

static void gpad_pm_free_sw_buffer(struct gpad_pm_info *pm_info)
{
	unsigned int                cnt = 0;
	struct list_head            tmp_list;
	struct list_head           *p, *n;
	struct gpad_pm_sample_list *curr;
	USE_USED_SAMPLE_LIST_LOCK;
	USE_FREE_SAMPLE_LIST_LOCK;

	INIT_LIST_HEAD(&tmp_list);

	LOCK_ALL_SAMPLE_LIST(pm_info);
	list_splice_init(&pm_info->free_sample_list, &tmp_list);
	list_splice_init(&pm_info->used_sample_list, &tmp_list);
	UNLOCK_ALL_SAMPLE_LIST(pm_info);

	list_for_each_safe(p, n, &tmp_list) {
		list_del(p);

		curr = list_entry(p, struct gpad_pm_sample_list, node);
		kfree(curr);
		cnt++;
	}

	GPAD_LOG_INFO("%u s/w PM samples freed\n", cnt);
	WARN_ON(cnt != pm_info->samples_alloc);
}

static int gpad_pm_alloc_hw_buffer(struct gpad_pm_info *pm_info, unsigned int samples_to_alloc)
{
	unsigned int          align = GPAD_PM_HW_BUFFER_ALIGN;
	G3DKMD_KMIF_MEM_DESC *desc;
	USE_HW_BUFFER;

	WARN_ON(pm_info->hw_buffer_desc);

	/*
	 * Allocate a 32-byte aligned MMU mapped memory block for HW to dump GPU PM counters.
	 */
	desc = g3dKmdKmifAllocMemory(sizeof(struct gpad_pm_sample) * samples_to_alloc, align);
	if (likely(desc)) {
		GPAD_LOG_LOUD("Allocated memory mapped desc=%p for %u samples:\n", desc, samples_to_alloc);
		GPAD_LOG_LOUD("size=%u, hw_addr=0x%08x, vaddr=%p\n", desc->size, desc->hw_addr, desc->vaddr);
		BUG_ON(desc->size < sizeof(struct gpad_pm_sample) * samples_to_alloc);
		BUG_ON((desc->hw_addr % align));

		LOCK_HW_BUFFER(pm_info);
		pm_info->hw_buffer_desc = (void *)desc;
		pm_info->hw_buffer_addr = (unsigned char *)(desc->vaddr); /* kernel logical address. */
		UNLOCK_HW_BUFFER(pm_info);
		GPAD_LOG_LOUD("HW buffer's kernel logical addr=%p\n", pm_info->hw_buffer_addr);
		return 0;
	} else {
		GPAD_LOG_ERR("gpad_pm_alloc_hw_buffer() failed to allocate %u samples!\n", samples_to_alloc);
		return -ENOMEM;
	}
}

static void gpad_pm_free_hw_buffer(struct gpad_pm_info *pm_info)
{
	USE_HW_BUFFER;

	/*
	 * Free the MMU mapped memory block.
	 */
	LOCK_HW_BUFFER(pm_info);
	if (likely(pm_info->hw_buffer_desc)) {
		G3DKMD_KMIF_MEM_DESC *desc = (G3DKMD_KMIF_MEM_DESC *)pm_info->hw_buffer_desc;
		pm_info->hw_buffer_desc = NULL;
		UNLOCK_HW_BUFFER(pm_info);

		g3dKmdKmifFreeMemory(desc);
		GPAD_LOG_LOUD("Freed memory mapped desc=%p\n", desc);
	} else {
		UNLOCK_HW_BUFFER(pm_info);
	}
}

static int gpad_pm_alloc_buffers(struct gpad_pm_info *pm_info, unsigned int interval)
{
	unsigned int    samples_to_alloc;
	int             err;
	unsigned long   intvl_j;
	USE_HW_BUFFER;

	/*
	 * Assume all buffers are freed to system before calling  this function.
	 */
	WARN_ON(!list_empty(&pm_info->free_sample_list));
	WARN_ON(!list_empty(&pm_info->used_sample_list));

	/*
	 * Work out #samples for specified polling interval.
	 */
	samples_to_alloc = ((GPAD_PM_MAX_SAMPLE_PER_SEC * interval * 2) + 999) / 1000;
#if GPAD_PM_ITST
	samples_to_alloc = 3;
#endif
	intvl_j =  MS_TO_JIFFIES(interval);
	GPAD_LOG_INFO("interval: %u ms = %lu jiffies, %u samples\n", interval, intvl_j, samples_to_alloc);

	LOCK_HW_BUFFER(pm_info);
	pm_info->interval = interval;
	pm_info->intvl_j = intvl_j;
	pm_info->samples_alloc = samples_to_alloc;
	UNLOCK_HW_BUFFER(pm_info);

	/*
	 * Allocate required s/w and h/w buffers for these samples.
	 */
	err = gpad_pm_alloc_sw_buffer(pm_info, samples_to_alloc);
	if (unlikely(err))
		return err;

	err = gpad_pm_alloc_hw_buffer(pm_info, samples_to_alloc);
	if (unlikely(err))
		gpad_pm_free_sw_buffer(pm_info);

	return err;
}

static void gpad_pm_free_buffers(struct gpad_pm_info *pm_info)
{
	gpad_pm_free_sw_buffer(pm_info);
	gpad_pm_free_hw_buffer(pm_info);
}

/*static int gpad_pm_realloc_buffer(struct gpad_pm_info *pm_info, unsigned int interval) */
/*{ */
/*    gpad_pm_free_buffers(pm_info); */
/*    return gpad_pm_alloc_buffers(pm_info, interval); */
/*} */

static int gpad_pm_sw_init(struct gpad_device *dev)
{
	struct gpad_pm_info    *pm_info = GET_PM_INFO(dev);
	int                     err;

	GPAD_LOG_LOUD("gpad_pm_header size: %zd gpad_pm_sample size: %zd\n", sizeof(struct gpad_pm_header), sizeof(struct gpad_pm_sample));
	BUG_ON(sizeof(struct gpad_pm_header) != 32);
	BUG_ON(sizeof(struct gpad_pm_sample) != 160);

	/*
	 * Initialize members of gpad_pm_info.
	 */
	spin_lock_init(&pm_info->free_sample_lock);
	INIT_LIST_HEAD(&pm_info->free_sample_list);

	spin_lock_init(&pm_info->used_sample_lock);
	INIT_LIST_HEAD(&pm_info->used_sample_list);

	spin_lock_init(&pm_info->hw_buffer_lock);
	pm_info->hw_buffer_desc = NULL;
	pm_info->hw_buffer_addr = NULL;
	pm_info->hw_buffer_idx = 0;
	pm_info->interval = 0;
	pm_info->intvl_j = 0;
	pm_info->samples_alloc = 0;
	init_timer(&pm_info->hw_buffer_timer);
	pm_info->last_cpu_ts = 0;
	pm_info->last_gpu_ts = 0;
	pm_info->unsync_intvl = 0;
	pm_info->to_restart_ts = 0;
	pm_info->gpu_freq = GPAD_GPU_DEF_FREQ;

	pm_info->config = GPAD_PM_DEF_CONFIG;
	GPAD_LOG_INFO("default config: %d\n", pm_info->config);
	memset(&pm_info->stat, 0, sizeof(pm_info->stat));
	gpad_pm_reset_backup_config(pm_info);

	/*
	 * Use default polling interval to allocate buffers.
	 */
	err = gpad_pm_alloc_buffers(pm_info, GPAD_PM_DEFAULT_POOL_INTERVAL);
	return err;
}

static void gpad_pm_sw_exit(struct gpad_device *dev)
{
	struct gpad_pm_info        *pm_info = GET_PM_INFO(dev);

	gpad_pm_free_buffers(pm_info);
}

static void gpad_pm_stat_add_sample(struct gpad_pm_info *pm_info, struct gpad_pm_sample *sample)
{
	struct gpad_pm_stat *stat = &pm_info->stat;
	long long            diff_ns;
	/* Debug TOTAL_CYCLE value*/
	/* GPAD_LOG_LOUD("[PM] %s: VP:%u PP:%u SEGMT:%u %d!\n", __func__, VP_CYCLE(sample), PP_CYCLE(sample), SEGMT_CYCLE(sample), sample->header.frame_id);
	 * CHECK_TOTAL_CYCLE(sample);
	 */
	stat->read_samples++;
	stat->busy_time += TOTAL_CYCLE(sample);
	stat->vertex_cnt += VERTEX_CNT(sample);
	stat->triangle_cnt += TRIANGLE_CNT(sample);
	stat->pixel_cnt += PIXEL_CNT(sample);

	if (sample->header.frame_id != stat->last_frame_id) {
		stat->frame_cnt++;
		stat->last_frame_id = sample->header.frame_id;
	}

	diff_ns = ((long long)TOTAL_CYCLE(sample) * 1000000000);
	do_div(diff_ns, pm_info->gpu_freq);
	stat->acc_busy_ns += (unsigned long long)diff_ns;
}

static void gpad_pm_stat_refresh(struct gpad_pm_info *pm_info)
{
	struct gpad_pm_stat *stat = &pm_info->stat;
	unsigned long len = FIELD_OFFSET(struct gpad_pm_stat, read_samples);

	memset(stat, 0, len);
}

/*static void gpad_pm_dump_header(struct gpad_pm_header *header) */
/*{ */
/*    GPAD_LOG_LOUD("--------------------\n"); */
/*    GPAD_LOG_LOUD("sample=%p, header=%p\n", */
/*    ((unsigned char*)header - GPAD_PM_COUNTER_CNT*sizeof(unsigned int)), header); */
/* */
/*    GPAD_LOG_LOUD("sel_central: 0x%04x, sel_par: 0x%04x, sel_mod: 0x%04x\n", */
/*                  header->sel_central, header->sel_par, header->sel_mod); */
/*    GPAD_LOG_LOUD("saturation: 0x%08x\n", header->saturation); */
/*    GPAD_LOG_LOUD("timestamp: 0x%08x\n", header->timestamp); */
/*    GPAD_LOG_LOUD("frame_id: 0x%08x, subframe_id: 0x%08x, segment_id: 0x%08x\n", */
/*                  header->frame_id, header->subframe_id, header->segment_id); */
/*    GPAD_LOG_LOUD("tag: 0x%02x, mode: 0x%02x\n", */
/*                  header->tag, header->mode); */
/*} */

/*static void gpad_pm_dump_sample(struct gpad_pm_sample *sample) */
/*{ */
/*    unsigned int idx; */
/* */
/*    gpad_pm_dump_header(&sample->header); */
/*    for (idx = 0; idx < GPAD_PM_COUNTER_CNT; idx++) { */
/*        GPAD_LOG_LOUD("data[%u] = 0x%08x\n", idx, sample->data[idx]); */
/*    } */
/*} */

/*
 * Assumption: hw_buffer_lock is held.
 */
static void gpad_pm_reset_timestamp(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0314
	pm_info->last_gpu_ts = 0;
	pm_info->last_cpu_ts = cpu_clock(0);

	GPAD_LOG_LOUD("gpad_pm_reset_timestamp(): GPU timestamp: 0x%x CPU timestamp: 0x%llx\n", pm_info->last_gpu_ts, pm_info->last_cpu_ts);
	RESET_GPU_TIMESTAMP(pm_info->last_gpu_ts);
	RESET_CPU_CLOCK(pm_info->last_cpu_ts);
	RESET_GPU_TS_REG();
#else
	unsigned long long  curr_cpu_ts;
	long long           diff_cpu_ts;
	unsigned int        curr_gpu_ts;
	int                 diff_gpu_ts;

	curr_gpu_ts = gpad_hal_read32(0x1f40);
	curr_cpu_ts = cpu_clock(0);

	if (pm_info->stat.acc_total_ns > 0) {
		diff_gpu_ts = (int)curr_gpu_ts - (int)pm_info->last_gpu_ts;
		diff_cpu_ts = (long long)curr_cpu_ts - (long long)pm_info->last_cpu_ts;

		pm_info->stat.total_time += (unsigned int)diff_gpu_ts;
		pm_info->stat.acc_total_ns += (unsigned long long)diff_cpu_ts;
	}

	pm_info->last_gpu_ts = curr_gpu_ts;
	pm_info->last_cpu_ts = curr_cpu_ts;

	GPAD_LOG_LOUD("gpad_pm_reset_timestamp(): GPU timestamp: 0x%x CPU timestamp: 0x%llx\n", pm_info->last_gpu_ts, pm_info->last_cpu_ts);
	RESET_GPU_TIMESTAMP(pm_info->last_gpu_ts);
	RESET_CPU_CLOCK(pm_info->last_cpu_ts);
	RESET_GPU_TS_REG();
#endif
}

/*
 * Assumption: hw_buffer_lock is held.
 */
static void gpad_pm_sync_timestamp(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0314
	unsigned long long  curr_cpu_ts;
	long long           diff_cpu_ts;
	int                 diff_gpu_ts;

	curr_cpu_ts = cpu_clock(0);
	CHECK_CPU_CLOCK(curr_cpu_ts);

	diff_cpu_ts = (long long)curr_cpu_ts - (long long)pm_info->last_cpu_ts;
	if (likely(diff_cpu_ts >= 0)) {
		/*
		 * Normal case: the CPU timestamp is increasing.
		 * Use the difference in CPU timestamp to estimate GPU timestamp.
		 * => current GPU timestamp =  last sample GPU timestamp +
		 *    ((current CPU timestamp - last sample CPU timestamp) * GPU_FREQ / 10^9)
		 */
		/*GPAD_LOG_LOUD("gpad_pm_sync_timestamp(): 0x%llx - 0x%llx => %lld >= 0\n",  curr_cpu_ts, pm_info->last_cpu_ts, diff_cpu_ts); */
		diff_cpu_ts *= pm_info->gpu_freq;
		do_div(diff_cpu_ts, 1000000000);
		diff_gpu_ts = (int)diff_cpu_ts;

		pm_info->last_cpu_ts = curr_cpu_ts;
		pm_info->last_gpu_ts += (unsigned long)diff_gpu_ts;
		pm_info->stat.total_time += diff_gpu_ts;
		pm_info->stat.acc_total_ns += (unsigned long long)diff_cpu_ts;

		/*GPAD_LOG_INFO("sync gpu_ts: 0x%08x cpu_ts: 0x%lld\n", pm_info->last_gpu_ts, curr_cpu_ts); */
		CHECK_GPU_TIMESTAMP(pm_info->last_gpu_ts);
	} else {
		/*
		 * Error case 1: the CPU timestamp is not increasing,  * which means something wrongs in calculation from previous GPU PM samples.
		 * Use next GPU PM sample as new base of timestamp.
		 */
		if (!pm_info->to_restart_ts) {
			GPAD_LOG_ERR("gpad_pm_sync_timestamp(): 0x%llx - 0x%llx => %lld < 0\n", curr_cpu_ts, pm_info->last_cpu_ts, diff_cpu_ts);
			pm_info->to_restart_ts = 1;
		}
	}
#else
	/*
	 * Sync current GPU and CPU timestamp.
	 */
#if GPAD_ON_FPGA
	long long           diff_cpu_ts;
	int                 diff_gpu_ts;
	unsigned int        curr_gpu_ts;

	curr_gpu_ts = gpad_hal_read32(0x1f40);
	CHECK_GPU_TIMESTAMP(curr_gpu_ts);

	/*
	 * Work out CPU time base on GPU time difference for CPU clock rate is skewed.
	 */
	diff_gpu_ts = (int)curr_gpu_ts - (int)pm_info->last_gpu_ts;
	if (likely(diff_gpu_ts > 0)) {
		diff_cpu_ts = ((long long)diff_gpu_ts * 1000000000);
		do_div(diff_cpu_ts, pm_info->gpu_freq);

		pm_info->last_cpu_ts += (unsigned long long)diff_cpu_ts;
		pm_info->last_gpu_ts = curr_gpu_ts;
		CHECK_CPU_CLOCK(pm_info->last_cpu_ts);
	} else {
		GPAD_LOG_ERR("gpad_pm_sync_timestamp(): 0x%08x - 0x%08x => %d < 0\n", curr_gpu_ts, pm_info->last_gpu_ts, diff_gpu_ts);
		gpad_pm_reset_timestamp(pm_info);
	}
#else
	gpad_pm_reset_timestamp(pm_info);
#endif /* gpad_pm_sync_timestamp */
#endif
}

/*
 * Assumption: hw_buffer_lock is held.
 */
static int gpad_pm_calc_timestamp(struct gpad_pm_info *pm_info, struct gpad_pm_sample_list *sample_list)
{
	long long  diff_cpu_ts;
	int        diff_gpu_ts;

	/*GPAD_LOG_ERR("gpad_pm_calc_timestamp(): 0x%x - 0x%x\n",  sample_list->sample.header.timestamp, pm_info->last_gpu_ts); */
	CHECK_GPU_TIMESTAMP(sample_list->sample.header.timestamp);

	if (likely(!pm_info->to_restart_ts)) {
		diff_gpu_ts = (int)sample_list->sample.header.timestamp - (int)pm_info->last_gpu_ts;
		if (likely(diff_gpu_ts > 0)) {
			/*
			 * Normal case: the GPU timestamp is increasing.
			 * Use the difference in GPU timestamp to estimate GPU timestamp of current sample.
			 * => current sample CPU timestamp =  last sample CPU timestamp +
			 *    ((current sample GPU timestamp - last sample GPU timestamp) * 10^9 / GPU_FREQ)
			 */
			/*GPAD_LOG_LOUD("gpad_pm_calc_timestamp(): diff_gpu_ts: %d >= 0\n", diff_gpu_ts); */
			diff_cpu_ts = ((long long)diff_gpu_ts * 1000000000);
			do_div(diff_cpu_ts, pm_info->gpu_freq);

			pm_info->last_cpu_ts += (unsigned long long)diff_cpu_ts;
			pm_info->last_gpu_ts = sample_list->sample.header.timestamp;
			sample_list->timestamp_ns = pm_info->last_cpu_ts;
			pm_info->stat.total_time += diff_gpu_ts;
			pm_info->stat.acc_total_ns += (unsigned long long)diff_cpu_ts;
			/*GPAD_LOG_INFO("sample gpu_ts: 0x%08x cpu_ts: %lld\n",  pm_info->last_gpu_ts, pm_info->last_cpu_ts); */
			return 0;
		} else if (0 == diff_gpu_ts) {
			sample_list->timestamp_ns = pm_info->last_cpu_ts;
			/*GPAD_LOG_INFO("diff_gpu_ts == 0\n"); */
			return -1;
		} else {
			/*
			 * Error case 2: the GPU timestamp is not increasing.
			 * Use current GPU PM sample as new base of timestamp.
			 */
			GPAD_LOG_ERR("gpad_pm_calc_timestamp(): 0x%x - 0x%x => %d < 0\n", sample_list->sample.header.timestamp, pm_info->last_gpu_ts, diff_gpu_ts);
		}
	} else {
		pm_info->to_restart_ts = 0;
	}
#if GPAD_GPU_VER == 0x0314
	pm_info->last_cpu_ts = cpu_clock(0);
	CHECK_CPU_CLOCK(pm_info->last_cpu_ts);

	pm_info->last_gpu_ts = sample_list->sample.header.timestamp;
	sample_list->timestamp_ns = pm_info->last_cpu_ts;
	gpad_pm_stat_refresh(pm_info);
#else
	pm_info->last_cpu_ts = cpu_clock(0);
	CHECK_CPU_CLOCK(pm_info->last_cpu_ts);

	pm_info->last_gpu_ts = gpad_hal_read32(0x1f40);
	CHECK_GPU_TIMESTAMP(pm_info->last_gpu_ts);

	sample_list->timestamp_ns = pm_info->last_cpu_ts;
	gpad_pm_stat_refresh(pm_info);
#endif
	return -2;
}

static int gpad_pm_hw_init(struct gpad_device *dev)
{
	struct gpad_pm_info        *pm_info = GET_PM_INFO(dev);
	G3DKMD_KMIF_MEM_DESC       *desc;
#if GPAD_GPU_VER == 0x0314
	USE_HW_BUFFER;
#endif

	GPAD_LOG_LOUD("gpad_pm_hw_init()...\n");

#if GPAD_GPU_VER == 0x0314
	LOCK_HW_BUFFER(pm_info);
	gpad_pm_reset_timestamp(pm_info);
	UNLOCK_HW_BUFFER(pm_info);
#endif

	/*
	 * Initialize to default configuration.
	 */
	desc = (G3DKMD_KMIF_MEM_DESC *)pm_info->hw_buffer_desc;
	gpad_pm_do_config(pm_info, pm_info->config, desc->hw_addr, desc->size);
	return 0;
}

static void gpad_pm_hw_exit(struct gpad_device *dev)
{
	GPAD_LOG_LOUD("gpad_pm_hw_exit()...\n");
}

/*
 * Assumption: hw_buffer_lock is held.
 */
static void gpad_pm_read_hw_sample(struct gpad_pm_info *pm_info, struct gpad_pm_sample *sample)
{
	struct list_head           *p = NULL;
	struct gpad_pm_sample_list *sample_list;
	USE_USED_SAMPLE_LIST_LOCK;
	USE_FREE_SAMPLE_LIST_LOCK;

	LOCK_ALL_SAMPLE_LIST(pm_info);
	if (!list_empty(&pm_info->free_sample_list)) {
		p = pm_info->free_sample_list.next;
		/*GPAD_LOG_INFO("Get %p from free_sample_list\n", p); */
	} else if (!list_empty(&pm_info->used_sample_list)) {
		p = pm_info->used_sample_list.next;
		/*GPAD_LOG_INFO("Get %p from used_sample_list\n", p); */
	}

	if (likely(p)) {
		list_del(p);
		UNLOCK_ALL_SAMPLE_LIST(pm_info);

		sample_list = list_entry(p, struct gpad_pm_sample_list, node);
		memcpy(&sample_list->sample, sample, sizeof(*sample));
		sample_list->freq = pm_info->gpu_freq;
		if (0 == gpad_pm_calc_timestamp(pm_info, sample_list) &&
				IS_DEFAULT_CONFIG(pm_info)) {
			gpad_pm_stat_add_sample(pm_info, sample);
		}

		LOCK_USED_SAMPLE_LIST(pm_info);
		list_add_tail(&sample_list->node, &pm_info->used_sample_list);
		UNLOCK_USED_SAMPLE_LIST(pm_info);
#if GPAD_PM_ITST
		gpad_pm_check_used_list(pm_info);
#endif
	} else {
		UNLOCK_ALL_SAMPLE_LIST(pm_info);
	}
}

static void gpad_pm_poll_hw_buffer(struct gpad_pm_info *pm_info, int from_timer)
{
	struct gpad_pm_stat    *stat = &pm_info->stat;
	unsigned int            cnt, samples_alloc;
	struct gpad_pm_sample  *start, *curr;
	USE_HW_BUFFER;

	LOCK_HW_BUFFER(pm_info);

	if (from_timer) {
		if (stat->passed_intvl >= GPAD_PM_STAT_REFRESH_INTERVAL)
			gpad_pm_stat_refresh(pm_info);
		stat->passed_intvl += pm_info->interval;
	}

	samples_alloc = pm_info->samples_alloc;
	start = (struct gpad_pm_sample *)(pm_info->hw_buffer_addr);

	for (cnt = samples_alloc; cnt > 0; --cnt) {
		curr = start + pm_info->hw_buffer_idx;
		/*GPAD_LOG_LOUD("Checking %u sample: %p, tag: 0x%02x...\n", pm_info->hw_buffer_idx, curr, curr->header.tag); */

		if (IS_SAMPLE_READY(curr)) {
			CHECK_SAMPLE_HEADER_TAG(curr);
			CHECK_SAMPLE_HEADER_SATURATION(curr);

			gpad_pm_read_hw_sample(pm_info, curr);
			RECYCLE_SAMPLE(curr);

			pm_info->hw_buffer_idx++;
			if (pm_info->hw_buffer_idx >= samples_alloc)
				pm_info->hw_buffer_idx = 0;
		} else {
			break;
		}
	}

	if (cnt == samples_alloc) {
		pm_info->unsync_intvl += pm_info->interval;
		if (pm_info->unsync_intvl >= GPAD_PM_MAX_UNSYNC_INTERVAL) {
			pm_info->unsync_intvl = 0;
			gpad_pm_sync_timestamp(pm_info); /* Sync timestamp again if no sample polled this time. */
		}
	} else {
		pm_info->unsync_intvl = 0;
	}

	UNLOCK_HW_BUFFER(pm_info);
}

static void gpad_pm_hw_timer_callback(unsigned long data)
{
	struct gpad_device  *dev = (struct gpad_device *)data;
	struct gpad_pm_info *pm_info = GET_PM_INFO(dev);
	unsigned long        expires;
	USE_HW_BUFFER;

	CHECK_GPU_TS_REG();

	/*
	 * Reschulde the timer and process HW buffer if PM is enabled.
	 */
	if (likely(IS_PM_ENABLED(dev))) {
		gpad_pm_poll_hw_buffer(pm_info, 1);
		gpad_pm_notify_ipem(dev);

		LOCK_HW_BUFFER(pm_info);
		expires = jiffies + pm_info->intvl_j;
		UNLOCK_HW_BUFFER(pm_info);

		mod_timer(&pm_info->hw_buffer_timer, expires);
	} else {
		GPAD_LOG_INFO("gpad_pm_hw_timer_callback(): exit for PM is disabled\n");
	}
}

int __init gpad_pm_init(struct gpad_device *dev)
{
	int err;
	int sw_init = 0;
	int hw_init = 0;

	GPAD_LOG_LOUD("gpad_pm_init()...\n");

	err = gpad_pm_sw_init(dev);
	if (unlikely(err)) {
		GPAD_LOG_ERR("gpad_pm_sw_init() failed with %d\n", err);
		goto pm_init_error;
	}
	sw_init = 1;

	err = gpad_pm_hw_init(dev);
	if (unlikely(err)) {
		GPAD_LOG_ERR("gpad_pm_hw_init() failed with %d\n", err);
		goto pm_init_error;
	}
	hw_init = 1;
	set_bit(GPAD_PM_INIT, &dev->status);

	gpad_pm_enable(dev);
	/** To check register value in MT8163 Sapphire*/
	dev->ext_id_reg = gpad_hal_read32(0x0020);

	return 0;

pm_init_error:
	if (hw_init)
		gpad_pm_hw_exit(dev);

	if (sw_init)
		gpad_pm_sw_exit(dev);

	return err;
}

void gpad_pm_exit(struct gpad_device *dev)
{
	GPAD_LOG_LOUD("gpad_pm_exit()...\n");

	if (likely(test_and_clear_bit(GPAD_PM_INIT, &dev->status))) {
		gpad_pm_disable(dev);
		gpad_pm_hw_exit(dev);
		gpad_pm_sw_exit(dev);
	}
}

void gpad_pm_enable(struct gpad_device *dev)
{
	struct gpad_pm_info    *pm_info = GET_PM_INFO(dev);
	G3DKMD_KMIF_MEM_DESC   *desc;
	unsigned long           expires;
	USE_HW_BUFFER;

	/*
	 * Turn on PM mechanism and start periodical timer to process the HW buffer.
	 */
	if (!IS_PM_ENABLED(dev)) {
#if GPAD_GPU_VER == 0x0530
		gpad_hal_write32_mask(0x1f00, 0x0000003f, 0x00000011);
		/* Reset PM (5/30 version still needs PM_EN while doing reset) */
#elif GPAD_GPU_VER >= 0x0704
		gpad_hal_write32_mask(0x1f00, 0x0000003f, 0x00000010); /* Reset PM */
#endif

		/*
		 * Reset HW buffer ring while PM_RESET pulled up.
		 */
		LOCK_HW_BUFFER(pm_info);
		pm_info->hw_buffer_idx = 0;
		desc = (G3DKMD_KMIF_MEM_DESC *)pm_info->hw_buffer_desc;
		if (likely(desc && pm_info->hw_buffer_addr && desc->size))
			memset(pm_info->hw_buffer_addr, 0, desc->size);
		else
			GPAD_LOG_ERR("gpad_pm_hw_init(): invalid HW buffer desc!\n");
		UNLOCK_HW_BUFFER(pm_info);

#if GPAD_GPU_VER >= 0x0801
		gpad_hal_write32(0x1f44, 0xffffffff); /* PM_Counter_EN */
#endif
		gpad_hal_write32_mask(0x1f00, 0x0000003f, 0x00000001); /* Enable PM, clear reset bit */

		LOCK_HW_BUFFER(pm_info);
		gpad_pm_reset_timestamp(pm_info); /* Sync up GPU and CPU timestamp after PM is enabled. */
		UNLOCK_HW_BUFFER(pm_info);

		set_bit(GPAD_PM_ENABLE, &dev->status);
		GPAD_LOG_INFO("PM is enabled\n");

		LOCK_HW_BUFFER(pm_info);
		expires = jiffies + pm_info->intvl_j;
		pm_info->hw_buffer_timer.data = (uintptr_t)dev;
		pm_info->hw_buffer_timer.function = gpad_pm_hw_timer_callback;
		gpad_pm_stat_refresh(pm_info);
		UNLOCK_HW_BUFFER(pm_info);

		mod_timer(&pm_info->hw_buffer_timer, expires);
	}
}

void gpad_pm_force_enable(struct gpad_device *dev, int reset_to_default)
{
	struct gpad_pm_info *pm_info = &dev->pm_info;
	int                  config = (reset_to_default ? GPAD_PM_DEF_CONFIG : pm_info->config);

	if (IS_PM_ENABLED(dev))
		gpad_pm_disable(dev);
	gpad_pm_set_config(dev, config);
	gpad_pm_enable(dev);
}

void gpad_pm_disable(struct gpad_device *dev)
{
	struct gpad_pm_info    *pm_info = GET_PM_INFO(dev);

	/*
	 * Stop software timer and turn off PM mechanism.
	 */
	if (IS_PM_ENABLED(dev)) {
		del_timer(&pm_info->hw_buffer_timer);

		gpad_hal_write32_mask(0x1f00, 0x00000001, 0x00000000); /* Disable PM */

		clear_bit(GPAD_PM_ENABLE, &dev->status);
		GPAD_LOG_INFO("PM is disabled\n");
	}
}

enum gpad_pm_state gpad_pm_get_state(struct gpad_device *dev)
{
	return IS_PM_ENABLED(dev) ?  GPAD_PM_ENABLED : GPAD_PM_DISABLED;
}

unsigned int gpad_pm_get_config(struct gpad_device *dev)
{
	struct gpad_pm_info        *pm_info = GET_PM_INFO(dev);
	return pm_info->config;
}

int gpad_pm_set_config(struct gpad_device *dev, unsigned int config)
{
	struct gpad_pm_info        *pm_info = GET_PM_INFO(dev);
	int                         err = 0;
	G3DKMD_KMIF_MEM_DESC       *desc = (G3DKMD_KMIF_MEM_DESC *)pm_info->hw_buffer_desc;

	if (likely(gpad_pm_is_valid_config(config))) {
		pm_info->config = config;
		gpad_pm_do_config(pm_info, pm_info->config, desc->hw_addr, desc->size);
	} else if (gpad_pm_is_custom_config(config)) {
		pm_info->config = config;
		gpad_pm_restore_config(pm_info, pm_info->config, desc->hw_addr, desc->size);
	} else {
		GPAD_LOG_ERR("gpad_pm_set_config() invalid config=%u!\n", config);
		err = -EINVAL;
	}

	return err;
}

unsigned int gpad_pm_get_timestamp(void)
{
	return gpad_hal_read32(0x1f40);
}

struct gpad_pm_sample_list *gpad_pm_read_sample_list(struct gpad_device *dev)
{
	struct gpad_pm_info        *pm_info = GET_PM_INFO(dev);
	struct list_head           *p;
	struct gpad_pm_sample_list *sample_list = NULL;
	USE_USED_SAMPLE_LIST_LOCK;

	LOCK_USED_SAMPLE_LIST(pm_info);
	if (!list_empty(&pm_info->used_sample_list)) {
		p = pm_info->used_sample_list.next;
		list_del(p);
		sample_list = list_entry(p, struct gpad_pm_sample_list, node);
	}
	UNLOCK_USED_SAMPLE_LIST(pm_info);

	return sample_list;
}

void gpad_pm_complete_sample_list(struct gpad_device *dev, struct gpad_pm_sample_list *sample_list)
{
	struct gpad_pm_info        *pm_info = GET_PM_INFO(dev);
	USE_FREE_SAMPLE_LIST_LOCK;

	LOCK_FREE_SAMPLE_LIST(pm_info);
	list_add_tail(&sample_list->node, &pm_info->free_sample_list);
	UNLOCK_FREE_SAMPLE_LIST(pm_info);
}

void gpad_pm_change_freq(struct gpad_device *dev, int gpu_freq)
{
	struct gpad_pm_info        *pm_info = GET_PM_INFO(dev);
	USE_HW_BUFFER;

	/*
	 * Recycle all sampled queued in h/w before changing frequency.
	 */
	if (likely(gpu_freq != 0)) {
		LOCK_HW_BUFFER(pm_info);
		if (gpu_freq != pm_info->gpu_freq) {
			UNLOCK_HW_BUFFER(pm_info);

			if (likely(IS_PM_ENABLED(dev)))
				gpad_pm_poll_hw_buffer(pm_info, 0);

			LOCK_HW_BUFFER(pm_info);
			pm_info->gpu_freq = gpu_freq;
			gpad_pm_sync_timestamp(pm_info);
			UNLOCK_HW_BUFFER(pm_info);
		} else {
			UNLOCK_HW_BUFFER(pm_info);
		}
	}
}

int g3dkmd_kQueryAPI(const char *cmdStr, const unsigned int cmdStrLen, char *buffer, const unsigned int bufferLen, const unsigned int bufferDataLen)
{
	int size = 0;
	long int value = 0;

	/* check if string ends with '\0' */
	if (cmdStrLen > 0 && '\0' != cmdStr[cmdStrLen-1]) {
		GPAD_LOG_ERR("g3dkmd_kQueryAPI: command should end with '\\0'\n");
		return 0;
	} else if (bufferLen > 0 && '\0' != buffer[bufferLen-1]) {
		GPAD_LOG_ERR("g3dkmd_kQueryAPI: buffer should end with '\\0'\n");
		return 0;
	}

	if (0 == strcmp("perf", cmdStr) && bufferDataLen > 0) {
		if (0 != kstrtol(buffer, 10, &value)) {
			size = 0;
			return size;
		}
		GPAD_LOG_INFO("value = %d\n", value);
		/* call your function
		 * Note that if there are invalid string in buffer,  * value will be zero
		 */
		if (value == 1)
			g3dKmdPerfModeEnable();
		else
			g3dKmdPerfModeDisable();
		size = 0;
	} else {
		size = bufferDataLen;
	}

	return size;
}
