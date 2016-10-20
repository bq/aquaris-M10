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

#include "mt_ftl.h"
#include "ubi/ubi.h"
#include <linux/crypto.h>
#ifdef MT_FTL_PROFILE
#include <linux/time.h>

unsigned long profile_time[MT_FTL_PROFILE_TOTAL_PROFILE_NUM];
unsigned long start_time[MT_FTL_PROFILE_TOTAL_PROFILE_NUM];
unsigned long end_time;
unsigned long start_time_all[MT_FTL_PROFILE_TOTAL_PROFILE_NUM];
unsigned long end_time_all;

unsigned long getnstimenow(void)
{
	struct timespec tv;

	getnstimeofday(&tv);
	return tv.tv_sec * 1000000000 + tv.tv_nsec;
}

#define MT_FTL_PROFILE_START(x)		(start_time[x] = getnstimenow())

#define MT_FTL_PROFILE_START_ALL(x)	(start_time_all[x] = getnstimenow())

#define MT_FTL_PROFILE_END(x)	\
	do { \
		end_time = getnstimenow(); \
		if (end_time >= start_time[x]) \
			profile_time[x] += (end_time - start_time[x]) / 1000; \
		else { \
			mt_ftl_err(dev, "end_time = %lu, start_time = %lu", end_time, start_time[x]);\
			profile_time[x] += (end_time + 0xFFFFFFFF - start_time[x]) / 1000; \
		} \
	} while (0)
#define MT_FTL_PROFILE_END_ALL(x)	\
	do { \
		end_time_all = getnstimenow(); \
		if (end_time_all >= start_time_all[x]) \
			profile_time[x] += (end_time_all - start_time_all[x]) / 1000; \
		else { \
			mt_ftl_err(dev, "end_time = %lu, start_time = %lu", end_time_all, start_time_all[x]); \
			profile_time[x] += (end_time_all + 0xFFFFFFFF - start_time_all[x]) / 1000; \
		} \
	} while (0)

char *mtk_ftl_profile_message[MT_FTL_PROFILE_TOTAL_PROFILE_NUM] = {
	"MT_FTL_PROFILE_WRITE_ALL",
	"MT_FTL_PROFILE_WRITE_COPYTOCACHE",
	"MT_FTL_PROFILE_WRITE_UPDATEPMT",
	"MT_FTL_PROFILE_UPDATE_PMT_MODIFYPMT",
	"MT_FTL_PROFILE_UPDATE_PMT_FINDCACHE_COMMITPMT",
	"MT_FTL_PROFILE_COMMIT_PMT",
	"MT_FTL_PROFILE_UPDATE_PMT_DOWNLOADPMT",
	"MT_FTL_PROFILE_WRITE_COMPRESS",
	"MT_FTL_PROFILE_WRITE_WRITEPAGE",
	"MT_FTL_PROFILE_WRITE_PAGE_WRITEOOB",
	"MT_FTL_PROFILE_WRITE_PAGE_GETFREEBLK",
	"MT_FTL_PROFILE_GETFREEBLOCK_PMT_GETLEB",
	"MT_FTL_PROFILE_GETFREEBLOCK_GETLEB",
	"MT_FTL_PROFILE_GC_FINDBLK",
	"MT_FTL_PROFILE_GC_CPVALID",
	"MT_FTL_PROFILE_GC_DATA_READOOB",
	"MT_FTL_PROFILE_GC_DATA_READ_UPDATE_PMT",
	"MT_FTL_PROFILE_GC_DATA_WRITEOOB",
	"MT_FTL_PROFILE_GC_REMAP",
	"MT_FTL_PROFILE_GETFREEBLOCK_PUTREPLAY_COMMIT",
	"MT_FTL_PROFILE_COMMIT",
	"MT_FTL_PROFILE_WRITE_PAGE_RESET",
	"MT_FTL_PROFILE_READ_ALL",
	"MT_FTL_PROFILE_READ_GETPMT",
	"MT_FTL_PROFILE_READ_DATATOCACHE",
	"MT_FTL_PROFILE_READ_DATATOCACHE_TEST1",
	"MT_FTL_PROFILE_READ_DATATOCACHE_TEST2",
	"MT_FTL_PROFILE_READ_DATATOCACHE_TEST3",
	"MT_FTL_PROFILE_READ_ADDRNOMATCH",
	"MT_FTL_PROFILE_READ_DECOMP",
	"MT_FTL_PROFILE_READ_COPYTOBUFF",
};
#else
#define MT_FTL_PROFILE_START(x)
#define MT_FTL_PROFILE_START_ALL(x)
#define MT_FTL_PROFILE_END(x)
#define MT_FTL_PROFILE_END_ALL(x)
#endif

static int mt_ftl_compress(struct mt_ftl_blk *dev, void *in_buf, int in_len, int *out_len)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_param *param = dev->param;

	if (!param->cc) {
		*out_len = in_len;
		param->cmpr_page_buffer = in_buf;
		return ret;
	}
	ubi_assert(in_len <= FS_PAGE_SIZE);
	memset(param->cmpr_page_buffer, 0xFF, dev->min_io_size * sizeof(unsigned char));
	ret = crypto_comp_compress(param->cc, in_buf, in_len, param->cmpr_page_buffer, out_len);
	if (ret) {
		mt_ftl_err(dev, "ret=%d, out_len=%d, in_len=0x%x", ret, *out_len, in_len);
		mt_ftl_err(dev, "cc=0x%lx, buffer=0x%lx", (unsigned long int)param->cc, (unsigned long int)in_buf);
		mt_ftl_err(dev, "cmpr_page_buffer=0x%lx", (unsigned long int)param->cmpr_page_buffer);
		return MT_FTL_FAIL;
	}
	if (*out_len >= FS_PAGE_SIZE) {
		/* mt_ftl_err(dev, "compress out_len(%d) large than in_len(%d)FS_PAGE_SIZE", *out_len, in_len); */
		memcpy(param->cmpr_page_buffer, in_buf, in_len);
		*out_len = in_len;
	}
	return ret;
}

static int mt_ftl_decompress(struct mt_ftl_blk *dev, void *in_buf, int in_len, int *out_len)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_param *param = dev->param;

	if (!param->cc) {
		*out_len = in_len;
		param->cmpr_page_buffer = in_buf;
		return ret;
	}
	ubi_assert(in_len <= FS_PAGE_SIZE);
	memset(param->cmpr_page_buffer, 0xFF, dev->min_io_size * sizeof(unsigned char));
	if (in_len == FS_PAGE_SIZE) {
		/* mt_ftl_err(dev, "decompress out_len(%d) equal (FS_PAGE_SIZE)", in_len); */
		memcpy(param->cmpr_page_buffer, in_buf, in_len);
		*out_len = in_len;
	} else {
		ret = crypto_comp_decompress(param->cc, in_buf, in_len, param->cmpr_page_buffer, out_len);
		if (ret) {
			mt_ftl_err(dev, "ret=%d, out_len=%d, in_len=0x%x", ret, *out_len, in_len);
			mt_ftl_err(dev, "cmpr_page_buffer=0x%lx", (unsigned long int)param->cmpr_page_buffer);
			return MT_FTL_FAIL;
		}
	}
	return ret;
}

#ifdef MT_FTL_SUPPORT_WBUF

#define MT_FTL_WBUF_NOSYNC(dev)    ((dev)->need_sync = 0)

static int mt_ftl_wbuf_init(struct mt_ftl_blk *dev)
{
	int i, num;
	struct mt_ftl_wbuf_entry *ftl_wbuf;

	/* mutex_init(&dev->wbuf_mutex); */
	init_waitqueue_head(&dev->wbuf_wait);
	init_waitqueue_head(&dev->sync_wait);
	INIT_LIST_HEAD(&dev->wbuf_list);
	INIT_LIST_HEAD(&dev->pool_free_list);
	spin_lock_init(&dev->wbuf_lock);
	spin_lock_init(&dev->pool_lock);
	dev->wbuf_pool_num = 0;
	dev->wbuf_count = 0;
	memset(dev->pool_bitmap, 'E', MT_FTL_MAX_WBUF_NUM);
	dev->wbuf_pool[0] = vmalloc(dev->leb_size);
	if (!dev->wbuf_pool[0]) {
		mt_ftl_err(dev, "alloc wbuf_pool[%d] FAIL", dev->wbuf_pool_num);
		return -ENOMEM;
	}
	num = dev->leb_size / dev->min_io_size;
	for (i = 0; i < num; i++) {
		ftl_wbuf = kzalloc(sizeof(struct mt_ftl_wbuf_entry), GFP_KERNEL);
		if (!ftl_wbuf) {
			mt_ftl_err(dev, "alloc wbuf_entry FAIL");
			return -ENOMEM;
		}
		ftl_wbuf->pool_num = 0;
		ftl_wbuf->lnum = -1;
		ftl_wbuf->offset = -1;
		ftl_wbuf->len = -1;
		ftl_wbuf->wbuf = dev->wbuf_pool[0] + dev->min_io_size * i;
		ftl_wbuf->dev = NULL;
		spin_lock(&dev->pool_lock);
		list_add_tail(&ftl_wbuf->list, &dev->pool_free_list);
		dev->wbuf_count++;
		spin_unlock(&dev->pool_lock);
	}
	spin_lock(&dev->pool_lock);
	dev->pool_bitmap[0] = 'B';
	dev->wbuf_pool_num++;
	spin_unlock(&dev->pool_lock);
	return 0;
}

static int mt_ftl_wbuf_get_pool(struct mt_ftl_blk *dev)
{
	int i;

	for (i = 0; i < MT_FTL_MAX_WBUF_NUM; i++) {
		if (dev->pool_bitmap[i] == 'E')
			return i;
	}
	return -1;
}

static int mt_ftl_wbuf_expand(struct mt_ftl_blk *dev)
{
	int i, num, pool_num;
	struct mt_ftl_wbuf_entry *ftl_wbuf;

	if (dev->wbuf_pool_num >= MT_FTL_MAX_WBUF_NUM)
		return MT_FTL_FAIL;
	pool_num = mt_ftl_wbuf_get_pool(dev);
	dev->wbuf_pool[pool_num] = vmalloc(dev->leb_size);
	if (!dev->wbuf_pool[pool_num]) {
		mt_ftl_err(dev, "alloc wbuf_pool[%d] FAIL", pool_num);
		return -ENOMEM;
	}
	num = dev->leb_size / dev->min_io_size;
	for (i = 0; i < num; i++) {
		ftl_wbuf = kzalloc(sizeof(struct mt_ftl_wbuf_entry), GFP_KERNEL);
		if (!ftl_wbuf) {
			mt_ftl_err(dev, "alloc wbuf_entry FAIL");
			return -ENOMEM;
		}
		ftl_wbuf->pool_num = pool_num;
		ftl_wbuf->lnum = -1;
		ftl_wbuf->offset = -1;
		ftl_wbuf->len = -1;
		ftl_wbuf->wbuf = dev->wbuf_pool[pool_num] + dev->min_io_size * i;
		ftl_wbuf->dev = NULL;
		spin_lock(&dev->pool_lock);
		list_add_tail(&ftl_wbuf->list, &dev->pool_free_list);
		dev->wbuf_count++;
		spin_unlock(&dev->pool_lock);
	}
	spin_lock(&dev->pool_lock);
	dev->pool_bitmap[pool_num] = 'B';
	dev->wbuf_pool_num++;
	spin_unlock(&dev->pool_lock);
	mt_ftl_err(dev, "expand up to %d %d", pool_num, dev->wbuf_pool_num);
	return 0;
}

static void mt_ftl_wbuf_shrink(struct mt_ftl_blk *dev)
{
	int i, num, pool_num[MT_FTL_MAX_WBUF_NUM];
	struct mt_ftl_wbuf_entry *rr, *n;

	if (dev->wbuf_pool_num == 1)
		return;

	num = dev->leb_size / dev->min_io_size;
	if (dev->wbuf_count < num)
		return;
	memset(pool_num, 0, sizeof(int) * MT_FTL_MAX_WBUF_NUM);
	for (i = 0; i < dev->wbuf_pool_num; i++) {
		spin_lock(&dev->pool_lock);
		list_for_each_entry_safe(rr, n, &dev->pool_free_list, list) {
			if (i == rr->pool_num && dev->pool_bitmap[i] == 'A')
				pool_num[i]++;
		}
		if (pool_num[i] == num) {
			mt_ftl_err(dev, "found %d, need to shrink %d:%d", i, dev->wbuf_pool_num, dev->wbuf_count);
			list_for_each_entry_safe(rr, n, &dev->pool_free_list, list) {
				if (i == rr->pool_num) {
					list_del_init(&rr->list);
					kfree(rr);
					dev->wbuf_count--;
					rr = NULL;
				}
			}
			dev->pool_bitmap[i] = 'E';
			dev->wbuf_pool_num--;
			spin_unlock(&dev->pool_lock);
			vfree(dev->wbuf_pool[i]);
			if (waitqueue_active(&dev->wbuf_wait)) {
				mt_ftl_err(dev, "wakeup wbuf1");
				wake_up(&dev->wbuf_wait);
			}
			schedule();
			continue;
		}
		spin_unlock(&dev->pool_lock);
	}
}

static int mt_ftl_wbuf_flush(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_wbuf_entry *ww;

	if (list_empty(&dev->wbuf_list)) {
		dev->need_bgt = 0;
		mt_ftl_wbuf_shrink(dev);
		if (waitqueue_active(&dev->wbuf_wait))
			wake_up(&dev->wbuf_wait);
		if (waitqueue_active(&dev->sync_wait))
			wake_up(&dev->sync_wait);
		cond_resched();
		return ret;
	}
	spin_lock(&dev->wbuf_lock);
	ww = list_first_entry(&dev->wbuf_list, struct mt_ftl_wbuf_entry, list);
	list_del_init(&ww->list);
	spin_unlock(&dev->wbuf_lock);
	/* mt_ftl_err(dev, "w%d %d %d(%lx:%lx:%lx)", ww->lnum, ww->offset, ww->len,
		(unsigned long)ww->wbuf, (unsigned long)desc, (unsigned long)dev->desc); */
	ret = ubi_leb_write(ww->dev->desc, ww->lnum, ww->wbuf, ww->offset, ww->len);
	if (ret) {
		mt_ftl_err(dev, "Write failed:leb=%u,offset=%d,len=%d ret=%d",
			ww->lnum, ww->offset, ww->len, ret);
		ubi_assert(false);
		return ret;
	}
	spin_lock(&dev->pool_lock);
	list_add_tail(&ww->list, &dev->pool_free_list);
	dev->wbuf_count++;
	dev->pool_bitmap[ww->pool_num] = 'A';
	spin_unlock(&dev->pool_lock);
	if (dev->wbuf_pool_num > MT_FTL_WBUF_THRESHOLD) {
		mt_ftl_wbuf_shrink(dev);
		if (waitqueue_active(&dev->wbuf_wait)) {
			mt_ftl_err(dev, "wakeup wbuf2");
			wake_up(&dev->wbuf_wait);
		}
	}
	return ret;
}

static void mt_ftl_wakeup_wbuf_thread(struct mt_ftl_blk *dev)
{
	if (dev->bgt && !dev->need_bgt && !dev->readonly) {
		dev->need_bgt = 1;
		wake_up_process(dev->bgt);
	}
}

int mt_ftl_wbuf_forced_flush(struct mt_ftl_blk *dev)
{
	if (!waitqueue_active(&dev->sync_wait)) {
		mt_ftl_wakeup_wbuf_thread(dev);
		wait_event(dev->sync_wait, list_empty(&dev->wbuf_list));
	}

	return 0;
}

int mt_ftl_wbuf_thread(void *info)
{
	int err;
	struct mt_ftl_blk *dev = info;

	mt_ftl_err(dev, "FTL wbuf thread started, PID %d", current->pid);
	set_freezable();

	while (1) {
		if (kthread_should_stop())
			break;

		if (try_to_freeze())
			continue;

		set_current_state(TASK_INTERRUPTIBLE);
		/* Check if there is something to do */
		if (!dev->need_bgt) {
			if (kthread_should_stop())
				break;
			schedule();
			continue;
		} else
			__set_current_state(TASK_RUNNING);

		mutex_lock(&dev->dev_mutex);
		err = mt_ftl_wbuf_flush(dev);
		mutex_unlock(&dev->dev_mutex);
		if (err)
			return err;

		cond_resched();
	}

	mt_ftl_err(dev, "FTL wbuf thread stops");
	return 0;
}

static int mt_ftl_wbuf_alloc(struct mt_ftl_blk *dev)
{
	int err;

	dev->need_sync = 1;
	dev->need_bgt = 0;
	dev->bgt = NULL;
	if (!dev->readonly) {
		err = mt_ftl_wbuf_init(dev);
		if (err < 0)
			return err;
		/* Create background thread */
		sprintf(dev->bgt_name, "ftl_wbuf%d_%d", dev->ubi_num, dev->vol_id);
		dev->bgt = kthread_create(mt_ftl_wbuf_thread, dev, "%s", dev->bgt_name);
		if (IS_ERR(dev->bgt)) {
			err = PTR_ERR(dev->bgt);
			dev->bgt = NULL;
			mt_ftl_err(dev, "cannot spawn \"%s\", error %d", dev->bgt_name, err);
			return MT_FTL_FAIL;
		}
		wake_up_process(dev->bgt);
	}
	return MT_FTL_SUCCESS;
}

static void mt_ftl_wbuf_free(struct mt_ftl_blk *dev)
{
	int i;
	struct mt_ftl_wbuf_entry *dd, *n;

	dev->need_sync = 1;
	dev->need_bgt = 0;
	if (!dev->readonly) {
		if (dev->bgt)
			kthread_stop(dev->bgt);
		list_for_each_entry_safe(dd, n, &dev->wbuf_list, list) {
			list_del_init(&dd->list);
			kfree(dd);
		}
		list_for_each_entry_safe(dd, n, &dev->pool_free_list, list) {
			list_del_init(&dd->list);
			kfree(dd);
		}
		for (i = 0; i < MT_FTL_MAX_WBUF_NUM; i++) {
			if (!dev->wbuf_pool[i])
				vfree(dev->wbuf_pool);
		}
		/* mutex_destroy(&dev->wbuf_mutex); */
	}
}
#else
#define MT_FTL_WBUF_NOSYNC(dev)
#define mt_ftl_wbuf_alloc(dev) (MT_FTL_SUCCESS)
#define mt_ftl_wbuf_free(dev)
#endif


static int mt_ftl_leb_map(struct mt_ftl_blk *dev, int lnum)
{
	int ret;

	ret = ubi_is_mapped(dev->desc, lnum);
	if (!ret) {
		mt_ftl_debug(dev, "leb %d is unmapped", lnum);
		ubi_leb_map(dev->desc, lnum);
	}

	return ret;
}

static void mt_ftl_leb_remap(struct mt_ftl_blk *dev, int lnum)
{
	int ret;

	mt_ftl_debug(dev, "eba_tbl[%d]=%d to unmap", lnum, dev->desc->vol->eba_tbl[lnum]);
	ret = ubi_is_mapped(dev->desc, lnum);
	if (ret)
		ubi_leb_unmap(dev->desc, lnum);
	ubi_leb_map(dev->desc, lnum);
	mt_ftl_debug(dev, "eba_tbl[%d]=%d has mapped", lnum, dev->desc->vol->eba_tbl[lnum]);
}

