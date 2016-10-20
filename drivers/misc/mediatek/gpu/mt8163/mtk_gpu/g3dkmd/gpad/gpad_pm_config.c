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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>  /* for IS_ERR, PTR_ERR */
#include <linux/slab.h> /* for memory allocation */
#include <linux/sched.h> /* for cpu_clock */

#include "gpad_common.h"
#include "gpad_kmif.h"
#include "gpad_hal.h"
#include "gpad_pm.h"
#include "gpad_pm_priv.h"
#include "gpad_pm_config.h"

#define MASK_ALL_BITS    0xffffffff

typedef void (*gpad_pm_config_func)(struct gpad_pm_info *);

void gpad_pm_config_common(struct gpad_pm_info *pm_info, unsigned int ring_addr, unsigned int ring_size)
{
#if GPAD_GPU_VER == 0x0314
	/* step 1. active pm clck/module and reset timestamp (pm_clear_reg and pm_en) */
	/*D.S PM:0x13001F00 %LONG 0x00000011  */
	gpad_hal_write32(0x1F00, 0x00000011);

	/* step 2. release clear (pm_en) */
	/* step 3. pm_sample_level[7:6] = HW sub-frame (00b) */
	/*D.S PM:0x13001F00 %LONG 0x00000001 */
	gpad_hal_write32(0x1F00, 0x00000001);

	/* step 4. pm_m_run=0 */
	/*D.S PM:0x13001F04 %LONG 0x00000000 */
	gpad_hal_write32(0x1F04, 0x00000000);

	/*; step 5. pm_n_units=2 */
	/*D.S PM:0x13001F08 %LONG 0x00000002 */
	gpad_hal_write32(0x1F08, 0xffffffff);

	/* step 6. DRAM write start address */
	/*D.S PM:0x13001F10 %LONG 0x7f90b000 */
	gpad_hal_write32(0x1F10, ring_addr);

	/* step 7. DRAM size */
	/*D.S PM:0x13001F4C %LONG 0x00014000 */
	gpad_hal_write32(0x1F4C, ring_size);

	/* step 8. FF:03, TX:02, MX:01, DW:00 */
	/*D.S PM:0x13001F0C %LONG 0x000000ff */
	gpad_hal_write32(0x1F0C, 0x000000ff);

	/* step 9. select post, vl, ce, cf */
	/*D.S PM:0x13001F20 %LONG 0x00004713 */
	gpad_hal_write32(0x1F20, 0x00004713);

	/*; step 10. all choose group 0 */
	/*D.S PM:0x13001F24 %LONG 0x00000000 */
	gpad_hal_write32(0x1F24, 0x00000000);
	/*D.S PM:0x13001F28 %LONG 0x00000000 */
	gpad_hal_write32(0x1F28, 0x00000000);
	/*D.S PM:0x13001F2C %LONG 0x00000001 ; post group 1 */
	gpad_hal_write32(0x1F2C, 0x00000001);
	/*D.S PM:0x13001F30 %LONG 0x00000000 */
	gpad_hal_write32(0x1F30, 0x00000000);
	/*D.S PM:0x13001F34 %LONG 0x00000004 ; vl group 4 */
	gpad_hal_write32(0x1F34, 0x00000004);
	/*D.S PM:0x13001F38 %LONG 0x00000000 */
	gpad_hal_write32(0x1F38, 0x00000000);
	/*D.S PM:0x13001F3C %LONG 0x00000000 */
	gpad_hal_write32(0x1F3C, 0x00000000);
#elif GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32_mask(0x1f00, 0x000002c0, 0x00000000); /* set sample unit as segment TODO: make it configurable */
	/* clear bit9 set for overview config patch */
	gpad_hal_write32(0x1f04, 0x00000000); /* set start from frame 0 */
	gpad_hal_write32(0x1f08, 0x00000000); /* set monitoring untill PM disable, that is non-stop mode */
	gpad_hal_write32(0x3054, 0x00000100); /* set DWP mux to pm */
	gpad_hal_write32(0x4184, 0x00000100); /* set MX mux to pm */
	gpad_hal_write32(0x1f30, 0x00000100); /* set FF/TX mux to pm */
	gpad_hal_write32(0x1f24, 0x1f1f1f1f); /* set FF mod mux to pm */
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000); /* clear RESET function */

	GPAD_LOG_INFO("PM ring start address: 0x%08x, size: 0x%08x\n", ring_addr, ring_size);
	gpad_hal_write32(0x1f0c, ring_addr); /* set DRAM write start address */
	gpad_hal_write32(0x1f20, ring_size); /* set DRAM size */
