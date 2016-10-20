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

#include <linux/slab.h>

#include "gpad_common.h"
#include "gpad_hal.h"
#include "gpad_dfd.h"
#include "gpad_dfd_reg.h"
#include "gpad_kmif.h"

static G3DKMD_KMIF_MEM_DESC *desc;
static struct gpad_dfd_info *s_dfd_info;
struct gpad_recev_poll gpad_debug_out;

/* original mask */
static unsigned int care_mask = 0xFFCFFD63;

void gpad_dfd_dump_sram(struct seq_file *m, struct gpad_dfd_info *dfd_info)
{
	unsigned int idx;

	for (idx = 0; idx < DFD_BUFFER_SIZE/sizeof(unsigned int); idx++)
		seq_printf(m, "0x%08x\n", s_dfd_info->data_buffer[idx]);
}

int gpad_dfd_alloc_sw_buffer(void)
{
	/*
	 * Allocate SRAM data sample.
	 */
	if (!s_dfd_info->alloc_sw_buffer) {
		s_dfd_info->data_buffer = (unsigned int *)kzalloc(DFD_BUFFER_SIZE , GFP_KERNEL);
		if (!unlikely(s_dfd_info->data_buffer)) {
			GPAD_LOG_ERR("Failed to allocate data sample sw buffer!\n");
			return -ENOMEM;
		}
		GPAD_LOG_INFO("ALLOCATE SW BUFFER\n");
		s_dfd_info->alloc_sw_buffer = 1;
	} else {
		GPAD_LOG_INFO("Already allocated SW BUFFER\n");
	}
	return 0;
}

int gpad_dfd_free_sw_buffer(void)
{
	/*
	 * Allocate SRAM data sample.
	 */
	GPAD_LOG_INFO("FREE SW BUFFER\n");
	kfree(s_dfd_info->data_buffer);
	return 0;
}


int gpad_dfd_init(void *dev_ptr)
{
	int err = 0;
	struct gpad_device *dev = (struct gpad_device *)dev_ptr;

	s_dfd_info = &(dev->dfd_info);
	s_dfd_info->trigger_interrupt = 0;
	s_dfd_info->dump_sram = 0;
	s_dfd_info->alloc_sw_buffer = 0;
	s_dfd_info->fin_sram_dump = 0;

	/* Allocate SW buffer at initialization
	 * If there is no memory space for allocation, it returns -ENOMEM
	 */
	err = gpad_dfd_alloc_sw_buffer();
	return err;
}

void gpad_dfd_exit(void *dev_ptr)
{
	struct gpad_device *dev = (struct gpad_device *)dev_ptr;
	struct gpad_dfd_info  *dfd_info = &(dev->dfd_info);
	if (NULL == dfd_info)
		return;

	dfd_info->trigger_interrupt = 0;
	dfd_info->dump_sram = 0;
	s_dfd_info->alloc_sw_buffer = 0;
	s_dfd_info->fin_sram_dump = 0;
	/* Free SW buffer when exiting gpad */
	if (s_dfd_info->alloc_sw_buffer) {
		gpad_dfd_free_sw_buffer();
		s_dfd_info->alloc_sw_buffer = 0;
		GPAD_LOG_INFO("FREE SW BUFFER\n");
	}
	s_dfd_info->dump_sram = 0;
}

int gpad_dfd_get_sram(void)
{
	unsigned int *start;
	unsigned int *sw_data_buffer;
	GPAD_LOG_INFO("hw_addr = 0x%08x, vaddr = %p\n", desc->hw_addr, desc->vaddr);

	sw_data_buffer = s_dfd_info->data_buffer;
	/* Read internal dump data from vaddr */
	start = (unsigned int *)desc->vaddr;
	memcpy(sw_data_buffer, start, DFD_BUFFER_SIZE);

	/* finish SRAM dump and record this status! */
	GPAD_LOG_INFO("GET SRAM DONE!!\n");
	s_dfd_info->fin_sram_dump = 1;
	return 0;
}

int gpad_dfd_internal_dump(void)
{

	unsigned int value;
	s_dfd_info->trigger_interrupt = 1;
	if (s_dfd_info->dump_sram) {
		gpad_dfd_get_sram();
	} else {
		s_dfd_info->debug_out.data[0] = gpad_hal_read32(DFD_BUS_VALUE_DW0);
		s_dfd_info->debug_out.data[1] = gpad_hal_read32(DFD_BUS_VALUE_DW1);

		value = gpad_hal_read32(DFD_BUS_CTL);
		gpad_hal_write32(DFD_BUS_CTL, (value ^ (0x1 << 24)));

		value = gpad_hal_read32(DFD_BUS_CTL);
		gpad_hal_write32(DFD_BUS_CTL, (value ^ (0x1 << 24)));
	}
	return 0;
}

int gpad_dfd_mx_debug(void)
{
	return 0;
}
int gpad_dfd_frame_end(void)
{
	return 0;
}
int gpad_dfd_frame_swap(void)
{
	return 0;
}

void gpad_dfd_interrupt_handler(unsigned int isr_mask)
{
	if (~care_mask & isr_mask & VAL_IRQ_DFD_DUMP_TRIGGERD)
		gpad_dfd_internal_dump();
	else if (~care_mask & isr_mask & VAL_IRQ_MX_DBG)
		gpad_dfd_mx_debug();
	else if (~care_mask & isr_mask & VAL_IRQ_PA_FRAME_END)
		gpad_dfd_frame_end();
	else if (~care_mask & isr_mask & VAL_IRQ_PA_FRAME_SWAP)
		gpad_dfd_frame_swap();
	else
		GPAD_LOG_ERR("No care interrupt!!\n");
}

void gpad_set_dfd_debug_mode(int mode)
{
	g3dKmdKmifSetDFDDebugMode(mode);
	if (mode) {
		desc = g3dKmdKmifDFDAllocMemory(DFD_BUFFER_SIZE, DFD_ALIGN);
	} else {
		g3dKmdKmifDFDFreeMemory(desc);
	}
}

void gpad_set_dfd_dump_sram(int mode)
{
	unsigned int value;
	if (mode) {
		s_dfd_info->dump_sram = 1;
		/* Enable Internal Dump using 32KB SRAM*/
		value = gpad_hal_read32(DFD_DUMP_LRAM_EN);
		gpad_hal_write32(DFD_DUMP_LRAM_EN, (value | 0x1));
	} else {
		/* Disable Internal Dump using 32KB SRAM*/
		value = gpad_hal_read32(DFD_DUMP_LRAM_EN);
		gpad_hal_write32(DFD_DUMP_LRAM_EN, (value & 0xFFFFFFFE));
	}
}

void gpad_set_dfd_mask(unsigned int mask)
{
	care_mask = mask;
	GPAD_LOG_INFO("MASK = 0x%08x\n", mask);
}
