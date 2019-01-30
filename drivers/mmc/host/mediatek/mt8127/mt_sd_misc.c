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

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/dma-mapping.h>

#include "mt_sd.h"
#include "sd_misc.h"
#include <queue.h>

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#ifdef CONFIG_MTK_EMMC_SUPPORT
#include "partition_define.h"
#endif

#define DRV_NAME_MISC            "misc-sd"

#define DEBUG_MMC_IOCTL   0
#define DEBUG_MSDC_SSC    1

static u32 *sg_msdc_multi_buffer;

static int simple_sd_open(struct inode *inode, struct file *file)
{
	return 0;
}

#ifdef CONFIG_MTK_EMMC_SUPPORT
void msdc_check_init_done(void)
{
#if 1
	struct msdc_host *host = NULL;

	host = msdc_get_host(MSDC_EMMC, 1, 0);
	BUG_ON(!host);
	if (!wait_for_completion_timeout(&host->card_init_done, 5 * HZ))
		BUG();
#endif
}

static int simple_sd_ioctl_single_rw(struct msdc_ioctl *msdc_ctl)
{
	char l_buf[512];
	struct scatterlist msdc_sg;
	struct mmc_data msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_request msdc_mrq;
	struct msdc_host *host_ctl;
	int ret;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];
	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

	mmc_claim_host(host_ctl->mmc);

#if DEBUG_MMC_IOCTL
	pr_debug("user want access %d partition\n", msdc_ctl->partition);
#endif

	ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
	if (ret) {
		pr_err("Faield to send ext_csd! %s, %d\n", __func__, __LINE__);
		mmc_release_host(host_ctl->mmc);
		return ret;
	}
	switch (msdc_ctl->partition) {
	case EMMC_PART_BOOT1:
		if (0x1 != (l_buf[179] & 0x7)) {
			/* change to access boot partition 1 */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x1;
			mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179], 1000);
		}
		break;
	case EMMC_PART_BOOT2:
		if (0x2 != (l_buf[179] & 0x7)) {
			/* change to access boot partition 2 */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x2;
			mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179], 1000);
		}
		break;
	default:
		/* make sure access partition is user data area */
		if (0 != (l_buf[179] & 0x7)) {
			/* set back to access user area */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x0;
			mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179], 1000);
		}
		break;
	}

	if (msdc_ctl->total_size > 512) {
		msdc_ctl->result = -1;
		mmc_release_host(host_ctl->mmc);
		return msdc_ctl->result;
	}
#if DEBUG_MMC_IOCTL
	pr_debug("start MSDC_SINGLE_READ_WRITE !!\n");
#endif
	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

	if (msdc_ctl->iswrite) {
		msdc_data.flags = MMC_DATA_WRITE;
		msdc_cmd.opcode = MMC_WRITE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
			if (copy_from_user(sg_msdc_multi_buffer, msdc_ctl->buffer, 512))
				mmc_release_host(host_ctl->mmc);
			return -EFAULT;
		} else {
			/* called from other kernel module */
			memcpy(sg_msdc_multi_buffer, msdc_ctl->buffer, 512);
		}
	} else {
		msdc_data.flags = MMC_DATA_READ;
		msdc_cmd.opcode = MMC_READ_SINGLE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;

		memset(sg_msdc_multi_buffer, 0, 512);
	}

	msdc_cmd.arg = msdc_ctl->address;

	if (!mmc_card_blockaddr(host_ctl->mmc->card)) {
		pr_debug("the device is used byte address!\n");
		msdc_cmd.arg <<= 9;
	}

	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_data.stop = NULL;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

#if DEBUG_MMC_IOCTL
	pr_debug("single block: ueser buf address is 0x%p!\n", msdc_ctl->buffer);
#endif
	sg_init_one(&msdc_sg, sg_msdc_multi_buffer, msdc_ctl->total_size);
	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);

	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

	if (!msdc_ctl->iswrite) {
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
			if (copy_to_user(msdc_ctl->buffer, sg_msdc_multi_buffer, 512))
				mmc_release_host(host_ctl->mmc);
			return -EFAULT;
		} else
			/* called from other kernel module */
			memcpy(msdc_ctl->buffer, sg_msdc_multi_buffer, 512);
	}

	if (msdc_ctl->partition) {
		ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
		if (ret) {
			pr_err("Faield to send ext_csd! %s, %d\n", __func__, __LINE__);
			mmc_release_host(host_ctl->mmc);
			return ret;
		}

		if (l_buf[179] & 0x7) {
			/* set back to access user area */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x0;
			mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179], 1000);
		}
	}

	mmc_release_host(host_ctl->mmc);

	if (msdc_cmd.error)
		msdc_ctl->result = msdc_cmd.error;

	if (msdc_data.error)
		msdc_ctl->result = msdc_data.error;
	else
		msdc_ctl->result = 0;

	return msdc_ctl->result;
}

