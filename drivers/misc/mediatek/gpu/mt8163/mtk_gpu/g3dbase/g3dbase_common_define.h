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

/* Shared between ALL projects : GL / CL / G3D / G3DKMD */

#ifndef _G3DBASE_COMMON_DEFINE_H_
#define _G3DBASE_COMMON_DEFINE_H_

#undef G3D64
#if defined(_WIN32)
#ifdef _WIN64
#define __G3D_64__
#endif
#else
#if defined(__x86_64__) || defined(__aarch64__)
#define __G3D_64__
#endif
#endif

#if (!defined(linux) || !defined(__KERNEL__)) && !defined(assert)
#include <assert.h>
#endif

#if defined(ANDROID) && !defined(__KERNEL__)
#include <cutils/log.h>
#if (ANDROID_VERSION < 0x0600)
#include <cutils/trace.h> /* include it by yourself in android m */
#endif
#define GRALLOC_PERFORM_QUERY_G3DMEM 0xff
#define GRALLOC_PERFORM_QUERY_YUV_PRIVATE_FORMAT 0xfe
#define GRALLOC_PERFORM_SET_GPU_TYPE 0xfd
#define ATRACE_FUNC_G3D_BEGIN() atrace_begin(ATRACE_TAG_GRAPHICS, __func__)
#define ATRACE_FUNC_G3D_END() atrace_end(ATRACE_TAG_GRAPHICS)
#define ATRACE_STR_G3D_BEGIN(str) atrace_begin(ATRACE_TAG_GRAPHICS, str)
#define ATRACE_STR_G3D_INT(str, digit) atrace_int(ATRACE_TAG_GRAPHICS, str, digit)
#if (ANDROID_VERSION < 0x0410)
#define ALOGE LOGE
#define ALOGD LOGD
#define ALOGW LOGW
#define ALOGI LOGI
#endif
#else
#define ATRACE_FUNC_G3D_BEGIN()
#define ATRACE_FUNC_G3D_END()
#define ATRACE_STR_G3D_BEGIN(str)
#define ATRACE_STR_G3D_INT(str, digit)
#endif

#if defined(ANDROID) && defined(__KERNEL__) && defined(FPGA_G3D_HW) && !defined(SEMU64)
/* #define ANDROID_IOCTL_PROFILE */
/* #define ANDROID_HT_COUNT */
#ifdef ANDROID_HT_COUNT
#include <linux/mtk_ftrace.h>
#endif
#define ATRACE_FUNC_G3DKMD_BEGIN()      mt_kernel_trace_begin(__func__)
#define ATRACE_STR_G3DKMD_BEGIN(str)    mt_kernel_trace_begin(str)
#define ATRACE_FUNC_G3DKMD_END()        mt_kernel_trace_end()
#ifdef ANDROID_IOCTL_PROFILE
#define ATRACE_FUNC_IOCTL_BEGIN()       mt_kernel_trace_begin(__func__)
#define ATRACE_STR_IOCTL_BEGIN(str)     mt_kernel_trace_begin(str)
#define ATRACE_FUNC_IOCTL_END()         mt_kernel_trace_end()
#else /* ! ANDROID_IOCTL_PROFILE */
#define ATRACE_FUNC_IOCTL_BEGIN()
#define ATRACE_STR_IOCTL_BEGIN(str)
#define ATRACE_FUNC_IOCTL_END()
#endif /* ANDROID_IOCTL_PROFILE */

#else /* ! defined(ANDROID) && defined(__KERNEL__) && defined(FPGA_G3D_HW) */
#define ATRACE_FUNC_G3DKMD_BEGIN()
#define ATRACE_STR_G3DKMD_BEGIN(str)
#define ATRACE_FUNC_G3DKMD_END()
#define ATRACE_FUNC_IOCTL_BEGIN()
#define ATRACE_STR_IOCTL_BEGIN(str)
#define ATRACE_FUNC_IOCTL_END()
#endif /* defined(ANDROID) && defined(__KERNEL__) && defined(FPGA_G3D_HW) */

#ifndef INLINE
#if defined(_WIN32)
#define DASHDASH(x) __ ## x
#define INLINE  DASHDASH(inline)
#else
#define INLINE inline
#endif
#endif

#ifdef ANDROID
#define ANDROID_PREF_NDEBUG
/* #define ANDROID_PREF_BYPASS */
#if !defined(YL_FAKE_X11) /* exclusive from LDVT */
    /* #define TIMEOUT_FENCE */
    /* #define ANDROID_UNIQUE_QUEUE */
