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

/*
 *************************************************************************
 * Ralink Tech Inc.
 * 5F., No.36, Taiyuan St., Jhubei City,  * Hsinchu County 302,  * Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2007, Ralink Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program; if not, write to the                         *
 * Free Software Foundation, Inc.,                                       *
 *                                       *
 *************************************************************************

 Module Name:
 crypt_config.h (origin: rt_config.h)

Abstract:
Central header file to maintain all include files for all NDIS
miniport driver routines.

Revision History:
Who		 When		  What
--------	----------	----------------------------------------------
Paul Lin	08-01-2002	created

 */

#ifndef	__CRYPT_CONFIG_H__
#define	__CRYPT_CONFIG_H__

#include <linux/kernel.h>
#include <linux/slab.h>

/* file rt_linux.h */
#define IN
#define OUT
#define INOUT

#define  RTDebugLevel RT_DEBUG_ERROR

#define NdisMoveMemory(Destination, Source, Length) memmove(Destination, Source, Length)
#define NdisCopyMemory(Destination, Source, Length) memcpy(Destination, Source, Length)
#define NdisZeroMemory(Destination, Length)		 memset(Destination, 0, Length)
#define NdisFillMemory(Destination, Length, Fill)   memset(Destination, Fill, Length)
#define NdisCmpMemory(Destination, Source, Length)  memcmp(Destination, Source, Length)
#define NdisEqualMemory(Source1, Source2, Length)   (!memcmp(Source1, Source2, Length))

#define __DBGPRINT(level, Fmt, args...) printk(level Fmt, ##args)

#define _DBGPRINT_ERR(Fmt, args...)   __DBGPRINT(KERN_ERR, Fmt, ##args)
#define _DBGPRINT_INFO(Fmt, args...)  __DBGPRINT(KERN_INFO, Fmt, ##args)

#define DBGPRINT_RAW(Level, Fmt)    \
do {                                   \
	if (Level <= RTDebugLevel)      \
		printk Fmt;               \
} while (0)

#define DBGPRINT(Level, Fmt)    DBGPRINT_RAW(Level, Fmt)

#define DBGPRINT_ERR(Fmt)           \
{                                   \
	_DBGPRINT_ERR("Error")          \
	_DBGPRINT_ERR Fmt              \
}

#define DBGPRINT_S(Status, Fmt)		\
{									\
	DBGPRINT_INFO Fmt				\
}

/* file rtmp_type.h */
/* Put platform dependent declaration here */
/* For example, linux type definition */
#define UINT8   unsigned char
#define UINT16  unsigned short
#define UINT32  unsigned int
#define UINT64  unsigned long long
#define INT32   int
#define INT64   long long
#define STRING  char
#define UCHAR   unsigned char
#define USHORT  unsigned short
#define INT     int
#define UINT    unsigned int
#define ULONG   unsigned long
#define VOID    void

/* gile rt_def.h */
/* */
/*  Debug information verbosity: lower values indicate higher urgency */
/* */
#define RT_DEBUG_OFF		0
#define RT_DEBUG_ERROR		1
#define RT_DEBUG_WARN		2
#define RT_DEBUG_TRACE		3
#define RT_DEBUG_INFO		4
#define RT_DEBUG_LOUD		5

#endif	/* __CRYPT_CONFIG_H__ */

