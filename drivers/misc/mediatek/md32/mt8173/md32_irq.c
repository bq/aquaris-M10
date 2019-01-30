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

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <mach/md32_ipi.h>
#include <mach/md32_helper.h>
#include <mt-plat/aee.h>
#include <mt-plat/sync_write.h>

#include "md32_irq.h"

#define MD32_CFGREG_SIZE	0x100
#define MD32_AED_PHY_SIZE	(MD32_PTCM_SIZE + MD32_DTCM_SIZE + \
				MD32_CFGREG_SIZE)
#define MD32_AED_STR_LEN	(512)

#define IPC_MD2HOST	(1 << 0)
#define MD32_IPC_INT	(1 << 8)
#define WDT_INT		(1 << 9)
#define PMEM_DISP_INT	(1 << 10)
#define DMEM_DISP_INT	(1 << 11)

static struct workqueue_struct *wq_md32_reboot;

struct reg_md32_to_host_ipc {
	unsigned int ipc_md2host	:1;
	unsigned int			:7;
	unsigned int md32_ipc_int	:1;
	unsigned int wdt_int		:1;
	unsigned int pmem_disp_int	:1;
	unsigned int dmem_disp_int	:1;
	unsigned int			:20;
};

struct md32_aed_cfg {
	int *log;
	int log_size;
	int *phy;
	int phy_size;
	char *detail;
};

struct md32_reboot_work {
	struct work_struct work;
	struct md32_aed_cfg aed;
};

static struct md32_reboot_work work_md32_reboot;

int md32_dump_regs(char *buf)
{
	unsigned long *reg;
	char *ptr = buf;

	if (!buf)
		return 0;
	/* dbg_msg("md32_dump_regs\n"); */
	ptr +=
	    sprintf(ptr, "md32 pc=0x%08x, r14=0x%08x, r15=0x%08x\n",
		    readl(MD32_DEBUG_PC_REG), readl(MD32_DEBUG_R14_REG),
		    readl(MD32_DEBUG_R15_REG));
	ptr += sprintf(ptr, "md32 to host inerrupt = 0x%08x\n",
		       readl(MD32_TO_HOST_REG));
	ptr += sprintf(ptr, "wdt en=%d, count=0x%08x\n",
		       (readl(MD32_WDT_REG) & 0x10000000) ? 1 : 0,
		       (readl(MD32_WDT_REG) & 0xFFFFF));

	/*dump all md32 regs */
	for (reg = (unsigned long *)MD32_BASE;
	     (unsigned long)reg < (unsigned long)(MD32_BASE + 0x90); reg++) {
		if (!((unsigned long)reg & 0xF)) {
			ptr += sprintf(ptr, "\n");
			ptr += sprintf(ptr, "[0x%016lx]   ",
				       (unsigned long)reg);
		}
		ptr += sprintf(ptr, "0x%016lx  ", *reg);
	}
	ptr += sprintf(ptr, "\n");

	return ptr - buf;
}

