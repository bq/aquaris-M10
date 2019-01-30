
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

#include "ddp_cmdq.h"
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include "ddp_drv.h"
#include "ddp_reg.h"
#include "ddp_cmdq_reg.h"
#include "ddp_cmdq_record.h"

#ifdef __CMDQ_SECURE_PATH_SUPPORT__
#include "tz_cmdq.h"
#endif
/* use CMDQ THREAD 13 EXEC_CMD_CNT as test register currently */
#define TEST_REGISTER_VA (DISPSYS_CMDQ_BASE + 0x7A0)
#define TEST_REGISTER_PA (0x1400F000+0x7A0)


#include "ddp_cmdq_sec.h"

typedef struct {
	pid_t open_pid;
	pid_t open_tgid;
	unsigned int u4LockedMutex;
	spinlock_t node_lock;

	/* secure context data */
	cmdqSecContextHandle secCtxHandle;
} cmdq_proc_node_struct;


static struct proc_dir_entry *gCmdqTestProcEntry;
static DEFINE_MUTEX(gCmdqTestProcLock);
static int32_t gCmdqTestConfig[2] = {0, 0};  /* {normal, secure} */


/* function ptr for testcase */
typedef void (*CmdqTestCasePtr)(bool);

int32_t cmdq_test_process_context_init(uint32_t pid)
{
	cmdqSecContextHandle secCtxHandle = NULL;

	secCtxHandle = cmdq_sec_acquire_context_handle(pid);
	return 0;
}

int32_t cmdq_test_process_context_deinit(uint32_t pid)
{
	cmdq_sec_release_context_handle(pid);
	return 0;
}

/*-------------------------------------------------------------------------------------- */


/*-------------------------------------------------------------------------------------- */

/* __CMDQ_CMD_STATISTIC__
* To gater the operation usage in command block, and the statistic info will be shown in /proc/cmdq/record
*
* How to enable:
* a) define in __CMDQ_CMD_STATISTIC__ in cmdq header
* b) extern CommandStatsStruct in cmdq header, and CommandStatsStruct stats at the end ofRecordStruct
* c) extern void cmdq_gather_cmd_statistic(RecordStruct *pRecord, TaskStruct *pTask)
* d) call cmdq_gather_cmd_statistic before task release
*/
#ifdef __CMDQ_CMD_STATISTIC__

typedef struct CommandStatsStruct {
	uint32_t op[7];
} CommandStatsStruct;

uint32_t cmdq_get_op_statistic_id(CMDQ_CODE_ENUM code)
{
	switch (code) {
	case CMDQ_CODE_READ:
		return 0;
	case CMDQ_CODE_MOVE:
		return 1;
	case CMDQ_CODE_WRITE:
		return 2;
	case CMDQ_CODE_POLL:
		return 3;
	case CMDQ_CODE_JUMP:
		return 4;
	case CMDQ_CODE_WFE:
		return 5;
	case CMDQ_CODE_EOC:
		return 6;
	default:
		return 7;
	}

}

void cmdq_gather_cmd_statistic(RecordStruct *pRecord, TaskStruct *pTask)
{
	const uint32_t TOTAL_OP_CODE = cmdq_get_op_statistic_id(CMDQ_CODE_EOC) + 1;
	uint32_t op_statistic[TOTAL_OP_CODE];
	uint32_t instructionCount;
	uint32_t i;
	uint32_t *pCMD;
	uint32_t indexA;
	uint32_t indexB;
	uint32_t op;
	uint32_t statisticId;

	CMDQ_MSG("-->statistic: start");
	do {
		if (NULL == pTask || NULL == pRecord) {
			CMDQ_ERR("task[0x%x], record[0x%x]", pTask, pRecord);
			break;
		}

		for (i = 0; i < TOTAL_OP_CODE; i++)
			op_statistic[i] = 0;

		instructionCount = (pTask->blockSize) >> 3;
		for (i = 0; i < instructionCount; i++) {
			pCMD	= pTask->pVABase;
			indexA = i*2 + 1;
			indexB = i*2;
			op = (pCMD[indexA] & 0xFF000000) >> 24;
			statisticId = cmdq_get_op_statistic_id(op);
			op_statistic[statisticId] += 1;
		}

		/* update to record */
		for (i = 0; i < TOTAL_OP_CODE; i++)
			pRecord->stats.op[i] = op_statistic[i];

	} while (0);

	CMDQ_MSG("<--statistic: done. SCE: %d, instru_count: %d, op: %d, %d, %d, %d, %d, %d, %d\n",
		pTask->scenario,
		instructionCount,
		op_statistic[0], op_statistic[1], op_statistic[2],
		op_statistic[3], op_statistic[4], op_statistic[5], op_statistic[6]);
}
#endif

