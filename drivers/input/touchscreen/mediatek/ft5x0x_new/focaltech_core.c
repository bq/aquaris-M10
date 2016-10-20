/*
 * (c) MediaTek Inc. 2010
 */


#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>

#include "include/tpd_ft5x0x_common.h"

#include "focaltech_core.h"
/* #include "ft5x06_ex_fun.h" */

#include "tpd.h"

/* #define TIMER_DEBUG */


#ifdef TIMER_DEBUG
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#endif

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
#include <mach/md32_ipi.h>
#include <mach/md32_helper.h>
#include <linux/irqchip/mtk-gic-extend.h>
#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
enum DOZE_T {
	DOZE_DISABLED = 0,
	DOZE_ENABLED = 1,
	DOZE_WAKEUP = 2,
};
static enum DOZE_T doze_status = DOZE_DISABLED;
#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
static s8 ftp_enter_doze(struct i2c_client *client);

enum TOUCH_IPI_CMD_T {
	/* SCP->AP */
	IPI_COMMAND_SA_GESTURE_TYPE,
	/* AP->SCP */
	IPI_COMMAND_AS_CUST_PARAMETER,
	IPI_COMMAND_AS_ENTER_DOZEMODE,
	IPI_COMMAND_AS_ENABLE_GESTURE,
	IPI_COMMAND_AS_GESTURE_SWITCH,
};

struct Touch_Cust_Setting {
	u32 i2c_num;
	u32 int_num;
	u32 io_int;
	u32 io_rst;
};

struct Touch_IPI_Packet {
	u32 cmd;
	union {
		u32 data;
		struct Touch_Cust_Setting tcs;
	} param;
};

static bool tpd_scp_doze_en;
/*static bool tpd_scp_doze_en = true;*/
DEFINE_MUTEX(i2c_access);
#endif

#define TPD_SUPPORT_POINTS	10


struct i2c_client *i2c_client = NULL;
struct task_struct *thread_tpd = NULL;
/*******************************************************************************
* 4.Static variables
*******************************************************************************/
struct i2c_client *fts_i2c_client 				= NULL;
struct input_dev *fts_input_dev				=NULL;
#ifdef TPD_AUTO_UPGRADE
static bool is_update = false;
#endif
#ifdef CONFIG_FT_AUTO_UPGRADE_SUPPORT
u8 *tpd_i2c_dma_va = NULL;
dma_addr_t tpd_i2c_dma_pa = 0;
#endif

#define tp_debug	1
#ifdef  tp_debug
#define tp_log	printk
#else
#define tp_log	pr_debug
#endif

#ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
static struct proc_dir_entry *ft5x0x_proc_entry = NULL;
int double_tap_wakeup_enable = 0;
static  u8 tpd_restart_flag = 0;
#define FTS_GESTURE_SWITCH_REG          0xd0
#define FTS_GESTURE_DOUBLECLICK_REG     0xd1
#define FTS_GESTURE_OUTPUT_REG          0xd3
#define FTS_GESTURE_DOUBLECLICK_ID	    0x24
#define FTS_GESTURE_SWITCH_OPEN	        1
#define FTS_GESTURE_SWITCH_CLOSE	    0
#define FTS_GESTURE_READ_REPEAT         5
#define FTS_GESTURE_WRITE_REPEAT        5
static unsigned int gesture_switch = FTS_GESTURE_SWITCH_CLOSE;
static struct wake_lock db_click_wakeup_lock;
#endif


static DECLARE_WAIT_QUEUE_HEAD(waiter);

static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id);


static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);
static void tpd_resume(struct device *h);
static void tpd_suspend(struct device *h);
static int tpd_flag;
/*static int point_num = 0;
static int p_point_num = 0;*/

unsigned int tpd_rst_gpio_number = 0;
unsigned int tpd_int_gpio_number = 1;
unsigned int touch_irq = 0;
#define TPD_OK 0

/* Register define */
#define DEVICE_MODE	0x00
#define GEST_ID		0x01
#define TD_STATUS	0x02

#define TOUCH1_XH	0x03
#define TOUCH1_XL	0x04
#define TOUCH1_YH	0x05
#define TOUCH1_YL	0x06

#define TOUCH2_XH	0x09
#define TOUCH2_XL	0x0A
#define TOUCH2_YH	0x0B
#define TOUCH2_YL	0x0C

#define TOUCH3_XH	0x0F
#define TOUCH3_XL	0x10
#define TOUCH3_YH	0x11
#define TOUCH3_YL	0x12

#define TPD_RESET_ISSUE_WORKAROUND
#define TPD_MAX_RESET_COUNT	3

#ifdef TIMER_DEBUG

static struct timer_list test_timer;

static void timer_func(unsigned long data)
{
	tpd_flag = 1;
	wake_up_interruptible(&waiter);

	mod_timer(&test_timer, jiffies + 100*(1000/HZ));
}

static int init_test_timer(void)
{
	memset((void *)&test_timer, 0, sizeof(test_timer));
	test_timer.expires  = jiffies + 100*(1000/HZ);
	test_timer.function = timer_func;
	test_timer.data     = 0;
	init_timer(&test_timer);
	add_timer(&test_timer);
	return 0;
}
#endif


#if defined(CONFIG_TPD_ROTATE_90) || defined(CONFIG_TPD_ROTATE_270) || defined(CONFIG_TPD_ROTATE_180)
/*
static void tpd_swap_xy(int *x, int *y)
{
	int temp = 0;

	temp = *x;
	*x = *y;
	*y = temp;
}
*/
/*
static void tpd_rotate_90(int *x, int *y)
{
//	int temp;

	*x = TPD_RES_X + 1 - *x;

	*x = (*x * TPD_RES_Y) / TPD_RES_X;
	*y = (*y * TPD_RES_X) / TPD_RES_Y;

	tpd_swap_xy(x, y);
}
*/
static void tpd_rotate_180(int *x, int *y)
{
	*y = TPD_RES_Y + 1 - *y;
	*x = TPD_RES_X + 1 - *x;
}
/*
static void tpd_rotate_270(int *x, int *y)
{
//	int temp;

	*y = TPD_RES_Y + 1 - *y;

	*x = (*x * TPD_RES_Y) / TPD_RES_X;
	*y = (*y * TPD_RES_X) / TPD_RES_Y;

	tpd_swap_xy(x, y);
}
*/
#endif
struct touch_info {
	int y[TPD_SUPPORT_POINTS];
	int x[TPD_SUPPORT_POINTS];
	int p[TPD_SUPPORT_POINTS];
	int id[TPD_SUPPORT_POINTS];
	int count;
};

