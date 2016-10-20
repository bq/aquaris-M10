/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 * Author: Isaac.Lee <isaac.lee@mediatek.com>
 *	  Bean.Li <bean.li@mediatek.com>
 *	  Moon.Li <moon.li@mediatek.com>
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

/*
 * Read-Write block devices on top of UBI volumes
 *
 * A robust implementation to allow a block device to be layered on top of a
 * UBI volume. The implementation is provided by creating a dynamic page
 * mapping between the block device and the UBI volume.
 *
 * The addressed byte is obtained from the addressed block sector, which is
 * mapped table into the corresponding LEB & PAGE:
 *
 * This feature is compiled in the UBI core, and adds a 'block' parameter
 * to allow early creation of block devices on top of UBI volumes. Runtime
 * block creation/removal for UBI volumes is provided through two UBI ioctls:
 * UBI_IOCVOLCRBLK and UBI_IOCVOLRMBLK.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mtd/ubi.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <asm/div64.h>

#include "ubi/ubi-media.h"
#include "ubi/ubi.h"
#include "mt_ftl.h"

/* Maximum number of supported devices */
#define MT_FTL_BLK_MAX_DEVICES 32

/* Maximum length of the 'block=' parameter */
#define MT_FTL_BLK_PARAM_LEN 63

/* Maximum number of comma-separated items in the 'block=' parameter */
#define MT_FTL_BLK_PARAM_COUNT 2

struct mt_ftl_blk_param {
	int ubi_num;
	int vol_id;
	char name[MT_FTL_BLK_PARAM_LEN+1];
};

/* Numbers of elements set in the @mt_ftl_blk_param array */
static int mt_ftl_blk_devs __initdata;

/* MTD devices specification parameters */
static struct mt_ftl_blk_param mt_ftl_blk_param[MT_FTL_BLK_MAX_DEVICES] __initdata;

/* Linked list of all mt_ftl_blk instances */
static LIST_HEAD(mt_ftl_blk_devices);
static DEFINE_MUTEX(devices_mutex);
static int mt_ftl_blk_major;

static int __init mt_ftl_blk_set_param(const char *val,
				     const struct kernel_param *kp)
{
	int i, ret;
	size_t len;
	struct mt_ftl_blk_param *param;
	char buf[MT_FTL_BLK_PARAM_LEN];
	char *pbuf = &buf[0];
	char *tokens[MT_FTL_BLK_PARAM_COUNT];

	if (!val)
		return -EINVAL;

	len = strnlen(val, MT_FTL_BLK_PARAM_LEN);
	if (len == 0) {
		ubi_warn("block: empty 'block=' parameter - ignored\n");
		return 0;
	}

	if (len == MT_FTL_BLK_PARAM_LEN) {
		ubi_err("block: parameter \"%s\" is too long, max. is %d\n", val, MT_FTL_BLK_PARAM_LEN);
		return -EINVAL;
	}

	strcpy(buf, val);

	/* Get rid of the final newline */
	if (buf[len - 1] == '\n')
		buf[len - 1] = '\0';

	for (i = 0; i < MT_FTL_BLK_PARAM_COUNT; i++)
		tokens[i] = strsep(&pbuf, ",");

	param = &mt_ftl_blk_param[mt_ftl_blk_devs];
	if (tokens[1]) {
		/* Two parameters: can be 'ubi, vol_id' or 'ubi, vol_name' */
		ret = kstrtoint(tokens[0], 10, &param->ubi_num);
		if (ret < 0)
			return -EINVAL;

		/* Second param can be a number or a name */
		ret = kstrtoint(tokens[1], 10, &param->vol_id);
		if (ret < 0) {
			param->vol_id = -1;
			strcpy(param->name, tokens[1]);
		}

	} else {
		/* One parameter: must be device path */
		strcpy(param->name, tokens[0]);
		param->ubi_num = -1;
		param->vol_id = -1;
	}

	mt_ftl_blk_devs++;

	return 0;
}

static struct kernel_param_ops mt_ftl_blk_param_ops = {
	.set    = mt_ftl_blk_set_param,
};
module_param_cb(block, &mt_ftl_blk_param_ops, NULL, 0);
MODULE_PARM_DESC(block, "Attach block devices to UBI volumes. Parameter format: block=<path|dev,num|dev,name>.\n"
			"Multiple \"block\" parameters may be specified.\n"
			"UBI volumes may be specified by their number, name, or path to the device node.\n"
			"Examples\n"
			"Using the UBI volume path:\n"
			"ubi.block=/dev/ubi0_0\n"
			"Using the UBI device, and the volume name:\n"
			"ubi.block=0,rootfs\n"
			"Using both UBI device number and UBI volume number:\n"
			"ubi.block=0,0\n");