/*-------------------------------------------------------------------------------------- */
#if 0
static int32_t _test_submit_async(cmdqRecHandle handle, TaskStruct **ppTask)
{
	/* not suppot ASYNC  */
	return -1;
}
static void testcase_errors(void)
{
#if 0
	cmdqRecHandle hReq;
	cmdqRecHandle hLoop;
	TaskStruct *pTask;
	int32_t ret = 0;
	const uint32_t MMSYS_DUMMY_REG = MMSYS_CONFIG_BASE + 0x0890;

	do {
		/* SW timeout */
		CMDQ_MSG("%s line:%d\n", __func__, __LINE__);

		cmdqRecCreate(CMDQ_SCENARIO_TRIGGER_LOOP, &hLoop);
		cmdqRecReset(hLoop);
		cmdqRecPoll(hLoop, IO_VIRT_TO_PHYS(MMSYS_DUMMY_REG), 1, 0xFFFFFFFF);
		cmdqRecStartLoop(hLoop);

		CMDQ_MSG("=============== INIFINITE Wait ===================\n");

		CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_UPD, CMDQ_ENG_MDP_TDSHP0);
		cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &hReq);

		/* turn on ALL engine flag to test dump */
		for (ret = 0; ret < CMDQ_MAX_ENGINE_COUNT; ++ret)
			hReq->engineFlag |= 1 << ret;

		cmdqRecReset(hReq);
		cmdqRecWait(hReq, CMDQ_ENG_MDP_TDSHP0);
		cmdqRecFlush(hReq);

		CMDQ_MSG("=============== INIFINITE JUMP ===================\n");

		/* HW timeout */
		CMDQ_MSG("%s line:%d\n", __func__, __LINE__);
		CMDQ_REG_SET32(CMDQ_SYNC_TOKEN_UPD, CMDQ_ENG_MDP_TDSHP0);
		cmdqRecReset(hReq);
		cmdqRecWait(hReq, CMDQ_ENG_MDP_TDSHP0);
		cmdq_append_command(hReq, CMDQ_CODE_JUMP, 0, 8);	/* JUMP to connect tasks */
		ret = _test_submit_async(hReq, &pTask);
		msleep_interruptible(500);
		ret = cmdqCoreWaitAndReleaseTask(pTask, 8000);

		CMDQ_MSG("================POLL INIFINITE====================\n");
		CMDQ_REG_SET32(MMSYS_DUMMY_REG, 0x0);
		cmdqRecReset(hReq);
		cmdqRecPoll(hReq, IO_VIRT_TO_PHYS(MMSYS_DUMMY_REG), 1, 0xFFFFFFFF);
		cmdqRecFlush(hReq);

		CMDQ_MSG("=================INVALID INSTR=================\n");

		/* invalid instruction */
		CMDQ_MSG("%s line:%d\n", __func__, __LINE__);
		cmdqRecReset(hReq);
		cmdq_append_command(hReq, CMDQ_CODE_JUMP, -1, 0);
		cmdqRecFlush(hReq);
	} while (0);

	cmdqRecDestroy(hReq);
	cmdqRecDestroy(hLoop);
	CMDQ_MSG("testcase_errors done\n");
	return;

#endif

}



static void testcase_scenario(bool isSecure)
{
	cmdqRecHandle hRec;
	int32_t ret;
	int i = 0;

	CMDQ_LOG("[CMDQ] %s START\n", __func__);

	/* make sure each scenario runs properly with empty commands */
	for (i = 0; i < CMDQ_MAX_SCENARIO_COUNT; ++i) {
		if (CMDQ_SCENARIO_MDP_STREAM_UNKNOWN <= i && CMDQ_SCENARIO_MDP_STREAM_ISP_VSS >= i) {
			/* bypass user scenario */
			continue;
		}
		CMDQ_MSG("testcase_scenario id:%d\n", i);
		cmdqRecCreate((CMDQ_SCENARIO_ENUM)i, &hRec);
		cmdqRecReset(hRec);
		cmdqRecSetCommandSecure(hRec, isSecure);
		ret = cmdqRecFlush(hRec);
		cmdqRecDestroy(hRec);
	}

	CMDQ_LOG("[CMDQ] %s END\n", __func__);

}


