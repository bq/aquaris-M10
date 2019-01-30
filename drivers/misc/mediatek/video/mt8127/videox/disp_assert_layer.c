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

#include <linux/types.h>
#include "disp_drv.h"
#include "ddp_hal.h"
#include "disp_drv_log.h"
#include "mtkfb_priv.h"
#include "disp_assert_layer.h"
#include <linux/semaphore.h>
#include <linux/mutex.h>

/* /common part */
#define DAL_BPP             (2)
#define DAL_WIDTH           (DISP_GetScreenWidth())
#define DAL_HEIGHT          (DISP_GetScreenHeight())

#ifdef CONFIG_MTK_FB_SUPPORT_ASSERTION_LAYER

#include <linux/string.h>
#include <linux/semaphore.h>
#include <asm/cacheflush.h>
#include <linux/module.h>

#include "mtkfb_console.h"

#if defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
#include "disp_ovl_engine_api.h"
#endif

/* --------------------------------------------------------------------------- */
#define DAL_FORMAT          (eRGB565)
#define DAL_BG_COLOR        (dal_bg_color)
#define DAL_FG_COLOR        (dal_fg_color)

#define RGB888_To_RGB565(x) ((((x) & 0xF80000) >> 8) |                      \
				(((x) & 0x00FC00) >> 5) |                      \
				(((x) & 0x0000F8) >> 3))

#define MAKE_TWO_RGB565_COLOR(high, low)  (((low) << 16) | (high))

DEFINE_SEMAPHORE(dal_sem);

void DAL_LOCK(void)
{
	do {
		if (down_interruptible(&dal_sem)) {
			DISP_LOG_PRINT(ANDROID_LOG_WARN, "DAL",
							"Can't get semaphore in %s()\n",
							__func__);
			return;
		}
	} while (0);
}

#define DAL_UNLOCK() up(&dal_sem)

#define DAL_CHECK_MFC_RET(expr)                                             \
	do {                                                                    \
		if (MFC_STATUS_OK != expr) {                                         \
			DISP_LOG_PRINT(ANDROID_LOG_WARN, "DAL", \
						"Warning: call MFC_XXX function failed "           \
						"in %s(), line: %d\n",              \
						__func__, __LINE__);                            \
		}                                                                   \
	} while (0)

#define DAL_LOG(fmt, arg...) DISP_LOG_PRINT(ANDROID_LOG_INFO, "DAL", fmt, ##arg)
/* --------------------------------------------------------------------------- */

static MFC_HANDLE mfc_handle;

static void *dal_fb_addr;
uint32_t dal_fb_pa = 0;
unsigned int isAEEEnabled = 0;

/* static bool  dal_shown   = false; */
bool dal_shown = false;
static bool dal_enable_when_resume;
static bool dal_disable_when_resume;
static unsigned int dal_fg_color = RGB888_To_RGB565(DAL_COLOR_WHITE);
static unsigned int dal_bg_color = RGB888_To_RGB565(DAL_COLOR_RED);

/* #define DAL_LOWMEMORY_ASSERT */

#ifdef DAL_LOWMEMORY_ASSERT

static unsigned int dal_lowmemory_fg_color = RGB888_To_RGB565(DAL_COLOR_PINK);
static unsigned int dal_lowmemory_bg_color = RGB888_To_RGB565(DAL_COLOR_YELLOW);
static bool dal_enable_when_resume_lowmemory;
static bool dal_disable_when_resume_lowmemory;
static bool dal_lowMemory_shown;
static const char low_memory_msg[] = { "Low Memory!" };

#define DAL_LOWMEMORY_FG_COLOR (dal_lowmemory_fg_color)
#define DAL_LOWMEMORY_BG_COLOR (dal_lowmemory_bg_color)
#endif
/* DECLARE_MUTEX(dal_sem); */

static char dal_print_buffer[1024];

