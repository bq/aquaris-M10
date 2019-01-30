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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include "mach/emi_bwl.h"
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_meminfo.h>


DEFINE_SEMAPHORE(emi_bwl_sem);

static const struct of_device_id emi_bwl_of_match[] = {
	{ .compatible = "mediatek,mt2701-emi", },
	{ .compatible = "mediatek,mt8127-emi", },
	{ .compatible = "mediatek,mt7623-emi", },
	{}
};

static struct mt_emi_bwl_of emi_bwl_of;

struct mt_emi_bwl_of {
	void __iomem *emi_bwl_regs;
};

/* define EMI bandwiwth limiter control table */
static struct emi_bwl_ctrl ctrl_tbl[NR_CON_SCE];

/* current concurrency scenario */
static int cur_con_sce = 0x0FFFFFFF;

/* define concurrency scenario strings */
static const char * const con_sce_str[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) (#con_sce),
#include "mach/con_sce_lpddr2.h"
#undef X_CON_SCE
};

/* define EMI bandwidth allocation tables */
/****************** For LPDDR2 ******************/
static const unsigned int emi_arba_lpddr2_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arba,
#include "mach/con_sce_lpddr2.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbb_lpddr2_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbb,
#include "mach/con_sce_lpddr2.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbc_lpddr2_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbc,
#include "mach/con_sce_lpddr2.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbd_lpddr2_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbd,
#include "mach/con_sce_lpddr2.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbe_lpddr2_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbe,
#include "mach/con_sce_lpddr2.h"
#undef X_CON_SCE
};

/****************** For DDR3-16bit ******************/

static const unsigned int emi_arba_ddr3_16_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arba,
#include "mach/con_sce_ddr3_16.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbb_ddr3_16_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbb,
#include "mach/con_sce_ddr3_16.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbc_ddr3_16_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbc,
#include "mach/con_sce_ddr3_16.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbd_ddr3_16_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbd,
#include "mach/con_sce_ddr3_16.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbe_ddr3_16_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbe,
#include "mach/con_sce_ddr3_16.h"
#undef X_CON_SCE
};

/****************** For LPDDR3 ******************/

static const unsigned int emi_arba_lpddr3_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arba,
#include "mach/con_sce_lpddr3.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbb_lpddr3_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbb,
#include "mach/con_sce_lpddr3.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbc_lpddr3_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbc,
#include "mach/con_sce_lpddr3.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbd_lpddr3_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbd,
#include "mach/con_sce_lpddr3.h"
#undef X_CON_SCE
};

static const unsigned int emi_arbe_lpddr3_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe) arbe,
#include "mach/con_sce_lpddr3.h"
#undef X_CON_SCE
};

int get_ddr_type(void)
{
	unsigned int value;

	value = ucDram_Register_Read(DRAMC_LPDDR2);
	if ((value >> 28) & 0x1)
		return LPDDR2;

	value = ucDram_Register_Read(DRAMC_PADCTL4);
	if ((value >> 7) & 0x1) {
		if (ucDram_Register_Read(DRAMC_CONF1) & 0x1)
			return DDR3_32;
		else
			return DDR3_16;
	}

	value = ucDram_Register_Read(DRAMC_ACTIM1);
	if ((value >> 28) & 0x1)
		return LPDDR3;

	return mDDR;
}
EXPORT_SYMBOL(get_ddr_type);

/*
 * mtk_mem_bw_ctrl: set EMI bandwidth limiter for memory bandwidth control
 * @sce: concurrency scenario ID
 * @op: either ENABLE_CON_SCE or DISABLE_CON_SCE
 * Return 0 for success; return negative values for failure.
 */
int mtk_mem_bw_ctrl(int sce, int op)
{
	int i, highest;

	if (sce >= NR_CON_SCE)
		return -1;

	if (op != ENABLE_CON_SCE && op != DISABLE_CON_SCE)
		return -1;

	if (in_interrupt())
		return -1;

	down(&emi_bwl_sem);

	if (op == ENABLE_CON_SCE) {
		ctrl_tbl[sce].ref_cnt++;
	} else if (op == DISABLE_CON_SCE) {
		if (ctrl_tbl[sce].ref_cnt != 0)
			ctrl_tbl[sce].ref_cnt--;
	}

  /* find the scenario with the highest priority */
	highest = -1;
	for (i = 0; i < NR_CON_SCE; i++) {
		if (ctrl_tbl[i].ref_cnt != 0) {
			highest = i;
			break;
		}
	}

	if (highest == -1)
		highest = CON_SCE_NORMAL;

  /* set new EMI bandwidth limiter value */
	if (highest != cur_con_sce) {
		if (get_ddr_type() == LPDDR2) {
			mt_reg_sync_writel(emi_arba_lpddr2_val[highest], EMI_ARBA);
			mt_reg_sync_writel(emi_arbb_lpddr2_val[highest], EMI_ARBB);
			mt_reg_sync_writel(emi_arbc_lpddr2_val[highest], EMI_ARBC);
			mt_reg_sync_writel(emi_arbd_lpddr2_val[highest], EMI_ARBD);
			mt_reg_sync_writel(emi_arbe_lpddr2_val[highest], EMI_ARBE);
		} else if (get_ddr_type() == DDR3_16) {
			mt_reg_sync_writel(emi_arba_ddr3_16_val[highest], EMI_ARBA);
			mt_reg_sync_writel(emi_arbb_ddr3_16_val[highest], EMI_ARBB);
			mt_reg_sync_writel(emi_arbc_ddr3_16_val[highest], EMI_ARBC);
			mt_reg_sync_writel(emi_arbd_ddr3_16_val[highest], EMI_ARBD);
			mt_reg_sync_writel(emi_arbe_ddr3_16_val[highest], EMI_ARBE);
		} else if (get_ddr_type() == LPDDR3) {
			mt_reg_sync_writel(emi_arba_lpddr3_val[highest], EMI_ARBA);
			mt_reg_sync_writel(emi_arbb_lpddr3_val[highest], EMI_ARBB);
			mt_reg_sync_writel(emi_arbc_lpddr3_val[highest], EMI_ARBC);
			mt_reg_sync_writel(emi_arbd_lpddr3_val[highest], EMI_ARBD);
			mt_reg_sync_writel(emi_arbe_lpddr3_val[highest], EMI_ARBE);
		}
		cur_con_sce = highest;
	}

	up(&emi_bwl_sem);

	return 0;
}

