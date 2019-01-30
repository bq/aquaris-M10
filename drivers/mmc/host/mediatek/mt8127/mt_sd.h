/*
 * Copyright (c) 2014-2015 MediaTek Inc.
 * Author: Chaotian.Jing <chaotian.jing@mediatek.com>
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

#ifndef MT8127_SD_H
#define MT8127_SD_H

#define PORTING_FOR_MT8127_TURNKEY
#define MSDC_EMMC       (0)
#define MSDC_SD         (1)
#define MSDC_SDIO       (2)

#define MSDC_BOOT_EN        (1)

struct msdc_dma {
	struct scatterlist *sg;	/* I/O scatter list */
	struct mt_gpdma_desc *gpd;	/* pointer to gpd array */
	struct mt_bdma_desc *bd;	/* pointer to bd array */
	dma_addr_t gpd_addr;	/* the physical address of gpd array */
	dma_addr_t bd_addr;	/* the physical address of bd array */
};

struct msdc_save_para {
	u32 msdc_cfg;
	u32 iocon;
	u32 sdc_cfg;
	u32 pad_tune;
	u32 patch_bit0;
	u32 patch_bit1;
	u32 pad_ds_tune;
	u32 emmc50_cfg0;
};

struct mt81xx_mmc_compatible {
	u8 clk_div_bits;
};

struct msdc_host {
	struct device *dev;
	struct mmc_host *mmc;	/* mmc structure */
	int cmd_rsp;

	spinlock_t lock;
	struct mmc_request *mrq;
	struct mmc_command *cmd;
	struct mmc_data *data;
	int error;

	void __iomem *base;	/* host base address */

	struct msdc_dma dma;	/* dma channel */
	u64 dma_mask;

	u32 timeout_ns;		/* data timeout ns */
	u32 timeout_clks;	/* data timeout clks */

	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_uhs;
	struct delayed_work req_timeout;
	int irq;		/* host interrupt */

	struct clk *src_clk;	/* msdc source clock */
	struct clk *h_clk;	/* msdc h_clk */
	u32 mclk;		/* mmc subsystem clock frequency */
	u32 src_clk_freq;	/* source clock frequency */
	u32 sclk;		/* SD/MS bus clock frequency */
	unsigned char timing;
	bool vqmmc_enabled;
	u32 hs400_ds_delay;
	u32 hs200_cmd_int_delay; /* cmd internal delay for HS200/SDR104 */
	u32 hs200_write_int_delay; /* write internal delay for HS200/SR104 */
	struct msdc_save_para save_para;	/* used when gate HCLK */
	const struct mt81xx_mmc_compatible *dev_comp;
#ifdef PORTING_FOR_MT8127_TURNKEY
	u32 host_function;
	u32 host_id;
	u32 boot;
	struct completion card_init_done;
#endif
};


#endif				/* end of MT8127_SD_H */
