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

#ifndef _LINUX_SW_SYNC64_H
#define _LINUX_SW_SYNC64_H

#include <linux/types.h>
#include <linux/kconfig.h>
#include "sync.h"
/* #include "uapi/sw_sync.h" */

struct sw_sync64_create_fence_data {
	__u64 value;
	char name[32];
	__s32 fence; /* fd of new fence */
};

#define SW_SYNC64_IOC_MAGIC       'W'

#define SW_SYNC64_IOC_CREATE_FENCE        _IOWR(SW_SYNC64_IOC_MAGIC, 0,\
		struct sw_sync64_create_fence_data)
#define SW_SYNC64_IOC_INC                 _IOW(SW_SYNC64_IOC_MAGIC, 1, __u64)

struct sw_sync64_timeline {
	struct sync_timeline obj;

	u64 value;
};

struct sw_sync64_pt {
	struct sync_pt pt;

	u64 value;
};

#if IS_ENABLED(CONFIG_SW_SYNC64)
struct sw_sync64_timeline *sw_sync64_timeline_create(const char *name);
void sw_sync64_timeline_inc(struct sw_sync64_timeline *obj, u64 inc);

struct sync_pt *sw_sync64_pt_create(struct sw_sync64_timeline *obj, u64 value);
#else
static inline struct sw_sync64_timeline *sw_sync64_timeline_create(const char *name)
{
	return NULL;
}

static inline void sw_sync64_timeline_inc(struct sw_sync64_timeline *obj, u64 inc)
{
}

static inline struct sync_pt *sw_sync64_pt_create(struct sw_sync64_timeline *obj, u32 value)
{
	return NULL;
}
#endif /* IS_ENABLED(CONFIG_SW_SYNC64) */

#endif /* _LINUX_SW_SYNC64_H */
