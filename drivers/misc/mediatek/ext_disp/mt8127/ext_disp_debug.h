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

#ifndef EXTDEBUG_H
#define EXTDEBUG_H

extern void hdmi_log_enable(int enable);
extern void hdmi_cable_fake_plug_in(void);
extern void hdmi_cable_fake_plug_out(void);
extern void hdmi_mmp_enable(int enable);
extern void hdmi_pattern(int enable);
extern void hdmi_log_enable(int enable);
extern void init_hdmi_mmp_events(void);

void DBG_Init(void);
void DBG_Deinit(void);

#endif
