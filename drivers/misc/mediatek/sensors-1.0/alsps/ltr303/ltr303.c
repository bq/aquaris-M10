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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include "cust_alsps.h"
#include "ltr303.h"
#include "alsps.h"

/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define LTR303_DEV_NAME   "ltr303"
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[LTR303]"
#define APS_FUN(f)               pr_debug(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)
/*----------------------------------------------------------------------------*/
static int ltr303_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int ltr303_i2c_remove(struct i2c_client *client);
#ifdef CONFIG_PM_SLEEP
static int ltr303_suspend(struct device *dev);
static int ltr303_resume(struct device *dev);
#endif
static int ltr303_local_init(void);
static int ltr303_remove(void);
/*----------------------------------------------------------------------------*/
static struct i2c_client *ltr303_i2c_client = NULL;
static int als_gainrange = 0;
static int ltr303_init_flag = -1; //0:OK -1:fail
static DEFINE_MUTEX(read_lock);
/*----------------------------------------------------------------------------*/
typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;
/*----------------------------------------------------------------------------*/
struct ltr303_priv {
    struct alsps_hw  hw;
    struct i2c_client *client;
    struct mutex lock;

    /*misc*/
    u16		    als_modulus;
    atomic_t    i2c_retry;
    atomic_t    als_debounce;   /*debounce time after enabling als*/
    atomic_t    als_deb_on;     /*indicates if the debounce is on*/
    atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
    atomic_t    als_suspend;
    atomic_t    trace;
    atomic_t    init_done;

    /*data*/
    u16         als;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    ulong       enable;         /*enable mask*/
};

static struct ltr303_priv *ltr303_obj = NULL;
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ltr303_i2c_id[] = {{LTR303_DEV_NAME,0},{}};

