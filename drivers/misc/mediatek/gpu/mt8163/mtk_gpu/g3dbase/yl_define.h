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

#ifndef _YL_DEFINE_H_
#define _YL_DEFINE_H_

#include "g3dbase_common_define.h"

/* This is used to enable visual studio memory leakage detection */
/* #ifndef _CRTDBG_MAP_ALLOC */
/* #define _CRTDBG_MAP_ALLOC */
/* #endif */
#ifdef _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

/* IP reversion options */

/*-- memory allocation related macros --*/
#if defined(_DEBUG) && !defined(GLSL_CC_90)
#ifdef ANDROID_PERF_EXP
#define G3D_MALLOC  malloc
#define G3D_CALLOC  calloc
#define G3D_FREE    free
#define G3D_REALLOC realloc
#ifdef linux
#define G3D_STRDUP  strdup
#else
#define G3D_STRDUP  _strdup
#endif
#else /* !ANDROID_PERF_EXP */
#define G3D_MALLOC(x)       g3dMAlloc3(x, __FILE__, __func__ , __LINE__)
#define G3D_CALLOC(x, y)    g3dCAlloc3(x, y, __FILE__, __func__, __LINE__)
#define G3D_FREE(x)         g3dFree3(x, __FILE__, __func__, __LINE__)
#define G3D_REALLOC(x, y)   g3dReAlloc3(x, y, __FILE__, __func__, __LINE__)
#define G3D_STRDUP(x)       g3dStrDup3(x, __FILE__, __func__, __LINE__)
#endif /* ANDROID_PERF_EXP */
#else /* ! ( defined(_DEBUG) && !defined(GLSL_CC_90) ) */
#define G3D_MALLOC  malloc
#define G3D_CALLOC  calloc
#define G3D_FREE    free
#define G3D_REALLOC realloc
#ifdef linux
#define G3D_STRDUP  strdup
#else
#define G3D_STRDUP  _strdup
#endif
#endif /* defined(_DEBUG) && !defined(GLSL_CC_90) */

#define G3D_ALIGN(v, a) (((v) + (a) - 1) & ~((a) - 1))

#define YL_FAKE_WIN32_DISPLAY /* turn on this to modify win32 display */

/* #define YL_API_RECORD        //turn on this to enable recorder */
#ifdef YL_API_RECORD
#define _YL_RECORD_PACK /* turn on this to pack recorded data */
#endif

#ifdef _DEBUG
/* #define YL_DEBUG_ALL_TEX_FMT_TEST */
#ifdef YL_DEBUG_ALL_TEX_FMT_TEST
#define FILTER_MODE 1 /* NEAREST: 0 ; LINEAR: 1 */
#endif
/* #define YL_DEBUG_ALL_CBUF_FMT_TEST */
#endif

#ifdef _DEBUG
#define YL_DEBUG_SHADER
#define YL_REALTIME_DISPLAY
#define YL_DRD			/* driver register dump functionality */
#endif

/* For CoZ CModel to solve blend object sort issue */
#define BLEND_TILE_CHECK

/* for FBDEV EGL usage */
#ifdef FPGA_G3D_HW
#define FPGA_CONTEXT_SWITCH
#endif

#ifdef FPGA_CONTEXT_SWITCH
#define FBDEV_NUM_FRAME_DISPLAY 2
#else
#define FBDEV_NUM_FRAME_DISPLAY 1
#endif

#ifdef FPGA_G3D_HW
#define FPGA_RUN_FRAME_BY_FRAME
#if defined(ANDROID) && defined(YL_GRALLOC)
#define YL_DEBUG_MMU_TRANS_FAULT
#endif
#endif
#if defined(FPGA_RUN_FRAME_BY_FRAME) && !defined(G3D_NONCACHE_BUFFERS_WITH_GRALLOC_BUF_CACHED)
/* if MMU=WCP, performance is better if disable it */
/* if MMU!=WCP, performance is better if enable it */
#define ALWAYS_INVALIDATE_FRAME_BY_FRAME
#endif
/* #define YL_REAL_TIME_PPM_DUMP */

#ifdef FPGA_G3D_HW
#define YL_PERF_MAX_FRAME_QUEUED    3
#else
#define YL_PERF_MAX_FRAME_QUEUED    0
#endif

/* pattern related options */
/* #define YL_PATTERN */
#ifdef YL_PATTERN
#define YL_MISC_CHECK
#endif

#define YL_DEBUG_DUMP_IMG /* let img dump has a option, not use _DEBUG or DEBUG */
/* #define YL_DEBUG_MD5        //enable MD5 generation module */

