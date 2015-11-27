#pragma once
/// \file
/// \brief Internal.  Link related trunk helper functions.

#include "yl_mmu_trunk_public_class.h"
#include "yl_mmu_trunk_internal.h"
#include "yl_mmu_hw_table_helper.h"



#ifdef __cplusplus
extern "C" {
#endif

static INLINE void Trunk_link_reset( Trunk target ) __attribute__((always_inline));

static INLINE void _Trunk_insert_before(Trunk current, Trunk new_one ) __attribute__((always_inline));

static INLINE void _Trunk_insert_after(Trunk current, Trunk new_one ) __attribute__((always_inline));

static INLINE void _Trunk_detach_second(Trunk pre, Trunk target ) __attribute__((always_inline));

static INLINE void _Trunk_detach_second_and_third( Trunk first, Trunk second, Trunk third ) __attribute__((always_inline));

static INLINE void Trunk_siblings_reset( Trunk target ) __attribute__((always_inline));

static INLINE void Trunk_siblings_connect( Trunk left, Trunk right ) __attribute__((always_inline));

static INLINE void Trunk_siblings_insert_after( Trunk anchor, Trunk new_one ) __attribute__((always_inline));

static INLINE void _Trunk_siblings_detach( Trunk target ) __attribute__((always_inline));


static INLINE void
Trunk_link_reset( Trunk target )
{
    target->pre    = NULL;
    target->next   = NULL;
}

static INLINE void 
_Trunk_insert_before(Trunk current, Trunk new_one )
{
    Trunk prev = current->pre;

    // you can't insert before head.
    // You will ruin the datastructure.
    YL_MMU_ASSERT( ! Trunk_is_head( current ) ); 
    YL_MMU_ASSERT( current != new_one  );
    YL_MMU_ASSERT( prev    != NULL     );
    YL_MMU_ASSERT( current != NULL     );
    YL_MMU_ASSERT( new_one != NULL     );

    prev->next      = new_one;
    new_one->pre    = prev;

    new_one->next   = current;
    current->pre    = new_one;
}

static INLINE void 
_Trunk_insert_after(Trunk current, Trunk new_one )
{
    Trunk next = current->next;

    YL_MMU_ASSERT( current != new_one  );
    YL_MMU_ASSERT( current != NULL     );
    YL_MMU_ASSERT( new_one != NULL     );

    current->next   = new_one;
    new_one->pre    = current;

    new_one->next   = next;
    next->pre       = new_one;
}

static INLINE void 
_Trunk_detach_second(Trunk pre, Trunk target )
{
    Trunk next  = target->next;

    pre->next   = next;
    next->pre   = pre;

    // For safty
    Trunk_link_reset( target );   // not necessary
}


static INLINE void 
_Trunk_detach_second_and_third( Trunk first, Trunk second, Trunk third )
{
    Trunk forth;

    YL_MMU_ASSERT( first != NULL );
    YL_MMU_ASSERT( second != NULL );
    YL_MMU_ASSERT( third != NULL );

    forth       = third->next;
    first->next = forth;
    forth->pre  = first;

    // For safty
    Trunk_link_reset( second  );   // not necessary
    Trunk_link_reset( third   );   // not necessary
}

static INLINE void
Trunk_siblings_reset( Trunk target )
{
    target->left_sibling    = NULL;
    target->right_sibling   = NULL;
}

static INLINE void
Trunk_siblings_connect( Trunk left, Trunk right )
{
    YL_MMU_ASSERT( left != NULL    );
    YL_MMU_ASSERT( right != NULL   );

    left->right_sibling = right;
    right->left_sibling = left;
}

static INLINE void
Trunk_siblings_insert_after( Trunk anchor, Trunk new_one )
{
    Trunk next = anchor->right_sibling;

    YL_MMU_ASSERT( anchor  != NULL     );
    YL_MMU_ASSERT( new_one != NULL     );
    YL_MMU_ASSERT( next    != NULL     );

    Trunk_siblings_connect( anchor, new_one );
    Trunk_siblings_connect( new_one, next   );
}

static INLINE void 
_Trunk_siblings_detach( Trunk target )
{
    Trunk left;
    Trunk right;

    YL_MMU_ASSERT( target                          );
    YL_MMU_ASSERT( ! Trunk_is_pivot_trunk(target)  ); // something wrong if you want to detach a trunk actually a bin
    YL_MMU_ASSERT( ! Trunk_is_pivot_bin(target)    ); // something wrong if you want to detach a trunk actually a bin

    left  = target->left_sibling;
    right = target->right_sibling;

    YL_MMU_ASSERT( left != NULL     );
    YL_MMU_ASSERT( right != NULL    );

    Trunk_siblings_connect( left, right );
    Trunk_siblings_reset( target );
}

#ifdef __cplusplus
}
#endif
