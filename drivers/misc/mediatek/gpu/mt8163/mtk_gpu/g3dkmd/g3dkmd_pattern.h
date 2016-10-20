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

#ifndef _G3DKMD_PATTERN_H_
#define _G3DKMD_PATTERN_H_

#include "g3dkmd_api_common.h"
#include "g3dkmd_task.h"


unsigned int g3dKmdPatternAddPhysicalAddress(int size);

void g3dKmdPatternSetPhysicalStartAddr(unsigned int pa_base);

void g3dKmdPatternFreeAll(void);

#ifdef YL_SECURE
unsigned int g3dKmdPatternRemapPhysical(G3d_Memory_Type memType,
				    void *va, unsigned int size, unsigned int secure_flag);
#else
unsigned int g3dKmdPatternRemapPhysical(G3d_Memory_Type memType, void *va, unsigned int size);
#endif

unsigned int g3dKmdPatternAllocPhysical(unsigned int size);

#ifdef PATTERN_DUMP
enum {
	G3DKMD_RIU_G3D,
/*
#ifdef USE_GCE
	G3DKMD_RIU_GCE,
#endif
*/
};

enum {
	/* for each dumpCount, we have to output "G3D reset/ MMU init/ GCE init/ G3D start" APB registers to riu */
	G3DKMD_DUMP_CE_MODE_PATTERN,

	/* We only output "G3D reset/ MMU init/ GCE init/ G3D start" APB registers to riu ONCE */
	G3DKMD_DUMP_CE_MODE_DRIVER,
};

#define BYPASS_RESERVE  (0x1 << 0) /* skip the value of RADIX_TREE_INDIRECT_PTR */
#define BYPASS_RIU      (0x1 << 1)
#define BYPASS_REG      (0x1 << 2)

#define _DEFINE_BYPASS_     unsigned int old_bypass
#define _BYPASS_(flag)      (old_bypass = g3dKmdBypass(flag))
#define _RESTORE_()         g3dKmdBypassRestore(old_bypass)

extern unsigned int curPhyAddr;

unsigned int g3dKmdBypass(unsigned int bypass);
void g3dKmdBypassRestore(unsigned int old_bypass);
unsigned char g3dKmdIsBypass(unsigned int bypass);

void g3dKmdRiuOpen(unsigned int dumpCount);
void g3dKmdRiuClose(unsigned int dumpCount);
void g3dKmdRiuWrite(unsigned int mod, const char *fmt, ...);
void g3dKmdRiuWriteIommu(unsigned int reg, unsigned int value);

void g3dKmdDumpDataToFile(void *file, unsigned char *data, unsigned int hwData, unsigned int size);
#ifdef YL_MMU_PATTERN_DUMP_V1
void g3dKmdDumpDataToFileFull(void *file, unsigned char *data, void *m, unsigned int size);
void g3dKmdDumpDataToFileFullOffset(void *file, unsigned char *data, void *m,
				    unsigned int size, unsigned int offset);
#endif
#ifdef YL_MMU_PATTERN_DUMP_V2
void g3dKmdDumpDataToFilePage(void *m, void *file, unsigned char *data, unsigned int g3d_va,
			      unsigned int size);
#endif
void _g3dKmdDumpCommandBegin(int taskID, unsigned int dumpCount);
void _g3dKmdDumpCommandEnd(int taskID, unsigned int dumpCount);
void g3dKmdDumpInitRegister(int taskID, unsigned int dumpCount);
void g3dKmdDumpQbaseBuffer(struct g3dExeInst *pInst, unsigned int dumpCount);
#else

#define _DEFINE_BYPASS_
#define _BYPASS_(flag)
#define _RESTORE_()
#define g3dKmdIsBypass(...) G3DKMD_TRUE

#endif

#ifdef G3DKMD_SNAPSHOT_PATTERN
enum sspMode {
	SSP_DUMP_CURRENT,
	SSP_DUMP_PREVIOUS,
	SSP_DUMP_BYTASKID,
};
unsigned int g3dKmdSnapshotPattern(int taskID, enum sspMode mode);
unsigned int g3dKmdSSPEnable(int bIsQuery, int *pValue);
int g3dkmdSSPPath(int bIsQuery, char *path, int len);
#endif

#include "mpd/g3dkmd_mpd.h"

#endif /* _G3DKMD_PATTERN_H_ */
