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

#ifndef _HOTPLUG
#define _HOTPLUG

#include <linux/kernel.h>
#include <linux/atomic.h>

/*
#define INFRACFG_AO_BASE	(0xF0001000)
#define MCUCFG_BASE		(0xF0200000)
*/

/* log */
#define HOTPLUG_LOG_NONE		0
#define HOTPLUG_LOG_WITH_DEBUG		1
#define HOTPLUG_LOG_WITH_WARN		2

#define HOTPLUG_LOG_PRINT		HOTPLUG_LOG_NONE

#if (HOTPLUG_LOG_PRINT == HOTPLUG_LOG_NONE)
#define HOTPLUG_INFO(fmt, args...)
#elif (HOTPLUG_LOG_PRINT == HOTPLUG_LOG_WITH_DEBUG)
#define HOTPLUG_INFO(fmt, args...)	pr_debug("[Power/hotplug] "fmt, ##args)
#elif (HOTPLUG_LOG_PRINT == HOTPLUG_LOG_WITH_WARN)
#define HOTPLUG_INFO(fmt, args...)	pr_warn("[Power/hotplug] "fmt, ##args)
#endif

/* profilling */
/* #define CONFIG_HOTPLUG_PROFILING */
#define CONFIG_HOTPLUG_PROFILING_COUNT		100

/* register address - bootrom power*/
/*
#define BOOTROM_BOOT_ADDR		(INFRACFG_AO_BASE + 0x800)
#define BOOTROM_SEC_CTRL		(INFRACFG_AO_BASE + 0x804)
*/

#define REG_WRITE(addr, value)		mt_reg_sync_writel(value, addr)

/* power on/off cpu*/
#define CONFIG_HOTPLUG_WITH_POWER_CTRL

/* global variable */
extern atomic_t hotplug_cpu_count;

extern void __disable_dcache(void);
extern void __enable_dcache(void);
extern void __inner_clean_dcache_L2(void);
extern void inner_dcache_flush_L1(void);
extern void inner_dcache_flush_L2(void);
extern void __switch_to_smp(void);
extern void __switch_to_amp(void);
extern void __disable_dcache__inner_flush_dcache_L1(void);
extern void
__disable_dcache__inner_flush_dcache_L1__inner_clean_dcache_L2(void);
extern void
__disable_dcache__inner_flush_dcache_L1__inner_flush_dcache_L2(void);

/* mt cpu hotplug callback for smp_operations */
extern int mt_cpu_kill(unsigned int cpu);
extern void mt_cpu_die(unsigned int cpu);
extern int mt_cpu_disable(unsigned int cpu);

#endif