/*dma declare, allocate and release*/
//#define __MSG_DMA_MODE__
#ifdef CONFIG_MTK_I2C_EXTENSION
	u8 *g_dma_buff_va = NULL;
	dma_addr_t g_dma_buff_pa = 0;
#endif

#ifdef CONFIG_MTK_I2C_EXTENSION

	static void msg_dma_alloct(void)
	{
	    if (NULL == g_dma_buff_va)
    		{
       		 tpd->dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
       		 g_dma_buff_va = (u8 *)dma_alloc_coherent(&tpd->dev->dev, 128, &g_dma_buff_pa, GFP_KERNEL);
    		}

	    	if(!g_dma_buff_va)
		{
	        	TPD_DMESG("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
	    	}
	}
	static void msg_dma_release(void){
		if(g_dma_buff_va)
		{
	     		dma_free_coherent(NULL, 128, g_dma_buff_va, g_dma_buff_pa);
	        	g_dma_buff_va = NULL;
	        	g_dma_buff_pa = 0;
			TPD_DMESG("[DMA][release] Allocate DMA I2C Buffer release!\n");
	    	}
	}
#endif

//static DEFINE_MUTEX(i2c_access);
static DEFINE_MUTEX(i2c_rw_access);

#if (defined(CONFIG_TPD_HAVE_CALIBRATION) && !defined(CONFIG_TPD_CUSTOM_CALIBRATION))
/* static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX; */
/* static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX; */
static int tpd_def_calmat_local_normal[8]  = TPD_CALIBRATION_MATRIX_ROTATION_NORMAL;
static int tpd_def_calmat_local_factory[8] = TPD_CALIBRATION_MATRIX_ROTATION_FACTORY;
#endif

static const struct i2c_device_id ft5x0x_tpd_id[] = {{"ft5x0x", 0}, {} };
static const struct of_device_id ft5x0x_dt_match[] = {
	{.compatible = "mediatek,cap_touch"},
	{},
};
MODULE_DEVICE_TABLE(of, ft5x0x_dt_match);

static struct i2c_driver tpd_i2c_driver = {
	.driver = {
		.of_match_table = of_match_ptr(ft5x0x_dt_match),
		.name = "ft5x0x",
	},
	.probe = tpd_probe,
	.remove = tpd_remove,
	.id_table = ft5x0x_tpd_id,
	.detect = tpd_i2c_detect,
};

static int of_get_ft5x0x_platform_data(struct device *dev)
{
	/*int ret, num;*/

	if (dev->of_node) {
		const struct of_device_id *match;

		match = of_match_device(of_match_ptr(ft5x0x_dt_match), dev);
		if (!match) {
			TPD_DMESG("Error: No device match found\n");
			return -ENODEV;
		}
	}
	tpd_rst_gpio_number = of_get_named_gpio(dev->of_node, "rst-gpio", 0);
	tpd_int_gpio_number = of_get_named_gpio(dev->of_node, "int-gpio", 0);
	/*ret = of_property_read_u32(dev->of_node, "rst-gpio", &num);
	if (!ret)
		tpd_rst_gpio_number = num;
	ret = of_property_read_u32(dev->of_node, "int-gpio", &num);
	if (!ret)
		tpd_int_gpio_number = num;
  */
	TPD_DMESG("g_vproc_en_gpio_number %d\n", tpd_rst_gpio_number);
	TPD_DMESG("g_vproc_vsel_gpio_number %d\n", tpd_int_gpio_number);
	return 0;
}

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT

void tpd_scp_wakeup_enable(bool en)
{
	tpd_scp_doze_en = en;
}

void tpd_enter_doze(void)
{

}

static ssize_t show_scp_ctrl(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t store_scp_ctrl(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long cmd;
	/*struct Touch_IPI_Packet ipi_pkt;*/

	if (kstrtoul(buf, 10, &cmd)) {
		TPD_DEBUG("[SCP_CTRL]: Invalid values\n");
		return -EINVAL;
	}

	TPD_DEBUG("SCP_CTRL: Command=%lu", cmd);
	switch (cmd) {
	case 1:
	    /* make touch in doze mode */
	    tpd_scp_wakeup_enable(true);
	    tpd_suspend(NULL);
	    break;
	case 2:
	    tpd_resume(NULL);
	    break;
		/*case 3:
	    // emulate in-pocket on
	    ipi_pkt.cmd = IPI_COMMAND_AS_GESTURE_SWITCH,
	    ipi_pkt.param.data = 1;
		md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
	    break;
	case 4:
	    // emulate in-pocket off
	    ipi_pkt.cmd = IPI_COMMAND_AS_GESTURE_SWITCH,
	    ipi_pkt.param.data = 0;
		md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
	    break;*/
	case 5:
		{
				struct Touch_IPI_Packet ipi_pkt;

				ipi_pkt.cmd = IPI_COMMAND_AS_CUST_PARAMETER;
			    ipi_pkt.param.tcs.i2c_num = 0;/* shuold be modify according your hardware design*/
			ipi_pkt.param.tcs.int_num = get_hardware_irq(touch_irq);
				ipi_pkt.param.tcs.io_int = tpd_int_gpio_number;
			ipi_pkt.param.tcs.io_rst = tpd_rst_gpio_number;
			if (md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0) < 0)
				TPD_DEBUG("[TOUCH] IPI cmd failed (%d)\n", ipi_pkt.cmd);

			break;
		}
	default:
	    TPD_DEBUG("[SCP_CTRL] Unknown command");
	    break;
	}

	return size;
}
static DEVICE_ATTR(tpd_scp_ctrl, 0664, show_scp_ctrl, store_scp_ctrl);
#endif

static struct device_attribute *ft5x0x_attrs[] = {
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	&dev_attr_tpd_scp_ctrl,
#endif
};


static void tpd_down(int x, int y, int p, int id)
{
#if defined(CONFIG_TPD_ROTATE_90)
	tpd_rotate_90(&x, &y);
#elif defined(CONFIG_TPD_ROTATE_270)
	tpd_rotate_270(&x, &y);
#elif defined(CONFIG_TPD_ROTATE_180)
	tpd_rotate_180(&x, &y);
#endif

#ifdef TPD_SOLVE_CHARGING_ISSUE
	if (0 != x) {
#else
	{
#endif
		//input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, id);
		TPD_DEBUG("%s x:%d y:%d p:%d\n", __func__, x, y, p);
		input_report_key(tpd->dev, BTN_TOUCH, 1);
		input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 20);
		input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0x3f);
		input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
		input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
		input_mt_sync(tpd->dev);
	}
}