#ifndef ANDROID_PREF_NDEBUG
#define YL_DUMP_IMG_ALPHA	/* if buffer has alpha info, also dump alpha into another ppm file */
    /* #define YL_DEBUG_PROBE      // enable this to dump driver probes */
#define YL_HW_DEBUG_DUMP
#define YL_HW_DEBUG_DUMP_TEMP	/* temp solution for simulation */
#endif /* ! ANDROID_PREF_NDEBUG */

#ifdef YL_PATTERN
/* #define TEXTURE_LOCAL_SIM */
#ifndef FPGA_G3D_HW
/* #define YL_REDUCE_PATTERN_SIZE //turn off for FPGA */
#endif /* FPGA_G3D_HW */
#endif

#define YL_FAKE_ADDR	0xABCD0000

#ifdef CMODEL
#define YL_G3DMEM_ADDRESS_TRANSLATION
#endif

/* #define FORCE_LINEAR */

#ifndef ANDROID_PREF_NDEBUG
/* #define YL_GET_AVERAGE_FPS */
#endif

#define GSSI

/* disable the binding flags of GL_PIXEL_PACK_BUFFER_ARB/ GL_PIXEL_UNPACK_BUFFER_ARB in st_bufferobj_data() */
#define DISABLE_PACK_UNPACK_BINDING_FLAGS

#define YL_GLSL_V06_32PXL  0
#define YL_GLSL_V08_32PXL  1
#define YL_GLSL_V08_64PXL  2

/* number of texture pixel pipeline */
#define TEX_PIXEL_PIPE 1

/* vertex list split */
#define VERTEX_LIST_SPLIT2
#define YL_SPLIT_FIX2

#define YL_DEFAULT_MAX_VERTEX_NUM 12
/* the maximum allowed vertex count in a list is calculated as:
 * (VERTEX_BUFFER_SIZE / max_possible_vertex_plus_index_size - max_compensation_vertex_count_per_split)
 * then find the integer that is multiple of 6 and nearest but no more than above value
 */
#ifdef YL_SPLIT_FIX2
#define YL_MAX_VERTEX_COUNT (((((VERTEX_BUFFER_SIZE_BASIC>>1) / ((PIPE_MAX_ATTRIBS * 4 * 4) + 4)) - 2) / 6) * 6)
#else
#define YL_MAX_VERTEX_COUNT ((((VERTEX_BUFFER_SIZE_BASIC / ((PIPE_MAX_ATTRIBS * 4 * 4) + 4)) - 2) / 6) * 6)
#endif
/* #define MAX_HW_ATTRIB 16 */

/* use cache for store the max/min index intead of calculate each draw */
#define YL_CACHE_MIN_MAX_INDEX

/* Flush sub frame lists when the accumulated draw count reach a threshold */
#define YL_SWSUBFB_DRAWCOUNT

/* #define YL_ZBUF_AA_OLD_FMT */
/* CModel fix for dEQP for better quality */
#define YL_1010102_FROM_1212128
#define YL_ZSRC_ALWAYS_CLAMP

/* */
#define YL_READ_PIXELS_UNSIGNED_BYTE

/* (eglMakeCurrent setting) When the visual format is z24x8, set renderbuffer internal format
 * GL_Z24 instead of GL_Z24S8. After that, it would choose z24x8 instead of z24s8 as the
 * renderbuffer format. [2013.08.05]
 */
#define YL_SET_GL_Z24_INSTEAD_OF_GL_Z24S8

/* HW just support RGB5A1. [2013.08.19] */
#define YL_CHOOSE_RGB5A1_INSTEAD_OF_A1RGB5

/* choose the RGBA4 format instead of ARGB4 sync with ES3.0 spec p.115.     [2013.10.17] */
#define YL_CHOOSE_RGBA4_INSTEAD_OF_ARGB4

/* choose the A2BGR10 format instead of A2RGB10 sync with ES3.0 spec p.115. [2013.10.17] */
#define YL_CHOOSE_A2BGR10_INSTEAD_OF_A2RGB10

/* Choose the XBGR8 format instead of XRGB8. This is work with yl.ini: chooseXBRG8InsteadOfXRGB8
 * It is just patched for choosing android/linux format.  [2014.3.04]
 */
/* #ifdef _DEBUG */
#define YL_CHOOSE_XBGR8_INSTEAD_OF_XRGB8
/* #endif */

/* We fix the path for the priority :1.PE  2.HW  3.SW                        [2013.10.28] */
#define YL_COPYTEXIMAGE2D_FIX

/* Change driver to not combine clears when triangle clear needed */
#define YL_DO_NOT_COMBINE_CLEARS_INTO_QUAD_CLEAR

