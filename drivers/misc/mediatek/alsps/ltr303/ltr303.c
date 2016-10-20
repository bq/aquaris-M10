/*
 *
 * Author: MingHsien Hsieh <minghsien.hsieh@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include "batch.h"
#include "upmu_sw.h"
#include "upmu_common.h"
#include "cust_alsps.h"
#include <sensors_io.h>

#include "ltr303.h"
#include "alsps.h"
#undef CUSTOM_KERNEL_SENSORHUB
#ifdef CUSTOM_KERNEL_SENSORHUB
#include <SCP_sensorHub.h>
#endif

/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define LTR303_DEV_NAME   "ltr303"

#define POWER_NONE_MACRO MT65XX_POWER_NONE
#define GN_MTK_BSP_PS_DYNAMIC_CALI

//#define APS_DEBUG_ENABLE

/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[LTR303] "
#ifdef APS_DEBUG_ENABLE
#define APS_FUN(f)               printk(KERN_ERR APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)

#define APS_ERR_ST(f)    printk(KERN_ERR  APS_TAG"%s %d : ", __FUNCTION__, __LINE__)

#define APS_LOG(fmt, args...)    printk(KERN_ERR APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_ERR APS_TAG fmt, ##args)
#else
#define APS_FUN(f)
#define APS_ERR(fmt, args...)  //printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)

#define APS_ERR_ST(f)

#define APS_LOG(fmt, args...)  //printk(KERN_ERR APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)
#endif

/*----------------------------------------------------------------------------*/

static struct i2c_client *ltr303_i2c_client = NULL;
/* Maintain alsps cust info here */
struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
struct platform_device *alspsPltFmDev;
static bool readenable=true;
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ltr303_i2c_id[] = {{LTR303_DEV_NAME,0},{}};
//static struct i2c_board_info __initdata i2c_ltr303={ I2C_BOARD_INFO(LTR303_DEV_NAME, 0x23)};
static unsigned long long int_top_time;
/*the adapter id & i2c address will be available in customization*/
//static struct i2c_board_info __initdata i2c_ltr303={ I2C_BOARD_INFO(LTR303_DEV_NAME, 0x23)};

//static unsigned short ltr303_force[] = {0x00, 0x46, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const ltr303_forces[] = { ltr303_force, NULL };
//static struct i2c_client_address_data ltr303_addr_data = { .forces = ltr303_forces,};
/*----------------------------------------------------------------------------*/
static int ltr303_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int ltr303_i2c_remove(struct i2c_client *client);
//static int ltr303_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int ltr303_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int ltr303_i2c_resume(struct i2c_client *client);
static int ltr303_init_device(void);

static int ltr303_ps_enable(int gainrange);
static int dynamic_calibrate=0;

//static int ps_trigger_high = 800;	
//static int ps_trigger_low = 760;

static int ps_gainrange = 0;
static int als_gainrange = 0;

static int final_prox_val , prox_val;
static int final_lux_val;

static int ltr303_remove(void);
static int ltr303_local_init(void);
static int ltr303_init_flag =-1; // 0<==>OK -1 <==> fail
static struct alsps_init_info ltr303_init_info = {
		.name = "ltr303",
		.init = ltr303_local_init,
		.uninit = ltr303_remove,
};
/*----------------------------------------------------------------------------*/
static DEFINE_MUTEX(read_lock);

/*----------------------------------------------------------------------------*/
static int ltr303_als_read(int gainrange);
static int ltr303_ps_read(void);


/*----------------------------------------------------------------------------*/


typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;

/*----------------------------------------------------------------------------*/
struct ltr303_i2c_addr {    /*define a series of i2c slave address*/
    u8  write_addr;  
    u8  ps_thd;     /*PS INT threshold*/
};

/*----------------------------------------------------------------------------*/

struct ltr303_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct work_struct  eint_work;
    struct mutex lock;
	/*i2c address group*/
    struct ltr303_i2c_addr  addr;

     /*misc*/
    u16		    als_modulus;
    atomic_t    i2c_retry;
    atomic_t    als_debounce;   /*debounce time after enabling als*/
    atomic_t    als_deb_on;     /*indicates if the debounce is on*/
    atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
    atomic_t    ps_mask;        /*mask ps: always return far away*/
    atomic_t    ps_debounce;    /*debounce time after enabling ps*/
    atomic_t    ps_deb_on;      /*indicates if the debounce is on*/
    atomic_t    ps_deb_end;     /*the jiffies representing the end of debounce*/
    atomic_t    ps_suspend;
    atomic_t    als_suspend;
    atomic_t trace;
    atomic_t init_done;
    struct device_node *irq_node;
    int irq;

    /*data*/
    u16         als;
    u16          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    atomic_t    als_cmd_val;    /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_cmd_val;     /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val;     /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val_high;     /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val_low;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;         /*enable mask*/
    ulong       pending_intr;   /*pending interrupt*/

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};

 struct PS_CALI_DATA_STRUCT
{
    int close;
    int far_away;
    int valid;
} ;
 
static int intr_flag_value = 0;


static struct ltr303_priv *ltr303_obj = NULL;

#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{ .compatible = "mediatek,alsps", },
	{},
};
#endif

static struct i2c_driver ltr303_i2c_driver = {	
	.probe      = ltr303_i2c_probe,
	.remove     = ltr303_i2c_remove,
	.suspend    = ltr303_i2c_suspend,
	.resume     = ltr303_i2c_resume,
	.id_table   = ltr303_i2c_id,
	.driver = {
		.name           = LTR303_DEV_NAME,
#ifdef CONFIG_OF            
               .of_match_table = alsps_of_match,
#endif              
	},
};


/* 
 * #########
 * ## I2C ##
 * #########
 */
/*----------------------------------------------------------------------------*/
// I2C Read
static int ltr303_i2c_read_reg(u8 regnum)
{
    u8 buffer[1],reg_value[1];
	int res = 0;
	mutex_lock(&read_lock);
	
	buffer[0]= regnum;
	res = i2c_master_send(ltr303_obj->client, buffer, 0x1);
	if(res <= 0)	{
	   
	   APS_ERR("read reg send res = %d\n",res);
		return res;
	}
	res = i2c_master_recv(ltr303_obj->client, reg_value, 0x1);
	if(res <= 0)
	{
		APS_ERR("read reg recv res = %d\n",res);
		return res;
	}
	mutex_unlock(&read_lock);
	return reg_value[0];
}

