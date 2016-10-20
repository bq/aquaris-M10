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

#ifdef _WIN32
#include <Windows.h>
#include <stdio.h>
#endif

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dkmd_util.h"
#include "g3dkmd_task.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_internal_memory.h"

#include "g3dkmd_mmcq.h"
#include "g3dkmd_mmcq_internal.h"
#include "g3dkmd_mmce.h"
#include "g3dkmd_mmce_reg.h"

#ifdef linux
#ifdef linux_user_mode
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <linux/slab.h>
#endif
#endif

#ifdef PATTERN_DUMP
#include "g3dkmd_file_io.h"
#include "g3dkmd_pattern.h"
#include "g3dkmd_cfg.h"
#endif


#ifdef USE_GCE

/* local define */

/* assume each HT cmd is composed by 40 MMCE instruction, each MMCE instruction is 8 byte */
/* "+ 1" is for global instructions */
#define MAX_MMCQ_SIZE           ((MAX_HT_COMMAND + 1) * 40 * MMCQ_INSTRUCTION_SIZE)
#define MMCQ_TIMEOUT_CNT        1000

enum MMCQ_CMD_OP {
	MMCQ_CMD_OP_READ = 0x01,
	MMCQ_CMD_OP_MOVE = 0x02,
	MMCQ_CMD_OP_WRITE = 0x04,
	MMCQ_CMD_OP_POLL = 0x08,
	MMCQ_CMD_OP_JUMP = 0x10,
	MMCQ_CMD_OP_SYNC = 0x20,
	MMCQ_CMD_OP_MARKER = 0x40,
};

#define MSK_OP_CODE                 0xFF000000
#define SFT_OP_CODE                 24

#define MSK_ARG_A                   0x00FFFFFF

/* bit-mask of extended instruction */
#define MSK_EXT_ARGA_TYPE			0x00800000
#define SFT_EXT_ARGA_TYPE			23
#define MSK_EXT_ARGB_TYPE			0x00400000
#define SFT_EXT_ARGB_TYPE			22
#define MSK_EXT_NON_SUSPENDABLE		0x00200000
#define SFT_EXT_NON_SUSPENDABLE     21
#define MSK_EXT_SUBSYS				0x001F0000
#define SFT_EXT_SUBSYS				16

/* read/ write/ poll instruction */
#define IO_EVENT_ARG_A(io_subsys, val, is_suspendable) \
	(unsigned int)(((io_subsys << SFT_EXT_SUBSYS) & MSK_EXT_SUBSYS) | \
		(val & 0x1FFFF) | ((is_suspendable << SFT_EXT_NON_SUSPENDABLE) & MSK_EXT_NON_SUSPENDABLE))

/* bit-mask of marker instruction */
#define MSK_MARK_NO_INC_EXEC_CMDS_CNT	0x00010000
#define SFT_MARK_NO_INC_EXEC_CMDS_CNT	16
#define MSK_MARK_PREFETCH_EN			0x00010000
#define SFT_MARK_PREFETCH_EN            16

#define MARKER_EVENT_ARG_A(non_suspendable, no_inc_exec) \
	(((non_suspendable << SFT_EXT_NON_SUSPENDABLE) & MSK_EXT_NON_SUSPENDABLE) | \
		((no_inc_exec << SFT_MARK_NO_INC_EXEC_CMDS_CNT) & MSK_MARK_NO_INC_EXEC_CMDS_CNT))
#define MARKER_EVENT_ARG_B(prefetch_en) ((prefetch_en << SFT_MARK_PREFETCH_EN) & MSK_MARK_PREFETCH_EN)

/* bit-mask of sync instruction */
#define MSK_SYNC_TO_UPDATE			0x80000000
#define SFT_SYNC_TO_UPDATE			31
#define MSK_SYNC_UPDATE_OP			0x70000000
#define SFT_SYNC_UPDATE_OP			28
#define MSK_SYNC_UPDATE_V			0x0FFF0000
#define SFT_SYNC_UPDATE_V			16
#define MSK_SYNC_TO_WAIT			0x00008000
#define SFT_SYNC_TO_WAIT			15
#define MSK_SYNC_WAIT_OP			0x00007000
#define SFT_SYNC_WAIT_OP			12
#define MSK_SYNC_WAIT_V				0x00000FFF
#define SFT_SYNC_WAIT_V				0

