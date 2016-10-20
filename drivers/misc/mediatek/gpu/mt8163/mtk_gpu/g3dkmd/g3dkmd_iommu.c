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
#include "g3dkmd_task.h"
#include "g3dkmd_iommu.h"
#include "g3dkmd_iommu_reg.h"
#include "g3dkmd_macro.h"
#include "g3dkmd_cfg.h"
#include "g3dkmd_util.h"
#include "g3dkmd_pattern.h"
#include "g3dkmd_engine.h"

#ifdef G3DKMD_SUPPORT_IOMMU

static iommu_regwrite_callback _regCbkFunc;

static void _g3dKmdIommuRegWrite(unsigned int hw_mod, unsigned int reg, unsigned int value,
						unsigned int mask, unsigned int sft)
{
	_DEFINE_BYPASS_;
	unsigned int dumpRiu = G3DKMD_FALSE;

	dumpRiu = !g3dKmdIsBypass(BYPASS_RIU);
	_BYPASS_(BYPASS_RIU);

	g3dKmdRegWrite(hw_mod, reg, value, mask, sft);

	_RESTORE_();

	if (dumpRiu && _regCbkFunc) {
		unsigned int write_val = g3dKmdRegRead(hw_mod, reg, ~mask, 0) | ((value << sft) & mask);
		unsigned int reg_base = 0;

		switch (hw_mod) {
		case G3DKMD_REG_IOMMU:
			reg_base = IOMMU_MOD_OFST;
			break;

#ifdef G3DKMD_SUPPORT_EXT_IOMMU
		case G3DKMD_REG_IOMMU2:
			reg_base = IOMMU2_MOD_OFST;
			break;
#endif
		case G3DKMD_REG_MFG:
			reg_base = MFG_MOD_OFST;
			break;

		default:
			YL_KMD_ASSERT(0);
			break;
		}
		_regCbkFunc(reg_base + reg, write_val);
	}
}

static unsigned int _g3dKmdIommuRegRead(unsigned int hw_mod, unsigned int reg, unsigned int mask,
							unsigned int sft)
{
	return g3dKmdRegRead(hw_mod, reg, mask, sft);
}

static void _g3dKmdIommuInvalidTlb(unsigned int mod, unsigned int isInvAll, unsigned int mva_start,
						unsigned int mva_end)
{
	unsigned int value = (0x1ul << SFT_INVLD_EN0);
	unsigned int hw_mod = G3DKMD_REG_IOMMU + mod;

	if (!g3dKmdCfgGet(CFG_MMU_ENABLE))
		return;

	/* enable invalid channel (L1/L2) */
#ifdef G3DKMD_SUPPORT_IOMMU_L2
	value |= (0x1ul << SFT_INVLD_EN1);
#endif
	_g3dKmdIommuRegWrite(hw_mod, REG_MMU_INV_SEL, value, 0xFFFFFFFF, 0);

	mva_start &= MSK_RANGE_IVD_START_ADDR;
	mva_end &= MSK_RANGE_IVD_END_ADDR;
	/* YL_KMD_ASSERT((mva_start & (~MSK_RANGE_IVD_START_ADDR)) == 0); */
	/* YL_KMD_ASSERT((mva_end & (~MSK_RANGE_IVD_END_ADDR)) == 0); */

	if (mva_start >= mva_end)
		isInvAll = 1;

	/* invalidate */
	if (isInvAll) {
		_g3dKmdIommuRegWrite(hw_mod, REG_MMU_INVALIDATE, 0x1, MSK_ALL_INVLD, SFT_ALL_INVLD);
	} else {
		_g3dKmdIommuRegWrite(hw_mod, REG_MMU_INVLD_START_A, mva_start,
				     MSK_RANGE_IVD_START_ADDR, 0);
		_g3dKmdIommuRegWrite(hw_mod, REG_MMU_INVLD_END_A, mva_end, MSK_RANGE_IVD_END_ADDR,
				     0);
		_g3dKmdIommuRegWrite(hw_mod, REG_MMU_INVALIDATE, 0x1, MSK_RANGE_INVLD,
				     SFT_RANGE_INVLD);
	}

	/* wait invalidate done */
#if defined(CMODEL) || defined(G3D_HW)
#ifdef G3DKMD_SUPPORT_IOMMU_L2
	if (!isInvAll) {
#ifdef CMODEL
		_DEFINE_BYPASS_;

		_BYPASS_(BYPASS_RIU);
		_g3dKmdIommuRegWrite(hw_mod, REG_MMU_CPE_DONE, 0x1, MSK_RANGE_INVAL_DONE,
				     SFT_RANGE_INVAL_DONE);
		_RESTORE_();
#endif

		do {
			value = _g3dKmdIommuRegRead(hw_mod, REG_MMU_CPE_DONE, MSK_RANGE_INVAL_DONE,
						    SFT_RANGE_INVAL_DONE);
		} while (value == 0);

#ifdef PATTERN_DUMP
		if (!g3dKmdIsBypass(BYPASS_RIU)) {
			g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfff8, MSK_RANGE_INVAL_DONE);
			g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n",
				       MOD_OFST(IOMMU, REG_MMU_CPE_DONE), MSK_RANGE_INVAL_DONE);
		}
