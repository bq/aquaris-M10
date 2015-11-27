/*
 * (c) MediaTek Inc. 2010
 */


#ifndef TOUCHPANEL_UPGRADE_H__
#define TOUCHPANEL_UPGRADE_H__



//#define CTP_DETECT_SUPPLIER_THROUGH_GPIO  1

#ifdef TPD_ALWAYS_UPGRADE_FW
#undef TPD_ALWAYS_UPGRADE_FW
#endif

#define GPIO_CTP_COMPAT_PIN1	GPIO28
#define GPIO_CTP_COMPAT_PIN2	GPIO29

#define TPD_SUPPLIER_0		0
#define TPD_SUPPLIER_1		1
#define TPD_SUPPLIER_2		2
#define TPD_SUPPLIER_3		3

#define TPD_FW_VER_0		0x00
#define TPD_FW_VER_1		0x11  //ht tp   0   1 (choice 0)
#define TPD_FW_VER_2            0x00
#define TPD_FW_VER_3            0x10  //zxv tp  0   0 

static unsigned char TPD_FW0[] =
{
#include "ft5626_Ref_V11_20150612_app.i"
};

static unsigned char TPD_FW1[] =
{
 // #include "ft5x0x_fw_6127_ht.i"	
#include "ft5626_Ref_V11_20150612_app.i"
};

static unsigned char TPD_FW2[] =
{
#include "ft5626_Ref_V11_20150612_app.i"
};

static unsigned char TPD_FW3[] =
{
// #include "ft5x0x_fw_6127_zxv.i"
#include "ft5626_Ref_V11_20150612_app.i"	
};


#endif /* TOUCHPANEL_UPGRADE_H__ */