static struct mt_ftl_blk *find_dev_nolock(int ubi_num, int vol_id)
{
	struct mt_ftl_blk *dev;

	list_for_each_entry(dev, &mt_ftl_blk_devices, list)
		if (dev->ubi_num == ubi_num && dev->vol_id == vol_id)
			return dev;
	return NULL;
}

static int mt_ftl_blk_flush(struct mt_ftl_blk *dev, bool sync)
{
	int ret = 0;

#ifdef MT_FTL_SUPPORT_WBUF
	mt_ftl_wbuf_forced_flush(dev);
#endif

	/* Sync ubi device when device is released and on block flush ioctl */
	if (sync)
		ret = ubi_sync(dev->ubi_num);

	return ret;
}

#ifdef MT_FTL_SUPPORT_MQ
static int mt_ftl_blk_mq_request(struct mt_ftl_blk *dev, struct request *req)
{
	int ret = MT_FTL_SUCCESS;
	sector_t sec;
	unsigned nsects, len;
	unsigned long flags;
	struct bio_vec bvec;
	struct req_iterator iter;
	char *data = NULL;

	if (req->cmd_type != REQ_TYPE_FS) {
		mt_ftl_err(dev, "cmd_type != REQ_TYPE_FS %d", req->cmd_type);
		blk_dump_rq_flags(req, "[Bean]");
		return -EIO;
	}

	sec = blk_rq_pos(req);
	len = blk_rq_bytes(req);
	nsects = blk_rq_sectors(req);

	if (req->cmd_flags & REQ_FLUSH)
		return mt_ftl_flush(dev);

	if (sec + nsects > get_capacity(req->rq_disk)) {
		mt_ftl_debug(dev, "0x%lx 0x%x", sec, len);
		if (req->cmd_flags & REQ_DISCARD) {
			nsects = get_capacity(req->rq_disk);
			return mt_ftl_discard(dev, sec, nsects);
		}
		mt_ftl_err(dev, "request(0x%lx) exceed the capacity(0x%lx)", sec + nsects, get_capacity(req->rq_disk));
		return -EIO;
	}

	if (req->cmd_flags & REQ_DISCARD)
		return mt_ftl_discard(dev, sec, nsects);

	if ((req->bio->bi_rw & WRITE_SYNC) == WRITE_SYNC)
		dev->sync = 1;

	switch (rq_data_dir(req)) {
	case READ:
		rq_for_each_segment(bvec, req, iter) {
			data = bvec_kmap_irq(&bvec, &flags);
			ret = mt_ftl_read(dev, data, iter.iter.bi_sector, bvec.bv_len);
		}
		rq_flush_dcache_pages(req);
		break;
	case WRITE:
		rq_for_each_segment(bvec, req, iter) {
			data = bvec_kmap_irq(&bvec, &flags);
			ret = mt_ftl_write(dev, data, iter.iter.bi_sector, bvec.bv_len);
		}
		break;
	default:
		mt_ftl_err(dev, "unknown request\n");
		return -EIO;
	}
	return ret;
}

#ifdef MT_FTL_SUPPORT_THREAD
int mt_ftl_blk_thread(void *info)
{
	int err;
	unsigned long flags;
	struct mt_ftl_blk *dev = info;
	struct mt_ftl_blk_rq *rq = NULL;
	struct request *req = NULL;

	mt_ftl_err(dev, "%s thread started, PID %d", dev->blk_name, current->pid);
	while (1) {
		if (kthread_should_stop())
			break;

		set_current_state(TASK_INTERRUPTIBLE);
		if (list_empty_careful(&dev->ftl_rq_list)) {
			if (kthread_should_stop()) {
				__set_current_state(TASK_RUNNING);
				break;
			}
			schedule();
			continue;
		} else
			__set_current_state(TASK_RUNNING);

		rq = list_first_entry(&dev->ftl_rq_list, struct mt_ftl_blk_rq, list);
		spin_lock_irqsave(&dev->ftl_rq_lock, flags);
		list_del_init(&rq->list);
		spin_unlock_irqrestore(&dev->ftl_rq_lock, flags);
		req = rq->req;
		err = mt_ftl_blk_mq_request(dev, req);
		if (err) {
			mt_ftl_err(dev, "[Bean]request error %d\n", err);
			dump_stack();
			break;
		}
		req->errors = err;
		blk_mq_end_request(req, req->errors);
		cond_resched();
	}
	if (req)
		blk_mq_end_request(req, -EIO);
	mt_ftl_err(dev, "%s thread stops, err = %d", dev->blk_name, err);
	return 0;
}

