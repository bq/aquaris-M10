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

#if defined(_WIN32) && defined(G3DKMD_EXPORTS) \
	|| (defined(linux) && defined(linux_user_mode)) /* linux user space */
#if defined(_WIN32) && defined(G3DKMD_EXPORTS)
#include <Windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#else
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/memory.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/slab.h> /* kmalloc() */

#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <asm/uaccess.h> /* set_fs */
#include <linux/file.h> /* fput */
#include <linux/delay.h>
#include <linux/io.h>

#ifdef FPGA_G3D_HW
#ifdef CONFIG_ARM64
#define outer_flush_all()
#else
#include <asm/cacheflush.h>
#endif
unsigned long __yl_base;/* due to 6752 use device_tree, so we need a variable to know register virtual address */
unsigned long __iommu_base;
#endif
#endif

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dkmd_cfg.h"
#include "g3dkmd_util.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_macro.h"
#include "g3dkmd_iommu.h"
#include "g3dkmd_mmce.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_pattern.h"
#include "g3dkmd_isr.h"		/* for Suspend/resuem Isr. */
#include "g3dkmd_recovery.h"
#include "test/tester_non_polling_flushcmd.h"	/* for tester_fence_block() */

#include "test/tester.h"
#include "platform/g3dkmd_platform_waitqueue.h"


#if defined(G3D_HW) && defined(G3DKMD_SUPPORT_ISR)
#include <linux/interrupt.h>
#include "g3dkmd_isr.h"
#endif

#ifdef G3DKMD_SUPPORT_FDQ
#include "g3dkmd_fdq.h"
#endif

#if (USE_MMU == MMU_USING_WCP)
#if defined(FPGA_mt6752_fpga) || defined(FPGA_fpga8163) || defined(FPGA_fpga8163_64) || defined(MT8163)
#include <m4u.h>
#define GPU_IOMMU_ID 2
#elif defined(FPGA_mt6592_fpga)
#include "mach/gpu_mmu.h"
#define GPU_IOMMU_ID
#else
#error "currently mt6752/mt6592 FPGA are supported only"
#endif
#endif

#define G3DKMD_ENGINE_TIMEOUT_CNT   500000

#ifdef ANDROID_HT_COUNT
#include <linux/ftrace_event.h>
#endif
static __DEFINE_SPIN_LOCK(resetlock);
/* __DEFINE_SPIN_LOCK(regLock); */


REG_ACCESS_DATA_IMP(MFG)
REG_ACCESS_FUNC_IMP(MFG)

REG_ACCESS_DATA_IMP(G3D)
REG_ACCESS_FUNC_IMP(G3D)
/* UX_DWP needs to be considered in Android */
REG_ACCESS_DATA_IMP(UX_DWP)
REG_ACCESS_FUNC_IMP(UX_DWP)

REG_ACCESS_DATA_IMP(MX)
REG_ACCESS_FUNC_IMP(MX)

REG_ACCESS_DATA_IMP(IOMMU)
REG_ACCESS_FUNC_IMP(IOMMU)
#ifdef G3DKMD_SUPPORT_EXT_IOMMU
REG_ACCESS_DATA_IMP(IOMMU2)
REG_ACCESS_FUNC_IMP(IOMMU2)
#endif
REG_ACCESS_DATA_IMP(UX_CMN)
REG_ACCESS_FUNC_IMP(UX_CMN)
#define TIME_OUT_CNT 1000
enum G3DKMD_TOKEN {
	G3DKMD_TOKEN_0 = REG_AREG_G3D_TOKEN0,
	G3DKMD_TOKEN_1 = REG_AREG_G3D_TOKEN1,
	G3DKMD_TOKEN_2 = REG_AREG_G3D_TOKEN2,
	G3DKMD_TOKEN_3 = REG_AREG_G3D_TOKEN3
};

static unsigned int _g3dKmdGetToken(enum G3DKMD_TOKEN token)
{
	unsigned int cnt = 0;

	while (cnt++ < TIME_OUT_CNT) {
		if (g3dKmdRegRead(G3DKMD_REG_G3D, token, MSK_AREG_G3D_TOKEN0, SFT_AREG_G3D_TOKEN0))
			break;

		G3DKMD_UDELAY(1);
	}

	if (cnt >= TIME_OUT_CNT)
		return G3DKMD_FALSE;

	return G3DKMD_TRUE;
}

static void _g3dKmdReleaseToken(enum G3DKMD_TOKEN token)
{
	g3dKmdRegWrite(G3DKMD_REG_G3D, token, 0x1, MSK_AREG_G3D_TOKEN0, SFT_AREG_G3D_TOKEN0);
}

static void _g3dKmdDataSync(struct g3dExeInst *pInstIn);
static void _g3dKmdDataSync(struct g3dExeInst *pInstIn)
{
	int i;
	struct g3dExeInst *pInst;

	if (pInstIn->dataSynced == G3DKMD_FALSE) {
#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "DataBarrier", 1);
		met_tag_start(0x8163, "DataBarrier-T");
#endif

#ifdef G3D_NONCACHE_BUFFERS
		G3DKMD_BARRIER();
#else
		g3dKmdCacheFlushInvalidate();
#endif

#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_end(0x8163, "DataBarrier-T");
		met_tag_oneshot(0x8163, "DataBarrier", 0);
#endif

		for (i = 0; i < gExecInfo.exeInfoCount; i++) {
			pInst = gExecInfo.exeList[i];
			if (pInst)
				pInst->dataSynced = G3DKMD_TRUE;
		}
	}
}

unsigned int g3dKmdRegGetReadPtr(int dispatchHwIndex)
{
	unsigned int hwreadptr;

	/* for FPGA20188 to refine this option */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x0, MSK_AREG_MCU2G3D_SET, SFT_AREG_MCU2G3D_SET);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x1, MSK_AREG_MCU2G3D_SET, SFT_AREG_MCU2G3D_SET);

	hwreadptr = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_CQ_DONE_HTBUF_RPTR,
					MSK_AREG_CQ_DONE_HTBUF_RPTR, SFT_AREG_CQ_DONE_HTBUF_RPTR);

	return hwreadptr;
}


#ifdef USE_GCE
unsigned int g3dKmdCheckBufferConsistency(int hwChannel, unsigned int bufferStartIn, unsigned int bufferSizeIn)
{
	unsigned int bufferStart = 0;
	unsigned int bufferEnd = 0;

	if (hwChannel >= NUM_HW_CMD_QUEUE)
		return 0;


	bufferStart = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_CQ_HTBUF_START_ADDR,
					MSK_AREG_CQ_HTBUF_START_ADDR, SFT_AREG_CQ_HTBUF_START_ADDR);
	bufferEnd = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_CQ_HTBUF_END_ADDR, MSK_AREG_CQ_HTBUF_END_ADDR,
					SFT_AREG_CQ_HTBUF_END_ADDR);

	if ((bufferStart != bufferStartIn) || (bufferEnd != (bufferStart + bufferSizeIn)))
		return 0;


	return 1;
}
#endif

const unsigned char *g3dKmdRegGetG3DRegister(void)
{
/* force hw refresh all the status to ARegs. */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x0, MSK_AREG_MCU2G3D_SET,
			SFT_AREG_MCU2G3D_SET);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x1, MSK_AREG_MCU2G3D_SET,
			SFT_AREG_MCU2G3D_SET);

	return (unsigned char *)REG_BASE(G3D);
}