#ifdef SEMU64
/* #define YL_MET */
#endif
#endif
#endif


/* G3DMemory Link List */
/* [Memory link list]
 *
 * [Louis 2015/03/27]:
 * - Use a global link list to link G3DMemory, so that we can get G3DMemory
 *   Information by triversing the link list.
 *
 * - Usage:
 *   . G3D Memory Footprint.
 *
 * - Overhead:
 *   . [Low] Alloc: a list add operation(g3dkmd_list_add). No search operation
 *   . [Low] Free : a list del operation(g3dkmd_list_del). No search operation
 *   . [High] Triverse: may need a lock to protect it.
 */
#define ENABLE_G3DMEMORY_LINK_LIST


/* heap management */
#define ENABLE_HEAP_MM

/* g3d resource profiler */
/* #define ENABLE_RESOURCE_PROFILE */
/* #define ENABLE_WCP_MMU_GET_USR_BUF_USAGE */  /* if you need m4u information.
						 * this will call our own m4u function to
						 * get m4u info.
						 */

#if defined(ENABLE_HEAP_MM) && defined(YL_REDUCE_PATTERN_SIZE)
	#error HEAP_MM does not support YL_REDUCE_PATTERN_SIZE yet. Please remove one of them.
#endif /* defined(ENABLE_HEAP_MM) && defined(YL_REDUCE_PATTERN_SIZE) */

/* g3d clcok_gettime for preformance measurement. */
#if defined(linux)
	#define ENABLE_CLOCK_GETTIME
#endif

#if !defined(linux) && defined(ENABLE_CLOCK_GETTIME)
	#error clock_gettime only supoort on linux now.
#endif


/* define register size for kernel driver reference */
#define SIZE_OF_FREG	0x3F8	/* sizeof(SapphireFreg) */
#define SIZE_OF_VPREG	0x108	/* sizeof(SapphireVp) */

/* non-cached large memory allocation */

#if defined(FPGA_G3D_HW) && defined(ANDROID)
#define ANDROID_PERF_EXP
#endif

#if defined(FPGA_G3D_HW) || defined(ANDROID)
#ifndef QEMU64
#define G3D_NONCACHE_BUFFERS
#endif
/* #define YL_ENABLE_HW_NOFIRE */
/* #define YL_NOFIRE_FULL */
#endif
#ifdef G3D_NONCACHE_BUFFERS
#define G3D_NONCACHE_BUFFERS_WITH_GRALLOC_BUF_CACHED
#endif

/* Define for HW driver overhead testing*/
/* #define G3D_SUPPORT_HW_NOFIRE */

/* Defnies for test hangup-recovery flow */
/* #define G3D_HANG_TEST_NULL_QLOAD */
/* #define G3D_HANG_TEST_ERR_ECC */
/* #define G3D_HANG_TEST_ERR_FREG */
/* */

#if defined(linux) && !defined(linux_user_mode) /* linux kernel mode */
/* the offset must page alligned. */
#define NON_CACHED_PAGE_MMAP_OFFSET 0x0	/* 0xAAFFF000 */
#endif


/* bufferfile */

#if defined(linux) && !defined(linux_user_mode) /* linux kernel mode */

/* For using buffer file to replace file I/O in kernel. */
#define G3D_ENABLE_BUFFERFILE
/* #define G3D_ENABLE_TEST_BUFFERFILE */

#define MAX_BUFFERFILE_NAME 64

/* enable bufferfile */
#if defined(G3D_ENABLE_BUFFERFILE)
#define G3D_USE_BUFFERFILE
#define G3DKMD_USE_BUFFERFILE
#endif

/* enable bufferfile test. */
#if defined(G3D_ENABLE_TEST_BUFFERFILE)
#define G3D_TEST_BUFFERFILE
#define G3DKMD_TEST_BUFFERFILE
#endif

#endif /* linux kernel mode */

#define G3D_MAX_NUM_STAMPS 4

/* 1: Use mirror table ( G3dVa -> G3dVa ) for MMU patterns */
/* 0: Use G3dVa -> Pattern Pa for MMU patterns */
#define MMU_MIRROR_ROUTE 0

/* 1: Use va ( instead of pa ) in non-pattern querying */
/* 0: Use pa ( pa == va in C-Model but pa != va in Linux ) in non-pattern querying */
#define MMU_VIRTUAL_ROUTE 1

/* 1: Let mmu detects the continuous 1 mb, and [PATTERN] let pa aligns to va. */
#define MMU_SECTION_DETECTION 0

/* 1: Inject extra settings which not included in release code. */
#if defined(_WIN32)

