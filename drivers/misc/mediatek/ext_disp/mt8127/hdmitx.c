/*
* Copyright (C) 2016 MediaTek Inc.
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

#if defined(CONFIG_MTK_HDMI_SUPPORT)
#define TMFL_TDA19989

#define _tx_c_

#include "hdmitx_drv.h"
#include "hdmi_utils.h"
#define MTK_HDMI_MULTI_DISPLAY_SUPPORT
#define MTK_HDMI_FENCE_SUPPORT
#define MTK_HDMI_ION_SUPPORT

#define HDMI_DEVNAME "hdmitx"

#undef OUTREG32
#define OUTREG32(x, y) {/*pr_err("[hdmi]write 0x%08x to 0x%08x\n", (y), (x)); */__OUTREG32((x), (y))}
#define __OUTREG32(x, y) {*(unsigned int *)(x) = (y); }

#define HDMI_DPI(suffix)        DPI1  ## suffix
#define HMID_DEST_DPI           DISP_MODULE_DPI1

#ifdef MTK_OVERLAY_ENGINE_SUPPORT
static int hdmi_bpp = 3;
#else
static int hdmi_bpp = 4;
#endif

unsigned int addbuf = 0, delbuf = 0, rdmabuf = 0;

#ifdef MTK_HDMI_FENCE_SUPPORT
/* Fence Sync Object */
#include "mtk_sync.h"
spinlock_t hdmi_lock;
DEFINE_SPINLOCK(hdmi_lock);
#endif

static bool factory_mode;

#define ALIGN_TO(x, n) (((x) + ((n) - 1)) & ~((n) - 1))
#define hdmi_abs(a) (((a) < 0) ? -(a) : (a))

static int hdmi_log_on;
static int hdmi_bufferdump_on = 1;
static unsigned long hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;

#if defined(MTK_HDMI_ION_SUPPORT)
#include <ion_drv.h>

static struct ion_client *ion_client;
#endif

static struct switch_dev hdmi_switch_data;
static struct switch_dev hdmires_switch_data;
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)
static struct switch_dev hdmi_cec_switch_data;
#endif

struct HDMI_PARAMS _s_hdmi_params = { 0 };

struct HDMI_PARAMS *hdmi_params = &_s_hdmi_params;
static struct HDMI_DRIVER *hdmi_drv;

#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
static size_t hdmi_colorspace = HDMI_RGB;

int flag_resolution_interlace(HDMI_VIDEO_RESOLUTION resolution)
{
	if ((resolution == HDMI_VIDEO_1920x1080i_60Hz) ||
	    (resolution == HDMI_VIDEO_1920x1080i_50Hz) ||
#if defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	    (resolution == HDMI_VIDEO_1920x1080i3d_sbs_60Hz) ||
	    (resolution == HDMI_VIDEO_1920x1080i3d_sbs_50Hz) ||
#endif
	    (resolution == HDMI_VIDEO_1920x1080i3d_60Hz) ||
	    (resolution == HDMI_VIDEO_1920x1080i3d_60Hz))
		return true;
	else
		return false;
}

int flag_resolution_3d(HDMI_VIDEO_RESOLUTION resolution)
{
	if ((resolution == HDMI_VIDEO_1280x720p3d_60Hz) ||
	    (resolution == HDMI_VIDEO_1280x720p3d_50Hz) ||
	    (resolution == HDMI_VIDEO_1920x1080i3d_60Hz) ||
	    (resolution == HDMI_VIDEO_1920x1080i3d_60Hz) ||
	    (resolution == HDMI_VIDEO_1920x1080p3d_24Hz) ||
	    (resolution == HDMI_VIDEO_1920x1080p3d_23Hz))
		return true;
	else
		return false;
}
#endif

void hdmi_log_enable(int enable)
{
	pr_info("hdmi log %s\n", enable ? "enabled" : "disabled");
	hdmi_log_on = enable;
	hdmi_drv->log_enable(enable);
}

void hdmi_mmp_enable(int enable)
{
	pr_info("hdmi log %s\n", enable ? "enabled" : "disabled");
	hdmi_bufferdump_on = enable;
}

static DEFINE_SEMAPHORE(hdmi_update_mutex);
static DEFINE_SEMAPHORE(hdmi_ioctl_mutex);

typedef struct {
	bool is_reconfig_needed;	/* whether need to reset HDMI memory */
	bool is_enabled;	/* whether HDMI is enabled or disabled by user */
	bool is_force_disable;	/* used for camera scenario. */
	bool is_clock_on;	/* DPI is running or not */
	atomic_t state;		/* HDMI_POWER_STATE state */
	int lcm_width;		/* LCD write buffer width */
	int lcm_height;		/* LCD write buffer height */
	bool lcm_is_video_mode;
	int hdmi_width;		/* DPI read buffer width */
	int hdmi_height;	/* DPI read buffer height */
	int bg_width;		/* DPI read buffer width */
	int bg_height;		/* DPI read buffer height */
	HDMI_VIDEO_RESOLUTION output_video_resolution;
	enum HDMI_AUDIO_FORMAT output_audio_format;
	int orientation;
	/* MDP's orientation, 0 means 0 degree, 1 means 90 degree, 2 means 180 degree, 3 means 270 degree */
	enum HDMI_OUTPUT_MODE output_mode;
	int scaling_factor;
} _t_hdmi_context;

typedef enum {
	create_new,
	insert_new,
	reg_configed,
	reg_updated,
	buf_read_done,
	buf_dropped
} BUFFER_STATE;

typedef struct _hdmi_buffer_list {
	hdmi_video_buffer_info buffer_info;
	BUFFER_STATE buf_state;

	unsigned int idx;	/* /fence count */
	int fence;		/* /fence fd */
	struct ion_handle *hnd;
	unsigned int mva;
	unsigned int va;
	struct list_head list;
} hdmi_video_buffer_list;

static struct list_head hdmi_video_mode_buffer_list;
/* static struct list_head *hdmi_video_buffer_list_head = &hdmi_video_mode_buffer_list; */
DEFINE_SEMAPHORE(hdmi_video_mode_mutex);
/* static atomic_t hdmi_video_mode_flag = ATOMIC_INIT(0); */
/* static int hdmi_add_video_buffer
(struct hdmi_video_buffer_info *buffer_info, struct file *file); */
/* static struct hdmi_video_buffer_list *hdmi_search_video_buffer
(struct hdmi_video_buffer_info *buffer_info, struct file *file); */
/* static void hdmi_destroy_video_buffer(void); */
#define IS_HDMI_IN_VIDEO_MODE()        atomic_read(&hdmi_video_mode_flag)
#define SET_HDMI_TO_VIDEO_MODE()       atomic_set(&hdmi_video_mode_flag, 1)
#define SET_HDMI_LEAVE_VIDEO_MODE()    atomic_set(&hdmi_video_mode_flag, 0)
static wait_queue_head_t hdmi_video_mode_wq;
#define IS_HDMI_VIDEO_MODE_DPI_IN_CHANGING_ADDRESS()    atomic_read(&hdmi_video_mode_dpi_change_address)
#define SET_HDMI_VIDEO_MODE_DPI_CHANGE_ADDRESS()        atomic_set(&hdmi_video_mode_dpi_change_address, 1)
#define SET_HDMI_VIDEO_MODE_DPI_CHANGE_ADDRESS_DONE()   atomic_set(&hdmi_video_mode_dpi_change_address, 0)


static _t_hdmi_context hdmi_context;
static _t_hdmi_context *p = &hdmi_context;
static struct list_head HDMI_Buffer_List;

#define IS_HDMI_ON()            (HDMI_POWER_STATE_ON == atomic_read(&p->state))
#define IS_HDMI_OFF()           (HDMI_POWER_STATE_OFF == atomic_read(&p->state))
#define IS_HDMI_STANDBY()       (HDMI_POWER_STATE_STANDBY == atomic_read(&p->state))

#define IS_HDMI_NOT_ON()        (HDMI_POWER_STATE_ON != atomic_read(&p->state))
#define IS_HDMI_NOT_OFF()       (HDMI_POWER_STATE_OFF != atomic_read(&p->state))
#define IS_HDMI_NOT_STANDBY()   (HDMI_POWER_STATE_STANDBY != atomic_read(&p->state))

#define SET_HDMI_ON()           atomic_set(&p->state, HDMI_POWER_STATE_ON)
#define SET_HDMI_OFF()          atomic_set(&p->state, HDMI_POWER_STATE_OFF)
#define SET_HDMI_STANDBY()      atomic_set(&p->state, HDMI_POWER_STATE_STANDBY)

#ifdef MTK_HDMI_SCREEN_CAPTURE
bool capture_screen = false;
unsigned long capture_addr;
#endif
static int dp_mutex_dst = -1;
static dev_t hdmi_devno;
static struct cdev *hdmi_cdev;
static struct class *hdmi_class;
static unsigned int rdma1_addr_shadowed;
static unsigned int rdma1_addr_using;

struct HDMI_MMP_Events_t HDMI_MMP_Events;

typedef enum {
	insert_Buffer_Err1 = 0xeff0,
	insert_Buffer_Err2,
	insert_Buffer_Err3,
	insert_Buffer_Err4,
	insert_Buffer_Err5,
	Buffer_INFO_Err,	/* /5 */
	Timeline_Err,
	Buffer_Not_Enough,	/* /7 */
	Buffer_Empt_Err,
	Fence_Err,		/* /9 */
	Mutex_Err1,
	Mutex_Err2,
	Mutex_Err3,
	Buff_Dup_Err1,		/* /0xeffd */
	Buff_ION_Err1
} HDMI_MMP_Err;

typedef enum {
	Plugout,
	Plugin,
	ResChange
} HDMI_State;

/* --------------------------------------------------------------------------- */
/* Information Dump Routines */
/* --------------------------------------------------------------------------- */

void init_hdmi_mmp_events(void)
{
	/*return;*/
	HDMI_FUNC();
	if (HDMI_MMP_Events.HDMI == 0) {
		HDMI_MMP_Events.HDMI = MMProfileRegisterEvent(MMP_RootEvent, "HDMI");
		HDMI_MMP_Events.OverlayDone =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "OverlayDone");
		HDMI_MMP_Events.DDPKBitblt =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "DDPKBitblt");
		HDMI_MMP_Events.SwitchRDMABuffer =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "SwitchRDMABuffer");
		HDMI_MMP_Events.SwitchOverlayBuffer =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "SwitchOverlayBuffer");
		HDMI_MMP_Events.WDMA1RegisterUpdated =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "WDMA1RegisterUpdated");
		HDMI_MMP_Events.WaitVSync =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "WaitVSync");
		HDMI_MMP_Events.StopOverlayBuffer =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "StopOverlayBuffer");
		HDMI_MMP_Events.RDMA1RegisterUpdated =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "RDMA1RegisterUpdated");

		HDMI_MMP_Events.FenceCreate =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "FenceCreate");
		HDMI_MMP_Events.BufferPost =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "BufferPost");
		HDMI_MMP_Events.BufferInsert =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "BufferInsert");
		HDMI_MMP_Events.BufferCfg =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "BufferCfg");
		HDMI_MMP_Events.BufferUsed =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "BufferUsed");
		HDMI_MMP_Events.BufferUpdate =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "BufferUpdate");
		HDMI_MMP_Events.BufferRemove =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "BufferRemove");
		HDMI_MMP_Events.FenceSignal =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "FenceSignal");
		HDMI_MMP_Events.HDMIState =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "HDMIState");
		HDMI_MMP_Events.GetDevInfo =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "GetDevInfo");
		HDMI_MMP_Events.ErrorInfo =
		    MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "ErrorInfo");
		HDMI_MMP_Events.MutexErr = MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "MutexErr");

		MMProfileEnableEventRecursive(HDMI_MMP_Events.HDMI, 1);
	}
}

#define ENABLE_HDMI_FPS_CONTROL_LOG 1
#if ENABLE_HDMI_FPS_CONTROL_LOG
/* static unsigned int hdmi_fps_control_fps_wdma0 = 0; */
/* static unsigned long hdmi_fps_control_time_base_wdma0 = 0; */
/* static unsigned int hdmi_fps_control_fps_wdma1 = 0; */
/* static unsigned long hdmi_fps_control_time_base_wdma1 = 0; */
/* static unsigned int hdmi_fps_control_fps_rdma1 = 0; */
/* static unsigned long hdmi_fps_control_time_base_rdma1 = 0; */
#endif

typedef enum {
	HDMI_OVERLAY_STATUS_STOPPED,
	HDMI_OVERLAY_STATUS_STOPPING,
	HDMI_OVERLAY_STATUS_STARTING,
	HDMI_OVERLAY_STATUS_STARTED,
} HDMI_OVERLAY_STATUS;

static unsigned int hdmi_fps_control_dpi;
static unsigned int hdmi_fps_control_overlay;

static DPI_POLARITY clk_pol, de_pol, hsync_pol, vsync_pol;
static unsigned int dpi_clk_div, dpi_clk_duty, intermediat_buffer_num;
static unsigned int hsync_pulse_width, hsync_back_porch, hsync_front_porch;
static unsigned int vsync_pulse_width, vsync_back_porch, vsync_front_porch;

#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)

static bool fg3DFrame, fgInterlace;
static unsigned int hdmi_res = HDMI_VIDEO_1280x720p_50Hz;
#endif

static enum HDMI_COLOR_ORDER rgb_order;

static struct task_struct *hdmi_rdma_config_task;
static struct task_struct *hdmi_rdma_update_task;

static wait_queue_head_t hdmi_rdma_config_wq;
static atomic_t hdmi_rdma_config_event = ATOMIC_INIT(0);

static wait_queue_head_t hdmi_rdma_update_wq;
static atomic_t hdmi_rdma_update_event = ATOMIC_INIT(0);

static wait_queue_head_t reg_update_wq;
/* static atomic_t reg_update_event = ATOMIC_INIT(0); */

static wait_queue_head_t dst_reg_update_wq;
/* static atomic_t dst_reg_update_event = ATOMIC_INIT(0); */

static unsigned int hdmi_resolution_param_table[][3] = {
	{720, 480, 60},
	{1280, 720, 60},
	{1920, 1080, 30},
};

#define ENABLE_HDMI_BUFFER_LOG 1
#if ENABLE_HDMI_BUFFER_LOG
bool enable_hdmi_buffer_log = 0;
#define HDMI_BUFFER_LOG(fmt, arg...)do { if (enable_hdmi_buffer_log)pr_info("[hdmi_buffer]"fmt, ##arg);	} while (0)
#else
bool enable_hdmi_buffer_log = 0;
#define HDMI_BUFFER_LOG(fmt, arg...)
#endif

int hdmi_rdma_buffer_switch_mode = 0;	/* 0: switch in rdma1 frame done interrupt, 1: switch after DDPK_Bitblt done */
static bool otg_enable_status;

static DEFINE_SEMAPHORE(hdmi_buffer_mutex);

static int fill_buffer_rgb(void *addr, unsigned int size)
{
	int i;
	char *buf_addr;

	buf_addr = (char *)addr;
	/* vertical  strip */
	for (i = 0; i < size; i += 12) {
		buf_addr[i] = 0xff;
		buf_addr[i + 1] = 0x00;
		buf_addr[i + 2] = 0x00;

		buf_addr[i + 3] = 0x00;
		buf_addr[i + 4] = 0xff;
		buf_addr[i + 5] = 0x00;

		buf_addr[i + 6] = 0x00;
		buf_addr[i + 7] = 0x00;
		buf_addr[i + 8] = 0xff;

		buf_addr[i + 9] = 0xff;
		buf_addr[i + 10] = 0xff;
		buf_addr[i + 11] = 0xff;
	}
	return 0;
}

static int hdmi_alloc_buffer(M4U_PORT_ID_ENUM port, unsigned int size, unsigned int *pVa,
			     unsigned int *pMva)
{
	M4U_PORT_STRUCT m4uport;
	unsigned int mva, va;
	struct page *pg;

	if (size == 0)
		return -1;
	pr_err("hdmi_alloc_buffer size =%d\n", size);
	va = (unsigned int)vmalloc(size); /* kmalloc , GFP_KERNEL|GFP_ATOMIC); */
	if (((void *)va) == NULL) {
		pr_err("vmalloc %d bytes fail!!!\n", size);
		return -1;
	}

	memset((void *)va, 0, size);

	fill_buffer_rgb((void *)va, size);

	if (m4u_alloc_mva(port, va, size, 0, 0, &mva)) {
		pr_err("m4u_alloc_mva(va=0x%x) fail\n", va);
		vfree((void *)va);
		return -1;
	}

	memset((void *)&m4uport, 0, sizeof(M4U_PORT_STRUCT));
	m4uport.ePortID = port;
	m4uport.Virtuality = 1;
	m4uport.domain = 1;
	m4uport.Security = 0;
	m4uport.Distance = 1;
	m4uport.Direction = 0;
	m4u_config_port(&m4uport);

	*pMva = mva;
	*pVa = va;

	pg = vmalloc_to_page((const void *)va);
	pr_err("pa = %x\n", (unsigned int)page_to_pfn(pg) << PAGE_SHIFT);

	pr_err("[Pattern] alloc_buffer va=0x%08x, mva=0x%08x\n", va, mva);

	return 0;
}


#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
KREE_SESSION_HANDLE hdmi_pattern_secure_memory_session_handle(void)
{
	static KREE_SESSION_HANDLE hdmi_pattern_secure_memory_session;

	pr_info("hdmi_pattern_secure_memory_session_handle() acquire TEE session\n");
	if (NULL == hdmi_pattern_secure_memory_session) {
		TZ_RESULT ret;

		pr_info("hdmi_pattern_secure_memory_session_handle() create session\n");
		ret = KREE_CreateSession(TZ_TA_MEM_UUID, &hdmi_pattern_secure_memory_session);
		if (ret != TZ_RESULT_SUCCESS) {
			pr_err("KREE_CreateSession fail, ret=%d\n", ret);
			return NULL;
		}
	}

	pr_info("hdmi_pattern_secure_memory_session_handle() session=%x\n",
		(unsigned int)hdmi_pattern_secure_memory_session);
	return hdmi_pattern_secure_memory_session;
}