/*----------------------------------------------------------------------------*/
// I2C Write
static int ltr303_i2c_write_reg(u8 regnum, u8 value)
{
	u8 databuf[2];    
	int res = 0;
   
	databuf[0] = regnum;   
	databuf[1] = value;
	res = i2c_master_send(ltr303_obj->client, databuf, 0x2);

	if (res < 0)
		{
			APS_ERR("wirte reg send res = %d\n",res);
		   	return res;
		}
		
	else
		return 0;
}

/*----------------------------------------------------------------------------*/
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
static ssize_t ltr303_dynamic_calibrate(void)			
{																				
	int i=0;
	int data;
	int data_total=0;
	ssize_t len = 0;
	int noise = 0;
	int count = 5;
	int max = 0;
	int error = 0;
        int ps_thd_val_low;
        int ps_thd_val_high ;
	struct ltr303_priv *obj = ltr303_obj;
	if(!ltr303_obj)
	{	
		APS_ERR("ltr303_obj is null!!\n");
		return -1;
	}

	msleep(15);


	for (i = 0; i < count; i++) {
		
		msleep(15);
		
		data=ltr303_ps_read();
		if (data < 0) {
			i--;
			continue;
		}
				
		if(data & 0x8000){
			noise = 0;
			break;
		}else{
			noise=data;
		}	
		
		data_total+=data;

		if (max++ > 100) {
			return len;
		}
	}

	noise=data_total/count;
	dynamic_calibrate = noise;
	if(noise < 100){
		atomic_set(&obj->ps_thd_val_high,  noise+100);
		atomic_set(&obj->ps_thd_val_low, noise+50);
	}else if(noise < 200){
		atomic_set(&obj->ps_thd_val_high,  noise+150);
		atomic_set(&obj->ps_thd_val_low, noise+60);
	}else if(noise < 300){
		atomic_set(&obj->ps_thd_val_high,  noise+150);
		atomic_set(&obj->ps_thd_val_low, noise+60);
	}else if(noise < 400){
		atomic_set(&obj->ps_thd_val_high,  noise+150);
		atomic_set(&obj->ps_thd_val_low, noise+60);
	}else if(noise < 600){
		atomic_set(&obj->ps_thd_val_high,  noise+180);
		atomic_set(&obj->ps_thd_val_low, noise+90);
	}else if(noise < 1000){
		atomic_set(&obj->ps_thd_val_high,  noise+300);
		atomic_set(&obj->ps_thd_val_low, noise+180);	
	}else if(noise < 1250){
		atomic_set(&obj->ps_thd_val_high,  noise+400);
		atomic_set(&obj->ps_thd_val_low, noise+300);
	}
	else{
		atomic_set(&obj->ps_thd_val_high,  1450);
		atomic_set(&obj->ps_thd_val_low, 1000);
		APS_ERR("ltr558 the proximity sensor structure is error\n");
	}
	
	ps_thd_val_low = atomic_read(&obj->ps_thd_val_low);
	ps_thd_val_high = atomic_read(&obj->ps_thd_val_high);
	APS_DBG("LTR-55X ps_thd_val_high=%d, ps_thd_val_low=%d\n.",ps_thd_val_high,ps_thd_val_low);

   	 error = ltr303_i2c_write_reg(LTR303_PS_THRES_UP_0, ps_thd_val_high & 0xff); 
    	if(error<0)
    	{
        	APS_LOG("ltr303_ps_enable setting LTR303_PS_THRES_UP_0 error \n");
       	 return error;
    	} 
   	 error = ltr303_i2c_write_reg(LTR303_PS_THRES_UP_1, (ps_thd_val_high>>8) & 0X07);
    	if(error<0)
    	{
        	APS_LOG("ltr303_ps_enable setting LTR303_PS_THRES_UP_1 error \n");
        	return error;
    	} 
    	error = ltr303_i2c_write_reg(LTR303_PS_THRES_LOW_0, 0x0); 
    	if(error<0)
    	{
        	APS_LOG("ltr303_ps_enable setting LTR303_PS_THRES_LOW_0 error \n");
        	return error;
    	} 
    	error = ltr303_i2c_write_reg(LTR303_PS_THRES_LOW_1, 0x0); 
   	 if(error<0)
    	{
        	APS_LOG("ltr303_ps_enable setting LTR303_PS_THRES_LOW_1 error \n");
        	return error;
    	} 

	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
static ssize_t ltr303_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}
	res = ltr303_als_read(als_gainrange);
    return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);    
	
}

/*----------------------------------------------------------------------------*/
static ssize_t ltr303_show_ps(struct device_driver *ddri, char *buf)
{
	int  res;
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}
	res = ltr303_ps_read();
    return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);     
}

/*----------------------------------------------------------------------------*/
static ssize_t ltr303_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}
	
	if(ltr303_obj->hw)
	{
	
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			ltr303_obj->hw->i2c_num, ltr303_obj->hw->power_id, ltr303_obj->hw->power_vol);
		
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}


	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&ltr303_obj->als_suspend), atomic_read(&ltr303_obj->ps_suspend));

	return len;
}

/*----------------------------------------------------------------------------*/
/*
static ssize_t ltr303_store_status(struct device_driver *ddri, char *buf, size_t count)
{
	int status1,ret;
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "%d ", &status1))
	{ 
	    ret=ltr303_ps_enable(ps_gainrange);
		APS_DBG("iret= %d, ps_gainrange = %d\n", ret, ps_gainrange);
	}
	else
	{
		APS_DBG("invalid content: '%s', length = %ld\n", buf, count);
	}
	return count;    
}
*/

