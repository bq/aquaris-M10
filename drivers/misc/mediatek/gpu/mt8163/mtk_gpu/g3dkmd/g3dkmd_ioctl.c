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
#include "g3dkmd_power.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/memory.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/stacktrace.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h> /* copy_*_user() */
#include <asm/atomic.h> /* for atomic operations */
#include <linux/mm.h> /* mmap */
#include <linux/interrupt.h>
#include <linux/vmalloc.h> /* PATTERN_DUMP */
#include <linux/sched.h>
#include <linux/version.h>
#if defined(SEMU_QEMU) || defined(QEMU64)
#undef CONFIG_OF
#else
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#ifdef USING_MISC_DEVICE
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#endif

#if defined(G3D_NONCACHE_BUFFERS) && defined(__aarch64__)
#if 1 /* built-in linux kernel */
#include <asm/cacheflush.h>
#ifdef SEMU64
/* __dma_flush_range is for built-in kernel. */
/* g3dkmd is not built-in kernel on SEMU, so it doesn't know __dma_flush_range. */
/* in order to solve it, there are three solutions which are listed below. */
/* 1. try to use cache_range which is already removed. */
/* 2. add another function such as yl_dma_flush_range() and export its symbol */
/* 3. use cacheable mode */
#define dmac_flush_range yl_dma_flush_range
#else /* !SEMU64 */
#define dmac_flush_range __dma_flush_range
#endif /* SEMU64 */
#else   /* external module */
/*
#define dmac_flush_range cache_flush_range
extern void cache_flush_range(const void *va_start, const void *va_end);
*/
#endif
#endif

#include "g3dkmd_api.h"
#include "g3dbase_ioctl.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_signal.h"
#include "g3dkmd_macro.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_msgq.h"
#include "g3dkmd_util.h"
#include "g3dkmd_ion.h"
#include "g3d_memory.h"

#if defined(G3D_HW) && defined(G3DKMD_SUPPORT_ISR)
#include <linux/interrupt.h>
#include "g3dkmd_isr.h"
#endif

#ifdef G3DKMD_SUPPORT_FENCE
#include "g3dkmd_fence.h"
#endif

#ifdef G3D_IOCTL_MAKE_VA_LOOKUP
#include "mmu/yl_mmu_mapper_public_class.h"
#endif

#ifdef G3DKMD_HACK_FRAME_CMD_DMA
#include "g3dkmd_internal_memory.h"
#endif

#include "mmu/yl_mmu_kernel_alloc.h"

#if defined(G3D_PROFILER)
#include "g3dkmd_profiler.h"
#endif /* (G3D_PROFILER) */

#if defined(G3DKMD_USE_BUFFERFILE)
#include "bufferfile/g3dkmd_bufferfile_io.h"
#endif

#if defined(G3D_PROFILER)
#include "g3dkmd_profiler.h"
#if defined(G3D_PROFILER_RINGBUFFER_STATISTIC)
#include "profiler/g3dkmd_prf_ringbuffer.h"
#endif
#endif


MODULE_LICENSE("GPL");

#define DEV_BUFSIZE         1024

#ifdef USING_MISC_DEVICE
#define g3d_dts_name "mediatek,g3d_sapphire_lite"
/*#define g3d_iommu_dts_name "mediatek,G3D_IOMMU"*/
#define g3d_iommu_dts_name "mediatek,mt8163-g3d_iommu"


static char g3d_dev_name[] = G3D_NAME;
static struct miscdevice g3d_miscdevice = { 0, };
#else
static int g3d_major;
static int g3d_minor;
struct cdev *g3d_cdevp = NULL;
#endif

/* misc device's platorm device access function and its storage variable */
#ifdef USING_MISC_DEVICE
static struct platform_device *g_g3d_pdev;

const struct platform_device *g3dGetPlatformDevice(void)
{
	return g_g3d_pdev;
}
EXPORT_SYMBOL(g3dGetPlatformDevice);

/* only allow yolig3d_probe set this global value. */
static void _g3dStorePlatformDevice(struct platform_device *dev)
{
	g_g3d_pdev = dev;
}
#endif /* USING_MISC_DEVICE */


static int _g3d_NC_Page_count;

inline struct page *_g3dkmd_NC_allocpages(gfp_t gfp_mask)
{
	struct page *page = alloc_pages(gfp_mask, 0);

	if (page != 0) {
		_g3d_NC_Page_count++;
		KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL,
				"[NON_CACHE_MEMORY]: page:%p (%d)\n", page, _g3d_NC_Page_count);

#if defined(G3D_NONCACHE_BUFFERS) && defined(__aarch64__)
		/* must flush cache in ARM64, or RAM may be overwritten when cache line is evicted */
		{
			void *ptr = vmap(&page, 1, VM_MAP, PAGE_KERNEL);

			dmac_flush_range(ptr, ptr + PAGE_SIZE);
			vunmap(ptr);
		}
#endif

		return page;
	}

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
		"[NON_CACHE_MEMORY]: alloc page fail. (%d)\n", _g3d_NC_Page_count);

	return NULL;
}

inline void _g3dkmd_NC_free_pages(struct page *page)
{
	/* alert first if page is not valid */
	YL_KMD_ASSERT(pfn_valid(page_to_pfn(page)));
	_g3d_NC_Page_count--;
	KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL,
		   "[NON_CACHE_MEMORY]: page:%p (%d)\n", page, _g3d_NC_Page_count);
	__free_pages(page, 0);
}


static __DEFINE_SEM_LOCK(memCleanLock);

static struct kmem_cache *g3d_memlist_cachep;


void g3d_addMemList(g3dkmd_MemListNode **head, uintptr_t index, uintptr_t data0, unsigned int data1)
{
	g3dkmd_MemListNode *pNode = (g3dkmd_MemListNode *) kmem_cache_alloc(g3d_memlist_cachep, GFP_KERNEL);

	if (pNode == NULL)
		return;

	__SEM_LOCK(memCleanLock);
	pNode->index = index;
	pNode->data0 = data0;
	pNode->data1 = data1;
	pNode->pNext = *head;
	*head = pNode;
	__SEM_UNLOCK(memCleanLock);
}

void g3d_delMemList(g3dkmd_MemListNode **head, uintptr_t index)
{
	g3dkmd_MemListNode *pNode = *head;
	g3dkmd_MemListNode *pPrevNode = NULL;
	int nfound = 0;

	__SEM_LOCK(memCleanLock);
	while (pNode != NULL) {
		if (index == pNode->index) {
			if (pPrevNode == NULL)
				*head = pNode->pNext;
			else
				pPrevNode->pNext = pNode->pNext;

			kmem_cache_free(g3d_memlist_cachep, pNode);
			nfound = 1;
			break;
		}

		pPrevNode = pNode;
		pNode = pNode->pNext;
	}
	__SEM_UNLOCK(memCleanLock);

	YL_KMD_ASSERT(nfound);
	/* YL_KMD_ASSERT(pNode != NULL); */
}

void g3d_cleanMemList(g3dkmd_MemListNode **head, int nClean)
{
	g3dkmd_MemListNode *pNode;

	__SEM_LOCK(memCleanLock);
	while (*head != NULL) {
		pNode = *head;
		*head = pNode->pNext;

		switch (nClean) {
		case 0:
			g3dKmdReleaseContext((int)pNode->index, 1);
			break;
		case 1:
			g3dKmdPhyFree((uint64_t) pNode->index, 0);
			break;
		case 2:
			/* if vmalloc VIRT_ALLOC way */
			/* yl_mmu_free((void*)(uintptr_t)pNode->data0); */
			/* if USERPAGE way */
			g3dKmdUserPageFree(pNode->index, (unsigned int)pNode->data0);
			break;
		case 3:
			g3dKmdMmuUnmap((void *)pNode->data0, (unsigned int)pNode->index, pNode->data1);
			break;
		case 4:
			_g3dkmd_NC_free_pages((struct page *)pNode->index);
			break;
		}

		kmem_cache_free(g3d_memlist_cachep, pNode);
	}
	__SEM_UNLOCK(memCleanLock);
	/* *head = NULL; */
}

#define ADD_TASK_LIST(private_data, idx) \
do { \
	if (private_data) \
		g3d_addMemList(&((g3d_private_data *)private_data)->gTaskhead, idx , 0, 0); \
} while (0)
#define DEL_TASK_LIST(private_data, idx) \
do { \
	if (private_data) \
		g3d_delMemList(&((g3d_private_data *)private_data)->gTaskhead, idx); \
} while (0)
#define ADD_PHYMEM_LIST(private_data, idx) \
do { \
	if (private_data) \
		g3d_addMemList(&((g3d_private_data *)private_data)->gPhyMemhead, idx , 0, 0); \
} while (0)
#define DEL_PHYMEM_LIST(private_data, idx) \
do { \
	if (private_data) \
		g3d_delMemList(&((g3d_private_data *)private_data)->gPhyMemhead, idx); \
} while (0)
#define ADD_VIRTMEM_LIST(private_data, idx , data) \
do { \
	if (private_data) \
		g3d_addMemList(&((g3d_private_data *)private_data)->gVirtMemhead, idx , data, 0); \
} while (0)
#define DEL_VIRTMEM_LIST(private_data, idx) \
do { \
	if (private_data) \
		g3d_delMemList(&((g3d_private_data *)private_data)->gVirtMemhead, idx); \
} while (0)
#define ADD_G3DMMU_LIST(private_data, idx , data0, data1) \
do { \
	if (private_data) \
		g3d_addMemList(&((g3d_private_data *)private_data)->gG3DMMUhead, idx, \
			       (uintptr_t)data0, (unsigned int)data1); \
} while (0)
#define DEL_G3DMMU_LIST(private_data, idx) \
do { \
	if (private_data) \
		g3d_delMemList(&((g3d_private_data *)private_data)->gG3DMMUhead, idx); \
} while (0)

