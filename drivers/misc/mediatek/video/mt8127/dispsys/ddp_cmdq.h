
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

#ifndef __DDP_CMDQ_H__
#define __DDP_CMDQ_H__

#ifdef __KERNEL__

#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/proc_fs.h>

#include <mt-plat/aee.h>
#include <linux/slab.h>

/* #include <mach/mt_clkmgr.h> */

/* ftrace profile */
#include <linux/ftrace.h>
/* #include <linux/mtk_ftrace.h> */


/* #if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT) */
#if defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
#define __CMDQ_SECURE_PATH_SUPPORT__
#endif


#define CMDQ_DUMP_GIC	(0)
#define CMDQ_MAX_TASK_COUNT		   (512)
#define CMDQ_MAX_ENGINE_COUNT		  (12)
#define CMDQ_MAX_THREAD_COUNT		  (14)
#define CMDQ_MIN_SECURE_THREAD_ID	   (0)
#define CMDQ_MAX_SECURE_THREAD_COUNT	(1) /* support executing secure task  */
#define CMDQ_MAX_RECORD_COUNT		(1024)
#define CMDQ_MAX_ERROR_COUNT		   (1)
#define CMDQ_MAX_WARNING_COUNT		 (20)
#define CMDQ_MAX_RETRY_COUNT		   (1)

#define CMDQ_MAX_FIXED_TASK			(70)
#define CMDQ_MAX_BLOCK_SIZE	 (32 * 1024)
#define CMDQ_MAX_DMA_BUF_SIZE   (CMDQ_MAX_FIXED_TASK * CMDQ_MAX_BLOCK_SIZE)
#define CMDQ_EXTRA_MARGIN	   (512)

#define CMDQ_MAX_LOOP_COUNT	 (0x100000)
#define CMDQ_MAX_INST_CYCLE	 (0)	/* no HW timeout */
#define CMDQ_MIN_AGE_VALUE	  (5)

#define CMDQ_DEFAULT_TIMEOUT_MS			(1200)
#define CMDQ_DEFAULT_PREDUMP_START_TIME_MS  (400)
#define CMDQ_DEFAULT_PREDUMP_TIMEOUT_MS	 (100)
#define CMDQ_DEFAULT_PREDUMP_RETRY_COUNT	  (8)

#define CMDQ_INVALID_THREAD	 (-1)

/* log print api */
#define CMDQ_LOG_TO_UART 1

/* CMDQ_LOG_TO_UART for development & debug */
#if CMDQ_LOG_TO_UART
#define CMDQ_LOG(string, args...) do {\
	if (1) {\
		pr_notice("[CMDQ_LOG]"string, ##args); \
	}	\
} while (0)

#define CMDQ_MSG(string, args...) do {	\
	if (cmdq_core_should_print_msg()) {	\
		pr_notice("[CMDQ_MSG]"string, ##args);	\
	}	\
} while (0)
#define CMDQ_VERBOSE(string, args...) do {\
	if (cmdq_core_should_print_msg()) {\
		pr_notice("[CMDQ_VERBOSE]"string, ##args); \
	}	\
} while (0)

#define CMDQ_ERR(string, args...) do {	 \
	if (1) { \
		pr_notice("[CMDQ][ERR]"string, ##args); \
	}	\
} while (0)

