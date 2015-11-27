#pragma once
/// \file
/// \brief Memory functions. Not necessary.

#include "yl_mmu_common.h"
#include "yl_mmu_mapper.h"
#include "yl_mmu_iterator.h"

#ifdef __cplusplus
extern "C" {
#endif


YL_BOOL Remapper_memset( Mapper m, g3d_va_t ptr, unsigned char value, size_t size );
YL_BOOL Remapper_memcpy(                       Mapper m, g3d_va_t destination, const g3d_va_t source, size_t num );
YL_BOOL Remapper_memcpy_from_g3d_va(           Mapper m, va_t destination, const g3d_va_t source, size_t num );
YL_BOOL Remapper_memcpy_to_g3d_va(             Mapper m, g3d_va_t destination, const va_t source, size_t num );
YL_BOOL RemapperIterator_memset(               RemapperIterator iter, g3d_va_t ptr, int value, size_t num );
YL_BOOL RemapperIterator_memcpy(               RemapperIterator iter, g3d_va_t destination, const g3d_va_t source, size_t num );
YL_BOOL RemapperIterator_memcpy_from_g3d_va(   RemapperIterator iter, va_t destination, const g3d_va_t source, size_t num );
YL_BOOL RemapperIterator_memcpy_to_g3d_va(     RemapperIterator iter, g3d_va_t destination, const va_t source, size_t num );
int  Remapper_memcmp(                       Mapper m, va_t va, const g3d_va_t gva, size_t size );

#ifdef __cplusplus
}
#endif