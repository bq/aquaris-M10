#pragma once
/// \file
/// \brief Allocation functions.  Need to change when port to kernel mode module.

/**
 * A wrapper for real resource allocation from system.
 * 
 * malloc / free / zeromemory can't used in kernal, should be replace by other functions.
 * 
 */

#include "yl_mmu_common.h"

#ifdef __cplusplus
extern "C" {
#endif


/// Should be replaced by valloc()
void * yl_mmu_alloc( size_t size );

/// Should be replaced by free()
void yl_mmu_free( void * p );

/// Should be re-written manually
void yl_mmu_zeromem( void * p, size_t size);

/// Should be re-written manually
void yl_mmu_memset( void * p, unsigned char value, size_t size);

/// Should be re-written manually
void yl_mmu_memcpy( void * destination, const void * source, size_t size );

/// Should be re-written manually
int yl_mmu_memcmp(const void *s1, const void *s2, size_t n);



#ifdef __cplusplus
}
#endif