#define CMDQ_AEE(tag, string, args...) \
	do {	 \
		char output[255];	\
		sprintf(output, "[CMDQ][AEE]"string, ##args);	\
		aee_kernel_warning_api(__FILE__, __LINE__,	\
			DB_OPT_DEFAULT | DB_OPT_PROC_CMDQ_INFO,	\
			output, string, ##args); \
		pr_err(tag": "string, ##args); \
	} while (0)

#else

#define CMDQ_LOG(string, args...) do {\
	if (1) {\
		pr_notice("[CMDQ]"string, ##args); \
	}	\
} while (0)

#define CMDQ_MSG(string, args...) do {	\
	if (cmdq_core_should_print_msg()) {	\
		pr_notice("[CMDQ]"string, ##args);	\
	}	\
} while (0)

#define CMDQ_VERBOSE(string, args...) do {\
	if (cmdq_core_should_print_msg()) {\
		pr_notice("[CMDQ]"string, ##args); \
	}	\
} while (0)

#define CMDQ_ERR(string, args...) do {	\
	if (1)	{	\
		pr_notice("[CMDQ][ERR]"string, ##args); \
	}	\
} while (0)

#define CMDQ_AEE(tag, string, args...)	\
	do {			\
		char output[255];	\
		sprintf(output, "[CMDQ][AEE]"string, ##args);		\
		aee_kernel_warning_api(__FILE__, __LINE__,	\
			DB_OPT_DEFAULT | DB_OPT_PROC_CMDQ_INFO,	\
			output, string, ##args); \
		pr_err(tag": "string, ##args);	\
	} while (0)

#endif

/*
	ftrace profile
	.need enable CONFIG_MTK_KERNEL_MARKER in kernel config
*/

/* #define CMDQ_PROFILE */

#ifdef CMDQ_PROFILE
#define CMDQ_PROF_INIT()
#define CMDQ_PROF_START(tag) mt_kernel_trace_begin(tag)
#define CMDQ_PROF_END(tag) mt_kernel_trace_end()
#define CMDQ_PROF_ONESHOT(args...) mt_kernel_trace_counter(args...)
#else
#define CMDQ_PROF_INIT()
#define CMDQ_PROF_START(args...)
#define CMDQ_PROF_END(args...)
#define CMDQ_PROF_ONESHOT(args...)
#endif


/*
	get time notice:
	.void do_gettimeofday(struct timeval *tv)
	.struct timeval {
		__kernel_time_t		 tv_sec;
		__kernel_suseconds_t tv_usec;
	}
*/
#define CMGQ_GET_CURRENT_TIME(value)					   \
{														  \
	do_gettimeofday(&(value));							 \
}

/* get duration in millisecond(ms) */
#define CMDQ_GET_TIME_DURATION(start, end, duration)	   \
{														  \
	int32_t time1;										 \
	int32_t time2;										 \
														   \
	time1 = start.tv_sec * 1000000 + start.tv_usec;		\
	time2 = end.tv_sec   * 1000000 + end.tv_usec;		  \
														   \
	duration = (time2 - time1) / 1000;					 \
}

#endif /* __KERNEL__ */


typedef enum CMDQ_THR_IRQ_FLAG_ENUM {
	CMDQ_THR_IRQ_FALG_EXEC_CMD	   = 0x01,
	CMDQ_THR_IRQ_FALG_INSTN_TIMEOUT  = 0x02,
	CMDQ_THR_IRQ_FALG_INVALID_INSTN  = 0x10
} CMDQ_THR_IRQ_FLAG_ENUM;


typedef enum CMDQ_SCENARIO_ENUM {
	/* 82/92 scenario is sync with MDP
		from user space, i.e, MDP */
	CMDQ_SCENARIO_MDP_STREAM_UNKNOWN = 0,
	CMDQ_SCENARIO_MDP_STREAM_BITBLT,
	CMDQ_SCENARIO_MDP_STREAM_FRAG,
	CMDQ_SCENARIO_MDP_STREAM_MULTI,
	CMDQ_SCENARIO_MDP_STREAM_ISP_IC,
	CMDQ_SCENARIO_MDP_STREAM_ISP_VR,
	CMDQ_SCENARIO_MDP_STREAM_ISP_ZSD,
	CMDQ_SCENARIO_MDP_STREAM_ISP_IP,
	CMDQ_SCENARIO_MDP_STREAM_ISP_VSS,

	/* CMDQ_SCENARIO_DEBUG is from kernel */
	CMDQ_SCENARIO_DEBUG,
	CMDQ_MAX_SCENARIO_COUNT			  /* ALWAYS keep at the end */
} CMDQ_SCENARIO_ENUM;


typedef enum CMDQ_CODE_ENUM {
	CMDQ_CODE_READ  = 0x01,
	CMDQ_CODE_MOVE  = 0x02,
	CMDQ_CODE_WRITE = 0x04,
	CMDQ_CODE_POLL  = 0x08,
	CMDQ_CODE_JUMP  = 0x10,
	CMDQ_CODE_WFE   = 0x20,  /* wait for event */
	CMDQ_CODE_EOC   = 0x40,  /* end of command */

	/* these are pseudo op code defined by SW
		for instruction generation */
	CMDQ_CODE_SET_TOKEN = 0x21,	  /* set event */
	CMDQ_CODE_WAIT_NO_CLEAR = 0x22,  /* wait event, but don't clear it */
} CMDQ_CODE_ENUM;


typedef enum CMDQ_ENG_ENUM {
	/* CAM */
	tIMGI			   = 0,
	tIMGO			   = 1,
	tIMG2O			  = 2,

	/* MDP */
	tRDMA0			  = 3,
	tCAMIN			  = 4,
	tSCL0			   = 5,
	tSCL1			   = 6,
	tTDSHP			  = 7,
	tWROT			   = 8,
	tWDMA1			  = 9,

	tLSCI			   = 10,
	tCMDQ			   = 11,

	tTotal			  = 12,
} CMDQ_ENG_ENUM;

#ifdef __KERNEL__

enum {
	cbMDP			   = 0,
	cbISP			   = 1,
	cbMAX			   = 2,
};


/**
* callback to notify engine timeout/reset
* params
*	 init32_t engineFlag
*/
typedef int (*CMDQ_TIMEOUT_PTR)(int32_t engineFlag, int32_t level);
typedef int (*CMDQ_RESET_PTR)(int32_t engineFlag);

typedef enum TASK_TYPE_ENUM {
	TASK_TYPE_FIXED	 = 0,
	TASK_TYPE_DYNAMIC   = 1
} TASK_TYPE_ENUM;

typedef enum TASK_STATE_ENUM {
	TASK_STATE_IDLE,		/* free task */
	TASK_STATE_BUSY,		/* task running on a thread */
	TASK_STATE_KILLED,	  /* task process being killed */
	TASK_STATE_ERROR,	   /* task execution error */
	TASK_STATE_DONE,		/* task finished */
	TASK_STATE_WAITING,	 /* allocated but waiting for available thread */
} TASK_STATE_ENUM;


typedef struct cmdqSecDataStruct {
	bool	 isSecure;			 /* [IN] true for secure command */
	uint32_t addrListLength;	   /* [IN] addrList length */
	uint32_t portListLength;	   /* [IN] portList length */
	/* [IN] address metadata, used to translate secure buffer address related instruction in secure world */
	void *addrList;
	/* [IN] address metadata, used to config HW' access sec/non-sec DRAM(i.e. sec port), pa/mva in secure world */
	void *portList;
} cmdqSecDataStruct;

typedef struct cmdqCommandStruct {
	uint32_t scenario;			  /* [IN] deprecated. will remove in the future. */
	uint32_t priority;			  /* [IN] task schedule priority. this is NOT HW thread priority. */
	uint32_t engineFlag;			/* [IN] bit flag of engines used. */
	uint32_t *pVABase;			  /* [IN] pointer to instruction buffer */
	uint32_t blockSize;			 /* [IN] size of instruction buffer, in bytes. */
	cmdqSecDataStruct secData;	  /* [IN] metadata about secure path */
} cmdqCommandStruct;

typedef struct TaskStruct {
	struct list_head	listEntry;

	TASK_TYPE_ENUM	  taskType;

	/* For buffer state */
	TASK_STATE_ENUM	 taskState;
	uint32_t			*pVABase;
	uint32_t			MVABase;
	uint32_t			firstEOC;
	uint32_t			bufSize;
	uint32_t			thread;

	/* For execution */
	int32_t			 scenario;
	int32_t			 priority;
	uint32_t			engineFlag;
	int32_t			 blockSize;
	uint32_t			*pCMDEnd;
	int32_t			 reorder;
	int32_t			 irqFlag;

	/* For seucre execution */
	cmdqSecDataStruct   secData;

	/* For statistics */
	struct timeval	 trigger;
	struct timeval	 gotIRQ;
	struct timeval	 wakedUp;
} TaskStruct;


typedef struct EngineStruct {
	int32_t			 userCount;
	int32_t			 currOwner;
	int32_t			 resetCount;
	int32_t			 failCount;
} EngineStruct;


typedef struct ThreadStruct {
	uint32_t			taskCount;
	uint32_t			waitCookie;	 /* the min cookie value which should be handled in ISR */
	uint32_t			nextCookie;	 /* the cookie value which will be assigned to a new task */
	TaskStruct * pCurTask[CMDQ_MAX_TASK_COUNT];
} ThreadStruct;


typedef struct RecordStruct {
	int32_t			 scenario;
	int32_t			 priority;
	int32_t			 reorder;
	bool				isSecure;

	struct timeval	 start;
	struct timeval	 trigger;
	struct timeval	 gotIRQ;
	struct timeval	 wakedUp;
	struct timeval	 done;
} RecordStruct;


typedef struct ErrorStruct {
	TaskStruct		  *pTask;
} ErrorStruct;


typedef struct WarngingStrcut {
	void *taskAddr;						/* warning task address. just for warning memo. */

	/* thread information */
	uint32_t			thread;			 /* the cmdq HW thread which is triggerd IRQ */
	uint32_t			taskCount;		  /* total task count in thread when IRQ received */
	uint32_t			cookie;			 /* hw thread executed counter */
	uint32_t			irqFlag;			/* IRQ value */
	struct timeval	  gotIRQ;			  /* time stamp get IRQ */
} WarngingStrcut;


typedef struct ContextStruct {
	/* Task information */
	struct list_head	 taskWaitList;	  /* Tasks waiting for available thread */

	/* Basic information */
	TaskStruct		  taskInfo[CMDQ_MAX_FIXED_TASK];
	EngineStruct		engine[CMDQ_MAX_ENGINE_COUNT];
	ThreadStruct		thread[CMDQ_MAX_THREAD_COUNT];

	/* Profile information */
	int32_t			 lastID;
	int32_t			 recNum;
	RecordStruct		record[CMDQ_MAX_RECORD_COUNT];

	/* Error information */
	int32_t			 errNum;
	ErrorStruct		 error[CMDQ_MAX_ERROR_COUNT];
	int32_t			 logLevel;

	/* Warning information */
	int32_t			 warningNum;
	WarngingStrcut	  warning[CMDQ_MAX_WARNING_COUNT];

	/* SW timeout setting */
	uint32_t swTimeoutDurationMS;
	uint32_t predumpStartTimeMS;
	uint32_t predumpDurationMS;
	uint32_t predumpMaxRetryCount;
} ContextStruct;


#ifdef __cplusplus
extern "C" {
#endif

int32_t cmdqRegisterCallback(int32_t		  index,
							 CMDQ_TIMEOUT_PTR pTimeout,
							 CMDQ_RESET_PTR   pReset);

void cmdqInitialize(struct device *dev);

int32_t cmdqSuspendTask(void);

int32_t cmdqResumeTask(void);

void cmdq_handle_done_unlocked(int32_t thread, uint32_t value, const bool attachWarning);
void cmdq_handle_error_unlocked(int32_t thread, uint32_t value, const bool attachWarning);
void cmdqHandleIRQ(int32_t thread, const uint32_t value);
void cmdqHandleError(int32_t thread, uint32_t value);
void cmdqHandleDone(int32_t thread, uint32_t value);

int32_t cmdqSubmitTask(cmdqCommandStruct *pCommandDesc);

void cmdqDeInitialize(void);

void cmdqResetHWEngine(const TaskStruct *pTask, int32_t thread);

bool cmdq_core_should_print_msg(void);
void cmdq_debug_set_progressive_timeout(bool enable);
void cmdq_debug_set_sw_timeout(
			const uint32_t sw_timeout_ms,
			const uint32_t predump_start_time_ms,
			const uint32_t predump_duration_ms);

const int32_t cmdq_can_start_to_acquire_HW_thread(const TaskStruct *pTask);
void cmdq_insert_to_thread_dispatch_queue(TaskStruct *pTask);
void cmdq_remove_from_thread_dispatch_queue(const TaskStruct *pTask);
/* extern void smi_dumpDebugMsg(void); */
/* extern void mt_irq_dump_status(int irq); */


#ifdef __cplusplus
}
#endif

#endif /* __KERNEL__ */

#endif  /* __DDP_CMDQ_H__ */
