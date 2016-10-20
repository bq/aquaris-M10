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
/* brief Private. MMU Table for Hardware reading. */

#ifndef _YL_MMU_HW_TABLE_PRIVATE_H_
#define _YL_MMU_HW_TABLE_PRIVATE_H_

#include "yl_mmu_common.h"
#include "yl_mmu_hw_table.h"




static INLINE void FirstPage_entry_release(struct FirstPageEntry *entry) __attribute__ ((always_inline));

static INLINE void FirstMeta_entry_release(struct FirstMetaEntry *meta_entry) __attribute__ ((always_inline));

static INLINE void SecondPage_entry_release(struct SecondPageEntry *entry) __attribute__ ((always_inline));

static INLINE void SecondMeta_entry_release(struct SecondMetaEntry *meta_entry) __attribute__ ((always_inline));

static INLINE void SecondPage_release(struct SecondPageTable *table) __attribute__ ((always_inline));

static INLINE void SecondMeta_release(struct SecondMetaTable *meta_table) __attribute__ ((always_inline));



/*************************** Entry Release ****************************/

static INLINE void FirstPage_entry_release(struct FirstPageEntry *entry)
{
	entry->all = 0;		/* 32bits */
}

static INLINE void FirstMeta_entry_release(struct FirstMetaEntry *meta_entry)
{
	meta_entry->trunk = NULL;
}

static INLINE void SecondPage_entry_release(struct SecondPageEntry *entry)
{
	/* TODO check if all 0 will be okey or not */
	entry->all = 0;		/* 32bits */
}

static INLINE void SecondMeta_entry_release(struct SecondMetaEntry *meta_entry)
{
	meta_entry->trunk = NULL;	/* 32bits */
}


/*************************** Table Release ****************************/

static INLINE void SecondPage_release(struct SecondPageTable *table)
{
	unsigned int i;

	for (i = 0; i < SMALL_LENGTH; ++i)
		SecondPage_entry_release(&table->entries[i]);

}

static INLINE void SecondMeta_release(struct SecondMetaTable *meta_table)
{
	unsigned int i;

	for (i = 0; i < SMALL_LENGTH; ++i)
		SecondMeta_entry_release(&meta_table->entries[i]);

	meta_table->cnt = 0;
}

#endif /* _YL_MMU_HW_TABLE_PRIVATE_H_ */