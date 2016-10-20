/*
 * Copyright (c) 2014 MediaTek Inc.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h> /* copy_from_user()... */

#include "gpad_common.h"
#include "gpad_ioctl.h"
#include "gpad_hal.h"
#include "gpad_pm.h"
#include "gpad_pm_config.h"
#include "gpad_sdbg.h"
#include "gpad_crypt.h"

#if defined(GPAD_SUPPORT_DFD)
#include "gpad_dfd.h"
#endif /* defined(GPAD_SUPPORT_DFD) */

#if defined(GPAD_SUPPORT_DVFS)
#include "gpad_gpufreq_utst.h"
#include "gpad_ipem_utst.h"
#endif /* defined(GPAD_SUPPORT_DVFS) */

static long gpad_ioc_get_mod_info(struct gpad_device *dev, void __user *user_arg)
{
	struct gpad_mod_info    info;

	info.version = GPAD_VERSION;

	if (copy_to_user(user_arg, &info, sizeof(info))) {
		GPAD_LOG_ERR("GPAD_IOC_GET_MOD_INFO: failed to write %zd-byte to user_arg %p!\n", sizeof(info), user_arg);
		return -EINVAL;
	}

	return 0;
}

static long gpad_ioc_read_reg(struct gpad_device *dev, void __user *user_arg)
{
	struct gpad_reg_rw  req;
	int                 size = sizeof(req);

	if (copy_from_user(&req, user_arg, size)) {
		GPAD_LOG_ERR("GPAD_IOC_READ_REG: failed to read %d-byte from user_arg %p!\n", size, user_arg);
		return -EINVAL;
	}

	switch (req.size) {
	case 1:
		req.value = (unsigned int)gpad_hal_read8(req.offset);
		break;
	case 2:
		req.value = (unsigned int)gpad_hal_read16(req.offset);
		break;
	case 4:
		req.value = (unsigned int)gpad_hal_read32(req.offset);
		break;
	default:
		GPAD_LOG_ERR("GPAD_IOC_READ_REG: invalid size=%d for offset=0x%08X\n",  req.size, req.offset);
		return -EINVAL;
	}

	GPAD_LOG_LOUD("GPAD_IOC_READ_REG: offset=0x%08X, value=0x%08X\n", req.offset, req.value);

	if (copy_to_user(user_arg, &req, size)) {
		GPAD_LOG_ERR("GPAD_IOC_READ_REG: failed to write %d-byte to user_arg %p!\n", size, user_arg);
		return -EINVAL;
	}

	return 0;
}

static long gpad_ioc_write_reg(struct gpad_device *dev, void __user *user_arg)
{
	struct gpad_reg_rw  req;
	int                 size = sizeof(req);

	if (copy_from_user(&req, user_arg, size)) {
		GPAD_LOG_ERR("GPAD_IOC_WRITE_REG: failed to read %d-byte from user_arg %p!\n", size, user_arg);
		return -EINVAL;
	}

	switch (req.size) {
	case 1:
		gpad_hal_write8(req.offset, (unsigned char)req.value);
		break;
	case 2:
		gpad_hal_write16(req.offset, (unsigned short)req.value);
		break;
	case 4:
		gpad_hal_write32(req.offset, (unsigned int)req.value);
		break;
	default:
		GPAD_LOG_ERR("GPAD_IOC_WRITE_REG: invalid size=%d for offset=0x%08X, value=0x%08X\n",  req.size, req.offset, req.value);
		return -EINVAL;
	}

	GPAD_LOG_LOUD("GPAD_IOC_WRITE_REG: offset=0x%08X, value=0x%08X\n", req.offset, req.value);
	return 0;
}

static long gpad_ioc_toggle_reg(struct gpad_device *dev, void __user *user_arg)
{
	struct gpad_reg_toggle  req;
	unsigned int            value;

	if (copy_from_user(&req, user_arg, sizeof(req))) {
		GPAD_LOG_ERR("GPAD_IOC_TOGGLE_REG: failed to read %zd-byte from user_arg %p!\n", sizeof(req), user_arg);
		return -EINVAL;
	}

	value = gpad_hal_read32(req.offset);
	gpad_hal_write32(req.offset, (value ^ (0x1 << req.offset)));
	gpad_hal_write32(req.offset, value);

	GPAD_LOG_LOUD("GPAD_IOC_TOGGLE_REG: offset=0x%08X, bit=%d\n", req.offset, req.bit);
	return 0;
}