#ifdef MT_FTL_SUPPORT_WBUF
static int mt_ftl_leb_read(struct mt_ftl_blk *dev, int lnum, void *buf, int offset, int len)
{
	int ret = MT_FTL_SUCCESS;
	int copy_offset = 0;
	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_wbuf_entry *rr, *n;

	if (!dev->readonly && lnum >= DATA_START_BLOCK) {
		spin_lock(&dev->wbuf_lock);
		list_for_each_entry_safe(rr, n, &dev->wbuf_list, list) {
			if ((rr->lnum == lnum) && (rr->offset <= offset) &&
				((offset + len) <= (rr->offset + rr->len))) {
				copy_offset = (offset == rr->offset) ? 0 : (offset % dev->min_io_size);
				mt_ftl_err(dev, "rbuf read %d %d %d %d", lnum, offset, copy_offset, len);
				memcpy(buf, rr->wbuf + copy_offset, len);
				spin_unlock(&dev->wbuf_lock);
				return MT_FTL_SUCCESS;
			}
		}
		spin_unlock(&dev->wbuf_lock);
	}
	ret = ubi_leb_read(desc, lnum, (char *)buf, offset, len, 0);
	if (ret == UBI_IO_BITFLIPS) {
		ret = MT_FTL_SUCCESS;
	} else if (ret) {
		mt_ftl_err(dev, "Read failed:leb=%u,offset=%d ret=%d", lnum, offset, ret);
		ubi_assert(false);
		return MT_FTL_FAIL;
	}
	cond_resched();
	return ret;
}

static int mt_ftl_leb_write(struct mt_ftl_blk *dev, int lnum, void *buf, int offset, int len)
{
	int ret = MT_FTL_SUCCESS;
	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_wbuf_entry *ftl_wbuf;
	/* unsigned long flags; */

	if (dev->readonly)
		dev->need_sync = 1;

	if (dev->need_sync || lnum < DATA_START_BLOCK) {
retry:
		ret = ubi_leb_write(desc, lnum, buf, offset, len);
		if (ret) {
			if (ret == -EROFS) {
				mt_ftl_err(dev, "RO Volume, switch to RW");
				ubi_close_volume(dev->desc);
				dev->desc = ubi_open_volume(dev->ubi_num, dev->vol_id, UBI_READWRITE);
				desc = dev->desc;
				goto retry;
			}
			mt_ftl_err(dev, "Write failed:leb=%u,offset=%d ret=%d", lnum, offset, ret);
			ubi_assert(false);
			return MT_FTL_FAIL;
		}
		cond_resched();
		return ret;
	}
	ubi_assert(len <= dev->min_io_size);
	/*if (list_empty(&dev->pool_free_list)) {
		mt_ftl_wakeup_wbuf_thread(dev);
		wait_event_timeout(dev->wbuf_wait, !list_empty(&dev->pool_free_list),
			MT_FTL_TIMEOUT_MSECS * HZ / 1000);
	}*/
	if (list_empty(&dev->pool_free_list)) {
		ret = mt_ftl_wbuf_expand(dev);
		if (ret) {
			ret = MT_FTL_SUCCESS;
			mt_ftl_err(dev, "sleep wbuf");
			mt_ftl_wakeup_wbuf_thread(dev);
			wait_event_timeout(dev->wbuf_wait, !list_empty(&dev->pool_free_list),
				MT_FTL_TIMEOUT_MSECS * HZ / 1000);
		}
	}
	spin_lock(&dev->pool_lock);
	ftl_wbuf = list_first_entry(&dev->pool_free_list, struct mt_ftl_wbuf_entry, list);
	list_del_init(&ftl_wbuf->list);
	dev->wbuf_count--;
	spin_unlock(&dev->pool_lock);
	ftl_wbuf->lnum = lnum;
	ftl_wbuf->offset = offset;
	ftl_wbuf->len = len;
	ftl_wbuf->dev = dev;
	memset(ftl_wbuf->wbuf, 0xFF, dev->min_io_size);
	memcpy(ftl_wbuf->wbuf, buf, len);
	/* mt_ftl_err(dev, "ww%d %d %d(%lx)", lnum, offset, len, (unsigned long)ftl_wbuf->wbuf); */
	spin_lock(&dev->wbuf_lock);
	list_add_tail(&ftl_wbuf->list, &dev->wbuf_list);
	spin_unlock(&dev->wbuf_lock);
	dev->need_sync = 1;
	mt_ftl_wakeup_wbuf_thread(dev);
	return ret;
}
#else
static int mt_ftl_leb_read(struct mt_ftl_blk *dev, int lnum, void *buf, int offset, int len)
{
	int ret = MT_FTL_SUCCESS;
	struct ubi_volume_desc *desc = dev->desc;

	ret = ubi_leb_read(desc, lnum, (char *)buf, offset, len, 0);
	if (ret == UBI_IO_BITFLIPS) {
		ret = MT_FTL_SUCCESS;
	} else if (ret) {
		mt_ftl_err(dev, "Read failed:leb=%u,peb=%u,offset=%d ret=%d", lnum, desc->vol->eba_tbl[lnum],
			offset, ret);
		mt_ftl_show_param(dev);
		ret = MT_FTL_FAIL;
		ubi_assert(false);
	}
	cond_resched();
	return ret;
}
static int mt_ftl_leb_write(struct mt_ftl_blk *dev, int lnum, void *buf, int offset, int len)
{
	int ret = MT_FTL_SUCCESS;
	struct ubi_volume_desc *desc = dev->desc;

retry:
	ret = ubi_leb_write(desc, lnum, buf, offset, len);
	if (ret) {
		if (ret == -EROFS) {
			mt_ftl_err(dev, "RO Volume, switch to RW");
			ubi_close_volume(dev->desc);
			dev->desc = ubi_open_volume(dev->ubi_num, dev->vol_id, UBI_READWRITE);
			desc = dev->desc;
			goto retry;
		}
		mt_ftl_err(dev, "Write failed:leb=%u,offset=%d ret=%d", lnum, offset, ret);
		mt_ftl_show_param(dev);
		ret = MT_FTL_FAIL;
		ubi_assert(false);
	}
	/* cond_resched(); */
	return ret;
}
#endif
static int mt_ftl_leb_recover(struct mt_ftl_blk *dev, int lnum, int size, int check)
{
	int ret = MT_FTL_FAIL, offset = size;
	unsigned char *tmpbuf = NULL;
	struct mt_ftl_param *param = dev->param;

	if (size == 0)  {
		mt_ftl_err(dev, "size(%d)==0", size);
		mt_ftl_leb_remap(dev, lnum);
		return MT_FTL_SUCCESS;
	}
	if (size >= dev->leb_size) {
		mt_ftl_err(dev, "size(%d)>=leb_size(%d)", size, dev->leb_size);
		return MT_FTL_SUCCESS;
	}
	if (check) {
		ret = mt_ftl_leb_read(dev, lnum, param->replay_page_buffer, offset, dev->min_io_size);
		if (ret == MT_FTL_SUCCESS) {
			offset += dev->min_io_size;
			if (offset >= dev->leb_size) {
				mt_ftl_err(dev, "offset(%d) over leb_size(%d)", offset, dev->leb_size);
				return ret;
			}
			mt_ftl_err(dev, "lnum(%d) read offset(%d) OK, read next(%d) for check", lnum, size, offset);
			ret = mt_ftl_leb_read(dev, lnum, param->replay_page_buffer, offset, dev->min_io_size);
		}
	}
	if (ret == MT_FTL_FAIL) {
		mt_ftl_err(dev, "recover leb(%d) size(%d)", lnum, size);
		tmpbuf = vmalloc(dev->leb_size);
		if (!tmpbuf) {
			ubi_assert(false);
			ret = MT_FTL_FAIL;
			goto out;
		}
		ret = mt_ftl_leb_read(dev, lnum, tmpbuf, 0, size);
		if (ret)
			goto out;
		mt_ftl_leb_remap(dev, lnum);
		ret = mt_ftl_leb_write(dev, lnum, tmpbuf, 0, size);
		if (ret)
			goto out;
		ret = 1; /* recover ok */
	}
out:
	if (tmpbuf) {
		vfree(tmpbuf);
		tmpbuf = NULL;
	}
	return ret;
}

static int mt_ftl_check_empty(const u8 *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (buf[i] != 0xFF)
			return 0;
	}
	return 1;
}

static int mt_ftl_leb_lastpage_offset(struct mt_ftl_blk *dev, int leb, int page)
{
	int ret = MT_FTL_SUCCESS;
	int offset = page * dev->min_io_size;
	struct mt_ftl_param *param = dev->param;
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;

	while (offset <= max_offset_per_block) {
		ret = mt_ftl_leb_read(dev, leb, param->tmp_page_buffer, offset, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		if (param->tmp_page_buffer[0] == 0xFFFFFFFF) {
			/* mt_ftl_err(dev, "[INFO] Get last page in leb:%d, page:%d", leb, offset / NAND_PAGE_SIZE); */
			break;
		}
		offset += dev->min_io_size;
	}
	return offset;
}

/* des sort */
static void mt_ftl_bit_sort(struct mt_ftl_blk *dev, int *arr, int start, int len)
{
	int i, j;
	int temp = 0;
	struct mt_ftl_param *param = dev->param;

	for (i = 0; i < len; i++)
		arr[i] = start + i;
	for (i = 1; i < len; i++) {
		temp = arr[i];
		for (j = i - 1; j >= 0 && (param->u4BIT[arr[j]] < param->u4BIT[temp]); j--)
			arr[j + 1] = arr[j];
		arr[j + 1] = temp;
	}
}

static int mt_ftl_get_move_num(struct mt_ftl_blk *dev, int *mv_arr, int start, int len)
{
	int i = 0, j = 0, ret = 0;
	int *arr = NULL;
	int valid = 0;
	struct mt_ftl_param *param = dev->param;
	int threshold = dev->leb_size / MT_CHARGE_THRESHOLD;

	arr = param->tmp_page_buffer;
	mt_ftl_bit_sort(dev, arr, start, len);
	if (arr[0] == param->gc_pmt_charge_blk) {
		threshold <<= 1;
		if (param->u4BIT[arr[1]] >= threshold)
			goto check_move;
		return 0;
	}
	valid = dev->leb_size - param->u4BIT[arr[0]];
	mv_arr[j++] = arr[0];
check_move:
	for (i = 1; i < PMT_CHARGE_NUM; i++) {
		if (arr[i] == param->gc_pmt_charge_blk)
			continue;
		valid += (dev->leb_size - param->u4BIT[arr[i]]);
		mv_arr[j++] = arr[i];
		if (valid >= dev->leb_size) {
			ret = i + 1;
			if (param->u4BIT[arr[i]] < threshold) {
				mt_ftl_err(dev, "bit balance arr[%d]=%d, blk=%d",
					i, arr[i], param->gc_pmt_charge_blk);
				ret = 0;
			}
			break;
		}
		threshold >>= 1;
	}
	return ret;
}

static int mt_ftl_gc_findblock(struct mt_ftl_blk *dev, int start_leb, int end_leb, bool ispmt, bool *invalid)
{
	int i = 0;
	struct mt_ftl_param *param = dev->param;
	int return_leb = MT_INVALID_BLOCKPAGE;
	int max_invalid = 0, skip_leb = param->u4GCReserveLeb;
	int total_invalid = dev->leb_size - (dev->leb_size / dev->min_io_size * 4);

	if (start_leb > dev->dev_blocks || end_leb > dev->dev_blocks) {
		mt_ftl_err(dev, "u4StartLeb(%d)u4EndLeb(%d) is larger than NAND_TOTAL_BLOCK_NUM(%d)",
			start_leb, end_leb, dev->dev_blocks);
		return MT_FTL_FAIL;
	}
	if (ispmt) {
		total_invalid = dev->leb_size;
		skip_leb = param->u4PMTChargeLebIndicator;
	}
	for (i = start_leb; i < end_leb; i++) {
		if (i == skip_leb)
			continue;
		if (param->u4BIT[i] >= total_invalid) {
			return_leb = i;
			*invalid = true;
			mt_ftl_debug(dev, "u4BIT[%d] = %d", return_leb, param->u4BIT[return_leb]);
			return return_leb;
		}
		if (param->u4BIT[i] > max_invalid) {
			max_invalid = param->u4BIT[i];
			return_leb = i;
			mt_ftl_debug(dev, "BIT[%d] = %d(%d:%d)", return_leb, param->u4BIT[return_leb],
				start_leb, end_leb);
		}
	}

	*invalid = false;
	return return_leb;
}


static bool mt_ftl_check_replay_pmt(struct mt_ftl_blk *dev, int sleb, int dleb)
{
	int ret;
	int pmt_len = dev->min_io_size << 1;
	struct mt_ftl_param *param = dev->param;

	if (param->last_replay_flag == REPLAY_FREE_BLK) {
		ret = mt_ftl_leb_map(dev, sleb);
		if (ret > 0) {
			ret = mt_ftl_leb_read(dev, sleb, param->gc_pmt_buffer, 0, pmt_len);
			if (ret)
				return false; /*error find redo the block*/
			ret = mt_ftl_check_empty((char *)param->gc_pmt_buffer, pmt_len);
			if (ret == 0) {
				mt_ftl_err(dev, "[Bean]src_leb(%d) not empty, need to do", sleb);
				mt_ftl_leb_remap(dev, dleb);
				param->last_replay_flag = REPLAY_CLEAN_BLK;
				return false;
			}
			param->last_replay_flag = REPLAY_END;
		} else
			param->last_replay_flag = REPLAY_END;
	}
	return true;
}

/* The process for PMT should traverse PMT indicator
 * if block num of items in PMT indicator is equal to u4SrcInvalidLeb, copy the
 * corresponding page to new block, and update corresponding PMT indicator */
static int mt_ftl_gc_pmt(struct mt_ftl_blk *dev, int sleb, int dleb, int *offset_dst, bool replay)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int offset_src = 0, dirty = 0;
	int page = 0, cache_num = 0;
	int count = 0, move_num = 0;
	int tmp_offset = *offset_dst;
	int pmt_len = dev->min_io_size << 1;
	int mv_arr[PMT_CHARGE_NUM] = {0};
	struct mt_ftl_param *param = dev->param;

	if (replay)
		replay = mt_ftl_check_replay_pmt(dev, sleb, dleb);

	if (!replay)
		mt_ftl_leb_remap(dev, dleb);
move_pmt:
	/* use tmp_page_buffer replace of gc_page_buffer, because gc data would update PMT & gc pmt */
	for (i = 0; i < PMT_TOTAL_CLUSTER_NUM; i++) {
		if (PMT_INDICATOR_GET_BLOCK(param->u4PMTIndicator[i]) == sleb) {
			offset_src = PMT_INDICATOR_GET_PAGE(param->u4PMTIndicator[i]) * dev->min_io_size;
			if (!replay) {
				/* Copy PMT & META PMT */
				mt_ftl_debug(dev, "Copy PMT&META PMT %d %d %d %d", sleb, offset_src, dleb, tmp_offset);
				ret = mt_ftl_leb_read(dev, sleb, param->gc_pmt_buffer, offset_src, pmt_len);
				if (ret)
					return ret;
				ret = mt_ftl_leb_write(dev, dleb, param->gc_pmt_buffer, tmp_offset, pmt_len);
				if (ret)
					return ret;
			}
			BIT_UPDATE(param->u4BIT[sleb], pmt_len);
			dirty = PMT_INDICATOR_IS_DIRTY(param->u4PMTIndicator[i]);
			cache_num = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[i]);
			page = tmp_offset / dev->min_io_size;
			PMT_INDICATOR_SET_BLOCKPAGE(param->u4PMTIndicator[i], dleb, page, dirty, cache_num);
			mt_ftl_debug(dev, "u4PMTIndicator[%d] = 0x%x, dirty = %d, cache_num = %d, page = %d %d %d\n",
				i, param->u4PMTIndicator[i], dirty, cache_num, page, ret, pmt_len);	/* Temporary */
			tmp_offset += pmt_len;
			if (tmp_offset >= dev->leb_size) {
				mt_ftl_debug(dev, "tmp_offset(%d) >= dev->leb_size", tmp_offset);
				break;
			}
		}
	}

	if (count == 0) {
		param->u4BIT[sleb] = 0;
		*offset_dst = tmp_offset;
		move_num = mt_ftl_get_move_num(dev, mv_arr, PMT_START_BLOCK, PMT_BLOCK_NUM);
		if (move_num == 0) {
			mt_ftl_err(dev, "NO_MOVE");
			return ret;
		}
		tmp_offset = 0;
		if (!replay)
			mt_ftl_leb_remap(dev, param->u4PMTChargeLebIndicator);
		mt_ftl_debug(dev, "retry=0, offset_dst = %d, sleb = %d", *offset_dst, sleb);
	}
	if (count < PMT_CHARGE_NUM) {
		sleb = mv_arr[count++];
		dleb = param->u4PMTChargeLebIndicator;
		mt_ftl_debug(dev, "sleb=%d, dleb=%d", sleb, dleb);
		if (sleb) {
			mt_ftl_err(dev, "move begin, tmp_offset=%d, count=%d", tmp_offset, count);
			goto move_pmt;
		}
	}
	param->gc_pmt_charge_blk = param->u4PMTChargeLebIndicator;
	param->u4PMTChargeLebIndicator = mv_arr[0];
	param->u4BIT[param->u4PMTChargeLebIndicator] = 0;
	mt_ftl_err(dev, "u4PMTChargeLebIndicator = %d, count=%d, gc_pmt_charge_blk = %d",
		param->u4PMTChargeLebIndicator, count, param->gc_pmt_charge_blk);
	mt_ftl_debug(dev, "BIT[3]=%d, BIT[4]=%d, BIT[5]=%d, BIT[6]=%d, BIT[7]=%d, BIT[8]=%d", param->u4BIT[3],
	param->u4BIT[4], param->u4BIT[5], param->u4BIT[6], param->u4BIT[7], param->u4BIT[8]);
	return ret;
}

static int mt_ftl_check_header(struct mt_ftl_blk *dev, struct mt_ftl_data_header *header, int num)
{
	int i;
	sector_t sec = NAND_DEFAULT_SECTOR_VALUE;

	for (i = 0; i < num; i++)
		sec &= header[i].sector;

	if (sec == NAND_DEFAULT_SECTOR_VALUE) {
		mt_ftl_debug(dev, "all sectors are 0xFFFFFFFF");
		for (i = 0; i < num; i++)
			mt_ftl_err(dev, "header_buffer[%d].sector = 0x%lx", i, (unsigned long int)header[i].sector);
		return 0;
	}
	return 1;
}

static int mt_ftl_get_pmt(struct mt_ftl_blk *dev, sector_t sector, u32 *pmt, u32 *meta_pmt)
{
	int ret = MT_FTL_SUCCESS;
	u32 cluster = 0, sec_offset = 0;
	int cache_num = 0;
	int pm_per_page = dev->pm_per_io;
	int page_size = dev->min_io_size;
	int pmt_block = 0, pmt_page = 0;
	struct mt_ftl_param *param = dev->param;

	/* Calculate clusters and sec_offsets */
	cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> pm_per_page);
	sec_offset = (u32)(FS_PAGE_OF_SECTOR(sector) & ((1 << pm_per_page) - 1));
	ubi_assert(cluster <= PMT_TOTAL_CLUSTER_NUM);
	/* Download PMT to read PMT cache */
	/* Don't use mt_ftl_updatePMT, that will cause PMT indicator mixed in replay */
	if (PMT_INDICATOR_IS_INCACHE(param->u4PMTIndicator[cluster])) {
		cache_num = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[cluster]);
		ubi_assert(cache_num < PMT_CACHE_NUM);
		*pmt = param->u4PMTCache[(cache_num << pm_per_page) + sec_offset];
		*meta_pmt = param->u4MetaPMTCache[(cache_num << pm_per_page) + sec_offset];
	} else if (cluster == param->i4CurrentReadPMTClusterInCache) {
		/* mt_ftl_err(dev,  "cluster == i4CurrentReadPMTClusterInCache (%d)",
		   param->i4CurrentReadPMTClusterInCache); */
		*pmt = param->u4ReadPMTCache[sec_offset];
		*meta_pmt = param->u4ReadMetaPMTCache[sec_offset];
	} else {
		pmt_block = PMT_INDICATOR_GET_BLOCK(param->u4PMTIndicator[cluster]);
		pmt_page = PMT_INDICATOR_GET_PAGE(param->u4PMTIndicator[cluster]);

		if (unlikely(pmt_block == 0) ||
			unlikely(pmt_block == PMT_INDICATOR_GET_BLOCK(NAND_DEFAULT_VALUE))) {
			mt_ftl_debug(dev, "pmt_block == 0");
			memset(param->u4ReadPMTCache, 0xFF, sizeof(unsigned int) << pm_per_page);
			memset(param->u4ReadMetaPMTCache, 0xFF, sizeof(unsigned int) << pm_per_page);
			param->i4CurrentReadPMTClusterInCache = NAND_DEFAULT_VALUE;
		} else {
			mt_ftl_debug(dev, "Get PMT of cluster (%d)", cluster);
			ret = mt_ftl_leb_read(dev, pmt_block, param->u4ReadPMTCache,
				pmt_page * page_size, page_size);
			if (ret)
				goto out;
			ret = mt_ftl_leb_read(dev, pmt_block, param->u4ReadMetaPMTCache,
					(pmt_page + 1) * page_size, page_size);
			if (ret)
				goto out;
			param->i4CurrentReadPMTClusterInCache = cluster;
		}
		*pmt = param->u4ReadPMTCache[sec_offset];
		*meta_pmt = param->u4ReadMetaPMTCache[sec_offset];
	}

	if (*pmt == MT_INVALID_BLOCKPAGE) {
		mt_ftl_err(dev, "PMT of sector(0x%lx) is invalid", (unsigned long int)sector);
		ret = MT_FTL_FAIL;
	}