static int mt_ftl_queue_rq(struct blk_mq_hw_ctx *hctx, struct request *req, bool last)
{
	unsigned long flags;
	struct mt_ftl_blk *dev = hctx->queue->queuedata;
	struct mt_ftl_blk_rq *ftl_rq = blk_mq_rq_to_pdu(req);

	blk_mq_start_request(req);
	ftl_rq->req = req;
	spin_lock_irqsave(&dev->ftl_rq_lock, flags);
	list_add_tail(&ftl_rq->list, &dev->ftl_rq_list);
	spin_unlock_irqrestore(&dev->ftl_rq_lock, flags);

	if (dev->blk_bgt)
		wake_up_process(dev->blk_bgt);

	return BLK_MQ_RQ_QUEUE_OK;
}

static struct blk_mq_ops mt_ftl_mq_ops = {
	.queue_rq	= mt_ftl_queue_rq,
	.map_queue	= blk_mq_map_queue,
};

static int mt_ftl_blk_mq_init(struct mt_ftl_blk *dev)
{
	int ret;

	spin_lock_init(&dev->ftl_rq_lock);
	INIT_LIST_HEAD(&dev->ftl_rq_list);

	sprintf(dev->blk_name, "mt_ftl%d/%d", dev->ubi_num, dev->vol_id);
	dev->blk_bgt = kthread_create(mt_ftl_blk_thread, dev, "%s", dev->blk_name);
	if (IS_ERR(dev->blk_bgt)) {
		ret = PTR_ERR(dev->blk_bgt);
		dev->blk_bgt = NULL;
		mt_ftl_err(dev, "cannot spawn \"%s\", error %d", dev->blk_name, ret);
		return ret;
	}
	wake_up_process(dev->blk_bgt);
	return MT_FTL_SUCCESS;
}
#else
static void mt_ftl_do_mq_work(struct work_struct *work)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_blk_pdu *blk_pdu = container_of(work, struct mt_ftl_blk_pdu, work);
	struct request *req = blk_mq_rq_from_pdu(blk_pdu);

	blk_mq_start_request(req);

	/* mutex_lock(&blk_pdu->dev->dev_mutex); */
	ret = mt_ftl_blk_mq_request(blk_pdu->dev, req);
	if (ret) {
		mt_ftl_err(blk_pdu->dev, "[Bean]request error %d\n", ret);
		dump_stack();
	}
	/* mutex_unlock(&blk_pdu->dev->dev_mutex); */
	req->errors = ret;
	blk_mq_end_request(req, ret);
}

static int mt_ftl_queue_rq(struct blk_mq_hw_ctx *hctx, struct request *req, bool last)
{
	struct mt_ftl_blk *dev = hctx->queue->queuedata;
	struct mt_ftl_blk_pdu *blk_pdu = blk_mq_rq_to_pdu(req);

	queue_work(dev->wq, &blk_pdu->work);
	return BLK_MQ_RQ_QUEUE_OK;
}

static int mt_ftl_init_request(void *data, struct request *req,
				 unsigned int hctx_idx,
				 unsigned int request_idx,
				 unsigned int numa_node)
{
	struct mt_ftl_blk_pdu *blk_pdu = blk_mq_rq_to_pdu(req);

	blk_pdu->dev = data;

	INIT_WORK(&blk_pdu->work, mt_ftl_do_mq_work);

	return 0;
}

static struct blk_mq_ops mt_ftl_mq_ops = {
	.queue_rq	= mt_ftl_queue_rq,
	.map_queue	= blk_mq_map_queue,
	.init_request	= mt_ftl_init_request,
};

#define mt_ftl_blk_mq_init(dev) (MT_FTL_SUCCESS)
#endif
#else
#define mt_ftl_blk_mq_init(dev) (MT_FTL_SUCCESS)

