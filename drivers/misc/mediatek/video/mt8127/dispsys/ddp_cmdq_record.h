
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

/*
* 82 CmdQ command recorder
*/

#ifndef __DDP_CMDQ_RECORD_H__
#define __DDP_CMDQ_RECORD_H__

#include <linux/types.h>
#include "ddp_cmdq.h"
#include "ddp_reg.h"
#include "ddp_cmdq_reg.h"

/* static bool gBypassSecAccessControl; */

/*-------------------------------------------------------------------------------------- */
/*-------------------------------------------------------------------------------------- */
#define DISP_REG_CMDQ_MM_SECURITY						(DISPSYS_CMDQ_BASE + 0x24)
#define DISP_REG_CMDQ_MM_SECURITY_SET					(DISPSYS_CMDQ_BASE + 0x28)
#define DISP_REG_CMDQ_MM_SECURITY_CLR					(DISPSYS_CMDQ_BASE + 0x2c)

#define CMDQ_INITIAL_CMD_BLOCK_SIZE			 (32 * 1024)

typedef enum CMDQ_HW_THREAD_PRIORITY_ENUM {
	CMDQ_THR_PRIO_NORMAL				= 0,	/* nomral (lowest) priority */
	CMDQ_THR_PRIO_DISPLAY_TRIGGER	   = 1,
	CMDQ_THR_PRIO_DISPLAY_CONFIG		= 2,

	CMDQ_THR_PRIO_MAX				   = 7,	/* maximum possible priority  */
} CMDQ_HW_THREAD_PRIORITY_ENUM;


typedef enum CMDQ_EVENT_ENUM {
	/* HW event */
	CMDQ_EVENT_MUTEX_ALL_ENGINE_UPDATE_0 = 0,
	CMDQ_EVENT_MUTEX_ALL_ENGINE_UPDATE_1 = 1,
	CMDQ_EVENT_MUTEX_ALL_ENGINE_UPDATE_2 = 2,
	CMDQ_EVENT_MUTEX_ALL_ENGINE_UPDATE_3 = 3,
	CMDQ_EVENT_MUTEX_ALL_ENGINE_UPDATE_4 = 4,
	CMDQ_EVENT_MUTEX_ALL_ENGINE_UPDATE_5 = 5,
	CMDQ_EVENT_MUTEX_ALL_ENGINE_UPDATE_6 = 6,
	CMDQ_EVENT_MUTEX_ALL_ENGINE_UPDATE_7 = 7,

	CMDQ_EVENT_MUTEX_RESERVED_0 = 8,
	CMDQ_EVENT_MUTEX_RESERVED_1 = 9,
	CMDQ_EVENT_MUTEX_RESERVED_2 = 10,
	CMDQ_EVENT_MUTEX_RESERVED_3 = 11,

	CMDQ_EVENT_DISP_WDMA_EOF = 12,
	CMDQ_EVENT_DISP_RDMA_EOF = 13,
	CMDQ_EVENT_DISP_BLS_EOF = 14,
	CMDQ_EVENT_DISP_COLOR_EOF = 15,
	CMDQ_EVENT_DISP_OVL_EOF = 16,
	CMDQ_EVENT_MDP_TDSHP_EOF = 17,
	CMDQ_EVENT_MDP_RSZ1_EOF = 18,
	CMDQ_EVENT_MDP_RSZ0_EOF = 19,
	CMDQ_EVENT_MDP_RDMA_EOF = 20,
	CMDQ_EVENT_MDP_WDMA_EOF = 21,
	CMDQ_EVENT_MDP_WROT_EOF = 22,
	CMDQ_EVENT_ISP1_EOF = 23,
	CMDQ_EVENT_ISP2_EOF = 24,

	CMDQ_EVENT_MDP_WROT_SOF = 25,
	CMDQ_EVENT_MDP_RSZ0_SOF = 26,
	CMDQ_EVENT_MDP_RSZ1_SOF = 27,
	CMDQ_EVENT_DISP_OVL_SOF = 28,
	CMDQ_EVENT_MDP_WDMA_SOF = 29,
	CMDQ_EVENT_MDP_RDMA_SOF = 30,
	CMDQ_EVENT_DISP_WDMA_SOF = 31,
	CMDQ_EVENT_DISP_COLOR_SOF = 32,
	CMDQ_EVENT_MDP_TDSHP_SOF = 33,
	CMDQ_EVENT_DISP_BLS_SOF = 34,
	CMDQ_EVENT_DISP_RDMA_SOF = 35,
	CMDQ_EVENT_CAM_MDP_SOF = 36,

	/* SW event */

	/* CMDQ driver reserved */
	CMDQ_SYNC_TOKEN_MAX	= 511,   /* TODO:  event id is 9 bit  */
	CMDQ_SYNC_TOKEN_INVALID = -1
} CMDQ_EVENT_ENUM;

