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

#ifndef _G3DKMD_UTIL_H_
#define _G3DKMD_UTIL_H_

#include "g3dkmd_define.h"

/* basic include */
#if defined(_WIN32)
#include <windows.h>
#elif defined(linux) && !defined(linux_user_mode)
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#endif

#if (defined(_WIN32) || (defined(linux) && defined(linux_user_mode)))
#define G3DKMD_BARRIER()
#elif defined(linux)
#define G3DKMD_BARRIER()     mb()  /*To wait memory data sync*/
#endif

/* for alignment, align must be power of 2 */
#define G3DKMD_ALIGN(base, align)   (((base) + (align) - 1) & (~((align) - 1)))
#define G3DKMD_FLOOR(base, align)    ((base) & (~((align) - 1)))
#define G3DKMD_CEIL(base, align)   G3DKMD_ALIGN(base, align)

/* for Sleep */
#if defined(_WIN32)
#define G3DKMD_SLEEP(ms)    Sleep(ms)

#elif defined(linux)
#if defined(linux_user_mode)
#include <unistd.h>
#define G3DKMD_SLEEP(ms)    usleep(ms * 1000)

#else
#include <linux/delay.h>
#define G3DKMD_SLEEP(ms)    do { if (!in_interrupt()) msleep(ms); } while (0)
#endif
#endif

/* for Delay    // busy waiting */
#if defined(_WIN32)
#define G3DKMD_NDELAY(ns)
#define G3DKMD_UDELAY(us)
#define G3DKMD_MDELAY(ms)

#elif defined(linux)
#if defined(linux_user_mode)
#include <unistd.h>

#define G3DKMD_NDELAY(ns)
#define G3DKMD_UDELAY(us)
#define G3DKMD_MDELAY(ms)

#else
#include <linux/delay.h>

#define G3DKMD_NDELAY(ns)   ndelay(ns)  /* 1/(10^10) */
#define G3DKMD_UDELAY(us)   udelay(us)  /* 1/(10^6) */
#define G3DKMD_MDELAY(ms)   mdelay(ms)  /* 1/(10^3) */
#endif
#endif

/* for semaphore Lock */
#if (defined(_WIN32) || (defined(linux) && defined(linux_user_mode)))
#define __DEFINE_SEM_LOCK(lock) (int lock)
/* Simple implementation.... but not strictly correct */
#define __SEM_LOCK(lock)                do { while (lock); lock = 1; } while (0)
#define __SEM_UNLOCK(lock)              (lock = 0)

#elif defined(linux)
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#ifdef DEFINE_SEMAPHORE /* 3.0.13 */
#define __DEFINE_SEM_LOCK(lock) DEFINE_SEMAPHORE(lock)
#else /* 2.6.29 */
#define __DEFINE_SEM_LOCK(lock) DECLARE_MUTEX(lock)
#endif
#define __SEM_LOCK(lock)                down(&lock)
#define __SEM_UNLOCK(lock)              up(&lock)
#endif

/* for spin lock */
#if (defined(_WIN32) || (defined(linux) && defined(linux_user_mode)))
#define __DEFINE_SPIN_LOCK(lock)        __DEFINE_SEM_LOCK(lock)
#define __SPIN_LOCK(lock)               __SEM_LOCK(lock)
#define __SPIN_UNLOCK(lock)             __SEM_UNLOCK(lock)
#define __SPIN_LOCK2_DECLARE(flag)
#define __SPIN_LOCK2(lockname, flag)    __SEM_LOCK(lock)
#define __SPIN_UNLOCK2(lockname, flag)  __SEM_UNLOCK(lock)
#elif defined(linux)
#include <linux/spinlock.h>

#define __DEFINE_SPIN_LOCK(lock)        DEFINE_SPINLOCK(lock)
#define __SPIN_LOCK(lock)               spin_lock(&lock)
#define __SPIN_UNLOCK(lock)             spin_unlock(&lock)
#define __SPIN_LOCK_BH(lock)            spin_lock_bh(&lock)
#define __SPIN_UNLOCK_BH(lock)          spin_unlock_bh(&lock)
#define __SPIN_LOCK2_DECLARE(flag)      unsigned long flag
#define __SPIN_LOCK2(lockname, flag)    spin_lock_irqsave(&lockname, flag)
#define __SPIN_UNLOCK2(lockname, flag)  spin_unlock_irqrestore(&lockname, flag)
#endif

