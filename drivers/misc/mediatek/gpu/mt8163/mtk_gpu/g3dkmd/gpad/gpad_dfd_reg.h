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

#ifndef _GPAD_DFD_H_
#define _GPAD_DFD_H_

#define DFD_AREG_BASE                       (0x00001000)
#define DFD_MX_DBG_BASE                     (0x00004000)
#define DFD_BUS_VALUE_DW0                   (DFD_AREG_BASE | 0xFA0)
#define DFD_BUS_VALUE_DW1                   (DFD_AREG_BASE | 0xFA4)
#define DFD_BUS_CTL                         (DFD_AREG_BASE | 0xF60)

#define DFD_DUMP_LRAM_EN                    (DFD_MX_DBG_BASE | 0x180)

#define VAL_IRQ_PA_FRAME_SWAP               (1<<3)
#define VAL_IRQ_PA_FRAME_END                (1<<4)
#define VAL_IRQ_MX_DBG                      (1<<10)
#define VAL_IRQ_DFD_DUMP_TRIGGERD           (1<<12)

#endif /*_GPAD_DFD_H_*/
