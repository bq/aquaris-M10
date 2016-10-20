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

#ifdef ANDROID
#include <linux/kernel.h>
#include <linux/pid.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/radix-tree.h>
#include <linux/err.h>
#ifdef CMODEL
#include <linux/export.h>
#include "../../../ion/new/ion_m/ion.h"
#include "../../../ion/new/ion_m/ion_priv.h"
#else
#include <ion.h>
#include <ion_priv.h>
#endif

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#include "g3dkmd_file_io.h"
#include "g3dbase_ioctl.h"
#include "g3dkmd_util.h"
#include "g3dkmd_ion.h"
#include "g3dkmd_api.h"

#ifdef CMODEL
struct ion_device *g_ion_device_in_gpu;
int (*ion_mm_heap_register_buf_destroy_callback_in_gpu)(struct ion_buffer *buffer,
							 ion_mm_buf_destroy_callback_t *fn);
void (*ion_free_in_gpu)(struct ion_client *client, struct ion_handle *handle);
struct ion_client *(*ion_client_create_in_gpu)(struct ion_device *dev, const char *name);
struct ion_handle *(*ion_import_dma_buf_in_gpu)(struct ion_client *client, int fd);
#else
#define g_ion_device_in_gpu g_ion_device
#define ion_mm_heap_register_buf_destroy_callback_in_gpu ion_mm_heap_register_buf_destroy_callback
#define ion_free_in_gpu ion_free
#define ion_client_create_in_gpu ion_client_create
#define ion_import_dma_buf_in_gpu ion_import_dma_buf
#endif



RADIX_TREE(gIonMapper, GFP_KERNEL);	/* Declare and initialize */

static __DEFINE_SEM_LOCK(ionLock);

#define LOCK_TYPE_READ   0x01
#define LOCK_TYPE_WRITE  0x10
struct g3d_ion_buffer {
	struct mutex lock;
	struct list_head stamp_list;
	struct kref ref;
	void *ion_buffer;
	unsigned int lock_now;
	unsigned int lock_tid; /* keep the last lock process */
	int prepare_to_die;
	int rotate;
#if !defined(YL_PRE_ROTATE_SW_SHIFT)
	int rot_dx;
	int rot_dy;
#endif
};

struct ionGrallocLockInfoNode {
	struct ionGrallocLockInfo base;
	struct list_head list;
};

static int ion_buffer_destroy_callback(struct ion_buffer *buffer, unsigned int phyAddr);

static int ion_radix_tree_insert_safe(struct radix_tree_root *tree, unsigned long key,
				      struct g3d_ion_buffer *item)
{
	int result = 0;

	if (tree == NULL)
		return 0;

	result = radix_tree_insert(tree, key, (void *)item);

	if (result == -EEXIST) {
		struct g3d_ion_buffer *buffer;

		buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, key);

		if (buffer != NULL && item != NULL)
			YL_KMD_ASSERT(buffer->ion_buffer == NULL || buffer->ion_buffer != item->ion_buffer);

		kfree(buffer);
		radix_tree_delete(&gIonMapper, key);
		result = radix_tree_insert(tree, key, (void *)item);
	}

	if (result == -ENOMEM) {
		result = radix_tree_preload(GFP_KERNEL);
		if (result == 0) {
			result = radix_tree_insert(tree, key, (void *)item);
			radix_tree_preload_end();
		}
	}

	return result;
}

static void ion_mirror_check_from_ion(struct g3d_ion_buffer *buffer, unsigned long addr, int fd)
{
	static struct ion_client *yoli_client;
	struct ion_handle *handle;

	if (yoli_client == NULL)
		yoli_client = ion_client_create_in_gpu(g_ion_device_in_gpu, "kern_call_from_yoli");

	handle = ion_import_dma_buf_in_gpu(yoli_client, fd);

	if (IS_ERR(handle))
		buffer->ion_buffer = NULL;
	else {
		buffer->ion_buffer = (void *)handle->buffer;
		ion_mm_heap_register_buf_destroy_callback_in_gpu(handle->buffer, ion_buffer_destroy_callback);
		ion_free_in_gpu(yoli_client, handle);
	}
}

