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

#ifndef _G3DKMD_IOCTL_H_
#define _G3DKMD_IOCTL_H_

#if defined(linux)
#include <linux/ioctl.h>
#endif
#include "yl_define.h" /* for YL_PRE_ROTATE_SW_SHIFT */
#include "g3d_memory.h"

/* *******************************************************************s
 * Attention!
 * For 64bit portability, the following structures used in IOCTL interface
 * should have the same layout (size and alignment) in both 32 & 64 bit env.
 * 1. Do not use pointer, uintptr_t, size_t, etc. which size may change.
 * 2. 64bit type should be aligned by macro, because default alignment rules
 * are different in 32 and 64 bit env. It's better to use uint64_t as the
 * pointer type no matter it's from kernel or user space
 * 3. It's recommended to put 64bit type at the beginning of the structure
 * for better memory utilization.(must use FIELD_OF_UINT64 macro to declare it)
 * 4. all structure size shall be 32bits int in pair or 64bits int by single
 * and you can't put a 64bits int between two of 32bits int.
 * It's to let the ioctl size match between 32bits user space and 64bits kernel space
 * 5. no matter you use kernel or user pointer in ioctl structure, please use it by 64bits int
 * 6. please not use compile option to turn on/off a field of a ioctl structure
 * ******************************************************************* */

#define FIELD_OF_UINT64(name)    uint64_t name __aligned(8)

typedef struct _g3dkmd_context {
	/* in: */
	/* frame command buffer */
	unsigned int cmdBufStart;
	unsigned int cmdBufSize;

	/* device flag */
	unsigned int flag;

	/* dfr buffer */
	unsigned int usrDfrEnable;
	unsigned int dfrVaryStart;
	unsigned int dfrVarySize;
	unsigned int dfrBinhdStart;
	unsigned int dfrBinhdSize;
	unsigned int dfrBinbkStart;
	unsigned int dfrBinbkSize;
	unsigned int dfrBintbStart;
	unsigned int dfrBintbSize;

	/* for check g3d ioctl struct mismatch betweet g3d user and g3dkmd; */
	unsigned int g3dbaseioctl_all_struct_size;

	/* out: */
	int dispatchIndex;
} g3dkmd_context;


typedef struct _g3dkmd_flushCmd {
	/* in */
	FIELD_OF_UINT64(head); /* original type: void __user * */
	FIELD_OF_UINT64(tail); /* original type: void __user * */
	int dispatchIndex;
	int fence;
	unsigned int lastFlushStamp;
	unsigned int flushDCache;

	unsigned int lastFlushPtr;
	unsigned int carry;

	/* out */
	uint32_t result; /* original type: G3DKMDRETVAL */
	uint32_t reserved;

} g3dkmd_flushCmd;

typedef struct _g3dkmd_MMUCmd {
	/* [g3dKmdMmuInit] out and in other functions, it's in */
	FIELD_OF_UINT64(mapper); /* original type: void* */
	/* [g3dKmdMmuMap] in */
	FIELD_OF_UINT64(va); /* original type: void __user * */
	/* [g3dKmdMmuInit] in */
	unsigned int va_base;
	/* [g3dKmdMmuMap] in */
	unsigned int size;
	int could_be_section;
	unsigned int flag;

	/* [g3dKmdMmuG3dVaToPa/g3dKmdMmuUnmap] in and [g3dKmdMmuMap] out */
	unsigned int g3d_va;
	/* [g3dKmdMmuMap_given_pa] in [g3dKmdMmuG3dVaToPa] out */
	unsigned int pa;

	unsigned int cache_action;
	unsigned int reserved;
} g3dkmd_MMUCmd;

#ifdef YL_MMU_PATTERN_DUMP_V2
typedef struct _g3dkmd_MMUCmdChunk {
	/* in */
	uint64_t mapper; /* original type: void* */
	uint32_t g3d_va;
	uint32_t size;

	/* out */
	uint32_t chunk_pa;
	uint32_t chunk_size;
	uint32_t reserved; /* padding for 64 bits alignment */

} g3dkmd_MMUCmdChunk;
#endif

