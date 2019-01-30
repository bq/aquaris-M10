
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

#ifndef __DDP_CMDQ_SEC_H__
#define __DDP_CMDQ_SEC_H__

#include "ddp_cmdq.h"

#if defined(__CMDQ_SECURE_PATH_SUPPORT__)
#include "tz_cross/trustzone.h"
#include "tz_cross/ta_mem.h"
#include "trustzone/kree/system.h"
#include "trustzone/kree/mem.h"
#include "trustzone/tz_cross/tz_cmdq.h"
#endif

/**
* error code for CMDQ
*/

#define CMDQ_ERR_NULL_SEC_CTX_HANDLE (6000)
#define CMDQ_ERR_SEC_CTX_SETUP (6001)
#define CMDQ_ERR_SEC_CTX_TEARDOWN (6002)

/**
* inter-world communication state
*/
typedef enum {
	IWC_INIT			  = 0,
	IWC_MOBICORE_OPENED   = 1,
	IWC_WSM_ALLOCATED	 = 2,
	IWC_SES_OPENED		= 3,
	IWC_SES_MSG_PACKAGED  = 4,
	IWC_SES_TRANSACTED	= 5,
	IWC_SES_ON_TRANSACTED = 6,
	IWC_END_OF_ENUM	   = 7,
} CMDQ_IWC_STATE_ENUM;


/**
* CMDQ secure context struct
* note it is not global data, each process has its own CMDQ sec context
*/
typedef struct cmdqSecContextStruct {
	struct list_head listEntry;

	/* basic info */
	uint32_t tgid;		/* tgid of procexx context */
	uint32_t referCount; /*reference count for open cmdq device node  */

	/* iwc state */
	CMDQ_IWC_STATE_ENUM state;

	/* iwc information */
	void *iwcMessage;						 /* message buffer  */
#if defined(__CMDQ_SECURE_PATH_SUPPORT__)
	KREE_SESSION_HANDLE sessionHandle;
	KREE_SESSION_HANDLE memSessionHandle;
#endif
} cmdqSecContextStruct, *cmdqSecContextHandle;


/**
* Callback to fill message buffer for secure task
*
* Params:
*	 init32_t command id
*	 void*	the inter-world communication buffer
* Return:
*	 >=0 for success;
*/
typedef int32_t (*CmdqSecFillIwcCB)(int32_t, void*);


/**
* submit task to secure world
*/
int32_t cmdq_exec_task_secure_with_retry(TaskStruct *pTask, int32_t thread, const uint32_t retry);


/**
* secure context API
*/
cmdqSecContextHandle cmdq_sec_find_context_handle_unlocked(uint32_t tgid);
cmdqSecContextHandle cmdq_sec_acquire_context_handle(uint32_t tgid);
int32_t cmdq_sec_release_context_handle(uint32_t tgid);
void cmdq_sec_dump_context_list(void);

void cmdqSecInitialize(void);
void cmdqSecDeInitialize(void);


/**
* debug
*/
void cmdq_sec_set_commandId(uint32_t cmdId);
const uint32_t cmdq_sec_get_commandId(void);

void cmdq_debug_set_sw_copy(int32_t value);
int32_t cmdq_debug_get_sw_copy(void);



#if defined(__CMDQ_SECURE_PATH_SUPPORT__)
/* the session to communicate with TA */
static KREE_SESSION_HANDLE cmdq_session;
static KREE_SESSION_HANDLE cmdq_mem_session;
KREE_SESSION_HANDLE cmdq_session_handle(void);
KREE_SESSION_HANDLE cmdq_mem_session_handle(void);
int32_t cmdq_sec_test_proc(int testValue);

#endif


#endif /*__DDP_CMDQ_SEC_H__ */
