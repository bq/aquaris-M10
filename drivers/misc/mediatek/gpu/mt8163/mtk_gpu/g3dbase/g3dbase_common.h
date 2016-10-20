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

/* Shared by G3D / G3DKMD */

#ifndef _G3DBASE_COMMON_H_
#define _G3DBASE_COMMON_H_

#include "g3dbase_common_define.h"
#include "g3dbase_type.h"

#if defined(linux) && defined(__KERNEL__)
#include <linux/types.h>
#else
#include <stdint.h>
#endif

typedef enum {
	G3DKMDRET_SUCCESS = 0,
	G3DKMDRET_FAIL = 1,
	G3DKMDRET_ERROR = 2
} G3DKMDRETVAL;

/* Context */
#define G3DKMD_CTX_URGENT   (0x1ul << 0)
#define G3DKMD_CTX_SECURE   (0x1ul << 1)
#define G3DKMD_CTX_DVFSPERF (0x1ul << 2)
#define G3DKMD_CTX_NOFIRE   (0x1ul << 3)

typedef struct _G3DKMD_DEV_REG_PARAM_ {
	/* frame command buffer */
	unsigned int cmdBufStart;
	unsigned int cmdBufSize;

	/* device flag */
	unsigned int flag;

	/* dfr buffer */
	unsigned int usrDfrEnable;
	unsigned int dfrVaryStart;
	unsigned int dfrVarySize;
	unsigned int dfrBinhdStart;
	unsigned int dfrBinhdSize;
	unsigned int dfrBinbkStart;
	unsigned int dfrBinbkSize;
	unsigned int dfrBintbStart;
	unsigned int dfrBintbSize;
} G3DKMD_DEV_REG_PARAM;

/* operations for g3dKmdGetDrvInfo */
enum {
	G3DKMD_DRV_INFO_FDQ,
	G3DKMD_DRV_INFO_AREG,
	G3DKMD_DRV_INFO_QREG,
	G3DKMD_DRV_INFO_DFR_SIZE,
	G3DKMD_DRV_INFO_MEM_USAGE,
	G3DKMD_DRV_INFO_UX_DWP,
	G3DKMD_DRV_INFO_EXEC_MODE,
	G3DKMD_DRV_INFO_PM,
	G3DKMD_DRV_INFO_SSP,
	G3DKMD_DRV_INFO_G3DID,
};

/* operations for g3dKmdSetDrvInfo */
enum {
	G3DKMD_DRV_INFO_DFD_SETTING,
	G3DKMD_DRV_INFO_SSP_SETTING,
	G3DKMD_DRV_INFO_MEMLIST_ADD,
	G3DKMD_DRV_INFO_MEMLIST_DEL,
};

#if defined(WIN32) || (defined(linux) && defined(linux_user_mode))
typedef struct _G3DKMD_CFG_ {
	unsigned int ltc_enable;

	unsigned int mmu_enable;
	unsigned int mmu_vabase;

	unsigned int vary_size;
	unsigned int binhd_size;
	unsigned int binbk_size;
	unsigned int bintb_size;

	unsigned int gce_enable;

	unsigned int dump_enable;
	unsigned int phy_heap_enable;

	unsigned int mem_shift_size;
	unsigned int vx_bs_hd_test;

	unsigned int cq_pmt_md;
	unsigned int bkjmp_en;

#ifdef YL_SECURE
	unsigned int secure_enable;
#endif
} G3DKMD_CFG;

#endif

typedef enum _G3DKMD_MSGQ_RTN {
	G3DKMD_MSGQ_RTN_OK,
	G3DKMD_MSGQ_RTN_ERR,
	G3DKMD_MSGQ_RTN_FULL,
	G3DKMD_MSGQ_RTN_EMPTY,
} G3DKMD_MSGQ_RTN;

typedef enum _G3DKMD_MSGQ_TYPE {
	G3DKMD_MSGQ_TYPE_ALLOC_MEM,
	G3DKMD_MSGQ_TYPE_FREE_MEM,
	G3DKMD_MSGQ_TYPE_SET_REG,
	G3DKMD_MSGQ_TYPE_SKIP,
} G3DKMD_MSGQ_TYPE;

/***** Helper functions *****/
typedef struct _G3DKMD_MSGQ_ALLOC_MEM {
	unsigned int hwAddr;
	unsigned int size;
	uint64_t phyAddr;
} G3DKMD_MSGQ_ALLOC_MEM;

typedef struct _G3DKMD_MSGQ_FREE_MEM {
	unsigned int hwAddr;
	unsigned int size;
} G3DKMD_MSGQ_FREE_MEM;

typedef struct _G3DKMD_MSGQ_SET_REG {
	unsigned int reg;
	unsigned int value;
} G3DKMD_MSGQ_SET_REG;

typedef struct _g3dFrameSwData {
	unsigned int stamp;
	unsigned int reserved0;
	uintptr_t readPtr; /* attention: this takes 2 dwords in 64bit platform */
#ifndef __G3D_64__
	unsigned int reserved1;
#endif
} g3dFrameSwData;

typedef struct _G3DKMD_SSP_DUMP_PARAM {
	int taskID;
	unsigned char mode;
} G3DKMD_SSP_DUMP_PARAM;

#define G3D_STAMP_FIX /* to take care the case when time stamp increases to 0 */
/* #define G3D_STAMP_TEST //turn on to test on a narrow range of stamp value */
/* #define G3D_NATIVEFENCE64_TEST  //for speed up test sw stamp exhausted and increase stampCarry value by 1. */
#ifdef G3D_STAMP_TEST
#define G3D_MAX_STAMP_VALUE 0xffffffff
#define G3D_STAMP_VALUE(x) ((x) & G3D_MAX_STAMP_VALUE)
#else
#define G3D_STAMP_VALUE(x) (x)
#endif

#ifdef G3D_STAMP_FIX
#define G3D_STAMP_DONE(stamp, hw, sw) \
	(char)((((hw >= stamp) && ((sw >= hw) || (sw < stamp))) || \
		((hw < stamp) && (sw < stamp) && (sw >= hw))) ? 1 : 0)

#define G3D_STAMP_NOT_FLUSH(stamp, lastflush, sw) \
	(char)((((lastflush >= stamp) && ((sw >= lastflush) || (sw < stamp))) || \
		((lastflush < stamp) && (sw < stamp) && (sw >= lastflush))) ? 0 : 1)


#else
#define G3D_STAMP_DONE(stamp, hw, sw) \
	(char)(((stamp <= hw) || ((stamp > hw) && (stamp > sw))) ? 1 : 0)
#define G3D_STAMP_NOT_FLUSH(stamp, lastflush) \
	(char)((stamp > lastflush) ? 1 : 0)
#endif
#endif /* _G3DBASE_COMMON_H_ */