#define SYNC_ARG_A(val, is_suspendable) (unsigned int)((val & 0x1FFFF) | \
		((is_suspendable << SFT_EXT_NON_SUSPENDABLE) & MSK_EXT_NON_SUSPENDABLE))
#define SYNC_ARG_B(to_update, update_op, update_v, to_wait, wait_op, wait_v) \
	(((((unsigned int)to_update) << SFT_SYNC_TO_UPDATE) & MSK_SYNC_TO_UPDATE) | \
	((((unsigned int)update_op) << SFT_SYNC_UPDATE_OP) & MSK_SYNC_UPDATE_OP) | \
	((((unsigned int)update_v)  << SFT_SYNC_UPDATE_V)  & MSK_SYNC_UPDATE_V)  | \
	((((unsigned int)to_wait)   << SFT_SYNC_TO_WAIT)   & MSK_SYNC_TO_WAIT)   | \
	((((unsigned int)wait_op)   << SFT_SYNC_WAIT_OP)   & MSK_SYNC_WAIT_OP)   | \
	((((unsigned int)wait_v)    << SFT_SYNC_WAIT_V)    & MSK_SYNC_WAIT_V))

/* local variable definition */


/* local function implementation */
struct MMCQ_INST *_g3dKmdMmcqCreateQueue(void)
{
	struct MMCQ_BUF *buf = NULL;
	struct MMCQ_INST *mmcq = NULL;
	unsigned int flags = 0;

	/* allocate struct MMCQ_INST */
	mmcq = (struct MMCQ_INST *) G3DKMDMALLOC(sizeof(struct MMCQ_INST));
	YL_KMD_ASSERT(mmcq != NULL);

	if (mmcq == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "G3DKMDMALLOC fail\n");
		return NULL;
	}

	memset(mmcq, 0, sizeof(struct MMCQ_INST));

#if defined(G3D_NONCACHE_BUFFERS)
	flags |= G3DKMD_MMU_DMA;
#endif

	/* prepare inst_buf */
	buf = &mmcq->inst_buf;

#ifdef MMCE_SUPPORT_IOMMU
	flags |= G3DKMD_MMU_ENABLE | G3DKMD_MMU_SHARE;
	if (!allocatePhysicalMemory(&buf->buf_data, MAX_MMCQ_SIZE, 8, flags)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "allocatePhysicalMemory fail\n");
		return NULL;
	}
#else
	if (!allocatePhysicalMemory(&buf->buf_data, MAX_MMCQ_SIZE, 8, flags)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "allocatePhysicalMemory fail\n");
		return NULL;
	}
#endif
	memset(buf->buf_data.data, 0, MAX_MMCQ_SIZE);

	buf->read_ptr = buf->write_ptr = (unsigned int *)buf->buf_data.data;
	buf->base0_phy = mmcq->backup_pc_phy = buf->buf_data.hwAddr;
	buf->base0_vir = buf->buf_data.data;

	buf->end1_phy = buf->base0_phy + MAX_MMCQ_SIZE;
	buf->end1_vir = (void *)((uintptr_t) buf->base0_vir + MAX_MMCQ_SIZE);

	/* update state */
	mmcq->state = MMCQ_STATE_STOP;

	return mmcq;
}

void _g3dKmdMmcqFreeQueue(struct MMCQ_INST *mmcq)
{
	freePhysicalMemory(&(mmcq->inst_buf.buf_data));
}

unsigned int _g3dKmdMmcqGetEmptySize(struct MMCQ_INST *mmcq)
{				/* if read_ptr == write_ptr, it means empty */
	unsigned int size, buf_size;
	struct MMCQ_BUF *buf;
	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,"mmcq 0x%x", mmcq)); */

	YL_KMD_ASSERT(mmcq != NULL);

	if (mmcq == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "\nmmcq is NULL\n");
		return 0;
	}

	buf = &mmcq->inst_buf;
	if (mmcq->state == MMCQ_STATE_RUN) {
		buf->read_ptr =
		    (unsigned int *)MMCE_TO_VIR(&(mmcq->inst_buf),
						g3dKmdMmceGetReadPtr(mmcq->thd_handle));
	}

	buf_size =
	    (buf->base1_vir) ? ((uintptr_t) buf->end1_vir -
				(uintptr_t) buf->base1_vir) : MAX_MMCQ_SIZE;

	size = (buf->read_ptr > buf->write_ptr) ?
	    (unsigned int)((uintptr_t) buf->read_ptr - (uintptr_t) buf->write_ptr) :
	    (unsigned int)(buf_size - (uintptr_t) buf->read_ptr + (uintptr_t) buf->write_ptr);
	size = (size < MMCQ_INSTRUCTION_SIZE) ? 0 : size - MMCQ_INSTRUCTION_SIZE;

	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,", free size %d, read_ptr 0x%x, write_ptr 0x%x, QSIZE 0x%x\n", \
		size, buf->read_ptr, buf->write_ptr, MAX_MMCQ_SIZE));
	*/

	return size;
}

