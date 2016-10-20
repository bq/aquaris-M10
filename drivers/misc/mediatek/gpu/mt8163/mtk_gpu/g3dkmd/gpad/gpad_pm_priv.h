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

#ifndef _GPAD_PM_PRIV_H
#define _GPAD_PM_PRIV_H

/*
 * Priviate helper utilities.
 */
#define IS_PM_ENABLED(_dev) \
	GPAD_CHECK_DEV_STATUS(_dev, GPAD_PM_ENABLE)

#define GET_PM_INFO(_dev) \
	(&((_dev)->pm_info))

#define USE_FREE_SAMPLE_LIST_LOCK \
	unsigned long free_sample_lock_flags

#define LOCK_FREE_SAMPLE_LIST(_pm_info) \
	spin_lock_irqsave(&(_pm_info)->free_sample_lock, free_sample_lock_flags)

#define UNLOCK_FREE_SAMPLE_LIST(_pm_info) \
	spin_unlock_irqrestore(&(_pm_info)->free_sample_lock, free_sample_lock_flags)

#define USE_USED_SAMPLE_LIST_LOCK \
	unsigned long used_sample_lock_flags

#define LOCK_USED_SAMPLE_LIST(_pm_info) \
	spin_lock_irqsave(&(_pm_info)->used_sample_lock, used_sample_lock_flags)

#define UNLOCK_USED_SAMPLE_LIST(_pm_info) \
	spin_unlock_irqrestore(&(_pm_info)->used_sample_lock, used_sample_lock_flags)

#define LOCK_ALL_SAMPLE_LIST(_pm_info) \
	{ \
		LOCK_FREE_SAMPLE_LIST(_pm_info); \
		LOCK_USED_SAMPLE_LIST(_pm_info); \
	}

#define UNLOCK_ALL_SAMPLE_LIST(_pm_info) \
	{ \
	UNLOCK_USED_SAMPLE_LIST(_pm_info); \
	UNLOCK_FREE_SAMPLE_LIST(_pm_info); \
	}

#define USE_HW_BUFFER \
	unsigned long hw_lock_flags

#define LOCK_HW_BUFFER(_pm_info) \
	spin_lock_irqsave(&(_pm_info)->hw_buffer_lock, hw_lock_flags)

#define UNLOCK_HW_BUFFER(_pm_info) \
	spin_unlock_irqrestore(&(_pm_info)->hw_buffer_lock, hw_lock_flags)

#define IS_SAMPLE_READY(_sample) \
	((_sample)->header.tag != 0)

#define RECYCLE_SAMPLE(_sample) \
	(_sample)->header.tag = 0

#define IS_DEFAULT_CONFIG(_pm_info) \
	(0 == (_pm_info)->config || 49 == (_pm_info)->config)

#define MS_TO_JIFFIES(_ms) \
	(((_ms) * HZ + 999) / 1000)

#define FIELD_OFFSET(_struct, _field) \
	((unsigned long) &((_struct *)0)->_field)

#endif /* ifndef _GPAD_PM_PRIV_H */