/*----------------------------------------------------------------------------*/
static ssize_t ltr303_show_reg(struct device_driver *ddri, char *buf)
{
	int i,len=0;
	int reg[]={0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
		0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x97,0x98,0x99,0x9a,0x9e};
	for(i=0;i<27;i++)
		{
		len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%04X value: 0x%04X\n", reg[i],ltr303_i2c_read_reg(reg[i]));	

	    }
	return len;
}
/*----------------------------------------------------------------------------*/
/*
static ssize_t ltr303_store_reg(struct device_driver *ddri, char *buf, size_t count)
{
	int ret,value;
	u32 reg;
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}
	
	if(2 == sscanf(buf, "%x %x ", &reg,&value))
	{ 
		APS_DBG("before write reg: %x, reg_value = %x  write value=%x\n", reg,ltr303_i2c_read_reg(reg),value);
	    ret=ltr303_i2c_write_reg(reg,value);
		APS_DBG("after write reg: %x, reg_value = %x\n", reg,ltr303_i2c_read_reg(reg));
	}
	else
	{
		APS_DBG("invalid content: '%s', length = %ld\n", buf, count);
	}
	return count;    
}
*/

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(als,S_IWUSR | S_IRUGO,ltr303_show_als,NULL);
static DRIVER_ATTR(ps,S_IWUSR | S_IRUGO,ltr303_show_ps,NULL);
static DRIVER_ATTR(status,S_IWUSR | S_IRUGO,ltr303_show_status,NULL);
static DRIVER_ATTR(reg,S_IWUSR | S_IRUGO,ltr303_show_reg,NULL);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *ltr303_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,    
    &driver_attr_status,
    &driver_attr_reg,
};

/*----------------------------------------------------------------------------*/
static int ltr303_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(ltr303_attr_list)/sizeof(ltr303_attr_list[0]));

	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
               if(0 != (err = driver_create_file(driver, ltr303_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", ltr303_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}

/*----------------------------------------------------------------------------*/
	static int ltr303_delete_attr(struct device_driver *driver)
	{
	int idx ,err = 0;
	int num = (int)(sizeof(ltr303_attr_list)/sizeof(ltr303_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, ltr303_attr_list[idx]);
	}
	
	return err;
}

/* 
 * ###############
 * ## PS CONFIG ##
 * ###############
 */
/*----------------------------------------------------------------------------*/
static int ltr303_ps_enable(int gainrange)
{
	int res;
	int data;
	int error;
	int setgain;
    	APS_LOG("ltr303_ps_enable() ...start!\n");
	readenable =true;	
	switch (gainrange) {
		case PS_RANGE16:
			setgain = MODE_PS_ON_Gain16;
			break;

		case PS_RANGE32:
			setgain = MODE_PS_ON_Gain32;
			break;

		case PS_RANGE64:
			setgain = MODE_PS_ON_Gain64;
			break;


		default:
			setgain = MODE_PS_ON_Gain16;
			break;
	}

	res = ltr303_init_device();
	if (res < 0)
	{
	   APS_ERR("ltr303_init_devicet: %d\n", res);
	   goto EXIT_ERR;
	}

	APS_LOG("LTR303_PS setgain = %d!\n",setgain);

	error = ltr303_i2c_write_reg(LTR303_PS_CONTR, setgain); 
	if(error<0)
	{
	    APS_LOG("ltr303_ps_enable() error1\n");
	    return error;
	}
	
	data = ltr303_i2c_read_reg(LTR303_PS_CONTR);
	
	#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI  
	if (data & 0x02) {	
		if (ltr303_dynamic_calibrate() < 0)
			return -1;
	}
	#endif	

 	APS_LOG("ltr303_ps_enable ...OK!\n");

	return error;

	EXIT_ERR:
	APS_ERR("set thres: %d\n", res);
	return res;
}

/*----------------------------------------------------------------------------*/
// Put PS into Standby mode
static int ltr303_ps_disable(void)
{
	int error;
	struct ltr303_priv *obj = ltr303_obj;
	readenable =false;
	error = ltr303_i2c_write_reg(LTR303_PS_CONTR, MODE_PS_StdBy); 
	if(error<0)
 	    APS_LOG("ltr303_ps_disable ...ERROR\n");
 	else
        APS_LOG("ltr303_ps_disable ...OK\n");

	if(0 == obj->hw->polling_mode_ps)
	{
	    cancel_work_sync(&obj->eint_work);
	}
	
	return error;
}

/*----------------------------------------------------------------------------*/
static int ltr303_ps_read(void)
{
	int psval_lo, psval_hi, psdata;

	psval_lo = ltr303_i2c_read_reg(LTR303_PS_DATA_0);
	APS_DBG("ps_rawdata_psval_lo = %d\n", psval_lo);
	if (psval_lo < 0){
	    
	    APS_DBG("psval_lo error\n");
		psdata = psval_lo;
		goto out;
	}
	psval_hi = ltr303_i2c_read_reg(LTR303_PS_DATA_1);
    APS_DBG("ps_rawdata_psval_hi = %d\n", psval_hi);

	if (psval_hi < 0){
	    APS_DBG("psval_hi error\n");
		psdata = psval_hi;
		goto out;
	}
	
	psdata = ((psval_hi & 7)* 256) + psval_lo;
    //psdata = ((psval_hi&0x7)<<8) + psval_lo;
    APS_DBG("ps_rawdata = %d\n", psdata);

	prox_val = psdata;
    
	out:
	final_prox_val = psdata;
	
	
	return psdata;
}

/* 
 * ################
 * ## ALS CONFIG ##
 * ################
 */

/*----------------------------------------------------------------------------*/
static int ltr303_als_enable(int gainrange)
{
	int error;

	APS_LOG("gainrange = %d\n",gainrange);
      readenable =true;
	switch (gainrange)
	{
		case ALS_RANGE_64K:
			error = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range1);
			break;

		case ALS_RANGE_32K:
			error = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range2);
			break;

		case ALS_RANGE_16K:
			error = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range3);
			break;
			
		case ALS_RANGE_8K:
			error = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range4);
			break;
			
		case ALS_RANGE_1300:
			error = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range5);
			break;

		case ALS_RANGE_600:
			error = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range6);
			break;
			
		default:
			error = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range1);			
			APS_LOG("proxmy sensor gainrange %d!\n", gainrange);
			break;
	}

 	if(error<0)
 	    APS_LOG("ltr303_als_enable ...ERROR\n");
 	else
        APS_LOG("ltr303_als_enable ...OK\n");

	mdelay(WAKEUP_DELAY);

	ltr303_i2c_read_reg(LTR303_ALS_CONTR);
        
	return error;
}

/*----------------------------------------------------------------------------*/
// Put ALS into Standby mode
static int ltr303_als_disable(void)
{
	int error;
	 readenable =false;
	error = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_StdBy); 
	if(error<0)
 	    APS_LOG("ltr303_als_disable ...ERROR\n");
 	else
        APS_LOG("ltr303_als_disable ...OK\n");
	return error;
}

