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

#ifndef _G3DKMD_FENCE_H_
#define _G3DKMD_FENCE_H_

#include "g3dkmd_define.h"

#define MUST_ENABLE_CONFIG_SW_SYNC64

#if IS_ENABLED(CONFIG_SW_SYNC64)
#define SW_SYNC64
#else
#ifdef MUST_ENABLE_CONFIG_SW_SYNC64
#error CONFIG_SW_SYNC64 is not set
#endif /* MUST_ENABLE_CONFIG_SW_SYNC64 */
#endif /* IS_ENABLED(CONFIG_SW_SYNC64) */


#include <linux/seq_file.h>
#if defined(FPGA_mt6752_fpga) || defined(FPGA_fpga8163) || defined(FPGA_fpga8163_64)\
	|| defined(QEMU64) || defined(MT8163)
#ifdef SW_SYNC64
#if defined(QEMU64) || defined(MT8163) || defined(SEMU64) || defined(FPGA_fpga8163) || defined(FPGA_fpga8163_64)
#include "sync64/sw_sync64.h"
#else
#include "sw_sync64.h"
#endif
#else
#include "sw_sync.h"
#endif
#else /* FPGA_mt6592_fpga or others */
#ifdef SW_SYNC64
#include <linux/sw_sync64.h>
#else
#include <linux/sw_sync.h>
#endif
#endif

/* export for ioctl or fdq */
void g3dkmdCreateFence(int taskID, int *fd, uint64_t stamp);
void g3dkmdUpdateTimeline(unsigned int taskID, unsigned int fid);


/*
 * TIMEOUT_NEVER may be passed to the wait method to indicate that it
 * should wait indefinitely for the fence to signal.
 */
#define TIMEOUT_NEVER   -1

/*
 * sync_timeline, sync_fence data structure
 */
#ifdef SW_SYNC64
struct fence_data {
	__u64 value;
	char name[32];
	__s32 fence;		/* fd of new fence */
};
#else
struct fence_data {
	__u32 value;
	char name[32];
	__s32 fence;		/* fd of new fence */
};
#endif

#ifdef KERNEL_FENCE_DEFER_WAIT
struct g3dExeInst;
int g3dkmd_fence_wait_async(int fd, struct g3dExeInst *pInst, unsigned int cmdIndex);
void g3dkmd_fence_ClearHTSignaledStatus(struct g3dExeInst *pInst, unsigned int index);
int g3dkmd_fence_GetHTWaitingIndex(struct g3dExeInst *pInst);
void g3dkmd_fence_PrintFenceWaiter(struct seq_file *m, struct g3dExeInst *pInst);
#endif

void g3dkmdFenceInit(void);
void g3dkmdFenceUninit(void);

/*
 * sync_timeline, sync_fence API
 */

/**
 * timeline_create() - creates a sync object
 * @name:   sync_timeline name
 *
 * The timeline_create() function creates a sync object named @name,
 * which represents a 32-bit monotonically increasing counter.
 */
#ifdef SW_SYNC64
struct sw_sync64_timeline *g3dkmd_timeline_create(const char *name);
#else
struct sw_sync_timeline *g3dkmd_timeline_create(const char *name);
#endif

/**
 * timeline_destroy() - releases a sync object
 * @obj:    sync_timeline obj
 *
 * The timeline_destroy() function releases a sync object.
 * The remaining active points would be put into signaled list,
 * and their statuses are set to !VENOENT.
 */
#ifdef SW_SYNC64
void g3dkmd_timeline_destroy(struct sw_sync64_timeline *obj);
#else
void g3dkmd_timeline_destroy(struct sw_sync_timeline *obj);
#endif

/**
 * timeline_inc() - increases timeline
 * @obj:    sync_timeline obj
 * @value:  the increment to a sync object
 *
 * The timeline_inc() function increase the counter of @obj by @value
 * Each sync point contains a value. A sync point on a parent timeline transits
 * from active to signaled status when the counter of a timeline reaches
 * to that of a sync point.
 */
#ifdef SW_SYNC64
void g3dkmd_timeline_inc(struct sw_sync64_timeline *obj, u64 value);
#else
void g3dkmd_timeline_inc(struct sw_sync_timeline *obj, u32 value);
#endif

/**
 * fence_create() - create a fence
 * @obj:    sync_timeline obj
 * @data:   fence struct with its name and the number a sync point bears
 *
 * The fence_create() function creates a new sync point with @data->value,
 * and assign the sync point to a newly created fence named @data->name.
 * A file descriptor binded with the fence is stored in @data->fence.
 */
#ifdef SW_SYNC64
long g3dkmd_fence_create(struct sw_sync64_timeline *obj, struct fence_data *data);
#else
long g3dkmd_fence_create(struct sw_sync_timeline *obj, struct fence_data *data);
#endif

/**
 * fence_merge() - merge two fences into a new one
 * @name:   fence name
 * @fd1:    file descriptor of the first fence
 * @fd2:    file descriptor of the second fence
 *
 * The fence_merge() function creates a new fence which contains copies of all
 * the sync_pts in both @fd1 and @fd2.
 * @fd1 and @fd2 remain valid, independent fences.
 * On success, the newly created fd is returned; Otherwise, a -errno is returned.
 */
int g3dkmd_fence_merge(char *const name, int fd1, int fd2);

/**
 * fence_wait() - wait for a fence
 * @fence:      fence pointer to a fence obj
 * @timeout:    how much time we wait at most
 *
 * The fence_wait() function waits for up to @timeout milliseconds
 * for the fence to signal.
 * A timeout of TIMEOUT_NEVER may be used to indicate that
 * the call should wait indefinitely for the fence to signal.
 */
inline int g3dkmd_fence_wait(struct sync_fence *fence, int timeout);

#endif /* _G3DKMD_FENCE_H_ */
