/*
* Copyright (C) 2016 MediaTek Inc.

* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

* This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/vmalloc.h>
#include <linux/memblock.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/cacheflush.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <mt-plat/mt_io.h>
#include <mt-plat/dma.h>
#include <mt-plat/sync_write.h>

#include "mt_dramc.h"

#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif
#ifdef CONFIG_MTK_DCM
#include <mach/mt_dcm.h>
#endif

void __iomem *DRAMCAO_BASE_ADDR;
void __iomem *DDRPHY_BASE_ADDR;
void __iomem *DRAMCNAO_BASE_ADDR;

static int dt_init_done;
static int __init dram_dt_init(void);

int Binning_DRAM_complex_mem_test(void)
{
	unsigned char *MEM8_BASE;
	unsigned short *MEM16_BASE;
	unsigned int *MEM32_BASE;
	unsigned int *MEM_BASE;
	unsigned long mem_ptr;
	unsigned char pattern8;
	unsigned short pattern16;
	unsigned int i, j, size, pattern32;
	unsigned int value;
	unsigned int len = MEM_TEST_SIZE;
	void *ptr;

	ptr = vmalloc(PAGE_SIZE*2);
	MEM8_BASE = (unsigned char *)ptr;
	MEM16_BASE = (unsigned short *)ptr;
	MEM32_BASE = (unsigned int *)ptr;
	MEM_BASE = (unsigned int *)ptr;
	pr_debug("Test DRAM start address 0x%lx\n", (unsigned long)ptr);
	pr_debug("Test DRAM SIZE 0x%x\n", MEM_TEST_SIZE);
	size = len >> 2;

	/* === Verify the tied bits (tied high) === */
	for (i = 0; i < size; i++)
		MEM32_BASE[i] = 0;

	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0) {
			vfree(ptr);
			return -1;
		}
		MEM32_BASE[i] = 0xffffffff;
	}

	/* === Verify the tied bits (tied low) === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffffffff) {
			vfree(ptr);
			return -2;
		}
		MEM32_BASE[i] = 0x00;
	}

	/* === Verify pattern 1 (0x00~0xff) === */
	pattern8 = 0x00;
	for (i = 0; i < len; i++)
		MEM8_BASE[i] = pattern8++;
	pattern8 = 0x00;
	for (i = 0; i < len; i++) {
		if (MEM8_BASE[i] != pattern8++) {
			vfree(ptr);
			return -3;
		}
	}

	/* === Verify pattern 2 (0x00~0xff) === */
	pattern8 = 0x00;
	for (i = j = 0; i < len; i += 2, j++) {
		if (MEM8_BASE[i] == pattern8)
			MEM16_BASE[j] = pattern8;
		if (MEM16_BASE[j] != pattern8) {
			vfree(ptr);
			return -4;
		}
		pattern8 += 2;
	}

	/* === Verify pattern 3 (0x00~0xffff) === */
	pattern16 = 0x00;
	for (i = 0; i < (len >> 1); i++)
		MEM16_BASE[i] = pattern16++;
	pattern16 = 0x00;
	for (i = 0; i < (len >> 1); i++) {
		if (MEM16_BASE[i] != pattern16++) {
			vfree(ptr);
			return -5;
		}
	}

	/* === Verify pattern 4 (0x00~0xffffffff) === */
	pattern32 = 0x00;
	for (i = 0; i < (len >> 2); i++)
		MEM32_BASE[i] = pattern32++;
	pattern32 = 0x00;
	for (i = 0; i < (len >> 2); i++) {
		if (MEM32_BASE[i] != pattern32++) {
			vfree(ptr);
			return -6;
		}
	}

	/* === Pattern 5: Filling memory range with 0x44332211 === */
	for (i = 0; i < size; i++)
		MEM32_BASE[i] = 0x44332211;

	/* === Read Check then Fill Memory with a5a5a5a5 Pattern === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0x44332211) {
			vfree(ptr);
			return -7;
		}
		MEM32_BASE[i] = 0xa5a5a5a5;
	}

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 0h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5a5a5a5) {
			vfree(ptr);
			return -8;
		}
		MEM8_BASE[i * 4] = 0x00;
	}

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 2h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5a5a500) {
			vfree(ptr);
			return -9;
		}
		MEM8_BASE[i * 4 + 2] = 0x00;
	}

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 1h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa500a500) {
			vfree(ptr);
			return -10;
		}
		MEM8_BASE[i * 4 + 1] = 0x00;
	}

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 3h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5000000) {
			vfree(ptr);
			return -11;
		}
		MEM8_BASE[i * 4 + 3] = 0x00;
	}

	/* === Read Check then Fill Memory with ffff Word Pattern at offset 1h == */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0x00000000) {
			vfree(ptr);
			return -12;
		}
		MEM16_BASE[i * 2 + 1] = 0xffff;
	}

	/* === Read Check then Fill Memory with ffff Word Pattern at offset 0h == */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffff0000) {
			vfree(ptr);
			return -13;
		}
		MEM16_BASE[i * 2] = 0xffff;
	}

	/*===  Read Check === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffffffff) {
			vfree(ptr);
			return -14;
		}
	}

	/************************************************
	* Additional verification
	************************************************/
	/* === stage 1 => write 0 === */

	for (i = 0; i < size; i++)
		MEM_BASE[i] = PATTERN1;

	/* === stage 2 => read 0, write 0xF === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];

		if (value != PATTERN1) {
			vfree(ptr);
			return -15;
		}
		MEM_BASE[i] = PATTERN2;
	}

	/* === stage 3 => read 0xF, write 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN2) {
			vfree(ptr);
			return -16;
		}
		MEM_BASE[i] = PATTERN1;
	}

	/* === stage 4 => read 0, write 0xF === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN1) {
			vfree(ptr);
			return -17;
		}
		MEM_BASE[i] = PATTERN2;
	}

	/* === stage 5 => read 0xF, write 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN2) {
			vfree(ptr);
			return -18;
		}
		MEM_BASE[i] = PATTERN1;
	}

	/* === stage 6 => read 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN1) {
			vfree(ptr);
			return -19;
		}
	}

	/* === 1/2/4-byte combination test === */
	mem_ptr = (unsigned long)MEM_BASE;
	while (mem_ptr < ((unsigned long)MEM_BASE + (size << 2))) {
		*((unsigned char *) mem_ptr) = 0x78;
		mem_ptr += 1;
		*((unsigned char *) mem_ptr) = 0x56;
		mem_ptr += 1;
		*((unsigned short *) mem_ptr) = 0x1234;
		mem_ptr += 2;
		*((unsigned int *) mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned short *) mem_ptr) = 0x5678;
		mem_ptr += 2;
		*((unsigned char *) mem_ptr) = 0x34;
		mem_ptr += 1;
		*((unsigned char *) mem_ptr) = 0x12;
		mem_ptr += 1;
		*((unsigned int *) mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned char *) mem_ptr) = 0x78;
		mem_ptr += 1;
		*((unsigned char *) mem_ptr) = 0x56;
		mem_ptr += 1;
		*((unsigned short *) mem_ptr) = 0x1234;
		mem_ptr += 2;
		*((unsigned int *) mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned short *) mem_ptr) = 0x5678;
		mem_ptr += 2;
		*((unsigned char *) mem_ptr) = 0x34;
		mem_ptr += 1;
		*((unsigned char *) mem_ptr) = 0x12;
		mem_ptr += 1;
		*((unsigned int *) mem_ptr) = 0x12345678;
		mem_ptr += 4;
	}

	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != 0x12345678) {
			vfree(ptr);
			return -20;
		}
	}

	/* === Verify pattern 1 (0x00~0xff) === */
	pattern8 = 0x00;
	MEM8_BASE[0] = pattern8;
	for (i = 0; i < size * 4; i++) {
		unsigned char waddr8, raddr8;

		waddr8 = i + 1;
		raddr8 = i;
		if (i < size * 4 - 1)
			MEM8_BASE[waddr8] = pattern8 + 1;
		if (MEM8_BASE[raddr8] != pattern8) {
			vfree(ptr);
			return -21;
		}
		pattern8++;
	}

	/* === Verify pattern 2 (0x00~0xffff) === */
	pattern16 = 0x00;
	MEM16_BASE[0] = pattern16;
	for (i = 0; i < size * 2; i++) {
		if (i < size * 2 - 1)
			MEM16_BASE[i + 1] = pattern16 + 1;
		if (MEM16_BASE[i] != pattern16) {
			vfree(ptr);
			return -22;
		}
		pattern16++;
	}
	/* === Verify pattern 3 (0x00~0xffffffff) === */
	pattern32 = 0x00;
	MEM32_BASE[0] = pattern32;
	for (i = 0; i < size; i++) {
		if (i < size - 1)
			MEM32_BASE[i + 1] = pattern32 + 1;
		if (MEM32_BASE[i] != pattern32) {
			vfree(ptr);
			return -23;
		}
		pattern32++;
	}
	pr_debug("complex R/W mem test pass\n");
	vfree(ptr);
	return 1;
}