G3DKMD_APICALL int G3DAPIENTRY ion_mirror_create(unsigned long addr, int fd)
{
	struct g3d_ion_buffer *buffer;
	int ret;

	buffer = kzalloc(sizeof(struct g3d_ion_buffer), GFP_KERNEL);
	if (buffer == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s : out of memory\n", __func__);
		YL_KMD_ASSERT(0);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&buffer->stamp_list);
	buffer->lock_now = 0;
	buffer->lock_tid = 0;
	buffer->rotate = 0;
#if !defined(YL_PRE_ROTATE_SW_SHIFT)
	buffer->rot_dx = 0;
	buffer->rot_dy = 0;
#endif
	if (fd != -1) /* fb fd will cause to get ion handle error or not pass it but -1 */
		ion_mirror_check_from_ion(buffer, addr, fd);
	mutex_init(&buffer->lock);
	kref_init(&buffer->ref);

	__SEM_LOCK(ionLock);
	ret = ion_radix_tree_insert_safe(&gIonMapper, addr, buffer);
	__SEM_UNLOCK(ionLock);

	if (ret) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION,
		       "%s : ion_radix_tree_insert_safe error(%d)\n", __func__, ret);
		YL_KMD_ASSERT(0);
	}

	return ret;
}

static void _ion_mirror_destroy(struct kref *kref)
{
	struct g3d_ion_buffer *buffer = container_of(kref, struct g3d_ion_buffer, ref);
	struct ionGrallocLockInfoNode *node, *tmp;

	list_for_each_entry_safe(node, tmp, &buffer->stamp_list, list) {
		list_del(&node->list);
		kfree(node);
	}
	kfree(buffer);
}

static void ion_mirror_get(struct g3d_ion_buffer *buffer)
{
	if (buffer == NULL)
		return;
	kref_get(&buffer->ref);
}

static int ion_mirror_put(struct g3d_ion_buffer *buffer)
{
	if (buffer == NULL)
		return 0; /* if 1 means object is removed */
	return kref_put(&buffer->ref, _ion_mirror_destroy);
}

static int ion_mirror_wait_finish(unsigned long addr)
{
	int ret;
	unsigned int i, num_task, num_pass;
	struct ionGrallocLockInfo *data = NULL;

	/* KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_ION,"%s: wait_finish(0x%lx)\n", __func__, addr); */
	ret = ion_mirror_stamp_get(addr, &num_task, NULL);
	if (ret != 0) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s : ion_mirror_stamp_get 1st(%d)\n",
		       __func__, ret);
		return ret;
	}

	if (num_task == 0) /* no need to check wait_finish */
		return 0;

	/* data = kzalloc(sizeof(struct ionGrallocLockInfo) * num_task, GFP_KERNEL); */
	data = kcalloc(num_task, sizeof(struct ionGrallocLockInfo), GFP_KERNEL);
	if (unlikely(NULL == data)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s : kzalloc memory fail(size = %u)\n",
		       __func__, sizeof(struct ionGrallocLockInfo) * num_task);
		return -ENOMEM;
	}
	/* get buffer stamp info from data */
	ret = ion_mirror_stamp_get(addr, &num_task, data);
	if (ret != 0) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s : ion_mirror_stamp_get 2nd(%d)\n",
		       __func__, ret);
		return ret;
	}
	/* KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_ION,"%s: [wait_finish(0x%lx)\n", __func__, addr); */
	do {
		/* always do flush in g3d_device.c _g3dDeviceFreeMallocIon */
		g3dKmdLockCheckDone(num_task, data);

		num_pass = 0;
		for (i = 0; i < num_task; i++) {
			KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ION,
			       "%s : g3dKmdLockCheckDone [%d] %u %u %u %u %u\n", __func__, i,
			       data[i].pid, data[i].taskID, data[i].write_stamp, data[i].read_stamp,
			       data[i].pass);
			if ((data[i].pass & ALL_PASS) == ALL_PASS)
				num_pass++;
		}

		if (num_pass == num_task)
			break;

		/*msleep(1);*/ /* the value can be adjusted */
		usleep_range(1000, 1001);
	} while (1);
	/* KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_ION,"%s: wait_finish](0x%lx)\n", __func__, addr); */

	/* no need to check stamp update again due to we set prepare_to_die first */

	return 0;
}

static int ion_mirror_real_destroy(unsigned long addr)
{
	struct g3d_ion_buffer *buffer;

	__SEM_LOCK(ionLock);
	buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, addr);
	if (buffer) {
		__SEM_UNLOCK(ionLock);
		mutex_lock(&buffer->lock); /* no ionLock is needed due to initalized kref=1 still */
		buffer->prepare_to_die = 1;
		mutex_unlock(&buffer->lock);
		ion_mirror_wait_finish(addr); /* it call functions with buffer->lock */
		__SEM_LOCK(ionLock);
	}
	radix_tree_delete(&gIonMapper, addr);
	ion_mirror_put(buffer); /* remove the initalized kref = 1 */
	__SEM_UNLOCK(ionLock);

	return 0;
}

