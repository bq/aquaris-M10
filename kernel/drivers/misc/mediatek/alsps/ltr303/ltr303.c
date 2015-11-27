/*
 * (c) MediaTek Inc. 2010
 */


#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/wakelock.h>

#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include "ltr303.h"
#include <linux/hwmsen_helper.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include <alsps.h>
#define POWER_NONE_MACRO -1

#ifdef CONFIG_GN_DEVICE_CHECK
#include <linux/gn_device_check.h>
#endif

/******************************************************************************
 * extern functions
*******************************************************************************/
extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
extern void mt_eint_print_status(void);

/******************************************************************************
 * configuration
*******************************************************************************/
#define I2C_DRIVERID_LTR303 303
/*----------------------------------------------------------------------------*/
#define LTR303_I2C_ADDR_RAR	0 /*!< the index in obj->hw->i2c_addr: alert response address */
#define LTR303_I2C_ADDR_ALS	1 /*!< the index in obj->hw->i2c_addr: ALS address */
#define LTR303_I2C_ADDR_PS	2 /*!< the index in obj->hw->i2c_addr: PS address */
#define LTR303_DEV_NAME		"LTR303"
/*----------------------------------------------------------------------------*/
#define APS_TAG	"[ALS/PS] "
#define APS_DEBUG
#if defined(APS_DEBUG)
#define APS_FUN(f)		printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)	printk(KERN_ERR APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)	printk(KERN_ERR APS_TAG "%s(%d):" fmt, __FUNCTION__, __LINE__, ##args)
#define APS_DBG(fmt, args...)	printk(KERN_ERR fmt, ##args)
#else
#define APS_FUN(f)
#define APS_ERR(fmt, args...)
#define APS_LOG(fmt, args...)
#define APS_DBG(fmt, args...)
#endif


//#define GN_MTK_BSP_ALSPS_INTERRUPT_MODE
//#define GN_MTK_BSP_PS_DYNAMIC_CALI
/******************************************************************************
 * extern functions
*******************************************************************************/
static struct i2c_client *ltr303_i2c_client = NULL;