const unsigned char *g3dKmdRegGetUxDwpRegister(void)
{
	return (unsigned char *)REG_BASE(UX_DWP);
}

#ifndef G3D_HW
void g3dKmdRegUpdateReadPtr(int hwChannel, unsigned int read_ptr)
{
	_DEFINE_BYPASS_;

	if (hwChannel >= NUM_HW_CMD_QUEUE)
		return;


	_BYPASS_(BYPASS_RIU);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_DONE_HTBUF_RPTR, read_ptr,
			MSK_AREG_CQ_DONE_HTBUF_RPTR, SFT_AREG_CQ_DONE_HTBUF_RPTR);
	_RESTORE_();
}

void g3dKmdRegUpdateWritePtr(int hwChannel, unsigned int write_ptr)
{
	_DEFINE_BYPASS_;

	if (hwChannel >= NUM_HW_CMD_QUEUE)
		return;


	_BYPASS_(BYPASS_RIU);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_HTBUF_WPTR, write_ptr, MSK_AREG_CQ_HTBUF_WPTR,
			SFT_AREG_CQ_HTBUF_WPTR);
	_RESTORE_();
}
#endif

unsigned int g3dKmdEngLTCFlush(void)
{
	unsigned int cnt, rtnVal = G3DKMD_FALSE;

	/* get token */
	if (!_g3dKmdGetToken(G3DKMD_TOKEN_0))
		return rtnVal;

	/* enable LTC-flush */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FLUSH_LTC_SET, 0x1, MSK_AREG_PA_FLUSH_LTC_EN,
		       SFT_AREG_PA_FLUSH_LTC_EN);

	/* wait LTC-flush done */
	cnt = 0;
	while (cnt++ < TIME_OUT_CNT) {
		if (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FLUSH_LTC_DONE, MSK_AREG_PA_FLUSH_LTC_DONE,
				  SFT_AREG_PA_FLUSH_LTC_DONE))
			break;

		G3DKMD_UDELAY(1);
	}

	if (cnt >= TIME_OUT_CNT)
		goto FLUSH_EXIT;

	/* disable LTC-flush */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FLUSH_LTC_SET, 0x0, MSK_AREG_PA_FLUSH_LTC_EN,
		       SFT_AREG_PA_FLUSH_LTC_EN);

	/* wait LTC-disable done */
	cnt = 0;
	while (cnt++ < TIME_OUT_CNT) {
		if (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_FLUSH_LTC_DONE, MSK_AREG_PA_FLUSH_LTC_DONE,
				  SFT_AREG_PA_FLUSH_LTC_DONE) == 0)
			break;

		G3DKMD_UDELAY(1);
	}

	if (cnt >= TIME_OUT_CNT)
		goto FLUSH_EXIT;


	rtnVal = G3DKMD_TRUE;

FLUSH_EXIT:

	/* release token */
	_g3dKmdReleaseToken(G3DKMD_TOKEN_0);

	return G3DKMD_TRUE;
}

unsigned int g3dKmdRegRead(enum G3DKMD_MOD mod, unsigned int reg, unsigned int mask, unsigned int sft)
{
	unsigned int rtnVal;

	/* __SPIN_LOCK_BH(regLock); */

	switch (mod) {
	case G3DKMD_REG_MFG:
		rtnVal = MFG_REG_READ32_MSK_SFT(reg, mask, sft);
		break;

	case G3DKMD_REG_G3D:
		rtnVal = G3D_REG_READ32_MSK_SFT(reg, mask, sft);
		break;

	case G3DKMD_REG_UX_CMN:
		rtnVal = UX_CMN_REG_READ32_MSK_SFT(reg, mask, sft);
		break;

	case G3DKMD_REG_UX_DWP:
		rtnVal = UX_DWP_REG_READ32_MSK_SFT(reg, mask, sft);
		break;

	case G3DKMD_REG_MX:
		rtnVal = MX_REG_READ32_MSK_SFT(reg, mask, sft);
		break;

	case G3DKMD_REG_IOMMU:
		rtnVal = IOMMU_REG_READ32_MSK_SFT(reg, mask, sft);
		break;

#ifdef G3DKMD_SUPPORT_EXT_IOMMU
	case G3DKMD_REG_IOMMU2:
		rtnVal = IOMMU2_REG_READ32_MSK_SFT(reg, mask, sft);
		break;
#endif

	default:
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ENGINE, "mod(0x%x) not implement yet\n", mod);
		rtnVal = 0;
		break;
	}

	/*
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "RegRead (%d, r 0x%x, v 0x%x, m 0x%x, s 0x%x)\n",
		mod, reg, rtnVal, mask, sft);
	*/
	/* __SPIN_UNLOCK_BH(regLock); */

	return rtnVal;
}

void g3dKmdRegWrite(enum G3DKMD_MOD mod, unsigned int reg, unsigned int value, unsigned int mask, unsigned int sft)
{
	/* __SPIN_LOCK_BH(regLock); */

	/*
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "RegWrite (%d, r 0x%x, v 0x%x, m 0x%x, s 0x%x)\n",
		mod, reg, value, mask, sft);
	*/

	switch (mod) {
	case G3DKMD_REG_MFG:
		MFG_REG_WRITE32_MSK_SFT(reg, value, mask, sft);
		break;

	case G3DKMD_REG_G3D:
		G3D_REG_WRITE32_MSK_SFT(reg, value, mask, sft);
		break;

	case G3DKMD_REG_UX_CMN:
		UX_CMN_REG_WRITE32_MSK_SFT(reg, value, mask, sft);
		break;

	case G3DKMD_REG_UX_DWP:
		UX_DWP_REG_WRITE32_MSK_SFT(reg, value, mask, sft);
		break;

	case G3DKMD_REG_MX:
		MX_REG_WRITE32_MSK_SFT(reg, value, mask, sft);
		break;

	case G3DKMD_REG_IOMMU:
		IOMMU_REG_WRITE32_MSK_SFT(reg, value, mask, sft);
		break;

#ifdef G3DKMD_SUPPORT_EXT_IOMMU
	case G3DKMD_REG_IOMMU2:
		IOMMU2_REG_WRITE32_MSK_SFT(reg, value, mask, sft);
		break;
#endif

	default:
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ENGINE, "mod(0x%x) not implement yet\n", mod);
		break;
	}

	/* __SPIN_UNLOCK_BH(regLock); */
}

/*  */
void g3dKmdRegReset(void)
{
#ifndef G3D_HW
	_DEFINE_BYPASS_;

#endif
	unsigned int cnt = 0;

	__SPIN_LOCK_BH(resetlock);

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "in\n");

#if (USE_MMU == MMU_USING_WCP)
	if (g3dKmdCfgGet(CFG_MMU_ENABLE))
		m4u_reg_backup(GPU_IOMMU_ID);

