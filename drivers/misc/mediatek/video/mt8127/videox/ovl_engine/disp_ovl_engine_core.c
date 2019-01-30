/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/time.h>
#include <linux/version.h>
#include <linux/sched.h>
/* #include <linux/ion.h> */
#include "disp_ovl_engine_core.h"
#include "disp_ovl_engine_sw.h"
#include "disp_ovl_engine_hw.h"
#include "disp_debug.h"

/* Ovl_engine parameter */
DISP_OVL_ENGINE_PARAMS disp_ovl_engine;
DEFINE_SEMAPHORE(disp_ovl_engine_semaphore);
int disp_ovl_engine_log_level = 0;

/* Ovl_engine thread */
static struct task_struct *disp_ovl_engine_task;
static int disp_ovl_engine_kthread(void *data);
static wait_queue_head_t disp_ovl_engine_wq;
static atomic_t gWakeupOvlEngineThread = ATOMIC_INIT(0);

/* rdma0 update thread */
static struct task_struct *disp_rdma0_update_task;
static int disp_ovl_engine_rdma0_update_kthread(void *data);
static wait_queue_head_t disp_rdma0_update_wq;
static atomic_t gWakeupRdma0UpdateThread = ATOMIC_INIT(0);
DEFINE_SEMAPHORE(disp_rdma0_update_semaphore);

/*  */
static int curr_instance_id;

/* Complete notification */
wait_queue_head_t disp_ovl_complete_wq;
unsigned long gStartTriggerTime = 0;

#ifdef DISP_OVL_ENGINE_REQUEST
struct disp_ovl_engine_request_struct gOverlayRequest;
wait_queue_head_t disp_ovl_get_request_wq;
static unsigned int gWakeupGetRequest;
wait_queue_head_t disp_ovl_ack_request_wq;
static unsigned int gWakeupAckRequest;
#endif

/* static size_t ovl_eng_log_on = false; */

static unsigned long disp_ovl_engine_get_current_time_us(void);

void set_ovlengine_debug_level(int level)
{
	disp_ovl_engine_log_level = level;
}

void disp_ovl_engine_init(void)
{
	unsigned int i = 0;
	static int init;

	DISP_OVL_ENGINE_INFO("disp_ovl_engine_init\n");

	if (!init) {
		init = 1;
		/* Init ovl_engine parameter */
		disp_ovl_engine.bInit = true;

		memset(disp_ovl_engine.Instance, 0,
		       sizeof(DISP_OVL_ENGINE_INSTANCE) * OVL_ENGINE_INSTANCE_MAX_INDEX);
		memset(disp_ovl_engine.OvlBufAddr, 0,
		       sizeof(unsigned int) * OVL_ENGINE_OVL_BUFFER_NUMBER);
		disp_ovl_engine.current_instance = NULL;

		disp_ovl_engine.bCouple = true;
		disp_ovl_engine.captured_layer_config = disp_ovl_engine.layer_config[0];
		disp_ovl_engine.realtime_layer_config = disp_ovl_engine.layer_config[0];

		while (i < OVL_ENGINE_INSTANCE_MAX_INDEX) {
			disp_ovl_engine.Instance[i].index = i;
			atomic_set(&disp_ovl_engine.Instance[i].fgCompleted, 0);
			atomic_set(&disp_ovl_engine.Instance[i].OverlaySettingDirtyFlag, 0);
			atomic_set(&disp_ovl_engine.Instance[i].OverlaySettingApplied, 0);
			i++;
		}
		i = 0;
		while (i < DDP_OVL_LAYER_MUN) {
			disp_ovl_engine.layer_config[0][i].layer = i;
			disp_ovl_engine.layer_config[1][i].layer = i;
			i++;
		}

#ifdef DISP_OVL_ENGINE_SW_SUPPORT
		disp_ovl_engine_sw_init();
		disp_ovl_engine_sw_register_irq(disp_ovl_engine_interrupt_handler);
#endif

#ifdef DISP_OVL_ENGINE_HW_SUPPORT
		disp_ovl_engine_hw_init();
		disp_ovl_engine_hw_register_irq(disp_ovl_engine_interrupt_handler);
#endif

#ifdef CONFIG_MTK_ION
		/* Init ion */
		disp_ovl_engine.ion_client = ion_client_create(g_ion_device, "overlay");
		if (IS_ERR(disp_ovl_engine.ion_client))
			DISP_OVL_ENGINE_ERR("failed to create ion client\n");
#endif

		/* Init ovl_engine complete notification */
		init_waitqueue_head(&disp_ovl_complete_wq);

		/* Init ovl_engine thread */
		init_waitqueue_head(&disp_ovl_engine_wq);
		disp_ovl_engine_task =
		    kthread_create(disp_ovl_engine_kthread, NULL, "ovl_engine_kthread");
		wake_up_process(disp_ovl_engine_task);
		DISP_OVL_ENGINE_INFO("kthread_create ovl_engine_kthread\n");

		/* Init rdma0 update thread */
		init_waitqueue_head(&disp_rdma0_update_wq);
		disp_rdma0_update_task =
		    kthread_create(disp_ovl_engine_rdma0_update_kthread, NULL,
				   "rdma0_update_kthread");
		wake_up_process(disp_rdma0_update_task);
		DISP_OVL_ENGINE_INFO("kthread_create rdma0_update_kthread\n");

#ifdef DISP_OVL_ENGINE_REQUEST
		/* Init ovl_engine request */
		init_waitqueue_head(&disp_ovl_get_request_wq);
		init_waitqueue_head(&disp_ovl_ack_request_wq);
#endif

	}
}