//static struct wake_lock ps_wake_lock;
//static int ps_wakeup_timeout = 3;
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ltr303_i2c_id[] = {{LTR303_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_ltr303={ I2C_BOARD_INFO(LTR303_DEV_NAME, (LTR303_I2C_SLAVE_ADDR>>1))};

/*----------------------------------------------------------------------------*/
static int ltr303_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int ltr303_i2c_remove(struct i2c_client *client);
static int ltr303_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int ltr303_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int ltr303_i2c_resume(struct i2c_client *client);

static int ltr303_remove(void);
static int ltr303_local_init(void);

static int ltr303_init_flag =-1; // 0<==>OK -1 <==> fail
static struct alsps_init_info ltr303_init_info = {
		.name = "ltr303",
		.init = ltr303_local_init,
		.uninit = ltr303_remove,
};

static int is_dynamic_calibrate = 0;
static u8 part_id = 0;
static u8 als_range = 0;
static u8 ps_gain = 0;

static struct ltr303_priv *g_ltr303_ptr = NULL;

/*----------------------------------------------------------------------------*/
typedef enum {
	CMC_TRC_APS_DATA	= 0x0002,
	CMC_TRC_EINT		= 0x0004,
	CMC_TRC_IOCTL		= 0x0008,
	CMC_TRC_I2C		= 0x0010,
	CMC_TRC_CVT_ALS		= 0x0020,
	CMC_TRC_CVT_PS		= 0x0040,
	CMC_TRC_DEBUG		= 0x8000,
} CMC_TRC;
/*----------------------------------------------------------------------------*/
typedef enum {
	CMC_BIT_ALS		= 1,
	CMC_BIT_PS		= 2,
} CMC_BIT;
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
struct ltr303_priv {
	struct alsps_hw *hw;
	struct i2c_client *client;
#if 0//def GN_MTK_BSP_ALSPS_INTERRUPT_MODE
	struct delayed_work eint_work;
#endif /* GN_MTK_BSP_ALSPS_INTERRUPT_MODE */
	//struct timer_list first_read_ps_timer;
	//struct timer_list first_read_als_timer;

	/*misc*/
	atomic_t	trace;
	atomic_t	i2c_retry;
	atomic_t	als_suspend;
	atomic_t	als_debounce;	/*debounce time after enabling als*/
	atomic_t	als_deb_on;	/*indicates if the debounce is on*/
	atomic_t	als_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_mask;	/*mask ps: always return far away*/
	atomic_t	ps_debounce;	/*debounce time after enabling ps*/
	atomic_t	ps_deb_on;	/*indicates if the debounce is on*/
	atomic_t	ps_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_suspend;


	/*data*/
	// u8		als;
	// u8		ps;
	int		als;
	int		ps;
	u8		_align;
	u16		als_level_num;
	u16		als_value_num;
	u32		als_level[C_CUST_ALS_LEVEL-1];
	u32		als_value[C_CUST_ALS_LEVEL];

	bool		als_enable;	/*record current als status*/
	unsigned int	als_widow_loss;

	bool		ps_enable;	 /*record current ps status*/
	unsigned int	ps_thd_val;	 /*the cmd value can't be read, stored in ram*/
	ulong		enable;		 /*record HAL enalbe status*/
	ulong		pending_intr;	/*pending interrupt*/
	//ulong		first_read;	// record first read ps and als
	unsigned int	polling;
	/*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend	early_drv;
#endif
};
static struct platform_driver ltr303_alsps_driver;

/*----------------------------------------------------------------------------*/
static struct i2c_driver ltr303_i2c_driver = {
	.probe		= ltr303_i2c_probe,
	.remove		= ltr303_i2c_remove,
	.detect		= ltr303_i2c_detect,
	.suspend	= ltr303_i2c_suspend,
	.resume		= ltr303_i2c_resume,
	.id_table	= ltr303_i2c_id,
//	.address_data	= &ltr303_addr_data,
	.driver = {
//		.owner	= THIS_MODULE,
		.name	= LTR303_DEV_NAME,
	},
};

static struct ltr303_priv *ltr303_obj = NULL;

//static int ps_gainrange;
//static int als_gainrange;

static int final_prox_val;
static int final_lux_val;

/*
 * The ps_trigger_xxx_table
 * controls the interrupt trigger range
 * bigger value means close to p-sensor
 * smaller value means far away from p-sensor
 */
static int ps_trigger_high = 800;				//yaoyaoqin: trigger up threshold
static int ps_trigger_low = 760;				//yaoyaoqin: trigger low threshold
static int ps_trigger_delta = 0x0f;				//yaoyaoqin: delta for adjust threshold

static int ltr303_get_ps_value(struct ltr303_priv *obj, int ps);
static int ltr303_get_als_value(struct ltr303_priv *obj, int als);

#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
static int ltr303_dynamic_calibrate(void);
#endif

/*----------------------------------------------------------------------------*/
static int hwmsen_read_byte_sr(struct i2c_client *client, u8 addr, u8 *data)
{
	u8 buf;
	int ret = 0;
	struct i2c_client client_read = *client;

	client_read.addr = (client->addr & I2C_MASK_FLAG) | I2C_WR_FLAG |I2C_RS_FLAG;
	buf = addr;
	ret = i2c_master_send(&client_read, (const char*)&buf, 1<<8 | 1);
	if (ret < 0) {
		APS_ERR("send command error!!\n");
		return -EFAULT;
	}

	*data = buf;
	client->addr = client->addr& I2C_MASK_FLAG;
	return 0;
}

/*----------------------------------------------------------------------------*/
int ltr303_read_data_als(struct i2c_client *client, int *data)
{
	struct ltr303_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	int alsval_ch1_lo = 0;
	int alsval_ch1_hi = 0;
	int alsval_ch0_lo = 0;
	int alsval_ch0_hi = 0;
	int luxdata_int;
	int luxdata_flt;
	int ratio;
	int alsval_ch0;
	int alsval_ch1;

	int als_zero_try = 0;

als_data_try:
	if (hwmsen_read_byte_sr(client, APS_RO_ALS_DATA_CH1_0, &alsval_ch1_lo)) {
		APS_ERR("reads als data error (ch1 lo) = %d\n", alsval_ch1_lo);
		return -EFAULT;
	}
	if (hwmsen_read_byte_sr(client, APS_RO_ALS_DATA_CH1_1, &alsval_ch1_hi)) {
		APS_ERR("reads aps data error (ch1 hi) = %d\n", alsval_ch1_hi);
		return -EFAULT;
	}
	alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;
	//APS_ERR("alsval_ch1_hi=%x alsval_ch1_lo=%x\n",alsval_ch1_hi,alsval_ch1_lo);


	if (hwmsen_read_byte_sr(client, APS_RO_ALS_DATA_CH0_0, &alsval_ch0_lo)) {
		APS_ERR("reads als data error (ch0 lo) = %d\n", alsval_ch0_lo);
		return -EFAULT;
	}
	if (hwmsen_read_byte_sr(client, APS_RO_ALS_DATA_CH0_1, &alsval_ch0_hi)) {
		APS_ERR("reads als data error (ch0 hi) = %d\n", alsval_ch0_hi);
		return -EFAULT;
	}
	alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;

	if ((alsval_ch0 + alsval_ch1) == 0) {
		APS_ERR("Both CH0 and CH1 are zero\n");
		ratio = 1000;
	} else {
		ratio = (alsval_ch1 * 1000) / (alsval_ch0 + alsval_ch1);
	}

	if (ratio < 450){
		luxdata_flt = (17743 * alsval_ch0) - (-11059 * alsval_ch1);
		luxdata_flt = luxdata_flt / 10000;
	} else if ((ratio >= 450) && (ratio < 640)){
		luxdata_flt = (42785 * alsval_ch0) - (19548 * alsval_ch1);
		luxdata_flt = luxdata_flt / 10000;
	} else if ((ratio >= 640) && (ratio < 840)){
		luxdata_flt = (5926 * alsval_ch0) - (-1185 * alsval_ch1);
		luxdata_flt = luxdata_flt / 10000;
	} else {
		luxdata_flt = 0;
		als_zero_try++;
		if (als_zero_try < 3 ) {
			APS_ERR("als_zero_try:%d\n", als_zero_try);
			mdelay(200);
			goto als_data_try;
		} else {
			APS_ERR("als_zero_try:device bug:%d!!!!!\n", als_zero_try);
		}		
	}

/*
	// For Range1
	if (als_gainrange == ALS_RANGE1_320)
		luxdata_flt = luxdata_flt / 150;
*/
	// convert float to integer;
	luxdata_int = luxdata_flt;
	if ((luxdata_flt - luxdata_int) > 0.5){
		luxdata_int = luxdata_int + 1;
	} else {
		luxdata_int = luxdata_flt;
	}

	if (atomic_read(&obj->trace) & CMC_TRC_APS_DATA) {
		APS_ERR("ALS (CH0): 0x%04X\n", alsval_ch0);
		APS_ERR("ALS (CH1): 0x%04X\n", alsval_ch1);
		APS_ERR("ALS (Ratio): %d\n", ratio);
		APS_ERR("ALS: %d\n", luxdata_int);
	}

	*data = luxdata_int;
	
	//APS_ERR("luxdata_int=%x \n",luxdata_int);
	final_lux_val = luxdata_int;

	return 0;
}

int ltr303_read_data_ps(struct i2c_client *client, int *data)
{
	struct ltr303_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	int psval_lo = 0;
	int psval_hi = 0;
	int psdata = 0;

	if (hwmsen_read_byte_sr(client, APS_RO_PS_DATA_0, &psval_lo)) {
		printk("reads aps data = %d\n", psval_lo);
		return -EFAULT;
	}

	if (hwmsen_read_byte_sr(client, APS_RO_PS_DATA_1, &psval_hi)) {
		printk("reads aps hi data = %d\n", psval_hi);
		return -EFAULT;
	}

	psdata = ((psval_hi & 7) * 256) + psval_lo;
	printk("psensor rawdata is:%d\n", psdata);

	*data = psdata;
	final_prox_val = psdata;
	return 0;
}

/*----------------------------------------------------------------------------*/

int ltr303_init_device(struct i2c_client *client)
{
	//struct ltr303_priv *obj = i2c_get_clientdata(client);
	APS_LOG("ltr303_init_device.........\r\n");
	u8 buf =0;
	int i = 0;
	int ret = 0;
		
	//hwmsen_write_byte(client, 0x82, 0x7F);				//100mA,100%,60kHz		//yaoyaoqin
	//hwmsen_write_byte(client, 0x83, 0x05);				///5 pulse	
	hwmsen_write_byte(client, 0x85, 0x01);				//als measurement time 100 ms
	//hwmsen_write_byte(client, 0x9e, 0x20);				///2 consecutive data outside range to interrupt
	if(part_id == LTR303_PART_ID){
	//	hwmsen_write_byte(client, 0x84, 0x0f);			//ps measurement time 10 ms
		als_range = LTR303_ALS_ON_Range8;
		//ps_gain = LTR303_PS_ON_Gain16;
	}else if(part_id == LTR558_PART_ID){
		//hwmsen_write_byte(client, 0x84, 0x0);			//ps measurement time 50 ms
		als_range = LTR558_ALS_ON_Range2;
		//ps_gain = LTR558_PS_ON_Gain8;
	}else{
		APS_ERR("LTR303 part_id is error, part_id = 0x%x\n", part_id);
	}

#if 0
#ifdef GN_MTK_BSP_ALSPS_INTERRUPT_MODE
	hwmsen_write_byte(client, 0x8f, 0x01);				//enable ps for interrupt mode
	hwmsen_write_byte(client, 0x90, ps_trigger_high & 0xff);
	hwmsen_write_byte(client, 0x91, (ps_trigger_high>>8) & 0X07);

	hwmsen_write_byte(client, 0x92, 0x00);
	hwmsen_write_byte(client, 0x93, 0x00);
#endif  // GN_MTK_BSP_ALSPS_INTERRUPT_MODE 
#endif
	
	mdelay(WAKEUP_DELAY);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_power(struct alsps_hw *hw, unsigned int on)
{
	static unsigned int power_on = 0;
	int status = 0; 

	APS_LOG("power %s\n", on ? "on" : "off");
	APS_LOG("power id:%d POWER_NONE_MACRO:%d\n", hw->power_id, POWER_NONE_MACRO);

	if(hw->power_id != POWER_NONE_MACRO)
	{
		if(power_on == on)
		{
			APS_LOG("ignore power control: %d\n", on);
			status = 0;
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "LTR303"))
			{
				APS_ERR("power on fails!!\n");
			}
			status =  1;
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "LTR303"))
			{
				APS_ERR("power off fail!!\n");
			}
			status =  -1;
		}
	}
	power_on = on;

	return status;
}
/*----------------------------------------------------------------------------*/
static int ltr303_enable_als(struct i2c_client *client, bool enable)
{
	struct ltr303_priv *obj =i2c_get_clientdata(client);
	int err=0;
	int trc = atomic_read(&obj->trace);
	u8 regdata=0;
	u8 regint=0;

	if(enable == obj->als_enable)
	{
		return 0;
	}
	else if(enable == true)
	{
		if (hwmsen_write_byte(client, APS_RW_ALS_CONTR, als_range)) {
			APS_LOG("ltr303_enable_als enable failed!\n");
			return -1;
		}
		hwmsen_read_byte_sr(client, APS_RW_ALS_CONTR, &regdata);			//yaoyaoqin: need delay at least 50ms
		//mdelay(200);
		APS_LOG("ltr303_enable_als, regdata: 0x%x!\n", regdata);
	}
	else if(enable == false)
	{
		if (hwmsen_write_byte(client, APS_RW_ALS_CONTR, MODE_ALS_StdBy)) {
			APS_LOG("ltr303_enable_als disable failed!\n");
			return -1;
		}
		hwmsen_read_byte_sr(client, APS_RW_ALS_CONTR, &regdata);
		APS_LOG("ltr303_enable_als, regdata: 0x%x!\n", regdata);
	}

	obj->als_enable = enable;

	if(trc & CMC_TRC_DEBUG)
	{
		APS_LOG("enable als (%d)\n", enable);
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr303_enable_ps(struct i2c_client *client, bool enable)
{
	return 0;

	struct ltr303_priv *obj = i2c_get_clientdata(client);
	int err=0;
	int trc = atomic_read(&obj->trace);
	u8 regdata = 0;
	u8 regint = 0;
	hwm_sensor_data sensor_data;

	APS_LOG(" ltr303_enable_ps: enable:  %d, obj->ps_enable: %d\n",enable, obj->ps_enable);
	if (enable == obj->ps_enable)
	{
		return 0;
	}
	else if (enable == true)
	{
		if (hwmsen_write_byte(client, APS_RW_PS_CONTR, ps_gain)) {
			APS_LOG("ltr303_enable_ps enable failed!\n");
			return -1;
		}
		hwmsen_read_byte_sr(client, APS_RW_PS_CONTR, &regdata);
		APS_ERR("ltr303_enable_ps, regdata: %0xx!\n", regdata);

#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
		if((regdata == ps_gain) && (is_dynamic_calibrate == 0))
		{
			mt_eint_mask(CUST_EINT_ALS_NUM);
		
			ltr303_dynamic_calibrate();

			mt_eint_unmask(CUST_EINT_ALS_NUM);
		}
#endif

		sensor_data.values[0] = 1;
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		if (err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)) {
			APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
			return err;
		}	
		
	}
	else if(enable == false)
	{
		if (hwmsen_write_byte(client, APS_RW_PS_CONTR, MODE_PS_StdBy)) {
			APS_LOG("ltr303_enable_ps disable failed!\n");
			return -1;
		}
		hwmsen_read_byte_sr(client, APS_RW_PS_CONTR, &regdata);
		APS_ERR("ltr303_enable_ps, regdata: %0xx!\n", regdata);
	}
	obj->ps_enable = enable;
	if(trc & CMC_TRC_DEBUG)
	{
		APS_LOG("enable ps (%d)\n", enable);
	}

	return err;
}
/*----------------------------------------------------------------------------*/

static int ltr303_check_intr(struct i2c_client *client)
{
	struct ltr303_priv *obj = i2c_get_clientdata(client);
	int err;
	u8 data=0;

	// err = hwmsen_read_byte_sr(client,APS_INT_STATUS,&data);
	err = hwmsen_read_byte_sr(client,APS_RO_ALS_PS_STATUS,&data);
	APS_ERR("INT flage: = %x\n", data);

	if (err) {
		APS_ERR("WARNING: read int status: %d\n", err);
		return 0;
	}

	if (data & 0x08) {
		set_bit(CMC_BIT_ALS, &obj->pending_intr);
	} else {
		clear_bit(CMC_BIT_ALS, &obj->pending_intr);
	}

	if (data & 0x02) {
		set_bit(CMC_BIT_PS, &obj->pending_intr);
	} else {
		clear_bit(CMC_BIT_PS, &obj->pending_intr);
	}

	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG) {
		//APS_LOG("check intr: 0x%08X\n", obj->pending_intr);
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
#if 0//def GN_MTK_BSP_ALSPS_INTERRUPT_MODE
void ltr303_eint_func(void)
{
	struct ltr303_priv *obj = g_ltr303_ptr;
	APS_LOG("fwq interrupt fuc\n");
	if(!obj)
	{
		return;
	}

	schedule_delayed_work(&obj->eint_work,0);
	if(atomic_read(&obj->trace) & CMC_TRC_EINT)
	{
		APS_LOG("eint: als/ps intrs\n");
	}
}
/*----------------------------------------------------------------------------*/
static void ltr303_eint_work(struct work_struct *work)	
{
	struct ltr303_priv *obj = (struct ltr303_priv *)container_of(work, struct ltr303_priv, eint_work);
	int err;
	hwm_sensor_data sensor_data;
	u8 buf;

	APS_ERR("interrupt........");
	memset(&sensor_data, 0, sizeof(sensor_data));

	if (0 == atomic_read(&obj->ps_deb_on)) {
		// first enable do not check interrupt
		err = ltr303_check_intr(obj->client);
	}

	if (err) {
		APS_ERR("check intrs: %d\n", err);
	}

	APS_ERR("ltr303_eint_work obj->pending_intr =%d, obj:%p\n",obj->pending_intr,obj);

	if ((1<<CMC_BIT_ALS) & obj->pending_intr) {
		// get raw data
		APS_ERR("fwq als INT\n");
		if (err = ltr303_read_data_als(obj->client, &obj->als)) {
			APS_ERR("ltr303 read als data: %d\n", err);;
		}
		//map and store data to hwm_sensor_data
		while(-1 == ltr303_get_als_value(obj, obj->als)){
			ltr303_read_data_als(obj->client, &obj->als);
			msleep(50);
		}
 		sensor_data.values[0] = ltr303_get_als_value(obj, obj->als);
		APS_LOG("ltr303_eint_work sensor_data.values[0] =%d\n",sensor_data.values[0]);
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		// let up layer to know
		if (err = hwmsen_get_interrupt_data(ID_LIGHT, &sensor_data)) {
			APS_ERR("call hwmsen_get_interrupt_data light fail = %d\n", err);
		}
	}
	
	if ((1 << CMC_BIT_PS) & obj->pending_intr) {
		// get raw data
		APS_ERR("fwq ps INT\n");
		if (err = ltr303_read_data_ps(obj->client, &obj->ps)) {
			APS_ERR("ltr303 read ps data: %d\n", err);;
		}
		//map and store data to hwm_sensor_data
		while(-1 == ltr303_get_ps_value(obj, obj->ps)) {
			ltr303_read_data_ps(obj->client, &obj->ps);
			msleep(50);
			APS_ERR("ltr303 read ps data delay\n");;
		}
		sensor_data.values[0] = ltr303_get_ps_value(obj, obj->ps);
		
		if(sensor_data.values[0] == 0)
		{
			hwmsen_write_byte(obj->client,0x90,0xff);
			hwmsen_write_byte(obj->client,0x91,0x07);
			
			hwmsen_write_byte(obj->client,0x92,ps_trigger_low & 0xff);				//yaoyaoqin:low threshold for faraway interrupt only
			hwmsen_write_byte(obj->client,0x93,(ps_trigger_low>>8) & 0X07);		
			//wake_unlock(&ps_wake_lock);
		}
		else if(sensor_data.values[0] == 1)
		{
			//wake_lock_timeout(&ps_wake_lock,ps_wakeup_timeout*HZ);
			hwmsen_write_byte(obj->client,0x90,ps_trigger_high & 0xff);				//yaoyaoqin:high threshold for close interrupt only
			hwmsen_write_byte(obj->client,0x91,(ps_trigger_high>>8) & 0X07);
			
			hwmsen_write_byte(obj->client,0x92,0x00);
			hwmsen_write_byte(obj->client,0x93,0x00);	
		}

		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		//let up layer to know
		APS_ERR("ltr303 read ps data = %d \n",sensor_data.values[0]);
		if(err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)) {
			APS_ERR("call hwmsen_get_interrupt_data proximity fail = %d\n", err);
		}
	}

	mt_eint_unmask(CUST_EINT_ALS_NUM);
}

int ltr303_setup_eint(struct i2c_client *client)
{
	struct ltr303_priv *obj = i2c_get_clientdata(client);

	APS_FUN();
	g_ltr303_ptr = obj;

	mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
	mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, TRUE);
	mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);

	mt_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_TYPE, ltr303_eint_func, 0);
	mt_eint_unmask(CUST_EINT_ALS_NUM);  

	return 0;
}
#endif /* GN_MTK_BSP_ALSPS_INTERRUPT_MODE */
/*----------------------------------------------------------------------------*/
static int ltr303_init_client(struct i2c_client *client)
{
	struct ltr303_priv *obj = i2c_get_clientdata(client);
	int err=0;
	APS_LOG("ltr303_init_client.........\r\n");

#if 0//ndef CUSTOM_KERNEL_ALSPS_NO_PROXIMITY 	
#ifdef GN_MTK_BSP_ALSPS_INTERRUPT_MODE
	if((err = ltr303_setup_eint(client)))
	{
		APS_ERR("setup eint: %d\n", err);
		return err;
	}
#endif
#endif
	if((err = ltr303_init_device(client)))
	{
		APS_ERR("init dev: %d\n", err);
		return err;
	}
	return err;
}
/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t ltr303_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;

	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n",
		atomic_read(&ltr303_obj->i2c_retry), atomic_read(&ltr303_obj->als_debounce),
		atomic_read(&ltr303_obj->ps_mask), ltr303_obj->ps_thd_val, atomic_read(&ltr303_obj->ps_debounce));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_store_config(struct device_driver *ddri, char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}

	if(5 == sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb))
	{
		atomic_set(&ltr303_obj->i2c_retry, retry);
		atomic_set(&ltr303_obj->als_debounce, als_deb);
		atomic_set(&ltr303_obj->ps_mask, mask);
		ltr303_obj->ps_thd_val= thres;
		atomic_set(&ltr303_obj->ps_debounce, ps_deb);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, (int)count);
	}
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&ltr303_obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_store_trace(struct device_driver *ddri, char *buf, size_t count)
{
	int trace;
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}

	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&ltr303_obj->trace, trace);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, (int)count);
	}
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	u8 dat = 0;

	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}
	// if(res = ltr303_read_data(ltr303_obj->client, &ltr303_obj->als))
	if(res = ltr303_read_data_als(ltr303_obj->client, &ltr303_obj->als))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