typedef struct _g3dkmd_LookbackCmd {
	/* in */
	FIELD_OF_UINT64(mapper); /* original type: void* */

	/* out */
	uint32_t G3d_va_base;
	uint32_t hwAddr;
	uint32_t size;
	uint32_t reserved; /* padding for 64 bits alignment */
} g3dkmd_LookbackCmd;

typedef struct _g3dkmd_PhyMemCmd {
	/* in */
	unsigned int size;
	unsigned int reserved; /* padding for 64 bits alignment */

	/* out */
	 FIELD_OF_UINT64(phyAddr);
} g3dkmd_PhyMemCmd;

typedef struct _g3dkmd_VirtMemCmd {
	/* in */
	FIELD_OF_UINT64(virtAddr); /* original type: void __user * */

	/* out */
	FIELD_OF_UINT64(pageArray); /* original type: void* */

	/* in */
	unsigned int size;

	/* out */
	unsigned int nrPages;
} g3dkmd_VirtMemCmd;

typedef struct _g3dkmd_Virt2MemCmd {
	/* out */
	FIELD_OF_UINT64(virtAddr); /* original type: void __user * */

	/* in */
	unsigned int size;
	unsigned int reserved; /* padding for 64 bits alignment */

} g3dkmd_Virt2MemCmd;

typedef struct _g3dkmd_HWAllocCmd {
	/* in */
	FIELD_OF_UINT64(phyAddr);

	/* in */
	unsigned int size;

	/* out or in */
	unsigned int hwAddr;
} g3dkmd_HWAllocCmd;

typedef struct _g3dkmd_GetSwRdbackMemCmd {
	/* in */
	unsigned int taskID;

	/* out */
	unsigned int hwAddr;

	/* out */
	 FIELD_OF_UINT64(phyAddr);

} g3dkmd_GetSwRdbackMemCmd;

typedef struct _g3dkmd_FlushTriggerCmd {
	/* in */
	unsigned int set;
	unsigned int taskID;

	/* out */
	unsigned int value;
	unsigned int reserved;
} g3dkmd_FlushTriggerCmd;

typedef struct _g3dkmd_DataCmd {
	uint32_t arg[4];

	FIELD_OF_UINT64(addr); /* original type: void __user * */
} g3dkmd_DataCmd;

#if 1 /* def YL_MMU_PATTERN_DUMP_V1 */
typedef struct _g3dkmd_PaPairInfoCmd {
	/* in */
	FIELD_OF_UINT64(mapper); /* original type: void* */
	uint32_t g3d_va;
	uint32_t size;

	/* out */
	uint32_t buffer_size;
	uint32_t unit_size;
	uint32_t offset_mask;
	uint32_t reserved; /* padding for 64 bits alignment */

} g3dkmd_PaPairInfoCmd;
#endif

typedef struct _G3DKMD_PATTERN_CFG_ {
	unsigned int dump_start_frm;
	unsigned int dump_end_frm;
	unsigned int dump_ce_mode;
} G3DKMD_PATTERN_CFG;

typedef struct _g3dkmd_IOCTL_1_ARG {
	unsigned int auiArg;
	unsigned int reserved; /* padding for 64 bits alignment */
} G3DKMD_IOCTL_1_ARG;

typedef struct _g3dkmd_IOCTL_2_ARG {
	unsigned int auiArg[2];
} G3DKMD_IOCTL_2_ARG;

typedef struct _g3dkmd_IOCTL_3_ARG {
	unsigned int auiArg[3];
	unsigned int reserved; /* padding for 64 bits alignment */
} G3DKMD_IOCTL_3_ARG;

typedef struct _g3dkmd_IOCTL_4_ARG {
	unsigned int auiArg[4];
} G3DKMD_IOCTL_4_ARG;

