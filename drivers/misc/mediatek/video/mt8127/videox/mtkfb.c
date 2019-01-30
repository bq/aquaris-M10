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

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include "disp_assert_layer.h"
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/file.h>
#include <linux/list.h>
#include <linux/wakelock.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include "mt-plat/dma.h"
#include <linux/dma-mapping.h>
#include "mt-plat/mt_boot.h"
#include "disp_debug.h"
#include "disp_drv.h"
#include "ddp_hal.h"
#include "disp_drv_log.h"
#include "disp_hal.h"
#include "mtkfb.h"
#include "mtkfb_console.h"
#include "mtkfb_info.h"
#include "ddp_ovl.h"
#include "ddp_drv.h"
#include "mtkfb_priv.h"
#include "disp_drv_platform.h"
#include "leds_drv.h"

unsigned int EnableVSyncLog = 0;

static u32 MTK_FB_XRES;
static u32 MTK_FB_YRES;
static u32 MTK_FB_BPP;
static u32 MTK_FB_PAGES;
static u32 fb_xres_update;
static u32 fb_yres_update;

#define MTK_FB_XRESV (ALIGN_TO(MTK_FB_XRES, disphal_get_fb_alignment()))
#define MTK_FB_YRESV (ALIGN_TO(MTK_FB_YRES, disphal_get_fb_alignment()) * MTK_FB_PAGES)	/* For page flipping */
#define MTK_FB_BYPP  ((MTK_FB_BPP + 7) >> 3)
#define MTK_FB_LINE  (ALIGN_TO(MTK_FB_XRES, disphal_get_fb_alignment()) * MTK_FB_BYPP)
#define MTK_FB_SIZE  (MTK_FB_LINE * ALIGN_TO(MTK_FB_YRES, disphal_get_fb_alignment()))
#define MTK_FB_SIZEV (MTK_FB_LINE * ALIGN_TO(MTK_FB_YRES, disphal_get_fb_alignment()) * MTK_FB_PAGES)

#ifdef CONFIG_ANDROID
#define mtkfb_aee_print(string, args...) \
		aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_MMPROFILE_BUFFER, "sf-mtkfb blocked", string, ##args)
#else
#define mtkfb_aee_print(string, args...) dev_err(fb_dev, string, ##args)
#endif

/* --------------------------------------------------------------------------- */
/* local variables */
/* --------------------------------------------------------------------------- */
unsigned int fb_mva = 0;
#ifdef CONFIG_MTK_M4U
static bool mtkfb_enable_mmu = true;
#else
static bool mtkfb_enable_mmu;
#endif

static const struct timeval FRAME_INTERVAL = { 0, 30000 };	/* 33ms */

atomic_t has_pending_update = ATOMIC_INIT(0);
struct fb_overlay_layer video_layerInfo;
uint32_t dbr_backup = 0;
uint32_t dbg_backup = 0;
uint32_t dbb_backup = 0;
bool fblayer_dither_needed = false;
static unsigned int video_rotation;
static uint32_t mtkfb_using_layer_type = LAYER_2D;
static bool hwc_force_fb_enabled = true;
bool is_ipoh_bootup = false;
struct fb_info *mtkfb_fbi;
struct device *fb_dev;
struct fb_overlay_layer fb_layer_context;
mtk_dispif_info_t dispif_info[MTKFB_MAX_DISPLAY_COUNT];

/* This mutex is used to prevent tearing due to page flipping when adbd is
   reading the front buffer
*/
DEFINE_SEMAPHORE(sem_flipping);
DEFINE_SEMAPHORE(sem_early_suspend);
DEFINE_SEMAPHORE(sem_overlay_buffer);
DEFINE_MUTEX(OverlaySettingMutex);
atomic_t OverlaySettingDirtyFlag = ATOMIC_INIT(0);
atomic_t OverlaySettingApplied = ATOMIC_INIT(0);
unsigned int PanDispSettingPending = 0;
unsigned int PanDispSettingDirty = 0;
unsigned int PanDispSettingApplied = 0;

DECLARE_WAIT_QUEUE_HEAD(disp_reg_update_wq);

unsigned int need_esd_check = 0;
DECLARE_WAIT_QUEUE_HEAD(esd_check_wq);
DEFINE_MUTEX(ScreenCaptureMutex);
static struct wake_lock mtkfb_wake_lock;
static bool is_wake_lock;

bool is_early_suspended;
static int sem_flipping_cnt = 1;
static int sem_early_suspend_cnt = 1;
static int sem_overlay_buffer_cnt = 1;
static int vsync_cnt;
static bool is_work_skip;

/* --------------------------------------------------------------------------- */
/* local function declarations */
/* --------------------------------------------------------------------------- */

static int init_framebuffer(struct fb_info *info);
static int mtkfb_set_overlay_layer(struct mtkfb_device *fbdev, struct fb_overlay_layer *layerInfo);
static int mtkfb_get_overlay_layer_info(struct fb_overlay_layer_info *layerInfo);
static int mtkfb_update_screen(struct fb_info *info);
static void mtkfb_update_screen_impl(void);
static void mtkfb_suspend(void);
static void mtkfb_resume(void);

/* --------------------------------------------------------------------------- */
/* Timer Routines */
/* --------------------------------------------------------------------------- */
static struct task_struct *esd_recovery_task;
unsigned int lcd_fps = 6000;
wait_queue_head_t screen_update_wq;

/* Grallc extra bit type */
enum {
	GRALLOC_EXTRA_BIT_TYPE_CPU = 0x00000000,
	GRALLOC_EXTRA_BIT_TYPE_GPU = 0x00000001,
	GRALLOC_EXTRA_BIT_TYPE_VIDEO = 0x00000002,
	GRALLOC_EXTRA_BIT_TYPE_CAMERA = 0x00000003,
	GRALLOC_EXTRA_MASK_TYPE = 0x00000003,
};


#if 1 /* defined(DFO_USE_NEW_API) */
#else
#include <mach/dfo_boot.h>
static disp_dfo_item_t disp_dfo_setting[] = {
	{"LCM_FAKE_WIDTH", 0},
	{"LCM_FAKE_HEIGHT", 0},
	{"DISP_DEBUG_SWITCH", 0}
};

/* this function will be called in mt_fixup()@mt_devs.c.*/
/* which will send DFO information organized as tag_dfo_boot struct. */
/* because lcm_params isn't inited here, so we will change lcm_params later in mtkfb_probe. */
unsigned int mtkfb_parse_dfo_setting(void *dfo_tbl, int num)
{
	char *disp_name = NULL;
	/* int  *disp_value; */
	char *tag_name;
	int tag_value;
	int i, j;
	tag_dfo_boot *dfo_data;

	DISP_FB("enter mtkfb_parse_dfo_setting\n");

	if (dfo_tbl == NULL)
		return -1;

	dfo_data = (tag_dfo_boot *) dfo_tbl;
	for (i = 0; i < (sizeof(disp_dfo_setting) / sizeof(disp_dfo_item_t)); i++) {
		disp_name = disp_dfo_setting[i].name;

		for (j = 0; j < num; j++) {
			tag_name = dfo_data->name[j];
			tag_value = dfo_data->value[j];
			if (!strcmp(disp_name, tag_name)) {
				disp_dfo_setting[i].value = tag_value;
				DISP_FB("%s = [DEC]%d [HEX]0x%08x\n",
						disp_dfo_setting[i].name, disp_dfo_setting[i].value,
						disp_dfo_setting[i].value);
			}
		}
	}

	DISP_FB("leave mtkfb_parse_dfo_setting\n");

	return 0;
}

int mtkfb_get_dfo_setting(const char *string, unsigned int *value)
{
	char *disp_name;
	int disp_value;
	int i;

	if (string == NULL)
		return -1;

	for (i = 0; i < (sizeof(disp_dfo_setting) / sizeof(disp_dfo_item_t)); i++) {
		disp_name = disp_dfo_setting[i].name;
		disp_value = disp_dfo_setting[i].value;
		if (!strcmp(disp_name, string)) {
			*value = disp_value;
			DISP_FB("%s = [DEC]%d [HEX]0x%08x\n", disp_name, disp_value,
					disp_value);
			return 0;
		}
	}

	return 0;
}
#endif


void mtkfb_pan_disp_test(void)
{
	if (down_interruptible(&sem_flipping)) {
		dev_err(fb_dev, "[fb driver] can't get semaphore:%d\n", __LINE__);
		return;
	}
	sem_flipping_cnt--;
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "MTKFB", "wait sem_flipping\n");
	if (down_interruptible(&sem_early_suspend)) {
		dev_err(fb_dev, "[fb driver] can't get semaphore:%d\n", __LINE__);
		sem_flipping_cnt++;
		up(&sem_flipping);
		return;
	}
	sem_early_suspend_cnt--;

	DISP_LOG_PRINT(ANDROID_LOG_WARN, "MTKFB", "wait sem_early_suspend\n");
	if (down_interruptible(&sem_overlay_buffer)) {
		dev_err(fb_dev, "[fb driver] can't get semaphore,%d\n", __LINE__);
		sem_early_suspend_cnt++;
		up(&sem_early_suspend);

		sem_flipping_cnt++;
		up(&sem_flipping);
		return;
	}
	sem_overlay_buffer_cnt--;
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "MTKFB", "wait sem_overlay_buffer\n");
	if (is_early_suspended)
		goto end;

end:
	sem_overlay_buffer_cnt++;
	sem_early_suspend_cnt++;
	sem_flipping_cnt++;
	up(&sem_overlay_buffer);
	up(&sem_early_suspend);
	up(&sem_flipping);
}

void mtkfb_show_sem_cnt(void)
{
	DISP_FB("[FB driver: sem cnt = %d, %d, %d. fps = %d, vsync_cnt = %d\n",
		sem_overlay_buffer_cnt, sem_early_suspend_cnt, sem_flipping_cnt, lcd_fps,
		vsync_cnt);
	DISP_FB("[FB driver: sem cnt = %d, %d, %d\n", sem_overlay_buffer.count,
		sem_early_suspend.count, sem_flipping.count);
}

void mtkfb_hang_test(bool en)
{
	if (en) {
		if (down_interruptible(&sem_flipping)) {
			dev_err(fb_dev, "[fb driver] can't get semaphore:%d\n", __LINE__);
			return;
		}
		sem_flipping_cnt--;
	} else {
		sem_flipping_cnt++;
		up(&sem_flipping);
	}
}

bool esd_kthread_pause = true;

void esd_recovery_pause(bool en)
{
	esd_kthread_pause = en;
}

static int esd_recovery_kthread(void *data)
{
	DISP_FB("enter esd_recovery_kthread()\n");
	for (;;) {

		if (kthread_should_stop())
			break;

		DISP_FB("sleep start in esd_recovery_kthread()\n");
		msleep(2000);	/* 2s */
		DISP_FB("sleep ends in esd_recovery_kthread()\n");

		if (!esd_kthread_pause) {
			if (is_early_suspended) {
				DISP_FB("is_early_suspended in esd_recovery_kthread()\n");
				continue;
			}
			/* /execute ESD check and recover flow */
			DISP_FB("DISP_EsdCheck starts\n");
			need_esd_check = 1;
			wait_event_interruptible(esd_check_wq, !need_esd_check);
			DISP_FB("DISP_EsdCheck ends\n");
		}
	}


	DISP_FB("exit esd_recovery_kthread()\n");
	return 0;
}


/*
 * ---------------------------------------------------------------------------
 *  mtkfb_set_lcm_inited() will be called in mt6516_board_init()
 * ---------------------------------------------------------------------------
 */
static bool is_lcm_inited;
void mtkfb_set_lcm_inited(bool inited)
{
	is_lcm_inited = inited;
}

/*
* sometimes, work that has not be executed will no need, so skip it.
*/
bool mtkfb_work_is_skip(void)
{
	return is_work_skip;
}

static int mtkfb_open(struct fb_info *info, int user)
{
	MSG_FUNC_ENTER();
	MSG_FUNC_LEAVE();
	return 0;
}

static int mtkfb_release(struct fb_info *info, int user)
{
	pr_err("mtkfb_release\n");
	MSG_FUNC_ENTER();
	if (down_interruptible(&sem_early_suspend)) {
		dev_err(fb_dev, "semaphore down failed in %s\n", __func__);
		return -ERESTARTSYS;
	}
	is_work_skip = true;
	flush_workqueue(((struct mtkfb_device *)info->par)->update_ovls_wq);
	is_work_skip = false;
	MSG_FUNC_LEAVE();
	up(&sem_early_suspend);
	return 0;
}

/* Store a single color palette entry into a pseudo palette or the hardware
 * palette if one is available. For now we support only 16bpp and thus store
 * the entry only to the pseudo palette.
 */
static int mtkfb_setcolreg(u_int regno, u_int red, u_int green,
			   u_int blue, u_int transp, struct fb_info *info)
{
	int r = 0;
	unsigned bpp, m;

	/* MSG_FUNC_ENTER(); */

	bpp = info->var.bits_per_pixel;
	m = 1 << bpp;
	if (regno >= m) {
		r = -EINVAL;
		goto exit;
	}

	switch (bpp) {
	case 16:
		/* RGB 565 */
		((u32 *) (info->pseudo_palette))[regno] =
		    ((red & 0xF800) | ((green & 0xFC00) >> 5) | ((blue & 0xF800) >> 11));
		break;
	case 32:
		/* ARGB8888 */
		((u32 *) (info->pseudo_palette))[regno] =
		    (0xff000000) |
		    ((red & 0xFF00) << 8) | ((green & 0xFF00)) | ((blue & 0xFF00) >> 8);
		break;

		/* TODO: RGB888, BGR888, ABGR8888 */

	default:
		ASSERT(0);
	}

exit:
	/* MSG_FUNC_LEAVE(); */
	return r;
}

static void mtkfb_update_screen_impl(void)
{
	bool down_sem = false;

	MMProfileLog(MTKFB_MMP_Events.UpdateScreenImpl, MMProfileFlagStart);
	if (down_interruptible(&sem_overlay_buffer))
		dev_err(fb_dev, "[FB Driver] can't get semaphore in mtkfb_update_screen_impl()\n");
	else {
		down_sem = true;
		sem_overlay_buffer_cnt--;
	}

	DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update));

	if (down_sem) {
		sem_overlay_buffer_cnt++;
		up(&sem_overlay_buffer);
	}
	MMProfileLog(MTKFB_MMP_Events.UpdateScreenImpl, MMProfileFlagEnd);
}


static int mtkfb_update_screen(struct fb_info *info)
{
	if (down_interruptible(&sem_early_suspend)) {
		dev_err(fb_dev, "[FB Driver] can't get semaphore in mtkfb_update_screen()\n");
		return -ERESTARTSYS;
	}
	sem_early_suspend_cnt--;
	if (is_early_suspended)
		goto End;
	mtkfb_update_screen_impl();

End:
	sem_early_suspend_cnt++;
	up(&sem_early_suspend);
	return 0;
}

