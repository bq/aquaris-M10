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

#ifndef _SAPPHIRE_HT_H_
#define _SAPPHIRE_HT_H_

/* Last updated on 2014/08/20, 14:32. The version : 459 */




/**********************************************************************************************************/
/*  */
#define REG_HT_G3D_TASKID              0x0000	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_HT_G3D_TASKID              0x000000ff	/* [default:8'b0][perf:] g3d task id */
#define SFT_HT_G3D_TASKID              0
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_HT_RESERVED_0              0xffffff00	/* [default:24'b0][perf:] reserved */
#define SFT_HT_RESERVED_0              8

/**********************************************************************************************************/
/*  */
#define REG_HT_RESERVED_1              0x0004	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_HT_RESERVED_1              0xffffffff	/* [default:32'b0][perf:] reserved */
						  /* reserved */
#define SFT_HT_RESERVED_1              0

/**********************************************************************************************************/
/*  */
#define REG_HT_FM_HEAD                 0x0008	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_HT_FM_HEAD                 0xffffffff	/* [default:32'b0][perf:] the start address of
							frame global buffer , 16bytes alignment,
							(tail - head)must <=8M byte */
						  /* head tail context */
#define SFT_HT_FM_HEAD                 0

/**********************************************************************************************************/
/*  */
#define REG_HT_FM_TAIL                 0x000c	/*  */
/*--------------------------------------------------------------------------------------------------------*/
#define MSK_HT_FM_TAIL                 0xffffffff	/* [default:32'b0][perf:] the end address of
							frame global buffer, 16 bytes alignment */
						  /* head tail context */
#define SFT_HT_FM_TAIL                 0

/**********************************************************************************************************/


typedef struct {
	union			/* 0x0000, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int g3d_taskid:8;	/* g3d task id */
			/* default: 8'b0 */
			unsigned int reserved_0:24;	/* reserved */
			/* default: 24'b0 */
		} s;
	} reg_g3d_taskid;

	union			/* 0x0004, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int reserved_1:32;	/* reserved */
			/* reserved */
			/* default: 32'b0 */
		} s;
	} reg_reserved_1;

	union			/* 0x0008, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int fm_head:32;	/* the start address of frame global buffer ,
							16bytes alignment, (tail - head)must <=8M byte */
			/* head tail context */
			/* default: 32'b0 */
		} s;
	} reg_fm_head;

	union			/* 0x000c, CQ */
	{
		unsigned int dwValue;	/*  */
		struct {
			unsigned int fm_tail:32;	/* the end address of frame global buffer,
							16 bytes alignment */
			/* head tail context */
			/* default: 32'b0 */
		} s;
	} reg_fm_tail;

} SapphireHt;


#endif /* _SAPPHIRE_HT_H_ */