/**
 * struct G3DKMD_IOCTL_BF_DUMP_BEGIN_ARG
 * @offset: [OUT] the offset id to g3d ioctl to recognize bufferfile mmap request.
 * @length: [OUT] the length prepared to map to user land.
 * Provided by userspace as an argument to the ioctl G3D_IOCTL_ION_MIRROR_STAMP_SET
 */
typedef struct _g3dkmd_IOCTL_BF_DUMP_BEGIN_ARG {
	/* out */
	FIELD_OF_UINT64(offset);
	/* out */
	unsigned int length;
	unsigned int reserved; /* padding for 64 bits alignment */
} G3DKMD_IOCTL_BF_DUMP_BEGIN_ARG;

typedef struct _g3dkmd_LockCheckCmd {
	/* in */
	FIELD_OF_UINT64(data); /* original type: void __user * (ionGrallocLockInfo*) */
	unsigned int num_tasks;
	/* out */
	unsigned int result;
} g3dkmd_LockCheckCmd;

typedef struct _g3dkmd_FenceCmd {
	/* in */
	FIELD_OF_UINT64(stamp);
	unsigned int taskID;
	unsigned int reserved;

	/* in or out */
	int fd;

	/* out */
	int status;
} g3dkmd_FenceCmd;

enum MPD_Control_OP {
	MPD_SET_ENABLE = 1,
	MPD_SET_DUMP_PATH = 2,
	MPD_SET_DUMP_APNAME = 3,
	MPD_SET_DUMP_PID = 4,
	MPD_SET_SKIP_FRAME = 5,
	MPD_SET_DUMP_FRAME = 6,
	MPD_SET_DUMP_ALLPID = 7,

	MPD_NOTICE_APNAME = 8,
	MPD_QUERY_ENABLE = 9,
	MPD_QUERY_DUMP_PATH = 10,

};

typedef struct _g3dkmd_IOCTL_MPD_Control {
	FIELD_OF_UINT64(str);

	unsigned int op;
	unsigned int str_len;

	unsigned int arg;
	unsigned int reserved; /* padding for 64 bits alignment */
} g3dkmd_IOCTL_MPD_Control;

#define WRITE_PASS (1<<0)
#define READ_PASS (1<<1)
#define ALL_PASS (WRITE_PASS | READ_PASS)

#define LOCK_DONE     0 /* you lock the buffer already */
#define LOCK_DIRTY    1 /* someone update stamp when you check H/W finish */
#define LOCK_ALREADY  2 /* someone lock the buffer already */

struct ionGrallocLockInfo {
	unsigned int pid;
	unsigned int taskID;
	unsigned int write_stamp;
	unsigned int read_stamp;
	unsigned int pass;
	unsigned int reserved;
};

/**
 * struct ion_mirror_stamp_set_data - metadata passed from userspace to set stamp of an allocation
 * @addr:     a buffer start address to specify the buffer
 * @stamp:    the stamp is set to the allocation representing that fd
 * @readonly: the memory access related to stamp is readonly or not
 * @pid:      the process pid to set stamp to this buffer
 * Provided by userspace as an argument to the ioctl G3D_IOCTL_ION_MIRROR_STAMP_SET
 */
typedef struct _ion_mirror_stamp_set_data {
	FIELD_OF_UINT64(addr); /* unsigned long addr; */
	unsigned int stamp;
	unsigned int readonly;
	unsigned int pid; /* indeed, it's pid */
	unsigned int taskID;
} ion_mirror_stamp_set_data;

/**
 * struct ion_mirror_stamp_get_data - metadata passed from userspace to get stamp of an allocation
 * @addr:     a buffer start address to specify the buffer
 * @data:     the pointer to task's stamp of the allocation representing that fd
 *            you can allocate the enough array to put stamps if query the value of num_task first
 * @num_task: the number of task's stamp of the allocation representing that fd
 *            you can get it if set data value = NULL
 * Provided by userspace as an argument to the ioctl G3D_IOCTL_ION_MIRROR_STAMP_GET
 */
