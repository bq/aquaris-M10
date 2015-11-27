#pragma once
/// \file
/// \brief A temp object to reduce va to pa translation.

#include "yl_mmu_section_lookup_public_class.h"

#include "yl_mmu_common.h"

#ifdef __cplusplus
extern "C" {
#endif


//typedef struct Mapper_ MapperRec, *Mapper;


void PaSectionLookupTable_set_valid( PaSectionLookupTable self );
void PaSectionLookupTable_set_invalid( PaSectionLookupTable self );
pa_t PaSectionLookupTable_use_if_availabile( Mapper m, PaSectionLookupTable self, int index1, va_t va_base );



#ifdef __cplusplus
}
#endif