void *hdmi_pattern_allocate_secure_memory(int size)
{
	KREE_SECUREMEM_HANDLE mem_handle;
	TZ_RESULT ret;
	struct disp_mva_map mva_map_struct;
	MTEEC_PARAM param[4];
	unsigned int paramTypes;

	/* Allocate */
	ret =
	    KREE_AllocSecurechunkmemWithTag(hdmi_pattern_secure_memory_session_handle(), &mem_handle, 4096,
				     size, "extd_disp");
	if (ret != TZ_RESULT_SUCCESS) {
		pr_err("KREE_AllocSecurechunkmemWithTag fail, ret=%d\n", ret);
		return NULL;
	}

	pr_info("KREE_AllocSecurechunkmemWithTag handle=0x%x\n", mem_handle);

	param[0].value.a = DISP_RDMA1;
	param[1].value.a = 0;
	param[2].value.a = (unsigned int)mem_handle;
	param[3].value.a = size;
	paramTypes =
	    TZ_ParamTypes4(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
	ret =
	    KREE_TeeServiceCall(ddp_session_handle(), TZCMD_DDP_SECURE_MVA_MAP, paramTypes, param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_err("KREE_TeeServiceCall(TZCMD_DDP_SECURE_MVA_MAP) fail, ret=%d\n", ret);
		return -1;
	}

	return (void *)mem_handle;
}
#endif

static unsigned int hdmi_pattern_va;
static unsigned int hdmi_pattern_mva;
static unsigned int hdmi_pattern_mva_sec;
static unsigned int g_hdmi_pattern;

void hdmi_pattern(int enable)
{
	int ret;
	static unsigned int hdmi_pattern_buffer_init;
	unsigned int pattern_size = p->hdmi_width * p->hdmi_height * hdmi_bpp;

	if (hdmi_pattern_buffer_init == 0) {
		if (hdmi_pattern_mva == 0) {	/* normal mva */
			ret =
			    hdmi_alloc_buffer(DISP_RDMA1, pattern_size, &hdmi_pattern_va,
					      &hdmi_pattern_mva);
			if (ret == -1)
				return;
			/* printk("hdmi_pattern hdmi_alloc_buffer result is va=0x%x; mva=0x%x pattern_size=0x%x",
				hdmi_pattern_va, hdmi_pattern_mva, pattern_size); */
		}
#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
		if (hdmi_pattern_mva_sec == 0) {	/* handle */
			hdmi_pattern_mva_sec =
			    (unsigned int)hdmi_pattern_allocate_secure_memory(pattern_size);
		}
#endif

		hdmi_pattern_buffer_init = 1;
	}

	g_hdmi_pattern = enable;

	HDMI_LOG("hdmi_pattern %d\n", enable);

}


#ifdef MTK_HDMI_ION_SUPPORT
#ifndef MTK_OVERLAY_ENGINE_SUPPORT
static hdmi_video_buffer_list *hdmi_query_buf_mva(unsigned int idx)
{
	hdmi_video_buffer_list *buf = 0;
	/* /spin_lock_bh(&hdmi_lock); */
	list_for_each_entry(buf, &HDMI_Buffer_List, list) {
		if (buf->idx == idx) {
			HDMI_LOG("query buf list=%x, idx=%d, mva=0x%08x\n", (unsigned int)buf, idx,
				 buf->mva);
			return buf;
		}
	}
	/* /spin_unlock_bh(&hdmi_lock); */
	return NULL;
}
static void hdmi_ion_init(void)
{
	/* struct ion_mm_data mm_data; */

	if (!ion_client && g_ion_device) {
		ion_client = ion_client_create(g_ion_device, "hdmi");

		if (!ion_client) {
			HDMI_LOG("create ion client failed!\n");
			return;
		}

		HDMI_LOG("create ion client 0x%p\n", ion_client);
	}
}

/*
static void hdmi_ion_deinit()
{
	if (ion_client)
	{
		ion_client_destroy(ion_client);
		ion_client = NULL;
		HDMI_LOG("destroy ion client 0x%p\n", ion_client);
	}
}*/

static struct ion_handle *hdmi_ion_import_handle(struct ion_client *client, int fd)
{
	struct ion_handle *handle = NULL;
	struct ion_mm_data mm_data;

	/* If no need Ion support, do nothing! */
	if (fd == MTK_HDMI_NO_ION_FD) {
		HDMI_LOG("NO NEED ion support");
		return handle;
	}

	if (!ion_client) {
		HDMI_LOG("invalid ion client!\n");
		return handle;
	}

	if (fd == MTK_HDMI_NO_FENCE_FD) {
		HDMI_LOG("invalid ion fd!\n");
		return handle;
	}

	handle = ion_import_dma_buf(client, fd);

	if (IS_ERR_OR_NULL(handle)) {
		HDMI_LOG("import ion handle failed!\n");
		handle = 0;
		return handle;
	}

	mm_data.mm_cmd = ION_MM_CONFIG_BUFFER;
	mm_data.config_buffer_param.handle = (ion_user_handle_t)handle;
	mm_data.config_buffer_param.eModuleID = 0;
	mm_data.config_buffer_param.security = 0;
	mm_data.config_buffer_param.coherent = 0;

	if (ion_kernel_ioctl(ion_client, ION_CMD_MULTIMEDIA, (unsigned int)&mm_data))
		HDMI_LOG("configure ion buffer failed!\n");

	return handle;
}
static size_t hdmi_ion_phys_mmu_addr(struct ion_client *client, struct ion_handle *handle,
				     unsigned int *mva)
{
	size_t size;

	if (!ion_client) {
		HDMI_LOG("invalid ion client!\n");
		return 0;
	}

	if (!handle)
		return 0;

	ion_phys(client, handle, (ion_phys_addr_t *) mva, &size);
	HDMI_LOG("alloc mmu addr hnd=0x%p,mva=0x%08x\n", handle, (unsigned int)*mva);
	return size;
}

#endif
static void hdmi_ion_free_handle(struct ion_client *client, struct ion_handle *handle)
{
	if (!ion_client) {
		HDMI_LOG("invalid ion client!\n");
		return;
	}

	if (!handle)
		return;

	ion_free(client, handle);
	HDMI_LOG("free ion handle 0x%p\n", handle);
}
#endif

#ifdef MTK_HDMI_FENCE_SUPPORT
#define FENCE_STEP_COUNTER 1
DEFINE_MUTEX(FenceMutex);
static atomic_t timeline_counter = ATOMIC_INIT(0);
static atomic_t fence_counter = ATOMIC_INIT(0);
static struct sw_sync_timeline *hdmi_timeline;
static unsigned int hdmi_get_fence_counter(void)
{
	return atomic_add_return(FENCE_STEP_COUNTER, &fence_counter);
}

static struct sw_sync_timeline *hdmi_create_timeline(void)
{
	char name[32];
	const char *prefix = "hdmi_timeline";

	sprintf(name, "%s", prefix);

	hdmi_timeline = timeline_create(name);

	if (hdmi_timeline == NULL)
		pr_err(" error: cannot create timeline!\n");
	else
		HDMI_LOG(" hdmi_create_timeline() %s\n", name);

	return hdmi_timeline;
}

static struct fence_data hdmi_create_fence(void)
{
	int fenceFd = MTK_HDMI_NO_FENCE_FD;
	struct fence_data data;
	const char *prefix = "hdmi_fence";
	struct sync_fence *fence;

	spin_lock_bh(&hdmi_lock);
	data.value = hdmi_get_fence_counter();
	spin_unlock_bh(&hdmi_lock);
	sprintf(data.name, "%s-%d", prefix, data.value);

	if (hdmi_timeline != NULL) {
		if (fence_create(hdmi_timeline, &data)) {
			pr_err(" error: cannot create Fence Object!\n");
		} else {
			fenceFd = data.fence;
			fence = sync_fence_fdget(fenceFd);
			HDMI_LOG("fence/create: %s(0x%p) fd_%d\n",
					data.name, fence, fenceFd);
		}
	} else
		pr_err(" error: there is no Timeline to create Fence!\n");

	MMProfileLogEx(HDMI_MMP_Events.FenceCreate, MMProfileFlagPulse, fenceFd, data.value);
	return data;
}


static void hdmi_release_fence(void)
{
	unsigned int fence_cnt, timeline_cnt, inc;

	spin_lock_bh(&hdmi_lock);
	fence_cnt = atomic_read(&fence_counter);
	timeline_cnt = atomic_read(&timeline_counter);
	inc = fence_cnt - timeline_cnt;
	spin_unlock_bh(&hdmi_lock);

	if (inc < 0) {
		HDMI_MSG("fence/timeline_inc error: fence_cnt_%d < timeline_cnt_%d!\n",
			fence_cnt, timeline_cnt);
		return;
	} else if (inc < 0 || inc > 5) {
		HDMI_MSG("fence/timeline_inc maybe block too much time: inc=%d, fence_cnt=%d, timeline_cnt=%d!\n",
			inc, fence_cnt, timeline_cnt);
	}

	if (hdmi_timeline != NULL)
		timeline_inc(hdmi_timeline, inc);

	spin_lock_bh(&hdmi_lock);
	atomic_add(inc, &timeline_counter);
	spin_unlock_bh(&hdmi_lock);

	MMProfileLogEx(HDMI_MMP_Events.FenceSignal, MMProfileFlagPulse, atomic_read(&fence_counter),
		       inc);

}

/**
 * step forward timeline
 * all fence(sync_point) will be signaled prior to it's counter
 * refer to {@link sw_sync_timeline_inc}
 */
static void hdmi_signal_fence(void)
{
	if (hdmi_timeline != NULL) {
		timeline_inc(hdmi_timeline, 1);

		spin_lock_bh(&hdmi_lock);
		atomic_add(1, &timeline_counter);
		spin_unlock_bh(&hdmi_lock);
		HDMI_LOG("fence/timeline++: %d (fence_%d)\n",
			atomic_read(&timeline_counter), atomic_read(&fence_counter));

		if (hdmi_bufferdump_on > 0) {
			MMProfileLogEx(HDMI_MMP_Events.FenceSignal, MMProfileFlagPulse,
				       atomic_read(&timeline_counter), atomic_read(&fence_counter));
		}
	} else {
		HDMI_LOG(" no Timeline to inc tl %d, fd %d\n", atomic_read(&timeline_counter),
			 atomic_read(&fence_counter));
	}


}

static void hdmi_sync_init(void)
{

	/* /spin_lock_init(&hdmi_lock); */
	hdmi_create_timeline();
	/* Reset all counter to 0 */
	atomic_set(&timeline_counter, 0);
	atomic_set(&fence_counter, 0);
}

static void hdmi_sync_destroy(void)
{

	if (hdmi_timeline != NULL) {
		HDMI_LOG(" destroy timeline %s:%d\n", hdmi_timeline->obj.name,
			 hdmi_timeline->value);
		timeline_destroy(hdmi_timeline);
		hdmi_timeline = NULL;
	}

	/* Reset all counter to 0 */
	atomic_set(&timeline_counter, 0);
	atomic_set(&fence_counter, 0);
}


#endif

static void hdmi_udelay(unsigned int us)
{
	udelay(us);
}

static void hdmi_mdelay(unsigned int ms)
{
	msleep(ms);
}

static unsigned int hdmi_get_width(HDMI_VIDEO_RESOLUTION r)
{
	ASSERT(r < HDMI_VIDEO_RESOLUTION_NUM);
	return hdmi_resolution_param_table[r][0];
}

static unsigned int hdmi_get_height(HDMI_VIDEO_RESOLUTION r)
{
	ASSERT(r < HDMI_VIDEO_RESOLUTION_NUM);
	return hdmi_resolution_param_table[r][1];
}

HDMI_VIDEO_RESOLUTION hdmi_get_resolution(void)
{
	return p->output_video_resolution;
}


static atomic_t hdmi_fake_in = ATOMIC_INIT(false);
#define IS_HDMI_FAKE_PLUG_IN()  (true == atomic_read(&hdmi_fake_in))
#define SET_HDMI_FAKE_PLUG_IN() (atomic_set(&hdmi_fake_in, true))
#define SET_HDMI_NOT_FAKE()     (atomic_set(&hdmi_fake_in, false))

/* For Debugfs */
void hdmi_cable_fake_plug_in(void)
{
	SET_HDMI_FAKE_PLUG_IN();
	HDMI_LOG("[HDMIFake]Cable Plug In\n");

	if (p->is_force_disable == false) {
		if (IS_HDMI_STANDBY()) {
			hdmi_resume();
			/* /msleep(1000); */
			switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
			hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;
		}
	}
}

/* For Debugfs */
void hdmi_cable_fake_plug_out(void)
{
	SET_HDMI_NOT_FAKE();
	HDMI_LOG("[HDMIFake]Disable\n");

	if (p->is_force_disable == false) {
		if (IS_HDMI_ON()) {
			if (hdmi_drv->get_state() != HDMI_STATE_ACTIVE) {
				hdmi_suspend();
				switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
			}
		}
	}
}

void hdmi_set_mode(unsigned char ucMode)
{
	HDMI_FUNC();

	hdmi_drv->set_mode(ucMode);

	/* return; */
}

void hdmi_reg_dump(void)
{
	hdmi_drv->dump();
}

void hdmi_read_reg(unsigned char u8Reg)
{
}

void hdmi_write_reg(unsigned char u8Reg, unsigned char u8Data)
{
}

/* Will be called in LCD Interrupt handler to check whether HDMI is actived */
bool is_hdmi_active(void)
{
	return IS_HDMI_ON();
}

int get_hdmi_dev_info(HDMI_QUERY_TYPE type)
{
	switch (type) {
	case HDMI_CHARGE_CURRENT:
		{
			if ((p->is_enabled == false)
			    || hdmi_params->cabletype == HDMI_CABLE) {
				return 0;
			} else if (hdmi_params->cabletype == MHL_CABLE) {
				return 500;
			} else if (hdmi_params->cabletype == MHL_2_CABLE) {
				return 900;
			}

		}

	default:
		return 0;
	}

}

#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
void hdmi_config_m4u(unsigned int bSecure)
{
	MTEEC_PARAM param[4];
	unsigned int paramTypes;
	TZ_RESULT ret;

	param[0].value.a = DISP_RDMA1;
	param[1].value.a = bSecure;
	paramTypes = TZ_ParamTypes2(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
	ret =
	    KREE_TeeServiceCall(ddp_session_handle(), TZCMD_DDP_SET_SECURE_MODE, paramTypes, param);
	if (ret != TZ_RESULT_SUCCESS)
		HDMI_LOG("KREE_TeeServiceCall(TZCMD_DDP_SET_SECURE_MODE) fail, ret=%d\n", ret);
}

void hdmi_set_rdma_addr(unsigned int hdmiSourceAddr, unsigned int size, unsigned int bSecure)
{
	MTEEC_PARAM param[4];
	unsigned int paramTypes;
	TZ_RESULT ret;

	param[0].value.a = (uint32_t) hdmiSourceAddr;
	param[1].value.a = (uint32_t) bSecure;
	param[2].value.a = (uint32_t) size;
	paramTypes = TZ_ParamTypes3(TZPT_VALUE_INPUT, TZPT_VALUE_INPUT, TZPT_VALUE_INPUT);
	ret =
	    KREE_TeeServiceCall(ddp_session_handle(), TZCMD_DDP_RDMA1_ADDR_CONFIG, paramTypes,
				param);
	if (ret != TZ_RESULT_SUCCESS)
		HDMI_LOG("TZCMD_DDP_RDMA_ADDR_CONFIG fail, ret=%d\n", ret);
}
#endif


static int hdmi_rdma_config_kthread(void *data)
{
	struct sched_param param = {.sched_priority = 94};
	int buf_configed = 0;

	sched_setscheduler(current, SCHED_RR, &param);

	for (;;) {
		buf_configed = 0;
		wait_event_interruptible(hdmi_rdma_config_wq, atomic_read(&hdmi_rdma_config_event));
		atomic_set(&hdmi_rdma_config_event, 0);

#ifdef MTK_HDMI_FENCE_SUPPORT
		MMProfileLogEx(HDMI_MMP_Events.BufferCfg, MMProfileFlagStart, p->is_clock_on, 1);

		if (down_interruptible(&hdmi_update_mutex)) {
			MMProfileLogEx(HDMI_MMP_Events.MutexErr, MMProfileFlagPulse, Mutex_Err1,
				       Mutex_Err1);
			MMProfileLogEx(HDMI_MMP_Events.BufferCfg, MMProfileFlagEnd, p->is_clock_on,
				       1);
			pr_err("[HDMI] hdmi_rdma_config_kthread down_interruptible fail\n");
			continue;	/* / continue  return EAGAIN */
		}


		if (p->is_clock_on == true) {	/* /remove the first head here */
			if (!list_empty(&HDMI_Buffer_List)) {
				hdmi_video_buffer_list *pBuffList = NULL;

				spin_lock_bh(&hdmi_lock);
				pBuffList =
				    list_first_entry(&HDMI_Buffer_List, hdmi_video_buffer_list,
						     list);

				while (pBuffList->buf_state != insert_new) {
					buf_configed++;

					if (list_is_last(&pBuffList->list, &HDMI_Buffer_List))
						break;

					pBuffList =
					    list_entry(pBuffList->list.next, hdmi_video_buffer_list,
						       list);
				}

				spin_unlock_bh(&hdmi_lock);

				/* ensure update rdma1 register every vsync in interlace resolution */
				if ((pBuffList == NULL) || ((pBuffList->buf_state != insert_new) && !fgInterlace)) {
					/* HDMI_LOG("pBuffList == NULL and it's not insert new"); */
					if (pBuffList && (hdmi_bufferdump_on > 0)) {
						MMProfileLogEx(HDMI_MMP_Events.ErrorInfo,
							       MMProfileFlagPulse,
							       Buffer_Not_Enough,
							       (unsigned int)pBuffList);
					} else if (hdmi_bufferdump_on > 0) {
						MMProfileLogEx(HDMI_MMP_Events.ErrorInfo,
							       MMProfileFlagPulse,
							       Buffer_Not_Enough, buf_configed);
					}
				} else if (pBuffList->fence) {
#ifdef MTK_OVERLAY_ENGINE_SUPPORT
					if (sync_fence_wait((struct sync_fence *)pBuffList->fence, 1000) < 0) {
						pr_err("[HDMI] fence/wait: %s timeout",
							((struct sync_fence *)pBuffList->fence)->name);
						pBuffList->buf_state = buf_dropped;
					} else
#endif
					{
					if (hdmi_rdma_address_config(true, pBuffList->buffer_info) < 0) {
						HDMI_LOG(" rdma config(pBuffList %p) error to exit\n",
									pBuffList);
						pBuffList->buf_state = buf_read_done;
					} else {
						spin_lock_bh(&hdmi_lock);
						pBuffList->buf_state = reg_configed;
						spin_unlock_bh(&hdmi_lock);
						buf_configed = 1;
					}	/* end else of config fail */
					}	/* end else of fence timeout */
				} else {
					pr_err("HDMI buffer fence is NULL\n");
				}
			} else {
				MMProfileLogEx(HDMI_MMP_Events.ErrorInfo, MMProfileFlagPulse,
					       Buffer_Empt_Err, Buffer_Empt_Err);
				HDMI_LOG(" rdma config buffer is NULL\n");
			}
		}

		up(&hdmi_update_mutex);
		MMProfileLogEx(HDMI_MMP_Events.BufferCfg, MMProfileFlagEnd, p->is_clock_on, 1);
#else

#endif

		if (kthread_should_stop())
			break;

	}

	return 0;
}

#ifdef MTK_HDMI_FENCE_SUPPORT
static hdmi_video_buffer_list *hdmi_update_buffer_state(hdmi_video_buffer_list *pBuffList)
{
	int buf_sequence = 0;
	hdmi_video_buffer_list *pUpdateList = NULL;
	hdmi_video_buffer_list *pNextList = NULL;

	while (pBuffList) {
		/* pr_err("[1]buf_state=%d src_phy_addr=0x%08x buf_sequence=%d",
		   pBuffList->buf_state,pBuffList->buffer_info.src_phy_addr, buf_sequence); */
		if (pBuffList->buf_state == insert_new) {
			if (!list_is_last
				(&pBuffList->list, &HDMI_Buffer_List))
				pNextList =
					list_entry(pBuffList->list.next,
						   hdmi_video_buffer_list,
						   list);
			if (pNextList && pNextList->buf_state == insert_new) {
				pBuffList->buf_state = buf_dropped;
				pBuffList = pNextList;
				pNextList = NULL;
			} else
				return pUpdateList;
		} else if (pBuffList->buf_state == reg_configed) {
			buf_sequence++;
			pBuffList->buf_state = reg_updated;
			if (buf_sequence > 1) {
				pUpdateList->buf_state = buf_read_done;
				/* pr_err("[look back]src_phy_addr=0x%08x => buf_read_done",
				   pUpdateList->buffer_info.src_phy_addr); */
			}
			pUpdateList = pBuffList;
		} else if (pBuffList->buf_state == reg_updated) {
			pBuffList->buf_state = buf_read_done;
		}
		/* pr_err("[2]buf_state=%d src_phy_addr=0x%08x buf_sequence=%d",
		   pBuffList->buf_state,pBuffList->buffer_info.src_phy_addr, buf_sequence); */
		if (!list_is_last(&pBuffList->list, &HDMI_Buffer_List)) {
			pBuffList =
				list_entry(pBuffList->list.next,
					   hdmi_video_buffer_list, list);
		} else {
			pBuffList = NULL;
		}
	}
	return pUpdateList;
}

static int hdmi_rotate_buffer(hdmi_video_buffer_list *pBuffList)
{
	int rm_buf_cnt = 0;
	int using_buf_cnt = 0;

	while (pBuffList
		   && !list_is_last(&pBuffList->list, &HDMI_Buffer_List)) {
		if (pBuffList && ((pBuffList->buf_state == buf_read_done)
				  || (pBuffList->buf_state == buf_dropped))) {
			if (pBuffList->buffer_info.src_phy_addr !=
				(void *)rdma1_addr_using) {
				/* do { */
				if (hdmi_bufferdump_on > 0) {
					MMProfileLogEx
						(HDMI_MMP_Events.BufferRemove,
						 MMProfileFlagPulse,
						 (unsigned int)
						 pBuffList, (unsigned int)
						 pBuffList->buffer_info.src_phy_addr);
				} else {
					HDMI_LOG
						("remove list %x-->buffer %x\n",
						 (unsigned int)pBuffList,
						 (unsigned int)
						 pBuffList->buffer_info.src_phy_addr);
				}
				/* pr_err("[deleting]src_phy_addr=0x%08x",
				   pBuffList->buffer_info.src_phy_addr); */

#ifdef MTK_HDMI_ION_SUPPORT
				if (pBuffList->va)
					ion_unmap_kernel(ion_client,
							 pBuffList->hnd);

				hdmi_ion_free_handle(ion_client,
							 pBuffList->hnd);
#endif
				spin_lock_bh(&hdmi_lock);
				if (pBuffList->fence)
					sync_fence_put((struct sync_fence *)pBuffList->fence);
				list_del(&pBuffList->list);
				kfree(pBuffList);
				pBuffList = NULL;
				spin_unlock_bh(&hdmi_lock);

				hdmi_signal_fence();

				rm_buf_cnt++;
				HDMI_LOG("[HDMI] DEL BUF[%d]\n", delbuf++);

				spin_lock_bh(&hdmi_lock);
				pBuffList =
					list_first_entry(&HDMI_Buffer_List,
					hdmi_video_buffer_list, list);
				spin_unlock_bh(&hdmi_lock);
				/* }while(using_buf_cnt--); */

				/* using_buf_cnt = 0; */
			} else {
				/* pr_err("src_phy_addr(0x%08x) != rdma1_addr_using(0x%08x) so doesn't signal fence",
					pBuffList->buffer_info.src_phy_addr,rdma1_addr_using); */
				return rm_buf_cnt;
			}
		} else if (pBuffList && (pBuffList->buf_state == create_new)) {
			using_buf_cnt++;
			pBuffList = list_entry(pBuffList->list.next, hdmi_video_buffer_list, list);
		} else {
			return rm_buf_cnt;
		}
	}
	return rm_buf_cnt;
}

static int hdmi_rdma_update_kthread(void *data)
{
	struct sched_param param = {.sched_priority = 94};
	hdmi_video_buffer_list *pUpdateList = NULL;
	hdmi_video_buffer_list *pBuffList = NULL;
	int remove_buffer_cnt = 0;

	sched_setscheduler(current, SCHED_RR, &param);

	for (;;) {
		remove_buffer_cnt = 0;

		wait_event_interruptible(hdmi_rdma_update_wq, atomic_read(&hdmi_rdma_update_event));
		atomic_set(&hdmi_rdma_update_event, 0);

		MMProfileLogEx(HDMI_MMP_Events.BufferUpdate, MMProfileFlagStart, p->is_clock_on, 1);


		if (down_interruptible(&hdmi_update_mutex)) {
			MMProfileLogEx(HDMI_MMP_Events.MutexErr, MMProfileFlagPulse, Mutex_Err1,
				       Mutex_Err1);
			MMProfileLogEx(HDMI_MMP_Events.BufferUpdate, MMProfileFlagEnd,
				       p->is_clock_on, 1);
			HDMI_LOG("[HDMI] can't get semaphore in\n");
			continue;
		}

		pUpdateList = NULL;

		if (!is_early_suspended && p->is_clock_on == true) {	/* /remove the first head here */
			if (!list_empty(&HDMI_Buffer_List)) {
				pBuffList = NULL;

				spin_lock_bh(&hdmi_lock);
				pBuffList =
				list_first_entry(&HDMI_Buffer_List,
				hdmi_video_buffer_list, list);
				pUpdateList = hdmi_update_buffer_state(pBuffList);

				pBuffList = NULL;
				pBuffList =
				list_first_entry(&HDMI_Buffer_List,
					hdmi_video_buffer_list, list);
				spin_unlock_bh(&hdmi_lock);

				remove_buffer_cnt = hdmi_rotate_buffer(pBuffList);

				if (remove_buffer_cnt > 1) {
					if (hdmi_bufferdump_on > 0) {
						MMProfileLogEx(HDMI_MMP_Events.ErrorInfo,
						MMProfileFlagPulse, Buff_Dup_Err1, remove_buffer_cnt);
					}

					HDMI_LOG("[hdmi] %s, %d remove two buffer one time\n",
						 __func__, __LINE__);

				}
			}

		} else {
			HDMI_LOG("fence stop rdma\n");
			pBuffList = NULL;

			while (!list_empty(&HDMI_Buffer_List)) {
				spin_lock_bh(&hdmi_lock);
				pBuffList =
				    list_first_entry(&HDMI_Buffer_List, hdmi_video_buffer_list,
						     list);
				spin_unlock_bh(&hdmi_lock);

				if (hdmi_bufferdump_on > 0) {
					MMProfileLogEx(HDMI_MMP_Events.BufferRemove,
						       MMProfileFlagPulse, (unsigned int)pBuffList,
						       (unsigned int)pBuffList->buffer_info.src_phy_addr);
				} else {
					HDMI_LOG("delete list %x-->buffer %x\n",
						 (unsigned int)pBuffList,
						 (unsigned int)pBuffList->buffer_info.src_phy_addr);
				}

#ifdef MTK_HDMI_ION_SUPPORT

				if (pBuffList->va)
					ion_unmap_kernel(ion_client, pBuffList->hnd);

				hdmi_ion_free_handle(ion_client, pBuffList->hnd);
#endif

				spin_lock_bh(&hdmi_lock);
				if (pBuffList->fence)
					sync_fence_put((struct sync_fence *)pBuffList->fence);
				list_del(&pBuffList->list);
				kfree(pBuffList);
				pBuffList = NULL;
				spin_unlock_bh(&hdmi_lock);
				HDMI_LOG("[HDMI] DEL BUF[%d]\n", delbuf++);
			}

			hdmi_release_fence();
			HDMI_LOG("fence stop rdma done\n");
		}

		up(&hdmi_update_mutex);
		MMProfileLogEx(HDMI_MMP_Events.BufferUpdate, MMProfileFlagEnd, p->is_clock_on,
			       (unsigned int)pUpdateList);

		if (kthread_should_stop())
			break;

	}

	return 0;
}
#endif

#undef HDMI_ANALOG_BASE
#undef MHL_TVDPLL_CON0
#undef RG_TVDPLL_EN
#undef RG_TVDPLL_POSDIV
#undef RG_TVDPLL_POSDIV_MASK
#undef MHL_TVDPLL_CON1
#undef RG_TVDPLL_SDM_PCW
#undef RG_TVDPLL_SDM_PCW_MASK
#undef TVDPLL_SDM_PCW_CHG
#undef TVDPLL_SDM_PCW_F
#undef MHL_TVDPLL_PWR
#undef RG_TVDPLL_PWR_ON

static unsigned long HDMI_ANALOG_BASE;
#define MHL_TVDPLL_CON0	0x260
#define RG_TVDPLL_EN			(1)
#define RG_TVDPLL_POSDIV				(4)
#define RG_TVDPLL_POSDIV_MASK			(0x07 << 4)
#define MHL_TVDPLL_CON1	0x264
#define RG_TVDPLL_SDM_PCW				(0)
#define RG_TVDPLL_SDM_PCW_MASK			(0x1FFFFF)
#define TVDPLL_SDM_PCW_CHG        (1 << 31)
#define TVDPLL_SDM_PCW_F        (1<<23)

#define MHL_TVDPLL_PWR	0x26C
#define RG_TVDPLL_PWR_ON		(1)

#define vWriteHdmiANA(dAddr, dVal)  (*((volatile unsigned int *)(HDMI_ANALOG_BASE + dAddr)) = (dVal))
#define dReadHdmiANA(dAddr)         (*((volatile unsigned int *)(HDMI_ANALOG_BASE + dAddr)))
#define vWriteHdmiANAMsk(dAddr, dVal, dMsk)\
	(vWriteHdmiANA((dAddr), (dReadHdmiANA(dAddr) & (~(dMsk))) | ((dVal) & (dMsk))))
/* --------------------------FIXME------------------------------- */
void hdmi_config_pll(HDMI_VIDEO_RESOLUTION resolution)
{

	vWriteHdmiANAMsk(0x0, 0x1131, 0x1131);
	vWriteHdmiANAMsk(MHL_TVDPLL_PWR, RG_TVDPLL_PWR_ON, RG_TVDPLL_PWR_ON);
	mdelay(1);
	vWriteHdmiANAMsk(0x0, 0x1133, 0x1133);
	mdelay(5);
	/* vWriteHdmiANAMsk(MHL_TVDPLL_CON1,TVDPLL_SDM_PCW_CHG,TVDPLL_SDM_PCW_CHG); */
	switch (resolution) {
	case HDMI_VIDEO_720x480p_60Hz:
	case HDMI_VIDEO_720x576p_50Hz:
		vWriteHdmiANAMsk(MHL_TVDPLL_CON0, 0, RG_TVDPLL_EN);
		vWriteHdmiANAMsk(MHL_TVDPLL_CON0, (0x04 << RG_TVDPLL_POSDIV),
				 RG_TVDPLL_POSDIV_MASK);
		vWriteHdmiANAMsk(MHL_TVDPLL_CON1, (0x0109d8a << RG_TVDPLL_SDM_PCW),
				 RG_TVDPLL_SDM_PCW_MASK);
		vWriteHdmiANAMsk(MHL_TVDPLL_CON0, RG_TVDPLL_EN, RG_TVDPLL_EN);

		break;

	case HDMI_VIDEO_1920x1080p_30Hz:
	case HDMI_VIDEO_1280x720p_50Hz:
	case HDMI_VIDEO_1920x1080i_50Hz:
#if defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	case HDMI_VIDEO_1920x1080i3d_sbs_50Hz:
#endif
	case HDMI_VIDEO_1920x1080p_25Hz:
	case HDMI_VIDEO_1920x1080p_24Hz:
	case HDMI_VIDEO_1920x1080p_50Hz:
	case HDMI_VIDEO_1280x720p3d_50Hz:
	case HDMI_VIDEO_1920x1080i3d_50Hz:
	case HDMI_VIDEO_1920x1080p3d_24Hz:
		vWriteHdmiANAMsk(MHL_TVDPLL_CON0, 0, RG_TVDPLL_EN);
		vWriteHdmiANAMsk(MHL_TVDPLL_CON0, (0x04 << RG_TVDPLL_POSDIV),
				 RG_TVDPLL_POSDIV_MASK);
		vWriteHdmiANAMsk(MHL_TVDPLL_CON1, (1497246 << RG_TVDPLL_SDM_PCW),
				 RG_TVDPLL_SDM_PCW_MASK);
		vWriteHdmiANAMsk(MHL_TVDPLL_CON0, RG_TVDPLL_EN, RG_TVDPLL_EN);

		break;

	case HDMI_VIDEO_1280x720p_60Hz:
	case HDMI_VIDEO_1920x1080i_60Hz:
#if defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	case HDMI_VIDEO_1920x1080i3d_sbs_60Hz:
#endif
	case HDMI_VIDEO_1920x1080p_23Hz:
	case HDMI_VIDEO_1920x1080p_29Hz:
	case HDMI_VIDEO_1920x1080p_60Hz:
	case HDMI_VIDEO_1280x720p3d_60Hz:
	case HDMI_VIDEO_1920x1080i3d_60Hz:
	case HDMI_VIDEO_1920x1080p3d_23Hz:
		vWriteHdmiANAMsk(MHL_TVDPLL_CON0, 0, RG_TVDPLL_EN);
		vWriteHdmiANAMsk(MHL_TVDPLL_CON0, (0x04 << RG_TVDPLL_POSDIV),
				 RG_TVDPLL_POSDIV_MASK);
		vWriteHdmiANAMsk(MHL_TVDPLL_CON1, (1495733 << RG_TVDPLL_SDM_PCW),
				 RG_TVDPLL_SDM_PCW_MASK);
		vWriteHdmiANAMsk(MHL_TVDPLL_CON0, RG_TVDPLL_EN, RG_TVDPLL_EN);

		break;
	default:
		{
			break;
		}
	}

	mdelay(20);
	/* vWriteHdmiANAMsk(MHL_TVDPLL_CON1,TVDPLL_SDM_PCW_CHG,TVDPLL_SDM_PCW_CHG); */
}


static wait_queue_head_t hdmi_vsync_wq;
static bool hdmi_vsync_flag;
static int hdmi_vsync_cnt;

void hdmi_waitVsync(void)
{
	MMProfileLogEx(HDMI_MMP_Events.WaitVSync, MMProfileFlagStart, hdmi_vsync_cnt,
		       p->is_clock_on);

	if (p->is_clock_on == false) {
		pr_info("[hdmi]:hdmi has suspend, return directly\n");
		msleep(19);
		MMProfileLogEx(HDMI_MMP_Events.WaitVSync, MMProfileFlagEnd, hdmi_vsync_cnt,
			       p->is_clock_on);
		return;
	}

	hdmi_vsync_cnt++;

	hdmi_vsync_flag = 0;

	if (wait_event_interruptible_timeout(hdmi_vsync_wq, hdmi_vsync_flag, HZ / 10) == 0)
		pr_err("[hdmi] Wait VSync timeout. early_suspend=%d\n", p->is_clock_on);

	MMProfileLogEx(HDMI_MMP_Events.WaitVSync, MMProfileFlagEnd, hdmi_vsync_cnt, p->is_clock_on);
	hdmi_vsync_cnt--;
	/* return; */
}
EXPORT_SYMBOL(hdmi_waitVsync);

static void _rdma1_irq_handler(unsigned int param)
{
	if (!is_hdmi_active())
		return;

	/*if(!p->lcm_is_video_mode)
	   return; */
	/* HDMI_LOG("_rdma1_irq_handler irq %x\n", param); */

	if (param & 0x20) {	/* taget line 0x20 */
		MMProfileLogEx(HDMI_MMP_Events.RDMA1RegisterUpdated, MMProfileFlagPulse, param,
			       0x20);

		/* /hdmi_update_buffer_switch(); */

		atomic_set(&hdmi_rdma_config_event, 1);
		wake_up_interruptible(&hdmi_rdma_config_wq);
	}

	if (param & (1 << 2)) {	/* frame done */
		/* pr_err("enter _rdma1_irq_handler frame done rdma1_addr_using=0x%08x rdma1_addr_shadowed=0x%08x",
		   rdma1_addr_using, rdma1_addr_shadowed); */
		rdma1_addr_using = rdma1_addr_shadowed;
	}
	if ((param & 2) && (hdmi_params->cabletype == MHL_SMB_CABLE)) {	/* rdma1 register updated */
		MMProfileLogEx(HDMI_MMP_Events.RDMA1RegisterUpdated, MMProfileFlagPulse, param, 2);

		hdmi_vsync_flag = 1;
		wake_up_interruptible(&hdmi_vsync_wq);

	}

	if (param & 1) {	/* rdma1 register updated */
		MMProfileLogEx(HDMI_MMP_Events.RDMA1RegisterUpdated, MMProfileFlagPulse, param, 1);

		atomic_set(&hdmi_rdma_update_event, 1);
		wake_up_interruptible(&hdmi_rdma_update_wq);

	}
#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
	else if (gRDMASecureSwitch && (param & 4)) {
		atomic_set(&hdmi_rdma_update_event, 1);
		wake_up_interruptible(&hdmi_rdma_update_wq);
		gRDMASecureSwitch = 0;
	}
#endif
}

/* Allocate memory, set M4U, LCD, MDP, DPI */
/* LCD overlay to memory -> MDP resize and rotate to memory -> DPI read to HDMI */
/* Will only be used in ioctl(MTK_HDMI_AUDIO_VIDEO_ENABLE) */
static HDMI_STATUS hdmi_drv_init(void)
{
	int lcm_width, lcm_height;

	HDMI_FUNC();

	if (p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS) {
		HDMI_LOG(" dpi bypass mode ,return directly\n");
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}

	lcm_width = DISP_GetScreenWidth();
	lcm_height = DISP_GetScreenHeight();

	p->hdmi_width = hdmi_get_width(hdmi_params->init_config.vformat);
	p->hdmi_height = hdmi_get_height(hdmi_params->init_config.vformat);
	p->bg_width = 0;
	p->bg_height = 0;
	p->lcm_width = lcm_width;
	p->lcm_height = lcm_height;
	p->output_video_resolution = hdmi_params->init_config.vformat;
	p->output_audio_format = hdmi_params->init_config.aformat;
	p->scaling_factor = hdmi_params->scaling_factor < 10 ? hdmi_params->scaling_factor : 10;

	HDMI_LOG("hdmi_width=%d, hdmi_height=%d\n", p->hdmi_width, p->hdmi_height);
	HDMI_LOG("lcm_width=%d, lcm_height=%d\n", lcm_width, lcm_height);

	/* hdmi_pattern(1);  //M0 debug */

	hdmi_dpi_config_clock();	/* configure dpi clock */

	hdmi_dpi_power_switch(false);	/* but dpi power is still off */

	disp_register_irq(DISP_MODULE_RDMA1, _rdma1_irq_handler);

#ifdef MTK_HDMI_FENCE_SUPPORT

	if (!hdmi_rdma_config_task) {
		hdmi_rdma_config_task =
		    kthread_create(hdmi_rdma_config_kthread, NULL,
				   "hdmi_rdma_config_kthread");
		wake_up_process(hdmi_rdma_config_task);
	}

	if (!hdmi_rdma_update_task) {
		hdmi_rdma_update_task =
		    kthread_create(hdmi_rdma_update_kthread, NULL,
				   "hdmi_rdma_update_kthread");
		wake_up_process(hdmi_rdma_update_task);
	}
#endif

	return HDMI_STATUS_OK;
}

/* free IRQ */
/*static*/ void hdmi_dpi_free_irq(void)
{
	if (p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS) {
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}
	DPI_CHECK_RET(HDMI_DPI(_FreeIRQ) ());
}

/* Release memory */
/* Will only be used in ioctl(MTK_HDMI_AUDIO_VIDEO_ENABLE) */
static HDMI_STATUS hdmi_drv_deinit(void)
{
	HDMI_FUNC();
	if (p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}

	disp_unregister_irq(DISP_MODULE_RDMA1, _rdma1_irq_handler);

	hdmi_dpi_power_switch(false);
	hdmi_dpi_free_irq();
	return HDMI_STATUS_OK;
}

static void hdmi_dpi_config_update(void)
{
	HDMI_FUNC();
	DPI_CHECK_RET(HDMI_DPI(_SW_Reset) (0x1));
	DPI_CHECK_RET(HDMI_DPI(_ConfigPixelClk) (clk_pol, dpi_clk_div, dpi_clk_duty));

	DPI_CHECK_RET(HDMI_DPI(_ConfigDataEnable) (de_pol));	/* maybe no used */

	DPI_CHECK_RET(HDMI_DPI(_ConfigHsync)
		      (hsync_pol, hsync_pulse_width, hsync_back_porch, hsync_front_porch));

	DPI_CHECK_RET(HDMI_DPI(_ConfigVsync)
		      (vsync_pol, vsync_pulse_width, vsync_back_porch, vsync_front_porch));

#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	if (fgInterlace) {
		if (fg3DFrame) {
			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_LEVEN)
				      (vsync_pulse_width, vsync_back_porch, vsync_front_porch,
				       fgInterlace));

			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_RODD)
				      (vsync_pulse_width, vsync_back_porch, vsync_front_porch));

			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_REVEN)
				      (vsync_pulse_width, vsync_back_porch, vsync_front_porch,
				       fgInterlace));
		} else {
			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_LEVEN)
				      (vsync_pulse_width, vsync_back_porch + 1, vsync_front_porch,
				       fgInterlace));

			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_RODD) (0, 0, 0));

			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_REVEN) (0, 0, 0, 0));
		}
	} else {
		if (fg3DFrame) {
			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_LEVEN) (0, 0, 0, 0));

			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_RODD)
				      (vsync_pulse_width, vsync_back_porch, vsync_front_porch));

			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_REVEN) (0, 0, 0, 0));
		} else {
			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_LEVEN) (0, 0, 0, 0));

			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_RODD) (0, 0, 0));

			DPI_CHECK_RET(HDMI_DPI(_ConfigVsync_REVEN) (0, 0, 0, 0));
		}
	}
	DPI_CHECK_RET(HDMI_DPI(_Config_Ctrl) (fg3DFrame, fgInterlace));	/* config 3D and Interlace */
