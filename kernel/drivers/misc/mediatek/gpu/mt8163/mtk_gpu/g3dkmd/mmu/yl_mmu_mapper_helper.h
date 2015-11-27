#pragma once
/// \file
/// \brief Mapper is the main object of this module.

#include "yl_mmu_hw_table.h"

/*
 * We don't inline anything for the public API in case we make it as DLL or .so
 *
 */


#ifdef __cplusplus
extern "C" {
#endif

#define Mapper_get_section_length   SECTION_LENGTH
#define Mapper_get_small_length     SMALL_LENGTH

//size_t Mapper_get_required_size_all(void)           __attribute__((const));
size_t Mapper_get_required_size_mapper(void)        __attribute__((const));
size_t Mapper_get_required_size_page_table(void)    __attribute__((const));
size_t Mapper_get_required_size_meta_table(void)    __attribute__((const));

/**************** Implememtation ***************/


#ifdef __cplusplus
}
#endif

