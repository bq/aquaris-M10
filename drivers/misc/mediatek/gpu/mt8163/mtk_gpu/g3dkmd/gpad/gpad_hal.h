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

#ifndef _GPAD_HAL_H
#define _GPAD_HAL_H

#define gpad_hal_write32_mask(_offset, _mask, _value) \
	gpad_hal_write32((_offset), \
			((gpad_hal_read32((_offset)) & (~(_mask))) | (_value)))

int gpad_hal_open(void);
void gpad_hal_close(void);
void gpad_hal_write32(unsigned int offset, unsigned int value);
void gpad_hal_write16(unsigned int offset, unsigned short value);
void gpad_hal_write8(unsigned int offset, unsigned char value);
unsigned int gpad_hal_read32(unsigned int offset);
unsigned short gpad_hal_read16(unsigned int offset);
unsigned char gpad_hal_read8(unsigned int offset);
int gpad_hal_read_mem(unsigned int offset, int size, int type, unsigned int *data);
int gpad_hal_write_mem(unsigned int offset, int type, unsigned int value);

#endif /* _GPAD_HAL_H */