/*----------------------------------------------------------------------------*/
static int ltr303_als_read(int gainrange)
{
	int alsval_ch0_lo, alsval_ch0_hi, alsval_ch0;
	int alsval_ch1_lo, alsval_ch1_hi, alsval_ch1;
	int  luxdata_int = -1;
	int ratio;

	alsval_ch1_lo = ltr303_i2c_read_reg(LTR303_ALS_DATA_CH1_0);
	alsval_ch1_hi = ltr303_i2c_read_reg(LTR303_ALS_DATA_CH1_1);
	alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;
	APS_DBG("alsval_ch1_lo = %d,alsval_ch1_hi=%d,alsval_ch1=%d\n",alsval_ch1_lo,alsval_ch1_hi,alsval_ch1);
	
	alsval_ch0_lo = ltr303_i2c_read_reg(LTR303_ALS_DATA_CH0_0);
	alsval_ch0_hi = ltr303_i2c_read_reg(LTR303_ALS_DATA_CH0_1);
	alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;
	APS_DBG("alsval_ch0_lo = %d,alsval_ch0_hi=%d,alsval_ch0=%d\n",alsval_ch0_lo,alsval_ch0_hi,alsval_ch0);

    if((alsval_ch1==0)||(alsval_ch0==0))
    {
        luxdata_int = 0;
        goto err;
    }
	ratio = (alsval_ch1*100) /(alsval_ch0+alsval_ch1);
	APS_DBG("ratio = %d  gainrange = %d\n",ratio,gainrange);
	if (ratio < 45){
		luxdata_int = (((17743 * alsval_ch0)+(11059 * alsval_ch1)))/10000;
	}
	else if ((ratio < 64) && (ratio >= 45)){
		luxdata_int = (((42785 * alsval_ch0)-(19548 * alsval_ch1)))/10000;
	}
	else if ((ratio <= 100) && (ratio >= 64)) {
		luxdata_int = (((5926 * alsval_ch0)+(1185 * alsval_ch1)))/10000;
	}
	else {
		luxdata_int = 0;
		}
	#if defined(CONFIG_LTRKARIN)
        luxdata_int = luxdata_int * ALS_BQ_CALIBRATION_KARIN / ALS_BQ_CALIBRATION_DIV_KARIN; 
		APS_DBG("Calibration factor aplicated for Karin");
  	#endif

	APS_DBG("als_value_lux = %d\n", luxdata_int);
	return luxdata_int;

err:
	final_lux_val = luxdata_int;
	APS_DBG("err als_value_lux = 0x%x\n", luxdata_int);
	return luxdata_int;
}

/*-----------------------------------------------------------------------------*/
void ltr303_eint_func(void)
{
        struct ltr303_priv *obj = ltr303_obj;
        APS_FUN();
        if(!obj)
        {
        return;
        }
        int_top_time = sched_clock();
        schedule_work(&obj->eint_work);
}

/*----------------------------------------------------------------------------*/
#if defined(CONFIG_OF)
static irqreturn_t ltr303_eint_handler(int irq, void *desc)
{
	ltr303_eint_func();
	disable_irq_nosync(ltr303_obj->irq);

	return IRQ_HANDLED;
}
#endif

/*----------------------------------------------------------------------------*/
int ltr303_setup_eint(struct i2c_client *client)
{
	int ret;
	struct pinctrl * pinctrl;
	struct pinctrl_state * pins_default;
	struct pinctrl_state * pins_cfg;
	u32 ints[2] = {0,0};
        APS_FUN();
	alspsPltFmDev = get_alsps_platformdev();
/* gpio setting */
	pinctrl = devm_pinctrl_get(&alspsPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		APS_ERR("Cannot find alsps pinctrl!\n");
		return ret;
	}
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		APS_ERR("Cannot find alsps pinctrl default!\n");
	}
	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		APS_ERR("Cannot find alsps pinctrl pin_cfg!\n");
		return ret;
	}
	pinctrl_select_state(pinctrl, pins_cfg);