static long gpad_ioc_pm_read_counters(struct gpad_device *dev, void __user *user_arg)
{
	struct gpad_buf_read        req;
	struct gpad_pm_counters    *user_out;
	unsigned int                alloc_cnt;
	unsigned int                read_cnt;
	struct gpad_pm_sample_list *sample_list;

	if (copy_from_user(&req, user_arg, sizeof(req))) {
		GPAD_LOG_ERR("GPAD_IOC_PM_READ_COUNTERS: failed to read %zd-byte from user_arg %p!\n", sizeof(req), user_arg);
		return -EINVAL;
	}

	user_out = (struct gpad_pm_counters *)((struct gpad_buf_read *)user_arg + 1);
	alloc_cnt = req.bytes_alloc / sizeof(struct gpad_pm_counters);
	for (read_cnt = 0; read_cnt < alloc_cnt; read_cnt++) {
		sample_list = gpad_pm_read_sample_list(dev);
		if (sample_list) {
			if (unlikely(copy_to_user(&user_out->timestamp_ns,   &sample_list->timestamp_ns,   sizeof(sample_list->timestamp_ns)))) {
				GPAD_LOG_ERR("GPAD_IOC_PM_READ_COUNTERS: failed to timestamp_ns (0x%llx) to %p!\n",  sample_list->timestamp_ns, &user_out->timestamp_ns);
				gpad_pm_complete_sample_list(dev, sample_list);
				break;
			}

			if (unlikely(copy_to_user(&user_out->sample,   &sample_list->sample,   sizeof(sample_list->sample)))) {
				GPAD_LOG_ERR("GPAD_IOC_PM_READ_COUNTERS: failed copy sample (%x) to %p!\n",  sample_list->sample.header.frame_id, &user_out->sample);
				gpad_pm_complete_sample_list(dev, sample_list);
				break;
			}

			if (unlikely(copy_to_user(&user_out->freq,   &sample_list->freq,   sizeof(sample_list->freq)))) {
				GPAD_LOG_ERR("GPAD_IOC_PM_READ_COUNTERS: failed copy freq!\n");
				gpad_pm_complete_sample_list(dev, sample_list);
				break;
			}


			gpad_pm_complete_sample_list(dev, sample_list);
			user_out++;
		} else {
			break;
		}
	}

	req.bytes_read = read_cnt * sizeof(struct gpad_pm_counters);
	if (copy_to_user(user_arg, &req, sizeof(req))) {
		GPAD_LOG_ERR("GPAD_IOC_PM_READ_COUNTERS: failed to write %zd-byte to user_arg %p!\n", sizeof(req), user_arg);
		return -EINVAL;
	}

	return 0;
}

static long gpad_ioc_get_state(struct gpad_device *dev, void __user *user_arg)
{
	enum gpad_pm_state    state;

	state = gpad_pm_get_state(dev);
	if (copy_to_user(user_arg, &state, sizeof(state))) {
		GPAD_LOG_ERR("GPAD_IOC_PM_GET_STATE: failed to write %zd-byte to user_arg %p!\n", sizeof(state), user_arg);
		return -EINVAL;
	}

	return 0;
}

static long gpad_ioc_set_state(struct gpad_device *dev, void __user *user_arg)
{
	enum gpad_pm_state    state;

	if (copy_from_user(&state, user_arg, sizeof(state))) {
		GPAD_LOG_ERR("GPAD_IOC_PM_SET_STATE: failed to read %zd-byte from user_arg %p!\n", sizeof(state), user_arg);
		return -EINVAL;
	}

	switch (state) {
	case GPAD_PM_DISABLED:
		gpad_pm_disable(dev);
		break;

	case GPAD_PM_ENABLED:
		gpad_pm_enable(dev);
		break;

	default:
		GPAD_LOG_ERR("GPAD_IOC_PM_SET_STATE: invalid state %d to set!\n", (int)state);
		return -EINVAL;
	}

	return 0;
}

