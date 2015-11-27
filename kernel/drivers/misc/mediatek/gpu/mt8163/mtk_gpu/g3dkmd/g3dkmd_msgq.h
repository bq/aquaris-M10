#ifndef _G3DKMD_MSGQ_H_
#define _G3DKMD_MSGQ_H_

#include "g3dkmd_api_common.h"

void g3dKmdMsgQInit(void);
void g3dKmdMsgQUninit(void);

G3DKMD_MSGQ_RTN g3dKmdMsgQSend(G3DKMD_MSGQ_TYPE type, unsigned int size, void* buf);

G3DKMD_APICALL G3DKMD_MSGQ_RTN G3DAPIENTRY g3dKmdMsgQGetMsgInfo(G3DKMD_MSGQ_TYPE* type, unsigned int* size);


#if (defined(linux) && !defined(linux_user_mode) && defined(__KERNEL__))
G3DKMD_APICALL G3DKMD_MSGQ_RTN G3DAPIENTRY g3dKmdMsgQReceive(G3DKMD_MSGQ_TYPE* type, unsigned int *size, void __user* buf);
#else
G3DKMD_APICALL G3DKMD_MSGQ_RTN G3DAPIENTRY g3dKmdMsgQReceive(G3DKMD_MSGQ_TYPE* type, unsigned int *size, void* buf);
#endif

void g3dKmdMsgQSetReg(unsigned int reg, unsigned int value);
void g3dKmdMsgQAllocMem(unsigned int hwAddr, uint64_t phyAddr, unsigned int size);
void g3dKmdMsgQFreeMem(unsigned int hwAddr, unsigned int size);

#endif //_G3DKMD_MSGQ_H_
