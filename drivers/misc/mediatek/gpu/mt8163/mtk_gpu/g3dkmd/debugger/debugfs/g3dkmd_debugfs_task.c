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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>


#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dkmd_debugfs.h"
#include "g3dkmd_debugfs_helper.h"
#include "g3dkmd_task.h"
#include "g3dkmd_buffer.h"



/* debugfs - debuging */



static char *exeInstStateStrs[] = {
	"IDLE     ",
	"RUNNING  ",
	"WAITQUEUE",
	"INACTIVED"
};


/* Trigger */
#if defined(G3DKMD_ENABLE_TASK_EXEINST_MIGRATION)
static int trigger_debugfs_seq_show(struct seq_file *m, void *v)
{
	seq_printf(m, "[%s]: trigger [%s]\n", __func__, "_g3dkmd_create_exeInst_workqueue");

	{
		int taskID = 0;
		G3DKMD_BOOL isMigratingAllTasks = G3DKMD_FALSE;	/* G3DKMD_TRUE; */
		unsigned int boff_va_ovf_cnt = 0;
		unsigned int boff_bk_ovf_cnt = 0;
		unsigned int boff_tb_ovf_cnt = 0;
		unsigned int boff_hd_ovf_cnt = 0;

		g3dKmdTriggerCreateExeInst_workqueue(taskID, isMigratingAllTasks,
						     boff_va_ovf_cnt, boff_bk_ovf_cnt,
						     boff_tb_ovf_cnt, boff_hd_ovf_cnt);
	}

	return 0;
}


#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */

/* List exeInsts */
static int exeInsts_debugfs_seq_show(struct seq_file *m, void *v)
{
	int i, cnt;
	struct g3dExeInst *pInst = NULL;
	static const char * const executionModeStr[] = {
		"eG3dExecMode_single_queue",
		"eG3dExecMode_two_queues",
		"eG3dExecMode_per_task_queue",
	};


	seq_printf(m, "ExecuteMode: %s\n\n", executionModeStr[gExecInfo.exeuteMode]);

#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
	seq_printf(m, "Default Normal ExeInstIdx(): %u\n", g3dKmdGetNormalExeInstIdx());
	if (gExecInfo.exeuteMode == eG3dExecMode_two_queues)
		seq_printf(m, "Defalut Urgent ExeInstIdx(): %u\n", g3dKmdGetUrgentExeInstIdx());
#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */

	seq_puts(m, "- ExeInsts:\n");
	seq_puts(m, " *: working instance, -: default instance\n");
	seq_puts(m, " ------------------------------------------------------------------\n");

	lock_g3d();
	{
		for (i = 0, cnt = 0; i < gExecInfo.exeInfoCount; i++) {
			G3DKMD_DEV_REG_PARAM param_inst = { 0 };
			G3DKMD_DEV_REG_PARAM *param = &param_inst;

			pInst = gExecInfo.exeList[i];

			if (NULL == pInst)
				continue;

			g3dKmdQueryBuffSizesFromInst(pInst, param);

			seq_printf(m, "  %c%c %d) %p %s, TaskRefCnt: %d, Type: %s, Ht(Hw%cSw):(%d, %d) isMigrating:%c(%d, %d) hawChnl: %d HtSlot(h/s/t): %d/%d hwempty(%c) htInst(hw,sw):va(%p, %p)\n\trdptr_g3dva:(hw: 0x%x sw: 0x%x)\n\treadyptr_g3dva:(hw: 0x%x sw: 0x%x)\n\twtptr_g3dva:(hw: 0x%x sw: 0x%x)\n\tDfr(va,bk,tb,hd): (%d %d %d %d)\n",
#if defined(G3DKMD_ENABLE_TASK_EXEINST_MIGRATION)
				(g3dKmdGetNormalExeInstIdx() == i || g3dKmdGetUrgentExeInstIdx() == i) ? '-' : ' ',
				(gExecInfo.currentExecInst == pInst) ? '*' : ' ',
				i, pInst,
				exeInstStateStrs[pInst->state], pInst->refCnt,
				pInst->isUrgentPrio ? "Urgent" : "Normal",
				(pInst->pHtDataForHwRead == pInst->pHtDataForSwWrite) ? '=' : ',',
				-1,/* g3dKmdGetQueuedHtSlots(G3dKMD_HWCHANNEL_USE_LOCAL_RDPTR,
								pInst->pHtDataForHwRead->htInfo), */
				-1,/* g3dKmdGetQueuedHtSlots(G3dKMD_HWCHANNEL_USE_LOCAL_RDPTR,
								pInst->pHtDataForSwWrite->htInfo), */
				(pInst->childInstIdx == EXE_INST_NONE) ? 'N' : 'Y',
				pInst->childInstIdx,
				pInst->parentInstIdx,
#else
				' ',
				(gExecInfo.currentExecInst == pInst) ? '*' : ' ',
				i, pInst,
				exeInstStateStrs[pInst->state], pInst->refCnt,
				pInst->isUrgentPrio ? "Urgent" : "Normal",
				'=',
				-1,
				-1,
				'N',
				EXE_INST_NONE,
				EXE_INST_NONE,
#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */
				pInst->hwChannel,
				(int)(MAX_HT_COMMAND * HT_CMD_SIZE) -
				g3dKmdCheckFreeHtSlots(pInst->hwChannel,
							pInst->pHtDataForHwRead->htInfo),
				(int)(MAX_HT_COMMAND * HT_CMD_SIZE) -
				g3dKmdCheckFreeHtSlots(pInst->hwChannel,
							pInst->pHtDataForSwWrite->htInfo),
				   g3dKmdIsHtInfoEmpty(pInst->hwChannel,
							pInst->pHtDataForHwRead->htInfo) ? 'Y' : 'N',
				pInst->pHtDataForHwRead,
				pInst->pHtDataForSwWrite,
				pInst->pHtDataForHwRead->htInfo->rdPtr,
				pInst->pHtDataForSwWrite->htInfo->rdPtr,
				pInst->pHtDataForHwRead->htInfo->wtPtr,
				pInst->pHtDataForSwWrite->htInfo->wtPtr,
				pInst->pHtDataForHwRead->htInfo->readyPtr,
				pInst->pHtDataForSwWrite->htInfo->readyPtr,
				param->dfrVarySize,
				param->dfrBinbkSize,
				param->dfrBintbSize,
				param->dfrBinhdSize);
			++cnt;
		}
		seq_puts(m, " ------------------------------------------------------------------\n");
		seq_printf(m, "- ExeInst.count: %d, exeInfoCount: %d\n", cnt, gExecInfo.exeInfoCount);

	}
	unlock_g3d();

	return 0;
}

