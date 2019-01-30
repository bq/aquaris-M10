/*****************************************************************************
*
* Filename:
* ---------
*   bq27520.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   bq27520 header file
*
* Author:
* -------
*
****************************************************************************/



#ifndef _bq27520_SW_H_
#define _bq27520_SW_H_


/**********************************************************
  *
  *   [MASK/SHIFT] 
  *
  *********************************************************/
#define bq27520_CMD_ControlStatus 				0x00
#define bq27520_CMD_AtRate  					0x02
#define bq27520_CMD_AtRateTimeToEmpty  			0x04
#define bq27520_CMD_Temperature  				0x06
#define bq27520_CMD_Voltage  					0x08
#define bq27520_CMD_Flags  						0x0A
#define bq27520_CMD_NominalAvailableCapacity  	0x0C
#define bq27520_CMD_FullAvailableCapacity  		0x0E
#define bq27520_CMD_RemainingCapacity  			0x10
#define bq27520_CMD_FullChargeCapacity  		0x12
#define bq27520_CMD_AverageCurrent				0x14
#define bq27520_CMD_TimeToEmpty					0x16
#define bq27520_CMD_StandbyCurrent				0x18
#define bq27520_CMD_StandbyTimeToEmpty			0x1A
#define bq27520_CMD_StateOfHealth				0x1C
#define bq27520_CMD_CycleCount					0x1E
#define bq27520_CMD_StateOfCharge				0x20
#define bq27520_CMD_InstantaneousCurrent		0x22
#define bq27520_CMD_InternalTemperature			0x28
#define bq27520_CMD_ResistanceScale				0x2A
#define bq27520_CMD_OperationConfiguration		0x2C
#define bq27520_CMD_DesignCapacity				0x2E
#define bq27520_CMD_UnfilteredRM				0x6C
#define bq27520_CMD_FilteredRM					0x6E
#define bq27520_CMD_UnfilteredFCC				0x70
#define bq27520_CMD_FilteredFCC					0x72
#define bq27520_CMD_TrueSOC						0x74

#define bq27520_FLAG_BAT_DET					BIT(3)

//judge whether hardware support  BQ27520
#define MATCH_BQ27520_HARDWARE_VERSION         4

#ifndef kal_uint8
typedef unsigned char   kal_uint8;
#endif

#ifndef kal_uint32
typedef unsigned int    kal_uint32;
#endif

#ifndef kal_int32
typedef signed int      kal_int32;
#endif


/**********************************************************
  *
  *   [Extern Function] 
  *
  *********************************************************/

extern int ext_get_data_from_bq27520(kal_uint8 cmd, char *returnData,u8 len);
extern int bq27520_set_hibernate_mode(int enable);
extern int bq27520_write_batt_temperature(int temp);

#endif // _bq27520_SW_H_

