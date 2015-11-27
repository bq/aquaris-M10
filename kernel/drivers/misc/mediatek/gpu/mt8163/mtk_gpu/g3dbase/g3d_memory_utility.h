/// Shared by G3D / G3DKMD
#ifndef _G3D_MEMORY_UTILITY_H_
#define _G3D_MEMORY_UTILITY_H_

#include "g3dbase_common.h"
#include "g3d_memory.h"

INLINE static void 
g3dbaseCleanG3dMemory( G3dMemory *target )
{
    target->data0   = 0;
    target->data    = 0;
    target->hwAddr0 = 0;
    target->hwAddr  = 0;
    target->phyAddr = 0;
    target->size0   = 0;
    target->size    = 0;
    target->remap   = 0;
//#if defined(YL_PATTERN) && defined(CMODEL)
    target->patPhyAddr = 0;
#ifdef YL_MMU_PATTERN_DUMP_V1
    target->pa_pair_info = 0;
#endif
//#endif
    target->deferAlloc = 0;
//    target->type    = G3D_MEM_NONE;
}


INLINE static unsigned int 
g3dbaseVaToContinuousPa( const G3dMemory *base, void* va )
{
    return (unsigned int)( (uintptr_t)va - (uintptr_t)base->data ) + base->hwAddr;
}

INLINE static void * 
g3dbaseContinuousPaToVa( const G3dMemory *base, unsigned int pa )
{
    return (void *)( pa - base->hwAddr + (uintptr_t)base->data );
}

#endif //_G3D_MEMORY_H_