#ifdef DAL_LOWMEMORY_ASSERT
static DAL_STATUS Show_LowMemory(void)
{
	uint32_t update_width, update_height;
	MFC_CONTEXT *ctxt = (MFC_CONTEXT *) mfc_handle;

	if (!dal_shown) {	/* only need show lowmemory assert */
		update_width = ctxt->font_width * strlen(low_memory_msg);
		update_height = ctxt->font_height;
		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DAL", "update size:%d,%d", update_width,
			       update_height);
#if defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
		{
			struct fb_overlay_layer layer = { 0 };

			layer.layer_id = ASSERT_LAYER;
			Disp_Ovl_Engine_Get_layer_info(mtkfb_instance, &layer);
			layer.src_offset_x = DAL_WIDTH - update_width;
			layer.src_offset_y = 0;
			layer.src_width = update_width;
			layer.src_height = update_height;
			layer.tgt_offset_x = DAL_WIDTH - update_width;
			layer.tgt_offset_y = 0;
			layer.tgt_width = update_width;
			layer.tgt_height = update_height;
			layer.layer_enable = true;
			Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
		}
#else
		mutex_lock(&OverlaySettingMutex);
		cached_layer_config[ASSERT_LAYER].src_x = DAL_WIDTH - update_width;
		cached_layer_config[ASSERT_LAYER].src_y = 0;
		cached_layer_config[ASSERT_LAYER].src_w = update_width;
		cached_layer_config[ASSERT_LAYER].src_h = update_height;
		cached_layer_config[ASSERT_LAYER].dst_x = DAL_WIDTH - update_width;
		cached_layer_config[ASSERT_LAYER].dst_y = 0;
		cached_layer_config[ASSERT_LAYER].dst_w = update_width;
		cached_layer_config[ASSERT_LAYER].dst_h = update_height;
		cached_layer_config[ASSERT_LAYER].layer_en = true;
		cached_layer_config[ASSERT_LAYER].isDirty = true;
		atomic_set(&OverlaySettingDirtyFlag, 1);
		atomic_set(&OverlaySettingApplied, 0);
		mutex_unlock(&OverlaySettingMutex);
#endif
	}

	return DAL_STATUS_OK;
}
#endif

uint32_t DAL_GetLayerSize(void)
{
	/* xuecheng, avoid lcdc read buffersize+1 issue */
	return DAL_WIDTH * DAL_HEIGHT * DAL_BPP + 4096;
}

int DAL_address_burst_align(void)
{
	int align = 0;
	int burst_mod = DAL_WIDTH * DAL_BPP % (8 * DAL_BPP);
	int burst = DAL_WIDTH * DAL_BPP / (8 * DAL_BPP) + (burst_mod ? 1 : 0);

	int burst_8mod = burst % 8;

	if (burst_8mod < 4)
		align = 0x40;

	DISP_MSG("dal address align 0x%x\n", align);
	return align;
}

DAL_STATUS DAL_SetScreenColor(DAL_COLOR color)
{
	uint32_t i;
	uint32_t size;
	uint32_t offset;
	uint32_t *addr;
	MFC_CONTEXT *ctxt;
	uint32_t BG_COLOR;

	color = RGB888_To_RGB565(color);

	BG_COLOR = MAKE_TWO_RGB565_COLOR(color, color);

	ctxt = (MFC_CONTEXT *) mfc_handle;
	if (!ctxt)
		return DAL_STATUS_FATAL_ERROR;
	if (ctxt->screen_color == color)
		return DAL_STATUS_OK;
	offset = MFC_Get_Cursor_Offset(mfc_handle);
	addr = (uint32_t *) ((uint32_t) ctxt->fb_addr + offset);
	size = DAL_GetLayerSize() - offset - DAL_address_burst_align();

	for (i = 0; i < size / sizeof(uint32_t); ++i)
		*addr++ = BG_COLOR;

	ctxt->screen_color = color;

	return DAL_STATUS_OK;
}
EXPORT_SYMBOL(DAL_SetScreenColor);

DAL_STATUS DAL_Init(uint32_t layerVA, uint32_t layerPA)
{
	int align;

	DISP_MSG("%s", __func__);
	align = DAL_address_burst_align();
	dal_fb_addr = (void *)(layerVA + align);
	dal_fb_pa = layerPA + align;
	DAL_CHECK_MFC_RET(MFC_Open(&mfc_handle, dal_fb_addr,
				   DAL_WIDTH, DAL_HEIGHT, DAL_BPP, DAL_FG_COLOR, DAL_BG_COLOR));

	/* DAL_Clean(); */
	DAL_SetScreenColor(DAL_COLOR_RED);

	return DAL_STATUS_OK;
}