#if 0
	else
	{
		// dat = ltr303_obj->als & 0x3f;
		dat = ltr303_obj->als;
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", dat);
	}
#endif
	while(-1 == ltr303_get_als_value(ltr303_obj,ltr303_obj->als))
	{
		ltr303_read_data_als(ltr303_obj->client,&ltr303_obj->als);
		msleep(50);
	}
	dat = ltr303_get_als_value(ltr303_obj,ltr303_obj->als);
	
	return snprintf(buf, PAGE_SIZE, "0x%04X\n", dat);
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_show_ps(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	u8 dat=0;
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}

	// if(res = ltr303_read_data(ltr303_obj->client, &ltr303_obj->ps))
	if(res = ltr303_read_data_ps(ltr303_obj->client, &ltr303_obj->ps))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", (int)res);
	}
	else
	{
		// dat = ltr303_obj->ps & 0x80;
		dat = ltr303_get_ps_value(ltr303_obj, ltr303_obj->ps);
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", dat);
	}
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

	#ifdef MT6516
	len += snprintf(buf+len, PAGE_SIZE-len, "EINT: %d (%d %d %d %d)\n", mt_get_gpio_in(GPIO_ALS_EINT_PIN),
				CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_DEBOUNCE_CN);

	len += snprintf(buf+len, PAGE_SIZE-len, "GPIO: %d (%d %d %d %d)\n",	GPIO_ALS_EINT_PIN,
				mt_get_gpio_dir(GPIO_ALS_EINT_PIN), mt_get_gpio_mode(GPIO_ALS_EINT_PIN),
				mt_get_gpio_pull_enable(GPIO_ALS_EINT_PIN), mt_get_gpio_pull_select(GPIO_ALS_EINT_PIN));
	#endif

	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&ltr303_obj->als_suspend), atomic_read(&ltr303_obj->ps_suspend));

	return len;
}

