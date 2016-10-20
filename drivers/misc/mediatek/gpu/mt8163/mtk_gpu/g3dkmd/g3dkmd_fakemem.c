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

#if defined(_WIN32)
/* #include <windows.h> */
#include <stdio.h>
#elif defined(inux) && defined(linux_user_mode)
#include <stdio.h>
#endif

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#if defined(G3DKMD_FAKE_HW_ADDR) || defined(G3DKMD_PHY_HEAP)

#include "g3dkmd_engine.h"
#include "g3dkmd_api.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_fakemem.h"

#define NO_SYSTEM_MEMORY_RESERVED

struct mem_chunk_t {
	unsigned int uSize;
	unsigned int phys_addr;
	void *virt_addr;
};

#define VIDEO_BASE_ADDRESS  (1*1024*1024)
#define VIDEO_MEMORY_SIZE	((uint64_t)4*1024*1024*1024 - VIDEO_BASE_ADDRESS)
#define VIDEO_PAGE_SIZE		(4*1024)

unsigned int g_pa_base = VIDEO_BASE_ADDRESS;

#ifdef G3DKMD_PHY_HEAP
unsigned int g_max_pa_addr = 0;
#endif

#define _MAX(a, b) (((a) >= (b)) ? (a) : (b))

/* each bit indicate the page is allocated or not */
static unsigned int PAGE_FLAG[VIDEO_MEMORY_SIZE / VIDEO_PAGE_SIZE / 32] = { 0 };

#ifndef NO_SYSTEM_MEMORY_RESERVED
static char *pVideoBaseAddress;
#endif

static INLINE unsigned int _aligment(unsigned int uSize, unsigned int uAlignment)
{
	return (uSize + uAlignment - 1) & ~(uAlignment - 1);	/* make sure uAlignment is p2 */
}

static INLINE G3Dboolean _isp2(unsigned int uLength)
{
	return (uLength & (uLength - 1)) == 0;
}

static INLINE void _setFlag(int index)
{
	PAGE_FLAG[index / 32] |= 1 << (index % 32);
}

static INLINE void _clearFlag(int index)
{
	PAGE_FLAG[index / 32] &= ~(1 << (index % 32));
}

static INLINE G3Dboolean _isFlag(int index)
{
	return (PAGE_FLAG[index / 32] & (1 << (index % 32))) != 0;
}

#ifndef NO_SYSTEM_MEMORY_RESERVED
static HANDLE hBufferMapFile;
#endif

unsigned int mmAllocByVa(unsigned int uSize, void *va)
{
	unsigned int uAlignedSize;
	int ilen, index, step, i;
	unsigned int va_offset = (unsigned int)(uintptr_t) va & (VIDEO_PAGE_SIZE - 1);
	unsigned int uAlignment = VIDEO_PAGE_SIZE;
	int retval;

	YL_KMD_ASSERT(_isp2(uAlignment) && (uSize > 0));
	uAlignedSize = _aligment(uSize, uAlignment);

	ilen = ((int)uAlignedSize) / VIDEO_PAGE_SIZE;

	if (va_offset)
		ilen++;

	index = 0, step = uAlignment / VIDEO_PAGE_SIZE;
	i = 0;
	while (index < (VIDEO_MEMORY_SIZE / VIDEO_PAGE_SIZE) - ilen) {/* find continuous free pages for required size */
		for (i = 0; i < ilen; i++) {
			if (_isFlag(index + i))
				break;
		}
		if (i == ilen) {
			break;	/* found */
		} else if (i == 0) {
			index += step;
		} else {
			/* (index += (i + step - 1)) &= ~(step-1); //step is p2 */
			index += (i + step - 1);
			index &= ~(step - 1);	/* step is p2 */
		}

	}
	if (i == ilen) {
		for (i = 0; i < ilen; i++)
			_setFlag(index + i);

#ifdef G3DKMD_PHY_HEAP
		g_max_pa_addr =
		    _MAX(g_max_pa_addr, g_pa_base + index * VIDEO_PAGE_SIZE + va_offset);
#endif
		retval = g_pa_base + index * VIDEO_PAGE_SIZE + va_offset;
	} else {
		YL_KMD_ASSERT(0);
		retval = 0;
	}

	return retval;
}

