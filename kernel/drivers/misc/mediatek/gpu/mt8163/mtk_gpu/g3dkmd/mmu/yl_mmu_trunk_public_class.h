#pragma once
/// \file
/// \brief Class Definition. Trunk.

#include "yl_mmu_common.h"
#include "yl_mmu_hw_table_public_class.h"


#ifdef __cplusplus
extern "C" {
#endif

/********************* Data Sturcture *****************/

typedef struct Trunk_
{
    size_t              size;
    struct Trunk_ *     pre;          // in addressing space
    struct Trunk_ *     next;         // in addressing space
    struct Trunk_ *     left_sibling;     // inside bin
    struct Trunk_ *     right_sibling;    // inside bin

    PageIndexRec        index;

    unsigned char       flags;

} TrunkRec, *Trunk;

static INLINE void Trunk_set_use(Trunk self) 		__attribute__((always_inline));
static INLINE void Trunk_unset_use(Trunk self) 		__attribute__((always_inline));
static INLINE YL_BOOL Trunk_in_use(Trunk self) 			__attribute__((always_inline));
static INLINE void Trunk_unset_pivot_trunk(Trunk self) 	__attribute__((always_inline));
static INLINE YL_BOOL Trunk_is_pivot_trunk(Trunk self) 		__attribute__((always_inline));
static INLINE void Trunk_set_pivot_bin(Trunk self) 		__attribute__((always_inline));
static INLINE void Trunk_unset_pivot_bin(Trunk self) 	__attribute__((always_inline));
static INLINE YL_BOOL Trunk_is_pivot_bin(Trunk self) 		__attribute__((always_inline));


/******** Small tools ********/
static INLINE void 
Trunk_set_use(Trunk self)
{
    self->flags |= 0x1;
}

static INLINE void 
Trunk_unset_use(Trunk self)
{
    self->flags &= ~0x1;
}

static INLINE YL_BOOL 
Trunk_in_use(Trunk self)
{
    return (self->flags & 0x1) > 0;
}

static INLINE void 
Trunk_set_pivot_trunk(Trunk self)
{
    self->flags |= 0x2;
}

static INLINE void 
Trunk_unset_pivot_trunk(Trunk self)
{
    self->flags &= ~0x2;
}

static INLINE YL_BOOL 
Trunk_is_pivot_trunk(Trunk self)
{
    return (self->flags & 0x2) > 0;
    //return self->is_pivot_trunk > 0;
}

static INLINE void 
Trunk_set_pivot_bin(Trunk self)
{
    self->flags |= 0x4;
}

static INLINE void 
Trunk_unset_pivot_bin(Trunk self)
{
    self->flags &= ~0x4;
}

static INLINE YL_BOOL 
Trunk_is_pivot_bin(Trunk self)
{
    return (self->flags & 0x4) > 0;
    //return self->is_pivot_bin > 0;
}


#ifdef __cplusplus
}
#endif