#else
#error "gpad_pm_config_common(): infinit n_units should be ready"
#endif
}

static void gpad_pm_config_overview(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0314
#elif GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x1e000100);
	gpad_hal_write32(0x4188, 0x1e000100);
	gpad_hal_write32(0x1f34, 0x13000100);
	gpad_hal_write32(0x1f28, 0x0a000d0d);
	gpad_hal_write32(0x1f2c, 0x2a000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000200);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x31330000);
#else
#error "gpad_pm_config_overview(): not yet implemented!"
#endif
}

static void gpad_pm_config_bandwidth(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x02050100);
	gpad_hal_write32(0x4188, 0x02050100);
	gpad_hal_write32(0x1f34, 0x02050100);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x11110000);
#else
#error "gpad_pm_config_bandwidth(): not yet implemented!"
#endif
}

static void gpad_pm_config_cache0(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x120d0302);
	gpad_hal_write32(0x4188, 0x120d0302);
	gpad_hal_write32(0x1f34, 0x120d0302);
	gpad_hal_write32(0x1f28, 0x0a000d0d);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x31330000);
#else
#error "gpad_pm_config_cache0(): not yet implemented!"
#endif
}

static void gpad_pm_config_cache1(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x06050403);
	gpad_hal_write32(0x4188, 0x06050403);
	gpad_hal_write32(0x1f34, 0x06050403);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x22220000);
#else
#error "gpad_pm_config_cache1(): not yet implemented!"
#endif
}

static void gpad_pm_config_shader(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x00060b1e);
	gpad_hal_write32(0x4188, 0x00060b1e);
	gpad_hal_write32(0x1f34, 0x00060b1e);
	gpad_hal_write32(0x1f28, 0x0a0a0a00);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33300000);
#else
#error "gpad_pm_config_shader(): not yet implemented!"
#endif
}

static void gpad_pm_config_pipeline(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x1f030504);
	gpad_hal_write32(0x4188, 0x1f030504);
	gpad_hal_write32(0x1f34, 0x1f030504);
	gpad_hal_write32(0x1f28, 0x000a0d0d);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x03330000);
#else
#error "gpad_pm_config_pipeline(): not yet implemented!"
#endif
}

static void gpad_pm_config_mem_latency(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x1e020100);
	gpad_hal_write32(0x4188, 0x1e020100);
	gpad_hal_write32(0x1f34, 0x13020100);
	gpad_hal_write32(0x1f28, 0x0a000d0d);
	gpad_hal_write32(0x1f2c, 0x2a000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000200);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x31330000);
#else
#error "gpad_pm_config_mem_latency(): not yet implemented!"
#endif
}

static void gpad_pm_config_inst_cnt(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x0c0a0908);
	gpad_hal_write32(0x4188, 0x0c0a0908);
	gpad_hal_write32(0x1f34, 0x0c0a0908);
	gpad_hal_write32(0x1f28, 0x0a000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x30000000);
#else
#error "gpad_pm_config_inst_cnt(): not yet implemented!"
#endif
}

static void gpad_pm_config_wave_cnt(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x0f04000b);
	gpad_hal_write32(0x4188, 0x0f04000b);
	gpad_hal_write32(0x1f34, 0x0f04000b);
	gpad_hal_write32(0x1f28, 0x0000000a);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00030000);
#else
#error "gpad_pm_config_wave_cnt(): not yet implemented!"
#endif
}