/*-------------------------------------------------------------------------------------- */

#ifdef __CMDQ_SECURE_PATH_SUPPORT__
#include "trustzone/tz_cross/tz_cmdq.h"
#endif

#if 0
typedef struct cmdqRecSecStruct {
	bool isSecure;
	uint32_t totalSecFd;
	uint32_t pSecFdIndex[CMDQ_IWC_MAX_FD_COUNT];
	uint32_t pSecPortList[CMDQ_IWC_PORTLIST_LENGTH];
	uint32_t pSecSizeList[CMDQ_IWC_SIZELIST_LENGTH];
} cmdqRecSecStruct;

typedef struct cmdqReadRegStruct {
	uint32_t count;		 /* number of entries in regAddresses */
	uint32_t *regAddresses; /* an array of register addresses */
} cmdqReadRegStruct;

typedef struct cmdqRegValueStruct {
	uint32_t count;
		/* number of entries in result */
	uint32_t *regValues;	/* array of register values. */
							/* in the same order as cmdqReadRegStruct */
} cmdqRegValueStruct;

typedef struct cmdqCommandStruct {
	uint32_t scenario;			  /* [IN] deprecated. will remove in the future. */
	uint32_t priority;			  /* [IN] task schedule priority. this is NOT HW thread priority. */
	uint64_t engineFlag;			/* [IN] bit flag of engines used. */
	uint32_t *pVABase;			  /* [IN] pointer to instruction buffer */
	uint32_t blockSize;			 /* [IN] size of instruction buffer, in bytes. */
	cmdqRecSecStruct secData;	   /* [IN] metadata about secure command */
	cmdqReadRegStruct regRequest;   /* [IN] request to read register values at the end of command */
	cmdqRegValueStruct regValue;	/* [OUT] register values of regRequest */
} cmdqCommandStruct;
#endif
/*-------------------------------------------------------------------------------------- */
/*-------------------------------------------------------------------------------------- */

struct TaskStruct;

typedef struct cmdqRecStruct {
	uint64_t engineFlag;
	int32_t  scenario;
	uint32_t blockSize;		   /* command size */
	void *pBuffer;
	uint32_t bufferSize;		  /* allocated buffer size */
	struct TaskStruct *pRunningTask;	 /* running task after flush() or startLoop() */
	CMDQ_HW_THREAD_PRIORITY_ENUM  priority; /* setting high priority. This implies Prefetch ENABLE. */
	bool	 finalized;		   /* set to true after flush() or startLoop() */
	uint32_t prefetchCount;	   /* maintenance prefetch instruction	 */
	cmdqSecDataStruct secData;	 /* metadata about secure command */
} cmdqRecStruct, *cmdqRecHandle;

typedef void *CmdqRecLoopHandle;