DAL_STATUS DAL_SetColor(unsigned int fgColor, unsigned int bgColor)
{
	if (NULL == mfc_handle)
		return DAL_STATUS_NOT_READY;

	DAL_LOCK();
	dal_fg_color = RGB888_To_RGB565(fgColor);
	dal_bg_color = RGB888_To_RGB565(bgColor);
	DAL_CHECK_MFC_RET(MFC_SetColor(mfc_handle, dal_fg_color, dal_bg_color));
	DAL_UNLOCK();

	return DAL_STATUS_OK;
}
EXPORT_SYMBOL(DAL_SetColor);


DAL_STATUS DAL_Dynamic_Change_FB_Layer(unsigned int isAEEEnabled)
{
	static int ui_layer_tdshp;

	pr_info("[DDP] DAL_Dynamic_Change_FB_Layer, isAEEEnabled=%d\n", isAEEEnabled);

	if (DISP_DEFAULT_UI_LAYER_ID == DISP_CHANGED_UI_LAYER_ID) {
		pr_info("[DDP] DAL_Dynamic_Change_FB_Layer, no dynamic switch\n");
		return DAL_STATUS_OK;
	}

	if (isAEEEnabled == 1) {
#if defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
		{
			struct fb_overlay_layer layer = { 0 };

			layer.layer_id = DISP_DEFAULT_UI_LAYER_ID;
			Disp_Ovl_Engine_Get_layer_info(mtkfb_instance, &layer);
			layer.layer_id = DISP_CHANGED_UI_LAYER_ID;
			Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
			layer.layer_id = DISP_DEFAULT_UI_LAYER_ID;
			layer.isTdshp = 0;
			Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
		}
#else
		/* change ui layer from DISP_DEFAULT_UI_LAYER_ID to DISP_CHANGED_UI_LAYER_ID */
		memcpy((void *)(&cached_layer_config[DISP_CHANGED_UI_LAYER_ID]),
		       (void *)(&cached_layer_config[DISP_DEFAULT_UI_LAYER_ID]),
		       sizeof(OVL_CONFIG_STRUCT));
		ui_layer_tdshp = cached_layer_config[DISP_DEFAULT_UI_LAYER_ID].isTdshp;
		cached_layer_config[DISP_DEFAULT_UI_LAYER_ID].isTdshp = 0;
#endif
		/* change global variable value, else error-check will find layer 2, 3 enable tdshp together */
		disp_path_change_tdshp_status(DISP_DEFAULT_UI_LAYER_ID, 0);
		FB_LAYER = DISP_CHANGED_UI_LAYER_ID;
	} else {
#if defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
		{
			struct fb_overlay_layer layer = { 0 };

			layer.layer_id = DISP_CHANGED_UI_LAYER_ID;
			Disp_Ovl_Engine_Get_layer_info(mtkfb_instance, &layer);
			layer.layer_id = DISP_DEFAULT_UI_LAYER_ID;
			layer.isTdshp = ui_layer_tdshp;
			Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
		}
#else
		memcpy((void *)(&cached_layer_config[DISP_DEFAULT_UI_LAYER_ID]),
		       (void *)(&cached_layer_config[DISP_CHANGED_UI_LAYER_ID]),
		       sizeof(OVL_CONFIG_STRUCT));
		cached_layer_config[DISP_DEFAULT_UI_LAYER_ID].isTdshp = ui_layer_tdshp;
#endif
		FB_LAYER = DISP_DEFAULT_UI_LAYER_ID;
		memset((void *)(&cached_layer_config[DISP_CHANGED_UI_LAYER_ID]), 0,
		       sizeof(OVL_CONFIG_STRUCT));
	}

#if !defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
	/* no matter memcpy or memset, layer ID should not be changed */
	cached_layer_config[DISP_DEFAULT_UI_LAYER_ID].layer = DISP_DEFAULT_UI_LAYER_ID;
	cached_layer_config[DISP_CHANGED_UI_LAYER_ID].layer = DISP_CHANGED_UI_LAYER_ID;
	cached_layer_config[DISP_DEFAULT_UI_LAYER_ID].isDirty = 1;
	cached_layer_config[DISP_CHANGED_UI_LAYER_ID].isDirty = 1;
#endif

	return DAL_STATUS_OK;
}