static int simple_sd_ioctl_multi_rw(struct msdc_ioctl *msdc_ctl)
{
	char l_buf[512];
	struct scatterlist msdc_sg;
	struct mmc_data msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_command msdc_stop;
	struct mmc_request msdc_mrq;
	struct msdc_host *host_ctl;
	int ret;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];
	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

	mmc_claim_host(host_ctl->mmc);

#if DEBUG_MMC_IOCTL
	pr_debug("user want access %d partition\n", msdc_ctl->partition);
#endif
	ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
	if (ret) {
		pr_err("Faield to send ext_csd! %s, %d\n", __func__, __LINE__);
		mmc_release_host(host_ctl->mmc);
		return ret;
	}
	switch (msdc_ctl->partition) {
	case EMMC_PART_BOOT1:
		if (0x1 != (l_buf[179] & 0x7)) {
			/* change to access boot partition 1 */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x1;
			mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179], 1000);
		}
		break;
	case EMMC_PART_BOOT2:
		if (0x2 != (l_buf[179] & 0x7)) {
			/* change to access boot partition 2 */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x2;
			mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179], 1000);
		}
		break;
	default:
		/* make sure access partition is user data area */
		if (0 != (l_buf[179] & 0x7)) {
			/* set back to access user area */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x0;
			mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179], 1000);
		}
		break;
	}

	if (msdc_ctl->total_size > 64 * 1024) {
		msdc_ctl->result = -1;
		mmc_release_host(host_ctl->mmc);
		return msdc_ctl->result;
	}

	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));
	memset(&msdc_stop, 0, sizeof(struct mmc_command));

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;
	msdc_mrq.stop = &msdc_stop;

	if (msdc_ctl->iswrite) {
		msdc_data.flags = MMC_DATA_WRITE;
		msdc_cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
			if (copy_from_user
			    (sg_msdc_multi_buffer, msdc_ctl->buffer, msdc_ctl->total_size)) {
				mmc_release_host(host_ctl->mmc);
				return -EFAULT;
			}
		} else {
			/* called from other kernel module */
			memcpy(sg_msdc_multi_buffer, msdc_ctl->buffer, msdc_ctl->total_size);
		}
	} else {
		msdc_data.flags = MMC_DATA_READ;
		msdc_cmd.opcode = MMC_READ_MULTIPLE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;
		memset(sg_msdc_multi_buffer, 0, msdc_ctl->total_size);
	}

	msdc_cmd.arg = msdc_ctl->address;

	if (!mmc_card_blockaddr(host_ctl->mmc->card)) {
		pr_debug("this device use byte address!!\n");
		msdc_cmd.arg <<= 9;
	}
	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_stop.opcode = MMC_STOP_TRANSMISSION;
	msdc_stop.arg = 0;
	msdc_stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	msdc_data.stop = &msdc_stop;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

#if DEBUG_MMC_IOCTL
	pr_debug("total size is %d\n", msdc_ctl->total_size);
#endif
	sg_init_one(&msdc_sg, sg_msdc_multi_buffer, msdc_ctl->total_size);
	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);
	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

	if (!msdc_ctl->iswrite) {
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
			if (copy_to_user
			    (msdc_ctl->buffer, sg_msdc_multi_buffer, msdc_ctl->total_size)) {
				mmc_release_host(host_ctl->mmc);
				return -EFAULT;
			}
		} else {
			/* called from other kernel module */
			memcpy(msdc_ctl->buffer, sg_msdc_multi_buffer, msdc_ctl->total_size);
		}
	}

	if (msdc_ctl->partition) {
		ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
		if (ret) {
			pr_err("Faield to send ext_csd! %s, %d\n", __func__, __LINE__);
			mmc_release_host(host_ctl->mmc);
			return ret;
		}

		if (l_buf[179] & 0x7) {
			/* set back to access user area */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x0;
			mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179], 1000);
		}
	}

	mmc_release_host(host_ctl->mmc);

	if (msdc_cmd.error)
		msdc_ctl->result = msdc_cmd.error;

	if (msdc_data.error)
		msdc_ctl->result = msdc_data.error;
	else
		msdc_ctl->result = 0;

	return msdc_ctl->result;
}