#endif

	/*
	 * SW workaround for HW reset bug [rst_prot module bug]
	 * Due to 0x13005068 return 0xa82 (EMI bus is busy), it lead to timeout in waiting MFG_idle and g3d_idle loop.
	 * To solve this problem by setting blocking MX transaction to close the input of MX module,
	 * then waiting for MX module to finish rest of task.
	 * Reset g3d engine after MX module finishing works.
	 * 1. Force clock enable
	 */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_DBG_CTRL, 1, MSK_AREG_DBG_CLK_EN, SFT_AREG_DBG_CLK_EN);
	g3dKmdRegWrite(G3DKMD_REG_MX, REG_MX_DBG_DEBUG_CTRL, 1, MSK_MX_DBG_MX_APB_DBG_CK_ON,
		       SFT_MX_DBG_MX_APB_DBG_CK_ON);
	/* 2. Enable blocking MX transaction */
	g3dKmdRegWrite(G3DKMD_REG_MX, REG_MX_DBG_RADDR, 0x3, MSK_MX_DBG_MX_DBG_RADDR, SFT_MX_DBG_MX_DBG_RADDR);
	g3dKmdRegWrite(G3DKMD_REG_MX, REG_MX_DBG_WADDR, 0x3, MSK_MX_DBG_MX_DBG_WADDR, SFT_MX_DBG_MX_DBG_WADDR);
	g3dKmdRegWrite(G3DKMD_REG_MX, REG_MX_DBG_WDATA, 0x08000000, MSK_MX_DBG_MX_DBG_WDATA, SFT_MX_DBG_MX_DBG_WDATA);
	g3dKmdRegWrite(G3DKMD_REG_MX, REG_MX_DBG_MON_BRK_CTRL, 0x40000000, 0xFFFFFFFF, 0x0);
	/* 3. Check blocked */
	cnt = 0;
#ifndef G3D_HW
	_BYPASS_(BYPASS_RIU);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, VAL_IRQ_MX_DBG, VAL_IRQ_MX_DBG, 0);
	g3dKmdRegWrite(G3DKMD_REG_MFG, 0x1c, 0x1, 0x1, 0x0);
	_RESTORE_();
#endif
	while (cnt++ < TIME_OUT_CNT) {
		if ((g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, VAL_IRQ_MX_DBG, 0)
			|| (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, VAL_IRQ_G3D_HANG, 0)))
			&& g3dKmdRegRead(G3DKMD_REG_MFG, 0x1c, 0x1, 0)
			&& g3dKmdRegRead(G3DKMD_REG_MFG, 0x50, 0x2, 0)) {
			/* 4. Clear IRQ */
			g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 1, 0x400, 10);
			break;
		}
		G3DKMD_UDELAY(1);
	}
	if (cnt >= TIME_OUT_CNT) {
#if 0
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ENGINE,
		       "TIMEOUT mx_debug: 0x%x MFG(0x1c): 0x%x\n",
		       g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 0x400, 10),
		       g3dKmdRegRead(G3DKMD_REG_MFG, 0x1c, 0x1, 0));
		YL_KMD_ASSERT(0);
		return;
#else
	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE, "TIMEOUT IRQ: 0x%x MFG(0x1c): 0x%x MFG(0x50): 0x%x\n",
			g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 0xFFFFFFFF, 0),
			g3dKmdRegRead(G3DKMD_REG_MFG, 0x1c, 0x1, 0), g3dKmdRegRead(G3DKMD_REG_MFG, 0x50, 0x2, 0));
#endif
	}
	/* SW workaround end */

	/* reset g3d engine */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_SWRST, 1, MSK_AREG_G3D_SWRST, SFT_AREG_G3D_SWRST);

	cnt = 0;
	while (cnt++ < TIME_OUT_CNT) {
#ifndef G3D_HW
		_BYPASS_(BYPASS_RIU);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS0,
			       VAL_PA_IDLE_ST0_G3D_ALL_IDLE, VAL_PA_IDLE_ST0_G3D_ALL_IDLE, 0);
		g3dKmdRegWrite(G3DKMD_REG_MFG, 0x50, 0x1, 0x2, 0x1);
		_RESTORE_();
#endif
		/* check if MFG idle && g3d_idle (REG_AREG_PA_IDLE_STATUS0 bit[0]) */
		if (g3dKmdRegRead(G3DKMD_REG_MFG, 0x50, 0x2, 0x1) &&
		    g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS0, VAL_PA_IDLE_ST0_G3D_ALL_IDLE, 0)) {
			break;
		}

		G3DKMD_UDELAY(1);
	}

#ifdef PATTERN_DUMP
	if (!g3dKmdIsBypass(BYPASS_RIU)) {
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfff8, 0x02);
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", MOD_OFST(MFG, 0x50), 0x02);

		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfff8, VAL_PA_IDLE_ST0_G3D_ALL_IDLE);
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n",
			       MOD_OFST(G3D, REG_AREG_PA_IDLE_STATUS0),
			       VAL_PA_IDLE_ST0_G3D_ALL_IDLE);
	}
#endif

	/* SW workaround for HW reset bug */
	/* 5. Release MX blocking */
	g3dKmdRegWrite(G3DKMD_REG_MX, REG_MX_DBG_MON_BRK_CTRL, 0x0, 0xFFFFFFFF, 0x0);
	/* 6. Release forced clock */
	g3dKmdRegWrite(G3DKMD_REG_MX, REG_MX_DBG_DEBUG_CTRL, 0, MSK_MX_DBG_MX_APB_DBG_CK_ON,
		       SFT_MX_DBG_MX_APB_DBG_CK_ON);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_DBG_CTRL, 0, MSK_AREG_DBG_CLK_EN, SFT_AREG_DBG_CLK_EN);
	/* SW workaround end */

	/* reset APB interface */
	/* g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0, 0xFFFFFFFF, 0); */
	/* g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_ENG_FIRE, 0x1, MSK_AREG_G3D_ENG_FIRE, SFT_AREG_G3D_ENG_FIRE); */

	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, MSK_AREG_IRQ_RAW, MSK_AREG_IRQ_RAW,
		       SFT_AREG_IRQ_RAW);
	g3dKmdRegWrite(G3DKMD_REG_UX_CMN, REG_UX_CMN_DEBUG_ENABLE, g3dKmdCfgGet(CFG_UXCMM_REG_DBG_EN), 0xFFFFFFFF, 0);
	g3dKmdRegWrite(G3DKMD_REG_UX_DWP, REG_UX_DWP_ERROR_DETECTION,
			g3dKmdCfgGet(CFG_UXDWP_REG_ERR_DETECT), 0xFFFFFFFF, 0);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_SWRST, 0, MSK_AREG_G3D_SWRST, SFT_AREG_G3D_SWRST);
	/* make sure that HW is true idle */
	G3DKMD_MDELAY(1);

#if (USE_MMU == MMU_USING_WCP)
	if (g3dKmdCfgGet(CFG_MMU_ENABLE))
		m4u_reg_restore(GPU_IOMMU_ID);

#endif

	/* check if reset HW ok or not */
	if (cnt >= TIME_OUT_CNT) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ENGINE, "SWRST timeout\n");
		YL_KMD_ASSERT(0);
		return;
	}
#ifdef PATTERN_DUMP
	/* when pattern dump for HW sim, we have to enable frame-swap/ frame-end interrupt */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASK, 0,
		       (VAL_IRQ_PA_FRAME_SWAP | VAL_IRQ_PA_FRAME_END | VAL_IRQ_UX_DWP0_RESPONSE |
			VAL_IRQ_UX_DWP1_RESPONSE), 0);
#endif

	/* [debug] enable ecc for robert for further debugging */
#ifdef G3DKMD_SUPPORT_ECC
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0x1, MSK_AREG_CQ_ECC_EN, SFT_AREG_CQ_ECC_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASK, 0, VAL_IRQ_CQ_ECC, 0);
#endif

