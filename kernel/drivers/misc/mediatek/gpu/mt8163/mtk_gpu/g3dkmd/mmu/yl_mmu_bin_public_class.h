#pragma once
/// \file
/// \brief Definition of BinList class

#include "yl_mmu_trunk_public_class.h"



/********************* Data Sturcture *****************/
/// Haven't count the pivot bin.
#define BIN_LENGTH      (MAX_ADDRESSING_BIT - MIN_ADDRESSING_BIT + 1) 
/// Handy shortcut. Last bin before the pivot bin
#define BIN_LAST_INDEX  ( BIN_LENGTH - 1 )

/// A Bin contains an dummy trunk.
/** Reuse the trunk properties for the bin properties 
 *  The design simplifies the insertion/deletion of trunks inside the bin. 
 */
typedef struct Bin_
{
    TrunkRec    trunk_head;     /// circular / doubly-linklist list
    // size_t      size;        /// use the size in trunk_head

} BinRec, *Bin;

/// A BinList categorizes trunks into different bin by their size.
/** Using Pivot.
 */
typedef struct BinList_
{
    // 28 - 12 + 1 + 1 // the last one is pivot
    //BinRec occupied[BIN_LENGTH + 1 ];  // Mindos occupied

    /** 28 - 12 + 1 + 1 
     * the last one is pivot.
     * Trunks inside a bin is stored in doubly-linklist
     */
    BinRec empty[BIN_LENGTH + 1 ]; 

    /** First Trunk is unique. It doesn't belong the bin list. 
      * And should be always the topmost trunk in the entire addressing space.
      * It always empty. i.e. in_use == 0 
      */
    Trunk  first_trunk;

} BinListRec, *BinList;