#define ADD_VMA_PAGE_LIST(private_data, idx) \
do { \
	if (&private_data) \
		g3d_addMemList((g3dkmd_MemListNode **)(&private_data), idx , 0, 0); \
} while (0)
#define DEL_VMA_PAGE_LIST(private_data, idx) \
do { \
	if (&private_data) \
		g3d_delMemList((g3dkmd_MemListNode **)(&private_data), idx); \
} while (0)


static __DEFINE_SEM_LOCK(backtraceLock);
void g3dkmd_backtrace(void)
{
	__SEM_LOCK(backtraceLock);
	pr_warn("====[ backtrace begin ]===========\n");
	{
#if 1
#define STACK_DEPTH 16
		static unsigned long entries[STACK_DEPTH];
		static struct stack_trace trace = {
			.skip = 1,	/* skip the last one entries */
			.entries = entries,
			.max_entries = ARRAY_SIZE(entries),
		};
		trace.nr_entries = 0;
		save_stack_trace(&trace);
		print_stack_trace(&trace, 2);
#else
		dump_stack();
#endif
	}
	pr_warn("====[ backtrace end ]===========\n");
	__SEM_UNLOCK(backtraceLock);
}

static int g3d_open(struct inode *inode, struct file *filp)
{
	struct task_struct *task = current->group_leader;
	char task_comm[TASK_COMM_LEN];
	g3d_private_data *p_private_data;

	(void)p_private_data;

	/* change to do things like get_task_comm in here due to some old kernel not export get_task_comm */
	task_lock(task);
	strncpy(task_comm, task->comm, sizeof(task->comm));
	task_unlock(task);

	/* 0. create file->private_data if need. */
	if (sizeof(g3d_private_data)) {
		g3d_private_data *p_private_data = G3DKMDVMALLOC(sizeof(g3d_private_data));

		filp->private_data = p_private_data;

		/* 1. memory cleanup-list */
		p_private_data->gTaskhead = NULL;
		p_private_data->gPhyMemhead = NULL;
		p_private_data->gVirtMemhead = NULL;
		p_private_data->gG3DMMUhead = NULL;

#if defined(G3DKMD_USE_BUFFERFILE)
		/* 2. bufferfile initialization */
		_g3dKmdBufferFileDeviceOpenInit(filp);
#endif

#if defined(G3D_PROFILER)
		/* 3. profiler initialzation */
		_g3dKmdProfilerDeviceInit(&(p_private_data->g3dkmd_profiler_private_data));
#endif
	}

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "(%s): private_data = 0x%p\n", task_comm, filp->private_data);


	g3dKmdDeviceOpen((void *)filp);


	return 0;
}

static int g3d_release(struct inode *inode, struct file *filp)
{
	g3dKmdDeviceRelease((void *)filp);

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL, "private_data = 0x%p\n", filp->private_data);

	if (filp->private_data) {
		g3d_private_data *p_private_data = filp->private_data;

#if defined(G3D_PROFILER)
		/* 3. profiler terminate */
		_g3dKmdProfilerDeviceTerminate(p_private_data->g3dkmd_profiler_private_data);
#endif

		/* 1. memory cleanup */
		/* clean the non-destroy task if program exit abnormally */
		/* the first param is set to 0 due to unused in g3dKmdReleaseContext */
		g3d_cleanMemList(&p_private_data->gTaskhead, 0);
		g3d_cleanMemList(&p_private_data->gPhyMemhead, 1);
		g3d_cleanMemList(&p_private_data->gVirtMemhead, 2);
		g3d_cleanMemList(&p_private_data->gG3DMMUhead, 3);

#if defined(G3DKMD_USE_BUFFERFILE)
		/* 2. buffer file terminate */
		_g3dKmdBufferFileDeviceRelease(filp);
#endif /* G3DKMD_USE_BUFFERFILE */

		/* 0. release file->private_data */
		G3DKMDVFREE(filp->private_data);
		filp->private_data = NULL;
	}
#ifdef SUPPORT_MPD
	MPDReleaseUser();
#endif

	return 0;
}

/* static int g3d_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long args) */
long g3d_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	int err = 0;

	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "cmd = 0x%x, NR %d\n", cmd, _IOC_NR(cmd))); */
	if (_IOC_TYPE(cmd) != G3D_IOCTL_MAGIC) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL, "[IOCTL] error on magic check\n");
		return -ENOTTY;
	}
	if (_IOC_NR(cmd) > G3D_IOCTL_MAXNR) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL, "[IOCTL] error on number check\n");
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok(VERIFY_WRITE, (void __user *)args, _IOC_SIZE(cmd));
		if (err) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL, "[IOCTL] err on read check\n");
			return -EFAULT;
		}
	}
	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err = !access_ok(VERIFY_READ, (void __user *)args, _IOC_SIZE(cmd));
		if (err) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL, "[IOCTL] err on write check\n");
			return -EFAULT;
		}
	}

	switch (cmd) {

	case G3D_IOCTL_REGISTERCONTEXT:
	{
		g3dkmd_context context;

		ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_REGISTERCONTEXT");
		if (copy_from_user((void *)&context, (const void __user *)args, sizeof(g3dkmd_context)) == 0) {
			G3DKMD_DEV_REG_PARAM param;

			if (context.g3dbaseioctl_all_struct_size != sizeof(g3dbaseioctl_all_struct)) {
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
				       "[IOCTL] Error! g3dbaseioctl_all_struct size not match\n");
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
				       "[IOCTL]     g3d user size = %u\n", context.g3dbaseioctl_all_struct_size);
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
				       "[IOCTL]     g3dkmd size = %u\n", sizeof(g3dbaseioctl_all_struct));

				BUG_ON(context.g3dbaseioctl_all_struct_size != sizeof(g3dbaseioctl_all_struct));
			}


			param.cmdBufStart = context.cmdBufStart;
			param.cmdBufSize = context.cmdBufSize;
			param.flag = context.flag;
			param.usrDfrEnable = context.usrDfrEnable;
			param.dfrVaryStart = context.dfrVaryStart;
			param.dfrVarySize = context.dfrVarySize;
			param.dfrBinhdStart = context.dfrBinhdStart;
			param.dfrBinhdSize = context.dfrBinhdSize;
			param.dfrBinbkStart = context.dfrBinbkStart;
			param.dfrBinbkSize = context.dfrBinbkSize;
			param.dfrBintbStart = context.dfrBintbStart;
			param.dfrBintbSize = context.dfrBintbSize;

			context.dispatchIndex = g3dKmdRegisterContext(&param);
			ADD_TASK_LIST(filp->private_data, context.dispatchIndex);
			/*
			KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_REGISTERCONTEXT(0x%x)\n",
				context.dispatchIndex));
			*/
			if (copy_to_user((void __user *)args, (const void *)&context, sizeof(g3dkmd_context)) != 0) {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
		} else {
			ATRACE_FUNC_IOCTL_END();
			return -EFAULT;
		}
		ATRACE_FUNC_IOCTL_END();
	}
		break;
	case G3D_IOCTL_RELEASECONTEXT:
		{
			g3dkmd_context context;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_RELEASECONTEXT");
			if (copy_from_user((void *)&context, (const void __user *)args, sizeof(g3dkmd_context)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_RELEASECONTEXT(0x%x)\n",
					context.dispatchIndex));
				*/
				g3dKmdReleaseContext(context.dispatchIndex, 0);
				DEL_TASK_LIST(filp->private_data, context.dispatchIndex);
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_FLUSHCOMMAND:
		{
			g3dkmd_flushCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_FLUSHCOMMAND");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_flushCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_FLUSHCOMMAND(%d,%p,%p)\n",
					cmd.dispatchIndex,cmd.head,cmd.tail));
				*/
#ifdef G3D_SWRDBACK_RECOVERY
				cmd.result = (uint32_t) g3dKmdFlushCommand(cmd.dispatchIndex,
								  (void *)(uintptr_t) cmd.head,
								  (void *)(uintptr_t) cmd.tail,
								  cmd.fence,
								  cmd.lastFlushStamp, cmd.carry,
								  cmd.lastFlushPtr,
								  cmd.flushDCache);
#else
				cmd.result = (uint32_t) g3dKmdFlushCommand(cmd.dispatchIndex,
								  (void *)(uintptr_t) cmd.head,
								  (void *)(uintptr_t) cmd.tail,
								  cmd.fence,
								  cmd.lastFlushStamp, cmd.carry,
								  cmd.flushDCache);
#endif

				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(g3dkmd_flushCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_LTCFLUSH:
		{
			G3DKMD_IOCTL_1_ARG cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_LTCFLUSH");
			cmd.auiArg = g3dKmdLTCFlush();
			if (copy_to_user((void __user *)args, (const void *)&cmd, sizeof(G3DKMD_IOCTL_1_ARG)) != 0) {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_PATTERN_CONFIG:
		{
			G3DKMD_PATTERN_CFG cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_PATTERN_CONFIG");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(G3DKMD_PATTERN_CFG)) == 0) {
				g3dKmdPatternConfig(&cmd);
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
#if 0
	case G3D_IOCTL_MMUINIT:
		{
			g3dkmd_MMUCmd cmd;

			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_MMUCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_MMUINIT(%u)\n", cmd.va_base));
				*/
				cmd.mapper = (uint64_t) (uintptr_t) g3dKmdMmuInit(cmd.va_base);
				if (copy_to_user((void __user *)args, (const void *)&cmd, sizeof(g3dkmd_MMUCmd)) != 0)
					return -EFAULT;
			} else {
				return -EFAULT;
			}
		}
		break;
	case G3D_IOCTL_MMUUNINIT:
		{
			g3dkmd_MMUCmd cmd;

			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_MMUCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_MMUUNINIT(%p)\n",
					(void*)(uintptr_t)cmd.mapper));
				*/
				g3dKmdMmuUninit((void *)(uintptr_t) cmd.mapper);
				if (copy_to_user((void __user *)args, (const void *)&cmd, sizeof(g3dkmd_MMUCmd)) != 0)
					return -EFAULT;
			} else {
				return -EFAULT;
			}
		}
		break;
