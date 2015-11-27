#pragma once
/// \file
/// \brief Helper for size conversion.

#include "yl_mmu_common.h"

static INLINE size_t
Mapper_to_size( mmu_size_t msize )
{
    YL_MMU_ASSERT( msize );
    return msize << MMU_GRANULARITY_BIT;
}

/// Simple Floor
static INLINE mmu_size_t
Mapper_to_msize( size_t size )
{
    YL_MMU_ASSERT( size );
    return size >> MMU_GRANULARITY_BIT;
}


#ifdef __cplusplus
}
#endif

