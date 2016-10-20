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

#ifndef _TESTER_NON_POLLING_FLUSHCMD_H_
#define _TESTER_NON_POLLING_FLUSHCMD_H_

#include "g3dbase_common_define.h"	/* for define G3DKMD_USE_BUFFERFILE */
#include "g3dkmd_define.h"

/*
 * Testing:
 *        1. Enable define: TEST_NON_POLLING_FLUSHCMD,
 *        2. Run APP. dmesg will show mesg
 *             Wait until "(G3D_NOTICE) taskID: %d Not enough".
 *        3. Run another App instance. g3dKmdIsrTrigger() will invoked.
 *           dmesg will show:
 *                       "[IsrTop] wake up s_flushCmdWq"
 *                       "NON-POLLING: released(taskID:%d)"
 *           if the above message appears. it means non-polling solution works.
 *             a. processes will sleep if no Ht slots.
 *             b. processes will wake up when HW single coming(IsrTop)
 */


#if defined(TEST_NON_POLLING_FLUSHCMD) && !defined(ENABLE_NON_POLLING_FLUSHCMD)
#error "ENABLE_NON_POLLING_FLUSHCMD" needs to be enable when enable TEST_NON_POLLING_FLUSHCMD.
#endif


/* make sure the test only apply on linux_ko mode. */
#if !(defined(TEST_NON_POLLING_FLUSHCMD) && (defined(linux) && !defined(linux_user_mode)))

#define tester_wake_up()
#define tester_wq_release(arg)
#define tester_wq_signaled(arg)
#define tester_wq_timeout(arg)

#define tester_isHtNotFull(ret, arg) (ret)
#define tester_fence_block() 1

#else

#include <asm/atomic.h> /* for atomic operations */

#define TRIGGER_WHEN_WAITQUEUE_COUNT 10

struct tester_t {
	int is_fence_opened;
	void *instance;
	atomic_t cmdCnt; /* command count */
	atomic_t wqCnt; /* wait queue count */
	int triggerWhnWqCnt;
	void (*triggerFunc)(struct tester_t *obj, const int);
};


struct tester_t *tester_getInstance(void);
void _wake_up_callback(const char *funcName);
int _tester_isHtNotFull(struct tester_t *obj, const int isFull, const int taskID);
void _tester_wq_release(const int taskID);
void _tester_wq_signaled(const int taskID);
void _tester_wq_timeout(const int taskID);

int _tester_fence_block_door(struct tester_t *obj);

#define tester_wake_up()          _wake_up_callback(__func__)
#define tester_wq_release(arg)    _tester_wq_release(arg)
#define tester_wq_signaled(arg)   _tester_wq_signaled(arg)
#define tester_wq_timeout(arg)    _tester_wq_timeout(arg)
#define tester_isHtNotFull(ret, arg) _tester_isHtNotFull(tester_getInstance(), ret, arg)


#define tester_fence_block() _tester_fence_block_door(tester_getInstance())
/* if (((struct tester_t*)tester_getInstance())->is_fence_opened) */

#endif

#endif /* End of file */