#ifdef G3DKMD_RECOVERY_BY_IRQ
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASK, 0, VAL_IRQ_G3D_HANG, 0);
#endif

#ifdef FPGA_G3D_HW
	/* for FPGA20188 to disable C */
#ifndef G3DKMD_ENABLE_CG
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SETTING, 0, 0xFFFFFFFF, 0);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING0, 0, MSK_AREG_ECLK_CGC0_EN,
		       SFT_AREG_ECLK_CGC0_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING1, 0, MSK_AREG_ECLK_CGC1_EN,
		       SFT_AREG_ECLK_CGC1_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING2, 0, MSK_AREG_ECLK_CGC2_EN,
		       SFT_AREG_ECLK_CGC2_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING3, 0, MSK_AREG_ECLK_CGC3_EN,
		       SFT_AREG_ECLK_CGC3_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING4, 0, MSK_AREG_ECLK_CGC4_EN,
		       SFT_AREG_ECLK_CGC4_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING5, 0, MSK_AREG_ECLK_CGC5_EN,
		       SFT_AREG_ECLK_CGC5_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING6, 0, MSK_AREG_ECLK_CGC6_EN,
		       SFT_AREG_ECLK_CGC6_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING7, 0, MSK_AREG_ECLK_CGC7_EN,
		       SFT_AREG_ECLK_CGC7_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING8, 0, MSK_AREG_ECLK_CGC8_EN,
		       SFT_AREG_ECLK_CGC8_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING9, 0, MSK_AREG_ECLK_CGC9_EN,
		       SFT_AREG_ECLK_CGC9_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING10, 0, MSK_AREG_ECLK_CGC10_EN,
		       SFT_AREG_ECLK_CGC10_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING11, 0, MSK_AREG_ECLK_CGC11_EN,
		       SFT_AREG_ECLK_CGC11_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING12, 0, MSK_AREG_ECLK_CGC12_EN,
		       SFT_AREG_ECLK_CGC12_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING13, 0, MSK_AREG_ECLK_CGC13_EN,
		       SFT_AREG_ECLK_CGC13_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING14, 0, MSK_AREG_ECLK_CGC14_EN,
		       SFT_AREG_ECLK_CGC14_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_CGC_SUB_SETTING15, 0, MSK_AREG_ECLK_CGC15_EN,
		       SFT_AREG_ECLK_CGC15_EN);
#endif

	/* enable +0x80000000 dram address offset for G3D access DRAM like CPU */
	g3dKmdRegWrite(G3DKMD_REG_G3D, 0x600, 1, 1, 0);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASK,
			0, VAL_IRQ_G3D_MMCE_FRAME_FLUSH | VAL_IRQ_UX_CMN_RESPONSE, 0); /* for qload notify */
#endif

	if (g3dKmdCfgGet(CFG_LTC_ENABLE)) {
#ifdef G3DKMD_SINGLE_COMMAND_FOR_PARSER
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_BYPASS_SET, 0xA, 0xFFFFFFFF, 0);
#else /* !G3DKMD_SINGLE_COMMAND_FOR_PARSER */

#ifdef G3DKMD_LTC_IN_ORDER_MODE
		/* [HW BUG] LTC interleave mode will cause HW hang up */
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_BYPASS_SET, 0x0,
			       MSK_AREG_MX_AXI_INTERLEAVE_EN, SFT_AREG_MX_AXI_INTERLEAVE_EN);
#else /* ! G3DKMD_LTC_IN_ORDER_MODE */
		/* set performance mode */
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_BYPASS_SET, 0x1,
			       MSK_AREG_MX_AXI_INTERLEAVE_EN, SFT_AREG_MX_AXI_INTERLEAVE_EN);
#endif /* G3DKMD_LTC_IN_ORDER_MODE */

#endif /* G3DKMD_SINGLE_COMMAND_FOR_PARSER */

	} else {
		/* bypass LTC */
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_BYPASS_SET, 0x1,
			       MSK_AREG_MX_PSEUDOBYPASS_EN, SFT_AREG_MX_PSEUDOBYPASS_EN);
	}

#ifdef G3DKMD_SUPPORT_IOMMU
	/* register 0x18 : control if IOMMU HW enabled or bypass */
	/* 1 : enabled (this is default) */
	/* 0 : bypass */
	g3dKmdRegWrite(G3DKMD_REG_MFG, 0x18, g3dKmdCfgGet(CFG_MMU_ENABLE), 0xFFFFFFFF, 0);
#endif

#ifdef G3DKMD_SUPPORT_PM
	if (gExecInfo.pm.hwAddr != 0) {
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PM_DRAM_START_ADDR, gExecInfo.pm.hwAddr,
			       MSK_AREG_PM_DRAM_START_ADDR, SFT_AREG_PM_DRAM_START_ADDR);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PM_DRAM_SIZE, gExecInfo.pm.size, 0xFFFFFFFF,
			       0);
	}
#endif


#ifdef CMODEL
	/* set default value to below registers, this is only needed for CMODEL mode */
	/* CMODEL will get areg from G3DKMD, to do some calculation */
	/* But for real HW, it will initially to be the default value, we don't need to set them */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_DEBUG_MOD_SEL, 0x0, 0xFFFFFFFF, 0);

	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_VX_VTX_SETTING, 0x200, 0xFFFFFFFF, 0);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_VX_VTX_SETTING, VX_VW_BUF_MGN_SIZE,
		       MSK_AREG_VX_VW_BUF_MGN, SFT_AREG_VX_VW_BUF_MGN);

	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_VX_DEFER_SETTING, 0x10080800, 0xFFFFFFFF, 0);
#endif

	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_VX_VTX_SETTING, g3dKmdCfgGet(CFG_VX_BS_HD_TEST),
		       MSK_AREG_VX_BS_HD_TEST, SFT_AREG_VX_BS_HD_TEST);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_VX_VTX_SETTING, g3dKmdCfgGet(CFG_BKJMP_EN),
		       MSK_AREG_VX_BKJMP_EN, SFT_AREG_VX_BKJMP_EN);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_VX_VTX_SETTING, 0x0, MSK_AREG_VX_BS_BOFF_MODE,
		       SFT_AREG_VX_BS_BOFF_MODE);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CONTEXT_CTRL, g3dKmdCfgGet(CFG_CQ_PMT_MD),
		       MSK_AREG_CQ_PMT_MD, SFT_AREG_CQ_PMT_MD);

	g3dKmdRegWrite(G3DKMD_REG_UX_DWP, REG_UX_DWP_ERROR_DETECTION, 0x0, MSK_UX_DWP_FTZ_RND_ORDER,
		       SFT_UX_DWP_FTZ_RND_ORDER);

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "out\n");
	__SPIN_UNLOCK_BH(resetlock);
}

void g3dKmdRegResetCmdQ(void)
{
#ifndef G3D_HW
	_DEFINE_BYPASS_;

#endif
	unsigned int cnt = 0;

	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x0, MSK_AREG_MCU2G3D_SET,
		       SFT_AREG_MCU2G3D_SET);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x1, MSK_AREG_MCU2G3D_SET,
		       SFT_AREG_MCU2G3D_SET);

	/* wait engine idle */
	while (cnt++ < TIME_OUT_CNT) {
#ifndef G3D_HW
		_BYPASS_(BYPASS_RIU);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS0,
			       VAL_PA_IDLE_ST0_G3D_ALL_IDLE, VAL_PA_IDLE_ST0_G3D_ALL_IDLE, 0);
		_RESTORE_();
