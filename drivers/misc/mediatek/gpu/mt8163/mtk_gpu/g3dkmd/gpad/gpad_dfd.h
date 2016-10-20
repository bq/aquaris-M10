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

#ifndef _GPAD_DFD_H
#define _GPAD_DFD_H

#include "gpad_ioctl.h"
#define DFD_BUFFER_SIZE           (32*1024)
#define DFD_ALIGN           16

typedef enum {
	GPAD_DFD_INTERNAL_DUMP_APB, GPAD_DFD_INTERNAL_DUMP_SRAM, GPAD_DFD_INTERNAL_DUMP_SRAM_DONE
} gpad_dfd_recev_type;

struct gpad_dfd_info {
	int trigger_interrupt;
	int dump_sram;
	int fin_sram_dump;
	int alloc_sw_buffer;
	struct gpad_recev_poll  debug_out;
	unsigned int *data_buffer;  /* SW buffer for store internal dump data from SRAM */
};

int gpad_dfd_init(void *dev_ptr);
void gpad_dfd_exit(void *dev_ptr);
void gpad_dfd_interrupt_handler(unsigned int isr_mask);
void gpad_set_dfd_debug_mode(int mode);
void gpad_set_dfd_dump_sram(int mode);
void gpad_set_dfd_mask(unsigned int mask);
int gpad_dfd_internal_dump(void);
int gpad_dfd_mx_debug(void);
int gpad_dfd_frame_end(void);
int gpad_dfd_frame_swap(void);
int gpad_dfd_get_sram(void);
int gpad_dfd_alloc_sw_buffer(void);
int gpad_dfd_free_sw_buffer(void);
void gpad_dfd_dump_sram(struct seq_file *m, struct gpad_dfd_info *dfd_info);
#endif /* _GPAD_DFD_H */