#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct ltr303_priv *obj, const char* buf, size_t count,
							 u32 data[], int len)
{
	int idx = 0;
	char *cur = (char*)buf, *end = (char*)(buf+count);

	while(idx < len)
	{
		while((cur < end) && IS_SPACE(*cur))
		{
			cur++;
		}

		if(1 != sscanf(cur, "%d", &data[idx]))
		{
			break;
		}

		idx++;
		while((cur < end) && !IS_SPACE(*cur))
		{
			cur++;
		}
	}
	return idx;
}
/*----------------------------------------------------------------------------*/


static ssize_t ltr303_show_reg(struct device_driver *ddri, char *buf)
{
	int i = 0;
	u8 bufdata;
	int count  = 0;
	
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}

	for(i = 0;i < 31 ;i++)
	{
		hwmsen_read_byte_sr(ltr303_obj->client,0x80+i,&bufdata);
		count+= sprintf(buf+count,"[%x] = (%x)\n",0x80+i,bufdata);
	}

	return count;
}

static ssize_t ltr303_store_reg(struct device_driver *ddri,char *buf,ssize_t count)
{

	u32 data[2];
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null\n");
		return 0;
	}
	/*else if(2 != sscanf(buf,"%d %d",&addr,&data))*/
	else if(2 != read_int_from_buf(ltr303_obj,buf,count,data,2))
	{
		APS_ERR("invalid format:%s\n",buf);
		return 0;
	}

	hwmsen_write_byte(ltr303_obj->client,data[0],data[1]);

	return count;
}
/*----------------------------------------------------------------------------*/