static unsigned int BL_level;
static bool BL_set_level_resume;
int mtkfb_set_backlight_level(unsigned int level)
{
	DISP_FB("mtkfb_set_backlight_level:%d\n", level);
	if (down_interruptible(&sem_flipping)) {
		dev_err(fb_dev, "[FB Driver] can't get semaphore:%d\n", __LINE__);
		return -ERESTARTSYS;
	}
	sem_flipping_cnt--;
	if (down_interruptible(&sem_early_suspend)) {
		dev_err(fb_dev, "[FB Driver] can't get semaphore:%d\n", __LINE__);
		sem_flipping_cnt++;
		up(&sem_flipping);
		return -ERESTARTSYS;
	}

	sem_early_suspend_cnt--;
	if (is_early_suspended) {
		BL_level = level;
		BL_set_level_resume = true;
		DISP_FB("[FB driver] set backlight level but FB has been suspended\n");
		goto End;
	}
	DISP_SetBacklight(level);
	BL_set_level_resume = false;
End:
	sem_flipping_cnt++;
	sem_early_suspend_cnt++;
	up(&sem_early_suspend);
	up(&sem_flipping);
	return 0;
}
EXPORT_SYMBOL(mtkfb_set_backlight_level);

int mtkfb_set_backlight_mode(unsigned int mode)
{
	if (down_interruptible(&sem_flipping)) {
		dev_err(fb_dev, "[FB Driver] can't get semaphore:%d\n", __LINE__);
		return -ERESTARTSYS;
	}
	sem_flipping_cnt--;
	if (down_interruptible(&sem_early_suspend)) {
		dev_err(fb_dev, "[FB Driver] can't get semaphore:%d\n", __LINE__);
		sem_flipping_cnt++;
		up(&sem_flipping);
		return -ERESTARTSYS;
	}

	sem_early_suspend_cnt--;
	if (is_early_suspended)
		goto End;

	DISP_SetBacklight_mode(mode);
End:
	sem_flipping_cnt++;
	sem_early_suspend_cnt++;
	up(&sem_early_suspend);
	up(&sem_flipping);
	return 0;
}
EXPORT_SYMBOL(mtkfb_set_backlight_mode);


int mtkfb_set_backlight_pwm(int div)
{
	if (down_interruptible(&sem_flipping)) {
		dev_err(fb_dev, "[FB Driver] can't get semaphore:%d\n", __LINE__);
		return -ERESTARTSYS;
	}
	sem_flipping_cnt--;
	if (down_interruptible(&sem_early_suspend)) {
		dev_err(fb_dev, "[FB Driver] can't get semaphore:%d\n", __LINE__);
		sem_flipping_cnt++;
		up(&sem_flipping);
		return -ERESTARTSYS;
	}
	sem_early_suspend_cnt--;
	if (is_early_suspended)
		goto End;
	DISP_SetPWM(div);
End:
	sem_flipping_cnt++;
	sem_early_suspend_cnt++;
	up(&sem_early_suspend);
	up(&sem_flipping);
	return 0;
}
EXPORT_SYMBOL(mtkfb_set_backlight_pwm);

int mtkfb_get_backlight_pwm(int div, unsigned int *freq)
{
	DISP_GetPWM(div, freq);
	return 0;
}
EXPORT_SYMBOL(mtkfb_get_backlight_pwm);

void mtkfb_waitVsync(void)
{
	if (is_early_suspended) {
		DISP_FB("[MTKFB_VSYNC]:mtkfb has suspend, return directly\n");
		msleep(20);
		return;
	}
	vsync_cnt++;
	DISP_WaitVSYNC();
	vsync_cnt--;
}
EXPORT_SYMBOL(mtkfb_waitVsync);

#ifdef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT

DISP_OVL_ENGINE_INSTANCE_HANDLE mtkfb_instance = 0xff;

static int mtkfb_pan_display_impl(struct fb_var_screeninfo *var, struct fb_info *info)
{
	uint32_t offset;
	uint32_t paStart;
	char *vaStart, *vaEnd;
	struct fb_overlay_layer layerInfo = { 0 };

	MMProfileLogStructure(MTKFB_MMP_Events.PanDisplay, MMProfileFlagStart,
						var, struct fb_var_screeninfo);
	MSG_FUNC_ENTER();

	DISP_FB("pan_display: xoffset=%u, yoffset=%u, xres=%u, yres=%u, xresv=%u, yresv=%u\n",
	    var->xoffset, var->yoffset,
	    info->var.xres, info->var.yres, info->var.xres_virtual, info->var.yres_virtual);

	if (down_interruptible(&sem_flipping)) {
		DISP_ERR("can't get semaphore in mtkfb_pan_display_impl()\n");
		MMProfileLogMetaString(MTKFB_MMP_Events.PanDisplay, MMProfileFlagEnd,
				       "Can't get semaphore in mtkfb_pan_display_impl()");
		return -ERESTARTSYS;
	}
	sem_flipping_cnt--;

	info->var.yoffset = var->yoffset;

	offset = var->yoffset * info->fix.line_length;
	paStart = fb_mva + offset;
	vaStart = info->screen_base + offset;
	vaEnd = vaStart + info->var.yres * info->fix.line_length;

	if (isAEEEnabled)
		layerInfo.layer_id = FB_LAYER-1;
	else
		layerInfo.layer_id = FB_LAYER;
	layerInfo.layer_enable = true;
	layerInfo.src_base_addr = (void *)((unsigned int)vaStart);
	layerInfo.src_phy_addr = (void *)paStart;
	layerInfo.src_direct_link = 0;
	switch (var->bits_per_pixel) {
	case 16:
		layerInfo.src_fmt = MTK_FB_FORMAT_RGB565;
		break;
	case 24:
		layerInfo.src_fmt = MTK_FB_FORMAT_RGB888;
		break;
	case 32:
		layerInfo.src_fmt = MTK_FB_FORMAT_ARGB8888;
		break;
	default:
		dev_err(fb_dev, "Invalid color format bpp: 0x%d\n", var->bits_per_pixel);
		goto out;
	}
	layerInfo.src_use_color_key = 0;
	layerInfo.src_color_key = 0xFF;
	layerInfo.src_pitch = ALIGN_TO(var->xres, disphal_get_fb_alignment());
	layerInfo.src_offset_x = 0;
	layerInfo.src_offset_y = 0;
	layerInfo.src_width = var->xres;
	layerInfo.src_height = var->yres;
	layerInfo.tgt_offset_x = 0;
	layerInfo.tgt_offset_y = 0;
	layerInfo.tgt_width = var->xres;
	layerInfo.tgt_height = var->yres;
	layerInfo.layer_rotation = MTK_FB_ORIENTATION_0;
	layerInfo.layer_type = LAYER_2D;
	layerInfo.video_rotation = MTK_FB_ORIENTATION_0;
	layerInfo.isTdshp = true;
	layerInfo.next_buff_idx = -1;
	layerInfo.identity = 0;
	layerInfo.connected_type = 0;
	layerInfo.security = 0;
	DISP_FB("pan display: va=0x%x, pa=0x%x\n",
			(unsigned int)vaStart, paStart);
	Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layerInfo);
	layerInfo.layer_id = 0;
	layerInfo.layer_enable = false;
	Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layerInfo);
	layerInfo.layer_id = 1;
	layerInfo.layer_enable = false;
	Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layerInfo);
	if (!isAEEEnabled) {
		layerInfo.layer_id = 2;
		layerInfo.layer_enable = false;
		Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layerInfo);
	}


	PanDispSettingPending = 1;
	PanDispSettingDirty = 1;
	PanDispSettingApplied = 0;
	is_ipoh_bootup = false;

	Disp_Ovl_Engine_Trigger_Overlay(mtkfb_instance);
	/* Disp_Ovl_Engine_Wait_Overlay_Complete(mtkfb_instance, 1000); */

out:
	sem_flipping_cnt++;
	up(&sem_flipping);
	MMProfileLog(MTKFB_MMP_Events.PanDisplay, MMProfileFlagEnd);
	MSG_FUNC_LEAVE();
	return 0;
}

void mtkfb_ion_cpu_cache_flush(struct ion_client *client, struct ion_handle *handle)
{
#ifdef CONFIG_MTK_ION

	struct ion_sys_data sys_data;

	sys_data.sys_cmd = ION_SYS_CACHE_SYNC;
	sys_data.cache_sync_param.handle = (ion_user_handle_t) handle;
	sys_data.cache_sync_param.sync_type = ION_CACHE_FLUSH_BY_RANGE;

	if (ion_kernel_ioctl(client, ION_CMD_SYSTEM, (unsigned long)&sys_data))
		dev_err(fb_dev, "ion cache flush failed!\n");
#endif
}

static void mtkfb_update_ovls_handler(struct work_struct *_work)
{
	int ret, i, sem_down = 1;
	update_ovls_work_t *work = (update_ovls_work_t *) _work;
	struct mtkfb_device *fbdev = (struct mtkfb_device *)work->dev;
	struct fb_overlay_config *config = &work->config;
#ifdef CONFIG_SYNC
	struct sync_fence **fences = work->fences;
#endif
#ifdef CONFIG_MTK_ION
	struct ion_handle **ion_handles = work->ion_handles;
	ion_mm_data_t data;
	size_t _unused;
	ion_phys_addr_t mva;
#endif

	if (down_interruptible(&sem_early_suspend)) {
		dev_err(fb_dev, "semaphore down failed in %s\n", __func__);
		sem_down = 0;
		goto out;
	}

	if (mtkfb_is_suspend() || mtkfb_work_is_skip()) {
		DISP_FB("work skip\n");
		goto out;
	}

	DISP_FB("mtkfb_update_ovls_handler start\n");
	MMProfileLog(MTKFB_MMP_Events.WrokHandler, MMProfileFlagStart);

#ifdef CONFIG_MTK_ION
	/* ion handle to mva */
	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		if (!ion_handles[i])
			continue;

		/* configure buffer */
		memset(&data, 0, sizeof(ion_mm_data_t));
		data.mm_cmd = ION_MM_CONFIG_BUFFER;
		data.config_buffer_param.handle = (ion_user_handle_t) ion_handles[i];
		data.config_buffer_param.eModuleID = DISP_OVL_0;
		ion_kernel_ioctl(fbdev->ion_client, ION_CMD_MULTIMEDIA, (unsigned long)&data);

		/* Get "physical" address (mva) */
		if (ion_phys(fbdev->ion_client, ion_handles[i], &mva, &_unused)) {
			dev_err(fb_dev, "ion_phys failed, disable ovl %d\n", i);
			config->layers[i].layer_enable = 0;
			config->layers[i].src_phy_addr = 0;
			ion_free(fbdev->ion_client, ion_handles[i]);
			ion_handles[i] = NULL;
			continue;
		}

		if (!g_fb_pattern_en)
			config->layers[i].src_phy_addr = (void *)mva;

		DISP_FB("ion import layer%d: mva_0x%p\n",
				i, config->layers[i].src_phy_addr);
	}

	/* Flush CPU cache */
	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		ion_mm_data_t data;

		if (!work->ion_handles[i])
			continue;

		/* configure buffer */
		memset(&data, 0, sizeof(ion_mm_data_t));
		data.mm_cmd = ION_MM_GET_DEBUG_INFO;
		data.config_buffer_param.handle = (ion_user_handle_t) ion_handles[i];
		ion_kernel_ioctl(fbdev->ion_client, ION_CMD_MULTIMEDIA, (unsigned long)&data);

		if ((data.buf_debug_info_param.value4 & GRALLOC_EXTRA_MASK_TYPE) !=
		    GRALLOC_EXTRA_BIT_TYPE_CPU)
			continue;

		mtkfb_ion_cpu_cache_flush(fbdev->ion_client, work->ion_handles[i]);
	}
#endif

#ifdef CONFIG_SYNC
	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		if (!fences[i])
			continue;

		if (sync_fence_wait(fences[i], 1000) < 0) {
			config->layers[i].layer_enable = 0;
			config->layers[i].src_phy_addr = 0;
			dev_err(fb_dev, "wait acquire fence(fd%d/name: %s) timeout\n",
				config->layers[i].fence_fd, fences[i]->name);
			goto out;
		}
		DISP_FENCE("Acquire fence wait: layer%d/fence_fd%d/%s/ion_fd%d\n",
			 i, config->layers[i].fence_fd, fences[i]->name,
			 config->layers[i].ion_fd);

		MMProfileLogEx(MTKFB_MMP_Events.HWCFence[i], MMProfileFlagEnd, 0,
			       (unsigned int)fences[i]);
		sync_fence_put(fences[i]);
		fences[i] = NULL;
	}
#endif

	mutex_lock(&OverlaySettingMutex);
	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		if (!isAEEEnabled || (i < (HW_OVERLAY_COUNT - 1)))
			mtkfb_set_overlay_layer(fbdev, &config->layers[i]);
		else
			DISP_FB("dal is enable\n");
	}
	mutex_unlock(&OverlaySettingMutex);

	Disp_Ovl_Engine_Sync_Captured_layer_info(mtkfb_instance);
	Disp_Ovl_Engine_Trigger_Overlay(mtkfb_instance);
	ret = Disp_Ovl_Engine_Wait_Overlay_Complete(mtkfb_instance, 1000);
	if (ret < 0)
		pr_warn(" Disp_Ovl_Engine_Wait_Overlay_Complete timed out, tearing risk\n");

out:

#ifdef CONFIG_MTK_ION
	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		if (ion_handles[i]) {
			ion_free(fbdev->ion_client, ion_handles[i]);
			DISP_FB("ion free layer%d: mva_0x%p\n",
				i, config->layers[i].src_phy_addr);
			ion_handles[i] = NULL;
		}
	}
#endif

#ifdef CONFIG_SYNC
	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		if (fences[i]) {
			sync_fence_put(fences[i]);
			fences[i] = NULL;
		}
	}
	mutex_lock(&fbdev->timeline_lock);
	sw_sync_timeline_inc(fbdev->timeline, 1);
	MMProfileLogEx(MTKFB_MMP_Events.FBTimeline, MMProfileFlagEnd, fbdev->timeline->value, 1);
	DISP_FENCE("release fence: timeline++_%d\n", fbdev->timeline->value);
	mutex_unlock(&fbdev->timeline_lock);
#endif

	if (work != NULL)
		kfree(work);

	MMProfileLog(MTKFB_MMP_Events.WrokHandler, MMProfileFlagEnd);
	DISP_FB("mtkfb_update_ovls_handler end\n");
	if (sem_down)
		up(&sem_early_suspend);
}

