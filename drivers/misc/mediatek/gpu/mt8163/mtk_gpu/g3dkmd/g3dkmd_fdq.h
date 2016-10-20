/*
 * Copyright (c) 2014 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _G3DKMD_FDQ_H_
#define _G3DKMD_FDQ_H_

#include "g3dkmd_define.h"
#include "g3dbase_type.h"

#define G3DKMD_FDQ_ALIGN        16
#define G3DKMD_FDQ_ENTRY_SIZE   16
#define G3DKMD_FDQ_BUF_SIZE     (((COMMAND_QUEUE_SIZE/(SIZE_OF_FREG/2)) * G3DKMD_MAX_TASK_CNT) * G3DKMD_FDQ_ENTRY_SIZE)

void g3dKmdFdqInit(void);
void g3dKmdFdqUninit(void);
void g3dKmdFdqUpdateBufPtr(void);
void g3dKmdFdqBackupBufPtr(void);

void g3dKmdFdqReadQ(void);
#ifndef G3D_HW
void g3dKmdFdqWriteQ(int taskID);
#endif

void g3dkmdResetFpsInfo(void);

/* for debugfs access fdq information. */
void *_g3dkmd_getFdqDataPtr(void);
G3Duint _g3dkmd_getFdqDataSize(void);
G3Duint _g3dkmd_getFdqReadIdx(void);
G3Duint _g3dkmd_getFdqWriteIdx(void);

#endif /* _G3DKMD_FDQ_H_ */
