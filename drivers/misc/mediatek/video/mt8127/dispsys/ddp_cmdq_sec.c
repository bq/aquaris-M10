
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

#include "ddp_cmdq_sec.h"


static atomic_t gDebugSecSwCopy = ATOMIC_INIT(0);
static atomic_t gDebugSecCmdId = ATOMIC_INIT(0);

static DEFINE_MUTEX(gCmdqSecExecLock);	   /* lock to protect atomic secure task execution */
static DEFINE_MUTEX(gCmdqSecContextLock);	/* lock to protext atomic access gCmdqSecContextList */
static struct list_head gCmdqSecContextList; /* secure context list. note each porcess has its own sec context */

/* function declaretion */
/* cmdqSecContextHandle cmdq_sec_context_handle_create(uint32_t tgid); */

#if defined(__CMDQ_SECURE_PATH_SUPPORT__)

/* the API for other code to acquire cmdq_mem session handle */
/* a NULL handle is returned when it fails */
KREE_SESSION_HANDLE cmdq_session_handle(void)
{
	CMDQ_MSG("cmdq_session_handle() acquire TEE session\n");
	if (NULL == cmdq_session) {
		TZ_RESULT ret;

		CMDQ_MSG("cmdq_session_handle() create session\n");
		ret = KREE_CreateSession(TZ_TA_CMDQ_UUID, &cmdq_session);
		if (ret != TZ_RESULT_SUCCESS) {
			CMDQ_ERR("cmdq_session_handle() failed to create session, ret=%d\n", ret);
			return NULL;
		}
	}

	CMDQ_MSG("cmdq_session_handle() session=%x\n", (unsigned int)cmdq_session);
	return cmdq_session;
}

KREE_SESSION_HANDLE cmdq_mem_session_handle(void)
{
	CMDQ_MSG("cmdq_mem_session_handle() acquires TEE memory session\n");
	if (NULL == cmdq_mem_session) {
		TZ_RESULT ret;

		CMDQ_MSG("cmdq_mem_session_handle() create memory session\n");
		ret = KREE_CreateSession(TZ_TA_MEM_UUID, &cmdq_mem_session);
		if (ret != TZ_RESULT_SUCCESS) {
			CMDQ_ERR("cmdq_mem_session_handle() failed to create session: ret=%d\n", ret);
			return NULL;
		}
	}

	CMDQ_MSG("cmdq_mem_session_handle() session=%x\n", (unsigned int)cmdq_mem_session);
	return cmdq_mem_session;
}

static int32_t cmdq_sec_setup_context_session(cmdqSecContextHandle handle)
{
	int32_t status = 0;

	/* alloc message bufer */
	handle->iwcMessage = kmalloc(sizeof(uint8_t *) * (sizeof(iwcCmdqMessage_t)), GFP_KERNEL);
	if (NULL == handle->iwcMessage) {
		CMDQ_ERR("handle->iwcMessage kmalloc failed!\n");
		return -ENOMEM;
	}
	CMDQ_MSG("handle->iwcMessage 0x%x\n", (uint32_t)handle->iwcMessage);
	/*init session handle */
	handle->sessionHandle = cmdq_session_handle();
	handle->memSessionHandle = cmdq_mem_session_handle();
	CMDQ_MSG("handle->sessionHandle 0x%x\n", (uint32_t)handle->sessionHandle);
	CMDQ_MSG("handle->memSessionHandle 0x%x\n", (uint32_t)handle->memSessionHandle);
	return status;
}


static void cmdq_sec_deinit_session_unlocked(cmdqSecContextHandle handle)
{

	CMDQ_MSG("[SEC]-->SESSION_DEINIT\n");
	do {
		if (NULL != handle->iwcMessage) {
			kfree(handle->iwcMessage);
			handle->iwcMessage = NULL;
		}

	} while (0);

	CMDQ_MSG("[SEC]<--SESSION_DEINIT\n");
}