out:
	return ret;
}

static int mt_ftl_gc_check_data(struct mt_ftl_blk *dev, struct mt_ftl_data_header *header_buffer,
	struct mt_ftl_valid_data *valid_data, int data_num, int *last_data_len, bool replay)
{
	int i = 0, ret = MT_FTL_SUCCESS;
	int last_data_len_src = 0;
	u32 pmt = 0, meta_pmt = 0;
	sector_t sector = 0;
	u32 total_consumed_size = 0;
	u32 offset_in_page = 0, data_len = 0;
	int page_size = dev->min_io_size;
	struct mt_ftl_data_header valid_header;
	u32 cluster = 0, sec_offset = 0, check_num = 0;
	struct mt_ftl_param *param = dev->param;
	char *last_data_buffer = NULL;

	last_data_buffer = vmalloc(FS_PAGE_SIZE);
	if (!last_data_buffer) {
		ret = MT_FTL_FAIL;
		ubi_assert(false);
		goto out;
	}
	for (i = 0; i < data_num; i++) {
		/* Get sector in the page */
		sector = header_buffer[data_num - i - 1].sector;
		if ((sector & NAND_DEFAULT_SECTOR_VALUE) == NAND_DEFAULT_SECTOR_VALUE)
			continue;
		ret = mt_ftl_get_pmt(dev, sector, &pmt, &meta_pmt);
		if (ret)
			goto out;
		cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> dev->pm_per_io);
		sec_offset = (u32)(FS_PAGE_OF_SECTOR(sector) & ((1 << dev->pm_per_io) - 1));
		ubi_assert(cluster <= PMT_TOTAL_CLUSTER_NUM);
		if ((!replay) &&
			(valid_data->info.src_leb == PMT_GET_BLOCK(pmt)) &&
			(valid_data->info.page_src == PMT_GET_PAGE(pmt)) &&
			(i == PMT_GET_PART(meta_pmt))) {
			offset_in_page = (header_buffer[data_num - i - 1].offset_len >> 16) & 0xFFFF;
			data_len = header_buffer[data_num - i - 1].offset_len & 0xFFFF;
			ubi_assert((data_len <= FS_PAGE_SIZE) && (offset_in_page < page_size));
			last_data_len_src = 0;
			total_consumed_size = valid_data->valid_data_offset + data_len +
				(valid_data->valid_data_num + 1) * sizeof(struct mt_ftl_data_header) + 4;
			if ((total_consumed_size > page_size) ||
					valid_data->valid_data_num >= DATA_NUM_PER_PAGE) {
				/* dump_valid_data(dev, valid_data); */
				if ((offset_in_page + data_len +
					data_num * sizeof(struct mt_ftl_data_header) + 4) > page_size) {
					last_data_len_src = page_size -
						data_num * sizeof(struct mt_ftl_data_header) - offset_in_page - 4;
					memcpy(last_data_buffer,
						(char *)param->gc_page_buffer + offset_in_page, last_data_len_src);
					ret = mt_ftl_leb_read(dev, valid_data->info.src_leb,
						param->tmp_page_buffer, valid_data->info.offset_src + page_size,
						page_size);
					if (ret)
						goto out;
					memcpy((char *)last_data_buffer + last_data_len_src,
						(char *)param->tmp_page_buffer, (data_len - last_data_len_src));
					check_num =
						PAGE_GET_DATA_NUM(param->tmp_page_buffer[(page_size >> 2) - 1]);
					if (check_num == 0x3FFFFFFF) {
						valid_data->info.page_src += 1;
						valid_data->info.offset_src += page_size;
					}
					/* offset_in_page = 0; */
				}
				*last_data_len = page_size - (valid_data->valid_data_num + 1) *
					sizeof(struct mt_ftl_data_header) - valid_data->valid_data_offset - 4;
				if (*last_data_len > 0) {
					if (last_data_len_src)
						memcpy(&valid_data->valid_buffer[valid_data->valid_data_offset],
							last_data_buffer, *last_data_len);
					else
						memcpy(&valid_data->valid_buffer[valid_data->valid_data_offset],
							(char *)param->gc_page_buffer + offset_in_page,
							*last_data_len);
					valid_header.offset_len = (valid_data->valid_data_offset << 16) | data_len;
					valid_header.sector = sector;
					valid_data->valid_header_offset -= sizeof(struct mt_ftl_data_header);
					mt_ftl_debug(dev, "valid_header_offset: %d", valid_data->valid_header_offset);
					memcpy(&valid_data->valid_buffer[valid_data->valid_header_offset],
						&valid_header, sizeof(struct mt_ftl_data_header));
					valid_data->valid_data_num++;
					memcpy(&valid_data->valid_buffer[page_size - 4],
						&valid_data->valid_data_num, 4);
					mt_ftl_updatePMT(dev, cluster, sec_offset, valid_data->info.dst_leb,
						valid_data->info.page_dst * page_size,
						valid_data->valid_data_num - 1, data_len, replay, false);
				}
				mt_ftl_debug(dev, "leb:%doffset:%d", valid_data->info.dst_leb,
					valid_data->info.offset_dst);
				PAGE_SET_READ(*(u32 *)(valid_data->valid_buffer + page_size - 4));
				ret = mt_ftl_leb_write(dev, valid_data->info.dst_leb, valid_data->valid_buffer,
					valid_data->info.offset_dst, page_size);
				if (ret)
					goto out;
				if (*last_data_len <= 0) {
					BIT_UPDATE(param->u4BIT[valid_data->info.dst_leb],
						(*last_data_len + sizeof(struct mt_ftl_data_header)));
					*last_data_len = 0;
				}
				valid_data->valid_data_offset = 0;
				valid_data->valid_data_num = 0;
				memset(valid_data->valid_buffer, 0xFF, page_size * sizeof(char));
				valid_data->info.offset_dst += page_size;
				valid_data->info.page_dst += 1;
			}
			if (*last_data_len > 0) {
				valid_data->valid_data_offset = data_len - *last_data_len;
				if (last_data_len_src)
					memcpy(valid_data->valid_buffer,
						(char *)last_data_buffer + *last_data_len,
						valid_data->valid_data_offset);
				else
					memcpy(valid_data->valid_buffer, (char *)param->gc_page_buffer +
						offset_in_page + *last_data_len, valid_data->valid_data_offset);
				*last_data_len = 0;
			} else {
				mt_ftl_updatePMT(dev, cluster, sec_offset, valid_data->info.dst_leb,
						 valid_data->info.page_dst * page_size,
						 valid_data->valid_data_num, data_len, replay, false);
				valid_header.offset_len = (valid_data->valid_data_offset << 16) | data_len;
				valid_header.sector = sector;
				total_consumed_size = offset_in_page + data_len +
					data_num  * sizeof(struct mt_ftl_data_header) + 4;
				if (total_consumed_size > page_size) {
					last_data_len_src = page_size - offset_in_page -
						(data_num * sizeof(struct mt_ftl_data_header) + 4);
					memcpy(&valid_data->valid_buffer[valid_data->valid_data_offset],
						(char *)param->gc_page_buffer + offset_in_page, last_data_len_src);
					ret = mt_ftl_leb_read(dev, valid_data->info.src_leb,
						param->tmp_page_buffer,
						valid_data->info.offset_src + page_size, page_size);
					if (ret)
						goto out;
					memcpy(&valid_data->valid_buffer[valid_data->valid_data_offset +
						last_data_len_src], (char *)param->tmp_page_buffer,
						data_len - last_data_len_src);
					check_num =
						PAGE_GET_DATA_NUM(param->tmp_page_buffer[(page_size >> 2) - 1]);
					if (check_num == 0x3FFFFFFF) {
						valid_data->info.page_src += 1;
						valid_data->info.offset_src += page_size;
					}
				} else
					memcpy(&valid_data->valid_buffer[valid_data->valid_data_offset],
						(char *)param->gc_page_buffer + offset_in_page,
						data_len);
				valid_data->valid_data_offset += data_len;
				valid_data->valid_header_offset = page_size -
					(valid_data->valid_data_num + 1) * sizeof(struct mt_ftl_data_header) - 4;
				memcpy(&valid_data->valid_buffer[valid_data->valid_header_offset],
						&valid_header, sizeof(struct mt_ftl_data_header));
				valid_data->valid_data_num += 1;
				memcpy(&valid_data->valid_buffer[page_size - 4], &valid_data->valid_data_num, 4);
			}
		}
		if (replay)
			mt_ftl_updatePMT(dev, cluster, sec_offset, valid_data->info.dst_leb,
					 valid_data->info.page_dst * page_size, i,
					 (header_buffer[data_num - i - 1].offset_len & 0xFFFF), replay, false);
		/* cond_resched(); */
	}
	if (replay) {
		data_len = header_buffer[0].offset_len & 0xFFFF;
		offset_in_page = (header_buffer[0].offset_len >> 16) & 0xFFFF;
		total_consumed_size = data_len + offset_in_page + data_num * sizeof(struct mt_ftl_data_header) + 4;
		if (total_consumed_size > page_size)
			*last_data_len = total_consumed_size - page_size;
		else if (total_consumed_size < page_size)
			BIT_UPDATE(param->u4BIT[valid_data->info.dst_leb], (page_size - total_consumed_size));
	}
out:
	if (last_data_buffer) {
		vfree(last_data_buffer);
		last_data_buffer = NULL;
	}
	return ret;
}

/* The process for data is to get all the sectors in the source block
 * and compare to PMT, if the corresponding block/page/part in PMT is
 * the same as source block/page/part, then current page should be copied
 * destination block, and then update PMT
 */
static int mt_ftl_gc_data(struct mt_ftl_blk *dev, int sleb, int dleb, int *offset_dst, bool replay)
{
	int ret = MT_FTL_SUCCESS;
	u32 data_num = 0, retry = 0;
	u32 data_hdr_offset = 0;
	u32 page_been_read = 0;
	struct mt_ftl_data_header *header_buffer = NULL;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_valid_data valid_data;
	int head_data = 0, last_data_len;
	u32 total_consumed_size = 0;
	int page_size = dev->min_io_size;
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;

	valid_data.info.offset_src = 0;
	valid_data.info.offset_dst = 0;
	valid_data.info.src_leb = sleb;
	valid_data.info.dst_leb = dleb;
	valid_data.info.page_src = 0;
	valid_data.info.page_dst = 0;
	valid_data.valid_data_num = 0;
	valid_data.valid_data_offset = 0;
	valid_data.valid_header_offset = 0;
	valid_data.valid_buffer = NULL;

	valid_data.valid_buffer = vmalloc(page_size);
	if (!valid_data.valid_buffer) {
		ret = MT_FTL_FAIL;
		ubi_assert(false);
		goto out_free;
	}
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_DATA_READOOB);
	if (!replay)
		mt_ftl_leb_remap(dev, dleb);
	if (dleb >= dev->dev_blocks) {
		mt_ftl_err(dev, "dest_leb(%d) over total block", dleb);
		ret = MT_FTL_FAIL;
		goto out_free;
	}
	if (!replay) {
		ret = mt_ftl_leb_read(dev, sleb, param->gc_page_buffer, valid_data.info.offset_src, page_size);
	} else {
		/* When replay, the data has been copied to dst, so we read dst instead of src
		 * but we still use offset_src to control flow */
		ret = mt_ftl_leb_read(dev, dleb, param->gc_page_buffer, valid_data.info.offset_src, page_size);
	}
	if (ret)
		goto out_free;

	data_num = PAGE_GET_DATA_NUM(param->gc_page_buffer[(page_size >> 2) - 1]);
	if ((data_num == 0x7FFFFFFF) || (data_num == 0x3FFFFFFF)) {
		mt_ftl_err(dev, "End GC Data, offset_src=%d", valid_data.info.offset_src);
		goto out_free;
	}
	if (replay) {
		page_been_read = PAGE_BEEN_READ(param->gc_page_buffer[(page_size >> 2) - 1]);
		if (page_been_read == 0) {
			mt_ftl_err(dev, "End of replay GC Data, offset_src = %d", valid_data.info.offset_src);
			goto out_free;
		}
	}
	data_hdr_offset = page_size - (data_num * sizeof(struct mt_ftl_data_header) + 4);
	header_buffer = (struct mt_ftl_data_header *)(&param->gc_page_buffer[data_hdr_offset >> 2]);

	head_data = mt_ftl_check_header(dev, header_buffer, data_num);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_DATA_READOOB);

	while (head_data && (valid_data.info.offset_src <= max_offset_per_block)) {
		retry = 1;
		last_data_len = 0;
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_DATA_READ_UPDATE_PMT);
		ret = mt_ftl_gc_check_data(dev, header_buffer, &valid_data, data_num, &last_data_len, replay);
		if (ret)
			goto out_free;
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_DATA_READ_UPDATE_PMT);
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_DATA_READOOB);
retry_once:
		if (replay) {
			valid_data.info.offset_dst += page_size;
			valid_data.info.page_dst += 1;
		}

		valid_data.info.offset_src += page_size;
		valid_data.info.page_src += 1;
		if (valid_data.info.offset_src > max_offset_per_block)
			break;
		if (!replay)
			ret = mt_ftl_leb_read(dev, sleb, param->gc_page_buffer, valid_data.info.offset_src, page_size);
		else
			ret = mt_ftl_leb_read(dev, dleb, param->gc_page_buffer, valid_data.info.offset_src, page_size);

		if (ret)
			goto out_free;
		data_num = PAGE_GET_DATA_NUM(param->gc_page_buffer[(page_size >> 2) - 1]);
		if (data_num == 0x3FFFFFFF) {
			if (replay && last_data_len && retry) {
				BIT_UPDATE(param->u4BIT[dleb], (page_size - last_data_len - 4));
				last_data_len = 0;
				retry = 0;
				mt_ftl_debug(dev, "replay: data_num = 0x%x", data_num);
				goto retry_once;
			} else if (retry) {
				mt_ftl_debug(dev, "gc_data: data_num = 0x%x", data_num);
				retry = 0;
				goto retry_once;
			}
			mt_ftl_debug(dev, "break: data_num = 0x%x", data_num);
			break;
		}
		if (data_num == 0x7FFFFFFF) {
			mt_ftl_debug(dev, "break: data_num = 0x%x", data_num);
			break;
		}
		data_hdr_offset = page_size - (data_num * sizeof(struct mt_ftl_data_header) + 4);
		header_buffer = (struct mt_ftl_data_header *)(&param->gc_page_buffer[data_hdr_offset >> 2]);

		head_data = mt_ftl_check_header(dev, header_buffer, data_num);
		if (replay) {
			page_been_read = PAGE_BEEN_READ(param->gc_page_buffer[(page_size >> 2) - 1]);
			if (page_been_read == 0) {
				mt_ftl_err(dev, "End of replay GC Data, offset_src = %d", valid_data.info.offset_src);
				goto out_free;
			}
		}
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_DATA_READOOB);
	}
	if ((valid_data.valid_data_num || valid_data.valid_data_offset) && (!replay)) {
		if (valid_data.valid_data_num) {
			total_consumed_size = valid_data.valid_data_offset +
				valid_data.valid_data_num * sizeof(struct mt_ftl_data_header) + 4;
			PAGE_SET_READ(*(u32 *)(valid_data.valid_buffer + page_size - 4));
		} else {
			total_consumed_size = valid_data.valid_data_offset + 4;
			PAGE_SET_LAST_DATA(*(u32 *)(valid_data.valid_buffer + page_size - 4));
		}
		ret = mt_ftl_leb_write(dev, dleb, valid_data.valid_buffer, valid_data.info.offset_dst, page_size);
		if (ret)
			goto out_free;
		BIT_UPDATE(param->u4BIT[dleb], (page_size - total_consumed_size));
		valid_data.info.offset_dst += page_size;
		param->u4NextPageOffsetIndicator = 0;
		param->u4DataNum = 0;
		mt_ftl_debug(dev, "gc finished, valid_data_num=%d", valid_data.valid_data_num);
	}
out_free:
	*offset_dst = valid_data.info.offset_dst;
	if (valid_data.valid_buffer) {
		vfree(valid_data.valid_buffer);
		valid_data.valid_buffer = NULL;
	}
	mt_ftl_debug(dev, "u4BIT[%d] = %d", dleb, param->u4BIT[dleb]);

	return ret;
}

static int mt_ftl_check_replay_commit(struct mt_ftl_blk *dev, int leb, bool ispmt)
{
	int i, commit = 0, data_leb = 0;
	unsigned int *rec = NULL, index = 0;
	struct mt_ftl_param *param = dev->param;

	index = ispmt ? param->replay_pmt_blk_index : param->replay_blk_index;
	rec = ispmt ? param->replay_pmt_blk_rec : param->replay_blk_rec;
	for (i = 0; i < index; i++) {
		if (leb == rec[i]) {
			mt_ftl_err(dev, "u4SrcInvalidLeb(%d)==replay_blk_rec[%d](%d)", leb, i, rec[i]);
			if (!ispmt) {
				data_leb = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4NextLebPageIndicator);
				PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(param->u4NextLebPageIndicator, leb,
					dev->leb_size / dev->min_io_size);
			}
			commit = 1;
			break;
		}
	}
	return commit;
}