int simple_sd_ioctl_rw(struct msdc_ioctl *msdc_ctl)
{
	int ret;
	int retry = 5;

	while (retry--) {
		if (msdc_ctl->total_size > 512)
			ret = simple_sd_ioctl_multi_rw(msdc_ctl);
		else
			ret = simple_sd_ioctl_single_rw(msdc_ctl);

		if (ret == 0)
			break;
	}

	return ret;
}

static int simple_sd_ioctl_get_cid(struct msdc_ioctl *msdc_ctl)
{
	struct msdc_host *host_ctl;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

#if DEBUG_MMC_IOCTL
	pr_debug("user want the cid in msdc slot%d\n", msdc_ctl->host_num);
#endif

	if (copy_to_user(msdc_ctl->buffer, &host_ctl->mmc->card->raw_cid, 16))
		return -EFAULT;

#if DEBUG_MMC_IOCTL
	pr_debug("cid:0x%x,0x%x,0x%x,0x%x\n", host_ctl->mmc->card->raw_cid[0],
	       host_ctl->mmc->card->raw_cid[1],
	       host_ctl->mmc->card->raw_cid[2], host_ctl->mmc->card->raw_cid[3]);
#endif
	return 0;

}

static int simple_sd_ioctl_get_csd(struct msdc_ioctl *msdc_ctl)
{
	struct msdc_host *host_ctl;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

#if DEBUG_MMC_IOCTL
	pr_debug("user want the csd in msdc slot%d\n", msdc_ctl->host_num);
#endif

	if (copy_to_user(msdc_ctl->buffer, &host_ctl->mmc->card->raw_csd, 16))
		return -EFAULT;

#if DEBUG_MMC_IOCTL
	pr_debug("csd:0x%x,0x%x,0x%x,0x%x\n", host_ctl->mmc->card->raw_csd[0],
	       host_ctl->mmc->card->raw_csd[1],
	       host_ctl->mmc->card->raw_csd[2], host_ctl->mmc->card->raw_csd[3]);
#endif
	return 0;

}

static int simple_sd_ioctl_get_excsd(struct msdc_ioctl *msdc_ctl)
{
	char l_buf[512];
	struct msdc_host *host_ctl;
	int ret;

#if DEBUG_MMC_IOCTL
	int i;
#endif

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

	mmc_claim_host(host_ctl->mmc);

#if DEBUG_MMC_IOCTL
	pr_debug("user want the extend csd in msdc slot%d\n", msdc_ctl->host_num);
#endif
	ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
	if (ret) {
		pr_err("Faield to send ext_csd! %s, %d\n", __func__, __LINE__);
		mmc_release_host(host_ctl->mmc);
		return ret;
	}

	if (copy_to_user(msdc_ctl->buffer, l_buf, 512)) {
		mmc_release_host(host_ctl->mmc);
		return -EFAULT;
	}

#if DEBUG_MMC_IOCTL
	for (i = 0; i < 512; i++) {
		pr_debug("%x", l_buf[i]);
		if (0 == ((i + 1) % 16))
			pr_debug("\n");
	}
#endif

	if (copy_to_user(msdc_ctl->buffer, l_buf, 512)) {
		mmc_release_host(host_ctl->mmc);
		return -EFAULT;
	}

	mmc_release_host(host_ctl->mmc);

	return 0;

}

static int sd_ioctl_get_excsd(struct msdc_ioctl *msdc_ctl)
{
	int ret;
	int retry = 5;

	while (retry--) {
		ret = simple_sd_ioctl_get_excsd(msdc_ctl);
		if (ret == 0)
			break;
	}

	return ret;
}
#endif

/*  to ensure format operate is clean the emmc device fully(partition erase) */
struct mbr_part_info {
	unsigned int start_sector;
	unsigned int nr_sects;
	unsigned int part_no;
	unsigned char *part_name;
};

#define MBR_PART_NUM  6
#define __MMC_ERASE_ARG        0x00000000
#define __MMC_TRIM_ARG         0x00000001

struct __mmc_blk_data {
	spinlock_t lock;
	struct gendisk *disk;
	struct mmc_queue queue;

	unsigned int usage;
	unsigned int read_only;
};