static void tpd_up(int x, int y)
{
#if defined(CONFIG_TPD_ROTATE_90)
	tpd_rotate_90(&x, &y);
#elif defined(CONFIG_TPD_ROTATE_270)
	tpd_rotate_270(&x, &y);
#elif defined(CONFIG_TPD_ROTATE_180)
	tpd_rotate_180(&x, &y);
#endif

#ifdef TPD_SOLVE_CHARGING_ISSUE
	if (0 != x) {
#else
	{
#endif
		TPD_DEBUG("%s x:%d y:%d\n", __func__, x, y);
		input_report_key(tpd->dev, BTN_TOUCH, 0);
		input_mt_sync(tpd->dev);
	}
}

/*Coordination mapping*/
/*
static void tpd_calibrate_driver(int *x, int *y)
{
	int tx;

	tx = ((tpd_def_calmat[0] * (*x)) + (tpd_def_calmat[1] * (*y)) + (tpd_def_calmat[2])) >> 12;
	*y = ((tpd_def_calmat[3] * (*x)) + (tpd_def_calmat[4] * (*y)) + (tpd_def_calmat[5])) >> 12;
	*x = tx;
}
*/


 /************************************************************************
* Name: tpd_touchinfo
* Brief: touch info
* Input: touch info point, no use
* Output: no
* Return: success nonzero
***********************************************************************/
#ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
     static void tpd_report_key(void)
     {
         input_report_key(tpd->dev, KEY_POWER, 1);
		 input_sync(tpd->dev);
		 input_report_key(tpd->dev, KEY_POWER, 0);
		 input_sync(tpd->dev);
     }
	 
	 static int tpd_read_gesture_data(void)
	 {
		 unsigned char gesture_id = 0;
		 fts_read_reg(fts_i2c_client, FTS_GESTURE_OUTPUT_REG, &gesture_id);
	 
         if(FTS_GESTURE_DOUBLECLICK_ID == gesture_id){
            gesture_switch = FTS_GESTURE_SWITCH_CLOSE;
            tpd_report_key();
			tp_log("***double click wake up ok!***\n");
		 }
	 
		 return -1;
	 }
#endif

static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
{
	int i = 0;
	char data[128] = {0};
	u8 report_rate = 0;
	u16 high_byte, low_byte;
	char writebuf[10]={0};
	u8 fwversion = 0;

	writebuf[0]=0x00;
	fts_i2c_read(i2c_client, writebuf,  1, data, 64);

	fts_read_reg(i2c_client, 0xa6, &fwversion);
	fts_read_reg(i2c_client, 0x88, &report_rate);

	TPD_DEBUG("FW version=%x\n", fwversion);

#if 0
	TPD_DEBUG("received raw data from touch panel as following:\n");
	for (i = 0; i < 8; i++)
		TPD_DEBUG("data[%d] = 0x%02X ", i, data[i]);
	TPD_DEBUG("\n");
	for (i = 8; i < 16; i++)
		TPD_DEBUG("data[%d] = 0x%02X ", i, data[i]);
	TPD_DEBUG("\n");
	for (i = 16; i < 24; i++)
		TPD_DEBUG("data[%d] = 0x%02X ", i, data[i]);
	TPD_DEBUG("\n");
	for (i = 24; i < 32; i++)
		TPD_DEBUG("data[%d] = 0x%02X ", i, data[i]);
	TPD_DEBUG("\n");
#endif
	if (report_rate < 8) {
		report_rate = 0x8;
		if ((fts_write_reg(i2c_client, 0x88, report_rate)) < 0)
			TPD_DMESG("I2C write report rate error, line: %d\n", __LINE__);
	}

	/* Device Mode[2:0] == 0 :Normal operating Mode*/
	if ((data[0] & 0x70) != 0)
		return false;

	memcpy(pinfo, cinfo, sizeof(struct touch_info));
	memset(cinfo, 0, sizeof(struct touch_info));
	for (i = 0; i < TPD_SUPPORT_POINTS; i++)
		cinfo->p[i] = 1;	/* Put up */

	/*get the number of the touch points*/
	cinfo->count = data[2] & 0x0f;

	TPD_DEBUG("Number of touch points = %d\n", cinfo->count);

	TPD_DEBUG("Procss raw data...\n");

	for (i = 0; i < cinfo->count; i++) {
		cinfo->p[i] = (data[3 + 6 * i] >> 6) & 0x0003; /* event flag */
		cinfo->id[i] = data[3+6*i+2]>>4; 						// touch id

		/*get the X coordinate, 2 bytes*/
		high_byte = data[3 + 6 * i];
		high_byte <<= 8;
		high_byte &= 0x0F00;

		low_byte = data[3 + 6 * i + 1];
		low_byte &= 0x00FF;
		cinfo->x[i] = high_byte | low_byte;

		/*get the Y coordinate, 2 bytes*/
		high_byte = data[3 + 6 * i + 2];
		high_byte <<= 8;
		high_byte &= 0x0F00;

		low_byte = data[3 + 6 * i + 3];
		low_byte &= 0x00FF;
		cinfo->y[i] = high_byte | low_byte;

		if((tpd_dts_data.tpd_convert_ratio[0] != 0) && (tpd_dts_data.tpd_convert_ratio[1] != 0)){
			cinfo->x[i] = (cinfo->x[i] * tpd_dts_data.tpd_convert_ratio[0])/tpd_dts_data.tpd_convert_ratio[1];
			cinfo->y[i] = (cinfo->y[i] * tpd_dts_data.tpd_convert_ratio[0])/tpd_dts_data.tpd_convert_ratio[1];
		}
		TPD_DEBUG("covert after: cinfo->x[%d] = %d, cinfo->y[%d] = %d, cinfo->p[%d] = %d, ratio=%d : %d\n", i,
		cinfo->x[i], i, cinfo->y[i], i, cinfo->p[i], tpd_dts_data.tpd_convert_ratio[0], tpd_dts_data.tpd_convert_ratio[1]);
	}




#ifdef CONFIG_TPD_HAVE_CALIBRATION
	for (i = 0; i < cinfo->count; i++) {
		tpd_calibrate_driver(&(cinfo->x[i]), &(cinfo->y[i]));
		TPD_DEBUG(" cinfo->x[%d] = %d, cinfo->y[%d] = %d, cinfo->p[%d] = %d\n", i,
		cinfo->x[i], i, cinfo->y[i], i, cinfo->p[i]);
	}
#endif

	return true;

};


#ifdef CONFIG_MTK_I2C_EXTENSION

int fts_i2c_read(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen)
{
	int ret=0;

	// for DMA I2c transfer

	mutex_lock(&i2c_rw_access);

	if((NULL!=client) && (writelen>0) && (writelen<=128))
	{
		// DMA Write
		memcpy(g_dma_buff_va, writebuf, writelen);
		client->addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;
		if((ret=i2c_master_send(client, (unsigned char *)g_dma_buff_pa, writelen))!=writelen)
			//dev_err(&client->dev, "###%s i2c write len=%x,buffaddr=%x\n", __func__,ret,*g_dma_buff_pa);
			printk("i2c write failed\n");
		client->addr = (client->addr & I2C_MASK_FLAG) &(~ I2C_DMA_FLAG);
	}

	// DMA Read

	if((NULL!=client) && (readlen>0) && (readlen<=128))

	{
		client->addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;

		ret = i2c_master_recv(client, (unsigned char *)g_dma_buff_pa, readlen);

		memcpy(readbuf, g_dma_buff_va, readlen);

		client->addr = (client->addr & I2C_MASK_FLAG) &(~ I2C_DMA_FLAG);
	}

	mutex_unlock(&i2c_rw_access);

	return ret;	
}
int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int i = 0;
	int ret = 0;

	if (writelen <= 8) {
	    client->ext_flag = client->ext_flag & (~I2C_DMA_FLAG);
		return i2c_master_send(client, writebuf, writelen);
	}
	else if((writelen > 8)&&(NULL != tpd_i2c_dma_va))
	{
		for (i = 0; i < writelen; i++)
			tpd_i2c_dma_va[i] = writebuf[i];

		client->addr = (client->addr & I2C_MASK_FLAG )| I2C_DMA_FLAG;
	    //client->ext_flag = client->ext_flag | I2C_DMA_FLAG;
	    ret = i2c_master_send(client, (unsigned char *)tpd_i2c_dma_pa, writelen);
	    client->addr = client->addr & I2C_MASK_FLAG & ~I2C_DMA_FLAG;
		//ret = i2c_master_send(client, (u8 *)(uintptr_t)tpd_i2c_dma_pa, writelen);
	    //client->ext_flag = client->ext_flag & (~I2C_DMA_FLAG);
		return ret;
	}
	return 1;
}

