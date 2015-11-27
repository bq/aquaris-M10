#ifndef _G3DKMD_FAKEMEM_H_

#define _G3DKMD_FAKEMEM_H_



extern unsigned int mmAllocByVa(unsigned int uSize, void* va);

extern unsigned int mmAlloc(unsigned int uSize);

extern void mmFree(unsigned int uAddr,unsigned int uSize);







#endif

