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

#include <asm/arch_timer.h>

unsigned int localtimer_get_counter(void)
{
	unsigned int val;

	asm volatile("mrc p15, 0, %0, c14, c3, 0" : "=r" (val));

	return val;
}

int localtimer_set_next_event(unsigned int evt)
{
	unsigned int ctrl;

	asm volatile("mrc p15, 0, %0, c14, c3, 1" : "=r" (ctrl));
	ctrl |= ARCH_TIMER_CTRL_ENABLE;
	ctrl &= ~ARCH_TIMER_CTRL_IT_MASK;
	asm volatile("mcr p15, 0, %0, c14, c3, 0" : : "r" (evt));
	asm volatile("mcr p15, 0, %0, c14, c3, 1" : : "r" (ctrl));

	return 0;
}