/* eint request */
	if (ltr303_obj->irq_node) {
		of_property_read_u32_array(ltr303_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_request(ints[0], "p-sensor");
		gpio_set_debounce(ints[0], ints[1]);
		APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);

		ltr303_obj->irq = irq_of_parse_and_map(ltr303_obj->irq_node, 0);
		APS_LOG("ltr303_obj->irq = %d\n", ltr303_obj->irq);
		if (!ltr303_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}

		if (request_irq(ltr303_obj->irq, ltr303_eint_handler, IRQF_TRIGGER_NONE, "mediatek,ltr303", NULL)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
	//	enable_irq(ltr303_obj->irq);
	} else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
static void ltr303_power(struct alsps_hw *hw, unsigned int on) 
{
        /*TODO:power on*/
        static unsigned int power_on = 0;
/*
	if(hw->power_id != POWER_NONE_MACRO)
	{
		if(power_on == on)
		{
			APS_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "LTR303")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "LTR303")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}*/
	power_on = on;
}

/*----------------------------------------------------------------------------*/
static int ltr303_check_and_clear_intr(struct i2c_client *client) 
{
	int res = 0;
        int intp,intl;
	u8 buffer[2];	
	u8 temp;
	APS_FUN();
	buffer[0] = LTR303_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	temp = buffer[0];
	res = 1;
	intp = 0;
	intl = 0;
	if(0 != (buffer[0] & 0x02))
	{
		res = 0;
		intp = 1;
	}
	if(0 != (buffer[0] & 0x08))
	{
		res = 0;
		intl = 1;		
	}

	if(0 == res)
	{
		if((1 == intp) && (0 == intl))
		{
			buffer[1] = buffer[0] & 0xfD;
			
		}
		else if((0 == intp) && (1 == intl))
		{
			buffer[1] = buffer[0] & 0xf7;
		}
		else
		{
			buffer[1] = buffer[0] & 0xf5;
		}
		buffer[0] = LTR303_ALS_PS_STATUS	;
		res = i2c_master_send(client, buffer, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		else
		{
			res = 0;
		}
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr303_check_and_clear_intr fail\n");
	return 1;

}

/*----------------------------------------------------------------------------*/
static int ltr303_check_intr(struct i2c_client *client) 
{
	int res,intp,intl;
	u8 buffer[2];

        APS_FUN();
	buffer[0] = LTR303_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = 1;
	intp = 0;
	intl = 0;
	if(0 != (buffer[0] & 0x02))
	{
		res = 0; //Ps int
		intp = 1;
	}
	if(0 != (buffer[0] & 0x08))
	{
		res = 0;
		intl = 1;		
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr303_check_intr fail\n");
	return 1;
}

/*----------------------------------------------------------------------------*/
static int ltr303_clear_intr(struct i2c_client *client) 
{
	int res;
	u8 buffer[2];

	APS_FUN();
	
	buffer[0] = LTR303_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	APS_DBG("buffer[0] = %d \n",buffer[0]);
	buffer[1] = buffer[0] & 0x01;
	buffer[0] = LTR303_ALS_PS_STATUS	;

	res = i2c_master_send(client, buffer, 0x2);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	else
	{
		res = 0;
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr303_check_and_clear_intr fail\n");
	return 1;
}

/*----------------------------------------------------------------------------*/
static int ltr303_init_device(void)
{
	int res;
	int error = 0;
	u8 databuf[2];	
        
	struct ltr303_priv *obj = ltr303_obj;   
	struct i2c_client *client = ltr303_obj->client;	

	ps_gainrange = PS_RANGE16;

	als_gainrange = ALS_RANGE_1300;
       APS_FUN();
	error = ltr303_i2c_write_reg(LTR303_PS_LED, 0x11); // 0x1f=0001 1111;0x1b=0001 1011;0x1a=0001 1010;0x12=0001 0010;0x11=0001 0001;
	if(error<0)
    	{
        	APS_LOG("ltr303_ps_enable setting LTR303_PS_LED error...\n");
	    	return error;
	}
	error = ltr303_i2c_write_reg(LTR303_PS_N_PULSES, 0xa); 
	if(error<0)
   	{
        	APS_LOG("ltr303_ps_enable setting LTR303_PS_N_PULSES error..\n");
	    	return error;
	} 
	error = ltr303_i2c_write_reg(LTR303_PS_MEAS_RATE, 0x0); 
	if(error<0)
	{
        	APS_LOG("ltr303_ps_enable setting LTR303_PS_MEAS_RATE error\n");
        	return error;
	} 
	error = ltr303_i2c_write_reg(LTR303_ALS_MEAS_RATE, 0x02); 
	if(error<0)
	{
        APS_LOG("ltr303_ps_enable setting LTR303_ALS_MEAS_RATE error.\n");
        return error;
	} 

	/*for interrup work mode support */
	if(0 == obj->hw->polling_mode_ps)
	{	
		databuf[0] = LTR303_INTERRUPT;	
		databuf[1] = 0x01;
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			APS_LOG("LTR303_INTERRUPT error: %x\n",res);
			return LTR303_ERR_I2C;
		}

		databuf[0] = LTR303_INTERRUPT_PERSIST;	
		databuf[1] = 0x20;
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			APS_LOG("LTR303_INTERRUPT_PERSIST error: %x\n",res);
			return LTR303_ERR_I2C;
		}

	}

#ifndef GN_MTK_BSP_PS_DYNAMIC_CALI  
   	 error = ltr303_i2c_write_reg(LTR303_PS_THRES_UP_0, ps_trigger_high & 0xff); 
    	if(error<0)
    	{
        	APS_LOG("ltr303_ps_enable setting LTR303_PS_THRES_UP_0 error \n");
       	 return error;
    	} 
   	 error = ltr303_i2c_write_reg(LTR303_PS_THRES_UP_1, (ps_trigger_high>>8) & 0X07);
    	if(error<0)
    	{
        	APS_LOG("ltr303_ps_enable setting LTR303_PS_THRES_UP_1 error \n");
        	return error;
    	} 
    	error = ltr303_i2c_write_reg(LTR303_PS_THRES_LOW_0, 0x0); 
    	if(error<0)
    	{
        	APS_LOG("ltr303_ps_enable setting LTR303_PS_THRES_LOW_0 error \n");
        	return error;
    	} 
    	error = ltr303_i2c_write_reg(LTR303_PS_THRES_LOW_1, 0x0); 
   	 if(error<0)
    	{
        	APS_LOG("ltr303_ps_enable setting LTR303_PS_THRES_LOW_1 error \n");
        	return error;
    	} 
#endif

    mdelay(WAKEUP_DELAY);
    return error;

}

/*----------------------------------------------------------------------------*/
static int ltr303_devinit(void)
{
	int res;
	u8 databuf[2];	
	u8 chip_id;
  
	struct i2c_client *client = ltr303_obj->client;
	
	mdelay(PON_DELAY);

	/* check whether is the LT's sensor. */
	chip_id = ltr303_i2c_read_reg(LTR303_PART_ID);
	if(chip_id == 0xA0){
		APS_LOG("LTR-303ALS is detected.\n");
	}else if(chip_id == 0x80){
		APS_LOG("LTR-558ALS is detected.\n");
	}else{
		APS_LOG("Detected LTR-303 chip_id %d error.\n",chip_id);
	}

	/*reset the sensor when first reading or writing.*/
	databuf[0] = LTR303_ALS_CONTR;	
	databuf[1] = 0x00;
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		goto EXIT_ERR;
		return LTR303_ERR_I2C;
	}

	if((res = ltr303_setup_eint(client))!=0)
	{
		APS_ERR("setup eint: %d\n", res);
		return res;
	}

	res = ltr303_init_device();
	if (res < 0)
	{
	   APS_ERR("ltr303_init_devicet: %d\n", res);
		return res;
	}

	if((res = ltr303_check_and_clear_intr(client)))
	{
		APS_ERR("check/clear intr: %d\n", res);
		//    return res;
	}

	res = 0;
//    enable_irq(ltr303_obj->irq);
	EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return res;

}

/*----------------------------------------------------------------------------*/
static int ltr303_get_als_value(struct ltr303_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
	APS_DBG("als  = %d\n",als); 
	for(idx = 0; idx < obj->als_level_num; idx++)
	{
		if(als < obj->hw->als_level[idx])
		{
			break;
		}
	}
	
	if(idx >= obj->als_value_num)
	{
		APS_ERR("exceed range\n"); 
		idx = obj->als_value_num - 1;
	}
	
	if(1 == atomic_read(&obj->als_deb_on))
	{
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->als_deb_on, 0);
		}
		
		if(1 == atomic_read(&obj->als_deb_on))
		{
			invalid = 1;
		}
	}
	if(!invalid)
	{
		APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);	
		return obj->hw->als_value[idx];
	}
	else
	{
		APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		return -1;
	}
}

