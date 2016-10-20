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
/* brief Helper. Page Index, index0 is for first-level page table index1 is for second-level. */

#ifndef _YL_MMU_INDEX_PUBLIC_CLASS_H_
#define _YL_MMU_INDEX_PUBLIC_CLASS_H_

typedef union PageIndex_ {
	unsigned int all;
	struct {
		unsigned short index0;
		unsigned short index1;
	} s;

} *PageIndex, PageIndexRec;

#endif /* _YL_MMU_INDEX_PUBLIC_CLASS_H_ */