int mtkfb_queue_overlay_config_ex(struct mtkfb_device *fbdev, struct fb_overlay_config *config,
				  unsigned int is_v4l2)
{
	int fd = -1, ret = 0, sem_down = 1;
	update_ovls_work_t *work = NULL;
#ifdef CONFIG_SYNC
	int i;
	struct sync_fence *fence;
	struct sync_pt *pt;
	struct fb_overlay_layer *layer;
#endif

	/* if surfaceflinger thread is killed, sem will be interrupted. */
	if (down_interruptible(&sem_early_suspend)) {
		dev_err(fbdev->dev, "semaphore down failed in %s\n", __func__);
		is_work_skip = true;
		sem_down = 0;
		goto err;
	} else {
		is_work_skip = false;
	}

	MMProfileLogStructure(MTKFB_MMP_Events.QueueWork, MMProfileFlagStart,
				config, struct fb_overlay_config);

	if (0 == is_v4l2) {
		fd = get_unused_fd();
		if (fd < 0) {
			dev_err(fbdev->dev, "could not get a file descriptor\n");
			ret = -ENOMEM;
			goto err;
		}
	}

	work = kzalloc(sizeof(update_ovls_work_t), GFP_KERNEL);
	if (!work) {
		dev_err(fb_dev, "could not allocate update_ovls_work_t\n");
		ret = -ENOMEM;
		goto err;
	}

	INIT_WORK((struct work_struct *)work, mtkfb_update_ovls_handler);
	work->dev = (void *)fbdev;
	work->is_v4l2 = is_v4l2;
	memcpy(&work->config, config, sizeof(struct fb_overlay_config));

#ifdef CONFIG_SYNC
	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		layer = &config->layers[i];
		if (config->layers[i].fence_fd < 0) {
			work->fences[i] = NULL;
			continue;
		}
		work->fences[i] = sync_fence_fdget(config->layers[i].fence_fd);
		if (work->fences[i]) {
			MMProfileLogEx(MTKFB_MMP_Events.HWCFence[i], MMProfileFlagStart,
					layer->fence_fd, (unsigned int)work->fences[i]);
			DISP_FENCE("Acquire fence prepare: layer%d/fence_fd%d/%s/ion_fd%d\n",
					i, layer->fence_fd, work->fences[i]->name, layer->ion_fd);
		}
	}

	/* Create release fence */
	if (0 == is_v4l2) {
		mutex_lock(&fbdev->timeline_lock);
		fbdev->timeline_max++;
		pt = sw_sync_pt_create(fbdev->timeline, fbdev->timeline_max);
		if (pt == NULL) {
			dev_err(fb_dev, "sw_sync_pt_create NULL\n");
			goto err;
		}
		fence = sync_fence_create("display", pt);
		if (fence == NULL) {
			sync_pt_free(pt);
			dev_err(fb_dev, "sync_fence_create NULL\n");
			goto err;
		}

		sync_fence_install(fence, fd);
		config->fence = fd;
		MMProfileLogEx(MTKFB_MMP_Events.FBFence, MMProfileFlagPulse, config->fence,
			       fbdev->timeline->value);
		MMProfileLogEx(MTKFB_MMP_Events.FBTimeline, MMProfileFlagStart,
			       fbdev->timeline->value, fbdev->timeline_max);
		DISP_FENCE("RF prepare: value%d/timeline%d\n", fbdev->timeline_max,
			 fbdev->timeline->value);
		mutex_unlock(&fbdev->timeline_lock);
	}
#endif

#ifdef CONFIG_MTK_ION
	/* Import ion handles so userspace (hwc) doesn't need to have a ref to them */
	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		struct fb_overlay_layer *layer = &config->layers[i];

		if (layer->ion_fd < 0 || layer->layer_enable == 0) {
			work->ion_handles[i] = NULL;
			continue;	/* worker will use mva from userspace */
		}
		work->ion_handles[i] = ion_import_dma_buf(fbdev->ion_client, layer->ion_fd);
		if (IS_ERR(work->ion_handles[i])) {
			dev_err(fbdev->dev, "failed to import ion fd, disable ovl %d\n", i);
			work->ion_handles[i] = NULL;
			layer->layer_enable = 0;
			continue;
		}
	}
#endif

	/* Queue work */
	queue_work(fbdev->update_ovls_wq, (struct work_struct *)work);
	goto out;

err:
	pr_warn("mtkfb_queue_overlay_config fd=%d failed\n", fd);
	config->fence = -1;
	if (fd >= 0)
		put_unused_fd(fd);

	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		if (work && work->fences[i])
			sync_fence_put(work->fences[i]);
	}

	if (work != NULL)
		kfree(work);

out:
	MMProfileLog(MTKFB_MMP_Events.QueueWork, MMProfileFlagEnd);

	if (sem_down)
		up(&sem_early_suspend);

	return ret;
}

int mtkfb_queue_overlay_config(struct mtkfb_device *fbdev, struct fb_overlay_config *config)
{
	return mtkfb_queue_overlay_config_ex(fbdev, config, 0);
}

static int mtkfb_set_overlay_layer(struct mtkfb_device *fbdev, struct fb_overlay_layer *layerInfo)
{
	unsigned int id = layerInfo->layer_id;
	int enable = layerInfo->layer_enable ? 1 : 0;
	int ret = 0;

	MSG_FUNC_ENTER();
	MMProfileLogEx(MTKFB_MMP_Events.SetOverlayLayer, MMProfileFlagStart, (id << 16) | enable,
		       (unsigned int)layerInfo->src_phy_addr);

	if (layerInfo->layer_enable) {
		DISP_FB("mtkfb_set_overlay_layer ===============================\n");
		DISP_FB("  id=%u, en=%u, next_idx=%u, vaddr=0x%x, paddr=0x%x\n",
			  layerInfo->layer_id, layerInfo->layer_enable, layerInfo->next_buff_idx,
			  (unsigned int)(layerInfo->src_base_addr),
			  (unsigned int)(layerInfo->src_phy_addr));
		DISP_FB("  fmt=%u, d-link=%u, pitch=%u, xoff=%u, yoff=%u, w=%u, h=%u\n",
			  layerInfo->src_fmt, (unsigned int)(layerInfo->src_direct_link),
			  layerInfo->src_pitch, layerInfo->src_offset_x, layerInfo->src_offset_y,
			  layerInfo->src_width, layerInfo->src_height);
		DISP_FB("  target xoff=%u, target yoff=%u, target w=%u, target h=%u\n\n",
			  layerInfo->tgt_offset_x, layerInfo->tgt_offset_y, layerInfo->tgt_width,
			  layerInfo->tgt_height);
	}

	/* Update Layer Enable Bits and Layer Config Dirty Bits */
	if ((((fbdev->layer_enable >> id) & 1) ^ enable)) {
		fbdev->layer_enable ^= (1 << id);
		fbdev->layer_config_dirty |= MTKFB_LAYER_ENABLE_DIRTY;
	}
	/* Update Layer Format and Layer Config Dirty Bits */
	if (fbdev->layer_format[id] != layerInfo->src_fmt) {
		fbdev->layer_format[id] = layerInfo->src_fmt;
		fbdev->layer_config_dirty |= MTKFB_LAYER_FORMAT_DIRTY;
	}
	/* Enter Overlay Mode if any layer is enabled except the FB layer */

	if (fbdev->layer_enable & ((1 << VIDEO_LAYER_COUNT) - 1)) {
		if (DISP_STATUS_OK == DISP_EnterOverlayMode())
			DISP_FB("mtkfb_ioctl(MTKFB_ENABLE_OVERLAY)\n");
	}

	if (!enable) {
		struct fb_overlay_layer layer = { 0 };

		layer.layer_id = layerInfo->layer_id;
		Disp_Ovl_Engine_Get_layer_info(mtkfb_instance, &layer);
		layer.layer_enable = enable;
		Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
		ret = 0;
		goto LeaveOverlayMode;
	}
	Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, layerInfo);

	{
#if defined(MTK_HDMI_SUPPORT)

#define SUPPORT_HMDI_SVP_P1 1

#if SUPPORT_HMDI_SVP_P1
		/* temporary solution for hdmi svp p1, always mute hdmi for svp */
		int tl = 0;
		int has_prot_layer = 0;
		int has_sec_layer = 0;

		OVL_CONFIG_STRUCT cached_layer[DDP_OVL_LAYER_MUN] = { {0}, {0}, {0}, {0} };

		Disp_Ovl_Engine_Dump_layer_info(mtkfb_instance, &cached_layer[0], NULL, NULL);

		for (tl = 0; tl < HW_OVERLAY_COUNT; tl++) {
			if (OVL_LAYER_SECURE_BUFFER == cached_layer[tl].security) {
				has_sec_layer = 1;
				break;
			}

			if (OVL_LAYER_PROTECTED_BUFFER == cached_layer[tl].security) {
				has_prot_layer = 1;
				break;
			}
		}
#else
		int tl = 0;
		int cnt_security_layer = 0;

		DISP_FB("Totally %d security layer is set now\n", cnt_security_layer);
		/* MTK_HDMI_Set_Security_Output(!!cnt_security_layer); */
#endif				/* SUPPORT_HMDI_SVP_P1 */
#endif
	}

#if defined(MTK_LCM_PHYSICAL_ROTATION)
	if (0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3)) {
		layerInfo->layer_rotation =
		    (layerInfo->layer_rotation + MTK_FB_ORIENTATION_180) % 4;
		layerInfo->tgt_offset_x =
		    MTK_FB_XRES - (layerInfo->tgt_offset_x + layerInfo->tgt_width);
		layerInfo->tgt_offset_y =
		    MTK_FB_YRES - (layerInfo->tgt_offset_y + layerInfo->tgt_height);
	}
#endif

	video_rotation = layerInfo->video_rotation;

LeaveOverlayMode:

	/* Leave Overlay Mode if only FB layer is enabled */
	if ((fbdev->layer_enable & ((1 << VIDEO_LAYER_COUNT) - 1)) == 0) {
		if (DISP_STATUS_OK == DISP_LeaveOverlayMode()) {
			DISP_FB("mtkfb_ioctl(MTKFB_DISABLE_OVERLAY)\n");
			if (fblayer_dither_needed)
				DISP_ConfigDither(14, 14, 14, dbr_backup, dbg_backup, dbb_backup);
		}
	}

	MSG_FUNC_LEAVE();
	MMProfileLog(MTKFB_MMP_Events.SetOverlayLayer, MMProfileFlagEnd);

	return ret;
}


#else

static int mtkfb_pan_display_impl(struct fb_var_screeninfo *var, struct fb_info *info)
{
	uint32_t offset;
	uint32_t paStart;
	char *vaStart, *vaEnd;
	int ret = 0;

	MMProfileLogStructure(MTKFB_MMP_Events.PanDisplay, MMProfileFlagStart,
						var, struct fb_var_screeninfo);
	MSG_FUNC_ENTER();

	DISP_FB("pan_display: xoffset=%u, yoffset=%u, xres=%u, yres=%u, xresv=%u, yresv=%u\n",
	    var->xoffset, var->yoffset,
	    info->var.xres, info->var.yres, info->var.xres_virtual, info->var.yres_virtual);

	if (down_interruptible(&sem_flipping)) {
		DISP_ERR("can't get semaphore in mtkfb_pan_display_impl()\n");
		MMProfileLogMetaString(MTKFB_MMP_Events.PanDisplay, MMProfileFlagEnd,
				       "Can't get semaphore in mtkfb_pan_display_impl()");
		return -ERESTARTSYS;
	}
	sem_flipping_cnt--;

	info->var.yoffset = var->yoffset;

	offset = var->yoffset * info->fix.line_length;
	paStart = fb_mva + offset;
	vaStart = info->screen_base + offset;
	vaEnd = vaStart + info->var.yres * info->fix.line_length;

	mutex_lock(&OverlaySettingMutex);
	DISP_CHECK_RET(DISP_SetFrameBufferAddr(paStart));
	cached_layer_config[FB_LAYER].vaddr = (unsigned int)vaStart;
	cached_layer_config[FB_LAYER].layer_en = 1;
	cached_layer_config[FB_LAYER].src_x = 0;
	cached_layer_config[FB_LAYER].src_y = 0;
	cached_layer_config[FB_LAYER].src_w = var->xres;
	cached_layer_config[FB_LAYER].src_h = var->yres;
	cached_layer_config[FB_LAYER].dst_x = 0;
	cached_layer_config[FB_LAYER].dst_y = 0;
	cached_layer_config[FB_LAYER].dst_w = var->xres;
	cached_layer_config[FB_LAYER].dst_h = var->yres;
	{
		unsigned int layerpitch;
		unsigned int src_pitch = ALIGN_TO(var->xres, disphal_get_fb_alignment());

		switch (var->bits_per_pixel) {
		case 16:
			cached_layer_config[FB_LAYER].fmt = eRGB565;
			layerpitch = 2;
			cached_layer_config[FB_LAYER].aen = false;
			break;
		case 24:
			cached_layer_config[FB_LAYER].fmt = eRGB888;
			layerpitch = 3;
			cached_layer_config[FB_LAYER].aen = false;
			break;
		case 32:
			cached_layer_config[FB_LAYER].fmt = ePARGB8888;
			layerpitch = 4;
			cached_layer_config[FB_LAYER].aen = true;
			break;
		default:
			dev_err(fb_dev, "Invalid color format bpp: 0x%d\n", var->bits_per_pixel);
			return -1;
		}
		cached_layer_config[FB_LAYER].alpha = 0xFF;
		cached_layer_config[FB_LAYER].buff_idx = -1;
		cached_layer_config[FB_LAYER].src_pitch = src_pitch * layerpitch;
	}

	atomic_set(&OverlaySettingDirtyFlag, 1);
	atomic_set(&OverlaySettingApplied, 0);
	PanDispSettingPending = 1;
	PanDispSettingDirty = 1;
	PanDispSettingApplied = 0;
	is_ipoh_bootup = false;
	mutex_unlock(&OverlaySettingMutex);

	ret = mtkfb_update_screen(info);
	ret = wait_event_interruptible_timeout(disp_reg_update_wq, PanDispSettingApplied, HZ / 10);
	DISP_FB("[WaitQ] wait_event_interruptible() ret = %d, %d\n", ret, __LINE__);

	sem_flipping_cnt++;
	up(&sem_flipping);
	MMProfileLog(MTKFB_MMP_Events.PanDisplay, MMProfileFlagEnd);

	return 0;
}

int mtkfb_queue_overlay_config_ex(struct mtkfb_device *fbdev, struct fb_overlay_config *config,
				  unsigned int is_v4l2)
{
	return 0;
}

int mtkfb_queue_overlay_config(struct mtkfb_device *fbdev, struct fb_overlay_config *config)
{
	return 0;
}