/* List Hw Status */
static int hwStatus_debugfs_seq_show(struct seq_file *m, void *v)
{
	int idx;

	seq_puts(m, "- HW Status:\n");
	seq_puts(m, " ------------------------------------------------------------------\n");
	seq_puts(m, " channel | state   | InstIdx\n");

	for (idx = 0; idx < NUM_HW_CMD_QUEUE; ++idx) {
		seq_printf(m, " %d)       %s %d\n",
			   idx, exeInstStateStrs[gExecInfo.hwStatus[idx].state],
			   gExecInfo.hwStatus[idx].inst_idx);
	}

	seq_puts(m, " ------------------------------------------------------------------\n");

	return 0;
}

/* List Tasks */
static int tasks_debugfs_seq_show(struct seq_file *m, void *v)
{
	int i, cnt;
	struct g3dTask *pTask = NULL;
	g3dFrameSwData *ptr = (g3dFrameSwData *) gExecInfo.SWRdbackMem.data;

	seq_puts(m, "- Tasks:\n");
	seq_puts(m, " ------------------------------------------------------------------\n");

	lock_g3d();
	{
		for (i = 0, cnt = 0; i < gExecInfo.taskInfoCount; i++) {
			pTask = gExecInfo.taskList[i];
			if (NULL == pTask)
				continue;

			seq_printf(m,
				" id:%u, inst: %u, pid: %u tid: %u, comm: %s, isUrgent: %c, lastFlushStamp: %u, HWStamp: %u, Carry: %u\n",
				pTask->task_idx, pTask->inst_idx,
				(unsigned int)pTask->pid, (unsigned int)pTask->tid,
				pTask->task_comm,
				(pTask->isUrgentPrio) ? 'Y' : 'N',
				pTask->lastFlushStamp, (ptr) ? ptr[i].stamp : (unsigned int)-1, pTask->carry);
			++cnt;
		}
		seq_puts(m, " ------------------------------------------------------------------\n");
		seq_printf(m, "- Task.count: %d, taskInfoCount: %d\n", cnt, gExecInfo.taskInfoCount);
	}
	unlock_g3d();

	return 0;
}

#if defined(KERNEL_FENCE_DEFER_WAIT)

static int fencewaiter_debugfs_seq_show(struct seq_file *m, void *v)
{
	int i, cnt;
	struct g3dExeInst *pInst = NULL;

	seq_puts(m, "- Fence Waiter:\n");
	seq_puts(m, " ------------------------------------------------------------------\n");

	lock_g3d();
	{
	for (i = 0, cnt = 0; i < gExecInfo.exeInfoCount; i++) {
		pInst = gExecInfo.exeList[i];
		if (NULL == pInst)
			continue;

		seq_printf(m, "  Inst: %d\n", i);
		g3dkmd_fence_PrintFenceWaiter(m, pInst);

		++cnt;
	}
	seq_puts(m, " ------------------------------------------------------------------\n");
	seq_printf(m, "- ExeInst.count: %d, exeInfoCount: %d\n", cnt, gExecInfo.exeInfoCount);
	}
	unlock_g3d();

	return 0;
}

#endif
/* =================================================================== */
/* Debugfs */



void g3dKmd_debugfs_task_create(struct dentry *task_dir)
{
	_g3dKmdCreateDebugfsFileNode("exeInsts", task_dir, exeInsts_debugfs_seq_show);
	_g3dKmdCreateDebugfsFileNode("tasks", task_dir, tasks_debugfs_seq_show);
	_g3dKmdCreateDebugfsFileNode("HwChannel", task_dir, hwStatus_debugfs_seq_show);
#if defined(G3DKMD_ENABLE_TASK_EXEINST_MIGRATION)
	_g3dKmdCreateDebugfsFileNode("trigger0", task_dir, trigger_debugfs_seq_show);
#endif
#if defined(KERNEL_FENCE_DEFER_WAIT)
	_g3dKmdCreateDebugfsFileNode("waiter", task_dir, fencewaiter_debugfs_seq_show);
#endif
}

void g3dKmd_debugfs_task_destroy(void)
{

}
