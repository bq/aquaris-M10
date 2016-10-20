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

#include <linux/kernel.h>
#include <linux/module.h>
#include "gpad_common.h"
#include "gpad_kmif.h"
#include "gpad_hal.h"
#include "gpad_pm_config.h"
#if defined(GPAD_SUPPORT_DVFS)
#include "mt_gpufreq.h"
#endif /* defined(GPAD_SUPPORT_DVFS) */
#include "gpad_sdbg.h"
#if defined(GPAD_SUPPORT_DFD)
#include "gpad_dfd.h"
#endif /* GPAD_SUPPORT_DFD */

static volatile unsigned char *gpad_reg_base_s; /* GPU register base address. */
static volatile unsigned char *gpad_mmu_reg_base_s; /* GPU register base address. */
#if defined(GPAD_SUPPORT_MET)
/*
 * MET function pointer declaration.
 */
extern unsigned int (*mtk_get_gpu_memory_usage_fp)(void);
#endif /* GPAD_SUPPORT_MET */

#if defined(MTK_INHOUSE_GPU)
#define GPAD_MEMORY_PA      0x40000000
#define GPAD_MEMORY_RANGE   0x80000000
#endif


/*-----------------------------------------------------------------------------
 * Private implementation.
 *---------------------------------------------------------------------------*/
#if defined(GPAD_SUPPORT_MET)
static unsigned int gpad_get_gpu_mem_met_(void)
{
	unsigned int mem = g3dKmdKmifQueryMemoryUsage_MET();

	return mem;
};
#endif /* GPAD_SUPPORT_MET */

/*-----------------------------------------------------------------------------
 * Callback functions for other kernel modules.
 *---------------------------------------------------------------------------*/
void gpad_notify_g3d_hang(void)
{
	GPAD_LOG_INFO("gpad_notify_g3d_hang():\n");

	/* sdbg core dump */
	gpad_sdbg_save_sdbg_info(DUMP_CORE_LEVEL);

}

void gpad_notify_g3d_reset(void)
{
	struct gpad_device  *dev = gpad_get_default_dev();
	struct gpad_pm_info *pm_info = &dev->pm_info;
	int                  reset_to_default = 0;

	GPAD_LOG_INFO("gpad_notify_g3d_reset()\n");

	if (GPAD_PM_ENABLED == gpad_pm_get_state(dev)) {
		if (gpad_pm_is_custom_config(pm_info->config)) {
			/*
			 * Reset to default config since we're not sure if PM config is still valid in this case.
			 */
			reset_to_default = 1;
			gpad_pm_reset_backup_config(pm_info);
		}
		gpad_pm_force_enable(dev, reset_to_default);
	}
}

void gpad_notify_g3d_early_suspend(void)
{
	struct gpad_device  *dev = gpad_get_default_dev();
	struct gpad_pm_info *pm_info = &dev->pm_info;

	GPAD_LOG_INFO("gpad_notify_g3d_early_suspend()\n");

#if defined(GPAD_SUPPORT_DVFS)
	if (gpad_ipem_enable_get(dev)) {
		GPAD_SET_DEV_STATUS(dev, GPAD_IPEM_EN_PREV);
		gpad_ipem_disable(dev);
	} else {
		GPAD_CLEAR_DEV_STATUS(dev, GPAD_IPEM_EN_PREV);
	}
#endif

	if (GPAD_PM_ENABLED == gpad_pm_get_state(dev)) {
		GPAD_SET_DEV_STATUS(dev, GPAD_PM_EN_PREV);

		/*
		 * Back up PM registers for the case that PM configuration is customized by user application.
		 */
		if (gpad_pm_is_custom_config(pm_info->config)) {
			gpad_pm_backup_config(pm_info);
		}
		gpad_pm_disable(dev);
	} else {
		GPAD_CLEAR_DEV_STATUS(dev, GPAD_PM_EN_PREV);
	}
}

void gpad_notify_g3d_late_resume(void)
{
	struct gpad_device  *dev = gpad_get_default_dev();

	GPAD_LOG_INFO("gpad_notify_g3d_late_resume()\n");

	if (GPAD_CHECK_DEV_STATUS(dev, GPAD_PM_EN_PREV)) {
		GPAD_CLEAR_DEV_STATUS(dev, GPAD_PM_EN_PREV);
		gpad_pm_force_enable(dev, 0);
	}

#if defined(GPAD_SUPPORT_DVFS)
	if (GPAD_CHECK_DEV_STATUS(dev, GPAD_IPEM_EN_PREV)) {
		GPAD_CLEAR_DEV_STATUS(dev, GPAD_IPEM_EN_PREV);
		gpad_ipem_enable(dev);
	}
#endif
}