static int mt_ftl_write_replay_blk(struct mt_ftl_blk *dev, int leb, int *page, bool replay)
{
	int ret = MT_FTL_SUCCESS, i;
	int replay_blk, replay_offset;
	struct mt_ftl_param *param = dev->param;
	const int max_replay_offset = dev->max_replay_blks * dev->min_io_size;

	if (!replay) {
		if (param->u4NextReplayOffsetIndicator == 0) {
			for (i = 0; i < REPLAY_BLOCK_NUM; i++)
				mt_ftl_leb_remap(dev, REPLAY_BLOCK + i);
		}
		replay_blk = param->u4NextReplayOffsetIndicator / dev->leb_size + REPLAY_BLOCK;
		replay_offset = param->u4NextReplayOffsetIndicator % dev->leb_size;
		param->replay_page_buffer[0] = MT_MAGIC_NUMBER;
		param->replay_page_buffer[1] = leb;
		mt_ftl_err(dev, "u4NextReplayOffsetIndicator=%d(%d) %d",
			param->u4NextReplayOffsetIndicator, leb, param->replay_blk_index);
		ret = mt_ftl_leb_write(dev, replay_blk, param->replay_page_buffer,
			replay_offset, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		param->u4NextReplayOffsetIndicator += dev->min_io_size;
		param->replay_blk_rec[param->replay_blk_index] = leb;
		param->replay_blk_index++;
		if (param->u4NextReplayOffsetIndicator == max_replay_offset) {
			param->u4NextReplayOffsetIndicator = 0;
			PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(param->u4NextLebPageIndicator, leb, *page);
			mt_ftl_commit(dev);
			*page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(param->u4NextLebPageIndicator);
		}
	} else {
		param->u4NextReplayOffsetIndicator += dev->min_io_size;
		param->replay_blk_rec[param->replay_blk_index] = leb;
		param->replay_blk_index++;
		if (param->u4NextReplayOffsetIndicator == max_replay_offset) {
			param->u4NextReplayOffsetIndicator = 0;
			memset(param->replay_blk_rec, 0xFF, dev->max_replay_blks * sizeof(unsigned int));
			param->replay_blk_index = 0;
		}
	}
	return ret;
}

static int mt_ftl_gc(struct mt_ftl_blk *dev, int *updated_page, bool ispmt, bool replay)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int src_leb = MT_INVALID_BLOCKPAGE;
	int return_leb = 0, offset_dst = 0;
	bool invalid = false;
	int start_leb = DATA_START_BLOCK, end_leb = dev->dev_blocks;
	struct mt_ftl_param *param = dev->param;
	const int page_num_per_block = dev->leb_size / dev->min_io_size;

	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_FINDBLK);
	/* Get the most invalid block */
	if (ispmt) {
		start_leb = PMT_START_BLOCK;
		end_leb = DATA_START_BLOCK;
	}
	src_leb = mt_ftl_gc_findblock(dev, start_leb, end_leb, ispmt, &invalid);
	if (!replay) {
		ret = mt_ftl_check_replay_commit(dev, src_leb, ispmt);
		if (ret) {
			mt_ftl_debug(dev, "commit src_leb(%d) == replay_blk_rec", src_leb);
			if (ispmt)
				param->get_pmt_blk_flag = 1;
			else
				mt_ftl_commit(dev);
		}
	}
	if (invalid) {
		if (!replay) {
			if (ispmt)
				mt_ftl_leb_remap(dev, param->u4GCReservePMTLeb);
			else
				mt_ftl_leb_remap(dev, param->u4GCReserveLeb);
		}
		goto gc_end;
	}

	if (((u32)src_leb) == MT_INVALID_BLOCKPAGE) {
		mt_ftl_err(dev, "cannot find block for GC, isPMT = %d", ispmt);
		return MT_FTL_FAIL;
	}

	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_FINDBLK);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_REMAP);

	/* Call sub function for pmt/data */
	if (ispmt)
		ret = mt_ftl_gc_pmt(dev, src_leb, param->u4GCReservePMTLeb, &offset_dst, replay);
	else {
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_DATA_READ_UPDATE_PMT);
		param->gc_dat_cmit_flag = 1;
		ret = mt_ftl_gc_data(dev, src_leb, param->u4GCReserveLeb, &offset_dst, replay);
		param->gc_dat_cmit_flag = 0;
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_DATA_READ_UPDATE_PMT);
	}
	/* PMT not in PMTBlock but dirty, ret == 1 is u4SrcInvalidLeb mapped*/
	if (ispmt && (ret == 1))
		goto gc_end;
	else if (ret) {
		mt_ftl_err(dev, "GC sub function failed, u4SrcInvalidLeb=%d, offset_dst=%d, ret=%d",
			   src_leb, offset_dst, ret);
		return MT_FTL_FAIL;
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_CPVALID);
gc_end:
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_REMAP);

	/* TODO: Use this information for replay, instead of check MT_PAGE_HAD_BEEN_READ */
	*updated_page = offset_dst / dev->min_io_size;
	if (*updated_page == page_num_per_block) {
		mt_ftl_err(dev, "There is no more free pages in the gathered block");
		mt_ftl_err(dev, "desc->vol->ubi->volumes[%d]->reserved_pebs = %d",
			   dev->vol_id, dev->dev_blocks);
		for (i = PMT_START_BLOCK; i < dev->dev_blocks; i += 8) {
			mt_ftl_err(dev, "%d\t %d\t %d\t %d\t %d\t %d\t %d\t %d", param->u4BIT[i],
				   param->u4BIT[i + 1], param->u4BIT[i + 2], param->u4BIT[i + 3],
				   param->u4BIT[i + 4], param->u4BIT[i + 5], param->u4BIT[i + 6],
				   param->u4BIT[i + 7]);
		}
		return MT_FTL_FAIL;
	}

	if (ispmt) {
		return_leb = param->u4GCReservePMTLeb;
		param->u4GCReservePMTLeb = src_leb;
		mt_ftl_debug(dev, "u4GCReservePMTLeb = %d, *updated_page = %d, u4GCReserveLeb = %d",
				param->u4GCReservePMTLeb, *updated_page, param->u4GCReserveLeb);
		param->u4BIT[param->u4GCReservePMTLeb] = 0;
		mt_ftl_debug(dev, "u4BIT[%d] = %d", param->u4GCReservePMTLeb, param->u4BIT[param->u4GCReservePMTLeb]);
	} else {
		return_leb = param->u4GCReserveLeb;
		param->u4GCReserveLeb = src_leb;
		mt_ftl_debug(dev, "u4GCReserveLeb = %d, *updated_page = %d", param->u4GCReserveLeb, *updated_page);
		param->u4BIT[param->u4GCReserveLeb] = 0;
		mt_ftl_debug(dev, "u4BIT[%d] = %d", param->u4GCReserveLeb, param->u4BIT[param->u4GCReserveLeb]);
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_REMAP);
	return return_leb;
}



/* TODO: Add new block to replay block, but have to consider the impact of original valid page to replay */
static int mt_ftl_getfreeblock(struct mt_ftl_blk *dev, int *updated_page, bool ispmt, bool replay)
{
	int ret = MT_FTL_SUCCESS;
	int free_leb = 0;
	struct mt_ftl_param *param = dev->param;

	if (ispmt) {
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_GETFREEBLOCK_PMT_GETLEB);
		if (param->u4NextFreePMTLebIndicator != MT_INVALID_BLOCKPAGE) {
			free_leb = param->u4NextFreePMTLebIndicator;
			if (!replay)
				mt_ftl_leb_map(dev, free_leb);
			*updated_page = 0;
			param->u4NextFreePMTLebIndicator++;
			mt_ftl_debug(dev, "u4NextFreePMTLebIndicator=%d", param->u4NextFreePMTLebIndicator);
			/* The PMT_BLOCK_NUM + 2 block is reserved to param->u4GCReserveLeb */
			if (param->u4NextFreePMTLebIndicator >= DATA_START_BLOCK - 2) {
				/* mt_ftl_err(dev,  "[INFO] u4NextFreePMTLebIndicator is in the end"); */
				param->u4NextFreePMTLebIndicator = MT_INVALID_BLOCKPAGE;
				mt_ftl_debug(dev, "u4NextFreePMTLebIndicator=%d", param->u4NextFreePMTLebIndicator);
			}
		} else
			free_leb = mt_ftl_gc(dev, updated_page, ispmt, replay);
		if (!replay) {
			param->replay_pmt_blk_rec[param->replay_pmt_blk_index] = free_leb;
			param->replay_pmt_blk_index++;
		}
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_GETFREEBLOCK_GETLEB);
		return free_leb;
	}
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GETFREEBLOCK_GETLEB);
	if (param->u4NextFreeLebIndicator != MT_INVALID_BLOCKPAGE) {
		free_leb = param->u4NextFreeLebIndicator;
		*updated_page = 0;
		if (!replay)
			mt_ftl_leb_map(dev, free_leb);
		param->u4NextFreeLebIndicator++;
		param->u4NextPageOffsetIndicator = 0;
		mt_ftl_debug(dev, "u4NextFreeLebIndicator=%d", param->u4NextFreeLebIndicator);
		/* The last block is reserved to param->u4GCReserveLeb */
		/* [Bean] Please reserved block for UBI WL 2 PEB*/
		if (param->u4NextFreeLebIndicator >= dev->dev_blocks - 1) {
			param->u4NextFreeLebIndicator = MT_INVALID_BLOCKPAGE;
			mt_ftl_debug(dev, "u4NextFreeLebIndicator=%d", param->u4NextFreeLebIndicator);
		}
	} else {
		if (param->pmt_updated_flag == PMT_FREE_BLK) {
			free_leb = mt_ftl_gc(dev, updated_page, ispmt, replay);
			param->pmt_updated_flag = PMT_FREE_BLK;
			mt_ftl_debug(dev, "pmt_updated_flag = PMT_FREE_BLK");
		} else
			free_leb = mt_ftl_gc(dev, updated_page, ispmt, replay);
	}

	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GETFREEBLOCK_GETLEB);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GETFREEBLOCK_PUTREPLAY_COMMIT);
	if (!replay) {
		if (param->pmt_updated_flag != PMT_FREE_BLK) {
			ret = mt_ftl_write_replay_blk(dev, free_leb, updated_page, replay);
			if (ret)
				return MT_FTL_FAIL;
		} else {
			param->pmt_updated_flag = PMT_COMMIT;
			mt_ftl_debug(dev, "pmt_updated_flag = PMT_COMMIT");
		}
	} else
		ret = mt_ftl_write_replay_blk(dev, free_leb, updated_page, replay);

	mt_ftl_debug(dev, "u4FreeLeb = %d, u4NextFreeLebIndicator = %d, isPMT = %d, updated_page = %d",
		   free_leb, param->u4NextFreeLebIndicator, ispmt, *updated_page);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GETFREEBLOCK_PUTREPLAY_COMMIT);
	return free_leb;
}

/* sync *page_buffer to the end of dst_leb */
static int mt_ftl_write_to_blk(struct mt_ftl_blk *dev, int dst_leb, u32 *page_buffer, int *offset)
{
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;

	mt_ftl_leb_map(dev, dst_leb);
	*offset = mt_ftl_leb_lastpage_offset(dev, dst_leb, 0);
	if (*offset > max_offset_per_block) {
		mt_ftl_leb_remap(dev, dst_leb);
		*offset = 0;
	}

	mt_ftl_debug(dev, "dst_leb=%d, offset=%d, page_buffer[0]=0x%x", dst_leb, offset, page_buffer[0]);
	return mt_ftl_leb_write(dev, dst_leb, page_buffer, *offset, dev->min_io_size);
}

int mt_ftl_write_page(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int leb = 0, page = 0, cache_num = 0;
	sector_t sector = 0;
	u32 cluster = 0, sec_offset = 0;
	struct mt_ftl_param *param = dev->param;
	const int page_num_per_block = dev->leb_size / dev->min_io_size;
	u32 data_hdr_offset = 0;
	int data_offset = 0, data_len = 0;
	int data_num_last = 0;

	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_PAGE_WRITEOOB);
	leb = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4NextLebPageIndicator);
	page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(param->u4NextLebPageIndicator);

	mt_ftl_debug(dev, "u4NextLebPageIndicator = 0x%x, leb = %d, page = %d",
		param->u4NextLebPageIndicator, leb, page);
	/* write the last data of page, but cannot write into the page, write to next */
	if ((param->u4DataNum == 0) && param->u4NextPageOffsetIndicator) {
		MT_FTL_WBUF_NOSYNC(dev);
		PAGE_SET_LAST_DATA(*(u32 *)(param->u1DataCache + dev->min_io_size - 4));
		data_num_last = PAGE_GET_DATA_NUM(*(u32 *)(param->u1DataCache + dev->min_io_size - 4));
		ret = mt_ftl_leb_write(dev, leb, param->u1DataCache, page * dev->min_io_size, dev->min_io_size);
		if (ret)
			return ret;
		BIT_UPDATE(param->u4BIT[leb], (dev->min_io_size - param->u4NextPageOffsetIndicator - 4));
		mt_ftl_debug(dev, "data_num==0, BIT[%d]=%d, page=%d, size=%d, data_num_last=0x%x",
			leb, param->u4BIT[leb], page,
			(dev->min_io_size - param->u4NextPageOffsetIndicator - 4), data_num_last);
		param->u4NextPageOffsetIndicator = 0;
		goto next_indicator;
	}
	data_hdr_offset = dev->min_io_size - (param->u4DataNum * sizeof(struct mt_ftl_data_header) + 4);
	memcpy(&param->u1DataCache[data_hdr_offset],
	       &param->u4Header[DATA_NUM_PER_PAGE - param->u4DataNum],
	       param->u4DataNum * sizeof(struct mt_ftl_data_header));
	memcpy(&param->u1DataCache[dev->min_io_size - 4], &param->u4DataNum, 4);
	MT_FTL_WBUF_NOSYNC(dev);
	ret = mt_ftl_leb_write(dev, leb, param->u1DataCache, page * dev->min_io_size, dev->min_io_size);
	if (ret)
		return ret;
	data_offset = (param->u4Header[DATA_NUM_PER_PAGE - param->u4DataNum].offset_len >> 16) & 0xFFFF;
	data_len = param->u4Header[DATA_NUM_PER_PAGE - param->u4DataNum].offset_len & 0xFFFF;
	if ((data_offset + data_len) < data_hdr_offset)
		BIT_UPDATE(param->u4BIT[leb], (data_hdr_offset - data_offset - data_len));
	param->u4NextPageOffsetIndicator = 0;
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_PAGE_WRITEOOB);
	for (i = DATA_NUM_PER_PAGE - 1; i >= DATA_NUM_PER_PAGE - param->u4DataNum; i--) {
		sector = param->u4Header[i].sector;
		cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> dev->pm_per_io);
		sec_offset = (u32)(FS_PAGE_OF_SECTOR(sector) & ((1 << dev->pm_per_io) - 1));
		ubi_assert(cluster <= PMT_TOTAL_CLUSTER_NUM);
		if (PMT_INDICATOR_IS_INCACHE(param->u4PMTIndicator[cluster])) {
			cache_num = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[cluster]);
			ubi_assert(cache_num < PMT_CACHE_NUM);
			PMT_RESET_DATA_INCACHE(param->u4MetaPMTCache[(cache_num << dev->pm_per_io) +
								     sec_offset]);
		}
		/* cond_resched(); */
	}
next_indicator:
	param->u4DataNum = 0;
	page++;
	if (page == page_num_per_block) {
		if (param->pmt_updated_flag == PMT_START) {
			param->pmt_updated_flag = PMT_FREE_BLK;
			mt_ftl_debug(dev, "pmt_updated_flag = PMT_FREE_BLK");
		}
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_PAGE_GETFREEBLK);
		leb = mt_ftl_getfreeblock(dev, &page, false, false);
		if (((u32)leb) == MT_INVALID_BLOCKPAGE) {
			mt_ftl_err(dev, "mt_ftl_getfreeblock failed");
			return MT_FTL_FAIL;
		}
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_PAGE_GETFREEBLK);
	}

	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_PAGE_RESET);
	memset(param->u1DataCache, 0xFF, dev->min_io_size * sizeof(unsigned char));
	memset(param->u4Header, 0xFF, DATA_NUM_PER_PAGE * sizeof(struct mt_ftl_data_header));

	PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(param->u4NextLebPageIndicator, leb, page);
	/*mt_ftl_err(dev,  "u4NextLebPageIndicator = 0x%x", param->u4NextLebPageIndicator);*/

	MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_PAGE_RESET);

	return ret;
}

static int mt_ftl_do_repay_pmt(struct mt_ftl_blk *dev, int block, int page)
{
	int ret;
	struct mt_ftl_param *param = dev->param;

	/* check replay conditions */
	switch (param->last_replay_flag) {
	case REPLAY_LAST_BLK:
		mt_ftl_err(dev, "case 1 REPLAY_LAST_BLK");
		if (param->recorded_pmt_blk != block) {
			ret = mt_ftl_leb_recover(dev, block, page * dev->min_io_size, 0);
			if (ret == MT_FTL_FAIL) {
				mt_ftl_err(dev, "[Bean]leb(%d) recover fail", block);
				return MT_FTL_FAIL;
			} else if (ret == 1) {
				param->recorded_pmt_blk = block;
				param->last_replay_flag = REPLAY_FREE_BLK;
			}
		}
		break;
	case REPLAY_CLEAN_BLK:
		mt_ftl_err(dev, "case 2 REPLAY_CLEAN_BLK");
		param->recorded_pmt_blk = block;
		break;
	default:
		break;
	}
	return MT_FTL_SUCCESS;
}

int mt_ftl_commitPMT(struct mt_ftl_blk *dev, bool replay, bool commit)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int pmt_block = 0, pmt_page = 0;
	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_param *param = dev->param;
	const int page_num_per_block = dev->leb_size / dev->min_io_size;

	MT_FTL_PROFILE_START(MT_FTL_PROFILE_COMMIT_PMT);
	if (((!replay) && commit && (param->u4DataNum != 0)) || param->u4NextPageOffsetIndicator) {
		mt_ftl_debug(dev, "commit PMT write page, u4NextPageOffsetIndicator=%d,data_num=%d",
			param->u4NextPageOffsetIndicator, param->u4DataNum);
		ret = mt_ftl_write_page(dev);
	}

	for (i = 0; i < PMT_CACHE_NUM; i++) {
		if (param->i4CurrentPMTClusterInCache[i] == 0xFFFFFFFF)
			continue;
		ubi_assert(param->i4CurrentPMTClusterInCache[i] <= PMT_TOTAL_CLUSTER_NUM);
		if (!PMT_INDICATOR_IS_INCACHE
		    (param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]])) {
			mt_ftl_err(dev, "i4CurrentPMTClusterInCache(%d) is not in cache",
				   param->i4CurrentPMTClusterInCache[i]);
			return MT_FTL_FAIL;
		}
		if (!PMT_INDICATOR_IS_DIRTY
		    (param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]])) {
			mt_ftl_debug(dev, "u4PMTIndicator[%d]=0x%x is not dirty", param->i4CurrentPMTClusterInCache[i],
				   param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
			/* clear i4CurrentPMTClusterInCache */
			PMT_INDICATOR_RESET_INCACHE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
			param->i4CurrentPMTClusterInCache[i] = 0xFFFFFFFF;
			continue;
		}
		/* Update param->u4BIT of the block that is originally in param->u4PMTCache */
		pmt_block = PMT_INDICATOR_GET_BLOCK(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		pmt_page = PMT_INDICATOR_GET_PAGE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		ubi_assert(pmt_block < dev->dev_blocks);
		if ((((u32)pmt_block) != MT_INVALID_BLOCKPAGE) && (pmt_block != 0)) {
			BIT_UPDATE(param->u4BIT[pmt_block], (dev->min_io_size * 2));
			mt_ftl_debug(dev, "u4BIT[%d] = %d", pmt_block, param->u4BIT[pmt_block]);
		}

		/* Calculate new block/page from param->u4CurrentPMTLebPageIndicator
		 * Update param->u4CurrentPMTLebPageIndicator
		 * and check the correctness of param->u4CurrentPMTLebPageIndicator
		 * and if param->u4CurrentPMTLebPageIndicator is full
		 * need to call get free block/page function
		 * Write old param->u4PMTCache back to new block/page
		 * Update param->u4PMTIndicator of the block/page that is originally in param->u4PMTCache
		 */

		pmt_block = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4CurrentPMTLebPageIndicator);
		pmt_page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(param->u4CurrentPMTLebPageIndicator);
		/* mt_ftl_err(dev, "u4CurrentPMTLebPageIndicator = 0x%x", param->u4CurrentPMTLebPageIndicator); */
		if (replay) {
			ret = mt_ftl_do_repay_pmt(dev, pmt_block, pmt_page);
			if (ret)
				return MT_FTL_FAIL;
		}

		if (!replay || (param->recorded_pmt_blk == pmt_block)) {
			if (ubi_is_mapped(desc, pmt_block)) {
				mt_ftl_err(dev, "write pmt %d %d", pmt_block, pmt_page);
				ret = mt_ftl_leb_write(dev, pmt_block, &param->u4PMTCache[i << dev->pm_per_io],
						 pmt_page * dev->min_io_size, dev->min_io_size);
				ret = mt_ftl_leb_write(dev, pmt_block, &param->u4MetaPMTCache[i << dev->pm_per_io],
						 (pmt_page + 1) * dev->min_io_size, dev->min_io_size);
			} else {
				mt_ftl_err(dev, "pmt_block(%d) is not mapped", pmt_block);
				return MT_FTL_FAIL;
			}
		}
		PMT_INDICATOR_SET_BLOCKPAGE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]],
					pmt_block, pmt_page, 0, i);
		mt_ftl_debug(dev, "u4PMTIndicator[%d] = 0x%x", param->i4CurrentPMTClusterInCache[i],
			   param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		/* clear i4CurrentPMTClusterInCache */
		if (param->i4CurrentReadPMTClusterInCache == param->i4CurrentPMTClusterInCache[i]) {
			mt_ftl_debug(dev, "[Bean]clear read PMTCluster cache");
			param->i4CurrentReadPMTClusterInCache = 0xFFFFFFFF;
		}
		PMT_INDICATOR_RESET_INCACHE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		param->i4CurrentPMTClusterInCache[i] = 0xFFFFFFFF;
		pmt_page += 2;
		if (pmt_page >= page_num_per_block) {
			pmt_block = mt_ftl_getfreeblock(dev, &pmt_page, true, replay);
			if (((u32)pmt_block) == MT_INVALID_BLOCKPAGE) {
				mt_ftl_err(dev, "mt_ftl_getfreeblock failed");
				return MT_FTL_FAIL;
			}
		}
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(param->u4CurrentPMTLebPageIndicator, pmt_block, pmt_page);
	}
	mt_ftl_err(dev, "commit pmt end 0x%x", param->u4CurrentPMTLebPageIndicator);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_COMMIT_PMT);
	return ret;
}