static long gpad_ioc_get_config(struct gpad_device *dev, void __user *user_arg)
{
	unsigned int            config;

	config = gpad_pm_get_config(dev);
	if (copy_to_user(user_arg, &config, sizeof(config))) {
		GPAD_LOG_ERR("GPAD_IOC_PM_GET_CONFIG: failed to write %zd-byte to user_arg %p!\n", sizeof(config), user_arg);
		return -EINVAL;
	}
	return 0;
}

static long gpad_ioc_set_config(struct gpad_device *dev, void __user *user_arg)
{
	unsigned int            config;

	if (copy_from_user(&config, user_arg, sizeof(config))) {
		GPAD_LOG_ERR("GPAD_IOC_PM_SET_CONFIG: failed to read %zd-byte from user_arg %p!\n", sizeof(config), user_arg);
		return -EINVAL;
	}

	gpad_pm_reset_backup_config(&dev->pm_info);
	return gpad_pm_set_config(dev, config);
}

static long gpad_ioc_change_output_pin(struct gpad_device *dev, void __user *user_arg)
{
	struct gpad_outpin_req outpin_req;
	unsigned int old_value;

	if (unlikely(copy_from_user(&outpin_req, user_arg, sizeof(outpin_req)))) {
		GPAD_LOG_ERR("GPAD_IOC_CHANGE_OUTPUT_PIN: failed to read %zd-byte from user_arg %p!\n", sizeof(outpin_req), user_arg);
		return -EINVAL;
	}

	old_value = gpad_hal_read32(0x1f00);
	switch (outpin_req.act) {
	case GPAD_OUTPIN_ACT_PULSE:
		gpad_hal_write32(0x1f00, (old_value | (0x1 << (24 + outpin_req.pin))));
		gpad_hal_write32(0x1f00, old_value);
		break;

	case GPAD_OUTPIN_ACT_UP:
		gpad_hal_write32(0x1f00, (old_value | (0x1 << (24 + outpin_req.pin))));
		break;

	case GPAD_OUTPIN_ACT_DOWN:
		gpad_hal_write32(0x1f00, (old_value & (~(0x1 << (24 + outpin_req.pin)))));
		break;

	default:
		break;
	}

	return 0;
}

static long gpad_ioc_get_timestamp(struct gpad_device *dev, void __user *user_arg)
{
	unsigned int            gpu_ts;

	gpu_ts = gpad_hal_read32(0x1f40);
	if (copy_to_user(user_arg, &gpu_ts, sizeof(gpu_ts))) {
		GPAD_LOG_ERR("GPAD_IOC_PM_GET_TIMESTAMP: failed to write %zd-byte to user_arg %p!\n", sizeof(gpu_ts), user_arg);
		return -EINVAL;
	}

	return 0;
}

static long gpad_ioc_dump_info(struct gpad_device *dev, void __user *user_arg)
{
	unsigned int level;
	struct gpad_sdbg_info *gpad_sdbg_info;
	gpad_sdbg_info = &(dev->sdbg_info);
	if (unlikely(copy_from_user(&level, user_arg, sizeof(level)))) {
		GPAD_LOG_ERR("GPAD_IOC_DUMP_INFO: failed to read %zd-byte from user_arg %p!\n", sizeof(unsigned int), user_arg);
		return -EINVAL;
	}
	gpad_sdbg_info->level = level;
	GPAD_LOG_ERR("GPAD_IOC_DUMP_INFO: level = %d\n", gpad_sdbg_info->level);
	if (level == DUMP_DBGREG_LEVEL)
		gpad_dfd_save_debug_info();
	else
		gpad_sdbg_save_sdbg_info(gpad_sdbg_info->level);

	return 0;

}

