
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
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/errno.h>

#include "ddp_cmdq_record.h"
#include "ddp_cmdq.h"


/*-------------------------------------------------------------------------------------- */

int32_t cmdqCoreSubmitTask(cmdqCommandStruct *pCommandDesc)
{
	int32_t status;


	CMDQ_MSG("-->SUBMIT: SYNC cmd 0x%p begin, isSecure[%d]\n",
		pCommandDesc->pVABase, (pCommandDesc->secData.isSecure));
	status = cmdqSubmitTask(pCommandDesc);
	CMDQ_MSG("<--SUBMIT: SYNC cmd 0x%p end\n", pCommandDesc->pVABase);
	return status;
}


bool cmdq_core_should_enable_prefetch(CMDQ_SCENARIO_ENUM scenario)
{
	/* not suppot prefretch until Rome */
	return false;
}

int32_t cmdq_subsys_from_phys_addr(uint32_t physAddr)
{
	int32_t  msb = 0;
	int32_t  subsys = -1;

	msb = (physAddr & 0x0FFFF0000) >> 16;

	if (msb == 0x1400)
		subsys = 0;	 /* MM sys	  */
	else if (msb == 0x1500)
		subsys = 1;	 /* IMG sys */
	else {
		subsys = 2;
		CMDQ_ERR("unrecognized subsys, msb=0x%04x\n", msb);
	}

	return subsys;
}

void cmdq_dump_command_buffer(void *buffer, uint32_t size)
{

	#if 0
	CMDQ_LOG("[CMDQ] --> cmdq_dump_command_buffer, buffer[0x%08x], size[%d]\n", buffer, size);
	File *pF = fopen("cmd_dump.raw", "w+");

	fwrite(buffer, 1 , size, pF);
	fclose(pF);
	#else
	uint32_t *pCMD = (uint32_t *) buffer;
	uint32_t instrCount = size / 8;
	uint32_t i = 0;

	CMDQ_LOG("[CMDQ] --> cmdq_dump_command_buffer, buffer[0x%p], size[%d], intrCount[%d]\n",
				buffer, size, instrCount);

	/* 64 bit per instruction */
	for (i = 0;  i < instrCount; i = i + 1) {
		uint32_t indexA = 2 * i + 1;
		uint32_t indexB = 2 * i;

		CMDQ_LOG("[CMDQ] ...inst[%3d] = 0x%08x, inst[%3d] = 0x%08x\n",
			(indexA), pCMD[indexA], (indexB), pCMD[indexB]);
	}

	#endif

	CMDQ_LOG("[CMDQ] <-- cmdq_dump_command_buffer\n");

}

/*-------------------------------------------------------------------------------------- */



static uint32_t cmdq_rec_flag_from_scenario_32bit(CMDQ_SCENARIO_ENUM scn)
{
	uint32_t flag = 0;

	switch (scn) {
	case CMDQ_SCENARIO_MDP_STREAM_UNKNOWN ... CMDQ_SCENARIO_MDP_STREAM_ISP_VSS:
		/* according to dpFW's user configuration, dpFW configures engine in DpPathTopology.cpp */
		/* it is user space case, engine flag is passed from user space		  */
		flag = 0LL;
		break;
	case CMDQ_SCENARIO_DEBUG:
		/* TODO: fake flag */
		flag = 0LL;
		break;
	default:
		CMDQ_ERR("Unknown scenario type %d\n", scn);
		flag = 0LL;
		break;
	}

	CMDQ_LOG("REC: %s, scenario:%d, flag:%d\n", __func__, scn, flag);
	return (uint32_t)flag;
}


int cmdq_rec_realloc_cmd_buffer(cmdqRecHandle handle, uint32_t size)
{
	void *pNewBuf = NULL;

	if (size <= handle->bufferSize)
		return 0;

	pNewBuf = vmalloc(size);

	if (NULL == pNewBuf) {
		CMDQ_ERR("REC: vmalloc %d bytes failed\n", size);
		return -ENOMEM;
	}

	memset(pNewBuf, 0, size);

	if (handle->pBuffer && handle->blockSize > 0)
		memcpy(pNewBuf, handle->pBuffer, handle->blockSize);

	CMDQ_MSG("REC: realloc size from %d to %d bytes\n", handle->bufferSize, size);

	vfree(handle->pBuffer);
	handle->pBuffer = pNewBuf;
	handle->bufferSize = size;

	return 0;
}

