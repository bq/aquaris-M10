/*
 * (c) MediaTek Inc. 2010
 */

/*
 * Definitions for ltr303 als/ps sensor chip.
 */

/*
 * Change log
 *
 * Date		Author		Description
 * ----------------------------------------------------------------------------
 * 11/17/2011	chenqy		Initial modification from ltr502.
 * 01/06/2012	chenqy		Do not enable the als and ps by default.
 *
 */

#ifndef __LTR303_H__
#define __LTR303_H__

#include <linux/ioctl.h>

#define LTR303_I2C_SLAVE_ADDR 0x52//0x46 // 0x23 << 1

/*REG address*/

#define APS_RW_ALS_CONTR		0x80 // ALS operation mode control SW reset
#define APS_RW_PS_CONTR			0x81 // PS operation mode control
#define APS_RW_PS_LED			0x82 // PS LED setting
#define APS_RW_PS_N_PULSES		0x83 // PS number of pulses
#define APS_RW_PS_MEAS_RATE		0x84 // PS measurement rate in active mode
#define APS_RW_ALS_MEAS_RATE		0x85 // ALS measurement rate in active mode
#define APS_RO_PART_ID			0x86 // Part Number ID and Revision ID
#define APS_RO_MANUFAC_ID		0x87 // Manufacturer ID
#define APS_RO_ALS_DATA_CH1_0		0x88 // ALS measurement CH1 data, lower byte
#define APS_RO_ALS_DATA_CH1_1		0x89 // ALS measurement CH1 data, upper byte
#define APS_RO_ALS_DATA_CH0_0		0x8A // ALS measurement CH0 data, lower byte
#define APS_RO_ALS_DATA_CH0_1		0x8B // ALS measurement CH0 data, upper byte
#define APS_RO_ALS_PS_STATUS		0x8C // ALS and PS new data status
#define APS_RO_PS_DATA_0		0x8D // PS measurement data, lower byte
#define APS_RO_PS_DATA_1		0x8E // PS measurement data, upper byte
#define APS_RW_INTERRUPT		0x8F // Interrupt settings
#define APS_RW_PS_THRES_UP_0		0x90 // PS interrupt upper threshold, lower byte
#define APS_RW_PS_THRES_UP_1		0x91 // PS interrupt upper threshold, upper byte
#define APS_RW_PS_THRES_LOW_0		0x92 // PS interrupt lower threshold, lower byte
#define APS_RW_PS_THRES_LOW_1		0x93 // PS interrupt lower threshold, upper byte
#define APS_RW_PS_OFFSET_1			0x94 // LTR303 PS offset, upper byte
#define APS_RW_PS_OFFSET_0			0x95 // LTR303 PS offset, lower byte
#define APS_RW_ALS_THRES_UP_0		0x97 // ALS interrupt upper threshold, lower byte
#define APS_RW_ALS_THRES_UP_1		0x98 // ALS interrupt upper threshold, upper byte
#define APS_RW_ALS_THRES_LOW_0		0x99 // ALS interrupt lower threshold, lower byte
#define APS_RW_ALS_THRES_LOW_1		0x9A // ALS interrupt lower threshold, upper byte
#define APS_RW_INTERRUPT_PERSIST	0x9E // ALS / PS Interrupt persist setting

/* Basic Operating Modes */

#define LTR303_ALS_ON_Range1		0x01   // LTR303 ALS lux range: 1 lux to 64k lux
#define LTR303_ALS_ON_Range2		0x05   // LTR303 ALS lux range: 0.5 lux to 32k lux
#define LTR303_ALS_ON_Range4		0x09   // LTR303 ALS lux range: 0.25 lux to 16k lux
#define LTR303_ALS_ON_Range8		0x0D   // LTR303 ALS lux range: 0.125 lux to 8k lux
#define LTR303_ALS_ON_Range48		0x19   // LTR303 ALS lux range: 0.02 lux to 1.3k lux
#define LTR303_ALS_ON_Range96		0x1D   // LTR303 ALS lux range: 0.01 lux to 600 lux

#define LTR558_ALS_ON_Range1		0x0B   // LTR558 ALS lux range: 0.01 lux to 320 lux
#define LTR558_ALS_ON_Range2		0x03   // LTR558 ALS lux range: 2 lux to 64k lux

#define MODE_ALS_StdBy			0x00

#define LTR303_PS_ON_Gain16		    	0x03  // LTR303 PS gain: X16
#define LTR303_PS_ON_Gain32			0x0B  // LTR303 PS gain: X32
#define LTR303_PS_ON_Gain64			0x07  // LTR303 PS gain: X64

#define LTR558_PS_ON_Gain1		    	0x03  // LTR558 PS gain: X1
#define LTR558_PS_ON_Gain4		    	0x07  // LTR558 PS gain: X4
#define LTR558_PS_ON_Gain8		    	0x0B  // LTR558 PS gain: X8
#define LTR558_PS_ON_Gain16		0x0F  // LTR558 PS gain: X16

#define MODE_PS_StdBy			0x00

/* Power On response time in ms */
#define PON_DELAY			600
#define WAKEUP_DELAY			10

#define LTR303_PART_ID    0xA0
#define LTR558_PART_ID    0x80


#endif