static int do_mt_ftl_blk_request(struct mt_ftl_blk *dev, struct request *req)
{
	int ret = MT_FTL_SUCCESS;
	sector_t sec;
	unsigned nsects, len;

	if (req->cmd_type != REQ_TYPE_FS) {
		mt_ftl_err(dev, "cmd_type != REQ_TYPE_FS %d", req->cmd_type);
		return -EIO;
	}

	sec = blk_rq_pos(req);
	len = blk_rq_cur_bytes(req);
	nsects = blk_rq_cur_sectors(req);

	if (req->cmd_flags & REQ_FLUSH)
		return mt_ftl_flush(dev);

	if (sec + nsects > get_capacity(req->rq_disk)) {
		mt_ftl_debug(dev, "0x%x 0x%x", (u32)sec, (u32)len);
		if (req->cmd_flags & REQ_DISCARD) {
			nsects = get_capacity(req->rq_disk);
			return mt_ftl_discard(dev, sec, nsects);
		}
		mt_ftl_err(dev, "request(0x%x) exceed the capacity(0x%x)",
			(u32)(sec + nsects), (u32)get_capacity(req->rq_disk));
		return -EIO;
	}

	if (req->cmd_flags & REQ_DISCARD)
		return mt_ftl_discard(dev, sec, nsects);

	if ((req->bio->bi_rw & WRITE_SYNC) == WRITE_SYNC)
		dev->sync += 1;
	/* else if (dev->sync > 0) { */
		/* Notice: improve write performance, but power cut at last sync will lost data and occur error */
		/* dev->sync = 0;
		if (dev->param->u4DataNum != 0 || dev->param->u4NextPageOffsetIndicator != 0) {
			ret = mt_ftl_write_page(dev);
			if (ret)
				return ret;
		}
	} */

	switch (rq_data_dir(req)) {
	case READ:
		ret = mt_ftl_read(dev, bio_data(req->bio), sec, len);
		break;
	case WRITE:
		ret = mt_ftl_write(dev, bio_data(req->bio), sec, len);
		break;
	default:
		mt_ftl_err(dev, "unknown request\n");
		return -EIO;
	}

	return ret;
}

static void mt_ftl_blk_do_work(struct work_struct *work)
{
	struct mt_ftl_blk *dev =
		container_of(work, struct mt_ftl_blk, work);
	struct request_queue *rq = dev->rq;
	struct request *req = NULL;

	spin_lock_irq(rq->queue_lock);
	req = blk_fetch_request(rq);
	while (req) {
		int res;

		spin_unlock_irq(rq->queue_lock);

		cond_resched();
		res = do_mt_ftl_blk_request(dev, req);
		if (res < 0) {
			mt_ftl_err(dev, "[Bean]request error %d\n", res);
			dump_stack();
			break;
		}

		spin_lock_irq(rq->queue_lock);

		if (!__blk_end_request_cur(req, res))
			req = blk_fetch_request(rq);
	}

	if (req)
		__blk_end_request_all(req, -EIO);

	spin_unlock_irq(rq->queue_lock);
}

static void mt_ftl_blk_request(struct request_queue *rq)
{
	struct mt_ftl_blk *dev;
	struct request *req;

	dev = rq->queuedata;

	if (!dev)
		while ((req = blk_fetch_request(rq)) != NULL)
			__blk_end_request_all(req, -ENODEV);
	else
		queue_work(dev->wq, &dev->work);
}
#endif

static int mt_ftl_blk_open(struct block_device *bdev, fmode_t mode)
{
	struct mt_ftl_blk *dev = bdev->bd_disk->private_data;
	int ret;

	mutex_lock(&dev->dev_mutex);
	if (dev->refcnt > 0) {
		/*
		 * The volume is already open, just increase the reference
		 * counter.
		 */
		goto out_done;
	}

	/*
	 * We want users to be aware they should only mount us as read-only.
	 * It's just a paranoid check, as write requests will get rejected
	 * in any case.
	 */
	if (mode & FMODE_WRITE)
		dev->desc = ubi_open_volume(dev->ubi_num, dev->vol_id, UBI_READWRITE);
	else
		dev->desc = ubi_open_volume(dev->ubi_num, dev->vol_id, UBI_READONLY);

	if (IS_ERR(dev->desc)) {
		mt_ftl_err(dev, "%s failed to open ubi volume %d_%d",
			dev->gd->disk_name, dev->ubi_num, dev->vol_id);
		ret = PTR_ERR(dev->desc);
		dev->desc = NULL;
		goto out_unlock;
	}
out_done:
	dev->refcnt++;
	mutex_unlock(&dev->dev_mutex);
	return 0;

out_unlock:
	mutex_unlock(&dev->dev_mutex);
	return ret;
}

