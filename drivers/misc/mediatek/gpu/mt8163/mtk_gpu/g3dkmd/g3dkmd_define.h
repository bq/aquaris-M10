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

#include "g3dbase_common_define.h"
#ifndef _G3DKMD_DEFINE_H_
#define _G3DKMD_DEFINE_H_


#ifdef FPGA_G3D_HW
#define G3D_HW
#endif


/* Config keys */
/* #if !defined(__KERNEL__) //&& defined(CMODEL) */
#define PATTERN_DUMP
/* #endif */

#if defined(ANDROID) && !defined(linux_user_mode)
#define USING_MISC_DEVICE
#define G3DKMD_SUPPORT_FENCE
#endif

#ifndef ANDROID_PREF_BYPASS
#define G3DKMD_SUPPORT_ISR
#define G3DKMD_SUPPORT_FDQ

#if defined(linux) && !defined(linux_user_mode)
#if !(defined(FPGA_fpga8163) || defined(FPGA_fpga8163_64))
#define G3DKMD_SUPPORT_AUTO_RECOVERY
#endif
#endif

#ifdef G3DKMD_SUPPORT_FENCE
#define G3DKMD_SUPPORT_FDQ
#endif

#ifdef G3DKMD_SUPPORT_FDQ
#define G3DKMD_SUPPORT_ISR
#endif

#ifdef G3DKMD_RECOVERY_BY_IRQ
#define G3DKMD_SUPPORT_ISR
#endif
#endif

#if defined(linux) && !defined(linux_user_mode) && defined(G3DKMD_SUPPORT_FDQ)
#define G3DKMD_FPS_BY_FDQ
#endif

#define G3DKMD_SUPPORT_ECC

#define G3DKMD_SUPPORT_PM

#define G3DKMD_SUPPORT_HW_DEBUG

#ifndef REL_GLC			/* for release GLC tool */
#define G3DKMD_SNAPSHOT_PATTERN	/* shrchin */
#endif

#ifndef FPGA_G3D_HW
#define USE_GCE
#endif

#if defined(linux) && !defined(linux_user_mode) && !defined(FPGA_G3D_HW)
#define G3DKMD_SCHEDULER_OS_CTX_SWTICH
#endif

#if defined(FPGA_G3D_HW)
  /* #define G3DKMD_SUPPORT_AUTO_RECOVERY */
#if !defined(ANDROID_UNIQUE_QUEUE)
#define G3DKMD_SCHEDULER_OS_CTX_SWTICH
#endif
  /* #define G3DKMD_SUPPORT_ISR */
  /* #define G3DKMD_SUPPORT_FDQ */
  /* #define G3DKMD_SUPPORT_RESET_BY_IRQ */
  /* #define G3DKMD_SUPPORT_SYNC_SW */
  /* #define G3DKMD_SUPPORT_AUTO_RECOVERY */
  /* #define G3DKMD_HACK_FRAME_CMD_DMA */
  /* #define G3DKMD_AUTO_RECOVERY */
  /* #define G3DKMD_RECOVERY_SIGNAL */
  /* #define G3DKMD_HACK_CMDQ_NONCACHE */
  /* #define G3DKMD_SIGNAL_CL_PRINT */
  /* #define G3DKMD_SLEEP_CL_PRINT */
  /* #define G3DKMD_LTC_IN_ORDER_MODE */
  /* #define G3DKMD_SINGLE_COMMAND_FOR_PARSER */
  /* #define G3DKMD_AREG_RAND_FAIL_WORKAROUND */
#endif

#ifdef G3DKMD_SUPPORT_AUTO_RECOVERY
#define G3DKMD_RECOVERY_BY_IRQ
#define G3DKMD_RECOVERY_BY_TIMER
/* #ifdef G3DKMD_SNAPSHOT_PATTERN */
/* #define G3DKMD_RECOVERY_DETECT_ONLY */
/* #endif */
#endif

/* #define G3DKMD_ENABLE_TASK_EXEINST_MIGRATION */


#if defined(G3DKMD_ENABLE_TASK_EXEINST_MIGRATION)

