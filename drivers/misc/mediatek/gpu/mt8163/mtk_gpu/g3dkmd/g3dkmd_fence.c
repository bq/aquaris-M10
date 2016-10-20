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

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#ifdef G3DKMD_SUPPORT_FENCE
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/slab.h>
#include "g3dkmd_isr.h"
#include "g3dkmd_fence.h"
#include "g3dkmd_task.h"
#include "g3dkmd_util.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_task.h"
#include "g3dkmd_scheduler.h"



#ifdef KERNEL_FENCE_DEFER_WAIT

__DEFINE_SPIN_LOCK(g3dFenceSpinLock);

static struct kmem_cache *g3dkmd_fence_waiter_cachep;

struct sw_sync_fence_waiter {
	struct sync_fence_waiter obj;
	struct g3dExeInst *pInst;
	unsigned int cmdIndex;
	struct list_head sw_waiter_list;
	struct sync_fence *fence;
};
#endif

#ifdef SW_SYNC64
struct sw_sync64_timeline *g3dkmd_timeline_create(const char *name)
{
	return sw_sync64_timeline_create(name);
}

void g3dkmd_timeline_destroy(struct sw_sync64_timeline *obj)
{
	sync_timeline_destroy(&obj->obj);
}

void g3dkmd_timeline_inc(struct sw_sync64_timeline *obj, u64 value)
{
	sw_sync64_timeline_inc(obj, value);
}

long g3dkmd_fence_create(struct sw_sync64_timeline *obj, struct fence_data *data)
{
	int fd = get_unused_fd();
	int err;
	struct sync_pt *pt;
	struct sync_fence *fence;

	if (fd < 0)
		return fd;

	pt = sw_sync64_pt_create(obj, data->value);
	if (pt == NULL) {
		err = -ENOMEM;
		goto err;
	}

	data->name[sizeof(data->name) - 1] = '\0';
	fence = sync_fence_create(data->name, pt);
	if (fence == NULL) {
		sync_pt_free(pt);
		err = -ENOMEM;
		goto err;
	}

	data->fence = fd;

	sync_fence_install(fence, fd);

	return 0;

err:
	put_unused_fd(fd);
	return err;
}
#else
struct sw_sync_timeline *g3dkmd_timeline_create(const char *name)
{
	return sw_sync_timeline_create(name);
}

void g3dkmd_timeline_destroy(struct sw_sync_timeline *obj)
{
	sync_timeline_destroy(&obj->obj);
}

void g3dkmd_timeline_inc(struct sw_sync_timeline *obj, u32 value)
{
	sw_sync_timeline_inc(obj, value);
}

long g3dkmd_fence_create(struct sw_sync_timeline *obj, struct fence_data *data)
{
	int fd = get_unused_fd();
	int err;
	struct sync_pt *pt;
	struct sync_fence *fence;

	if (fd < 0)
		return fd;

	pt = sw_sync_pt_create(obj, data->value);
	if (pt == NULL) {
		err = -ENOMEM;
		goto err;
	}

	data->name[sizeof(data->name) - 1] = '\0';
	fence = sync_fence_create(data->name, pt);
	if (fence == NULL) {
		sync_pt_free(pt);
		err = -ENOMEM;
		goto err;
	}

	data->fence = fd;

	sync_fence_install(fence, fd);

	return 0;

err:
	put_unused_fd(fd);
	return err;
}
#endif

int g3dkmd_fence_merge(char *const name, int fd1, int fd2)
{
	int fd = get_unused_fd();
	int err;
	struct sync_fence *fence1, *fence2, *fence3;

	if (fd < 0)
		return fd;

	fence1 = sync_fence_fdget(fd1);
	if (NULL == fence1) {
		err = -ENOENT;
		goto err_put_fd;
	}

	fence2 = sync_fence_fdget(fd2);
	if (NULL == fence2) {
		err = -ENOENT;
		goto err_put_fence1;
	}

	name[sizeof(name) - 1] = '\0';
	fence3 = sync_fence_merge(name, fence1, fence2);
	if (fence3 == NULL) {
		err = -ENOMEM;
		goto err_put_fence2;
	}

	sync_fence_install(fence3, fd);
	sync_fence_put(fence2);
	sync_fence_put(fence1);

	return fd;

err_put_fence2:
	sync_fence_put(fence2);

err_put_fence1:
	sync_fence_put(fence1);

err_put_fd:
	put_unused_fd(fd);
	return err;
}

inline int g3dkmd_fence_wait(struct sync_fence *fence, int timeout)
{
	return sync_fence_wait(fence, timeout);
}

