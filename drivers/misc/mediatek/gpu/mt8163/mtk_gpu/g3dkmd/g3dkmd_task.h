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

#ifndef _G3DKMD_TASK_H_
#define _G3DKMD_TASK_H_

#if defined(_WIN32)
#include <stdint.h>
typedef void *HANDLE;
#else
#if defined(linux_user_mode)
#include <unistd.h>
#include <stdint.h>
#else
#include <linux/types.h>
#include <linux/sched.h>
#endif
#endif

#include "g3d_memory.h"
#include "g3dkmd_define.h"
#include "g3dkmd_util.h"
#include "g3dkmd_api.h"

#ifdef G3DKMD_SUPPORT_FENCE
#include "g3dkmd_fence.h" /* for fence */
#endif /* G3DKMD_SUPPORT_FENCE */

#ifdef USE_GCE
#include "g3dkmd_mmcq.h"
#include "g3dkmd_mmce.h"
#endif

#ifdef G3DKMD_SUPPORT_PM
#include "g3dkmd_pm.h"
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
#include "g3dkmd_power.h"
#endif

#include "platform/g3dkmd_platform_waitqueue.h"

struct g3dHtCommand {
	unsigned int command;
	unsigned int val0;
	unsigned int val1;
	unsigned int val2;
};

#ifdef PATTERN_DUMP
#define MAX_HT_COMMAND  512
#else
#define MAX_HT_COMMAND  128
#endif

#define HT_CMD_SIZE     (sizeof(struct g3dHtCommand))
#define HT_BUF_SIZE     (MAX_HT_COMMAND * HT_CMD_SIZE)
#define HT_BUF_LOW_THD  (1 * HT_CMD_SIZE)

#define G3DKMD_MAX_TASK_CNT 0xFF

#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
#define G3DKMD_MAX_HT_INSTANCE 2
#else
#define G3DKMD_MAX_HT_INSTANCE 1
#endif

struct g3dHtInfo {
	unsigned int rdPtr;	/* it's not the real time status of HW */
	/* it should only be used in g3dKmdCheckFreeHtSlots */
	unsigned int wtPtr;
	unsigned int prev_rdPtr;
	unsigned int readyPtr;
	unsigned char fenceSignaled[MAX_HT_COMMAND/8];
	G3DKMD_BOOL fenceUpdated;
	unsigned int recovery_polling_cnt;
	G3DKMD_BOOL recovery_first_attempt;
	struct g3dHtCommand *htCommand;
	unsigned int htCommandBaseHw;
};


enum g3dExeState {
	G3DEXESTATE_IDLE = 0,	/* idle    : no command */
	G3DEXESTATE_RUNNING = 1,	/* running : current executing one. */
	G3DEXESTATE_WAITQUEUE = 2,	/* waitque : waiting in queue for executing */
	G3DEXESTATE_INACTIVED = 3,	/* inactive: not join context switch yet. */
};

struct g3dRecoveryData {
	unsigned int stamp;
	unsigned int wPtr;
};

struct g3dRecoveryDataArray {
	struct g3dRecoveryData dataArray[MAX_HT_COMMAND];
};

struct G3dHtInst {
	unsigned int htBufferSize;
	G3dMemory htBuffer;
	struct g3dHtInfo *htInfo;
	G3DKMD_BOOL isUsed;	/* the n-th htInstance in current exeinstance. */
};

struct g3dExeInst {
	enum g3dExeState state;
	int hwChannel;		/* dispatch to which HW command channel */
	unsigned int refCnt;

/* unsigned int    htBufferSize; */
/* G3dMemory       htBuffer; */
/* struct g3dHtInfo *htInfo; */

	struct G3dHtInst htInstance[G3DKMD_MAX_HT_INSTANCE];	/* not access directly, only access via pointers: */
	/* pHtDataForSwWrite or pHtDataForHwRead. */
	struct G3dHtInst *pHtDataForSwWrite;	/* Used when preparing to add new Ht commands. */
	struct G3dHtInst *pHtDataForHwRead;	/* Used when preparing to ask hw do Ht commands. */

	unsigned int dfrBufFromUsr;
	G3dMemory defer_vary;
	G3dMemory defer_binHd;
	G3dMemory defer_binTb;
	G3dMemory defer_binBk;

