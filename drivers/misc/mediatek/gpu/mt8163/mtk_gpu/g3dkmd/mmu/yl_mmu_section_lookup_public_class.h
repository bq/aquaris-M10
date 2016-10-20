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
/* brief Class Definition. A temp object to reduce va to pa translation. */


#ifndef _YL_MMU_SECTION_LOOKUP_PUBLIC_CLASS_H_
#define _YL_MMU_SECTION_LOOKUP_PUBLIC_CLASS_H_

#include "yl_mmu_common.h"


/* 1kb data section, don't put into function stack, you will overflow the stack */
struct PaSectionLookupTable {
	YL_BOOL is_valid;
	char Reserved1;
	char Reserved2;
	char Reserved3;

	pa_t entries[SMALL_LENGTH];


};

#endif /* _YL_MMU_SECTION_LOOKUP_PUBLIC_CLASS_H_ */