static void mt_ftl_blk_release(struct gendisk *gd, fmode_t mode)
{
	struct mt_ftl_blk *dev = gd->private_data;

	mutex_lock(&dev->dev_mutex);
	dev->refcnt--;
	if (dev->refcnt == 0) {
		mt_ftl_blk_flush(dev, true);
		ubi_close_volume(dev->desc);
		dev->desc = NULL;
	}
	mutex_unlock(&dev->dev_mutex);
}

static int mt_ftl_blk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	/* Some tools might require this information */
	geo->heads = 1;
	geo->cylinders = 1;
	geo->sectors = get_capacity(bdev->bd_disk);
	geo->start = 0;
	return 0;
}

static int mt_ftl_blk_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	struct mt_ftl_blk *dev = bdev->bd_disk->private_data;
	int ret = -ENXIO, data;

	if (!dev)
		return ret;

	mutex_lock(&dev->dev_mutex);
	switch (cmd) {
	case BLKROSET:
		/*need to do only for adb remount*/
		if (copy_from_user(&data, (void __user *)arg, sizeof(int))) {
			ret = -EFAULT;
			break;
		}
		if (data == 0) {
			mt_ftl_debug(dev, "[Bean]set rw %d %d", dev->ubi_num, dev->vol_id);
			set_disk_ro(dev->gd, 0);
			ubi_close_volume(dev->desc);
			dev->desc = ubi_open_volume(dev->ubi_num, dev->vol_id, UBI_READWRITE);
			dev->param->u4BlockDeviceModeFlag = 0;
			mt_ftl_commit(dev);
		} /*else if (data == 1) {
			set_disk_ro(dev->gd, 1);
			dev->param->u4BlockDeviceModeFlag = 1;
		}*/ /* Don't deal with data:1, thanks*/
		ret = 0;
		break;
	case BLKFLSBUF:
		ret = mt_ftl_blk_flush(dev, true);
		break;
	default:
		ret = -ENOTTY;
	}
	mt_ftl_debug(dev, "CMD:%d data:%d ret:%d", cmd, data, ret);
	mutex_unlock(&dev->dev_mutex);
	return ret;
}

static const struct block_device_operations mt_ftl_blk_ops = {
	.owner = THIS_MODULE,
	.open = mt_ftl_blk_open,
	.release = mt_ftl_blk_release,
	.getgeo	= mt_ftl_blk_getgeo,
	.ioctl = mt_ftl_blk_ioctl,
};