/*----------------------------------------------------------------------------*/
static int ltr303_get_ps_value(struct ltr303_priv *obj, u16 ps)
{
	int val;
	int invalid = 0;
	static int val_temp = 1;
	int ps_thd_val_high = atomic_read(&obj->ps_thd_val_high);
	int ps_thd_val_low = atomic_read(&obj->ps_thd_val_low);

	APS_DBG("ltr303_get_ps_value: - ps:%d ps_trigger_high:%d ps_trigger_low:%d\n",
		    ps,ps_thd_val_high,ps_thd_val_low);
	if((ps > ps_thd_val_high))
	{
		val = 0;  /*close*/
		val_temp = 0;
		intr_flag_value = 1;
	} else if((ps < ps_thd_val_low)) {
		val = 1;  /*far away*/
		val_temp = 1;
		intr_flag_value = 0;
	} else {
		val = val_temp;	
	}
	
	if(atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}
	else if(1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}
		
		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}
	else if (obj->als > 50000)
	{
		//invalid = 1;
		APS_DBG("ligh too high will result to failt proximiy\n");
		return 1;  /*far away*/
	}

	if(!invalid)
	{
		APS_DBG("PS:  %05d => %05d\n", ps, val);
		return val;
	}	
	else
	{
		return -1;
	}	
}

/*----------------------------------------------------------------------------*/
/*for interrup work mode support */
static void ltr303_eint_work(struct work_struct *work)
{
	struct ltr303_priv *obj = (struct ltr303_priv *)container_of(work, struct ltr303_priv, eint_work);
	int err;
	struct hwm_sensor_data sensor_data;
	int temp_noise=0;	 
	u8 databuf[2];
	int res = 0;
	APS_FUN();
	APS_LOG("ltr303 int top half time = %lld\n", int_top_time);
	err = ltr303_check_intr(obj->client);
	if(err < 0)
	{
		APS_ERR("ltr303_eint_work check intrs: %d\n", err);
	}
	else
	{
		obj->ps = ltr303_ps_read();
    		if(obj->ps < 0)
    		{
    			err = -1;
    			return;
    		}
				
		APS_DBG("ltr303_eint_work rawdata ps=%d als_ch0=%d!\n",obj->ps,obj->als);
		sensor_data.values[0] = ltr303_get_ps_value(obj, obj->ps);
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;	
		/*singal interrupt function add*/
		//APS_DBG("intr_flag_value=%d,ps_thd_val_high=%d,ps_thd_val_low=%d\n",intr_flag_value,(atomic_read(&obj->ps_thd_val_high)),(atomic_read(&obj->ps_thd_val_low)));
		if(intr_flag_value){
			APS_DBG(" interrupt value ps will < ps_threshold_low");

			databuf[0] = LTR303_PS_THRES_LOW_0;	
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR303_PS_THRES_LOW_1;	
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR303_PS_THRES_UP_0;	
			databuf[1] = (u8)(0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR303_PS_THRES_UP_1; 
			databuf[1] = (u8)((0xFF00) >> 8);;
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
		}
		else{		
			if(dynamic_calibrate > 50 && obj->ps < (dynamic_calibrate - 50)){ 
				 if(obj->ps < 100){			
					atomic_set(&obj->ps_thd_val_high,  obj->ps+100);
					atomic_set(&obj->ps_thd_val_low, obj->ps+50);
				}else if(obj->ps < 200){
					atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
					atomic_set(&obj->ps_thd_val_low, obj->ps+60);
				}else if(obj->ps < 300){
					atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
					atomic_set(&obj->ps_thd_val_low, obj->ps+60);
				}else if(obj->ps < 400){
					atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
					atomic_set(&obj->ps_thd_val_low, obj->ps+60);
				}else if(obj->ps < 600){
					atomic_set(&obj->ps_thd_val_high,  obj->ps+180);
					atomic_set(&obj->ps_thd_val_low, obj->ps+90);
				}else if(obj->ps < 1000){
					atomic_set(&obj->ps_thd_val_high,  obj->ps+300);
					atomic_set(&obj->ps_thd_val_low, obj->ps+180);	
				}else if(obj->ps < 1250){
					atomic_set(&obj->ps_thd_val_high,  obj->ps+400);
					atomic_set(&obj->ps_thd_val_low, obj->ps+300);
				}else{
					atomic_set(&obj->ps_thd_val_high,  1400);
					atomic_set(&obj->ps_thd_val_low, 1000);
					APS_ERR("ltr303 the proximity sensor structure is error\n");
				}
		
				dynamic_calibrate = obj->ps;

				if(obj->ps	> 50){
					temp_noise = obj->ps - 50;
				}else{
					temp_noise = 0;
				}
			}	
			
			databuf[0] = LTR303_PS_THRES_LOW_0; 
			databuf[1] = (u8)(temp_noise & 0x00FF);//get the noise one time 
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR303_PS_THRES_LOW_1; 
			databuf[1] = (u8)((temp_noise & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR303_PS_THRES_UP_0;	
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR303_PS_THRES_UP_1; 
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_high)) & 0xFF00) >> 8);;
			res = i2c_master_send(obj->client, databuf, 0x2);
            APS_DBG("temp_noise=%d,ps_thd_val_high=%d,ps_thd_val_low=%d\n",temp_noise,(atomic_read(&obj->ps_thd_val_high)),(atomic_read(&obj->ps_thd_val_low)));
			if(res <= 0)
			{
				return;
			}
		}
		
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		//let up layer to know
		if ((err = ps_report_interrupt_data(sensor_data.values[0]))) {
		 	APS_ERR("call ps_report_interrupt_data fail = %d\n", err);
		}
	}
	ltr303_clear_intr(obj->client);
#ifdef CONFIG_OF
        enable_irq(obj->irq);
#endif       
}

/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int ltr303_open(struct inode *inode, struct file *file)
{
	file->private_data = ltr303_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
	return nonseekable_open(inode, file);
}

/*----------------------------------------------------------------------------*/
static int ltr303_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