#endif
	if ((HDMI_VIDEO_1920x1080i_60Hz == p->output_video_resolution) ||
	    (HDMI_VIDEO_1920x1080i_50Hz == p->output_video_resolution)
#if defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	    || (HDMI_VIDEO_1920x1080i3d_sbs_60Hz == p->output_video_resolution)
	    || (HDMI_VIDEO_1920x1080i3d_sbs_50Hz == p->output_video_resolution)
#endif
	    ) {
		DPI_CHECK_RET(HDMI_DPI(_FBSetSize) (p->hdmi_width, p->hdmi_height / 2));
	} else {
		DPI_CHECK_RET(HDMI_DPI(_FBSetSize) (p->hdmi_width, p->hdmi_height));
	}

	DPI_CHECK_RET(HDMI_DPI(_FBSetPitch) (DPI_FB_0, p->hdmi_width * 3));	/* do nothing */
	DPI_CHECK_RET(HDMI_DPI(_FBEnable) (DPI_FB_0, TRUE));	/* do nothing */
#if defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	DPI_CHECK_RET(DPI1_SetChannelLimit(0x0100, 0x0EB0, 0x0100, 0x0EB0));
#endif
	/* OUTREG32(0xF208C090, 0x41); */
	DPI_CHECK_RET(HDMI_DPI(_FBSetFormat) (DPI_FB_FORMAT_RGB888));	/* do nothing */

	if (HDMI_COLOR_ORDER_BGR == rgb_order)
		DPI_CHECK_RET(HDMI_DPI(_SetRGBOrder) (DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_BGR));	/* do nothing */
	else
		DPI_CHECK_RET(HDMI_DPI(_SetRGBOrder) (DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_RGB));	/* do nothing */