#endif
		if (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS0, VAL_PA_IDLE_ST0_G3D_ALL_IDLE, 0))
			break;


		G3DKMD_UDELAY(1);
	}

	if (cnt >= TIME_OUT_CNT) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ENGINE, "Wait idle fail\n");
		YL_KMD_ASSERT(0);
		return;
	}
	/* trigger cmdQ reset */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0x1, MSK_AREG_CQ_RST, SFT_AREG_CQ_RST);

	/* engine fire */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_ENG_FIRE, 0x1, MSK_AREG_G3D_ENG_FIRE, SFT_AREG_G3D_ENG_FIRE);

	/* disable cmdQ reset */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0x0, MSK_AREG_CQ_RST, SFT_AREG_CQ_RST);
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "out\n");
}

#ifdef YL_SECURE
static INLINE void _qload_set_secure(void)
{
	/* [brady todo] */
	/* set qbuffer security range */
	if (g3dKmdCfgGet(CFG_SECURE_ENABLE)) {
		unsigned int value;

		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0x1, MSK_AREG_CQ_SECURITY,
			       SFT_AREG_CQ_SECURITY);

		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MX_SECURE0_BASE_ADDR, 0x1,
			       MSK_AREG_MX_SECURE0_ENABLE, SFT_AREG_MX_SECURE0_ENABLE);

		value = G3DKMD_FLOOR(pInst->qbaseBuffer.hwAddr, 0x1000) >> 12;
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MX_SECURE0_BASE_ADDR, value,
			       MSK_AREG_MX_SECURE0_BASE_ADDR, SFT_AREG_MX_SECURE0_BASE_ADDR);

		value = G3DKMD_CEIL(pInst->qbaseBuffer.hwAddr + pInst->qbaseBufferSize, 0x1000) >> 12;
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MX_SECURE0_TOP_ADDR, value,
			       MSK_AREG_MX_SECURE0_TOP_ADDR, SFT_AREG_MX_SECURE0_TOP_ADDR);
	} else {
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0x0, MSK_AREG_CQ_SECURITY,
			       SFT_AREG_CQ_SECURITY);
	}
}
#else
#define _qload_set_secure()
#endif

#ifdef G3DKMD_SUPPORT_SYNC_SW
static INLINE void _qload_sync_sw(void)
{
	while (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, VAL_IRQ_PA_SYNC_SW, 5) != 0) {
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "VAL_IRQ_PA_SYNC_SW\n");

		/* check whether pa_flush_ltc_done (areg 0x194) [1] is on */
		while (g3dKmdRegRead
		       (G3DKMD_REG_G3D, REG_AREG_PA_FLUSH_LTC_DONE, MSK_AREG_PA_SYNC_SW_DONE,
			SFT_AREG_PA_SYNC_SW_DONE) != 0) {
			/* set pa_flush_ltc_set (areg 0x190)[1] on */
			g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FLUSH_LTC_SET, 1,
				       MSK_AREG_PA_SYNC_SW_EN, SFT_AREG_PA_SYNC_SW_EN);
		}
		/* set pa_flush_ltc_set [1] off */
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_PA_FLUSH_LTC_SET, 0, MSK_AREG_PA_SYNC_SW_EN,
			       SFT_AREG_PA_SYNC_SW_EN);

		/* clear all isq */
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 1, VAL_IRQ_PA_SYNC_SW, 5);
	}
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "VAL_IRQ_PA_SYNC_SW out\n");
}
#else
#define _qload_sync_sw()
#endif

#ifdef G3D_HANG_TEST_NULL_QLOAD
static unsigned int HANG_DEBUG(void)
{
	static int dbg_cnt;

	if ((++dbg_cnt) % 7 == 0) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ENGINE, "Bad Qload!!!\n");
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_QBUF_START_ADDR, 0,
			       MSK_AREG_CQ_QBUF_START_ADDR, SFT_AREG_CQ_QBUF_START_ADDR);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_QBUF_END_ADDR, 0,
			       MSK_AREG_CQ_QBUF_END_ADDR, SFT_AREG_CQ_QBUF_END_ADDR);
		return G3DKMD_TRUE;
	}
	return G3DKMD_FALSE;
}
#else
#define HANG_DEBUG() G3DKMD_FALSE
#endif

static INLINE void _qload_set_qload_regs(struct g3dExeInst *pInst)
{

	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x0, MSK_AREG_MCU2G3D_SET,
		       SFT_AREG_MCU2G3D_SET);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x1, MSK_AREG_MCU2G3D_SET,
		       SFT_AREG_MCU2G3D_SET);

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE, "IDLE0 0x%x, IDLE1 0x%x, RAW 0x%x\n",
	       g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS0, 0xFFFFFFFF, 0),
	       g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS1, 0xFFFFFFFF, 0),
	       g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 0xFFFFFFFF, 0));

	/* set qbase buffer address */
	if (!HANG_DEBUG()) {
		KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_ENGINE, "qbase g3dva 0x%x, pa 0x%x!!!\n",
		       pInst->qbaseBuffer.hwAddr, pInst->qbaseBuffer.patPhyAddr);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_QBUF_START_ADDR,
			       pInst->qbaseBuffer.hwAddr, MSK_AREG_CQ_QBUF_START_ADDR,
			       SFT_AREG_CQ_QBUF_START_ADDR);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_QBUF_END_ADDR,
			       pInst->qbaseBuffer.hwAddr + pInst->qbaseBufferSize,
			       MSK_AREG_CQ_QBUF_END_ADDR, SFT_AREG_CQ_QBUF_END_ADDR);
	}
	/* trigger qld upd */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0x1, MSK_AREG_CQ_QLD_UPD,
		       SFT_AREG_CQ_QLD_UPD);

	/* enable clk (fire if may triggered by GCE) // for FPGA20188 to refine this option */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_ENG_FIRE, 0x1, MSK_AREG_G3D_ENG_FIRE,
		       SFT_AREG_G3D_ENG_FIRE);

	/* disable qld upd */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0x0, MSK_AREG_CQ_QLD_UPD,
		       SFT_AREG_CQ_QLD_UPD);

#ifdef PATTERN_DUMP
	if (!g3dKmdIsBypass(BYPASS_RIU)) {
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfff8, VAL_IRQ_G3D_MMCE_FRAME_FLUSH);
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n",
			       MOD_OFST(G3D, REG_AREG_IRQ_STATUS_RAW),
			       VAL_IRQ_G3D_MMCE_FRAME_FLUSH);
	}
#ifndef G3D_HW
	{
		_DEFINE_BYPASS_;

		_BYPASS_(BYPASS_RIU);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 0x1,
			       VAL_IRQ_G3D_MMCE_FRAME_FLUSH, 7);
		g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASKED, 0x1,
			       VAL_IRQ_G3D_MMCE_FRAME_FLUSH, 7);
		_RESTORE_();

		g3dKmdIsrTrigger();
	}
#endif /* G3D_HW */
#endif
}


