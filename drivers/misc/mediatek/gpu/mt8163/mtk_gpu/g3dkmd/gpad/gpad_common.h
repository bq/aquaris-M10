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

#ifndef _GPAD_COMMON_H
#define _GPAD_COMMON_H

#include "gpad_def.h"
#include "gpad_log.h"
#include "gpad_dev.h"

#define GPAD_UNUSED(x) (void)(x)

#define gpad_find_dev(_id) \
			gpad_get_default_dev()

struct gpad_device *gpad_get_default_dev(void);

#endif /* _GPAD_COMMON_H */