#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	DPI_CHECK_RET(HDMI_DPI(_Config_ColorSpace) (hdmi_colorspace, hdmi_res));
	DPI_CHECK_RET(HDMI_DPI(_SW_Reset) (0x0));
#endif
}


/* Will only be used in hdmi_drv_init(), this means that will only be use in ioctl(MTK_HDMI_AUDIO_VIDEO_ENABLE) */
/*static*/ void hdmi_dpi_config_clock(void)
{
	int ret = 0;

	HDMI_FUNC();

	if (p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS) {
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}
#if 0
	ret = enable_pll(TVDPLL, "HDMI");
	p->is_clock_on = true;

	HDMI_LOG("set clock on true\n");

	if (ret)
		HDMI_LOG("enable_pll fail!!\n");
#endif

	p->is_clock_on = true;	/* ???Right?? */


	switch (hdmi_params->init_config.vformat) {
	case HDMI_VIDEO_720x480p_60Hz:
		{
			pr_info("[hdmi]480p\n");
			/* ret = pll_fsel(TVDPLL, 0x1C7204C7); */
			ASSERT(!ret);

			dpi_clk_div = 2;
			dpi_clk_duty = 1;

			break;
		}
	case HDMI_VIDEO_1280x720p_60Hz:
		{
			pr_info("[hdmi]720p 60Hz\n");

			dpi_clk_div = 2;
			dpi_clk_duty = 1;

			break;
		}
#if defined(MTK_MT8193_HDMI_SUPPORT) ||\
	defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) ||\
	defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	case HDMI_VIDEO_1280x720p_50Hz:
		{
			pr_info("[hdmi]720p 50Hz\n");
			/* ret = pll_fsel(TVDPLL, 0x1C7204C7); */
			ASSERT(!ret);

			dpi_clk_div = 2;
			dpi_clk_duty = 1;

			break;
		}
#endif
	case HDMI_VIDEO_1920x1080p_30Hz:
		{
			pr_info("[hdmi]1080p 30Hz\n");
			dpi_clk_div = 2;
			dpi_clk_duty = 1;

			break;
		}

	default:
		{
			pr_err("[hdmi] not supported format, %s, %d, format = %d\n", __func__,
			       __LINE__, hdmi_params->init_config.vformat);
			break;
		}
	}

	clk_pol = hdmi_params->clk_pol;
	de_pol = hdmi_params->de_pol;
	hsync_pol = hdmi_params->hsync_pol;
	vsync_pol = hdmi_params->vsync_pol;

	hsync_pulse_width = hdmi_params->hsync_pulse_width;
	vsync_pulse_width = hdmi_params->vsync_pulse_width;
	hsync_back_porch = hdmi_params->hsync_back_porch;
	vsync_back_porch = hdmi_params->vsync_back_porch;
	hsync_front_porch = hdmi_params->hsync_front_porch;
	vsync_front_porch = hdmi_params->vsync_front_porch;

	rgb_order = hdmi_params->rgb_order;
	intermediat_buffer_num = hdmi_params->intermediat_buffer_num;


	DPI_CHECK_RET(HDMI_DPI(_Init) (TRUE));
}

static int rdmafpscnt;
int hdmi_rdma_address_config(bool enable, hdmi_video_buffer_info buffer_info)
{
	int rdmaInputFormat;
	int rdmaInputsize;
	int buffer_size;
	unsigned int offset;
	unsigned int hdmiSourceAddr = 0;
	struct disp_path_config_struct config = { 0 };
	bool need_config;

	/* /HDMI_FUNC(); */
	/*pr_err
		("[rdma_addr_config]info: fmt %d, h %d, w %d, x_o %d, y_o %d, pitch %d hdmi_h %d -w %d\n",
		 buffer_info.src_fmt, buffer_info.src_height, buffer_info.src_width,
		 buffer_info.src_offset_x, buffer_info.src_offset_y,
		 buffer_info.src_pitch, p->hdmi_height, p->hdmi_width);*/

	if (enable) {
		if (p->is_clock_on == false) {
			HDMI_LOG(" clock stopped enable(%d), is_clock_on(%d)\n", enable,
				 p->is_clock_on);
			return -1;
		}
#ifdef MTK_OVERLAY_ENGINE_SUPPORT
		rdmaInputFormat = eRGB888;	/* RDMA_INPUT_FORMAT_RGB888; */
		rdmaInputsize = 3;
#else
		rdmaInputFormat = 16;
		rdmaInputsize = 3;

		if (buffer_info.src_fmt == MTK_FB_FORMAT_ARGB8888) {
			rdmaInputsize = 4;
			rdmaInputFormat = 16;
		} else if (buffer_info.src_fmt == MTK_FB_FORMAT_BGR888) {
			rdmaInputsize = 3;
			rdmaInputFormat = 8;
		}
#endif

		buffer_size = buffer_info.src_width * buffer_info.src_height * rdmaInputsize;

#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
		if (buffer_info.security == 1) {
			/* This will be a secure buffer handle */
			hdmiSourceAddr = (unsigned int)buffer_info.src_phy_addr;
		} else
#endif
		{
			offset =
			    (buffer_info.src_pitch - buffer_info.src_width) / 2 * rdmaInputsize;
			hdmiSourceAddr =
			    (unsigned int)buffer_info.src_phy_addr +
			    buffer_info.src_offset_y * buffer_info.src_pitch * rdmaInputsize +
			    buffer_info.src_offset_x * rdmaInputsize + offset;
		}

		config.addr = hdmiSourceAddr;
		config.srcWidth = buffer_info.src_width;
		config.srcHeight = buffer_info.src_height;
		config.bgROI.width = buffer_info.src_width;
		config.bgROI.height = buffer_info.src_height;
		config.srcROI.width = buffer_info.src_width;
		config.srcROI.height = buffer_info.src_height;
		config.srcROI.x = 0;
		config.srcROI.y = 0;
		config.bgROI.x = 0;
		config.bgROI.y = 0;
		config.bgColor = 0x0;	/* background color */
		config.inFormat = rdmaInputFormat;
		config.pitch = buffer_info.src_pitch * rdmaInputsize;
		config.outFormat = RDMA_OUTPUT_FORMAT_ARGB;
		config.srcModule = DISP_MODULE_RDMA1;
		config.dstModule = HMID_DEST_DPI;

#if 1
		if ((hdmi_abs(buffer_info.src_height - p->hdmi_height) > 32)
		    || (hdmi_abs(buffer_info.src_width - p->hdmi_width) > 32)) {
			HDMI_LOG
			    ("[error]info: fmt %d, h %d, w %d, x_o %d, y_o %d, pitch %d hdmi_h %d -w %d\n",
			     buffer_info.src_fmt, buffer_info.src_height, buffer_info.src_width,
			     buffer_info.src_offset_x, buffer_info.src_offset_y,
			     buffer_info.src_pitch, p->hdmi_height, p->hdmi_width);
			return -1;
		}
#endif

		if ((HDMI_VIDEO_1920x1080i_60Hz == p->output_video_resolution) ||
		    (HDMI_VIDEO_1920x1080i_50Hz == p->output_video_resolution)
#if defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
		    || (HDMI_VIDEO_1920x1080i3d_sbs_60Hz == p->output_video_resolution)
		    || (HDMI_VIDEO_1920x1080i3d_sbs_50Hz == p->output_video_resolution)
#endif
		    ) {
			config.pitch = config.pitch * 2;
			config.srcHeight /= 2;
			if (DPI1_IS_TOP_FIELD()) {
#ifdef CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT
				if (buffer_info.security == 0)
#endif
					config.addr += buffer_info.src_pitch * rdmaInputsize;
			}
		}

		need_config = true;

		if (dp_mutex_dst <= 0) {
			dp_mutex_dst = 2;
			rdmafpscnt = 0;
		} else {
			need_config = false;
		}

		rdmafpscnt++;

#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
		disp_path_get_mutex_(dp_mutex_dst);
		if (true == need_config) {
			/* Config M4U setting */
#if 1
			gRDMASecure = buffer_info.security;
			hdmi_config_m4u(buffer_info.security);	/* Config M4U normal or secure */

			/* RDMA1 will not be started and setted buffer address in this function for SVP */
			disp_path_config_(&config, dp_mutex_dst);

			if (buffer_info.security) {
				hdmi_set_rdma_addr(hdmiSourceAddr, buffer_size,
						   buffer_info.security);
			} else {
				DISP_REG_SET(DISP_RDMA_OFFSET + DISP_REG_RDMA_MEM_START_ADDR, hdmiSourceAddr);
			}
			disp_path_release_mutex_(dp_mutex_dst);
			DPI_CHECK_RET(HDMI_DPI(_EnableClk) ());
			RDMAStart(1);	/* Start RDMA1 for HDMI Path */
#else
			M4U_PORT_STRUCT m4uport;

			memset((void *)&m4uport, 0, sizeof(M4U_PORT_STRUCT));
			m4uport.ePortID = DISP_RDMA1;
			m4uport.Virtuality = 1;
			m4uport.domain = 0;
			m4uport.Security = 0;
			m4uport.Distance = 1;
			m4uport.Direction = 0;
			m4u_config_port(&m4uport);
			disp_path_config_(&config, dp_mutex_dst);
			/* RDMA1 will not be started and setted buffer address in this function for SVP */
			disp_path_release_mutex_(dp_mutex_dst);
#endif
		} else {
			/* hdmi_set_rdma_addr(hdmiSourceAddr, buffer_info.security); */
			if (buffer_info.security != gRDMASecure) {
				/* if (gRDMASecure) */
				/* register tee interrupt handler */
				disp_register_intr(MT6582_DISP_RDMA1_IRQ_ID, 1);
				hdmi_set_rdma_addr(hdmiSourceAddr, buffer_size,
						   buffer_info.security);
				rdma1_addr_shadowed = (unsigned int)hdmiSourceAddr;
				disp_path_release_mutex_(dp_mutex_dst);
				/* if (gRDMASecure) */
				/* register ree interrupt handler */
				disp_register_intr(MT6582_DISP_RDMA1_IRQ_ID, 0);
				gRDMASecure = buffer_info.security;
				gRDMASecureSwitch = 1;
			} else {
				/* rdma1_addr_shadowed = (unsigned int)buffer_info.src_phy_addr; */
				if (buffer_info.security) {
					hdmi_set_rdma_addr(hdmiSourceAddr, buffer_size,
							   buffer_info.security);
				} else {
					/*need to config more than just addr
					DISP_REG_SET(DISP_RDMA_OFFSET + DISP_REG_RDMA_MEM_START_ADDR, hdmiSourceAddr);*/
					RDMAConfig(1,
						RDMA_MODE_MEMORY,
						rdmaInputFormat,
						config.addr,
						RDMA_OUTPUT_FORMAT_ARGB,
						config.pitch,
						buffer_info.src_width,
						config.srcHeight,
						0,
						0);
				}
				rdma1_addr_shadowed = (unsigned int)config.addr;
				disp_path_release_mutex_(dp_mutex_dst);
			}

		}
#else
		disp_path_get_mutex_(dp_mutex_dst);
		if (true == need_config) {
			M4U_PORT_STRUCT m4uport;

			memset((void *)&m4uport, 0, sizeof(M4U_PORT_STRUCT));
			m4uport.ePortID = DISP_RDMA1;
			m4uport.Virtuality = 1;
			m4uport.domain = 1;
			m4uport.Security = 0;
			m4uport.Distance = 1;
			m4uport.Direction = 0;
			HDMI_LOG("HDMI m4u_config port\n");
			m4u_config_port(&m4uport);
			disp_path_config_(&config, dp_mutex_dst);

			/* DPI_CHECK_RET(HDMI_DPI(_EnableColorBar)()); */
			DPI_CHECK_RET(HDMI_DPI(_EnableClk) ());
			DPI_CHECK_RET(HDMI_DPI(_DumpRegisters) ());
		} else {
			/*DISP_REG_SET(0xa000 + DISP_REG_RDMA_MEM_START_ADDR, config.addr); */
			/*need to config more than just addr
			DISP_REG_SET(DISP_RDMA_OFFSET + DISP_REG_RDMA_MEM_START_ADDR, hdmiSourceAddr);*/
			RDMAConfig(1,
				RDMA_MODE_MEMORY,
				rdmaInputFormat,
				config.addr,
				RDMA_OUTPUT_FORMAT_ARGB,
				config.pitch,
				buffer_info.src_width,
				config.srcHeight,
				0,
				0);

			rdma1_addr_shadowed = (unsigned int)config.addr;
		}
		disp_path_release_mutex_(dp_mutex_dst);
#endif
		/* disp_path_release_mutex_(dp_mutex_dst); */
	}	else {
	if (-1 != dp_mutex_dst) {
			/* FIXME: release mutex timeout */
			HDMI_LOG("Stop RDMA1>DPI\n");
			disp_path_get_mutex_(dp_mutex_dst);

			/* DISP_REG_SET_FIELD(1 << dp_mutex_src , DISP_REG_CONFIG_MUTEX_INTEN,	1); */
			RDMAStop(1);
			RDMAReset(1);
			disp_path_release_mutex_(dp_mutex_dst);

			/* disp_unlock_mutex(dp_mutex_dst); */
			dp_mutex_dst = -1;
#ifdef MTK_SEC_VIDEO_PATH_SUPPORT
			gRDMASecure = 0;
			gRDMASecureSwitch = 0;
			hdmi_config_m4u(0);
#endif
		}
	}
	return 0;
}

/* Switch DPI Power for HDMI Driver */
/*static*/ void hdmi_dpi_power_switch(bool enable)
{
	HDMI_FUNC();
	HDMI_LOG("%s enable : %d\n", __func__, enable);

	if (p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS)
		HDMI_LOG("output_mode:%d dpi bypass\n", p->output_mode);

	if (p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS) {
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}

	if (enable) {
		if (p->is_clock_on == true) {
			HDMI_LOG("power on request while already powered on!\n");
			return;
		}
		HDMI_DPI(_PowerOn) ();
		HDMI_DPI(_EnableIrq) ();
		DPI_CHECK_RET(HDMI_DPI(_EnableClk) ());

		p->is_clock_on = true;
	} else {
		if (p->is_clock_on == false) {
			HDMI_MSG("power off request while already powered off!\n");
			return;
		}
		p->is_clock_on = false;
		atomic_set(&hdmi_rdma_update_event, 1);	/* /release buffer */
		wake_up_interruptible(&hdmi_rdma_update_wq);

		HDMI_DPI(_DisableIrq) ();
		HDMI_DPI(_DisableClk) ();
		HDMI_DPI(_PowerOff) ();
	}
}

/* Configure video attribute */
static int hdmi_video_config(HDMI_VIDEO_RESOLUTION vformat, enum HDMI_VIDEO_INPUT_FORMAT vin,
			     enum HDMI_VIDEO_OUTPUT_FORMAT vout)
{
	HDMI_FUNC();

