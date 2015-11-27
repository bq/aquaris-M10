#pragma once
/// \file
/// \brief Internal.  Trunk.

#if defined(_WIN32)
#include <limits.h>
#else
#if defined(linux) && defined(linux_user_mode)
#include <limits.h>
//#include <linux/types.h>
#endif
#endif

#include "yl_mmu_common.h"
#include "yl_mmu_trunk_public_class.h"
#include "yl_mmu_hw_table.h"
#include "yl_mmu_trunk_private.h"
#include "yl_mmu_trunk_helper_link.h"



#ifdef __cplusplus
extern "C" {
#endif







/********************* Private Function Declarations *****************/

// For initialization
Trunk Trunk_create_first_trunk_and_pivot(size_t size);

// For Termination
void Trunk_release_list(Trunk trunk_head);

Trunk Trunk_split_tail(Trunk current, size_t second_size);
Trunk Trunk_split_front(Trunk current, size_t amount1);
Trunk Trunk_free( Trunk target, Trunk first_trunk );
YL_BOOL Trunk_is_start_from_small_page( Trunk current );
static INLINE g3d_va_t Trunk_get_g3d_va( Trunk self, FirstPageTable master, g3d_va_t g3d_va_base, va_t va ) __attribute__((always_inline));



//unsigned short 
//Trunk_reference_count(Trunk self)
//{
//    return self->ref_cnt;
//}
//
//unsigned short 
//Trunk_increase_reference_count(Trunk self)
//{
//    YL_MMU_ASSERT( self->ref_cnt < USHRT_MAX );
//
//    return ++self->ref_cnt;
//}
//
//unsigned short 
//Trunk_decrease_reference_count(Trunk self)
//{
//    YL_MMU_ASSERT( self->ref_cnt > 0 );
//
//    return --self->ref_cnt;
//}

/********************* Private Function Declarations *****************/

static INLINE g3d_va_t 
Trunk_get_g3d_va( Trunk self, FirstPageTable master, g3d_va_t g3d_va_base, va_t va )
{
    //if ( Trunk_is_start_from_small_page( self ) )
    //if ( Section_has_second_level(master, self->index0) )
    if ( PageTable_has_second_level( master, self->index.s.index0 ) )
    {
        return SmallPage_index_to_g3d_va( master, g3d_va_base, self->index, va );
    }
    else
    {
        return SectionPage_index_to_g3d_va( master, g3d_va_base, self->index.s.index0, va );
    }
}



#ifdef __cplusplus
}
#endif