#if defined(ENABLE_QLOAD_NON_POLLING)
static INLINE G3DKMD_BOOL _qload_wait_hw_qload_non_polling(void)
{
	int wqRet = 1;		/* default is normal exit. */
	G3DKMD_BOOL is_timeout = G3DKMD_FALSE;

	if (!g3dKmdIsBypass(BYPASS_REG)) {
#ifdef FPGA_G3D_HW
		YL_KMD_ASSERT(!in_interrupt());
#endif /* FPGA_G3D_HW */

		KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE,
			   "QLoad wait queue: going to wait.\n");

#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_oneshot(0x8163, "WAIT-FF_QLOAD", g3dKmdExeInstGetIdx(pInst));
		met_tag_start(0x8163, "WAIT-FF-ANALYSIS_QLOAD");
#endif

		wqRet = g3dkmd_wait_event_interruptible_timeout(
								       /* wait queue */ gExecInfo.
								       s_QLoadWq,
								       /* condition  */
								       gExecInfo.s_isHwQloadDone,
								       /* timeout    */
								       G3DKMD_QLOAD_WAIT_TIMEOUT_MS);

#ifdef G3DKMD_MET_CTX_SWITCH
		met_tag_end(0x8163, "WAIT-FF-ANALYSIS_QLOAD");
		met_tag_oneshot(0x8163, "WAIT-FF_QLOAD", 0);
#endif
	}

	if (wqRet == 0) { /* timeout */
		is_timeout = G3DKMD_TRUE;

		KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE, "QLoad wait queue: timeout.\n");
		/* tester_is_pass("qload_test", "QLOAD_TIMEOUT", TESTER_PASS); */
		/* tester_custom("qload_test", "QLOAD_SIGNAL", "FailTimeout"); */
		/* tester_custom("qload_test", "QLOAD_DO_NOTHING", "FailTimeout"); */
	} else if (wqRet == -ERESTARTSYS) {	/* system interrupt */
		KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE,
			   "QLoad wait queue: user process interrupted by signal.\n");
		/* tester_custom("qload_test", NULL, "FailInterrupt"); */
	} else {		/*  */
		KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE,
			   "QLoad wait queue: released by wq single or condition is true.\n");
		/* tester_is_pass("qload_test", "QLOAD_SIGNAL", TESTER_PASS); */
		/* tester_is_pass("qload_test", "QLOAD_DO_NOTHING", TESTER_PASS); */
		/* tester_custom("qload_test", "QLOAD_Timeout", "FailNormal"); */
	}

	return is_timeout;
}


#else /* !defined(ENABLE_QLOAD_NON_POLLING) */

static INLINE G3DKMD_BOOL _qload_wait_hw_qload_polling(void)
{
	unsigned int cnt = 0;
	G3DKMD_BOOL is_timeout = G3DKMD_FALSE;

	if (!g3dKmdIsBypass(BYPASS_REG)) {
		while (cnt++ < G3DKMD_ENGINE_TIMEOUT_CNT) {
			if (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, VAL_IRQ_G3D_MMCE_FRAME_FLUSH, 0))
				break;


			G3DKMD_UDELAY(1);

#if 0				/* def FPGA_G3D_HW */
			if (cnt > 1) {
				KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE,
				       "wait qload finish, cnt %d\n", cnt);
			}
#endif
		}
		if (cnt >= G3DKMD_ENGINE_TIMEOUT_CNT) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ENGINE, "g3dKmdQLoad TIMEOUT!!!\n");
			is_timeout = G3DKMD_TRUE;
		}
	}
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 1, VAL_IRQ_G3D_MMCE_FRAME_FLUSH, 7);	/* clear irq */

	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x0, MSK_AREG_MCU2G3D_SET,
		       SFT_AREG_MCU2G3D_SET);
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_MCU2G3D_SET, 0x1, MSK_AREG_MCU2G3D_SET,
		       SFT_AREG_MCU2G3D_SET);

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE,
	       "_qload_wait_hw_qload_polling, IDLE0 0x%x, IDLE1 0x%x, RAW 0x%x\n",
	       g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS0, 0xFFFFFFFF, 0),
	       g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS1, 0xFFFFFFFF, 0),
	       g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 0xFFFFFFFF, 0));

	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 1, VAL_IRQ_G3D_MMCE_FRAME_FLUSH, 7);	/* clear irq */

	return is_timeout;
}
#endif /* defined(ENABLE_QLOAD_NON_POLLING) */

unsigned int g3dKmdQLoad(struct g3dExeInst *pInst)
{
	G3DKMD_BOOL is_timeout;

	_g3dKmdGetToken(G3DKMD_TOKEN_0);
	_g3dKmdDataSync(pInst);

#if defined(ENABLE_QLOAD_NON_POLLING)
	_qload_set_secure();
	_qload_sync_sw();

	gExecInfo.s_isHwQloadDone = G3DKMD_FALSE;
	_qload_set_qload_regs(pInst);

	is_timeout = _qload_wait_hw_qload_non_polling();

	if (!is_timeout)
		gExecInfo.currentExecInst = pInst;

#else /* !defined(ENABLE_QLOAD_NON_POLLING) */
#if defined(G3D_HW) && defined(G3DKMD_SUPPORT_ISR)
	disable_irq(G3DHW_IRQ_ID);
#endif

/* local_bh_disable(); */
#ifdef G3DKMD_SUPPORT_ISR
	g3dKmdSuspendIsr();
#endif

	_qload_set_secure();

	_qload_sync_sw();

	_qload_set_qload_regs(pInst);

	is_timeout = _qload_wait_hw_qload_polling();

	if (!is_timeout)
		gExecInfo.currentExecInst = pInst;

/* local_bh_enable(); */
#ifdef G3DKMD_SUPPORT_ISR
	g3dKmdResumeIsr();
#endif

#if defined(G3D_HW) && defined(G3DKMD_SUPPORT_ISR)
	enable_irq(G3DHW_IRQ_ID);
#endif

#endif /* defined(ENABLE_QLOAD_NON_POLLING) */

	_g3dKmdReleaseToken(G3DKMD_TOKEN_0);

#ifndef G3D_HW
	/*
	 * We'll trigger Qload to do context switch
	 * For HW, it will restore ht read ptr from rsm buffer
	 * For non-HW, we have to restore by previously saved write pointer (because for non-HW,
	 * read pointer equals write pointer)
	 */

#if !defined(TEST_NON_POLLING_FLUSHCMD)
	/* original path */
	g3dKmdRegUpdateReadPtr(pInst->hwChannel, pInst->pHtDataForHwRead->htInfo->wtPtr);
#else
	/* When Testing, we stop HW. so just restore rdPtr back. */
	g3dKmdRegUpdateReadPtr(pInst->hwChannel, pInst->pHtDataForHwRead->htInfo->rdPtr);
#endif
#endif

#ifdef G3DKMD_SUPPORT_SYNC_SW	/* [force to use prim boundary during ctx switch] */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CONTEXT_CTRL, 1, MSK_AREG_CQ_PMT_MD,
		       SFT_AREG_CQ_PMT_MD);
#endif

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "out\n");
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "QBUF 0x%x\n",
	       *((unsigned int *)(pInst->qbaseBuffer.data)));

	return !is_timeout;
}