#else

int fts_i2c_read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen)
{
	int ret = 0;

	mutex_lock(&i2c_rw_access);

	if(readlen > 0)
	{
		if (writelen > 0) {
			struct i2c_msg msgs[] = {
				{
					 .addr = client->addr,
					 .flags = 0,
					 .len = writelen,
					 .buf = writebuf,
				 },
				{
					 .addr = client->addr,
					 .flags = I2C_M_RD,
					 .len = readlen,
					 .buf = readbuf,
				 },
			};
			ret = i2c_transfer(client->adapter, msgs, 2);
			if (ret < 0)
				dev_err(&client->dev, "%s: i2c read error.\n", __func__);
		} else {
			struct i2c_msg msgs[] = {
				{
					 .addr = client->addr,
					 .flags = I2C_M_RD,
					 .len = readlen,
					 .buf = readbuf,
				 },
			};
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret < 0)
				dev_err(&client->dev, "%s:i2c read error.\n", __func__);
		}
	}

	mutex_unlock(&i2c_rw_access);
	
	return ret;
}

int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret = 0;

	struct i2c_msg msgs[] = {
		{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
		 },
	};
	mutex_lock(&i2c_rw_access);

	if(writelen > 0)
	{
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s: i2c write error.\n", __func__);
	}

	mutex_unlock(&i2c_rw_access);
	
	return ret;
}

#endif
/************************************************************************
* Name: fts_write_reg
* Brief: write register
* Input: i2c info, reg address, reg value
* Output: no
* Return: fail <0
***********************************************************************/
int fts_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue)
{
	unsigned char buf[2] = {0};

	buf[0] = regaddr;
	buf[1] = regvalue;

	return fts_i2c_write(client, buf, sizeof(buf));
}
/************************************************************************
* Name: fts_read_reg
* Brief: read register
* Input: i2c info, reg address, reg value
* Output: get reg value
* Return: fail <0
***********************************************************************/
int fts_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue)
{

	return fts_i2c_read(client, &regaddr, 1, regvalue, 1);

}

static int touch_event_handler(void *unused)
{
	int i = 0;
	struct touch_info cinfo, pinfo, finfo;
	struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };

#ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
    u8 read_rep = 0;
	u8 wake_state = 0;
    u8 ret;