void g3dkmd_create_task_timeline(int taskID, struct g3dTask *task)
{
	const char str_timeline_tmpl[] = "task%03d_timeline";
	unsigned int strSize;
	char timelinename[20];

	strSize = snprintf(NULL, 0, str_timeline_tmpl, taskID);
	YL_KMD_ASSERT(strSize <= 20);
	snprintf(timelinename, strSize + 1, str_timeline_tmpl, taskID);
	task->timeline = g3dkmd_timeline_create((const char *)timelinename);

#ifdef G3D_NATIVEFENCE64_TEST
	task->timeline->value = 0xFFFFF000;
	task->lasttime = 0xFFFFF000;
#endif

}


void g3dkmdCreateFence(int taskID, int *fd, uint64_t stamp)
{
#ifdef SW_SYNC64
	const char str_fence_tmpl[] = "f_%02x_%08x_%016llx";
#else
	const char str_fence_tmpl[] = "f_%02x_%08x_%08x";
#endif

	unsigned int strSize;
	struct g3dTask *task;
	struct fence_data data;

	if (fd == NULL)
		return;

	*fd = -1;

	lock_g3d();

	if (taskID >= gExecInfo.taskInfoCount) {
		unlock_g3d();
		return;
	}

	task = g3dKmdGetTask(taskID);
	if (task == NULL) {
		unlock_g3d();
		return;
	}

	if (task->timeline == NULL)
		g3dkmd_create_task_timeline(taskID, task);

	if (task->timeline == NULL) {
		unlock_g3d();
		return;
	}
#ifdef SW_SYNC64
	data.value = stamp;
#else
	data.value = (__u32) stamp;
#endif

	strSize = snprintf(NULL, 0, str_fence_tmpl, taskID, task->tid, data.value);
	YL_KMD_ASSERT(strSize <= 31);

	snprintf(data.name, strSize + 1, str_fence_tmpl, taskID, task->tid, data.value);


	if (g3dkmd_fence_create(task->timeline, &data) == 0)
		*fd = data.fence;

	unlock_g3d();
}

void g3dkmdUpdateTimeline(unsigned int taskID, unsigned int fid)
{
	struct g3dTask *task;
	unsigned int old_lasttime, delta;
	const unsigned int maxStampValue = 0xFFFF;
	const unsigned int maxDeltaValue = maxStampValue - (COMMAND_QUEUE_SIZE/(SIZE_OF_FREG/2));

#ifdef G3DKMD_SUPPORT_ISR
	lock_isr();
#endif

	if (taskID >= gExecInfo.taskInfoCount) {
#ifdef G3DKMD_SUPPORT_ISR
		unlock_isr();
#endif
		return;
	}

	task = g3dKmdGetTask(taskID);
	if (task == NULL) {
#ifdef G3DKMD_SUPPORT_ISR
		unlock_isr();
#endif
		return;
	}

	if (task->timeline == NULL) {
#ifdef G3DKMD_SUPPORT_ISR
		unlock_isr();
#endif
		return;
	}

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_FENCE,
	       "taskID %d, lastime 0x%x, fid 0x%d\n", taskID, task->lasttime, fid);


#ifdef G3D_STAMP_TEST
	if (G3D_MAX_STAMP_VALUE < maxStampValue)
		maxStampValue = G3D_MAX_STAMP_VALUE;
#endif

	old_lasttime = task->lasttime & maxStampValue;
	delta = (fid >= old_lasttime) ? fid - old_lasttime :
	    fid + (maxStampValue + 1) - old_lasttime;

	if (unlikely(delta > maxDeltaValue)) {
#ifdef G3DKMD_SUPPORT_ISR
		unlock_isr();
#endif
		return;
	}

	g3dkmd_timeline_inc(task->timeline, delta);
	task->lasttime += delta;
#ifdef G3DKMD_SUPPORT_ISR
	unlock_isr();
#endif

	YL_KMD_ASSERT((task->lasttime & maxStampValue) == fid);
#ifdef SW_SYNC64
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_FENCE,
	       "update task[%d] timeline to value(0x%016llx)\n", taskID, task->lasttime);
#else
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_FENCE,
		"update task[%d] timeline to value(0x%x)\n", taskID, task->lasttime);
#endif
}
#ifdef KERNEL_FENCE_DEFER_WAIT
static inline unsigned char _getBit(unsigned char *p, int bit)
{
	return (p[bit >> 3]  >> (bit % 8)) & 1;
}

static inline void _setBit(unsigned char *p, int bit)
{
	p[bit >> 3] |= 1 << (bit % 8);
}

static inline void _clearBit(unsigned char *p, int bit)
{
	p[bit >> 3] &= ~(1 << (bit % 8));
}

