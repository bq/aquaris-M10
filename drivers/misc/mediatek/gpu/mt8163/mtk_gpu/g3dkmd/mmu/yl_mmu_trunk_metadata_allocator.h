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
/* brief Internal.  Memory Management for trunk objects. */
/** This class can be replaced by simply malloc,
 * but I don't want small and frequently allocation in kernel mode module.
 */

#ifndef _YL_MMU_TRUNK_METADATA_ALLOCATOR_H_
#define _YL_MMU_TRUNK_METADATA_ALLOCATOR_H_

#include "yl_mmu_common.h"
#include "yl_mmu_trunk_internal.h"



/********************* Data Structure *****************/
#define TRUNK_DUMMY ((struct Trunk *)-1)


/********************* Function Declaration *****************/
YL_BOOL TrunkMetaAlloc_initialize(void);


struct Trunk *TrunkMetaAlloc_alloc(void);

void TrunkMetaAlloc_free(struct Trunk *t);

struct Trunk *TrunkMetaAlloc_get_empty_head(void);

struct Trunk *TrunkMetaAlloc_get_used_head(void);

YL_BOOL TrunkMetaAlloc_release(void);


#endif /* _YL_MMU_TRUNK_METADATA_ALLOCATOR_H_ */