#endif
	
	if (tpd_dts_data.use_tpd_button) {
		memset(&finfo, 0, sizeof(struct touch_info));
		for (i = 0; i < TPD_SUPPORT_POINTS; i++)
			finfo.p[i] = 1;
	}

	sched_setscheduler(current, SCHED_RR, &param);

	do {
		/*enable_irq(touch_irq);*/
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, tpd_flag != 0);
		tpd_flag = 0;
		set_current_state(TASK_RUNNING);
	
        #ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
		       if(FTS_GESTURE_SWITCH_OPEN == gesture_switch){
                  wake_lock_timeout(&db_click_wakeup_lock, HZ*4);//let the system not sleep till 3s, wwj add
                  tp_log("***double_tap_wakeup_enable=%d***\n",double_tap_wakeup_enable);
			   }
		          
			   if(tpd_restart_flag)
			   	   continue;

               if (FTS_GESTURE_SWITCH_OPEN == gesture_switch && double_tap_wakeup_enable)
		       {  
				   read_rep = 0;   
				   do{
                       wake_state = 0;
					   ret = fts_read_reg(fts_i2c_client, FTS_GESTURE_SWITCH_REG, &wake_state);
					   if(FTS_GESTURE_SWITCH_OPEN == wake_state)
						   break;
					   else
					   	   read_rep++;
                    
				   }while(read_rep < FTS_GESTURE_READ_REPEAT);

				   if(read_rep < FTS_GESTURE_READ_REPEAT){
				   	
					   tp_log("---read OK !, read_rep = %d, wake_state=%d---\n",read_rep,wake_state);
					   if (FTS_GESTURE_SWITCH_OPEN == wake_state)
					   {
						   tpd_read_gesture_data();
						   continue;
					   }
				   }
				   else{
				   	      tp_log("---read failed!, resume the system still!---\n");
                          gesture_switch = FTS_GESTURE_SWITCH_CLOSE;
	                      tpd_report_key();
					      continue;
				   }
		      }	   
        #endif 
		
		TPD_DEBUG("touch_event_handler start\n");

		if (tpd_touchinfo(&cinfo, &pinfo)) {
			if (tpd_dts_data.use_tpd_button) {
				if (cinfo.p[0] == 0)
					memcpy(&finfo, &cinfo, sizeof(struct touch_info));
			}

			if ((cinfo.y[0] >= TPD_RES_Y) && (pinfo.y[0] < TPD_RES_Y)
			&& ((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
				TPD_DEBUG("Dummy release --->\n");
				tpd_up(pinfo.x[0], pinfo.y[0]);
				input_sync(tpd->dev);
				continue;
			}

			if (tpd_dts_data.use_tpd_button) {
				if ((cinfo.y[0] <= TPD_RES_Y && cinfo.y[0] != 0) && (pinfo.y[0] > TPD_RES_Y)
				&& ((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
					TPD_DEBUG("Dummy key release --->\n");
					tpd_button(pinfo.x[0], pinfo.y[0], 0);
					input_sync(tpd->dev);
					continue;
				}

				if ((cinfo.y[0] > TPD_RES_Y) || (pinfo.y[0] > TPD_RES_Y)) {
					if (finfo.y[0] > TPD_RES_Y) {
                        				TPD_DEBUG("Key x=%d, y=%d --->\n", pinfo.x[0], pinfo.y[0]);
						if ((cinfo.p[0] == 0) || (cinfo.p[0] == 2)) {
								TPD_DEBUG("Key press --->\n");
								tpd_button(pinfo.x[0], pinfo.y[0], 1);
						} else if ((cinfo.p[0] == 1) &&
							((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
								TPD_DEBUG("Key release --->\n");
								tpd_button(pinfo.x[0], pinfo.y[0], 0);
						}
						input_sync(tpd->dev);
					}
					continue;
				}
			}

			if (cinfo.count > 0) {
				for (i = 0; i < cinfo.count; i++)
					tpd_down(cinfo.x[i], cinfo.y[i], i + 1, cinfo.id[i]);
			} else {
#ifdef TPD_SOLVE_CHARGING_ISSUE
				tpd_up(1, 48);
#else
				tpd_up(cinfo.x[0], cinfo.y[0]);
#endif

			}
			input_sync(tpd->dev);

		}
	} while (!kthread_should_stop());

	TPD_DEBUG("touch_event_handler exit\n");

	return 0;
}

static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, TPD_DEVICE);

	return 0;
}

static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id)
{
	TPD_DEBUG("TPD interrupt has been triggered\n");
	//tp_log("\n***TPD interrupt been triggered*****\n");
	tpd_flag = 1;
	wake_up_interruptible(&waiter);
	return IRQ_HANDLED;
}
static int tpd_irq_registration(void)
{
	struct device_node *node = NULL;
	int ret = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,cap_touch");
	if (node) {
		/*touch_irq = gpio_to_irq(tpd_int_gpio_number);*/
		touch_irq = irq_of_parse_and_map(node, 0);
		ret = request_irq(touch_irq, tpd_eint_interrupt_handler,
					IRQF_TRIGGER_FALLING, TPD_DEVICE, NULL);
			if (ret > 0)
				TPD_DMESG("tpd request_irq IRQ LINE NOT AVAILABLE!.");
			
       #ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
          enable_irq_wake(touch_irq); //wwj add, set irq wake up source
       #endif
		
	} else {
		TPD_DMESG("[%s] tpd request_irq can not find touch eint device node!.", __func__);
	}

	return 0;
}
#if 0
int hidi2c_to_stdi2c(struct i2c_client * client)
{
	u8 auc_i2c_write_buf[5] = {0};
	int bRet = 0;

	auc_i2c_write_buf[0] = 0xeb;
	auc_i2c_write_buf[1] = 0xaa;
	auc_i2c_write_buf[2] = 0x09;

	fts_i2c_write(client, auc_i2c_write_buf, 3);

	msleep(10);

	auc_i2c_write_buf[0] = auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = 0;

	fts_i2c_read(client, auc_i2c_write_buf, 0, auc_i2c_write_buf, 3);

	if(0xeb==auc_i2c_write_buf[0] && 0xaa==auc_i2c_write_buf[1] && 0x08==auc_i2c_write_buf[2])
	{
		bRet = 1;
	}
	else
		bRet = 0;

	return bRet;

}
#endif
static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int retval = TPD_OK;
	u8 report_rate = 0;
	int reset_count = 0;
	char data;
	int err = 0;

	i2c_client = client;
	fts_i2c_client = client;
	fts_input_dev=tpd->dev;
	if(i2c_client->addr != 0x38)
	{
		i2c_client->addr = 0x38;
		printk("frank_zhonghua:i2c_client_FT->addr=%d\n",i2c_client->addr);
	}

	of_get_ft5x0x_platform_data(&client->dev);
	/* configure the gpio pins */

	TPD_DMESG("mtk_tpd: tpd_probe ft5x0x\n");


	retval = regulator_enable(tpd->reg);
	if (retval != 0)
		TPD_DMESG("Failed to enable reg-vgp6: %d\n", retval);

	/* set INT mode */

	tpd_gpio_as_int(tpd_int_gpio_number);

	tpd_irq_registration();
	msleep(100);
	#ifdef CONFIG_MTK_I2C_EXTENSION
	msg_dma_alloct();
	#endif

#ifdef CONFIG_FT_AUTO_UPGRADE_SUPPORT

    if (NULL == tpd_i2c_dma_va)
    {
        tpd->dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
        tpd_i2c_dma_va = (u8 *)dma_alloc_coherent(&tpd->dev->dev, 250, &tpd_i2c_dma_pa, GFP_KERNEL);
    }
    if (!tpd_i2c_dma_va)
		TPD_DMESG("TPD dma_alloc_coherent error!\n");
	else
		TPD_DMESG("TPD dma_alloc_coherent success!\n");
#endif

#ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
		gesture_switch = FTS_GESTURE_SWITCH_CLOSE;
		wake_lock_init(&db_click_wakeup_lock, WAKE_LOCK_SUSPEND, "db_click_wakeup_lock"); //wwj add
		fts_Gesture_init(tpd->dev);
#endif

reset_proc:
	/* Reset CTP */
	tpd_gpio_output(tpd_rst_gpio_number, 0);
	msleep(20);
	tpd_gpio_output(tpd_rst_gpio_number, 1);
	msleep(400);
	err = fts_read_reg(i2c_client, 0x00, &data);

//	TPD_DMESG("fts_i2c:err %d,data:%d\n", err,data);
	if(err< 0 || data!=0)// reg0 data running state is 0; other state is not 0
	{
		TPD_DMESG("I2C transfer error, line: %d\n", __LINE__);
#ifdef TPD_RESET_ISSUE_WORKAROUND
		if ( ++reset_count < TPD_MAX_RESET_COUNT )
		{
			goto reset_proc;
		}
#endif
		retval	= regulator_disable(tpd->reg); //disable regulator
		if(retval)
		{
			printk("focaltech tpd_probe regulator_disable() failed!\n");
		}

		regulator_put(tpd->reg);
		#ifdef CONFIG_MTK_I2C_EXTENSION
		msg_dma_release();
		#endif
		
		gpio_free(tpd_rst_gpio_number);
		gpio_free(tpd_int_gpio_number);
		return -1;
	}
	tpd_load_status = 1;
/*#ifdef CONFIG_CUST_FTS_APK_DEBUG
	//ft_rw_iic_drv_init(client);

	//ft5x0x_create_sysfs(client);

	ft5x0x_create_apk_debug_channel(client);
#endif
*/

	#ifdef SYSFS_DEBUG
                fts_create_sysfs(fts_i2c_client);
	#endif
	//hidi2c_to_stdi2c(fts_i2c_client);
	fts_get_upgrade_array();
	#ifdef FTS_CTL_IIC
		 if (fts_rw_iic_drv_init(fts_i2c_client) < 0)
			 dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n", __func__);
	#endif

	#ifdef FTS_APK_DEBUG
		fts_create_apk_debug_channel(fts_i2c_client);
	#endif


	#if 0
	/* Reset CTP */

	tpd_gpio_output(tpd_rst_gpio_number, 0);
	msleep(20);
	tpd_gpio_output(tpd_rst_gpio_number, 1);
	msleep(400);
	#endif
	#ifdef TPD_AUTO_UPGRADE
		printk("********************Enter CTP Auto Upgrade********************\n");
		is_update = true;
		fts_ctpm_auto_upgrade(fts_i2c_client);
		is_update = false;
	#endif

	#if 0
	/* Reset CTP */

	tpd_gpio_output(tpd_rst_gpio_number, 0);
	msleep(20);
	tpd_gpio_output(tpd_rst_gpio_number, 1);
	msleep(400);
	#endif
/*#ifdef CONFIG_FT_AUTO_UPGRADE_SUPPORT
	tpd_auto_upgrade(client);
#endif*/
	/* Set report rate 80Hz */
	report_rate = 0x8;
	if ((fts_write_reg(i2c_client, 0x88, report_rate)) < 0) {
		if ((fts_write_reg(i2c_client, 0x88, report_rate)) < 0)
			TPD_DMESG("I2C write report rate error, line: %d\n", __LINE__);
	}

	/* tpd_load_status = 1; */

	thread_tpd = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	if (IS_ERR(thread_tpd)) {
		retval = PTR_ERR(thread_tpd);
		TPD_DMESG(TPD_DEVICE " failed to create kernel thread_tpd: %d\n", retval);
	}

	TPD_DMESG("Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");

#ifdef TIMER_DEBUG
	init_test_timer();
#endif

	{
		u8 ver;

		fts_read_reg(client, 0xA6, &ver);

		TPD_DMESG(TPD_DEVICE " fts_read_reg version : 0x%x\n", ver);
	}

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	retval = get_md32_semaphore(SEMAPHORE_TOUCH);
	if (retval < 0)
		pr_err("[TOUCH] HW semaphore reqiure timeout\n");
#endif

	return 0;
}


#ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP

static ssize_t ft5x0x_proc_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{

    int value;
	char data_buf[50] = {0};

	if((NULL == filp) || (NULL == buff))
		return -1;

	if((len <= 0) || (len > 50))
		return -1;

	if(copy_from_user(data_buf, buff, len))
	{
		return -EFAULT;
	}

	value = simple_strtoul(data_buf, NULL, 16);
    if (value != 0 && value != 1)
	{
		tp_log("[FTS]: ft5x0x_proc_write, invalid value.\n");
		return -1;
	}
	else
	{
		double_tap_wakeup_enable = value;
	}
	tp_log("[FTS]: ft5x0x_proc_write has been writed to %d.\n", value);\
		
	return len;

}

static int ft5x0x_proc_show(struct seq_file *m, void *v)
{
	tp_log("ft5x0x_proc_show: double_tap_wakeup_enable=%d\n", double_tap_wakeup_enable);
	seq_printf(m, "%d\n", double_tap_wakeup_enable);
	return 0;
}


static int ft5x0x_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ft5x0x_proc_show, NULL);
}

static const struct file_operations ft5x0x_proc_fops = {
	.open = ft5x0x_proc_open,
	.write = ft5x0x_proc_write,
	.read = seq_read,
};

#endif


static int tpd_remove(struct i2c_client *client)
{
	TPD_DEBUG("TPD removed\n");
#ifdef CONFIG_CUST_FTS_APK_DEBUG
	//ft_rw_iic_drv_exit();
#endif

#ifdef CONFIG_FT_AUTO_UPGRADE_SUPPORT
	if (tpd_i2c_dma_va) {
		dma_free_coherent(NULL, 4096, tpd_i2c_dma_va, tpd_i2c_dma_pa);
		tpd_i2c_dma_va = NULL;
		tpd_i2c_dma_pa = 0;
	}
#endif

	#ifdef CONFIG_MTK_I2C_EXTENSION
		msg_dma_release();
	#endif
	
	gpio_free(tpd_rst_gpio_number);
	gpio_free(tpd_int_gpio_number);

	return 0;
}