static int mtkfb_set_overlay_layer(struct mtkfb_device *fbdev, struct fb_overlay_layer *layerInfo)
{
	unsigned int layerpitch;
	unsigned int layerbpp;
	unsigned int id = layerInfo->layer_id;
	int enable = layerInfo->layer_enable ? 1 : 0;
	int ret = 0;

	MSG_FUNC_ENTER();
	MMProfileLogEx(MTKFB_MMP_Events.SetOverlayLayer, MMProfileFlagStart, (id << 16) | enable,
		       (unsigned int)layerInfo->src_phy_addr);

	DISP_FB("mtkfb_set_overlay_layer ===============================\n");
	DISP_FB("  id=%u, en=%u, next_idx=%u, vaddr=0x%x, paddr=0x%x\n",
		  layerInfo->layer_id, layerInfo->layer_enable, layerInfo->next_buff_idx,
		  (unsigned int)(layerInfo->src_base_addr),
		  (unsigned int)(layerInfo->src_phy_addr));
	DISP_FB("  fmt=%u, d-link=%u, pitch=%u, xoff=%u, yoff=%u, w=%u, h=%u\n",
		  layerInfo->src_fmt, (unsigned int)(layerInfo->src_direct_link),
		  layerInfo->src_pitch, layerInfo->src_offset_x, layerInfo->src_offset_y,
		  layerInfo->src_width, layerInfo->src_height);
	DISP_FB("  target xoff=%u, target yoff=%u, target w=%u, target h=%u\n\n",
		  layerInfo->tgt_offset_x, layerInfo->tgt_offset_y, layerInfo->tgt_width,
		  layerInfo->tgt_height);

	/* Update Layer Enable Bits and Layer Config Dirty Bits */
	if ((((fbdev->layer_enable >> id) & 1) ^ enable)) {
		fbdev->layer_enable ^= (1 << id);
		fbdev->layer_config_dirty |= MTKFB_LAYER_ENABLE_DIRTY;
	}
	/* Update Layer Format and Layer Config Dirty Bits */
	if (fbdev->layer_format[id] != layerInfo->src_fmt) {
		fbdev->layer_format[id] = layerInfo->src_fmt;
		fbdev->layer_config_dirty |= MTKFB_LAYER_FORMAT_DIRTY;
	}
	/* Enter Overlay Mode if any layer is enabled except the FB layer */

	if (fbdev->layer_enable & ((1 << VIDEO_LAYER_COUNT) - 1)) {
		if (DISP_STATUS_OK == DISP_EnterOverlayMode())
			DISP_FB("mtkfb_ioctl(MTKFB_ENABLE_OVERLAY)\n");
	}

	if (!enable) {
		cached_layer_config[id].layer_en = enable;
		cached_layer_config[id].isDirty = true;
		cached_layer_config[id].security = false;
		ret = 0;
		goto LeaveOverlayMode;
	}

	switch (layerInfo->src_fmt) {
	case MTK_FB_FORMAT_YUV422:
		cached_layer_config[id].fmt = eYUY2;
		layerpitch = 2;
		layerbpp = 24;
		break;

	case MTK_FB_FORMAT_RGB565:
		cached_layer_config[id].fmt = eRGB565;
		layerpitch = 2;
		layerbpp = 16;
		break;

	case MTK_FB_FORMAT_RGB888:
		cached_layer_config[id].fmt = eRGB888;
		layerpitch = 3;
		layerbpp = 24;
		break;
	case MTK_FB_FORMAT_BGR888:
		cached_layer_config[id].fmt = eBGR888;
		layerpitch = 3;
		layerbpp = 24;
		break;

	case MTK_FB_FORMAT_ARGB8888:
		cached_layer_config[id].fmt = ePARGB8888;
		layerpitch = 4;
		layerbpp = 32;
		break;
	case MTK_FB_FORMAT_ABGR8888:
		cached_layer_config[id].fmt = ePABGR8888;
		layerpitch = 4;
		layerbpp = 32;
		break;
	case MTK_FB_FORMAT_XRGB8888:
		cached_layer_config[id].fmt = eARGB8888;
		layerpitch = 4;
		layerbpp = 32;
		break;
	case MTK_FB_FORMAT_XBGR8888:
		cached_layer_config[id].fmt = eABGR8888;
		layerpitch = 4;
		layerbpp = 32;
		break;
	default:
		dev_err(fbdev->dev, "Invalid color format: 0x%x\n", layerInfo->src_fmt);
		ret = -EFAULT;
		goto LeaveOverlayMode;
	}
	cached_layer_config[id].vaddr = (unsigned int)layerInfo->src_base_addr;
	cached_layer_config[id].security = layerInfo->security;
	cached_layer_config[id].addr = (unsigned int)layerInfo->src_phy_addr;
	cached_layer_config[id].isTdshp = layerInfo->isTdshp;
	cached_layer_config[id].buff_idx = layerInfo->next_buff_idx;

	{
#if defined(MTK_HDMI_SUPPORT)

#define SUPPORT_HMDI_SVP_P1 1

#if SUPPORT_HMDI_SVP_P1
		/* temporary solution for hdmi svp p1, always mute hdmi for svp */
		int tl = 0;
		int has_prot_layer = 0;
		int has_sec_layer = 0;

		for (tl = 0; tl < HW_OVERLAY_COUNT; tl++) {
			if (OVL_LAYER_SECURE_BUFFER == cached_layer_config[tl].security) {
				has_sec_layer = 1;
				break;
			}

			if (OVL_LAYER_PROTECTED_BUFFER == cached_layer_config[tl].security) {
				has_prot_layer = 1;
				break;
			}
		}
#else
		int tl = 0;
		int cnt_security_layer = 0;

		for (tl = 0; tl < HW_OVERLAY_COUNT; tl++)
			cnt_security_layer += cached_layer_config[tl].security;
		DISP_FB("Totally %d security layer is set now\n", cnt_security_layer);
		/* MTK_HDMI_Set_Security_Output(!!cnt_security_layer); */
#endif				/* SUPPORT_HMDI_SVP_P1 */
#endif
	}
	cached_layer_config[id].identity = layerInfo->identity;
	cached_layer_config[id].connected_type = layerInfo->connected_type;

	/* set Alpha blending */
	cached_layer_config[id].alpha = 0xFF;
	if (layerInfo->alpha_enable) {
		cached_layer_config[id].aen = true;
		cached_layer_config[id].alpha = layerInfo->alpha;
	} else
		cached_layer_config[id].aen = false;
	if (MTK_FB_FORMAT_ARGB8888 == layerInfo->src_fmt ||
	    MTK_FB_FORMAT_ABGR8888 == layerInfo->src_fmt)
		cached_layer_config[id].aen = true;
	/* set src width, src height */
	cached_layer_config[id].src_x = layerInfo->src_offset_x;
	cached_layer_config[id].src_y = layerInfo->src_offset_y;
	cached_layer_config[id].src_w = layerInfo->src_width;
	cached_layer_config[id].src_h = layerInfo->src_height;
	cached_layer_config[id].dst_x = layerInfo->tgt_offset_x;
	cached_layer_config[id].dst_y = layerInfo->tgt_offset_y;
	cached_layer_config[id].dst_w = layerInfo->tgt_width;
	cached_layer_config[id].dst_h = layerInfo->tgt_height;
	if (cached_layer_config[id].dst_w > cached_layer_config[id].src_w)
		cached_layer_config[id].dst_w = cached_layer_config[id].src_w;
	if (cached_layer_config[id].dst_h > cached_layer_config[id].src_h)
		cached_layer_config[id].dst_h = cached_layer_config[id].src_h;

	cached_layer_config[id].src_pitch = layerInfo->src_pitch * layerpitch;

#if defined(MTK_LCM_PHYSICAL_ROTATION)
	if (0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3)) {
		layerInfo->layer_rotation =
		    (layerInfo->layer_rotation + MTK_FB_ORIENTATION_180) % 4;
		layerInfo->tgt_offset_x =
		    MTK_FB_XRES - (layerInfo->tgt_offset_x + layerInfo->tgt_width);
		layerInfo->tgt_offset_y =
		    MTK_FB_YRES - (layerInfo->tgt_offset_y + layerInfo->tgt_height);
	}
#endif

	video_rotation = layerInfo->video_rotation;

	/* set color key */
	cached_layer_config[id].key = layerInfo->src_color_key;
	cached_layer_config[id].keyEn = layerInfo->src_use_color_key;

	/* data transferring is triggerred in MTKFB_TRIG_OVERLAY_OUT */
	cached_layer_config[id].layer_en = enable;
	cached_layer_config[id].isDirty = true;

LeaveOverlayMode:

	/* Leave Overlay Mode if only FB layer is enabled */
	if ((fbdev->layer_enable & ((1 << VIDEO_LAYER_COUNT) - 1)) == 0) {
		if (DISP_STATUS_OK == DISP_LeaveOverlayMode()) {
			DISP_FB("mtkfb_ioctl(MTKFB_DISABLE_OVERLAY)\n");
			if (fblayer_dither_needed)
				DISP_ConfigDither(14, 14, 14, dbr_backup, dbg_backup, dbb_backup);
		}
	}

	MSG_FUNC_LEAVE();
	MMProfileLog(MTKFB_MMP_Events.SetOverlayLayer, MMProfileFlagEnd);

	return ret;
}

#endif


static int mtkfb_pan_display_proxy(struct fb_var_screeninfo *var, struct fb_info *info)
{
	return mtkfb_pan_display_impl(var, info);
}


/* Set fb_info.fix fields and also updates fbdev.
 * When calling this fb_info.var must be set up already.
 */
static void set_fb_fix(struct mtkfb_device *fbdev)
{
	struct fb_info *fbi = fbdev->fb_info;
	struct fb_fix_screeninfo *fix = &fbi->fix;
	struct fb_var_screeninfo *var = &fbi->var;
	struct fb_ops *fbops = fbi->fbops;

	strncpy(fix->id, MTKFB_DRIVER, sizeof(fix->id));
	fix->type = FB_TYPE_PACKED_PIXELS;

	switch (var->bits_per_pixel) {
	case 16:
	case 24:
	case 32:
		fix->visual = FB_VISUAL_TRUECOLOR;
		break;
	case 1:
	case 2:
	case 4:
	case 8:
		fix->visual = FB_VISUAL_PSEUDOCOLOR;
		break;
	default:
		ASSERT(0);
	}

	fix->accel = FB_ACCEL_NONE;
	fix->line_length =
	    ALIGN_TO(var->xres_virtual, disphal_get_fb_alignment()) * var->bits_per_pixel / 8;
	fix->smem_len = fbdev->fb_size_in_byte;
	fix->smem_start = fbdev->fb_pa_base;

	fix->xpanstep = 0;
	fix->ypanstep = 1;

	fbops->fb_fillrect = cfb_fillrect;
	fbops->fb_copyarea = cfb_copyarea;
	fbops->fb_imageblit = cfb_imageblit;
}


/* Check values in var, try to adjust them in case of out of bound values if
 * possible, or return error.
 */
static int mtkfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fbi)
{
	unsigned int bpp;
	unsigned long max_frame_size;
	unsigned long line_size;

	struct mtkfb_device *fbdev = (struct mtkfb_device *)fbi->par;

	MSG_FUNC_ENTER();

	DISP_FB(
	    "xres=%u, yres=%u, xres_virtual=%u, yres_virtual=%u, xoffset=%u, yoffset=%u, bits_per_pixel=%u)\n",
	    var->xres, var->yres, var->xres_virtual, var->yres_virtual,
	    var->xoffset, var->yoffset, var->bits_per_pixel);

	bpp = var->bits_per_pixel;

	if (bpp != 16 && bpp != 24 && bpp != 32) {
		DISP_FB("[%s]unsupported bpp: %d", __func__, bpp);
		return -1;
	}

	switch (var->rotate) {
	case 0:
	case 180:
		var->xres = MTK_FB_XRES;
		var->yres = MTK_FB_YRES;
		break;
	case 90:
	case 270:
		var->xres = MTK_FB_YRES;
		var->yres = MTK_FB_XRES;
		break;
	default:
		return -1;
	}

	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;
	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;

	max_frame_size = fbdev->fb_size_in_byte;
	line_size = var->xres_virtual * bpp / 8;

	if (line_size * var->yres_virtual > max_frame_size) {
		/* Try to keep yres_virtual first */
		line_size = max_frame_size / var->yres_virtual;
		var->xres_virtual = line_size * 8 / bpp;
		if (var->xres_virtual < var->xres) {
			/* Still doesn't fit. Shrink yres_virtual too */
			var->xres_virtual = var->xres;
			line_size = var->xres * bpp / 8;
			var->yres_virtual = max_frame_size / line_size;
		}
	}
	if (var->xres + var->xoffset > var->xres_virtual)
		var->xoffset = var->xres_virtual - var->xres;
	if (var->yres + var->yoffset > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;

	if (16 == bpp) {
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
	} else if (24 == bpp) {
		var->red.length = var->green.length = var->blue.length = 8;
		var->transp.length = 0;

		/* Check if format is RGB565 or BGR565 */

		ASSERT(8 == var->green.offset);
		ASSERT(16 == var->red.offset + var->blue.offset);
		ASSERT(16 == var->red.offset || 0 == var->red.offset);
	} else if (32 == bpp) {
		var->red.length = var->green.length = var->blue.length = var->transp.length = 8;

		/* Check if format is ARGB565 or ABGR565 */

		ASSERT(8 == var->green.offset && 24 == var->transp.offset);
		ASSERT(16 == var->red.offset + var->blue.offset);
		ASSERT(16 == var->red.offset || 0 == var->red.offset);
	}

	var->red.msb_right = var->green.msb_right = var->blue.msb_right = var->transp.msb_right = 0;

	var->activate = FB_ACTIVATE_NOW;

	var->height = UINT_MAX;
	var->width = UINT_MAX;
	var->grayscale = 0;
	var->nonstd = 0;

	var->pixclock = UINT_MAX;
	var->left_margin = UINT_MAX;
	var->right_margin = UINT_MAX;
	var->upper_margin = UINT_MAX;
	var->lower_margin = UINT_MAX;
	var->hsync_len = UINT_MAX;
	var->vsync_len = UINT_MAX;

	var->vmode = FB_VMODE_NONINTERLACED;
	var->sync = 0;

	MSG_FUNC_LEAVE();
	return 0;
}


/* Switch to a new mode. The parameters for it has been check already by
 * mtkfb_check_var.
 */
static int mtkfb_set_par(struct fb_info *fbi)
{
	struct fb_var_screeninfo *var = &fbi->var;
	struct mtkfb_device *fbdev = (struct mtkfb_device *)fbi->par;
	struct fb_overlay_layer fb_layer;
	u32 bpp = var->bits_per_pixel;

	MSG_FUNC_ENTER();
	memset(&fb_layer, 0, sizeof(struct fb_overlay_layer));
	switch (bpp) {
	case 16:
		fb_layer.src_fmt = MTK_FB_FORMAT_RGB565;
		fb_layer.src_use_color_key = 1;
		fb_layer.src_color_key = 0xFF000000;
		break;

	case 24:
		fb_layer.src_use_color_key = 1;
		fb_layer.src_fmt = (0 == var->blue.offset) ?
		    MTK_FB_FORMAT_RGB888 : MTK_FB_FORMAT_BGR888;
		fb_layer.src_color_key = 0xFF000000;
		break;

	case 32:
		fb_layer.src_use_color_key = 0;
		fb_layer.src_fmt = (0 == var->blue.offset) ?
		    MTK_FB_FORMAT_ARGB8888 : MTK_FB_FORMAT_ABGR8888;
		fb_layer.src_color_key = 0;
		break;

	default:
		fb_layer.src_fmt = MTK_FB_FORMAT_UNKNOWN;
		DISP_FB("[%s]unsupported bpp: %d\n", __func__, bpp);
		return -1;
	}

	/* If the framebuffer format is NOT changed, nothing to do */
	/*  */
	if (fb_layer.src_fmt == fbdev->layer_format[FB_LAYER])
		goto Done;
	/* else, begin change display mode */
	/*  */
	set_fb_fix(fbdev);

	if (isAEEEnabled)
		fb_layer.layer_id = FB_LAYER-1;
	else
		fb_layer.layer_id = FB_LAYER;

	fb_layer.layer_enable = 1;
	fb_layer.src_base_addr =
	    (void *)((unsigned long)fbdev->fb_va_base + var->yoffset * fbi->fix.line_length);
	fb_layer.src_phy_addr = (void *)(fb_mva + var->yoffset * fbi->fix.line_length);
	DISP_FB("%s fb_mva:0x%x, yoffset:%d line_length:%d\n", __func__, fb_mva, var->yoffset, fbi->fix.line_length);
	fb_layer.src_direct_link = 0;
	fb_layer.src_offset_x = fb_layer.src_offset_y = 0;
#if defined(HWGPU_SUPPORT)
	fb_layer.src_pitch = ALIGN_TO(var->xres, MTK_FB_ALIGNMENT);
#else
	if (get_boot_mode() == META_BOOT || get_boot_mode() == FACTORY_BOOT
	    || get_boot_mode() == ADVMETA_BOOT || get_boot_mode() == RECOVERY_BOOT)
		fb_layer.src_pitch = ALIGN_TO(var->xres, MTK_FB_ALIGNMENT);
	else
		fb_layer.src_pitch = var->xres;
#endif
	fb_layer.src_width = fb_layer.tgt_width = var->xres;
	fb_layer.src_height = fb_layer.tgt_height = var->yres;
	fb_layer.tgt_offset_x = fb_layer.tgt_offset_y = 0;

/* fb_layer.src_color_key = 0; */
	fb_layer.layer_rotation = MTK_FB_ORIENTATION_0;
	fb_layer.layer_type = LAYER_2D;

	mutex_lock(&OverlaySettingMutex);
	mtkfb_set_overlay_layer((struct mtkfb_device *)fbi->par, &fb_layer);
	if (!isAEEEnabled) {
		fb_layer.layer_id = FB_LAYER - 1;
		fb_layer.layer_enable = 0;
		mtkfb_set_overlay_layer((struct mtkfb_device *)fbi->par, &fb_layer);
	}
	atomic_set(&OverlaySettingDirtyFlag, 1);
	atomic_set(&OverlaySettingApplied, 0);
	mutex_unlock(&OverlaySettingMutex);

	/* backup fb_layer information. */
	memcpy(&fb_layer_context, &fb_layer, sizeof(fb_layer));

Done:
	MSG_FUNC_LEAVE();
	return 0;
}


static int mtkfb_soft_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	return 0;
}

