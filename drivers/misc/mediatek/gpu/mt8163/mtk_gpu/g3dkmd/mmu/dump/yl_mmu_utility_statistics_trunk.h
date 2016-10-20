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

#ifndef _YL_MMU_UTILITY_STATISTICS_TRUNK_H_
#define _YL_MMU_UTILITY_STATISTICS_TRUNK_H_

#if defined(_WIN32)
#include <stdio.h>
#else
/* #include <linux/types.h> */
#if defined(linux_user_mode)
#include <stdio.h>
#endif
#endif

#include "yl_mmu_utility_common.h"
#include "../yl_mmu_mapper.h"

#if defined(_WIN32)
void Statistics_Trunk_Usage_Map(struct Mapper *m, FILE *fp);
#else
#if defined(linux_user_mode)	/* linux user space */
void Statistics_Trunk_Usage_Map(struct Mapper *m, FILE *fp);
#else
void Statistics_Trunk_Usage_Map(struct Mapper *m);
#endif
#endif

#endif /* _YL_MMU_UTILITY_STATISTICS_TRUNK_H_ */
