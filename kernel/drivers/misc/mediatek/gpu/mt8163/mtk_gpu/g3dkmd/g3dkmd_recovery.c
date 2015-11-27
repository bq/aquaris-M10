#ifdef G3DKMD_RECOVERY_SIGNAL
#include <signal.h>
#endif
#include <linux/time.h>
#include <linux/interrupt.h>

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#include "g3dkmd_recovery.h"
#include "g3dkmd_file_io.h" // to use GKMDDPF
#include "g3dkmd_util.h"
#include "g3dkmd_api.h"
#include "g3dkmd_isr.h"
#ifdef G3DKMD_SUPPORT_PM
#include "gpad/gpad_kmif.h"
#endif

#define G3DKMD_RECOVERY_SHARE_CNT
#define G3DKMD_RECOVERY_WQ


extern g3dExecuteInfo gExecInfo; //(1). read ht (2).hack for signal 

#ifdef G3DKMD_RECOVERY_BY_TIMER

#define MAX_POLLING_THRESHOLD 5//10

static struct timer_list recovery_timer; // Timer list.
static unsigned int recovery_timer_interval = 500;
static unsigned int polling_count = 0;   // Counter.

#ifdef G3DKMD_RECOVERY_WQ
static void g3dKmd_recovery_workqueue(struct work_struct *work)
{    
    pG3dExeInst pInst = gExecInfo.currentExecInst;
    
    if (!pInst)
    {
        KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "pInst is NULL\n");
        return;
    }

    lock_g3d();
    disable_irq(G3DHW_IRQ_ID);
    g3dKmdSuspendIsr();

    //if (pInst->pHtDataForHwRead->htInfo->recovery_first_attempt)
    //{        
    //    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "First Hang => Retry HT with read ptr 0x%x\n", g3dKmdRegGetReadPtr(pInst->hwChannel));  
    //    g3dKmdRegRestart(-1, G3DKMD_RECOVERY_RETRY);
    //    pInst->pHtDataForHwRead->htInfo->recovery_first_attempt = G3DKMD_FALSE;
    //}
    //else
    //{
        KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "Second Hang => Skip HT with read ptr 0x%x\n", g3dKmdRegGetReadPtr(pInst->hwChannel));
        g3dKmdRegRestart(-1, G3DKMD_RECOVERY_SKIP);
    //    pInst->pHtDataForHwRead->htInfo->recovery_first_attempt = G3DKMD_TRUE;
    //}

#ifdef G3DKMD_RECOVERY_SHARE_CNT
    polling_count = 0;
#else // ! G3DKMD_RECOVERY_SHARE_CNT
    {
    unsigned int iter_inst = 0;
    for (iter_inst = 0; iter_inst < gExecInfo.exeInfoCount; iter_inst++)
    {
        gExecInfo.exeList[iter_inst]->pHtDataForHwRead->htInfo->recovery_polling_cnt = 0;
    }
    }
#endif //G3DKMD_RECOVERY_SHARE_CNT

    enable_irq(G3DHW_IRQ_ID);
    g3dKmdResumeIsr();
    unlock_g3d();
}

static DECLARE_WORK(mtk_gpu_recovery_work, g3dKmd_recovery_workqueue);
#endif // G3DKMD_RECOVERY_WQ

void g3dKmd_recovery_timer_uninit(void)
{
    del_timer(&recovery_timer);
}

static void g3dKmd_recovery_timer_callback(unsigned long unUsed)
{
    pG3dExeInst pInst = gExecInfo.currentExecInst;

    if (pInst && pInst->pHtDataForHwRead->htInfo && (pInst->hwChannel != G3DKMD_HWCHANNEL_NONE))
    {
        lock_hang_detect();

        if (gExecInfo.hwStatus[pInst->hwChannel].state != G3DKMD_HWSTATE_HANG)
        {
            g3dKmd_recovery_detect_hang(pInst);

#ifdef G3DKMD_RECOVERY_SHARE_CNT
            if (polling_count > MAX_POLLING_THRESHOLD)
#else  // !G3DKMD_RECOVERY_SHARE_CNT
            if (pInst->pHtDataForHwRead->htInfo->recovery_polling_cnt > MAX_POLLING_THRESHOLD)
#endif // G3DKMD_RECOVERY_SHARE_CNT
            {
                gExecInfo.hwStatus[pInst->hwChannel].state = G3DKMD_HWSTATE_HANG;

                #ifdef G3DKMD_DUMP_HW_HANG_INFO
                g3dKmdQueryHWIdleStatus(G3DKMD_SW_DETECT_HANG);
                #endif

                g3dKmd_recovery_reset_hw(pInst);
            }
        }

        unlock_hang_detect();
    }

    recovery_timer.expires = jiffies + recovery_timer_interval * HZ / 1000;
    add_timer(&recovery_timer);
}

