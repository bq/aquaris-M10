#pragma once
/// \file
/// \brief Class Definition. Iterator is a fast way to manipulate the memory block inside an allocation.

#include "yl_mmu_mapper.h"


typedef struct RemapperIterator_
{
    Mapper          m;
    FirstPageTable  Page_space;
    g3d_va_t        g3d_address;
    Trunk           trunk;
    PageIndexRec    index;
    int             size;
    int             cursor;
    int             size_to_next_boundary;
} RemapperIteratorRec, *RemapperIterator;

