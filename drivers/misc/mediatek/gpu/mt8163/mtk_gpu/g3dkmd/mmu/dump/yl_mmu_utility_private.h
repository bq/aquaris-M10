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

#ifndef _YL_MMU_UTILITY_PRIVATE_H_
#define _YL_MMU_UTILITY_PRIVATE_H_

#include "../yl_mmu_mapper.h"

size_t times(char c, int n, char *buf);
size_t render(struct Mapper *m, struct Trunk *t, char c_section, char c_small, char *buf);

#endif /* _YL_MMU_UTILITY_PRIVATE_H_ */
