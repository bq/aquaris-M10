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
#include "g3dkmd_buffer.h"
#include "g3dbase_buffer.h"
#include "g3dkmd_internal_memory.h"
#include "g3dkmd_task.h"
#include "g3dkmd_cfg.h"
#include "g3dkmd_util.h"
#include "g3dkmd_file_io.h"

void g3dKmdQueryBuffSizesFromInst(const struct g3dExeInst *pInst, G3DKMD_DEV_REG_PARAM *param)
{
	param->dfrVarySize = pInst->defer_vary.size;
	/* divide by 2, because it will be mulitpled by 2. */
	param->dfrBinbkSize = pInst->defer_binBk.size / 2;
	param->dfrBintbSize = pInst->defer_binTb.size / 2;
	param->dfrBinhdSize = pInst->defer_binHd.size / 2;
}


int g3dKmdCreateBuffers(struct g3dExeInst *pInst, G3DKMD_DEV_REG_PARAM *param)
{

	/* create from user */
	if (param && (param->usrDfrEnable == G3DKMD_TRUE)) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_BUFFERS,
		       "Create DFR from user, vary 0x%x(0x%x), binbk 0x%x(0x%x), bintb 0x%x(0x%x), binhd 0x%x(0x%x)\n",
		       param->dfrVaryStart, param->dfrVarySize,
		       param->dfrBinbkStart, param->dfrBinbkSize / 2,
		       param->dfrBintbStart, param->dfrBintbSize / 2,
		       param->dfrBinhdStart, param->dfrBinhdSize / 2);

		pInst->defer_vary.hwAddr = param->dfrVaryStart;
		pInst->defer_vary.size = param->dfrVarySize;
		pInst->defer_binBk.hwAddr = param->dfrBinbkStart;
		pInst->defer_binBk.size = param->dfrBinbkSize;
		pInst->defer_binTb.hwAddr = param->dfrBintbStart;
		pInst->defer_binTb.size = param->dfrBintbSize;
		pInst->defer_binHd.hwAddr = param->dfrBinhdStart;
		pInst->defer_binHd.size = param->dfrBinhdSize;

		pInst->dfrBufFromUsr = param->usrDfrEnable;
	} else
		/* create from kernel */
	{
		unsigned int va_size, hd_size, tb_size, bk_size;
		unsigned int flag = G3DKMD_MMU_ENABLE;

		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_BUFFERS, "Create DFR from kernel\n");

		if (NULL == param) {
			va_size = G3DKMD_VARY_SIZE_ALIGNED(g3dKmdCfgGet(CFG_DEFER_VARY_SIZE));	/* 4KB align */
			bk_size = G3DKMD_BINBK_SIZE_ALIGNED(g3dKmdCfgGet(CFG_DEFER_BINBK_SIZE));	/* 16B align */
			tb_size = G3DKMD_BINTB_SIZE_ALIGNED(g3dKmdCfgGet(CFG_DEFER_BINTB_SIZE));	/* 128B align */
			hd_size = G3DKMD_BINHD_SIZE_ALIGNED(g3dKmdCfgGet(CFG_DEFER_BINHD_SIZE));	/* 128B align */
		} else {
			va_size = G3DKMD_VARY_SIZE_ALIGNED(param->dfrVarySize);	/* 4K align */
			bk_size = G3DKMD_BINBK_SIZE_ALIGNED(param->dfrBinbkSize);	/* 16B align */
			tb_size = G3DKMD_BINTB_SIZE_ALIGNED(param->dfrBintbSize);	/* 128B align */
			hd_size = G3DKMD_BINHD_SIZE_ALIGNED(param->dfrBinhdSize);	/* 128B align */
		}


#if defined(G3D_NONCACHE_BUFFERS)
		/* flag |= G3DKMD_MMU_DMA; */
#endif

		pInst->dfrBufFromUsr = G3DKMD_FALSE;

		if (!allocatePhysicalMemory
		    (&pInst->defer_vary, va_size, G3DKMD_DEFER_BASE_ALIGN, flag)) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERS,
			       "allocate vary fail (0x%x)\n", va_size);
			goto fail_handler;
		}
		/* twice for buffer0 base1 */
		if (!allocatePhysicalMemory
		    (&pInst->defer_binBk, bk_size * 2, G3DKMD_DEFER_BASE_ALIGN, flag)) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERS,
			       "allocate binbk fail (0x%x)\n", bk_size);
			goto fail_handler;
		}

		if (!allocatePhysicalMemory
		    (&pInst->defer_binTb, tb_size * 2, G3DKMD_DEFER_BASE_ALIGN, flag)) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERS,
			       "allocate bintb fail (0x%x)\n", tb_size);
			goto fail_handler;
		}

		if (!allocatePhysicalMemory
		    (&pInst->defer_binHd, hd_size * 2, G3DKMD_DEFER_BASE_ALIGN, flag)) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERS,
			       "allocate binhd fail (0x%x)\n", hd_size);
			goto fail_handler;
		}

		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_BUFFERS,
		       "Create DFR from kernel, vary 0x%x(0x%x), binbk 0x%x(0x%x), bintb 0x%x(0x%x), binhd 0x%x(0x%x)\n",
		       pInst->defer_vary.hwAddr, pInst->defer_vary.size,
		       pInst->defer_binBk.hwAddr, pInst->defer_binBk.size,
		       pInst->defer_binTb.hwAddr, pInst->defer_binTb.size,
		       pInst->defer_binHd.hwAddr, pInst->defer_binHd.size);
	}

	return G3DKMD_TRUE;

fail_handler:
	freePhysicalMemory(&pInst->defer_vary);
	freePhysicalMemory(&pInst->defer_binBk);
	freePhysicalMemory(&pInst->defer_binTb);
	freePhysicalMemory(&pInst->defer_binHd);
	return G3DKMD_FALSE;

}

void g3dKmdDestroyBuffers(struct g3dExeInst *pInst)
{
	if (pInst->dfrBufFromUsr == G3DKMD_FALSE) {
		KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_BUFFERS,
		       "Destroy DFR from kernel, vary 0x%x(0x%x), binbk 0x%x(0x%x), bintb 0x%x(0x%x), binhd 0x%x(0x%x)\n",
		       pInst->defer_vary.hwAddr, pInst->defer_vary.size,
		       pInst->defer_binBk.hwAddr, pInst->defer_binBk.size,
		       pInst->defer_binTb.hwAddr, pInst->defer_binTb.size,
		       pInst->defer_binHd.hwAddr, pInst->defer_binHd.size);

		freePhysicalMemory(&pInst->defer_vary);
		freePhysicalMemory(&pInst->defer_binBk);
		freePhysicalMemory(&pInst->defer_binTb);
		freePhysicalMemory(&pInst->defer_binHd);
	}
}