int mt_ftl_blk_create(struct ubi_volume_desc *desc)
{
	struct mt_ftl_blk *dev;
	struct gendisk *gd;
	u64 disk_capacity;
	int ret;
	struct ubi_volume_info vi;

	ubi_get_volume_info(desc, &vi);
	/* Check that the volume isn't already handled */
	mutex_lock(&devices_mutex);
	if (find_dev_nolock(vi.ubi_num, vi.vol_id)) {
		mutex_unlock(&devices_mutex);
		return -EEXIST;
	}
	mutex_unlock(&devices_mutex);

	dev = kzalloc(sizeof(struct mt_ftl_blk), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	mutex_init(&dev->dev_mutex);
	dev->dev_blocks = desc->vol->reserved_pebs - NAND_RESERVED_PEB;
	dev->ubi_num = vi.ubi_num;
	dev->vol_id = vi.vol_id;
	dev->leb_size = vi.usable_leb_size;
	dev->min_io_size = desc->vol->ubi->min_io_size;
	/* Page Mapping per nand page (must be power of 2), so SHIFT value */
	dev->pm_per_io = FS_PAGE_SHIFT;
	dev->readonly = 0;
	dev->max_replay_blks = dev->leb_size / dev->min_io_size * REPLAY_BLOCK_NUM;
	pr_crit("\n\tFTL INFO:\n\tubi_num:%d vol_id:%d name:%s leb_size:%d",
		dev->ubi_num, dev->vol_id, vi.name, dev->leb_size);
	pr_crit("\n\tmin_io_size:%d pm_per_io:%d dev_blocks:%d\n\tmax_replay_blks:%d\n",
		dev->min_io_size, dev->pm_per_io, dev->dev_blocks, dev->max_replay_blks);
	/* Initialize the gendisk of this mt_ftl_blk device */
	gd = alloc_disk(1);
	if (!gd) {
		mt_ftl_err(dev, "block: alloc_disk failed");
		ret = -ENODEV;
		goto out_free_dev;
	}

	gd->fops = &mt_ftl_blk_ops;
	gd->major = mt_ftl_blk_major;
	gd->first_minor = dev->ubi_num * UBI_MAX_VOLUMES + dev->vol_id;
	gd->private_data = dev;
	sprintf(gd->disk_name, "mt_ftl_blk%d_%d", dev->ubi_num, dev->vol_id);
	/* To do readonly in attach ftl */
	if (strstr(vi.name, "system")) {
		disk_capacity = ((vi.used_bytes - DATA_START_BLOCK * vi.usable_leb_size) >> 9);
		dev->readonly = 1;
	} else
		disk_capacity = ((vi.used_bytes - NAND_RESERVED_FTL * vi.usable_leb_size) >> 9);
	if ((sector_t)disk_capacity != disk_capacity) {
		ret = -EFBIG;
		goto out_put_disk;
	}
	set_capacity(gd, disk_capacity);
	dev->gd = gd;

	spin_lock_init(&dev->queue_lock);
#ifdef MT_FTL_SUPPORT_MQ
	memset(&dev->tag_set, 0, sizeof(dev->tag_set));
	dev->tag_set.ops = &mt_ftl_mq_ops;
	dev->tag_set.queue_depth = MT_FTL_MAX_SLOTS;
	dev->tag_set.numa_node = NUMA_NO_NODE;
	dev->tag_set.flags = BLK_MQ_F_SHOULD_MERGE;
#ifdef MT_FTL_SUPPORT_THREAD
	dev->tag_set.cmd_size = sizeof(struct mt_ftl_blk_rq);
#else
	dev->tag_set.cmd_size = sizeof(struct mt_ftl_blk_pdu);
#endif
	dev->tag_set.driver_data = dev;
	/*because we have ubi device support nr_cpu_ids, else default 1*/
	dev->tag_set.nr_hw_queues = 1;
	dev->tag_set.reserved_tags = 1;

	ret = blk_mq_alloc_tag_set(&dev->tag_set);
	if (ret)
		goto out_put_disk;

	dev->rq = blk_mq_init_queue(&dev->tag_set);
#else
	dev->rq = blk_init_queue(mt_ftl_blk_request, &dev->queue_lock);
#endif
	if (IS_ERR(dev->rq)) {
		mt_ftl_err(dev, "block: blk_init_queue failed");
		ret = -ENODEV;
		goto out_put_disk;
	}
	dev->rq->queuedata = dev;
	dev->gd->queue = dev->rq;

	/*ret = elevator_change(dev->rq, "noop");
	if (ret)
		goto out_put_disk;*/
	/*if (dev->readonly)
		blk_queue_flush(dev->rq, REQ_FLUSH);  *//*cost down perf.*/

	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, dev->rq);
	queue_flag_clear_unlocked(QUEUE_FLAG_ADD_RANDOM, dev->rq);
	queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, dev->rq);
	dev->rq->limits.discard_zeroes_data = 1;
	dev->rq->limits.discard_granularity = FS_PAGE_SIZE;
	dev->rq->limits.max_discard_sectors = NAND_VOLUME << 1;

	blk_queue_max_segments(dev->rq, MT_FTL_MAX_SG);
	blk_queue_bounce_limit(dev->rq, BLK_BOUNCE_HIGH);
	blk_queue_max_hw_sectors(dev->rq, -1U);
	blk_queue_max_segment_size(dev->rq, MT_FTL_MAX_SG_SZ);
	blk_queue_physical_block_size(dev->rq, FS_PAGE_SIZE);
	blk_queue_logical_block_size(dev->rq, FS_PAGE_SIZE);
	blk_queue_io_min(dev->rq, FS_PAGE_SIZE);
#ifdef MT_FTL_SUPPORT_MQ
#ifndef MT_FTL_SUPPORT_THREAD
	/* if use create_workqueue, please open mutex_lock on mt_ftl_do_mq_work */
	dev->wq = create_singlethread_workqueue(gd->disk_name);
	if (!dev->wq) {
		ret = -ENOMEM;
		goto out_free_queue;
	}
#endif
#else
	/* dev->wq = create_singlethread_workqueue(gd->disk_name); */
	dev->wq = create_workqueue(gd->disk_name);
	if (!dev->wq) {
		ret = -ENOMEM;
		goto out_free_queue;
	}

	INIT_WORK(&dev->work, mt_ftl_blk_do_work);