void _g3dKmdMmcqAddCommand(struct MMCQ_BUF *buf, enum MMCQ_CMD_OP op, unsigned int argA, unsigned int argB)
{
	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,"buf 0x%x, op %d (0x%x, 0x%x)\n", buf, op, argA, argB)); */

	YL_KMD_ASSERT(buf != NULL);

	if ((uintptr_t) buf->write_ptr + MMCQ_INSTRUCTION_SIZE >= (uintptr_t) buf->end1_vir) {
		/* add jump instruction */
		*buf->write_ptr++ = buf->base1_phy;
		*buf->write_ptr++ =
		    (MMCQ_CMD_OP_JUMP << SFT_OP_CODE) | ((unsigned int)MMCQ_JUMP_MODE_ABS &
							 MSK_ARG_A);

		/* reset write_ptr */
		buf->write_ptr = (unsigned int *)buf->base1_vir;
	}

	*buf->write_ptr++ = argB;
	*buf->write_ptr++ = (op << SFT_OP_CODE) | (argA & MSK_ARG_A);
}

void _g3dKmdMmcqWait(MMCQ_HANDLE handle, unsigned int token_id,
		     enum MMCQ_TO_UPDATE toUpdate, enum MMCQ_UPDATE_OP updateOp, unsigned int update_value,
		     enum MMCQ_TO_WAIT toWait, enum MMCQ_WAIT_OP waitOp, unsigned int wait_value)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	_g3dKmdMmcqAddCommand(&mmcq->inst_buf, MMCQ_CMD_OP_SYNC,
			      SYNC_ARG_A(token_id, mmcq->is_suspendable), SYNC_ARG_B(toUpdate,
										     updateOp,
										     update_value,
										     toWait, waitOp,
										     wait_value));
}

/* external function implementation */
MMCQ_HANDLE g3dKmdMmcqCreate(void)
{
	MMCQ_HANDLE handle = NULL;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "\n");

	handle = (MMCQ_HANDLE) _g3dKmdMmcqCreateQueue();
	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "G3DKMDMALLOC fail\n");
		return NULL;
	}

	return handle;
}

/* release thread id if ref_count == 0 */
/* freeQ */
void g3dKmdMmcqRelease(MMCQ_HANDLE handle)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x\n", handle);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	_g3dKmdMmcqFreeQueue(mmcq);

	G3DKMDFREE(handle);
}

void g3dKmdMmcqReset(void *handle)
{
	struct MMCQ_BUF *buf;
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x\n", handle);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	buf = &mmcq->inst_buf;
	buf->read_ptr = buf->write_ptr = (unsigned int *)buf->base0_vir;
	buf->end0_vir = buf->base1_vir = NULL;
	buf->end0_phy = buf->base1_phy = 0;
	memset(buf->buf_data.data, 0, MAX_MMCQ_SIZE);

#ifdef PATTERN_DUMP
	if (g3dKmdCfgGet(CFG_DUMP_ENABLE))
		g3dKmdMmceSetReadPtr(mmcq->thd_handle, buf->base0_phy);
#endif

}

void g3dKmdMmcqSetThread(MMCQ_HANDLE mmcq_handle, unsigned int thd_handle)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) mmcq_handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "mmcq_handle 0x%x, thd_handle 0x%x\n",
	       mmcq_handle, thd_handle);

	YL_KMD_ASSERT(mmcq_handle != NULL && thd_handle != MMCE_INVALID_THD_HANDLE);

	if (mmcq_handle == NULL || thd_handle == MMCE_INVALID_THD_HANDLE) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	mmcq->thd_handle = thd_handle;
}