void gpad_notify_g3d_power_low(void)
{
	GPAD_LOG_INFO("gpad_notify_g3d_power_low()\n");
#if defined(GPAD_SUPPORT_DVFS)
	gpad_ipem_callback_powerlow();
#endif /* defined(GPAD_SUPPORT_DVFS) */
}

void gpad_notify_g3d_power_high(void)
{
	GPAD_LOG_INFO("gpad_notify_g3d_power_high()\n");
#if defined(GPAD_SUPPORT_DVFS)
	gpad_ipem_callback_powerhigh();
#endif /* defined(GPAD_SUPPORT_DVFS) */
}

void gpad_notify_g3d_power_off(void)
{
	struct gpad_device  *dev = gpad_get_default_dev();
	struct gpad_pm_info *pm_info = &dev->pm_info;

	GPAD_LOG_INFO("gpad_notify_g3d_power_off()\n");

#if defined(GPAD_SUPPORT_DVFS)
	if (gpad_ipem_enable_get(dev)) {
		GPAD_SET_DEV_STATUS(dev, GPAD_IPEM_EN_PREV);
		gpad_ipem_timer_disable(dev);
	} else {
		GPAD_CLEAR_DEV_STATUS(dev, GPAD_IPEM_EN_PREV);
	}
#endif

	if (GPAD_PM_ENABLED == gpad_pm_get_state(dev)) {
		GPAD_SET_DEV_STATUS(dev, GPAD_PM_EN_PREV);

		/*
		 * Back up PM registers for the case that PM configuration is customized by user application.
		 */
		if (gpad_pm_is_custom_config(pm_info->config)) {
			gpad_pm_backup_config(pm_info);
		}
		gpad_pm_disable(dev);
	} else {
		GPAD_CLEAR_DEV_STATUS(dev, GPAD_PM_EN_PREV);
	}
}

void gpad_notify_g3d_power_on(void)
{
	struct gpad_device  *dev = gpad_get_default_dev();

	GPAD_LOG_INFO("gpad_notify_g3d_power_on()\n");

	if (GPAD_CHECK_DEV_STATUS(dev, GPAD_PM_EN_PREV)) {
		GPAD_CLEAR_DEV_STATUS(dev, GPAD_PM_EN_PREV);
		gpad_pm_force_enable(dev, 0);
	}

#if defined(GPAD_SUPPORT_DVFS)
	if (GPAD_CHECK_DEV_STATUS(dev, GPAD_IPEM_EN_PREV)) {
		GPAD_CLEAR_DEV_STATUS(dev, GPAD_IPEM_EN_PREV);
		gpad_ipem_timer_enable(dev);
	}
#endif
}

void gpad_notify_g3d_power_off_begin(void)
{
	GPAD_LOG_INFO("gpad_notify_g3d_power_off_begin()\n");

#if defined(GPAD_SUPPORT_DVFS)
	gpad_ipem_callback_poweroff_begin();
#endif
}

void gpad_notify_g3d_power_off_end(void)
{
	GPAD_LOG_INFO("gpad_notify_g3d_power_off_end()\n");

#if defined(GPAD_SUPPORT_DVFS)
	gpad_ipem_callback_poweroff_end();
#endif
}

void gpad_notify_g3d_power_on_begin(void)
{
	GPAD_LOG_INFO("gpad_notify_g3d_power_on_begin()\n");

#if defined(GPAD_SUPPORT_DVFS)
	gpad_ipem_callback_poweron_begin();
#endif
}

void gpad_notify_g3d_power_on_end(void)
{
	GPAD_LOG_INFO("gpad_notify_g3d_power_on_end()\n");

#if defined(GPAD_SUPPORT_DVFS)
	gpad_ipem_callback_poweron_end();
#endif
}

void gpad_notify_g3d_dfd_interrupt(unsigned int type)
{
	GPAD_LOG_INFO("gpad_notify_g3d_internal_dump()\n");
	/*! This function is called in g3dkmdIsrTop (taskelt) */
	/*  (1) Check trigger condition ever hit.                   */
	/*      If so, then read debug bus output data              */
	/*  (2) Check internal dump interrupt is trigger.           */
	/*      If so, read internal dump.                          */
#if defined(GPAD_SUPPORT_DFD)
	gpad_dfd_interrupt_handler(type);
#endif /* GPAD_SUPPORT_DFD */

}

void gpad_notify_g3d_performance_mode(unsigned int enabled)
{
	GPAD_LOG_INFO("gpad_notify_g3d_performance_mode\n");

#if defined(GPAD_SUPPORT_DVFS)
	gpad_ipem_performance_mode(enabled);
#endif
}