#endif
	case G3D_IOCTL_MMUGETHANDLE:
		{
			g3dkmd_MMUCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_MMUGETHANDLE");
			/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_MMUGETHANDLE\n")); */
			cmd.mapper = (uint64_t) (uintptr_t) g3dKmdMmuGetHandle();
			if (copy_to_user((void __user *)args, (const void *)&cmd, sizeof(g3dkmd_MMUCmd)) != 0) {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			 ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_G3DVATOPA:
		{
			g3dkmd_MMUCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_G3DVATOPA");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_MMUCmd)) == 0) {
#if defined(PHY_ADDR_FROM_VIRT_ADDR)
				cmd.pa = cmd.g3d_va;
#else
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_G3DVATOPA(%p %u)\n",
					(void*)(uintptr_t)cmd.mapper, cmd.g3d_va));
				*/
				cmd.pa = g3dKmdMmuG3dVaToPa((void *)(uintptr_t) cmd.mapper, cmd.g3d_va);
#endif
				if (copy_to_user((void __user *)args, (const void *)&cmd, sizeof(g3dkmd_MMUCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
#ifdef YL_MMU_PATTERN_DUMP_V2
	case G3D_IOCTL_G3DVATOPA_CHUNK:
		{
			g3dkmd_MMUCmdChunk cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_G3DVATOPA_CHUNK");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_MMUCmdChunk)) == 0) {
				if (cmd.mapper == 0)
					return -EFAULT;
				if (cmd.g3d_va == 0)
					return -EFAULT;
				if (cmd.size == 0)
					return -EFAULT;

#if defined(PHY_ADDR_FROM_VIRT_ADDR)
				cmd.chunk_pa = cmd.g3d_va;
				cmd.chunk_size = cmd.size;
#else
				g3dKmdMmuG3dVaToPaChunk((void *)(uintptr_t) cmd.mapper, cmd.g3d_va,
							cmd.size, &cmd.chunk_pa, &cmd.chunk_size);
#endif
				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(g3dkmd_MMUCmdChunk)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
#endif
	case G3D_IOCTL_MMUMAP:
		{
			g3dkmd_MMUCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_MMUMAP");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_MMUCmd)) == 0) {
#if defined(PHY_ADDR_FROM_VIRT_ADDR)
				cmd.g3d_va = cmd.va;
#else
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_MMUMAP(%p %p %u %d)\n",
					(void*)(uintptr_t)cmd.mapper, (void*)(uintptr_t)cmd.va,
					cmd.size, cmd.could_be_section));
				*/
				cmd.g3d_va = g3dKmdMmuMap((void *)(uintptr_t) cmd.mapper,
						 (void *)(uintptr_t) cmd.va, cmd.size,
						 cmd.could_be_section, cmd.flag);
				if (cmd.g3d_va != 0)
					ADD_G3DMMU_LIST(filp->private_data, cmd.g3d_va, cmd.mapper, cmd.size);

#endif
				if (copy_to_user((void __user *)args, (const void *)&cmd, sizeof(g3dkmd_MMUCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_MMUUNMAP:
		{
			g3dkmd_MMUCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_MMUUNMAP");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_MMUCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_MMUUNMAP(%p %u)\n",
					(void*)(uintptr_t)cmd.mapper, cmd.g3d_va));
				*/
#if !defined(PHY_ADDR_FROM_VIRT_ADDR)
				g3dKmdMmuUnmap((void *)(uintptr_t) cmd.mapper, cmd.g3d_va, cmd.size);
				if (cmd.g3d_va != 0)
					DEL_G3DMMU_LIST(filp->private_data, cmd.g3d_va);
#endif
				if (copy_to_user((void __user *)args, (const void *)&cmd, sizeof(g3dkmd_MMUCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_MMU_RANGE_SYNC:
		{
			g3dkmd_MMUCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_RANGE_SYNC");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_MMUCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_MMU_RANGE_SYNC (%p %u %u %u %d)\n",
					cmd.mapper, cmd.va, cmd.size, cmd.g3d_va, cmd.cache_action);
				*/
				if (!g3dKmdMmuRangeSync((void *)(uintptr_t) cmd.mapper, (unsigned long)cmd.va,
							     (unsigned int)cmd.size, cmd.g3d_va, cmd.cache_action)) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_MAKE_VA_LOOKUP:
		{
			g3dkmd_LookbackCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_MAKE_VA_LOOKUP");
#if MMU_LOOKBACK_VA
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_LookbackCmd)) == 0) {
				if (cmd.mapper == 0)
					return -EFAULT;
				cmd.hwAddr = ((struct Mapper *) (uintptr_t) cmd.mapper)->lookback_mem.hwAddr;
				cmd.size = ((struct Mapper *) (uintptr_t) cmd.mapper)->lookback_mem.size;
				if (cmd.hwAddr == 0)
					return -EFAULT;
				cmd.G3d_va_base = (unsigned int)((struct Mapper *) (uintptr_t) cmd.mapper)->G3d_va_base;
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_MAKE_VA_LOOKUP(%p) = 0x%x\n",
					(void*)(uintptr_t)cmd.mapper, cmd.hwAddr));
				*/
				if (copy_to_user((void __user *)args, (const void *)&cmd,
						  sizeof(g3dkmd_LookbackCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else
#endif
			{
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_PHY_ALLOC:
		{
			g3dkmd_PhyMemCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_PHY_ALLOC");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_PhyMemCmd)) == 0) {

				cmd.phyAddr = g3dKmdPhyAlloc(cmd.size);


				if (cmd.phyAddr == 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}


#ifndef G3DKMD_HACK_FRAME_CMD_DMA
				ADD_PHYMEM_LIST(filp->private_data, cmd.phyAddr);
#endif

				if (copy_to_user((void __user *)args, (const void *)&cmd,
						  sizeof(g3dkmd_PhyMemCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_PHY_FREE:
		{
			g3dkmd_PhyMemCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_PHY_FREE");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_PhyMemCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_PHY_FREE(0x%lx)\n",
					cmd.phyAddr));
				*/
				g3dKmdPhyFree(cmd.phyAddr, 0);
				DEL_PHYMEM_LIST(filp->private_data, cmd.phyAddr);
				if (copy_to_user((void __user *)args, (const void *)&cmd,
						  sizeof(g3dkmd_PhyMemCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_HW_ALLOC:
		{
			g3dkmd_HWAllocCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_HW_ALLOC");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_HWAllocCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_HW_ALLOC(phyAddr %llu, size %u)\n", cmd.phyAddr, cmd.size));
				*/
				cmd.hwAddr = g3dKmdAllocHw(cmd.phyAddr, cmd.size);
				if (cmd.hwAddr == 0)
					return -EFAULT;
				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(g3dkmd_HWAllocCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_HW_FREE:
		{
			g3dkmd_HWAllocCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_HW_FREE");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(g3dkmd_HWAllocCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_HW_FREE(hwAddr %u, size %u)\n", cmd.hwAddr, cmd.size));
				*/
				g3dKmdFreeHw(cmd.hwAddr, cmd.size);
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_GET_MSG:
		{
			g3dkmd_DataCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_GET_MSG");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_DataCmd)) == 0) {
				G3DKMD_MSGQ_TYPE type;
				uint32_t size;
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_GET_MSG(type %u, size %u)\n",
					cmd.arg[0], cmd.arg[1]));
				*/

				size = cmd.arg[1];
				if (g3dKmdMsgQReceive(&type, &size,
					(void __user *)(uintptr_t) cmd.addr) == G3DKMD_MSGQ_RTN_EMPTY) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}

				cmd.arg[0] = (uint32_t) type;
				cmd.arg[1] = size;

				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(g3dkmd_DataCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_USERPAGE_GET:
		{
			g3dkmd_VirtMemCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_USERPAGE_GET");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_VirtMemCmd)) == 0) {
				int ret;
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_USERPAGE_GET(0x%x 0x%x)\n",
					cmd.virtAddr, cmd.size));
				*/
				if (cmd.virtAddr == 0)
					return -EFAULT;
				ret = g3dKmdUserPageGet((unsigned long)cmd.virtAddr, cmd.size,
							(uintptr_t *) &cmd.pageArray, &cmd.nrPages);
				if (ret <= 0)
					return (ret == 0) ? -EFAULT : ret;
				ADD_VIRTMEM_LIST(filp->private_data, (uintptr_t) cmd.pageArray, cmd.nrPages);
				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(g3dkmd_VirtMemCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_USERPAGE_FREE:
		{
			g3dkmd_VirtMemCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_USERPAGE_FREE");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_VirtMemCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_USERPAGE_FREE(0x%x 0x%x 0x%x)\n",
					cmd.virtAddr, cmd.size, cmd.nrPages));
				*/
				g3dKmdUserPageFree(cmd.pageArray, cmd.nrPages);
				DEL_VIRTMEM_LIST(filp->private_data, cmd.pageArray);
				if (copy_to_user((void __user *)args, (const void *)&cmd,
						  sizeof(g3dkmd_VirtMemCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_CALLBACK_TEST:
		{

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_CALLBACK_TEST");
#ifdef G3DKMD_CALLBACK_INT
			G3DKMD_IOCTL_1_ARG cmd;

			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(G3DKMD_IOCTL_1_ARG)) == 0) {
				pid_t signal_pid = g3dKmdGetTaskProcess(cmd.auiArg);

#ifdef G3DKMD_CALLBACK_FROM_KEYBOARD_INT
				init_sim_keyboard_int(signal_pid);
#elif defined(G3DKMD_CALLBACK_FROM_TIMER)
				init_sim_timer(signal_pid);
#else
				if (signal_pid != 0)
					trigger_callback(signal_pid);
#endif
			}
#endif
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_VIRT_ALLOC:
		{
			g3dkmd_Virt2MemCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_VIRT_ALLOC");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_Virt2MemCmd)) == 0) {
				cmd.virtAddr = (uint64_t) (uintptr_t) yl_mmu_alloc(cmd.size);
									/* g3dKmdPhyAlloc(cmd.resType, cmd.size); */
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       "G3D_IOCTL_VIRT_ALLOC vaddr %p, size 0x%x, pfn 0x%lx\n",
				       (void *)(uintptr_t) cmd.virtAddr, cmd.size,
				       vmalloc_to_pfn((void *)(uintptr_t) cmd.virtAddr));
				if (cmd.virtAddr == 0)
					return -EFAULT;
				ADD_VIRTMEM_LIST(filp->private_data, (uintptr_t) cmd.virtAddr,
						 cmd.virtAddr);
				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(g3dkmd_Virt2MemCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_VIRT_FREE:
		{
			g3dkmd_Virt2MemCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_VIRT_FREE");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(g3dkmd_Virt2MemCmd)) == 0) {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       "G3D_IOCTL_VIRT_FREE(%p)\n",
				       (void *)(uintptr_t) cmd.virtAddr);
				yl_mmu_free((void *)(uintptr_t) cmd.virtAddr);
				DEL_VIRTMEM_LIST(filp->private_data, cmd.virtAddr);
				/* DEL_VIRT2MEM_LIST(filp->private_data, cmd.virtAddr); */
				/* if( copy_to_user ( (void __user *)args,(const void *)&cmd,
						  sizeof(g3dkmd_Virt2MemCmd)) != 0)
					return -EFAULT;
				*/
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
#ifdef G3D_SUPPORT_SWRDBACK
	case G3D_IOCTL_SWRDBACK_ALLOC:
		{
			g3dkmd_GetSwRdbackMemCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_SWRDBACK_ALLOC");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(g3dkmd_GetSwRdbackMemCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_SWRDBACK_ALLOC(%u)\n", cmd.taskID));
				*/
				cmd.hwAddr = 0;
				cmd.phyAddr = g3dKmdSwRdbackAlloc(cmd.taskID, &cmd.hwAddr);
				if (cmd.phyAddr == 0)
					return -EFAULT;
				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(g3dkmd_GetSwRdbackMemCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
#endif /* G3D_SUPPORT_SWRDBACK */
#ifdef G3D_SUPPORT_FLUSHTRIGGER
	case G3D_IOCTL_FLUSHTRIGGER:
		{
			g3dkmd_FlushTriggerCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_FLUSHTRIGGER");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(g3dkmd_FlushTriggerCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				    "G3D_IOCTL_FLUSHTRIGGER(%u, %u, %u)\n", cmd.set, cmd.taskID, cmd.value));
				*/
				g3dKmdFlushTrigger(cmd.set, (int)cmd.taskID, &cmd.value);
				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(g3dkmd_FlushTriggerCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
#endif /* G3D_SUPPORT_FLUSHTRIGGER */
	case G3D_IOCTL_CHECK_HWSTATUS:
		{
			G3DKMD_IOCTL_3_ARG cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_CHECK_HWSTATUS");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(G3DKMD_IOCTL_3_ARG)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_CHECK_HWSTATUS(0x%x 0x%x)\n", cmd.auiArg[0], cmd.auiArg[1])); */
				cmd.auiArg[2] = g3dKmdCheckHWStatus(cmd.auiArg[0], cmd.auiArg[1]);
				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(G3DKMD_IOCTL_3_ARG)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_DUMP_BEGIN:
		{
			G3DKMD_IOCTL_2_ARG cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_DUMP_BEGIN");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(G3DKMD_IOCTL_2_ARG)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_DUMP_BEGIN enter\n")); */
				g3dKmdDumpCommandBegin(cmd.auiArg[0], cmd.auiArg[1]);
				/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_DUMP_BEGIN leave\n")); */
			} else {
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
				       "G3D_IOCTL_DUMP_BEGIN copy_from_user fail\n");
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_DUMP_END:
		{
			G3DKMD_IOCTL_2_ARG cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_DUMP_END");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(G3DKMD_IOCTL_2_ARG)) == 0) {
				/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_DUMP_END enter\n")); */
				g3dKmdDumpCommandEnd(cmd.auiArg[0], cmd.auiArg[1]);
				/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_DUMP_END leave\n")); */
			} else {
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
				       "G3D_IOCTL_DUMP_END copy_from_user fail\n");
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
#if defined(G3DKMD_USE_BUFFERFILE)
	case G3D_IOCTL_BUFFERFILE_DUMP_BEGIN:
		{
			G3DKMD_IOCTL_BF_DUMP_BEGIN_ARG cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_BUFFERFILE_DUMP_BEGIN");

			g3dKmdBufferFileDumpBegin(filp, (unsigned int *)(&cmd.offset), &cmd.length);
			KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
			       " G3D_IOCTL_BUFFERFILE_DUMP_BEGIN(mmap pointer: 0x%lX, length: %u)\n",
			       (unsigned long int)cmd.offset, (unsigned int)cmd.length);
			if (copy_to_user((void __user *)args, (const void *)&cmd,
			     sizeof(G3DKMD_IOCTL_BF_DUMP_BEGIN_ARG)) != 0) {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_BUFFERFILE_DUMP_END:
		{
			G3DKMD_IOCTL_1_ARG cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_BUFFERFILE_DUMP_END");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(G3DKMD_IOCTL_1_ARG)) == 0) {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       " G3D_IOCTL_BUFFERFILE_DUMP_END, isFreeClosedData: %s\n",
				       cmd.auiArg ? "true" : "false");
				g3dKmdBufferFileDumpEnd(filp, cmd.auiArg);
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;


#endif /* G3DKMD_USE_BUFFERFILE */

#ifdef ANDROID
	case G3D_IOCTL_CHECK_DONE:
		{
			g3dkmd_LockCheckCmd cmd;
			struct ionGrallocLockInfo *data;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_CHECK_DONE");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(g3dkmd_LockCheckCmd)) == 0) {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       "G3D_IOCTL_CHECK_DONE(0x%x 0x%x)\n", cmd.num_tasks,
				       cmd.data);
				if (cmd.data == 0)
					return -EFAULT;
				data = (struct ionGrallocLockInfo *)
				    G3DKMDVMALLOC(sizeof(struct ionGrallocLockInfo) * cmd.num_tasks);
				if (copy_from_user((void *)data, (const void __user *)(uintptr_t) cmd.data,
				     sizeof(struct ionGrallocLockInfo) * cmd.num_tasks) == 0) {
					g3dKmdLockCheckDone(cmd.num_tasks, data);
					if (copy_to_user((void __user *)(uintptr_t) cmd.data, (void *)data,
					     sizeof(struct ionGrallocLockInfo) * cmd.num_tasks) != 0) {
						KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
						       "G3D_IOCTL_CHECK_DONE copy_to_user fail\n");
						kfree(data);
						ATRACE_FUNC_IOCTL_END();
						return -EFAULT;
					}
					G3DKMDVFREE(data);
				} else {
					G3DKMDVFREE(data);
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_CHECK_FLUSH:
		{
			g3dkmd_LockCheckCmd cmd;
			struct ionGrallocLockInfo *data;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_CHECK_FLUSH");
			cmd.result = 0;
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(g3dkmd_LockCheckCmd)) == 0) {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       "G3D_IOCTL_CHECK_FLUSH(0x%x 0x%x)\n", cmd.num_tasks,
				       cmd.data);
				if (cmd.data == 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
				data = (struct ionGrallocLockInfo *)
				    G3DKMDVMALLOC(sizeof(struct ionGrallocLockInfo) * cmd.num_tasks);
				if (copy_from_user((void *)data, (const void __user *)(uintptr_t) cmd.data,
				     sizeof(struct ionGrallocLockInfo) * cmd.num_tasks) == 0) {
					cmd.result = g3dKmdLockCheckFlush(cmd.num_tasks, data);
					G3DKMDVFREE(data);
					if (copy_to_user((void __user *)args, (const void *)&cmd,
					     sizeof(g3dkmd_LockCheckCmd)) != 0) {
						ATRACE_FUNC_IOCTL_END();
						return -EFAULT;
					}
				} else {
					G3DKMDVFREE(data);
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_FENCE_CREATE:
		{
			g3dkmd_FenceCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_FENCE_CREATE");
#ifdef G3DKMD_SUPPORT_FENCE


			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(g3dkmd_FenceCmd)) == 0) {
				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					" G3D_IOCTL_FENCE_CREATE(taskID %u, stamp %u)\n", cmd.taskID, cmd.stamp));
				*/

				g3dKmdTaskCreateFence(cmd.taskID, &cmd.fd, cmd.stamp);
				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(g3dkmd_FenceCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else
#endif
			{
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_ION_MIRROR_CREATE:
		{
			int ret;
			ion_mirror_data cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_ION_MIRROR_CREATE");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(ion_mirror_data)) == 0) {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       " G3D_IOCTL_ION_MIRROR_CREATE[%d] 0x%llx %d\n",
				       current->tgid, cmd.addr, cmd.fd);
				ret = ion_mirror_create(cmd.addr, cmd.fd);
				if (ret < 0) {
					ATRACE_FUNC_IOCTL_END();
					return ret;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_ION_MIRROR_DESTROY:
		{
			ion_mirror_data cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_ION_MIRROR_DESTROY");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(ion_mirror_data)) == 0) {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       " G3D_IOCTL_ION_MIRROR_DESTROY[%d] 0x%llx\n", current->tgid, cmd.addr);
				ion_mirror_destroy(cmd.addr);
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_ION_MIRROR_STAMP_SET:
		{
			int ret;
			ion_mirror_stamp_set_data cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_MIRROR_STAMP_SET");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
					    sizeof(ion_mirror_stamp_set_data)) == 0) {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       " G3D_IOCTL_ION_MIRROR_STAMP_SET[%d] 0x%llx 0x%x %u %d %u\n",
				       current->tgid, cmd.addr, cmd.stamp, cmd.readonly, cmd.pid,
				       cmd.taskID);
				ret = ion_mirror_stamp_set(cmd.addr, cmd.stamp, cmd.readonly, cmd.pid, cmd.taskID);
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       " G3D_IOCTL_ION_MIRROR_STAMP_SET[%d] %d\n", current->tgid, ret);
				if (ret < 0)
					ATRACE_FUNC_IOCTL_END();
					return ret;
				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(ion_mirror_stamp_set_data)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_ION_MIRROR_STAMP_GET:
		{
			int ret;
			ion_mirror_stamp_get_data cmd;
			struct ionGrallocLockInfo *info = NULL;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_ION_MIRROR_STAMP_GET");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(ion_mirror_stamp_get_data)) == 0) {
				if ((void __user *)(uintptr_t) cmd.data != NULL)
					info = kmalloc_array(cmd.num_task, sizeof(struct ionGrallocLockInfo),
							     GFP_KERNEL);

				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       " G3D_IOCTL_ION_MIRROR_STAMP_GET[%d] 0x%llx %p\n",
				       current->tgid, cmd.addr, info);
				ret = ion_mirror_stamp_get(cmd.addr, &cmd.num_task, info);
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       " G3D_IOCTL_ION_MIRROR_STAMP_GET[%d] %u %d\n", current->tgid,
				       cmd.num_task, ret);

				if (ret >= 0 && info) {
					if (copy_to_user((void __user *)(uintptr_t) cmd.data, info,
					     sizeof(struct ionGrallocLockInfo) * cmd.num_task)) {
						ATRACE_FUNC_IOCTL_END();
						ret = -EFAULT;
					}
				}


				kfree(info);

				if (ret < 0) {
					ATRACE_FUNC_IOCTL_END();
					return ret;
				}
				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(ion_mirror_stamp_get_data)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_ION_MIRROR_LOCK_UPD:
		{
			int ret = 0;
			ion_mirror_lock_update_data cmd;
			struct ionGrallocLockInfo *info = NULL;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_ION_MIRROR_LOCK_UPD");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(ion_mirror_lock_update_data)) == 0) {
				if ((void __user *)(uintptr_t) cmd.data != NULL) {
					info = kmalloc_array(cmd.num_task, sizeof(struct ionGrallocLockInfo),
							     GFP_KERNEL);
					if (copy_from_user(info, (void __user *)(uintptr_t) cmd.data,
					     sizeof(struct ionGrallocLockInfo) * cmd.num_task)) {
						ATRACE_FUNC_IOCTL_END();
						ret = -EFAULT;
					}
				}
				if (ret >= 0) {
					KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					       " G3D_IOCTL_ION_MIRROR_LOCK_UPD[%d] 0x%llx %u %u %p\n",
					       current->tgid, cmd.addr, cmd.num_task, cmd.readonly, info);
					ret = ion_mirror_lock_update(cmd.addr, cmd.num_task,
								   cmd.readonly, info, &cmd.lock_status);
					KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					       " G3D_IOCTL_ION_MIRROR_LOCK_UPD[%d] %u %d\n",
					       current->tgid, cmd.lock_status, ret);
				}

				kfree(info);

				if (ret < 0) {
					ATRACE_FUNC_IOCTL_END();
					return ret;
				}
				if (copy_to_user((void __user *)args, (const void *)&cmd,
				     sizeof(ion_mirror_lock_update_data)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_ION_MIRROR_UNLOCK_UPD:
		{
			int ret;
			ion_mirror_data cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_ION_MIRROR_UNLOCK_UPD");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(ion_mirror_data)) == 0) {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       " G3D_IOCTL_ION_MIRROR_UNLOCK_UPD[%d] 0x%llx\n", current->tgid, cmd.addr);
				ret = ion_mirror_unlock_update(cmd.addr);
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       " G3D_IOCTL_ION_MIRROR_UNLOCK_UPD[%d] %d\n", current->tgid, ret);
				if (ret < 0) {
					ATRACE_FUNC_IOCTL_END();
					return ret;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_ION_MIRROR_SET_ROTATE_INFO:
		{
			int ret = 0;
			ion_mirror_rotate_data cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_ION_MIRROR_SET_ROTATE_INFO");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
					     sizeof(ion_mirror_rotate_data)) == 0) {
#ifdef YL_PRE_ROTATE_SW_SHIFT
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					" G3D_IOCTL_ION_MIRROR_SET_ROTATE_INFO[%d] 0x%llx %d\n", current->tgid,
					cmd.addr, cmd.rotate);
				ret = ion_mirror_set_rotate_info(cmd.addr, cmd.rotate);
#else
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					" G3D_IOCTL_ION_MIRROR_SET_ROTATE_INFO[%d] 0x%llx %d %d %d\n", current->tgid,
					cmd.addr, cmd.rotate, cmd.rot_dx, cmd.rot_dy);
				ret = ion_mirror_set_rotate_info(cmd.addr, cmd.rotate, cmd.rot_dx, cmd.rot_dy);
#endif
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					" G3D_IOCTL_ION_MIRROR_SET_ROTATE_INFO[%d] %d\n", current->tgid, ret);

				if (ret < 0) {
					ATRACE_FUNC_IOCTL_END();
					return ret;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
	case G3D_IOCTL_ION_MIRROR_GET_ROTATE_INFO:
		{
			int ret = 0;
			ion_mirror_rotate_data cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_ION_MIRROR_GET_ROTATE_INFO");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
					     sizeof(ion_mirror_rotate_data)) == 0) {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					" G3D_IOCTL_ION_MIRROR_GET_ROTATE_INFO[%d] 0x%llx %d\n",
					current->tgid, cmd.addr, ret);
#ifdef YL_PRE_ROTATE_SW_SHIFT
				ret = ion_mirror_get_rotate_info(cmd.addr, &(cmd.rotate));
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					" G3D_IOCTL_ION_MIRROR_GET_ROTATE_INFO[%d] 0x%llx %d %d\n",
					current->tgid, cmd.addr, cmd.rotate, ret);
#else
				ret = ion_mirror_get_rotate_info(cmd.addr, &(cmd.rotate), &(cmd.rot_dx), &(cmd.rot_dy));
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					" G3D_IOCTL_ION_MIRROR_GET_ROTATE_INFO[%d] 0x%llx %d %d %d %d\n",
					current->tgid, cmd.addr, cmd.rotate, cmd.rot_dx, cmd.rot_dy, ret);
#endif
				if (ret < 0) {
					ATRACE_FUNC_IOCTL_END();
					return ret;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}

			if (copy_to_user((void __user *)args, (const void *)&cmd,
					  sizeof(ion_mirror_rotate_data)) != 0) {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
#endif

#ifdef YL_MMU_PATTERN_DUMP_V1
	case G3D_IOCTL_PA_PAIR_INFO:
		{
			g3dkmd_PaPairInfoCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_PA_PAIR_INFO");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
					   sizeof(g3dkmd_PaPairInfoCmd)) == 0) {
				if (cmd.mapper == 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
				if (cmd.g3d_va == 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
				if (cmd.size == 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}

				g3dKmdMmuPaPairBufInfo((void *)cmd.mapper, cmd.g3d_va, cmd.size,
						       &cmd.buffer_size, &cmd.unit_size,
						       &cmd.offset_mask);

				/*
				KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_MAKE_VA_LOOKUP(%p) = 0x%x\n",
					(void*)(uintptr_t)cmd.mapper, cmd.hwAddr));
				*/

				if (copy_to_user((void __user *)args, (const void *)&cmd,
						  sizeof(g3dkmd_PaPairInfoCmd)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
#endif
	case G3D_IOCTL_GET_DRV_INFO:
		{
			g3dkmd_DataCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_GET_DRV_INFO");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(g3dkmd_DataCmd)) == 0) {
				unsigned int drv_cmd = cmd.arg[0]; /* command */
				unsigned int arg = cmd.arg[1]; /* argument */
				unsigned int dataSize = cmd.arg[2]; /* output buffer size */

				void *vdata;
				unsigned int size;

				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       "G3D_IOCTL_GET_DRV_INFO: drv_cmd:%u arg: %u dataSize:%u\n",
				       drv_cmd, arg, dataSize);

				vdata = G3DKMDVMALLOC(dataSize);
				if (!vdata) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}

				size = g3dKmdGetDrvInfo(drv_cmd, arg, vdata, dataSize, (void *)filp);

				if (!size || (size > dataSize)) {
					G3DKMDVFREE(vdata);
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
				cmd.arg[3] = size; /* real copied size */

				if (copy_to_user((void __user *)(uintptr_t) cmd.addr, (const void *)vdata,
						  size) != 0) {
					G3DKMDVFREE(vdata);
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}

				if (copy_to_user((void __user *)args, (const void *)&cmd,
						  sizeof(g3dkmd_DataCmd)) != 0) {
					G3DKMDVFREE(vdata);
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}

				G3DKMDVFREE(vdata);
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_SET_DRV_INFO:
		{
			g3dkmd_DataCmd cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_SET_DRV_INFO");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
			     sizeof(g3dkmd_DataCmd)) == 0) {
				unsigned int drv_cmd = cmd.arg[0]; /* command */
				unsigned int arg = cmd.arg[1]; /* argument */
				unsigned int dataSize = cmd.arg[2]; /* input buffer size */
				unsigned int rtnVal = G3DKMD_TRUE;
				void *vdata = NULL;

				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       "G3D_IOCTL_SET_DRV_INFO: drv_cmd:%u arg: %u dataSize:%u\n",
				       drv_cmd, arg, dataSize);

				if (dataSize > 0) {
					vdata = G3DKMDVMALLOC(dataSize);
					if (!vdata) {
						ATRACE_FUNC_IOCTL_END();
						return -EFAULT;
					}

					if (copy_from_user(vdata, (const void __user *)((uintptr_t) cmd.addr),
							     dataSize) != 0) {
						G3DKMDVFREE(vdata);
						ATRACE_FUNC_IOCTL_END();
						return -EFAULT;
					}
				}

				rtnVal = g3dKmdSetDrvInfo(drv_cmd, arg, vdata, dataSize, filp);

				if (dataSize > 0)
					G3DKMDVFREE(vdata);

				if (rtnVal != G3DKMD_TRUE) {

					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_CL_GET_STAMP:
		{
			g3dkmd_IOCTL_CL_PRINTF_INFO cl_printf_info;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_CL_GET_STAMP");
			KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "Enter G3D_IOCTL_CL_GET_STAMP\n");
			if (copy_from_user((void *)&cl_printf_info, (const void __user *)args,
					     sizeof(g3dkmd_IOCTL_CL_PRINTF_INFO)) == 0) {
				struct g3dTask *task;

				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "found taskID %d\n",
				       cl_printf_info.taskID);
				task = g3dKmdGetTask(cl_printf_info.taskID);
				KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_CL_GET_STAMP task(%p)[%u](isTrigger: %u, waveID: %u)\n",
					task, cl_printf_info.taskID,
					task->clPrintfInfo.isIsrTrigger,
					task->clPrintfInfo.waveID);
				/* wait_until_clprint_happened() */

				{
					unsigned int ret;

					KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
						"going to wait.task->clPrintfInfo.isIsrTrigger =%d\n",
						task->clPrintfInfo.isIsrTrigger);

					ret = g3dkmd_wait_event_interruptible_timeout(
						gExecInfo.s_waitqueue_head_cl,
						(task->clPrintfInfo.isIsrTrigger != 0) ,
						G3DKMD_NON_POLLING_CLPRINT_WAIT_TIMEOUT);
					KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
						"return from wait. Ret = %d\n", ret);
				}

				#if defined(G3D_HW) && defined(G3DKMD_SUPPORT_ISR)
				g3dKmdSuspendIsr();
				#endif
				cl_printf_info.isrTrigger = task->clPrintfInfo.isIsrTrigger;
				cl_printf_info.waveID = task->clPrintfInfo.waveID;
				cl_printf_info.ctxsID = task->clPrintfInfo.ctxsID;

				if (copy_to_user((void __user *)args, (const void *)&cl_printf_info,
						   sizeof(g3dkmd_IOCTL_CL_PRINTF_INFO)) != 0) {

					KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
						"G3D_IOCTL_CL_GET_STAMP copy_to_user fail\n");
					ATRACE_FUNC_IOCTL_END();

					#if defined(G3D_HW) && defined(G3DKMD_SUPPORT_ISR)
					g3dKmdResumeIsr();
					#endif

					return -EFAULT;
				}

				task->clPrintfInfo.isIsrTrigger = 0; /*Wave A is read; however wave B is*/
				#if defined(G3D_HW) && defined(G3DKMD_SUPPORT_ISR)
				g3dKmdResumeIsr();
				#endif

				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_CL_GET_STAMP copy_to_user success\n");
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_CL_GET_STAMP task(%p)[%u](isTrigger: %u, waveID: %u)\n",
					task, cl_printf_info.taskID,
					task->clPrintfInfo.isIsrTrigger,
					task->clPrintfInfo.waveID);

			} else {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_CL_GET_STAMP copy_from_user fail\n");
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;


	case G3D_IOCTL_CL_UPDATE_READ_STAMP:
		{
			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_CL_UPDATE_READ_STAMP");
#ifdef G3D_SIGNAL_CL_PRINT
			/* update gExecInfo.cl_print_read_stamp */
			G3DKMD_IOCTL_1_ARG cmd;

			if (copy_from_user((void *)&cmd, (const void __user *)args,
					     sizeof(G3DKMD_IOCTL_1_ARG)) == 0) {
#ifdef G3DKMD_SIGNAL_CL_PRINT
				gExecInfo.cl_print_read_stamp = cmd.auiArg;
#endif
			} else {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       "G3D_IOCTL_CL_RESUME_DEV resumes hw\n");
			}
#else				/* ! G3D_SIGNAL_CL_PRINT */
			KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
			       "G3D_SIGNAL_CL_PRINT is undefined\n");
#endif /* G3D_SIGNAL_CL_PRINT */
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_CL_RESUME_DEV:
		{
			g3dkmd_IOCTL_CL_PRINTF_INFO cl_printf_info;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_CL_RESUME_DEV");

			KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
			       "G3D_IOCTL_CL_RESUME_DEV resumes hw\n");
			if (copy_from_user((void *)&cl_printf_info, (const void __user *)args,
					   sizeof(g3dkmd_IOCTL_CL_PRINTF_INFO)) == 0) {
				unsigned int syscall_info;
				unsigned int syscall_wp0_info = 0;
				unsigned int syscall_wp2_info = 0;

				struct g3dTask *task = g3dKmdGetTask(cl_printf_info.taskID);

				if (NULL == task)
					return -EFAULT;

				syscall_info = task->clPrintfInfo.waveID;
				/* clear wave to release HW */
				syscall_wp0_info = (syscall_info & 0xFFF);
				/* g3dKmdRegRead(G3DKMD_REG_UX_DWP, REG_UX_DWP_SYSCALL_INFO_0,
					MSK_UX_DWP_SYSCALL_WP0_INFO, SFT_UX_DWP_SYSCALL_WP0_INFO); */    /*[11: 0]*/
				g3dKmdRegWrite(G3DKMD_REG_UX_DWP, REG_UX_DWP_SYSCALL_INFO_0, syscall_wp0_info,
						MSK_UX_DWP_SYSCALL_WP0_INFO, SFT_UX_DWP_SYSCALL_WP0_INFO);

				syscall_wp2_info = (syscall_info & 0xFFF000);
				/* g3dKmdRegRead(G3DKMD_REG_UX_DWP,REG_UX_DWP_SYSCALL_INFO_0,
					MSK_UX_DWP_SYSCALL_WP2_INFO, SFT_UX_DWP_SYSCALL_WP2_INFO); */
					/* [27:16]   1111 1111 1111 0000 0000 0000 0000 */
				g3dKmdRegWrite(G3DKMD_REG_UX_DWP, REG_UX_DWP_SYSCALL_INFO_0, syscall_wp2_info,
						MSK_UX_DWP_SYSCALL_WP2_INFO, SFT_UX_DWP_SYSCALL_WP2_INFO);
			}

			KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "G3D_IOCTL_CL_RESUME_DEV resumes hw success\n");
			ATRACE_FUNC_IOCTL_END();
		}
		break;

	case G3D_IOCTL_HT_READ_WRITE_EQUAL:
		{
			G3DKMD_BOOL cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_HT_READ_WRITE_EQUAL");
			if (copy_from_user((void *)&cmd, (const void __user *)args, sizeof(G3DKMD_BOOL)) == 0) {
				unsigned int rptr = 0;
				unsigned int wptr = 0;

				struct g3dExeInst *pInst = gExecInfo.currentExecInst;

				if (pInst) {
					rptr = g3dKmdRegGetReadPtr(pInst->hwChannel);

					/* Louis not sure it should be HwRead or SwWrite for Ht */
					wptr = pInst->pHtDataForHwRead->htInfo->wtPtr;
					cmd = (wptr == rptr) ? G3DKMD_TRUE : G3DKMD_FALSE;
					KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					       "wptr 0x%x, rptr 0x%x\n", wptr, rptr);
				} else {
					cmd = G3DKMD_TRUE;
					KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
					       "G3D_IOCTL_HT_READ_WRITE_EQUAL: struct g3dExeInst *pInst is NULL\n");
				}

				if (copy_to_user((void __user *)args, (const void *)&cmd,
						     sizeof(G3DKMD_BOOL)) != 0) {
					KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					       "G3D_IOCTL_CL_GET_STAMP copy_to_user success\n");
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
					"G3D_IOCTL_CL_GET_STAMP copy_to_user success\n");

			} else {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       "G3D_IOCTL_CL_GET_STAMP copy_to_user success\n");
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;

#ifdef SUPPORT_MPD
	case G3D_IOCTL_MPD_CONTROL:
		{
			g3dkmd_IOCTL_MPD_Control cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_MPD_CONTROL");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
					     sizeof(g3dkmd_IOCTL_MPD_Control)) == 0) {
				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       " G3D_IOCTL_MPD_CONTROL, op: %d\n", cmd.op);

				CMMPDControl(&cmd);

				if (copy_to_user((void __user *)args, (const void *)&cmd,
						     sizeof(g3dkmd_IOCTL_MPD_Control)) != 0) {
					ATRACE_FUNC_IOCTL_END();
					return -EFAULT;
				}
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
#endif

#if defined(G3D_PROFILER_RINGBUFFER_STATISTIC)
	case G3D_IOCTL_UPDATE_RINGBUFFER_PROFILING_DATA:
		{
			G3DKMD_IOCTL_6_ARG cmd;

			ATRACE_STR_IOCTL_BEGIN("G3D_IOCTL_UPDATE_RINGBUFFER_PROFILING_DATA");
			if (copy_from_user((void *)&cmd, (const void __user *)args,
					     sizeof(G3DKMD_IOCTL_6_ARG)) == 0) {
				G3Duint pid, usage, pollCnt, askSize, availableSize, stamp;
				g3d_private_data *p_private_data;

				pid = cmd.auiArg[0];
				usage = cmd.auiArg[1];
				pollCnt = cmd.auiArg[2];
				askSize = cmd.auiArg[3];
				availableSize = cmd.auiArg[4];
				stamp = cmd.auiArg[5];
				p_private_data = filp->private_data;

				KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
				       " G3D_IOCTL_UPDATE_RINGBUFFER_PROFILING_DATA, pid: %u, usage: %u, pollCnt: %u, askSize: %u, availableSize: %u, stamp: %u\n",
				       pid, usage, pollCnt, askSize, availableSize, stamp);

				g3dKmdProfilerRingBufferPollingCnt(
					_g3dKmdGetDevicePrivateData(p_private_data->g3dkmd_profiler_private_data,
								    RINGBUFFER_PROFIELR_PRIVATE_DATA_IDX),
								    pid, usage, pollCnt, askSize,
								    availableSize, stamp);
			} else {
				ATRACE_FUNC_IOCTL_END();
				return -EFAULT;
			}
			ATRACE_FUNC_IOCTL_END();
		}
		break;
#endif /* (G3D_PROFILER_RINGBUFFER_STATISTIC) */


	default:
		return -ENOTTY;
	}

	return 0;
}


struct G3d_non_cached_private_data {
	atomic_t nRef; /* count */
	void *nonCachedMemhead;
};

int _g3d_release_pages_non_cached_pages(struct vm_area_struct *vma)
{
	/* cleanup pages: */
	/* do __free_pages( page, 0) for every node in link list. */
	KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL,
		   "[NON_CACHE_MEMORY] vma: %p privat_data:%p  range: %x~%x\n", vma,
		   vma->vm_private_data, vma->vm_start, vma->vm_end);


	if (NULL == vma->vm_private_data) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
		       "vma is not belong to g3d_non_cached_vma.\n");
		return 1;
	}
	{
		struct G3d_non_cached_private_data *pdata =
		    (struct G3d_non_cached_private_data *)vma->vm_private_data;
		atomic_dec(&(pdata->nRef));

		KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL,
			   "[NON_CACHE_MEMORY]vma->vm_private_data->nRef: %d\n",
			   atomic_read(&(pdata->nRef)));

		if (0 >= atomic_read(&(pdata->nRef))) {
			g3d_cleanMemList(((g3dkmd_MemListNode **) &pdata->nonCachedMemhead), 4);
			G3DKMDFREE(pdata);
		}
	}


	return 0;
}