#ifndef NO_SYSTEM_MEMORY_RESERVED
struct mem_chunk_t *mmAlloc(unsigned int uSize)
#else
unsigned int mmAlloc(unsigned int uSize)
#endif
{
#ifdef G3DKMD_PHY_HEAP
	return mmAllocByVa(uSize, NULL);
#else
	unsigned int uAlignedSize;
	int ilen, index, step, i;
	unsigned int uAlignment = VIDEO_PAGE_SIZE;

	YL_KMD_ASSERT(_isp2(uAlignment) && (uSize > 0));
	uAlignedSize = _aligment(uSize, uAlignment);
#ifndef NO_SYSTEM_MEMORY_RESERVED
	struct mem_chunk_t *pChunk = (mem_chunk_t *) G3DKMDMALLOC(sizeof(mem_chunk_t));

	if (!pChunk)
		return NULL;

	memset((void *)pChunk, 0, sizeof(struct mem_chunk_t));
#endif

	ilen = ((int)uAlignedSize) / VIDEO_PAGE_SIZE;
	index = 0, step = uAlignment / VIDEO_PAGE_SIZE;
	i = 0;
	while (index < (VIDEO_MEMORY_SIZE / VIDEO_PAGE_SIZE) - ilen) {/* find continuous free pages for required size */
		for (i = 0; i < ilen; i++) {
			if (_isFlag(index + i))
				break;
		}
		if (i == ilen) {
			break;/* found */
		} else if (i == 0) {
			index += step;
		} else {
			/* (index += (i + step - 1)) &= ~(step-1); //step is p2 */
			index += (i + step - 1);
			index &= ~(step - 1);	/* step is p2 */
		}

	}
	if (i == ilen) {
		for (i = 0; i < ilen; i++)
			_setFlag(index + i);
#ifndef NO_SYSTEM_MEMORY_RESERVED
		pChunk->uSize = uAlignedSize;
		pChunk->phys_addr = g_pa_base + index * VIDEO_PAGE_SIZE;
		pChunk->virt_addr = pVideoBaseAddress + index * VIDEO_PAGE_SIZE;
		return pChunk;
#else
		return g_pa_base + index * VIDEO_PAGE_SIZE;
#endif
	} else {
		YL_KMD_ASSERT(0);
		return 0;
	}
#endif
}



#ifndef NO_SYSTEM_MEMORY_RESERVED
void mmFree(struct mem_chunk_t *pChunk)
{
	if (pChunk != NULL) {
		int index = (pChunk->phys_addr - g_pa_base) / VIDEO_PAGE_SIZE;
		int ilen = (pChunk->uSize / VIDEO_PAGE_SIZE);

		for (; ilen > 0; ilen--, index++)
			_clearFlag(index);

		G3DKMDFREE(pChunk);
		pChunk = NULL;
	}
}
#else
void mmFree(unsigned int uAddr, unsigned int uSize)
{
	int index, ilen;

	if (uAddr < g_pa_base || uAddr + uSize > g_pa_base + VIDEO_MEMORY_SIZE) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FAKEMEM,
		       "mmFree Addr out of Range (0x%08x)\n", uAddr);
		return;
	}
#ifndef G3DKMD_PHY_HEAP
	if ((uAddr & (VIDEO_PAGE_SIZE - 1)) != 0) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FAKEMEM, "mmFree Addr Error (0x%08x)\n",
		       uAddr);
		return;
	}
#endif

	/* uSize = (uSize + VIDEO_PAGE_SIZE-1)& ~(VIDEO_PAGE_SIZE-1); */
	uSize = _aligment(uSize, VIDEO_PAGE_SIZE);
	index = (uAddr - g_pa_base) / VIDEO_PAGE_SIZE;
	ilen = (uSize / VIDEO_PAGE_SIZE);

#ifdef G3DKMD_PHY_HEAP
	if ((uAddr & (VIDEO_PAGE_SIZE - 1)) != 0)
		ilen++;
#endif

	for (; ilen > 0; ilen--, index++) {
#ifdef G3DKMD_PHY_HEAP
		YL_KMD_ASSERT(_isFlag(index));
#endif
		_clearFlag(index);
	}
}
#endif

#ifndef NO_SYSTEM_MEMORY_RESERVED
void mmOpen(void)
{
	if (hBufferMapFile)
		return;

	hBufferMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,
					   NULL,
					   PAGE_READWRITE, 0, VIDEO_MEMORY_SIZE, "videomemory");
	if (hBufferMapFile == NULL || hBufferMapFile == INVALID_HANDLE_VALUE) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FAKEMEM,
		       "Could not create file mapping object (%d).\n", GetLastError());
		return;
	}
	pVideoBaseAddress = (char *)MapViewOfFile(hBufferMapFile, FILE_MAP_ALL_ACCESS,	/* read/write permission */
						  0, 0, VIDEO_MEMORY_SIZE);
	if (pVideoBaseAddress == NULL) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FAKEMEM, "Could not map view of file (%d).\n",
		       GetLastError());
		return;
	}
	memset(pVideoBaseAddress, 0, VIDEO_MEMORY_SIZE);
}

void mmClose(void)
{
	if (hBufferMapFile) {
		UnmapViewOfFile(pVideoBaseAddress);
		CloseHandle(hBufferMapFile);
		hBufferMapFile = 0;
	}
}
#endif

#endif /* G3DKMD_FAKE_HW_ADDR */