int mt_ftl_commit_indicators(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;
	int index = 0;
	struct mt_ftl_param *param = dev->param;
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;

	memset(param->commit_page_buffer, 0, (dev->min_io_size >> 2) * sizeof(unsigned int));
	param->commit_page_buffer[index++] = MT_MAGIC_NUMBER;
	param->commit_page_buffer[index++] = param->u4ComprType;
	param->commit_page_buffer[index++] = param->u4BlockDeviceModeFlag;
	param->commit_page_buffer[index++] = param->u4NextReplayOffsetIndicator;
	param->commit_page_buffer[index++] = param->u4NextLebPageIndicator;
	param->commit_page_buffer[index++] = param->u4CurrentPMTLebPageIndicator;
	param->commit_page_buffer[index++] = param->u4NextFreeLebIndicator;
	param->commit_page_buffer[index++] = param->u4NextFreePMTLebIndicator;
	param->commit_page_buffer[index++] = param->u4GCReserveLeb;
	param->commit_page_buffer[index++] = param->u4GCReservePMTLeb;
	param->commit_page_buffer[index++] = param->u4PMTChargeLebIndicator |
		(param->gc_pmt_charge_blk << PMT_CHARGE_LEB_SHIFT);

	if ((index + PMT_TOTAL_CLUSTER_NUM) * sizeof(unsigned int) > dev->min_io_size) {
		mt_ftl_err(dev, "(index + PMT_TOTAL_CLUSTER_NUM) * sizeof(unsigned int)(0x%x) > NAND_PAGE_SIZE(%d)",
			   (u32)((index + PMT_TOTAL_CLUSTER_NUM) * sizeof(unsigned int)), dev->min_io_size);
		mt_ftl_err(dev, "index = %d, PMT_TOTAL_CLUSTER_NUM = %d", index, PMT_TOTAL_CLUSTER_NUM);
	}
	memcpy(&param->commit_page_buffer[index], param->u4PMTIndicator, PMT_TOTAL_CLUSTER_NUM * sizeof(unsigned int));
	index += ((PMT_TOTAL_CLUSTER_NUM * sizeof(unsigned int)) >> 2);
	if ((index + NAND_TOTAL_BLOCK_NUM) * sizeof(unsigned int) > dev->min_io_size) {
		mt_ftl_err(dev, "index + NAND_TOTAL_BLOCK_NUM * sizeof(unsigned int)(0x%x) > NAND_PAGE_SIZE(%d)",
			   (u32)((index + NAND_TOTAL_BLOCK_NUM) * sizeof(unsigned int)), dev->min_io_size);
		mt_ftl_err(dev, "index = %d, NAND_TOTAL_BLOCK_NUM = %d", index, NAND_TOTAL_BLOCK_NUM);
	}
	memcpy(&param->commit_page_buffer[index], param->u4BIT, NAND_TOTAL_BLOCK_NUM * sizeof(unsigned int));
	index += ((NAND_TOTAL_BLOCK_NUM * sizeof(unsigned int)) >> 2);
	if ((index + PMT_CACHE_NUM) * sizeof(unsigned int) > dev->min_io_size) {
		mt_ftl_err(dev, "index + PMT_CACHE_NUM * sizeof(unsigned int)(0x%x) > NAND_PAGE_SIZE(%d)",
			   (u32)((index + PMT_CACHE_NUM) * sizeof(unsigned int)), dev->min_io_size);
		mt_ftl_err(dev, "index = %d, PMT_CACHE_NUM = %d", index, PMT_CACHE_NUM);
	}
	memcpy(&param->commit_page_buffer[index], param->i4CurrentPMTClusterInCache,
		PMT_CACHE_NUM * sizeof(unsigned int));

	mt_ftl_err(dev, "u4NextPageOffsetIndicator=0x%x", param->u4NextPageOffsetIndicator);
	mt_ftl_err(dev, "u4PMTChargeLebIndicator=0x%x", param->u4PMTChargeLebIndicator);
	mt_ftl_err(dev, "u4BlockDeviceModeFlag=0x%x", param->u4BlockDeviceModeFlag);
	mt_ftl_err(dev, "u4NextReplayOffsetIndicator=0x%x", param->u4NextReplayOffsetIndicator);
	mt_ftl_err(dev, "u4NextLebPageIndicator=0x%x", param->u4NextLebPageIndicator);
	mt_ftl_err(dev, "u4CurrentPMTLebPageIndicator=0x%x", param->u4CurrentPMTLebPageIndicator);
	mt_ftl_err(dev, "u4NextFreeLebIndicator=0x%x", param->u4NextFreeLebIndicator);
	mt_ftl_err(dev, "u4NextFreePMTLebIndicator=0x%x", param->u4NextFreePMTLebIndicator);
	mt_ftl_err(dev, "u4GCReserveLeb=0x%x", param->u4GCReserveLeb);
	mt_ftl_err(dev, "u4GCReservePMTLeb=0x%x", param->u4GCReservePMTLeb);
	mt_ftl_err(dev, "u4PMTIndicator=0x%x, 0x%x, 0x%x, 0x%x", param->u4PMTIndicator[0],
		param->u4PMTIndicator[1], param->u4PMTIndicator[2], param->u4PMTIndicator[3]);
	mt_ftl_err(dev, "u4BIT=0x%x,0x%x,0x%x,0x%x",
		param->u4BIT[0], param->u4BIT[1], param->u4BIT[2], param->u4BIT[3]);
	mt_ftl_err(dev, "i4CurrentPMTClusterInCache = 0x%x, 0x%x, 0x%x, 0x%x",
		   param->i4CurrentPMTClusterInCache[0], param->i4CurrentPMTClusterInCache[1],
		   param->i4CurrentPMTClusterInCache[2], param->i4CurrentPMTClusterInCache[3]);

	/* mt_ftl_write_to_blk(dev, CONFIG_START_BLOCK, param->commit_page_buffer);
	mt_ftl_write_to_blk(dev, CONFIG_START_BLOCK + 1, param->commit_page_buffer); */
	/* write config block*/
	param->i4CommitOffsetIndicator0 += dev->min_io_size;
	if (param->i4CommitOffsetIndicator0 > max_offset_per_block) {
		mt_ftl_leb_remap(dev, CONFIG_START_BLOCK);
		param->i4CommitOffsetIndicator0 = 0;
	}
	mt_ftl_leb_map(dev, CONFIG_START_BLOCK);
	mt_ftl_leb_write(dev, CONFIG_START_BLOCK, param->commit_page_buffer,
		param->i4CommitOffsetIndicator0, dev->min_io_size);
	/* write config backup block*/
	param->i4CommitOffsetIndicator1 += dev->min_io_size;
	if (param->i4CommitOffsetIndicator1 > max_offset_per_block) {
		mt_ftl_leb_remap(dev, CONFIG_START_BLOCK + 1);
		param->i4CommitOffsetIndicator1 = 0;
	}
	mt_ftl_leb_map(dev, CONFIG_START_BLOCK + 1);
	mt_ftl_leb_write(dev, CONFIG_START_BLOCK + 1, param->commit_page_buffer,
		param->i4CommitOffsetIndicator1, dev->min_io_size);

	memset(param->replay_blk_rec, 0xFF, dev->max_replay_blks * sizeof(unsigned int));
	param->replay_blk_index = 0;
	memset(param->replay_pmt_blk_rec, 0xFF, MAX_PMT_REPLAY_BLOCKS * sizeof(unsigned int));
	param->replay_pmt_blk_index = 0;
	param->replay_pmt_blk_rec[param->replay_pmt_blk_index]
		= PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4CurrentPMTLebPageIndicator);
	param->replay_pmt_blk_index += 1;

	return ret;
}

int mt_ftl_commit(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	struct mt_ftl_param *param = dev->param;

	MT_FTL_PROFILE_START(MT_FTL_PROFILE_COMMIT);
	ret = mt_ftl_leb_map(dev, CONFIG_START_BLOCK);
	ret = mt_ftl_leb_map(dev, CONFIG_START_BLOCK + 1);

	mt_ftl_commitPMT(dev, false, true);
	for (i = 0; i < PMT_CACHE_NUM; i++) {
		if (param->i4CurrentPMTClusterInCache[i] == 0xFFFFFFFF)
			continue;
		ubi_assert(param->i4CurrentPMTClusterInCache[i] <= PMT_TOTAL_CLUSTER_NUM);
		if (!PMT_INDICATOR_IS_INCACHE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]])) {
			mt_ftl_err(dev, "i4CurrentPMTClusterInCache(%d)not in cache",
				param->i4CurrentPMTClusterInCache[i]);
			return MT_FTL_FAIL;
		}

		PMT_INDICATOR_RESET_INCACHE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		mt_ftl_debug(dev, "u4PMTIndicator[%d] = %d", param->i4CurrentPMTClusterInCache[i],
			   param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		param->i4CurrentPMTClusterInCache[i] = 0xFFFFFFFF;
		/* cond_resched(); */
	}

	ret = mt_ftl_commit_indicators(dev);

	MT_FTL_PROFILE_END(MT_FTL_PROFILE_COMMIT);
	return ret;
}

static int mt_ftl_downloadPMT(struct mt_ftl_blk *dev, u32 cluster, int cache_num)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int pmt_block = 0, pmt_page = 0;

	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_param *param = dev->param;

	ubi_assert(cluster <= PMT_TOTAL_CLUSTER_NUM);
	pmt_block = PMT_INDICATOR_GET_BLOCK(param->u4PMTIndicator[cluster]);
	pmt_page = PMT_INDICATOR_GET_PAGE(param->u4PMTIndicator[cluster]);
	ubi_assert(cache_num < PMT_CACHE_NUM);

	if (unlikely(pmt_block == 0) || unlikely(ubi_is_mapped(desc, pmt_block) == 0)) {
		mt_ftl_debug(dev, "unlikely pmt_block %d", ubi_is_mapped(desc, pmt_block));
		memset(&param->u4PMTCache[cache_num << dev->pm_per_io], 0xFF,
			sizeof(unsigned int) << dev->pm_per_io);
		memset(&param->u4MetaPMTCache[cache_num << dev->pm_per_io], 0xFF,
			sizeof(unsigned int) << dev->pm_per_io);
	} else {
		ret = mt_ftl_leb_read(dev, pmt_block, &param->u4PMTCache[cache_num << dev->pm_per_io],
				pmt_page * dev->min_io_size , dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		ret = mt_ftl_leb_read(dev, pmt_block, &param->u4MetaPMTCache[cache_num << dev->pm_per_io],
			(pmt_page + 1) * dev->min_io_size, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
	}
	/* consider cluser if in cache */
	for (i = 0; i < PMT_CACHE_NUM; i++) {
		if (param->i4CurrentPMTClusterInCache[i] == cluster) {
			mt_ftl_debug(dev, "[Bean]Tempory solution cluster is in cache already(%d)\n", i);
			dump_stack();
			break;
		}
	}
	param->i4CurrentPMTClusterInCache[cache_num] = cluster;
	mt_ftl_debug(dev, "i4CurrentPMTClusterInCache[%d]=%d", cache_num, param->i4CurrentPMTClusterInCache[cache_num]);
	PMT_INDICATOR_SET_CACHE_BUF_NUM(param->u4PMTIndicator[cluster], cache_num);
	mt_ftl_debug(dev, "u4PMTIndicator[%d] = 0x%x", cluster, param->u4PMTIndicator[cluster]);

	return ret;
}

static void mt_ftl_discard_check(struct mt_ftl_blk *dev, u32 cluster, int sec_offset)
{
	int i = 0;
	u32 cluster_tmp, sec_tmp;
	struct mt_ftl_discard_entry *dd, *n;

	list_for_each_entry_safe(dd, n, &dev->discard_list, list) {
		/* check in the list */
		for (i = 0; i < dd->nr_sects; i += SECS_OF_FS_PAGE) {
			cluster_tmp = (u32)(FS_PAGE_OF_SECTOR(dd->sector + i) >> dev->pm_per_io);
			sec_tmp = (u32)(FS_PAGE_OF_SECTOR(dd->sector + i) & ((1 << dev->pm_per_io) - 1));
			if (cluster == cluster_tmp && sec_offset <= sec_tmp) {
				mt_ftl_debug(dev, "updated PMT in discard list, remove it %d:%d", i, dd->nr_sects);
				break;
			}
		}
		/* delete it from list */
		if (i < dd->nr_sects) {
			list_del_init(&dd->list);
			kmem_cache_free(dev->discard_slab, dd);
			dev->discard_count--;
			break;
		}
	}
}

int mt_ftl_updatePMT(struct mt_ftl_blk *dev, u32 cluster, int sec_offset, int leb, int offset,
		     int part, u32 cmpr_data_size, bool replay, bool commit)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	u32 *pmt = NULL;
	u32 *meta_pmt = NULL;
	int old_leb = 0, old_data_size = 0;
	struct mt_ftl_param *param = dev->param;

	/* check discard list */
	if (((u32)leb) != MT_INVALID_BLOCKPAGE && !replay)
		mt_ftl_discard_check(dev, cluster, sec_offset);

	ubi_assert(cluster <= PMT_TOTAL_CLUSTER_NUM);
	if (!PMT_INDICATOR_IS_INCACHE(param->u4PMTIndicator[cluster])) { /* cluster is not in cache */
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_UPDATE_PMT_FINDCACHE_COMMITPMT);
		for (i = 0; i < PMT_CACHE_NUM; i++) {
			if (param->i4CurrentPMTClusterInCache[i] == 0xFFFFFFFF)
				break;
			ubi_assert(param->i4CurrentPMTClusterInCache[i] <= PMT_TOTAL_CLUSTER_NUM);
			if (!PMT_INDICATOR_IS_INCACHE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]])) {
				/* Cluster download PMT CLUSTER cache, but i4CurrentPMTClusterInCache not to update */
				mt_ftl_err(dev, "i4CurrentPMTClusterInCache (%d) is not in cache",
					   param->i4CurrentPMTClusterInCache[i]);
				dump_stack();
				return MT_FTL_FAIL;
			}
			if (!PMT_INDICATOR_IS_DIRTY(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]))
				break;
		}
		if (i == PMT_CACHE_NUM) {
			mt_ftl_debug(dev, "All PMT cache are dirty, start commit PMT");
			/* Just for downloading corresponding PMT in cache */
			param->pmt_updated_flag = PMT_START;
			mt_ftl_commitPMT(dev, replay, commit);
			i = 0;
		}
		if (param->i4CurrentPMTClusterInCache[i] != 0xFFFFFFFF) {
			PMT_INDICATOR_RESET_INCACHE(param->u4PMTIndicator
						    [param->i4CurrentPMTClusterInCache[i]]);
			mt_ftl_debug(dev, "Reset (%d) u4PMTIndicator[%d]=0x%x", i, param->i4CurrentPMTClusterInCache[i],
				   param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
			param->i4CurrentPMTClusterInCache[i] = 0xFFFFFFFF;
		}
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_UPDATE_PMT_FINDCACHE_COMMITPMT);
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_UPDATE_PMT_DOWNLOADPMT);

		/* Download PMT from the block/page in param->u4PMTIndicator[cluster] */
		ret = mt_ftl_downloadPMT(dev, cluster, i);
		if (ret)
			return ret;

		MT_FTL_PROFILE_END(MT_FTL_PROFILE_UPDATE_PMT_DOWNLOADPMT);
	} else
		i = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[cluster]);

	MT_FTL_PROFILE_START(MT_FTL_PROFILE_UPDATE_PMT_MODIFYPMT);

	ubi_assert(i < PMT_CACHE_NUM);

	pmt = &param->u4PMTCache[(i << dev->pm_per_io) + sec_offset];
	meta_pmt = &param->u4MetaPMTCache[(i << dev->pm_per_io) + sec_offset];

	/* Update param->u4BIT */
	if (*pmt != NAND_DEFAULT_VALUE) {
		old_leb = PMT_GET_BLOCK(*pmt);
		old_data_size = PMT_GET_DATASIZE(*meta_pmt);
		ubi_assert(old_leb < dev->dev_blocks);
		BIT_UPDATE(param->u4BIT[old_leb], (old_data_size + sizeof(struct mt_ftl_data_header)));
	}
	if (((u32)leb) == MT_INVALID_BLOCKPAGE) {
		*pmt = NAND_DEFAULT_VALUE;
		*meta_pmt = NAND_DEFAULT_VALUE;
		PMT_INDICATOR_SET_DIRTY(param->u4PMTIndicator[cluster]);
		param->pmt_updated_flag = PMT_END;
		return ret;
	}
	/* Update param->u4PMTCache and param->u4MetaPMTCache */
	PMT_SET_BLOCKPAGE(*pmt, leb, offset / dev->min_io_size);
	META_PMT_SET_DATA(*meta_pmt, cmpr_data_size, part, -1);	/* Data not in cache */
	PMT_INDICATOR_SET_DIRTY(param->u4PMTIndicator[cluster]);
	mt_ftl_debug(dev, "u4PMTIndicator[%d] = 0x%x", cluster, param->u4PMTIndicator[cluster]);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_UPDATE_PMT_MODIFYPMT);
	if (!param->gc_dat_cmit_flag) {
		if ((param->pmt_updated_flag == PMT_COMMIT) || (param->get_pmt_blk_flag == 1)) {
			mt_ftl_debug(dev, "pmt_updated_flag(%d),get_pmt_blk=%d",
				param->pmt_updated_flag, param->get_pmt_blk_flag);
			mt_ftl_commit(dev);
		}
		param->get_pmt_blk_flag = 0;
	}
	param->pmt_updated_flag = PMT_END;
	return ret;
}

int mt_ftl_flush(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_param *param = dev->param;

	if (param->u4DataNum > 0 || param->u4NextPageOffsetIndicator) {
		mt_ftl_err(dev, "FTL Flush!");
		ret = mt_ftl_commit(dev);
	}
#ifdef MT_FTL_SUPPORT_WBUF
	mt_ftl_wakeup_wbuf_thread(dev);
#endif

	return ret;
}

