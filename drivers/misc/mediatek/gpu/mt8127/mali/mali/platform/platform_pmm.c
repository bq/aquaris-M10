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

#include <linux/mali/mali_utgard.h>
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "platform_pmm.h"
#include <linux/kernel.h>
#include <asm/atomic.h>
#include "arm_core_scaling.h"
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>

#if defined(CONFIG_MALI400_PROFILING)
#include "mali_osk_profiling.h"
#endif

#ifdef CONFIG_MALI_DT
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/clk-private.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/init.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include "mt-plat/mt_smi.h"
#include <linux/proc_fs.h>
#else
#include "mach/mt_gpufreq.h"
#endif

/*
extern unsigned long (*mtk_thermal_get_gpu_loading_fp) (void);
extern unsigned long (*mtk_get_gpu_loading_fp) (void);
*/

static int bPoweroff;
unsigned int current_sample_utilization;

extern u32 get_devinfo_with_index(u32 index);
static int _need_univpll;

#ifdef CONFIG_MALI_DT

/* MFG begin
 * GPU controller.
 *
 * If GPU power domain needs to be enabled after disp, we will break flow
 * of mfg_enable_gpu to be 2 functions like mfg_prepare/mfg_enable.
 *
 * Currently no lock for mfg, Mali driver will take .
 */

#define MFG_CG_CON 0x0
#define MFG_CG_SET 0x4
#define MFG_CG_CLR 0x8
#define MFG_DEBUG_SEL 0x180
#define MFG_DEBUG_STAT 0x184
#define MFG_SPD_MASK 0x80000
#define MFG_GPU_QUAL_MASK 0x3

#define MFG_READ32(r) __raw_readl((void __iomem *)((unsigned long)mfg_start + (r)))
#define MFG_WRITE32(v, r) __raw_writel((v), (void __iomem *)((unsigned long)mfg_start + (r)))

static struct platform_device *mfg_dev;
static void __iomem *mfg_start;
static void __iomem *scp_start;

#ifdef CONFIG_OF
static const struct of_device_id mfg_dt_ids[] = {
	{.compatible = "mediatek,mt8127-mfg"},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, mfg_dt_ids);
#endif

static int mfg_device_probe(struct platform_device *pdev)
{

	/* Make sure disp pm is ready to operate .. */
	if (!mtk_smi_larb_get_base(0)) {
		pr_warn("MFG is defer for disp domain ready\n");
		return -EPROBE_DEFER;
	} else
		pr_info("MFG domain ready\n");

	mfg_start = of_iomap(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(mfg_start)) {
		mfg_start = NULL;
		goto error_out;
	}
	pr_info("MFG start is mapped %p\n", mfg_start);

	pm_runtime_set_autosuspend_delay(&(pdev->dev), 300);
	pm_runtime_use_autosuspend(&(pdev->dev));

	pm_runtime_enable(&pdev->dev);

	mfg_dev = pdev;
	{
		struct device_node *node;
		static const struct of_device_id scp_ids[] = {
			{.compatible = "mediatek,mt8127-scpsys"},
				{ /* sentinel */ }
		};
		node = of_find_matching_node(NULL, scp_ids);
		if (node)
			scp_start = of_iomap(node, 0);
		pr_info("MFG scp_start is mapped %p\n", scp_ids);
	}
	pr_info("MFG device probed\n");
	return 0;
error_out:
	if (mfg_start)
		iounmap(mfg_start);
	if (scp_start)
		iounmap(scp_start);

	return -1;
}

static int mfg_device_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);

	if (mfg_start)
		iounmap(mfg_start);
	if (scp_start)
		iounmap(scp_start);

	return 0;
}

static struct platform_driver mtk_mfg_driver = {
	.probe = mfg_device_probe,
	.remove = mfg_device_remove,
	.driver = {
		   .name = "mfg",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(mfg_dt_ids),
		   },
};

