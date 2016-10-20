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


/* #include <linux/types.h> */ /* for NULL */
/* #include <linux/sched.h> */ /* for current */
/* #include <linux/delay.h> */ /* for mdelay */
#include <linux/interrupt.h> /* fro macro DECLARE_TASKLET */
/* #include <linux/workqueue.h> */

#include "g3dbase_common_define.h"
#include "g3dkmd_define.h"

#include "tester.h"

#include "tester_non_polling_qload.h"
#include "g3dkmd_api.h" /* lock/unlock_g3d() */
#include "g3dkmd_file_io.h" /* KMDDPF family */
#include "g3dkmd_macro.h" /* VAL_IRQ_* */
#include "g3dkmd_engine.h" /* g3dkmdWriteReg(...)*/
#include "g3dkmd_isr.h" /* g3dKmdInvokeIsrTop() */



#if defined(ENABLE_TESTER) && defined(ENABLE_TESTER_NON_POLLING_QLOAD)


/* Note:
 * This tester will test QLoad path, so require to disable GCE.
 */


/*unsigned int g3dKmdRegRead(unsigned int reg, unsigned int mask, unsigned int sft);*/
/*void g3dKmdRegWrite(unsigned int reg, unsigned int value, unsigned int mask, unsigned int sft);*/

/* testcase function prototypes: */
static void tester_prepare_qload_wait_on_queue(void *);
static void tester_restore_qload_wait_on_queue(void *);
static int tester_custom_qload(void *pdata, void *cdata);
static void report(void *pdata);


/* qload timeout/signal */

/* qload test case variables: */

DECLARE_TEST_CASE(tcase_qload_signal, "qload_test", "QLOAD_SIGNAL",	/* var, group_name, test_name */
		  tester_prepare_qload_wait_on_queue,	/* setup func    */
		  tester_restore_qload_wait_on_queue,	/* teardown func */
		  NULL, tester_custom_qload, report, -1);	/* setpass func, report func, ForeverTest */

DECLARE_TEST_CASE(tcase_qload_timeout, "qload_test", "QLOAD_TIMEOUT",
		  tester_prepare_qload_wait_on_queue,
		  tester_restore_qload_wait_on_queue, NULL, tester_custom_qload, report, -1);


struct myData {
	long interruptCnt;
	long timeoutCnt;
	long normalCnt;
	long unknownCnt;
};

struct myData qSignal = { 0 }, qTimeout = {
0};



void tester_reg_qload_tcases(void)
{
	tester_register_testcase(tcase_qload_signal);
	tcase_qload_signal.private_data = &qSignal;

	tester_register_testcase(tcase_qload_timeout);
	tcase_qload_timeout.private_data = &qTimeout;
}


/* qload test timeout/signal func */

static void qload_wq_tasklet_callback(unsigned long var)
{
	/* pr_debug("[%s] pid %d\n", __FUNCTION__, current->pid); */
	tester_restore_qload_wait_on_queue(NULL);

	/* set mmce_Frame_flush flag */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_MASKED, VAL_IRQ_G3D_MMCE_FRAME_FLUSH,
		       MSK_AREG_IRQ_MASKED, SFT_AREG_IRQ_MASK);

	g3dKmdInvokeIsrTop();
}

static DECLARE_TASKLET(tester_qload_wq_signal_tasklet, qload_wq_tasklet_callback, (unsigned long)0);

static int regFlag;

static void tester_prepare_qload_wait_on_queue(void *pdata)
{
	struct tester_case_t *ptcase = container_of(pdata, struct tester_case_t, private_data);
/* pr_debug("test_name: %s\n", ptcase->name); */



	/* save reg value. */
	regFlag = g3dKmdRegRead(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, VAL_IRQ_G3D_MMCE_FRAME_FLUSH, 0);

	/* set reg let the process waitig. */
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, 0, VAL_IRQ_G3D_MMCE_FRAME_FLUSH, 0);

	/* if testing signal, do some other works. */
	if (0 == strncmp(ptcase->name, "QLOAD_SIGNAL", MAX_TESTER_CASE_NAME))
		tasklet_schedule(&tester_qload_wq_signal_tasklet);
}

static void tester_restore_qload_wait_on_queue(void *pdata)
{
	g3dKmdRegWrite(G3DKMD_REG_G3D, REG_AREG_IRQ_STATUS_RAW, regFlag, VAL_IRQ_G3D_MMCE_FRAME_FLUSH, 0);
}

static int tester_custom_qload(void *pdata, void *cdata)
{
	struct tester_case_t *ptcase = container_of(pdata, struct tester_case_t, private_data);
	struct myData *mypdata = (struct myData *)ptcase->private_data;

	if (0 == strncmp((const char *)cdata, "FailInterrupt", MAX_TESTER_CASE_NAME))
		mypdata->interruptCnt++;
	else if (0 == strncmp((const char *)cdata, "FailTimeout", MAX_TESTER_CASE_NAME))
		mypdata->timeoutCnt++;
	else if (0 == strncmp((const char *)cdata, "FailNormal", MAX_TESTER_CASE_NAME))
		mypdata->normalCnt++;
	else
		mypdata->unknownCnt++;


	return 0;
}

static void report(void *pdata)
{
	struct tester_case_t *ptcase = container_of(pdata, struct tester_case_t, private_data);
	struct myData *mypdata = (struct myData *)ptcase->private_data;

	pr_debug
	    ("Testcase Name: %-20s  (%ld/%ld), unexpect: intrpt: %ld, timout: %ld, nrml: %ld unknwn: %ld\n",
	     ptcase->name, ptcase->passCnt, ptcase->testCnt, mypdata->interruptCnt,
	     mypdata->timeoutCnt, mypdata->normalCnt, mypdata->unknownCnt);
}

#endif /* defined(ENABLE_TESTER) && defined(ENABLE_TESTER_NON_POLLING_QLOAD) */
