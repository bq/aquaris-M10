#pragma once
/// \file
/// \brief Class Definition. A temp object to reduce va to pa translation.

#include "yl_mmu_common.h"

#ifdef __cplusplus
extern "C" {
#endif



// 1kb data section, don't put into function stack, you will overflow the stack
typedef struct PaSectionLookupTable_
{
    YL_BOOL is_valid;
    char Reserved1;
    char Reserved2;
    char Reserved3;

    pa_t entries[ SMALL_LENGTH ];


} PaSectionLookupTableRec, *PaSectionLookupTable;





#ifdef __cplusplus
}
#endif