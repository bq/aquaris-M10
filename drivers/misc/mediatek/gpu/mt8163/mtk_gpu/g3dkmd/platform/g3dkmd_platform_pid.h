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

#ifndef _G3DKMD_PLATFORM_PID_H_
#define _G3DKMD_PLATFORM_PID_H_

/* headers */

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)
#include <windows.h>
#include <stdlib.h>
#else
#if defined(linux_user_mode)
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#else
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/memory.h>
#include <linux/errno.h>
#include <linux/types.h>
#endif
#endif

#ifdef _WIN32
typedef int pid_t
int pid; /* the related process to request create this task */
#endif

static inline pid_t g3dkmd_get_pid(void)
{
#ifdef _WIN32
	return GetCurrentProcessId();
#elif defined(linux) && defined(linux_user_mode)
	return getpid();
#elif defined(linux) && !defined(linux_user_mode)
	return current->tgid;
#else
#error not supported OS
	return 0;
#endif
}


static inline pid_t g3dkmd_get_tid(void)
{
#ifdef _WIN32
	return GetCurrentThreadId();
#elif defined(linux) && defined(linux_user_mode)
	return gettid();
#elif defined(linux) && !defined(linux_user_mode)
	return current->pid;
#else
#error not supported OS
	return 0;
#endif
}


#endif