static int mtkfb_get_overlay_layer_info(struct fb_overlay_layer_info *layerInfo)
{
	DISP_LAYER_INFO layer;

	if (layerInfo->layer_id >= DDP_OVL_LAYER_MUN)
		return 0;
	layer.id = layerInfo->layer_id;
	DISP_GetLayerInfo(&layer);
	layerInfo->layer_enabled = layer.hw_en;
	layerInfo->curr_en = layer.curr_en;
	layerInfo->next_en = layer.next_en;
	layerInfo->hw_en = layer.hw_en;
	layerInfo->curr_idx = layer.curr_idx;
	layerInfo->next_idx = layer.next_idx;
	layerInfo->hw_idx = layer.hw_idx;
	layerInfo->curr_identity = layer.curr_identity;
	layerInfo->next_identity = layer.next_identity;
	layerInfo->hw_identity = layer.hw_identity;
	layerInfo->curr_conn_type = layer.curr_conn_type;
	layerInfo->next_conn_type = layer.next_conn_type;
	layerInfo->hw_conn_type = layer.hw_conn_type;

	DISP_FB("[FB Driver] mtkfb_get_overlay_layer_info():\n");
	DISP_FB
	    ("id=%u, layer en=%u, next_en=%u, curr_en=%u, hw_en=%u, next_idx=%u, curr_idx=%u, hw_idx=%u\n",
	     layerInfo->layer_id, layerInfo->layer_enabled, layerInfo->next_en, layerInfo->curr_en,
	     layerInfo->hw_en, layerInfo->next_idx, layerInfo->curr_idx, layerInfo->hw_idx);

	MMProfileLogEx(MTKFB_MMP_Events.LayerInfo[layerInfo->layer_id], MMProfileFlagPulse,
		       (layerInfo->next_idx << 16) + ((layerInfo->curr_idx) & 0xFFFF),
		       (layerInfo->hw_idx << 16) + (layerInfo->next_en << 8) +
		       (layerInfo->curr_en << 4) + layerInfo->hw_en);
	return 0;
}


static atomic_t capture_ui_layer_only = ATOMIC_INIT(0);
void mtkfb_capture_fb_only(bool enable)
{
	atomic_set(&capture_ui_layer_only, enable);
}

static int mtkfb_capture_framebuffer(struct fb_info *info, unsigned int pvbuf)
{
	int ret = 0;

	MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagStart, pvbuf, 0);
	if (down_interruptible(&sem_flipping)) {
		DISP_FB("[FB Driver] can't get semaphore:%d\n", __LINE__);
		MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagEnd, 0, 1);
		return -ERESTARTSYS;
	}
	sem_flipping_cnt--;
	mutex_lock(&ScreenCaptureMutex);

    /** LCD registers can't be R/W when its clock is gated in early suspend
	mode; power on/off LCD to modify register values before/after func.
    */
	if (is_early_suspended)
		disp_path_clock_on("mtkfb, capture");

	if (atomic_read(&capture_ui_layer_only)) {
		unsigned int w_xres = (unsigned short)fb_layer_context.src_width;
		unsigned int h_yres = (unsigned short)fb_layer_context.src_height;
		unsigned int pixel_bpp = info->var.bits_per_pixel / 8;	/* bpp is either 32 or 16 */
		unsigned int w_fb = (unsigned int)fb_layer_context.src_pitch;
		unsigned int fbsize = w_fb * h_yres * pixel_bpp;	/* frame buffer size */
		unsigned int fbaddress =
		    info->fix.smem_start + info->var.yoffset * info->fix.line_length;
		unsigned int mem_off_x = (unsigned short)fb_layer_context.src_offset_x;
		unsigned int mem_off_y = (unsigned short)fb_layer_context.src_offset_y;
		unsigned int fbv = 0;

		fbaddress += (mem_off_y * w_fb + mem_off_x) * pixel_bpp;
		fbv = (unsigned int)ioremap_nocache(fbaddress, fbsize);
		DISP_FB
	("[FB Driver], w_xres = %d, h_yres = %d, w_fb = %d, pixel_bpp = %d, fbsize = %d, fbaddress = 0x%08x\n",
		     w_xres, h_yres, w_fb, pixel_bpp, fbsize, fbaddress);
		if (!fbv) {
			DISP_FB
			    ("[FB Driver], Unable to allocate memory for frame buffer: address=0x%08x, size=0x%08x\n",
			     fbaddress, fbsize);
			goto EXIT;
		}
		{
			unsigned int i;

			for (i = 0; i < h_yres; i++)
				memcpy((void *)(pvbuf + i * w_xres * pixel_bpp),
				       (void *)(fbv + i * w_fb * pixel_bpp), w_xres * pixel_bpp);
		}
		iounmap((void *)fbv);
	} else
		DISP_Capture_Framebuffer(pvbuf, info->var.bits_per_pixel, is_early_suspended);


EXIT:
	if (is_early_suspended)
		disp_path_clock_off("mtkfb");

	mutex_unlock(&ScreenCaptureMutex);
	sem_flipping_cnt++;
	up(&sem_flipping);
	MSG_FUNC_LEAVE();
	MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagEnd, 0, 0);

	return ret;
}

void mtkfb_dump_layer_info(void)
{
#ifdef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
	unsigned int i;
	OVL_CONFIG_STRUCT cached_layer[DDP_OVL_LAYER_MUN] = { {0}, {0}, {0}, {0} };
	/*OVL_CONFIG_STRUCT captured_layer[DDP_OVL_LAYER_MUN] = { {0}, {0}, {0}, {0} };
	   OVL_CONFIG_STRUCT realtime_layer[DDP_OVL_LAYER_MUN] = { {0}, {0}, {0}, {0} }; */
	Disp_Ovl_Engine_Dump_layer_info(mtkfb_instance, &cached_layer[0], NULL, NULL);
	DISP_FB("[mtkfb] start dump layer info, early_suspend=%d\n", is_early_suspended);
	DISP_FB("[mtkfb] cache(next):\n");
	for (i = 0; i < DDP_OVL_LAYER_MUN; i++)
		DISP_FB("[mtkfb] layer=%d, layer_en=%d, idx=%d, fmt=%d, addr=0x%x, %d, %d, %d\n ",
			cached_layer[i].layer,	/* layer */
			cached_layer[i].layer_en, cached_layer[i].buff_idx, cached_layer[i].fmt,
			cached_layer[i].addr,	/* addr */
			cached_layer[i].identity,
			cached_layer[i].connected_type, cached_layer[i].security);
#else
	unsigned int i;

	DISP_FB("[mtkfb] start dump layer info, early_suspend=%d\n", is_early_suspended);
	DISP_FB("[mtkfb] cache(next):\n");
	for (i = 0; i < 4; i++)
		DISP_FB("[mtkfb] layer=%d, layer_en=%d, idx=%d, fmt=%d, addr=0x%x, %d, %d, %d\n ",
			cached_layer_config[i].layer,	/* layer */
			cached_layer_config[i].layer_en, cached_layer_config[i].buff_idx,
			cached_layer_config[i].fmt, cached_layer_config[i].addr,	/* addr */
			cached_layer_config[i].identity,
			cached_layer_config[i].connected_type, cached_layer_config[i].security);

	DISP_FB("[mtkfb] captured(current):\n");
	for (i = 0; i < 4; i++)
		DISP_FB("[mtkfb] layer=%d, layer_en=%d, idx=%d, fmt=%d, addr=0x%x, %d, %d, %d\n ",
			captured_layer_config[i].layer,	/* layer */
			captured_layer_config[i].layer_en, captured_layer_config[i].buff_idx,
			captured_layer_config[i].fmt, captured_layer_config[i].addr,	/* addr */
			captured_layer_config[i].identity,
			captured_layer_config[i].connected_type, captured_layer_config[i].security);
	DISP_FB("[mtkfb] realtime(hw):\n");
	for (i = 0; i < 4; i++)
		DISP_FB("[mtkfb] layer=%d, layer_en=%d, idx=%d, fmt=%d, addr=0x%x, %d, %d, %d\n ",
			realtime_layer_config[i].layer,	/* layer */
			realtime_layer_config[i].layer_en, realtime_layer_config[i].buff_idx,
			realtime_layer_config[i].fmt, realtime_layer_config[i].addr,
			realtime_layer_config[i].identity,
			realtime_layer_config[i].connected_type, realtime_layer_config[i].security);
#endif
	/* dump mmp data */
	/* mtkfb_aee_print("surfaceflinger-mtkfb blocked"); */

}

static char *_mtkfb_ioctl_spy(unsigned int cmd)
{
	switch (cmd) {
	case MTKFB_SET_OVERLAY_LAYER:
		return "MTKFB_SET_OVERLAY_LAYER";
	case MTKFB_TRIG_OVERLAY_OUT:
		return "MTKFB_TRIG_OVERLAY_OUT";
	case MTKFB_SET_VIDEO_LAYERS:
		return "MTKFB_SET_VIDEO_LAYERS";
	case MTKFB_CAPTURE_FRAMEBUFFER:
		return "MTKFB_CAPTURE_FRAMEBUFFER";
	case MTKFB_CONFIG_IMMEDIATE_UPDATE:
		return "MTKFB_CONFIG_IMMEDIATE_UPDATE";
	case MTKFB_SET_MULTIPLE_LAYERS:
		return "MTKFB_SET_MULTIPLE_LAYERS";
	case MTKFB_REGISTER_OVERLAYBUFFER:
		return "MTKFB_REGISTER_OVERLAYBUFFER";
	case MTKFB_UNREGISTER_OVERLAYBUFFER:
		return "MTKFB_UNREGISTER_OVERLAYBUFFER";
	case MTKFB_SET_ORIENTATION:
		return "MTKFB_SET_ORIENTATION";
	case MTKFB_FBLAYER_ENABLE:
		return "MTKFB_FBLAYER_ENABLE";
	case MTKFB_LOCK_FRONT_BUFFER:
		return "MTKFB_LOCK_FRONT_BUFFER";
	case MTKFB_UNLOCK_FRONT_BUFFER:
		return "MTKFB_UNLOCK_FRONT_BUFFER";
	case MTKFB_POWERON:
		return "MTKFB_POWERON";
	case MTKFB_POWEROFF:
		return "MTKFB_POWEROFF";
	case MTKFB_PREPARE_OVERLAY_BUFFER:
		return "MTKFB_PREPARE_OVERLAY_BUFFER";
	case MTKFB_SET_COMPOSING3D:
		return "MTKFB_SET_COMPOSING3D";
	case MTKFB_SET_S3D_FTM:
		return "MTKFB_SET_S3D_FTM";
	case MTKFB_GET_DEFAULT_UPDATESPEED:
		return "MTKFB_GET_DEFAULT_UPDATESPEED";
	case MTKFB_GET_CURR_UPDATESPEED:
		return "MTKFB_GET_CURR_UPDATESPEED";
	case MTKFB_CHANGE_UPDATESPEED:
		return "MTKFB_CHANGE_UPDATESPEED";
	case MTKFB_GET_INTERFACE_TYPE:
		return "MTKFB_GET_INTERFACE_TYPE";
	case MTKFB_GET_POWERSTATE:
		return "MTKFB_GET_POWERSTATE";
	case MTKFB_GET_DISPLAY_IF_INFORMATION:
		return "MTKFB_GET_DISPLAY_IF_INFORMATION";
	case MTKFB_AEE_LAYER_EXIST:
		return "MTKFB_AEE_LAYER_EXIST";
	case MTKFB_GET_OVERLAY_LAYER_INFO:
		return "MTKFB_GET_OVERLAY_LAYER_INFO";
	case MTKFB_FACTORY_AUTO_TEST:
		return "MTKFB_FACTORY_AUTO_TEST";
	case MTKFB_GET_FRAMEBUFFER_MVA:
		return "MTKFB_GET_FRAMEBUFFER_MVA";
	case MTKFB_SLT_AUTO_CAPTURE:
		return "MTKFB_SLT_AUTO_CAPTURE";
	case MTKFB_GETVFRAMEPHYSICAL:
		return "MTKFB_GETVFRAMEPHYSICAL";
	case MTKFB_WAIT_OVERLAY_READY:
		return "MTKFB_WAIT_OVERLAY_READY";
	case MTKFB_GET_OVERLAY_LAYER_COUNT:
		return "MTKFB_GET_OVERLAY_LAYER_COUNT";
	case MTKFB_GET_VIDEOLAYER_SIZE:
		return "MTKFB_GET_VIDEOLAYER_SIZE";
	case MTKFB_CAPTURE_VIDEOBUFFER:
		return "MTKFB_CAPTURE_VIDEOBUFFER";
	case MTKFB_TV_POST_VIDEO_BUFFER:
		return "MTKFB_TV_POST_VIDEO_BUFFER";
	case MTKFB_TV_LEAVE_VIDEO_PLAYBACK_MODE:
		return "MTKFB_TV_LEAVE_VIDEO_PLAYBACK_MODE";
	case MTKFB_IS_TV_CABLE_PLUG_IN:
		return "MTKFB_IS_TV_CABLE_PLUG_IN";
	case MTKFB_BOOTANIMATION:
		return "MTKFB_BOOTANIMATION";
	case MTKFB_GETFPS:
		return "MTKFB_GETFPS";
	case MTKFB_VSYNC:
		return "MTKFB_VSYNC";
	case MTKFB_FM_NOTIFY_FREQ:
		return "MTKFB_FM_NOTIFY_FREQ";
	case MTKFB_RESET_UPDATESPEED:
		return "MTKFB_RESET_UPDATESPEED";
	case MTKFB_SET_UI_LAYER_ALPHA:
		return "MTKFB_SET_UI_LAYER_ALPHA";
	case MTKFB_SET_UI_LAYER_SRCKEY:
		return "MTKFB_SET_UI_LAYER_SRCKEY";
	case MTKFB_GET_MAX_DISPLAY_COUNT:
		return "MTKFB_GET_MAX_DISPLAY_COUNT";
	case MTKFB_SET_FB_LAYER_SECURE:
		return "MTKFB_SET_FB_LAYER_SECURE";
	case MTKFB_META_RESTORE_SCREEN:
		return "MTKFB_META_RESTORE_SCREEN";
	case MTKFB_ERROR_INDEX_UPDATE_TIMEOUT:
		return "MTKFB_ERROR_INDEX_UPDATE_TIMEOUT";
	case MTKFB_ERROR_INDEX_UPDATE_TIMEOUT_AEE:
		return "MTKFB_ERROR_INDEX_UPDATE_TIMEOUT_AEE";
	case MTKFB_QUEUE_OVERLAY_CONFIG:
		return "MTKFB_QUEUE_OVERLAY_CONFIG";
	default:
		return "Invalid";
	}
}