void disp_ovl_engine_free_ion(int instance)
{
#ifdef CONFIG_MTK_ION
	int j;
	DISP_OVL_ENGINE_INSTANCE *temp = &disp_ovl_engine.Instance[instance];

	if (DECOUPLE_MODE == temp->mode) {
		for (j = 0; j < HW_OVERLAY_COUNT; j++) {
			if (temp->cached_layer_config[j].fgIonHandleImport
				&& temp->cached_layer_config[j].ion_handles) {
				ion_free(disp_ovl_engine.ion_client,
						temp->cached_layer_config[j].ion_handles);
				temp->cached_layer_config[j].fgIonHandleImport = false;
				temp->cached_layer_config[j].ion_handles = NULL;
			}
		}
	}
#endif
}

/*
1. Instance status manager
2. signal fence and free ION handle
3. wake up OVL complete WQ
*/
static int disp_ovl_engine_kthread(void *data)
{
	int i, wait_ret = 0;
	struct sched_param param = {.sched_priority = 94 };
	DISP_OVL_ENGINE_INSTANCE *temp = &disp_ovl_engine.Instance[curr_instance_id];

	sched_setscheduler(current, SCHED_RR, &param);

	while (1) {
		wait_ret = wait_event_interruptible(disp_ovl_engine_wq,
					     atomic_read(&gWakeupOvlEngineThread));
		atomic_set(&gWakeupOvlEngineThread, 0);

		DISP_OVL_ENGINE_DBG("disp_ovl_engine_kthread wake_up_interruptible\n");

		if (down_interruptible(&disp_ovl_engine_semaphore)) {
			DISP_OVL_ENGINE_ERR
			    ("disp_ovl_engine_kthread down_interruptible(disp_ovl_engine_semaphore) fail\n");
			continue;
		}

		DISP_OVL_ENGINE_DBG("disp_ovl_engine_kthread instance %d status %d\n",
				    curr_instance_id, temp->status);

		/* Overlay complete notification */
		for (i = 0; i < OVL_ENGINE_INSTANCE_MAX_INDEX; i++) {
			if (disp_ovl_engine.Instance[i].status == OVERLAY_STATUS_COMPLETE) {
				disp_ovl_engine_dump_ovl(i);
				disp_ovl_engine_free_ion(i);
				MMProfileLogEx(MTKFB_MMP_Events.Instance_status[i],
					       MMProfileFlagEnd, curr_instance_id,
					       OVERLAY_STATUS_COMPLETE);
				disp_ovl_engine.Instance[i].status = OVERLAY_STATUS_IDLE;
				atomic_set(&disp_ovl_engine.Instance[i].fgCompleted, 1);
				wake_up_all(&disp_ovl_complete_wq);
				DISP_OVL_ENGINE_INFO
				    ("disp_ovl_engine_kthread overlay complete %d\n", i);
			}
		}

		if (temp->status == OVERLAY_STATUS_BUSY) {
			unsigned long currentTime = disp_ovl_engine_get_current_time_us();

			if ((currentTime - gStartTriggerTime) > 1000) {
				disp_ovl_engine_free_ion(curr_instance_id);
				temp->status = OVERLAY_STATUS_IDLE;
				DISP_OVL_ENGINE_ERR
				    ("disp_ovl_engine.Instance[%d] busy too long, reset to idle\n",
				     curr_instance_id);
			} else {
				up(&disp_ovl_engine_semaphore);
				MMProfileLogEx(MTKFB_MMP_Events.Instance_status[curr_instance_id],
					       MMProfileFlagPulse, curr_instance_id,
					       OVERLAY_STATUS_BUSY);
				continue;
			}
		}

		{
			int i;

			/* DISP_OVL_ENGINE_DBG("disp_ovl_engine_kthread find next request\n"); */

			/* Find next overlay request */
			for (i = 0; i < OVL_ENGINE_INSTANCE_MAX_INDEX; i++) {
				curr_instance_id++;
				curr_instance_id %= OVL_ENGINE_INSTANCE_MAX_INDEX;
				temp = &disp_ovl_engine.Instance[curr_instance_id];

				/* DISP_OVL_ENGINE_DBG("disp_ovl_engine_kthread instance %d status %d\n", */
				/* curr_instance_id,temp->status); */

				if (temp->status == OVERLAY_STATUS_TRIGGER) {
					MMProfileLogEx(MTKFB_MMP_Events.Instance_status
						       [curr_instance_id], MMProfileFlagStart,
						       curr_instance_id, OVERLAY_STATUS_TRIGGER);
					if (temp->MemOutConfig.dirty)
						atomic_set(&temp->OverlaySettingDirtyFlag, 1);

					DISP_OVL_ENGINE_INFO
					    ("disp_ovl_engine instance %d(status %d)will be service\n",
					     curr_instance_id, temp->status);
					temp->status = OVERLAY_STATUS_BUSY;

					gStartTriggerTime = disp_ovl_engine_get_current_time_us();
					/* Trigger overlay */
#ifdef DISP_OVL_ENGINE_SW_SUPPORT
					disp_ovl_engine_sw_set_params(temp);
					disp_ovl_engine_trigger_sw_overlay();
#endif

#ifdef DISP_OVL_ENGINE_HW_SUPPORT
					disp_ovl_engine_hw_set_params(temp);
					disp_ovl_engine_trigger_hw_overlay();
#endif

					break;
				}
			}
		}

		up(&disp_ovl_engine_semaphore);

		if (kthread_should_stop())
			break;
	}

	return OVL_OK;
}