static int ion_buffer_destroy_callback(struct ion_buffer *buffer, unsigned int phyAddr)
{
	struct g3d_ion_buffer *g3d_buffer;
	unsigned long phyAddrl = (unsigned long)phyAddr;

	__SEM_LOCK(ionLock);
	g3d_buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, phyAddrl);
	if (g3d_buffer) {
		if ((void *)buffer != g3d_buffer->ion_buffer) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION,
			       "%s : not match ion_buffer %p %p\n", __func__, buffer, g3d_buffer->ion_buffer);
			YL_KMD_ASSERT((void *)buffer == g3d_buffer->ion_buffer);
		}
	} else {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s (0x%lx): invalid hwaddr\n", __func__, phyAddrl);
	}
	__SEM_UNLOCK(ionLock);
	if (g3d_buffer)
		ion_mirror_real_destroy(phyAddrl);

	return 0;
}

G3DKMD_APICALL int G3DAPIENTRY ion_mirror_destroy(unsigned long addr)
{
	struct g3d_ion_buffer *buffer;
	void *ion_buffer;

	__SEM_LOCK(ionLock);
	buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, addr);
	ion_buffer = (buffer) ? buffer->ion_buffer : NULL;
	__SEM_UNLOCK(ionLock);

	/* if not from ion, delete right now. if from ion, delete when ion buffer destroy callback */
	if (ion_buffer == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s (0x%lx): invalid hwaddr\n", __func__, addr);
		return ion_mirror_real_destroy(addr);
	}

	return 0;
}


G3DKMD_APICALL int G3DAPIENTRY ion_mirror_stamp_set(unsigned long addr, unsigned int stamp,
						    unsigned readonly, pid_t pid,
						    unsigned int taskID)
{
	struct g3d_ion_buffer *buffer;
	struct ionGrallocLockInfoNode *node;

	int ret = 0;
	int found = 0;

	__SEM_LOCK(ionLock);
	buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, addr);
	ion_mirror_get(buffer);	/* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	if (buffer == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s (0x%lx): invalid hwaddr\n", __func__, addr);
		return -EINVAL;
	}

	mutex_lock(&buffer->lock);

	/* unlock dead thread */
	if (buffer->lock_now && find_vpid(buffer->lock_tid) == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION,
		       "ERROR: ion_mirror_stamp_set, locked thread is dead, [%d,%d]\n",
		       buffer->lock_tid, current->pid);
		buffer->lock_now = 0;
	}

	if (buffer->prepare_to_die) {
		mutex_unlock(&buffer->lock);
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s (0x%lx): buffer preapre to die\n", __func__, addr);
		ret = -EPERM;
		goto fail;
	}
	/* some one already lock it by write or try to lock it by write */
	if (buffer->lock_now && ((buffer->lock_now & LOCK_TYPE_WRITE) != 0 || !readonly)) {
		mutex_unlock(&buffer->lock);
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION,
		       "%s : buffer(mva=0x%lx) is locked now (lock tid=%d)\n", __func__, addr, buffer->lock_tid);
		ret = -EAGAIN;
		goto fail;
	}

	list_for_each_entry(node, &buffer->stamp_list, list) {
		if (node->base.pid == pid && node->base.taskID == taskID) {
			found = 1;
			break;
		}
	}

	if (found == 0) {
		node = kzalloc(sizeof(struct ionGrallocLockInfoNode), GFP_KERNEL);
		node->base.pid = pid;
		node->base.taskID = taskID;
		node->base.pass |= ALL_PASS;
		/* in default, the read_stamp/write_stamp = 0 */
		/* add the latest one in the head of list */
		list_add(&node->list, &buffer->stamp_list);
	}

	if (readonly) {
		node->base.read_stamp = stamp;
		node->base.pass &= (~READ_PASS);
	} else {
		node->base.write_stamp = stamp;
		node->base.pass &= (~WRITE_PASS);
	}

	mutex_unlock(&buffer->lock);