typedef struct _ion_mirror_stamp_get_data {
	FIELD_OF_UINT64(addr); /* unsigned long addr; */
	FIELD_OF_UINT64(data); /* struct ionGrallocLockInfo* data; */
	unsigned int num_task;
	unsigned int reserved;
} ion_mirror_stamp_get_data;

/**
 * struct ion_mirror_lock_update_data - metadata passed from userspace to try to lock an allocation
 * @addr:       a buffer start address to specify the buffer
 * @data:       the pointer to task's stamp of the allocation representing that fd
 * @num_task:   the number of task's stamp of the allocation representing that fd
 * @readonly:   the lock action is readonly or not
 * @lock_status:the status to show ion lock this buffer or not
 *
 * Provided by userspace as an argument to the ioctl G3D_IOCTL_ION_MIRROR_LOCK_UPD
 */
typedef struct _ion_mirror_lock_update_data {
	FIELD_OF_UINT64(addr); /* unsigned long addr; */
	FIELD_OF_UINT64(data); /* struct ionGrallocLockInfo* data; */
	unsigned int num_task;
	unsigned int readonly;
	unsigned int lock_status;
	unsigned int reserved;
} ion_mirror_lock_update_data;

/**
 * struct ion_mirror_data(ion_mirror_unlock_update_data) - metadata passed from userspace to try to unlock an allocation
 * @addr:     a buffer start address to specify the buffer
 *
 * Provided by userspace as an argument to the ioctl G3D_IOCTL_ION_MIRROR_UNLOCK_UPD
 */
typedef struct _ion_mirror_data {
	FIELD_OF_UINT64(addr); /* unsigned long addr; */
	int fd;
	int reserved;
} ion_mirror_data;

/**
 * struct ion_mirror_rotate_data - metadata between userspace to set & get buffer rotation information
 * @rotate:  rotation angle (0: 0 degree, 1: 90 degree,  2: 180 degree, 3: 270 degree
 * @rot_dx:  rotation buffer x offset
 * @rot_dy:  rotation buffer y offset
 * Provided by userspace as an argument to the ioctl G3D_IOCTL_ION_MIRROR_SET_ROTATE_INFO and
 * G3D_IOCTL_ION_MIRROR_GET_ROTATE_INFO
 */
typedef struct _ion_mirror_rotate_data {
	FIELD_OF_UINT64(addr); /* unsigned long addr; */
	int rotate;
#if !defined(YL_PRE_ROTATE_SW_SHIFT)
	int rot_dx;
	int rot_dy;
#endif
	int reserved;
} ion_mirror_rotate_data;

typedef struct _g3dkmd_IOCTL_CL_PRINTF_INFO {
	unsigned int taskID;
	unsigned int ctxsID;
	unsigned int isrTrigger;
	unsigned int waveID;
} g3dkmd_IOCTL_CL_PRINTF_INFO;


/**
 * struct _g3dkmd_IOCTL_DRV_INFO_MEMLIST - pass alloc/free data from user to
 * kernel.
 * @va:     the memory address in userland
 * @size:   the size of va.
 * @hwAddr: the hardware address.
 */
typedef struct _g3dkmd_IOCTL_DRV_INFO_MEMLIST {
	FIELD_OF_UINT64(va);
	unsigned int size;
	unsigned int hwAddr;
} g3dkmd_IOCTL_DRV_INFO_MEMLIST;


#define LINUX_MMAP_ALIGN_OFST   0x00000fffUL
#define LINUX_MMAP_ALIGN_MASK   (~LINUX_MMAP_ALIGN_OFST)

#define G3D_MAJOR           121
#define G3D_NAME            "yolig3d"
#define G3D_IOCTL_MAGIC     'G'