#ifdef __cplusplus
extern "C" {
#endif


/**
* Create command queue recorder handle
* Parameter:
*	 pHandle: pointer to retrieve the handle
* Return:
*	 0 for success; else the error code is returned
*/
int32_t cmdqRecCreate(CMDQ_SCENARIO_ENUM scenario,
					  cmdqRecHandle * pHandle);


/**
* Reset command queue recorder commands
* Parameter:
*	handle: the command queue recorder handle
* Return:
*	 0 for success; else the error code is returned
*/
int32_t cmdqRecReset(cmdqRecHandle handle);


/**
* Append mark command to the recorder
* Parameter:
*	 handle: the command queue recorder handle
* Return:
*	 0 for success; else the error code is returned
*/
int32_t cmdqRecMark(cmdqRecHandle handle);


/**
* Append write command to the recorder
* Parameter:
*	 handle: the command queue recorder handle
*	 addr: the specified target register address
*	 value: the specified target register value
*	 mask: the specified target register mask
* Return:
*	 0 for success; else the error code is returned
*/
int32_t cmdqRecWrite(cmdqRecHandle handle,
					 uint32_t	  addr,
					 uint32_t	  value,
					 uint32_t	  mask);

/**
* Append poll command to the recorder
* Parameter:
*	 handle: the command queue recorder handle
*	 addr: the specified register address
*	 value: the required register value
*	 mask: the required register mask
* Return:
*	 0 for success; else the error code is returned
*/
int32_t cmdqRecPoll(cmdqRecHandle handle,
					uint32_t	  addr,
					uint32_t	  value,
					uint32_t	  mask);

/**
* Append wait command to the recorder
* Parameter:
*	 handle: the command queue recorder handle
*	 event: the desired event type for wait
* Return:
*	 0 for success; else the error code is returned
*/
int32_t cmdqRecWait(cmdqRecHandle   handle,
					CMDQ_EVENT_ENUM event);

/**
* Unconditionally set to given event to 1.
* Parameter:
*	 handle: the command queue recorder handle
*	 event: the desired event type to set
* Return:
*	 0 for success; else the error code is returned
*/
int32_t cmdqRecSetEventToken(cmdqRecHandle   handle,
							 CMDQ_EVENT_ENUM event);

int32_t cmdqRecReadToDataRegister(cmdqRecHandle handle, uint32_t hwRegAddr/*, CMDQ_DATA_REGISTER_ENUM dstDataReg*/);

/**
* Trigger CMDQ to execute the recorded commands
* Parameter:
*	 handle: the command queue recorder handle
* Return:
*	 0 for success; else the error code is returned
* Note:
*	 This is a synchronous function. When the function
*	 returned, the recorded commands have been done.
*/
int32_t cmdqRecFlush(cmdqRecHandle handle);

/**
* Trigger CMDQ to execute the recorded commands in loop.
* each loop completion generates callback in interrupt context.
*
* Parameter:
*	 handle: the command queue recorder handle
*	 irqCallback: this CmdqInterruptCB callback is called after each loop completion.
*	 data:   user data, this will pass back to irqCallback
*	 hLoop:  output, a handle used to stop this loop.
*
* Return:
*	 0 for success; else the error code is returned
*
* Note:
*	 This is an asynchronous function. When the function
*	 returned, the thread has started. Return -1 in irqCallback to stop it.
*/
int32_t cmdqRecStartLoop(cmdqRecHandle handle);

int32_t cmdqRecStopLoop(cmdqRecHandle handle);

/**
* Destroy command queue recorder handle
* Parameter:
*	 handle: the command queue recorder handle
*/
void cmdqRecDestroy(cmdqRecHandle handle);



/**
* Configure the recorded commands's secure attibute
* Parameter:
*	 handle: the command queue recorder handle
*	 isSecure: true, execute the recorder command in secure world
* Return:
*	 0 for success; else the error code is returned
*
* Note:
*	 Secure CMDQ support when t-base support only, not default enabled.
*/
int32_t cmdqRecSetCommandSecure(cmdqRecHandle  handle,
								bool		   isSecure);


#ifdef __cplusplus
}
#endif

#endif  /* __DDP_CMDQ_RECORD_H__ */