static int tpd_local_init(void)
{
	int retval;

	TPD_DMESG("Focaltech FT5x0x I2C Touchscreen Driver...\n");
	tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
	retval = regulator_set_voltage(tpd->reg, 2800000, 2800000);
	if (retval != 0) {
		TPD_DMESG("Failed to set reg-vgp6 voltage: %d\n", retval);
		return -1;
	}
	if (i2c_add_driver(&tpd_i2c_driver) != 0) {
		TPD_DMESG("unable to add i2c driver.\n");
		return -1;
	}
     /* tpd_load_status = 1; */
	if (tpd_dts_data.use_tpd_button) {
		tpd_button_setting(tpd_dts_data.tpd_key_num, tpd_dts_data.tpd_key_local,
		tpd_dts_data.tpd_key_dim_local);
	}

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
	TPD_DO_WARP = 1;
	memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
	memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif

#if (defined(CONFIG_TPD_HAVE_CALIBRATION) && !defined(CONFIG_TPD_CUSTOM_CALIBRATION))

	memcpy(tpd_calmat, tpd_def_calmat_local_factory, 8 * 4);
	memcpy(tpd_def_calmat, tpd_def_calmat_local_factory, 8 * 4);

	memcpy(tpd_calmat, tpd_def_calmat_local_normal, 8 * 4);
	memcpy(tpd_def_calmat, tpd_def_calmat_local_normal, 8 * 4);

#endif

	TPD_DMESG("end %s, %d\n", __func__, __LINE__);
	tpd_type_cap = 1;

#if defined(CONFIG_TPD_DOUBLE_CLICK_WAKEUP)

       	ft5x0x_proc_entry = proc_create("gesture_open", 0666, NULL, &ft5x0x_proc_fops);
	    if (ft5x0x_proc_entry == NULL)
	    {
		   tp_log("create_proc_entry gesture_open failed !\n");
	    }

#endif

	return 0;

}

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
static s8 ftp_enter_doze(struct i2c_client *client)
{
	s8 ret = -1;
	s8 retry = 0;
	char gestrue_on = 0x01;
	char gestrue_data;
	int i;

	/* TPD_DEBUG("Entering doze mode..."); */
	pr_alert("Entering doze mode...");

	/* Enter gestrue recognition mode */
	ret = fts_write_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, gestrue_on);
	if (ret < 0) {
		/* TPD_DEBUG("Failed to enter Doze %d", retry); */
		pr_alert("Failed to enter Doze %d", retry);
		return ret;
	}
	msleep(30);

	for (i = 0; i < 10; i++) {
		fts_read_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, &gestrue_data);
		if (gestrue_data == 0x01) {
			doze_status = DOZE_ENABLED;
			/* TPD_DEBUG("FTP has been working in doze mode!"); */
			pr_alert("FTP has been working in doze mode!");
			break;
		}
		msleep(20);
		fts_write_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, gestrue_on);

	}

	return ret;
}
#endif

static void tpd_resume(struct device *h)
{
	int retval = TPD_OK;
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	int data;
#endif

tp_log("***TPD wake up, call tpd_resume()***\n");

#ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
	   gesture_switch = FTS_GESTURE_SWITCH_CLOSE;	
#endif

#ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
	   tpd_restart_flag = 1; //[head] wwj add
#endif

//tp restart
retval = regulator_enable(tpd->reg);
if (retval != 0)
	TPD_DMESG("Failed to enable reg-vgp6: %d\n", retval);

tpd_gpio_output(tpd_rst_gpio_number, 0);
msleep(30);
tpd_gpio_output(tpd_rst_gpio_number, 1);
msleep(300);

#ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
	     tpd_restart_flag = 0; //[end] wwj add
#endif


#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	if (tpd_scp_doze_en) {
		retval = get_md32_semaphore(SEMAPHORE_TOUCH);
		if (retval < 0) {
			TPD_DEBUG("[TOUCH] HW semaphore reqiure timeout\n");
		} else {
			struct Touch_IPI_Packet ipi_pkt = {.cmd = IPI_COMMAND_AS_ENABLE_GESTURE, .param.data = 0};

			md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
		}
	}
#endif

#ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
	   if(!double_tap_wakeup_enable)
	   	   enable_irq(touch_irq);
#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	doze_status = DOZE_DISABLED;
	data = 0x00;
	fts_write_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, data);
#else
	enable_irq(touch_irq);
#endif

}