DAL_STATUS DAL_Clean(void)
{
	MFC_CONTEXT *ctxt;
	DAL_STATUS ret = DAL_STATUS_OK;

	static int dal_clean_cnt;

	pr_info("[MTKFB_DAL] DAL_Clean\n");
	if (NULL == mfc_handle)
		return DAL_STATUS_NOT_READY;

	DAL_LOCK();

	DAL_CHECK_MFC_RET(MFC_ResetCursor(mfc_handle));


	ctxt = (MFC_CONTEXT *) mfc_handle;
	ctxt->screen_color = 0;
	DAL_SetScreenColor(DAL_COLOR_RED);

	if (down_interruptible(&sem_early_suspend)) {
		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DAL", "can't get semaphore in DAL_Clean()\n");
		goto End;
	}

#if !defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
	mutex_lock(&OverlaySettingMutex);
#endif

	/* TODO: if dal_shown=false, and 3D enabled, mtkfb may disable UI layer, please modify 3D driver */
	if (isAEEEnabled == 1) {
#if defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
		{
			int i;
			struct fb_overlay_layer layer = { 0 };

			for (i = 0; i < HW_OVERLAY_COUNT; i++) {
				layer.layer_id = i;
				layer.layer_enable = false;
				Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
			}
		}
#else
		cached_layer_config[ASSERT_LAYER].layer_en = false;
		cached_layer_config[ASSERT_LAYER].isDirty = true;
		DAL_Dynamic_Change_FB_Layer(isAEEEnabled);	/* restore UI layer to DEFAULT_UI_LAYER */
#endif
		/* DAL disable, switch UI layer to default layer 3 */
		pr_info("[DDP]* isAEEEnabled from 1 to 0, %d\n", dal_clean_cnt++);
		isAEEEnabled = 0;
	}

	dal_shown = false;
#ifdef DAL_LOWMEMORY_ASSERT
	if (dal_lowMemory_shown) {	/* only need show lowmemory assert */
		uint32_t LOWMEMORY_FG_COLOR =
		    MAKE_TWO_RGB565_COLOR(DAL_LOWMEMORY_FG_COLOR, DAL_LOWMEMORY_FG_COLOR);
		uint32_t LOWMEMORY_BG_COLOR =
		    MAKE_TWO_RGB565_COLOR(DAL_LOWMEMORY_BG_COLOR, DAL_LOWMEMORY_BG_COLOR);

		DAL_CHECK_MFC_RET(MFC_LowMemory_Printf
				  (mfc_handle, low_memory_msg, LOWMEMORY_FG_COLOR,
				   LOWMEMORY_BG_COLOR));
		Show_LowMemory();
	}
	dal_enable_when_resume_lowmemory = false;
	dal_disable_when_resume_lowmemory = false;
#endif
	dal_disable_when_resume = false;
#if !defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
	atomic_set(&OverlaySettingDirtyFlag, 1);
	atomic_set(&OverlaySettingApplied, 0);
	mutex_unlock(&OverlaySettingMutex);
#endif

	if (DISP_STATUS_OK != DISP_UpdateScreen(0, 0, DAL_WIDTH, DAL_HEIGHT)) {
		DISP_LOG_PRINT(ANDROID_LOG_WARN, "DAL",
		"Warning: call DISP_XXX function failed in %s(), line: %d, ret: %x\n",
					__func__, __LINE__, ret);
	}

	up(&sem_early_suspend);
End:
	DAL_UNLOCK();
	return ret;
}
EXPORT_SYMBOL(DAL_Clean);