static int32_t cmdq_sec_fill_iwc_buffer_unlocked(
			iwcCmdqMessage_t *pIwc,
			uint32_t iwcCommand,
			TaskStruct *pTask,
			int32_t thread,
			CmdqSecFillIwcCB iwcFillCB)
{
	int32_t status = 0;

	CMDQ_MSG("[SEC]-->SESSION_MSG: cmdId[%d]\n", iwcCommand);

	/* fill message buffer for inter world communication */
	memset(pIwc, 0x0, sizeof(iwcCmdqMessage_t));
	if (NULL != pTask && CMDQ_INVALID_THREAD != thread) {
		/* basic data */
		pIwc->cmd			 = iwcCommand;
		pIwc->scenario		= pTask->scenario;
		pIwc->thread		  = thread;
		pIwc->priority		= pTask->priority;
		pIwc->engineFlag	  = pTask->engineFlag;
		pIwc->blockSize	   = pTask->blockSize;
		memcpy((pIwc->pVABase), (pTask->pVABase), (pTask->blockSize));

		/* metadata */
		CMDQ_MSG("[SEC]SESSION_MSG: addrList[%d][0x%p], portList[%d][0x%p]\n",
			pTask->secData.addrListLength,
			pTask->secData.addrList,
			pTask->secData.portListLength,
			pTask->secData.portList);
		if (0 < pTask->secData.addrListLength) {
			pIwc->metadata.addrListLength = pTask->secData.addrListLength;
			memcpy((pIwc->metadata.addrList),
				(pTask->secData.addrList),
				(pTask->secData.addrListLength)*sizeof(iwcCmdqAddrMetadata_t));
		}

		if (0 < pTask->secData.portListLength) {
			pIwc->metadata.portListLength = pTask->secData.portListLength;
			memcpy((pIwc->metadata.portList),
				(pTask->secData.portList),
				(pTask->secData.portListLength)*sizeof(iwcCmdqPortMetadata_t));
		}

		/* medatada: debug config */
		pIwc->metadata.debug.logLevel = (cmdq_core_should_print_msg()) ? (1) : (0);
	} else if (NULL != iwcFillCB) {
		status = (*iwcFillCB)(iwcCommand, (void *)pIwc);
	} else {
		/* relase resource, or debug function will go here */
		CMDQ_MSG("[SEC]-->SESSION_MSG: no task, cmdId[%d]\n", iwcCommand);
		pIwc->cmd = iwcCommand;
		pIwc->blockSize = 0;
		pIwc->metadata.addrListLength = 0;
		pIwc->metadata.portListLength = 0;
	}

	CMDQ_MSG("[SEC]<--SESSION_MSG[%d]\n", status);
	return status;
}


static int32_t cmdq_sec_execute_session_unlocked(cmdqSecContextHandle handle)
{
	TZ_RESULT tzRes;

	CMDQ_PROF_START("CMDQ_SEC_EXE");

	do {
		/* Register share memory */
		MTEEC_PARAM cmdq_param[4];
		unsigned int paramTypes;
		KREE_SHAREDMEM_HANDLE cmdq_share_handle = 0;
		KREE_SHAREDMEM_PARAM  cmdq_shared_param;

		cmdq_shared_param.buffer = handle->iwcMessage;
		cmdq_shared_param.size = sizeof(uint8_t *) * (sizeof(iwcCmdqMessage_t));
		CMDQ_MSG("cmdq_shared_param.buffer %x\n", (uint32_t)cmdq_shared_param.buffer);

		tzRes = KREE_RegisterSharedmem(handle->memSessionHandle, &cmdq_share_handle, &cmdq_shared_param);
		if (tzRes != TZ_RESULT_SUCCESS) {
			CMDQ_ERR("cmdq register share memory Error: %d, line:%d, cmdq_mem_session_handle(%x)\n",
				tzRes, __LINE__, (unsigned int)(handle->memSessionHandle));
			return tzRes;
		}

		/* KREE_Tee service call */
		cmdq_param[0].memref.handle = (uint32_t)cmdq_share_handle;
		cmdq_param[0].memref.offset = 0;
		cmdq_param[0].memref.size = cmdq_shared_param.size;
		paramTypes = TZ_ParamTypes1(TZPT_MEMREF_INPUT);
		tzRes = KREE_TeeServiceCall(handle->sessionHandle, TZCMD_CMDQ_SUBMIT_TASK, paramTypes, cmdq_param);
		if (tzRes != TZ_RESULT_SUCCESS) {
			CMDQ_ERR("TZCMD_CMDQ_SUBMIT_TASK fail, ret=0x%x\n", tzRes);
			return tzRes;
		}
		CMDQ_MSG("KREE_TeeServiceCall tzRes =0x%x\n", tzRes);

		/* Unregister share memory */
		KREE_UnregisterSharedmem(handle->memSessionHandle, cmdq_share_handle);

		/* wait respond */

		/* config with timeout */

	} while (0);

	CMDQ_PROF_END("CMDQ_SEC_EXE");

	/* return tee service call result */
	return tzRes;
}