static void gpad_pm_config_port_conflict_l2_flush(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x0413121d);
	gpad_hal_write32(0x4188, 0x0413121d);
	gpad_hal_write32(0x1f34, 0x0413121d);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x10000000);
#else
#error "gpad_pm_config_port_conflict_l2_flush(): not yet implemented!"
#endif
}

static void gpad_pm_config_wave_active_inactive0(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x04000602);
	gpad_hal_write32(0x4188, 0x04000602);
	gpad_hal_write32(0x1f34, 0x04000602);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_wave_active_inactive0(): not yet implemented!"
#endif
}

static void gpad_pm_config_wave_active_inactive1(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x05010703);
	gpad_hal_write32(0x4188, 0x05010703);
	gpad_hal_write32(0x1f34, 0x05010703);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_wave_active_inactive1(): not yet implemented!"
#endif
}

static void gpad_pm_config_smu_block_cnt(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x11100f0b);
	gpad_hal_write32(0x4188, 0x11100f0b);
	gpad_hal_write32(0x1f34, 0x11100f0b);
	gpad_hal_write32(0x1f28, 0x0000000a);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00030000);
#else
#error "gpad_pm_config_smu_block_cnt(): not yet implemented!"
#endif
}

static void gpad_pm_config_wave_block_cnt(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x0a09080b);
	gpad_hal_write32(0x4188, 0x0a09080b);
	gpad_hal_write32(0x1f34, 0x0a09080b);
	gpad_hal_write32(0x1f28, 0x0a0a0a0a);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_wave_block_cnt(): not yet implemented!"
#endif
}

static void gpad_pm_config_wave_pipeline_time0(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x07001d0e);
	gpad_hal_write32(0x4188, 0x07001d0e);
	gpad_hal_write32(0x1f34, 0x07001d0e);
	gpad_hal_write32(0x1f28, 0x0a00000a);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x30030000);
#else
#error "gpad_pm_config_wave_pipeline_time0(): not yet implemented!"
#endif
}

static void gpad_pm_config_wave_pipeline_time1(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x1615140c);
	gpad_hal_write32(0x4188, 0x1615140c);
	gpad_hal_write32(0x1f34, 0x1615140c);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_wave_pipeline_time1(): not yet implemented!"
#endif
}

static void gpad_pm_config_wave_pipeline_time2(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x1918170d);
	gpad_hal_write32(0x4188, 0x1918170d);
	gpad_hal_write32(0x1f34, 0x1918170d);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_wave_pipeline_time2(): not yet implemented!"
#endif
}

static void gpad_pm_config_wave_pipeline_time3(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x1c1b1a0e);
	gpad_hal_write32(0x4188, 0x1c1b1a0e);
	gpad_hal_write32(0x1f34, 0x1c1b1a0e);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_wave_pipeline_time3(): not yet implemented!"
#endif
}

static void gpad_pm_config_dw_0_3(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x03020100);
	gpad_hal_write32(0x4188, 0x03020100);
	gpad_hal_write32(0x1f34, 0x03020100);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_dw_0_3(): not yet implemented!"
#endif
}

static void gpad_pm_config_dw_4_7(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x07060504);
	gpad_hal_write32(0x4188, 0x07060504);
	gpad_hal_write32(0x1f34, 0x07060504);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_dw_4_7(): not yet implemented!"
#endif
}

static void gpad_pm_config_dw_8_11(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x0b0a0908);
	gpad_hal_write32(0x4188, 0x0b0a0908);
	gpad_hal_write32(0x1f34, 0x0b0a0908);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_dw_8_11(): not yet implemented!"
#endif
}

static void gpad_pm_config_dw_12_15(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x0f0e0d0c);
	gpad_hal_write32(0x4188, 0x0f0e0d0c);
	gpad_hal_write32(0x1f34, 0x0f0e0d0c);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_dw_12_15(): not yet implemented!"
#endif
}

static void gpad_pm_config_dw_16_19(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x13121110);
	gpad_hal_write32(0x4188, 0x13121110);
	gpad_hal_write32(0x1f34, 0x13121110);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_dw_16_19(): not yet implemented!"
