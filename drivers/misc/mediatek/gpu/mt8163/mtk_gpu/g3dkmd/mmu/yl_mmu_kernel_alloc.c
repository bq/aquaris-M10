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

#if defined(_WIN32)
#include <string.h>		/* For memset */
#include <stdlib.h>		/* For malloc */
#else
#if defined(linux_user_mode)	/* linux user space */
#include <string.h>		/* For memset */
#include <stdlib.h>		/* For malloc */
#else
#include <linux/vmalloc.h>
#include <linux/string.h>
#endif
#endif

#include "yl_mmu_common.h"
#include "yl_mmu_kernel_alloc.h"
#include "../g3dkmd_engine.h"

#include "../g3dkmd_file_io.h"	/* For KMDDPF */

void *yl_mmu_alloc(size_t size)
{
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))
	return malloc(size);
#else
#ifdef G3DKMD_MEM_CHECK
	void *p = vmalloc(size);

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_MMU, "(SW_MEMORY) vmalloc: ptr = 0x%x, size = %d\n",
	       p, size);

	return p;
#else
	return vmalloc(size);
#endif
#endif
}

void yl_mmu_free(void *p)
{
	if (p) {
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))
		free(p);
#else
#ifdef G3DKMD_MEM_CHECK
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_MMU, "(SW_MEMORY) vfree: ptr = 0x%x\n", p);
#endif
		vfree(p);
#endif
	}
}

void yl_mmu_zeromem(void *p, size_t size)
{
	memset(p, 0, size);
}


void yl_mmu_memset(void *p, unsigned char value, size_t size)
{
	memset(p, value, size);
}


void yl_mmu_memcpy(void *destination, const void *source, size_t size)
{
	memcpy(destination, source, size);
}

int yl_mmu_memcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}
