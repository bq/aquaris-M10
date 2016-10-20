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

#ifndef _GPAD_LOG_H
#define _GPAD_LOG_H

#include <linux/kernel.h>

#define GPAD_LOG_SIG "[GPAD] "
#define GPAD_LOG_LEVEL_ERR      0
#define GPAD_LOG_LEVEL_INFO     1
#define GPAD_LOG_LEVEL_LOUD     2
#define GPAD_LOG_LEVEL GPAD_LOG_LEVEL_ERR

#define _GPAD_LOG(level, fmt, args...) \
			printk(level GPAD_LOG_SIG fmt, ##args)

#define GPAD_LOG(fmt, args...) \
			printk(GPAD_LOG_SIG fmt, ##args)

#if GPAD_LOG_LEVEL >= GPAD_LOG_LEVEL_LOUD
#define GPAD_LOG_LOUD(fmt, args...) \
			_GPAD_LOG(KERN_DEBUG, fmt, ##args)
#else
#define GPAD_LOG_LOUD(fmt, args...)
#endif

#if GPAD_LOG_LEVEL >= GPAD_LOG_LEVEL_INFO
#define GPAD_LOG_INFO(fmt, args...) \
			GPAD_LOG(KERN_INFO, fmt, ##args)
#else
#define GPAD_LOG_INFO(fmt, args...)
#endif

#if GPAD_LOG_LEVEL >= GPAD_LOG_LEVEL_ERR
#define GPAD_LOG_ERR(fmt, args...) \
			_GPAD_LOG(KERN_ERR, fmt, ##args)
#else
#define GPAD_LOG_ERR(fmt, args...)
#endif

#endif /* _GPAD_LOG_H */