static ssize_t ltr303_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}

	for(idx = 0; idx < ltr303_obj->als_level_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", ltr303_obj->hw->als_level[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_store_alslv(struct device_driver *ddri, char *buf, size_t count)
{
	struct ltr303_priv *obj;
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(ltr303_obj->als_level, ltr303_obj->hw->als_level, sizeof(ltr303_obj->als_level));
	}
	else if(ltr303_obj->als_level_num != read_int_from_buf(ltr303_obj, buf, count,
			ltr303_obj->hw->als_level, ltr303_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}

	for(idx = 0; idx < ltr303_obj->als_value_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", ltr303_obj->hw->als_value[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr303_store_alsval(struct device_driver *ddri, char *buf, size_t count)
{
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(ltr303_obj->als_value, ltr303_obj->hw->als_value, sizeof(ltr303_obj->als_value));
	}
	else if(ltr303_obj->als_value_num != read_int_from_buf(ltr303_obj, buf, count,
			ltr303_obj->hw->als_value, ltr303_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}
	return count;
}

static ssize_t ltr303_show_enable_als(struct device_driver *ddrv,char *buf)
{
	ssize_t len =  0;
	int idx;
	if(!ltr303_obj)
	{
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}

	if(true == ltr303_obj->als_enable)
	{
		len = sprintf(buf,"%d\n",1);
	}
	else
	{
		len = sprintf(buf,"%d\n",0);
	}
	return len;

}
static  ssize_t ltr303_store_enable_als(struct device_driver *ddrv,char *buf, size_t count)
{
	int enable;
	if(!ltr303_obj)
	{	
		APS_ERR("ltr303_obj is null!!\n");
		return 0;
	}
	if(1 == sscanf(buf,"%d",&enable))
	{
		if(enable)
		{
			ltr303_enable_als(ltr303_obj->client,true);
		}
		else
		{
			ltr303_enable_als(ltr303_obj->client,false);
		}
	}
	else
	{
		APS_ERR("enable als fail\n");
	}
	return count;
}

#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
static ssize_t ltr303_dynamic_calibrate(void)			
{																				
	int ret=0;
	int i=0;
	int data;
	int data_total=0;
	ssize_t len = 0;
	int noise = 0;
	int count = 10;
	int max = 0;

	if(!ltr303_obj)
	{	
		APS_ERR("ltr303_obj is null!!\n");
		len = sprintf(buf, "ltr303_obj is null\n");
		return -1;
	}

	// wait for register to be stable
	if(part_id == LTR303_PART_ID){
		msleep(30);
	}else{
		msleep(100);
	}

	for (i = 0; i < count; i++) {
		// wait for ps value be stable
		if(part_id == LTR303_PART_ID){
			msleep(15);
		}else{
			msleep(55);
		}		

		ret=ltr303_read_data_ps(ltr303_obj->client,&data);
		if (ret < 0) {
			i--;
			continue;
		}
		
		data_total+=data;

		if (max++ > 100) {
			len = sprintf(buf,"adjust fail\n");
			return len;
		}
	}
	
	noise=data_total/count;
	if(noise < 50){
		ps_trigger_high = noise + 45;
		ps_trigger_low = noise + 35;	
		is_dynamic_calibrate = 1;
	}else if(noise < 100){
		ps_trigger_high = noise + 60;
		ps_trigger_low = noise + 50;
		is_dynamic_calibrate = 1;
	}else if(noise < 200){
		ps_trigger_high = noise + 80;
		ps_trigger_low = noise + 70;
		is_dynamic_calibrate = 1;
	}else if(noise < 300){
		ps_trigger_high = noise + 100;
		ps_trigger_low = noise + 80;
		is_dynamic_calibrate = 1;
	}else if(noise < 400){
		ps_trigger_high = noise + 120;
		ps_trigger_low = noise + 100;
		is_dynamic_calibrate = 1;
	}else if(noise < 500){
		ps_trigger_high = noise + 140;
		ps_trigger_low = noise + 120;
		is_dynamic_calibrate = 1;
	}else if(noise < 600){
		ps_trigger_high = noise + 160;
		ps_trigger_low = noise + 140;
		is_dynamic_calibrate = 1;
	}else{
		ps_trigger_high = 800;
		ps_trigger_low = 780;
		is_dynamic_calibrate = 0;
	}

	hwmsen_write_byte(ltr303_obj->client, 0x90, ps_trigger_high & 0XFF);
	hwmsen_write_byte(ltr303_obj->client, 0x91, (ps_trigger_high>>8) & 0X07);
	hwmsen_write_byte(ltr303_obj->client, 0x92, 0x00);
	hwmsen_write_byte(ltr303_obj->client, 0x93, 0x00);

	len = sprintf(buf,"ps_trigger_high: %d, ps_trigger_low: %d\n",
		ps_trigger_high,ps_trigger_low);
	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(als,	 S_IWUSR | S_IRUGO, ltr303_show_als,	NULL);
static DRIVER_ATTR(ps,	 S_IWUSR | S_IRUGO, ltr303_show_ps,	NULL);
static DRIVER_ATTR(alslv, S_IWUSR | S_IRUGO, ltr303_show_alslv, ltr303_store_alslv);
static DRIVER_ATTR(alsval, S_IWUSR | S_IRUGO, ltr303_show_alsval,ltr303_store_alsval);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, ltr303_show_trace, ltr303_store_trace);
static DRIVER_ATTR(status, S_IWUSR | S_IRUGO, ltr303_show_status, NULL);
static DRIVER_ATTR(reg,	 S_IWUSR | S_IRUGO, ltr303_show_reg, ltr303_store_reg);
static DRIVER_ATTR(enable_als,	 S_IRUGO | S_IRUGO, ltr303_show_enable_als, ltr303_store_enable_als);
//static DRIVER_ATTR(adjust, S_IWUSR | S_IRUGO, ltr303_ps_adjust, NULL);
/*----------------------------------------------------------------------------*/
static struct device_attribute *ltr303_attr_list[] = {
	&driver_attr_als,
	&driver_attr_ps,
	&driver_attr_trace,		/*trace log*/
	&driver_attr_alslv,
	&driver_attr_alsval,
	&driver_attr_status,
	&driver_attr_reg,
	&driver_attr_enable_als
	//&driver_attr_adjust
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
		if(err = driver_create_file(driver, ltr303_attr_list[idx]))
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
/******************************************************************************
 * Function Configuration
******************************************************************************/
// static int ltr303_get_als_value(struct ltr303_priv *obj, u8 als)
static int ltr303_get_als_value(struct ltr303_priv *obj, int als)
{
	int idx;
	int invalid = 0;
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


		#if defined(CONFIG_MTK_AAL_SUPPORT)
       	int level_high = obj->hw->als_level[idx];
    	      	int level_low = (idx > 0) ? obj->hw->als_level[idx-1] : 0;
             	int level_diff = level_high - level_low;
	      	int value_high = obj->hw->als_value[idx];
       	 int value_low = (idx > 0) ? obj->hw->als_value[idx-1] : 0;
        	int value_diff = value_high - value_low;
       	 int value = 0;
        
        if ((level_low >= level_high) || (value_low >= value_high))
            value = value_low;
        else
            value = (level_diff * value_low + (als - level_low) * value_diff + ((level_diff + 1) >> 1)) / level_diff;

		APS_DBG("ALS: %d [%d, %d] => %d [%d, %d] \n", als, level_low, level_high, value, value_low, value_high);
		return value;
		#endif
		
		if (atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
		}

		return obj->hw->als_value[idx];
	}
	else
	{
		if(atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);
		}
		return -1;
	}
}
/*----------------------------------------------------------------------------*/

static int ltr303_get_ps_value(struct ltr303_priv *obj, int ps)
{
	int val= -1;
	int invalid = 0;
	

	if (ps > ps_trigger_high) {
		// bigger value, close
		val = 0;
	} else if (ps < ps_trigger_low) {
		// smaller value, far away
		val = 1;
	} else {
		val = 1;
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

	if(!invalid)
	{	
		if(unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
		{
			APS_DBG("PS: %05d => %05d\n", ps, val);
		}
		return val;

	}
	else
	{
		if(unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
		{
			APS_DBG("PS: %05d => %05d (-1)\n", ps, val);
		}
		return -1;
	}

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
static long ltr303_unlocked_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct ltr303_priv *obj = i2c_get_clientdata(client);
	int err = 0;
	void __user *ptr = (void __user*) arg;
	int dat;
	uint32_t enable;

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
				if(err = ltr303_enable_ps(obj->client, true))
				{
					APS_ERR("enable ps fail: %d\n", err);
					goto err_out;
				}
				set_bit(CMC_BIT_PS, &obj->enable);
			}
			else
			{
				if(err = ltr303_enable_ps(obj->client, false))
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
			if(err = ltr303_read_data_ps(obj->client, &obj->ps))
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
			if(err = ltr303_read_data_ps(obj->client, &obj->ps))
			{
				goto err_out;
			}

			dat = obj->ps & 0x80;
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
				if(err = ltr303_enable_als(obj->client, true))
				{
					APS_ERR("enable als fail: %d\n", err);
					goto err_out;
				}
				set_bit(CMC_BIT_ALS, &obj->enable);
			}
			else
			{
				if(err = ltr303_enable_als(obj->client, false))
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
			if(err = ltr303_read_data_als(obj->client, &obj->als))
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

		case ALSPS_GET_ALS_RAW_DATA:
			if(err = ltr303_read_data_als(obj->client, &obj->als))
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
//	.owner = THIS_MODULE,
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
		if(err = ltr303_enable_als(client, false))
		{
			APS_ERR("disable als: %d\n", err);
			return err;
		}
		if(!obj->ps_enable)
			ltr303_power(obj->hw, 0);
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr303_i2c_resume(struct i2c_client *client)
{
	struct ltr303_priv *obj = i2c_get_clientdata(client);
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	if(1 == ltr303_power(obj->hw, 1))
	{
		if(err = ltr303_init_device(client))
		{
			APS_ERR("initialize client fail!!\n");
			return err;
		}
		if(obj->ps_enable)
		{
			if(err = ltr303_enable_ps(client,true))
			{
				APS_ERR("enable ps fail: %d\n",err);
			}
			return err;
		}
	}
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))														//yaoyaoqin
	{
		if(err = ltr303_enable_als(client, true))
		{
			APS_ERR("enable als fail: %d\n", err);
		}
	}
	else
	{
		if(err = ltr303_enable_als(client, false))
		{
			APS_ERR("enable als fail: %d\n", err);
		}
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static void ltr303_early_suspend(struct early_suspend *h)
{
	/*early_suspend is only applied for ALS*/
	struct ltr303_priv *obj = container_of(h, struct ltr303_priv, early_drv);
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->als_suspend, 1);
	if(err = ltr303_enable_als(obj->client, false))
	{
		APS_ERR("disable als fail: %d\n", err);
	}
}
/*----------------------------------------------------------------------------*/
static void ltr303_late_resume(struct early_suspend *h)
{
	/*early_suspend is only applied for ALS*/
	struct ltr303_priv *obj = container_of(h, struct ltr303_priv, early_drv);
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
		if(err = ltr303_enable_als(obj->client, true))
		{
			APS_ERR("enable als fail: %d\n", err);
		}
	}
	else
	{
		if(err = ltr303_enable_als(obj->client, false))
		{
			APS_ERR("enable als fail: %d\n", err);
		}
	}
}

int ltr303_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct ltr303_priv *obj = (struct ltr303_priv *)self;

	APS_LOG("ltr303_ps_operate command:%d\n",command);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value)
				{
					if(err = ltr303_enable_ps(obj->client, true))
					{
						APS_ERR("enable ps fail: %d\n", err);
						return -1;
					}
					set_bit(CMC_BIT_PS, &obj->enable);
				}
				else
				{
					if(err = ltr303_enable_ps(obj->client, false))
					{
						APS_ERR("disable ps fail: %d\n", err);
						return -1;
					}
					clear_bit(CMC_BIT_PS, &obj->enable);
				}
			}
			break;

		case SENSOR_GET_DATA:
			if ((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data))) {
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			} else {
				sensor_data = (hwm_sensor_data *)buff_out;
				if (err = ltr303_read_data_ps(obj->client, &obj->ps)) {
					err = -1;
					break;
				} else {
					while(-1 == ltr303_get_ps_value(obj, obj->ps)) {
						ltr303_read_data_ps(obj->client, &obj->ps);
						msleep(50);
					}
					sensor_data->values[0] = ltr303_get_ps_value(obj, obj->ps);
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
					APS_ERR("fwq get ps raw_data = %d, sensor_data =%d\n",obj->ps, sensor_data->values[0]);
				}
			}
			break;
		default:
			APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}