static void tpd_suspend(struct device *h)
{
	int retval = TPD_OK;
#ifndef CONFIG_MTK_SENSOR_HUB_SUPPORT
	static char data = 0x3;
#endif
#ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
    u8 i;
    u8 state = 0;
    char gesture_open;
	char gesture_data;
	char sleep_data = 0x3;
#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	char gestrue_data;
	static int scp_init_flag;
	struct Touch_IPI_Packet ipi_pkt = {.cmd = IPI_COMMAND_AS_ENABLE_GESTURE, .param.data = 1};
#endif
	tp_log("***TPD enter sleep,tpd_suspend()***\n");

    #ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
		   gesture_open = FTS_GESTURE_SWITCH_OPEN;
		   gesture_data = 0x1f;
		   gesture_switch = FTS_GESTURE_SWITCH_OPEN;
           TPD_DMESG("tpd_suspend gesture_switch = %d \r\n", gesture_switch);

		   if (FTS_GESTURE_SWITCH_OPEN == gesture_switch){
               if(double_tap_wakeup_enable)
			   {
                   fts_write_reg(fts_i2c_client, FTS_GESTURE_SWITCH_REG, gesture_open);
			       fts_write_reg(fts_i2c_client, FTS_GESTURE_DOUBLECLICK_REG, gesture_data);
                   msleep(10);
			       for(i = 0; i < FTS_GESTURE_WRITE_REPEAT; i++)
			       {
			  	       fts_read_reg(i2c_client, FTS_GESTURE_SWITCH_REG, &state);

				       if(0x01 == state){
                            tp_log("total write times: %d",i+1);
						    TPD_DMESG("TPD gesture write 0x01\n");
	        			    return;
				       }else{
					       fts_write_reg(fts_i2c_client, FTS_GESTURE_SWITCH_REG, gesture_open);
			               fts_write_reg(fts_i2c_client, FTS_GESTURE_DOUBLECLICK_REG, gesture_data);
					       msleep(10);
			           }
		           }
			       if(i >= FTS_GESTURE_WRITE_REPEAT){
			           TPD_DMESG("TPD gesture write 0x01 to d0 fail \n");
			           return;
		           }
			   }
			   else
			   {
                   disable_irq(touch_irq);
	               fts_write_reg(i2c_client, 0xA5, sleep_data);  /* TP enter sleep mode */

	               retval = regulator_disable(tpd->reg);
	               if (retval != 0)
		              TPD_DMESG("Failed to disable reg-vgp6: %d\n", retval);
				   return;
			   } 
		   }
    #endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	tpd_enter_doze();

	/* TPD_DEBUG("[tpd_scp_doze]:init=%d en=%d", scp_init_flag, tpd_scp_doze_en); */
	mutex_lock(&i2c_access);

	retval = release_md32_semaphore(SEMAPHORE_TOUCH);

	if (scp_init_flag == 0) {
		struct Touch_IPI_Packet ipi_pkt;

		ipi_pkt.cmd = IPI_COMMAND_AS_CUST_PARAMETER;
		ipi_pkt.param.tcs.i2c_num = 0;/* shuold be modify according your hardware design*/
		ipi_pkt.param.tcs.int_num = get_hardware_irq(touch_irq);
		ipi_pkt.param.tcs.io_int = tpd_int_gpio_number;
		ipi_pkt.param.tcs.io_rst = tpd_rst_gpio_number;

		TPD_DEBUG("[TOUCH]SEND CUST command :%d ", IPI_COMMAND_AS_CUST_PARAMETER);

		retval = md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
		if (retval < 0)
			TPD_DEBUG(" IPI cmd failed (%d)\n", ipi_pkt.cmd);

		msleep(20); /* delay added between continuous command */
		/* Workaround if suffer MD32 reset */
		/* scp_init_flag = 1; */
	}

	if (tpd_scp_doze_en) {
		TPD_DEBUG("[TOUCH]SEND ENABLE GES command :%d ", IPI_COMMAND_AS_ENABLE_GESTURE);
		retval = ftp_enter_doze(i2c_client);
		if (retval < 0) {
			TPD_DEBUG("FTP Enter Doze mode failed\n");
	  	} else {
			int retry = 5;
			{
				/* check doze mode */
				fts_read_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, &gestrue_data);
				TPD_DEBUG("========================>0x%x", gestrue_data);
			}

			msleep(20);

			do {
				if (md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 1) == DONE)
			    		break;
			    	msleep(20);
			    	TPD_DEBUG("==>retry=%d", retry);
			} while (retry--);

			if (retry <= 0)
				TPD_DEBUG("############# md32_ipi_send failed retry=%d", retry);

			/*while(release_md32_semaphore(SEMAPHORE_TOUCH) <= 0) {
				//TPD_DEBUG("GTP release md32 sem failed\n");
				pr_alert("GTP release md32 sem failed\n");
			}*/

		}
		/* disable_irq(touch_irq); */
	}

	mutex_unlock(&i2c_access);
#else
	disable_irq(touch_irq);
	fts_write_reg(i2c_client, 0xA5, data);  /* TP enter sleep mode */

	retval = regulator_disable(tpd->reg);
	if (retval != 0)
		TPD_DMESG("Failed to disable reg-vgp6: %d\n", retval);

#endif

}

static struct tpd_driver_t tpd_device_driver = {
	.tpd_device_name = "FT5x0x",
	.tpd_local_init = tpd_local_init,
	.suspend = tpd_suspend,
	.resume = tpd_resume,
	.attrs = {
		.attr = ft5x0x_attrs,
		.num  = ARRAY_SIZE(ft5x0x_attrs),
	},
};

/* called when loaded into kernel */
static int __init tpd_driver_init(void)
{
	TPD_DMESG("MediaTek FT5x0x touch panel driver init\n");
	tpd_get_dts_info();
	if (tpd_driver_add(&tpd_device_driver) < 0)
		TPD_DMESG("add FT5x0x driver failed\n");

	return 0;
}

/* should never be called */
static void __exit tpd_driver_exit(void)
{
	TPD_DMESG("MediaTek FT5x0x touch panel driver exit\n");
	
	#ifdef CONFIG_TPD_DOUBLE_CLICK_WAKEUP
	    if (ft5x0x_proc_entry != NULL)
	    {
		   remove_proc_entry("gesture_open", NULL);
		   ft5x0x_proc_entry = NULL;
	    }
    #endif
	
	tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);