void gpad_get_top_pm_counters(G3DKMD_KMIF_TOP_PM_COUNTERS *counters)
{
	struct gpad_device     *dev = gpad_get_default_dev();
	struct gpad_pm_info    *pm_info = &dev->pm_info;
	struct gpad_pm_stat    *stat = &pm_info->stat;

	GPAD_LOG_INFO("get_top_pm_counters()\n");
	if (unlikely(NULL == counters)) {
		return;
	}
	counters->gpu_loading = ((stat->total_time/100 > 0) ? ((stat->busy_time) / (stat->total_time / 100)) : 0);
	counters->frame_cnt = stat->frame_cnt;
	counters->vertex_cnt = stat->vertex_cnt;
	counters->pixel_cnt = stat->pixel_cnt;
	counters->time = stat->total_time;
}

int __init gpad_hal_open(void)
{
	int err = 0;

	static G3DKMD_KMIF_OPS ops = {
		.notify_g3d_hang = gpad_notify_g3d_hang, .notify_g3d_reset = gpad_notify_g3d_reset, .notify_g3d_early_suspend = gpad_notify_g3d_early_suspend, .notify_g3d_late_resume = gpad_notify_g3d_late_resume, .notify_g3d_power_low = gpad_notify_g3d_power_low, .notify_g3d_power_high = gpad_notify_g3d_power_high, .notify_g3d_power_off = gpad_notify_g3d_power_off, .notify_g3d_power_on = gpad_notify_g3d_power_on, .notify_g3d_power_off_begin = gpad_notify_g3d_power_off_begin, .notify_g3d_power_off_end = gpad_notify_g3d_power_off_end, .notify_g3d_power_on_begin = gpad_notify_g3d_power_on_begin, .notify_g3d_power_on_end = gpad_notify_g3d_power_on_end, .notify_g3d_dfd_interrupt = gpad_notify_g3d_dfd_interrupt, .notify_g3d_performance_mode = gpad_notify_g3d_performance_mode, .get_top_pm_counters = gpad_get_top_pm_counters, };

	GPAD_LOG_LOUD("gpad_hal_open()...\n");
	err = g3dKmdKmifRegister(&ops);
	if (err) {
		GPAD_LOG_ERR("gpad_hal_open(): g3dKmdKmifRegister failed with %d\n", err);
		return 0;
	}

#if !defined(GPAD_NULL_HAL)
	gpad_reg_base_s = g3dKmdKmifGetRegBase();
	gpad_mmu_reg_base_s = g3dKmdKmifGetMMURegBase();
#endif

#if defined(GPAD_SUPPORT_MET)
	mtk_get_gpu_memory_usage_fp = gpad_get_gpu_mem_met_;
#endif

	GPAD_LOG_INFO("gpad_hal_open(): registered with g3dkmd. base register: %p\n", gpad_reg_base_s);

	return err;
}

void gpad_hal_close(void)
{
	GPAD_LOG_LOUD("gpad_hal_close()...\n");
	g3dKmdKmifDeregister();
	GPAD_LOG_INFO("gpad_hal_close(): unregistered with g3dkmd\n");
}

void gpad_hal_write32(unsigned int offset, unsigned int value)
{
	if (unlikely(!gpad_reg_base_s || !gpad_mmu_reg_base_s)) {
		return;
	}

	if (offset + 0x4 <= 0x5000) {
		*((volatile unsigned int *)(gpad_reg_base_s + offset)) = value;
	} else if (offset >= 0x5000 && offset + 0x4 <= 0x6000) {
		offset -= 0x5000;
		*((volatile unsigned int *)(gpad_mmu_reg_base_s + offset)) = value;
	} else {
		GPAD_LOG_ERR("gpad_hal_write32(): Invalid register space, offset=%08X\n", offset);
	}
}

void gpad_hal_write16(unsigned int offset, unsigned short value)
{
	if (unlikely(!gpad_reg_base_s || !gpad_mmu_reg_base_s)) {
		return;
	}

	if (offset + 0x2 <= 0x5000) {
		*((volatile unsigned short *)(gpad_reg_base_s + offset)) = value;
	} else if (offset >= 0x5000 && offset + 0x2 <= 0x6000) {
		offset -= 0x5000;
		*((volatile unsigned short *)(gpad_mmu_reg_base_s + offset)) = value;
	} else {
		GPAD_LOG_ERR("gpad_hal_write16(): Invalid register space, offset=%08X\n", offset);
	}
}