	if (IS_HDMI_NOT_ON()) {
		HDMI_LOG(" hdmi power state(%d) is not on,change resolution\n",
			 atomic_read(&p->state));
	}
	if (IS_HDMI_NOT_ON()) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}

#if 0	/* for debug using */
	hdmi_pattern(0);
#endif

	hdmi_fps_control_overlay = 0;
	hdmi_fps_control_dpi = 0;

	return hdmi_drv->video_config(vformat, vin, vout);
}

/* Configure audio attribute, will be called by audio driver */
int hdmi_audio_config(int samplerate)
{
	HDMI_FUNC();

	if (!p->is_enabled) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}
	if (IS_HDMI_NOT_ON()) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}

	HDMI_LOG("sample rate=%d\n", samplerate);

	if (samplerate == 48000)
		p->output_audio_format = HDMI_AUDIO_PCM_16bit_48000;
	else if (samplerate == 44100)
		p->output_audio_format = HDMI_AUDIO_PCM_16bit_44100;
	else if (samplerate == 32000)
		p->output_audio_format = HDMI_AUDIO_PCM_16bit_32000;
	else
		HDMI_LOG("samplerate not support:%d\n", samplerate);


	hdmi_drv->audio_config(p->output_audio_format);

	return 0;
}

/* No one will use this function */
/*static*/ int hdmi_video_enable(bool enable)
{
	HDMI_FUNC();


	return hdmi_drv->video_enable(enable);
}

/* No one will use this function */
/*static*/ int hdmi_audio_enable(bool enable)
{
	HDMI_FUNC();


	return hdmi_drv->audio_enable(enable);
}

struct timer_list timer;
void __timer_isr(unsigned long n)
{
	HDMI_FUNC();

	if (hdmi_drv->audio_enable)
		hdmi_drv->audio_enable(true);

	del_timer(&timer);
}

int hdmi_audio_delay_mute(int latency)
{
	HDMI_FUNC();
	memset((void *)&timer, 0, sizeof(timer));
	timer.expires = jiffies + (latency * HZ / 1000);
	timer.function = __timer_isr;
	init_timer(&timer);
	add_timer(&timer);

	if (hdmi_drv->audio_enable)
		hdmi_drv->audio_enable(false);

	return 0;
}

#if (defined(MTK_ALPS_BOX_SUPPORT)) ||\
	((!defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)) &&\
	(!defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)))
/* Reset HDMI Driver state */
static void hdmi_state_reset(void)
{
	HDMI_FUNC();

	if (hdmi_drv->get_state() == HDMI_STATE_ACTIVE) {
		switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
		hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;
	} else {
		switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
		switch_set_state(&hdmires_switch_data, 0);
	}
}
#endif



/* HDMI Driver state callback function */
void hdmi_state_callback(enum HDMI_STATE state)
{

	pr_info("[hdmi]%s, state = %d\n", __func__, state);

	if (p->is_force_disable == true)
		HDMI_LOG("hdmi is force disable(%d), return directly\n", p->is_force_disable);

	if (IS_HDMI_FAKE_PLUG_IN())
		HDMI_LOG("hdmi is fake in , return directly\n");

	if (p->is_force_disable == true) {
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}

	if (IS_HDMI_FAKE_PLUG_IN()) {
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}

	switch (state) {
	case HDMI_STATE_NO_DEVICE:
		{
			hdmi_suspend();
			switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
			switch_set_state(&hdmires_switch_data, 0);
			/* smi_dynamic_adj_hint_mhl(0); */
			break;
		}

	case HDMI_STATE_ACTIVE:
		{
			hdmi_resume();
			switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
			hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;
			break;
		}

	default:
		{
			pr_err("[hdmi]%s, state not support\n", __func__);
			break;
		}
	}

	/* return; */
}

#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)
void hdmi_cec_state_callback(enum HDMI_CEC_STATE state)
{
	pr_info("[hdmi]%s, cec_state = %d\n", __func__, state);
	switch_set_state(&hdmi_cec_switch_data, 0xff);
	switch (state) {
	case HDMI_CEC_STATE_PLUG_OUT:
		switch_set_state(&hdmi_cec_switch_data, HDMI_CEC_STATE_PLUG_OUT);
		break;
	case HDMI_CEC_STATE_GET_PA:
		switch_set_state(&hdmi_cec_switch_data, HDMI_CEC_STATE_GET_PA);
		break;
	case HDMI_CEC_STATE_TX_STS:
		switch_set_state(&hdmi_cec_switch_data, HDMI_CEC_STATE_TX_STS);
		break;
	case HDMI_CEC_STATE_GET_CMD:
		switch_set_state(&hdmi_cec_switch_data, HDMI_CEC_STATE_GET_CMD);
		break;
	default:
		pr_err("[hdmi]%s, cec_state not support\n", __func__);
		break;
	}
}
#endif

/*static*/ void hdmi_power_on(void)
{
	HDMI_FUNC();

	if (IS_HDMI_NOT_OFF()) {
		HDMI_LOG("hdmi power state(%d) is not power off,return directly\n",
			 atomic_read(&p->state));
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}

	if (down_interruptible(&hdmi_update_mutex)) {
		pr_err("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
		return;
	}

	/* Why we set power state before calling hdmi_drv->power_on()? */
	/* Because when power on, the hpd irq will come immediately, */
	/* that means hdmi_resume will be called before hdmi_drv->power_on() retuen here. */
	/* So we have to ensure the power state is STANDBY before hdmi_resume() be called. */
	SET_HDMI_STANDBY();
	HDMI_LOG("hdmi set power state(%d) standby\n", atomic_read(&p->state));

	/* enable/disable pll must be couple */
	/* hdmi_dpi_power_switch(true); */
#ifdef CONFIG_MTK_IOMMU
	HDMI_LOG("iommu Enable larb0\n");
	mtk_smi_larb_get(&disp_dev.psmidev->dev);
#else
	HDMI_LOG("Enable disp power domain, smi larb0/common\n");
	mtk_smi_larb_clock_on(0, 1);
#endif
	clk_prepare_enable(disp_dev.clk_map[DISP_REG_MUTEX][0]);
	clk_prepare_enable(disp_dev.clk_map[DISP_REG_RDMA1][0]);

	hdmi_drv->power_on();

	/* When camera is open, the state will only be changed when camera exits. */
	/* So we bypass state_reset here, if camera is open. */
	/* The related scenario is: suspend in camera with hdmi enabled. */
	/* Why need state_reset() here? */
	/* When we suspend the phone, and then plug out hdmi cable, the hdmi chip status will change immediately */
	/* But when we resume the phone and check hdmi status, the irq will never come again */
	/* So we have to reset hdmi state manually, to ensure the status is the same between the host and hdmi chip. */

#if ((!defined(MTK_MT8193_HDMI_SUPPORT)) &&\
	(!defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)) &&\
	(!defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT))) ||\
	(defined(MTK_ALPS_BOX_SUPPORT))
	if (p->is_force_disable == false) {
		if (IS_HDMI_FAKE_PLUG_IN()) {
			/* FixMe, deadlock may happened here, due to recursive use mutex */
			hdmi_resume();
			msleep(1000);
			switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
			hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;
		} else {
			hdmi_state_reset();

			HDMI_LOG("hdmi state %d\n", hdmi_drv->get_state());
			/* this is just a ugly workaround for some tv sets... */
			/* if(hdmi_drv->get_state() == HDMI_STATE_ACTIVE) */
			/* hdmi_resume(); */
		}
	}
#endif

	up(&hdmi_update_mutex);

	/* return; */
}

/*static*/ void hdmi_power_off(void)
{
	hdmi_video_buffer_info temp;

	HDMI_FUNC();

	switch_set_state(&hdmires_switch_data, 0);

	if (IS_HDMI_OFF()) {
		HDMI_LOG("hdmi power state(%d) is already power off,return directly\n",
			 atomic_read(&p->state));
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}

	if (down_interruptible(&hdmi_update_mutex)) {
		pr_err("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
		return;
	}

	hdmi_drv->power_off();
	/* hdmi_dpi_power_switch(false); */
	SET_HDMI_OFF();

	memset(&temp, 0, sizeof(temp));
	hdmi_rdma_address_config(false, temp);
	clk_disable_unprepare(disp_dev.clk_map[DISP_REG_RDMA1][0]);
	clk_disable_unprepare(disp_dev.clk_map[DISP_REG_MUTEX][0]);
#ifdef CONFIG_MTK_IOMMU
	HDMI_LOG("iommu Disable larb0\n");
	mtk_smi_larb_put(&disp_dev.psmidev->dev);
#else
	mtk_smi_larb_clock_off(0, 1);
	HDMI_LOG("disable smi larb0/common, disp power domain\n");
#endif
	HDMI_LOG("hdmi set power state(%d) off\n", atomic_read(&p->state));

	up(&hdmi_update_mutex);
	/* return; */
}

/*static*/ void hdmi_suspend(void)
{
	hdmi_video_buffer_info temp;

	HDMI_FUNC();

	if (IS_HDMI_NOT_ON()) {
		HDMI_LOG("hdmi power state is not power on,return directly\n");
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}

	if (hdmi_bufferdump_on > 0)
		MMProfileLogEx(HDMI_MMP_Events.HDMIState, MMProfileFlagStart, Plugout, 0);

	if (down_interruptible(&hdmi_update_mutex)) {
		pr_err("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
		return;
	}

	hdmi_dpi_power_switch(false);
	memset(&temp, 0, sizeof(temp));
	hdmi_rdma_address_config(false, temp);

	hdmi_drv->suspend();
	SET_HDMI_STANDBY();

	disp_module_clock_off(DISP_MODULE_RDMA1, "HDMI");
	up(&hdmi_update_mutex);

	if (hdmi_bufferdump_on > 0)
		MMProfileLogEx(HDMI_MMP_Events.HDMIState, MMProfileFlagEnd, Plugout, 0);
}

/*static*/ void hdmi_resume(void)
{
	HDMI_FUNC();

	if (IS_HDMI_NOT_STANDBY()) {
		HDMI_LOG("hdmi power state(%d) is not standby,return directly\n",
			 atomic_read(&p->state));
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}

	SET_HDMI_ON();

	HDMI_LOG("hdmi set power state(%d)\n", atomic_read(&p->state));

	if (hdmi_bufferdump_on > 0)
		MMProfileLogEx(HDMI_MMP_Events.HDMIState, MMProfileFlagStart, Plugin, 0);

	if (down_interruptible(&hdmi_update_mutex)) {
		pr_err("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
		return;
	}

	disp_module_clock_on(DISP_MODULE_RDMA1, "HDMI");

	hdmi_dpi_power_switch(true);
	hdmi_drv->resume();
	up(&hdmi_update_mutex);

	if (hdmi_bufferdump_on > 0)
		MMProfileLogEx(HDMI_MMP_Events.HDMIState, MMProfileFlagEnd, Plugin, 0);
}

/* Set HDMI orientation, will be called in mtkfb_ioctl(SET_ORIENTATION) */
/*static*/ void hdmi_setorientation(int orientation)
{
	HDMI_FUNC();
	/* /RET_VOID_IF(!p->is_enabled); */

	if (down_interruptible(&hdmi_update_mutex)) {
		pr_err("[hdmi][HDMI] can't get semaphore in %s\n", __func__);
		return;
	}

	p->orientation = orientation;
	p->is_reconfig_needed = true;

	/* done: */
	up(&hdmi_update_mutex);
}

static int hdmi_release(struct inode *inode, struct file *file)
{
	HDMI_FUNC();
	return 0;
}

static int hdmi_open(struct inode *inode, struct file *file)
{
	HDMI_FUNC();
	return 0;
}

static bool hdmi_drv_init_context(void);

static void dpi_setting_res(u8 arg)
{
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	fg3DFrame = FALSE;
	fgInterlace = FALSE;
#endif

	switch (arg) {
	case HDMI_VIDEO_720x480p_60Hz:
		{

			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_RISING;
			vsync_pol = HDMI_POLARITY_RISING;

			dpi_clk_div = 2;

			hsync_pulse_width = 62;
			hsync_back_porch = 60;
			hsync_front_porch = 16;

			vsync_pulse_width = 6;
			vsync_back_porch = 30;
			vsync_front_porch = 9;

			p->hdmi_width = 720;
			p->hdmi_height = 480;
			p->output_video_resolution = HDMI_VIDEO_720x480p_60Hz;
			break;
		}
#if defined(MTK_MT8193_HDMI_SUPPORT) ||\
	defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) ||\
	defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	case HDMI_VIDEO_720x576p_50Hz:
		{

			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_RISING;
			vsync_pol = HDMI_POLARITY_RISING;

			dpi_clk_div = 2;

			hsync_pulse_width = 64;
			hsync_back_porch = 68;
			hsync_front_porch = 12;

			vsync_pulse_width = 5;
			vsync_back_porch = 39;
			vsync_front_porch = 5;

			p->hdmi_width = 720;
			p->hdmi_height = 576;
			p->output_video_resolution = HDMI_VIDEO_720x576p_50Hz;
			break;
		}
#endif
	case HDMI_VIDEO_1280x720p_60Hz:
		{

			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
#if defined(HDMI_TDA19989) ||\
	defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) ||\
	defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
			hsync_pol = HDMI_POLARITY_FALLING;
#else
			hsync_pol = HDMI_POLARITY_RISING;
#endif
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
			vsync_pol = HDMI_POLARITY_FALLING;
#else
			vsync_pol = HDMI_POLARITY_RISING;
#endif

			dpi_clk_div = 2;

			hsync_pulse_width = 40;
			hsync_back_porch = 220;
			hsync_front_porch = 110;

			vsync_pulse_width = 5;
			vsync_back_porch = 20;
			vsync_front_porch = 5;

			p->hdmi_width = 1280;
			p->hdmi_height = 720;
			p->output_video_resolution = HDMI_VIDEO_1280x720p_60Hz;
			break;
		}
#if defined(MTK_MT8193_HDMI_SUPPORT) ||\
	defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) ||\
	defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	case HDMI_VIDEO_1280x720p_50Hz:
		{

			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#else
			hsync_pol = HDMI_POLARITY_RISING;
			vsync_pol = HDMI_POLARITY_RISING;
#endif

			dpi_clk_div = 2;

			hsync_pulse_width = 40;
			hsync_back_porch = 220;
			hsync_front_porch = 440;

			vsync_pulse_width = 5;
			vsync_back_porch = 20;
			vsync_front_porch = 5;

			p->hdmi_width = 1280;
			p->hdmi_height = 720;
			p->output_video_resolution = HDMI_VIDEO_1280x720p_50Hz;
			break;
		}
	case HDMI_VIDEO_1920x1080p_24Hz:
		{

			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#else
			hsync_pol = HDMI_POLARITY_RISING;
			vsync_pol = HDMI_POLARITY_RISING;
#endif

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 638;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080p_24Hz;
			break;
		}
	case HDMI_VIDEO_1920x1080p_25Hz:
		{

			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#else
			hsync_pol = HDMI_POLARITY_RISING;
			vsync_pol = HDMI_POLARITY_RISING;
#endif

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 528;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080p_25Hz;
			break;
		}
#endif
	case HDMI_VIDEO_1920x1080p_30Hz:
		{

			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#else
			hsync_pol = HDMI_POLARITY_RISING;
			vsync_pol = HDMI_POLARITY_RISING;
#endif

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 88;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080p_30Hz;
			break;
		}
#if defined(MTK_MT8193_HDMI_SUPPORT) ||\
	defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) ||\
	defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	case HDMI_VIDEO_1920x1080p_29Hz:
		{

			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#else
			hsync_pol = HDMI_POLARITY_RISING;
			vsync_pol = HDMI_POLARITY_RISING;
#endif

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 88;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080p_29Hz;
			break;
		}
	case HDMI_VIDEO_1920x1080p_23Hz:
		{

			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;
#else
			hsync_pol = HDMI_POLARITY_RISING;
			vsync_pol = HDMI_POLARITY_RISING;
#endif

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 638;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080p_23Hz;
			break;
		}
#endif

#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	case HDMI_VIDEO_1920x1080p_60Hz:
		{

			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 88;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080p_60Hz;
			break;
		}
	case HDMI_VIDEO_1920x1080p_50Hz:
		{

			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 528;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080p_50Hz;
			break;
		}
	case HDMI_VIDEO_1920x1080i_60Hz:
		{
			fgInterlace = TRUE;
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 88;

			vsync_pulse_width = 5;
			vsync_back_porch = 15;
			vsync_front_porch = 2;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080i_60Hz;
			break;
		}
#if defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	case HDMI_VIDEO_1920x1080i3d_sbs_60Hz:
		{
			fgInterlace = TRUE;
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 88;

			vsync_pulse_width = 5;
			vsync_back_porch = 15;
			vsync_front_porch = 2;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080i3d_sbs_60Hz;
			break;
		}
#endif
	case HDMI_VIDEO_1920x1080i_50Hz:
		{
			fgInterlace = TRUE;
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 528;

			vsync_pulse_width = 5;
			vsync_back_porch = 15;
			vsync_front_porch = 2;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080i_50Hz;
			break;
		}
#if defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	case HDMI_VIDEO_1920x1080i3d_sbs_50Hz:
		{
			fgInterlace = TRUE;
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;

			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 528;

			vsync_pulse_width = 5;
			vsync_back_porch = 15;
			vsync_front_porch = 2;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080i3d_sbs_50Hz;
			break;
		}
#endif
	case HDMI_VIDEO_1920x1080i3d_60Hz:
		{
			fgInterlace = TRUE;
			fg3DFrame = TRUE;
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;

			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;


			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 88;

			vsync_pulse_width = 5;
			vsync_back_porch = 15;
			vsync_front_porch = 2;

			p->hdmi_width = 1920;
			p->hdmi_height = 540;
			p->output_video_resolution = HDMI_VIDEO_1920x1080i3d_60Hz;
			break;
		}

	case HDMI_VIDEO_1920x1080i3d_50Hz:
		{
			fgInterlace = TRUE;
			fg3DFrame = TRUE;
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;

			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;


			dpi_clk_div = 2;

			hsync_pulse_width = 168;
			hsync_back_porch = 184;
			hsync_front_porch = 32;

			vsync_pulse_width = 5;
			vsync_back_porch = 57;
			vsync_front_porch = 23;

			p->hdmi_width = 1920;
			p->hdmi_height = 540;
			p->output_video_resolution = HDMI_VIDEO_1920x1080i3d_50Hz;
			break;
		}
	case HDMI_VIDEO_1280x720p3d_60Hz:
		{
			fg3DFrame = TRUE;
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;

			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;


			dpi_clk_div = 2;

			hsync_pulse_width = 40;
			hsync_back_porch = 220;
			hsync_front_porch = 110;

			vsync_pulse_width = 5;
			vsync_back_porch = 20;
			vsync_front_porch = 5;

			p->hdmi_width = 1280;
			p->hdmi_height = 720;
			p->output_video_resolution = HDMI_VIDEO_1280x720p3d_60Hz;
			break;
		}
	case HDMI_VIDEO_1280x720p3d_50Hz:
		{
			fg3DFrame = TRUE;
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;


			dpi_clk_div = 2;

			hsync_pulse_width = 40;
			hsync_back_porch = 220;
			hsync_front_porch = 440;

			vsync_pulse_width = 5;
			vsync_back_porch = 20;
			vsync_front_porch = 5;

			p->hdmi_width = 1280;
			p->hdmi_height = 720;
			p->output_video_resolution = HDMI_VIDEO_1280x720p3d_50Hz;
			break;
		}
	case HDMI_VIDEO_1920x1080p3d_24Hz:
		{
			fg3DFrame = TRUE;
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;


			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 638;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080p3d_24Hz;
			break;
		}
	case HDMI_VIDEO_1920x1080p3d_23Hz:
		{
			fg3DFrame = TRUE;
			clk_pol = HDMI_POLARITY_FALLING;
			de_pol = HDMI_POLARITY_RISING;
			hsync_pol = HDMI_POLARITY_FALLING;
			vsync_pol = HDMI_POLARITY_FALLING;


			dpi_clk_div = 2;

			hsync_pulse_width = 44;
			hsync_back_porch = 148;
			hsync_front_porch = 638;

			vsync_pulse_width = 5;
			vsync_back_porch = 36;
			vsync_front_porch = 4;

			p->hdmi_width = 1920;
			p->hdmi_height = 1080;
			p->output_video_resolution = HDMI_VIDEO_1920x1080p3d_23Hz;
			break;
		}
#endif

	default:
		break;
	}
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	hdmi_res = p->output_video_resolution;
#endif
}

