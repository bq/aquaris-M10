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

/* file */
/* brief Public. Define all the things used all over the MMU Remapper module. */

#ifndef _YL_MMU_COMMON_H_
#define _YL_MMU_COMMON_H_

#include "g3dbase_common_define.h"


#define MMU_MIRROR      1
#define MMU_LOOKBACK_VA 1

/* Platforms */
#ifdef linux
#if !defined(linux_user_mode)	/* linux kernel space */
#include <linux/kernel.h>	/* for UINT_MAX */
#endif
#endif


#ifndef __GNUC__
#ifndef __attribute__
#define  __attribute__(x) /*NOTHING*/
#endif
#endif
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif
#ifndef YL_BOOL
#define YL_BOOL char
#endif

#if defined(_WIN32)
#include <stdint.h>
#endif

#ifdef linux
#if defined(linux_user_mode)	/* linux user space */
#include <stdint.h>
#include <sys/types.h>
#else
#include <linux/types.h>
#endif
#endif


#define MMU_GRANULARITY     4096UL
#define MMU_GRANULARITY_BIT 12UL

#if MMU_FOUR_MB

#define MMU_SECTION_SIZE    (4*1048576UL)
#define SECTION_BIT         22UL

#else

#define MMU_SECTION_SIZE    (1048576UL)
#define SECTION_BIT         20UL

#endif

#define MMU_LARGE_SIZE      65536UL
#define LARGE_BIT           16UL

#define MMU_SMALL_SIZE      4096UL
#define SMALL_BIT           12UL

/* The length of this array */
#define SECTION_LENGTH      (uint32_t)(MAX_ADDRESSING_SIZE / MMU_SECTION_SIZE)
/* The length of array */
#define SMALL_LENGTH     (MMU_SECTION_SIZE / MMU_SMALL_SIZE)


/* [19:0] ( MMU_SECTION_SIZE - 1 ) */
#define SECTION_OFFSET_MASK                     (MMU_SECTION_SIZE-1)
/* [11:0] ( MMU_SMALL_SIZE - 1 ) */
#define SMALL_PAGE_OFFSET_MASK                  (MMU_SMALL_SIZE-1)


/* #define MAX_ADDRESSING_SIZE ( 4096ULL * 1048576ULL ) */
/* #define MAX_ADDRESSING_BIT  ( 12UL + 20UL        ) */
/* #define MAX_ADDRESSING_SIZE ( 2048UL * 1048576UL ) */
/* #define MAX_ADDRESSING_BIT  ( 11UL   + 20UL      ) */
#define MAX_ADDRESSING_SIZE (1024UL * 1048576UL)
#define MAX_ADDRESSING_BIT  (10UL   + 20UL)
/* #define MAX_ADDRESSING_SIZE ( 256UL * 1048576UL ) */
/* #define MAX_ADDRESSING_BIT  ( 8UL + 20UL        ) */
/* #define MAX_ADDRESSING_SIZE ( 512UL * 1048576UL ) */
/* #define MAX_ADDRESSING_BIT  ( 9UL + 20UL        ) */

#define MIN_ADDRESSING_SIZE 4096UL
#define MIN_ADDRESSING_BIT 12UL

/* Max size per request. */
#define MAX_ALLOCATION_SIZE (1024UL * 1024UL * 1024UL)

/* We won't dereference those addresses inside the module, */
/* and we use the value only. */
typedef uintptr_t va_t;	/* virtual_address type */
typedef unsigned long pa_t;	/* physical_address type */
typedef unsigned int g3d_va_t;	/* g3d_virtual_address type */
typedef size_t mmu_size_t;	/* size_t / MMU_GRANULARITY */

#ifdef _DEBUG
#if defined(_WIN32)
#include <assert.h>
#define YL_MMU_ASSERT(x) assert(x)
#elif defined(linux) && !defined(linux_kernel)	/* linux user space */
#include <stdio.h>
#define YL_MMU_ASSERT(expr) \
do { \
	if (!(expr)) { \
		printf("G3DKMD MMU ASSERTION FAILED (%s) at: %s:%d:%s()\n", \
		#expr, __FILE__, __LINE__, __func__);\
	} \
} while (0)
#else
#include <linux/kernel.h>
#define YL_MMU_ASSERT(expr) \
do { \
	if (unlikely(!(expr))) { \
		pr_error(KBUILD_MODNAME ": " \
			"G3DKMD MMU ASSERTION FAILED (%s) at: %s:%d:%s()\n", \
			#expr, __FILE__, __LINE__, __func__); \
	} \
} while (0)
#endif
#else
#define YL_MMU_ASSERT(x)
#endif

#endif /* _YL_MMU_COMMON_H_ */
