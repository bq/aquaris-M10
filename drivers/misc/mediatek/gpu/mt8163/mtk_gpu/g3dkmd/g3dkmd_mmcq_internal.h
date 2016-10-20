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

#ifndef _G3DKMD_MMCQ_INTERNAL_H_
#define _G3DKMD_MMCQ_INTERNAL_H_

#include "g3dkmd_mmcq.h"
#include "g3dkmd_mmce.h"
#include "g3d_memory.h"

#define MMCE_TO_PHY(buf, addr)  (((uintptr_t)(addr) - (uintptr_t)(buf)->base1_vir) + (buf)->base1_phy)
#define MMCE_TO_VIR(buf, addr)  (((unsigned int)(addr) - (buf)->base1_phy) + (uintptr_t)(buf)->base1_vir)

struct MMCQ_BUF {
	unsigned int *read_ptr;
	unsigned int *write_ptr;

	G3dMemory buf_data;

	void *base0_vir;
	void *end0_vir;
	unsigned int base0_phy;
	unsigned int end0_phy;
	void *base1_vir;
	void *end1_vir;
	unsigned int base1_phy;
	unsigned int end1_phy;
};

struct MMCQ_INST {
	enum MMCQ_STATE state;
	MMCE_THD_HANDLE thd_handle;
	struct MMCQ_BUF inst_buf;
	unsigned int backup_pc_phy;
	enum MMCQ_IS_SUSPENDABLE is_suspendable;
};

#endif /* _G3DKMD_MMCQ_INTERNAL_H_ */
