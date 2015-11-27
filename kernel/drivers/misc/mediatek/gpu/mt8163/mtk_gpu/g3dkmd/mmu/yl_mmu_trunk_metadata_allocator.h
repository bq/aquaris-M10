#pragma once
/// \file
/// \brief Internal.  Memory Management for trunk objects. 
/** This class can be replaced by simply malloc, 
 * but I don't want small and frequently allocation in kernel mode module.
 */


#include "yl_mmu_common.h"
#include "yl_mmu_trunk_internal.h"



#ifdef __cplusplus
extern "C" {
#endif



/********************* Data Sturcture *****************/
#define TRUNK_DUMMY ( (Trunk)-1 )


/********************* Function Declaration *****************/
YL_BOOL TrunkMetaAlloc_initialize(void);

    
Trunk TrunkMetaAlloc_alloc(void);

void TrunkMetaAlloc_free(Trunk t);

Trunk TrunkMetaAlloc_get_empty_head(void);

Trunk TrunkMetaAlloc_get_used_head(void);

YL_BOOL TrunkMetaAlloc_release(void);



#ifdef __cplusplus
}
#endif