void md32_prepare_aed(char *aed_str, struct md32_aed_cfg *aed)
{
	char *detail;
	u8 *log, *phy, *ptr;
	u32 log_size, phy_size;

	detail = kmalloc(MD32_AED_STR_LEN, GFP_KERNEL);

	ptr = detail;
	detail[MD32_AED_STR_LEN - 1] = '\0';
	ptr += snprintf(detail, MD32_AED_STR_LEN, "%s", aed_str);
	ptr +=
		sprintf(ptr, "md32 pc=0x%08x, r14=0x%08x, r15=0x%08x\n",
			readl(MD32_DEBUG_PC_REG), readl(MD32_DEBUG_R14_REG),
			readl(MD32_DEBUG_R15_REG));

	phy_size = MD32_AED_PHY_SIZE;

	phy = kmalloc(phy_size, GFP_KERNEL);

	ptr = phy;
	memcpy((void *)ptr, (void *)MD32_BASE, MD32_CFGREG_SIZE);

	ptr += MD32_CFGREG_SIZE;

	memcpy((void *)ptr, (void *)MD32_PTCM, MD32_PTCM_SIZE);

	ptr += MD32_PTCM_SIZE;

	memcpy((void *)ptr, (void *)MD32_DTCM, MD32_DTCM_SIZE);

	ptr += MD32_DTCM_SIZE;

	log_size = 0x10000;	/* 64K for try */
	log = kmalloc(log_size, GFP_KERNEL);
	if (!log) {
		pr_err("ap allocate buffer fail\n");
		log_size = 0;
	} else {
		int size;

		memset(log, 0, log_size);

		ptr = log;

		ptr += md32_dump_regs(ptr);

		/* print log in kernel */
		pr_debug("%s", log);

		ptr += sprintf(ptr, "dump memory info\n");
		ptr += sprintf(ptr, "md32 cfgreg: 0x%08x\n", 0);
		ptr += sprintf(ptr, "md32 ptcm  : 0x%08x\n", MD32_CFGREG_SIZE);
		ptr += sprintf(ptr, "md32 dtcm  : 0x%08x\n",
			       MD32_CFGREG_SIZE + MD32_PTCM_SIZE);
		ptr += sprintf(ptr, "<<md32 log buf>>\n");
		size = log_size - (ptr - log);
		ptr += md32_get_log_buf(ptr, size);
		*ptr = '\0';
		log_size = ptr - log;
	}

	aed->log = (int *)log;
	aed->log_size = log_size;
	aed->phy = (int *)phy;
	aed->phy_size = phy_size;
	aed->detail = detail;
}

void md32_dmem_abort_handler(void)
{
	pr_err("[MD32] DMEM Abort\n");
}

void md32_pmem_abort_handler(void)
{
	pr_err("[MD32] PMEM Abort\n");
}

irqreturn_t md32_irq_handler(int irq, void *dev_id)
{
	struct reg_md32_to_host_ipc *md32_irq;
	int reboot = 0;

	md32_irq = (struct reg_md32_to_host_ipc *)MD32_TO_HOST_ADDR;

	if (md32_irq->wdt_int) {
		md32_prepare_aed("md32 wdt", &work_md32_reboot.aed);
		reboot = 1;
		md32_irq->wdt_int = 0;
	}

	if (md32_irq->pmem_disp_int) {
		md32_prepare_aed("md32 pmem abort", &work_md32_reboot.aed);
		md32_pmem_abort_handler();
		md32_irq->pmem_disp_int = 0;
		/* reboot = 1; */
	}

	if (md32_irq->dmem_disp_int) {
		md32_dmem_abort_handler();
		md32_irq->dmem_disp_int = 0;
		/* reboot = 1; */
	}

	if (md32_irq->md32_ipc_int) {
		md32_ipi_handler();
		md32_irq->ipc_md2host = 0;
		md32_irq->md32_ipc_int = 0;
	}

	writel(0x0, MD32_TO_HOST_REG);

	if (reboot) {
		queue_work(wq_md32_reboot,
			   (struct work_struct *)&work_md32_reboot);
	}

	return IRQ_HANDLED;
}

void md32_reboot_from_irq(struct work_struct *ws)
{
	struct md32_reboot_work *rb_ws = (struct md32_reboot_work *)ws;
	struct md32_aed_cfg *aed = &rb_ws->aed;

	aed_md32_exception_api(aed->log, aed->log_size, aed->phy, aed->phy_size,
			       aed->detail, DB_OPT_DEFAULT);

	pr_warn("%s", aed->detail);
}

void md32_irq_init(void)
{
	writel(0x0, MD32_TO_HOST_REG); /* clear md32 irq */

	wq_md32_reboot = create_workqueue("MD32_REBOOT_WQ");

	INIT_WORK((struct work_struct *)&work_md32_reboot,
		  md32_reboot_from_irq);
}