void MTK_HDMI_Set_Security_Output(int enable)
{
	return;
#if 0
	if (!p->is_enabled) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}

	if (IS_HDMI_OFF()) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}

	if (enable) {
		if (hdmi_drv->get_state() == HDMI_STATE_ACTIVE) {
			hdmi_resume();
			msleep(1000);
			switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
		}
	} else {
		if (hdmi_drv->get_state() == HDMI_STATE_ACTIVE) {
			hdmi_suspend();
			switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
			switch_set_state(&hdmires_switch_data, 0);
		}
	}
#endif
}
#ifdef CONFIG_MTK_INTERNAL_MHL_SUPPORT
static void hdmi_wait_on(void)
{
	unsigned int tmp = 0;

	if (get_boot_mode() == FACTORY_BOOT) {
		for (tmp = 0; tmp < 60; tmp++) {
			msleep(100);
			if (IS_HDMI_ON())
				return;
		}
	}
}
#endif

int hdmi_get_support_info(void)
{
	int value = 0, temp = 0;

#ifdef USING_SCALE_ADJUSTMENT
	value |= HDMI_SCALE_ADJUSTMENT_SUPPORT;
#endif

#ifdef MHL_PHONE_GPIO_REUSAGE
	value |= HDMI_PHONE_GPIO_REUSAGE;
#endif

#ifdef MTK_AUDIO_MULTI_CHANNEL_SUPPORT
	temp = hdmi_drv->getcapablity();
#else
	temp = 0x2 << 3;
#endif

	value |= temp;

	HDMI_LOG("value is 0x%x\n", value);
	return value;
}


int hdmi_get_capability(void *info)
{
	int ret = 0;
	int query_type = 0;

	query_type = hdmi_get_support_info();

	if (copy_to_user(info, &query_type, sizeof(query_type))) {
		HDMI_LOG(": copy_to_user error! line:%d\n", __LINE__);
		ret = -EFAULT;
	}
	HDMI_LOG("[hdmi][HDMI] query_type  done %x\n", query_type);

	return ret;
}

static long hdmi_ioctl_ex(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	int r = 0;
#if defined(MTK_MT8193_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)
	hdmi_device_write w_info;
#if (defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT) && \
	defined(CONFIG_MTK_HDMI_HDCP_SUPPORT) && \
	defined(CONFIG_MTK_DRM_KEY_MNG_SUPPORT))
	hdmi_hdcp_drmkey key;
#else
	hdmi_hdcp_key key;
#endif
	send_slt_data send_sltdata;
	struct CEC_SLT_DATA get_sltdata;
	hdmi_para_setting data_info;
	HDMI_EDID_T pv_get_info;
	CEC_FRAME_DESCRIPTION_IO cec_frame;
	CEC_ADDRESS_IO cecaddr;
	struct CEC_DRV_ADDR_CFG_T cecsetAddr;
	struct CEC_SEND_MSG_T cecsendframe;
	READ_REG_VALUE regval;

#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)
	CEC_USR_CMD_T cec_usr_cmd;
	struct CEC_ACK_INFO_T cec_tx_status;
#endif
#endif
#ifdef CONFIG_MTK_INTERNAL_MHL_SUPPORT
	RW_VALUE stSpi;
	unsigned int addr, u4data;
	unsigned int tmp;
	stMhlCmd_st stMhlCmd;
	HDMI_EDID_T pv_get_info;
	unsigned char pdata[16];
	MHL_3D_INFO_T pv_3d_info;
	hdmi_para_setting data_info;
#if (defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT) &&\
	defined(CONFIG_MTK_HDMI_HDCP_SUPPORT) &&\
	defined(CONFIG_MTK_DRM_KEY_MNG_SUPPORT))
	hdmi_hdcp_drmkey key;
#else
	hdmi_hdcp_key key;
#endif