#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{ .compatible = "mediatek,als_ps", },
	{},
};
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops ltr303_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ltr303_suspend, ltr303_resume)
};
#endif
/*----------------------------------------------------------------------------*/
static struct i2c_driver ltr303_i2c_driver = {
	.probe = ltr303_i2c_probe,
	.remove = ltr303_i2c_remove,
	.id_table = ltr303_i2c_id,
	.driver = {
		.name = LTR303_DEV_NAME,
#ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
#endif
#ifdef CONFIG_PM_SLEEP
		.pm = &ltr303_pm_ops,
#endif
	},
};
/*----------------------------------------------------------------------------*/
static struct alsps_init_info ltr303_init_info = {
	.name = "ltr303",
	.init = ltr303_local_init,
	.uninit = ltr303_remove,
};
/*----------------------------------------------------------------------------*/
static int ltr303_i2c_read_reg(u8 regnum)
{
	u8 buffer[1],reg_value[1];
	int res = 0;
	mutex_lock(&read_lock);

	buffer[0]= regnum;
	res = i2c_master_send(ltr303_obj->client, buffer,0x1);
	if (res <= 0) {
		APS_ERR("read reg send res = %d\n",res);
		return res;
	}
	res = i2c_master_recv(ltr303_obj->client, reg_value,0x1);
	if (res <= 0) {
		APS_ERR("read reg recv res = %d\n",res);
		return res;
	}
	mutex_unlock(&read_lock);
	return reg_value[0];
}
/*----------------------------------------------------------------------------*/
static int ltr303_i2c_write_reg(u8 regnum, u8 value)
{
	u8 databuf[2];    
	int res = 0;

	databuf[0] = regnum;   
	databuf[1] = value;
	res = i2c_master_send(ltr303_obj->client, databuf, 0x2);

	if (res < 0) {
		APS_ERR("wirte reg send res = %d\n",res);
		return res;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr303_als_read(int gainrange)
{
	int alsval_ch0_lo, alsval_ch0_hi, alsval_ch0;
	int alsval_ch1_lo, alsval_ch1_hi, alsval_ch1;
	int luxdata_int = -1;
	int ratio;

	alsval_ch1_lo = ltr303_i2c_read_reg(LTR303_ALS_DATA_CH1_0);
	alsval_ch1_hi = ltr303_i2c_read_reg(LTR303_ALS_DATA_CH1_1);
	alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;
	APS_DBG("alsval_ch1_lo = %d,alsval_ch1_hi=%d,alsval_ch1=%d\n",alsval_ch1_lo,alsval_ch1_hi,alsval_ch1);

	alsval_ch0_lo = ltr303_i2c_read_reg(LTR303_ALS_DATA_CH0_0);
	alsval_ch0_hi = ltr303_i2c_read_reg(LTR303_ALS_DATA_CH0_1);
	alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;
	APS_DBG("alsval_ch0_lo = %d,alsval_ch0_hi=%d,alsval_ch0=%d\n",alsval_ch0_lo,alsval_ch0_hi,alsval_ch0);

	if ((alsval_ch1 == 0)||(alsval_ch0 == 0)) {
		luxdata_int = 0;
		goto err;
	}
	ratio = (alsval_ch1*100) /(alsval_ch0+alsval_ch1);
	APS_DBG("ratio = %d  gainrange = %d\n",ratio,gainrange);
	if (ratio < 45)
		luxdata_int = (((17743 * alsval_ch0)+(11059 * alsval_ch1)))/10000;
	else if ((ratio < 64) && (ratio >= 45))
		luxdata_int = (((42785 * alsval_ch0)-(19548 * alsval_ch1)))/10000;
	else if ((ratio <= 100) && (ratio >= 64))
		luxdata_int = (((5926 * alsval_ch0)+(1185 * alsval_ch1)))/10000;
	else
		luxdata_int = 0;

	APS_DBG("als_value_lux = %d\n", luxdata_int);
	return luxdata_int;

err:
	APS_ERR("err als_value_lux = 0x%x\n", luxdata_int);
	return luxdata_int;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_show_als(struct device_driver *ddri, char *buf)
{
	int res;

	if (!ltr303_obj) {
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}
	res = ltr303_als_read(als_gainrange);
	return snprintf(buf, PAGE_SIZE, "0x%04X\n",res);
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;

	if (!ltr303_obj) {
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}

	len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n",
		ltr303_obj->hw.i2c_num, ltr303_obj->hw.power_id, ltr303_obj->hw.power_vol);

	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d\n", atomic_read(&ltr303_obj->als_suspend));

	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_show_reg(struct device_driver *ddri, char *buf)
{
	int i,len = 0;
	int reg[] = {0x80,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8f,0x97,0x98,0x99,0x9a,0x9e};
	for (i = 0;i < 15;i++) {
		len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%04X value: 0x%04X\n", reg[i],ltr303_i2c_read_reg(reg[i]));
	}
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_store_reg(struct device_driver *ddri, const char *buf, size_t count)
{
	int ret,value;
	u32 reg;
	if (!ltr303_obj) {
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}

	if (2 == sscanf(buf, "%x %x ", &reg,&value)) {
		APS_DBG("before write reg: %x, reg_value = %x  write value=%x\n", reg,ltr303_i2c_read_reg(reg),value);
		ret = ltr303_i2c_write_reg(reg,value);
		APS_DBG("after write reg: %x, reg_value = %x\n", reg,ltr303_i2c_read_reg(reg));
	}
	else {
		APS_DBG("invalid content: '%s', length = %ld\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(als,S_IWUSR | S_IRUGO,ltr303_show_als,NULL);
static DRIVER_ATTR(status,S_IWUSR | S_IRUGO,ltr303_show_status,NULL);
static DRIVER_ATTR(reg,S_IWUSR | S_IRUGO,ltr303_show_reg,ltr303_store_reg);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *ltr303_attr_list[] = {
    &driver_attr_als,
    &driver_attr_status,
    &driver_attr_reg,
};
/*----------------------------------------------------------------------------*/
static int ltr303_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(ltr303_attr_list)/sizeof(ltr303_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, ltr303_attr_list[idx]);
		if (err) {
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

	for (idx = 0; idx < num; idx++) {
		driver_remove_file(driver, ltr303_attr_list[idx]);
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr303_als_enable(int gainrange)
{
	int err;

	APS_LOG("gainrange = %d\n",gainrange);

	switch (gainrange) {
	case ALS_RANGE_64K:
		err = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range1);
		break;

	case ALS_RANGE_32K:
		err = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range2);
		break;

	case ALS_RANGE_16K:
		err = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range3);
		break;

	case ALS_RANGE_8K:
		err = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range4);
		break;

	case ALS_RANGE_1300:
		err = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range5);
		break;

	case ALS_RANGE_600:
		err = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range6);
		break;

	default:
		err = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_ON_Range1);
		APS_LOG("light sensor gainrange %d!\n",gainrange);
		break;
	}

 	if (err)
 	    APS_ERR("ltr303_als_enable ...ERROR\n");

	mdelay(WAKEUP_DELAY);

	return err;
}
/*----------------------------------------------------------------------------*/
// Put ALS into Standby mode
static int ltr303_als_disable(void)
{
	int err;
	err = ltr303_i2c_write_reg(LTR303_ALS_CONTR, MODE_ALS_StdBy); 
	if (err)
		APS_ERR("ltr303_als_disable ...ERROR\n");

	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr303_get_als_value(struct ltr303_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
	APS_DBG("als  = %d\n",als); 
	for (idx = 0; idx < obj->als_level_num; idx++) {
		if (als < obj->hw.als_level[idx]) {
			break;
		}
	}

	if (idx >= obj->als_value_num) {
		APS_ERR("exceed range\n"); 
		idx = obj->als_value_num - 1;
	}

	if (1 == atomic_read(&obj->als_deb_on)) {
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if (time_after(jiffies, endt)) {
			atomic_set(&obj->als_deb_on, 0);
		}
		
		if (1 == atomic_read(&obj->als_deb_on)) {
			invalid = 1;
		}
	}
	if (!invalid) {
		APS_DBG("ALS: %05d => %05d\n", als, obj->hw.als_value[idx]);
		return obj->hw.als_value[idx];
	}
	else {
		APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw.als_value[idx]);
		return -1;
	}
}
/*----------------------------------------------------------------------------*/
static int als_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}
/*----------------------------------------------------------------------------*/
static int als_enable_nodata(int enable)
{
	int err = 0;
	APS_FUN();	
	if (!ltr303_obj) {
		APS_ERR("ltr303_i2c_client is null!!\n");
		return -1;
	}

	if (enable > 0) {
		err = ltr303_als_enable(als_gainrange);
		if (err)
			APS_ERR("enable als fail: %d\n", err);
		set_bit(CMC_BIT_ALS, &ltr303_obj->enable);
	}
	else {
		err = ltr303_als_disable();
		if (err)
			APS_ERR("disable als fail: %d\n", err);
		clear_bit(CMC_BIT_ALS, &ltr303_obj->enable);
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int als_set_delay(u64 ns)
{
	return 0;
}
/*----------------------------------------------------------------------------*/
static int als_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return als_set_delay(samplingPeriodNs);
}
/*----------------------------------------------------------------------------*/
static int als_flush(void)
{
	return als_flush_report();
}
/*----------------------------------------------------------------------------*/
static int als_get_data(int* value, int* status)
{
	int alsraw = 0;
	APS_FUN();
	if (!ltr303_obj) {
		APS_ERR("ltr303_i2c_client is null!!\n");
		return -1;
	}

	alsraw = ltr303_als_read(als_gainrange);
	*value = ltr303_get_als_value(ltr303_obj, alsraw);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	//APS_LOG("als_get_data *value = %d, *status =%d\n",*value, *status);

	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr303_als_register(void)
{
	int err = 0;
	struct als_control_path als_ctl = {0};
	struct als_data_path als_data = {0};

	als_ctl.open_report_data = als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay = als_set_delay;
	als_ctl.batch = als_batch;
	als_ctl.flush = als_flush;
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

	als_data.get_data = als_get_data;
	als_data.vender_div = 100;
	err = als_register_data_path(&als_data);	
	if (err) {
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

exit_sensor_obj_attach_fail:
	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr303_open(struct inode *inode, struct file *file)
{
	file->private_data = ltr303_i2c_client;

	if (!file->private_data) {
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
	switch (cmd) {
	case ALSPS_SET_ALS_MODE:
		if (copy_from_user(&enable, ptr, sizeof(enable))) {
			err = -EFAULT;
			goto err_out;
		}
		if (enable) {
			err = ltr303_als_enable(als_gainrange);
			if (err) {
				APS_ERR("enable als fail: %d\n", err);
				goto err_out;
			}
			set_bit(CMC_BIT_ALS, &obj->enable);
		}
		else {
			err = ltr303_als_disable();
			if (err) {
				APS_ERR("disable als fail: %d\n", err); 
				goto err_out;
			}
			clear_bit(CMC_BIT_ALS, &obj->enable);
		}
		break;

	case ALSPS_GET_ALS_MODE:
		enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
		if (copy_to_user(ptr, &enable, sizeof(enable))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_ALS_DATA: 
		obj->als = ltr303_als_read(als_gainrange);
		if (obj->als < 0) {
			goto err_out;
		}

		dat = ltr303_get_als_value(obj,obj->als);
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_ALS_RAW_DATA:    
		obj->als = ltr303_als_read(als_gainrange);
		if (obj->als < 0) {
			goto err_out;
		}

		dat = obj->als;
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
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
static int ltr303_devinit(void)
{
	int err;
	u8 chip_id;

	als_gainrange = ALS_RANGE_1300;

	mdelay(PON_DELAY);

	/* check whether is the LT's sensor. */
	chip_id = ltr303_i2c_read_reg(LTR303_PART_ID);
	if (chip_id == 0xA0) {
		APS_LOG("LTR-303ALS is detected.\n");
	}
	else if (chip_id == 0x80) {
		APS_LOG("LTR-558ALS is detected.\n");
	}
	else {
		APS_ERR("Detected LTR-303 chip_id %d error.\n",chip_id);
	}

	/*reset the sensor when first reading or writing.*/
	err = ltr303_i2c_write_reg(LTR303_ALS_CONTR, 0x00);
	if(err)
		return err;

	err = ltr303_i2c_write_reg(LTR303_ALS_MEAS_RATE, 0x02);
	if(err)
		APS_ERR("setting LTR303_ALS_MEAS_RATE error.\n");

	mdelay(WAKEUP_DELAY);

	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr303_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ltr303_priv *obj;
	int err = 0;

	APS_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}
	memset(obj,0,sizeof(*obj));
	ltr303_obj = obj;

	err = get_alsps_dts_func(client->dev.of_node, &obj->hw);
	if (err < 0) {
		APS_ERR("get dts info fail\n");
		goto exit_init_failed;
	}

	obj->client = client;
	i2c_set_clientdata(client,obj);

	atomic_set(&obj->als_debounce,300);
	atomic_set(&obj->als_deb_on,0);
	atomic_set(&obj->als_deb_end,0);
	atomic_set(&obj->als_suspend,0);

	obj->enable = 0;
	obj->als_level_num = sizeof(obj->hw.als_level)/sizeof(obj->hw.als_level[0]);
	obj->als_value_num = sizeof(obj->hw.als_value)/sizeof(obj->hw.als_value[0]);
	obj->als_modulus = (400*100)/(16*150);//(1/Gain)*(400/Tine), this value is fix after init ATIME and CONTROL register value
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw.als_level));
	memcpy(obj->als_level, obj->hw.als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw.als_value));
	memcpy(obj->als_value, obj->hw.als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);

	ltr303_i2c_client = client;
	APS_LOG("ltr303_devinit() start...!\n");
	err = ltr303_devinit();
	if (err)
		goto exit_init_failed;

	err = misc_register(&ltr303_device);
	if (err) {
		APS_ERR("ltr303_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	err = ltr303_create_attr(&ltr303_init_info.platform_diver_addr->driver);
	if (err) {
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	err = ltr303_als_register();
	if (err) {
		APS_ERR("register alsps err = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

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
	err = ltr303_delete_attr(&ltr303_i2c_driver.driver);
	if (err)
		APS_ERR("ltr303_delete_attr fail: %d\n", err);

	misc_deregister(&ltr303_device);

	ltr303_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef CONFIG_PM_SLEEP
static int ltr303_suspend(struct device *dev)
{
	struct ltr303_priv *obj = i2c_get_clientdata(ltr303_i2c_client);
	APS_FUN();

	if (!obj) {
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	atomic_set(&obj->als_suspend,1);
	ltr303_als_disable();
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr303_resume(struct device *dev)
{
	struct ltr303_priv * obj = i2c_get_clientdata(ltr303_i2c_client);
	APS_FUN();

	if (!obj) {
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	atomic_set(&obj->als_suspend,0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
		ltr303_als_enable(als_gainrange);

	return 0;
}
#endif
/*----------------------------------------------------------------------------*/
static int  ltr303_local_init(void)
{
	APS_FUN();
	if (i2c_add_driver(&ltr303_i2c_driver)) {
		APS_ERR("add driver error\n");
		return -1;
	}
	if (ltr303_init_flag) {
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr303_remove(void)
{
	APS_FUN();    
	i2c_del_driver(&ltr303_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int __init ltr303_init(void)
{
	APS_FUN();
	alsps_driver_add(&ltr303_init_info);
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

MODULE_DESCRIPTION("LTR-303ALS Driver");
MODULE_LICENSE("GPL");

