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

#ifndef _G3DKMD_SIGNAL_H_
#define _G3DKMD_SIGNAL_H_

#ifdef G3DKMD_SIGNAL_CL_PRINT
void trigger_cl_printf_stamp(pid_t pid, unsigned int write_stamp);
#endif

#ifdef G3DKMD_CALLBACK_INT
int init_sim_keyboard_int(pid_t signal_pid);
void init_sim_timer(pid_t pid);
void trigger_callback(pid_t signal_pid);
#endif

#endif /* _G3DKMD_SIGNAL_H_ */