#define G3DKMD_BOFF_VA_THRLHLD 3u
#define G3DKMD_BOFF_BK_THRLHLD 3u
#define G3DKMD_BOFF_TB_THRLHLD 3u
#define G3DKMD_BOFF_HD_THRLHLD 3u

/* reference
    #define G3DKMD_MAP_VX_VARY_SIZE             (2*1024*1024)
    #define G3DKMD_MAP_VX_BINBK_SIZE            (2*1024*1024)
    #define G3DKMD_MAP_VX_BINTB_SIZE            (300*1024)
    #define G3DKMD_MAP_VX_BINHD_SIZE            (8*16384)
*/

#define G3DKMD_BOFF_VA_INC_SIZE (1024u * 1024u)	/* 1M */
#define G3DKMD_BOFF_BK_INC_SIZE (128u * 1024u)	/* 128K */
#define G3DKMD_BOFF_TB_INC_SIZE (32u * 1024u)	/* 32K */
#define G3DKMD_BOFF_HD_INC_SIZE (1u * 1024u)	/* 1K */

#define G3DKMD_DDFR_VA_MAX (0x4600000)
#define G3DKMD_DDFR_BK_MAX (0x500000)
#define G3DKMD_DDFR_TB_MAX (0x4b000)
#define G3DKMD_DDFR_HD_MAX (0x20000)

#define G3DKMD_DDFR_TASK_TRY_MEM_FAIL_MAX  10
#define G3DKMD_DDFR_INST_ENLARGE_FAIL_MAX  3
#define G3DKMD_DDFR_SYSRAM_REJECT_THRLHLD  (50u * 1024u)	/* Kbytes */

#endif /* defined(G3DKMD_ENABLE_TASK_EXEINST_MIGRATION) */


/* [non-polling flush command] for linux_ko only.
 *
 *  [Louis 2014/08/14]:
 *  - Avoid busy waiting (busy polling) when FlushCmd.
 *    Let the process goes to wait status until there is enough Ht slots.
 *    The sleepy process will be wake up when HW send a Frame_End interrupt.
 *
 *    All sleeping processes are storing in a g3dkmd_wait_queue(gExecInfo.s_flushCmdWq).
 *
 *  - Testing
 *    Eanble run-time simple testing, dmesg will show the information.
 */
#define ENABLE_NON_POLLING_FLUSHCMD
/* #define TEST_NON_POLLING_FLUSHCMD */

 /* The default time_out for flushCmd in the waiting queue. */
#define G3DKMD_NON_POLLING_FLUSHCMD_WAIT_TIMEOUT      1000

 /* The default time_out for clprint in the waiting queue. */
#define G3DKMD_NON_POLLING_CLPRINT_WAIT_TIMEOUT       1000
/* non-polling flushCmd check dependence */
#if defined(TEST_NON_POLLING_FLUSHCMD)
#if !defined(ENABLE_NON_POLLING_FLUSHCMD)
#error TEST_NON_POLLING_FLUSHCMD require ENABLE_NON_POLLING_FLUSHCMD
#elif !(defined(linux) & !defined(linux_user_mode))
#error non-polling flushcmd can only works on linux_ko due to g3d_wait_queue only works on linux_ko.
#endif
#endif

/* [non-polling Qload] for linux_ko only
 *
 * [Louis 2014/08/26]:
 * - Avoid busy waiting (polling) in g3dKmdQload(), wihch is waiting for a reg
 *   value.
 *   Using g3dkmd wait_event to solve this problem. Process will wait a signal
 *   from IsrTop() when interrupt "VAL_IRQ_PA_FRAME_END" is coming.
 *
 * - Testing
 *   Enable run-time testing. Reports will show in dmesg.
 *   testcase is in g3dkmd/test/tester_non_polling_qload.c, using tester
 *   framework.
 */
/* Keys: */
/* QLoad Timeout only works in kernel */
/* #define ENABLE_QLOAD_NON_POLLING */
/* G3D self testers */
/* #define ENABLE_TESTER_NON_POLLING_QLOAD */

/* values: */
#ifdef ENABLE_QLOAD_NON_POLLING
    /* Qload Timeout */
#define G3DKMD_QLOAD_WAIT_TIMEOUT_MS 1000	/* 1.0s */
#endif