void g3d_non_cached_vma_open(struct vm_area_struct *vma)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "Non Cached Page VMA Open vma:  %p\n",
	       (void *)vma->vm_start);
	KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL,
		   "[NON_CACHE_MEMORY]: vma:%p vma->private_data:%p (_g3d_NC_Page_count: %d)\n",
		   vma, vma->vm_private_data, _g3d_NC_Page_count);

	if (NULL == vma->vm_private_data) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
		       "vma is not belong to g3d_non_cached_vma.\n");
		return;
	}

	{
		struct G3d_non_cached_private_data *pdata =
		    (struct G3d_non_cached_private_data *)vma->vm_private_data;
		atomic_inc(&(pdata->nRef));
	}

	/* g3dkmd_backtrace(); */
}

void g3d_non_cached_vma_close(struct vm_area_struct *vma)
{
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "Non Cached Page VMA Close vma: %p\n",
	       (void *)vma->vm_start);
	KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL,
		   "[NON_CACHE_MEMORY]: vma:%p (_g3d_NC_Page_count: %d), in\n", vma,
		   _g3d_NC_Page_count);

	/* g3dkmd_backtrace(); */
	_g3d_release_pages_non_cached_pages(vma);

	KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL, "[NON_CACHE_MEMORY]: vma:%p (%d), out\n",
		   vma, _g3d_NC_Page_count);

}