static void testcase_eoc(bool isSecure)
{
	cmdqRecHandle handle;

	CMDQ_LOG("[CMDQ] %s START\n", __func__);

	cmdqRecCreate(CMDQ_SCENARIO_DEBUG, &handle);
	cmdqRecReset(handle);
	cmdqRecSetCommandSecure(handle, isSecure);
	cmdqRecFlush(handle);
	cmdqRecDestroy(handle);

	CMDQ_LOG("[CMDQ] %s END\n", __func__);
}



#endif

static void testcase_write(bool isSecure)
{
/*	int i; */
	cmdqRecHandle handle;
/*	int ret = 0; */
/*	const uint32_t DSI_REG = 0xF401b000; */
	const uint32_t PATTERN = 10;
	uint32_t value = 0;

	CMDQ_LOG("[CMDQ] %s START\n", __func__);


	/* set to 0xFFFFFFFF */
	CMDQ_ERR("harry >>>>>>>>>\n");
	DISP_REG_SET(TEST_REGISTER_VA, ~0);
	value = DISP_REG_GET(TEST_REGISTER_VA);
	if (value != ~0)
		CMDQ_ERR("CPU TEST FAIL: wrote value is 0x%08x, not 0x%08x\n", value, ~0);
	else
		CMDQ_LOG("CPU TEST ok: wrote value is 0x%08x, PATTERN 0x%08x\n", value, ~0);


	/* use CMDQ to set to PATTERN */
	cmdqRecCreate(CMDQ_SCENARIO_DEBUG, &handle);
	cmdqRecReset(handle);
	cmdqRecSetCommandSecure(handle, isSecure);

	cmdqRecWrite(handle, TEST_REGISTER_PA, PATTERN, ~0);
	cmdqRecFlush(handle);
	cmdqRecDestroy(handle);

	/* value check */
	value = DISP_REG_GET(TEST_REGISTER_VA);
	if (value != PATTERN)
		CMDQ_ERR("TEST FAIL: wrote value is 0x%08x, not 0x%08x\n", value, PATTERN);
	else
		CMDQ_LOG("TEST ok: wrote value is 0x%08x, PATTERN 0x%08x\n", value, PATTERN);

	CMDQ_LOG("[CMDQ] %s END\n", __func__);
}

#if 0
static void testcase_poll(bool isSecure)
{
/*	int i; */
	cmdqRecHandle handle;
	const uint32_t MMSYS_DUMMY_REG = DISP_REG_OVL_DUMMY;/*DISP_REG_CONFIG_MMSYS_DUMMY;  */

	uint32_t value = 0;
	uint32_t testReg = MMSYS_DUMMY_REG;
	uint32_t pollingVal = 0x00003001;

	CMDQ_LOG("[CMDQ] %s START\n", __func__);


	DISP_REG_SET(MMSYS_DUMMY_REG, ~0);

	/* it's too slow that set value after enable CMDQ  */
	/* sw timeout will be hanppened before CPU schedule to set value..., so we set value here  */
	DISP_REG_SET(MMSYS_DUMMY_REG, pollingVal);

	cmdqRecCreate(CMDQ_SCENARIO_DEBUG, &handle);
	cmdqRecReset(handle);
	cmdqRecSetCommandSecure(handle, isSecure);

	cmdqRecPoll(handle, IO_VIRT_TO_PHYS(testReg), pollingVal, ~0);

	cmdqRecFlush(handle);
	cmdqRecDestroy(handle);

	value = DISP_REG_GET(testReg);
	CMDQ_LOG("[CMDQ] polling target value is 0x%08x\n", value);

	CMDQ_LOG("[CMDQ] %s END\n", __func__);
}


