#pragma once
/// \file
/// \brief Private. MMU Table for Hardware reading.

#include "yl_mmu_common.h"
#include "yl_mmu_hw_table.h"




static INLINE void FirstPage_entry_release( FirstPageEntry entry ) __attribute__((always_inline));

static INLINE void FirstMeta_entry_release( FirstMetaEntry meta_entry ) __attribute__((always_inline));

static INLINE void SecondPage_entry_release( SecondPageEntry entry ) __attribute__((always_inline));

static INLINE void SecondMeta_entry_release( SecondMetaEntry meta_entry ) __attribute__((always_inline));

static INLINE void SecondPage_release( SecondPageTable table ) __attribute__((always_inline));

static INLINE void SecondMeta_release( SecondMetaTable meta_table ) __attribute__((always_inline));



/*************************** Entry Release ****************************/

static INLINE void 
FirstPage_entry_release( FirstPageEntry entry )
{
    entry->all = 0; // 32bits
}

static INLINE void
FirstMeta_entry_release( FirstMetaEntry meta_entry )
{
    meta_entry->trunk = NULL;
}

static INLINE void 
SecondPage_entry_release( SecondPageEntry entry )
{
    // TODO check if all 0 will be okey or not
    entry->all = 0; // 32bits
}

static INLINE void
SecondMeta_entry_release( SecondMetaEntry meta_entry )
{
    meta_entry->trunk = NULL;  // 32bits
}


/*************************** Table Release ****************************/

static INLINE void
SecondPage_release( SecondPageTable table )
{
    unsigned int i;

    for ( i=0; i < SMALL_LENGTH; ++i )
    {
        SecondPage_entry_release(  & table->entries[i] );
    }
}

static INLINE void
SecondMeta_release( SecondMetaTable meta_table )
{
    unsigned int i;

    for ( i=0; i < SMALL_LENGTH; ++i )
    {
        SecondMeta_entry_release( & meta_table->entries[i] );
    }
    meta_table->cnt = 0;
}
