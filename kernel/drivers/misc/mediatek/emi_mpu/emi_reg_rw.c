#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <mach/mt_reg_base.h>
#include <mach/mt_device_apc.h>
#include <mach/sync_write.h>
#include <mach/irqs.h>
#include <mach/dma.h>
#include <mach/memory.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <mach/emi_mpu.h>
#include <mach/mt_secure_api.h>

static void __iomem *EMI_BASE_ADDR;

static int is_emi_mpu_reg(unsigned int offset)
{
	if (offset >= EMI_MPUA && offset <= EMI_MPUY2)
		return 1;

	return 0;
}

void mt_emi_reg_write(unsigned int data, unsigned int offset)
{
#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
	if (is_emi_mpu_reg(offset)) {
		emi_mpu_smc_write(offset, data);
		return;
	}
#endif

	if (EMI_BASE_ADDR)
		mt_reg_sync_writel(data, EMI_BASE_ADDR + offset);
}

unsigned int mt_emi_reg_read(unsigned int offset)
{
#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
	if (is_emi_mpu_reg(offset))
		return (unsigned int)emi_mpu_smc_read(offset);
#endif

	if (EMI_BASE_ADDR)
		return readl(IOMEM(EMI_BASE_ADDR + offset));

	return 0;
}

void mt_emi_reg_base_set(void *base)
{
	EMI_BASE_ADDR = base;
}

void *mt_emi_reg_base_get(void)
{
	return EMI_BASE_ADDR;
}