static void testcase_infinite_timeout(bool isSecure, uint32_t max_retry_count)
{
	 int i;
	 cmdqRecHandle handle;
	 const uint32_t MMSYS_DUMMY_REG = DISP_REG_OVL_DUMMY;

	 uint32_t value = 0;
	 uint32_t testReg = MMSYS_DUMMY_REG;
	 uint32_t pollingVal = 0x00003001;
	 const uint32_t PATTERN = (1<<0) | (1<<2) | (1<<16); /*0x10005 */

	 CMDQ_LOG("[CMDQ] %s START\n", __func__);
	 for (i = 0 ; i < max_retry_count; i++) {
		CMDQ_LOG("[CMDQ] retry: %d, max_retry_count: %d\n", i, max_retry_count);

		/* clean and let timeout happened  */
		DISP_REG_SET(MMSYS_DUMMY_REG, ~0);

		/* use CMDQ to poll result */
		cmdqRecCreate(CMDQ_SCENARIO_DEBUG, &handle);
		cmdqRecReset(handle);
		cmdqRecSetCommandSecure(handle, isSecure);


		cmdqRecWrite(handle, IO_VIRT_TO_PHYS(testReg), PATTERN, ~0);
		cmdqRecPoll(handle, IO_VIRT_TO_PHYS(testReg), pollingVal, ~0);

		cmdqRecFlush(handle);
		cmdqRecDestroy(handle);

		value = DISP_REG_GET(testReg);
		CMDQ_LOG("[CMDQ] polling target value is 0x%08x\n", value);

		msleep(500);
	}
	CMDQ_LOG("[CMDQ] %s END\n", __func__);
}

#endif


#if 0

static int32_t _thread_func_write(void *data)
{
	allow_signal(SIGKILL);
	msleep(2000);
	testcase_write(false);
	return 0;
}


static int32_t _thread_func_infinite_timeout(void *data)
{
	allow_signal(SIGKILL);
	testcase_infinite_timeout(false, 1);
	msleep(1000*60 * 10);
	return 0;
}

static void testcase_remove_task_from_thread(void)
{
	struct task_struct *pKTh_1 = kthread_run(_thread_func_infinite_timeout, NULL, "cmdq_kthr_1");
	struct task_struct *pKTh_2 = kthread_run(_thread_func_write, NULL, "cmdq_kthr_2");
	struct task_struct *pKTh_3 = kthread_run(_thread_func_write, NULL, "cmdq_kthr_3");

	CMDQ_LOG("[CMDQ] %s start\n", __func__);

	if (IS_ERR(pKTh_1)) {
		CMDQ_LOG("%s, create pthread_1 failed\n", __func__);
		return;
	}



	if (IS_ERR(pKTh_2)) {
		CMDQ_LOG("%s, create pthread_2 failed\n", __func__);
		return;
	}



	if (IS_ERR(pKTh_3)) {
		CMDQ_LOG("%s, create pthread_3 failed\n", __func__);
		return;
	}

	CMDQ_LOG("%s, create pthread done, (%d, %d, %d)\n", __func__, pKTh_1->pid, pKTh_2->pid, pKTh_3->pid);
	CMDQ_LOG("[CMDQ] %s END\n", __func__);
	return;

}

#endif
/*-------------------------------------------------------------------------------------- */

extern int32_t cmdq_sec_submit_to_secure_world(
			uint32_t iwcCommand,
			TaskStruct *pTask,
			int32_t thread,
			CmdqSecFillIwcCB iwcFillCB);

int32_t cmdq_sec_fill_iwc_cb01(int32_t commandId, void *pWsm)
{
#ifdef __CMDQ_SECURE_PATH_SUPPORT__
	iwcCmdqMessage_t *pIwc = (iwcCmdqMessage_t *)pWsm;

	uint32_t i = 0;

	CMDQ_LOG("[CMDQ] cmdq_sec_fill_iwc_cb01....\n");
	if (NULL != pIwc) {
		const uint32_t cmdArrLength = CMDQ_IWC_MAX_CMD_LENGTH;

		for (i = 0; i < cmdArrLength;  i++)
			pIwc->pVABase[i] = i;

		pIwc->cmd = commandId;
		pIwc->scenario = CMDQ_SCENARIO_DEBUG;
		pIwc->thread = 8;
		pIwc->priority = 0;
		pIwc->engineFlag = 0x12345678;
		pIwc->blockSize = cmdArrLength*4;
	} else {
		CMDQ_LOG("[CMDQ] invalid IWC\n");
	}

#endif
	return 0;
}


/*-------------------------------------------------------------------------------------- */


