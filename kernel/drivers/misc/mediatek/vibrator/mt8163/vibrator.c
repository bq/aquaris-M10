/******************************************************************************
 * mt6575_vibrator.c - MT6575 Android Linux Vibrator Device Driver
 * 
 * Copyright 2009-2010 MediaTek Co.,Ltd.
 * 
 * DESCRIPTION:
 *     This file provid the other drivers vibrator relative functions
 *
 ******************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <mach/mt_typedefs.h>
#include <cust_vibrator.h>
#include <mach/upmu_common_sw.h>
#include <mach/upmu_hw.h>

extern S32 pwrap_read( U32  adr, U32 *rdata );
extern S32 pwrap_write( U32  adr, U32  wdata );

extern void dct_pmic_VIBR_enable(kal_bool dctEnable);

void vibr_Enable_HW(void)
{
	
	dct_pmic_VIBR_enable(1);

}

void vibr_Disable_HW(void)
{
	dct_pmic_VIBR_enable(0);
}
/******************************************
* Set RG_VIBR_VOSEL	Output voltage select
*  hw->vib_vol:  Voltage selection
* 3'b000: 1.3V
* 3'b001: 1.5V
* 3'b010: 1.8V
* 3'b011: 2.0V
* 3'b100: 2.5V
* 3'b101: 2.8V
* 3'b110: 3.0V
* 3'b111: 3.3V
*******************************************/

void vibr_power_set(void)
{
#ifdef CUST_VIBR_VOL
	struct vibrator_hw* hw = get_cust_vibrator_hw();	
	printk("[vibrator]vibr_init: vibrator set voltage = %d\n", hw->vib_vol);
	upmu_set_rg_vibr_vosel(hw->vib_vol);
#endif
}

struct vibrator_hw* mt_get_cust_vibrator_hw(void)
{
	return get_cust_vibrator_hw();
}
