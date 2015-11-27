#ifndef _G3DKMD_INTERNAL_MEMORY_H_
#define _G3DKMD_INTERNAL_MEMORY_H_
#include "g3d_memory.h"

#ifdef __cplusplus 
extern "C"
{
#endif 

struct G3dMemory_;

/// kmd only
int allocatePhysicalMemory( struct G3dMemory_ *memory, unsigned int size, unsigned int baseAlign, unsigned int flag);
void freePhysicalMemory( struct G3dMemory_ *memory);


#ifdef __cplusplus 
}
#endif

#endif //_G3DKMD_INTERNAL_MEMORY_H_
