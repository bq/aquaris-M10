/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/
/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H


typedef enum {
	SENSOR_MODE_INIT = 0,
	SENSOR_MODE_PREVIEW,
	SENSOR_MODE_CAPTURE,
	SENSOR_MODE_ZSD
} OV2659_SENSOR_MODE;

typedef enum _OV2659_OP_TYPE_ {
	OV2659_MODE_NONE,
	OV2659_MODE_PREVIEW,
	OV2659_MODE_CAPTURE,
	OV2659_MODE_QCIF_VIDEO,
	OV2659_MODE_CIF_VIDEO,
	OV2659_MODE_QVGA_VIDEO
} OV2659_OP_TYPE;

extern OV2659_OP_TYPE OV2659_g_iOV2659_Mode;

#define OV2659_MAX_GAIN							(0x38)
#define OV2659_MIN_GAIN							(1)

#define OV2659_ID_REG                          (0x300A)
#define OV2659_INFO_REG                        (0x300B)

#define OV2659_IMAGE_SENSOR_SVGA_WIDTH          (800)
#define OV2659_IMAGE_SENSOR_SVGA_HEIGHT         (600)
#define OV2659_IMAGE_SENSOR_UVGA_WITDH        (1600)
#define OV2659_IMAGE_SENSOR_UVGA_HEIGHT       (1200)

#define OV2659_PV_PERIOD_PIXEL_NUMS		(1300)
#define OV2659_PV_PERIOD_LINE_NUMS		(616)
#define OV2659_FULL_PERIOD_PIXEL_NUMS		(1951)
#define OV2659_FULL_PERIOD_LINE_NUMS		(1232)

#define OV2659_PV_EXPOSURE_LIMITATION	(616-4)
#define OV2659_FULL_EXPOSURE_LIMITATION	(1232-4)

#define OV2659_PV_GRAB_START_X				(5)
#define OV2659_PV_GRAB_START_Y			(5)
#define OV2659_FULL_GRAB_START_X			(5)
#define OV2659_FULL_GRAB_START_Y			(5)

#define OV2659_WRITE_ID							    0x60
#define OV2659_READ_ID								0x61

UINT32 OV2659Open(void);
UINT32 OV2659GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 OV2659GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
		     MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV2659Control(MSDK_SCENARIO_ID_ENUM ScenarioId,
		     MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
		     MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV2659FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,
			    UINT32 *pFeatureParaLen);
UINT32 OV2659Close(void);
UINT32 OV2659_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc);
extern int iReadReg(u16 a_u2Addr, u8 *a_puBuff, u16 i2cId);
extern int iWriteReg(u16 a_u2Addr, u32 a_u4Data, u32 a_u4Bytes, u16 i2cId);

#endif