unsigned int ucDram_Register_Read(unsigned int u4reg_addr)
{
	unsigned int pu4reg_value;

	if ((dt_init_done == 0) && (dram_dt_init() < 0)) {
		pr_warn("[error] get dram dt fail\n");
	return 0;
	}
	pu4reg_value = (readl(IOMEM(DRAMCAO_BASE_ADDR + u4reg_addr)) |
		readl(IOMEM(DDRPHY_BASE_ADDR + u4reg_addr)) |
		readl(IOMEM(DRAMCNAO_BASE_ADDR + u4reg_addr)));

	return pu4reg_value;
}

void ucDram_Register_Write(unsigned int u4reg_addr, unsigned int u4reg_value)
{
	if ((dt_init_done == 0) && (dram_dt_init() < 0)) {
		pr_warn("[error] get dram dt fail\n");
	return;
	}

	writel(u4reg_value, IOMEM(DRAMCAO_BASE_ADDR + u4reg_addr));
	writel(u4reg_value, IOMEM(DDRPHY_BASE_ADDR + u4reg_addr));
	writel(u4reg_value, IOMEM(DRAMCNAO_BASE_ADDR + u4reg_addr));
	/* dsb(); */
	mb();
}

/* ------------------------------------------------------------------------- */
/** Round_Operation
 *  Round operation of A/B
 *  @param  A
 *  @param  B
 *  @retval round(A/B)
 */