int msdc_get_info(enum STORAGE_TPYE storage_type, enum GET_STORAGE_INFO info_type, struct storage_info *info)
{
	struct msdc_host *host = NULL;
	int host_function = 0;
	bool boot = 0;

	switch (storage_type) {
	case EMMC_CARD_BOOT:
		host_function = MSDC_EMMC;
		boot = MSDC_BOOT_EN;
		break;
	case EMMC_CARD:
		host_function = MSDC_EMMC;
		break;
	case SD_CARD_BOOT:
		host_function = MSDC_SD;
		boot = MSDC_BOOT_EN;
		break;
	case SD_CARD:
		host_function = MSDC_SD;
		break;
	default:
		pr_err("No supported storage type!");
		return 0;
	}
	host = msdc_get_host(host_function, boot, 0);
	switch (info_type) {
	case CARD_INFO:
		if (host->mmc && host->mmc->card)
			info->card = host->mmc->card;
		else {
			pr_err("CARD was not ready<get card>!");
			return 0;
		}
		break;
	case DISK_INFO:
		if (host->mmc && host->mmc->card)
			info->disk = mmc_get_disk(host->mmc->card);
		else {
			pr_err("CARD was not ready<get disk>!");
			return 0;
		}
		break;
	case EMMC_USER_CAPACITY:
		info->emmc_user_capacity = msdc_get_capacity();
		break;
	case EMMC_RESERVE:
#ifdef CONFIG_MTK_EMMC_SUPPORT
		info->emmc_reserve = msdc_get_reserve();
#endif
		break;
	default:
		pr_err("Please check INFO_TYPE");
		return 0;
	}
	return 1;
}

#ifdef CONFIG_MTK_EMMC_SUPPORT
static int simple_mmc_get_disk_info(struct mbr_part_info *mpi, unsigned char *name)
{
	int i = 0;
	char *no_partition_name = "n/a";
	struct disk_part_iter piter;
	struct hd_struct *part;
	struct msdc_host *host;
	struct gendisk *disk;
	struct __mmc_blk_data *md;

	/* emmc always in slot0 */
	host = msdc_get_host(MSDC_EMMC, MSDC_BOOT_EN, 0);
	BUG_ON(!host);
	BUG_ON(!host->mmc);
	BUG_ON(!host->mmc->card);

	md = mmc_get_drvdata(host->mmc->card);
	BUG_ON(!md);
	BUG_ON(!md->disk);

	disk = md->disk;

	/* use this way to find partition info is to avoid handle addr transfer in scatter file
	 * and 64bit address calculate */
	disk_part_iter_init(&piter, disk, 0);
	while ((part = disk_part_iter_next(&piter))) {
		for (i = 0; i < PART_NUM; i++) {
			if (PartInfo[i].partition_idx != 0
			    && PartInfo[i].partition_idx == part->partno) {
#if DEBUG_MMC_IOCTL
				pr_debug("part_name = %s    name = %s\n", PartInfo[i].name, name);
#endif
				if (!strncmp(PartInfo[i].name, name, 25)) {
					mpi->start_sector = part->start_sect;
					mpi->nr_sects = part->nr_sects;
					mpi->part_no = part->partno;
					if (i < PART_NUM)
						mpi->part_name = PartInfo[i].name;
					else
						mpi->part_name = no_partition_name;

					disk_part_iter_exit(&piter);
					return 0;
				}

				break;
			}
		}
	}
	disk_part_iter_exit(&piter);

	return 1;
}

/* call mmc block layer interface for userspace to do erase operate */
static int simple_mmc_erase_func(unsigned int start, unsigned int size)
{
	struct msdc_host *host;

	/* emmc always in slot0 */
	host = msdc_get_host(MSDC_EMMC, MSDC_BOOT_EN, 0);
	BUG_ON(!host);
	BUG_ON(!host->mmc);
	BUG_ON(!host->mmc->card);

	mmc_claim_host(host->mmc);

	if (!mmc_can_trim(host->mmc->card)) {
		pr_debug("emmc card can't support trim\n");
		mmc_release_host(host->mmc);
		return 0;
	}

	mmc_erase(host->mmc->card, start, size, __MMC_TRIM_ARG);

#if DEBUG_MMC_IOCTL
	pr_debug("erase done....\n");
#endif

	mmc_release_host(host->mmc);

	return 0;
}
#endif