void g3dKmdMmcqWaitIdle(MMCQ_HANDLE mmcq_handle)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) mmcq_handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "mmcq_handle 0x%x\n", mmcq_handle);

	YL_KMD_ASSERT(mmcq_handle != NULL);

	if (mmcq_handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	g3dKmdMmceWaitIdle(mmcq->thd_handle);
}

void g3dKmdMmcqResume(MMCQ_HANDLE handle)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x\n", handle);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	mmcq->state = MMCQ_STATE_RUN;
}

void g3dKmdMmcqSuspend(MMCQ_HANDLE handle)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x\n", handle);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	mmcq->state = MMCQ_STATE_PAUSE;
}

void g3dKmdMmcqSetPC(MMCQ_HANDLE handle, unsigned int pc)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x, pc 0x%x\n", handle, pc);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	mmcq->backup_pc_phy = pc;
}

unsigned int g3dKmdMmcqGetPC(MMCQ_HANDLE handle)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x\n", handle);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return 0;
	}

	return mmcq->backup_pc_phy;
}

int g3dKmdMmcqWaitSize(MMCQ_HANDLE handle, unsigned int size)
{
	unsigned int cnt = 0;
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,"handle 0x%x, size %d\n", handle, size)); */

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return 0;
	}

	if (!mmcq->inst_buf.base1_vir) {
		if (size <= MAX_MMCQ_SIZE)
			return 1;
	} else {
		if (size <= MAX_MMCQ_SIZE - MMCQ_INSTRUCTION_SIZE) {
			while (cnt < MMCQ_TIMEOUT_CNT) {
				if (_g3dKmdMmcqGetEmptySize(mmcq) >= size)
					return 1;

				G3DKMD_SLEEP(1);
				cnt++;
			}
		}
	}

	YL_KMD_ASSERT(0);
	return 0;
}

void g3dKmdMmcqWrite(MMCQ_HANDLE handle, enum MMCQ_SUBSYS subsys, unsigned int reg_addr,
		     unsigned int value, unsigned int mask)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;
	struct MMCQ_BUF *buf;
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,"handle 0x%x, reg_addr 0x%x, value 0x%x, mask 0x%x\n", \
		handle, reg_addr, value, mask));
	*/

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}
/*
#ifdef PATTERN_DUMP
	if (subsys == MMCQ_SUBSYS_GPU && !g3dKmdIsBypass(BYPASS_RIU))
	{
		unsigned int reg_val = g3dKmdRegRead(G3DKMD_REG_G3D, reg_addr & 0xFFF, 0xFFFFFFFF, 0);
		reg_val = (reg_val & (~mask)) | (value & mask);
		g3dKmdRiuWrite(G3DKMD_RIU_GCE, "%04x_%08x\n", reg_addr, reg_val);
	}
#endif
*/

	buf = &mmcq->inst_buf;

	if (mask != MMCQ_DEFAULT_MASK) {
		_g3dKmdMmcqAddCommand(buf, MMCQ_CMD_OP_MOVE, SYNC_ARG_A(0, mmcq->is_suspendable),
				      ~mask);
		reg_addr = reg_addr | 0x01;
	}

	_g3dKmdMmcqAddCommand(buf, MMCQ_CMD_OP_WRITE,
			      IO_EVENT_ARG_A(subsys, reg_addr, mmcq->is_suspendable), value);
}

void g3dKmdMmcqRead(MMCQ_HANDLE handle, enum MMCQ_SUBSYS subsys, unsigned int reg_addr)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,"handle 0x%x, reg_addr 0x%x\n", handle, reg_addr)); */

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	_g3dKmdMmcqAddCommand(&mmcq->inst_buf, MMCQ_CMD_OP_READ,
			      IO_EVENT_ARG_A(subsys, reg_addr, mmcq->is_suspendable), 0);
}