int32_t cmdq_sec_send_context_session_message(
			cmdqSecContextHandle handle,
			uint32_t iwcCommand,
			TaskStruct *pTask,
			int32_t thread,
			CmdqSecFillIwcCB iwcFillCB)
{
	int32_t status = 0;
	int32_t iwcRsp = 0;

	do {
		/* fill message bufer */
		status = cmdq_sec_fill_iwc_buffer_unlocked((iwcCmdqMessage_ptr)(handle->iwcMessage),
							iwcCommand, pTask, thread, iwcFillCB);
		if (0 > status)
			break;

		/* send message */
		status = cmdq_sec_execute_session_unlocked(handle);
		if (TZ_RESULT_SUCCESS != status) {
			CMDQ_ERR("cmdq_sec_execute_session_unlocked status is %d\n", status);
			break;
		}

		/* get secure task execution result */
		/*
			iwcRsp = ((iwcCmdqMessage_t*)(handle->iwcMessage))->rsp;
			status = iwcRsp; (0 == iwcRsp) ? (0):(-EFAULT);
		*/

		/* and then, update task state */
		if (pTask)
			pTask->taskState = (0 == iwcRsp) ? (TASK_STATE_DONE) : (TASK_STATE_ERROR);

		/* log print */
		if (0 < status)
			CMDQ_ERR("SEC_SEND: status[%d], cmdId[%d], iwcRsp[%d]\n", status, iwcCommand, iwcRsp);
		else
			CMDQ_MSG("SEC_SEND: status[%d], cmdId[%d], iwcRsp[%d]\n", status, iwcCommand, iwcRsp);
	} while (0);

	return status;
}


/*added only for cmdq unit test begin */
int32_t cmdq_sec_test_proc(int testValue)
{
	CMDQ_MSG("DISP_IOCTL_CMDQ_SEC_TEST cmdq_test_proc\n");
	const int32_t tgid = current->tgid;
	cmdqSecContextHandle handle = NULL;
	TZ_RESULT ret;

	do {
		/* find handle first */
		handle = cmdq_sec_find_context_handle_unlocked(tgid);
		if (NULL == handle) {
			CMDQ_ERR("SEC_SUBMIT: tgid %d err[NULL secCtxHandle]\n", tgid);
			ret = -(CMDQ_ERR_NULL_SEC_CTX_HANDLE);
			break;
		}

		/* fill param */
		MTEEC_PARAM param[4];
		unsigned int paramTypes;
		TZ_RESULT ret;

		param[0].value.a = (uint32_t)111;
		param[1].value.a = (uint32_t)testValue;

		paramTypes = TZ_ParamTypes2(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);

		ret = KREE_TeeServiceCall(cmdq_session_handle(), TZCMD_CMDQ_TEST_HELLO, paramTypes, param);
		if (ret != TZ_RESULT_SUCCESS)
			CMDQ_ERR("TZCMD_CMDQ_TEST_HELLO fail, ret=%x\n", ret);
		else
			CMDQ_MSG("KREE_TeeServiceCall OK\n");
		return ret;

	} while (0);

}
/*added only for cmdq unit test end */



static int32_t cmdq_sec_teardown_context_session(cmdqSecContextHandle handle)
{
	int32_t status = 0;

	if (handle) {
		CMDQ_MSG("SEC_TEARDOWN:iwcMessage:0x%p\n", handle->iwcMessage);
		cmdq_sec_deinit_session_unlocked(handle);
	} else {
		CMDQ_ERR("SEC_TEARDOWN: null secCtxHandle\n");
		status = -1;
	}
	return status;
}