#if 0
static int cmdq_dummy_proc(char *pPage, char **ppStart, off_t offset, int32_t count, int32_t *pEOF, void *pData)
{
#ifdef __CMDQ_SECURE_PATH_SUPPORT__
	CMDQ_LOG("//\n//\n//\cmdq_dummy_proc\n");
	cmdq_sec_submit_to_secure_world(TZCMD_CMDQ_TEST_DUMMY, NULL, CMDQ_INVALID_THREAD, NULL);
#endif
	return 0;
}

static int cmdq_test_proc(char *pPage, char **ppStart, off_t offset, int32_t count, int32_t *pEOF, void *pData)
{
	bool isSecure = false;
	const uint32_t MMSYS_DUMMY_REG = DISP_REG_OVL_DUMMY;
	uint32_t pollingVal = 0x00003001;

	CMDQ_LOG("//\n//\n//cmdq_test_proc, CMDQ\n");

	switch (gCmdqTestConfig[0]) {
	case 3:

			DISP_REG_SET(MMSYS_DUMMY_REG, pollingVal);
			CMDQ_LOG("[CMDQ] write MMSYS_DUMMY to 0x%08x\n", pollingVal);
		break;
	case 2:
		testcase_remove_task_from_thread();
		break;
	case 1:
		testcase_infinite_timeout(isSecure, 1);
		break;
	case 0:
	default:
		cmdq_test_process_context_init(current->pid);
		testcase_scenario(isSecure);
		testcase_eoc(isSecure);
		testcase_write(isSecure);
		testcase_poll(isSecure);
		cmdq_test_process_context_deinit(current->pid);
		break;
	}

	return 0;

}

static int cmdq_test_smi_proc(char *pPage, char **ppStart, off_t offset, int32_t count, int32_t *pEOF, void *pData)
{
	CMDQ_LOG("//\n//\n//\%s\n", __func__);

#ifdef __CMDQ_SECURE_PATH_SUPPORT__
	CMDQ_LOG("//\n//\n//\[CMDQ] sec test....SMI dump\n");
	cmdq_test_process_context_init(current->pid);
	cmdq_sec_submit_to_secure_world(TZCMD_CMDQ_TEST_SMI_DUMP, NULL, CMDQ_INVALID_THREAD, NULL);
	cmdq_test_process_context_deinit(current->pid);
#endif

	return 0;
}
#endif

/* cmdq_proc_node_struct *pNode = NULL; */
#if 0
static int cmdq_test_create_ctx_handle(void)
{
	cmdqSecContextHandle secCtxHandle = NULL;

	CMDQ_MSG("enter cmdq_test_create_ctx_handle process:%s\n", current->comm);

	/* get porcess secure handle */
	secCtxHandle = cmdq_sec_acquire_context_handle(current->tgid);

	/*Allocate and initialize private data */
	pNode = kmalloc(sizeof(cmdq_proc_node_struct), GFP_ATOMIC);
	if (NULL == pNode) {
		CMDQ_MSG("pNode kmalloc failed!\n");
		cmdq_sec_release_context_handle(current->tgid);
		return -ENOMEM;
	}
	pNode->open_pid = current->pid;
	pNode->open_tgid = current->tgid;
	pNode->u4LockedMutex = 0;
	pNode->secCtxHandle = secCtxHandle;
	return 0;
}

static int cmdq_test_release_ctx_handle(void)
{
/*	unsigned int index = 0; */

	CMDQ_MSG("enter cmdq_test_release_ctx_handle() process:%s\n", current->comm);
	cmdq_sec_release_context_handle(current->tgid);

	if (NULL != pNode) {
		kfree(pNode);
		pNode = NULL;
	}

	return 0;
}
#endif

