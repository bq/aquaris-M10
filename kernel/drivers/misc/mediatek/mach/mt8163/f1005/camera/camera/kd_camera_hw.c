#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>

#include "kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(PFX fmt, ##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         pr_debug(PFX fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...) \
                do {    \
                   pr_debug(PFX fmt, ##arg); \
                } while(0)
#else
#define PK_DBG(a,...)
#define PK_ERR(a,...)
#define PK_XLOG_INFO(fmt, args...)
#endif


#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3


#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4


#ifndef BOOL
typedef unsigned char BOOL;
#endif

extern void ISP_MCLK1_EN(BOOL En);

int cntVCAMD = 0;
int cntVCAMA = 0;
int cntVCAMIO = 0;
int cntVCAMAF = 0;
int cntVCAMD_SUB = 0;

static DEFINE_SPINLOCK(kdsensor_pw_cnt_lock);


bool _hwPowerOn(MT65XX_POWER powerId, int powerVolt, char *mode_name)
{
	if (hwPowerOn(powerId,  powerVolt, mode_name))
	{
		spin_lock(&kdsensor_pw_cnt_lock);
		if (powerId == CAMERA_POWER_VCAM_D)
			cntVCAMD += 1;
		else if (powerId == CAMERA_POWER_VCAM_A)
			cntVCAMA += 1;
		else if (powerId == CAMERA_POWER_VCAM_IO)
			cntVCAMIO += 1;
		else if (powerId == CAMERA_POWER_VCAM_AF)
			cntVCAMAF += 1;
		else if (powerId == SUB_CAMERA_POWER_VCAM_D)
			cntVCAMD_SUB += 1;
		spin_unlock(&kdsensor_pw_cnt_lock);
		return true;
	}
	return false;
}

bool _hwPowerDown(MT65XX_POWER powerId, char *mode_name)
{
	if (hwPowerDown(powerId, mode_name))
	{
		spin_lock(&kdsensor_pw_cnt_lock);
		if (powerId == CAMERA_POWER_VCAM_D)
			cntVCAMD -= 1;
		else if (powerId == CAMERA_POWER_VCAM_A)
			cntVCAMA -= 1;
		else if (powerId == CAMERA_POWER_VCAM_IO)
			cntVCAMIO -= 1;
		else if (powerId == CAMERA_POWER_VCAM_AF)
			cntVCAMAF -= 1;
		else if (powerId == SUB_CAMERA_POWER_VCAM_D)
			cntVCAMD_SUB -= 1;
		spin_unlock(&kdsensor_pw_cnt_lock);
		return true;
	}
	return false;
}

void checkPowerBeforClose(char *mode_name)
{

	int i = 0;

	PK_DBG("[checkPowerBeforClose]cntVCAMD:%d, cntVCAMA:%d,cntVCAMIO:%d, cntVCAMAF:%d, cntVCAMD_SUB:%d,\n",
		cntVCAMD, cntVCAMA, cntVCAMIO, cntVCAMAF, cntVCAMD_SUB);


	for (i = 0; i < cntVCAMD; i++)
		hwPowerDown(CAMERA_POWER_VCAM_D, mode_name);
	for (i = 0; i < cntVCAMA; i++)
		hwPowerDown(CAMERA_POWER_VCAM_A, mode_name);
	for (i = 0; i < cntVCAMIO; i++)
		hwPowerDown(CAMERA_POWER_VCAM_IO, mode_name);
	for (i = 0; i < cntVCAMAF; i++)
		hwPowerDown(CAMERA_POWER_VCAM_AF, mode_name);
	for (i = 0; i < cntVCAMD_SUB; i++)
		hwPowerDown(SUB_CAMERA_POWER_VCAM_D, mode_name);

	 cntVCAMD = 0;
	 cntVCAMA = 0;
	 cntVCAMIO = 0;
	 cntVCAMAF = 0;
	 cntVCAMD_SUB = 0;

}


u32 pinSetIdx = 0;//default main sensor
u32 pinSet[3][8] = {
                        //for main sensor
                     {  CAMERA_CMRST_PIN,
                        CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,              /* ON state */
                        GPIO_OUT_ZERO,             /* OFF state */
                        CAMERA_CMPDN_PIN,
                        CAMERA_CMPDN_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     },
                     //for sub sensor
                     {  CAMERA_CMRST1_PIN,
                        CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                        CAMERA_CMPDN1_PIN,
                        CAMERA_CMPDN1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     },
                   };

int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{

	int pwListIdx,pwIdx;
    BOOL sensorInPowerList = KAL_FALSE;

    if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx) {
        pinSetIdx = 0;
    }
    else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
        pinSetIdx = 1;
    }
    else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx) {
        pinSetIdx = 2;
    }

    //power ON
    if (On) {
		PK_DBG("kdCISModulePowerOn -on:currSensorName=%s\n",currSensorName);
		PK_DBG("kdCISModulePowerOn -on:pinSetIdx=%d\n",pinSetIdx);

        // Temp solution: default power on/off sequence
        if (KAL_FALSE == sensorInPowerList)
        {
            PK_DBG("Default power on sequence");
            
						if (pinSetIdx == 0 ) {
                ISP_MCLK1_EN(1);
            }
            else if (pinSetIdx == 1) {
                ISP_MCLK1_EN(1);
            }

			if (pinSetIdx == 0 && (0 == strcmp(SENSOR_DRVNAME_OV5648_MIPI_RAW, currSensorName))) {
            			 mdelay(1);

       
            //OV5648 Power UP
            //First Power Pin low and Reset Pin Low
						if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                //PDN pin
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])) {PK_DBG("[CAMERA LENS] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {PK_DBG("[CAMERA LENS] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], 0)) {PK_DBG("[CAMERA LENS] set gpio failed!! \n"); }
            
                //Reset pin
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])) {PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],  0)) {PK_DBG("[CAMERA SENSOR] set gpio failed!! \n"); }
            }        
            
						if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800, mode_name))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
            
            mdelay(1);
            
            //2.        
						if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800, mode_name))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
            
            mdelay(1);
            
            //3.
						if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1500, mode_name))
            {
                 PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                 //return -EIO;
                 goto _kdCISModulePowerOn_exit_;
            } 
            
            mdelay(1);
            
            
            //4.
						if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800, mode_name))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
            
            // wait power to be stable 
            mdelay(5);
            
            //enable active sensor
						if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                //PDN pin
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])) {PK_DBG("[CAMERA LENS] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {PK_DBG("[CAMERA LENS] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], 1)) {PK_DBG("[CAMERA LENS] set gpio failed!! \n"); }
            
                mdelay(1);
            
                //RST pin
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])) {PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], 1)) {PK_DBG("[CAMERA SENSOR] set gpio failed!! \n"); }

						if (mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])) {PK_DBG("[CAMERA LENS] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {PK_DBG("[CAMERA LENS] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMPDN], 0)) {PK_DBG("[CAMERA LENS] set gpio failed!! \n"); }
            }
            
            //msleep(20);

		}
        else if (pinSetIdx == 1 && (0 == strcmp(SENSOR_DRVNAME_GC2355_MIPI_RAW, currSensorName))) {
			mdelay(1);

            //First Power Pin low and Reset Pin Low
						if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                //PDN pin
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])) {PK_DBG("[CAMERA LENS] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {PK_DBG("[CAMERA LENS] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], 0)) {PK_DBG("[CAMERA LENS] set gpio failed!! \n"); }
            
                //Reset pin
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])) {PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],  0)) {PK_DBG("[CAMERA SENSOR] set gpio failed!! \n"); }
            }        
       
						if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800, mode_name))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
            
            mdelay(1);
            
						if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800, mode_name))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
            
            mdelay(1);
            
            
            
            //3.
						if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800, mode_name))
            {
                 PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                 //return -EIO;
                 goto _kdCISModulePowerOn_exit_;
            } 
            
            mdelay(1);
           
            
            //enable active sensor
						if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                //PDN pin
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])) {PK_DBG("[CAMERA LENS] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {PK_DBG("[CAMERA LENS] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], 0)) {PK_DBG("[CAMERA LENS] set gpio failed!! \n"); }
            
                mdelay(1);
            
                //RST pin
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])) {PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], 1)) {PK_DBG("[CAMERA SENSOR] set gpio failed!! \n"); }

						if (mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])) {PK_DBG("[CAMERA LENS] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {PK_DBG("[CAMERA LENS] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMPDN], 1)) {PK_DBG("[CAMERA LENS] set gpio failed!! \n"); }
            }
            
            //msleep(20);
        }

        }
		
		
		/*
		if (pinSetIdx==0)
			for(;;)
				{}
		*/
		/*
		if (pinSetIdx==1)
			for(;;)
				{}
 		*/
		}
    else {//power OFF
        // Temp solution: default power on/off sequence
        if (KAL_FALSE == sensorInPowerList)
        {
            PK_DBG("Default power off sequence");
            
						if (pinSetIdx == 0 ) {
                ISP_MCLK1_EN(0);
            }
            else if (pinSetIdx == 1) {
				ISP_MCLK1_EN(0);
            }

			if (pinSetIdx == 0 && (0 == strcmp(SENSOR_DRVNAME_OV5648_MIPI_RAW, currSensorName))) {
            PK_DBG("kdCISModulePower--off get in---SENSOR_DRVNAME_OV8865_MIPI_RAW \n");
           			//PK_DBG("[OFF]sensorIdx:%d \n",SensorIdx);
						if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])) {PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])) {PK_DBG("[CAMERA LENS] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)) {PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)) {PK_DBG("[CAMERA LENS] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])) {PK_DBG("[CAMERA SENSOR] set gpio failed!! \n"); } //low == reset sensor
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])) {PK_DBG("[CAMERA LENS] set gpio failed!! \n"); } //high == power down lens module
            }
            
						if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_A, mode_name)) {
                PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
						if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2, mode_name))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
						if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {
                PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
						if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2, mode_name))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
		}
        else if (pinSetIdx == 1 && (0 == strcmp(SENSOR_DRVNAME_GC2355_MIPI_RAW, currSensorName))) {
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])) {PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])) {PK_DBG("[CAMERA LENS] set gpio mode failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)) {PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)) {PK_DBG("[CAMERA LENS] set gpio dir failed!! \n"); }
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])) {PK_DBG("[CAMERA SENSOR] set gpio failed!! \n"); } //low == reset sensor
    						if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])) {PK_DBG("[CAMERA LENS] set gpio failed!! \n"); } //high == power down lens module
            }
            
						if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {
                PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
            
						if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2, mode_name))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
						if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_A, mode_name)) {
                PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }  
        }
        

        }
    }//

	return 0;

_kdCISModulePowerOn_exit_:
    return -EIO;
}

EXPORT_SYMBOL(kdCISModulePowerOn);

//!--
//