#define MFG_DUMP_FOR(base, off) \
do {\
	u32 val;\
	val = __raw_readl(base + off);\
	pr_info("pwr_dump %s [%#x]: 0x%x\n", #base, (u32)off, val); \
	} while (0)

#define DEBUG_MFG_STAT \
do {\
	u32 con = 0xDEAC;\
	con = MFG_READ32(MFG_CG_CON);\
	pr_debug("MFG %s #%d CON: 0x%x\n", __func__, __LINE__, con);	\
} while (0)

static int mfg_enable_gpu(void)
{
	int ret = -1, i = 10;
	u32 con;

	if (mfg_start == NULL)
		return ret;
	ret = pm_runtime_get_sync(&mfg_dev->dev);
	if (ret < 0) {
		/*
		pm_runtime_enable(&mfg_dev->dev);
		ret = pm_runtime_get_sync(&mfg_dev->dev);
		*/
		pr_warn("MFG %s #%d get DISP[%d]\n", __func__, __LINE__, ret);
	}
	ret = mtk_smi_larb_clock_on(0, false);

	i = 10;
	DEBUG_MFG_STAT;
	do {
		MFG_WRITE32(0x1, MFG_CG_CLR);
		con = MFG_READ32(MFG_CG_CON);
		if (con == 0)
			break;
		else
			pr_warn("MFG MFG_CG_CON[0x%x]", con);
	} while (i--);
	DEBUG_MFG_STAT;

	return ret;
}

static void mfg_disable_gpu(void)
{
	if (mfg_start == NULL)
		return;

	DEBUG_MFG_STAT;
	MFG_WRITE32(0x1, MFG_CG_SET);
	DEBUG_MFG_STAT;
	mtk_smi_larb_clock_off(0, false);
	pm_runtime_mark_last_busy(&mfg_dev->dev);
	pm_runtime_put_autosuspend(&mfg_dev->dev);
}

static int __init mfg_driver_init(void)
{
	int ret;

	ret = platform_driver_register(&mtk_mfg_driver);
	return ret;
}

bool mtk_mfg_is_ready(void)
{
	return (mfg_start != NULL);
}

/* We need make mfg probed before GPU */
late_initcall(mfg_driver_init);

/* MFG end */

struct _mfg_base {
	void __iomem *g3d_base;
	struct clk *mm_smi;
	struct clk *mfg_pll;
	struct clk *mfg_sel;
	struct regulator *vdd_g3d;
};

static struct _mfg_base mfg_base;

#define REG_MFG_G3D BIT(0)

#define REG_MFG_CG_STA 0x00
#define REG_MFG_CG_SET 0x04
#define REG_MFG_CG_CLR 0x08

int mali_mfgsys_init(struct platform_device *device)
{
	int err = 0;
	struct clk *parent;
	unsigned long freq;

	mfg_base.g3d_base = mfg_start;

	mfg_base.mm_smi = devm_clk_get(&device->dev, "mm_smi");
	if (IS_ERR(mfg_base.mm_smi)) {
		err = PTR_ERR(mfg_base.mm_smi);
		dev_err(&device->dev, "devm_clk_get mm_smi failed\n");
		goto err_iounmap_reg_base;
	}
	if (!_need_univpll) {
		mfg_base.mfg_pll = devm_clk_get(&device->dev, "mfg_pll");
		if (IS_ERR(mfg_base.mfg_pll)) {
			err = PTR_ERR(mfg_base.mfg_pll);
			dev_err(&device->dev, "devm_clk_get mfg_pll failed\n");
			goto err_iounmap_reg_base;
		}
	} else{
			mfg_base.mfg_pll = devm_clk_get(&device->dev, "mfg_pll_univ");
		if (IS_ERR(mfg_base.mfg_pll)) {
			err = PTR_ERR(mfg_base.mfg_pll);
			dev_err(&device->dev, "devm_clk_get mfg_pll_univ failed\n");
			goto err_iounmap_reg_base;
		}
	}
	mfg_base.mfg_sel = devm_clk_get(&device->dev, "mfg_sel");
	if (IS_ERR(mfg_base.mfg_sel)) {
		err = PTR_ERR(mfg_base.mfg_sel);
		dev_err(&device->dev, "devm_clk_get mfg_sel failed\n");
		goto err_iounmap_reg_base;
	}
	clk_prepare_enable(mfg_base.mfg_sel);

	err = clk_set_parent(mfg_base.mfg_sel, mfg_base.mfg_pll);
	if (err != 0) {
		dev_err(&device->dev, "failed to clk_set_parent\n");
		goto err_iounmap_reg_base;
	}
	parent = clk_get_parent(mfg_base.mfg_sel);
	if (!IS_ERR_OR_NULL(parent)) {
		pr_info("MFG is now selected to %s\n", parent->name);
		freq = clk_get_rate(parent);
		pr_info("MFG parent rate %lu\n", freq);
		/* Don't set rate here, gpufreq will do this */
	} else {
		pr_err("Failed to select mfg\n");
	}

	/*clk_disable_unprepare(mfg_base.mfg_sel);*/

	mfg_base.vdd_g3d = devm_regulator_get(&device->dev, "vdd_g3d");
	if (IS_ERR(mfg_base.vdd_g3d)) {
		err = PTR_ERR(mfg_base.vdd_g3d);
		goto err_iounmap_reg_base;
	}

	err = regulator_enable(mfg_base.vdd_g3d);
	if (err != 0) {
		dev_err(&device->dev, "failed to enable regulator vdd_g3d\n");
		goto err_iounmap_reg_base;
	}

	return 0;

err_iounmap_reg_base:

	return err;
}

void mali_mfgsys_deinit(struct platform_device *device)
{
	MALI_IGNORE(device);
	pm_runtime_disable(&device->dev);
	regulator_disable(mfg_base.vdd_g3d);
}
void dump_clk_state(void)
{
	MALI_DEBUG_PRINT(2, ("mali platform_mmt dump_clk_state smi_ref[%d], smi_enabled[%d]\n",
		__clk_get_enable_count(mfg_base.mm_smi), __clk_is_enabled(mfg_base.mm_smi)));
	MALI_DEBUG_PRINT(2, ("MFG %s #%d MFG_DEBUG_SEL: 0x%x\n", __func__, __LINE__, MFG_READ32(MFG_DEBUG_SEL)));
	MALI_DEBUG_PRINT(2, ("MFG %s #%d MFG_DEBUG_CON: %x\n", __func__, __LINE__, MFG_READ32(MFG_CG_CON)));
	if (scp_start) {
		MFG_DUMP_FOR(scp_start, 0x060c); /*SPM_PWR_STATUS*/
		MFG_DUMP_FOR(scp_start, 0x0610); /*SPM_PWR_STATUS_2ND*/
	}
	mali_platform_power_mode_change(NULL, MALI_POWER_MODE_ON);
}
int mali_clk_enable(struct device *device)
{
	int ret;
	/*clk_prepare_enable(mfg_base.mfg_sel);*/

	ret = mfg_enable_gpu();

	MALI_DEBUG_PRINT(3, ("MFG %s #%d MFG_DEBUG_SEL: 0x%x\n", __func__, __LINE__, MFG_READ32(MFG_DEBUG_SEL)));
	MALI_DEBUG_PRINT(3, ("MFG %s #%d MFG_DEBUG_CON: %x\n", __func__, __LINE__, MFG_READ32(MFG_CG_CON)));
	MALI_DEBUG_PRINT(2, ("mali_clk_enable![%d]\n", ret));

	return 0;
}

int mali_clk_disable(struct device *device)
{
	mfg_disable_gpu();
	/*clk_disable_unprepare(mfg_base.mfg_sel);*/
	MALI_DEBUG_PRINT(2, ("mali_clk_disable done\n"));

	return 0;
}
#endif

int mali_pmm_init(struct platform_device *device)
{
	int err = 0;
	u32 idx = 0;

	MALI_DEBUG_PRINT(1, ("%s\n", __func__));
	idx = get_devinfo_with_index(3);
	if (idx & MFG_SPD_MASK)
		_need_univpll = 1;
	else
		_need_univpll = 0;
	MALI_DEBUG_PRINT(2, ("need univ src pll idx0x%d %d\n", idx, _need_univpll));

	/* Because clkmgr may do 'default on' for some clock.
	   We check the clock state on init and set power state atomic.
	 */

	MALI_DEBUG_PRINT(1, ("MFG G3D init enable if it is on\n"));
#ifndef CONFIG_MALI_DT
	mtk_thermal_get_gpu_loading_fp = gpu_get_current_utilization;
	mtk_get_gpu_loading_fp = gpu_get_current_utilization;
	if (clock_is_on(MT_CG_MFG_G3D)) {
		MALI_DEBUG_PRINT(1, ("MFG G3D default on\n"));
		atomic_set((atomic_t *) &bPoweroff, 0);
		/* Need call enable first for 'default on' clocks.
		 * Canbe removed if clkmgr remove this requirement.
		 */
		enable_clock(MT_CG_DISP0_SMI_COMMON, "MFG");
		enable_clock(MT_CG_MFG_G3D, "MFG");
	} else {
		MALI_DEBUG_PRINT(1, ("MFG G3D init default off\n"));
		atomic_set((atomic_t *) &bPoweroff, 1);
	}
#else
	err = mali_mfgsys_init(device);
	if (err)
		return err;
	atomic_set((atomic_t *) &bPoweroff, 1);
#endif
	/*mali_platform_power_mode_change(&(device->dev), MALI_POWER_MODE_ON);*/

	return err;
}

void mali_pmm_deinit(struct platform_device *device)
{
	MALI_DEBUG_PRINT(1, ("%s\n", __func__));

	mali_platform_power_mode_change(&device->dev, MALI_POWER_MODE_DEEP_SLEEP);
	mali_mfgsys_deinit(device);
}

unsigned int gpu_get_current_utilization(void)
{
	return (current_sample_utilization * 100) / 256;
}

void mali_platform_power_mode_change(struct device *device,
				     mali_power_mode power_mode)
{
	switch (power_mode) {
	case MALI_POWER_MODE_ON:
		MALI_DEBUG_PRINT(3,
				 ("Mali platform: Got MALI_POWER_MODE_ON event, %s\n",
				  atomic_read((atomic_t *) &bPoweroff) ?
				  "powering on" : "already on"));
		if (atomic_read((atomic_t *) &bPoweroff) == 1) {
			/*Leave this to undepend ref count of clkmgr */
			#ifndef CONFIG_MALI_DT
			if (!clock_is_on(MT_CG_MFG_G3D)) {
				MALI_DEBUG_PRINT(3, ("MFG enable_clock\n"));
				if (_need_univpll) {
					enable_pll(UNIVPLL, "GPU");
				}
				enable_clock(MT_CG_DISP0_SMI_COMMON, "MFG");
				enable_clock(MT_CG_MFG_G3D, "MFG");
				if (_need_univpll) {
					clkmux_sel(MT_MUX_MFG, 6, "GPU");
				}
				atomic_set((atomic_t *) &bPoweroff, 0);
			}
			#else
			if (!mali_clk_enable(device))
				atomic_set((atomic_t *) &bPoweroff, 0);
			#endif
#if defined(CONFIG_MALI400_PROFILING)
			_mali_osk_profiling_add_event
			    (MALI_PROFILING_EVENT_TYPE_SINGLE |
			     MALI_PROFILING_EVENT_CHANNEL_GPU |
			     MALI_PROFILING_EVENT_REASON_SINGLE_GPU_FREQ_VOLT_CHANGE,
			     500, 1200 / 1000, 0, 0, 0);

#endif
		}
		break;
	case MALI_POWER_MODE_LIGHT_SLEEP:
	case MALI_POWER_MODE_DEEP_SLEEP:
		MALI_DEBUG_PRINT(3,
				 ("Mali platform: Got %s event, %s\n",
				  power_mode ==
				  MALI_POWER_MODE_LIGHT_SLEEP ?
				  "MALI_POWER_MODE_LIGHT_SLEEP" :
				  "MALI_POWER_MODE_DEEP_SLEEP",
				  atomic_read((atomic_t *) &bPoweroff) ?
				  "already off" : "powering off"));

		if (atomic_read((atomic_t *) &bPoweroff) == 0) {
			#ifndef CONFIG_MALI_DT
			if (clock_is_on(MT_CG_MFG_G3D)) {
				MALI_DEBUG_PRINT(3, ("MFG disable_clock\n"));
				disable_clock(MT_CG_MFG_G3D, "MFG");
				disable_clock(MT_CG_DISP0_SMI_COMMON, "MFG");
				if (_need_univpll) {
					disable_pll(UNIVPLL, "GPU");
				}
				atomic_set((atomic_t *) &bPoweroff, 1);
			}
			#else
			if (!mali_clk_disable(device))
				atomic_set((atomic_t *) &bPoweroff, 1);
			#endif
#if defined(CONFIG_MALI400_PROFILING)
			_mali_osk_profiling_add_event
			    (MALI_PROFILING_EVENT_TYPE_SINGLE |
			     MALI_PROFILING_EVENT_CHANNEL_GPU |
			     MALI_PROFILING_EVENT_REASON_SINGLE_GPU_FREQ_VOLT_CHANGE,
			     0, 0, 0, 0, 0);
#endif
		}
		break;
	}
}