struct vm_operations_struct g3d_non_cached_vm_ops = {
	.open = g3d_non_cached_vma_open,
	.close = g3d_non_cached_vma_close,
};

#define remapPage2(vma, pageIdx, page) \
	(vm_insert_page((vma), ((vma)->vm_start + PAGE_SIZE * pageIdx), (page)))


#if defined(__arm64__) || defined(__aarch64__) /* ARM64 */
#define _setVmaNoncached(vma_page_prot) (pgprot_writecombine(vma_page_prot))
#else				/* linux */
#define _setVmaNoncached(vma_page_prot) (pgprot_noncached(vma_page_prot))
#endif

int _g3d_mmap_non_cached_pages(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long addr, size;
	struct page *page = NULL;
	gfp_t gfp_mask;
	struct G3d_non_cached_private_data *pdata;

	/* setup vma */
	vma->vm_ops = &g3d_non_cached_vm_ops;
	vma->vm_page_prot = _setVmaNoncached(vma->vm_page_prot); /* noncached. */

	/* page allocation setup */
	gfp_mask = GFP_HIGHUSER; /* GFP_KERNEL; */
	addr = vma->vm_start;
	size = vma->vm_end - vma->vm_start;
	/* vma->vm_private_data = NULL; */
	pdata = (void *)G3DKMDMALLOC(sizeof(struct G3d_non_cached_private_data));
	atomic_set(&(pdata->nRef), 0);
	pdata->nonCachedMemhead = NULL;
	vma->vm_private_data = pdata;

	KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL,
		   "[NON_CACHE_MEMORY] vma: 0x%p addr: 0x%p size:%lu\n", vma, addr, size);

	while (size > 0) {
		page = _g3dkmd_NC_allocpages(gfp_mask); /* alloc page from Linux */
		if (!page) { /* if page alloc fail */
			/* release all allocated pages if fail */
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
			       "[NON_CACHE_MEMORY] Non Cached Page: alloc_pages fail: gfp_mask: 0x%x  level: 0\n",
			       gfp_mask);
			_g3d_release_pages_non_cached_pages(vma);
			return -EAGAIN;
		}
		{
			ADD_VMA_PAGE_LIST(pdata->nonCachedMemhead, (uintptr_t) page);
		}

		if (vm_insert_page(vma, addr, page)) { /* insert fail error */
			/* release all allocated pages */
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
			       "[NON_CACHE_MEMORY] Non Cached Page: vm_insert_page fail. vma: 0x%p addr: %x page: 0x%p\n",
			       vma, addr, page);
			_g3d_release_pages_non_cached_pages(vma);
			return -EAGAIN;
		}
		addr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL,
		   "[NON_CACHE_MEMORY] Successed. vma: 0x%p addr: %x page: 0x%p\n", vma, addr,
		   page);

	return 0;
}