void g3dKmd_recovery_timer_init(void)
{
    init_timer(&recovery_timer);
    recovery_timer.expires = jiffies + recovery_timer_interval * HZ / 1000;
    recovery_timer.function = g3dKmd_recovery_timer_callback;
    add_timer(&recovery_timer);
}

void g3dKmd_recovery_detect_hang(pG3dExeInst pInst)
{
    unsigned task_cnt = gExecInfo.taskInfoCount;
    unsigned int pa_idle0, pa_idle1, cq_wptr, cq_rptr, irq_raw;

    if (pInst == NULL || task_cnt  == 0)
    {
        //KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "pInst = %x, PID = %d, Task Count = %d, return\n", pInst, (int)pid, task_cnt);
        return;
    }

    lock_g3dPwrState();
    if(gExecInfo.PwrState != G3DKMD_PWRSTATE_LATE_RESUME) 
    {
        unlock_g3dPwrState();
        return;
    }

    pa_idle0 = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS0, 0xFFFFFFFF, 0);
    pa_idle1 = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_PA_IDLE_STATUS1, 0xFFFFFFFF, 0);
    irq_raw = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW,  0xFFFFFFFF, 0);
    cq_wptr = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_CQ_HTBUF_WPTR, MSK_AREG_CQ_HTBUF_WPTR, SFT_AREG_CQ_HTBUF_WPTR);
    cq_rptr = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_CQ_DONE_HTBUF_RPTR, MSK_AREG_CQ_DONE_HTBUF_RPTR, SFT_AREG_CQ_DONE_HTBUF_RPTR);
    unlock_g3dPwrState();

#if 0//def G3DKMD_SUPPORT_ISR
    lock_isr(); // to avoid reseting another thread that we don't wat
#endif

    // Check g3d engine idle with (rdPtr != wtPtr) || (prev_rdPtr != rdPtr) conditions
    if ((cq_wptr == cq_rptr) || (pInst->pHtDataForSwWrite->htInfo->prev_rdPtr != pInst->pHtDataForSwWrite->htInfo->rdPtr) )
    {
#ifdef G3DKMD_RECOVERY_SHARE_CNT
        KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "is_idle 0x%x, prev_rdPtr 0x%x, cur_rdPtr 0x%x\n", 
           pa_idle0 & 0x01, pInst->pHtDataForSwWrite->htInfo->prev_rdPtr, pInst->pHtDataForSwWrite->htInfo->rdPtr);
        KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "IDLE0 0x%x, IDLE1 0x%x, RAW 0x%x\n", pa_idle0, pa_idle1, irq_raw);
        polling_count = 0;

#else // ! G3DKMD_RECOVERY_SHA RE_CNT

        unsigned iter_inst = 0;

        for (iter_inst = 0; iter_inst < gExecInfo.exeInfoCount; iter_inst++)
        {
            gExecInfo.exeList[iter_inst]->pHtDataForHwRead->htInfo->recovery_polling_cnt = 0;
        }

#endif // G3DKMD_RECOVERY_SHARE_CNT

        //KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "HW alive\n");
    }
    else
    {    
#ifdef G3DKMD_RECOVERY_SHARE_CNT
        ++polling_count;
        KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "[%s] HW hang cnt %d\n", __FUNCTION__, polling_count);
#else // ! G3DKMD_RECOVERY_SHARE_CNT
        ++pInst->htInfo->recovery_polling_cnt;
        KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "[%s] HW hang inst %d, cnt %d\n", __FUNCTION__, g3dKmdExeInstGetIdx(pInst), pInst->htInfo->recovery_polling_cnt);
#endif // G3DKMD_RECOVERY_SHARE_CNT

        KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "HW hang\n");
    }
    
    pInst->pHtDataForHwRead->htInfo->prev_rdPtr = pInst->pHtDataForHwRead->htInfo->rdPtr;

#if 0//def G3DKMD_SUPPORT_ISR
    unlock_isr();
#endif
}

void g3dKmd_recovery_reset_hw(pG3dExeInst pInst)
{
#ifndef G3DKMD_RECOVERY_WQ
    unsigned int iter_inst = 0;
#endif // !G3DKMD_RECOVERY_WQ

    if (pInst == NULL)
    {
        return;
    }

#ifdef G3DKMD_RECOVERY_WQ 

    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "Reset HW via workqueue\n");
    schedule_work(&mtk_gpu_recovery_work);

#else // ! G3DKMD_RECOVERY_WQ

    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "Reset HW via function calls\n");
    // first hang: retry this ht, otherwise skip this ht
    //if (pInst->pHtDataForHwRead->htInfo->recovery_first_attempt)
    //{
    //    unsigned rptr_bak = g3dKmdRegGetReadPtr(pInst->hwChannel);
    //    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "[%s] First Hang => Retry HT with read ptr 0x%x\n", __FUNCTION__, rptr_bak);  
    //    g3dKmdRegRestart(-1, G3DKMD_RECOVERY_RETRY);
    //    pInst->htInfo->recovery_first_attempt = G3DKMD_FALSE;
    //}
    //else
    //{
        unsigned rptr_bak = g3dKmdRegGetReadPtr(pInst->hwChannel);
        KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "Second Hang => Skip HT with read ptr 0x%x\n", rptr_bak);
        g3dKmdRegRestart(-1, G3DKMD_RECOVERY_SKIP);
    //    pInst->pHtDataForHwRead->htInfo->recovery_first_attempt = G3DKMD_TRUE;
    //}