/*-- tags for driver features --*/
#define YL_RES_ALIGN		/* driver to handle resource pitch alignment */
#define YL_TILE_ACCESS		/* driver to handle tiling mode data access */
#define YL_RCP_ZERO_PATCH	/* for 3DMM_Basemark_es3rc2 divide by zero issue */

/* not allocate memory from ringbuffer if the constant buffer is the same as previous one */
/* #define YL_SAME_CONSTBUF_DATA_WITH_SAME_ADDR */

/* #define YL_TEX_NAN_CHECK */
#define YL_DRAW_QUAD_BY_USER_VB	/* draw_quad with util_draw_user_vertex_buffer instead of util_draw_vertex_buffer */
/* #define YL_VS0_PACKING //patch VS0 attributes from interleaved buffer into a standalone buffer (for GLB2.5) */
/* #define YL_VS0_PACKING2 //to reduce the memory usage of patches of VS0 attributes (seems help not much) */
/* #define YL_VS0_PATCH 0x6735 // */
#define YL_SW_TFB_QUERY		/* the code about driver reports transform feedback query */
#define YL_FLIP_Y_COORD
#define YL_DEPTH_RW_BYPASS_HINT
#define YL_HW_VL
#define YL_HW_VL2
#define YL_SHADER_SOURCE_CHECKSUM	/* pass down shader source checksum to driver */
#define YL_DUMP_DS_IMG
#define YL_CLR_BUFFER		/* to support glClearBufferXX */
#define YL_CLR_BUFFER2		/* to support glClearBufferXX on triangle clear path */
/* #define YL_PRE_ROTATE //pre-rotation function: perform on setup */
#define YL_PRE_ROTATE2		/* pre-rotation function: perform on post */
#define YL_PRE_ROTATE_SW_SHIFT
#define YL_PRE_ROTATE_SW_SHIFT_NEW
/* #define YL_NO_ROTATE_WHEN_EGL_PRESERVED //no pre-rotation when egl preserved buffer is used */
#define YL_ROTATE_WH_ALIGN 32	/* 32 */
#define YL_ROTATE_PITCH_ALIGN 128	/* 32x4=128 */

#ifdef GS11
#define YL_ROTATE_YUV_WH_ALIGN 1	/* SPH:32 => ULC:1 */
#define YL_ROTATE_YUV_PITCH_ALIGN 16	/* SPH:64 => ULC:16 */
#else
#define YL_ROTATE_YUV_WH_ALIGN 32	/* SPH:32 => ULC:1 */
#define YL_ROTATE_YUV_PITCH_ALIGN 64	/* SPH:64 => ULC:16 */
#endif

#define YL_COARSEZ_FIX		/* the fix about binCZ enable */
#define YL_COARSEZ_FIX2		/* the fix about when to disable CZ */
#define YL_DISCARD_FRAMEBUFFER	/* to support glInvalidateFramebuffer and glDiscardFramebuffer */
#define YL_2CHANNEL_TUNING
#define YL_GAME_PATCH
#define YL_EGL_SYNC_FENCE
#define YL_SPLIT_FIX
#define YL_BUFFER_SIZE_FINAL
#define YL_UNIFIED_RINGBUFFER
#define YL_UNIFIED_RINGBUFFER_DYNAMIC
#define YL_PATCH_BY_SHADER

#define YL_WORKAROUND_POST_MAX_SIZE_MSAA_PATCH

#define YL_SW_TUNE_VP_PKT
#if !defined(Sapphire_ULC) && !defined(ES3_1_EP0)
#define YL_SW_TUNE_VP_VS0
#define YL_SW_TUNE_VP_VS1
#endif
#define YL_SW_TUNE_0420
#define YL_SW_TUNE_0422
#define YL_QUICK_STAMP2
#define YL_SW_TUNE_0425
#define YL_SW_TUNE_0427
#define YL_SW_TUNE_UNIFORM
#define YL_OPTIMIZE_SET_LIST_REG2
#define YL_SW_TUNE_0429
#define YL_SW_TUNE_0430		/* include UFO optimize */
#define YL_SW_TUNE_0504
#define YL_OPTIMIZE_SHADER_STAGE
#define YL_OPTIMIZE_API_VER
#define YL_OPTIMIZE_NONCACHE_READ
#define YL_ARRAY_LISTGLOBAL
#define YL_ARRAY_TEXBUF
#define YL_SW_TUNE_0504_2 /* pass */
#define YL_RING_BUFFER_TUNE /* pass */
#define YL_SW_TUNE_0505 /* pass */
#define YL_SW_TUNE_INPUT_BINDING /* pass */
#define YL_SW_TUNE_0508 /* pass: polygon_stipple, st_validate_state */
#define YL_SW_TUNE_0511 /* pass */
#define YL_SW_TUNE_0512 /* pass */
#define YL_SW_TUNE_BLEND /* pass */
#define YL_SW_TUNE_0513 /* pass */
#define YL_SW_TUNE_VALIDATION /* pass */
#define YL_SW_TUNE_0520 /* pass */
#define YL_OPTIMIZE_SHADER_CONSTANT
#define YL_OPTIMIZE_SHADER_PRIVATE

