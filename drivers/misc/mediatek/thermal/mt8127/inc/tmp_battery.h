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

#include "mt_charging.h"
/* Extern two API functions from battery driver to limit max charging current. */
/**
 *  return value means charging current in mA
 *  -1 means error
 *  Implementation in mt_battery.c and mt_battery_fan5405.c
 */
int  __attribute__ ((weak)) get_bat_charging_current_level(void) {
	return 0;
}

/**
 *  current_limit means limit of charging current in mA
 *  -1 means no limit
 *  Implementation in mt_battery.c and mt_battery_fan5405.c
 */

#if 1
#define CHARGER_TYPE int
#endif

extern int set_bat_charging_current_limit(int current_limit);
extern CHARGER_TYPE mt_get_charger_type(void);

extern int read_tbat_value(void);
