#ifndef _G3DKMD_ENGINE_H_
#define _G3DKMD_ENGINE_H_
#include "g3dkmd_define.h"
#include "g3dkmd_task.h"

//function declare
typedef enum
{
    G3DKMD_REG_MFG,
    G3DKMD_REG_G3D,
    G3DKMD_REG_UX_CMN,
    G3DKMD_REG_UX_DWP,
    G3DKMD_REG_MX,
    G3DKMD_REG_IOMMU,
#ifdef G3DKMD_SUPPORT_EXT_IOMMU
    G3DKMD_REG_IOMMU2,
#endif
} G3DKMD_MOD;
unsigned int g3dKmdRegRead(G3DKMD_MOD mod, unsigned int reg, unsigned int mask, unsigned int sft);
void g3dKmdRegWrite(G3DKMD_MOD mod, unsigned int reg, unsigned int value, unsigned int mask, unsigned int sft);



unsigned int g3dKmdEngLTCFlush(void);
void g3dKmdRegReset(void);
void g3dKmdRegResetCmdQ(void);
unsigned int g3dKmdQLoad(pG3dExeInst pInst);
void g3dKmdRegSendCommand(int dispatchHwIndex, pG3dExeInst pInst);
void g3dKmdRegRepeatCommand(int dispatchHwIndex, pG3dExeInst pInst, unsigned int readPtr);
unsigned int g3dKmdRegGetReadPtr(int dispatchHwIndex);

#ifdef USE_GCE
unsigned int g3dKmdCheckBufferConsistency(int dispatchHwIndex, unsigned int bufferStartIn, unsigned int bufferSizeIn);
#endif

const unsigned char* g3dKmdRegGetG3DRegister(void);
const unsigned char* g3dKmdRegGetUxDwpRegister(void);
#ifndef G3D_HW
void g3dKmdRegUpdateReadPtr(int dispatchHwIndex, unsigned int read_ptr);
void g3dKmdRegUpdateWritePtr(int dispatchHwIndex, unsigned int write_ptr);
#endif

void g3dKmdRegRestartHW(int taskID, int mode);
int g3dCheckDeviceId(void);
void g3dKmdCacheFlushInvalidate(void);
unsigned int g3dKmdCheckHwIdle(void);

#ifdef G3DKMD_MEM_CHECK 
void* g3dkmdmalloc(unsigned int size, const char * file, const char *func, unsigned int nline);
void  g3dkmdfree(void* p, const char * file, const char *func, unsigned int nline);
void* g3dkmdvmalloc(unsigned int size, const char * file, const char *func, unsigned int nline);
void  g3dkmdvfree(void* p, const char * file, const char *func, unsigned int nline);
#endif

#endif //_G3DKMD_ENGINE_H_