struct sw_sync_fence_waiter *sw_sync_waiter_alloc(struct g3dExeInst *pInst, unsigned int cmdIndex,
						       sync_callback_t callback)
{
	struct sw_sync_fence_waiter *sw_waiter = NULL;

	sw_waiter = kmem_cache_alloc(g3dkmd_fence_waiter_cachep, GFP_KERNEL);

	if (sw_waiter) {
		sw_waiter->pInst = pInst;
		sw_waiter->cmdIndex = cmdIndex;

		sync_fence_waiter_init((struct sync_fence_waiter *)sw_waiter, callback);
	} else {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FENCE, "kmem_cache_alloc failed\n");
	}

	return sw_waiter;
}

void sw_sync_waiter_free(struct sw_sync_fence_waiter *sw_waiter)
{
	if (sw_waiter)
		kmem_cache_free(g3dkmd_fence_waiter_cachep, sw_waiter);
}

void sw_sync_waiter_callback(struct sync_fence *fence, struct sync_fence_waiter *waiter)
{
	struct g3dExeInst *pInst;
	struct g3dHtInfo *ht;
	struct sw_sync_fence_waiter *sw_waiter = (struct sw_sync_fence_waiter *)container_of(waiter,
							struct sw_sync_fence_waiter, obj);
	__SPIN_LOCK2_DECLARE(flag);
	__SPIN_LOCK2(g3dFenceSpinLock, flag);

	pInst = sw_waiter->pInst;
	ht = pInst->pHtDataForSwWrite->htInfo;

/*
	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FENCE,
	    "%s %d %d %d %d\n", fence->name, sw_waiter->cmdIndex, sw_waiter->cmdIndex >> 3, sw_waiter->cmdIndex % 8,
	    _getBit(ht->fenceSignaled, sw_waiter->cmdIndex));
*/
	_setBit(ht->fenceSignaled, sw_waiter->cmdIndex);

	list_del(&sw_waiter->sw_waiter_list);

	ht->fenceUpdated = G3DKMD_TRUE;
	g3dKmdTriggerScheduler();

	__SPIN_UNLOCK2(g3dFenceSpinLock, flag);

	sync_fence_put(fence);
	sw_sync_waiter_free(sw_waiter);

}

int g3dkmd_fence_wait_async(int fd, struct g3dExeInst *pInst, unsigned int cmdIndex)
{
	struct sync_fence *fence = NULL;
	struct sw_sync_fence_waiter *sw_waiter = NULL;
	struct g3dHtInfo *ht = pInst->pHtDataForSwWrite->htInfo;
	int ret = -1;

	/*
	KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_FENCE, "cmdIndex = %d\n", cmdIndex);
	*/

	if (fd >= 0) {
		fence = sync_fence_fdget(fd);

		if (fence)
			sw_waiter = sw_sync_waiter_alloc(pInst, cmdIndex, sw_sync_waiter_callback);
		else
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FENCE, "sync_fence_fdget(%d) failed\n", fd);
	}

	if (sw_waiter) {

		__SPIN_LOCK2_DECLARE(flag);
		__SPIN_LOCK2(g3dFenceSpinLock, flag);

		ret = sync_fence_wait_async(fence, &sw_waiter->obj);

		if (ret == 0) {
			sw_waiter->fence = fence;
			list_add_tail(&sw_waiter->sw_waiter_list, &pInst->fenceWaiter_list_head);
		}
		__SPIN_UNLOCK2(g3dFenceSpinLock, flag);

		if (ret) {
			/*
			KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_FENCE,
				"%s 0x%x return %d\n", fence->name,
				pInst->recoveryDataArray.dataArray[cmdIndex].stamp,
				ret);
			  */

			sw_sync_waiter_free(sw_waiter);
			sync_fence_put(fence);
			sw_waiter = NULL;
		} else {
			/*
			KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_FENCE,
				"%s 0x%x cmdIndex=%d\n", fence->name,
				pInst->recoveryDataArray.dataArray[cmdIndex].stamp,
				cmdIndex);
			*/
		}

	} else if (fence) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FENCE,
			"%s 0x%x failed, change to call g3dkmd_fence_wait\n", fence->name,
			pInst->recoveryDataArray.dataArray[cmdIndex].stamp);

		g3dkmd_fence_wait(fence, -1);
		sync_fence_put(fence);
	}

	if (sw_waiter == NULL) {
		__SPIN_LOCK2_DECLARE(flag);
		__SPIN_LOCK2(g3dFenceSpinLock, flag);

		_setBit(ht->fenceSignaled, cmdIndex);
		ht->fenceUpdated = G3DKMD_TRUE;

		__SPIN_UNLOCK2(g3dFenceSpinLock, flag);

		g3dKmdUpdateReadyPtr();
	}



	return ret;

}