DAL_STATUS DAL_Printf(const char *fmt, ...)
{
	va_list args;
	uint i;
	DAL_STATUS ret = DAL_STATUS_OK;

	pr_info("%s", __func__);
	if (NULL == mfc_handle)
		return DAL_STATUS_NOT_READY;

	if (NULL == fmt)
		return DAL_STATUS_INVALID_ARGUMENT;

	DAL_LOCK();
	if (down_interruptible(&sem_early_suspend)) {
		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DAL", "can't get semaphore in DAL_Printf()\n");
		goto End;
	}

	if (isAEEEnabled == 0) {
		pr_info("[DDP] isAEEEnabled from 0 to 1, ASSERT_LAYER=%d, dal_fb_pa %x\n",
			ASSERT_LAYER, dal_fb_pa);
		isAEEEnabled = 1;
		DAL_CHECK_MFC_RET(MFC_Open(&mfc_handle, dal_fb_addr,
				DAL_WIDTH, DAL_HEIGHT, DAL_BPP,
				DAL_FG_COLOR, DAL_BG_COLOR));
	}
	va_start(args, fmt);
	i = vsprintf(dal_print_buffer, fmt, args);
	BUG_ON(i >= ARRAY_SIZE(dal_print_buffer));
	va_end(args);
	DAL_CHECK_MFC_RET(MFC_Print(mfc_handle, dal_print_buffer));

	flush_cache_all();

	if (!dal_shown)
		dal_shown = true;

#if !defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
	DAL_Dynamic_Change_FB_Layer(isAEEEnabled);
	cached_layer_config[ASSERT_LAYER].addr = dal_fb_pa;
	cached_layer_config[ASSERT_LAYER].alpha = 0x80;
	cached_layer_config[ASSERT_LAYER].aen = true;
	cached_layer_config[ASSERT_LAYER].src_pitch = DAL_WIDTH * DAL_BPP;
	cached_layer_config[ASSERT_LAYER].fmt = DAL_FORMAT;
	cached_layer_config[ASSERT_LAYER].src_x = 0;
	cached_layer_config[ASSERT_LAYER].src_y = 0;
	cached_layer_config[ASSERT_LAYER].src_w = DAL_WIDTH;
	cached_layer_config[ASSERT_LAYER].src_h = DAL_HEIGHT;
	cached_layer_config[ASSERT_LAYER].dst_x = 0;
	cached_layer_config[ASSERT_LAYER].dst_y = 0;
	cached_layer_config[ASSERT_LAYER].dst_w = DAL_WIDTH;
	cached_layer_config[ASSERT_LAYER].dst_h = DAL_HEIGHT;
	cached_layer_config[ASSERT_LAYER].layer_en = true;
	cached_layer_config[ASSERT_LAYER].isDirty = true;
	mutex_lock(&OverlaySettingMutex);
	atomic_set(&OverlaySettingDirtyFlag, 1);
	atomic_set(&OverlaySettingApplied, 0);
	mutex_unlock(&OverlaySettingMutex);
#else
	if (!mtkfb_is_suspend()) {
		struct fb_overlay_layer layer = { 0 };

		layer.layer_id = ASSERT_LAYER;
		Disp_Ovl_Engine_Get_layer_info(mtkfb_instance, &layer);
		layer.src_phy_addr = (void *)dal_fb_pa;
		layer.alpha = 0x80;
		layer.alpha_enable = true;
		layer.src_pitch = DAL_WIDTH;
		layer.src_fmt = MTK_FB_FORMAT_RGB565;
		layer.src_offset_x = 0;
		layer.src_offset_y = 0;
		layer.src_width = DAL_WIDTH;
		layer.src_height = DAL_HEIGHT;
		layer.tgt_offset_x = 0;
		layer.tgt_offset_y = 0;
		layer.tgt_width = DAL_WIDTH;
		layer.tgt_height = DAL_HEIGHT;
		layer.layer_enable = true;
		Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
		layer.layer_id = ASSERT_LAYER - 1;
		layer.layer_enable = false;
		Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
		layer.layer_id = ASSERT_LAYER - 2;
		layer.layer_enable = false;
		Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
		layer.layer_id = ASSERT_LAYER - 3;
		layer.layer_enable = false;
		Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
	}
#endif

	if (DISP_STATUS_OK != DISP_UpdateScreen(0, 0, DAL_WIDTH, DAL_HEIGHT)) {
		DISP_LOG_PRINT(ANDROID_LOG_WARN, "DAL",
		"Warning: call DISP_XXX function failed in %s(), line: %d, ret: %x\n",
					__func__, __LINE__, ret);
	}

	up(&sem_early_suspend);
End:
	DAL_UNLOCK();

	return ret;
}
EXPORT_SYMBOL(DAL_Printf);