static int mt_ftl_flush_discard(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;
	u32 cluster = 0, sec_offset = 0, i;
	struct mt_ftl_discard_entry *dd, *n;

	if (list_empty(&dev->discard_list))
		return MT_FTL_SUCCESS;
	list_for_each_entry_safe(dd, n, &dev->discard_list, list) {
		for (i = 0; i < dd->nr_sects; i += SECS_OF_FS_PAGE) {
			cluster = (u32)(FS_PAGE_OF_SECTOR(dd->sector + i) >> dev->pm_per_io);
			sec_offset = (u32)(FS_PAGE_OF_SECTOR(dd->sector + i) & ((1 << dev->pm_per_io) - 1));
			ret = mt_ftl_updatePMT(dev, cluster, sec_offset, MT_INVALID_BLOCKPAGE, 0, 0, 0, false, true);
			if (ret < 0) {
				mt_ftl_debug(dev, "cluster(%d) offset(%d)fail\n", cluster, sec_offset);
				return ret;
			}
		}
		list_del_init(&dd->list);
		kmem_cache_free(dev->discard_slab, dd);
	}
	dev->discard_count = 0;
	return ret;
}

int mt_ftl_discard(struct mt_ftl_blk *dev, sector_t sector, unsigned nr_sects)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_discard_entry *discard;

	discard = kmem_cache_alloc(dev->discard_slab, GFP_KERNEL);
	if (!discard) {
		mt_ftl_err(dev, "alloc FAIL");
		return -ENOMEM;
	}
	discard->sector = sector;
	discard->nr_sects = nr_sects;
	list_add_tail(&discard->list, &dev->discard_list);
	dev->discard_count++;
	mt_ftl_debug(dev, "discard 0x%x, 0x%x", (u32)sector, nr_sects);
	if ((dev->discard_count >= MT_FTL_MAX_DISCARD_NUM) || (nr_sects >= MT_FTL_DISCARD_THRESHOLD)) {
		mt_ftl_commit(dev);
		ret = mt_ftl_flush_discard(dev);
		mt_ftl_commit(dev);
	}
	return ret;
}


static int mt_ftl_write_cache(struct mt_ftl_blk *dev, char *buffer, sector_t sector, int len)
{
	int ret = MT_FTL_SUCCESS;
	int i = 0, part = 0;
	int data_len = 0, part_len = 0;
	int leb = 0, page = 0;
	u32 cluster = 0, sec_offset = 0;
	int data_offset = 0, part_offset = 0, total_size = 0;
	struct mt_ftl_param *param = dev->param;
	u8 *tmp_buf = NULL;

	for (i = 1; i <= param->u4DataNum; i++) {
		if (sector == (param->u4Header[DATA_NUM_PER_PAGE - i].sector)) {
			part = i; /* note the data number of same sector*/
			mt_ftl_debug(dev, "sector(0x%lx) is already in cache, part = %d, u4Datanum = %d",
				(unsigned long int)sector, i, param->u4DataNum);
			break;
		}
	}
	if (part) {
		leb = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4NextLebPageIndicator);
		page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(param->u4NextLebPageIndicator);
		cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> dev->pm_per_io);
		sec_offset = (u32)(FS_PAGE_OF_SECTOR(sector) & ((1 << dev->pm_per_io) - 1));
		part_len = param->u4Header[DATA_NUM_PER_PAGE - part].offset_len & 0xFFFF;
		part_offset = (param->u4Header[DATA_NUM_PER_PAGE - part].offset_len >> 16) & 0xFFFF;
		if (part_len == len) {
			memcpy(&param->u1DataCache[part_offset], buffer, len);
			ret = 1; /* data already in cache */
			mt_ftl_debug(dev, "part_len==len %d %d %d", part, part_len, part_offset);
		} else {
			data_offset = (param->u4Header[DATA_NUM_PER_PAGE -
				param->u4DataNum].offset_len >> 16) & 0xFFFF;
			data_len = param->u4Header[DATA_NUM_PER_PAGE - param->u4DataNum].offset_len & 0xFFFF;
			total_size = len - part_len + data_offset + data_len +
				param->u4DataNum * sizeof(struct mt_ftl_data_header) + 4;
			mt_ftl_debug(dev, "part(%d) is not equal to zero, total_size = %d %d",
				part, total_size, part_offset);
			if (total_size <= dev->min_io_size) {
				tmp_buf = vmalloc(dev->min_io_size);
				if (!tmp_buf) {
					mt_ftl_err(dev, "malloc fail");
					ret = MT_FTL_FAIL;
					goto out;
				}
				memset(tmp_buf, 0xFF, dev->min_io_size);
				memcpy(tmp_buf, param->u1DataCache, part_offset);
				/* Replace the part data */
				memcpy(tmp_buf + part_offset, buffer, len);
				param->u4Header[DATA_NUM_PER_PAGE - part].offset_len =
					(part_offset << 16) | len;
				part_offset += len;
				for (i = part + 1; i <= param->u4DataNum; i++) {
					data_offset =
						(param->u4Header[DATA_NUM_PER_PAGE - i].offset_len
						>> 16) & 0xFFFF;
					data_len = param->u4Header[DATA_NUM_PER_PAGE - i].offset_len
						& 0xFFFF;
					memcpy(tmp_buf + part_offset, &param->u1DataCache[data_offset], data_len);
					param->u4Header[DATA_NUM_PER_PAGE - i].offset_len =
						(part_offset << 16) | data_len;
					part_offset += data_len;
				}
				memcpy(param->u1DataCache, tmp_buf, dev->min_io_size);
				ret = mt_ftl_updatePMT(dev, cluster, sec_offset, leb, page * dev->min_io_size,
						part - 1, len, false, true);
				if (ret < 0) {
					mt_ftl_err(dev, "mt_ftl_updatePMT cluster(%d) offset(%d) leb(%d) page(%d) fail\n",
						cluster, sec_offset, leb, page);
					goto out;
				}
				ret = 1; /* data already in cache */
			}
		}
	}
out:
	if (tmp_buf) {
		vfree(tmp_buf);
		tmp_buf = NULL;
	}
	return ret;
}

/* Suppose FS_PAGE_SIZE for each write */
int mt_ftl_write(struct mt_ftl_blk *dev, char *buffer, sector_t sector, int len)
{
	int ret = MT_FTL_SUCCESS;
	int leb = 0, page = 0;
	u32 cluster = 0, sec_offset = 0;
	int cache_num = 0;
	int *meta_pmt = NULL;
	u32 cmpr_len = 0, data_offset = 0, total_consumed_size = 0;
	int last_data_len = 0;
	const int page_num_per_block = dev->leb_size / dev->min_io_size;
	struct mt_ftl_param *param = dev->param;

	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_WRITEPAGE);
	MT_FTL_PROFILE_START_ALL(MT_FTL_PROFILE_WRITE_ALL);

	ret = mt_ftl_compress(dev, buffer, len, &cmpr_len);
	if (ret) {
		mt_ftl_err(dev, "ret = %d, cmpr_len = %d, len = 0x%x", ret, cmpr_len, len);
		mt_ftl_err(dev, "cc = 0x%lx, buffer = 0x%lx", (unsigned long int)param->cc, (unsigned long int)buffer);
		mt_ftl_err(dev, "cmpr_page_buffer = 0x%lx", (unsigned long int)param->cmpr_page_buffer);
		return MT_FTL_FAIL;
	}

	MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_WRITEPAGE);

	cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> dev->pm_per_io);
	sec_offset = (u32)(FS_PAGE_OF_SECTOR(sector) & ((1 << dev->pm_per_io) - 1));
	if (param->u4DataNum > 0) {
		ret = mt_ftl_write_cache(dev, param->cmpr_page_buffer, sector, cmpr_len);
		if (ret == MT_FTL_FAIL) {
			mt_ftl_err(dev, "write cahce fail");
			return ret;
		} else if (ret == 1) {
			mt_ftl_debug(dev, "write into cahce");
			ret = MT_FTL_SUCCESS;
			goto out_cache;
		}
	}

	if (param->u4DataNum > 0) {
		data_offset =
			((param->u4Header[DATA_NUM_PER_PAGE - param->u4DataNum].offset_len >> 16) & 0xFFFF)
			+ (param->u4Header[DATA_NUM_PER_PAGE - param->u4DataNum].offset_len & 0xFFFF);
		param->u4NextPageOffsetIndicator = 0;
	} else
		data_offset = param->u4NextPageOffsetIndicator;
	total_consumed_size = data_offset + cmpr_len + (param->u4DataNum + 1) * sizeof(struct mt_ftl_data_header) + 4;
	leb = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4NextLebPageIndicator);
	page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(param->u4NextLebPageIndicator);
	if ((total_consumed_size > dev->min_io_size)
		|| (param->u4DataNum >= DATA_NUM_PER_PAGE)) {
		last_data_len = dev->min_io_size - data_offset -
			((param->u4DataNum + 1) * sizeof(struct mt_ftl_data_header) + 4);
		if ((page + 1) == page_num_per_block) {
			ret = mt_ftl_write_page(dev);
			data_offset = 0;
			last_data_len = 0;
		} else if (last_data_len <= 0) {
			mt_ftl_debug(dev, "write_page=(%d:%d),data_offset=%d,last_data_len=%d",
				page, leb, data_offset, last_data_len);
			ret = mt_ftl_write_page(dev);
			last_data_len = 0;
			data_offset = 0;
		}
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_WRITEPAGE);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_COPYTOCACHE);

	leb = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4NextLebPageIndicator);
	page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(param->u4NextLebPageIndicator);
	param->u4Header[DATA_NUM_PER_PAGE - param->u4DataNum - 1].sector =
	    FS_PAGE_OF_SECTOR(sector) << SECS_OF_SHIFT; /* caution !!! */
	param->u4Header[DATA_NUM_PER_PAGE - param->u4DataNum - 1].offset_len =
	    (data_offset << 16) | cmpr_len;
	if (last_data_len) {
		ubi_assert(data_offset + last_data_len < dev->min_io_size);
		memcpy(&param->u1DataCache[data_offset], param->cmpr_page_buffer, last_data_len);
	} else {
		ubi_assert(data_offset + cmpr_len < dev->min_io_size);
		memcpy(&param->u1DataCache[data_offset], param->cmpr_page_buffer, cmpr_len);
	}
	param->u4DataNum++;

	MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_COPYTOCACHE);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_UPDATEPMT);

	ret = mt_ftl_updatePMT(dev, cluster, sec_offset, leb, page * dev->min_io_size, param->u4DataNum - 1,
			 cmpr_len, false, true);
	if (ret < 0) {
		mt_ftl_err(dev, "mt_ftl_updatePMT cluster(%d) offset(%d) leb(%d) page(%d) fail\n",
			cluster, sec_offset, leb, page);
		return ret;
	}

	MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_UPDATEPMT);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_WRITEPAGE);
out_cache:
	ubi_assert(cluster <= PMT_TOTAL_CLUSTER_NUM);
	cache_num = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[cluster]);
	ubi_assert(cache_num < PMT_CACHE_NUM);
	meta_pmt = &param->u4MetaPMTCache[(cache_num << dev->pm_per_io) + sec_offset];
	if (param->u4DataNum != 0)
		PMT_SET_DATACACHE_BUF_NUM(*meta_pmt, 0);	/* Data is in cache 0 */

	PMT_INDICATOR_SET_DIRTY(param->u4PMTIndicator[cluster]);	/* Set corresponding PMT cache to dirty */
	if (last_data_len) {
		if (param->u4DataNum != 0)
			ret = mt_ftl_write_page(dev);
		data_offset = 0;
		param->u4NextPageOffsetIndicator = cmpr_len - last_data_len;
		memcpy(&param->u1DataCache[data_offset], &param->cmpr_page_buffer[last_data_len],
			param->u4NextPageOffsetIndicator);
		/*mt_ftl_err(dev,  "u4NextPageOffsetIndicator = %u", param->u4NextPageOffsetIndicator);*/
		if (dev->sync > 0) {
			ret = mt_ftl_write_page(dev);
			if (ret)
				return ret;
			dev->sync = 0;
#ifdef MT_FTL_SUPPORT_WBUF
			mt_ftl_wakeup_wbuf_thread(dev);
			wait_event(dev->sync_wait, list_empty(&dev->wbuf_list));
#endif
		}
	}
	if (dev->sync > 0) {
		if (param->u4DataNum != 0) {
			ret = mt_ftl_write_page(dev);
			if (ret)
				return ret;
		}
		dev->sync = 0;
#ifdef MT_FTL_SUPPORT_WBUF
		mt_ftl_wakeup_wbuf_thread(dev);
		wait_event(dev->sync_wait, list_empty(&dev->wbuf_list));
#endif
	}
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_WRITEPAGE);
	MT_FTL_PROFILE_END_ALL(MT_FTL_PROFILE_WRITE_ALL);
	return ret;
}

/* Suppose FS_PAGE_SIZE for each read */
int mt_ftl_read(struct mt_ftl_blk *dev, char *buffer, sector_t sector, int len)
{
	int ret = MT_FTL_SUCCESS;
	int leb = 0, page = 0, part = 0;
	u32 cluster = 0, sec_offset = 0;
	u32 pmt = 0, meta_pmt = 0;
	u32 offset = 0, data_len = 0;
	int pmt_block = 0, pmt_page = 0;
	int cache_num = 0, data_cache_num = 0;
	u32 decmpr_len = 0;
	u32 data_num = 0;
	int next_block = 0, next_page = 0;
	int last_data_len = 0;
	struct mt_ftl_param *param = dev->param;
	u32 data_hdr_offset = 0;
	unsigned char *page_buffer = NULL;
	struct mt_ftl_data_header *header_buffer = NULL;

	MT_FTL_PROFILE_START(MT_FTL_PROFILE_READ_GETPMT);
	MT_FTL_PROFILE_START_ALL(MT_FTL_PROFILE_READ_ALL);

	cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> dev->pm_per_io);
	sec_offset = (u32)(FS_PAGE_OF_SECTOR(sector) & ((1 << dev->pm_per_io) - 1));

	ubi_assert(cluster <= PMT_TOTAL_CLUSTER_NUM);
	ret = mt_ftl_get_pmt(dev, sector, &pmt, &meta_pmt);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_READ_GETPMT);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_READ_DATATOCACHE);
	/* Look up PMT in cache and get data from NAND flash */
	if (pmt == NAND_DEFAULT_VALUE) {
		/* mt_ftl_err(dev, "PMT of sector(0x%lx) is invalid", (unsigned long int)sector); */
		memset((void *)buffer, 0x0, len);
		return MT_FTL_SUCCESS;
	} else if (ret)
		return MT_FTL_FAIL;
	leb = PMT_GET_BLOCK(pmt);
	page = PMT_GET_PAGE(pmt);
	part = PMT_GET_PART(meta_pmt);

	/* mt_ftl_err(dev, "Copy to cache"); */
	ubi_assert(part < DATA_NUM_PER_PAGE);
	if (PMT_IS_DATA_INCACHE(meta_pmt)) {
		mt_ftl_debug(dev, "[INFO] Use data in cache");
		data_cache_num = PMT_GET_DATACACHENUM(meta_pmt);	/* Not used yet */
		header_buffer = &param->u4Header[DATA_NUM_PER_PAGE - part - 1];
		offset = FDATA_OFFSET(header_buffer->offset_len);
		page_buffer = &param->u1DataCache[offset];
	} else {
		data_hdr_offset = dev->min_io_size - (part + 1) * sizeof(struct mt_ftl_data_header) - 4;
		ret = mt_ftl_leb_read(dev, leb, param->general_page_buffer, page * dev->min_io_size,
					dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		header_buffer = (struct mt_ftl_data_header *)((char *)param->general_page_buffer + data_hdr_offset);
		offset = FDATA_OFFSET(header_buffer->offset_len);
		data_len = FDATA_LEN(header_buffer->offset_len);
		if (offset)
			memcpy(param->general_page_buffer, (char *)param->general_page_buffer + offset, data_len);

		last_data_len = dev->min_io_size - offset - ((part + 1) * sizeof(struct mt_ftl_data_header) + 4);
		if (last_data_len < data_len) {
			next_block = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4NextLebPageIndicator);
			next_page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(param->u4NextLebPageIndicator);
			if ((leb == next_block) && ((page + 1) == next_page)) {
				memcpy((char *)param->general_page_buffer + last_data_len, param->u1DataCache,
					data_len - last_data_len);
			} else {
				ret = mt_ftl_leb_read(dev, leb, (char *)param->general_page_buffer +
					last_data_len, (page + 1) * dev->min_io_size, data_len - last_data_len);
				if (ret)
					return MT_FTL_FAIL;
			}
		} 
		page_buffer = (unsigned char *)param->general_page_buffer;
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_READ_DATATOCACHE);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_READ_ADDRNOMATCH);

	/* mt_ftl_err(dev,  "Check sector"); */
	if (header_buffer->sector != (FS_PAGE_OF_SECTOR(sector) << SECS_OF_SHIFT)) {
		if ((header_buffer->sector == NAND_DEFAULT_SECTOR_VALUE) && (page_buffer[0] == 0xFF)) {
			mt_ftl_err(dev, "sector(0x%lx) hasn't been written", (unsigned long int)sector);
			mt_ftl_err(dev, "leb = %d, page = %d, part = %d", leb, page, part);
			memset((void *)buffer, 0xFF, len);
			return MT_FTL_SUCCESS;
		}
		ret = mt_ftl_leb_read(dev, leb, param->tmp_page_buffer, page * dev->min_io_size, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		data_num = PAGE_GET_DATA_NUM(param->tmp_page_buffer[(dev->min_io_size >> 2) - 1]);
		mt_ftl_err(dev, "header_buffer[%d].sector(0x%x) != sector (0x%x), header_buffer[%d].offset_len = 0x%x",
			   part, (u32)header_buffer->sector, (u32)sector, part, header_buffer->offset_len);
		mt_ftl_err(dev, "page_buffer[0] = 0x%x, u4PMTIndicator[%d] = 0x%x, data_num = %d",
			   page_buffer[0], cluster, param->u4PMTIndicator[cluster], data_num);
		mt_ftl_err(dev, "pmt = 0x%x, meta_pmt = 0x%x, leb = %d, page = %d, part = %d",
			   pmt, meta_pmt, leb, page, part);
		mt_ftl_err(dev, "data_num_offset = %d, data_hdr_offset = %d", offset, data_hdr_offset);
		mt_ftl_err(dev, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 1],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 2],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 3],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 4],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 5],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 6],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 7],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 8],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 9]);
		/*=====================Debug==========================*/
		/* Calculate clusters and sec_offsets */
		cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> dev->pm_per_io);
		sec_offset = (u32)(FS_PAGE_OF_SECTOR(sector) & ((1 << dev->pm_per_io) - 1));
		mt_ftl_err(dev, "cluster = %d", cluster);
		mt_ftl_err(dev, "u4PMTIndicator[%d] = 0x%x", cluster,
			   param->u4PMTIndicator[cluster]);

		/* Download PMT to read PMT cache */
		/* Don't use mt_ftl_updatePMT, that will cause PMT indicator mixed in replay */
		if (PMT_INDICATOR_IS_INCACHE(param->u4PMTIndicator[cluster])) {
			cache_num = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[cluster]);
			ubi_assert(cache_num < PMT_CACHE_NUM);
			pmt = param->u4PMTCache[(cache_num << dev->pm_per_io) + sec_offset];
			meta_pmt = param->u4MetaPMTCache[(cache_num << dev->pm_per_io) + sec_offset];
		} else if (cluster == param->i4CurrentReadPMTClusterInCache) {
			pmt = param->u4ReadPMTCache[sec_offset];
			meta_pmt = param->u4ReadMetaPMTCache[sec_offset];
		} else {
			pmt_block = PMT_INDICATOR_GET_BLOCK(param->u4PMTIndicator[cluster]);
			pmt_page = PMT_INDICATOR_GET_PAGE(param->u4PMTIndicator[cluster]);

			if (unlikely(pmt_block == 0)) {
				mt_ftl_err(dev, "pmt_block == 0");
				/* memset(param->u4ReadPMTCache, 0xFF, PM_PER_NANDPAGE * sizeof(unsigned int)); */
				pmt = 0xFFFFFFFF;
				meta_pmt = 0xFFFFFFFF;
			} else {
				mt_ftl_err(dev, "Get PMT of cluster (%d)", cluster);
				ret = mt_ftl_leb_read(dev, pmt_block, param->u4ReadPMTCache,
						pmt_page * dev->min_io_size, dev->min_io_size);
				if (ret)
					return MT_FTL_FAIL;
				ret = mt_ftl_leb_read(dev, pmt_block, param->u4ReadMetaPMTCache,
					(pmt_page + 1) * dev->min_io_size, dev->min_io_size);
				if (ret)
					return MT_FTL_FAIL;
				param->i4CurrentReadPMTClusterInCache = cluster;
				pmt = param->u4ReadPMTCache[sec_offset];
				meta_pmt = param->u4ReadMetaPMTCache[sec_offset];
			}
		}

		leb = PMT_GET_BLOCK(pmt);
		page = PMT_GET_PAGE(pmt);
		part = PMT_GET_PART(meta_pmt);
		mt_ftl_err(dev, "for sector (0x%lx), pmt = 0x%x, meta_pmt = 0x%x, leb = %d, page = %d, part = %d",
			   (unsigned long int)header_buffer->sector, pmt, meta_pmt, leb, page, part);
		/*====================================================*/
		/* not to report error, but set 0 for fs*/
		memset((void *)buffer, 0, len);
		return MT_FTL_SUCCESS;
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_READ_ADDRNOMATCH);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_READ_DECOMP);

	decmpr_len = dev->min_io_size;
	ret = mt_ftl_decompress(dev, page_buffer, (header_buffer->offset_len & 0xFFFF), &decmpr_len);
	if (ret) {
		ret = mt_ftl_leb_read(dev, leb, param->tmp_page_buffer, page * dev->min_io_size, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		mt_ftl_err(dev, "part = %d", part);
		mt_ftl_err(dev, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1)],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 1],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 2],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 3],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 4],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 5],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 6]);
		mt_ftl_err(dev, "ret = %d, decmpr_len = %d, header_buffer->offset_len = 0x%x",
			   ret, decmpr_len, header_buffer->offset_len);
		mt_ftl_err(dev, "cc = 0x%lx, page_buffer = 0x%lx",
			   (unsigned long int)param->cc, (unsigned long int)page_buffer);
		mt_ftl_err(dev, "cmpr_page_buffer = 0x%lx", (unsigned long int)param->cmpr_page_buffer);
		return MT_FTL_FAIL;
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_READ_DECOMP);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_READ_COPYTOBUFF);

	offset = (u32)((sector & (SECS_OF_FS_PAGE - 1)) << 9);
	if ((offset + len) > decmpr_len) {
		mt_ftl_err(dev, "offset(%d)+len(%d)>decmpr_len (%d)", offset, len, decmpr_len);
		return MT_FTL_FAIL;
	}
	/* mt_ftl_err(dev,  "Copy to buffer"); */
	ubi_assert(offset + len <= dev->min_io_size);
	memcpy((void *)buffer, &param->cmpr_page_buffer[offset], len);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_READ_COPYTOBUFF);
	MT_FTL_PROFILE_END_ALL(MT_FTL_PROFILE_READ_ALL);
	return ret;
}