void mtkfb_clear_lcm(void)
{
	int i;

#ifdef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
	struct fb_overlay_layer layer = { 0 };

	mutex_lock(&OverlaySettingMutex);
	for (i = 0; i < DDP_OVL_LAYER_MUN; i++) {
		if (isAEEEnabled && i == ASSERT_LAYER)
			continue;
		layer.layer_id = i;
		Disp_Ovl_Engine_Get_layer_info(mtkfb_instance, &layer);
		layer.layer_enable = 0;
		Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
	}
	mutex_unlock(&OverlaySettingMutex);
	DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update));
	Disp_Ovl_Engine_Wait_Overlay_Complete(mtkfb_instance, 1000);
#else
	mutex_lock(&OverlaySettingMutex);
	for (i = 0; i < DDP_OVL_LAYER_MUN; i++) {
		cached_layer_config[i].layer_en = 0;
		cached_layer_config[i].isDirty = 1;
	}
	atomic_set(&OverlaySettingDirtyFlag, 1);
	atomic_set(&OverlaySettingApplied, 0);
	mutex_unlock(&OverlaySettingMutex);

	DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update));
#endif
	if (!lcd_fps)
		msleep(60);
	else
		msleep(400000/lcd_fps);
}

int mtkfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	DISP_STATUS ret = 0;
	int r = 0, i;
	unsigned int from_v4l2 = 0;

	/* / M: dump debug mmprofile log info */
	MMProfileLogEx(MTKFB_MMP_Events.IOCtrl, MMProfileFlagPulse, _IOC_NR(cmd), arg);
	DISP_FB("mtkfb_ioctl, info=0x%08x, cmd=0x%08x(%s), arg=0x%08x, v4l2=%d\n",
		(unsigned int)info, (unsigned int)cmd, _mtkfb_ioctl_spy(cmd), (unsigned int)arg,
		from_v4l2);

	switch (cmd) {
	case MTKFB_QUEUE_OVERLAY_CONFIG:
		{
			struct fb_overlay_config config;

			if (copy_from_user(&config, (void __user *)arg, sizeof(config))) {
				DISP_FB("[FB]: copy_from_user failed! line: %d\n", __LINE__);
				return -EFAULT;
			}

			for (i = 0; i < HW_OVERLAY_COUNT; i++) {
				if (config.layers[i].layer_enable) {
					DISP_FB("MTKFB_QUEUE_OVERLAY_CONFIG:\n");
					DISP_FB("    fence:%d, time: 0x%x, layer%d en%d, next_idx=0x%x\n",
						  config.fence, config.time, config.layers[i].layer_id,
						  config.layers[i].layer_enable,
						  config.layers[i].next_buff_idx);
					DISP_FB
					    ("    vaddr=0x%x, paddr=0x%x, fmt=%u, d-link=%u, pitch=%u\n",
					     (unsigned int)(config.layers[i].src_base_addr),
					     (unsigned int)(config.layers[i].src_phy_addr),
					     config.layers[i].src_fmt,
					     (unsigned int)(config.layers[i].src_direct_link),
					     config.layers[i].src_pitch);
					DISP_FB
					    ("    xoff=%u, yoff=%u, w=%u, h=%u, alpha_en=%d, alpha=%d\n",
					     config.layers[i].src_offset_x, config.layers[i].src_offset_y,
					     config.layers[i].src_width, config.layers[i].src_height,
					     config.layers[i].alpha_enable, config.layers[i].alpha);
					DISP_FB("    fence_fd=%d, ion_fd=%d, security=%d\n",
						  config.layers[i].fence_fd, config.layers[i].ion_fd,
						  config.layers[i].security);
				}
			}

			if (!g_fb_pattern_en)
				r = mtkfb_queue_overlay_config_ex((struct mtkfb_device *)info->par,
								  &config, from_v4l2);
			else
				r = fb_pattern((struct mtkfb_device *)info->par, &config);

			if (copy_to_user((void __user *)arg, &config, sizeof(config))) {
				DISP_FB("[FB]: copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			return r;
		}
	case MTKFB_SET_OVERLAY_LAYER:
		{
			struct fb_overlay_layer layerInfo;

			DISP_FB(" mtkfb_ioctl():MTKFB_SET_OVERLAY_LAYER\n");

			if (copy_from_user(&layerInfo, (void __user *)arg, sizeof(layerInfo))) {
				DISP_FB("[FB]: copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else {
				/* in early suspend mode ,will not update buffer index, info SF by return value */
				if (is_early_suspended == true) {
					dev_err(fb_dev,
						"[FB] error, set overlay in early suspend ,skip!\n");
					return MTKFB_ERROR_IS_EARLY_SUSPEND;
				}

				mutex_lock(&OverlaySettingMutex);
				mtkfb_set_overlay_layer((struct mtkfb_device *)info->par,
							&layerInfo);
#ifndef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
				if (is_ipoh_bootup) {
					int i;

					for (i = 0; i < DDP_OVL_LAYER_MUN; i++)
						cached_layer_config[i].isDirty = 1;
					is_ipoh_bootup = false;
				}
				atomic_set(&OverlaySettingDirtyFlag, 1);
				atomic_set(&OverlaySettingApplied, 0);
#endif
				mutex_unlock(&OverlaySettingMutex);
			}
			return r;
		}
	case MTKFB_GET_FRAMEBUFFER_MVA:
		if (copy_to_user(argp, &fb_mva, sizeof(fb_mva))) {
			DISP_FB("[FB]: copy_to_user failed! line:%d\n", __LINE__);
			r = -EFAULT;
		}
		return r;
	case MTKFB_GET_DISPLAY_IF_INFORMATION:
		{
			int displayid = 0;

			if (copy_from_user(&displayid, (void __user *)arg, sizeof(displayid))) {
				DISP_FB("[FB]: copy_from_user failed! line:%d\n", __LINE__);
				return -EFAULT;
			}
			DISP_FB("MTKFB_GET_DISPLAY_IF_INFORMATION display_id=%d\n", displayid);
			if (displayid > MTKFB_MAX_DISPLAY_COUNT) {
				DISP_FB("[FB]: invalid display id:%d\n", displayid);
				return -EFAULT;
			}
			dispif_info[displayid].physicalHeight = DISP_GetActiveHeight();
			dispif_info[displayid].physicalWidth = DISP_GetActiveWidth();
			dispif_info[displayid].density = DISP_GetDensity();
			if (copy_to_user
			    ((void __user *)arg, &(dispif_info[displayid]),
			     sizeof(mtk_dispif_info_t))) {
				DISP_FB("[FB]: copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			return r;
		}
	case MTKFB_POWEROFF:
		{
			mtkfb_suspend();
			return r;
		}

	case MTKFB_POWERON:
		{
			mtkfb_resume();
			return r;
		}
	case MTKFB_GET_POWERSTATE:
		{
			unsigned long power_state;

			if (is_early_suspended == true)
				power_state = 0;
			else
				power_state = 1;

			return copy_to_user(argp, &power_state, sizeof(power_state)) ? -EFAULT : 0;
		}

	case MTKFB_CONFIG_IMMEDIATE_UPDATE:
		{
			DISP_FB("[%s] MTKFB_CONFIG_IMMEDIATE_UPDATE, enable = %lu\n",
				  __func__, arg);
			if (down_interruptible(&sem_early_suspend)) {
				DISP_FB("[mtkfb_ioctl] can't get semaphore:%d\n", __LINE__);
				return -ERESTARTSYS;
			}
			sem_early_suspend_cnt--;
			DISP_WaitForLCDNotBusy();
			ret = DISP_ConfigImmediateUpdate((bool) arg);
			sem_early_suspend_cnt++;
			up(&sem_early_suspend);
			return r;
		}

	case MTKFB_CAPTURE_FRAMEBUFFER:
		{
			unsigned int pbuf = 0;

			if (copy_from_user(&pbuf, (void __user *)arg, sizeof(pbuf))) {
				DISP_FB("[FB]: copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else {
				mtkfb_capture_framebuffer(info, pbuf);
			}

			return r;
		}

	case MTKFB_GET_OVERLAY_LAYER_INFO:
		{
			struct fb_overlay_layer_info layerInfo;

			DISP_FB(" mtkfb_ioctl():MTKFB_GET_OVERLAY_LAYER_INFO\n");

			if (copy_from_user(&layerInfo, (void __user *)arg, sizeof(layerInfo))) {
				DISP_FB("[FB]: copy_from_user failed! line:%d\n", __LINE__);
				return -EFAULT;
			}
			if (mtkfb_get_overlay_layer_info(&layerInfo) < 0) {
				DISP_FB("[FB]: Failed to get overlay layer info\n");
				return -EFAULT;
			}
			if (copy_to_user((void __user *)arg, &layerInfo, sizeof(layerInfo))) {
				DISP_FB("[FB]: copy_to_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			}
			return r;
		}

	case MTKFB_ERROR_INDEX_UPDATE_TIMEOUT:
		{
			DISP_FB("[DDP] mtkfb_ioctl():MTKFB_ERROR_INDEX_UPDATE_TIMEOUT\n");
			/* call info dump function here */
			mtkfb_dump_layer_info();
			return r;
		}

	case MTKFB_ERROR_INDEX_UPDATE_TIMEOUT_AEE:
		{
			DISP_FB("[DDP] mtkfb_ioctl():MTKFB_ERROR_INDEX_UPDATE_TIMEOUT\n");
			/* call info dump function here */
			mtkfb_dump_layer_info();
			mtkfb_aee_print("surfaceflinger-mtkfb blocked");
			return r;
		}

	case MTKFB_SET_VIDEO_LAYERS:
		{
			struct mmp_fb_overlay_layers {
				struct fb_overlay_layer Layer0;
				struct fb_overlay_layer Layer1;
				struct fb_overlay_layer Layer2;
				struct fb_overlay_layer Layer3;
			};
			struct fb_overlay_layer layerInfo[VIDEO_LAYER_COUNT];

			DISP_FB(" mtkfb_ioctl():MTKFB_SET_VIDEO_LAYERS\n");
			MMProfileLog(MTKFB_MMP_Events.SetOverlayLayers, MMProfileFlagStart);
			if (copy_from_user(&layerInfo, (void __user *)arg, sizeof(layerInfo))) {
				DISP_FB("[FB]: copy_from_user failed! line:%d\n", __LINE__);
				MMProfileLogMetaString(MTKFB_MMP_Events.SetOverlayLayers,
						       MMProfileFlagEnd, "Copy_from_user failed!");
				r = -EFAULT;
			} else {
				int32_t i;

				mutex_lock(&OverlaySettingMutex);
				for (i = 0; i < VIDEO_LAYER_COUNT; ++i)
					mtkfb_set_overlay_layer((struct mtkfb_device *)info->par,
								&layerInfo[i]);
				is_ipoh_bootup = false;
#ifndef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
				atomic_set(&OverlaySettingDirtyFlag, 1);
				atomic_set(&OverlaySettingApplied, 0);
#endif
				mutex_unlock(&OverlaySettingMutex);
				MMProfileLogStructure(MTKFB_MMP_Events.SetOverlayLayers,
						      MMProfileFlagEnd, layerInfo,
						      struct mmp_fb_overlay_layers);
			}

			return r;
		}

	case MTKFB_TRIG_OVERLAY_OUT:
		{
			DISP_FB(" mtkfb_ioctl():MTKFB_TRIG_OVERLAY_OUT\n");
			MMProfileLog(MTKFB_MMP_Events.TrigOverlayOut, MMProfileFlagPulse);
			return mtkfb_update_screen(info);
		}

/* #endif // MTK_FB_OVERLAY_SUPPORT */

	case MTKFB_SET_ORIENTATION:
		{
			DISP_FB("[MTKFB] Set Orientation: %lu\n", arg);
			/* surface flinger orientation definition of 90 and 270 */
			/* is different than DISP_TV_ROT */
			if (arg & 0x1)
				arg ^= 0x2;
			arg *= 90;
#if defined(MTK_HDMI_SUPPORT)
			hdmi_setorientation((int)arg);
#endif
			return 0;
		}
	case MTKFB_META_RESTORE_SCREEN:
		{
			struct fb_var_screeninfo var;

			if (copy_from_user(&var, argp, sizeof(var)))
				return -EFAULT;

			info->var.yoffset = var.yoffset;
			init_framebuffer(info);

			DISP_FB("[MTKFB] ioctl MTKFB_META_RESTORE_SCREEN\n");

			return mtkfb_pan_display_impl(&var, info);
		}

	case MTKFB_GET_INTERFACE_TYPE:
		{
			unsigned long lcm_type = lcm_params->type;

			DISP_FB("[MTKFB] MTKFB_GET_INTERFACE_TYPE\n");

			DISP_FB("[MTKFB EM]MTKFB_GET_INTERFACE_TYPE is %ld\n", lcm_type);

			return copy_to_user(argp, &lcm_type, sizeof(lcm_type)) ? -EFAULT : 0;
		}
	case MTKFB_GET_DEFAULT_UPDATESPEED:
		{
			unsigned int speed;

			DISP_FB("[MTKFB] get default update speed\n");
			DISP_Get_Default_UpdateSpeed(&speed);

			DISP_FB("[MTKFB EM]MTKFB_GET_DEFAULT_UPDATESPEED is %d\n", speed);
			return copy_to_user(argp, &speed, sizeof(speed)) ? -EFAULT : 0;
		}

	case MTKFB_GET_CURR_UPDATESPEED:
		{
			unsigned int speed;

			DISP_FB("[MTKFB] get current update speed\n");
			DISP_Get_Current_UpdateSpeed(&speed);

			DISP_FB("[MTKFB EM]MTKFB_GET_CURR_UPDATESPEED is %d\n", speed);
			return copy_to_user(argp, &speed, sizeof(speed)) ? -EFAULT : 0;
		}

	case MTKFB_CHANGE_UPDATESPEED:
		{
			unsigned int speed;

			DISP_FB("[MTKFB] change update speed\n");

			if (copy_from_user(&speed, (void __user *)arg, sizeof(speed))) {
				DISP_FB("[FB]: copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else {
				DISP_Change_Update(speed);

				DISP_FB("[MTKFB EM]MTKFB_CHANGE_UPDATESPEED is %d\n", speed);

			}
			return r;
		}

	case MTKFB_FBLAYER_ENABLE:
		{
			bool enable;

			if (copy_from_user(&enable, (void __user *)argp, sizeof(bool))) {
				DISP_FB("[FB]: copy_from_user failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else {
				DISP_FB("[FB]: FDLAYER_ENABLE:%d\n", enable);

				hwc_force_fb_enabled = (enable ? true : false);

				mutex_lock(&OverlaySettingMutex);
#ifdef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
				{
					struct fb_overlay_layer layer = { 0 };

					layer.layer_id = 0;
					Disp_Ovl_Engine_Get_layer_info(mtkfb_instance, &layer);
					layer.layer_enable = enable;
					Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
				}
#else
				cached_layer_config[FB_LAYER].layer_en = enable;
#endif
				if (mtkfb_using_layer_type != LAYER_2D) {
#ifdef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
					{
						struct fb_overlay_layer layer = { 0 };

						layer.layer_id = FB_LAYER + 1;
						Disp_Ovl_Engine_Get_layer_info(mtkfb_instance,
									       &layer);
						layer.layer_enable = enable;
						Disp_Ovl_Engine_Set_layer_info(mtkfb_instance,
									       &layer);
					}
#else
					cached_layer_config[FB_LAYER + 1].layer_en = enable;
#endif
				}
#ifndef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
				cached_layer_config[FB_LAYER].isDirty = true;
				atomic_set(&OverlaySettingDirtyFlag, 1);
				atomic_set(&OverlaySettingApplied, 0);
#endif
				mutex_unlock(&OverlaySettingMutex);


			}

			return r;
		}
	case MTKFB_AEE_LAYER_EXIST:
		{
			/* DISP_FB("[MTKFB] isAEEEnabled=%d\n", isAEEEnabled); */
			return copy_to_user(argp, &isAEEEnabled,
					    sizeof(isAEEEnabled)) ? -EFAULT : 0;
		}
	case MTKFB_LOCK_FRONT_BUFFER:
		return 0;
	case MTKFB_UNLOCK_FRONT_BUFFER:
		return 0;
/* ////////////////////////////////////////////// */
	default:
		dev_err(fb_dev, "mtkfb_ioctl 0x%08x(%s) Not support",
			(unsigned int)cmd,
			_mtkfb_ioctl_spy(cmd));
		return -EINVAL;
	}
}

#if defined(CONFIG_PM_AUTOSLEEP)
static int mtkfb_blank(int blank_mode, struct fb_info *info);
#endif

/* Callback table for the frame buffer framework. Some of these pointers
 * will be changed according to the current setting of fb_info->accel_flags.
 */
static struct fb_ops mtkfb_ops = {
	.owner = THIS_MODULE,
	.fb_open = mtkfb_open,
	.fb_release = mtkfb_release,
	.fb_setcolreg = mtkfb_setcolreg,
	.fb_pan_display = mtkfb_pan_display_proxy,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_cursor = mtkfb_soft_cursor,
	.fb_check_var = mtkfb_check_var,
	.fb_set_par = mtkfb_set_par,
	.fb_ioctl = mtkfb_ioctl,
#if defined(CONFIG_PM_AUTOSLEEP)
	.fb_blank = mtkfb_blank,
#endif
};

/*
 * ---------------------------------------------------------------------------
 * Sysfs interface
 * ---------------------------------------------------------------------------
 */

static int mtkfb_register_sysfs(struct mtkfb_device *fbdev)
{
	/*NOT_REFERENCED(fbdev);*/

	return 0;
}

static void mtkfb_unregister_sysfs(struct mtkfb_device *fbdev)
{
	/*NOT_REFERENCED(fbdev);*/
}

/*
 * ---------------------------------------------------------------------------
 * LDM callbacks
 * ---------------------------------------------------------------------------
 */
/* Initialize system fb_info object and set the default video mode.
 * The frame buffer memory already allocated by lcddma_init
 */
static int mtkfb_fbinfo_init(struct fb_info *info)
{
	struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;
	struct fb_var_screeninfo var;
	int r = 0;

	MSG_FUNC_ENTER();

	BUG_ON(!fbdev->fb_va_base);
	info->fbops = &mtkfb_ops;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->screen_base = (char *)fbdev->fb_va_base;
	info->screen_size = fbdev->fb_size_in_byte;
	info->pseudo_palette = fbdev->pseudo_palette;

	r = fb_alloc_cmap(&info->cmap, 16, 0);
	if (r != 0)
		dev_err(fb_dev, "unable to allocate color map memory\n");

	/* setup the initial video mode (RGB565) */

	memset(&var, 0, sizeof(var));

	var.xres = MTK_FB_XRES;
	var.yres = MTK_FB_YRES;
	var.xres_virtual = MTK_FB_XRESV;
	var.yres_virtual = MTK_FB_YRESV;

	var.bits_per_pixel = 16;

	var.red.offset = 11;
	var.red.length = 5;
	var.green.offset = 5;
	var.green.length = 6;
	var.blue.offset = 0;
	var.blue.length = 5;

	var.width = DISP_GetActiveWidth();
	var.height = DISP_GetActiveHeight();

	var.activate = FB_ACTIVATE_NOW;

	r = mtkfb_check_var(&var, info);
	if (r != 0)
		dev_err(fb_dev, "failed to mtkfb_check_var\n");

	info->var = var;

	r = mtkfb_set_par(info);
	if (r != 0)
		dev_err(fb_dev, "failed to mtkfb_set_par\n");

	MSG_FUNC_LEAVE();
	return r;
}

/* Release the fb_info object */
static void mtkfb_fbinfo_cleanup(struct mtkfb_device *fbdev)
{
	MSG_FUNC_ENTER();

	fb_dealloc_cmap(&fbdev->fb_info->cmap);

	MSG_FUNC_LEAVE();
}

#define RGB565_TO_ARGB8888(x)   \
	((((x) &   0x1F) << 3) |    \
	(((x) &  0x7E0) << 5) |    \
	(((x) & 0xF800) << 8) |    \
	(0xFF << 24))		/* opaque */

/* Init frame buffer content as 3 R/G/B color bars for debug */
static int init_framebuffer(struct fb_info *info)
{
	void *buffer = info->screen_base + info->var.yoffset * info->fix.line_length;

	/* clean whole frame buffer as black */
	memset(buffer, 0, info->screen_size);

	return 0;
}


/* Free driver resources. Can be called to rollback an aborted initialization
 * sequence.
 */
static void mtkfb_free_resources(struct mtkfb_device *fbdev, int state)
{
	int r = 0;

	switch (state) {
	case MTKFB_ACTIVE:
		r = unregister_framebuffer(fbdev->fb_info);
		ASSERT(0 == r);
		/* lint -fallthrough */
	case 5:
		mtkfb_unregister_sysfs(fbdev);
		/* lint -fallthrough */
	case 4:
		mtkfb_fbinfo_cleanup(fbdev);
		/* lint -fallthrough */
	case 3:
		DISP_CHECK_RET(DISP_Deinit());
		/* lint -fallthrough */
	case 2:
		dma_free_coherent(0, fbdev->fb_size_in_byte, fbdev->fb_va_base, fbdev->fb_pa_base);
		/* lint -fallthrough */
	case 1:
		dev_set_drvdata(fbdev->dev, NULL);
		framebuffer_release(fbdev->fb_info);
		/* lint -fallthrough */
	case 0:
		/* nothing to free */
		break;
	default:
		BUG();
	}
}


char mtkfb_lcm_name[256] = { 0 };

bool mtkfb_find_lcm_driver(void)
{
	bool ret = false;
	char *p, *q;

	p = strstr(saved_command_line, "lcm=");
	if (p == NULL)
		return DISP_SelectDevice(NULL);

	p += 4;
	if ((p - saved_command_line) > strlen(saved_command_line + 1)) {
		ret = false;
		goto done;
	}

	DISP_FB("%s, %s\n", __func__, p);
	q = p;
	while (*q != ' ' && *q != '\0')
		q++;

	memset((void *)mtkfb_lcm_name, 0, sizeof(mtkfb_lcm_name));
	strncpy((char *)mtkfb_lcm_name, (const char *)p, (int)(q - p));

	DISP_FB("%s, %s\n", __func__, mtkfb_lcm_name);
	if (DISP_SelectDevice(mtkfb_lcm_name))
		ret = true;

done:
	return ret;
}

void disp_get_fb_address(uint32_t *fbVirAddr, uint32_t *fbPhysAddr)
{
	struct mtkfb_device *fbdev = (struct mtkfb_device *)mtkfb_fbi->par;

	*fbVirAddr =
	    (uint32_t) fbdev->fb_va_base + mtkfb_fbi->var.yoffset * mtkfb_fbi->fix.line_length;
	*fbPhysAddr =
	    (uint32_t) fbdev->fb_pa_base + mtkfb_fbi->var.yoffset * mtkfb_fbi->fix.line_length;
}

static int mtkfb_fbinfo_modify(struct fb_info *info)
{
	struct fb_var_screeninfo var;
	int r = 0;

	MSG_FUNC_ENTER();

	memcpy(&var, &(info->var), sizeof(var));
	var.activate = FB_ACTIVATE_NOW;
	var.bits_per_pixel = 32;
	var.transp.offset = 24;
	var.transp.length = 8;
	var.red.offset = 16;
	var.red.length = 8;
	var.green.offset = 8;
	var.green.length = 8;
	var.blue.offset = 0;
	var.blue.length = 8;
	var.yoffset = MTK_FB_YRES;

	r = mtkfb_check_var(&var, info);
	if (r != 0)
		dev_err(fb_dev, "failed to mtkfb_check_var\n");

	info->var = var;

	r = mtkfb_set_par(info);
	if (r != 0)
		dev_err(fb_dev, "failed to mtkfb_set_par\n");

	MSG_FUNC_LEAVE();
	return r;
}

static void mtkfb_fb_565_to_8888(struct fb_info *fb_info)
{
	unsigned int xres = fb_info->var.xres;
	unsigned int yres = fb_info->var.yres;
	unsigned int x_virtual = ALIGN_TO(xres, disphal_get_fb_alignment());
	unsigned int fbsize = x_virtual * yres * 2;
	unsigned short *s = (unsigned short *)fb_info->screen_base;
	unsigned int *d = (unsigned int *)(fb_info->screen_base + fbsize * 2);
	unsigned short src_rgb565 = 0;
	int j = 0;
	int k = 0;

	DISP_FB("change default format RGB565 to ARGB8888\n");
	for (j = 0; j < yres; ++j) {
		for (k = 0; k < xres; ++k) {
			src_rgb565 = *s++;
			*d++ = RGB565_TO_ARGB8888(src_rgb565);
		}
		d += (ALIGN_TO(xres, MTK_FB_ALIGNMENT) - xres);
		s += (ALIGN_TO(xres, disphal_get_fb_alignment()) - xres);
	}
	s = (unsigned short *)fb_info->screen_base;
	d = (unsigned int *)(fb_info->screen_base + fbsize * 2);

	mtkfb_fbinfo_modify(fb_info);
	disp_path_wait_reg_update(0);

	/*
	* clear the first buffer, because surfaceflinger use it without fill data at the first time.
	* otherwise, it will show lk logo after loading logo
	*/
	memset(s, 0, fbsize * 2);
	DISP_FB("change format end\n");
}

struct tag_videolfb {
	u16 lfb_width;
	u16 lfb_height;
	u16 lfb_depth;
	u16 lfb_linelength;
	u32 lfb_base;
	u32 lfb_size;
	u8 red_size;
	u8 red_pos;
	u8 green_size;
	u8 green_pos;
	u8 blue_size;
	u8 blue_pos;
	u8 rsvd_size;
	u8 rsvd_pos;

};

unsigned int vramsize = 0;
phys_addr_t fb_base_pa = 0;
unsigned long fb_base_va = 0;

static int mtkfb_parse_tag_videolfb(void)
{
	struct tag_videolfb *videolfb_tag = NULL;
	const void *tag_ptr = NULL;
	static int is_videofb_parse_done;

	if (is_videofb_parse_done)
		return 0;

	if (of_chosen) {
		tag_ptr = (void *)of_get_property(of_chosen, "atag,videolfb", NULL);
		if (tag_ptr) {
			videolfb_tag = (struct tag_videolfb *)(tag_ptr + 8);
			if (videolfb_tag) {
				vramsize = videolfb_tag->lfb_size;
				fb_base_pa = videolfb_tag->lfb_base;
			} else {
				dev_err(fb_dev, "[DT][videolfb] videolfb_tag not found\n");
				return -1;
			}
		}
	} else {
		dev_err(fb_dev, "[DT][videolfb] of_chosen not found\n");
		return -1;
	}

	is_videofb_parse_done = 1;
	DISP_FB("[videolfb] fps        = %d\n", lcd_fps);
	DISP_FB("[videolfb] fb_base_pa = 0x%x\n", fb_base_pa);
	DISP_FB("[videolfb] vram       = 0x%x\n", vramsize);
	DISP_FB("[videolfb] lcmname    = %s\n", mtkfb_lcm_name);
	return 0;
}

/* #define MTKFB_UT */
#ifdef MTKFB_UT

static void _mtkfb_draw_block(unsigned long addr, unsigned int x, unsigned int y, unsigned int w,
			      unsigned int h, unsigned int color)
{
	int i = 0;
	int j = 0;
	unsigned long start_addr = addr + MTK_FB_XRESV * 4 * y + x * 4;

	pr_debug("@(%d,%d)addr=0x%lx, MTK_FB_XRESV=%d, draw_block start addr=0x%lx, w=%d, h=%d\n",
		 x, y, addr, MTK_FB_XRESV, start_addr, w, h);

	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++)
			*(unsigned int *)(start_addr + i * 4 + j * MTK_FB_XRESV * 4) = color;
	}
}

static int _mtkfb_internal_test(unsigned long va, unsigned int w, unsigned int h)
{
	/* this is for debug, used in bring up day */
	unsigned int i = 0, j, num;
	unsigned int color = 0;
	int _internal_test_block_size;
	unsigned int *pAddr = (unsigned int *)va;

	DISP_FB("UT starts @ va=0x%lx, w=%d, h=%d\n", va, w, h);
	DISP_FB("R\n");
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++)
			*(pAddr + i * w + j) = 0xFFFF0000U;
	}
	mtkfb_pan_display_impl(&mtkfb_fbi->var, mtkfb_fbi);
	msleep(1000);

	DISP_FB("G\n");
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++)
			*(pAddr + i * w + j) = 0xFF00FF00U;
	}
	mtkfb_pan_display_impl(&mtkfb_fbi->var, mtkfb_fbi);
	msleep(1000);

	DISP_FB("B\n");
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++)
			*(pAddr + i * w + j) = 0xFF0000FFU;
	}
	mtkfb_pan_display_impl(&mtkfb_fbi->var, mtkfb_fbi);
	msleep(1000);

	for (num = w / 8; num <= w / 4; num += w / 8) {
		_internal_test_block_size = num;
		for (i = 0; i < w * h / _internal_test_block_size / _internal_test_block_size; i++) {
			color = (i & 0x1) * 0xff;
			color += ((i & 0x2) >> 1) * 0xff00;
			color += ((i & 0x4) >> 2) * 0xff0000;
			color += 0xff000000U;
			_mtkfb_draw_block(va,
					  i % (w / _internal_test_block_size) *
					  _internal_test_block_size,
					  i / (w / _internal_test_block_size) *
					  _internal_test_block_size, _internal_test_block_size,
					  _internal_test_block_size, color);
		}
		mtkfb_pan_display_impl(&mtkfb_fbi->var, mtkfb_fbi);
		msleep(1000);
	}

	return 0;
}
#endif

#ifdef CONFIG_OF
phys_addr_t mtkfb_get_fb_base(void)
{
	mtkfb_parse_tag_videolfb();
	return fb_base_pa;
}
EXPORT_SYMBOL(mtkfb_get_fb_base);

size_t mtkfb_get_fb_size(void)
{
	mtkfb_parse_tag_videolfb();
	return vramsize;
}
EXPORT_SYMBOL(mtkfb_get_fb_size);
#endif


/* Called by LDM binding to probe and attach a new device.
 * Initialization sequence:
 *   1. allocate system fb_info structure
 *      select panel type according to machine type
 *   2. init LCD panel
 *   3. init LCD controller and LCD DMA
 *   4. init system fb_info structure
 *   5. init gfx DMA
 *   6. enable LCD panel
 *      start LCD frame transfer
 *   7. register system fb_info structure
 */

static int mtkfb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtkfb_device *fbdev = NULL;
	struct fb_info *fbi;
	int init_state;
	int r = 0;

	MSG_FUNC_ENTER();

	/* get parameter from lfb */
	mtkfb_parse_tag_videolfb();
	MTK_FB_XRES = DISP_GetScreenWidth();
	MTK_FB_YRES = DISP_GetScreenHeight();
	fb_xres_update = MTK_FB_XRES;
	fb_yres_update = MTK_FB_YRES;
	MTK_FB_BPP = DISP_GetScreenBpp();
	MTK_FB_PAGES = DISP_GetPages();
	DISP_FB("XRES=%d, YRES=%d\n", MTK_FB_XRES, MTK_FB_YRES);

	init_state = 0;
	fbi = framebuffer_alloc(sizeof(struct mtkfb_device), dev);
	if (!fbi) {
		dev_err(dev, "unable to allocate memory for device info\n");
		r = -ENOMEM;
		goto cleanup;
	}
	mtkfb_fbi = fbi;
	fb_dev = dev;
	fbdev = (struct mtkfb_device *)fbi->par;
	fbdev->fb_info = fbi;
	fbdev->dev = dev;
	fbdev->layer_format = vmalloc(sizeof(MTK_FB_FORMAT) * HW_OVERLAY_COUNT);
	if (!fbdev->layer_format) {
		r = -ENOMEM;
		goto cleanup;
	}
	memset(fbdev->layer_format, 0, sizeof(MTK_FB_FORMAT) * HW_OVERLAY_COUNT);
	dev_set_drvdata(dev, fbdev);

	init_state++;		/* 1 */
	/* Allocate and initialize video frame buffer */

	fbdev->fb_size_in_byte = MTK_FB_SIZEV;
	{
		struct resource res;

		res.start = fb_base_pa;
		res.end = fb_base_pa + vramsize - 1;
		disphal_enable_mmu(mtkfb_enable_mmu);
		disphal_allocate_fb(&res, &fbdev->fb_pa_base, (unsigned int *)&fbdev->fb_va_base,
				    &fb_mva);
	}

	DISP_FB("fb_pa_base: 0x%x, fb_va_base: 0x%x\n",
		  fbdev->fb_pa_base, (unsigned int)(fbdev->fb_va_base));

	if (!fbdev->fb_va_base) {
		dev_err(dev, "unable to allocate memory for frame buffer\n");
		r = -ENOMEM;
		goto cleanup;
	}
	init_state++;		/* 2 */

	init_waitqueue_head(&screen_update_wq);

	if (DISP_EsdRecoverCapbility()) {
		esd_recovery_task =
		    kthread_create(esd_recovery_kthread, NULL, "esd_recovery_kthread");

		if (IS_ERR(esd_recovery_task))
			DISP_FB("ESD recovery task create fail\n");
		else
			wake_up_process(esd_recovery_task);
	}

	/* Initialize Display Driver PDD Layer */
	if (DISP_STATUS_OK != DISP_Init((uint32_t) fbdev->fb_va_base, fb_mva, true)) {
		r = -ENOMEM;
		dev_err(dev, "DISP_Init fail\n");
		goto cleanup;
	}

	init_state++;		/* 3 */

#ifdef CONFIG_MTK_OVERLAY_ENGINE_SUPPORT
	disp_ovl_engine_init();

	Disp_Ovl_Engine_GetInstance(&mtkfb_instance, COUPLE_MODE);
	if (0xFF == mtkfb_instance) {
		dev_err(dev, "allocate overlay engine fail\n");
		goto cleanup;
	}

	disp_ovl_engine_indirect_link_overlay(fbdev->fb_va_base, (void *)fb_mva);

	/* Android native fence support */
	fbdev->update_ovls_wq = alloc_workqueue("mtkfb_ovls_wq",
						WQ_HIGHPRI | WQ_UNBOUND | WQ_MEM_RECLAIM, 1);

	if (!fbdev->update_ovls_wq) {
		dev_err(dev, "failed to create update_ovls_wq\n");
		return -ENODEV;
	}
#ifdef CONFIG_SYNC
	mutex_init(&fbdev->timeline_lock);
	fbdev->timeline = sw_sync_timeline_create("mtkfb");
	fbdev->timeline_max = 0;
	INIT_LIST_HEAD(&fbdev->pending_configs);
#endif
#ifdef CONFIG_MTK_ION
	fbdev->ion_client = ion_client_create(g_ion_device, "mtkfb");
	if (IS_ERR(fbdev->ion_client)) {
		dev_err(dev, "failed to create ion client\n");
		return -ENODEV;
	}
#endif
#endif

	/* Register to system */

	r = mtkfb_fbinfo_init(fbi);
	if (r) {
		dev_err(dev, "mtkfb_fbinfo_init fail\n");
		goto cleanup;
	}
	init_state++;		/* 4 */

	r = mtkfb_register_sysfs(fbdev);
	if (r) {
		dev_err(dev, "mtkfb_register_sysfs fail\n");
		goto cleanup;
	}
	init_state++;		/* 5 */

	r = register_framebuffer(fbi);
	if (r != 0) {
		dev_err(dev, "register_framebuffer failed\n");
		goto cleanup;
	}

	fbdev->state = MTKFB_ACTIVE;

	mtkfb_fb_565_to_8888(fbi);

#ifdef MTKFB_UT
	/* check BGRA color format */
	_mtkfb_internal_test((unsigned long)fbdev->fb_va_base + MTK_FB_XRES*MTK_FB_YRES*4, MTK_FB_XRES, MTK_FB_YRES);
#endif

	/* disp_dump_all_reg_info(); */
	DBG_Init();

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&mtkfb_early_suspend_handler);
#endif

	wake_lock_init(&mtkfb_wake_lock, WAKE_LOCK_SUSPEND, "mtkfb_wake_lock");
	is_wake_lock = false;

	MSG_FUNC_LEAVE();
	return 0;