void g3dKmdMmcqPoll(MMCQ_HANDLE handle, enum MMCQ_SUBSYS subsys, unsigned int reg_addr,
		    unsigned int value, unsigned int mask)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;
	struct MMCQ_BUF *buf;
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,"handle 0x%x, reg_addr 0x%x, value 0x%x, mask 0x%x\n", \
		handle, reg_addr, value, mask));
	*/

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	buf = &mmcq->inst_buf;

	if (mask != MMCQ_DEFAULT_MASK) {
		_g3dKmdMmcqAddCommand(buf, MMCQ_CMD_OP_MOVE, SYNC_ARG_A(0, mmcq->is_suspendable),
				      ~mask);
		reg_addr = reg_addr | 0x01;
	}

	_g3dKmdMmcqAddCommand(buf, MMCQ_CMD_OP_POLL,
			      IO_EVENT_ARG_A(subsys, reg_addr, mmcq->is_suspendable), value);
}

void g3dKmdMmcqUpdateToken(MMCQ_HANDLE handle, unsigned int token_id, unsigned int set_value)
{
	YL_KMD_ASSERT(handle != NULL);

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x, token %d, value %d\n", handle,
	       token_id, set_value);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	_g3dKmdMmcqWait(handle, token_id,
			MMCQ_TO_UPDATE_UPDATE, MMCQ_UPDATE_OP_ASSIGN, set_value,
			MMCQ_TO_WAIT_NO_WAIT, MMCQ_WAIT_OP_EQ, 0);
}

void g3dKmdMmcqWaitForEvent(MMCQ_HANDLE handle, unsigned int token_id, unsigned int wait_value)
{
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,"handle 0x%x, token %d, wait_value %d\n", \
		handle, token, wait_value));
	*/

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	_g3dKmdMmcqWait(handle, token_id,
			MMCQ_TO_UPDATE_NO_UPDATE, MMCQ_UPDATE_OP_ASSIGN, 0,
			MMCQ_TO_WAIT_WAIT, MMCQ_WAIT_OP_EQ, wait_value);
}

void g3dKmdMmcqWaitAndClear(MMCQ_HANDLE handle, unsigned int token_id, unsigned int wait_value,
			    unsigned int clear_value)
{
	/*
	KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,"handle 0x%x, token %d, wait_value %d, clear_value %d\n", \
		handle, token, wait_value, clear_value));
	*/

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	_g3dKmdMmcqWait(handle, token_id,
			MMCQ_TO_UPDATE_UPDATE, MMCQ_UPDATE_OP_ASSIGN, clear_value,
			MMCQ_TO_WAIT_WAIT, MMCQ_WAIT_OP_EQ, wait_value);
}

void g3dKmdMmcqJump(MMCQ_HANDLE handle, enum MMCQ_JUMP_MODE mode, unsigned int addr)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;
	unsigned int argA;

	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,"handle 0x%x, mode %d, addr 0x%x\n", handle, mode, addr)); */

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	argA = SYNC_ARG_A((unsigned int)mode, mmcq->is_suspendable);

	_g3dKmdMmcqAddCommand(&mmcq->inst_buf, MMCQ_CMD_OP_JUMP, argA, addr);
}

void g3dKmdMmcqJumpToNextInst(MMCQ_HANDLE handle)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;
	unsigned int nextInst;

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	nextInst =
	    (unsigned int)((uintptr_t) mmcq->inst_buf.write_ptr -
			   (uintptr_t) mmcq->inst_buf.base0_vir) + mmcq->inst_buf.base0_phy +
	    MMCQ_INSTRUCTION_SIZE;
	g3dKmdMmcqJump(handle, MMCQ_JUMP_MODE_ABS, nextInst);
	mmcq->backup_pc_phy = nextInst;
}

void g3dKmdMmcqFlush(MMCQ_HANDLE handle)
{
	unsigned int thd_id;
	struct MMCQ_BUF *buf;
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,"handle 0x%x\n", handle)); */

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	thd_id = g3dKmdMmceGetThreadId(mmcq->thd_handle);
	buf = &mmcq->inst_buf;
	YL_KMD_ASSERT(thd_id != MMCE_INVALID_THD_ID);

	g3dKmdMmcqWrite(handle, MMCQ_SUBSYS_MMCE, CMDQ_THRx_EXEC_CMDS_CNT(thd_id),
			MMCE_TO_PHY(buf, buf->write_ptr), MMCQ_DEFAULT_MASK);

	if (mmcq->state == MMCQ_STATE_RUN) {
		g3dKmdMmceFlush(mmcq->thd_handle);
#ifndef G3D_HW
		buf->read_ptr = buf->write_ptr;
#endif
	}
}

