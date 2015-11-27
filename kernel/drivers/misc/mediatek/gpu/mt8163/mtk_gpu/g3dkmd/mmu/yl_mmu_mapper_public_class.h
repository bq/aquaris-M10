#pragma once
/// \file
/// \brief Class definition.  Mapper is the main object of this module.
#include "yl_mmu_common.h"
#include "yl_mmu_bin_public_class.h"
#include "yl_mmu_hw_table_public_class.h"
#include "yl_mmu_rcache_public_class.h"
#include "yl_mmu_section_lookup_public_class.h"
#include "g3d_memory.h"

/// Mapper, the main object for this module.
typedef struct Mapper_
{
    /// The base address for our G3dVa 
    g3d_va_t                G3d_va_base; // Can be 0

    /// Page Table : use internally and also read by hw
    G3dMemory               Page_mem;
    FirstPageTable          Page_space;  // give me the continuous physical space to store the page table

    /// Internal data structure
    FirstMetaTable          Meta_space;
    BinListRec              Bin_list;

    /// [Small Packing] Reversed cache.
    RcacheRec               Rcache;

    /// An temp object to reduce the amount of table lookup.
    PaSectionLookupTableRec Pa_section_lookup_table;

    /// Cache for small page size squeezing 
    int                     Cache_enable;

    /// Mirror : G3dVa -> G3dVa
#if MMU_MIRROR
    G3dMemory               Mirror_mem;
    FirstPageTable          Mirror_space;
#endif 

    int8_t                  is_pa_given;
    va_t                    GIVEN_PA_va_base;
    pa_t                    GIVEN_PA_pa_base;

#if MMU_LOOKBACK_VA      // used for lookback to get va from g3d_va
    G3dMemory               lookback_mem;
    //void*                   Page_space_lookback0; // from alloc memory and can't be physical memory 
    FirstPageTable          Page_space_lookback;  // pointer to FirstPageTableRec 
#endif

} MapperRec, *Mapper;

