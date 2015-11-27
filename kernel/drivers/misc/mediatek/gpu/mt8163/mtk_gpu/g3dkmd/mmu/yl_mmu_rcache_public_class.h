#pragma once
/// \file
/// \brief Class Definition. Reversed cache

#include "yl_mmu_common.h"
#include "yl_mmu_hw_table_public_class.h"



#ifdef __cplusplus
extern "C" {
#endif



/// 1.. 2047 can be used.
#define REMAPPER_RCACHE_ITEM_LENGTH 2048    
/// change _Rcache_calc_hash() if u change this
#define REMAPPER_RCACHE_SLOT_LENGTH 1024    

typedef unsigned short Rcache_link_t;       ///< an index type for entry
typedef unsigned short Rcache_slot_link_t;  ///< an index type for slot 

/// Private. Each slot contains a entry linklist.
typedef struct RcacheSlot_
{
    Rcache_link_t link;

} *RcacheSlot, RcacheSlotRec;

/// Private. Each cache entry is a (pa_t, trunk) pair.
typedef struct RcacheEntry_
{
    pa_t            pa_base;    ///< Key
    struct Trunk_   *trunk;      ///< Value
    Rcache_link_t   h_next;     ///< hash Link
    Rcache_link_t   q_next;     ///< queue Link

} RcacheEntryRec, *RcacheEntry;

/// Private. Reference count for cache entry.
typedef struct RcacheEntryCount_
{
    unsigned short  ref_cnt;    ///< Reference Count. 
                                ///< Number of allocation currently use this entry.

} RcacheEntryCountRec, *RcacheEntryCount;

/// Private. Queue to contain cache entries. Order of entries is not important  
typedef struct RcacheQueue_
{
    Rcache_link_t head;         ///< head point to first element
    Rcache_link_t tail;         ///< tail point to last element

} RcacheQueueRec, *RcacheQueue;

/// Public. Main class for Rcache.
/** size is 4 bytes aligned. **/
typedef struct Rcache_
{
    RcacheEntryRec      items[ REMAPPER_RCACHE_ITEM_LENGTH ];       ///< item[0] won't be used
    RcacheEntryCountRec item_cnts[ REMAPPER_RCACHE_ITEM_LENGTH ];   ///< item[0] won't be used
    RcacheSlotRec       slots[ REMAPPER_RCACHE_SLOT_LENGTH ];

    RcacheQueueRec  empty_queue;    ///< contains free entries.
    unsigned int    used_count;     ///< debug purpose.

} RcacheRec, *Rcache;


/// Get reference count
static INLINE unsigned short Rcache_get_count(Rcache r, Rcache_link_t iter)  __attribute__((always_inline));
/// Set reference count
static INLINE void Rcache_set_count(Rcache r, Rcache_link_t iter, unsigned short value) __attribute__((always_inline));
/// reference count ++
static INLINE unsigned short Rcache_count_inc(Rcache r, Rcache_link_t iter) __attribute__((always_inline));
/// reference count --
static INLINE unsigned short Rcache_count_dec(Rcache r, Rcache_link_t iter) __attribute__((always_inline));


/* Implementation */

static INLINE unsigned short 
Rcache_get_count(Rcache r, Rcache_link_t iter)
{
    return r->item_cnts[ iter ].ref_cnt;
}

static INLINE void
Rcache_set_count(Rcache r, Rcache_link_t iter, unsigned short value)
{
    r->item_cnts[ iter ].ref_cnt = value;
}

static INLINE unsigned short 
Rcache_count_inc(Rcache r, Rcache_link_t iter)
{
    return ++(r->item_cnts[ iter ].ref_cnt);
}

static INLINE unsigned short 
Rcache_count_dec(Rcache r, Rcache_link_t iter)
{
   return --(r->item_cnts[ iter ].ref_cnt);
}



#ifdef __cplusplus
}
#endif