/* ------------------------------------------------------------------------- */
unsigned int Round_Operation(unsigned int A, unsigned int B)
{
	unsigned int temp;

	if (B == 0)
		return 0xffffffff;

	temp = A/B;

	if ((A-temp*B) >= ((temp+1)*B-A))
		return (temp+1);
	else
		return temp;
}

unsigned int get_dram_data_rate(void)
{
	unsigned int u4value1, u4value2, MPLL_POSDIV, MPLL_PCW, MPLL_FOUT;
	unsigned int MEMPLL_FBKDIV, MEMPLL_FOUT;

	u4value1 = (*(unsigned int *)(0xF0209280));
	u4value2 = (u4value1 & 0x00000070) >> 4;
	if (u4value2 == 0)
		MPLL_POSDIV = 1;
	else if (u4value2 == 1)
		MPLL_POSDIV = 2;
	else if (u4value2 == 2)
		MPLL_POSDIV = 4;
	else if (u4value2 == 3)
		MPLL_POSDIV = 8;
	else
		MPLL_POSDIV = 16;

	u4value1 = *(unsigned int *)(0xF0209284);
	MPLL_PCW = (u4value1 & 0x001fffff);

	MPLL_FOUT = 26/1*MPLL_PCW;
	MPLL_FOUT = Round_Operation(MPLL_FOUT, MPLL_POSDIV*28); /* freq*16384 */

	u4value1 = *(unsigned int *)(0xF000F614);
	MEMPLL_FBKDIV = (u4value1 & 0x007f0000) >> 16;

	MEMPLL_FOUT = MPLL_FOUT*1*4*(MEMPLL_FBKDIV+1);
	MEMPLL_FOUT = Round_Operation(MEMPLL_FOUT, 16384);

	/* pr_debug("MPLL_POSDIV=%d, MPLL_PCW=0x%x, MPLL_FOUT=%d, MEMPLL_FBKDIV=%d, MEMPLL_FOUT=%d\n",
	MPLL_POSDIV, MPLL_PCW, MPLL_FOUT, MEMPLL_FBKDIV, MEMPLL_FOUT); */

	return MEMPLL_FOUT;
}

