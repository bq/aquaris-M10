#pragma once
/// \file
/// \brief Private.  Trunk.

#include "yl_mmu_common.h"
#include "yl_mmu_trunk_public_class.h"


static INLINE YL_BOOL Trunk_is_pivot( Trunk self ) __attribute__((always_inline));
static INLINE YL_BOOL Trunk_is_head( Trunk self ) __attribute__((always_inline));



static INLINE YL_BOOL
Trunk_is_pivot( Trunk self )
{
    return self->size == 0 ;
}

static INLINE YL_BOOL
Trunk_is_head( Trunk self )
{
    return self->pre->size == 0 ;
}