/* for inline key word */
#if defined(_WIN32)
#define INLINE inline
#else
#define INLINE inline
#endif

/* for memory alloc */
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))
#define G3DKMDMALLOC(_SIZE)             malloc(_SIZE)
#define G3DKMDMALLOC_ATOMIC(_SIZE)      malloc(_SIZE)
#define G3DKMDMALLOCFLAGS(_SIZE, flags) malloc(_SIZE)
#define G3DKMDFREE(_PTR)        free(_PTR)
#define G3DKMDVMALLOC(_SIZE)    malloc(_SIZE)
#define G3DKMDVFREE(_PTR)       free(_PTR)
#elif defined(G3DKMD_MEM_CHECK)
#define G3DKMDMALLOC(_SIZE)             g3dkmdmalloc(_SIZE, GFP_KERNEL, __FILE__, __func__, __LINE__)
#define G3DKMDMALLOC_ATOMIC(_SIZE)      g3dkmdmalloc(_SIZE, GFP_ATOMIC, __FILE__, __func__, __LINE__)
#define G3DKMDMALLOCFLAGS(_SIZE, flags) g3dkmdmalloc(_SIZE, flags, __FILE__, __func__, __LINE__)
#define G3DKMDFREE(_PTR)        g3dkmdfree(_PTR, __FILE__, __func__, __LINE__)
#define G3DKMDVMALLOC(_SIZE)    g3dkmdvmalloc(_SIZE, __FILE__, __func__, __LINE__)
#define G3DKMDVFREE(_PTR)       g3dkmdvfree(_PTR, __FILE__, __func__, __LINE__)
#else
#define G3DKMDMALLOC(_SIZE)             kmalloc(_SIZE, GFP_KERNEL)
#define G3DKMDMALLOC_ATOMIC(_SIZE)      kmalloc(_SIZE, GFP_ATOMIC)
#define G3DKMDMALLOCFLAGS(_SIZE, flags) kmalloc(_SIZE, flags)
#define G3DKMDFREE(_PTR)        kfree(_PTR)
#define G3DKMDVMALLOC(_SIZE)    vmalloc(_SIZE)
#define G3DKMDVFREE(_PTR)       vfree(_PTR)
#endif

/* for ASSERTION */
#ifdef G3DKMDDEBUG
#if defined(_WIN32)
#include <assert.h>
#define YL_KMD_ASSERT(expr) \
do { \
	if (!(expr)) \
		__debugbreak(); \
} while (0)
#elif defined(linux) && !defined(__KERNEL__)
#include <stdio.h>
#define YL_KMD_ASSERT(expr) \
do { \
	if (!(expr)) { \
		printf("G3DKMD KMD ASSERTION FAILED (%s) at: %s:%d:%s()\n", \
			#expr, __FILE__, __LINE__, __func__); \
	} \
} while (0)
#else
#include <linux/kernel.h>
#define YL_KMD_ASSERT(expr) BUG_ON(!(expr))
#if 0
#define YL_KMD_ASSERT(expr) \
do { \
	if (unlikely(!(expr))) { \
		pr_error(KBUILD_MODNAME ": "\
			"G3DKMD KMD ASSERTION FAILED (%s) at: %s:%d:%s()\n",\
			#expr, __FILE__, __LINE__, __func__);\
	} \
} while (0)
#endif
#endif
#else
#if defined(_WIN32)
#include <assert.h>
#define YL_KMD_ASSERT(x) assert(x)
#else
#define YL_KMD_ASSERT(x)
#endif
#endif

/* for TRUE/FALSE define */
#ifdef G3DKMD_TRUE
#undef G3DKMD_TRUE
#endif
#define G3DKMD_TRUE     1

#ifdef G3DKMD_FALSE
#undef G3DKMD_FALSE
#endif
#define G3DKMD_FALSE    0

typedef unsigned char G3DKMD_BOOL;
void g3dkmd_backtrace(void);

#endif /* _G3DKMD_UTIL_H_ */