void g3dkmd_fence_ClearHTSignaledStatus(struct g3dExeInst *pInst, unsigned int index)
{
	struct g3dHtInfo *ht;

	__SPIN_LOCK2_DECLARE(flag);
	__SPIN_LOCK2(g3dFenceSpinLock, flag);

	ht = pInst->pHtDataForSwWrite->htInfo;
	_clearBit(ht->fenceSignaled, index);

	__SPIN_UNLOCK2(g3dFenceSpinLock, flag);

}

int g3dkmd_fence_GetHTWaitingIndex(struct g3dExeInst *pInst)
{
	struct g3dHtInfo *ht;
	int rdIndex;
	int wtIndex;
	int waitingIndex;
	int i;
	int found = 0;

	__SPIN_LOCK2_DECLARE(flag);
	__SPIN_LOCK2(g3dFenceSpinLock, flag);

	ht = pInst->pHtDataForSwWrite->htInfo;

	if (!ht->fenceUpdated) {
		__SPIN_UNLOCK2(g3dFenceSpinLock, flag);

		return -1;
	}

	ht->fenceUpdated = G3DKMD_FALSE;

	if (pInst->hwChannel != G3DKMD_HWCHANNEL_NONE)
		ht->rdPtr = g3dKmdRegGetReadPtr(pInst->hwChannel);

	rdIndex = (ht->rdPtr - ht->htCommandBaseHw)/HT_CMD_SIZE;
	wtIndex = (ht->wtPtr - ht->htCommandBaseHw)/HT_CMD_SIZE;

	if (rdIndex <= wtIndex) {
		for (i = rdIndex; i < wtIndex; i++) {
			if (_getBit(ht->fenceSignaled, i) == 0) {
				found = 1;
				waitingIndex = i;
				break;
			}
		}
	} else {
		for (i = rdIndex; i < MAX_HT_COMMAND; i++) {
			if (_getBit(ht->fenceSignaled, i) == 0) {
				found = 1;
				waitingIndex = i;
				break;
			}
		}

		if (!found) {
			for (i = 0; i < wtIndex; i++) {
				if (_getBit(ht->fenceSignaled, i) == 0) {
					found = 1;
					waitingIndex = i;
					break;
				}
			}
		}
	}

	if (!found)
		waitingIndex = wtIndex;

	__SPIN_UNLOCK2(g3dFenceSpinLock, flag);
	/*
	KMDDPF(G3DKMD_LLVL_DEBUG| G3DKMD_MSG_FENCE,
	    "waitingIndex = %d, readyPtr = 0x%x, rtPtr = 0x%x, wtPtr = 0x%x\n", waitingIndex,
	    waitingIndex * HT_CMD_SIZE + ht->htCommandBaseHw, ht->rdPtr, ht->wtPtr);
	*/

	return waitingIndex;
}

void g3dkmd_fence_PrintFenceWaiter(struct seq_file *m,  struct g3dExeInst *pInst)
{
	struct list_head *pos;
	unsigned int cnt = 0;

	__SPIN_LOCK2_DECLARE(flag);
	__SPIN_LOCK2(g3dFenceSpinLock, flag);

	list_for_each(pos, &pInst->fenceWaiter_list_head) {
		struct sw_sync_fence_waiter *sw_waiter =
			container_of(pos, struct sw_sync_fence_waiter, sw_waiter_list);

		seq_printf(m, "    cmdIndex: %d, stamp: 0x%x, fence: %s,  status: %d\n",
			   sw_waiter->cmdIndex, pInst->recoveryDataArray.dataArray[sw_waiter->cmdIndex].stamp,
			   sw_waiter->fence->name, atomic_read(&sw_waiter->fence->status));
		cnt++;
	}

	if (cnt == 0)
		seq_puts(m, "    none\n");

	__SPIN_UNLOCK2(g3dFenceSpinLock, flag);
}


#endif /* KERNEL_FENCE_DEFER_WAIT */

void g3dkmdFenceInit(void)
{
#ifdef KERNEL_FENCE_DEFER_WAIT
	g3dkmd_fence_waiter_cachep = kmem_cache_create("yolig3d_fence_waiter_cache",
					sizeof(struct sw_sync_fence_waiter), 0, 0, NULL);

	if (unlikely(!g3dkmd_fence_waiter_cachep)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FENCE,
			"kmem_cache_create sw_sync_fence_waiter failed\n");
	}
#endif
}

void g3dkmdFenceUninit(void)
{
#ifdef KERNEL_FENCE_DEFER_WAIT
	if (g3dkmd_fence_waiter_cachep) {
		kmem_cache_destroy(g3dkmd_fence_waiter_cachep);
		g3dkmd_fence_waiter_cachep = NULL;
	}
#endif
}
#endif /* G3DKMD_SUPPORT_FENCE */