void gpad_hal_write8(unsigned int offset, unsigned char value)
{
	if (unlikely(!gpad_reg_base_s || !gpad_mmu_reg_base_s)) {
		return;
	}

	if (offset + 0x1 <= 0x5000) {
		*((volatile unsigned char *)(gpad_reg_base_s + offset)) = value;
	} else if (offset >= 0x5000 && offset + 0x1 <= 0x6000) {
		offset -= 0x5000;
		*((volatile unsigned char *)(gpad_mmu_reg_base_s + offset)) = value;
	} else {
		GPAD_LOG_ERR("gpad_hal_write8(): Invalid register space, offset=%08X\n", offset);
	}
}

unsigned int gpad_hal_read32(unsigned int offset)
{
	if (unlikely(!gpad_reg_base_s || !gpad_mmu_reg_base_s)) {
		return 0;
	}

	if (offset + 0x4 <= 0x5000) {
		return *((volatile unsigned int *)(gpad_reg_base_s + offset));
	} else if (offset >= 0x5000 && offset + 0x4 <= 0x6000) {
		offset -= 0x5000;
		return *((volatile unsigned int *)(gpad_mmu_reg_base_s + offset));
	} else {
		GPAD_LOG_ERR("gpad_hal_read32(): Invalid register space, offset=%08X\n", offset);
		return 0;
	}
}

unsigned short gpad_hal_read16(unsigned int offset)
{
	if (unlikely(!gpad_reg_base_s || !gpad_mmu_reg_base_s)) {
		return 0;
	}

	if (offset + 0x2 <= 0x5000) {
		return *((volatile unsigned short *)(gpad_reg_base_s + offset));
	} else if (offset >= 0x5000 && offset + 0x2 <= 0x6000) {
		offset -= 0x5000;
		return *((volatile unsigned short *)(gpad_mmu_reg_base_s + offset));
	} else {
		GPAD_LOG_ERR("gpad_hal_read16(): Invalid register space, offset=%08X\n", offset);
		return 0;
	}
}

unsigned char gpad_hal_read8(unsigned int offset)
{
	if (unlikely(!gpad_reg_base_s || !gpad_mmu_reg_base_s)) {
		return 0;
	}

	if (offset + 0x1 <= 0x5000) {
		return *((volatile unsigned char *)(gpad_reg_base_s + offset));
	} else if (offset >= 0x5000 && offset + 0x1 <= 0x6000) {
		offset -= 0x5000;
		return *((volatile unsigned char *)(gpad_mmu_reg_base_s + offset));
	} else {
		GPAD_LOG_ERR("gpad_hal_read8(): Invalid register space, offset=%08X\n", offset);
		return 0;
	}
}

int gpad_hal_read_mem(unsigned int offset, int size, int type, unsigned int *data)
{
#if defined(MTK_INHOUSE_GPU)
	unsigned char  *vaddr;
	int             idx;
	phys_addr_t     addr = offset;

	if (type == GPAD_VA_ADDR_TYPE) {
		addr = g3dKmdKmifMvaToPa(offset);
	}

	if (addr < GPAD_MEMORY_PA || (addr + 0x4 > GPAD_MEMORY_PA + GPAD_MEMORY_RANGE)) {
		GPAD_LOG_ERR("gpad_hal_read_mem(): Invalid register space, offset=%08X\n", offset);
		return -EINVAL;
	}

	if (addr + size > GPAD_MEMORY_PA + GPAD_MEMORY_RANGE) {
		GPAD_LOG_ERR("gpad_hal_read_mem(): Invalid register space, offset=%08X size=%d bytes\n", offset, size);
		size = GPAD_MEMORY_PA + GPAD_MEMORY_RANGE - addr;
	}

	idx = 0;
	vaddr = phys_to_virt(addr);
	while (size > 0) {
		data[idx++] = *((volatile unsigned int *)vaddr);
		vaddr += sizeof(unsigned int);
		size -= sizeof(unsigned int);
	}
	return 0;
#else
	return -EINVAL;
#endif /* MTK_INHOUSE_GPU */

}

int gpad_hal_write_mem(unsigned int offset, int type, unsigned int value)
{
#if defined(MTK_INHOUSE_GPU)
	unsigned char  *vaddr;
	int             idx;
	phys_addr_t     addr = offset;

	if (type == GPAD_VA_ADDR_TYPE) {
		addr = g3dKmdKmifMvaToPa(offset);
	}

	if (addr < GPAD_MEMORY_PA || (addr + 0x4 > GPAD_MEMORY_PA + GPAD_MEMORY_RANGE)) {
		GPAD_LOG_ERR("gpad_hal_write_mem(): Invalid register space, offset=%08X\n", offset);
		return -EINVAL;
	}

	idx = 0;
	vaddr = phys_to_virt(addr);
	*((volatile unsigned int *)vaddr) = value;
	return 0;
#else
	return -EINVAL;
#endif /* MTK_INHOUSE_GPU */
}
