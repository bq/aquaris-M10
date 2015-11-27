#ifndef _KD_CAMERA_HW_H_
#define _KD_CAMERA_HW_H_
 

#include <mach/mt_gpio.h>

#ifdef CONFIG_MTK_MT6306_SUPPORT
#include <mach/dcl_sim_gpio.h>
#endif

#include <mach/mt_pm_ldo.h>
#include "pmic_drv.h"

/* Power */
#define CAMERA_POWER_VCAM_A MT6325_POWER_LDO_VCAMA
#define CAMERA_POWER_VCAM_D MT6325_POWER_LDO_VCAMD
#define CAMERA_POWER_VCAM_A2 MT6325_POWER_LDO_VCAM_AF
#define CAMERA_POWER_VCAM_D2 MT6325_POWER_LDO_VCAM_IO
/* AF */
#define CAMERA_POWER_VCAM_AF        MT6325_POWER_LDO_VCAM_AF
/* digital io */
#define CAMERA_POWER_VCAM_IO        MT6325_POWER_LDO_VCAM_IO

#define SUB_CAMERA_POWER_VCAM_D     MT6325_POWER_LDO_VCAMD



/* FIXME, should defined in DCT tool */

/* Main sensor */
    /* Common phone's reset pin uses extension GPIO10 of mt6306 */
    #define CAMERA_CMRST_PIN            GPIO_CAMERA_CMRST_PIN 
    #define CAMERA_CMRST_PIN_M_GPIO     GPIO_CAMERA_CMRST_PIN_M_GPIO


#define CAMERA_CMPDN_PIN            GPIO_CAMERA_CMPDN_PIN    
#define CAMERA_CMPDN_PIN_M_GPIO     GPIO_CAMERA_CMPDN_PIN_M_GPIO 
 
/* FRONT sensor */
#define CAMERA_CMRST1_PIN           GPIO_CAMERA_CMRST1_PIN 
#define CAMERA_CMRST1_PIN_M_GPIO    GPIO_CAMERA_CMRST1_PIN_M_GPIO 

#define CAMERA_CMPDN1_PIN           GPIO_CAMERA_CMPDN1_PIN 
#define CAMERA_CMPDN1_PIN_M_GPIO    GPIO_CAMERA_CMPDN1_PIN_M_GPIO 

/* Define I2C Bus Num */
#define SUPPORT_I2C_BUS_NUM1        0
#define SUPPORT_I2C_BUS_NUM2        0

#endif 