	unsigned int qbaseBufferSize;
	G3dMemory qbaseBuffer;

	unsigned int rsmBufferSize;
	G3dMemory rsmBuffer;

	unsigned int rsmVpBufferSize;
	G3dMemory rsmVpBuffer;

	/* bool, if a task is urgent priority, it will always trigger context swith when it has data to render */
	unsigned char isUrgentPrio;

	unsigned int time_slice;
	unsigned int prioWeighting;

	/* command engine */
#ifdef USE_GCE
	MMCQ_HANDLE mmcq_handle;
	MMCE_THD_HANDLE thd_handle;
#endif

#ifdef PATTERN_DUMP
	int sDumpPtr;
	int sDumpIndexCount;
	void *fp_qbuf;
#endif

#ifdef G3DKMD_SNAPSHOT_PATTERN
	unsigned int htCommandFlushedCount;
	unsigned int htCommandFlushedCountOverflow;
#endif

#ifdef G3D_SWRDBACK_RECOVERY
	struct g3dRecoveryDataArray recoveryDataArray;
#endif

	unsigned int dataSynced;
#ifdef KERNEL_FENCE_DEFER_WAIT
	struct list_head fenceWaiter_list_head;
#endif
#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
	int childInstIdx;	/* EXE_INST_NONE: no child, */
	/* others       : the instIdx of child Inst. */
	int parentInstIdx;
	unsigned int tryEnlargeFailCnt;
#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */

};


#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION

#define G3DKMD_INST_MIGRATION_INIT(pInst) \
	do {(pInst)->childInstIdx  = EXE_INST_NONE; \
		(pInst)->parentInstIdx = EXE_INST_NONE; } while (0)

#define G3DKMD_INST_MIGRATION_ASSOCIATE(pInstParent, pInstChild, idxP, idxC)\
	do {(pInstParent)->childInstIdx = idxC; \
		(pInstChild)->parentInstIdx = idxP; } while (0)

#define G3DKMD_INST_MIGRATION_DISASSOCIATE_AND_WAIT_CLEAN_UP(pInstP, pInstC) \
	do {(pInstP)->childInstIdx = EXE_INST_MIGRATED_WAIT_CLEANUP; \
		(pInstC)->parentInstIdx = EXE_INST_NONE; } while (0)

#define G3DKMD_INST_MIGRATION_SET_NORMAL(pInst) G3DKMD_INST_MIGRATION_INIT(pInst)

#define G3DKMD_IS_INST_MIGRATED_WAIT_CLEANUP_SOURCE(pInst) \
	(EXE_INST_MIGRATED_WAIT_CLEANUP  == (pInst)->childInstIdx)

#define G3DKMD_IS_INST_MIGRATING(pInst) \
	((EXE_INST_NONE     != (pInst)->childInstIdx) && \
		(EXE_INST_NONE     != (pInst)->parentInstIdx))

#define G3DKMD_IS_INST_MIGRATING_SOURCE(pInst) \
	(EXE_INST_NONE      < (pInst)->childInstIdx)

#define G3DKMD_IS_INST_MIGRATING_TARGET(pInst) \
	(EXE_INST_NONE      < (pInst)->parentInstIdx)

#else

#define G3DKMD_INST_MIGRATION_INIT(pInst)
#define G3DKMD_INST_MIGRATION_ASSOCIATE(pInstParent, pInstChild, idxP, idxC)
#define G3DKMD_INST_MIGRATION_DISASSOCIATE_AND_WAIT_CLEAN_UP(pInstP, pInstC)
#define G3DKMD_INST_MIGRATION_SET_NORMAL(pInst)

#define G3DKMD_IS_INST_MIGRATED_WAIT_CLEANUP_SOURCE(pInst) G3DKMD_FALSE
#define G3DKMD_IS_INST_MIGRATING(pInst)                    G3DKMD_FALSE
#define G3DKMD_IS_INST_MIGRATING_SOURCE(pInst)             G3DKMD_FALSE
#define G3DKMD_IS_INST_MIGRATING_TARGET(pInst)             G3DKMD_FALSE
#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */

struct g3dKmdClPrintfInfo {
	unsigned int ctxsID;
	unsigned int isIsrTrigger;
	unsigned int waveID;
};