/*
int get_ddr_type(void)
{
	unsigned int value;

	value =  ucDram_Register_Read(DRAMC_LPDDR2);

	if ((value>>28) & 0x1)
		return TYPE_LPDDR2;

	value = ucDram_Register_Read(DRAMC_PADCTL4);

	if ((value>>7) & 0x1)
		return TYPE_PCDDR3;

	value = ucDram_Register_Read(DRAMC_ACTIM1);

	if ((value>>28) & 0x1)
		return TYPE_LPDDR3;

	return TYPE_DDR1;
}
*/

static ssize_t complex_mem_test_show(struct device_driver *driver, char *buf)
{
	int ret;

	ret = Binning_DRAM_complex_mem_test();
	if (ret > 0)
		return snprintf(buf, PAGE_SIZE, "MEM Test all pass\n");
	else
		return snprintf(buf, PAGE_SIZE, "MEM TEST failed %d\n", ret);
}

static ssize_t complex_mem_test_store(struct device_driver *driver, const char *buf, size_t count)
{
	return count;
}


DRIVER_ATTR(emi_clk_mem_test, 0664, complex_mem_test_show, complex_mem_test_store);

static struct platform_driver dram_test_drv = {
	.driver = {
		.name = "emi_clk_test",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
	}
};

static int __init dram_dt_init(void)
{
	struct device_node *node;

	if (dt_init_done == 1)
		return 0;

	/* DTS version */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt8127-dramc");
	if (node) {
		DRAMCAO_BASE_ADDR = of_iomap(node, 0);
		pr_debug("get DRAMCAO_BASE_ADDR @ %p\n", DRAMCAO_BASE_ADDR);
	} else {
		pr_warn("can't find DRAMC0 compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt8127-ddrphy");
	if (node) {
		DDRPHY_BASE_ADDR = of_iomap(node, 0);
		pr_debug("get DDRPHY_BASE_ADDR @ %p\n", DDRPHY_BASE_ADDR);
	} else {
		pr_warn("can't find DDRPHY compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt8127-dramc_nao");
	if (node) {
		DRAMCNAO_BASE_ADDR = of_iomap(node, 0);
		pr_debug("get DRAMCNAO_BASE_ADDR @ %p\n", DRAMCNAO_BASE_ADDR);
	} else {
		pr_warn("can't find DRAMCNAO compatible node\n");
		return -1;
	}

	dt_init_done = 1;
	return 0;
}

/* extern char __ssram_text, _sram_start, __esram_text; */
int __init dram_test_init(void)
{
	int ret;

	pr_debug("dram test init\n");

	ret = dram_dt_init();
	if (ret) {
		pr_warn("fail to get dram dt\n");
		return ret;
	}

	ret = driver_register(&dram_test_drv.driver);
	if (ret) {
		pr_warn("fail to create the dram_test driver\n");
		return ret;
	}

	ret = driver_create_file(&dram_test_drv.driver, &driver_attr_emi_clk_mem_test);
	if (ret) {
		pr_warn("fail to create the emi_clk_mem_test sysfs files\n");
		return ret;
	}

	return 0;
}

arch_initcall(dram_test_init);