void g3dKmdRegSendCommand(int hwChannel, struct g3dExeInst *pInst)
{
	unsigned int firePtr;
	/* SapphireQreg* qreg = (SapphireQreg*)pInst->qbaseBuffer.data; */

	/*
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE,"HW[%d]  : wtPtr = %d\n",
		hwChannel, task->pHtDataForHwRead->htInfo->wtPtr);

	*/
	if ((!pInst) || (hwChannel >= NUM_HW_CMD_QUEUE) || (hwChannel == G3DKMD_HWCHANNEL_NONE))
		return;

#ifdef KERNEL_FENCE_DEFER_WAIT
	firePtr = pInst->pHtDataForHwRead->htInfo->readyPtr;

	if (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_CQ_HTBUF_WPTR,
		MSK_AREG_CQ_HTBUF_WPTR, SFT_AREG_CQ_HTBUF_WPTR) == firePtr) {
		return;
	}

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "[%s] pInst %p, SW_WPTR 0x%x, HW_WPTR 0x%x, readyPtr 0x%x\n",
		__func__, pInst, pInst->pHtDataForSwWrite->htInfo->wtPtr,
		g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_CQ_HTBUF_WPTR, MSK_AREG_CQ_HTBUF_WPTR, SFT_AREG_CQ_HTBUF_WPTR),
		firePtr);
#else
	firePtr = pInst->pHtDataForHwRead->htInfo->wtPtr;
#endif
	_g3dKmdDataSync(pInst);

#if 0				/* dump ctx switch info */
	{
		struct g3dExeInst *p1 = NULL;
		struct g3dExeInst *p2 = NULL;
		unsigned char p1nonEmpty, p1isNew, p1isPending, p1isRunning;
		unsigned char p2nonEmpty, p2isNew, p2isPending, p2isRunning;

		pr_debug("[Brady] there are total %d exeInfoCount\n", gExecInfo.exeInfoCount);

		p1 = gExecInfo.exeList[0];
		p2 = gExecInfo.exeList[1];

		pr_debug("[Brady]======= Inst Status==========\n");

		if (gExecInfo.exeInfoCount > 0) {
			p1nonEmpty = !g3dKmdIsHtInfoEmpty(p1->hwChannel, p1->htInfo);
			p1isNew = (p1->state == G3DEXESTATE_IDLE);
			p1isPending = (p1->state == G3DEXESTATE_WAITQUEUE);
			p1isRunning = (p1->state == G3DEXESTATE_RUNNING);
			pr_debug("[Brady] p1: nonEmpty %d, isNew %d isPending %d isRunning %d\n",
					p1nonEmpty, p1isNew, p1isPending, p1isRunning);
		}
		if (gExecInfo.exeInfoCount > 1) {
			p2nonEmpty = !g3dKmdIsHtInfoEmpty(p2->hwChannel, p2->htInfo);
			p2isNew = (p2->state == G3DEXESTATE_IDLE);
			p2isPending = (p2->state == G3DEXESTATE_WAITQUEUE);
			p2isRunning = (p2->state == G3DEXESTATE_RUNNING);
			pr_debug("[Brady] p2: nonEmpty %d, isNew %d isPending %d isRunnig %d\n",
				p2nonEmpty, p2isNew, p2isPending, p2isRunning);
		}

	}
#endif

#ifdef FPGA_G3D_HW
#if 0
	if (pInst->pHtDataForHwRead->htInfo->wtPtr == pInst->pHtDataForHwRead->htInfo->rdPtr) {
		KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "in, empty command\n");
		return;
	}
#endif
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "wptr 0x%x, rptr 0x%x\n", firePtr,
	       g3dKmdRegGetReadPtr(pInst->hwChannel));
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "hwAddr 0x%x, patPhyAddr 0x%x\n",
	       pInst->pHtDataForHwRead->htBuffer.hwAddr,
	       pInst->pHtDataForHwRead->htBuffer.patPhyAddr);
#endif

#if defined(G3D_HW) && (!defined(ANDROID_PREF_NDEBUG) || defined(ANDROID_HT_COUNT))
	{
		/* Louis: don't know if this is for HwRead or SwWrite. */
		int queued_slots = g3dKmdGetQueuedHt2Slots(pInst->hwChannel, pInst->pHtDataForHwRead->htInfo);

		/* HtData for HwRead and SwWrite could be different htInstance. */
		if (pInst->pHtDataForHwRead != pInst->pHtDataForSwWrite) {
			/* if pointing to differnt htInstance, add slots in HtForSwWrite. */
			int slots = g3dKmdGetQueuedHt2Slots(pInst->hwChannel, pInst->pHtDataForSwWrite->htInfo);

			queued_slots = (queued_slots > 0 && slots > 0) ? queued_slots : queued_slots + slots;
		}

		event_trace_printk(0, "C|%d|%s|%d\n", 0, "HT2", queued_slots);
	}
#endif
#ifndef YL_NOFIRE_FULL
	/* update tail pointer */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_HTBUF_WPTR, firePtr, MSK_AREG_CQ_HTBUF_WPTR,
		       SFT_AREG_CQ_HTBUF_WPTR);

#if 0 /* debug */
	pr_debug("[SEND_COMMAND] wrt_ptr = 0x%x, writePtr\n");
	pr_debug("[SEND_COMMAND] data[0] = 0x%x, writePtr[0]\n");
	pr_debug("[SEND_COMMAND] data[1] = 0x%x, writePtr[1]\n");
	pr_debug("[SEND_COMMAND] data[2] = 0x%x, writePtr[2]\n");
	pr_debug("[SEND_COMMAND] data[3] = 0x%x, writePtr[3]\n");
#endif

	/* trigger tail upd */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0x1, MSK_AREG_CQ_HTBUF_WPTR_UPD,
		       SFT_AREG_CQ_HTBUF_WPTR_UPD);

	/* enable clk (fire if may triggered by GCE) // for FPGA20188 to refine this option */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_ENG_FIRE, 0x1, MSK_AREG_G3D_ENG_FIRE,
		       SFT_AREG_G3D_ENG_FIRE);

	/* disable tail upd */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0x0, MSK_AREG_CQ_HTBUF_WPTR_UPD,
		       SFT_AREG_CQ_HTBUF_WPTR_UPD);
#endif /* YL_NOFIRE_FULL */

#ifndef G3D_HW
	if (tester_fence_block()) {
		KMDDPF_ENG(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE, "fence is opened!\n");
		g3dKmdRegUpdateReadPtr(pInst->hwChannel, pInst->pHtDataForHwRead->htInfo->wtPtr);
	}			/* tester_fence_block(); */
#endif

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_ENGINE, "out\n");

#ifdef G3DKMD_MET_CTX_SWITCH
	met_tag_oneshot(0x8163, "g3dKmdRegSendCommand", g3dKmdExeInstGetIdx(pInst));
#endif
}

void g3dKmdRegRepeatCommand(int hwChannel, struct g3dExeInst *pInst, unsigned int readPtr)
{
#ifdef YL_HW_DEBUG_DUMP_TEMP
	unsigned int writePtr = pInst->pHtDataForHwRead->htInfo->wtPtr;

	/* update head pointer */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_HTBUF_RPTR, readPtr, MSK_AREG_CQ_HTBUF_RPTR,
			SFT_AREG_CQ_HTBUF_RPTR);

	/* update tail pointer */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_HTBUF_WPTR, writePtr, MSK_AREG_CQ_HTBUF_WPTR,
			SFT_AREG_CQ_HTBUF_WPTR);

	/* trigger tail upd */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0x1, MSK_AREG_CQ_HTBUF_WPTR_UPD, SFT_AREG_CQ_HTBUF_WPTR_UPD);

	/* enable clk (fire if may triggered by GCE) // for FPGA20188 to refine this option */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_G3D_ENG_FIRE, 0x1, MSK_AREG_G3D_ENG_FIRE, SFT_AREG_G3D_ENG_FIRE);

	/* disable tail upd */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_CQ_CTRL, 0x0, MSK_AREG_CQ_HTBUF_WPTR_UPD, SFT_AREG_CQ_HTBUF_WPTR_UPD);

	if (!g3dKmdIsBypass(BYPASS_RIU)) {
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfff8, 0x02);
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", MOD_OFST(MFG, 0x50), 0x02);

		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n", 0xfff8, VAL_PA_IDLE_ST0_G3D_ALL_IDLE);
		g3dKmdRiuWrite(G3DKMD_RIU_G3D, "%04x_%08x\n",
			       MOD_OFST(G3D, REG_AREG_PA_IDLE_STATUS0), VAL_PA_IDLE_ST0_G3D_ALL_IDLE);
	}
