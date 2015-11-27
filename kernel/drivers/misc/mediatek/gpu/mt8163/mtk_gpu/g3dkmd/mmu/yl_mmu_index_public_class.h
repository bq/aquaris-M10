#pragma once
/// \file
/// \brief Helper. Page Index, index0 is for first-level page table index1 is for second-level.

typedef union PageIndex_
{
    unsigned int all;
    struct
    {
        unsigned short index0;
        unsigned short index1;
    } s;

} *PageIndex, PageIndexRec;