/*
 * ddr_type_show: sysfs ddr_type file show function.
 * @driver:
 * @buf: the string of ddr type
 * Return the number of read bytes.
 */
static ssize_t ddr_type_show(struct device_driver *driver, char *buf)
{
	if (get_ddr_type() == LPDDR2)
		sprintf(buf, "LPDDR2\n");
	else if (get_ddr_type() == DDR3_16)
		sprintf(buf, "DDR3_16\n");
	else if (get_ddr_type() == DDR3_32)
		sprintf(buf, "DDR3_32\n");
	else if (get_ddr_type() == LPDDR3)
		sprintf(buf, "LPDDR3\n");
	else
		sprintf(buf, "mDDR\n");

	return strlen(buf);
}

/*
 * ddr_type_store: sysfs ddr_type file store function.
 * @driver:
 * @buf:
 * @count:
 * Return the number of write bytes.
 */
static ssize_t ddr_type_store(struct device_driver *driver, const char *buf, size_t count)
{
	/*do nothing*/
	return count;
}

DRIVER_ATTR(ddr_type, 0644, ddr_type_show, ddr_type_store);


static ssize_t ddr_size_show(struct device_driver *driver, char *buf)
{
	sprintf(buf, "%d\n", get_max_DRAM_size());

	return strlen(buf);
}


DRIVER_ATTR(ddr_size, 0644, ddr_size_show, NULL);


/*
 * con_sce_show: sysfs con_sce file show function.
 * @driver:
 * @buf:
 * Return the number of read bytes.
 */
static ssize_t con_sce_show(struct device_driver *driver, char *buf)
{
	if (cur_con_sce >= NR_CON_SCE)
		sprintf(buf, "none\n");
	else
		sprintf(buf, "%s\n", con_sce_str[cur_con_sce]);

	return strlen(buf);
}

/*
 * con_sce_store: sysfs con_sce file store function.
 * @driver:
 * @buf:
 * @count:
 * Return the number of write bytes.
 */
static ssize_t con_sce_store(struct device_driver *driver, const char *buf, size_t count)
{
	int i;

	for (i = 0; i < NR_CON_SCE; i++) {
		if (!strncmp(buf, con_sce_str[i], strlen(con_sce_str[i]))) {
			if (!strncmp(buf + strlen(con_sce_str[i]) + 1, EN_CON_SCE_STR, strlen(EN_CON_SCE_STR))) {
				mtk_mem_bw_ctrl(i, ENABLE_CON_SCE);
				pr_info("concurrency scenario %s ON\n", con_sce_str[i]);
				break;
			} else if (!strncmp(buf + strlen(con_sce_str[i]) + 1,
						DIS_CON_SCE_STR, strlen(DIS_CON_SCE_STR))) {
				mtk_mem_bw_ctrl(i, DISABLE_CON_SCE);
				pr_info("concurrency scenario %s OFF\n", con_sce_str[i]);
				break;
			}
		}
	}

	return count;
}

DRIVER_ATTR(concurrency_scenario, 0644, con_sce_show, con_sce_store);

static int emi_bwl_probe(struct platform_device *dev)
{
	emi_bwl_of.emi_bwl_regs = of_iomap(dev->dev.of_node, 0);

	return 0;
}

static struct platform_driver ddr_type = {
	.driver = {
		.name = "ddr_type",
		.owner = THIS_MODULE,
	},
};

static struct platform_driver emi_bwl_driver = {
	.probe = emi_bwl_probe,
	.driver = {
		.name = "mem_bw_ctrl",
		.of_match_table = emi_bwl_of_match,
	},
};

/*
 * emi_bwl_mod_init: module init function.
 */
static int __init emi_bwl_mod_init(void)
{
	int ret;

	/*register emi bwl*/
	ret = platform_driver_register(&emi_bwl_driver);
	if (ret)
		pr_err("fail to register EMI_BW_LIMITER driver\n");

	ret = driver_create_file(&emi_bwl_driver.driver, &driver_attr_concurrency_scenario);
	if (ret)
		pr_err("fail to create EMI_BW_LIMITER sysfs file\n");

	/*register ddr type*/
	ret = platform_driver_register(&ddr_type);
	if (ret)
		pr_err("fail to register DDR TYPE driver\n");

	ret = driver_create_file(&ddr_type.driver, &driver_attr_ddr_type);
	if (ret)
		pr_err("fail to create DRAM_TYPE sysfs file\n");

	ret = driver_create_file(&ddr_type.driver, &driver_attr_ddr_size);
	if (ret)
		pr_err("fail to create DRAM_SIZE sysfs file\n");

	/*set normal concurrency*/
	ret = mtk_mem_bw_ctrl(CON_SCE_NORMAL, ENABLE_CON_SCE);
	if (ret)
		pr_err("fail to set EMI bandwidth limiter\n");

	return 0;
}

/*
 * emi_bwl_mod_exit: module exit function.
 */
static void __exit emi_bwl_mod_exit(void)
{
}

late_initcall(emi_bwl_mod_init);
module_exit(emi_bwl_mod_exit);

