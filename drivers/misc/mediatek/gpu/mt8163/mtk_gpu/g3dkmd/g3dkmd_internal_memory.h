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

#ifndef _G3DKMD_INTERNAL_MEMORY_H_
#define _G3DKMD_INTERNAL_MEMORY_H_
#include "g3d_memory.h"


	struct G3dMemory_;

/* kmd only */
	int allocatePhysicalMemory(struct G3dMemory_ *memory, unsigned int size,
				   unsigned int baseAlign, unsigned int flag);
	void freePhysicalMemory(struct G3dMemory_ *memory);


#endif /* _G3DKMD_INTERNAL_MEMORY_H_ */
