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

#ifndef _G3DKMD_ION_H_
#define _G3DKMD_ION_H_

#include "g3dkmd_api_common.h"
#include "yl_define.h" /* for YL_PRE_ROTATE_SW_SHIFT */

struct ionGrallocLockInfo;

G3DKMD_APICALL int G3DAPIENTRY ion_mirror_create(unsigned long addr, int fd);
G3DKMD_APICALL int G3DAPIENTRY ion_mirror_destroy(unsigned long addr);

G3DKMD_APICALL int G3DAPIENTRY ion_mirror_stamp_set(unsigned long addr, unsigned int stamp,
						    unsigned readonly, pid_t tid,
						    unsigned int taskID);
G3DKMD_APICALL int G3DAPIENTRY ion_mirror_stamp_get(unsigned int addr, unsigned int *num_task,
						    struct ionGrallocLockInfo *data);

G3DKMD_APICALL int G3DAPIENTRY ion_mirror_lock_update(unsigned long addr, unsigned int num_task,
						      unsigned int readonly,
						      struct ionGrallocLockInfo *data,
						      unsigned int *lock_status);
G3DKMD_APICALL int G3DAPIENTRY ion_mirror_unlock_update(unsigned int addr);

#ifdef YL_PRE_ROTATE_SW_SHIFT
G3DKMD_APICALL int G3DAPIENTRY ion_mirror_set_rotate_info(unsigned long addr, int set_rotate);
G3DKMD_APICALL int G3DAPIENTRY ion_mirror_get_rotate_info(unsigned long addr, int *get_rotate);
#else
G3DKMD_APICALL int G3DAPIENTRY ion_mirror_set_rotate_info(unsigned long addr, int set_rotate,
							  int set_rot_dx, int set_rot_dy);
G3DKMD_APICALL int G3DAPIENTRY ion_mirror_get_rotate_info(unsigned long addr, int *get_rotate,
							  int *get_rot_dx, int *get_rot_dy);
#endif

void ion_mirror_set_pointers(void *g_ion_device, void *ion_mm_heap_register_buf_destroy_callback,
			     void *ion_free, void *ion_client_create, void *ion_import_dma_buf);

#endif /* _G3DKMD_ION_H_ */