static long gpad_ioc_sdbg_update_key(struct gpad_device *dev, void __user *user_arg)
{
	int            result   = 0;
	unsigned char *key      = NULL;
	int            key_len  = GPAD_CRYPT_KEY_SIZE;

	key = (unsigned char *)kzalloc(key_len, GFP_KERNEL);
	if (NULL == key) {
		GPAD_LOG_ERR("%s: allocate public key failed\n", __func__);
		return -ENOMEM;
	}

	/* process */
	result = gpad_crypt_update_key(key, key_len);

	if (0 > result) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_UPDATE_KEY: failed to update key\n");
	}

	/* return data */
	if (copy_to_user(user_arg, key, key_len)) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_UPDATE_KEY: failed to write %d-byte to user_arg %p!\n",  key_len, user_arg);
		kfree(key);
		return -EINVAL;
	}

	kfree(key);
	return 0;

}

static long gpad_ioc_sdbg_read_mem(struct gpad_device *dev, void __user *user_arg)
{
	int                  ret = 0;
	struct gpad_mem_ctx  ctx;
	int                  ctx_size;
	unsigned char       *enc_ctx = NULL;
	int                  enc_ctx_size;
	int                  data_size;
	unsigned char       *data = NULL;
	unsigned int         magic = 0x12345678;

	ctx_size = sizeof(ctx);
	enc_ctx_size = GPAD_AES_BLOCK_SIZE;

	enc_ctx = (unsigned char *)kzalloc(enc_ctx_size, GFP_KERNEL);
	if (NULL == enc_ctx) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_READ_MEM: failed to allocate %d-byte data\n", enc_ctx_size);
		return -ENOMEM;
	}

	/* copy data from user */
	if (unlikely(copy_from_user(enc_ctx, user_arg, enc_ctx_size))) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_READ_MEM: failed to read %zd-byte from user_arg %p!\n", sizeof(unsigned int), user_arg);
		ret = -EINVAL;
		goto finish;
	}

	/* decrypt data */
	ret = gpad_crypt_decrypt(enc_ctx, enc_ctx_size, (unsigned char *)&ctx, sizeof(ctx));
	if (0 != ret || ctx.magic != magic) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_READ_MEM: decrypt data failed: %d\n", ret-1);
		ret = -EINVAL;
		goto finish;
	}

	if (ctx.size > 256) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_READ_MEM: max read mem size id 256\n");
		ctx.size = 256;
	}

	data_size = ctx.size + enc_ctx_size;
	data = (char *)kzalloc(data_size, GFP_KERNEL);
	if (NULL == data) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_READ_MEM: failed to allocate %d-byte data\n", data_size);
		ret = -ENOMEM;
		goto finish;
	}

	/* process */
	*((unsigned int *)data) = (unsigned int)ctx.size;
	ret = gpad_hal_read_mem(ctx.addr, ctx.size, ctx.type, (unsigned int *)(data+enc_ctx_size));
	if (ret != 0) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_READ_MEM: failed to read mem\n");
		goto finish;
	}

	/* return data */
	if (copy_to_user(user_arg, data, data_size)) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_READ_MEM: failed to write %d-byte to user_arg %p!\n",  data_size, user_arg+ctx_size);
		ret = -EINVAL;
	}

finish:
	kfree(enc_ctx);
	kfree(data);

	/* return data */
	return ret;
}

static long gpad_ioc_sdbg_write_mem(struct gpad_device *dev, void __user *user_arg)
{
	int                  ret = 0;
	struct gpad_mem_ctx  ctx;
	unsigned char       *enc_ctx = NULL;
	int                  enc_ctx_size;
	unsigned int         magic = 0x12345678;

	enc_ctx_size = GPAD_AES_BLOCK_SIZE;

	enc_ctx = (unsigned char *)kzalloc(enc_ctx_size, GFP_KERNEL);
	if (NULL == enc_ctx) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_WRITE_MEM: failed to allocate %d-byte data\n", enc_ctx_size);
		return -ENOMEM;
	}

	/* copy data from user */
	if (unlikely(copy_from_user(enc_ctx, user_arg, enc_ctx_size))) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_WRITE_MEM: failed to read %zd-byte from user_arg %p!\n", sizeof(unsigned int), user_arg);
		ret = -EINVAL;
		goto finish;
	}

	/* decrypt data */
	ret = gpad_crypt_decrypt(enc_ctx, enc_ctx_size, (unsigned char *)&ctx, sizeof(ctx));
	if (0 != ret || ctx.magic != magic || ctx.size != sizeof(unsigned int)) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_WRITE_MEM: decrypt data failed\n");
		ret = -EINVAL;
		goto finish;
	}

	/* process */
	ret = gpad_hal_write_mem(ctx.addr, ctx.type, ctx.value);
	if (ret != 0) {
		GPAD_LOG_ERR("GPAD_IOC_SDBG_WRITE_MEM: failed to write mem\n");
		goto finish;
	}