struct g3dTask {
	unsigned int task_idx;

#ifdef _WIN32
	int pid;		/* the related process to request create this task */
	int tid;		/* the related thread to request create this task */
	unsigned long PrioClass;
#elif defined(linux)
	pid_t pid;
	pid_t tid;
	int prio;
#endif

	unsigned int frameCmd_base;
	unsigned int frameCmd_size;
	unsigned int lastFlushStamp;

	unsigned int htCmdCnt;
	unsigned int accFlushHTCmdCnt;
	unsigned char isUrgentPrio;
	unsigned char isDVFSPerf;
#ifdef G3D_SUPPORT_HW_NOFIRE
	unsigned char isNoFire;
#endif

	unsigned int hw_reset_cnt;
	unsigned int inst_idx;	/* index to gExecInfo */

	/* execute */
#if 0
	int priorityWeighting;
	int missingCount;
#endif

#ifdef G3D_SUPPORT_FLUSHTRIGGER
	unsigned int flushTigger;
#endif

#ifdef ANDROID
	unsigned char isGrallocLockReq;
#endif

#ifdef G3DKMD_SUPPORT_FENCE
#ifdef SW_SYNC64
	struct sw_sync64_timeline *timeline;
	__u64 lasttime;
#else
	struct sw_sync_timeline *timeline;
	__u32 lasttime;
#endif
#endif

#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
	unsigned int tryAllocMemFailCnt; /* the number of DDfr Memory alloc fail. */
#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */

	unsigned int carry;
#if defined(linux) && !defined(linux_user_mode)
	char task_comm[TASK_COMM_LEN];
#endif
	struct g3dKmdClPrintfInfo clPrintfInfo;
};

enum eG3dExecuteMode {
	eG3dExecMode_single_queue = 0,
	eG3dExecMode_two_queues = 1,
	eG3dExecMode_per_task_queue = 2
};

struct g3dKmdSwReadBackData {
	unsigned int stamp;
	uintptr_t readPtr;
};

#define G3DKMD_HWCHANNEL_NONE  -1

enum g3dHwState {
	G3DKMD_HWSTATE_IDLE = 0,
	G3DKMD_HWSTATE_RUNNING = 1,
	G3DKMD_HWSTATE_HANG = 2
};

struct g3dKmdHwStatus {
	int inst_idx;		/* map HW channel to execute instance index */
	enum g3dHwState state;
};

/* only used in childInstIdx to mark that Inst's child joined CTX and ready to clean up. */
#define EXE_INST_MIGRATED_WAIT_CLEANUP   -2

#define EXE_INST_NONE       -1
#define EXE_INST_NORMAL      0 /* only used in g3dkmd_task.c */
#define EXE_INST_URGENT      1 /* only used in g3dkmd_task.c */

unsigned g3dKmdGetNormalExeInstIdx(void);
unsigned g3dKmdGetUrgentExeInstIdx(void);

struct g3dExecuteInfo {
	struct g3dKmdHwStatus hwStatus[NUM_HW_CMD_QUEUE]; /* map HW channel to task index */
	/* int enableHWCtxInterrupt; */

	int taskInfoCount; /* how many tasks have been registered */
	int taskAllocCount; /* how many tasks pointer slots have been allocated */
	struct g3dTask **taskList; /* array of pG3dTask in length of taskAllocCount */

	int exeInfoCount;
	int exeAllocCount;
	struct g3dExeInst **exeList;
	struct g3dExeInst *currentExecInst;

	/* eG3dExecMode_single_queue or eG3dExecMode_two_queues or eG3dExecMode_per_task_queue */
	enum eG3dExecuteMode exeuteMode;

	G3dMemory mmu_tbl_base;

#ifdef G3D_SUPPORT_SWRDBACK
	G3dMemory SWRdbackMem;
	struct g3dKmdSwReadBackData swq_bak[MAX_HT_COMMAND];
#endif /* G3D_SUPPORT_SWRDBACK */

#ifdef G3DKMD_HACK_FRAME_CMD_DMA
	G3dMemory DMAFrameCmdMem;
#endif /* G3DKMD_HACK_FRAME_CMD_DMA */

	/* wait queue */
	/* non-polling flush command */
#if defined(ENABLE_NON_POLLING_FLUSHCMD)
	g3dkmd_wait_queue_head_t s_flushCmdWq;	/* used by g3dkmdFlushCmd() and IsrTop() */
#endif