#endif
}

static void gpad_pm_config_dw_20_23(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x17161514);
	gpad_hal_write32(0x4188, 0x17161514);
	gpad_hal_write32(0x1f34, 0x17161514);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_dw_20_23(): not yet implemented!"
#endif
}

static void gpad_pm_config_dw_24_27(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x1b1a1918);
	gpad_hal_write32(0x4188, 0x1b1a1918);
	gpad_hal_write32(0x1f34, 0x1b1a1918);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_dw_24_27(): not yet implemented!"
#endif
}

static void gpad_pm_config_dw_28_31(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x1f1e1d1c);
	gpad_hal_write32(0x4188, 0x1f1e1d1c);
	gpad_hal_write32(0x1f34, 0x1f1e1d1c);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x00000000);
#else
#error "gpad_pm_config_dw_28_31(): not yet implemented!"
#endif
}

static void gpad_pm_config_dwcmn_0_3(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x03020100);
	gpad_hal_write32(0x4188, 0x03020100);
	gpad_hal_write32(0x1f34, 0x03020100);
	gpad_hal_write32(0x1f28, 0x0a0a0a0a);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_dwcmn_0_3(): not yet implemented!"
#endif
}

static void gpad_pm_config_dwcmn_4_7(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x07060504);
	gpad_hal_write32(0x4188, 0x07060504);
	gpad_hal_write32(0x1f34, 0x07060504);
	gpad_hal_write32(0x1f28, 0x0a0a0a0a);
	gpad_hal_write32(0x1f2c, 0x00008000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_dwcmn_4_7(): not yet implemented!"
#endif
}

static void gpad_pm_config_dwcmn_8_11(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x0b0a0908);
	gpad_hal_write32(0x4188, 0x0b0a0908);
	gpad_hal_write32(0x1f34, 0x0b0a0908);
	gpad_hal_write32(0x1f28, 0x0a0a0a0a);
	gpad_hal_write32(0x1f2c, 0x00404000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_dwcmn_8_11(): not yet implemented!"
#endif
}

static void gpad_pm_config_dwcmn_12_15(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x0f0e0d0c);
	gpad_hal_write32(0x4188, 0x0f0e0d0c);
	gpad_hal_write32(0x1f34, 0x0f0e0d0c);
	gpad_hal_write32(0x1f28, 0x0a0a0a0a);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_dwcmn_12_15(): not yet implemented!"
#endif
}

static void gpad_pm_config_dwcmn_16_19(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x13121110);
	gpad_hal_write32(0x4188, 0x13121110);
	gpad_hal_write32(0x1f34, 0x13121110);
	gpad_hal_write32(0x1f28, 0x0a0a0a0a);
	gpad_hal_write32(0x1f2c, 0x2c000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_dwcmn_16_19(): not yet implemented!"
#endif
}

static void gpad_pm_config_tx_0_3(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x03020100);
	gpad_hal_write32(0x4188, 0x03020100);
	gpad_hal_write32(0x1f34, 0x03020100);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x22220000);
#else
#error "gpad_pm_config_tx_0_3(): not yet implemented!"
#endif
}

static void gpad_pm_config_tx_4_7(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x00060504);
	gpad_hal_write32(0x4188, 0x00060504);
	gpad_hal_write32(0x1f34, 0x00060504);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x22220000);
#else
#error "gpad_pm_config_tx_4_7(): not yet implemented!"
#endif
}

static void gpad_pm_config_mxmf_0_3(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x03020100);
	gpad_hal_write32(0x4188, 0x03020100);
	gpad_hal_write32(0x1f34, 0x03020100);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x11110000);
#else
#error "gpad_pm_config_mxmf_0_3(): not yet implemented!"
#endif
}

static void gpad_pm_config_mxmf_4_7(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x07060504);
	gpad_hal_write32(0x4188, 0x07060504);
	gpad_hal_write32(0x1f34, 0x07060504);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x11110000);