finish:
	kfree(enc_ctx);

	/* return */
	return ret;
}

#if defined(GPAD_SUPPORT_DVFS)
static long gpad_ioc_gpufreq_utst(struct gpad_device *dev, void __user *user_arg)
{
	int param;
	int result;

	if (copy_from_user(&param, user_arg, sizeof(param))) {
		GPAD_LOG_ERR("GPAD_IOC_GPUFREQ_UTST: failed to read %zd-byte from user_arg %p!\n",  sizeof(param), user_arg);
		return -EINVAL;
	}

	result = gpad_gpufreq_utst(dev, param);

	if (copy_to_user(user_arg, &result, sizeof(result))) {
		GPAD_LOG_ERR("GPAD_IOC_GPUFREQ_UTST: failed to write %zd-byte to user_arg %p!\n",  sizeof(result), user_arg);
		return -EINVAL;
	}
	return 0;
}

static long gpad_ioc_ipem_utst(struct gpad_device *dev, void __user *user_arg)
{
	int param;
	int result;

	if (copy_from_user(&param, user_arg, sizeof(param))) {
		GPAD_LOG_ERR("GPAD_IOC_IPEM_UTST: failed to read %zd-byte from user_arg %p!\n",  sizeof(param), user_arg);
		return -EINVAL;
	}

	result = gpad_ipem_utst(dev, param);

	if (copy_to_user(user_arg, &result, sizeof(result))) {
		GPAD_LOG_ERR("GPAD_IOC_IPEM_UTST: failed to write %zd-byte to user_arg %p!\n",  sizeof(result), user_arg);
		return -EINVAL;
	}
	return 0;
}
#endif /* defined(GPAD_SUPPORT_DVFS) */

#if defined(GPAD_SUPPORT_DFD)
static long gpad_ioc_dfd_debug_mode(struct gpad_device *dev, void __user *user_arg)
{
	unsigned int mode;
	if (unlikely(copy_from_user(&mode, user_arg, sizeof(mode)))) {
		GPAD_LOG_ERR("GPAD_DFD_DEBUG_MODE: failed to read %zd-byte from user_arg %p!\n", sizeof(unsigned int), user_arg);
		return -EINVAL;
	}
	GPAD_LOG_INFO("GPAD_DFD_DEBUG_MODE: mode = %d\n", mode);
	gpad_set_dfd_debug_mode(mode);
	return 0;

}

static long gpad_ioc_dfd_dump_sram(struct gpad_device *dev, void __user *user_arg)
{
	unsigned int mode;
	if (unlikely(copy_from_user(&mode, user_arg, sizeof(mode)))) {
		GPAD_LOG_ERR("GPAD_DFD_DUMP_SRAM: failed to read %zd-byte from user_arg %p!\n", sizeof(unsigned int), user_arg);
		return -EINVAL;
	}
	GPAD_LOG_INFO("GPAD_DFD_DUMP_SRAM: mode = %d\n", mode);
	gpad_set_dfd_dump_sram(mode);
	return 0;

}

