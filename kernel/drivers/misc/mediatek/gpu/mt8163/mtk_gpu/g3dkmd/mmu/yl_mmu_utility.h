#ifndef _G3DKMD_MMU_U_
#define _G3DKMD_MMU_U_

#include "yl_mmu.h"
#include "dump/yl_mmu_utility_sanity_trunk.h"

//#include "list\linkList.h"

#ifdef __cplusplus
extern "C" {
#endif

int mmuMapperValid( Mapper m );
Mapper mmuInit( g3d_va_t g3d_va_base );
void mmuUninit( Mapper m );
void mmuGetTable(Mapper m, void** swAddr, void** hwAddr, unsigned int* size);
void mmuGetMirrorTable(Mapper m, void** swAddr, void** hwAddr, unsigned int* size);
unsigned int mmuG3dVaToG3dVa(Mapper m, g3d_va_t g3d_va);
unsigned int mmuG3dVaToPa(Mapper m, g3d_va_t g3d_va);
void* mmuGetMapper(void);

#ifdef __cplusplus
}
#endif

#endif //_G3DKMD_MMU_U_