void g3d_vma_open(struct vm_area_struct *vma)
{
	/*
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
		"g3d VMA open, virt %lx %lx, phys %lx\n", vma->vm_start, vma->vm_end, vma->vm_pgoff << PAGE_SHIFT);
	*/
}

void g3d_vma_close(struct vm_area_struct *vma)
{
	/*
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
		"g3d VMA close, virt %lx %lx, phys %lx\n", vma->vm_start, vma->vm_end, vma->vm_pgoff << PAGE_SHIFT);
	*/
}

struct vm_operations_struct g3d_remap_vm_ops = {
	.open = g3d_vma_open,
	.close = g3d_vma_close,
};

int g3d_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long addr, vaddr, size, addr_org, vaddr_org;
	unsigned long pfn;

	addr = addr_org = vma->vm_start;
	vaddr = vaddr_org = vma->vm_pgoff << PAGE_SHIFT;
	size = vma->vm_end - vma->vm_start;

	if (vaddr == NON_CACHED_PAGE_MMAP_OFFSET) { /* non cached memory alloc */
		int ret = _g3d_mmap_non_cached_pages(filp, vma);

		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL,
		       "map non cache buffer from usr space\n");
		if (ret == -EAGAIN)
			return ret;

		g3d_non_cached_vma_open(vma);
		return ret; /* successful */
	} else if (vaddr >= VMALLOC_START) {
		vma->vm_flags |= VM_SHARED;

		while (size > 0) {
			int ret;

			pfn = vmalloc_to_pfn((char *)vaddr);
			ret = remap_pfn_range(vma, addr, pfn, PAGE_SIZE, vma->vm_page_prot);
			if (ret < 0) {
				/* handle error */
				return -EAGAIN;
			}
			addr += PAGE_SIZE;
			vaddr += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
	} else {
#if defined(FPGA_G3D_HW) && defined(G3D_SUPPORT_SWRDBACK)
		if ((vma->vm_pgoff << PAGE_SHIFT) == gExecInfo.SWRdbackMem.phyAddr) { /* 1 page now */
			vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
			KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "map g3dcmdQ address\n");
		}
#ifdef G3DKMD_HACK_FRAME_CMD_DMA
		if ((vma->vm_pgoff << PAGE_SHIFT) == gExecInfo.DMAFrameCmdMem.phyAddr) { /* 1 page now */
			vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
			/* pr_debug("[G3DKMD_HACK_FRAME_CMD_DMA] You should appear only ONCE!\n"); */
			/* only show up once per process */
			KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "map frameCmdBuf address\n");
		}