cleanup:
	mtkfb_free_resources(fbdev, init_state);
	MSG_FUNC_LEAVE();
	return r;
}

/* Called when the device is being detached from the driver */
static int mtkfb_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtkfb_device *fbdev = dev_get_drvdata(dev);
	enum mtkfb_state saved_state = fbdev->state;

	MSG_FUNC_ENTER();
	/* FIXME: wait till completion of pending events */

	fbdev->state = MTKFB_DISABLED;
	mtkfb_free_resources(fbdev, saved_state);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&mtkfb_early_suspend_handler);
#endif

	DBG_Deinit();

	MSG_FUNC_LEAVE();
	return 0;
}

bool mtkfb_is_suspend(void)
{
	return is_early_suspended;
}
EXPORT_SYMBOL(mtkfb_is_suspend);

static void mtkfb_suspend(void)
{
	MSG_FUNC_ENTER();
	DISP_FB("enter mtkfb_suspend\n");
	mutex_lock(&ScreenCaptureMutex);
	MMProfileLog(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagStart);

#ifdef CONFIG_MTK_LEDS
	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, LED_OFF);
#endif
	if (down_interruptible(&sem_early_suspend)) {
		dev_err(fb_dev, "[FB Driver] can't get semaphore in mtkfb_early_suspend()\n");
		mutex_unlock(&ScreenCaptureMutex);
		return;
	}

	sem_early_suspend_cnt--;

	if (is_early_suspended) {
		is_early_suspended = true;
		sem_early_suspend_cnt++;
		up(&sem_early_suspend);
		DISP_FB("[FB driver] has been suspended\n");
		mutex_unlock(&ScreenCaptureMutex);
		return;
	}

	if (Disp_Ovl_Engine_Check_WFD_Instance()) {
		sem_early_suspend_cnt++;
		up(&sem_early_suspend);
		DISP_FB("[FB driver] wfd connect\n");
		mutex_unlock(&ScreenCaptureMutex);
		wake_lock(&mtkfb_wake_lock);
		is_wake_lock = true;
		return;
	}