int32_t cmdq_rec_set_device_access_control(cmdqRecHandle handle, uint32_t enable)
{
	if (!enable)
		return 0;

	/* TODO:
	 only secure case need to enable, and makesure disabled at end of command */
	return 0;
}

int32_t cmdqRecCreate(CMDQ_SCENARIO_ENUM scenario,
					  cmdqRecHandle	  *pHandle)
{
	cmdqRecHandle handle = NULL;

	if (scenario < 0 || scenario >= CMDQ_MAX_SCENARIO_COUNT) {
		CMDQ_ERR("Unknown scenario type %d\n", scenario);
		return -EINVAL;
	}

	handle = kmalloc(sizeof(uint8_t *) * sizeof(cmdqRecStruct), GFP_KERNEL);
	if (NULL == handle)
		return -ENOMEM;

	memset(handle, 0, sizeof(uint8_t *) * sizeof(cmdqRecStruct));

	handle->scenario   = scenario;
	handle->pBuffer	= NULL;
	handle->bufferSize = 0;
	handle->blockSize  = 0;
	handle->engineFlag = cmdq_rec_flag_from_scenario_32bit(scenario);
	handle->priority   = CMDQ_THR_PRIO_NORMAL;
	handle->prefetchCount = 0;
	handle->finalized  = false;
	handle->pRunningTask = NULL;
	/* init secure data */
	handle->secData.isSecure = false;
	/*handle->secData.totalSecFd = 0; */
	/*memset(handle->secData.pSecFdIndex, 0, sizeof(uint32_t) * CMDQ_IWC_MAX_FD_COUNT); */
	/*memset(handle->secData.pSecPortList, 0, sizeof(uint32_t) * CMDQ_IWC_PORTLIST_LENGTH); */
	/*memset(handle->secData.pSecSizeList, 0, sizeof(uint32_t) * CMDQ_IWC_SIZELIST_LENGTH); */

	if (0 != cmdq_rec_realloc_cmd_buffer(handle, CMDQ_INITIAL_CMD_BLOCK_SIZE)) {
		kfree(handle);
		return -ENOMEM;
	}

	*pHandle = handle;

	return 0;
}
#if 1
int32_t cmdq_append_command(cmdqRecHandle  handle,
							CMDQ_CODE_ENUM code,
							uint32_t	   argA,
							uint32_t	   argB)
{
	int32_t subsys;
	uint32_t *pCommand;

	pCommand = (uint32_t *)((uint8_t *)handle->pBuffer + handle->blockSize);

	if (handle->finalized) {
		CMDQ_ERR("Already finalized record 0x%p, cannot add more command", handle);
		return -EBUSY;
	}

	/* check if we have sufficient buffer size */
	/* we leave a 4 instruction (4 bytes each) margin. */
	if ((handle->blockSize + 32) >= handle->bufferSize)
		if (0 != cmdq_rec_realloc_cmd_buffer(handle, handle->bufferSize * 2))
			return -ENOMEM;

	#if 0
	/* force insert MARKER if prefetch memory is full */
	/* GCE deadlocks if we don't do so */
	if (CMDQ_CODE_EOC != code && cmdq_core_should_enable_prefetch(handle->scenario)) {
		if (handle->prefetchCount >= CMDQ_MAX_PREFETCH_INSTUCTION) {
			/* Mark END of prefetch section */
			cmdqRecMark(handle);
			/* BEGING of next prefetch section */
			cmdqRecMark(handle);
		} else
			++handle->prefetchCount;
	}
	#endif
	/* we must re-calculate current PC because we may already insert MARKER inst. */
	pCommand = (uint32_t *)((uint8_t *)handle->pBuffer + handle->blockSize);

	CMDQ_MSG("REC: 0x%p CMD: 0x%p, op: 0x%02x, argA: 0x%08x, argB: 0x%08x\n", handle, pCommand, code, argA, argB);

	switch (code) {
	case CMDQ_CODE_READ:
		/* argA is the HW register address to read from */
		/* argA = IO_VIRT_TO_PHYS(argA); */
		subsys = cmdq_subsys_from_phys_addr(argA);
		/* argB is the register id to read into */
		*pCommand++ = argB;
		*pCommand++ = (CMDQ_CODE_READ << 24) | (argA & 0x3fffff) | ((subsys & 0x3) << 22);
		/* *pCommand++ = (CMDQ_CODE_READ << 24) | (argA & 0xffff) | ((subsys & 0x1f) << 16) | (2 << 21);  */
		break;
	case CMDQ_CODE_MOVE:
		*pCommand++ = argB;
		*pCommand++ = CMDQ_CODE_MOVE << 24;
		break;
	case CMDQ_CODE_WRITE:
		subsys = cmdq_subsys_from_phys_addr(argA);
		if (-1 == subsys) {
			CMDQ_ERR("REC: Unsupported memory base address 0x%08x\n", argA);
			return -EFAULT;
		}

		*pCommand++ = argB;
		*pCommand++ = (CMDQ_CODE_WRITE << 24) | (argA & 0x3fffff) | ((subsys & 0x3) << 22);
		/* *pCommand++ = (CMDQ_CODE_WRITE << 24) | (argA & 0x0FFFF) | ((subsys & 0x01F) << 16); */
		break;
	case CMDQ_CODE_POLL:
		subsys = cmdq_subsys_from_phys_addr(argA);
		if (-1 == subsys) {
			CMDQ_ERR("REC: Unsupported memory base address 0x%08x\n", argA);
			return -EFAULT;
		}
		*pCommand++ = argB;
		*pCommand++ = (CMDQ_CODE_POLL << 24) | (argA & 0x3fffff) | ((subsys & 0x3) << 22);
		/* *pCommand++ = (CMDQ_CODE_POLL << 24) | (argA & 0x0FFFF) | ((subsys & 0x01F) << 16); */
		break;
	case CMDQ_CODE_JUMP:
		*pCommand++ = argB;
		*pCommand++ = (CMDQ_CODE_JUMP << 24) | (argA & 0x0FFFFFF);
		break;
	case CMDQ_CODE_WFE:
		/* bit 0-11: wait_value, 1 */
		/* bit 15: to_wait, true */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 0 */
		*pCommand++ = ((1 << 31) | (1 << 15) | 1);
		*pCommand++ = (CMDQ_CODE_WFE << 24) | argA;
		break;

	case CMDQ_CODE_SET_TOKEN:
		/* this is actually WFE(SYNC) but with different parameter */
		/* interpretation */
		/* bit 15: to_wait, false */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 1  */
		*pCommand++ = ((1 << 31) | (1 << 16));
		*pCommand++ = (CMDQ_CODE_WFE << 24) | argA;
		break;

	case CMDQ_CODE_WAIT_NO_CLEAR:
		/* bit 0-11: wait_value, 1 */
		/* bit 15: to_wait, true */
		/* bit 31: to_update, false */
		*pCommand++ = ((0 << 31) | (1 << 15) | 1);
		*pCommand++ = (CMDQ_CODE_WFE << 24) | argA;
		break;

	case CMDQ_CODE_EOC:
		*pCommand++ = argB;
		*pCommand++ = (CMDQ_CODE_EOC << 24) | (argA & 0x0FFFFFF);
		break;
	default:
		return -EFAULT;
	}

	handle->blockSize += 8;

	return 0;
}
#endif
bool cmdq_rec_is_dsi_cmd_mode(CMDQ_SCENARIO_ENUM scn)
{
	return false;
}