#endif
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	HDMITX_AUDIO_PARA audio_para;
#endif
	HDMI_LOG("[HDMI] hdmi ioctl= %s(%d)(%d), arg = %lu\n", _hdmi_ioctl_spy(cmd), cmd, cmd & 0xff, arg);

	switch (cmd) {
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	case MTK_HDMI_AUDIO_SETTING:
		{
			if (copy_from_user(&audio_para, (void __user *)arg, sizeof(audio_para))) {
				HDMI_LOG("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else {
				if (down_interruptible(&hdmi_update_mutex)) {
					HDMI_LOG("[HDMI] can't get semaphore in\n");
					return -EAGAIN;
				}
				hdmi_drv->audiosetting(&audio_para);
				up(&hdmi_update_mutex);
			}
			break;
		}
#endif
#if defined(MTK_MT8193_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)
	case MTK_HDMI_WRITE_DEV:
		{
			if (copy_from_user(&w_info, (void __user *)arg, sizeof(w_info))) {
				HDMI_LOG("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else
				hdmi_drv->write(w_info.u4Addr, w_info.u4Data);

			break;
		}

	case MTK_HDMI_INFOFRAME_SETTING:
		{
			if (copy_from_user(&data_info, (void __user *)arg, sizeof(data_info))) {
				HDMI_LOG("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else {
				hdmi_drv->InfoframeSetting(data_info.u4Data1 & 0xFF,
							   data_info.u4Data2 & 0xFF);
			}
			break;
		}

	case MTK_HDMI_HDCP_KEY:
		{
			if (copy_from_user(&key, (void __user *)arg, sizeof(key))) {
				HDMI_LOG("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else
				hdmi_drv->hdcpkey((uint8_t *) &key);

			break;
		}

	case MTK_HDMI_SETLA:
		{
			if (copy_from_user(&cecsetAddr, (void __user *)arg, sizeof(cecsetAddr))) {
				HDMI_LOG("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else
				hdmi_drv->setcecla(&cecsetAddr);

			break;
		}

	case MTK_HDMI_SENDSLTDATA:
		{
			if (copy_from_user(&send_sltdata, (void __user *)arg, sizeof(send_sltdata))) {
				HDMI_LOG("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else
				hdmi_drv->sendsltdata((uint8_t *) &send_sltdata);

			break;
		}

	case MTK_HDMI_SET_CECCMD:
		{
			if (copy_from_user(&cecsendframe, (void __user *)arg, sizeof(cecsendframe))) {
				HDMI_LOG("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else
				hdmi_drv->setceccmd(&cecsendframe);

			break;
		}

	case MTK_HDMI_CEC_ENABLE:
		{
			hdmi_drv->cecenable(arg & 0xFF);
			break;
		}


	case MTK_HDMI_GET_EDID:
		{
			hdmi_drv->getedid(&pv_get_info);
			if (copy_to_user((void __user *)arg, &pv_get_info, sizeof(pv_get_info))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}

	case MTK_HDMI_GET_CECCMD:
		{
			hdmi_drv->getceccmd(&cec_frame);
			if (copy_to_user((void __user *)arg, &cec_frame, sizeof(cec_frame))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}

	case MTK_HDMI_GET_CECSTS:
		{
			hdmi_drv->getcectxstatus(&cec_tx_status);
			if (copy_to_user
			    ((void __user *)arg, &cec_tx_status, sizeof(struct CEC_ACK_INFO_T))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}


	case MTK_HDMI_GET_CAPABILITY:
		{
			r = hdmi_get_capability(argp);
			break;
		}

	case MTK_HDMI_CEC_USR_CMD:
		{
			if (copy_from_user(&cec_usr_cmd, (void __user *)arg, sizeof(CEC_USR_CMD_T)))
				r = -EFAULT;
			else
				hdmi_drv->cecusrcmd(cec_usr_cmd.cmd, &(cec_usr_cmd.result));

			if (copy_to_user((void __user *)arg, &cec_usr_cmd, sizeof(CEC_USR_CMD_T))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}

	case MTK_HDMI_GET_SLTDATA:
		{
			hdmi_drv->getsltdata(&get_sltdata);
			if (copy_to_user((void __user *)arg, &get_sltdata, sizeof(get_sltdata))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}

	case MTK_HDMI_GET_CECADDR:
		{
			hdmi_drv->getcecaddr(&cecaddr);
			if (copy_to_user((void __user *)arg, &cecaddr, sizeof(cecaddr))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}

	case MTK_HDMI_COLOR_DEEP:
		{
			if (copy_from_user(&data_info, (void __user *)arg, sizeof(data_info))) {
				HDMI_LOG("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else {

				HDMI_LOG("MTK_HDMI_COLOR_DEEP: %d %d\n", data_info.u4Data1,
					 data_info.u4Data2);

				hdmi_drv->colordeep(data_info.u4Data1 & 0xFF,
						    data_info.u4Data2 & 0xFF);
			}
#if (defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT) || defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT))
			hdmi_colorspace = (unsigned char)data_info.u4Data1;
			DPI_CHECK_RET(HDMI_DPI(_Config_ColorSpace) (hdmi_colorspace, hdmi_res));
#endif
			break;
		}

	case MTK_HDMI_READ_DEV:
		{
			if (copy_from_user(&regval, (void __user *)arg, sizeof(regval))) {
				HDMI_LOG("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else
				hdmi_drv->read(regval.u1address, &regval.pu1Data);

			if (copy_to_user((void __user *)arg, &regval, sizeof(regval))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}

	case MTK_HDMI_ENABLE_LOG:
		{
			hdmi_drv->log_enable(arg & 0xFFFF);
			break;
		}

	case MTK_HDMI_ENABLE_HDCP:
		{
			hdmi_drv->enablehdcp(arg & 0xFFFF);
			break;
		}

	case MTK_HDMI_CECRX_MODE:
		{
			hdmi_drv->setcecrxmode(arg & 0xFFFF);
			break;
		}

	case MTK_HDMI_STATUS:
		{
			hdmi_drv->hdmistatus();
			break;
		}

	case MTK_HDMI_CHECK_EDID:
		{
			hdmi_drv->checkedid(arg & 0xFF);
			break;
		}

#elif defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
	case MTK_HDMI_READ:
		{
			if (copy_from_user(&stSpi, (void __user *)arg, sizeof(RW_VALUE)))
				r = -EFAULT;
			else
				hdmi_drv->read(stSpi.u4Addr, &(stSpi.u4Data));

			if (copy_to_user((void __user *)arg, &stSpi, sizeof(RW_VALUE))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}
	case MTK_HDMI_WRITE:
		{
			if (copy_from_user(&stSpi, (void __user *)arg, sizeof(RW_VALUE)))
				r = -EFAULT;
			else
				hdmi_drv->write(stSpi.u4Addr, stSpi.u4Data);
			break;
		}
	case MTK_HDMI_DUMP:
		{
			if (copy_from_user(&stSpi, (void __user *)arg, sizeof(RW_VALUE)))
				r = -EFAULT;
			break;
		}
	case MTK_HDMI_STATUS:
		{
			hdmi_drv->hdmistatus();
			break;
		}
	case MTK_HDMI_DUMP6397:
		{
			hdmi_drv->dump6397();
			break;
		}
	case MTK_HDMI_DUMP6397_W:
		{
			if (copy_from_user(&stSpi, (void __user *)arg, sizeof(RW_VALUE)))
				r = -EFAULT;
			else
				hdmi_drv->write6397(stSpi.u4Addr, stSpi.u4Data);
			break;
		}
	case MTK_HDMI_DUMP6397_R:
		{
			if (copy_from_user(&stSpi, (void __user *)arg, sizeof(RW_VALUE)))
				r = -EFAULT;
			else
				hdmi_drv->read6397(stSpi.u4Addr, &(stSpi.u4Data));

			if (copy_to_user((void __user *)arg, &stSpi, sizeof(RW_VALUE))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}

			break;
		}
	case MTK_HDMI_CBUS_STATUS:
		{
			hdmi_drv->cbusstatus();
			break;
		}
	case MTK_HDMI_CMD:
		{
			pr_info("MTK_HDMI_CMD\n");
			if (copy_from_user(&stMhlCmd, (void __user *)arg, sizeof(stMhlCmd_st))) {
				pr_err("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			pr_info("[MHL]cmd=%x%x%x%x\n", stMhlCmd.u4Cmd, stMhlCmd.u4Para,
				stMhlCmd.u4Para1, stMhlCmd.u4Para2);
			hdmi_drv->mhl_cmd(stMhlCmd.u4Cmd, stMhlCmd.u4Para, stMhlCmd.u4Para1,
					  stMhlCmd.u4Para2);
			break;
		}
	case MTK_HDMI_HDCP:
		{
			if (arg)
				hdmi_drv->enablehdcp(3);
			else
				hdmi_drv->enablehdcp(0);

			break;
		}
	case MTK_HDMI_HDCP_KEY:
		{
			if (copy_from_user(&key, (void __user *)arg, sizeof(key))) {
				HDMI_LOG("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else {
				hdmi_drv->hdcpkey((uint8_t *) &key);
			}
			break;
		}
	case MTK_HDMI_CONNECT_STATUS:
		{
			tmp = hdmi_drv->get_state();
			if (copy_to_user((void __user *)arg, &tmp, sizeof(unsigned int))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}
	case MTK_HDMI_GET_EDID:
		{
			hdmi_drv->getedid(&pv_get_info);
			if (copy_to_user((void __user *)arg, &pv_get_info, sizeof(pv_get_info))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}
	case MTK_MHL_GET_DCAP:
		{
			hdmi_drv->getdcapdata(pdata);
			if (copy_to_user((void __user *)arg, pdata, sizeof(pdata))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}
	case MTK_MHL_GET_3DINFO:
		{
			hdmi_drv->get3dinfo(&pv_3d_info);
			if (copy_to_user((void __user *)arg, &pv_3d_info, sizeof(pv_3d_info))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			break;
		}
	case MTK_HDMI_COLOR_DEEP:
		{
			if (copy_from_user(&data_info, (void __user *)arg, sizeof(data_info))) {
				HDMI_LOG("copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else {
				HDMI_LOG("MTK_HDMI_COLOR_DEEP: %d %d %d\n",
					 data_info.u4Data1, data_info.u4Data2, hdmi_res);


				hdmi_colorspace = (unsigned char)(data_info.u4Data1 & 0xFF);
				if ((hdmi_res == HDMI_VIDEO_1920x1080p_60Hz)
				    || (hdmi_res == HDMI_VIDEO_1920x1080p_50Hz)
				    || (hdmi_res == HDMI_VIDEO_1280x720p3d_60Hz)
				    || (hdmi_res == HDMI_VIDEO_1280x720p3d_50Hz)
				    || (hdmi_res == HDMI_VIDEO_1920x1080p3d_24Hz)
				    || (hdmi_res == HDMI_VIDEO_1920x1080p3d_23Hz))
					hdmi_colorspace = HDMI_YCBCR_422;

				hdmi_drv->colordeep(hdmi_colorspace);
			}
			DPI_CHECK_RET(HDMI_DPI(_Config_ColorSpace)
				      (hdmi_colorspace, hdmi_res));
			break;
		}
#endif
	case MTK_HDMI_AUDIO_VIDEO_ENABLE:
		{
			hdmi_video_buffer_info temp;

			if (arg) {
				if (p->is_enabled) {
					HDMI_LOG("already enabled !\n");
					return 0;
				}

				HDMI_CHECK_RET(hdmi_drv_init());
				if (hdmi_drv->enter)
					hdmi_drv->enter();
				hdmi_power_on();
				p->is_enabled = true;
#ifdef CONFIG_MTK_INTERNAL_MHL_SUPPORT
				hdmi_wait_on();
#endif
				HDMI_LOG("set hdmi to enable !\n");

			} else {
				if (!p->is_enabled) {
					HDMI_LOG("already disabled !\n");
					return 0;
				}

				/* when disable hdmi, HPD is disabled */
				switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);

				/* wait hdmi finish update */
				if (down_interruptible(&hdmi_update_mutex)) {
					pr_err("[hdmi][HDMI] can't get semaphore in %s()\n",
					       __func__);
					return -EFAULT;
				}

				hdmi_rdma_address_config(false, temp);
				up(&hdmi_update_mutex);
				hdmi_power_off();

				/* wait hdmi finish update */
				if (down_interruptible(&hdmi_update_mutex)) {
					pr_err("[hdmi][HDMI] can't get semaphore in %s()\n",
					       __func__);
					return -EFAULT;
				}

				HDMI_CHECK_RET(hdmi_drv_deinit());
				p->is_enabled = false;
				up(&hdmi_update_mutex);

				if (hdmi_drv->exit)
					hdmi_drv->exit();
			}

			break;
		}

	case MTK_HDMI_FORCE_FULLSCREEN_ON:
		/* case MTK_HDMI_FORCE_CLOSE: */
		{
			if (p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (!p->is_enabled) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (IS_HDMI_OFF()) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (p->is_force_disable == true)
				break;

			if (IS_HDMI_FAKE_PLUG_IN()) {
				hdmi_suspend();
				switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
				switch_set_state(&hdmires_switch_data, 0);
			} else {
				if (hdmi_drv->get_state() == HDMI_STATE_ACTIVE) {
					hdmi_suspend();
					switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
					switch_set_state(&hdmires_switch_data, 0);
				}
			}

			p->is_force_disable = true;
			break;
		}

	case MTK_HDMI_FORCE_FULLSCREEN_OFF:
		/* case MTK_HDMI_FORCE_OPEN: */
		{
			if (p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (!p->is_enabled) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (IS_HDMI_OFF()) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (p->is_force_disable == false)
				break;

			if (IS_HDMI_FAKE_PLUG_IN()) {
				hdmi_resume();
				msleep(1000);
				switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
				hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;
			} else {
				if (hdmi_drv->get_state() == HDMI_STATE_ACTIVE) {
					hdmi_resume();
					msleep(1000);
					switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
					hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;
				}
			}

			p->is_force_disable = false;

			break;
		}

		/* Shutdown thread(No matter IPO), system suspend/resume will go this way... */
	case MTK_HDMI_POWER_ENABLE:
		{
			if (!p->is_enabled) {
				HDMI_LOG("HDMI not enabled yet!\n");
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (arg) {
				if (otg_enable_status) {
					HDMI_LOG("HDMI otg_enable_status!\n");
					HDMI_LOG("return in %d\n", __LINE__);
					return 0;
				}

				hdmi_power_on();
			} else {
				hdmi_power_off();
				switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
			}

			break;
		}

	case MTK_HDMI_USBOTG_STATUS:
		{
			HDMI_LOG("MTK_HDMI_USBOTG_STATUS, arg=%ld, enable %d\n", arg,
				 p->is_enabled);

			if (!p->is_enabled) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}
			if (hdmi_params->cabletype != MHL_CABLE) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (arg)
				otg_enable_status = true;
			else {
				otg_enable_status = false;
				if (!p->is_force_disable) {
					HDMI_LOG("return in %d\n", __LINE__);
					return 0;
				}
				hdmi_power_on();
			}

			break;
		}

	case MTK_HDMI_AUDIO_ENABLE:
		{
			if (!p->is_enabled) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}
			if (arg)
				HDMI_CHECK_RET(hdmi_audio_enable(true));
			else
				HDMI_CHECK_RET(hdmi_audio_enable(false));

			break;
		}

	case MTK_HDMI_VIDEO_ENABLE:
		{
			if (!p->is_enabled) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}
			break;
		}

	case MTK_HDMI_VIDEO_CONFIG:
		{
			hdmi_video_buffer_info temp;
			int tmp = 0;

			/*pr_err("video resolution configuration, arg=%ld, %ld\n", arg,
				 hdmi_reschange);*/

			if (!p->is_enabled)
				HDMI_LOG(" hdmi not enabled,change resolution\n");

			if (IS_HDMI_NOT_ON()) {
				HDMI_LOG
				    (" hdmi power state(%d) is not on,change resolution\n",
				     atomic_read(&p->state));
			}
			if (!p->is_enabled) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (IS_HDMI_NOT_ON()) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (hdmi_reschange == arg) {
				HDMI_LOG("hdmi_reschange=%ld\n", hdmi_reschange);
				break;
			}

			hdmi_reschange = arg;
			p->is_clock_on = false;
			atomic_set(&hdmi_rdma_update_event, 1);
			wake_up_interruptible(&hdmi_rdma_update_wq);

			while (1) {
				if ((list_empty(&HDMI_Buffer_List)) || (tmp > 15)) {
					if (tmp > 15)
						HDMI_LOG(" Error HDMI_Buffer_List is not empty\n");
					break;
				}
				msleep(2);

				tmp++;
			}

			if (!p->is_enabled)
				HDMI_LOG(" hdmi not enabled,change resolution fail\n");

			if (IS_HDMI_NOT_ON())
				HDMI_LOG(" hdmi power state is not on,change resolution\n");

			if (!p->is_enabled) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (IS_HDMI_NOT_ON()) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}

			if (hdmi_bufferdump_on > 0) {
				MMProfileLogEx(HDMI_MMP_Events.HDMIState,
					       MMProfileFlagStart, ResChange, arg);
			}

			if (down_interruptible(&hdmi_update_mutex)) {
				HDMI_LOG("[HDMI] can't get semaphore in\n");
				return -EFAULT;
			}
#if defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
			if ((arg == HDMI_VIDEO_1920x1080p_60Hz)
			    || (arg == HDMI_VIDEO_1920x1080p_50Hz)
			    || (arg == HDMI_VIDEO_1280x720p3d_60Hz)
			    || (arg == HDMI_VIDEO_1280x720p3d_50Hz)
			    || (arg == HDMI_VIDEO_1920x1080p3d_24Hz)
			    || (arg == HDMI_VIDEO_1920x1080p3d_23Hz)
			    ) {
				hdmi_colorspace = HDMI_YCBCR_422;
			} else
				hdmi_colorspace = HDMI_RGB;
			hdmi_drv->colordeep(hdmi_colorspace);
#endif

#if (defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) || defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT))
			if (hdmi_drv->tmdsonoff) {
				hdmi_mdelay(50);
				hdmi_drv->tmdsonoff(0);
			}
#endif
			hdmi_rdma_address_config(false, temp);

			hdmi_config_pll(arg);
			dpi_setting_res((u8) arg);
			hdmi_video_config(p->output_video_resolution,
					  HDMI_VIN_FORMAT_RGB888, HDMI_VOUT_FORMAT_RGB888);

			{
				unsigned int rdma_idx = 1;

				unsigned int line = p->hdmi_height * 4 / 5;

				if ((arg == HDMI_VIDEO_1920x1080i_60Hz)
				    || (arg == HDMI_VIDEO_1920x1080i_50Hz)
#if defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT)
				    || (arg == HDMI_VIDEO_1920x1080i3d_sbs_60Hz)
				    || (arg == HDMI_VIDEO_1920x1080i3d_sbs_50Hz)
#endif
				    ) {
					line /= 8;
				}

				pr_info
				    ("[hdmi] set rdma1 target line %d,hdmi_height %d,res %ld\n",
				     line, p->hdmi_height, arg);
				RDMASetTargetLine(rdma_idx, line);
			}

			DPI_CHECK_RET(HDMI_DPI(_DisableClk) ());
#if (!defined(CONFIG_MTK_INTERNAL_MHL_SUPPORT) && !defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT))
			DPI_CHECK_RET(HDMI_DPI(_ConfigHDMI) ());
#endif
			hdmi_dpi_config_update();
			DPI_CHECK_RET(HDMI_DPI(_EnableClk) ());

			up(&hdmi_update_mutex);

			p->is_clock_on = true;
			if (factory_mode == false)
				switch_set_state(&hdmires_switch_data, hdmi_reschange + 1);
			if (hdmi_bufferdump_on > 0) {
				MMProfileLogEx(HDMI_MMP_Events.HDMIState, MMProfileFlagEnd,
					       ResChange, hdmi_reschange + 1);
			}

			HDMI_LOG("video resolution (%ld) configuration done\n", hdmi_reschange + 1);

			break;
		}

	case MTK_HDMI_AUDIO_CONFIG:
		{
			if (!p->is_enabled) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}
			break;
		}

	case MTK_HDMI_IS_FORCE_AWAKE:
		{
			if (!hdmi_drv_init_context()) {
				pr_err("[hdmi]%s, hdmi_drv_init_context fail\n", __func__);
				return HDMI_STATUS_NOT_IMPLEMENTED;
			}

			HDMI_LOG(" MTK_HDMI_IS_FORCE_AWAKE,%d\n", hdmi_params->is_force_awake);


			r = copy_to_user(argp, &hdmi_params->is_force_awake,
					 sizeof(hdmi_params->is_force_awake)) ? -EFAULT : 0;
			break;
		}

#if 1

	case MTK_HDMI_ENTER_VIDEO_MODE:
		{
			if (!p->is_enabled) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}
			if (HDMI_OUTPUT_MODE_VIDEO_MODE != p->output_mode) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}
			/* FIXME */
			break;
		}

	case MTK_HDMI_REGISTER_VIDEO_BUFFER:
		{
			break;
		}

	case MTK_HDMI_POST_VIDEO_BUFFER:
		{
			hdmi_video_buffer_info video_buffer_info;
			hdmi_video_buffer_list *pBuffList = NULL;
			MMP_MetaDataBitmap_t Bitmap;
			struct sync_fence *fence = NULL;

			if ((p->is_enabled == false) || (p->is_clock_on == false)
			    || IS_HDMI_NOT_ON()) {
				HDMI_LOG("hdmi status is not ready, skip POST_VIDEO_BUFFER\n");
				return 0;
			}

			video_buffer_info.src_fmt = MTK_FB_FORMAT_ARGB8888;
			if (copy_from_user
			    (&video_buffer_info, (void __user *)argp, sizeof(video_buffer_info))) {
				MMProfileLogEx(HDMI_MMP_Events.ErrorInfo,
					       MMProfileFlagPulse, insert_Buffer_Err2,
					       insert_Buffer_Err2);
				pr_err("copy_from_user failed! line\n");
				r = -EFAULT;
				break;
			}

			if (hdmi_bufferdump_on > 0) {
				MMProfileLogEx(HDMI_MMP_Events.BufferPost,
					       MMProfileFlagStart, p->is_enabled, p->is_clock_on);
			}

			DBG_OnTriggerHDMI();
#ifdef MTK_OVERLAY_ENGINE_SUPPORT
			HDMI_LOG
			    ("POST Buffer Info: va:0x%p mva:0x%p fmt:0x%x, pitch:%d, ROI(%d,%d,%d,%d),",
				video_buffer_info.src_base_addr,
				video_buffer_info.src_phy_addr, video_buffer_info.src_fmt,
				video_buffer_info.src_pitch, video_buffer_info.src_offset_x,
				video_buffer_info.src_offset_y, video_buffer_info.src_width,
				video_buffer_info.src_height);
			HDMI_LOG
				(" nextidx:0x%x, ident:0x%x, connect:%d, sec:%d, fenceFd:0x%x\n",
				video_buffer_info.next_buff_idx,
				video_buffer_info.identity, video_buffer_info.connected_type,
				video_buffer_info.security, video_buffer_info.fenceFd);

			if (video_buffer_info.fenceFd >= 0)
				fence = sync_fence_fdget(video_buffer_info.fenceFd);

			if (p->is_clock_on) {
				pBuffList = kmalloc(sizeof(hdmi_video_buffer_list), GFP_KERNEL);
				memset(pBuffList, 0, sizeof(hdmi_video_buffer_list));

				pBuffList->buf_state = insert_new;
				pBuffList->fence = (int)NULL;
				memcpy(&pBuffList->buffer_info, &video_buffer_info,
				       sizeof(video_buffer_info));
				pBuffList->fence = (int)fence;
				HDMI_LOG("fence/fdget: %s\n", fence->name);

				if ((HDMI_VIDEO_1920x1080i_60Hz == p->output_video_resolution ||
					HDMI_VIDEO_1920x1080i_50Hz == p->output_video_resolution) &&
					pBuffList->fence) {
					if (sync_fence_wait((struct sync_fence *)pBuffList->fence, 1000) < 0) {
						pr_err("[HDMI] fence/wait: %s timeout",
								((struct sync_fence *)pBuffList->fence)->name);
						pBuffList->buf_state = buf_dropped;
					}
				}

				/* for debug -- colorbar */
				if (g_hdmi_pattern == 1) {
					#if 0
					memcpy((void *)fb_base_pa, video_buffer_info.src_phy_addr,
						video_buffer_info.src_height * video_buffer_info.src_height * 4,
						1280*800*4);
					#endif

					HDMI_LOG("HDMI post buffer using hdmi_pattern_mva= 0x%x\n",
						hdmi_pattern_mva);
					pBuffList->buffer_info.src_phy_addr = (void *)hdmi_pattern_mva;
					/*HDMI_LOG("HDMI post buffer using fb_base_pa= 0x%x\n", fb_base_pa);
					pBuffList->buffer_info.src_phy_addr = (void *)fb_base_pa;*/
				}
				/* for debug -- svp colorbar */
				else if (g_hdmi_pattern == 2) {
					pBuffList->buffer_info.src_phy_addr = (void *)hdmi_pattern_mva_sec;
					pBuffList->buffer_info.security = 1;
				}

				INIT_LIST_HEAD(&pBuffList->list);
				spin_lock_bh(&hdmi_lock);
				list_add_tail(&pBuffList->list, &HDMI_Buffer_List);
				spin_unlock_bh(&hdmi_lock);
				HDMI_LOG("[HDMI] ADD BUF[%d], secure = %d, addr = %X\n",
					 addbuf++, video_buffer_info.security,
					 (unsigned int)video_buffer_info.src_phy_addr);

				if (dp_mutex_dst < 0) {
					HDMI_LOG("HDMI wait up wake queue\n");
					atomic_set(&hdmi_rdma_config_event, 1);
					wake_up_interruptible(&hdmi_rdma_config_wq);
				}

				if (hdmi_bufferdump_on >= 0x7) {
					unsigned int mva_size, kernel_va, kernel_size;

					mva_size =
					    video_buffer_info.src_width *
					    video_buffer_info.src_height * 3;
					m4u_do_mva_map_kernel((unsigned int)video_buffer_info.src_phy_addr,
							mva_size, 0, &kernel_va, &kernel_size);

					HDMI_LOG
					    ("video_buffer_info kernel_va: 0x%x kernel_size: 0x%x\n",
					     kernel_va, kernel_size);

					Bitmap.data1 = video_buffer_info.src_width;
					Bitmap.data2 = video_buffer_info.src_pitch;
					Bitmap.width = video_buffer_info.src_width;
					Bitmap.height = video_buffer_info.src_height;
					Bitmap.format = MMProfileBitmapRGB888;
					Bitmap.start_pos = 0;
					Bitmap.pitch = video_buffer_info.src_pitch * 3;
					Bitmap.data_size = kernel_size;
					Bitmap.down_sample_x = 10;
					Bitmap.down_sample_y = 10;
					Bitmap.pData = (void *)kernel_va;
					Bitmap.bpp = 24;

					if (kernel_size != 0)
						MMProfileLogMetaBitmap(HDMI_MMP_Events.DDPKBitblt,
								       MMProfileFlagPulse, &Bitmap);

					m4u_do_mva_unmap_kernel((unsigned int)video_buffer_info.src_phy_addr,
								mva_size, kernel_va);
				}

			} else {
				if (fence)
					sync_fence_put(fence);
			}
#else
			spin_lock_bh(&hdmi_lock);

			if (p->is_clock_on) {
#ifdef MTK_HDMI_ION_SUPPORT
				pBuffList =
				(hdmi_video_buffer_list *)
				hdmi_query_buf_mva(video_buffer_info.next_buff_idx);

				if (pBuffList == NULL) {
					spin_unlock_bh(&hdmi_lock);
					MMProfileLogEx(HDMI_MMP_Events.ErrorInfo,
						       MMProfileFlagPulse, Buff_ION_Err1, 0);
					HDMI_LOG("Warning: buffer list no buffers!\n");
					r = -EFAULT;
					break;
				}
				memcpy(&(pBuffList->buffer_info),
				       &video_buffer_info, sizeof(video_buffer_info));

				if (pBuffList->hnd != 0) {
					if (video_buffer_info.src_phy_addr != NULL) {
						HDMI_LOG
						("Warning: ion enable, but phy is not null\n");
						MMProfileLogEx(HDMI_MMP_Events.ErrorInfo,
						 MMProfileFlagPulse, Buff_ION_Err1, 1);
					} else {
						HDMI_LOG
						("ion disable, bufflist %x, vir %x, phy %x\n",
						(unsigned int)pBuffList,
						(unsigned int)video_buffer_info.src_base_addr,
						(unsigned int)video_buffer_info.src_phy_addr);
					}

					video_buffer_info.src_phy_addr = (void *)pBuffList->mva;
					video_buffer_info.src_base_addr = (void *)pBuffList->va;
					pBuffList->buffer_info.src_phy_addr = (void *)pBuffList->mva;
					pBuffList->buffer_info.src_base_addr = (void *)pBuffList->va;
				}
#endif
				pBuffList->buf_state = insert_new;
				spin_unlock_bh(&hdmi_lock);

				if (dp_mutex_dst <= 0) {
					atomic_set(&hdmi_rdma_config_event, 1);
					wake_up_interruptible(&hdmi_rdma_config_wq);
				}

				if ((capture_screen == true) || (hdmi_bufferdump_on >= 0x7)) {
					Bitmap.data1 = video_buffer_info.src_width;
					Bitmap.data2 = video_buffer_info.src_pitch;
					Bitmap.width = video_buffer_info.src_width;
					Bitmap.height = video_buffer_info.src_height;
					Bitmap.format = MMProfileBitmapBGRA8888;
					Bitmap.start_pos =
					    (video_buffer_info.src_pitch -
					     video_buffer_info.src_width) / 2 * 4;
					Bitmap.pitch = video_buffer_info.src_pitch * 4;

					Bitmap.data_size = Bitmap.pitch * Bitmap.height;
					Bitmap.down_sample_x = 10;
					Bitmap.down_sample_y = 10;
					Bitmap.pData = video_buffer_info.src_base_addr;
					Bitmap.bpp = 32;
				}

				if (hdmi_bufferdump_on >= 0x7) {
					MMProfileLogMetaBitmap(HDMI_MMP_Events.DDPKBitblt,
							       MMProfileFlagPulse, &Bitmap);
				}

				if (capture_screen == true) {
					memcpy((void *)capture_addr,
					       video_buffer_info.src_base_addr, Bitmap.data_size);
					capture_screen = false;
				}
			} else {
				spin_unlock_bh(&hdmi_lock);
			}
#endif


#ifndef MTK_HDMI_FENCE_SUPPORT
			HDMI_LOG("wait hdmi_rdma_config_wq\n");
			wait_event_interruptible(hdmi_rdma_config_wq,
						 atomic_read(&hdmi_rdma_config_event));
			atomic_set(&hdmi_rdma_config_event, 0);
#else

			if (hdmi_bufferdump_on > 0) {
				MMProfileLogEx(HDMI_MMP_Events.BufferInsert,
					       MMProfileFlagPulse, (unsigned int)pBuffList,
					       video_buffer_info.next_buff_idx);
			}


			/* /up(&hdmi_update_mutex); */
			if (hdmi_bufferdump_on > 0) {
				if (pBuffList) {
					MMProfileLogEx(HDMI_MMP_Events.BufferPost,
						       MMProfileFlagEnd,
						       (unsigned int)pBuffList,
						       (unsigned int)pBuffList->buffer_info.src_phy_addr);
				} else {
					MMProfileLogEx(HDMI_MMP_Events.BufferPost,
						       MMProfileFlagEnd,
						       (unsigned int)pBuffList, 0);
				}
			}
#endif

			HDMI_LOG("MTK_HDMI_POST_VIDEO_BUFFER done\n");
			break;
		}

	case MTK_HDMI_LEAVE_VIDEO_MODE:
		{
			if (!p->is_enabled) {
				HDMI_LOG("return in %d\n", __LINE__);
				return 0;
			}
			break;
		}

#endif

	case MTK_HDMI_FACTORY_MODE_ENABLE:
		{
			if (argp == NULL)
				hdmi_drv->power_off();
			else if (hdmi_drv->power_on())
				HDMI_LOG("Error factory mode test fail in on\n");
			break;
		}

	case MTK_HDMI_FACTORY_GET_STATUS:
		{
			unsigned idx;
			int hdmi_status = p->is_clock_on ? true : false;

			/*check_res  0:HDMI Plug in; 1 HDMI Plug in,EDID OK;
			-1: No HDMI Plug in; -2: DPI Clock OFF; -3: HDMI Plug in, EDID Error */
			for (idx = 0; idx < 30; idx++) {
				msleep(100);
				if (IS_HDMI_ON()) {
					hdmi_status = 0;
					pr_err("[hdmi] IS_HDMI_ON\n");
					break;
				} else if (IS_HDMI_STANDBY()) {
					hdmi_status = -1;
					pr_err("[hdmi] IS_HDMI_STANDBY\n");
					break;
				}
			}

			if (IS_HDMI_ON() == false) {
				pr_err("[hdmi]no tv connected\n");
				hdmi_status = -1;
			} else if (p->is_clock_on == false) {
				pr_err("[hdmi]no tv connected\n");
				hdmi_status = -2;
			} else if (hdmi_drv->checkedidheader) {
				if (hdmi_drv->checkedidheader() == FALSE) {
					pr_err("[hdmi]edid error\n");
					hdmi_status = -3;
				} else {
					hdmi_status = 1;
				}
			}
			HDMI_LOG("MTK_HDMI_FACTORY_GET_STATUS is %d\n", hdmi_status);
			if (copy_to_user((void __user *)arg, &hdmi_status, sizeof(hdmi_status))) {
				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}

			break;
		}

	case MTK_HDMI_FACTORY_DPI_TEST:
		{
#if 1

			if (down_interruptible(&hdmi_update_mutex)) {
				HDMI_LOG("[HDMI] can't get semaphore in\n");
				return -EAGAIN;
			}

			hdmi_dpi_config_update();
			/* for build error, by mtk40021 */
			/* DPI_CHECK_RET(HDMI_DPI(_ConfigDualEdge(true))); */
			/* DPI_CHECK_RET(HDMI_DPI(_ConfigPixelClk(DPI_POLARITY_FALLING, 0, 0))); */

			/* for build error, by mtk40021 */
			DPI_CHECK_RET(HDMI_DPI(_EnableColorBar()));
			/* *(volatile unsigned int *)0xf4014f00 = 0x00000041; */
			DPI_CHECK_RET(HDMI_DPI(_EnableClk()));
			DPI_CHECK_RET(HDMI_DPI(_DumpRegisters()));
			hdmi_enablecolorbar();
			up(&hdmi_update_mutex);
#endif
			break;
		}

	case MTK_HDMI_GET_DEV_INFO:
		{
			int displayid = 0;
			mtk_dispif_info_t hdmi_info;

			if (hdmi_bufferdump_on > 0) {
				MMProfileLogEx(HDMI_MMP_Events.GetDevInfo,
					       MMProfileFlagStart, p->is_enabled, p->is_clock_on);
			}

			HDMI_LOG("DEV_INFO configuration get +\n");

			if (copy_from_user(&displayid, (void __user *)arg, sizeof(displayid))) {
				if (hdmi_bufferdump_on > 0) {
					MMProfileLogEx(HDMI_MMP_Events.GetDevInfo,
						       MMProfileFlagEnd, 0xff, 0xff1);
				}

				HDMI_LOG(": copy_from_user failed! line:%d\n", __LINE__);
				return -EAGAIN;
			}

			if (displayid != MTKFB_DISPIF_HDMI) {
				if (hdmi_bufferdump_on > 0) {
					MMProfileLogEx(HDMI_MMP_Events.GetDevInfo,
						       MMProfileFlagPulse, 0xff, 0xff2);
				}

				HDMI_LOG(": invalid display id:%d\n", displayid);
				/* /return -EAGAIN; */
			}

			memset(&hdmi_info, 0, sizeof(hdmi_info));
			hdmi_info.displayFormat = DISPIF_FORMAT_RGB888;
			hdmi_info.displayHeight = p->hdmi_height;
			hdmi_info.displayWidth = p->hdmi_width;
			hdmi_info.display_id = displayid;
			hdmi_info.isConnected = 1;
			hdmi_info.displayMode = DISPIF_MODE_COMMAND;

			if (hdmi_params->cabletype == MHL_SMB_CABLE)
				hdmi_info.displayType = HDMI_SMARTBOOK;
			else if (hdmi_params->cabletype == MHL_CABLE)
				hdmi_info.displayType = MHL;
			else
				hdmi_info.displayType = HDMI;

			hdmi_info.isHwVsyncAvailable = 1;
			hdmi_info.vsyncFPS = 60;

			if (copy_to_user((void __user *)arg, &hdmi_info, sizeof(hdmi_info))) {
				if (hdmi_bufferdump_on > 0) {
					MMProfileLogEx(HDMI_MMP_Events.GetDevInfo,
						       MMProfileFlagEnd, 0xff, 0xff2);
				}

				HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}

			if (hdmi_bufferdump_on > 0) {
				MMProfileLogEx(HDMI_MMP_Events.GetDevInfo, MMProfileFlagEnd,
					       p->is_enabled, hdmi_info.displayType);
			}

			HDMI_LOG("DEV_INFO configuration get displayType-%d\n",
				 hdmi_info.displayType);

			break;
		}

	case MTK_HDMI_PREPARE_BUFFER:
		{
			hdmi_buffer_info hdmi_buffer;
#ifndef MTK_OVERLAY_ENGINE_SUPPORT
			struct ion_handle *handle = NULL;
			hdmi_video_buffer_list *pBuffList = NULL;
			unsigned int mva = 0x0;
			unsigned int va = 0x0;
#endif

#ifdef MTK_HDMI_FENCE_SUPPORT
			struct fence_data data;

			if (hdmi_bufferdump_on > 0) {
				MMProfileLogEx(HDMI_MMP_Events.FenceCreate,
					       MMProfileFlagStart, 0, 0);
			}

			if (copy_from_user(&hdmi_buffer, (void __user *)arg, sizeof(hdmi_buffer))) {
				pr_err("[HDMI]: copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
				break;
			}

			HDMI_LOG("ioctl/prepare_buffer: ion_fd_%d fence_fd_%d index_%d\n",
					hdmi_buffer.ion_fd, hdmi_buffer.fence_fd, hdmi_buffer.index);

			if (down_interruptible(&hdmi_update_mutex)) {
				MMProfileLogEx(HDMI_MMP_Events.MutexErr, MMProfileFlagPulse,
					       Mutex_Err2, Mutex_Err2);
				pr_err("[HDMI] Warning can't get semaphore in\n");
				r = -EFAULT;
				break;
			}

			hdmi_buffer.fence_fd = MTK_HDMI_NO_FENCE_FD;
			if (p->is_clock_on) {
				data = hdmi_create_fence();
#ifdef MTK_OVERLAY_ENGINE_SUPPORT
				hdmi_buffer.fence_fd = data.fence;
				hdmi_buffer.index = data.value;
#else
				pBuffList = kmalloc(sizeof(hdmi_video_buffer_list), GFP_KERNEL);
				memset(pBuffList, 0, sizeof(hdmi_video_buffer_list));
#ifdef MTK_HDMI_ION_SUPPORT
				if (!ion_client) {
					hdmi_ion_init();

					if (!ion_client) {
						HDMI_LOG(": get ion_client fail (0x%x)\n",
							 (unsigned int)ion_client);
						r = -EFAULT;
						up(&hdmi_update_mutex);
						break;
					}
				}
				handle = hdmi_ion_import_handle(ion_client, hdmi_buffer.ion_fd);
				hdmi_ion_phys_mmu_addr(ion_client, handle, &mva);
				va = (unsigned int)ion_map_kernel(ion_client, handle);
#endif
				spin_lock_bh(&hdmi_lock);
				pBuffList->buf_state = create_new;
				pBuffList->fence = data.fence;
				pBuffList->idx = data.value;
				pBuffList->hnd = handle;
				pBuffList->mva = mva;
				pBuffList->va = va;
				hdmi_buffer.fence_fd = data.fence;
				hdmi_buffer.index = data.value;
				INIT_LIST_HEAD(&pBuffList->list);
				list_add_tail(&pBuffList->list, &HDMI_Buffer_List);
				spin_unlock_bh(&hdmi_lock);
				HDMI_MSG(":NOT ERROR:add list :%x, index %d, fd %d, mav=0x%x,va =0x%x\n",
					 (unsigned int)pBuffList, pBuffList->idx, pBuffList->fence, &mva, &va);
#endif
			} else {
				HDMI_MSG(" : Error in hdmi_create_fence when is_clock_on is off\n");
				MMProfileLogEx(HDMI_MMP_Events.ErrorInfo,
					       MMProfileFlagPulse, Fence_Err, 0);
			}

			up(&hdmi_update_mutex);

			if (copy_to_user((void __user *)arg, &hdmi_buffer, sizeof(hdmi_buffer))) {
				HDMI_LOG(": copy_to_user error! line:%d\n", __LINE__);
				r = -EFAULT;
			}
#ifndef MTK_OVERLAY_ENGINE_SUPPORT
			if (hdmi_bufferdump_on > 0) {
				MMProfileLogEx(HDMI_MMP_Events.FenceCreate,
					       MMProfileFlagEnd, hdmi_buffer.fence_fd,
					       (unsigned int)pBuffList);
			}
#endif

#endif
			break;
		}

	case MTK_HDMI_SCREEN_CAPTURE:
		{
			int capture_wait_times = 0;

			capture_screen = true;

			if (copy_from_user(&capture_addr, (void __user *)arg, sizeof(capture_addr))) {
				HDMI_LOG(": copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}

			while (capture_wait_times < 3) {
				msleep(20);
				capture_wait_times++;

				if (capture_screen == false)
					break;
			}

			if (capture_screen == true) {
				HDMI_LOG
				    ("capture scree fail,is_enabled(%d), wait_times(%d)\n",
				     p->is_clock_on, capture_wait_times);
			} else {
				HDMI_LOG
				    ("screen_capture done,is_enabled(%d), wait_times(%d)\n",
				     p->is_clock_on, capture_wait_times);
			}

			capture_screen = false;
			break;
		}

	default:
		{
			HDMI_LOG("[hdmi][HDMI] arguments error %x, %x\n",
				 MTK_HDMI_GET_DEV_INFO, cmd);
			break;
		}
	}
	return r;
}

long hdmi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long r;
	bool fg_need_sync = false;

	switch (cmd) {
	case MTK_HDMI_AUDIO_VIDEO_ENABLE:
	case MTK_HDMI_POWER_ENABLE:
	case MTK_HDMI_VIDEO_CONFIG:
	case MTK_HDMI_POST_VIDEO_BUFFER:
	case MTK_HDMI_PREPARE_BUFFER:
		fg_need_sync = true;
		break;
	}

	if (fg_need_sync) {
		if (down_interruptible(&hdmi_ioctl_mutex))
			pr_err("[HDMI]warning: down err\n!");
	}

	r = hdmi_ioctl_ex(file, cmd, arg);

	if (fg_need_sync)
		up(&hdmi_ioctl_mutex);

	return r;
}


static int hdmi_remove(struct platform_device *pdev)
{
	return 0;
}

static bool hdmi_drv_init_context(void)
{
	static const struct HDMI_UTIL_FUNCS hdmi_utils = {
		.udelay = hdmi_udelay,
		.mdelay = hdmi_mdelay,
		.state_callback = hdmi_state_callback,
#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)
		.cec_state_callback = hdmi_cec_state_callback,
#endif
	};

	if (hdmi_drv != NULL)
		return TRUE;

	hdmi_drv = (struct HDMI_DRIVER *) HDMI_GetDriver();

	if (NULL == hdmi_drv)
		return FALSE;

	hdmi_drv->set_util_funcs(&hdmi_utils);
	hdmi_drv->get_params(hdmi_params);

	return TRUE;
}

static void __exit hdmi_exit(void)
{

#ifdef MTK_HDMI_FENCE_SUPPORT
	hdmi_sync_destroy();
#endif
	device_destroy(hdmi_class, hdmi_devno);
	class_destroy(hdmi_class);
	cdev_del(hdmi_cdev);
	unregister_chrdev_region(hdmi_devno, 1);

}

const struct file_operations hdmi_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = hdmi_ioctl,
	.open = hdmi_open,
	.release = hdmi_release,
};

static int hdmi_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct class_device *class_dev = NULL;

	pr_err("[hdmi]%s\n", __func__);

	/* Allocate device number for hdmi driver */
	ret = alloc_chrdev_region(&hdmi_devno, 0, 1, HDMI_DEVNAME);

	if (ret) {
		pr_err("[hdmi]alloc_chrdev_region fail\n");
		return -1;
	}

	/* For character driver register to system, device number binded to file operations */
	hdmi_cdev = cdev_alloc();
	hdmi_cdev->owner = THIS_MODULE;
	hdmi_cdev->ops = &hdmi_fops;
	ret = cdev_add(hdmi_cdev, hdmi_devno, 1);

	/* For device number binded to device name(hdmitx), one class is corresponeded to one node */
	hdmi_class = class_create(THIS_MODULE, HDMI_DEVNAME);
	/* mknod /dev/hdmitx */
	class_dev =
	    (struct class_device *)device_create(hdmi_class, NULL, hdmi_devno, NULL, HDMI_DEVNAME);

	pr_info("[hdmi][%s] current=0x%08x\n", __func__, (unsigned int)current);

	if (!hdmi_drv_init_context()) {
		pr_err("[hdmi]%s, hdmi_drv_init_context fail\n", __func__);
		return HDMI_STATUS_NOT_IMPLEMENTED;
	}

	init_waitqueue_head(&hdmi_video_mode_wq);
	INIT_LIST_HEAD(&hdmi_video_mode_buffer_list);

	init_waitqueue_head(&hdmi_rdma_config_wq);
	init_waitqueue_head(&hdmi_rdma_update_wq);
	init_waitqueue_head(&reg_update_wq);
	init_waitqueue_head(&dst_reg_update_wq);

	init_waitqueue_head(&hdmi_vsync_wq);

#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT
		ret = hdmi_internal_probe(pdev);
		if (ret) {
			pr_err("Fail to probe internal hdmi\n");
			return -1;
		}
#endif
	return 0;
}

#ifdef CONFIG_PM

int hdmi_pm_suspend(struct device *device)
{
	HDMI_LOG("[hdmi] %s\n", __func__);

	return 0;
}

int hdmi_pm_resume(struct device *device)
{
	HDMI_LOG("[hdmi] %s\n", __func__);

	return 0;
}

int hdmi_pm_restore_noirq(struct device *dev)
{
	HDMI_LOG("[hdmi] %s\n", __func__);

	hdmi_drv_pm_restore_noirq(dev);
	hdmi_module_init();

	return 0;
}

const struct dev_pm_ops hdmi_pm_ops = {
	.suspend = hdmi_pm_suspend,
	.resume = hdmi_pm_resume,
	.freeze = hdmi_pm_suspend,
	.thaw = hdmi_pm_resume,
	.poweroff = hdmi_pm_suspend,
	.restore = hdmi_pm_resume,
	.restore_noirq = hdmi_pm_restore_noirq
};
#endif

void hdmitx_suspend(void)
{
	HDMI_FUNC();

	hdmi_ioctl(NULL, MTK_HDMI_POWER_ENABLE, 0);
	DPI1_PowerOff();
}

void hdmitx_resume(void)
{
	HDMI_FUNC();

	hdmi_ioctl(NULL, MTK_HDMI_POWER_ENABLE, 1);
	DPI1_PowerOn();
}

static int hdmi_event_notify(struct notifier_block *self,
				unsigned long action, void *data)
{
	/* use hdmitx_suspend/hdmitx_resume */
	return 0;

	if (action != FB_EARLY_EVENT_BLANK)
		return 0;

	{
		struct fb_event *event = data;
		int blank_mode = *((int *)event->data);

		switch (blank_mode) {
		case FB_BLANK_UNBLANK:
		case FB_BLANK_NORMAL:
			HDMI_LOG(" hdmi_early_suspend\n");
			hdmi_ioctl(NULL, MTK_HDMI_POWER_ENABLE, 1);
			break;
		case FB_BLANK_VSYNC_SUSPEND:
		case FB_BLANK_HSYNC_SUSPEND:
			break;
		case FB_BLANK_POWERDOWN:
			hdmi_ioctl(NULL, MTK_HDMI_POWER_ENABLE, 0);
			break;
		default:
			return -EINVAL;
		}
	}
	return 0;
}


static const struct of_device_id hdmi_of_ids[] = {
	{.compatible = "mediatek,mt8127-disp-hdmi",},
	{},
};

static struct platform_driver hdmi_driver = {
	.probe = hdmi_probe,
	.remove = hdmi_remove,
	.driver = {
		.name = HDMI_DEVNAME,
#ifdef CONFIG_PM
		.pm = &hdmi_pm_ops,
#endif
	.of_match_table = hdmi_of_ids,

	}
};

static struct notifier_block hdmi_event_notifier = {
	.notifier_call  = hdmi_event_notify,
};

static int __init hdmi_init(void)
{
	int ret = 0;
	int tmp_boot_mode;
	struct device_node *np;
	unsigned int reg_value;


	pr_err("[hdmi]%s\n", __func__);

	if (platform_driver_register(&hdmi_driver)) {
		pr_err("[hdmi]failed to register hdmitx driver\n");
		return -1;
	}

	memset((void *)&hdmi_context, 0, sizeof(_t_hdmi_context));

	SET_HDMI_OFF();
	HDMI_LOG("hdmi set power state(%d) off\n", atomic_read(&p->state));

	np = of_find_compatible_node(NULL, NULL, "mediatek,mt8127-apmixedsys");
	if (np != NULL) {
		of_property_read_u32_index(np, "reg", 0, &reg_value);
		HDMI_ANALOG_BASE = (unsigned long)of_iomap(np, 0);
		pr_info("DT DPI1 PLL_Ref| reg base: 0x%x --> map:0x%lx\n",
			    reg_value, HDMI_ANALOG_BASE);
	} else
		pr_info("DPI1 [ERROR] fail to request DPI irq\n");

	init_hdmi_mmp_events();

	if (!hdmi_drv_init_context()) {
		pr_err("[hdmi]%s, hdmi_drv_init_context fail\n", __func__);
		return HDMI_STATUS_NOT_IMPLEMENTED;
	}

	p->output_mode = hdmi_params->output_mode;
	p->orientation = 0;
	hdmi_drv->init();
	HDMI_LOG("Output mode is %s\n",
		 (hdmi_params->output_mode ==
		  HDMI_OUTPUT_MODE_DPI_BYPASS) ? "HDMI_OUTPUT_MODE_DPI_BYPASS" :
		 "HDMI_OUTPUT_MODE_LCD_MIRROR");

	if (hdmi_params->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS)
		p->output_video_resolution = HDMI_VIDEO_RESOLUTION_NUM;

	HDMI_DBG_Init();

	hdmi_switch_data.name = "hdmi";
	hdmi_switch_data.index = 0;
	hdmi_switch_data.state = NO_DEVICE;

	/* for support hdmi hotplug, inform AP the event */
	ret = switch_dev_register(&hdmi_switch_data);

#if defined(CONFIG_MTK_INTERNAL_HDMI_SUPPORT)
	hdmi_cec_switch_data.name = "cec_hdmi";
	hdmi_cec_switch_data.index = 0;
	hdmi_cec_switch_data.state = 0;
	ret = switch_dev_register(&hdmi_cec_switch_data);
#endif

	hdmires_switch_data.name = "res_hdmi";
	hdmires_switch_data.index = 0;
	hdmires_switch_data.state = 0;

	/* for support hdmi hotplug, inform AP the event */
	ret = switch_dev_register(&hdmires_switch_data);

	if (ret) {
		pr_err("[hdmi][HDMI]switch_dev_register returned:%d!\n", ret);
		return 1;
	}
#ifdef MTK_HDMI_FENCE_SUPPORT
	hdmi_sync_init();
	INIT_LIST_HEAD(&HDMI_Buffer_List);
#endif
	tmp_boot_mode = get_boot_mode();

	if ((tmp_boot_mode == FACTORY_BOOT) || (tmp_boot_mode == ATE_FACTORY_BOOT))
		factory_mode = true;

	fb_register_client(&hdmi_event_notifier);

	return 0;
}

module_init(hdmi_init);
module_exit(hdmi_exit);
MODULE_DESCRIPTION("HDMI Driver");
MODULE_LICENSE("GPL");

#endif
