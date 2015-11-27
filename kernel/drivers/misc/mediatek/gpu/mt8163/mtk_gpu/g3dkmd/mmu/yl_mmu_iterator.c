#include "yl_mmu_iterator.h"

#include "yl_mmu_trunk_internal.h"
#include "yl_mmu_kernel_alloc.h"


void 
RemapperIterator_Init( RemapperIterator iter, Mapper m, g3d_va_t g3d_va, size_t size )
{
    iter->m                 = m;
    iter->Page_space        = m->Page_space;
    iter->g3d_address       = g3d_va - m->G3d_va_base;
    
    PageTable_g3d_va_to_index( m->G3d_va_base, g3d_va, & iter->index );

    iter->size              = size;
    iter->cursor            = 0;

    _RemapperIterator_update_size_to_next_boundary( iter );
}

void 
RemapperIterator_Uninit( RemapperIterator iter )
{

    iter->m                 = NULL;
    iter->Page_space        = NULL;
    iter->g3d_address       = 0;
    iter->trunk             = NULL;
    iter->index.all         = 0;
    iter->size              = 0;
    iter->cursor            = 0;
}