#endif
	INIT_LIST_HEAD(&dev->discard_list);
	dev->discard_count = 0;

	mutex_lock(&devices_mutex);
	list_add_tail(&dev->list, &mt_ftl_blk_devices);
	mutex_unlock(&devices_mutex);

	/* Must be the last step: anyone can call file ops from now on */
	add_disk(dev->gd);

	dev->param = kzalloc(sizeof(struct mt_ftl_param), GFP_KERNEL);
	if (!dev->param) {
		ret = -ENOMEM;
		goto out_free_queue;
	}

	dev->desc = desc;

	dev->discard_slab = kmem_cache_create("mtftl_discard_slab",
					sizeof(struct mt_ftl_discard_entry),
					0,
					SLAB_HWCACHE_ALIGN | SLAB_PANIC,
					NULL);
	if (!dev->discard_slab) {
		ret = -ENOMEM;
		goto out_free_param;
	}

	ret = mt_ftl_create(dev);
	if (ret) {
		ret = -EFAULT;
		goto out_free_cache;
	}

	ret = mt_ftl_blk_mq_init(dev);
	if (ret)
		goto out_free_cache;

	return 0;

out_free_cache:
	kmem_cache_destroy(dev->discard_slab);
	dev->discard_slab = NULL;
out_free_param:
	kfree(dev->param);
	dev->param = NULL;
out_free_queue:
	blk_cleanup_queue(dev->rq);
out_put_disk:
	put_disk(dev->gd);
out_free_dev:
	kfree(dev);
	mutex_destroy(&dev->dev_mutex);

	return ret;
}

static void mt_ftl_blk_cleanup(struct mt_ftl_blk *dev)
{
	struct mt_ftl_discard_entry *dd, *n;

	del_gendisk(dev->gd);
	destroy_workqueue(dev->wq);
	blk_cleanup_queue(dev->rq);
	if (dev->discard_slab) {
		list_for_each_entry_safe(dd, n, &dev->discard_list, list) {
			list_del_init(&dd->list);
			kmem_cache_free(dev->discard_slab, dd);
		}
		kmem_cache_destroy(dev->discard_slab);
		dev->discard_slab = NULL;
	}
#ifdef MT_FTL_SUPPORT_MQ
	blk_mq_free_tag_set(&dev->tag_set);
#ifdef MT_FTL_SUPPORT_THREAD
	kthread_stop(dev->blk_bgt);
#endif
#endif
	pr_notice("UBI: %s released", dev->gd->disk_name);
	put_disk(dev->gd);
}

int mt_ftl_blk_remove(struct ubi_volume_info *vi)
{
	struct mt_ftl_blk *dev;
	int ret;

	mutex_lock(&devices_mutex);
	dev = find_dev_nolock(vi->ubi_num, vi->vol_id);
	if (!dev) {
		mutex_unlock(&devices_mutex);
		return -ENODEV;
	}

	ret = mt_ftl_remove(dev);
	if (ret)
		return -EFAULT;

	/* Found a device, let's lock it so we can check if it's busy */
	mutex_lock(&dev->dev_mutex);
	if (dev->refcnt > 0) {
		mutex_unlock(&dev->dev_mutex);
		mutex_unlock(&devices_mutex);
		return -EBUSY;
	}

	/* Remove from device list */
	list_del(&dev->list);
	mutex_unlock(&devices_mutex);

	mt_ftl_blk_cleanup(dev);
	mutex_unlock(&dev->dev_mutex);
	kfree(dev);
	return 0;
}

static int mt_ftl_blk_resize(struct ubi_volume_info *vi)
{
	struct mt_ftl_blk *dev;
	u64 disk_capacity = ((vi->used_bytes - NAND_RESERVED_FTL * vi->usable_leb_size) >> 9);

	if (strstr(vi->name, "system"))
		disk_capacity = ((vi->used_bytes - DATA_START_BLOCK * vi->usable_leb_size) >> 9);
	/*
	 * Need to lock the device list until we stop using the device,
	 * otherwise the device struct might get released in
	 * 'mt_ftl_blk_remove()'.
	 */
	mutex_lock(&devices_mutex);
	dev = find_dev_nolock(vi->ubi_num, vi->vol_id);
	if (!dev) {
		mutex_unlock(&devices_mutex);
		return -ENODEV;
	}
	if ((sector_t)disk_capacity != disk_capacity) {
		mutex_unlock(&devices_mutex);
		mt_ftl_err(dev, "%s: the volume is too big (%d LEBs), cannot resize", dev->gd->disk_name, vi->size);
		return -EFBIG;
	}

	mutex_lock(&dev->dev_mutex);
	dev->dev_blocks = dev->desc->vol->reserved_pebs - NAND_RESERVED_PEB;
	pr_crit("FTL[%d] resize to blocks %d\n", vi->ubi_num, dev->dev_blocks);
	if (get_capacity(dev->gd) != disk_capacity) {
		set_capacity(dev->gd, disk_capacity);
		pr_notice("UBI[%d]: %s resized to %lld bytes", vi->ubi_num, dev->gd->disk_name,
			vi->used_bytes);
	}
	mutex_unlock(&dev->dev_mutex);
	mutex_unlock(&devices_mutex);
	return 0;
}

