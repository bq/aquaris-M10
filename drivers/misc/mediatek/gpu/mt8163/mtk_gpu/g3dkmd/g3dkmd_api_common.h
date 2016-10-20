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

#include "g3d_memory.h"

#ifndef _G3DKMD_API_COMMON_H_
#define _G3DKMD_API_COMMON_H_


#if defined(_WIN32)
#if defined(G3DKMD_EXPORTS)
#define G3DKMD_APICALL __declspec(dllexport)
#else
#define G3DKMD_APICALL __declspec(dllimport)
#endif
#elif (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 303) \
	|| (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590) || (defined(linux) && defined(linux_user_mode)))
#define G3DKMD_APICALL __attribute__((visibility("default")))
#elif (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 303) \
	|| (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
/* KHRONOS_APIATTRIBUTES is not used by the client API headers yet */
#define G3DKMD_APICALL
#else
#define G3DKMD_APICALL
#endif

#if defined(_WIN32)
#define G3DAPIENTRY __stdcall
#else
#define G3DAPIENTRY
#endif

#endif /* _G3DKMD_API_COMMON_H_ */