static void cmdq_rec_insert_frame_sync_instructions(cmdqRecHandle handle)
{
}

int32_t cmdqRecReset(cmdqRecHandle handle)
{


	if (NULL == handle)
		return -EFAULT;

	if (NULL != handle->pRunningTask)
		cmdqRecStopLoop(handle);

	handle->blockSize = 0;
	handle->prefetchCount = 0;
	handle->finalized = false;

	handle->secData.isSecure = false;

	if (cmdq_core_should_enable_prefetch(handle->scenario)) {
		/* enable prefetch */
		cmdqRecMark(handle);

		/* make sure our "config" won't corrupt frame update */
		/* e.g. do not update AAL setting when processing frame. */
		cmdq_rec_insert_frame_sync_instructions(handle);
	}

	return 0;
}

int32_t cmdqRecMark(cmdqRecHandle handle)
{
	return 0;
}


int32_t cmdqRecWrite(cmdqRecHandle handle,
					 uint32_t	  addr,
					 uint32_t	  value,
					 uint32_t	  mask)
{
	int32_t status;

	if (0xFFFFFFFF != mask) {
		status = cmdq_append_command(handle, CMDQ_CODE_MOVE, 0, ~mask);
		if (0 != status)
			return status;

		addr = addr | 0x1;
	}

	status = cmdq_append_command(handle, CMDQ_CODE_WRITE, addr, value);
	if (0 != status)
		return status;

	return 0;
}