static int mt_ftl_blk_notify(struct notifier_block *nb,
			 unsigned long notification_type, void *ns_ptr)
{
	struct ubi_notification *nt = ns_ptr;

	switch (notification_type) {
	case UBI_VOLUME_ADDED:
		/*
		 * We want to enforce explicit block device creation for
		 * volumes, so when a volume is added we do nothing.
		 */
		break;
	case UBI_VOLUME_REMOVED:
		mt_ftl_blk_remove(&nt->vi);
		break;
	case UBI_VOLUME_RESIZED:
		mt_ftl_blk_resize(&nt->vi);
		break;
	case UBI_VOLUME_UPDATED:
		/*
		 * If the volume is static, a content update might mean the
		 * size (i.e. used_bytes) was also changed.
		 */
		if (nt->vi.vol_type == UBI_STATIC_VOLUME)
			mt_ftl_blk_resize(&nt->vi);
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block mt_ftl_blk_notifier = {
	.notifier_call = mt_ftl_blk_notify,
};

static struct ubi_volume_desc * __init
open_volume_desc(const char *name, int ubi_num, int vol_id)
{
	if (ubi_num == -1)
		/* No ubi num, name must be a vol device path */
		return ubi_open_volume_path(name, UBI_READWRITE);
	else if (vol_id == -1)
		/* No vol_id, must be vol_name */
		return ubi_open_volume_nm(ubi_num, name, UBI_READWRITE);
	else
		return ubi_open_volume(ubi_num, vol_id, UBI_READWRITE);
}

static int __init mt_ftl_blk_create_from_param(void)
{
	int i, ret;
	struct mt_ftl_blk_param *p;
	struct ubi_volume_desc *desc;
	struct ubi_volume_info vi;

	for (i = 0; i < mt_ftl_blk_devs; i++) {
		p = &mt_ftl_blk_param[i];
		desc = open_volume_desc(p->name, p->ubi_num, p->vol_id);
		if (IS_ERR(desc)) {
			ubi_err("block: can't open volume, err=%ld\n", PTR_ERR(desc));
			ret = PTR_ERR(desc);
			break;
		}

		ubi_get_volume_info(desc, &vi);
		ret = mt_ftl_blk_create(desc);
		if (ret) {
			ubi_err("block: can't add '%s' volume, err=%d\n", vi.name, ret);
			break;
		}
		ubi_close_volume(desc);
	}
	return ret;
}

static void mt_ftl_blk_remove_all(void)
{
	struct mt_ftl_blk *next;
	struct mt_ftl_blk *dev;

	list_for_each_entry_safe(dev, next, &mt_ftl_blk_devices, list) {
		/* Flush pending work and stop workqueue */
		destroy_workqueue(dev->wq);
		/* The module is being forcefully removed */
		WARN_ON(dev->desc);
		/* Remove from device list */
		list_del(&dev->list);
		mt_ftl_blk_cleanup(dev);
		kfree(dev);
	}
}

int __init mt_ftl_blk_init(void)
{
	int ret;

	mt_ftl_blk_major = register_blkdev(0, "mt_ftl_blk");
	if (mt_ftl_blk_major < 0)
		return mt_ftl_blk_major;

	/* Attach block devices from 'block=' module param */
	ret = mt_ftl_blk_create_from_param();
	if (ret)
		goto err_remove;

	/*
	 * Block devices are only created upon user requests, so we ignore
	 * existing volumes.
	 */
	ret = ubi_register_volume_notifier(&mt_ftl_blk_notifier, 1);
	if (ret)
		goto err_unreg;
	return 0;

err_unreg:
	unregister_blkdev(mt_ftl_blk_major, "mt_ftl_blk");
err_remove:
	mt_ftl_blk_remove_all();
	return ret;
}

void __exit mt_ftl_blk_exit(void)
{
	ubi_unregister_volume_notifier(&mt_ftl_blk_notifier);
	mt_ftl_blk_remove_all();
	unregister_blkdev(mt_ftl_blk_major, "mt_ftl_blk");
}

module_init(mt_ftl_blk_init);
module_exit(mt_ftl_blk_exit);
