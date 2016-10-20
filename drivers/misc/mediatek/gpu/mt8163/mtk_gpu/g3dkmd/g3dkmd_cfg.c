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

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)
#include <windows.h>
#include <stdlib.h>
#elif defined(linux) && defined(linux_user_mode)	/* linux user space */
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#endif

#include "g3dbase_buffer.h"
#include "g3dkmd_util.h"
#include "g3dkmd_api.h"
#include "g3dkmd_pattern.h"
#include "g3dkmd_cfg.h"

static unsigned int _g3dkmd_cfg[CFG_MAX];

/* This value must be 1GB align, in order to fit WCP IOMMU HW using Mindos's MMU implementation */
#define G3DKMD_MMU_VA_BASE  0x40000000

void g3dKmdCfgInit(void)
{
	memset(_g3dkmd_cfg, 0, sizeof(_g3dkmd_cfg));

	_g3dkmd_cfg[CFG_LTC_ENABLE] = G3DKMD_TRUE;

#if (USE_MMU == MMU_NONE)
	_g3dkmd_cfg[CFG_MMU_ENABLE] = G3DKMD_FALSE;
	_g3dkmd_cfg[CFG_MMU_VABASE] = 0;
#else
	_g3dkmd_cfg[CFG_MMU_ENABLE] = G3DKMD_TRUE;
	_g3dkmd_cfg[CFG_MMU_VABASE] = G3DKMD_MMU_VA_BASE;
#endif

	/* defer default size */
#if defined(linux) && defined(__KERNEL__)
	_g3dkmd_cfg[CFG_DEFER_VARY_SIZE] =
	    (G3DKMD_MAP_VX_VARY_SIZE >
	     KMALLOC_UPPER_BOUND) ? KMALLOC_UPPER_BOUND : G3DKMD_MAP_VX_VARY_SIZE;
	_g3dkmd_cfg[CFG_DEFER_BINHD_SIZE] =
	    (G3DKMD_MAP_VX_BINHD_SIZE >
	     KMALLOC_UPPER_BOUND / 2) ? KMALLOC_UPPER_BOUND / 2 : G3DKMD_MAP_VX_BINHD_SIZE;
	_g3dkmd_cfg[CFG_DEFER_BINBK_SIZE] =
	    (G3DKMD_MAP_VX_BINBK_SIZE >
	     KMALLOC_UPPER_BOUND / 2) ? KMALLOC_UPPER_BOUND / 2 : G3DKMD_MAP_VX_BINBK_SIZE;
	_g3dkmd_cfg[CFG_DEFER_BINTB_SIZE] =
	    (G3DKMD_MAP_VX_BINTB_SIZE >
	     KMALLOC_UPPER_BOUND / 2) ? KMALLOC_UPPER_BOUND / 2 : G3DKMD_MAP_VX_BINTB_SIZE;
	_g3dkmd_cfg[CFG_DUMP_ENABLE] = G3DKMD_FALSE;	/* yiyi // turn off by brady */
	_g3dkmd_cfg[CFG_MEM_SHIFT_SIZE] = 0;
#else
	_g3dkmd_cfg[CFG_DEFER_VARY_SIZE] = G3DKMD_MAP_VX_VARY_SIZE;
	_g3dkmd_cfg[CFG_DEFER_BINHD_SIZE] = G3DKMD_MAP_VX_BINHD_SIZE;
	_g3dkmd_cfg[CFG_DEFER_BINBK_SIZE] = G3DKMD_MAP_VX_BINBK_SIZE;
	_g3dkmd_cfg[CFG_DEFER_BINTB_SIZE] = G3DKMD_MAP_VX_BINTB_SIZE;
	_g3dkmd_cfg[CFG_DUMP_ENABLE] = G3DKMD_TRUE;
	_g3dkmd_cfg[CFG_PHY_HEAP_ENABLE] = G3DKMD_FALSE;
	_g3dkmd_cfg[CFG_MEM_SHIFT_SIZE] = 0;
#endif

	/* defer buffer size checking */
	YL_KMD_ASSERT(((_g3dkmd_cfg[CFG_DEFER_VARY_SIZE] & G3DKMD_VARY_SIZE_ALIGN_MSK) == 0) &&
		(_g3dkmd_cfg[CFG_DEFER_VARY_SIZE] <= G3DKMD_DEFER_SIZE_MAX)); /* 4KB align & max 256MB */
	YL_KMD_ASSERT(((_g3dkmd_cfg[CFG_DEFER_BINBK_SIZE] & G3DKMD_BINBK_SIZE_ALIGN_MSK) == 0) &&
		(_g3dkmd_cfg[CFG_DEFER_BINBK_SIZE] <= G3DKMD_DEFER_SIZE_MAX)); /* 128B align & max 256MB */
	YL_KMD_ASSERT(((_g3dkmd_cfg[CFG_DEFER_BINTB_SIZE] & G3DKMD_BINTB_SIZE_ALIGN_MSK) == 0) &&
		(_g3dkmd_cfg[CFG_DEFER_BINTB_SIZE] <= G3DKMD_DEFER_SIZE_MAX));/* 16B align & max 256MB */

	_g3dkmd_cfg[CFG_DUMP_START_FRM] = 0;
	_g3dkmd_cfg[CFG_DUMP_END_FRM] = 0;
#ifdef PATTERN_DUMP
#ifdef FPGA_G3D_HW
	_g3dkmd_cfg[CFG_DUMP_CE_MODE] = G3DKMD_DUMP_CE_MODE_PATTERN;
#else
	_g3dkmd_cfg[CFG_DUMP_CE_MODE] = G3DKMD_DUMP_CE_MODE_DRIVER;
#endif
#endif

#ifdef USE_GCE
	_g3dkmd_cfg[CFG_GCE_ENABLE] = G3DKMD_FALSE;
#else
	_g3dkmd_cfg[CFG_GCE_ENABLE] = G3DKMD_FALSE;
#endif

	_g3dkmd_cfg[CFG_VX_BS_HD_TEST] = 0;

	/* preempt at triangle boundary */
	_g3dkmd_cfg[CFG_CQ_PMT_MD] = 1;
	_g3dkmd_cfg[CFG_BKJMP_EN] = 2;

	/* debug usage for ux reg */
	_g3dkmd_cfg[CFG_UXCMM_REG_DBG_EN] = 0x1;
	_g3dkmd_cfg[CFG_UXDWP_REG_ERR_DETECT] = 0x1;
}