static int mt_ftl_replay_single_block(struct mt_ftl_blk *dev, int leb, int *page)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int offset = 0;
	sector_t sector = 0;
	u32 cluster = 0, sec_offset = 0;
	u32 data_num = 0, data_hdr_offset = 0;
	int retry = 0, last_data_len = 0;
	struct mt_ftl_data_header *header_buffer = NULL;
	struct mt_ftl_param *param = dev->param;
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;

	ret = mt_ftl_leb_map(dev, leb);
	if (ret == 0) {
		mt_ftl_debug(dev, "leb%d is unmapped", leb);
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(param->u4NextLebPageIndicator, leb, *page);
		param->last_replay_flag = REPLAY_END;
		return MT_FTL_SUCCESS;
	}
	offset = (*page) * dev->min_io_size;
	if (offset == dev->leb_size) {
		mt_ftl_err(dev, "replay offset = leb size");
		return MT_FTL_SUCCESS;
	}
first_page:
	ret = mt_ftl_leb_read(dev, leb, param->general_page_buffer, offset, dev->min_io_size);
	if (ret)
		return MT_FTL_FAIL;

	data_num = PAGE_GET_DATA_NUM(param->general_page_buffer[(dev->min_io_size >> 2) - 1]);
	if (data_num == 0x3FFFFFFF) {
		mt_ftl_err(dev, "NextLebPage:data_num = 0x3FFFFFFF,read next page, page(%d)", *page);
		offset += dev->min_io_size;
		*page += 1;
		goto first_page;
	}
	if (data_num == 0x7FFFFFFF) {
		mt_ftl_err(dev, "End of block,page=%d", *page);
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(param->u4NextLebPageIndicator, leb, *page);
		param->last_replay_flag = REPLAY_END;
		return MT_FTL_SUCCESS;
	}
	data_hdr_offset = dev->min_io_size - (data_num * sizeof(struct mt_ftl_data_header) + 4);

	header_buffer = (struct mt_ftl_data_header *)(&param->general_page_buffer[data_hdr_offset >> 2]);

	mt_ftl_err(dev, "leb = %d, page = %d, data_num = %d", leb, *page, data_num);
	while (data_num && (offset <= max_offset_per_block)) {
		/* Update param->u4NextLebPageIndicator
		 * check the correctness of param->u4NextLebPageIndicator &
		 * if param->u4NextLebPageIndicator is full, need to call get free block & page function
		 */

		/* Update param->u4PMTCache and param->u4PMTIndicator and param->u4BIT */
		/* If the page is copied in GC, that means the page should not be replayed */
		retry = 1;
		last_data_len = dev->min_io_size - (((header_buffer[0].offset_len >> 16) & 0xFFFF) +
			(header_buffer[0].offset_len & 0xFFFF) + (data_num * sizeof(struct mt_ftl_data_header) + 4));
		if (last_data_len > 0)
			BIT_UPDATE(param->u4BIT[leb], last_data_len);

		if (PAGE_BEEN_READ(param->general_page_buffer[(dev->min_io_size >> 2) - 1]) == 0) {
			for (i = 0; i < data_num; i++) {
				/* Get sector in the page */
				sector = header_buffer[data_num - i - 1].sector;
				if ((sector & NAND_DEFAULT_SECTOR_VALUE) == NAND_DEFAULT_SECTOR_VALUE) {
					mt_ftl_err(dev, "header_buffer[%d].sector == 0xFFFFFFFF, leb = %d, page = %d",
						   i, leb, offset / dev->min_io_size);
					continue;
				}

				/* Calculate clusters and sec_offsets */
				cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> dev->pm_per_io);
				sec_offset = (u32)(FS_PAGE_OF_SECTOR(sector) & ((1 << dev->pm_per_io) - 1));

				mt_ftl_updatePMT(dev, cluster, sec_offset, leb, offset, i,
						 (header_buffer[data_num - i - 1].offset_len &
						  0xFFFF), true, true);
			}
		}
retry_once:
		offset += dev->min_io_size;
		*page = offset / dev->min_io_size;
		if (offset > max_offset_per_block)
			break;
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(param->u4NextLebPageIndicator, leb, *page);
		ret = mt_ftl_leb_read(dev, leb, param->general_page_buffer, offset, dev->min_io_size);
		if (ret == MT_FTL_FAIL) {
			/* write page power loss recover */
			ret = mt_ftl_leb_recover(dev, leb, offset, 0);
			if (ret == 1)
				ret = MT_FTL_SUCCESS;
			break;
		}

		data_num = PAGE_GET_DATA_NUM(param->general_page_buffer[(dev->min_io_size >> 2) - 1]);
		mt_ftl_debug(dev, "%d, %d, %u, %d", retry, last_data_len, data_num, *page);
		if ((data_num == 0x3FFFFFFF) && retry && last_data_len < 0) {
			mt_ftl_err(dev, "replay: read retry, last_data_len=%d, page=%d", last_data_len, *page);
			retry = 0;
			BIT_UPDATE(param->u4BIT[leb], dev->min_io_size + last_data_len - 4);
			goto retry_once;
		}
		if (data_num == 0x7FFFFFFF)
			break;
		data_hdr_offset = dev->min_io_size - (data_num * sizeof(struct mt_ftl_data_header) + 4);
		header_buffer = (struct mt_ftl_data_header *)(&param->general_page_buffer[data_hdr_offset >> 2]);
	}

	mt_ftl_err(dev, "offset = %d at the end, max_offset_per_block = %d, retry=%d, last_data_len=%d, data_num=%u, page=%d",
		offset, max_offset_per_block, retry, last_data_len, data_num, *page);

	return ret;
}
static int mt_ftl_check_replay_blk(struct mt_ftl_blk *dev, int offset, int *leb)
{
	int ret = 0, magic = 0;
	int replay_blk, replay_offset;
	struct mt_ftl_param *param = dev->param;

	*leb = MT_INVALID_BLOCKPAGE;
	replay_blk = offset / dev->leb_size + REPLAY_BLOCK;
	replay_offset = offset % dev->leb_size;
	mt_ftl_err(dev, "replay_blk:%d replay_offset:%d", replay_blk, replay_offset);
	/* Get the successive lebs to replay */
	ret = ubi_is_mapped(dev->desc, replay_blk);
	if (ret == 0) {
		mt_ftl_err(dev, "leb%d is unmapped", replay_blk);
		return 0;
	}
	if (offset >= (dev->max_replay_blks * dev->min_io_size))
		return 0;
	ret = mt_ftl_leb_read(dev, replay_blk, param->replay_page_buffer,
		replay_offset, sizeof(unsigned int) * 2);
	if (ret) {
		mt_ftl_leb_recover(dev, replay_blk, replay_offset, 0);
		return 0;
	}
	magic = param->replay_page_buffer[0];
	if (magic == MT_MAGIC_NUMBER) {
		*leb = param->replay_page_buffer[1];
		return 1; /* case 1 for gc done */
	}
	return 0;
}

static int mt_ftl_replay(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, check_num = 0;
	int leb = 0, page = 0, offset = 0, full = 1, first = 0;
	int nextleb_in_replay = MT_INVALID_BLOCKPAGE, tmp_leb;
	int pmt_block = 0, pmt_page = 0;
	struct mt_ftl_param *param = dev->param;
	const int max_replay_offset = dev->leb_size * dev->max_replay_blks;
	const int page_num_per_block = dev->leb_size / dev->min_io_size;

	if (param->u4BlockDeviceModeFlag) {
		mt_ftl_err(dev, "[Bean]Read Only, no need replay");
		return ret;
	}
	param->gc_pmt_charge_blk = param->u4PMTChargeLebIndicator >> PMT_CHARGE_LEB_SHIFT;
	param->u4PMTChargeLebIndicator &= ((1 << PMT_CHARGE_LEB_SHIFT) - 1);
	offset = param->u4NextReplayOffsetIndicator;
	param->replay_blk_index = param->u4NextReplayOffsetIndicator / dev->min_io_size % dev->max_replay_blks;
	mt_ftl_err(dev, "replay offset: %d", offset);
	/* check the next or not */
	check_num = mt_ftl_check_replay_blk(dev, offset, &tmp_leb);
	if (!check_num) {
		mt_ftl_err(dev, "first last replay start");
		param->last_replay_flag = REPLAY_LAST_BLK;
	}
	/* Replay leb/page of param->u4NextLebPageIndicator */
	leb = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4NextLebPageIndicator);
	page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(param->u4NextLebPageIndicator);
	ret = mt_ftl_replay_single_block(dev, leb, &page);
	if (ret)
		return ret;
	if (page == page_num_per_block && check_num) {
		mt_ftl_err(dev, "replay getfreeblock");
		first = 1;
	} else {
		mt_ftl_err(dev, "replay nextlebpage only");
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(param->u4NextLebPageIndicator, leb, page);
		param->last_replay_flag = REPLAY_END;
		goto out;
	}
	while (offset < max_replay_offset) {
		ret = mt_ftl_check_replay_blk(dev, offset, &nextleb_in_replay);
		if (ret) {
			if (page != page_num_per_block) {
				mt_ftl_err(dev, "last replay error");
				ubi_assert(false);
				break;
			}
			/* check the next or not */
			check_num = mt_ftl_check_replay_blk(dev, offset + dev->min_io_size, &tmp_leb);
			if (check_num != 1) {
				mt_ftl_err(dev, "last replay start");
				param->last_replay_flag = REPLAY_LAST_BLK;
			}
			/* check the first get free block */
			if (full) {
				if (first) {
					/* get the first free leb */
					leb = mt_ftl_getfreeblock(dev, &page, false, true);
					first = 0;
				}
				mt_ftl_err(dev, "[Bean]nextlebpage replay done %d %d", leb, nextleb_in_replay);
				if (leb == nextleb_in_replay) {
					mt_ftl_err(dev, "replay next leb in replay block");
					full = 0;
				} else {
					offset += dev->min_io_size;
					mt_ftl_err(dev, "replay replayblock mapped");
					continue;
				}
			} else {
				leb = mt_ftl_getfreeblock(dev, &page, false, true);
				if (leb != nextleb_in_replay) {
					mt_ftl_err(dev, "leb(%d)!=nextleb_in_replay(%d)", leb, nextleb_in_replay);
					return MT_FTL_FAIL;
				}
			}
		} else
			break;
		offset += dev->min_io_size;
		/* page = 0; */
		ret = mt_ftl_replay_single_block(dev, leb, &page);
		if (ret)
			return ret;
	}
out:
	/* recover power cut data block */
	ret = mt_ftl_leb_recover(dev, leb, page * dev->min_io_size, 0);
	if (ret == MT_FTL_FAIL) {
		mt_ftl_err(dev, "recover data blk fail %d %d", leb, page);
		return ret;
	}
	/* recover power cut pmt block */
	pmt_block = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4CurrentPMTLebPageIndicator);
	pmt_page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(param->u4CurrentPMTLebPageIndicator);
	ret = mt_ftl_leb_recover(dev, pmt_block, pmt_page * dev->min_io_size, 0);
	if (ret == MT_FTL_FAIL) {
		mt_ftl_err(dev, "recover pmt blk fail %d %d", pmt_block, pmt_page);
		return ret;
	}
	ret = MT_FTL_SUCCESS;
	param->recorded_pmt_blk = -1; /* reset  recorded pmt blk */
	param->last_replay_flag = REPLAY_EMPTY;
	if (page == page_num_per_block) {
		/* param->u4NextReplayOffsetIndicator = 0;
		memset(param->replay_blk_rec, 0xFF, dev->max_replay_blks * sizeof(unsigned int));
		param->replay_blk_index = 0; */
		mt_ftl_err(dev, "replay_blk_index:%d", param->replay_blk_index);
		leb = mt_ftl_getfreeblock(dev, &page, false, false);
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(param->u4NextLebPageIndicator, leb, page);
	}
	if (dev->readonly)
		param->u4BlockDeviceModeFlag = 1; /* reset Block device flag to read only*/
	/* replay over force commit by Bean*/
	mt_ftl_commit(dev);

	return ret;
}

static int mt_ftl_alloc_single_buffer(unsigned int **buf, int size, char *str)
{
	if (*buf == NULL) {
		*buf = kzalloc(size, GFP_KERNEL);
		if (!*buf) {
			ubi_err("%s allocate memory fail", str);
			return -ENOMEM;
		}
	}
	memset(*buf, 0xFF, size);

	return 0;
}

static int mt_ftl_alloc_buffers(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_param *param = dev->param;

	ret = mt_ftl_alloc_single_buffer(&param->u4PMTIndicator,
					 PMT_TOTAL_CLUSTER_NUM * sizeof(unsigned int),
					 "param->u4PMTIndicator");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->u4PMTCache,
					 PMT_CACHE_NUM * sizeof(unsigned int) << dev->pm_per_io,
					 "param->u4PMTCache");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->u4MetaPMTCache,
					 PMT_CACHE_NUM * sizeof(unsigned int) << dev->pm_per_io,
					 "param->u4MetaPMTCache");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer((unsigned int **)&param->i4CurrentPMTClusterInCache,
					 PMT_CACHE_NUM * sizeof(unsigned int),
					 "param->i4CurrentPMTClusterInCache");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->u4ReadPMTCache,
					 sizeof(unsigned int) << dev->pm_per_io,
					 "param->u4ReadPMTCache");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->u4ReadMetaPMTCache,
					 sizeof(unsigned int) << dev->pm_per_io,
					 "param->u4ReadMetaPMTCache");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->u4BIT,
					 NAND_TOTAL_BLOCK_NUM * sizeof(unsigned int),
					 "param->u4BIT");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer((unsigned int **)&param->u1DataCache,
					 dev->min_io_size * sizeof(unsigned char),
					 "param->u1DataCache");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer((unsigned int **)&param->u4Header,
					 DATA_NUM_PER_PAGE *
					 sizeof(struct mt_ftl_data_header), "param->u4Header");
	if (ret)
		return ret;

	/* ret = mt_ftl_alloc_single_buffer((unsigned int **)&param->u4ReadHeader,
					 DATA_NUM_PER_PAGE *
					 sizeof(struct mt_ftl_data_header), "param->u4ReadHeader");
	if (ret)
		return ret; */

	ret = mt_ftl_alloc_single_buffer(&param->replay_blk_rec,
					 dev->max_replay_blks * sizeof(unsigned int),
					 "param->replay_blk_rec");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->replay_pmt_blk_rec,
					MAX_PMT_REPLAY_BLOCKS * sizeof(unsigned int),
					"param->replay_pmt_blk_rec");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->general_page_buffer,
					 (dev->min_io_size >> 2) * sizeof(unsigned int),
					 "param->general_page_buffer");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->replay_page_buffer,
					 (dev->min_io_size >> 2) * sizeof(unsigned int),
					 "param->replay_page_buffer");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->commit_page_buffer,
					 (dev->min_io_size >> 2) * sizeof(unsigned int),
					 "param->commit_page_buffer");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->gc_page_buffer,
					 (dev->min_io_size >> 2) * sizeof(unsigned int),
					 "param->gc_page_buffer");
	if (ret)
		return ret;
	ret = mt_ftl_alloc_single_buffer(&param->gc_pmt_buffer,
					 (dev->min_io_size >> 1) * sizeof(unsigned int),
					 "param->gc_pmt_buffer");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->tmp_page_buffer,
					 (dev->min_io_size >> 2) * sizeof(unsigned int),
					 "param->tmp_page_buffer");
	if (ret)
		return ret;

	return ret;
}

