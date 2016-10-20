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

#ifndef _G3DKMD_IOMMU_H_
#define _G3DKMD_IOMMU_H_

#include "g3dkmd_define.h"

enum {
	IOMMU_MOD_1 = 0,
#ifdef G3DKMD_SUPPORT_EXT_IOMMU
	IOMMU_MOD_2,
#endif
	IOMMU_MOD_NS
};

typedef void (*iommu_regwrite_callback) (unsigned int reg, unsigned int value);

void g3dKmdIommuInit(unsigned int mod, unsigned int page_table_addr);
void g3dKmdIommuInvalidTlbAll(unsigned int mod);
void g3dKmdIommuInvalidTlbRange(unsigned int mod, unsigned int mva_start, unsigned mva_end);
/* void g3dKmdIommuBypass(unsigned int mod); */
void g3dKmdIommuRegCallback(iommu_regwrite_callback cb);

void g3dKmdIommuDisable(unsigned int mod);

#ifdef G3DKMD_SNAPSHOT_PATTERN
unsigned int g3dKmdIommuGetPtBase(unsigned int mod);
#endif

#endif /* _G3DKMD_IOMMU_H_ */