#define DEFER_MMU_BUFFER 1

#else

#define DEFER_MMU_BUFFER 0

#endif

/* 1: Build with 4MB 1st level pages. 0: with 1MB 1st level pages. */
#define MMU_FOUR_MB     0

#if (defined(_WIN32) || (defined(linux) && !defined(linux_user_mode))) && !defined(REL_GLC)
/* #define SUPPORT_MPD */
#endif

#ifdef SUPPORT_MPD
#undef MMU_VIRTUAL_ROUTE
#define MMU_VIRTUAL_ROUTE 0
#endif

#define YL_MT_FLUSH_CURRENT_ONLY

#if defined(linux) && !defined(linux_user_mode)
#define G3D_SUPPORT_SWRDBACK /* its related code shall be in g3d/g3dkmd modules */
#define G3D_SWRDBACK_SLOT_SIZE  16

#ifdef G3D_SUPPORT_SWRDBACK
#define G3D_SWRDBACK_RECOVERY
#endif

#define KERNEL_FENCE_DEFER_WAIT

#ifdef YL_MT_FLUSH_CURRENT_ONLY
#define G3D_SUPPORT_FLUSHTRIGGER /* its related code shall be in g3d/g3dkmd modules */
#endif
#ifdef ANDROID
#if (defined(__ARM64__) || defined(CONFIG_ARM64)) && !defined(FPGA_G3D_HW)
#define PHY_ADDR_FROM_VIRT_ADDR
#endif
#else
#ifdef YL_64BITS
#define PHY_ADDR_FROM_VIRT_ADDR
#endif
#endif /* ANDROID */
#endif /* defined(linux) && !defined(linux_user_mode) */

#define G3DKMD_MMU_ENABLE                   (0x1ul << 0)
#define G3DKMD_MMU_SECURE                   (0x1ul << 1)
#define G3DKMD_MMU_SHARE                    (0x1ul << 2)
#define G3DKMD_MMU_DMA                      (0x1ul << 4)

#define G3DKMD_ACTION_HW_RESET               1
#define G3DKMD_ACTION_CACHE_FLUSH_INVALIDATE 2
#define G3DKMD_ACTION_CHECK_HW_IDLE          3
#define G3DKMD_ACTION_QUERY_HW_RESET_COUNT   4
#define G3DKMD_ACTION_QUERY_MMU_TRANSFAULT   5

#define G3D_CACHE_CLEAN_BY_RANGE             1
#define G3D_CACHE_INVALID_BY_RANGE           2
#define G3D_CACHE_FLUSH_BY_RANGE             3

/* CmdQ size will be used in g3dkmd, in order to calculate suitable FDQ buffer size */
#define COMMAND_QUEUE_SIZE          (32*1024)

#ifdef FPGA_G3D_HW
#define COMMAND_QUEUE_SIZE_FOR_DUMP (3*1024*1024)
#else /* ! FPGA_G3D_HW */
#define COMMAND_QUEUE_SIZE_FOR_DUMP (6*1024*1024)
#endif /* FPGA_G3D_HW */

/* #define YL_SECURE */

/* #define G3D_CONTROL_PATH //for enable g3dshell */

/* enable linux signal flow for driver interrupt callback */
#ifdef linux
/* #define G3DKMD_CALLBACK_INT */
#ifdef G3DKMD_CALLBACK_INT
/* #define G3DKMD_CALLBACK_FROM_KEYBOARD_INT */
/* #define G3DKMD_CALLBACK_FROM_TIMER */
#endif /* G3DKMD_CALLBACK_INT */
#endif /* linux */

/* for mmu pattern dump:
   use either YL_MMU_PATTERN_DUMP_V1 or YL_MMU_PATTERN_DUMP_V2.
   open them in the same time will be a redundant, but works.  */
/* #define YL_MMU_PATTERN_DUMP_V1 */
#define YL_MMU_PATTERN_DUMP_V2

/* for SSP output path name max length*/
#define SSP_PATHNAMEMAX 128

/**
 * __builtin_expect macros
 */
#if !defined(__GNUC__)
	#define __builtin_expect(x, y) (x)
#endif

#ifndef likely
#ifdef __GNUC__
	#define likely(x)   __builtin_expect(!!(x), 1)
	#define unlikely(x) __builtin_expect(!!(x), 0)
#else
	#define likely(x)   (x)
	#define unlikely(x) (x)
#endif
#endif

/**
 *  ASSERT macro
 */

#ifndef ASSERT



#if !defined(_WIN32_WCE)