int ltr303_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct ltr303_priv *obj = (struct ltr303_priv *)self;

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value)
				{
					if(err = ltr303_enable_als(obj->client, true))
					{
						APS_ERR("enable als fail: %d\n", err);
						return -1;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
				}
				else
				{
					if(err = ltr303_enable_als(obj->client, false))
					{
						APS_ERR("disable als fail: %d\n", err);
						return -1;
					}
					clear_bit(CMC_BIT_ALS, &obj->enable);
				}
			}
			break;

		case SENSOR_GET_DATA:
			//APS_LOG("fwq get als data !!!!!!\n");
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("ltr303_als_operate get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (hwm_sensor_data *)buff_out;

				if(err = ltr303_read_data_als(obj->client, &obj->als))
				{
					err = -1;;
				}
				else
				{
					/*
					while(-1 == ltr303_get_als_value(obj, obj->als))
					{
						ltr303_read_data_als(obj->client, &obj->als);
						msleep(50);
					}
					*/
					//#if defined(MALATA_E7801)
					//sensor_data->values[0] = obj->als;
					//#else
					sensor_data->values[0] = ltr303_get_als_value(obj, obj->als);
					//#endif
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
					//APS_ERR("ltr303_als_operate get obj->als = %d, sensor_data =%d\n",obj->als, sensor_data->values[0]);
				}
			}
			break;
		default:
			APS_ERR("light sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}
static int als_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}
static int als_enable_nodata(int en)
{
        int err=0;
		
        if(ltr303_obj==NULL){
		APS_ERR("ltr303_i2c_client is null!!\n");
		return -1;
        }
	if(en)
	{
		if(err = ltr303_enable_als(ltr303_obj->client, true))
		{
			APS_ERR("enable als fail: %d\n", err);
			return -1;
		}
		set_bit(CMC_BIT_ALS, &ltr303_obj->enable);
	}
	else
	{
		if(err = ltr303_enable_als(ltr303_obj->client, false))
		{
			APS_ERR("disable als fail: %d\n", err);
			return -1;
		}
		clear_bit(CMC_BIT_ALS, &ltr303_obj->enable);
	}	

}
static int als_set_delay(u64 ns)
{
	return 0;
}
static int als_get_data(int* value, int* status)
{
        int alsraw=0;
        int err=0;
		
        if(ltr303_obj==NULL){
		APS_ERR("ltr303_i2c_client is null!!\n");
		return -EINVAL;
        }
	else
	{
		if(err = ltr303_read_data_als(ltr303_obj->client, &alsraw))
		{
			return -EINVAL;
		}
		else
		{
			*value= ltr303_get_als_value(ltr303_obj, alsraw);
			*status = SENSOR_STATUS_ACCURACY_MEDIUM;
			 APS_ERR("als_get_data *value = %d, *status =%d\n",*value, *status);
		}
	}

        return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr303_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
	APS_FUN();
	strcpy(info->type, LTR303_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr303_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ltr303_priv *obj;
	struct hwmsen_object obj_ps, obj_als;
	struct als_control_path als_ctl={0};
	struct als_data_path als_data={0};
	struct ps_control_path ps_ctl={0};
	struct ps_data_path ps_data={0};

	
	int err = 0;

	u8 buf = 0;
	int addr = 1;
	int ret = 0;
#ifdef CONFIG_GN_DEVICE_CHECK
	struct gn_device_info gn_dev_info_light = {0};
	struct gn_device_info gn_dev_info_proximity = {0};
#endif
		APS_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	ltr303_obj = obj;

    obj->hw = get_cust_alsps_hw();

#if 0//def GN_MTK_BSP_ALSPS_INTERRUPT_MODE
	INIT_DELAYED_WORK(&obj->eint_work, ltr303_eint_work);
#endif
	obj->client = client;
	APS_LOG("addr = %x\n",obj->client->addr);
	i2c_set_clientdata(client, obj);
	atomic_set(&obj->als_debounce, 1000);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 1000);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->trace, 0x00);
	atomic_set(&obj->als_suspend, 0);

	obj->ps_enable = 0;
	obj->als_enable = 0;
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);

	//pre set ps threshold
	obj->ps_thd_val = obj->hw->ps_threshold;
	//pre set window loss
	obj->als_widow_loss = obj->hw->als_window_loss;

	ltr303_i2c_client = client;

	if(err = hwmsen_read_byte_sr(client, APS_RO_PART_ID, &part_id))
	{
		APS_ERR("LTR303 i2c transfer error, part_id:%d\n", part_id);
		goto exit_init_failed;		
	}

	if(err = ltr303_init_client(client))
	{
		goto exit_init_failed;
	}

	if(err = ltr303_enable_als(client, false))
	{
		APS_ERR("disable als fail: %d\n", err);
	}
	APS_ERR("ltr303_i2c_probe %d\n",__LINE__);
	#if 0//ndef CUSTOM_KERNEL_ALSPS_NO_PROXIMITY 
	if(err = ltr303_enable_ps(client, false))
	{
		APS_ERR("disable ps fail: %d\n", err);
	}
	#endif//xmxl@20140820: ATA tool can not find alsps
	if(err = misc_register(&ltr303_device))
	{
		APS_ERR("ltr303_device register failed\n");
		goto exit_misc_device_register_failed;
	}
	
	if(err = ltr303_create_attr(&ltr303_init_info.platform_diver_addr->driver))
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

        als_ctl.open_report_data= als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay  = als_set_delay;
	als_ctl.is_report_input_direct = false;
       #if 0//def CUSTOM_KERNEL_SENSORHUB
	als_ctl.is_support_batch = obj->hw->is_batch_supported_als;
        #else
        als_ctl.is_support_batch = false;
        #endif
	
	err = als_register_control_path(&als_ctl);
	if(err)
	{
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	als_data.get_data = als_get_data;
	als_data.vender_div = 100;
	err = als_register_data_path(&als_data);	
	if(err)
	{
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	err = batch_register_support_info(ID_LIGHT,als_ctl.is_support_batch, 100, 0);
	if(err)
	{
		APS_ERR("register light batch support err = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}
	#if 0//ndef CUSTOM_KERNEL_ALSPS_NO_PROXIMITY 
	obj_ps.self = ltr303_obj;
	//APS_ERR("obj->hw->polling_mode:%d\n",obj->hw->polling_mode);
	if(1 == obj->hw->polling_mode_ps)
	{
		obj_ps.polling = 1;
	}
	else
	{
		obj_ps.polling = 0;//interrupt mode
	}
	obj_ps.sensor_operate = ltr303_ps_operate;
	if(err = hwmsen_attach(ID_PROXIMITY, &obj_ps))
	{
		APS_ERR("attach ID_PROXIMITY fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	#endif
        #if 	0
	obj_als.self = ltr303_obj;
	ltr303_obj->polling = obj->hw->polling_mode;
	if(1 == obj->hw->polling_mode_als)
	{
		obj_als.polling = 1;
		APS_ERR("als polling mode\n");
	}
	else
	{
		obj_als.polling = 0;//interrupt mode
		APS_ERR("als interrupt mode\n");
	}
	obj_als.sensor_operate = ltr303_als_operate;
	if(err = hwmsen_attach(ID_LIGHT, &obj_als))
	{
		APS_ERR("attach ID_LIGHT fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	#endif 				
#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level	= EARLY_SUSPEND_LEVEL_DISABLE_FB,
	obj->early_drv.suspend = ltr303_early_suspend,
	obj->early_drv.resume = ltr303_late_resume,
	register_early_suspend(&obj->early_drv);
#endif

	ltr303_init_flag = 0;
	APS_LOG("%s: OK\n", __func__);
	return 0;

#if 0//def CONFIG_GN_DEVICE_CHECK
	gn_dev_info_light.gn_dev_type = GN_DEVICE_TYPE_LIGHT;
	strcpy(gn_dev_info_light.name, LTR303_DEV_NAME);
	gn_set_device_info(gn_dev_info_light);

	gn_dev_info_proximity.gn_dev_type = GN_DEVICE_TYPE_PROXIMITY;
	strcpy(gn_dev_info_proximity.name, LTR303_DEV_NAME);
	gn_set_device_info(gn_dev_info_proximity);
#endif
	
	//APS_LOG("%s: OK\n", __func__);
	//return 0;

	exit_create_attr_failed:
        exit_sensor_obj_attach_fail:
	misc_deregister(&ltr303_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	exit_kfree:
	kfree(obj);
	exit:
	ltr303_i2c_client = NULL;
	ltr303_obj=NULL;
	mt_eint_unmask(CUST_EINT_ALS_NUM);

	ltr303_init_flag = -1;
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr303_i2c_remove(struct i2c_client *client)
{
	int err;

	if(err = ltr303_delete_attr(&ltr303_i2c_driver.driver))
	{
		APS_ERR("ltr303_delete_attr fail: %d\n", err);
	}

	if(err = misc_deregister(&ltr303_device))
	{
		APS_ERR("misc_deregister fail: %d\n", err);
	}

	ltr303_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

static int ltr303_probe(struct platform_device *pdev)
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_FUN();

	ltr303_power(hw,1);
	if(i2c_add_driver(&ltr303_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	}

	if(-1 == ltr303_init_flag)
	{
			return -1;
	}

	return 0;
}

static int  ltr303_local_init(void)
{
    struct alsps_hw *hw = get_cust_alsps_hw();
	printk("fwq loccal init+++\n");

	ltr303_power(hw, 1);
	if(i2c_add_driver(&ltr303_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	}
	if(-1 == ltr303_init_flag)
	{
	   return -1;
	}
	printk("fwq loccal init---\n");
	return 0;
}


static int ltr303_remove(void)
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_FUN();
	ltr303_power(hw, 0);
	i2c_del_driver(&ltr303_i2c_driver);
	return 0;
}

/*static struct platform_driver ltr303_alsps_driver = {
	.probe	= ltr303_probe,
	.remove	= ltr303_remove,
	.driver	= {
		.name	= "als_ps",
//		.owner	= THIS_MODULE,
	}
};*/


static int __init ltr303_init(void)
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
	//wake_lock_init(&ps_wake_lock,WAKE_LOCK_SUSPEND,"ps module");
	i2c_register_board_info(hw->i2c_num, &i2c_ltr303, 1);  //xiaoqian, 20120412, add for alsps
	alsps_driver_add(&ltr303_init_info);
	/*if (platform_driver_register(&ltr303_alsps_driver)) {
		APS_ERR("failed to register driver");
		return -ENODEV;
	} */
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit ltr303_exit(void)
{
	APS_FUN();
	//platform_driver_unregister(&ltr303_alsps_driver);
	//wake_lock_destroy(&ps_wake_lock);
}
/*----------------------------------------------------------------------------*/
module_init(ltr303_init);
module_exit(ltr303_exit);
/*----------------------------------------------------------------------------*/
MODULE_DESCRIPTION("LTR303 light sensor & p sensor driver");
MODULE_LICENSE("GPL");