int32_t cmdq_sec_submit_to_secure_world(
			uint32_t iwcCommand,
			TaskStruct *pTask,
			int32_t thread,
			CmdqSecFillIwcCB iwcFillCB)
{
	/* prevent nested lock gCmdqSecContextLock */
	const bool skipSecCtxDump = (TZCMD_CMDQ_RES_RELEASE == iwcCommand) ? (true) : (false);
	const int32_t tgid = current->tgid;
	cmdqSecContextHandle handle = NULL;
	int32_t status = 0;
	int32_t duration = 0;

	struct timeval tEntrySec;
	struct timeval tExitSec;

	mutex_lock(&gCmdqSecExecLock);
	smp_mb();

	CMDQ_MSG("[SEC]-->SEC_SUBMIT: tgid[%d]\n", tgid);
	do {
		/* find handle first */
		handle = cmdq_sec_find_context_handle_unlocked(tgid);
		if (NULL == handle) {
			CMDQ_ERR("SEC_SUBMIT: tgid %d err[NULL secCtxHandle]\n", tgid);
			status = -(CMDQ_ERR_NULL_SEC_CTX_HANDLE);
			break;
		}

		/*alloc iwc buffer */
		if (0 > cmdq_sec_setup_context_session(handle)) {
			status = -(CMDQ_ERR_SEC_CTX_SETUP);
			break;
		}

		/* */
		/* record profile data */
		/* tbase timer/time support is not enough currently, */
		/* so we treats entry/exit timing to secure world as the trigger/gotIRQ_&_wakeup timing */
		CMGQ_GET_CURRENT_TIME(tEntrySec);

		status = cmdq_sec_send_context_session_message(handle, iwcCommand, pTask, thread, iwcFillCB);

		CMGQ_GET_CURRENT_TIME(tExitSec);
		CMDQ_GET_TIME_DURATION(tEntrySec, tExitSec, duration);
		if (pTask) {
			pTask->trigger = tEntrySec;
			pTask->gotIRQ  = tExitSec;
			pTask->wakedUp = tExitSec;
		}

		/* release resource */
		cmdq_sec_teardown_context_session(handle);

		/* because secure world has no clkmgr support, delay reset HW when back to normal world */
		/* note do reset flow only if secure driver exec failed (i.e. result = -CMDQ_ERR_DR_EXEC_FAILED) */
		if (TZ_RESULT_SUCCESS != status)
			cmdqResetHWEngine(pTask, thread);
	} while (0);

	mutex_unlock(&gCmdqSecExecLock);

	if (0 > status) {
		if (!skipSecCtxDump) {
			mutex_lock(&gCmdqSecContextLock);
			cmdq_sec_dump_context_list();
			mutex_unlock(&gCmdqSecContextLock);
		}

		/* throw AEE */
		CMDQ_AEE("CMDQ",
			"[SEC]<--SEC_SUBMIT: err[%d], pTask[0x%p], THR[%d], tgid[%d], duration_ms[%d], cmdId[%d]\n",
			status, pTask, thread, tgid, duration, iwcCommand);
	} else {
		CMDQ_LOG("[SEC]<--SEC_SUBMIT: err[%d], pTask[0x%p], THR[%d], tgid[%d], duration_ms[%d], cmdId[%d]\n",
			status, pTask, thread, tgid, duration, iwcCommand);
	}
	return status;
}

#endif /* __CMDQ_SECURE_PATH_SUPPORT__ */

int32_t cmdq_exec_task_secure_with_retry(
	TaskStruct *pTask,
	int32_t thread,
	const uint32_t maxRetry)
{
#if defined(__CMDQ_SECURE_PATH_SUPPORT__)
	int32_t status = 0;
	int32_t i = 0;

	do {
		uint32_t commandId = cmdq_sec_get_commandId();

		commandId = (0 < commandId) ? (commandId) : (TZCMD_CMDQ_SUBMIT_TASK);

		status = cmdq_sec_submit_to_secure_world(commandId, pTask, thread, NULL);
		if (0 > status) {
			CMDQ_ERR("%s[%d]\n", __func__, status);
			break;
		}
		i++;
	} while (i < maxRetry);

	return status;

#else
	CMDQ_ERR("secure path not support\n");
	return -EFAULT;
#endif /*__CMDQ_SECURE_PATH_SUPPORT__ */
}


/*-------------------------------------------------------------------------------------------- */

cmdqSecContextHandle cmdq_sec_find_context_handle_unlocked(uint32_t tgid)
{
	cmdqSecContextHandle handle = NULL;

	do {
		struct cmdqSecContextStruct *secContextEntry = NULL;
		struct list_head *pos = NULL;

		list_for_each(pos, &gCmdqSecContextList) {
			secContextEntry = list_entry(pos, struct cmdqSecContextStruct, listEntry);
			if (secContextEntry && tgid == secContextEntry->tgid) {
				handle = secContextEntry;
				break;
			}
		}
	} while (0);

	CMDQ_MSG("SecCtxHandle_SEARCH: Handle[0x%p], tgid[%d]\n", handle, tgid);
	return handle;
}


int32_t cmdq_sec_release_context_handle_unlocked(cmdqSecContextHandle handle)
{
	int32_t status = 0;

	do {
		handle->referCount--;
		if (0 < handle->referCount)
			break;
		/* 1. clean up secure path in secure world */
		/* 2. delete secContext from list */
		list_del(&(handle->listEntry));
		/* 3. release secure path resource in normal world */

	} while (0);
	return status;
}