#ifdef CONFIG_MTK_HDMI_SUPPORT
	hdmitx_suspend();
#endif

	MMProfileLog(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagStart);

	if (!lcd_fps)
		msleep(30);
	else
		msleep(2 * 100000 / lcd_fps);	/* Delay 2 frames. */

	is_early_suspended = true;
	DISP_PrepareSuspend();
	wait_dsi_engine_notbusy();
	DISP_CHECK_RET(DISP_PanelEnable(false));
	DISP_CHECK_RET(DISP_PowerEnable(false));

	disp_path_clock_off("mtkfb");

	sem_early_suspend_cnt++;
	up(&sem_early_suspend);
	mutex_unlock(&ScreenCaptureMutex);

	DISP_FB("leave mtkfb_suspend\n");
	MSG_FUNC_LEAVE();
}

static void mtkfb_resume(void)
{
	MSG_FUNC_ENTER();
	DISP_FB("enter mtkfb_resume\n");

	mutex_lock(&ScreenCaptureMutex);

	if (is_wake_lock) {
		wake_unlock(&mtkfb_wake_lock);
		is_wake_lock = false;
	}

	sem_early_suspend_cnt--;
	if (!is_early_suspended) {
		sem_early_suspend_cnt++;
		DISP_FB("has been resumed\n");
		mutex_unlock(&ScreenCaptureMutex);
		return;
	}

	if (down_interruptible(&sem_early_suspend)) {
		dev_err(fb_dev, "can't get semaphore in mtkfb_late_resume()\n");
		mutex_unlock(&ScreenCaptureMutex);
		MMProfileLog(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagEnd);
		return;
	}

	MMProfileLog(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagEnd);
	if (is_ipoh_bootup) {
		atomic_set(&OverlaySettingDirtyFlag, 0);
		disp_path_clock_on("mtkfb_resume, ipoh");
		is_ipoh_bootup = false;
		is_early_suspended = false;
		up(&sem_early_suspend);
		mutex_unlock(&ScreenCaptureMutex);
		return;
	}

	disp_path_clock_on("mtkfb_resume");
	DISP_CHECK_RET(DISP_PowerEnable(true));
	DISP_CHECK_RET(DISP_PanelEnable(true));

	is_early_suspended = false;
	if (is_ipoh_bootup)
		DISP_StartConfigUpdate();
	else
		mtkfb_clear_lcm();

	sem_early_suspend_cnt++;
	up(&sem_early_suspend);
	mutex_unlock(&ScreenCaptureMutex);
	MMProfileLog(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagEnd);

#ifdef CONFIG_MTK_HDMI_SUPPORT
	hdmitx_resume();
#endif
	if (BL_set_level_resume) {
		mtkfb_set_backlight_level(BL_level);
		BL_set_level_resume = false;
	}
#ifdef CONFIG_MTK_LEDS
	if (!disp_ovl_engine.bCouple)
		mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, LED_HALF);
#endif
	DISP_FB("leave mtkfb_resume\n");
	MSG_FUNC_LEAVE();
}

static void mtkfb_shutdown(struct platform_device *pdev)
{
	MSG_FUNC_ENTER();
	DISP_FB("mtkfb_shutdown()\n");
	mtkfb_suspend();
	DISP_FB("leave mtkfb_shutdown\n");
	MSG_FUNC_LEAVE();
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mtkfb_early_suspend(struct early_suspend *h)
{
	MSG_FUNC_ENTER();
	DISP_FB("enter early_suspend\n");
	mtkfb_suspend();
	DISP_FB("leave early_suspend\n");
	MSG_FUNC_LEAVE();
}

static void mtkfb_late_resume(struct early_suspend *h)
{
	MSG_FUNC_ENTER();
	DISP_FB("enter late_resume\n");
	mtkfb_resume();
	DISP_FB("leave late_resume\n");
	MSG_FUNC_LEAVE();
}

static struct early_suspend mtkfb_early_suspend_handler = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB,
	.suspend = mtkfb_early_suspend,
	.resume = mtkfb_late_resume,
};
#endif

#ifdef CONFIG_PM
static int mtkfb_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	BUG_ON(pdev == NULL);
	return 0;
}

static int mtkfb_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	BUG_ON(pdev == NULL);
	return 0;
}

static int mtkfb_pm_restore_noirq(struct device *device)
{
	disphal_pm_restore_noirq(device);
	is_ipoh_bootup = true;
	return 0;
}

#else
#define mtkfb_pm_suspend NULL
#define mtkfb_pm_resume  NULL
#define mtkfb_pm_restore_noirq NULL
#endif

#if defined(CONFIG_PM_AUTOSLEEP)
static void mtkfb_blank_suspend(void)
{
	MSG_FUNC_ENTER();
	mtkfb_suspend();
	MSG_FUNC_LEAVE();
}

static void mtkfb_blank_resume(void)
{
	MSG_FUNC_ENTER();
	mtkfb_resume();
	MSG_FUNC_LEAVE();
}

static int mtkfb_blank(int blank_mode, struct fb_info *info)
{
	if (get_boot_mode() == RECOVERY_BOOT)
		return 0;

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
	case FB_BLANK_NORMAL:
		mtkfb_blank_resume();
		if (!lcd_fps)
			msleep(30);
		else
			msleep(2 * 100000 / lcd_fps);	/* Delay 2 frames. */
#if defined(CONFIG_MTK_LEDS) && defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
		if (get_boot_mode() == KERNEL_POWER_OFF_CHARGING_BOOT ||
			get_boot_mode() == LOW_POWER_OFF_CHARGING_BOOT)
			mt65xx_leds_brightness_set(6, 255);
#endif
		break;
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
		break;
	case FB_BLANK_POWERDOWN:
#if defined(CONFIG_MTK_LEDS) && defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
		if (get_boot_mode() == KERNEL_POWER_OFF_CHARGING_BOOT ||
			get_boot_mode() == LOW_POWER_OFF_CHARGING_BOOT)
			mt65xx_leds_brightness_set(6, 0);
#endif
		mtkfb_blank_suspend();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
#endif


const struct dev_pm_ops mtkfb_pm_ops = {
	.suspend = mtkfb_pm_suspend,
	.resume = mtkfb_pm_resume,
	.freeze = mtkfb_pm_suspend,
	.thaw = mtkfb_pm_resume,
	.poweroff = mtkfb_pm_suspend,
	.restore = mtkfb_pm_resume,
	.restore_noirq = mtkfb_pm_restore_noirq,
};

static const struct of_device_id mtkfb_fb_dt_ids[] = {
	{.compatible = "mediatek,mt8127-fb",},
	{.compatible = "mediatek,mt8590-fb",},
	{.compatible = "mediatek,mt2701-fb",},
	{.compatible = "mediatek,mt7623-fb",},
	{}
};
MODULE_DEVICE_TABLE(of, mtkfb_fb_dt_ids);

static struct platform_driver mtkfb_driver = {
	.driver = {
		.name = MTKFB_DRIVER,
#ifdef CONFIG_PM
		.pm = &mtkfb_pm_ops,
#endif
		.of_match_table	= mtkfb_fb_dt_ids,
	},
	.probe = mtkfb_probe,
	.remove = mtkfb_remove,
	.shutdown = mtkfb_shutdown,
};

static int __init mtkfb_init(void)
{
	pr_info("Register mtkfb driver\n");
	if (platform_driver_register(&mtkfb_driver)) {
		pr_err("failed to register mtkfb driver\n");
		return -ENODEV;
	}

	/* In order to Trigger Display Customization Tool */
	ConfigPara_Init();
	return 0;
}

static void __exit mtkfb_exit(void)
{
	platform_driver_unregister(&mtkfb_driver);
	pr_info("Unregister mtkfb driver done\n");
}

late_initcall(mtkfb_init);
/* module_init(mtkfb_init); */
module_exit(mtkfb_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("mediatek framebuffer driver");
MODULE_LICENSE("GPL");