#ifdef G3DKMD_RECOVERY_SHARE_CNT
    polling_count = 0;
#else // ! G3DKMD_RECOVERY_SHARE_CNT
    for (iter_inst = 0; iter_inst < gExecInfo.exeInfoCount; iter_inst++)
    {
        gExecInfo.exeList[iter_inst]->pHtDataForHwRead->htInfo->recovery_polling_cnt = 0;
    }
#endif //G3DKMD_RECOVERY_SHARE_CNT

#endif // ! G3DKMD_RECOVERY_WQ

#ifdef G3DKMD_RECOVERY_SIGNAL
    // try to send a signal to user space, target is _g3dDeviceSignalHandlerForRecovery()
    {
    pid_t pid = gExecInfo.current_tid;
    if (pid != 0)
    {
        struct siginfo info ; 
        struct task_struct * pTask;
    
        pTask =  pid_task( find_vpid(pid), PIDTYPE_PID);
        
        if (pTask == NULL || pid == 0)
        {
            KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "pTask is NULL or pid is zero\n");
            return;
        }

        KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "Send recovery signal to proc %d\n", pid);  
        info.si_signo = SIGUSR1;
        info.si_pid = pid;
        info.si_code = -1;     //__SI_RT;
        info.si_int = 0x1234;  // user data
        send_sig_info (SIGUSR1, &info, pTask) ; 
    }
    }
#endif // G3DKMD_RECOVERY_SIGNAL
}

#endif // G3DKMD_RECOVERY_BY_TIMER

#ifdef G3D_SWRDBACK_RECOVERY
void g3dKmdRecoveryAddInfo(pG3dExeInst pInst, unsigned int ptr, unsigned int stamp)
{
    unsigned int widx = (pInst->pHtDataForHwRead->htInfo->wtPtr - pInst->pHtDataForHwRead->htInfo->htCommandBaseHw)/HT_CMD_SIZE;

#if 1 // debug info
    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "[STAMP INFO] insert to arr[%d].ptr with val 0x%x\n", widx, ptr);
    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "[STAMP INFO] insert to arr[%d].stamp with val 0x%x\n", widx, stamp);
#endif
    
    // backup the necessary data such as stamp and ptr for recovery
    pInst->recoveryDataArray.dataArray[widx].wPtr = ptr;
    pInst->recoveryDataArray.dataArray[widx].stamp = stamp;
}

void g3dKmdRecoveryUpdateSWReadBack(pG3dExeInst pInst, unsigned int rptr)
{
    int taskID;
    unsigned int pLastFlushPtr, pLastFlushStamp, wIdx;
    unsigned char *pCmd;
    g3dFrameSwData* pSwData;

    if ((pInst == NULL) || (pInst->hwChannel >= NUM_HW_CMD_QUEUE)) 
    {
        return;
    }

    pCmd    = (unsigned char*)pInst->pHtDataForHwRead->htInfo->htCommand +    //sw base
              (rptr - pInst->pHtDataForHwRead->htInfo->htCommandBaseHw);      // hw offset
    taskID  = ((pG3dHtCommand)pCmd)->command;               // first 4B is the taskID
    wIdx    = (rptr - pInst->pHtDataForHwRead->htInfo->htCommandBaseHw) / HT_CMD_SIZE;

    pLastFlushPtr = pInst->recoveryDataArray.dataArray[wIdx].wPtr;
    pLastFlushStamp = pInst->recoveryDataArray.dataArray[wIdx].stamp;

    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "write wPtr 0x%x back to sw read back mem\n", pLastFlushPtr);
    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "write stamp 0x%x back to sw read back mem\n", pLastFlushStamp);

    pSwData = (g3dFrameSwData*)((unsigned char *)gExecInfo.SWRdbackMem.data + G3D_SWRDBACK_SLOT_SIZE * taskID);
    pSwData->readPtr = (uintptr_t)pLastFlushPtr;
    pSwData->stamp = pLastFlushStamp;

#ifdef G3DKMD_SUPPORT_FENCE
    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "g3dKmdTaskUpdateTimeline taskID %d, stamp %d\n", taskID, pLastFlushStamp);
    g3dKmdTaskUpdateTimeline(taskID, pLastFlushStamp & 0xFFFF);
#endif

    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_RECOVERY, "update recovery stamp done\n");
}
#endif // G3D_SWRDBACK_RECOVERY



