
/*
* Copyright (C) 2017 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#ifndef __DDP_CMDQ_REG_H__
#define __DDP_CMDQ_REG_H__
#include "ddp_drv.h"
#include "ddp_reg.h"

#define DDP_REG_BASE_MM_CMDQ			(disp_dev.regs_va[DISP_REG_CMDQ])
#define DDP_REG_BASE_MDP_RDMA			(disp_dev.regs_va[DISP_REG_MDP_RDMA])
#define DDP_REG_BASE_MDP_RSZ0			(disp_dev.regs_va[DISP_REG_MDP_RSZ0])
#define DDP_REG_BASE_MDP_RSZ1			(disp_dev.regs_va[DISP_REG_MDP_RSZ1])
#define DDP_REG_BASE_MDP_TDSHP			(disp_dev.regs_va[DISP_REG_MDP_TDSHP])
#define DDP_REG_BASE_MDP_WDMA			(disp_dev.regs_va[DISP_REG_MDP_WDMA])
#define DDP_REG_BASE_MDP_WROT			(disp_dev.regs_va[DISP_REG_MDP_WROT])



#define DISPSYS_CMDQ_BASE       DDP_REG_BASE_MM_CMDQ

#define DISPSYS_MDP_RDMA_BASE	DDP_REG_BASE_MDP_RDMA
#define DISPSYS_MDP_RSZ0_BASE	DDP_REG_BASE_MDP_RSZ0
#define DISPSYS_MDP_RSZ1_BASE	DDP_REG_BASE_MDP_RSZ1
#define DISPSYS_MDP_TDSHP_BASE	DDP_REG_BASE_MDP_TDSHP
#define DISPSYS_MDP_WDMA_BASE	DDP_REG_BASE_MDP_WDMA
#define DISPSYS_MDP_WROT_BASE	DDP_REG_BASE_MDP_WROT


/* CMDQ */
#define CMDQ_THREAD_NUM 7
#define DISP_REG_CMDQ_IRQ_FLAG                           (DISPSYS_CMDQ_BASE + 0x10)
#define DISP_REG_CMDQ_LOADED_THR                         (DISPSYS_CMDQ_BASE + 0x18)
#define DISP_REG_CMDQ_THR_SLOT_CYCLES                    (DISPSYS_CMDQ_BASE + 0x30)
#define DISP_REG_CMDQ_BUS_CTRL                           (DISPSYS_CMDQ_BASE + 0x40)
#define DISP_REG_CMDQ_ABORT                              (DISPSYS_CMDQ_BASE + 0x50)
#define DISP_REG_CMDQ_SYNC_TOKEN_ID                      (DISPSYS_CMDQ_BASE + 0x60)
#define DISP_REG_CMDQ_SYNC_TOKEN_VALUE                   (DISPSYS_CMDQ_BASE + 0x64)
#define DISP_REG_CMDQ_THRx_RESET(idx)                    (DISPSYS_CMDQ_BASE + 0x100 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_EN(idx)                       (DISPSYS_CMDQ_BASE + 0x104 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_SUSPEND(idx)                  (DISPSYS_CMDQ_BASE + 0x108 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_STATUS(idx)                   (DISPSYS_CMDQ_BASE + 0x10c + 0x80*idx)
#define DISP_REG_CMDQ_THRx_IRQ_FLAG(idx)                 (DISPSYS_CMDQ_BASE + 0x110 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_IRQ_FLAG_EN(idx)              (DISPSYS_CMDQ_BASE + 0x114 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_SECURITY(idx)                 (DISPSYS_CMDQ_BASE + 0x118 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_PC(idx)                       (DISPSYS_CMDQ_BASE + 0x120 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_END_ADDR(idx)                 (DISPSYS_CMDQ_BASE + 0x124 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_EXEC_CMDS_CNT(idx)            (DISPSYS_CMDQ_BASE + 0x128 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_WAIT_EVENTS0(idx)             (DISPSYS_CMDQ_BASE + 0x130 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_WAIT_EVENTS1(idx)             (DISPSYS_CMDQ_BASE + 0x134 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_OBSERVED_EVENTS0(idx)         (DISPSYS_CMDQ_BASE + 0x140 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_OBSERVED_EVENTS1(idx)         (DISPSYS_CMDQ_BASE + 0x144 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_OBSERVED_EVENTS0_CLR(idx)     (DISPSYS_CMDQ_BASE + 0x148 + 0x80*idx)
#define DISP_REG_CMDQ_THRx_OBSERVED_EVENTS1_CLR(idx)     (DISPSYS_CMDQ_BASE + 0x14c + 0x80*idx)
#define DISP_REG_CMDQ_THRx_INSTN_TIMEOUT_CYCLES(idx)     (DISPSYS_CMDQ_BASE + 0x150 + 0x80*idx)

/* MDP RDMA */
#define DISP_REG_MDP_RDMA_EN		(DISPSYS_MDP_RDMA_BASE + 0x00)
#define DISP_REG_MDP_RDMA_RESET		(DISPSYS_MDP_RDMA_BASE + 0x08)
#define DISP_REG_MDP_RDMA_MON_STA_1	(DISPSYS_MDP_RDMA_BASE + 0x408)


/* MDP SCL0 */
#define DISP_REG_MDP_RSZ0_EN (DISPSYS_MDP_RSZ0_BASE + 0x00)
/* MDP SCL1 */
#define DISP_REG_MDP_RSZ1_EN (DISPSYS_MDP_RSZ1_BASE + 0x00)

/* MDP TDSHP */
#define DISP_REG_MDP_TDSHP_CTRL		(DISPSYS_MDP_TDSHP_BASE + 0x100)
/* MDP WDMA */
#define DISP_REG_MDP_WDMA_INT_EN	(DISPSYS_MDP_WDMA_BASE + 0x00)
#define DISP_REG_MDP_WDMA_RESET		(DISPSYS_MDP_WDMA_BASE + 0x0c)
#define DISP_REG_MDP_WDMA_FLOW_CTRL_DBG	(DISPSYS_MDP_WDMA_BASE + 0xA0)
/* MDP WROT */
#define DISP_REG_MDP_WROT_VIDO_CTRL				(DISPSYS_MDP_WROT_BASE + 0x00)
#define DISP_REG_MDP_WROT_VIDO_SOFT_RST			(DISPSYS_MDP_WROT_BASE + 0x10)
#define DISP_REG_MDP_WROT_VIDO_SOFT_RST_STATUS	(DISPSYS_MDP_WROT_BASE + 0x14)




#endif
