#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include <mach/mt_gpio.h>

/* Pre-defined definition */
#define TPD_TYPE_CAPACITIVE
#define TPD_TYPE_RESISTIVE
#define TPD_POWER_SOURCE	MT6323_POWER_LDO_VGP2
#define TPD_I2C_NUMBER		0
#define TPD_WAKEUP_TRIAL		60
#define TPD_WAKEUP_DELAY	100

#define FT5X0X_I2C_ADDR		0x70
//#define GPIO_CTP_PWR_PIN	(GPIO45 | 0x80000000)
//#define TPD_ROTATE_270
#define TPD_AUTO_UPGRADE_SUPPORT
//#define FT5X36_UPGADE
//#define FTS_AUTO_UPGRADE
#define TPD_DELAY		(2*HZ/100)
/*#define CUST_FTS_APK_DEBUG 1 */


#define TPD_RES_X		800
#define TPD_RES_Y		1280

//#define TPD_HAVE_CALIBRATION
/*#define TPD_CALIBRATION_MATRIX	{962, 0, 0, 0, 1600, 0, 0, 0};*/

#define TPD_CALIBRATION_MATRIX_ROTATION_NORMAL  {-4096, 0, 800*4096, 0, -4096, 1280*4096, 0, 0};
#define TPD_CALIBRATION_MATRIX_ROTATION_FACTORY {-4096, 0, 800*4096, 0, -4096, 1280*4096, 0, 0};
/*#define TPD_CALIBRATION_MATRIX_ROTATION_NORMAL  {-5328, 0, 800*4096, 0, 4096, 0, 0, 0};
#define TPD_CALIBRATION_MATRIX_ROTATION_FACTORY  {-5328, 0, 800*4096, 0, 4096, 0, 0, 0};*/


#define TPD_LDO_VOL		VOL_3000


/*#define TPD_HAVE_TREMBLE_ELIMINATION*/

#endif /* TOUCHPANEL_H__ */
