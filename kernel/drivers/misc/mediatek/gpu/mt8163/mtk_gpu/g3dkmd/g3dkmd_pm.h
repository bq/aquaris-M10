#ifndef _G3DKMD_PM_H_
#define _G3DKMD_PM_H_

#include "g3dkmd_define.h"
#include "gpad/gpad_kmif.h"

#define G3DKMD_PM_ALIGN         32
#define G3DKMD_PM_DATA_SIZE     (10 * 16)   // PM data byte per frame
#define G3DKMD_PM_BUF_SIZE      (MAX_HT_COMMAND * G3DKMD_PM_DATA_SIZE)

void g3dKmdKmifAllocMemoryLocked(unsigned int size, unsigned int align);
void g3dKmdKmifFreeMemoryLocked(void);

#endif //_G3DKMD_PM_H_