#define YL_WAITSTAMP_FIX
#define YL_GETSTAMP_FIX

/* #define YL_FRAME_FEATURE //generate a frame UID to identify that frame */
/* #define YL_PERF_TUNE_TREX */

/*
  the shader input stage may operate incorrectly if the lreg_vp0_vs0in_att_num = 0.
  driver may be able to do the patch by forcing lreg_vp0_vs0in_att_num to 1
  when driver finds lreg_vp0_vs0in_att_num = 0 happens
 */
#define YL_SAPPHIRE_PATCH_VS0_ZERO_IN

#define HW_SNAP
#define G3D_MAX_FENCE_WAIT 3

#ifndef CMODEL
/* #define YL_ENABLE_HW_NOFIRE */
/* #define YL_NOFIRE_FULL */
#endif

#define YL_DO_PE_CHKSUM
#define PE_CHKSUM_NONE          0
#define PE_CHKSUM_PER_SUBFRM    1
#define PE_CHKSUM_PER_SWAP      2

#define YL_SUPPORT_PRE_BOFF

#if !defined(SEMU) && !defined(SEMU64) && !defined(QEMU64) && !defined(FPGA_fpga8163) && !defined(FPGA_fpga8163_64)
#define YL_SUPPORT_UFO
#endif

#ifdef YL_FRAME_FEATURE
#define ADD_FEATURE(ctx, feature) \
	((ctx)->frameFeature = ctx->frameFeature * 31 + (feature))
#else
#define ADD_FEATURE(ctx, feature)
#endif

#ifdef YL_SUPPORT_UFO
/* #define YL_UFO_COMPRESS_BUF */

#ifdef YL_PATTERN /* if the requesting range doesn't cleared by glClear by current frame, */
			/* we do not allocate range for it, just pretend that it is a non-UFO range. */
#ifdef CMODEL
#if defined(WIN32)
#define YL_UFO_COMPRESS_BUF
#endif
#endif

    /* #define YL_UFO_NOT_DISABLE_AFTER_DECOMPRESS */
#endif

#define YL_CLEAR_USE_QUAD_WHEN_PARTIAL_CLEAR
#endif

#define YL_MAX_TEXTURE_LOD_BIAS 6	/* sw:16 hw:6 */

/* For driver command Queue & CMODEL defer */
#ifdef YL_PATTERN
#define MAX_GLOBAL_LIST     4096
#else
#define MAX_GLOBAL_LIST     1024
#endif

#define PROBE_FRAME_MAX 16
#define PROBE_PIXEL_MAX 16

/* --- added by redman.huang at 20110928 (for binwalk) ---// */
#define BIN_LTILE_XSIZE  64
#define BIN_BTILE_XSIZE  32
#define BIN_CTILE_XSIZE  16
#define BIN_DTILE_XSIZE  8
#define BIN_STILE_XSIZE  4

#define BIN_LTILE_YSIZE  64
#define BIN_BTILE_YSIZE  32
#define BIN_CTILE_YSIZE  16
#define BIN_DTILE_YSIZE  8
#define BIN_STILE_YSIZE  4
#define BIN_HTILE_YSIZE  2

#define BIN_LTILE_XMASK  63
#define BIN_LTILE_YMASK  63
#define BIN_BTILE_XMASK  31
#define BIN_BTILE_YMASK  31

#define BIN_STILE_XSHFT  2
#define BIN_STILE_YSHFT  2
#define BIN_HTILE_YSHFT  1

#define BIN_QTILE_XSIZE 2
#define BIN_QTILE_YSIZE 2
#define BIN_QTILE_XSHFT 1
#define BIN_QTILE_YSHFT 1

#define DEFER_BIN_SIZE 32
#define DEFER_BIN_BIT   5

/* Not support format
 */
#define YL_NOT_SUPPORT_Z32_FIXED	/* 2013.07.25 */
#define YL_NOT_SUPPORT_RGB8	/* 2014.07.04 */

#define YL_SCISSOR_CONSIDER_VIEWPORT	/* 2013.09.03 */

