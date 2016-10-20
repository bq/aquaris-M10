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

#ifndef _G3DKMD_PM_H_
#define _G3DKMD_PM_H_

#include "g3dkmd_define.h"
#include "gpad/gpad_kmif.h"

#define G3DKMD_PM_ALIGN         32
#define G3DKMD_PM_DATA_SIZE     (10 * 16)	/* PM data byte per frame */
#define G3DKMD_PM_BUF_SIZE      (MAX_HT_COMMAND * G3DKMD_PM_DATA_SIZE)

void g3dKmdKmifAllocMemoryLocked(unsigned int size, unsigned int align);
void g3dKmdKmifFreeMemoryLocked(void);

#endif /* _G3DKMD_PM_H_ */
