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
/* brief Allocation functions.  Need to change when port to kernel mode module. */

/**
 * A wrapper for real resource allocation from system.
 *
 * malloc / free / zeromemory can't used in kernal, should be replace by other functions.
 *
 */

#ifndef _YL_MMU_KERNEL_ALLOC_H_
#define _YL_MMU_KERNEL_ALLOC_H_

#include "yl_mmu_common.h"

/* Should be replaced by valloc() */
void *yl_mmu_alloc(size_t size);

/* Should be replaced by free() */
void yl_mmu_free(void *p);

/* Should be re-written manually */
void yl_mmu_zeromem(void *p, size_t size);

/* Should be re-written manually */
void yl_mmu_memset(void *p, unsigned char value, size_t size);

/* Should be re-written manually */
void yl_mmu_memcpy(void *destination, const void *source, size_t size);

/* Should be re-written manually */
int yl_mmu_memcmp(const void *s1, const void *s2, size_t n);

#endif /* _YL_MMU_KERNEL_ALLOC_H_ */