/* non-polling QLoad check dependence: */
#if defined(ENABLE_QLOAD_NON_POLLING) && !defined(G3DKMD_SUPPORT_ISR)
#error QLoad non-polling requires ISR intterrupt.
#endif

#define G3DKMD_SUPPORT_CL_PRINT
#ifdef G3DKMD_SUPPORT_CL_PRINT
    /* #define G3DKMD_SIGNAL_CL_PRINT */
    /* #define G3DKMD_SLEEP_CL_PRINT */
#endif
#if defined(ENABLE_TESTER_NON_POLLING_QLOAD)
#define ENABLE_TESTER		/* require tester framework. */
#if !defined(ENABLE_QLOAD_NON_POLLING)
#error ENABLE_TESTER_NON_POLLING_QLOAD require ENABLE_QLOAD_NON_POLLING
#elif !(defined(linux) & !defined(linux_user_mode))
#error non-polling QLoad can only works on linux_ko due to g3d_wait_queue only works on linux_ko.
#endif
#endif


#define G3DKMD_RECOVERY_RETRY 0
#define G3DKMD_RECOVERY_SKIP  1

#define G3DKMD_CHECK_TASK_DONE_TIMEOUT_CNT 100

#define NUM_HW_CMD_QUEUE    1

#define MMU_NONE            0
#define MMU_USING_GPU       1
#define MMU_USING_M4U       2
#define MMU_USING_WCP       3

#ifdef FPGA_G3D_HW
#define USE_MMU             MMU_USING_WCP
#else
#define USE_MMU             MMU_USING_M4U
#endif

#define G3DKMD_FAKE_M4U

#define G3DKMD_SUPPORT_IOMMU
#ifdef G3DKMD_SUPPORT_IOMMU
	/* #define G3DKMD_SUPPORT_EXT_IOMMU */
#define G3DKMD_SUPPORT_IOMMU_L2
#endif

#define G3DKMD_PHY_HEAP		/* the way to have a physical memory heap for c model and pattern */

/* the below two defines are for "palladium/ driver cowork" */
/* #define G3DKMD_FAKE_HW_ADDR */
/* #define G3DKMD_SUPPORT_MSGQ */
#if (USE_MMU != MMU_NONE) && defined(G3DKMD_FAKE_HW_ADDR)
#error "G3DKMD_FAKE_HW_ADDR & USE_MMU are exclusive"
#endif

#ifdef QEMU64
#define G3DKMD_EXEC_MODE    eG3dExecMode_single_queue	/* reference to eG3dExecuteMode */
#else
/* #define G3DKMD_EXEC_MODE    eG3dExecMode_single_queue    // reference to eG3dExecuteMode */
#define G3DKMD_EXEC_MODE    eG3dExecMode_two_queues
/* #define G3DKMD_EXEC_MODE    eG3dExecMode_per_task_queue */
#endif

#if defined(PATTERN_DUMP) || defined(ANDROID_UNIQUE_QUEUE)
/* #undef G3DKMD_EXEC_MODE */
/* #define G3DKMD_EXEC_MODE    eG3dExecMode_single_queue */
#endif

#define KMALLOC_UPPER_BOUND  (1024*1024*4)

/* VX related setting */
#define VX_VW_BUF_MGN_SIZE      0x10	/* K-byte */

/* #define G3DKMD_MSG_ERROR    (1<<0) */
/* #define G3DKMD_MSG_WARNING  (1<<1) */
/* #define G3DKMD_MSG_COMMAND  (1<<2) */
/* #define G3DKMD_MSG_NOTICE   (1<<3) */


/* Log Level, (base on linux kernel) */
/* using bits 0~3 */
#define G3DKMD_LLVL_EMERG       0	/* KERN_EMERG */
#define G3DKMD_LLVL_ALET        1	/* KERN_ALET */
#define G3DKMD_LLVL_CRIT        2	/* KERN_CRIT */
#define G3DKMD_LLVL_ERROR       3	/* KERN_ERR */
#define G3DKMD_LLVL_WARNING     4	/* KERN_WARNIN */
#define G3DKMD_LLVL_NOTICE      5	/* KERN_NOTICE   () */
#define G3DKMD_LLVL_INFO        6	/* KERN_INFO     (api, command) */
#define G3DKMD_LLVL_DEBUG       7	/* KERN_DEBUG    (debug) */

