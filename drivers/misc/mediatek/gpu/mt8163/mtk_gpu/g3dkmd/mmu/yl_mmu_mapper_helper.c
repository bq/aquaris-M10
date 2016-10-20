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

#include "yl_mmu_mapper_helper.h"

/* #include <assert.h> */

#include "yl_mmu_mapper_public_class.h"
#include "yl_mmu_hw_table.h"






/* size_t */
/* Mapper_get_required_size_all() */
/* { */
/* return Mapper_get_required_size_mapper() */
/* + Mapper_get_required_size_page_table() */
/* + Mapper_get_required_size_meta_table(); */
/* } */

size_t Mapper_get_required_size_mapper(void)
{
	return sizeof(struct Mapper);
}

size_t Mapper_get_required_size_page_table(void)
{
	return PageTable_get_required_size();
}

size_t Mapper_get_required_size_meta_table(void)
{
	return MetaTable_get_required_size();
}