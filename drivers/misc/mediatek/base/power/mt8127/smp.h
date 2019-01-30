/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MT_SMP_H
#define __MT_SMP_H

#include <linux/cpumask.h>
#include <asm/cputype.h>

#include <mt-plat/sync_write.h>

#include "hotplug.h"

#ifdef CONFIG_ARM64
static inline int get_HW_cpuid(void)
{
	u64 mpidr;
	u32 id;

	mpidr = read_cpuid_mpidr();
	id = (mpidr & 0xff) + ((mpidr & 0xff00) >> 6);

	return id;
}
#else
static inline int get_HW_cpuid(void)
{
	int id;

	asm("mrc     p15, 0, %0, c0, c0, 5 @ Get CPUID\n":"=r"(id));
	return (id & 0x3) + ((id & 0xF00) >> 6);
}
#endif


#endif