/*----------------------------------------------------------------------------*/
static long ltr303_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)       
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct ltr303_priv *obj = i2c_get_clientdata(client);  
	int err = 0;
	void __user *ptr = (void __user*) arg;
	int dat;
	uint32_t enable;
	APS_DBG("ltr303_unlocked_ioctl cmd= %d\n", cmd); 
	switch (cmd)
	{
		case ALSPS_SET_PS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
			    err = ltr303_ps_enable(ps_gainrange);
				if(err < 0)
				{
					APS_ERR("enable ps fail: %d\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_PS, &obj->enable);
			}
			else
			{
			    err = ltr303_ps_disable();
				if(err < 0)
				{
					APS_ERR("disable ps fail: %d\n", err); 
					goto err_out;
				}
				
				clear_bit(CMC_BIT_PS, &obj->enable);
			}
			break;

		case ALSPS_GET_PS_MODE:
			enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_DATA:
			APS_DBG("ALSPS_GET_PS_DATA\n"); 
		    obj->ps = ltr303_ps_read();
			if(obj->ps < 0)
			{
				goto err_out;
			}
			
			dat = ltr303_get_ps_value(obj, obj->ps);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_GET_PS_RAW_DATA:
			
			#if 1
			obj->ps = ltr303_ps_read();
			if(obj->ps < 0)
			{
				goto err_out;
			}
			dat = obj->ps;
			#endif
			
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_SET_ALS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
			    err = ltr303_als_enable(als_gainrange);
				if(err < 0)
				{
					APS_ERR("enable als fail: %d\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_ALS, &obj->enable);
			}
			else
			{
			    err = ltr303_als_disable();
				if(err < 0)
				{
					APS_ERR("disable als fail: %d\n", err); 
					goto err_out;
				}
				clear_bit(CMC_BIT_ALS, &obj->enable);
			}
			break;

		case ALSPS_GET_ALS_MODE:
			enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_DATA: 
		    obj->als = ltr303_als_read(als_gainrange);
			if(obj->als < 0)
			{
				goto err_out;
			}

			dat = ltr303_get_als_value(obj, obj->als);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		case ALSPS_GET_ALS_RAW_DATA:    
			obj->als = ltr303_als_read(als_gainrange);
			if(obj->als < 0)
			{
				goto err_out;
			}

			dat = obj->als;
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		default:
			APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
			err = -ENOIOCTLCMD;
			break;
	}

	err_out:
	return err;    
}

/*----------------------------------------------------------------------------*/
static struct file_operations ltr303_fops = {
	.owner = THIS_MODULE,
	.open = ltr303_open,
	.release = ltr303_release,
	.unlocked_ioctl = ltr303_unlocked_ioctl,
};

/*----------------------------------------------------------------------------*/
static struct miscdevice ltr303_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ltr303_fops,
};