#define  G3D_IOCTL_REGISTERCONTEXT         _IOWR(G3D_IOCTL_MAGIC, 1, g3dkmd_context)
#define  G3D_IOCTL_RELEASECONTEXT          _IOWR(G3D_IOCTL_MAGIC, 2, g3dkmd_context)
#define  G3D_IOCTL_FLUSHCOMMAND            _IOWR(G3D_IOCTL_MAGIC, 3, g3dkmd_flushCmd)

#if 0
#define  G3D_IOCTL_MMUINIT                 _IOWR(G3D_IOCTL_MAGIC, 4, g3dkmd_MMUCmd)
#define  G3D_IOCTL_MMUUNINIT               _IOWR(G3D_IOCTL_MAGIC, 5, g3dkmd_MMUCmd)
#endif
#define  G3D_IOCTL_MMUGETHANDLE            _IOWR(G3D_IOCTL_MAGIC, 5, g3dkmd_MMUCmd)
#define  G3D_IOCTL_GETTABLE                _IOWR(G3D_IOCTL_MAGIC, 6, g3dkmd_MMUCmd)
#define  G3D_IOCTL_G3DVATOPA               _IOWR(G3D_IOCTL_MAGIC, 7, g3dkmd_MMUCmd)
#define  G3D_IOCTL_MMUMAP                  _IOWR(G3D_IOCTL_MAGIC, 8, g3dkmd_MMUCmd)
#define  G3D_IOCTL_MMUMAP_GIVENPA          _IOWR(G3D_IOCTL_MAGIC, 9, g3dkmd_MMUCmd)
#define  G3D_IOCTL_MMUUNMAP                _IOWR(G3D_IOCTL_MAGIC, 10, g3dkmd_MMUCmd)
#define  G3D_IOCTL_MMU_RANGE_SYNC          _IOWR(G3D_IOCTL_MAGIC, 11, g3dkmd_MMUCmd)
#define  G3D_IOCTL_MMU_GET_USR_BUF_USAGE   _IOWR(G3D_IOCTL_MAGIC, 12, G3DKMD_IOCTL_2_ARG)

/* YL_MMU_PATTERN_DUMP_V2 */
#define  G3D_IOCTL_G3DVATOPA_CHUNK         _IOWR(G3D_IOCTL_MAGIC, 13, g3dkmd_MMUCmdChunk)

#define  G3D_IOCTL_MAKE_VA_LOOKUP          _IOWR(G3D_IOCTL_MAGIC, 14, g3dkmd_LookbackCmd)
#define  G3D_IOCTL_PHY_ALLOC               _IOWR(G3D_IOCTL_MAGIC, 15, g3dkmd_PhyMemCmd)
#define  G3D_IOCTL_PHY_FREE                _IOWR(G3D_IOCTL_MAGIC, 16, g3dkmd_PhyMemCmd)
#define  G3D_IOCTL_USERPAGE_GET            _IOWR(G3D_IOCTL_MAGIC, 17, g3dkmd_VirtMemCmd)
#define  G3D_IOCTL_USERPAGE_FREE           _IOWR(G3D_IOCTL_MAGIC, 18, g3dkmd_VirtMemCmd)

#define  G3D_IOCTL_CALLBACK_TEST           _IOW(G3D_IOCTL_MAGIC,  19, G3DKMD_IOCTL_1_ARG)

#define  G3D_IOCTL_VIRT_ALLOC              _IOWR(G3D_IOCTL_MAGIC, 20, g3dkmd_Virt2MemCmd)
#define  G3D_IOCTL_VIRT_FREE               _IOWR(G3D_IOCTL_MAGIC, 21, g3dkmd_Virt2MemCmd)

#define  G3D_IOCTL_SWRDBACK_ALLOC          _IOWR(G3D_IOCTL_MAGIC, 22, g3dkmd_GetSwRdbackMemCmd)
#define  G3D_IOCTL_FLUSHTRIGGER            _IOWR(G3D_IOCTL_MAGIC, 23, g3dkmd_FlushTriggerCmd)
#define  G3D_IOCTL_CHECK_HWSTATUS          _IOWR(G3D_IOCTL_MAGIC, 24, G3DKMD_IOCTL_3_ARG)