#else
#error "gpad_pm_config_mxmf_4_7(): not yet implemented!"
#endif
}

static void gpad_pm_config_mxmf_8_11(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x0b0a0908);
	gpad_hal_write32(0x4188, 0x0b0a0908);
	gpad_hal_write32(0x1f34, 0x0b0a0908);
	gpad_hal_write32(0x1f28, 0x00000000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x11110000);
#else
#error "gpad_pm_config_mxmf_8_11(): not yet implemented!"
#endif
}

static void gpad_pm_config_mxmf_12_15(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x01000d0c);
	gpad_hal_write32(0x4188, 0x01000d0c);
	gpad_hal_write32(0x1f34, 0x01000d0c);
	gpad_hal_write32(0x1f28, 0x0b0b0000);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33110000);
#else
#error "gpad_pm_config_mxmf_12_15(): not yet implemented!"
#endif
}

static void gpad_pm_config_mxmf_16_19(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x00040302);
	gpad_hal_write32(0x4188, 0x00040302);
	gpad_hal_write32(0x1f34, 0x00040302);
	gpad_hal_write32(0x1f28, 0x0b0b0b0b);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_mxmf_16_19(): not yet implemented!"
#endif
}

static void gpad_pm_config_ff_0_3(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x03020100);
	gpad_hal_write32(0x4188, 0x03020100);
	gpad_hal_write32(0x1f34, 0x03020100);
	gpad_hal_write32(0x1f28, 0x0d0d0d0d);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_ff_0_3(): not yet implemented!"
#endif
}

static void gpad_pm_config_ff_4_7(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x01000504);
	gpad_hal_write32(0x4188, 0x01000504);
	gpad_hal_write32(0x1f34, 0x01000504);
	gpad_hal_write32(0x1f28, 0x00000d0d);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_ff_4_7(): not yet implemented!"
#endif
}

static void gpad_pm_config_ff_8_11(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x02010000);
	gpad_hal_write32(0x4188, 0x02010000);
	gpad_hal_write32(0x1f34, 0x02010000);
	gpad_hal_write32(0x1f28, 0x02020201);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_ff_8_11(): not yet implemented!"
#endif
}

static void gpad_pm_config_ff_12_15(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x01000000);
	gpad_hal_write32(0x4188, 0x01000000);
	gpad_hal_write32(0x1f34, 0x01000000);
	gpad_hal_write32(0x1f28, 0x06060503);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_ff_12_15(): not yet implemented!"
#endif
}

static void gpad_pm_config_ff_16_19(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x03020100);
	gpad_hal_write32(0x4188, 0x03020100);
	gpad_hal_write32(0x1f34, 0x03020100);
	gpad_hal_write32(0x1f28, 0x04040404);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_ff_16_19(): not yet implemented!"
#endif
}

static void gpad_pm_config_ff_20_23(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x03020100);
	gpad_hal_write32(0x4188, 0x03020100);
	gpad_hal_write32(0x1f34, 0x03020100);
	gpad_hal_write32(0x1f28, 0x07070707);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_ff_20_23(): not yet implemented!"
#endif
}

static void gpad_pm_config_ff_24_27(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x02010004);
	gpad_hal_write32(0x4188, 0x02010004);
	gpad_hal_write32(0x1f34, 0x02010004);
	gpad_hal_write32(0x1f28, 0x08080807);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_ff_24_27(): not yet implemented!"
#endif
}

static void gpad_pm_config_ff_28_31(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x01000403);
	gpad_hal_write32(0x4188, 0x01000403);
	gpad_hal_write32(0x1f34, 0x01000403);
	gpad_hal_write32(0x1f28, 0x09090808);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_ff_28_31(): not yet implemented!"
#endif
}

static void gpad_pm_config_ff_32_35(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x05040302);
	gpad_hal_write32(0x4188, 0x05040302);
	gpad_hal_write32(0x1f34, 0x05040302);
	gpad_hal_write32(0x1f28, 0x09090909);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_ff_32_35(): not yet implemented!"
#endif
}