fail:
	__SEM_LOCK(ionLock);
	ion_mirror_put(buffer);	/* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	return ret;
}

G3DKMD_APICALL int G3DAPIENTRY ion_mirror_stamp_get(unsigned int addr, unsigned int *num_task,
						    struct ionGrallocLockInfo *data)
{
	struct g3d_ion_buffer *buffer;
	struct ionGrallocLockInfoNode *node;
	unsigned int i = 0;

	__SEM_LOCK(ionLock);
	buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, addr);
	ion_mirror_get(buffer);	/* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	if (buffer == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s (0x%lx): invalid hwaddr\n", __func__, addr);
		return -EINVAL;
	}

	mutex_lock(&buffer->lock);
	list_for_each_entry(node, &buffer->stamp_list, list) {
		if (data) {
			if (i < *num_task)
				data[i] = node->base;
			else
				break;
		}

		i++;
	}

	*num_task = i; /* return set count */

	mutex_unlock(&buffer->lock);

	__SEM_LOCK(ionLock);
	ion_mirror_put(buffer); /* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	return 0;
}

G3DKMD_APICALL int G3DAPIENTRY ion_mirror_lock_update(unsigned long addr, unsigned int num_task,
						      unsigned int readonly,
						      struct ionGrallocLockInfo *data,
						      unsigned int *lock_status)
{
	struct g3d_ion_buffer *buffer;
	struct ionGrallocLockInfoNode *node;
	unsigned int i = 0;

	if (lock_status == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s : invalid lock_status pointer\n", __func__);
		return -EINVAL;
	}

	__SEM_LOCK(ionLock);
	buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, addr);
	ion_mirror_get(buffer); /* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	if (buffer == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s (0x%lx): invalid hwaddr\n", __func__, addr);
		return -EINVAL;
	}

	*lock_status = LOCK_DONE;

	mutex_lock(&buffer->lock);

	/* unlock dead thread */
	if (buffer->lock_now && find_vpid(buffer->lock_tid) == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION,
		       "ERROR: ion_lock_update, locked thread is dead, [%d,%d]\n", buffer->lock_tid, current->pid);
		buffer->lock_now = 0;
	}

	/* some one already lock it by write or try to lock it by write */
	if (buffer->lock_now && ((buffer->lock_now & LOCK_TYPE_WRITE) != 0 || !readonly)) {

		*lock_status = LOCK_ALREADY;
	} else {
		list_for_each_entry(node, &buffer->stamp_list, list) {
			if (data == NULL) {
				/* find a new one */
				if ((readonly && ((node->base.pass & WRITE_PASS) == WRITE_PASS)) ||
					(!readonly && ((node->base.pass & ALL_PASS) == ALL_PASS))) {
					/*
					 *empty statement?
					 */
				} else {
					*lock_status = LOCK_DIRTY;
					break;
				}
			} else if (node->base.pid == data[i].pid
				   && node->base.taskID == data[i].taskID && i < num_task) {
				if (node->base.write_stamp != data[i].write_stamp) {
					/* find a pid/taskID which updates stamp again */
					*lock_status = LOCK_DIRTY;
					break;
				} else if (!readonly && node->base.read_stamp != data[i].read_stamp) {
					/* find a pid/taskID which updates stamp again */
					*lock_status = LOCK_DIRTY;
					break;
				}

				if (node->base.pass != data[i].pass) {
					if (node->base.write_stamp == data[i].write_stamp)
						node->base.pass = ~WRITE_PASS | (data[i].pass & WRITE_PASS);

					if (node->base.read_stamp == data[i].read_stamp)
						node->base.pass = ~READ_PASS | (data[i].pass & READ_PASS);
				}

				i++;
			} else {
				if ((readonly && ((node->base.pass & WRITE_PASS) == WRITE_PASS)) ||
					(!readonly && ((node->base.pass & ALL_PASS) == ALL_PASS))) {
					/*
					 *empty statement?
					 */
				} else {
					*lock_status = LOCK_DIRTY;
					break;
				}
			}
		}

		if (*lock_status == LOCK_DONE) {
			YL_KMD_ASSERT(buffer->lock_now == 0 || readonly);
			buffer->lock_now = (readonly) ? LOCK_TYPE_READ : LOCK_TYPE_WRITE;
			buffer->lock_tid = current->pid;
		}
	}

	mutex_unlock(&buffer->lock);

	__SEM_LOCK(ionLock);
	ion_mirror_put(buffer);	/* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	return 0;
}

G3DKMD_APICALL int G3DAPIENTRY ion_mirror_unlock_update(unsigned int addr)
{
	struct g3d_ion_buffer *buffer;

	__SEM_LOCK(ionLock);
	buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, addr);
	ion_mirror_get(buffer);	/* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	if (buffer == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s (0x%lx): invalid hwaddr\n", __func__, addr);
		return -EINVAL;
	}

	mutex_lock(&buffer->lock);
	buffer->lock_now = 0;
	mutex_unlock(&buffer->lock);

	__SEM_LOCK(ionLock);
	ion_mirror_put(buffer);	/* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	return 0;
}

#ifdef YL_PRE_ROTATE_SW_SHIFT
G3DKMD_APICALL int G3DAPIENTRY ion_mirror_set_rotate_info(unsigned long addr, int set_rotate)
{
	struct g3d_ion_buffer *buffer;

	__SEM_LOCK(ionLock);
	buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, addr);
	ion_mirror_get(buffer); /* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	if (buffer == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s (0x%lx): invalid hwaddr\n", __func__, addr);
		return -EINVAL;
	}

	mutex_lock(&buffer->lock);
	buffer->rotate = set_rotate;
	mutex_unlock(&buffer->lock);

	__SEM_LOCK(ionLock);
	ion_mirror_put(buffer); /* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	return 0;
}

G3DKMD_APICALL int G3DAPIENTRY ion_mirror_get_rotate_info(unsigned long addr, int *get_rotate)
{

	struct g3d_ion_buffer *buffer;

	__SEM_LOCK(ionLock);
	buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, addr);
	ion_mirror_get(buffer);	/* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	if (buffer == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s (0x%lx): invalid hwaddr\n", __func__, addr);
		return -EINVAL;
	}

	mutex_lock(&buffer->lock);
	*get_rotate = buffer->rotate;
	mutex_unlock(&buffer->lock);

	__SEM_LOCK(ionLock);
	ion_mirror_put(buffer); /* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	return 0;
}
#else
G3DKMD_APICALL int G3DAPIENTRY ion_mirror_set_rotate_info(unsigned long addr, int set_rotate,
							  int set_rot_dx, int set_rot_dy)
{
	struct g3d_ion_buffer *buffer;

	__SEM_LOCK(ionLock);
	buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, addr);
	ion_mirror_get(buffer); /* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	if (buffer == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s (0x%lx): invalid hwaddr\n", __func__, addr);
		return -EINVAL;
	}


	mutex_lock(&buffer->lock);
	buffer->rotate = set_rotate;
	buffer->rot_dx = set_rot_dx;
	buffer->rot_dy = set_rot_dy;
	mutex_unlock(&buffer->lock);

	__SEM_LOCK(ionLock);
	ion_mirror_put(buffer);	/* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	return 0;
}

G3DKMD_APICALL int G3DAPIENTRY ion_mirror_get_rotate_info(unsigned long addr, int *get_rotate,
							  int *get_rot_dx, int *get_rot_dy)
{
	struct g3d_ion_buffer *buffer;

	__SEM_LOCK(ionLock);
	buffer = (struct g3d_ion_buffer *)radix_tree_lookup(&gIonMapper, addr);
	ion_mirror_get(buffer);	/* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	if (buffer == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ION, "%s (0x%lx): invalid hwaddr\n", __func__, addr);
		return -EINVAL;
	}

	mutex_lock(&buffer->lock);
	*get_rotate = buffer->rotate;
	*get_rot_dx = buffer->rot_dx;
	*get_rot_dy = buffer->rot_dy;
	mutex_unlock(&buffer->lock);

	__SEM_LOCK(ionLock);
	ion_mirror_put(buffer);	/* check buffer == NULL */
	__SEM_UNLOCK(ionLock);

	return 0;
}
#endif

#ifdef CMODEL
void ion_mirror_set_pointers(void *g_ion_device, void *ion_mm_heap_register_buf_destroy_callback,
			     void *ion_free, void *ion_client_create, void *ion_import_dma_buf)
{
	g_ion_device_in_gpu = (struct ion_device *)g_ion_device;
	ion_mm_heap_register_buf_destroy_callback_in_gpu =
	    (int (*)(struct ion_buffer *, ion_mm_buf_destroy_callback_t *))ion_mm_heap_register_buf_destroy_callback;
	ion_free_in_gpu = (void (*)(struct ion_client *, struct ion_handle *))ion_free;
	ion_client_create_in_gpu =
	    (struct ion_client * (*)(struct ion_device *, const char *))ion_client_create;
	ion_import_dma_buf_in_gpu =
	    (struct ion_handle * (*)(struct ion_client *client, int fd))ion_import_dma_buf;
}
EXPORT_SYMBOL(ion_mirror_set_pointers);
#endif
#endif /* ANDROID */
