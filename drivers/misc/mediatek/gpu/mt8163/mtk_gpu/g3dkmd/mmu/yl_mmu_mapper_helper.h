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
/* brief Mapper is the main object of this module. */

#ifndef  _YL_MMU_MAPPER_HELPER_H_
#define  _YL_MMU_MAPPER_HELPER_H_

#include "yl_mmu_hw_table.h"

/*
 * We don't inline anything for the public API in case we make it as DLL or .so
 *
 */

#define Mapper_get_section_length   SECTION_LENGTH
#define Mapper_get_small_length     SMALL_LENGTH

/* size_t Mapper_get_required_size_all(void)           __attribute__((const)); */
size_t Mapper_get_required_size_mapper(void) __attribute__ ((const));
size_t Mapper_get_required_size_page_table(void) __attribute__ ((const));
size_t Mapper_get_required_size_meta_table(void) __attribute__ ((const));

/**************** Implememtation ***************/


#endif /* _YL_MMU_MAPPER_HELPER_H_ */