#endif /* G3DKMD_HACK_FRAME_CMD_DMA */

#endif /* FPGA_G3D_HW && G3D_SUPPORT_SWRDBACK */

#if defined(G3DKMD_USE_BUFFERFILE)
		{
			if (_g3dKmdIsBufferFileMmap(filp, vma->vm_pgoff << PAGE_SHIFT)) {
				if (-1 == _g3dkmdBufferFileMmap(filp, vma))
					return -EAGAIN;

				return 0;
			}
		}
#endif /* G3DKMD_USE_BUFFERFILE */

		if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
				    vma->vm_end - vma->vm_start, vma->vm_page_prot))
			return -EAGAIN;

	}
	vma->vm_ops = &g3d_remap_vm_ops;
	g3d_vma_open(vma);
	return 0;
}

const struct file_operations g3d_fops = {
	.owner = THIS_MODULE,
	.open = g3d_open,
	.release = g3d_release,
	.mmap = g3d_mmap,
	/* .ioctl   = g3d_ioctl */
	.unlocked_ioctl = g3d_ioctl,
	.compat_ioctl = g3d_ioctl
};

#ifdef FPGA_G3D_HW
/* major to set __yl_base, __yl_irq & __iommu_base */
static void get_device_node(void)
{
	/* 0. declare */
#ifdef CONFIG_OF
	struct device_node *node = NULL;
#endif

#if ((USE_MMU == MMU_USING_GPU) || (USE_MMU == MMU_USING_M4U) || (USE_MMU == MMU_USING_WCP))
	/* workaround for SSP to access iommu registers in MMU_USING_WCP */
	struct device_node *iommu_node = NULL;
#endif

	/* 1. set init value */
	__yl_base = 0xF3000000;	/* borrow the 6592 address */
#ifdef G3DKMD_SUPPORT_ISR
#if defined(FPGA_mt6752_fpga) || defined(FPGA_fpga8163) || defined(FPGA_fpga8163_64) || defined(MT8163)
	__yl_irq = 225 + 32;	/* 257 */
#elif defined(FPGA_mt6592_fpga)
	__yl_irq = 234;
#else
#error "currently mt6752/mt6592/mt8163 FPGA are supported only"
#endif
#endif
#if ((USE_MMU == MMU_USING_GPU) || (USE_MMU == MMU_USING_M4U) || (USE_MMU == MMU_USING_WCP))
	/* workaround for SSP to access iommu registers in MMU_USING_WCP */
	__iommu_base = 0xF3005000;
#endif

#ifdef CONFIG_OF
	/* 2. if device tree on, get g3d node */
	node = of_find_compatible_node(NULL, NULL, g3d_dts_name);

	if (node) {
		/* iomap registers */
		__yl_base = (unsigned long)of_iomap(node, 0);
		KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_IOCTL,
		       "of_find_compatible_node \"%s\" to get reg address = 0x%lx\n", g3d_dts_name,
		       __yl_base);
#ifdef G3DKMD_SUPPORT_ISR
		/* get IRQ ID */
		__yl_irq = irq_of_parse_and_map(node, 0);
		KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_IOCTL,
		       "of_find_compatible_node \"%s\" to get irq num = %ld\n", g3d_dts_name,
		       __yl_irq);