static void gpad_pm_config_ff_36_39(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x09080706);
	gpad_hal_write32(0x4188, 0x09080706);
	gpad_hal_write32(0x1f34, 0x09080706);
	gpad_hal_write32(0x1f28, 0x09090909);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_ff_36_39(): not yet implemented!"
#endif
}

static void gpad_pm_config_ff_40_43(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x00000b0a);
	gpad_hal_write32(0x4188, 0x00000b0a);
	gpad_hal_write32(0x1f34, 0x00000b0a);
	gpad_hal_write32(0x1f28, 0x09090909);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x33330000);
#else
#error "gpad_pm_config_ff_40_43(): not yet implemented!"
#endif
}

static void gpad_pm_config_fps_bw(struct gpad_pm_info *pm_info)
{
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_hal_write32(0x3050, 0x01000100);
	gpad_hal_write32(0x4188, 0x01000100);
	gpad_hal_write32(0x1f34, 0x01000100);
	gpad_hal_write32(0x1f28, 0x00000d0d);
	gpad_hal_write32(0x1f2c, 0x00000000);
	gpad_hal_write32_mask(0x1f00, 0x00000200, 0x00000000);
	gpad_hal_write32_mask(0x1f38, 0xffff0000, 0x11330000);
#else
#error "gpad_pm_config_fps_bw(): not yet implemented!"
#endif
}

static const gpad_pm_config_func s_map[] = {
	gpad_pm_config_overview,    /* 0: Overview, default configuration */
#if GPAD_GPU_VER == 0x0530 || GPAD_GPU_VER >= 0x0704
	gpad_pm_config_bandwidth,   /* 1: Bandwidth */
	gpad_pm_config_cache0,      /* 2: Cache 0 */
	gpad_pm_config_cache1,      /* 3: Cache 1 */
	gpad_pm_config_shader,      /* 4: Shader */
	gpad_pm_config_pipeline,    /* 5: Pipeline */
	gpad_pm_config_mem_latency, /* 6: Memory latency */
	gpad_pm_config_inst_cnt,    /* 7: Instruction count */
	gpad_pm_config_wave_cnt,    /* 8: Wave count*/
	gpad_pm_config_port_conflict_l2_flush, /* 9: Port conflict and L2 flush count */
	gpad_pm_config_wave_active_inactive0,  /* 10: Wave active/inactive count 0 */
	gpad_pm_config_wave_active_inactive1,  /* 11: Wave active/inactive count 1 */
	gpad_pm_config_smu_block_cnt,          /* 12: SMU block count */
	gpad_pm_config_wave_block_cnt,         /* 13: wave block count */
	gpad_pm_config_wave_pipeline_time0,    /* 14: wave pipeline time 0 */
	gpad_pm_config_wave_pipeline_time1,    /* 15: wave pipeline time 1 */
	gpad_pm_config_wave_pipeline_time2,    /* 16: wave pipeline time 2 */
	gpad_pm_config_wave_pipeline_time3,    /* 17: wave pipeline time 3 */
	gpad_pm_config_dw_0_3,      /* 18 */
	gpad_pm_config_dw_4_7,      /* 19 */
	gpad_pm_config_dw_8_11,     /* 20 */
	gpad_pm_config_dw_12_15,    /* 21 */
	gpad_pm_config_dw_16_19,    /* 22 */
	gpad_pm_config_dw_20_23,    /* 23 */
	gpad_pm_config_dw_24_27,    /* 24 */
	gpad_pm_config_dw_28_31,    /* 25 */
	gpad_pm_config_dwcmn_0_3,   /* 26 */
	gpad_pm_config_dwcmn_4_7,   /* 27 */
	gpad_pm_config_dwcmn_8_11,  /* 28 */
	gpad_pm_config_dwcmn_12_15, /* 29 */
	gpad_pm_config_dwcmn_16_19, /* 30 */
	gpad_pm_config_tx_0_3,      /* 31 */
	gpad_pm_config_tx_4_7,      /* 32 */
	gpad_pm_config_mxmf_0_3,    /* 33 */
	gpad_pm_config_mxmf_4_7,    /* 34 */
	gpad_pm_config_mxmf_8_11,   /* 35 */
	gpad_pm_config_mxmf_12_15,  /* 36 */
	gpad_pm_config_mxmf_16_19,  /* 37 */
	gpad_pm_config_ff_0_3,      /* 38 */
	gpad_pm_config_ff_4_7,      /* 39 */
	gpad_pm_config_ff_8_11,     /* 40 */
	gpad_pm_config_ff_12_15,    /* 41 */
	gpad_pm_config_ff_16_19,    /* 42 */
	gpad_pm_config_ff_20_23,    /* 43 */
	gpad_pm_config_ff_24_27,    /* 44 */
	gpad_pm_config_ff_28_31,    /* 45 */
	gpad_pm_config_ff_32_35,    /* 46 */
	gpad_pm_config_ff_36_39,    /* 47 */
	gpad_pm_config_ff_40_43,    /* 48 */
	gpad_pm_config_fps_bw,      /* 49 */
#endif
};