#define G3DKMD_LLVL_MASK        0xF


/* Log types: */
#define G3DKMD_MSG_MASK     (0xFFFFFFF0)
/* start from 4th bit. */
#define G3DKMD_MSG_IOCTL       (1<<4)
#define G3DKMD_MSG_API         (1<<5)
#define G3DKMD_MSG_MMU         (1<<6)
#define G3DKMD_MSG_ENGINE      (1<<7)
#define G3DKMD_MSG_FDQ         (1<<8)
#define G3DKMD_MSG_FENCE       (1<<9)
#define G3DKMD_MSG_GCE         (1<<10)
#define G3DKMD_MSG_MEM         (1<<11)
#define G3DKMD_MSG_ION         (1<<12)
#define G3DKMD_MSG_ISR         (1<<13)
#define G3DKMD_MSG_MMCE        (1<<14)
#define G3DKMD_MSG_MMCQ        (1<<15)
#define G3DKMD_MSG_POWER       (1<<16)
#define G3DKMD_MSG_SCHDLR      (1<<16)
#define G3DKMD_MSG_SIG         (1<<17)
#define G3DKMD_MSG_TASK        (1<<18)
#define G3DKMD_MSG_PATTERN     (1<<19)
#define G3DKMD_MSG_PROFILER    (1<<20)
#define G3DKMD_MSG_RECOVERY    (1<<21)
#define G3DKMD_MSG_PM          (1<<22)
#define G3DKMD_MSG_DEBUGGER    (1<<23)

#define G3DKMD_MSG_BUFFERS     (1<<24)
#define G3DKMD_MSG_FAKEMEM     (1<<25)

#define G3DKMD_MSG_HWDBG       (1<<27)
#define G3DKMD_MSG_FILEIO      (1<<28)
#define G3DKMD_MSG_BUFFERFILE  (1<<29)
#define G3DKMD_MSG_DDFR        (1<<30)

/* #define G3DKMD_MSG_GENERNAL    (1<<30) */
#define G3DKMD_MSG_ASERT       (1<<31)
#define G3DKMD_MSG_ALL         (G3DKMD_MSG_MASK)


/* map old msg_tags to LLVL levels */
/* #define G3DKMD_MSG_ERROR        G3DKMD_LLVL_ERROR   | G3DKMD_MSG_GENERNAL */
/* #define G3DKMD_MSG_WARNING      G3DKMD_LLVL_WARNING | G3DKMD_MSG_GENERNAL */
/* #define G3DKMD_MSG_NOTICE       G3DKMD_LLVL_NOTICE  | G3DKMD_MSG_GENERNAL */
/* #define G3DKMD_MSG_INFO         G3DKMD_LLVL_INFO    | G3DKMD_MSG_GENERNAL */
/* #define G3DKMD_MSG_COMMAND      G3DKMD_LLVL_INFO    | G3DKMD_MSG_GENERNAL   // map COMMAND to INFO level. */


#define G3DKMD_LLVL_DEFALUT     G3DKMD_LLVL_WARNING
#define G3DKMD_MSG_DEFALUT      (G3DKMD_MSG_ALL)


/* [MT8163 definition] */
#if defined(MT8163)
/* #undef  G3DKMD_SUPPORT_AUTO_RECOVERY */
/* #undef  G3DKMD_SCHEDULER_OS_CTX_SWTICH */
/* #define G3DKMD_POWER_MANAGEMENT */
#define G3DKMD_ENABLE_CG
#endif

#define G3DKMDDEBUG
#define G3DKMDDEBUG_ENG
#ifndef ANDROID_PREF_NDEBUG
/* #define G3DKMD_MET_CTX_SWITCH */
#endif
/* #define PRINT2FILE */

#ifdef linux
/* #define G3DKMD_MEM_CHECK // check linux kernel mode memory leakage status */
#if defined(G3DKMD_MEM_CHECK) && !defined(PRINT2FILE)
#define PRINT2FILE
#endif
#endif

#endif /* _G3DKMD_DEFINE_H_ */