static long gpad_ioc_recev_poll(struct gpad_device *dev, void __user *user_arg)
{
	struct gpad_recev_poll debug_out;
	struct gpad_dfd_info  *ioc_dfd_info = &(dev->dfd_info);
	memset(&debug_out, 0, sizeof(debug_out));
	if (ioc_dfd_info->trigger_interrupt) {
		if (ioc_dfd_info->dump_sram) {
			if (ioc_dfd_info->fin_sram_dump) {
				debug_out.type = GPAD_DFD_INTERNAL_DUMP_SRAM_DONE;
				ioc_dfd_info->fin_sram_dump = 0;

			} else {
				debug_out.type = GPAD_DFD_INTERNAL_DUMP_SRAM;
			}
			/* No need to polling out data[0] and data[1], because using SRAM as internal dump storage*/
			debug_out.data[0] = -1;
			debug_out.data[1] = -1;
		} else {
			/* Polling internal data back */
			debug_out.type = GPAD_DFD_INTERNAL_DUMP_APB;
			debug_out.data[0] = ioc_dfd_info->debug_out.data[0];
			debug_out.data[1] = ioc_dfd_info->debug_out.data[1];
		}

		/* reset trigger interrupt flag for next internal dump interrupt */
		ioc_dfd_info->trigger_interrupt = 0;

	} else {
		if (ioc_dfd_info->dump_sram) {
			if (ioc_dfd_info->fin_sram_dump) {
				debug_out.type = GPAD_DFD_INTERNAL_DUMP_SRAM_DONE;
				ioc_dfd_info->fin_sram_dump = 0;
			} else {
				debug_out.type = GPAD_DFD_INTERNAL_DUMP_SRAM;
			}

		} else {
			debug_out.type = GPAD_DFD_INTERNAL_DUMP_APB;
		}

		debug_out.data[0] = -1;
		debug_out.data[1] = -1;
	}
	if (copy_to_user(user_arg, &debug_out, sizeof(debug_out))) {
		GPAD_LOG_ERR("GPAD_IOC_RECEV_POLL: failed to write %zd-byte to user_arg %p!\n",  sizeof(debug_out), user_arg);
		return -EINVAL;
	}
	return 0;

}

static long gpad_ioc_dfd_set_mask(struct gpad_device *dev, void __user *user_arg)
{
	unsigned int mask;
	if (unlikely(copy_from_user(&mask, user_arg, sizeof(mask)))) {
		GPAD_LOG_ERR("GPAD_IOC_DFD_SET_MASK: failed to read %zd-byte from user_arg %p!\n", sizeof(unsigned int), user_arg);
		return -EINVAL;
	}
	GPAD_LOG_INFO("GPAD_IOC_DFD_SET_MASK: mask = %d\n", mask);
	gpad_set_dfd_mask(mask);
	return 0;

}
#endif /* GPAD_SUPPORT_DFD */

static long gpad_ioc_kmd_query(struct gpad_device *dev, void __user *user_arg)
{
	char *data;
	struct gpad_kmd_query q;
	void __user *user_data;
	int value;
	int data_size;
	int i, ret;
	int buf_base;

	ret = 0;

	if (unlikely(copy_from_user(&q, user_arg, sizeof(struct gpad_kmd_query)))) {
		GPAD_LOG_ERR("GPAD_IOC_KMD_QUERY: failed to read %zd-byte from user_arg %p!\n", sizeof(unsigned int), user_arg);
		return -EINVAL;
	}

	data_size = q.buf_offset + q.buf_len - q.cmd_offset;
	user_data = user_arg + q.cmd_offset;
	data = (char *)kzalloc(data_size, GFP_KERNEL);
	if (unlikely(copy_from_user(data, user_data, data_size))) {
		GPAD_LOG_ERR("GPAD_IOC_KMD_QUERY: failed to read %d-byte from user_data %p!\n", data_size, user_data);
		return -EINVAL;
	}


	buf_base = q.buf_offset - q.cmd_offset;

	if (0 == strncmp("mtune_test", (char *)&(data[0]), 11) && q.buf_len > 4) {
		/* for ut */
		q.buf_len -= 2;
		for (i = 0; i < q.buf_len-1; i++) {
			data[buf_base+i] = 65 + (i%6);
		}
		data[buf_base+q.buf_len-1] = '\0';

	} else if (q.buf_len <= 0) {
		value = g3dkmd_kQueryAPI(data, q.cmd_len, NULL, 0, 0);

	} else {
		q.buf_len = g3dkmd_kQueryAPI(data, q.cmd_len, data+q.buf_offset-q.cmd_offset, q.buf_len, q.arg_len);
	}

	/* return gpad_kmd_query sturct and data(cmd and buf) */
	if (copy_to_user(user_arg, &q, sizeof(struct gpad_kmd_query))) {
		GPAD_LOG_ERR("GPAD_IOC_RECEV_POLL: failed to write %zd-byte to user_arg %p!\n",  sizeof(struct gpad_kmd_query), user_arg);
		ret = -EINVAL;
	}

	data_size = q.buf_offset + q.buf_len - q.cmd_offset;
	if (copy_to_user(user_data, data, data_size)) {
		GPAD_LOG_ERR("GPAD_IOC_RECEV_POLL: failed to write %d-byte to user_data %p!\n",  data_size, user_data);
		ret = -EINVAL;
	}

	kfree(data);

	return ret;
}

