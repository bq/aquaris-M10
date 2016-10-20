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

#ifndef _G3DKMD_BUFFER_H_
#define _G3DKMD_BUFFER_H_

#include "g3dkmd_define.h"
#include "g3dkmd_task.h"
#include "g3dkmd_api.h"

void g3dKmdQueryBuffSizesFromInst(const struct g3dExeInst *pInst, G3DKMD_DEV_REG_PARAM *param);

int g3dKmdCreateBuffers(struct g3dExeInst *pInst, G3DKMD_DEV_REG_PARAM *param);

void g3dKmdDestroyBuffers(struct g3dExeInst *pInst);

#endif /* _G3DKMD_BUFFER_H_ */
