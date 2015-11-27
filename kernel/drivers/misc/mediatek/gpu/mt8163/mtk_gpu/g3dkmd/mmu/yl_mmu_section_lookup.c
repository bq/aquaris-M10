
//#include <assert.h>

#include "yl_mmu_system_address_translator.h"
#include "yl_mmu_section_lookup.h"

void PaSectionLookupTable_set_invalid( PaSectionLookupTable self )
{
    self->is_valid = 0;
}

void PaSectionLookupTable_set_valid( PaSectionLookupTable self )
{
    self->is_valid = 1;
}

pa_t PaSectionLookupTable_use_if_availabile( Mapper m, PaSectionLookupTable self, int index1, va_t va_base )
{
    return ( self->is_valid ) ? 
        self->entries[index1] :
        va_to_pa( m, va_base );
}


