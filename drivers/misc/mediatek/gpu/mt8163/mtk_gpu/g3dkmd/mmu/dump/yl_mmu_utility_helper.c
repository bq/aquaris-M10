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

#include "yl_mmu_utility_helper.h"

#include <stdio.h>
#include <string.h>

void Dump_Usage_Map(char buf[], FILE *fp)
{
	char c = '\0';
	int i = 0;
	int line_width = 128;

	while ((c = *buf++)) {
		fputc(c, fp);

		if (++i == line_width) {
			i = 0;
			fputc('\n', fp);
		}
	}

	fputc('\n', fp);
}