/* For dEQP, when the level of glFramebufferTexture2D > the level of glTexImage2D
 * they expected GL_NO_ERROR. However if the level of glFramebufferTexture2D > 0,
 * and do not call glTexImage2D, they expect GL_INVALID_VALUE.
 * (default is close) [2013.09.13]
 */
/* #define YL_DEQP_FBTEX_LEVEL */

/* For dEQP, if no glGen(Fremebuffer/Renderbuffer), just call
 * glBind(Fremebuffer/Renderbuffer), we would also create for them.
 * [2013.09.13]
 */
#define YL_DEQP_NOGEN_JUST_BIND

/* dump the subframe hex
 */
#define YL_DUMP_SUBFRAME_HEX

/* For copy the exactly attribute size, app may alloc the
 * exacly memory at client space. This may slow down performance
 */
#define YL_EXACTLY_COPY_ATTRIBUTE

/* To optimize to send dirty list register
 * Not to send all list register, just send the dirty register.
 * [2014.6.20]
 */
#define YL_OPTIMIZE_SET_LIST_REG


/* One global EGL configs set for different display.
 * This is for patch new conformance.
 */
#define YL_GLOBAL_EGL_CONFIG

/* This define is for new conformance, new confromance3 add more constrain.
 * Use the define to sync with new conformance3. If we open it, new
 * conformance would pass, but old conformace would fail.
 */
#define YL_PATCH_COPYTEX

/* When app call glDicardDb and then call validate state, we do not
 * want to call request display buffer. EX: On Android, it would not
 * dequeue buffer.
 */
#define YL_DISCARD_FB_NO_REQUEST_BUFFER

/* We would check the buffer offset whether exceed the buffer size.
 * However, it may enhance the driver loading. The spec. do not
 * mention that, we would do it by ourself, or we would hit MMU
 * translation fault. [apk: spiderman1] {2015.8.25}
 */
#define YL_CHECK_BUFFER_OBJECT_OVERRANGE

/* When we alloc mutisample "color" resource(texture/renderbuffer), we
 * just alloc 4x buffer color buffer, and do not alloc 1x buffer.
 * [2015.8.21]
 */
#define YL_MSAA_NOT_ALLOC_1X_BUFFER

#if !defined(Sapphire_ULC) && !defined(ES3_1_EP0)
/* Since HW do not deal the blending in MRT, some RT can blend,
 * and other(integer render target) cannot. The driver would
 * split two lists, one for blendable render targets, one for
 * unblendable reder target. [2015.3.28]
 */
#define YL_PATCH_HW_MRT_BLEND_INTEGER

/* The buffer mode condition: [2015.8.14]
 * GS41     : Would check the bpp <= 32
 * Others   : Would check the bin_size <= 4KB
 */
#define YL_BUFFER_MODE_CHECK_BPP

#endif /* !defined(Sapphire_ULC) && !defined(ES3_1_EP0) */

/* For pass the dEQP
 * Put the 3rd vertex at the right angle, so that two vetors of
 * barycenric are pure x or y directions.
 */
#define YL_PUT_THE_3TH_VERTEX_AT_RIGHT_ANGLE

/* to support EGL_BUFFER_PRESERVED. [2014.03.03] */
#define YL_BUFFER_PRESERVED_SUPPORT

/* to solved AntutuV5 2D, VBO */
#define YL_MAX_NUM_BUFFER_RENAME 8	/* power of 2 */

/* to identify the frame is for ... */
#define FB_NAME_FAKE_FRAME  0x13570	/* we gen a fake frame to render */
#define FB_NAME_HW_TRANSFER (FB_NAME_FAKE_FRAME + 0x3)	/* hw transfer */
#define FB_NAME_GEN_MIPMAP  (FB_NAME_FAKE_FRAME + 0x5)	/* gen mipmap */
#define FB_NAME_BLT_PIXEL   (FB_NAME_FAKE_FRAME + 0x9)	/* util_blit_pixel */

/* #define YL_MAP_TEST     // just for test */

/* -- HW base and pitch alignment --*/
#define G3D_COARSEZ_TILE_BASE_ALIGN    256
#define G3D_COARSEZ_LINEAR_BASE_ALIGN  16
#define G3D_TFT_TILE_BASE_ALIGN    256
#define G3D_TFT_LINEAR_BASE_ALIGN  16

#define G3D_TEXTURE_PITCH_ALIGN 16	/* alignment for linear texture */

#define G3D_CUBEMAP_TEXCOORD_CORRECT 0

#define G3D_TEXTURE_LINEAR_BASE_ALIGN  32
#define G3D_TEXTURE_TILE_BASE_ALIGN    32

