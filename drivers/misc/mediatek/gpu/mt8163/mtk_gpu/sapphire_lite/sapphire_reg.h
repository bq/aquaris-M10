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

#ifndef _SAPPHIRE_REG_H_
#define _SAPPHIRE_REG_H_

#include "yl_define.h"

#ifdef FPGA_G3D_HW
extern unsigned long __yl_base;
extern unsigned long __iommu_base;
#define YL_BASE              __yl_base
#else
#define YL_BASE              0x00000000
#endif

#define MFG_MOD_OFST        (0x0000)
#define G3D_MOD_OFST        (0x1000)
#define UX_CMN_MOD_OFST     (0x2000)
#define UX_DWP_MOD_OFST     (0x3000)
#define MX_MOD_OFST         (0x4000)
#define IOMMU_MOD_OFST      (0x5000)
#define MMCE_MOD_OFST       (0x6000)
#define IOMMU2_MOD_OFST     (0x7000)

#define MFG_BASE            YL_BASE
#define MFG_REG_BASE        (MFG_BASE + MFG_MOD_OFST)
#define G3D_REG_BASE        (MFG_BASE + G3D_MOD_OFST)
#define UX_CMN_REG_BASE     (MFG_BASE + UX_CMN_MOD_OFST)
#define UX_DWP_REG_BASE     (MFG_BASE + UX_DWP_MOD_OFST)
#define MX_REG_BASE         (MFG_BASE + MX_MOD_OFST)
#ifdef FPGA_G3D_HW
#define IOMMU_REG_BASE      __iommu_base
#else
#define IOMMU_REG_BASE      (MFG_BASE + IOMMU_MOD_OFST)
#endif
#define MMCE_REG_BASE       (MFG_BASE + MMCE_MOD_OFST)
#define IOMMU2_REG_BASE     (MFG_BASE + IOMMU2_MOD_OFST)

#define MFG_REG_MAX         0x100
#define G3D_REG_MAX         0x1000
#define UX_CMN_REG_MAX      0x100
#define UX_DWP_REG_MAX      0x100
#define MX_REG_MAX          0x1000
#define IOMMU_REG_MAX       0x1000
#define MMCE_REG_MAX        0x180
#define IOMMU2_REG_MAX      0x1000

#include "sapphire_areg.h"
#include "sapphire_ht.h"
#include "sapphire_rsm.h"
#include "sapphire_qreg.h"
#include "sapphire_ux_cmn.h"
#include "sapphire_ux_dwp.h"
#include "sapphire_mx_dbg.h"

#ifndef G3DKMD_BUILTIN_KERNEL
#include "sapphire_freg.h"
#include "sapphire_vp.h"
#include "sapphire_vp_vl_pkt.h"
#include "sapphire_pp_cb.h"
#include "sapphire_pp_cpb.h"
#include "sapphire_pp_cpf.h"
#include "sapphire_pp_fs.h"
#include "sapphire_pp_xy.h"
#include "sapphire_pp_zpb.h"
#include "sapphire_pp_zpf.h"
#include "sapphire_tx.h"
#include "sapphire_cmodel.h"
#endif

#endif /* _SAPPHIRE_REG_H_ */