int32_t cmdqRecPoll(cmdqRecHandle handle,
					uint32_t	  addr,
					uint32_t	  value,
					uint32_t	  mask)
{
	int32_t status;

	status = cmdq_append_command(handle, CMDQ_CODE_MOVE, 0, ~mask);
	if (0 != status)
		return status;

	status = cmdq_append_command(handle, CMDQ_CODE_POLL, (addr | 0x1), value);	/* 0 bit: mask enabled */
	if (0 != status)
		return status;

	return 0;
}


int32_t cmdqRecWait(cmdqRecHandle   handle,
					CMDQ_EVENT_ENUM event)
{
	return cmdq_append_command(handle, CMDQ_CODE_WFE, event, 0);
}

int32_t cmdqRecSetEventToken(cmdqRecHandle   handle,
							 CMDQ_EVENT_ENUM event)
{
	if (CMDQ_SYNC_TOKEN_INVALID == event ||
		CMDQ_SYNC_TOKEN_MAX <= event ||
		0 > event)
		return -EINVAL;

	return cmdq_append_command(handle,
							 CMDQ_CODE_SET_TOKEN,
							 event,
							 1  /* actually this param is ignored. */
							 );
}

int32_t cmdqRecReadToDataRegister(cmdqRecHandle handle, uint32_t hwRegAddr/*, CMDQ_DATA_REGISTER_ENUM dstDataReg*/)
{
	return -1;
	/* return cmdq_append_command(handle, CMDQ_CODE_READ, hwRegAddr, dstDataReg); */
}

int32_t cmdq_rec_finalize_command(cmdqRecHandle handle, bool loop)
{
	int32_t status = 0;
	uint32_t argB = 0;

	if (!handle->finalized) {
		#if 0
		/* FOR disp sys, we should insert CONFIG_DIRTY event */
		/* so that trigger loop wakes up */
		if (cmdq_core_should_enable_prefetch(handle->scenario)) {
			cmdq_append_command(handle,
						 CMDQ_CODE_SET_TOKEN,
						 CMDQ_SYNC_TOKEN_CONFIG_DIRTY,
						 1  /* actually this param is ignored. */
						 );
		}
		#endif
		#if 1
		/*uint32_t secReg = DISP_REG_CMDQ_MM_SECURITY;		  */



		/*
		if (handle->secData.isSecure && !gBypassSecAccessControl) {
			flag  = (flag) | (0x1 << 0);
			CMDQ_LOG("[CMDQ] REC: cmdq_rec_finalize_command,", flag);
			CMDQ_LOG(" sec flag = 0x%08x, gBypassSecAccessControl:%d\n",
				gBypassSecAccessControl);
			cmdqRecWrite(handle, IO_VIRT_TO_PHYS(secReg) , flag, ~0);
		}*/

		#endif

		argB = 0x1; /* generate IRQ for each command iteration	 */
		if (handle->prefetchCount >= 1) {
			/* with prefetch threads we should end with a prefetch mark */
			argB |= 0x00130000;

			/* since we're finalized, no more prefetch */
			handle->prefetchCount = 0;
		}
		status = cmdq_append_command(handle, CMDQ_CODE_EOC,
									 0,
									 argB);

		if (0 != status)
			return status;

		/* insert JUMP to loop to beginning or as a scheduling mark(8) */
		status = cmdq_append_command(handle, CMDQ_CODE_JUMP,
									 0,   /* not absolute */
									 loop ? -handle->blockSize : 8
									 );
		if (0 != status)
			return status;

		handle->finalized = true;
	}

	return status;
}

int32_t cmdqRecFlush(cmdqRecHandle handle)
{
	int32_t status;
	cmdqCommandStruct desc = {0};

	status = cmdq_rec_finalize_command(handle, false);
	if (status < 0)
		return status;

	CMDQ_MSG("Submit task scenario: %d, priority: %d, engine: 0x%llx, buffer: 0x%p, size: %d\n",
		handle->scenario, handle->priority, handle->engineFlag, handle->pBuffer, handle->blockSize);

	desc.scenario = handle->scenario;
	desc.priority = handle->priority;
	desc.engineFlag = handle->engineFlag;
	desc.pVABase = handle->pBuffer;
	desc.blockSize = handle->blockSize;
	desc.secData = handle->secData;

	cmdq_dump_command_buffer(handle->pBuffer, handle->blockSize);

	return cmdqCoreSubmitTask(&desc);
}