#define G3D_BUFFER_PITCH_ALIGN 1
#define G3D_BUFFER_BASE_ALIGN  16

#ifdef YL_UNIFIED_RINGBUFFER
#define G3D_VERTEX_BASE_ALIGN          64 /* UNIFIED_RINGBUFFER_ALIGN */
#define G3D_TEXTURE_GLOBAL_BASE_ALIGN  64 /* UNIFIED_RINGBUFFER_ALIGN */
#define G3D_SHADERCONST_BASE_ALIGN     64 /* UNIFIED_RINGBUFFER_ALIGN */
#else
#define G3D_VERTEX_BASE_ALIGN          16
#define G3D_TEXTURE_GLOBAL_BASE_ALIGN  64
#define G3D_SHADERCONST_BASE_ALIGN     64 /* it shall be MAX(GSSI_IL1_BITS, GSSI_VPL1_BITS, GSSI_CPL1_BITS)/8 */
#endif
#define G3D_RINGBUFFER_BASE_ALIGN      16
#define G3D_SHADERINST_BASE_ALIGN      64
#define G3D_SHADERINST_BASE_ALIGN_8163E1_PATCH 2048
#define G3D_SHADERINST_BASE_ALIGN_DW_8163E1_PATCH 512
#define G3D_SHADERLOCAL_BASE_ALIGN     64
#define G3D_RESOURCE_ALIGN             64

#define G3D_NUM_SHADER_WAVES           24
#define G3D_NUM_SHADER_LANES           64

/* if optin is enabled, it means we will use dynamic flow to adjust private memory size */
#define G3D_PRIVATE_MEMORY_SLOT        3

/* 32 items x 16 bytes (vec4) x 64pixels x 24 (threads) = 0.75M */
#define G3D_PRIVATE_MEMORY_SIZE        (32*16*G3D_NUM_SHADER_LANES*G3D_NUM_SHADER_WAVES)

#define G3D_PRIVATE_MEMORY_ALIGN       64

#define LIST_INDEX_ITEM_SIZE           16

#define YL_MAX_QUERY_INDEX             128



/* NOTE: Sapphire tend to NOT support this, 20130729.  */
/* #define ENABLE_S8Z24_FORMAT //also for X8Z24 */

#define DEFER_RENDER_TO_BIN_RENDERBUFFERS
#if !defined(DEFER_RENDER_TO_BIN_RENDERBUFFERS)
#error DEFER_RENDER_TO_BIN_RENDERBUFFERS must be defined when enabling deferred mode and MSAA
#endif

 /**/
/* #define YL_TEMP_RB_BUFFER */
#ifdef YL_TEMP_RB_BUFFER
#define YL_TEMP_RB_BUFFER_FORCE	/* for test */
/* #define YL_TEMP_RB_BUFFER_DEBUG */
/* #define YL_TEMP_RB_BUFFER_PE_COPY //use PE instead of sw fallback */
/* #define YL_TEMP_RB_BUFFER_DIRTY_TX //tx buffer is always treated as dirty */
#endif

/* It's for rendering transfer path. Will not change for depth_stencil binding,
but will stil add render_target binding */
#define YL_NOT_CHANGE_PIPE_BIND_FLAG
/* align the pitch of mipmap texture to power-of-two */
#define YL_POT_PITCH_FOR_MIPMAP_TEXTURE
/* packed height for cube/2Darray/3D texture */
#define YL_PACKED_HEIGHT_FOR_MIPMAP_TEXTURE
/* work with SDK 2.0 Reflection */
/* #define YL_NPOT_TEST_FOR_CUBE */
/* if not defined, buffer will bigger than real need, but fit of hw need */
/* #define YL_EXACT_TEXTURE_SIZE */
/* a test to sync driver and c-model */
/* #define YL_TEXTURE_SIZE_TEST */
/* align every height to 16 instead of align hd */
/* #define YL_3D_TEX_PER_HEIGHT_TILE_ALIGN */
/* yuv texture with shader patch */
/* #define YL_TX_YUV_SHADER_PATCH */
/* allow target: GL_TEXTURE_EXTERNAL_OES in glTexImage2D */
/* #define YL_EXTERNAL_TEXTURE_IN_GL_API */
/* use hw rendering to transfer data */
#define YL_PBO_TO_TEX_RENDER_TRANSFER
/* #define YL_RB_TO_PBO_RENDER_TRANSFER */
/* pack texture index in the mapping table */
#define YL_TX_PACK_IDX
/* when prepare texture data, we may need to transfer level 0 data to new created mipmap */
/* #define YL_PE_FOR_LEVEL_0_TEXTURE_TRANS */
/* #define YL_3D_FOR_LEVEL_0_TEXTURE_TRANS */
/* when PE & VP use the same resource in multiple thread case, it is costy to
 * check frame command. So, we force all PE to wait previous frame when the
 * context is sharing. */
