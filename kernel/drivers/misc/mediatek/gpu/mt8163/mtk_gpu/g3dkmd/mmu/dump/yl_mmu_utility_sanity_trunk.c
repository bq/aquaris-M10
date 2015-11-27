#include "yl_mmu_utility_sanity_trunk.h"
#include <assert.h>

YL_BOOL 
Sanity_Trunk_ALL( Mapper m )
{
    Trunk head  = m->Bin_list.first_trunk;
    Trunk t     = m->Bin_list.first_trunk;
    Trunk next;

    next = t->next;
    while ( next != head )
    {
        // Property Checking
        assert( t->size > 0 );
        assert( ! Trunk_is_pivot_trunk( t )   );
        assert( ! Trunk_is_pivot_bin( t )     );

        // Link Checking
        assert( next->pre == t );
        assert( t->next == next );

        t       = next;
        next    = t->next;
    }

    return 1;
}