/*----------------------------------------------------------------------------*/
static int ltr303_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct ltr303_priv *obj = i2c_get_clientdata(client);    
	int err;
	APS_FUN();    

	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(!obj)
		{
			APS_ERR("null pointer!!\n");
			return -EINVAL;
		}
		
		atomic_set(&obj->als_suspend, 1);
		err = ltr303_als_disable();
		if(err < 0)
		{
			APS_ERR("disable als: %d\n", err);
			return err;
		}

		#if 0		//suspend not need ps suspend  not need power down
		atomic_set(&obj->ps_suspend, 1);
		err = ltr303_ps_disable();
		if(err < 0)
		{
			APS_ERR("disable ps:  %d\n", err);
			return err;
		}
				
		ltr303_power(obj->hw, 0);

		#endif
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_i2c_resume(struct i2c_client *client)
{
	struct ltr303_priv *obj = i2c_get_clientdata(client);        
	int err;
	err = 0;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	ltr303_power(obj->hw, 1);
/*	err = ltr303_devinit();
	if(err < 0)
	{
		APS_ERR("initialize client fail!!\n");
		return err;        
	}*/
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
	    err = ltr303_als_enable(als_gainrange);
	    if (err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        
		}
	}

	if(test_bit(CMC_BIT_PS,  &obj->enable))
	{
		//err = ltr303_ps_enable(ps_gainrange);
	    if (err < 0)
		{
			APS_ERR("enable ps fail: %d\n", err);                
		}
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_als_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_als_enable_nodata(int enable)
{
	int err=0;
	APS_FUN();	
	if (ltr303_obj == NULL){
		APS_ERR("ltr303_i2c_client is null!!\n");
		return -1;
	}
	
	if (enable > 0) {
		if (0 != (err = ltr303_als_enable(als_gainrange))) {
			APS_ERR("enable als fail: %d\n", err);
			return err;
		}
		set_bit(CMC_BIT_ALS, &ltr303_obj->enable);
	} else {
		if (0 != (err = ltr303_als_disable())) {
			APS_ERR("disable als fail: %d\n", err);
			return err;
		}
		clear_bit(CMC_BIT_ALS, &ltr303_obj->enable);
	}	
	
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_als_set_delay(u64 ns)
{
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_als_get_data(int* value, int* status)
{
	int alsraw=0;
	APS_FUN();
	if (ltr303_obj == NULL) {
		APS_ERR("ltr303_i2c_client is null!!\n");
		return -EINVAL;
	} else {
	      if(readenable){
			alsraw = ltr303_als_read(als_gainrange);
			*value=  ltr303_get_als_value(ltr303_obj, alsraw);
			*status = SENSOR_STATUS_ACCURACY_MEDIUM;
			//APS_ERR("als_get_data *value = %d, *status =%d\n",*value, *status);
		}
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_als_register(void)
{
	int err = 0;
	struct als_control_path als_ctl={0};
	struct als_data_path als_data={0};
	
	als_ctl.open_report_data= ltr303_als_open_report_data;
	als_ctl.enable_nodata = ltr303_als_enable_nodata;
	als_ctl.set_delay  = ltr303_als_set_delay;
	als_ctl.is_report_input_direct = false;
#if 0//def CUSTOM_KERNEL_SENSORHUB
	als_ctl.is_support_batch = obj->hw->is_batch_supported_als;
#else
	als_ctl.is_support_batch = false;
#endif

	err = als_register_control_path(&als_ctl);
	if (err) {
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	als_data.get_data = ltr303_als_get_data;
	als_data.vender_div = 100;
	err = als_register_data_path(&als_data);	
	if (err) {
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	err = batch_register_support_info(ID_LIGHT,als_ctl.is_support_batch, 100, 0);
	if (err) {
		APS_ERR("register light batch support err = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	exit_sensor_obj_attach_fail:
		return err;
}

/*----------------------------------------------------------------------------*/
static int ltr303_ps_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_ps_enable_nodata(int enable)
{
	int err=0;
	APS_FUN();	
	if (ltr303_obj == NULL){
		APS_ERR("ltr303_i2c_client is null!!\n");
		return -1;
	}
	
	if (enable > 0) {
		if (0 != (err = ltr303_ps_enable(ps_gainrange))) {
			APS_ERR("enable ps fail: %d\n", err);
			return err;
		}
		set_bit(CMC_BIT_PS, &ltr303_obj->enable);
	} else {
		if (0 != (err = ltr303_ps_disable())) {
			APS_ERR("disable ps fail: %d\n", err);
			return err;
		}
		clear_bit(CMC_BIT_PS, &ltr303_obj->enable);
	}	
	
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_ps_set_delay(u64 ns)
{
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_ps_get_data(int* value, int* status)
{
	int psraw=0;

	APS_FUN();
	if (ltr303_obj == NULL) {
		APS_ERR("ltr303_i2c_client is null!!\n");
		return -EINVAL;
	} else {
		psraw = ltr303_ps_read();
		*value= ltr303_get_ps_value(ltr303_obj, psraw);
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
		//APS_ERR("als_get_data *value = %d, *status =%d\n",*value, *status);
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_ps_register(void)
{
	int err = 0;
	struct ps_control_path ps_ctl={0};
	struct ps_data_path ps_data={0};
	
	ps_ctl.open_report_data= ltr303_ps_open_report_data;
	ps_ctl.enable_nodata = ltr303_ps_enable_nodata;
	ps_ctl.set_delay  = ltr303_ps_set_delay;
	ps_ctl.is_report_input_direct = false;
#if 0//def CUSTOM_KERNEL_SENSORHUB
	ps_ctl.is_support_batch = obj->hw->is_batch_supported_als;
#else
	ps_ctl.is_support_batch = false;
#endif
	ps_ctl.is_polling_mode= false;
		
	err = ps_register_control_path(&ps_ctl);
	if (err) {
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

        if((NULL!=ltr303_obj)){//xmxl
            ps_data.get_data = ltr303_ps_get_data;
            ps_data.vender_div = 100;
            err = ps_register_data_path(&ps_data);	
            if (err) {
                APS_ERR("tregister fail = %d\n", err);
                goto exit_sensor_obj_attach_fail;
            }
        }

	err = batch_register_support_info(ID_PROXIMITY,ps_ctl.is_support_batch, 100, 0);
	if (err) {
		APS_ERR("register PROXIMITY batch support err = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	exit_sensor_obj_attach_fail:
		return err;
}

/*----------------------------------------------------------------------------*/
static int ltr303_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ltr303_priv *obj;
	int err = 0;

	APS_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ltr303_obj = obj;

	obj->hw = hw;

	INIT_WORK(&obj->eint_work, ltr303_eint_work);
	obj->client = client;
	i2c_set_clientdata(client, obj);	

	atomic_set(&obj->als_debounce, 300);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 300);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
	//atomic_set(&obj->als_cmd_val, 0xDF);
	//atomic_set(&obj->ps_cmd_val,  0xC1);
	atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
    obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek,ltr303");
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);   
	obj->als_modulus = (400*100)/(16*150);//(1/Gain)*(400/Tine), this value is fix after init ATIME and CONTROL register value
										//(400)/16*2.72 here is amplify *100
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);

	ltr303_i2c_client = client;
	APS_LOG("ltr303_devinit() start...!\n");
	if(0 != (err = ltr303_devinit()))
	{
		goto exit_init_failed;
	}
	APS_LOG("ltr303_devinit() ...OK!\n");

	if(0 != (err = misc_register(&ltr303_device)))
	{
		APS_ERR("ltr303_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if(0 != (err = ltr303_create_attr(&ltr303_init_info.platform_diver_addr->driver)))
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	if ((0 != (err = ltr303_als_register())) || (0 != (err = ltr303_ps_register()))){
		APS_ERR("register alsps err = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

/*
#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = ltr303_early_suspend,
	obj->early_drv.resume   = ltr303_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif
*/
	ltr303_init_flag = 0;
	APS_LOG("%s: OK\n", __func__);
	return 0;

        exit_create_attr_failed:
        exit_sensor_obj_attach_fail:
	misc_deregister(&ltr303_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	kfree(obj);
	exit:
	ltr303_i2c_client = NULL;           
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}

/*----------------------------------------------------------------------------*/
static int ltr303_i2c_remove(struct i2c_client *client)
{
	int err;	
	if(0 != (err = ltr303_delete_attr(&ltr303_i2c_driver.driver)))
	{
		APS_ERR("ltr303_delete_attr fail: %d\n", err);
	} 

	if(0 != (err = misc_deregister(&ltr303_device)))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
	
	ltr303_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}

/*----------------------------------------------------------------------------*/
static int  ltr303_local_init(void)
{
	APS_FUN();
	ltr303_power(hw, 1);
	if(0 != (i2c_add_driver(&ltr303_i2c_driver)))
	{
		APS_ERR("add driver error\n");
		return -1;
	}
    else
    {
            APS_LOG("i2c_add_driver OK\n");
      }
	if(-1 == ltr303_init_flag)
	{
		return -1;
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_remove(void)
{
	APS_FUN();    
	ltr303_power(hw, 0);    	
	i2c_del_driver(&ltr303_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
static int __init ltr303_init(void)
{
	const char * name = "mediatek,ltr303";
       hw = get_alsps_dts_func(name,hw);
       APS_FUN();
	if(!hw){
		APS_ERR("get dts info fail\n");
               goto EXIT_NOW;
	}
//       i2c_register_board_info(hw->i2c_num, &i2c_ltr303, 1);
	alsps_driver_add(&ltr303_init_info);
EXIT_NOW:
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit ltr303_exit(void)
{
	APS_FUN();
}
/*----------------------------------------------------------------------------*/
module_init(ltr303_init);
module_exit(ltr303_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Waching  Fully");
MODULE_DESCRIPTION("LTR-303ALS Driver");
MODULE_LICENSE("GPL");