#endif

		_g3dKmdIommuRegWrite(hw_mod, REG_MMU_CPE_DONE, 0x0, MSK_RANGE_INVAL_DONE,
				     SFT_RANGE_INVAL_DONE);
	}
#endif
#endif

	_g3dKmdIommuRegWrite(hw_mod, REG_MMU_INVALIDATE, 0x0, 0xFFFFFFFF, 0);
}

void g3dKmdIommuInvalidTlbAll(unsigned int mod)
{
	if (mod >= IOMMU_MOD_NS)
		return;

	_g3dKmdIommuInvalidTlb(mod, 1, 0, 0);
}

void g3dKmdIommuInvalidTlbRange(unsigned int mod, unsigned int mva_start, unsigned mva_end)
{
	if (mod >= IOMMU_MOD_NS)
		return;

	_g3dKmdIommuInvalidTlb(mod, 0, mva_start, mva_end);
}

void g3dKmdIommuInit(unsigned int mod, unsigned int page_table_addr)
{
	unsigned int hw_mod = G3DKMD_REG_IOMMU + mod;

	if (!g3dKmdCfgGet(CFG_MMU_ENABLE))
		return;

	if (mod >= IOMMU_MOD_NS)
		return;

	YL_KMD_ASSERT((page_table_addr & (~MSK_BASE_ADDR)) == 0);

	/* enable DCM for low poer mode */
	_g3dKmdIommuRegWrite(hw_mod, REG_MMU_DCM_DIS, 0, 0xFFFFFFFF, 0);

	/* set page table address */
	_g3dKmdIommuRegWrite(hw_mod, REG_MMU_PT_BASE_ADDR, page_table_addr, MSK_BASE_ADDR, 0);
	_g3dKmdIommuRegWrite(hw_mod, REG_MMU_PT_BASE_ADDR_SEC, page_table_addr, MSK_BASE_ADDR_SEC,
			     0);

	/* enable ISR */
	_g3dKmdIommuRegWrite(hw_mod, REG_MMU_INT_CONTROL0, 0x7F, 0xFFFFFFFF, 0);
	_g3dKmdIommuRegWrite(hw_mod, REG_MMU_INT_CONTROL1, 0x7FF, 0xFFFFFFFF, 0);

	/* disable ultra command promotion when RS FULL (MSK_RS_FULL_ULTRA_EN) */
	_g3dKmdIommuRegWrite(hw_mod, REG_MMU_PRIORITY, (MSK_TABLE_WALK_ULTRA | MSK_AUTO_PF_ULTRA),
			     (MSK_TABLE_WALK_ULTRA | MSK_AUTO_PF_ULTRA), 0);

	/* enable coherence */
	/* _g3dKmdIommuRegWrite(hw_mod, REG_MMU_CTRL_REG, 0x1, MSK_COHERENCE_EN, SFT_COHERENCE_EN); */

	/* disable bypass */
	/*
	_g3dKmdIommuRegWrite(hw_mod, REG_MMU_WR_LEN, 0x0, MSK_WRITE_THROTTLING_DIS,
				SFT_WRITE_THROTTLING_DIS);
	*/

	/* g3dKmdIommuInvalidTlbAll(hw_mod); */

	/* register 0x18 : control if IOMMU HW enabled or bypass */
	/* 1 : enabled (this is default) */
	/* 0 : bypass */
	_g3dKmdIommuRegWrite(G3DKMD_REG_MFG, 0x18, 0x1, 0xFFFFFFFF, 0);
}

void g3dKmdIommuDisable(unsigned int mod)
{
	unsigned int hw_mod = G3DKMD_REG_IOMMU + mod;

	if (mod >= IOMMU_MOD_NS)
		return;

	/* disable ultra command promotion when RS FULL (MSK_RS_FULL_ULTRA_EN) */
	_g3dKmdIommuRegWrite(hw_mod, REG_MMU_PRIORITY, (MSK_TABLE_WALK_ULTRA | MSK_AUTO_PF_ULTRA),
				(MSK_TABLE_WALK_ULTRA | MSK_AUTO_PF_ULTRA), 0);
}

#ifdef G3DKMD_SNAPSHOT_PATTERN
unsigned int g3dKmdIommuGetPtBase(unsigned int mod)
{
	unsigned int hw_mod = G3DKMD_REG_IOMMU + mod;
	unsigned temp = _g3dKmdIommuRegRead(hw_mod, REG_MMU_PT_BASE_ADDR, MSK_BASE_ADDR, SFT_BASE_ADDR);

	return temp << SFT_BASE_ADDR;
}
#endif

void g3dKmdIommuRegCallback(iommu_regwrite_callback cb)
{
	_regCbkFunc = cb;
}

#if 0
void g3dKmdIommuBypass(unsigned int mod)
{
	REG_FUNC *func = &reg_func[mod];

	if (mod >= IOMMU_MOD_NS)
		return;

	/* bypass */
	func->reg_write32_msk_sft(REG_MMU_WR_LEN, 0x1, MSK_WRITE_THROTTLING_DIS,
				  SFT_WRITE_THROTTLING_DIS);
}
#endif
#endif /* G3DKMD_SUPPORT_IOMMU */