#if defined(BUILD_FOR_SNAP) && defined(CHECKED)

#define ASSERT(X)   _CHECK(X)

#elif defined(_DEBUG)

#ifndef linux

/* Win32 Debug */

#ifdef ASSERTION_FILE
#include <stdio.h> /* for file io in assert */
#include <assert.h> /* for assert() */

#define ASSERT(X) \
do { \
	if (!(X)) { \
		FILE *fp = fopen("DEBUG_assert.txt", "w+"); \
		fprintf(fp, "[CModel] ASSERT:%s(%d)\nExp:%s\n", __FILE__, __LINE__, #X); \
		fclose(fp); \
		assert((X)); \
	} \
} while (0)

#define ASSERT2(_Expression, _Msg) \
do { \
	if (!(_Expression)) { \
		FILE *fp = fopen("DEBUG_assert.txt", "w+"); \
		fprintf(fp, "[CModel] ASSERT2:%s(%d)\nExp:%s\nMsg:%s\n", __FILE__, __LINE__, #_Expression, _Msg); \
		fclose(fp); \
		_wassert(L##_Msg, _CRT_WIDE(__FILE__), __LINE__); \
	} \
} while (0)

#else
#include <assert.h> /* for assert() */

#define ASSERT(X) assert(X)

#define ASSERT2(_Expression, _Msg) ((void)((!!(_Expression)) || (_wassert(L##_Msg, _CRT_WIDE(__FILE__), __LINE__), 0)))

#endif

#elif defined(ANDROID)
#ifndef NDEBUG
#define ASSERT(X)   do { if (!likely(X)) { ALOGE("assert in %s(%d)", __FILE__, __LINE__); assert(X); } } while (0)
#else
#define ASSERT(X)   assert(X)
#endif
#define ASSERT2(_Expression, _Msg) \
do { \
	if (!(_Expression)) \
		ALOGE("assert msg = %s\n", _Msg); \
	assert(_Expression); \
} while (0)
#else /* linux */
#define ASSERT(X)  assert(X)
#define ASSERT2(_Expression, _Msg) \
do { \
	if (!(_Expression)) \
		printf("assert msg = %s\n", _Msg); \
	assert(_Expression); \
} while (0)
#define ASSERT_MSG(_Expression, fmt, ...) \
do { \
	if (!(_Expression)) { \
		printf("ASSERT: "); \
		printf(fmt, __VA_ARGS__); \
		assert(_Expression); \
	} \
} while (0)
#endif
#else

#ifndef linux

/* Win32 Release */

#ifdef ASSERTION_FILE
#include <stdio.h> /* file io */
#include <stdlib.h> /* exit() */


#define ASSERT(X) \
do { \
	if (!(X)) { \
		FILE *fp = fopen("DEBUG_assert.txt", "w+"); \
		fprintf(fp, "[CModel] ASSERT:%s(%d)\nExp:%s\n", __FILE__, __LINE__, #X); \
		fclose(fp); \
		exit(1); \
	} \
} while (0)


#define ASSERT2(_Expression, _Msg) \
do { \
	if (!(_Expression)) { \
		FILE *fp = fopen("DEBUG_assert.txt", "w+"); \
		fprintf(fp, "[CModel] ASSERT2:%s(%d)\nExp:%s\nMsg:%s\n", __FILE__, __LINE__, #_Expression, _Msg); \
		fclose(fp); \
		exit(1); \
	} \
} while (0)

#else

#include <assert.h> /* assert() */

#define ASSERT(X) assert(X)

#define ASSERT2(_Expression, _Msg) (void)((!!(_Expression)) || (_wassert(L##_Msg, _CRT_WIDE(__FILE__), __LINE__), 0))

#endif

#else

/* linux Release */
#ifndef __KERNEL__
#define ASSERT(X)
#define ASSERT2(_Expression, _Msg)
#endif
#endif

#endif
#endif
#ifdef ANDROID
#define ASSERT2_EX(_Expression, _Msg) \
do { \
	if (!(_Expression)) { \
		ALOGE("assert msg = %s\n", _Msg); \
		if (g3dGbl.config.assertMode == 2) \
			assert(_Expression); \
	} \
} while (0)
#else
#define ASSERT2_EX(_Expression, _Msg) \
do { \
	if (!(_Expression)) { \
		printf("assert msg = %s\n", _Msg); \
		if (g3dGbl.config.assertMode == 2) \
			assert(_Expression); \
	} \
} while (0)
#endif



#endif /* ASSERT */

#endif /* _G3DBASE_COMMON_DEFINE_H_ */