	/* non-polling Qload Wait Queue: */
#if defined(ENABLE_QLOAD_NON_POLLING)
	g3dkmd_wait_queue_head_t s_QLoadWq;
	int s_isHwQloadDone;
#endif

#if defined(linux) && !defined(linux_user_mode)
	G3dMemory *pDMAMemory;
#endif
	void *mmu;

#ifdef G3DKMD_SUPPORT_FDQ
	G3dMemory fdq;
	unsigned int fdq_size;
	unsigned int fdq_rptr;
	unsigned int fdq_wptr;
#endif

#ifdef G3DKMD_SUPPORT_PM
	G3dMemory pm;
	G3dMemory dfd;
	G3DKMD_KMIF_OPS kmif_ops;
	G3DKMD_KMIF_MEM_DESC kmif_mem_dec;
	G3DKMD_KMIF_MEM_DESC kmif_dfd_mem_dec;
	int kmif_shader_debug_enable;
	int kmif_dfd_debug_enable;
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(G3DKMD_POWER_MANAGEMENT)
	enum G3DKMD_PWRSTATE PwrState;
#endif

	/* variables for context switch */
#ifdef G3DKMD_SCHEDULER_OS_CTX_SWTICH
#ifdef linux
	struct task_struct *pSchThd;
	wait_queue_head_t s_SchedWq;
	unsigned int activateSch;
#elif defined(_WIN32)
	HANDLE hStopThd, hActThd;
	HANDLE pSchThd;
#endif
#endif /* G3DKMD_SCHEDULER_OS_CTX_SWTICH */
/* variables for CL print sleeping mechanism */
#ifdef G3DKMD_SUPPORT_CL_PRINT
	g3dkmd_wait_queue_head_t s_waitqueue_head_cl;
	/* wait_queue_t s_waitqueue_cl; */
#endif

#ifdef G3DKMD_RECOVERY_BY_TIMER
#ifdef G3DKMD_RECOVERY_SIGNAL
#if defined(linux) && !defined(linux_user_mode)
	pid_t current_tid;
#endif
#endif /* G3DKMD_RECOVERY_SIGNAL */
#endif /* G3DKMD_RECOVERY_BY_TIMER */

#ifdef G3DKMD_SIGNAL_CL_PRINT
	unsigned int cl_print_read_stamp;	/* update by CL user mode driver */
#endif


#ifdef G3DKMD_HACK_CMDQ_NONCACHE
#if defined(linux) && !defined(linux_user_mode)
	struct page **pageList;
	unsigned int page_num;
#endif
#endif /* G3DKMD_HACK_CMDQ_NONCACHE */

	unsigned int mmuTransFault;
};

extern struct g3dExecuteInfo gExecInfo;

void g3dKmdInitTaskList(void);
void g3dKmdDestroyTaskList(void);
struct g3dTask *g3dKmdCreateTask(unsigned int bufferStart, unsigned int size, unsigned int flag);
int g3dKmdRegisterNewTask(struct g3dExecuteInfo *execute, struct g3dTask *task);
void g3dKmdUnregisterAndDestroyTask(struct g3dTask *task, struct g3dExecuteInfo *execute, int taskID);

void g3dKmdTaskAssociateInstIdx(struct g3dTask *task, const unsigned int exe_index);
void g3dKmdTaskDissociateInstIdx(struct g3dTask *task);
struct g3dTask *g3dKmdGetTask(int taskID);

#ifdef G3DKMD_SUPPORT_FENCE
static INLINE void g3dKmdTaskCreateFence(int taskID, int *fd, uint64_t stamp) __attribute__ ((always_inline));
static INLINE void g3dKmdTaskCreateFence(int taskID, int *fd, uint64_t stamp)
{
	g3dkmdCreateFence(taskID, fd, stamp);
}

static INLINE void g3dKmdTaskUpdateTimeline(unsigned int taskID, unsigned int fid) __attribute__ ((always_inline));
static INLINE void g3dKmdTaskUpdateTimeline(unsigned int taskID, unsigned int fid)
{
	g3dkmdUpdateTimeline(taskID, fid);
}

void g3dkmd_create_task_timeline(int taskID, struct g3dTask *task);
#endif /* G3DKMD_SUPPORT_FENCE */