int gpad_pm_is_valid_config(unsigned int config)
{
	return config < ARRAY_SIZE(s_map);
}

int gpad_pm_is_custom_config(unsigned int config)
{
	return GPAD_PM_CUST_CONFIG == config;
}

void gpad_pm_do_config(struct gpad_pm_info *pm_info, unsigned int config, unsigned int ring_addr, unsigned int ring_size)
{
	if (likely(gpad_pm_is_valid_config(config))) {
		GPAD_LOG_INFO("gpad_pm_do_config(): config=%u\n", config);
		gpad_pm_config_common(pm_info, ring_addr, ring_size);
		s_map[config](pm_info);
	} else {
		GPAD_LOG_ERR("gpad_pm_do_config(): invalid config=%u!\n", config);
	}
}

static void _backup_reg(struct gpad_pm_info *pm_info, unsigned int offset, unsigned int mask)
{
	int                 idx;
	struct gpad_pm_reg *reg;
	unsigned int        value;

	if (pm_info->bak_reg_cnt < GPAD_PM_CONFIG_REG_CNT) {
		idx = pm_info->bak_reg_cnt++;
		reg = &(pm_info->bak_reg[idx]);

		value = gpad_hal_read32(offset);
		reg->offset = offset;
		reg->mask = mask;
		reg->value = (value & mask);
	} else {
		BUG_ON(1);
	}
}

int gpad_pm_has_backup_config(struct gpad_pm_info *pm_info)
{
	return pm_info->bak_reg_cnt > 0;
}

void gpad_pm_reset_backup_config(struct gpad_pm_info *pm_info)
{
	pm_info->bak_reg_cnt = 0;
}

void gpad_pm_backup_config(struct gpad_pm_info *pm_info)
{
	pm_info->bak_reg_cnt = 0;
	_backup_reg(pm_info, 0x3050, MASK_ALL_BITS);
	_backup_reg(pm_info, 0x4188, MASK_ALL_BITS);
	_backup_reg(pm_info, 0x1f34, MASK_ALL_BITS);
	_backup_reg(pm_info, 0x1f28, MASK_ALL_BITS);
	_backup_reg(pm_info, 0x1f2c, MASK_ALL_BITS);
	_backup_reg(pm_info, 0x1f00, 0x00000200);
	_backup_reg(pm_info, 0x1f38, 0xffff0000);
}

void gpad_pm_restore_config(struct gpad_pm_info *pm_info, unsigned int config, unsigned int ring_addr, unsigned int ring_size)
{
	int                 idx;
	struct gpad_pm_reg *reg;

	if (likely(gpad_pm_has_backup_config(pm_info))) {
		gpad_pm_config_common(pm_info, ring_addr, ring_size);

		for (idx = 0; idx < pm_info->bak_reg_cnt; ++idx) {
			reg = &(pm_info->bak_reg[idx]);
			gpad_hal_write32_mask(reg->offset, reg->mask, reg->value);
		}
	}
}