#endif
	} else {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
		       "of_find_compatible_node \"%s\" fail\n", g3d_dts_name);
	}

#if ((USE_MMU == MMU_USING_GPU) || (USE_MMU == MMU_USING_M4U) || (USE_MMU == MMU_USING_WCP))
	/* workaround for SSP to access iommu registers in MMU_USING_WCP */

	/* 3. if device tree on, get iommu node */
	iommu_node = of_find_compatible_node(NULL, NULL, g3d_iommu_dts_name);

	if (iommu_node) {
		/* iomap registers */
		__iommu_base = (unsigned long)of_iomap(iommu_node, 0);
		KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_IOCTL,
		       "of_find_compatible_node \"%s\" to get reg address = 0x%lx\n",
		       g3d_iommu_dts_name, __iommu_base);
	} else {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
		       "of_find_compatible_node \"%s\" fail\n", g3d_iommu_dts_name);
	}
#endif
#endif /* CONFIG_OF */
}
#endif /* FPGA_G3D_HW */



#ifdef USING_MISC_DEVICE
int yolig3d_probe(struct platform_device *pdev)
#else
static int __init init_modules(void)
#endif
{
#ifdef FPGA_G3D_HW
	get_device_node();
	if (!g3dCheckDeviceId())
		return -EINVAL;
#endif

#ifdef USING_MISC_DEVICE
	{
		int err;

		_g3dStorePlatformDevice(pdev);	/* save pdev to g3d_device global variable for future use. */
		/* ex: g3d's dma memory allocation. */

		g3d_miscdevice.minor = MISC_DYNAMIC_MINOR;
		g3d_miscdevice.name = g3d_dev_name;
		g3d_miscdevice.fops = &g3d_fops;
		g3d_miscdevice.parent = NULL;

		err = misc_register(&g3d_miscdevice);
		if (0 != err) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
			       "Failed to register misc device, misc_register() returned %d\n", err);
		}
	}
#else
	dev_t dev;
	int ret;

	ret = alloc_chrdev_region(&dev, 0, 1, G3D_NAME);
	if (unlikely(ret < 0)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL, "can't alloc chrdev\n");
		return ret;
	}

	g3d_major = MAJOR(dev);
	g3d_minor = MINOR(dev);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "register chrdev(%d,%d)\n", g3d_major, g3d_minor);

	g3d_cdevp = G3DKMDMALLOC(sizeof(struct cdev));
	if (unlikely(g3d_cdevp == NULL)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL, "kmalloc failed\n");
		goto failed;
	}
	cdev_init(g3d_cdevp, &g3d_fops);
	g3d_cdevp->owner = THIS_MODULE;
	ret = cdev_add(g3d_cdevp, MKDEV(g3d_major, g3d_minor), 1);
	if (unlikely(ret < 0)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL, "add chr dev failed\n");
		goto failed;
	}
#endif

	g3d_memlist_cachep = kmem_cache_create("yolig3d_memlist_cache",
					       sizeof(g3dkmd_MemListNode), 0, 0, NULL);

	if (unlikely(!g3d_memlist_cachep)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_IOCTL,
		       "kmem_cache_create yolig3d memlist failed\n");
		goto failed;
	}

	/* increase uses count if need */
	g3dKmdInitialize();

	return 0;

failed:
#ifdef USING_MISC_DEVICE
	misc_deregister(&g3d_miscdevice);
#else
	if (g3d_cdevp) {
		G3DKMDFREE(g3d_cdevp);
		g3d_cdevp = NULL;
	}
#endif
	return 0;
}

#ifdef USING_MISC_DEVICE
int yolig3d_remove(struct platform_device *pdev)
#else
static void __exit exit_modules(void)
#endif
{
#ifdef USING_MISC_DEVICE
	misc_deregister(&g3d_miscdevice);
#else
	dev_t dev;

	dev = MKDEV(g3d_major, g3d_minor);

	if (likely(g3d_cdevp)) {
		cdev_del(g3d_cdevp);
		G3DKMDFREE(g3d_cdevp);
	}

	unregister_chrdev_region(dev, 1);
#endif

	/* decrease uses count if need */
	g3dKmdTerminate();

	/* check if Non-cacheable memory count. */
	YL_KMD_ASSERT(_g3d_NC_Page_count == 0);	/* not 0: Mem leak, non-cached page */

#ifdef G3DKMD_CALLBACK_FROM_KEYBOARD_INT
	free_irq(1, NULL);
#endif

	if (unlikely(!g3d_memlist_cachep)) {
		kmem_cache_destroy(g3d_memlist_cachep);
		g3d_memlist_cachep = NULL;
	}

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_IOCTL, "unregister chrdev\n");

#ifdef USING_MISC_DEVICE
	return 0;
#endif
}

#ifdef USING_MISC_DEVICE
static int yolig3d_suspend(struct platform_device *pdev, pm_message_t state)
{
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL,
		"[%s]yolig3d_suspend %d\n", pdev->name, state.event); /* 2 = PM_EVENT_SUSPEND */
	gpu_suspend();
	return 0;
}

static int yolig3d_resume(struct platform_device *pdev)
{
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_IOCTL, "[%s]yolig3d_resume\n", pdev->name);
	gpu_resume();
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id apyolig3d_of_ids[] = {
	{.compatible = g3d_dts_name,},
	{}
};
#endif

static struct platform_driver yolig3d_driver = {
	.probe = yolig3d_probe,
	.remove = yolig3d_remove,
	.suspend = yolig3d_suspend,
	.resume = yolig3d_resume,
	.driver = {
		   .name = G3D_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = apyolig3d_of_ids,
#endif
		   }
};

static void yolig3d_device_release(struct device *dev)
{

}

static struct platform_device yolig3d_device = {
	.name = G3D_NAME,
	.id = 0,
	.dev = {
		/* avoid WARN_ON message in device_release() when platform_device_unregister is called by rmmod .. */
		.release = yolig3d_device_release,
		}
};

static int __init init_modules(void)
{
	gpu_register_early_suspend();

#ifndef CONFIG_OF
	if (platform_device_register(&yolig3d_device)) {
		gpu_unregister_early_suspend();
		return -ENODEV;
	}
#endif
	if (platform_driver_register(&yolig3d_driver)) {
		gpu_unregister_early_suspend();
		platform_device_unregister(&yolig3d_device);
		return -ENODEV;
	}
	return 0;
}

static void __exit exit_modules(void)
{
	gpu_unregister_early_suspend();
	platform_driver_unregister(&yolig3d_driver);
#ifndef CONFIG_OF
	platform_device_unregister(&yolig3d_device);
#endif
}
#endif

#ifdef G3DKMD_BUILTIN_KERNEL
#include "../g3dbase/yl_version.c"
device_initcall(init_modules);
#else
module_init(init_modules);
#endif
module_exit(exit_modules);

MODULE_SUPPORTED_DEVICE(G3D_NAME);