/* for pattern dump */
#define  G3D_IOCTL_DUMP_BEGIN              _IOW(G3D_IOCTL_MAGIC,  25, G3DKMD_IOCTL_2_ARG)
#define  G3D_IOCTL_DUMP_END                _IOW(G3D_IOCTL_MAGIC,  26, G3DKMD_IOCTL_2_ARG)

#define  G3D_IOCTL_HW_ALLOC                _IOWR(G3D_IOCTL_MAGIC, 27, g3dkmd_HWAllocCmd)
#define  G3D_IOCTL_HW_FREE                 _IOWR(G3D_IOCTL_MAGIC, 28, g3dkmd_HWAllocCmd)
#define  G3D_IOCTL_GET_MSG                 _IOWR(G3D_IOCTL_MAGIC, 29, g3dkmd_DataCmd)

#define  G3D_IOCTL_CHECK_DONE              _IOWR(G3D_IOCTL_MAGIC, 30, g3dkmd_LockCheckCmd)
#define  G3D_IOCTL_CHECK_FLUSH             _IOWR(G3D_IOCTL_MAGIC, 31, g3dkmd_LockCheckCmd)

#define  G3D_IOCTL_LTCFLUSH                _IOR(G3D_IOCTL_MAGIC, 32, G3DKMD_IOCTL_1_ARG)
#define  G3D_IOCTL_PATTERN_CONFIG          _IOW(G3D_IOCTL_MAGIC, 33, G3DKMD_PATTERN_CFG)

#define  G3D_IOCTL_FENCE_CREATE            _IOWR(G3D_IOCTL_MAGIC, 34, g3dkmd_FenceCmd)

#define  G3D_IOCTL_ION_MIRROR_CREATE       _IOWR(G3D_IOCTL_MAGIC, 36, ion_mirror_data)
#define  G3D_IOCTL_ION_MIRROR_DESTROY      _IOWR(G3D_IOCTL_MAGIC, 37, ion_mirror_data)
#define  G3D_IOCTL_ION_MIRROR_STAMP_SET    _IOWR(G3D_IOCTL_MAGIC, 38, ion_mirror_stamp_set_data)
#define  G3D_IOCTL_ION_MIRROR_STAMP_GET    _IOWR(G3D_IOCTL_MAGIC, 39, ion_mirror_stamp_get_data)
#define  G3D_IOCTL_ION_MIRROR_LOCK_UPD     _IOWR(G3D_IOCTL_MAGIC, 40, ion_mirror_lock_update_data)
#define  G3D_IOCTL_ION_MIRROR_UNLOCK_UPD   _IOWR(G3D_IOCTL_MAGIC, 41, ion_mirror_data)

/* for rotation */
#define  G3D_IOCTL_ION_MIRROR_SET_ROTATE_INFO    _IOW(G3D_IOCTL_MAGIC, 42, ion_mirror_rotate_data)
#define  G3D_IOCTL_ION_MIRROR_GET_ROTATE_INFO    _IOR(G3D_IOCTL_MAGIC, 43, ion_mirror_rotate_data)

/* YL_MMU_PATTERN_DUMP_V1 */
#define  G3D_IOCTL_PA_PAIR_INFO            _IOWR(G3D_IOCTL_MAGIC, 44, g3dkmd_PaPairInfoCmd)
#define  G3D_IOCTL_GET_DRV_INFO            _IOWR(G3D_IOCTL_MAGIC, 45, g3dkmd_DataCmd)
#define  G3D_IOCTL_SET_DRV_INFO            _IOWR(G3D_IOCTL_MAGIC, 46, g3dkmd_DataCmd)