#if 0
static int cmdq_test_sec_proc(char *pPage, char **ppStart, off_t offset, int32_t count, int32_t *pEOF, void *pData)
{
	bool isSecure = true;


	const uint32_t testCount = 4;
	CmdqTestCasePtr tests[4] = {
							   testcase_eoc,
							   testcase_scenario,
							   testcase_write,
							   testcase_poll};
	uint32_t i = 0;

	CMDQ_LOG("//\n//\n//cmdq_test_proc, CMDQ sec, test id:%d\n", gCmdqTestConfig[1]);

	/*cmdq_test_create_ctx_handle(); */
	cmdq_test_process_context_init(current->pid);

	switch (gCmdqTestConfig[1]) {
	case 0:
		testcase_scenario(isSecure);
		testcase_eoc(isSecure);
		testcase_write(isSecure);
		testcase_poll(isSecure);
		break;
	case 1:
		#ifdef __CMDQ_SECURE_PATH_SUPPORT__
		/* iwc test... */
		CMDQ_LOG("[CMDQ_TEST]cmdq_test_iwc_proc\n");
		CmdqSecFillIwcCB cb = cmdq_sec_fill_iwc_cb01;

		cmdq_sec_submit_to_secure_world(TZCMD_CMDQ_TEST_TASK_IWC, NULL, CMDQ_INVALID_THREAD, (cb));
		#endif
		break;
	case 2:
		testcase_eoc(isSecure);
		break;
	case 3:
		testcase_scenario(isSecure);
		break;
	case 4:
		CMDQ_LOG("[CMDQ_TEST]testcase_write\n");
		testcase_write(isSecure);
		break;
	case 5:
		testcase_poll(isSecure);
		break;
	case 100:
		for (i = 0; i < testCount; i++) {
			CmdqTestCasePtr pTest = tests[i];
			(*pTest)(isSecure);
		}
		break;
	case 99:
		#ifdef __CMDQ_SECURE_PATH_SUPPORT__
		CMDQ_LOG("//\n//\n//\[CMDQ] sec test....SMI dump\n");
		/*cmdq_test_process_context_init(current->pid); */
		cmdq_sec_submit_to_secure_world(TZCMD_CMDQ_TEST_SMI_DUMP, NULL, CMDQ_INVALID_THREAD, NULL);
		/*cmdq_test_process_context_deinit(current->pid); */
		#endif
		break;

	} while (0);

	/*cmdq_test_release_ctx_handle(); */
	cmdq_test_process_context_deinit(current->pid);

	return 0;

}
#endif



#if 0
static int cmdq_test_file_open(struct inode *pInode, struct file  *file)
{
	cmdq_proc_node_struct *pNode = NULL;
	cmdqSecContextHandle secCtxHandle = NULL;

	CMDQ_MSG("enter cmdq_test_file_open() process:%s\n", current->comm);

	/* get porcess secure handle */
	secCtxHandle = cmdq_sec_acquire_context_handle(current->tgid);

	/*Allocate and initialize private data */
	file->private_data = kmalloc(sizeof(cmdq_proc_node_struct), GFP_ATOMIC);
	if (NULL == file->private_data) {
		CMDQ_MSG("Not enough entry for cmdq open operation\n");
		cmdq_sec_release_context_handle(current->tgid);
		return -ENOMEM;
	}

	pNode = (cmdq_proc_node_struct *)file->private_data;
	pNode->open_pid = current->pid;
	pNode->open_tgid = current->tgid;
	pNode->u4LockedMutex = 0;
	pNode->secCtxHandle = secCtxHandle;
	spin_lock_init(&pNode->node_lock);
	return 0;
}
#endif

static int cmdq_test_open(struct inode *pInode, struct file *pFile)
{
	return 0;
}


ssize_t cmdq_read_test_config_proc(struct file *fp, char __user *u, size_t s, loff_t *l)
{
	/* int32_t len = 0; */
	bool isSecure = false;
	int32_t testID = -1;

	CMDQ_LOG("enter read /proc/cmdq_test/test\n");
#if 0
	mutex_lock(&gCmdqTestProcLock);
	smp_mb();

	len = sprintf(pPage, "[Current config]\n Normal test: %d, Secure test: %d\n",
		gCmdqTestConfig[0], gCmdqTestConfig[1]);
	mutex_unlock(&gCmdqTestProcLock);
#endif
	if (gCmdqTestConfig[0] != -1 && gCmdqTestConfig[1] != -1) {
		CMDQ_ERR("set testID error[%d]\n", __LINE__);
		return 0;
	}

	if (gCmdqTestConfig[0] != -1) {
		isSecure = false;
		testID = gCmdqTestConfig[0];
	} else if (gCmdqTestConfig[1] != -1) {
		isSecure = true;
		testID = gCmdqTestConfig[1];
	}


	switch (testID) {
	case 0:
		CMDQ_LOG("harry >>> enter testcase_write\n");
		testcase_write(isSecure);
		break;
	default:
		CMDQ_ERR("wrong testID %d\n", __LINE__);
		break;
	}

	return 0;
}