void g3dKmdCfgSet(enum CFG_ENUM cfg, unsigned int value)
{
	YL_KMD_ASSERT(cfg < CFG_MAX);

	switch (cfg) {
	case CFG_DEFER_VARY_SIZE:
		value = (value
			 && (value <=
			     G3DKMD_MAP_VX_VARY_SIZE)) ? value : _g3dkmd_cfg[CFG_DEFER_VARY_SIZE];
		value = G3DKMD_VARY_SIZE_ALIGNED(value);
		break;

	case CFG_DEFER_BINHD_SIZE:
		value = (value
			 && (value <=
			     G3DKMD_MAP_VX_BINHD_SIZE)) ? value : _g3dkmd_cfg[CFG_DEFER_BINHD_SIZE];
		value = G3DKMD_BINHD_SIZE_ALIGNED(value);
		break;

	case CFG_DEFER_BINBK_SIZE:
		value = (value
			 && (value <=
			     G3DKMD_MAP_VX_BINBK_SIZE)) ? value : _g3dkmd_cfg[CFG_DEFER_BINBK_SIZE];
		value = G3DKMD_BINBK_SIZE_ALIGNED(value);
		break;

	case CFG_DEFER_BINTB_SIZE:
		value = (value
			 && (value <=
			     G3DKMD_MAP_VX_BINTB_SIZE)) ? value : _g3dkmd_cfg[CFG_DEFER_BINTB_SIZE];
		value = G3DKMD_BINTB_SIZE_ALIGNED(value);
		break;

	case CFG_MEM_SHIFT_SIZE:
		/* if too big, set to 0. */
		value =
		    (value <= G3DKMD_MAP_MEM_SHIFT_SIZE) ? value : _g3dkmd_cfg[CFG_MEM_SHIFT_SIZE];
		break;

#if (USE_MMU == MMU_NONE)
	case CFG_MMU_ENABLE:
		value = 0;
		break;
#endif

	case CFG_MMU_VABASE:
		YL_KMD_ASSERT((value & (G3DKMD_MMU_VA_BASE - 1)) == 0);	/* to make sure it's 1GB align */
		value &= (~(G3DKMD_MMU_VA_BASE - 1));
		break;

#ifndef USE_GCE
	case CFG_GCE_ENABLE:
		value = 0;
		break;
#endif

	default:
		break;
	}

	_g3dkmd_cfg[cfg] = value;
}

unsigned int g3dKmdCfgGet(enum CFG_ENUM cfg)
{
	YL_KMD_ASSERT(cfg < CFG_MAX);
	return _g3dkmd_cfg[cfg];
}
