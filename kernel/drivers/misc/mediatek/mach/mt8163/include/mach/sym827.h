/*****************************************************************************
*
* Filename:
* ---------
*   sym827.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   sym827 header file
*
* Author:
* -------
*
****************************************************************************/

#ifndef _SYM827_SW_H_
#define _SYM827_SW_H_

extern void sym827_dump_register(void);
extern kal_uint32 sym827_read_interface
    (kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT);
extern kal_uint32 sym827_config_interface
    (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT);
extern int is_sym827_exist(void);
extern int is_sym827_sw_ready(void);
extern int sym827_buck_power_on();
extern int sym827_buck_power_off();
extern int sym827_buck_get_state();
extern int sym827_buck_set_mode(int mode);
extern int sym827_buck_get_mode();
extern int sym827_buck_set_switch(int reg);
extern int sym827_buck_get_switch();
extern int sym827_buck_set_voltage(unsigned long vol);
extern unsigned long sym827_buck_get_voltage();

#define	SYM827_REG_VSEL_0				0x00
#define	SYM827_REG_VSEL_1				0x01
#define	SYM827_REG_CONTROL				0x02
#define	SYM827_REG_ID_1					0x03
#define	SYM827_REG_ID_2					0x04
#define	SYM827_REG_PGOOD				0x05

#define SYM827_BUCK_ENABLE				0x01
#define SYM827_BUCK_DISABLE				0x00

#define SYM827_BUCK_EN0_SHIFT			0x07
#define SYM827_BUCK_MODE_SHIFT			0x06
#define SYM827_BUCK_NSEL_SHIFT			0x00

#define SYM827_ID_VENDOR_SHIFT			0x05

#endif