int32_t cmdq_sec_release_context_handle(uint32_t tgid)
{
	int32_t status = 0;
	cmdqSecContextHandle handle = NULL;

	mutex_lock(&gCmdqSecContextLock);
	smp_mb();

	handle = cmdq_sec_find_context_handle_unlocked(tgid);
	if (handle) {
		CMDQ_MSG("SecCtxHandle_RELEASE: +tgid[%d], handle[0x%p]\n", tgid, handle);
		status = cmdq_sec_release_context_handle_unlocked(handle);
		CMDQ_MSG("SecCtxHandle_RELEASE: -tgid[%d], status[%d]\n", tgid, status);
	} else {
		status = -1;
		CMDQ_ERR("SecCtxHandle_RELEASE: err[secCtxHandle not exist], tgid[%d]\n", tgid);
	}

	mutex_unlock(&gCmdqSecContextLock);
	return status;
}

cmdqSecContextHandle cmdq_sec_context_handle_create(uint32_t tgid)
{
	cmdqSecContextHandle handle = NULL;

	handle = kmalloc(sizeof(uint8_t *) * sizeof(cmdqSecContextStruct), GFP_ATOMIC);
	if (handle) {
		handle->iwcMessage = NULL;

		handle->tgid = tgid;
		handle->referCount = 0;
	} else {
		CMDQ_ERR("SecCtxHandle_CREATE: err[LOW_MEM], tgid[%d]\n", tgid);
	}

	CMDQ_MSG("SecCtxHandle_CREATE: create new, Handle[0x%p], tgid[%d]\n", handle, tgid);
	return handle;
}

cmdqSecContextHandle cmdq_sec_acquire_context_handle(uint32_t tgid)
{
	cmdqSecContextHandle handle = NULL;

	mutex_lock(&gCmdqSecContextLock);
	smp_mb();
	do {
		/* find sec context of a process */
		handle = cmdq_sec_find_context_handle_unlocked(tgid);
		/* if it dose not exist, create new one */
		if (NULL == handle) {
			handle = cmdq_sec_context_handle_create(tgid);
			list_add_tail(&(handle->listEntry), &gCmdqSecContextList);
		}
	} while (0);

	/* increase caller referCount */
	if (handle)
		handle->referCount++;

	CMDQ_MSG("[CMDQ]SecCtxHandle_ACQUIRE, Handle[0x%p], tgid[%d], refCount[%d]\n",
		handle, tgid, handle->referCount);
	mutex_unlock(&gCmdqSecContextLock);

	return handle;
}

void cmdq_sec_dump_context_list(void)
{
	struct cmdqSecContextStruct *secContextEntry = NULL;
	struct list_head *pos = NULL;

	CMDQ_ERR("=============== [CMDQ] sec context ===============\n");

	list_for_each(pos, &gCmdqSecContextList) {
		secContextEntry = list_entry(pos, struct cmdqSecContextStruct, listEntry);
		CMDQ_ERR("secCtxHandle[0x%p], tgid_%d[referCount: %d], state[%d], iwc[0x%p]\n",
			secContextEntry,
			secContextEntry->tgid,
			secContextEntry->referCount,
			secContextEntry->state,
			secContextEntry->iwcMessage);
	}
}

void cmdqSecDeInitialize(void)
{
#if defined(__CMDQ_SECURE_PATH_SUPPORT__)
	/* .TEE. close SVP CMDQ TEE serivice session */
	/* the sessions are created and accessed using [cmdq_session_handle()] /  */
	/* [cmdq_mem_session_handle()] API, and closed here. */
		TZ_RESULT ret;

		ret = KREE_CloseSession(cmdq_session);
		if (ret != TZ_RESULT_SUCCESS) {
			CMDQ_ERR("DDP close ddp_session fail ret=%d\n", ret);
			return -1;
		}

		ret = KREE_CloseSession(cmdq_mem_session);
		if (ret != TZ_RESULT_SUCCESS) {
			CMDQ_ERR("DDP close ddp_mem_session fail ret=%d\n", ret);
			return -1;
		}
#endif


}


void cmdqSecInitialize(void)
{
	INIT_LIST_HEAD(&gCmdqSecContextList);
}

/*------------------------------------------------------------------------------------------ */
/* debug */
void cmdq_sec_set_commandId(uint32_t cmdId)
{
	atomic_set(&gDebugSecCmdId, cmdId);
}

const uint32_t cmdq_sec_get_commandId(void)
{
	return (uint32_t)(atomic_read(&gDebugSecCmdId));
}

void cmdq_debug_set_sw_copy(int32_t value)
{
	atomic_set(&gDebugSecSwCopy, value);
}

int32_t cmdq_debug_get_sw_copy(void)
{
	return atomic_read(&gDebugSecSwCopy);
}

