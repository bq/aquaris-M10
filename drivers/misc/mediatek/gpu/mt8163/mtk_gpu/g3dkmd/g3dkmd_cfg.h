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

#ifndef _G3DKMD_CFG_H_
#define _G3DKMD_CFG_H_

enum CFG_ENUM {
	CFG_LTC_ENABLE,

	CFG_MMU_ENABLE,
	CFG_MMU_VABASE,

	CFG_DEFER_VARY_SIZE,
	CFG_DEFER_BINHD_SIZE,
	CFG_DEFER_BINBK_SIZE,
	CFG_DEFER_BINTB_SIZE,

	CFG_DUMP_ENABLE,
	CFG_DUMP_START_FRM,
	CFG_DUMP_END_FRM,
	CFG_DUMP_CE_MODE,

	CFG_GCE_ENABLE,

	CFG_PHY_HEAP_ENABLE, /* the way physical address generation (for pattern dump) */

	CFG_MEM_SHIFT_SIZE, /* in MB */

	CFG_VX_BS_HD_TEST,

	CFG_CQ_PMT_MD,

	CFG_BKJMP_EN,

	CFG_SECURE_ENABLE,

	CFG_UXCMM_REG_DBG_EN,

	CFG_UXDWP_REG_ERR_DETECT,

	CFG_MAX,
};

void g3dKmdCfgInit(void);
void g3dKmdCfgSet(enum CFG_ENUM, unsigned int value);
unsigned int g3dKmdCfgGet(enum CFG_ENUM);

#endif /* _G3DKMD_CFG_H_ */