void g3dKmdMmcqAddMarker(MMCQ_HANDLE handle, enum MMCQ_MARKER marker)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;
	struct MMCQ_BUF *buf;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x, marker %d\n", handle, marker);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	buf = &mmcq->inst_buf;

	switch (marker) {
	case MMCQ_MARKER_EOC:
		_g3dKmdMmcqAddCommand(buf, MMCQ_CMD_OP_MARKER,
				      MARKER_EVENT_ARG_A(mmcq->is_suspendable, 0),
				      MARKER_EVENT_ARG_B(0));
		break;

	case MMCQ_MARKER_PREFETCH_EN:
		_g3dKmdMmcqAddCommand(buf, MMCQ_CMD_OP_MARKER,
				      MARKER_EVENT_ARG_A(mmcq->is_suspendable, 1),
				      MARKER_EVENT_ARG_B(1));
		break;
	}
}

void g3dKmdMmcqSetIsSuspendable(MMCQ_HANDLE handle, enum MMCQ_IS_SUSPENDABLE is_suspendable)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x, isSuspendable %d\n", handle,
	       is_suspendable);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	mmcq->is_suspendable = is_suspendable;
}

void g3dKmdMmcqSetJumpTarget(MMCQ_HANDLE handle, unsigned int target)
{
	unsigned int *addr;

	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x\n", handle);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	for (addr = (unsigned int *)mmcq->inst_buf.base0_vir;
	     addr < (unsigned int *)mmcq->inst_buf.end0_vir; addr += 2) {
		if ((addr[1] & MSK_OP_CODE) == (MMCQ_CMD_OP_JUMP << SFT_OP_CODE)) {
			KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_MMCQ,
			       "jump target changed [0x%x -> 0x%x]\n", addr[0], target);
			addr[0] = target;
			return;
		}
	}

	KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "Can't find jump instruction\n");
	/* YL_KMD_ASSERT(0); */
}

unsigned int g3dKmdMmcqGetAddr(MMCQ_HANDLE handle, enum ADDR_TYPE type)
{
	unsigned int addr;

	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x, type %d\n", handle, type);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return 0;
	}

	switch (type) {
	case MMCQ_ADDR_BASE0:
		addr = mmcq->inst_buf.base0_phy;
		break;

	case MMCQ_ADDR_WRITE_PTR:
		addr = MMCE_TO_PHY(&mmcq->inst_buf, mmcq->inst_buf.write_ptr);
		break;

	default:
		addr = 0;
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "error type %d\n", type);
	}

	return addr;
}

void g3dKmdMmcqUpdateBuffAddr(MMCQ_HANDLE handle)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;
	struct MMCQ_BUF *buf;
	/* KMDDPF((G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ,"handle 0x%x\n", handle)); */

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	buf = &mmcq->inst_buf;
	buf->end0_vir = buf->base1_vir = (void *)buf->write_ptr;
	buf->end0_phy = buf->base1_phy =
	    (unsigned int)((uintptr_t) buf->write_ptr - (uintptr_t) buf->base0_vir) +
	    buf->base0_phy;
}

enum MMCQ_STATE g3dKmdMmcqGetState(MMCQ_HANDLE handle)
{
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x\n", handle);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return MMCQ_STATE_UNKNOWN;
	}

	return mmcq->state;
}

#ifdef PATTERN_DUMP
void g3dKmdMmcqDumpBegin(MMCQ_HANDLE handle)
{
	/* struct MMCQ_INST* mmcq = (struct MMCQ_INST*) handle; */

	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x\n", handle);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}
}

void _g3dKmdMmcqDumpBuf(void *fp, MMCQ_HANDLE handle)
{
	struct MMCQ_BUF *buf;
	struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;

	/* error check */
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x\n", handle);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}
	/* dump frame buf, which is a ring buffer */
	buf = &mmcq->inst_buf;
/*
#ifdef YL_MMU_PATTERN_DUMP_V2
	if( gExecInfo.mmu )
		g3dKmdDumpDataToFilePage(gExecInfo.mmu, fp, (unsigned char*)buf->buf_data.data, \
						buf->buf_data.hwAddr ==> invalid?, buf->buf_data.size);
	else
#endif
#ifdef YL_MMU_PATTERN_DUMP_V1
		if( buf->buf_data.pa_pair_info ==> NULL? )
			g3dKmdDumpDataToFileFull(fp, (unsigned char*)buf->buf_data.data, buf->buf_data.pa_pair_info,\
						buf->buf_data.size);
		else
#endif
*/
	g3dKmdDumpDataToFile(fp, (unsigned char *)buf->buf_data.data, buf->buf_data.patPhyAddr,
			     buf->buf_data.size);
}