long gpad_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct gpad_device  *dev = (struct gpad_device *)file->private_data;
	void __user         *user_arg = (void __user *)arg;

	switch (cmd) {
	case GPAD_IOC_GET_MOD_INFO:
		return gpad_ioc_get_mod_info(dev, user_arg);

	case GPAD_IOC_READ_REG:
		return gpad_ioc_read_reg(dev, user_arg);

	case GPAD_IOC_WRITE_REG:
		return gpad_ioc_write_reg(dev, user_arg);

	case GPAD_IOC_TOGGLE_REG:
		return gpad_ioc_toggle_reg(dev, user_arg);

	case GPAD_IOC_PM_GET_STATE:
		return gpad_ioc_get_state(dev, user_arg);

	case GPAD_IOC_PM_SET_STATE:
		return gpad_ioc_set_state(dev, user_arg);

	case GPAD_IOC_PM_GET_TIMESTAMP:
		return gpad_ioc_get_timestamp(dev, user_arg);

	case GPAD_IOC_PM_GET_CONFIG:
		return gpad_ioc_get_config(dev, user_arg);

	case GPAD_IOC_PM_SET_CONFIG:
		return gpad_ioc_set_config(dev, user_arg);

	/* TODO: case GPAD_IOC_GET_INTERVAL: */
	/* TODO: case GPAD_IOC_SET_INTERVAL: */

	case GPAD_IOC_PM_READ_COUNTERS:
		return gpad_ioc_pm_read_counters(dev, user_arg);

	case GPAD_IOC_CHANGE_OUTPUT_PIN:
		return gpad_ioc_change_output_pin(dev, user_arg);

	case GPAD_IOC_DUMP_INFO:
		return gpad_ioc_dump_info(dev, user_arg);

	case GPAD_IOC_SDBG_UPDATE_KEY:
		return gpad_ioc_sdbg_update_key(dev, user_arg);

	case GPAD_IOC_SDBG_READ_MEM:
		return gpad_ioc_sdbg_read_mem(dev, user_arg);

	case GPAD_IOC_SDBG_WRITE_MEM:
		return gpad_ioc_sdbg_write_mem(dev, user_arg);

#if defined(GPAD_SUPPORT_DVFS)
	case GPAD_IOC_GPUFREQ_UTST:
		return gpad_ioc_gpufreq_utst(dev, user_arg);

	case GPAD_IOC_IPEM_UTST:
		return gpad_ioc_ipem_utst(dev, user_arg);
#endif /* GPAD_SUPPORT_DVFS */

#if defined(GPAD_SUPPORT_DFD)
	case GPAD_IOC_DFD_DEBUG_MODE:
		return gpad_ioc_dfd_debug_mode(dev, user_arg);

	case GPAD_IOC_DFD_DUMP_SRAM:
		return gpad_ioc_dfd_dump_sram(dev, user_arg);

	case GPAD_IOC_DFD_RECEV_POLL:
		return gpad_ioc_recev_poll(dev, user_arg);

	case GPAD_IOC_DFD_SET_MASK:
		return gpad_ioc_dfd_set_mask(dev, user_arg);
#endif /* GPAD_SUPPORT_DFD */

	case GPAD_IOC_KMD_QUERY:
		return gpad_ioc_kmd_query(dev, user_arg);


	default:
		break;
	}

	return -EINVAL;
}
