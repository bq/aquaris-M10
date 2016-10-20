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

#ifndef _G3DBASE_TYPE_H_
#define _G3DBASE_TYPE_H_

#include "g3dbase_common_define.h"
#include "g3dbase_common.h"

/*
 * Datatypes
 */
typedef char G3Dchar;
typedef unsigned int G3Denum;
typedef unsigned char G3Dboolean;
typedef unsigned int G3Dbitfield;
typedef void G3Dvoid;
typedef signed char G3Dbyte;	/* 1-byte signed */
typedef short G3Dshort;		/* 2-byte signed */
typedef int G3Dint;		/* 4-byte signed */
typedef unsigned char G3Dubyte;	/* 1-byte unsigned */
typedef unsigned short G3Dushort;	/* 2-byte unsigned */
typedef unsigned int G3Duint;	/* 4-byte unsigned */
typedef uint64_t G3Duint64;	/* 8-byte unsigned */
typedef int G3Dsizei;		/* 4-byte signed */
typedef float G3Dfloat;		/* single precision float */
typedef float G3Dclampf;	/* single precision float in [0,1] */
typedef double G3Ddouble;	/* double precision float */
typedef double G3Dclampd;	/* double precision float in [0,1] */

typedef G3Dboolean YL_Boolean;	/* deprecated */

typedef float G3Dfloat4v[4];
typedef unsigned int G3Duint4v[4];

/*
 * Constants
 */

/* Boolean values */
#define G3D_TRUE                1
#define G3D_FALSE               0

typedef struct YL_Pointi_ {
	int x;
	int y;

} YL_Pointi, *pYL_Pointi;


typedef struct YL_Pointf_ {
	float x;
	float y;

} YL_Pointf, *pYL_Pointf;


typedef struct YL_Point4f_ {
	float x;
	float y;
	float z;
	float w;

} YL_Point4f, *pYL_Point4f;

typedef struct YL_Point4i_ {
	long long x;
	long long y;
	long long z;
	long long w;
} YL_Point4i, *pYL_Point4i;

typedef struct YL_Rangei_ {
	YL_Pointi min;
	YL_Pointi max;

} YL_Rangei, *pYL_Rangei;


typedef struct YL_Rangef_ {
	YL_Pointf min;
	YL_Pointf max;

} YL_Rangef, *pYL_Rangef;

typedef struct _g3dCounter {
	unsigned int frameCount;
	unsigned int subFrameCount;
	unsigned int swSubFrameCount;
	unsigned int listCount;
	unsigned int threadIdx;

#ifdef YL_SWSUBFB_DRAWCOUNT
	uint64_t StatisDrawCount;
	unsigned int StatisDrawList;
#endif

	/* have it always no matter YL_GET_AVERAGE_FPS on or off */
	unsigned long firstSwapTime;	/* the first time to call swap buffer */
	unsigned long lastSwapTime;	/* the last time to call swap buffer */
	unsigned long lastLogTime;	/* the last time to log fps in a interval */
	unsigned long lastLogDuration;	/* the last time duration to log fps */
	unsigned long lastMaxDuration;	/* the max duration between swap buffer in the last time of fps log */
	unsigned long lastMinDuration;	/* the min duration between swap buffer in the last time of fps log */

	unsigned int firstSwapframeCount;	/* the frame count index when record the firstSwapTime */
	unsigned int logFrameCount;	/* the frame count used in the log interval */

#ifdef YL_DRD
	unsigned int drdDumpCount;
	unsigned int drdNeedDump;
#endif
} g3dCounter;

#if defined(linux) && !defined(linux_user_mode)
/* #if defined(G3D_USE_BUFFERFILE) || defined(G3DKMD_USE_BUFFERFILE) */
/* --------------------------------------------------------------------- */
/* User file header */
/* --------------------------------------------------------------------- */
/* g3d will use this to decode files */
typedef struct _user_file_header {
	char name[MAX_BUFFERFILE_NAME];
	int pageOffset;
	int fileLen;
} UFileHeader;
#endif

#endif /* _G3DBASE_TYPE_H_ */