void g3dKmdMmcqDumpEnd(MMCQ_HANDLE handle, unsigned int dumpCount)
{
	void *fp;
	char fullpath[256];

	/* error check */
	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}
	/* dump common & frame buf */
	sprintf(fullpath, "mmcq_%03d.hex", dumpCount);

	fp = g_pKmdFileIo->fileOpen(fullpath);
	if (!fp) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "g3dKmdFileOpen fail [%s]\n", fullpath);
		YL_KMD_ASSERT(fp != NULL);
		return;
	}

	_g3dKmdMmcqDumpBuf(fp, handle);
	g_pKmdFileIo->fileClose(fp);

}


void g3dKmdRiuGceDump(MMCQ_HANDLE handle, unsigned int dumpCount)
{
	void *fp;
	char fullpath[256];

	/* error check */
	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}
	/* dump common & frame buf */
	sprintf(fullpath, "riu_gce_%03d.hex", dumpCount);

	fp = g_pKmdFileIo->fileOpen(fullpath);

	if (!fp) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "g3dKmdFileOpen fail [%s]\n", fullpath);
		YL_KMD_ASSERT(fp != NULL);
		return;
	}

	g3dKmdParseRiuGce(fp, handle);

	g_pKmdFileIo->fileClose(fp);

}

void g3dKmdParseRiuGce(void *fp, MMCQ_HANDLE handle)
{
	/* error check */
	KMDDPF(G3DKMD_LLVL_INFO | G3DKMD_MSG_MMCQ, "handle 0x%x\n", handle);

	YL_KMD_ASSERT(handle != NULL);

	if (handle == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMCQ, "handle is NULL\n");
		return;
	}

	{
		struct MMCQ_INST *mmcq = (struct MMCQ_INST *) handle;
		struct MMCQ_BUF *buf = &mmcq->inst_buf;
		unsigned int *dwData = (unsigned int *)buf->buf_data.data;
		G3Dboolean bFromJump = G3D_FALSE;
		unsigned int beforeJumpInst = 0;
		unsigned int beforeJumpData = 0;

		while (dwData != buf->write_ptr) {
			unsigned int inst = dwData[1];
			unsigned int data = dwData[0];

			unsigned int op = (inst & 0xff000000) >> 24;
			unsigned int addr = inst & 0x0000fffe;
			unsigned int type = (inst & 0x0000f000) >> 12;
			G3Dboolean useMask = inst & 0x00000001;

			if ((op == 0x4) && (type == 0x1)) { /* Write & G3D */
				if (useMask) {
					unsigned int *prev_dwData = dwData - 2;
					unsigned int prev_inst;
					unsigned int mask;
					unsigned int orig_value;

					(void)prev_inst;
					if (bFromJump) { /* Jump case, use instruction before jump */
						prev_inst = beforeJumpInst;
						mask = beforeJumpData;
					} else {
						prev_inst = prev_dwData[1];
						mask = prev_dwData[0];
					}
					YL_KMD_ASSERT(prev_inst == 0x02200000);

					orig_value =
					    g3dKmdRegRead(G3DKMD_REG_G3D, (addr & 0xfff),
							  0xffffffff, 0);

					g_pKmdFileIo->fPrintf(fp, "%04x_%08x\n", addr,
							      ((orig_value & mask) | data));
				} else {
					g_pKmdFileIo->fPrintf(fp, "%04x_%08x\n", addr, data);
				}
			} else if (op == 0x10) { /* Jump */
				/* save for next write command mask */
				unsigned int *prev_dwData = dwData - 2;

				beforeJumpInst = prev_dwData[1];
				beforeJumpData = prev_dwData[0];

				/* jump */
				dwData = (unsigned int *)buf->buf_data.data +
						((data - buf->buf_data.patPhyAddr) / 4);
				bFromJump = G3D_TRUE;
				continue;
			}

			dwData += 2;
			bFromJump = G3D_FALSE;
		}
	}
}

#endif /* PATTERN_DUMP */

#endif /* USE_GCE */