#define YL_PE_FORCE_WAIT_PREV_FRM_WHEN_SHARING_CTX
/* misc */
/* #define YL_FIND_UNSET_REG_MAGIC_NUM    0x5A */
#define YL_FIND_UNSET_REG_DUMP_FILE    "unset_reg.txt"
#define G3D_CMD_OP_NULL         0x00000000
#define G3D_CMD_OP_NULL_ECC_A       0x000A0A0A
#define G3D_CMD_OP_NULL_ECC_B       0x0A0A0A0A
#define G3D_CMD_OP_SINGLE       0x01000000
#define G3D_CMD_OP_SINGLE_ECC_A     0x001A0000
#define G3D_CMD_OP_BURST        0x02000000
#define G3D_CMD_OP_BURST_ECC_A      0x002A0000
#define G3D_CMD_OP_VL_PACKET    0x03000000
#define G3D_CMD_OP_VL_PACKET_ECC_A  0x003A0000
#define G3D_CMD_OP_END          0xff000000
#define G3D_CMD_OP_END_ECC_A        0x00FA0000

/* For list-base use (every list should have this flag, and for the last list of the frame,
 * it should also include G3D_CMD_END_FRAME_END)
 */
#define G3D_CMD_END_LIST_FLUSH              (1<<0)

/* The flag means frame boundary (both for swap-buffer frame or sub-frame) */
#define G3D_CMD_END_FRAME_END               (1<<1)

#define G3D_CMD_END_FRAME_SWAP              (1<<2) /* The flag is needed for swap-buffer frame */

/* The flag is for CL and defer mode, G3D will wait for previous frame done first, then process current frame */
#define G3D_CMD_END_WAIT_PREV_FRAME_READY   (1<<3)

#define G3D_CMD_END_SYNC_MMCQ               (1<<4) /* Notify "frame end signal" to MMCE */
/* #define SW_GUARDBAND */
#define YL_SHADER_WRITE_DEPTH_FIX
#define YL_TREAT_STENCIL_AS_COLOR
/* #define YL_WRITE_DEPTH_TO_RGBA */
/* #define YL_TFB_FORCE_FRAME_FLUSH */
/* YL_FORCE_TRIANGLE_CLEAR_AFTER_FAST_CLEAR refers to when an application clears,
 * draws, and then clears again (maybe draws again too), we make the later clears
 * triangle clears instead of flushing and creating a SW subframe. This will
 * reduce the colour/depth buffer memory bandwidth, but may decrease overall
 * performance due to processing more pixels.
 */
/* #define YL_FORCE_TRIANGLE_CLEAR_AFTER_FAST_CLEAR */
#ifdef YL_GRALLOC
#ifndef ANDROID_PREF_NDEBUG
#define GRALLOC_DEBUG		/* debug for yoli gralloc modules */
#endif
#define YL_GRALLOC_LOCK
#define DEQUEUE_BUFFER_ONLY_WHEN_DRAW
#define FENCE_DEFER_WAIT
#endif
/* ========= YUV:Start ======================================================*/
/* gralloc yuv buffer size patch: 1 or 0 */
#define YL_YUV_SIZE_PATCH 1
/* YV12 */
#define YL_YUV_YV12_Y_STRIDE 32
#define YL_YUV_YV12_UV_STRIDE 16
#define YL_YUV_YV12_HEIGHT_STRIDE 1
/* YUYV */
#define YL_YUV_YUYV_STRIDE 32
#define YL_YUV_YUYV_HEIGHT_STRIDE 1
/* For YV12 & YUYV formats, Sapphire GPU has a bigger width & height stride than others,
 * so we need two new formats to represents them: HAL_PIXEL_FORMAT_SPH0(for YV12) & HAL_PIXEL_FORMAT_SPH1(for YUYV).
 * and for PIPE_FORMAT, we use this flag to tell the difference: PIPE_BIND_YUV_SPH_RB */
