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

#ifndef _G3DBASE_BUFFER_H_
#define _G3DBASE_BUFFER_H_

#include "g3dbase_common_define.h"


#ifdef CMODEL
#define G3DKMD_MAP_VX_BINHD_SIZE            (8*16384)
#define G3DKMD_MAP_VX_BINTB_SIZE            (512*1024)
#define G3DKMD_MAP_VX_BINBK_SIZE            (8*1024*1024)
#define G3DKMD_MAP_VX_VARY_SIZE             (255*1024*1024) /* 28 */
#else
#define G3DKMD_MAP_VX_BINHD_SIZE            (8*16384)
#define G3DKMD_MAP_VX_BINTB_SIZE            (300*1024)
#define G3DKMD_MAP_VX_BINBK_SIZE            (2*1024*1024)
#define G3DKMD_MAP_VX_VARY_SIZE             (2*1024*1024) /* (40*1024*1024) to fit Hover */
#endif /* CMODEL */

#define G3DKMD_DEFER_BASE_ALIGN             (0x80)
#define G3DKMD_DEFER_BASE_ALIGN_MSK         (G3DKMD_DEFER_BASE_ALIGN - 1)

#define G3DKMD_VARY_SIZE_ALIGN              (0x1000)
#define G3DKMD_VARY_SIZE_ALIGN_MSK          (G3DKMD_VARY_SIZE_ALIGN - 1)
#define G3DKMD_VARY_SIZE_MIN                (1024)

#define G3DKMD_BINBK_SIZE_ALIGN             (0x80)
#define G3DKMD_BINBK_SIZE_ALIGN_MSK         (G3DKMD_BINBK_SIZE_ALIGN - 1)
#define G3DKMD_BINBK_SIZE_MIN               (1024)

#define G3DKMD_BINHD_SIZE_ALIGN             (0x80)
#define G3DKMD_BINHD_SIZE_ALIGN_MSK         (G3DKMD_BINHD_SIZE_ALIGN - 1)
#define G3DKMD_BINHD_SIZE_MIN               (1024)

#define G3DKMD_BINTB_SIZE_ALIGN             (0x10)
#define G3DKMD_BINTB_SIZE_ALIGN_MSK         (G3DKMD_BINTB_SIZE_ALIGN - 1)
#define G3DKMD_BINTB_SIZE_MIN               (1024)

#define G3DKMD_DEFER_SIZE_MAX               (256 * 1024 * 1024)

#define G3DKMD_VARY_SIZE_ALIGNED(s)         (((s) + G3DKMD_VARY_SIZE_ALIGN_MSK)  & (~G3DKMD_VARY_SIZE_ALIGN_MSK))
#define G3DKMD_BINBK_SIZE_ALIGNED(s)        (((s) + G3DKMD_BINBK_SIZE_ALIGN_MSK) & (~G3DKMD_BINBK_SIZE_ALIGN_MSK))
#define G3DKMD_BINHD_SIZE_ALIGNED(s)        (((s) + G3DKMD_BINHD_SIZE_ALIGN_MSK) & (~G3DKMD_BINHD_SIZE_ALIGN_MSK))
#define G3DKMD_BINTB_SIZE_ALIGNED(s)        (((s) + G3DKMD_BINTB_SIZE_ALIGN_MSK) & (~G3DKMD_BINTB_SIZE_ALIGN_MSK))

/* in MBytes */
#define G3DKMD_MAP_MEM_SHIFT_SIZE			(2*1024)

#endif /* _G3DBASE_BUFFER_H_ */
