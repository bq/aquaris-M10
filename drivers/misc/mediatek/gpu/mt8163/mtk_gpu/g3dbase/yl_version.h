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

#ifndef YL_VERSION_INCLUDED
#define YL_VERSION_INCLUDED

#define YL_VERSION             "Release_0"

#ifndef WIN32
#define YL_VERSION_TOUCH(v)    (v[7] = '_')
#define __init_version         __attribute__((section(".yoli_version")))
extern char yoli_version[];

#else /* WIN32 */
#define YL_VERSION_TOUCH(v)

#endif /* WIN32 */


#endif /* YL_VERSION_INCLUDED */
