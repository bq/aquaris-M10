//#include <assert.h>

#include "yl_mmu_trunk_metadata_allocator.h"
#include "yl_mmu_mapper_internal.h"
#include "yl_mmu_kernel_alloc.h"



#ifdef __cplusplus
extern "C" {
#endif



#define BARREL_SIZE 1000

typedef struct TrunkWrapper_
{
    TrunkRec                root;
    struct TrunkWrapper_ *  pre;          // this Free => pre free   // same as occupied 
    struct TrunkWrapper_ *  next;         // this free => next free 
} TrunkWrapperRec, *TrunkWrapper;


typedef struct TrunkBarrel_
{
    TrunkWrapperRec                 trunks[BARREL_SIZE];
    struct TrunkBarrel_ *           pre;          // this Free => pre free   // same as occupied 
    struct TrunkBarrel_ *           next;         // this free => next free 
} TrunkBarrelRec, *TrunkBarrel;


#define FIRST_TRUNK(barrel) ((barrel)->trunks)
#define LAST_TRUNK(barrel)  ((barrel)->trunks + (BARREL_SIZE-1))


TrunkBarrel trunk_barrel_head = 0; // trunk memory block
TrunkBarrel trunk_barrel_tail = 0; 

TrunkWrapper free_trunk_head = 0;
TrunkWrapper free_trunk_tail = 0;

TrunkWrapper used_trunk_head = 0;
TrunkWrapper used_trunk_tail = 0;


/************************ TrunkBarrel ***************************/


static void
insert_trunks(TrunkWrapper anchor, TrunkWrapper first, TrunkWrapper last)
{
    TrunkWrapper next   = anchor->next;
    anchor->next        = first;
    first->pre          = anchor;

    next->pre           = last;
    last->next          = next;
}

static void
insert_trunk(TrunkWrapper anchor, TrunkWrapper item)
{
    TrunkWrapper next   = anchor->next;

    anchor->next        = item;
    item->pre           = anchor;

    next->pre           = item;
    item->next          = next;
}

static TrunkWrapper 
cut_next_trunk(TrunkWrapper anchor)
{
    TrunkWrapper item   = anchor->next;
    TrunkWrapper next   = item->next;

    YL_MMU_ASSERT( anchor != item  );
    YL_MMU_ASSERT( item   != next  );
    YL_MMU_ASSERT( anchor != next  );

    anchor->next        = next;
    next->pre           = anchor;

    item->pre           = NULL;
    item->next          = NULL;

    return item;
}




/* new_trunk_barrel()
 * Create a trunk barrel and arrange the link from top to down
 */
static TrunkBarrel 
TrunkBarrel_new(void)
{
    size_t size = sizeof(TrunkBarrelRec);

    // alloc ...
    TrunkBarrel barrel = (TrunkBarrel)yl_mmu_alloc( size );

    if ( ! barrel )
    {
        return NULL;
    }

    // Make all trunks free but wrong links
    yl_mmu_zeromem(barrel, size);

    // Fix free trunks
    {
        TrunkWrapper current    = FIRST_TRUNK(barrel);
        TrunkWrapper last       = LAST_TRUNK(barrel);
        TrunkWrapper next       = NULL;

        current->pre            = NULL;
        for ( ; current!=last; current=next )
        {
            next = current+1;

            current->root.index.all  = 0;

            current->next           = next;
            next->pre               = current;

            next->next              = NULL;        // set the last one will do, but put it here
            // Free
            Trunk_unset_use( & next->root );
        }
    }

    return barrel;
}



static YL_BOOL
expand_trunk_barrel_if_needed(void)
{
    if ( free_trunk_tail->pre == free_trunk_head )
    {
        TrunkBarrel     new_barrel      = 0;
        TrunkWrapper    trunk_first     = 0;
        TrunkWrapper    trunk_last      = 0;
        
        YL_MMU_ASSERT( free_trunk_head->next == free_trunk_tail );

        new_barrel = TrunkBarrel_new();

        if ( ! new_barrel ) return 0;

        trunk_first     = FIRST_TRUNK(new_barrel);
        trunk_last      = LAST_TRUNK(new_barrel);

        insert_trunks(free_trunk_tail->pre, trunk_first, trunk_last);

        trunk_barrel_tail->next = new_barrel;
        new_barrel->pre         = trunk_barrel_tail;
        trunk_barrel_tail       = new_barrel;
    }

    return 1;
}


/************************ Trunk Metadata Allocator ***************************/


YL_BOOL
TrunkMetaAlloc_initialize()
{
    TrunkWrapper first          = NULL;
    TrunkWrapper last           = NULL;

    trunk_barrel_head = trunk_barrel_tail = TrunkBarrel_new();

    if ( ! trunk_barrel_head )
    {
        return 0;
    }

    // Create a dummy used trunk
    {
        used_trunk_head             = trunk_barrel_head->trunks;        // [0]
        used_trunk_tail             = trunk_barrel_head->trunks+1;      // [1]
        Trunk_set_use( & used_trunk_head->root );
        Trunk_set_use( & used_trunk_tail->root );
        used_trunk_head->pre        = NULL;
        used_trunk_head->next       = used_trunk_tail;
        used_trunk_tail->pre        = used_trunk_head;
        used_trunk_tail->next       = NULL;
    }

    // Create a dummy free trunk as head
    {

        free_trunk_head             = trunk_barrel_head->trunks + 2;                // [2]
        free_trunk_tail             = trunk_barrel_head->trunks + 3;                // [3]
        first                       = trunk_barrel_head->trunks + 4;
        last                        = trunk_barrel_head->trunks + (BARREL_SIZE - 1);
        // it's not empty, in some aspect
        Trunk_set_use( & free_trunk_head->root );
        Trunk_set_use( & free_trunk_tail->root );
        free_trunk_head->pre        = NULL;
        free_trunk_tail->next       = NULL;
        free_trunk_head->next       = first;
        first->pre                  = free_trunk_head;
        free_trunk_tail->pre        = last;
        last->next                  = free_trunk_tail;
    }

    YL_MMU_ASSERT( used_trunk_head != used_trunk_tail );
    YL_MMU_ASSERT( used_trunk_head != free_trunk_head );
    YL_MMU_ASSERT( free_trunk_head != free_trunk_tail );
    YL_MMU_ASSERT( free_trunk_tail != first );
    YL_MMU_ASSERT( first != last );

    return 1;
}

YL_BOOL
TrunkMetaAlloc_release()
{
    TrunkBarrel t, to_be_freed, tail;

    if ( ! trunk_barrel_head )
    {
        return 0;
    }

    t    = trunk_barrel_head;
    tail = trunk_barrel_tail; // for assertion only.

    // Disable the pointer as first as possible
    trunk_barrel_head = trunk_barrel_tail = NULL;

    while ( t )
    {
        to_be_freed = t;
        t = t->next;
        yl_mmu_free( to_be_freed );
    }

    YL_MMU_ASSERT( to_be_freed == tail );

    return 1;
}


static YL_BOOL
check_and_init_trunk_manager(void)
{
    if ( ! trunk_barrel_head )
    {
        return TrunkMetaAlloc_initialize();
    }
    return 1;
}

Trunk
TrunkMetaAlloc_alloc()
{
    YL_BOOL success = 1;

    success = check_and_init_trunk_manager();
    if ( ! success ) return NULL;

    success = expand_trunk_barrel_if_needed();
    if ( ! success ) return NULL;

    {
        // cut down the last free_trunk
        TrunkWrapper item = cut_next_trunk( free_trunk_head );

        // connect to the used_trunk
        insert_trunk(used_trunk_head, item);
        //item->root.used = 1;

        return (Trunk)item;
    }
}

void
TrunkMetaAlloc_free(Trunk t)
{
    TrunkWrapper item = (TrunkWrapper)t;   // Each trunk is actually contained in a TrunkWrapper w same address.

    {
        // cut down target trunk
        TrunkWrapper t = cut_next_trunk( item->pre );

        // connect to the used_trunk
        insert_trunk(free_trunk_head, t );
        Trunk_unset_use( & t->root );
    }
}

Trunk 
TrunkMetaAlloc_get_empty_head()
{
    YL_BOOL success = 1;

    success = check_and_init_trunk_manager();
    if ( ! success ) return NULL;

    return (Trunk)free_trunk_head;
}

Trunk 
TrunkMetaAlloc_get_used_head()
{
    YL_BOOL success = 1;

    success = check_and_init_trunk_manager();
    if ( ! success ) return NULL;

    return (Trunk)used_trunk_head;
}



#ifdef __cplusplus
}
#endif