#endif
}


void g3dKmdRegRestartHW(int taskID, int mode)
{
	unsigned int ridx;	/* which H/T entry */
	struct g3dTask *task;
	unsigned int rptr, wptr;
	int i;
	struct g3dExeInst *pInst, *pExeInst;
	struct g3dHtInfo *ht;
	SapphireQreg *qreg;

	pInst = (taskID < 0) ? gExecInfo.currentExecInst : g3dKmdTaskGetInstPtr(taskID);

	if ((pInst == NULL) || (pInst->hwChannel >= NUM_HW_CMD_QUEUE))
		return;

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ENGINE, "in, taskID %d\n", taskID);

	rptr = g3dKmdRegGetReadPtr(pInst->hwChannel);
	ht = pInst->pHtDataForHwRead->htInfo;
	/* increment reset count */
	ridx = (rptr - ht->htCommandBaseHw) / HT_CMD_SIZE;
	task = g3dKmdGetTask(ht->htCommand[ridx].command);
	if (task != NULL)
		task->hw_reset_cnt++;

	lock_g3dFdq();

	/* 2. reset HW, and check if idle */
	g3dKmdRegReset();

#ifdef G3DKMD_SUPPORT_FDQ
	/* 4. reset FDQ buffer pointer */
	gExecInfo.fdq_rptr = 0; /* 0-base, 64bit unit */
	gExecInfo.fdq_wptr = 0;
	g3dKmdFdqUpdateBufPtr();
#endif

	unlock_g3dFdq();

	/* 5. Set all the qbasebuf to skip all of HT contents. */
	for (i = 0; i < gExecInfo.exeInfoCount; i++) {
		pExeInst = gExecInfo.exeList[i];
		if (pExeInst != NULL) {
			ht = pExeInst->pHtDataForHwRead->htInfo;
			wptr = ht->wtPtr;
			rptr = ht->rdPtr;
			/* clear rsm bit */
			qreg = (SapphireQreg *)pExeInst->qbaseBuffer.data;
			qreg->reg_cq_rsm.dwValue = 0;
			qreg->reg_cq_htbuf_rptr.s.cq_htbuf_rptr = wptr;
			qreg->reg_cq_htbuf_wptr.s.cq_htbuf_wptr = wptr;

			KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_DDFR | G3DKMD_MSG_ENGINE,
				"pInst 0x%x qload ht rptr 0x%x, wptr 0x%x\n", pExeInst,
				qreg->reg_cq_htbuf_rptr.s.cq_htbuf_rptr,
				qreg->reg_cq_htbuf_wptr.s.cq_htbuf_wptr);

			while (wptr != rptr) {
			#ifdef G3D_SWRDBACK_RECOVERY
				g3dKmdRecoveryUpdateSWReadBack(pExeInst, rptr);
			#endif
				rptr += HT_CMD_SIZE;
				if (rptr >= ht->htCommandBaseHw + HT_BUF_SIZE)
					rptr = ht->htCommandBaseHw;

			}
			ht->rdPtr = rptr;
		}
	}
	/* 6. reset recovery counters */

#ifdef G3DKMD_RECOVERY_BY_TIMER
	g3dKmd_recovery_reset_counter();
#endif

	_g3dKmdDataSync(pInst);

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ENGINE, "out, taskID %d\n", taskID);

}

int g3dCheckDeviceId(void)
{
	/* not check 0x13000020 also due to CONFIG_OF doesn't map it */
	unsigned int deviceID = MFG_REG_READ32_MSK_SFT(0x20, 0xffffffff, 0);

	return (deviceID == 0x02075017) ? 1 : 0;
}

unsigned int g3dKmdCheckDeviceID(void)
{
	unsigned int g3dID;

	g3dID = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_G3D_ID, MSK_AREG_REVISION_ID | MSK_AREG_VERSION_ID,
			      SFT_AREG_REVISION_ID);
	return g3dID;
}
void g3dKmdCacheFlushInvalidate(void)
{
#ifdef FPGA_G3D_HW
#ifndef G3D_NONCACHE_BUFFERS
	if (in_interrupt()) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_ENGINE,
			"Cannot invoke smp_inner_dcache_flush_all() in interrupt/softirq context\n");
		return;
	}
	/* clone the way from ion driver or m4u driver (m4u_dma_cache_flush_all) */
	/* L1 cache clean before hw read */
	smp_inner_dcache_flush_all();

	/* L2 cache maintenance by physical pages */
	/* [Qusetion] not sure to use it on LDVT???? */
	outer_flush_all();

	/* __cpuc_flush_user_all(); */
	/* __cpuc_flush_kern_all(); */
#endif /* G3D_NONCACHE_BUFFERS */
#endif /* FPGA_G3D_HW */
}

unsigned int g3dKmdCheckHwIdle(void)
{
#define CHECK_HW_IDLE_CNT 10
	unsigned int result = 1;

#ifdef FPGA_G3D_HW
	unsigned int i;

	for (i = 0; i < CHECK_HW_IDLE_CNT; i++) {
		if (g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS0, 0xFFFFFFFF, 0) != 0xFFFFFFFF ||
		    g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS1, 0xFFFFFFFF, 0) != 0xFFFFFFFF) {
			result = 0;
			break;
		}

		G3DKMD_SLEEP(1);
	}
#endif

	return result;
}

#ifdef G3DKMDDEBUG

#ifdef G3DKMD_MEM_CHECK
void *g3dkmdmalloc(unsigned int size, unsigned int flags, const char *file, const char *func, unsigned int nline)
{
	void *p = kmalloc(size, flags);

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE,
	       "(HW_MEMORY) kmalloc: ptr = 0x%x, size = %d at %s():ln%d in %s\n", p, size, func,
	       nline, file);

	return p;
}

void g3dkmdfree(void *p, const char *file, const char *func, unsigned int nline)
{
	kfree(p);

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE,
	       "(HW_MEMORY) kfree: ptr = 0x%x at %s():ln%d in %s\n", p, func, nline, file);
}

void *g3dkmdvmalloc(unsigned int size, const char *file, const char *func, unsigned int nline)
{
	void *p = vmalloc(size);

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE,
	       "(HW_MEMORY) vmalloc: ptr = 0x%x, size = %d at %s():ln%d in %s\n", p, size, func, nline, file);

	return p;
}

void g3dkmdvfree(void *p, const char *file, const char *func, unsigned int nline)
{
	vfree(p);

	KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_ENGINE,
	       "(HW_MEMORY) vfree: ptr = 0x%x at %s():ln%d in %s\n", p, func, nline, file);
}
#endif /* G3DKMD_MEM_CHECK */

#endif /* G3DKMDDEBUG */