#if !(defined(FPGA_fpga8163) || defined(FPGA_fpga8163_64))
#define YL_YUV_SPH_FORMAT
#endif
#ifdef YL_YUV_SPH_FORMAT
/* SPH0 => YV12 out */
#define YL_YUV_SPH0_Y_STRIDE YL_ROTATE_YUV_PITCH_ALIGN
#define YL_YUV_SPH0_UV_STRIDE YL_ROTATE_YUV_PITCH_ALIGN
#define YL_YUV_SPH0_HEIGHT_STRIDE YL_ROTATE_YUV_WH_ALIGN
/* SPH1 => YUYV out */
#define YL_YUV_SPH1_STRIDE YL_ROTATE_YUV_PITCH_ALIGN
#define YL_YUV_SPH1_HEIGHT_STRIDE YL_ROTATE_YUV_WH_ALIGN
#endif
/* YUV_PRIVATE format is not a real format, it's only an alias of some yuv formats.
 * When YUV_PRIVATE format is import into GPU, we will query it's real yuv format, and use it in GPU driver.
 * But in some cases, we still need to tell the difference of this real yuv format
 * between the normal ones and the ones came from YUV_PRIVATE.
 * When this define is on, we use the flag: PIPE_BIND_YUV_PRIVATE_BUFFER to tell the differences.*/
#define YL_YUV_PRIVATE_FORMAT_PIPE_BIND
/* ========= YUV:End ========================================================*/
/* formats for YUV, move here temporarily */
#define YL_CP_FMT_UYVY          (7|(0<<3))
#define YL_CP_FMT_YUYV          (7|(1<<3))
#define YL_CP_FMT_YVYU          (7|(2<<3))
#define YL_CP_FMT_VYUY          (7|(3<<3))
#define YL_CP_FMT_YV12          (7|(4<<3))
/* use it for st_api.h, sp_flush.h and yl_flush.h */
#define YL_FLUSH_FRONT          (0x1 << 0)
#define YL_FLUSH_SWAPBUFFER     (0x1 << 1)	/* to hint flush is called from eglSwapBuffers */
#define YL_FLUSH_FORCE          (0x1 << 2)	/* to hint to force flush G3d commands to HW */
#define YL_FLUSH_FRAMEND        (0x1 << 3)	/* to hint this flush located at frame end */
#define YL_FLUSH_READPIXEL      (0x1 << 4)
#define YL_FLUSH_RESET_UFO_ALL  (0x1 << 5)
#define YL_FLUSH_WAIT           (0x1 << 6)
#define YL_FLUSH_GEN_FRAME      (0x1 << 7)
#define YL_FLUSH_FOR_PE         (0x1 << 8)
#define YL_FLUSH_TEXTURE_CACHE  (0x1 << 16)
/* #define YL_FLUSH_MSK          (YL_FLUSH_SWAPBUFFER | YL_FLUSH_FORCE | YL_FLUSH_FRAMEND) */
#define CMODEL_UFO_EN
/* [FPGA define] */
/* #define G3D_RECOVERY_SIGNAL */
/* #define G3D_CL_PRINT_SIGNAL */
/* #define G3D_RECOVERY_TEST */
#define G3D_DIRECT_DRAW_MMSYS
#define G3D_ENABLE_AUTO_RECOVERY_SIGNAL  1
#define G3D_ENABLE_CL_PRINT_SIGNAL       2
#define G3D_DISABLE_AUTO_RECOVERY_SIGNAL 3
#define G3D_DISABLE_CL_PRINT_SIGNAL      4
#define YL_NO_POS_PERSPECTIVE_WHEN_2D
#define YL_NO_TEX_ROJECTION_WHEN_2D
/* #define YL_ANTU2D_VBO_PATCH // disable vbo to copy data to ringbuffer */

/* Mainly allow integer format use memcpy to copy data */
#define YL_PIXEL_TRANSFER_ALLOW_INT_MEMCPY

/* [MT8163 definition] */
#if defined(MT8163)
/* #undef  YL_SUPPORT_UFO */
#endif

/* [ES3.1 definition] */
#define ES3_1_INDIRECT_DRAW
/* To patch HW read indirect buffer data using cache line issue.
 * The driver would use another aligned buffer for this indirect buffer.
 */
#define YL_PATCH_DRAW_INDIRECT_BUFFER

/* MAX of DispatchCompute: 12(3*4), DrawArrayIndirect: 16(4*4), DrawElementIndirect: 20(5*4) */
#define YL_PATCH_DRAW_INDIRECT_BUFFER_SIZE 20

#define G3D_PATCH_DRAW_INDIRECT_BASE_ALIGN 64
#define YL_HW_READ_INDIRECT_BUFFER_CACHE_SIZE 64
#define YL_EGL_SYNC_FIX
/* End of [ES3.1 definition] */
/* [ULC_Sapphire definition] */
#ifdef Sapphire_ULC
#endif
/* End of [ULC_Sapphire definition] */
#endif /* _YL_DEFINE_H_ */