static int mt_ftl_check_img_reload(struct mt_ftl_blk *dev, int leb)
{
	int ret = MT_FTL_SUCCESS;
	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_param *param = dev->param;
	int offset = 0;
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;

	ret = ubi_is_mapped(desc, leb);
	if (ret == 0) {
		mt_ftl_err(dev, "leb%d is unmapped", leb);
		return MT_FTL_FAIL;
	}
	while (offset <= max_offset_per_block) {
		ret = mt_ftl_leb_read(dev, leb, param->tmp_page_buffer, offset, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		if (param->tmp_page_buffer[0] == 0x00000000) {
			mt_ftl_err(dev, "image reloaded, offset = %d", offset);
			return 1;
		}
		offset += dev->min_io_size;
	}

	mt_ftl_debug(dev, "image not reloaded offset = %d", offset);
	return 0;
}

static int mt_ftl_recover_blk(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int offset = 0;
	struct mt_ftl_param *param = dev->param;
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;
	/* Recover Config Block */
	ret = mt_ftl_leb_read(dev, CONFIG_START_BLOCK, param->general_page_buffer, offset, dev->min_io_size);
	if (ret)
		return MT_FTL_FAIL;
	mt_ftl_err(dev, "param->general_page_buffer[0] = 0x%x, MT_MAGIC_NUMBER = 0x%x",
		param->general_page_buffer[0], MT_MAGIC_NUMBER);
	mt_ftl_leb_remap(dev, CONFIG_START_BLOCK);

	mt_ftl_write_to_blk(dev, CONFIG_START_BLOCK, param->general_page_buffer, &offset);

	/* Recover Backup Config Block */
	mt_ftl_leb_remap(dev, CONFIG_START_BLOCK + 1);

	/* Recover Replay Blocks */
	for (i = 0; i < REPLAY_BLOCK_NUM; i++)
		mt_ftl_leb_remap(dev, REPLAY_BLOCK + i);

	/* Recover PMT Blocks */
	offset = 0;
	for (i = PMT_START_BLOCK + 1; i < PMT_START_BLOCK + PMT_BLOCK_NUM; i++)
		mt_ftl_leb_remap(dev, i);
	while (offset <= max_offset_per_block) {
		ret = mt_ftl_leb_read(dev, PMT_START_BLOCK, param->general_page_buffer, offset, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		if (param->general_page_buffer[0] == 0x00000000) {
			mt_ftl_err(dev, "offset = %d, page = %d", offset, offset / dev->min_io_size);
			break;
		}
		ret = mt_ftl_leb_write(dev, PMT_START_BLOCK + 1, param->general_page_buffer, offset, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		offset += dev->min_io_size;
	}
	offset = 0;
	mt_ftl_leb_remap(dev, PMT_START_BLOCK);
	while (offset <= max_offset_per_block) {
		ret = mt_ftl_leb_read(dev, PMT_START_BLOCK + 1, param->general_page_buffer, offset, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		if (param->general_page_buffer[0] == 0xFFFFFFFF) {
			mt_ftl_err(dev, "offset = %d, page = %d", offset, offset / dev->min_io_size);
			break;
		}
		ret = mt_ftl_leb_write(dev, PMT_START_BLOCK, param->general_page_buffer, offset, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		offset += dev->min_io_size;
	}
	mt_ftl_leb_remap(dev, PMT_START_BLOCK + 1);

	return ret;
}

int mt_ftl_show_param(struct mt_ftl_blk *dev)
{
	struct mt_ftl_param *param = dev->param;

	mt_ftl_err(dev, "u4NextPageOffsetIndicator = 0x%x", param->u4NextPageOffsetIndicator);
	mt_ftl_err(dev, "u4PMTChargeLebIndicator = 0x%x", param->u4PMTChargeLebIndicator);
	mt_ftl_err(dev, "u4BlockDeviceModeFlag = 0x%x", param->u4BlockDeviceModeFlag);
	mt_ftl_err(dev, "u4NextReplayOffsetIndicator = 0x%x", param->u4NextReplayOffsetIndicator);
	mt_ftl_err(dev, "u4NextLebPageIndicator = 0x%x", param->u4NextLebPageIndicator);
	mt_ftl_err(dev, "u4CurrentPMTLebPageIndicator = 0x%x", param->u4CurrentPMTLebPageIndicator);
	mt_ftl_err(dev, "u4NextFreeLebIndicator = 0x%x", param->u4NextFreeLebIndicator);
	mt_ftl_err(dev, "u4NextFreePMTLebIndicator = 0x%x", param->u4NextFreePMTLebIndicator);
	mt_ftl_err(dev, "u4GCReserveLeb = 0x%x", param->u4GCReserveLeb);
	mt_ftl_err(dev, "u4GCReservePMTLeb = 0x%x", param->u4GCReservePMTLeb);
	mt_ftl_err(dev, "u4PMTIndicator = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
		   param->u4PMTIndicator[0],
		   param->u4PMTIndicator[1],
		   param->u4PMTIndicator[2],
		   param->u4PMTIndicator[3],
		   param->u4PMTIndicator[4],
		   param->u4PMTIndicator[5],
		   param->u4PMTIndicator[6],
		   param->u4PMTIndicator[7]);
	mt_ftl_err(dev, "u4BIT = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
		   param->u4BIT[3], param->u4BIT[4], param->u4BIT[5],
		   param->u4BIT[6], param->u4BIT[7], param->u4BIT[8]);
	mt_ftl_err(dev, "i4CurrentPMTClusterInCache = 0x%x, 0x%x, 0x%x, 0x%x",
		   param->i4CurrentPMTClusterInCache[0],
		   param->i4CurrentPMTClusterInCache[1],
		   param->i4CurrentPMTClusterInCache[2],
		   param->i4CurrentPMTClusterInCache[3]);

	return MT_FTL_SUCCESS;
}

static int mt_ftl_param_default(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_param *param = dev->param;

	param->u4DataNum = 0;
	param->replay_blk_index = 0;
	param->replay_pmt_blk_index = 0;
	param->i4CommitOffsetIndicator0 = 0;
	param->i4CommitOffsetIndicator1 = 0;
	param->i4CurrentReadPMTClusterInCache = 0xFFFFFFFF;
#ifdef CONFIG_UBIFS_FS_LZ4K
	param->cc = crypto_alloc_comp("lz4k", 0, 0);
	param->u4ComprType = FTL_COMPR_LZ4K;
#else
	param->cc = crypto_alloc_comp("lzo", 0, 0);
	param->u4ComprType = FTL_COMPR_LZO;
#endif
	if (IS_ERR(param->cc)) {
		mt_ftl_err(dev, "init LZO/LZ4K fail");
		return MT_FTL_FAIL;
	}
	if (param->cc) {
		ret = mt_ftl_alloc_single_buffer((unsigned int **)&param->cmpr_page_buffer,
						 dev->min_io_size * sizeof(unsigned char),
						 "param->cmpr_page_buffer");
		if (ret)
			return ret;
	}
	param->u4NextReplayOffsetIndicator = 0;
	param->u4NextPageOffsetIndicator = 0;
	if (dev->readonly)
		param->u4BlockDeviceModeFlag = 1; /* reset Block device flag to read only*/
	else
		param->u4BlockDeviceModeFlag = 0;

	/* There are some download information stored in some blocks
	   So unmap the blocks at first */
	for (i = 0; i < dev->dev_blocks; i++)
		ubi_leb_unmap(desc, i);

	ret = mt_ftl_leb_map(dev, CONFIG_START_BLOCK);
	ret = mt_ftl_leb_map(dev, CONFIG_START_BLOCK + 1);
	for (i = 0; i < REPLAY_BLOCK_NUM; i++)
		ret = mt_ftl_leb_map(dev, REPLAY_BLOCK + i);

	PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(param->u4NextLebPageIndicator, DATA_START_BLOCK, 0);
	mt_ftl_err(dev, "u4NextLebPageIndicator = 0x%x", param->u4NextLebPageIndicator);
	ret = mt_ftl_leb_map(dev, DATA_START_BLOCK);

	PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(param->u4CurrentPMTLebPageIndicator, PMT_START_BLOCK, 0);
	mt_ftl_err(dev, "u4CurrentPMTLebPageIndicator = 0x%x", param->u4CurrentPMTLebPageIndicator);
	ret = mt_ftl_leb_map(dev, PMT_START_BLOCK);

	param->u4NextFreeLebIndicator = DATA_START_BLOCK + 1;
	mt_ftl_err(dev, "u4NextFreeLebIndicator = 0x%x", param->u4NextFreeLebIndicator);
	param->u4NextFreePMTLebIndicator = PMT_START_BLOCK + 1;
	mt_ftl_err(dev, "u4NextFreePMTLebIndicator = %d", param->u4NextFreePMTLebIndicator);
	param->u4GCReserveLeb = dev->dev_blocks - 1;
	mt_ftl_err(dev, "u4GCReserveLeb = %d", param->u4GCReserveLeb);
	ret = mt_ftl_leb_map(dev, param->u4GCReserveLeb);
	param->u4GCReservePMTLeb = DATA_START_BLOCK - 2;
	ret = mt_ftl_leb_map(dev, param->u4GCReservePMTLeb);
	param->u4PMTChargeLebIndicator = DATA_START_BLOCK - 1;
	ret = mt_ftl_leb_map(dev, param->u4PMTChargeLebIndicator);

	memset(param->u4PMTIndicator, 0, PMT_TOTAL_CLUSTER_NUM * sizeof(unsigned int));
	memset(param->u4BIT, 0, NAND_TOTAL_BLOCK_NUM * sizeof(unsigned int));

	mt_ftl_show_param(dev);

	/* Replay */
	ret = mt_ftl_replay(dev);
	if (ret) {
		mt_ftl_err(dev, "mt_ftl_replay fail, ret = %d", ret);
		return ret;
	}

	mt_ftl_show_param(dev);

	/* Commit indicators */
	mt_ftl_commit_indicators(dev);
	if (ret) {
		mt_ftl_err(dev, "mt_ftl_commit_indicators fail, ret = %d", ret);
		return ret;
	}

	param->replay_pmt_blk_rec[param->replay_pmt_blk_index]
		= PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4CurrentPMTLebPageIndicator);
	param->replay_pmt_blk_index++;

	return ret;
}

static int mt_ftl_param_init(struct mt_ftl_blk *dev, u32 *buffer)
{
	int ret = MT_FTL_SUCCESS;
	int index = 0;
	struct mt_ftl_param *param = dev->param;

	param->u4DataNum = 0;
	param->replay_blk_index = 0;
	param->replay_pmt_blk_index = 0;
	param->i4CurrentReadPMTClusterInCache = 0xFFFFFFFF;
	param->u4NextPageOffsetIndicator = 0;
	param->u4PMTChargeLebIndicator = 0;

	index++; /* index = 0  MT_MAGIC_NUMBER */
	param->u4ComprType = buffer[index++];
	switch (param->u4ComprType) {
	case FTL_COMPR_LZO:
		param->cc = crypto_alloc_comp("lzo", 0, 0);
		mt_ftl_err(dev, "Using LZO");
		if (IS_ERR(param->cc)) {
			mt_ftl_err(dev, "init LZO fail");
			return MT_FTL_FAIL;
		}
		break;
	case FTL_COMPR_LZ4K:
		param->cc = crypto_alloc_comp("lz4k", 0, 0);
		mt_ftl_err(dev, "Using LZ4K");
		if (IS_ERR(param->cc)) {
			mt_ftl_err(dev, "init LZ4K fail");
			return MT_FTL_FAIL;
		}
		break;
	default:
		param->cc = NULL;
		mt_ftl_err(dev, "NO COMPR TYPE");
	}

	if (param->cc) {
		ret = mt_ftl_alloc_single_buffer((unsigned int **)&param->cmpr_page_buffer,
						 dev->min_io_size * sizeof(unsigned char),
						 "param->cmpr_page_buffer");
		if (ret)
			return ret;
	}

	param->u4BlockDeviceModeFlag = buffer[index++];
	param->u4NextReplayOffsetIndicator = buffer[index++];
	param->u4NextLebPageIndicator = buffer[index++];
	param->u4CurrentPMTLebPageIndicator = buffer[index++];
	param->u4NextFreeLebIndicator = buffer[index++];
	param->u4NextFreePMTLebIndicator = buffer[index++];
	param->u4GCReserveLeb = buffer[index++];
	if (param->u4GCReserveLeb == NAND_DEFAULT_VALUE)
		param->u4GCReserveLeb = dev->dev_blocks - 1;
	param->u4GCReservePMTLeb = buffer[index++];
	if (param->u4GCReservePMTLeb == NAND_DEFAULT_VALUE)
		param->u4GCReservePMTLeb = DATA_START_BLOCK - 2;
	param->u4PMTChargeLebIndicator = buffer[index++];
	if (param->u4PMTChargeLebIndicator == NAND_DEFAULT_VALUE)
		param->u4PMTChargeLebIndicator = DATA_START_BLOCK - 1;

	if ((index + PMT_TOTAL_CLUSTER_NUM) * sizeof(unsigned int) > dev->min_io_size) {
		mt_ftl_err(dev, "(index + PMT_TOTAL_CLUSTER_NUM) * sizeof(unsigned int)(0x%x) > NAND_PAGE_SIZE(%d)",
			   (u32)((index + PMT_TOTAL_CLUSTER_NUM) * sizeof(unsigned int)), dev->min_io_size);
		mt_ftl_err(dev, "index = %d, PMT_TOTAL_CLUSTER_NUM = %d", index, PMT_TOTAL_CLUSTER_NUM);
		return MT_FTL_FAIL;
	}
	memcpy(param->u4PMTIndicator, &buffer[index], PMT_TOTAL_CLUSTER_NUM * sizeof(unsigned int));

	index += ((PMT_TOTAL_CLUSTER_NUM * sizeof(unsigned int)) >> 2);
	if ((index + NAND_TOTAL_BLOCK_NUM) * sizeof(unsigned int) > dev->min_io_size) {
		mt_ftl_err(dev, "(index + NAND_TOTAL_BLOCK_NUM) * sizeof(unsigned int)(0x%x) > NAND_PAGE_SIZE(%d)",
			   (u32)((index + NAND_TOTAL_BLOCK_NUM) * sizeof(unsigned int)), dev->min_io_size);
		mt_ftl_err(dev, "index = %d, NAND_TOTAL_BLOCK_NUM = %d", index, NAND_TOTAL_BLOCK_NUM);
		return MT_FTL_FAIL;
	}
	memcpy(param->u4BIT, &buffer[index], NAND_TOTAL_BLOCK_NUM * sizeof(unsigned int));

	index += ((NAND_TOTAL_BLOCK_NUM * sizeof(unsigned int)) >> 2);
	if ((index + PMT_CACHE_NUM) * sizeof(unsigned int) > dev->min_io_size) {
		mt_ftl_err(dev, "(index + PMT_CACHE_NUM) * sizeof(unsigned int)(0x%x) > NAND_PAGE_SIZE(%d)",
			   (u32)((index + PMT_CACHE_NUM) * sizeof(unsigned int)), dev->min_io_size);
		mt_ftl_err(dev, "index = %d, PMT_CACHE_NUM = %d", index, PMT_CACHE_NUM);
		return MT_FTL_FAIL;
	}
	memcpy(param->i4CurrentPMTClusterInCache, &buffer[index],
	       PMT_CACHE_NUM * sizeof(unsigned int));

	mt_ftl_show_param(dev);

	/* Replay */
	ret = mt_ftl_replay(dev);
	if (ret) {
		mt_ftl_err(dev, "mt_ftl_replay fail, ret = %d", ret);
		return ret;
	}
	if (dev->readonly)
		param->u4BlockDeviceModeFlag = 1; /* reset Block device flag to read only*/

	param->replay_pmt_blk_rec[param->replay_pmt_blk_index]
		= PMT_LEB_PAGE_INDICATOR_GET_BLOCK(param->u4CurrentPMTLebPageIndicator);
	param->replay_pmt_blk_index++;

	mt_ftl_show_param(dev);

	return ret;
}

static void mt_ftl_flag_init(struct mt_ftl_blk *dev)
{
	struct mt_ftl_param *param = dev->param;

	/* recorded the pmt block for recovery */
	param->recorded_pmt_blk = -1;
	/* recorded the last replay for pmt recover*/
	param->last_replay_flag = REPLAY_EMPTY;
	/* recorded the last data's pmt for pmt recover*/
	param->pmt_updated_flag = PMT_END;
	/* recorded pmt get free blk for commit*/
	param->get_pmt_blk_flag = 0;
	/* recorded gc data updatePMT for commit*/
	param->gc_dat_cmit_flag = 0;
	/* recorded pmt charge blk for gc pmt */
	param->gc_pmt_charge_blk = -1;
}

int mt_ftl_create(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, i;
	int leb = 0, offset = 0, img_reload = 0;
	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_param *param = dev->param;

#ifdef MT_FTL_PROFILE
	memset(profile_time, 0, sizeof(profile_time));
#endif
	mt_ftl_flag_init(dev);

	/* Allocate buffers for FTL usage */
	ret = mt_ftl_alloc_buffers(dev);
	if (ret)
		return ret;
	/* Allocate wbuf for FTL */
	ret = mt_ftl_wbuf_alloc(dev);
	if (ret)
		return ret;

	/* Check the mapping of CONFIG and REPLAY block */
	ret = ubi_is_mapped(desc, CONFIG_START_BLOCK);
	if (ret == 0) {
		ret = ubi_is_mapped(desc, CONFIG_START_BLOCK + 1);
		if (ret)
			leb = CONFIG_START_BLOCK + 1;
		else {
			mt_ftl_err(dev, "leb%d/leb%d are both unmapped", CONFIG_START_BLOCK,
				   CONFIG_START_BLOCK + 1);
			ubi_leb_map(desc, CONFIG_START_BLOCK);
			ubi_leb_map(desc, CONFIG_START_BLOCK + 1);
			leb = CONFIG_START_BLOCK;
		}
	} else {
		leb = CONFIG_START_BLOCK;
	}

	for (i = 0; i < REPLAY_BLOCK_NUM; i++)
		ret = mt_ftl_leb_map(dev, REPLAY_BLOCK + i);

	/* Check if system.img/usrdata.img is just reloaded */
	img_reload = mt_ftl_check_img_reload(dev, CONFIG_START_BLOCK);
	if (img_reload < 0) {
		mt_ftl_err(dev, "mt_ftl_check_img_reload fail, ret = %d", img_reload);
		return MT_FTL_FAIL;
	}

	if (img_reload) {
		mt_ftl_err(dev, "system or usrdata image is reloaded");
		ret = mt_ftl_recover_blk(dev);
		if (ret) {
			mt_ftl_err(dev, "recover block fail");
			return ret;
		}
	}

	/* Get lastest config page */
	offset = mt_ftl_leb_lastpage_offset(dev, leb, 0);

	if (offset == 0) {
		if ((leb == CONFIG_START_BLOCK) && ubi_is_mapped(desc, CONFIG_START_BLOCK + 1))
			leb = CONFIG_START_BLOCK + 1;
		offset = mt_ftl_leb_lastpage_offset(dev, leb, 0);
		if (offset == 0) {
			mt_ftl_err(dev, "Config blocks are empty");
			ret = mt_ftl_param_default(dev);
			if (ret)
				mt_ftl_err(dev, "mt_ftl_param_default fail, ret = %d", ret);
			return ret;
		}
	}

	offset -= dev->min_io_size;
	/* Grab configs */
	mt_ftl_err(dev, "Get config page, leb:%d, page:%d", leb, offset / dev->min_io_size);
	ret = mt_ftl_leb_read(dev, leb, param->general_page_buffer, offset, dev->min_io_size);
	if (ret)
		return MT_FTL_FAIL;

	/* Sync to backup CONFIG block */
	if (leb == CONFIG_START_BLOCK) {
		param->i4CommitOffsetIndicator0 = offset;
		mt_ftl_write_to_blk(dev, CONFIG_START_BLOCK + 1, param->general_page_buffer,
			&param->i4CommitOffsetIndicator1);
	} else {
		param->i4CommitOffsetIndicator1 = offset;
		mt_ftl_write_to_blk(dev, CONFIG_START_BLOCK, param->general_page_buffer,
			&param->i4CommitOffsetIndicator0);
	}

	/* Init param */
	ret = mt_ftl_param_init(dev, param->general_page_buffer);
	if (ret) {
		mt_ftl_err(dev, "mt_ftl_param_init fail, ret = %d", ret);
		return ret;
	}

	return ret;
}

/* TODO: Tracking remove process to make sure mt_ftl_remove can be called during shut down */
int mt_ftl_remove(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;
#ifdef MT_FTL_PROFILE
	int i = 0;

	for (i = 0; i < MT_FTL_PROFILE_TOTAL_PROFILE_NUM; i++)
		mt_ftl_err(dev, "%s = %lu ms", mtk_ftl_profile_message[i], profile_time[i] / 1000);
#endif				/* PROFILE */
	mt_ftl_err(dev, "Enter");
	mt_ftl_commit(dev);

	if (dev->param->cc)
		crypto_free_comp(dev->param->cc);

	mt_ftl_wbuf_free(dev);
	mt_ftl_err(dev, "mt_ftl_commit done");

	return ret;
}
