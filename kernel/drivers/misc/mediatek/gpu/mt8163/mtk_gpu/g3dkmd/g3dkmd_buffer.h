#pragma once

#include "g3dkmd_define.h"
#include "g3dkmd_task.h"
#include "g3dkmd_api.h"

void g3dKmdQueryBuffSizesFromInst(const g3dExeInst* pInst, G3DKMD_DEV_REG_PARAM* param);

int g3dKmdCreateBuffers(g3dExeInst* pInst, G3DKMD_DEV_REG_PARAM* param);

void g3dKmdDestroyBuffers(g3dExeInst* pInst);