#ifdef CONFIG_MTK_EMMC_SUPPORT
static int simple_mmc_erase_partition(unsigned char *name)
{
	struct mbr_part_info mbr_part;
	int l_ret = -1;

	BUG_ON(!name);
	/* just support erase cache & data partition now */
	if (('u' == *name && 's' == *(name + 1) && 'r' == *(name + 2) && 'd' == *(name + 3) &&
	     'a' == *(name + 4) && 't' == *(name + 5) && 'a' == *(name + 6)) ||
	    ('c' == *name && 'a' == *(name + 1) && 'c' == *(name + 2) && 'h' == *(name + 3)
	     && 'e' == *(name + 4))) {
		/* find erase start address and erase size, just support high capacity emmc card now */
		l_ret = simple_mmc_get_disk_info(&mbr_part, name);


		if (l_ret == 0) {
			/* do erase */
			pr_debug("erase %s start sector: 0x%x size: 0x%x\n", mbr_part.part_name,
			       mbr_part.start_sector, mbr_part.nr_sects);
			simple_mmc_erase_func(mbr_part.start_sector, mbr_part.nr_sects);
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_MTK_EMMC_SUPPORT
static int simple_mmc_erase_partition_wrap(struct msdc_ioctl *msdc_ctl)
{
	unsigned char name[25];

	if (copy_from_user(name, (unsigned char *)msdc_ctl->buffer, msdc_ctl->total_size))
		return -EFAULT;

	return simple_mmc_erase_partition(name);
}
#endif
static long simple_sd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct msdc_ioctl *msdc_ctl = (struct msdc_ioctl *)arg;
	int ret;

	if (msdc_ctl == NULL) {
		switch (cmd) {
		default:
			pr_debug("mt_sd_ioctl:this opcode value is illegal!!\n");
			return -EINVAL;
		}
	} else {
		switch (msdc_ctl->opcode) {
#ifdef CONFIG_MTK_EMMC_SUPPORT
		case MSDC_SINGLE_READ_WRITE:
			msdc_ctl->result = simple_sd_ioctl_rw(msdc_ctl);
			break;
		case MSDC_MULTIPLE_READ_WRITE:
			msdc_ctl->result = simple_sd_ioctl_rw(msdc_ctl);
			break;
		case MSDC_GET_CID:
			msdc_ctl->result = simple_sd_ioctl_get_cid(msdc_ctl);
			break;
		case MSDC_GET_CSD:
			msdc_ctl->result = simple_sd_ioctl_get_csd(msdc_ctl);
			break;
		case MSDC_GET_EXCSD:
			msdc_ctl->result = sd_ioctl_get_excsd(msdc_ctl);
			break;
		case MSDC_ERASE_PARTITION:
			msdc_ctl->result = simple_mmc_erase_partition_wrap(msdc_ctl);
			break;
#endif
		default:
			pr_debug("simple_sd_ioctl:this opcode value is illegal!!\n");
			return -EINVAL;
		}

		return msdc_ctl->result;
	}

	return ret;
}

static const struct file_operations simple_msdc_em_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = simple_sd_ioctl,
	.open = simple_sd_open,
};

static struct miscdevice simple_msdc_em_dev[] = {
	{
	 .minor = MISC_DYNAMIC_MINOR,
	 .name = "misc-sd",
	 .fops = &simple_msdc_em_fops,
	 }
};

static int simple_sd_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_debug("in misc_sd_probe function\n");

	return ret;
}

static int simple_sd_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver simple_sd_driver = {
	.probe = simple_sd_probe,
	.remove = simple_sd_remove,

	.driver = {
		   .name = DRV_NAME_MISC,
		   .owner = THIS_MODULE,
		   },
};

static int __init simple_sd_init(void)
{
	int ret;

	sg_msdc_multi_buffer = kmalloc(64 * 1024, GFP_KERNEL);
	if (sg_msdc_multi_buffer == NULL)
		BUG_ON("allock 64KB memory failed\n");

	ret = platform_driver_register(&simple_sd_driver);
	if (ret) {
		pr_err(DRV_NAME_MISC ": Can't register driver\n");
		return ret;
	}
	pr_info(DRV_NAME_MISC ": MediaTek simple SD/MMC Card Driver\n");

	/* msdc0 is for emmc only, just for emmc */
	/* ret = misc_register(&simple_msdc_em_dev[host->id]); */
	ret = misc_register(&simple_msdc_em_dev[0]);
	if (ret) {
		pr_debug("register MSDC Slot[0] misc driver failed (%d)\n", ret);
		return ret;
	}

	return 0;
}

static void __exit simple_sd_exit(void)
{
	if (sg_msdc_multi_buffer != NULL) {
		kfree(sg_msdc_multi_buffer);
		sg_msdc_multi_buffer = NULL;
	}

	misc_deregister(&simple_msdc_em_dev[0]);

	platform_driver_unregister(&simple_sd_driver);
}

module_init(simple_sd_init);
module_exit(simple_sd_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("simple MediaTek SD/MMC Card Driver");
MODULE_AUTHOR("feifei.wang <feifei.wang@mediatek.com>");