/* for bufferfile */
#define  G3D_IOCTL_BUFFERFILE_DUMP_BEGIN   _IOW(G3D_IOCTL_MAGIC,  47, G3DKMD_IOCTL_BF_DUMP_BEGIN_ARG)
#define  G3D_IOCTL_BUFFERFILE_DUMP_END     _IOW(G3D_IOCTL_MAGIC,  48, G3DKMD_IOCTL_1_ARG)

/* for CL printf */
#define  G3D_IOCTL_CL_GET_STAMP            _IOWR(G3D_IOCTL_MAGIC, 49, g3dkmd_IOCTL_CL_PRINTF_INFO)
#define  G3D_IOCTL_CL_UPDATE_READ_STAMP    _IOW(G3D_IOCTL_MAGIC, 50, G3DKMD_IOCTL_1_ARG)
#define  G3D_IOCTL_CL_RESUME_DEV           _IO(G3D_IOCTL_MAGIC, 51)

/* for auto recovery */
#define  G3D_IOCTL_HT_READ_WRITE_EQUAL     _IOWR(G3D_IOCTL_MAGIC, 52, unsigned char)

#define  G3D_IOCTL_MPD_CONTROL             _IOWR(G3D_IOCTL_MAGIC, 53, g3dkmd_IOCTL_MPD_Control)

#define  G3D_IOCTL_MAXNR 53


typedef struct _g3dkmd_MemListNode {
	uintptr_t index;
	uintptr_t data0;
	unsigned int data1;
	struct _g3dkmd_MemListNode *pNext;
} g3dkmd_MemListNode;


typedef struct _g3d_private_data {
	g3dkmd_MemListNode *gTaskhead;
	g3dkmd_MemListNode *gPhyMemhead;
	g3dkmd_MemListNode *gVirtMemhead;
	g3dkmd_MemListNode *gG3DMMUhead;

	uintptr_t bf_mmap_address;
} g3d_private_data;



/* for check g3d ioctl struct mismatch betweet g3d user and g3dkmd; */
typedef struct _g3dbaseioctl_all_struct {
	g3dkmd_context s1;
	g3dkmd_flushCmd s2;
	g3dkmd_MMUCmd s3;
	g3dkmd_MMUCmdChunk s4;
	g3dkmd_LookbackCmd s5;
	g3dkmd_PhyMemCmd s6;
	g3dkmd_VirtMemCmd s7;
	g3dkmd_Virt2MemCmd s8;
	g3dkmd_HWAllocCmd s9;
	g3dkmd_GetSwRdbackMemCmd s10;
	g3dkmd_FlushTriggerCmd s11;
	g3dkmd_DataCmd s12;
	g3dkmd_PaPairInfoCmd s13;
	G3DKMD_PATTERN_CFG s14;
	G3DKMD_IOCTL_1_ARG s15;
	G3DKMD_IOCTL_2_ARG s16;
	G3DKMD_IOCTL_3_ARG s17;
	G3DKMD_IOCTL_4_ARG s18;
	G3DKMD_IOCTL_BF_DUMP_BEGIN_ARG s19;
	g3dkmd_LockCheckCmd s20;
	g3dkmd_FenceCmd s21;
	g3dkmd_IOCTL_MPD_Control s22;
	struct ionGrallocLockInfo s23;
	ion_mirror_stamp_set_data s24;
	ion_mirror_stamp_get_data s25;
	ion_mirror_lock_update_data s26;
	ion_mirror_data s27;
	ion_mirror_rotate_data s28;
	g3dkmd_IOCTL_CL_PRINTF_INFO s29;
	g3dkmd_IOCTL_DRV_INFO_MEMLIST s30;
	unsigned char s31;
} g3dbaseioctl_all_struct;


#if defined(linux) && !defined(linux_user_mode) && defined(USING_MISC_DEVICE)
/* linux kernel && misc device */
const struct platform_device *g3dGetPlatformDevice(void);
#endif /* USING_MISC_DEVICE */

#endif /* _G3DKMD_IOCTL_H_ */