static int32_t cmdqRecIRQCallback(unsigned long data)
{
	return 0;
}

int32_t cmdqRecStartLoop(cmdqRecHandle handle)
{
	int32_t status = 0;
	cmdqCommandStruct desc = {0};

	if (NULL != handle->pRunningTask)
		return -EBUSY;

	status = cmdq_rec_finalize_command(handle, true);
	if (status < 0)
		return status;

	CMDQ_MSG("Submit task loop: scenario: %d, priority: %d, engine: 0x%llx,",
			handle->scenario,
			handle->priority,
			handle->engineFlag);
	CMDQ_MSG(" buffer: 0x%p, size: %d, callback: 0x%p, data: %d\n",
			handle->pBuffer,
			handle->blockSize,
			&cmdqRecIRQCallback,
			0);

	desc.scenario = handle->scenario;
	desc.priority = handle->priority;
	desc.engineFlag = handle->engineFlag;
	desc.pVABase = handle->pBuffer;
	desc.blockSize = handle->blockSize;

#if 1
	status = cmdqCoreSubmitTask(&desc);
#else
	status = cmdqCoreSubmitTaskAsync(&desc,
									 &cmdqRecIRQCallback,
									 0,
									 &handle->pRunningTask);
#endif
	return status;
}

int32_t cmdqRecStopLoop(cmdqRecHandle handle)
{
	int32_t status = 0;
#if 1
	status = -5;
#else
	struct TaskStruct *pTask = handle->pRunningTask;

	if (NULL == pTask)
		return -EFAULT;

	status = cmdqCoreReleaseTask(pTask);
	handle->pRunningTask = NULL;
#endif
	return status;
}

void cmdqRecDestroy(cmdqRecHandle handle)
{
	if (NULL == handle)
		return;

	if (NULL != handle->pRunningTask)
		cmdqRecStopLoop(handle);

	/* Free command buffer */
	vfree(handle->pBuffer);
	handle->pBuffer = NULL;

	/* Free command handle */
	kfree(handle);
}



uint32_t cmdq_rec_setup_sec_access_control_flag(cmdqRecHandle handle)
{
	CMDQ_LOG("Reset hardware engine begin\n");
	return 0;
}

int32_t cmdq_get_sys_hw_id(uint32_t argA, uint32_t argB)
{
	int32_t subsys_id = (argA & 0x001F0000) >> 16;
	const char *module = "CMDQ";

	switch (subsys_id) {
	case 0:
		module = "MFG";
		break;
	case 1:
	case 2:
	case 3:
		module = "MMSYS";
		break;
	case 4:
		module = "IMG";
		break;
	case 5:
		module = "VDEC";
		break;
	case 6:
		module = "MJC";
		break;
	case 7:
		module = "VENC";
		break;
	case 8:
	case 9:
		module = "INFRA_AO";
		break;
	case 10 ... 13:
		module = "MD32";
		break;
	case 14:
	case 15:
		module = "INFRASYS";
		break;
	case 16 ... 22:
		module = "PERISYS";
		break;
	default:
		module = "CMDQ";
		break;
	}

	return 0;
	/* return module; */
}

int32_t cmdqRecSetCommandSecure(cmdqRecHandle handle,
								bool		  isSecure)
{
	if (NULL == handle)
		return -1;

	CMDQ_MSG("pHandle[0x%p], set secure = %d\n", handle, isSecure);
	handle->secData.isSecure = isSecure;

	/* insert access control at the head of command */
	/*uint32_t secReg = DISP_REG_CMDQ_MM_SECURITY; */



	/*
	if (isSecure && !gBypassSecAccessControl) {
		flag  = (flag) | (0x1 << 16);
		flag  = (flag) | (0x1 << 0);
		CMDQ_LOG("[CMDQ] sec flag: 0x%08x, gBypassSecAccessControl:%d\n", flag, gBypassSecAccessControl);
		cmdqRecWrite(handle, IO_VIRT_TO_PHYS(secReg) , flag, ~0);
	}
	*/
	return 0;
}

