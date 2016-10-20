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

#ifndef _G3DKMD_PLATFORM_SYSINFO_H_
#define _G3DKMD_PLATFORM_SYSINFO_H_

#if defined(linux) && !defined(linux_user_mode) /* linux kernel */

/* 1. Query System Memory */
/* - void si_meminfo(struct sysinfo *val); */

/* headers */
#include <linux/mm.h> /* si_meminfo(): query system memory */
#include <linux/sysinfo.h> /* struct sysinfo */

/* structs */
/*typedef struct sysinfo g3dkmd_sysinfo_t;*/


/* macros */
#define g3dkmd_get_sysinfo(si) si_meminfo(si)



#else /* WIN or linux_user */


/* headers */

/* structs */
struct sysinfo {
	/* __kernel_long_t uptime;*/ /* Seconds since boot */
	/* __kernel_ulong_t loads[3];*/ /* 1, 5, and 15 minute load averages */
	/* __kernel_ulong_t totalram;*/ /* Total usable main memory size */
	/* __kernel_ulong_t freeram;*/ /* Available memory size */
	unsigned long_t freeram; /* Available memory size */
	/* __kernel_ulong_t sharedram;*/ /* Amount of shared memory */
	/* __kernel_ulong_t bufferram;*/ /* Memory used by buffers */
	/* __kernel_ulong_t totalswap;*/ /* Total swap space size */
	/* __kernel_ulong_t freeswap;*/ /* swap space still available */
	/* __u16 procs;*/ /* Number of current processes */
	/* __u16 pad;*/ /* Explicit padding for m68k */
	/* __kernel_ulong_t totalhigh;*/ /* Total high memory size */
	/* __kernel_ulong_t freehigh;*/ /* Available high memory size */
	/* __u32 mem_unit;*/ /* Memory unit size in bytes */
	/* char _f[20-2*sizeof(__kernel_ulong_t)-sizeof(__u32)];*/ /* Padding: libc5 uses this.. */
};


/* macros */
#define g3dkmd_get_sysinfo(si) (si->freeram = 0)
#endif

#endif /* _G3DKMD_PLATFORM_SYSINFO_H_ */