/* struct g3dHtInfo *_g3dKmdCreateHtInfo(void); */
/* void _g3dKmdDestroyHtInfo(struct g3dHtInfo *list); */
int g3dKmdGetQueuedHtSlots(int hwIndex, struct g3dHtInfo *ht);
int g3dKmdCheckFreeHtSlots(int hwIndex, struct g3dHtInfo *ht);

#if defined(ENABLE_NON_POLLING_FLUSHCMD)
int g3dKmdCheckIfEnoughHtSlots(int hwIndex, struct g3dHtInfo *ht);
#endif

void g3dKmdCopyCmd(struct g3dHtInfo *list, struct g3dHtCommand *pSrcCmd);
void g3dKmdAddHTCmd(struct g3dExeInst *pInst, void *head, void *tail, unsigned int task_id, int fence);
/* int  g3dKmdCaculateIndex(struct g3dHtInfo *list,unsigned int wptr,int index); */
unsigned char g3dKmdIsHtInfoEmpty(int hwIndex, struct g3dHtInfo *ht);
unsigned char g3dKmdIsFiredHtInfoDone(int hwIndex, struct g3dHtInfo *ht);
void g3dKmdInitQbaseBuffer(struct g3dExeInst *pInst);
void g3dKmdUpdateHtInQbaseBuffer(struct g3dExeInst *pInst, unsigned char isUpdateHtRdPtr);

#ifdef CMODEL
const void *g3dKmdGetQreg(unsigned int taskID);
#endif

#define EXE_INST_URGENT_PRIO_THRESHOLD  30	/* assume the first 30 htCmd will goes to urgent queue */

void g3dKmdExeInstInit(void);
void g3dKmdExeInstUninit(void);

unsigned int g3dKmdExeInstCreateWithState(G3DKMD_DEV_REG_PARAM *param, const enum g3dExeState state_in);
#define g3dKmdExeInstCreate(param) g3dKmdExeInstCreateWithState(param, G3DEXESTATE_IDLE)
void g3dKmdExeInstRelease(unsigned int instIdx);

#define g3dKmdExeInstSetState(pInst, state) ((pInst)->state = (state))


#define G3DKMD_IS_EXEINST_HAS_NO_ASSOCIATED_TASK(pInst) (0 == (pInst)->refCnt)
/* void g3dKmdExeInstIncRef(unsigned int instIdx); */
/* void g3dKmdExeInstDecRef(unsigned int instIdx); */

void g3dKmdExeInstSetPriority(const unsigned int instIdx, const unsigned char isUrgent);
struct g3dExeInst *g3dKmdExeInstGetInst(unsigned int instIdx);
unsigned int g3dKmdExeInstGetIdx(struct g3dExeInst *pInst);

struct g3dExeInst *g3dKmdTaskGetInstPtr(unsigned int taskID);
int g3dKmdTaskGetInstIdx(unsigned int taskID);
void g3dKmdTaskUpdateInstIdx(struct g3dTask *task);


#ifdef G3DKMD_ENABLE_TASK_EXEINST_MIGRATION
void g3dKmdTriggerCreateExeInst_workqueue(const int taskID,
					  const G3DKMD_BOOL isMigratingAllTasks,
					  const unsigned int boff_va_ovf_cnt,
					  const unsigned int boff_bk_ovf_cnt,
					  const unsigned int boff_tb_ovf_cnt,
					  const unsigned int boff_hd_ovf_cnt);
void g3dKmdRequestExeInstRelease(const int instIdx);

/* =================================================================== /// */
/* The gobal data for task migration */
/* */
/* MigrationCnt: */
/*    Use to record the number of tasks is migration. */
/*    The migration Count APIs don't require atomic counter, because these */
/*    APIs need within lock_g3d() & unlock_g3d() */
/* void _g3dKmdInitMigrationData(void); */
void _g3dKmdIncreaseMigrationCount(void);
void _g3dKmdDecreaseMigrationCount(void);
unsigned char _g3dKmdTestMigrationCountLessEq(const int val);

#else
#define g3dKmdTriggerCreateExeInst_workqueue(...)

#endif /* G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */



void g3dKmdReleaseHtInst(struct G3dHtInst *pHtInst);


#endif /* _G3DKMD_TASK_H_ */
