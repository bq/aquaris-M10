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


#ifndef __DRAMC_H__
#define __DRAMC_H__

/* If defined, need to set MR2 in dramcinit. */
#define DUAL_FREQ_DIFF_RLWL
/* #define DDR_1866 */
#define TB_DRAM_SPEED

#define DMA_BASE          CQDMA_BASE_ADDR
#define DMA_SRC           IOMEM((DMA_BASE + 0x001C))
#define DMA_DST           IOMEM((DMA_BASE + 0x0020))
#define DMA_LEN1          IOMEM((DMA_BASE + 0x0024))
#define DMA_GSEC_EN       IOMEM((DMA_BASE + 0x0058))
#define DMA_INT_EN        IOMEM((DMA_BASE + 0x0004))
#define DMA_CON           IOMEM((DMA_BASE + 0x0018))
#define DMA_START         IOMEM((DMA_BASE + 0x0008))
#define DMA_INT_FLAG      IOMEM((DMA_BASE + 0x0000))

#define DMA_GDMA_LEN_MAX_MASK   (0x000FFFFF)
#define DMA_GSEC_EN_BIT         (0x00000001)
#define DMA_INT_EN_BIT          (0x00000001)
#define DMA_INT_FLAG_CLR_BIT (0x00000000)

#define CHA_DRAMCAO_BASE	0xF0004000
#define CHA_DDRPHY_BASE		0xF000F000
#define CLK_CFG_0_CLR		(0xF0000048)
#define CLK_CFG_0_SET		(0xF0000044)
#define CLK_CFG_UPDATE		(0xF0000004)
#define PCM_INI_PWRON0_REG	(0xF0006010)

#define DRAMC_REG_MRS 0x088
#define DRAMC_REG_PADCTL4 0x0e4
#define DRAMC_REG_LPDDR2_3 0x1e0
#define DRAMC_REG_SPCMD 0x1e4
#define DRAMC_REG_ACTIM1 0x1e8
#define DRAMC_REG_RRRATE_CTL 0x1f4
#define DRAMC_REG_MRR_CTL 0x1fc
#define DRAMC_REG_SPCMDRESP 0x3b8

#define PATTERN1 0x5A5A5A5A
#define PATTERN2 0xA5A5A5A5
#define MEM_TEST_SIZE 0x2000

/*Config*/

#define PHASE_NUMBER 3
#define DRAM_BASE (0x40000000ULL)
#define BUFF_LEN 0x100
#define IOREMAP_ALIGMENT 0x1000
/* We use GPT to measurement how many clk pass in 100us */
#define Delay_magic_num 0x295

extern unsigned int DMA_TIMES_RECORDER;
extern phys_addr_t get_max_DRAM_size(void);

enum {
	TYPE_DDR1 = 1,
	TYPE_LPDDR2,
	TYPE_LPDDR3,
	TYPE_PCDDR3,
} DRAM_TYPE;

/************************** Common Macro *********************/
#define delay_a_while(count)			\
do {						\
	register unsigned int delay = 0;	\
	asm volatile ("mov %0, %1\n\t"		\
		"1:\n\t"			\
		"subs %0, %0, #1\n\t"		\
		"bne 1b\n\t"			\
		: "+r" (delay)			\
		: "r" (count)			\
		: "cc");			\
} while (0)

#define mcDELAY_US(x)           delay_a_while((U32) (x*1000*10))

#endif   /*__WDT_HW_H__*/
