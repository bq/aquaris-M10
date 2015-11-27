#pragma once
/// \file
/// \brief BinList class.

#include "yl_mmu_common.h"
#include "yl_mmu_bin_public_class.h"

#ifdef __cplusplus
extern "C" {
#endif



/// Init before use. Please pass in the memory.
void BinList_initialize( BinList self );
/// Uninit after use. Won't release the memory.
void BinList_release( BinList self);
/// \return the bin where bin.size exactly <= size.
/** bin[0].size == 4k */
Bin BinList_floor( BinRec bins[], size_t size );
/// Nearly best-fit-first search for suitable trunk
/** The returned bin is large enough to contain size */
Trunk BinList_scan_for_first_empty_trunk( BinList b, size_t size );
/// Only Set in_use to true. Due to algorithm change.
void BinList_attach_trunk_to_occupied_list( BinList b, Trunk trunk );
/// Put trunk back to empty list.
void BinList_attach_trunk_to_empty_list( BinList b, Trunk trunk );
/// Cut down the first trunk in this bin. Error if nothing inside the bin.
Trunk Bin_cut_down_first_trunk( Bin bin );
/// Return the ceiling index for given size.
static INLINE int BinList_index_ceiling( size_t size )  __attribute__((always_inline));
/// Return the ceiling index for given size. But minimum size is a section.
static INLINE int BinList_index_ceiling_section( size_t size )   __attribute__((always_inline));
/// Return the floor index for given size.
static INLINE int BinList_index_floor( size_t size )  __attribute__((always_inline));
/// Return the floor index for given size. But minimum size is a section.
static INLINE int BinList_index_floor_section( size_t size )  __attribute__((always_inline));
/// Return both floor index and the ceiling index for given size.
static INLINE void BinList_index_rounding( size_t size, int *out_index_floor, int *out_index_ceiling )  __attribute__((always_inline));
/// Return both floor index and the ceiling index for given size. But minimum size is a section.
static INLINE void BinList_index_rounding_section( size_t size, int *out_index_floor, int *out_index_ceiling )  __attribute__((always_inline));

/// Helper. Return the first trunk of this bin.
static INLINE Trunk Bin_first_trunk( Bin bin )  __attribute__((always_inline));

/// [Section Combining] Search for the empty trunk which can be used for section combining. 
Trunk BinList_scan_for_empty_trunk_section_fit( BinList bin_list, size_t size, unsigned short index1_start, unsigned short index1_tail);

/// Private. Hook trunk on BinList according to trunk.size
void _BinList_attach_trunk( BinRec bins[], Trunk trunk );

/// Private. Find the index of ceiling bin for given size. Start at (idx, bin_size)  
static INLINE int _BinList_index_ceiling_generic( size_t size, int idx, size_t bin_size )  __attribute__((always_inline));

/// Private. Find the index of floor bin for given size. Start at (idx, bin_size)
static INLINE int _BinList_index_floor_generic( size_t size, int idx, size_t bin_size )  __attribute__((always_inline));

/// Private. Return both floor index and the ceiling index.
static INLINE void 
_BinList_index_rounding_generic( size_t size, int idx, size_t bin_size, int *out_index_floor, int *out_index_ceiling )  __attribute__((always_inline));






/* Implementation */
static INLINE Trunk 
Bin_first_trunk( Bin bin )
{
    return bin->trunk_head.right_sibling;
}

static INLINE int 
_BinList_index_ceiling_generic( size_t size, int idx, size_t bin_size )
{
    while( bin_size < size )
    {
        bin_size <<= 1;
        ++idx;
    }

    YL_MMU_ASSERT( bin_size >= size );

    return idx;
}

static INLINE int 
_BinList_index_floor_generic( size_t size, int idx, size_t bin_size )
{
    size_t next_bin_size = bin_size << 1;

    while( next_bin_size <= size )
    {
        next_bin_size <<= 1;
        ++idx;
    }

    YL_MMU_ASSERT( next_bin_size > size );

    return idx;
}

static INLINE void 
_BinList_index_rounding_generic( size_t size, int idx, size_t bin_size, int *out_index_floor, int *out_index_ceiling )
{
    while( bin_size < size )
    {
        bin_size <<= 1;
        ++idx;
    }

    YL_MMU_ASSERT( bin_size >= size );

    *out_index_floor    = ( bin_size == size ) ? idx : idx-1;
    *out_index_ceiling  = idx;
}

static INLINE int 
BinList_index_ceiling( size_t size )
{
    return _BinList_index_ceiling_generic( size, 0, MMU_SMALL_SIZE );
}

static INLINE int 
BinList_index_ceiling_section( size_t size )
{
    return _BinList_index_ceiling_generic( size, SECTION_BIT-SMALL_BIT, MMU_SECTION_SIZE );
}

static INLINE int 
BinList_index_floor( size_t size )
{
    return _BinList_index_floor_generic( size, 0, MMU_SMALL_SIZE );
}

static INLINE int 
BinList_index_floor_section( size_t size )
{
    return _BinList_index_floor_generic( size, SECTION_BIT-SMALL_BIT, MMU_SECTION_SIZE );
}

static INLINE void 
BinList_index_rounding( size_t size, int *out_index_floor, int *out_index_ceiling )
{
    _BinList_index_rounding_generic( size, 0, MMU_SMALL_SIZE, out_index_floor, out_index_ceiling );
}

static INLINE void 
BinList_index_rounding_section( size_t size, int *out_index_floor, int *out_index_ceiling )
{
    _BinList_index_rounding_generic( size, SECTION_BIT-SMALL_BIT, MMU_SECTION_SIZE, out_index_floor, out_index_ceiling );
}



#ifdef __cplusplus
}
#endif

