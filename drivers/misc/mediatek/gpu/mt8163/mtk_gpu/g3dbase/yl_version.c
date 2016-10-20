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


#include "yl_version.h"


#ifndef WIN32

char __init_version yoli_version[] = YL_VERSION;
char *g3dkmd_GetYoliVersion(void)
{
	return yoli_version;
}
#endif /* WIN32 */