DAL_STATUS DAL_OnDispPowerOn(void)
{
	DAL_LOCK();

	/* Re-enable assertion layer when display resumes */

	if (mtkfb_is_suspend()) {
		if (dal_enable_when_resume) {
			dal_enable_when_resume = false;
			if (!dal_shown) {
#if defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
				{
					struct fb_overlay_layer layer = { 0 };

					layer.layer_id = ASSERT_LAYER;
					Disp_Ovl_Engine_Get_layer_info(mtkfb_instance, &layer);
					layer.src_offset_x = 0;
					layer.src_offset_y = 0;
					layer.src_width = DAL_WIDTH;
					layer.src_height = DAL_HEIGHT;
					layer.tgt_offset_x = 0;
					layer.tgt_offset_y = 0;
					layer.tgt_width = DAL_WIDTH;
					layer.tgt_height = DAL_HEIGHT;
					layer.layer_enable = true;
					Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
				}
#else
				mutex_lock(&OverlaySettingMutex);
				cached_layer_config[ASSERT_LAYER].src_x = 0;
				cached_layer_config[ASSERT_LAYER].src_y = 0;
				cached_layer_config[ASSERT_LAYER].src_w = DAL_WIDTH;
				cached_layer_config[ASSERT_LAYER].src_h = DAL_HEIGHT;
				cached_layer_config[ASSERT_LAYER].dst_x = 0;
				cached_layer_config[ASSERT_LAYER].dst_y = 0;
				cached_layer_config[ASSERT_LAYER].dst_w = DAL_WIDTH;
				cached_layer_config[ASSERT_LAYER].dst_h = DAL_HEIGHT;
				cached_layer_config[ASSERT_LAYER].layer_en = true;
				cached_layer_config[ASSERT_LAYER].isDirty = true;
				atomic_set(&OverlaySettingDirtyFlag, 1);
				atomic_set(&OverlaySettingApplied, 0);
				mutex_unlock(&OverlaySettingMutex);
#endif
				dal_shown = true;
			}
#ifdef DAL_LOWMEMORY_ASSERT
			dal_enable_when_resume_lowmemory = false;
			dal_disable_when_resume_lowmemory = false;
#endif
			goto End;
		} else if (dal_disable_when_resume) {
			dal_disable_when_resume = false;
#if defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
			{
				struct fb_overlay_layer layer = { 0 };

				layer.layer_id = ASSERT_LAYER;
				Disp_Ovl_Engine_Get_layer_info(mtkfb_instance, &layer);
				layer.layer_enable = false;
				Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
			}
#else
			mutex_lock(&OverlaySettingMutex);
			cached_layer_config[ASSERT_LAYER].layer_en = false;
			cached_layer_config[ASSERT_LAYER].isDirty = true;
			atomic_set(&OverlaySettingDirtyFlag, 1);
			atomic_set(&OverlaySettingApplied, 0);
			mutex_unlock(&OverlaySettingMutex);
#endif
			dal_shown = false;
#ifdef DAL_LOWMEMORY_ASSERT
			if (dal_lowMemory_shown) {	/* only need show lowmemory assert */
				uint32_t LOWMEMORY_FG_COLOR =
				    MAKE_TWO_RGB565_COLOR(DAL_LOWMEMORY_FG_COLOR,
							  DAL_LOWMEMORY_FG_COLOR);
				uint32_t LOWMEMORY_BG_COLOR =
				    MAKE_TWO_RGB565_COLOR(DAL_LOWMEMORY_BG_COLOR,
							  DAL_LOWMEMORY_BG_COLOR);

				DAL_CHECK_MFC_RET(MFC_LowMemory_Printf
						  (mfc_handle, low_memory_msg, LOWMEMORY_FG_COLOR,
						   LOWMEMORY_BG_COLOR));
				Show_LowMemory();
			}
			dal_enable_when_resume_lowmemory = false;
			dal_disable_when_resume_lowmemory = false;
#endif
			goto End;

		}
#ifdef DAL_LOWMEMORY_ASSERT
		if (dal_enable_when_resume_lowmemory) {
			dal_enable_when_resume_lowmemory = false;
			if (!dal_shown)	/* only need show lowmemory assert */
				Show_LowMemory();
		} else if (dal_disable_when_resume_lowmemory) {
			if (!dal_shown) {	/* only low memory assert shown on screen */
				if (!dal_shown) {	/* only low memory assert shown on screen */
#if defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
					{
						struct fb_overlay_layer layer = { 0 };

						layer.layer_id = ASSERT_LAYER;
						Disp_Ovl_Engine_Get_layer_info(mtkfb_instance,
									       &layer);
						layer.src_offset_x = 0;
						layer.src_offset_y = 0;
						layer.src_width = DAL_WIDTH;
						layer.src_height = DAL_HEIGHT;
						layer.tgt_offset_x = 0;
						layer.tgt_offset_y = 0;
						layer.tgt_width = DAL_WIDTH;
						layer.tgt_height = DAL_HEIGHT;
						layer.layer_enable = false;
						Disp_Ovl_Engine_Set_layer_info(mtkfb_instance,
									       &layer);
					}
#else
					mutex_lock(&OverlaySettingMutex);
					cached_layer_config[ASSERT_LAYER].src_x = 0;
					cached_layer_config[ASSERT_LAYER].src_y = 0;
					cached_layer_config[ASSERT_LAYER].src_w = DAL_WIDTH;
					cached_layer_config[ASSERT_LAYER].src_h = DAL_HEIGHT;
					cached_layer_config[ASSERT_LAYER].dst_x = 0;
					cached_layer_config[ASSERT_LAYER].dst_y = 0;
					cached_layer_config[ASSERT_LAYER].dst_w = DAL_WIDTH;
					cached_layer_config[ASSERT_LAYER].dst_h = DAL_HEIGHT;
					cached_layer_config[ASSERT_LAYER].layer_en = false;
					cached_layer_config[ASSERT_LAYER].isDirty = true;
					atomic_set(&OverlaySettingDirtyFlag, 1);
					atomic_set(&OverlaySettingApplied, 0);
					mutex_unlock(&OverlaySettingMutex);
#endif
				}
			}
#endif
		}

End:
		DAL_UNLOCK();

		return DAL_STATUS_OK;
	}
#ifdef DAL_LOWMEMORY_ASSERT
	DAL_STATUS DAL_LowMemoryOn(void)
{
		uint32_t LOWMEMORY_FG_COLOR =
		    MAKE_TWO_RGB565_COLOR(DAL_LOWMEMORY_FG_COLOR, DAL_LOWMEMORY_FG_COLOR);
		uint32_t LOWMEMORY_BG_COLOR =
		    MAKE_TWO_RGB565_COLOR(DAL_LOWMEMORY_BG_COLOR, DAL_LOWMEMORY_BG_COLOR);
		DAL_LOG("Enter DAL_LowMemoryOn()\n");

		DAL_CHECK_MFC_RET(MFC_LowMemory_Printf
				  (mfc_handle, low_memory_msg, LOWMEMORY_FG_COLOR,
				   LOWMEMORY_BG_COLOR));

		Show_LowMemory();

		dal_lowMemory_shown = true;
		DAL_LOG("Leave DAL_LowMemoryOn()\n");
		return DAL_STATUS_OK;
	}
	EXPORT_SYMBOL(DAL_LowMemoryOn);

	DAL_STATUS DAL_LowMemoryOff(void)
{
		uint32_t BG_COLOR = MAKE_TWO_RGB565_COLOR(DAL_BG_COLOR, DAL_BG_COLOR);

		DAL_LOG("Enter DAL_LowMemoryOff()\n");

		DAL_CHECK_MFC_RET(MFC_SetMem(mfc_handle, low_memory_msg, BG_COLOR));

/* what about LCM_PHYSICAL_ROTATION = 180 */
		if (!dal_shown) {	/* only low memory assert shown on screen */
#if defined(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT)
			{
				struct fb_overlay_layer layer = { 0 };

				layer.layer_id = ASSERT_LAYER;
				Disp_Ovl_Engine_Get_layer_info(mtkfb_instance, &layer);
				layer.src_offset_x = 0;
				layer.src_offset_y = 0;
				layer.src_width = DAL_WIDTH;
				layer.src_height = DAL_HEIGHT;
				layer.tgt_offset_x = 0;
				layer.tgt_offset_y = 0;
				layer.tgt_width = DAL_WIDTH;
				layer.tgt_height = DAL_HEIGHT;
				layer.layer_enable = false;
				Disp_Ovl_Engine_Set_layer_info(mtkfb_instance, &layer);
			}
#else
			mutex_lock(&OverlaySettingMutex);
			cached_layer_config[ASSERT_LAYER].src_x = 0;
			cached_layer_config[ASSERT_LAYER].src_y = 0;
			cached_layer_config[ASSERT_LAYER].src_w = DAL_WIDTH;
			cached_layer_config[ASSERT_LAYER].src_h = DAL_HEIGHT;
			cached_layer_config[ASSERT_LAYER].dst_x = 0;
			cached_layer_config[ASSERT_LAYER].dst_y = 0;
			cached_layer_config[ASSERT_LAYER].dst_w = DAL_WIDTH;
			cached_layer_config[ASSERT_LAYER].dst_h = DAL_HEIGHT;
			cached_layer_config[ASSERT_LAYER].layer_en = false;
			cached_layer_config[ASSERT_LAYER].isDirty = true;
			atomic_set(&OverlaySettingDirtyFlag, 1);
			atomic_set(&OverlaySettingApplied, 0);
			mutex_unlock(&OverlaySettingMutex);
#endif
		}
		dal_lowMemory_shown = false;
		DAL_LOG("Leave DAL_LowMemoryOff()\n");
		return DAL_STATUS_OK;
	}
	EXPORT_SYMBOL(DAL_LowMemoryOff);
#endif
/* ########################################################################## */
/* !CONFIG_MTK_FB_SUPPORT_ASSERTION_LAYER */
/* ########################################################################## */
#else

uint32_t DAL_GetLayerSize(void)
{
	/* xuecheng, avoid lcdc read buffersize+1 issue */
	return DAL_WIDTH * DAL_HEIGHT * DAL_BPP + 4096;
}

DAL_STATUS DAL_Init(uint32_t layerVA, uint32_t layerPA)
{
	/*NOT_REFERENCED(layerVA);*/
	/*NOT_REFERENCED(layerPA);*/

	return DAL_STATUS_OK;
}

DAL_STATUS DAL_SetColor(unsigned int fgColor, unsigned int bgColor)
{
	/*NOT_REFERENCED(fgColor);*/
	/*NOT_REFERENCED(bgColor);*/

	return DAL_STATUS_OK;
}
EXPORT_SYMBOL(DAL_SetColor);

DAL_STATUS DAL_Clean(void)
{
	pr_info("[MTKFB_DAL] DAL_Clean is not implemented\n");
	return DAL_STATUS_OK;
}
EXPORT_SYMBOL(DAL_Clean);

DAL_STATUS DAL_Printf(const char *fmt, ...)
{
	/*NOT_REFERENCED(fmt);*/
	pr_info("[MTKFB_DAL] DAL_Printf is not implemented\n");
	return DAL_STATUS_OK;
}
EXPORT_SYMBOL(DAL_Printf);

DAL_STATUS DAL_OnDispPowerOn(void)
{
	return DAL_STATUS_OK;
}

#ifdef DAL_LOWMEMORY_ASSERT
DAL_STATUS DAL_LowMemoryOn(void)
{
	return DAL_STATUS_OK;
}
EXPORT_SYMBOL(DAL_LowMemoryOn);

DAL_STATUS DAL_LowMemoryOff(void)
{
	return DAL_STATUS_OK;
}
EXPORT_SYMBOL(DAL_LowMemoryOff);

#endif

#endif				/* CONFIG_MTK_FB_SUPPORT_ASSERTION_LAYER */