static int32_t cmdq_write_test_config_proc(struct file *file,
									 const char __user *userBuf,
									 size_t count,
									 loff_t *data)
{
	char desc[10];
	int32_t testType = -1;
	int32_t newTestSuit = -1;
	int32_t len = 0;

	CMDQ_LOG("enter write /proc/cmdq_test/test\n");
	/* copy user input */
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, userBuf, count)) {
		CMDQ_LOG("CMDQ, copy data fail, line=%d", __LINE__);
		return 0;
	}
	desc[len] = '\0';

	/* process and update config */
	if (0 >= sscanf(desc, "%d %d", &testType, &newTestSuit)) {
		/* sscanf returns the number of items in argument list successfully filled.  */
		CMDQ_LOG("CMDQ, sscanf failed, line=%d", __LINE__);
		return 0;
	}

	if ((0 > testType) || (2 < testType) || (-1 == newTestSuit)) {
		CMDQ_LOG("CMDQ, testType = %d, newTestSuit=%d", testType, newTestSuit);
		return 0;
	}

	mutex_lock(&gCmdqTestProcLock);
	smp_mb();
	gCmdqTestConfig[0] = -1;
	gCmdqTestConfig[1] = -1;
	gCmdqTestConfig[testType] = newTestSuit;
	mutex_unlock(&gCmdqTestProcLock);
	CMDQ_ERR("gCmdqTestConfig[0] = %d,gCmdqTestConfig[1] = %d\n",
		gCmdqTestConfig[0],
		gCmdqTestConfig[1]);
	return count;
}


static const struct file_operations cmdq_fops = {
	.owner = THIS_MODULE,
	.open = cmdq_test_open,
	.read = cmdq_read_test_config_proc,
	.write = cmdq_write_test_config_proc
};



static int __init cmdq_test_init(void)
{
	/* struct proc_dir_entry *pEntry = NULL; */

	CMDQ_LOG("[CMDQ] cmdq_test_init\n");

	/* Mout proc entry for debug */
	gCmdqTestProcEntry = proc_mkdir("cmdq_test", NULL);
	/* create sub entry  */
	if (NULL != gCmdqTestProcEntry) {
		if (NULL == proc_create("test", 0660, gCmdqTestProcEntry, &cmdq_fops))
			CMDQ_ERR("create /proc/cmdq_test/test failed\n");


		#if 0
		if (NULL != pEntry)
			pEntry->read_proc = cmdq_test_proc;
		pEntry = create_proc_entry("config", 0660, gCmdqTestProcEntry);
		if (NULL != pEntry) {
			pEntry->read_proc = cmdq_read_test_config_proc;
			pEntry->write_proc = cmdq_write_test_config_proc;
		}

		pEntry = create_proc_entry("sec", 0660, gCmdqTestProcEntry);
		if (NULL != pEntry)
			pEntry->read_proc = cmdq_test_sec_proc;


		pEntry = create_proc_entry("smi", 0660, gCmdqTestProcEntry);
		if (NULL != pEntry)
			pEntry->read_proc = cmdq_test_smi_proc;

		pEntry = create_proc_entry("dummy", 0660, gCmdqTestProcEntry);
		if (NULL != pEntry)
			pEntry->read_proc = cmdq_dummy_proc;
		#endif

	} else {
		CMDQ_ERR("create /proc/cmdq_test error\n");
	}
	return 0;
}

static void __exit cmdq_test_exit(void)
{
	CMDQ_LOG("[CMDQ] cmdq_test_exit\n");
	if (NULL != gCmdqTestProcEntry) {
		remove_proc_entry("config", gCmdqTestProcEntry);
		remove_proc_entry("test", gCmdqTestProcEntry);
		remove_proc_entry("sec", gCmdqTestProcEntry);
		remove_proc_entry("smi", gCmdqTestProcEntry);
		remove_proc_entry("dummy", gCmdqTestProcEntry);

		remove_proc_entry("cmdq_test", NULL);
		gCmdqTestProcEntry = NULL;
	}
}




module_init(cmdq_test_init);
module_exit(cmdq_test_exit);

MODULE_LICENSE("GPL");