void disp_ovl_engine_wake_up_ovl_engine_thread(void)
{
	MMProfileLogEx(MTKFB_MMP_Events.wake_up_ovl_engine_thread, MMProfileFlagPulse, 0, 0);

	atomic_set(&gWakeupOvlEngineThread, 1);
	wake_up_interruptible(&disp_ovl_engine_wq);
}

void disp_ovl_engine_interrupt_handler(unsigned int param)
{
	DISP_OVL_ENGINE_INFO("disp_ovl_engine_interrupt_handler %d\n", curr_instance_id);
	MMProfileLogEx(MTKFB_MMP_Events.interrupt_handler, MMProfileFlagPulse, param, 0);

	if (OVERLAY_STATUS_BUSY == disp_ovl_engine.Instance[curr_instance_id].status) {
		disp_ovl_engine.Instance[curr_instance_id].status = OVERLAY_STATUS_COMPLETE;
		MMProfileLogEx(MTKFB_MMP_Events.OVLEngine_ovlwdma_status, MMProfileFlagEnd,
			       disp_ovl_engine.OvlWrIdx,
			       disp_ovl_engine.OvlBufAddr[disp_ovl_engine.OvlWrIdx]);

		disp_ovl_engine.RdmaRdIdx = disp_ovl_engine.OvlWrIdx;
		MMProfileLogEx(MTKFB_MMP_Events.OVLEngine_rdma_status, MMProfileFlagPulse,
			       disp_ovl_engine.RdmaRdIdx,
			       disp_ovl_engine.OvlBufAddr[disp_ovl_engine.RdmaRdIdx]);
		DISP_OVL_ENGINE_INFO("wdma frame done\n");
	} else {
		int i = 0;

		for (i = 0; i < OVL_ENGINE_INSTANCE_MAX_INDEX; i++)
			if (OVERLAY_STATUS_BUSY == disp_ovl_engine.Instance[i].status)
				disp_ovl_engine.Instance[i].status = OVERLAY_STATUS_COMPLETE;
	}

	disp_ovl_engine_wake_up_ovl_engine_thread();
}

