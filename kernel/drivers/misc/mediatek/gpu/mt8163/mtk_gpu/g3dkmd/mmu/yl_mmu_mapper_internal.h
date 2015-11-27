#pragma once
/// \file
/// \brief Used inside the module. Mapper is the main object of this module.

/*
 *
 * Used by yl_mmu_mapper and yl_mmu_mapper_internal
 *
 */

#include "yl_mmu_common.h"

#include "yl_mmu_mapper.h"

#include "yl_mmu_hw_table.h"
#include "yl_mmu_trunk_internal.h"
#include "yl_mmu_bin_internal.h"



#ifdef __cplusplus
extern "C" {
#endif



/********************* Private Function Declarations *****************/

//void release_trunks(Trunk trunk);

//Trunk create_trunk();

//void initialize_bin_list(BinList theList);

YL_BOOL register_table(Mapper m, Trunk t, va_t va, YL_BOOL section_detected, unsigned int flag);

YL_BOOL unregister_table( Mapper m, Trunk t );


/************** small private helper functions *****************/
/************** Really need to think about where to put them and what's the naming convension *************/

static INLINE YL_BOOL _Mapper_is_section_aligned( va_t address ) __attribute__((always_inline));

// Use pa_section_lookup & pa_section_lookup_is_valid
YL_BOOL _Mapper_is_continuous_section( Mapper m, PaSectionLookupTable lookup_table, va_t va );

YL_BOOL _Mapper_contain_continuous_section( Mapper m, PaSectionLookupTable lookup_table, va_t va, size_t occupied_size );






static INLINE YL_BOOL 
_Mapper_is_section_aligned( va_t address )
{
    return ( address & SECTION_OFFSET_MASK ) ? 0 : 1;
}

/************** END small private helper functions *****************/


#ifdef __cplusplus
}
#endif