#ifdef DISP_OVL_ENGINE_REQUEST
int Disp_Ovl_Engine_Set_Request(struct disp_ovl_engine_request_struct *overlayRequest, int timeout)
{
	int wait_ret;

	gOverlayRequest.request = overlayRequest->request;
	gOverlayRequest.value = overlayRequest->value;
	gOverlayRequest.ret = -1;

	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_Set_Request request %d, value %d\n",
			     gOverlayRequest.request, gOverlayRequest.value);

	gWakeupGetRequest = 1;
	wake_up_interruptible(&disp_ovl_get_request_wq);

	wait_ret =
	    wait_event_interruptible_timeout(disp_ovl_ack_request_wq, gWakeupAckRequest, timeout);
	gWakeupAckRequest = 0;

	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_Set_Request ret %d\n", gOverlayRequest.ret);

	overlayRequest->ret = gOverlayRequest.ret;

	return OVL_OK;
}

int Disp_Ovl_Engine_Get_Request(struct disp_ovl_engine_request_struct *overlayRequest)
{
	int wait_ret;

	wait_ret = wait_event_interruptible(disp_ovl_get_request_wq, gWakeupGetRequest);
	gWakeupGetRequest = 0;

	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_Get_Request request %d, value %d\n",
			     gOverlayRequest.request, gOverlayRequest.value);

	overlayRequest->request = gOverlayRequest.request;
	overlayRequest->value = gOverlayRequest.value;

	return OVL_OK;
}

int Disp_Ovl_Engine_Ack_Request(struct disp_ovl_engine_request_struct *overlayRequest)
{
	gOverlayRequest.ret = overlayRequest->ret;

	DISP_OVL_ENGINE_INFO("Disp_Ovl_Engine_Ack_Request ret %d\n", gOverlayRequest.ret);

	gWakeupAckRequest = 1;
	wake_up_interruptible(&disp_ovl_ack_request_wq);

	return OVL_OK;
}
#endif


unsigned long disp_ovl_engine_get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return t.tv_sec * 1000 + t.tv_usec / 1000;
}

void disp_ovl_engine_wake_up_rdma0_update_thread(void)
{
	atomic_set(&gWakeupRdma0UpdateThread, 1);
	wake_up_interruptible(&disp_rdma0_update_wq);
}

static int disp_ovl_engine_rdma0_update_kthread(void *data)
{
	int wait_ret = 0;
	struct sched_param param = {.sched_priority = 94 };

	sched_setscheduler(current, SCHED_RR, &param);

	while (1) {
		wait_ret =
		    wait_event_interruptible(disp_rdma0_update_wq,
					     atomic_read(&gWakeupRdma0UpdateThread));
		atomic_set(&gWakeupRdma0UpdateThread, 0);

		if (down_interruptible(&disp_rdma0_update_semaphore)) {
			DISP_OVL_ENGINE_ERR
			    ("disp_ovl_engine_rdma0_update_kthread down_interruptible fail\n");
			continue;
		}

		disp_ovl_engine_update_rdma0();

		up(&disp_rdma0_update_semaphore);

		if (kthread_should_stop())
			break;
	}

	return OVL_OK;
}
