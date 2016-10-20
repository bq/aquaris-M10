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

#include "tpd_custom_fts.h"
/* #include "ft5x06_ex_fun.h" */

#include "tpd.h"

#ifdef CONFIG_MTK_BOOT
#include "mt_boot_common.h"
#endif
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
#include "focaltech_ex_fun.h"



//#define MT_PROTOCOL_B
//#define TPD_PROXIMITY



//#define FTS_CTL_IIC
//#define SYSFS_DEBUG
//#define FTS_APK_DEBUG


#ifdef FTS_CTL_IIC
#include "focaltech_ctl.h"
#endif





#ifdef TPD_PROXIMITY
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#endif

#ifdef TPD_PROXIMITY

#define APS_ERR(fmt,arg...)           	TPD_DEBUG("<<proximity>> "fmt"\n",##arg)

#define TPD_PROXIMITY_DEBUG(fmt,arg...) TPD_DEBUG("<<proximity>> "fmt"\n",##arg)

#define TPD_PROXIMITY_DMESG(fmt,arg...) TPD_DEBUG("<<proximity>> "fmt"\n",##arg)

static u8 tpd_proximity_flag 			= 0;

static u8 tpd_proximity_flag_one 		= 0; //add for tpd_proximity by wangdongfang

static u8 tpd_proximity_detect 		= 1;//0-->close ; 1--> far away


#endif

#ifdef FTS_WAKE_MENU
#define HOTKNOT_MENU_PROC_FILE      "hotknot_menu" //linhui
static struct proc_dir_entry *hotknot_menu_proc = NULL;

#define STRINGLEN 64  

static char global_buffer[STRINGLEN]={0};

int gtp_wakeup_flag = 0; //linhui
#endif

#ifdef FTS_MODE_HALL
#define HALL_STATE_PROC_FILE        "hall_state" //linhui
static struct proc_dir_entry *hall_state_menu_proc = NULL;

#define STRINGLEN_SEN_HALL 64  
static char global_hall_buffer[STRINGLEN_SEN_HALL]={0};

int fts_is_in_hall_mode = 0;
#endif


#ifdef FTS_TP_NAME //linhui
u8 pt_sensor_id = 0x07;
#define IDENTIFY_TP "tp_name"
static struct proc_dir_entry *tp_name_proc = NULL;
static char tp_name_buffer[64]={0};

#endif

#ifdef FTS_GESTRUE
#define  KEY_GESTURE_U KEY_U
#define  KEY_GESTURE_UP KEY_UP
#define  KEY_GESTURE_DOWN KEY_DOWN
#define  KEY_GESTURE_LEFT KEY_LEFT 
#define  KEY_GESTURE_RIGHT KEY_RIGHT
#define  KEY_GESTURE_O KEY_O
#define  KEY_GESTURE_E KEY_E
#define  KEY_GESTURE_M KEY_M 
#define  KEY_GESTURE_C KEY_C   //ugrec_tky
#define  KEY_GESTURE_L KEY_L
#define  KEY_GESTURE_W KEY_W
#define  KEY_GESTURE_S KEY_S 
#define  KEY_GESTURE_V KEY_V
#define  KEY_GESTURE_Z KEY_Z


#define GESTURE_LEFT		0x20
#define GESTURE_RIGHT		0x21
#define GESTURE_UP		    0x22
#define GESTURE_DOWN		0x23
#define GESTURE_DOUBLECLICK	0x24
#define GESTURE_O		    0x30
#define GESTURE_W		    0x31
#define GESTURE_M		    0x32
#define GESTURE_E		    0x33
#define GESTURE_C		    0x34  //ugrec_tky
#define GESTURE_L		    0x44
#define GESTURE_S		    0x46
#define GESTURE_V		    0x54
#define GESTURE_Z		    0x41

#include "ft_gesture_lib.h"

#define FTS_GESTRUE_POINTS 255
#define FTS_GESTRUE_POINTS_ONETIME  62
#define FTS_GESTRUE_POINTS_HEADER 8
#define FTS_GESTURE_OUTPUT_ADRESS 0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH 4

unsigned short coordinate_x[150] = {0};
unsigned short coordinate_y[150] = {0};
#endif
 
extern struct tpd_device *tpd;
 
struct i2c_client *i2c_client = NULL;
struct task_struct *thread = NULL;

unsigned int touch_irq = 0;

struct Upgrade_Info fts_updateinfo[] =
{
    	{0x55,"FT5x06",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 10, 2000},
    	{0x08,"FT5606",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 10, 0x79, 0x06, 100, 2000},
	{0x0a,"FT5x16",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x07, 10, 1500},
	{0x06,"FT6x06",TPD_MAX_POINTS_2,AUTO_CLB_NONEED,100, 30, 0x79, 0x08, 10, 2000},
	{0x36,"FT6x36",TPD_MAX_POINTS_2,AUTO_CLB_NONEED,10, 10, 0x79, 0x18, 10, 2000},
	{0x55,"FT5x06i",TPD_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 10, 2000},
	{0x14,"FT5336",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
	{0x13,"FT3316",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
	{0x12,"FT5436i",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
	{0x11,"FT5336i",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000},
	{0x54,"FT5x46",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,10, 10, 0x54, 0x2c, 20, 2000},
	{0x58,"FT5822",TPD_MAX_POINTS_5,AUTO_CLB_NONEED,2, 2, 0x58, 0x2c, 20, 2000},//"FT5822" "FT5626"
};
				
struct Upgrade_Info fts_updateinfo_curr;
//#define FTS_RESET_PIN	GPIO_CTP_RST_PIN


static DECLARE_WAIT_QUEUE_HEAD(waiter);
static DEFINE_MUTEX(i2c_access);
 
 
static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id);

//extern void mt_eint_mask(unsigned int eint_num);
//extern void mt_eint_unmask(unsigned int eint_num);
//extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
//extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
//extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
//extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);

 
static int  tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect (struct i2c_client *client, struct i2c_board_info *info);
static int  tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);
 

static int tpd_flag = 0;
static int tpd_halt=0;
static int point_num = 0;
static int p_point_num = 0;


#ifdef CONFIG_MTK_I2C_EXTENSION
#define __MSG_DMA_MODE__
#else
//#define __MSG_DMA_MODE__
#endif

#ifdef __MSG_DMA_MODE__
	u8 *g_dma_buff_va = NULL;    //
	dma_addr_t g_dma_buff_pa = 0;    // 
	#define IIC_DMA_MAX_TRANSFER_SIZE     128//250	
#endif

#if 0
#define IIC_MAX_TRANSFER_SIZE         8
#define GTP_ADDR_LENGTH             2
#define I2C_MASTER_CLOCK              300

s32 _do_i2c_read(struct i2c_msg *msgs, u16 addr, u8 *buffer, s32 len)
{
	s32 ret = -1;
	s32 pos = 0;
	s32 data_length = len;
	s32 transfer_length = 0;
	u8 *data = NULL;
	u16 address = addr;

	data =
	    kmalloc(IIC_MAX_TRANSFER_SIZE <
			   (len + GTP_ADDR_LENGTH) ? IIC_MAX_TRANSFER_SIZE : (len + GTP_ADDR_LENGTH), GFP_KERNEL);
	if (data == NULL)
		return -3;
	msgs[1].buf = data;

	while (pos != data_length) {
		if ((data_length - pos) > IIC_MAX_TRANSFER_SIZE)
			transfer_length = IIC_MAX_TRANSFER_SIZE;
		else
			transfer_length = data_length - pos;
		msgs[0].buf[0] = (address >> 8) & 0xFF;
		msgs[0].buf[1] = address & 0xFF;
		msgs[1].len = transfer_length;

		ret = i2c_transfer(i2c_client->adapter, msgs, 2);
		if (ret != 2) {
			TPD_DEBUG("I2c Transfer error! (%d)\n", ret);
			kfree(data);
			return -2;
		}
		memcpy(&buffer[pos], msgs[1].buf, transfer_length);
		pos += transfer_length;
		address += transfer_length;
	}

	kfree(data);
	return 0;
}


static s32 i2c_read_mtk(u16 addr, u8 *buffer, s32 len)
{
	int ret;
	u8 addr_buf[GTP_ADDR_LENGTH] = { (addr >> 8) & 0xFF, addr & 0xFF };

	struct i2c_msg msgs[2] = {
		{
#ifdef CONFIG_MTK_I2C_EXTENSION

		 .addr = ((i2c_client->addr & I2C_MASK_FLAG) | (I2C_ENEXT_FLAG)),
		 .timing = I2C_MASTER_CLOCK,
#else

		 .addr = i2c_client->addr,
#endif
		 .flags = 0,
		 .buf = addr_buf,
		 .len = GTP_ADDR_LENGTH,
		},
		{
#ifdef CONFIG_MTK_I2C_EXTENSION
		 .addr = ((i2c_client->addr & I2C_MASK_FLAG) | (I2C_ENEXT_FLAG)),
		 .timing = I2C_MASTER_CLOCK,
#else
		 .addr = i2c_client->addr,
#endif
		 .flags = I2C_M_RD,
		},
	};

	ret = _do_i2c_read(msgs, addr, buffer, len);
	return ret;
}


#endif


#ifdef __MSG_DMA_MODE__
static void msg_dma_alloct(void){
	tpd->dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	g_dma_buff_va = (u8 *)dma_alloc_coherent(&tpd->dev->dev, IIC_DMA_MAX_TRANSFER_SIZE, &g_dma_buff_pa, GFP_KERNEL);  //|GFP_DMA32

    if(!g_dma_buff_va){
        TPD_DMESG("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
	return ;
    }
	memset(g_dma_buff_va, 0, IIC_DMA_MAX_TRANSFER_SIZE);


}
static void msg_dma_release(void){
	if(g_dma_buff_va){
     	dma_free_coherent(&tpd->dev->dev, IIC_DMA_MAX_TRANSFER_SIZE, g_dma_buff_va, g_dma_buff_pa);
        g_dma_buff_va = NULL;
        g_dma_buff_pa = 0;
		TPD_DMESG("[DMA][release] Allocate DMA I2C Buffer release!\n");
    }
}
#endif


#define TPD_OK 0
//register define
#if 0
#define DEVICE_MODE 0x00
#define GEST_ID 0x01
#define TD_STATUS 0x02

#define TOUCH1_XH 0x03
#define TOUCH1_XL 0x04
#define TOUCH1_YH 0x05
#define TOUCH1_YL 0x06

#define TOUCH2_XH 0x09
#define TOUCH2_XL 0x0A
#define TOUCH2_YH 0x0B
#define TOUCH2_YL 0x0C

#define TOUCH3_XH 0x0F
#define TOUCH3_XL 0x10
#define TOUCH3_YH 0x11
#define TOUCH3_YL 0x12
//register define

#define TPD_RESET_ISSUE_WORKAROUND


#define TPD_MAX_RESET_COUNT 3
#endif

struct ts_event {
	u16 au16_x[CFG_MAX_TOUCH_POINTS];	/*x coordinate */
	u16 au16_y[CFG_MAX_TOUCH_POINTS];	/*y coordinate */
	u8 au8_touch_event[CFG_MAX_TOUCH_POINTS];	/*touch event:
					0 -- down; 1-- up; 2 -- contact */
	u8 au8_finger_id[CFG_MAX_TOUCH_POINTS];	/*touch ID */
	u16 pressure;
	u8 touch_point;
};


#ifdef TPD_HAVE_BUTTON 
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT]   = TPD_WARP_END;
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif

//#define VELOCITY_CUSTOM_fts
#ifdef VELOCITY_CUSTOM_fts
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

// for magnify velocity********************************************

#ifndef TPD_VELOCITY_CUSTOM_X
#define TPD_VELOCITY_CUSTOM_X 10
#endif
#ifndef TPD_VELOCITY_CUSTOM_Y
#define TPD_VELOCITY_CUSTOM_Y 10
#endif

#define TOUCH_IOC_MAGIC 'A'

#define TPD_GET_VELOCITY_CUSTOM_X _IO(TOUCH_IOC_MAGIC,0)
#define TPD_GET_VELOCITY_CUSTOM_Y _IO(TOUCH_IOC_MAGIC,1)

int g_v_magnify_x =TPD_VELOCITY_CUSTOM_X;
int g_v_magnify_y =TPD_VELOCITY_CUSTOM_Y;
static int tpd_misc_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int tpd_misc_release(struct inode *inode, struct file *file)
{
	return 0;
}
/*----------------------------------------------------------------------------*/

static long tpd_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{

	void __user *data;
	
	long err = 0;
	
	if(_IOC_DIR(cmd) & _IOC_READ)
	{
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	}
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
	{
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if(err)
	{
		TPD_DEBUG("tpd: access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch(cmd)
	{
		case TPD_GET_VELOCITY_CUSTOM_X:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}			
			
			if(copy_to_user(data, &g_v_magnify_x, sizeof(g_v_magnify_x)))
			{
				err = -EFAULT;
				break;
			}				 
			break;

	   case TPD_GET_VELOCITY_CUSTOM_Y:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}			
			
			if(copy_to_user(data, &g_v_magnify_y, sizeof(g_v_magnify_y)))
			{
				err = -EFAULT;
				break;
			}				 
			break;


		default:
			TPD_DEBUG("tpd: unknown IOCTL: 0x%08x\n", cmd);
			err = -ENOIOCTLCMD;
			break;
			
	}

	return err;
}


static struct file_operations tpd_fops = {
//	.owner = THIS_MODULE,
	.open = tpd_misc_open,
	.release = tpd_misc_release,
	.unlocked_ioctl = tpd_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice tpd_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "touch",
	.fops = &tpd_fops,
};

//**********************************************
#endif

struct touch_info {
    int y[10];
    int x[10];
    int p[10];
    int id[10];
    int count;
};
 
 static const struct i2c_device_id fts_tpd_id[] = {{"fts6x36",0},{}};

//static struct i2c_board_info __initdata fts_i2c_tpd={ I2C_BOARD_INFO("fts", (0x70>>1))};
#ifdef CONFIG_OF  //ugrec_tky
static const struct of_device_id ft6x36_dt_match[] = {
	{.compatible = "mediatek,cap_touch",},
	{},
};
#endif 
 static struct i2c_driver tpd_i2c_driver = {
  .driver = {
 	 .name = "fts6x36",
#ifdef CONFIG_OF	 	
	.of_match_table = ft6x36_dt_match,
#endif	
  },
  .probe = tpd_probe,
  .remove = tpd_remove,
  .id_table = fts_tpd_id,
  .detect = tpd_detect,

 };
 
#ifdef __MSG_DMA_MODE__
int fts_i2c_Read(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen)
{
	int ret=0;

	// for DMA I2c transfer

	if(writelen!=0)
	{
		//DMA Write
		memcpy(g_dma_buff_va, writebuf, writelen);
		
		client->addr = (client->addr & I2C_MASK_FLAG) | (I2C_DMA_FLAG);
		if((ret=i2c_master_send(client, (unsigned char *)g_dma_buff_pa, writelen))!=writelen)
			dev_err(&client->dev, "## i2c write len=,buffaddr=\n");

		client->addr = (client->addr & I2C_MASK_FLAG) &(~ I2C_DMA_FLAG);
	}

	//DMA Read 

	if(readlen!=0)
	{
		client->addr = (client->addr & I2C_MASK_FLAG) | (I2C_DMA_FLAG);

		ret = i2c_master_recv(client, (unsigned char *)g_dma_buff_pa, readlen);

		memcpy(readbuf, g_dma_buff_va, readlen);

		client->addr = (client->addr & I2C_MASK_FLAG) &(~ I2C_DMA_FLAG);
	}
	return ret;

}


/*write data by i2c*/
int fts_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret=0;

 	//client->addr = client->addr & I2C_MASK_FLAG;

	//ret = i2c_master_send(client, writebuf, writelen);
	memcpy(g_dma_buff_va, writebuf, writelen);
	
	client->addr = (client->addr & I2C_MASK_FLAG) | (I2C_DMA_FLAG);
	if((ret=i2c_master_send(client, (unsigned char *)g_dma_buff_pa, writelen))!=writelen)
		dev_err(&client->dev, "###%s i2c write len,\n", __func__);

	client->addr = (client->addr & I2C_MASK_FLAG) &(~ I2C_DMA_FLAG);
		
	return ret;
}
#else
int fts_i2c_Read(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen)
{
	int ret=0;

	if(writelen!=0)
	{
#ifdef CONFIG_MTK_I2C_EXTENSION
		client->addr = client->addr & I2C_MASK_FLAG;
#else
		client->addr = client->addr;
#endif
		ret = i2c_master_send(client, writebuf, writelen);
		if(ret  < 0)
			dev_err(&client->dev, "## i2c write error\n");
	}

	if(readlen!=0)
	{
#ifdef CONFIG_MTK_I2C_EXTENSION
		client->addr = client->addr & I2C_MASK_FLAG;
#else
		client->addr = client->addr;
#endif
		ret = i2c_master_recv(client, readbuf, readlen);
		if(ret  < 0)
			dev_err(&client->dev, "## i2c write error\n");
	}
	return ret;

}


/*write data by i2c*/
int fts_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret=0;

#ifdef CONFIG_MTK_I2C_EXTENSION
 	client->addr = client->addr & I2C_MASK_FLAG;
#else
	client->addr = client->addr;
#endif
	ret = i2c_master_send(client, writebuf, writelen);
	if(ret  < 0)
		dev_err(&client->dev, "## i2c write error\n");

	return ret;
}

#endif
int fts_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue)
{
	unsigned char buf[2] = {0};

	buf[0] = regaddr;
	buf[1] = regvalue;

	return fts_i2c_Write(client, buf, sizeof(buf));
}

int fts_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue)
{

	return fts_i2c_Read(client, &regaddr, 1, regvalue, 1);

}

void focaltech_get_upgrade_array(void)
{

	u8 chip_id;
	u32 i;

	if((i2c_smbus_read_i2c_block_data(i2c_client,FTS_REG_CHIP_ID,1,&chip_id)<0))
	{
		TPD_DMESG("I2C transfer error, line: %d\n", __LINE__);
	}

	TPD_DEBUG("%s chip_id = 0x%x\n", __func__, chip_id);

	for(i=0;i<sizeof(fts_updateinfo)/sizeof(struct Upgrade_Info);i++)
	{
		if(chip_id==fts_updateinfo[i].CHIP_ID)
		{
			memcpy(&fts_updateinfo_curr, &fts_updateinfo[i], sizeof(struct Upgrade_Info));
			break;
		}
	}

	if(i >= sizeof(fts_updateinfo)/sizeof(struct Upgrade_Info))
	{
		memcpy(&fts_updateinfo_curr, &fts_updateinfo[0], sizeof(struct Upgrade_Info));
	}
}

static  void tpd_down(int x, int y, int p) {
	
	if(x > TPD_RES_X)
	{
		TPD_DEBUG("warning: IC have sampled wrong value.\n");;
		return;
	}
   input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, p); 	
	 input_report_key(tpd->dev, BTN_TOUCH, 1);
	 input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 20);
	 input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0x3f);
	 input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
	 input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
	 TPD_DEBUG("tpd:D[%4d %4d %4d] \n", x, y, p);
	 /* track id Start 0 */
   input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, p); 
	 input_mt_sync(tpd->dev);
#ifdef  CONFIG_MTK_BOOT 
     if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
     {   
       tpd_button(x, y, 1);  
     }
#endif    
	 if(y > TPD_RES_Y) //virtual key debounce to avoid android ANR issue
	 {
         //msleep(50);
		 TPD_DEBUG("D virtual key \n");
	 }
	 TPD_EM_PRINT(x, y, x, y, p-1, 1);
 }
 
static  void tpd_up(int x, int y,int *count)
{
	 input_report_key(tpd->dev, BTN_TOUCH, 0);
	 //TPD_DEBUG("U[%4d %4d %4d] ", x, y, 0);
	 input_mt_sync(tpd->dev);
	 TPD_EM_PRINT(x, y, x, y, 0, 0);
#ifdef  CONFIG_MTK_BOOT
	if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
	{   
		tpd_button(x, y, 0); 
	}  
#endif	
 }

static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
{
	int i = 0;
	char data[128] = {0};
       u16 high_byte,low_byte;
//	u8 report_rate =0;
	char  reg[1];
	p_point_num = point_num;
	if (tpd_halt)
	{
		TPD_DMESG( "tpd_touchinfo return ..\n");
		return false;
	}
	mutex_lock(&i2c_access);

       reg[0] = 0x00;
	fts_i2c_Read(i2c_client, reg, 1, data, 64);
//	i2c_read_mtk(0x00, data, 64);
	mutex_unlock(&i2c_access);
	
	/*get the number of the touch points*/

	point_num= data[2] & 0x0f;
	
	for(i = 0; i < point_num; i++)  
	{
		cinfo->p[i] = data[3+6*i] >> 6; //event flag 
     		cinfo->id[i] = data[3+6*i+2]>>4; //touch id
	   	/*get the X coordinate, 2 bytes*/
		high_byte = data[3+6*i];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data[3+6*i + 1];
		cinfo->x[i] = high_byte |low_byte;	
		high_byte = data[3+6*i+2];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data[3+6*i+3];
		cinfo->y[i] = high_byte |low_byte;
	}

	TPD_DEBUG(" tpd cinfo->x[0] = %d, cinfo->y[0] = %d, cinfo->p[0] = %d\n", cinfo->x[0], cinfo->y[0], cinfo->p[0]);
	return true;

};
#ifdef MT_PROTOCOL_B
/*
*report the point information
*/
static int fts_read_Touchdata(struct ts_event *pinfo)
{
       u8 buf[POINT_READ_BUF] = { 0 };
	int ret = -1;
	int i = 0;
	u8 pointid = FT_MAX_ID;

	if (tpd_halt)
	{
		TPD_DMESG( "tpd_touchinfo return ..\n");
		return false;
	}

	mutex_lock(&i2c_access);
	ret = fts_i2c_Read(i2c_client, buf, 1, buf, POINT_READ_BUF);
	if (ret < 0) {
		dev_err(&i2c_client->dev, "%s read touchdata failed.\n",__func__);
		mutex_unlock(&i2c_access);
		return ret;
	}
	mutex_unlock(&i2c_access);
	memset(pinfo, 0, sizeof(struct ts_event));
	
	pinfo->touch_point = 0;
	TPD_DEBUG("tpd  fts_updateinfo_curr.TPD_MAX_POINTS=%d fts_updateinfo_curr.chihID=%d \n", fts_updateinfo_curr.TPD_MAX_POINTS,fts_updateinfo_curr.CHIP_ID);
	for (i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++)
	{
		pointid = (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;
		if (pointid >= FT_MAX_ID)
			break;
		else
			pinfo->touch_point++;
		pinfo->au16_x[i] =
		    (s16) (buf[FT_TOUCH_X_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
		    8 | (s16) buf[FT_TOUCH_X_L_POS + FT_TOUCH_STEP * i];
		pinfo->au16_y[i] =
		    (s16) (buf[FT_TOUCH_Y_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
		    8 | (s16) buf[FT_TOUCH_Y_L_POS + FT_TOUCH_STEP * i];
		pinfo->au8_touch_event[i] =
		    buf[FT_TOUCH_EVENT_POS + FT_TOUCH_STEP * i] >> 6;
		pinfo->au8_finger_id[i] =
		    (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;
	}
	
	return 0;
}
 /*
 *report the point information
 */
static void fts_report_value(struct ts_event *data)
 {
	 struct ts_event *event = data;
	 int i = 0;
	 int up_point = 0;
 
	 for (i = 0; i < event->touch_point; i++) 
	 {
		 input_mt_slot(tpd->dev, event->au8_finger_id[i]);
 
		 if (event->au8_touch_event[i]== 0 || event->au8_touch_event[i] == 2)
			 {
				 input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER,true);
				 input_report_abs(tpd->dev, ABS_MT_PRESSURE,0x3f);
				 input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR,0x05);
				 input_report_abs(tpd->dev, ABS_MT_POSITION_X,event->au16_x[i]);
				 input_report_abs(tpd->dev, ABS_MT_POSITION_Y,event->au16_y[i]);
              TPD_DEBUG("tpd D x[%d] =%d,y[%d]= %d",i,event->au16_x[i],i,event->au16_y[i]);
			 }
			 else
			 {
				 up_point++;
				 input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER,false);
			 }				 
		 
	 }
 
	 if(event->touch_point == up_point)
		 input_report_key(tpd->dev, BTN_TOUCH, 0);
	 else
		 input_report_key(tpd->dev, BTN_TOUCH, 1);
 
	 input_sync(tpd->dev);
    TPD_DEBUG("tpd D x =%d,y= %d",event->au16_x[0],event->au16_y[0]);
 }
#endif

#ifdef TPD_PROXIMITY
int tpd_read_ps(void)
{
	tpd_proximity_detect;
	return 0;    
}

static int tpd_get_ps_value(void)
{
	return tpd_proximity_detect;
}

static int tpd_enable_ps(int enable)
{
	u8 state;
	int ret = -1;
	
	i2c_smbus_read_i2c_block_data(i2c_client, 0xB0, 1, &state);
	TPD_DEBUG("[proxi_fts]read: 999 0xb0's value is 0x%02X\n", state);

	if (enable){
		state |= 0x01;
		tpd_proximity_flag = 1;
		TPD_PROXIMITY_DEBUG("[proxi_fts]ps function is on\n");	
	}else{
		state &= 0x00;	
		tpd_proximity_flag = 0;
		TPD_PROXIMITY_DEBUG("[proxi_fts]ps function is off\n");
	}
	
	ret = i2c_smbus_write_i2c_block_data(i2c_client, 0xB0, 1, &state);
	TPD_PROXIMITY_DEBUG("[proxi_fts]write: 0xB0's value is 0x%02X\n", state);
	return 0;
}

int tpd_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,

		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data *sensor_data;
	TPD_DEBUG("[proxi_fts]command = 0x%02X\n", command);		
	
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
					if((tpd_enable_ps(1) != 0))
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
				}
				else
				{
					if((tpd_enable_ps(0) != 0))
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
				}
			}
			break;
		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (hwm_sensor_data *)buff_out;				
				if((err = tpd_read_ps()))
				{
					err = -1;;
				}
				else
				{
					sensor_data->values[0] = tpd_get_ps_value();
					TPD_PROXIMITY_DEBUG("huang sensor_data->values[0] 1082 = %d\n", sensor_data->values[0]);
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
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
#endif

#ifdef FTS_GESTRUE
static void check_gesture(int gesture_id)
{
    TPD_DEBUG("fts gesture_id==0x%x\n ",gesture_id);
	switch(gesture_id)
	{
		case GESTURE_LEFT:
				input_report_key(tpd->dev, KEY_GESTURE_LEFT, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_LEFT, 0);
				input_sync(tpd->dev);
			break;
		case GESTURE_RIGHT:
				input_report_key(tpd->dev, KEY_GESTURE_RIGHT, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_RIGHT, 0);
				input_sync(tpd->dev);
			break;
		case GESTURE_UP:
				input_report_key(tpd->dev, KEY_GESTURE_UP, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_UP, 0);
				input_sync(tpd->dev);			
			break;
		case GESTURE_DOWN:
				input_report_key(tpd->dev, KEY_GESTURE_DOWN, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_DOWN, 0);
				input_sync(tpd->dev);
			break;
		case GESTURE_DOUBLECLICK:
				input_report_key(tpd->dev, KEY_GESTURE_U, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_U, 0);
				input_sync(tpd->dev);
			break;
		case GESTURE_O:
				input_report_key(tpd->dev, KEY_GESTURE_O, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_O, 0);
				input_sync(tpd->dev);
			break;
		case GESTURE_W:
				input_report_key(tpd->dev, KEY_GESTURE_W, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_W, 0);
				input_sync(tpd->dev);
			break;
		case GESTURE_M:
				input_report_key(tpd->dev, KEY_GESTURE_M, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_M, 0);
				input_sync(tpd->dev);
			break;
		case GESTURE_E:
				input_report_key(tpd->dev, KEY_GESTURE_E, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_E, 0);
				input_sync(tpd->dev);
			break;
		case GESTURE_C: //ugrec_tky 
				input_report_key(tpd->dev, KEY_GESTURE_C, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_C, 0);
				input_sync(tpd->dev);
			break;			
		case GESTURE_L:
				input_report_key(tpd->dev, KEY_GESTURE_L, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_L, 0);
				input_sync(tpd->dev);
			break;
		case GESTURE_S:
				input_report_key(tpd->dev, KEY_GESTURE_S, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_S, 0);
				input_sync(tpd->dev);
			break;
		case GESTURE_V:
				input_report_key(tpd->dev, KEY_GESTURE_V, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_V, 0);
				input_sync(tpd->dev);
			break;
		case GESTURE_Z:
				input_report_key(tpd->dev, KEY_GESTURE_Z, 1);
				input_sync(tpd->dev);
				input_report_key(tpd->dev, KEY_GESTURE_Z, 0);
				input_sync(tpd->dev);
			break;
		default:
			break;
	}
}

static int fts_read_Gestruedata(void)
{
    unsigned char buf[FTS_GESTRUE_POINTS * 3] = { 0 };
    int ret = -1;
    int i = 0;
    int gestrue_id = 0;
    short pointnum = 0;
    buf[0] = 0xd3;
  

    pointnum = 0;
    ret = fts_i2c_Read(i2c_client, buf, 1, buf, FTS_GESTRUE_POINTS_HEADER);
	TPD_DEBUG( "tpd read FTS_GESTRUE_POINTS_HEADER.\n");
    if (ret < 0)
    {
        TPD_DEBUG( "%s read touchdata failed.\n", __func__);
        return ret;
    }

    /* FW */
     if (fts_updateinfo_curr.CHIP_ID==0x54)
     {
     		 gestrue_id = buf[0];
		 pointnum = (short)(buf[1]) & 0xff;
	 	 buf[0] = 0xd3;
	 
	 	 if((pointnum * 4 + 8)<255)
	 	 {
	 	    	 ret = fts_i2c_Read(i2c_client, buf, 1, buf, (pointnum * 4 + 8));
	 	 }
	 	 else
	 	 {
	 	        ret = fts_i2c_Read(i2c_client, buf, 1, buf, 255);
	 	        ret = fts_i2c_Read(i2c_client, buf, 0, buf+255, (pointnum * 4 + 8) -255);
	 	 }
	 	 if (ret < 0)
	 	 {
	 	       TPD_DEBUG( "%s read touchdata failed.\n", __func__);
	 	       return ret;
	 	 }
        	 check_gesture(gestrue_id);
		 for(i = 0;i < pointnum;i++)
	        {
	        	coordinate_x[i] =  (((s16) buf[0 + (4 * i)]) & 0x0F) <<
	            	8 | (((s16) buf[1 + (4 * i)])& 0xFF);
	        	coordinate_y[i] = (((s16) buf[2 + (4 * i)]) & 0x0F) <<
	            	8 | (((s16) buf[3 + (4 * i)]) & 0xFF);
	   	 }
        	 return -1;
     }

    if (0x24 == buf[0])

    {
        gestrue_id = 0x24;
        check_gesture(gestrue_id);
		TPD_DEBUG( "tpd %d check_gesture gestrue_id.\n", gestrue_id);
        return -1;
    }
	
    pointnum = (short)(buf[1]) & 0xff;
    buf[0] = 0xd3;
    if((pointnum * 4 + 8)<255)
    {
    	ret = fts_i2c_Read(i2c_client, buf, 1, buf, (pointnum * 4 + 8));
    }
    else
    {
         ret = fts_i2c_Read(i2c_client, buf, 1, buf, 255);
         ret = fts_i2c_Read(i2c_client, buf, 0, buf+255, (pointnum * 4 + 8) -255);
    }
    if (ret < 0)
    {
        TPD_DEBUG( "%s read touchdata failed.\n", __func__);
        return ret;
    }

   gestrue_id = fetch_object_sample(buf, pointnum);
   check_gesture(gestrue_id);
   TPD_DEBUG( "tpd %d read gestrue_id.\n", gestrue_id);

    for(i = 0;i < pointnum;i++)
    {
        coordinate_x[i] =  (((s16) buf[0 + (4 * i)]) & 0x0F) <<
            8 | (((s16) buf[1 + (4 * i)])& 0xFF);
        coordinate_y[i] = (((s16) buf[2 + (4 * i)]) & 0x0F) <<
            8 | (((s16) buf[3 + (4 * i)]) & 0xFF);
    }
    return -1;
}
#endif

 static int touch_event_handler(void *unused)
 {
	struct touch_info cinfo, pinfo;

	int i=0;
#ifdef MT_PROTOCOL_B	
	int ret = 0;
	struct ts_event pevent;
#endif
 #if 1//def TPD_PROXIMITY	
	u8 state;
#endif
//	 struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
//	 sched_setscheduler(current, SCHED_RR, &param);
	 struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
	sched_setscheduler(current, SCHED_RR, &param);
		
 
	#ifdef TPD_PROXIMITY
		int err;
		hwm_sensor_data sensor_data;
		u8 proximity_status;
	#endif

	 do
	 {
//		 mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM); 
//		 enable_irq(touch_irq);
		 set_current_state(TASK_INTERRUPTIBLE); 
		 wait_event_interruptible(waiter,tpd_flag!=0);
						 
		 tpd_flag = 0;
			 
		 set_current_state(TASK_RUNNING);
		 //TPD_DEBUG("tpd touch_event_handler\n");
	 	 #ifdef FTS_GESTRUE
			if(gtp_wakeup_flag==1) //ugrec_tky
			{
				i2c_smbus_read_i2c_block_data(i2c_client, 0xd0, 1, &state);
				TPD_DEBUG("tpd fts_read_Gestruedata state=%d\n",state);
				     if(state ==1)
				     {
				        fts_read_Gestruedata();
				        continue;
				    }
			    }
		 #endif

		 #ifdef TPD_PROXIMITY

			 if (tpd_proximity_flag == 1)
			 {

				i2c_smbus_read_i2c_block_data(i2c_client, 0xB0, 1, &state);
	            		TPD_PROXIMITY_DEBUG("proxi_fts 0xB0 state value is 1131 0x%02X\n", state);
				if(!(state&0x01))
				{
					tpd_enable_ps(1);
				}
				i2c_smbus_read_i2c_block_data(i2c_client, 0x01, 1, &proximity_status);
	            		TPD_PROXIMITY_DEBUG("proxi_fts 0x01 value is 1139 0x%02X\n", proximity_status);
				if (proximity_status == 0xC0)
				{
					tpd_proximity_detect = 0;	
				}
				else if(proximity_status == 0xE0)
				{
					tpd_proximity_detect = 1;
				}

				TPD_PROXIMITY_DEBUG("tpd_proximity_detect 1149 = %d\n", tpd_proximity_detect);
				if ((err = tpd_read_ps()))
				{
					TPD_PROXIMITY_DMESG("proxi_fts read ps data 1156: %d\n", err);	
				}
				sensor_data.values[0] = tpd_get_ps_value();
				sensor_data.value_divide = 1;
				sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
				//if ((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
				//{
				//	TPD_PROXIMITY_DMESG(" proxi_5206 call hwmsen_get_interrupt_data failed= %d\n", err);	
				//}
			}  

		#endif
                                
		#ifdef MT_PROTOCOL_B
		{
            		ret = fts_read_Touchdata(&pevent);
			if (ret == 0)
				fts_report_value(&pevent);
		}
		#else
		{
			if (tpd_touchinfo(&cinfo, &pinfo)) 
			{
			    	TPD_DEBUG("tpd point_num = %d\n",point_num);
				TPD_DEBUG_SET_TIME;
				if(point_num >0) 
				{
				    for(i =0; i<point_num; i++)//only support 3 point
				    {
				         tpd_down(cinfo.x[i], cinfo.y[i], cinfo.id[i]);
				    }
				    input_sync(tpd->dev);
				}
				else  
		    		{
		              	tpd_up(cinfo.x[0], cinfo.y[0],&cinfo.id[0]);
		        	    //TPD_DEBUG("release --->\n");         	   
		        	    input_sync(tpd->dev);
		        	}
        		}
		}
		#endif
 }while(!kthread_should_stop());
	 return 0;
 }
 

 static int tpd_detect (struct i2c_client *client, struct i2c_board_info *info) 
 {
	 strcpy(info->type, TPD_DEVICE);	
	  return 0;
 }
 
 static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id)
 {
	 TPD_DEBUG("TPD interrupt has been triggered\n");
//	 TPD_DEBUG_PRINT_INT;
	 tpd_flag = 1;
	 wake_up_interruptible(&waiter);
	return IRQ_HANDLED;
 }

/*
 static int fts_init_gpio_hw(void)
{

	int ret = 0;
	int i = 0;
#if 0
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
#else
	tpd_gpio_output(0, 1); 
#endif
	return ret;
}*/

 



#ifdef FTS_WAKE_MENU
static ssize_t gt91xx_hotknot_menu_write_proc(struct file *file, const char *buffer, size_t count, loff_t *ppos)
{

   // char temp[25] = {0}; // for store special format cmd
    char mode_str[15] = {0};
    unsigned int mode;
		
    unsigned long lens = sizeof(global_buffer);	
	if(count>=lens)
	{
		count = lens -1;
	}

	if (copy_from_user(global_buffer, buffer, count))
	{
		return -EFAULT;
	}

	global_buffer[count]='\0';
	
    sscanf(global_buffer, "%s %d", (char *)&mode_str, &mode);
    
    /**********************************************/
    if((strcmp(mode_str, "0") == 0))
    {
           gtp_wakeup_flag = 0;
        return count;              	
    }

	if(strcmp(mode_str, "1") == 0)
    {
            gtp_wakeup_flag = 1;
        return count;              	
    }
	return count;
   // return -EFAULT;
}



static ssize_t gt91xx_hotknot_menu_read_proc(struct file *file, char *buffer, size_t count, loff_t *ppos)
{
	char *page = NULL;
    char *ptr = NULL;
    int err = -1;
    size_t len = 0;

	page = kmalloc(FT_PAGE_SIZE, GFP_KERNEL);	
	if (!page) 
	{		
		kfree(page);		
		return -ENOMEM;	
	}
    ptr = page; 
    //ptr += sprintf(ptr, "==== GT9XX hotknot_menu====\n");

    ptr += sprintf(ptr,"%s",global_buffer);
   // ptr += sprintf(ptr, "\n");

	len = ptr - page; 			 	
	  if(*ppos >= len)
	  {		
		  kfree(page); 		
		  return 0; 	
	  }	
	  err = copy_to_user(buffer,(char *)page,len); 			
	  *ppos += len; 	
	  if(err) 
	  {		
	    kfree(page); 		
		  return err; 	
	  }	
	  kfree(page); 	
	  return len;	

}


static const struct file_operations gt_hotknot_menu_proc_fops = { 
    .write = gt91xx_hotknot_menu_write_proc,
    .read = gt91xx_hotknot_menu_read_proc
};
#endif


#ifdef FTS_MODE_HALL
     
static ssize_t hall_state_menu_write_proc(struct file *file, const char *buffer, size_t count, loff_t *ppos)
{
	  u8 flagc1 = 0;
		u8 i = 0;
   // char temp[25] = {0}; // for store special format cmd
    char mode_str[15] = {0};
    unsigned int mode;
		
    unsigned long lens = sizeof(global_hall_buffer);	
	if(count>=lens)
	{
		count = lens -1;
	}

	if (copy_from_user(global_hall_buffer, buffer, count))
	{
		return -EFAULT;
	}

	global_hall_buffer[count]='\0';
	
    sscanf(global_hall_buffer, "%s %d", (char *)&mode_str, &mode);
    
    /**********************************************/
    if((strcmp(mode_str, "0") == 0))
    {

		fts_is_in_hall_mode = 0;

		TPD_DEBUG("HALL IS OPEN aaaaaaaaaaaaa\n");
		fts_write_reg(i2c_client, 0xc1, 0x00);  //ugrec_tky
		msleep(50);
		i2c_smbus_read_i2c_block_data(i2c_client,0xc1,1,&flagc1);
		TPD_DEBUG("tpd_resume Reg 0xc1 = %d\n", flagc1);
		if(flagc1 != 0)
		{
			for(i = 5; i >0; i--)
			{
				msleep(100);
				fts_write_reg(i2c_client, 0xc1, 0x00); //ugrec_tky
				msleep(20);
				i2c_smbus_read_i2c_block_data(i2c_client,0xc1,1,&flagc1);
				TPD_DEBUG("tpd_resume Reg 0xc1 = %d\n", flagc1);
				if(flagc1 == 0)
					break;
			}
		}
		
		TPD_DEBUG("HALL IS OPEN bbbbbbbbbbbbb\n");
		
       	 return count;              	
    }

    if(strcmp(mode_str, "1") == 0)
    {
	
		fts_is_in_hall_mode = 1;
		TPD_DEBUG("HALL IS CLOSE aaaaaaaaaaaaa\n");

		/********  randy add for Hall run out ************/
	
		fts_write_reg(i2c_client, 0xc1, 0x01); //ugrec_tky
		msleep(50);
		i2c_smbus_read_i2c_block_data(i2c_client,0xc1,1,&flagc1);
		TPD_DEBUG("tpd_resume Reg 0xc1 = %d\n", flagc1);
		if(flagc1 != 1)
		{
			for(i = 5; i >0; i--)
			{
				msleep(100);
				fts_write_reg(i2c_client, 0xc1, 0x01); //ugrec_tky
				msleep(20);
				i2c_smbus_read_i2c_block_data(i2c_client,0xc1,1,&flagc1);
				TPD_DEBUG("tpd_resume Reg 0xc1 = %d\n", flagc1);
				if(flagc1 == 1)
					break;
			}
		}
		/********  randy add for Hall run out ************/
		
		TPD_DEBUG("HALL IS CLOSE bbbbbbbbbbbbb\n");
        	return count;              	
    }
	return count;
   // return -EFAULT;
}


static ssize_t hall_state_menu_read_proc(struct file *file, char *buffer, size_t count, loff_t *ppos)
{
	char *page = NULL;
    char *ptr = NULL;
      int err = -1;
    size_t len = 0;

	page = kmalloc(FT_PAGE_SIZE, GFP_KERNEL);	
	if (!page) 
	{		
		kfree(page);		
		return -ENOMEM;	
	}
       ptr = page; 
	
	

	    ptr += sprintf(ptr,"%s",global_hall_buffer);
	   // ptr += sprintf(ptr, "\n");

	len = ptr - page; 			 	
	  if(*ppos >= len)
	  {		
		  kfree(page); 		
		  return 0; 	
	  }	
	  err = copy_to_user(buffer,(char *)page,len); 			
	  *ppos += len; 	
	  if(err) 
	  {		
	    kfree(page); 		
		  return err; 	
	  }	
	  kfree(page); 	
	  return len;	

}

static const struct file_operations hall_state_menu_proc_fops = { 
    .write = hall_state_menu_write_proc,
    .read = hall_state_menu_read_proc
};
#endif


#ifdef FTS_TP_NAME

static ssize_t tp_name_proc_write_proc(struct file *file, const char *buffer, size_t count, loff_t *ppos)
{

//	s32 ret = 0;
   // char temp[25] = {0}; // for store special format cmd
    char mode_str[15] = {0};
    unsigned int mode;
		
    unsigned long lens = sizeof(tp_name_buffer);	
	if(count>=lens)
	{
		count = lens -1;
	}

	if (copy_from_user(tp_name_buffer, buffer, count))
	{
		return -EFAULT;
	}

	tp_name_buffer[count]='\0';
	
    	sscanf(tp_name_buffer, "%s %d", (char *)&mode_str, &mode);
    
	return count;
   // return -EFAULT;
}

static ssize_t tp_name_proc_read_proc(struct file *file, char *buffer, size_t count, loff_t *ppos)
{
	char *page = NULL;
    char *ptr = NULL;
    int err = -1;
    size_t len = 0;

	page = kmalloc(FT_PAGE_SIZE, GFP_KERNEL);	
	if (!page) 
	{		
		kfree(page);		
		return -ENOMEM;	
	}
    	ptr = page; 
    	//ptr += sprintf(ptr, "==== GT9XX hotknot_menu====\n");

	switch(pt_sensor_id)
	{
		case 0x69:
			ptr += sprintf(ptr, "TP:FT5X46I_HXD\n");
			break;
		case 0x67:
			ptr += sprintf(ptr, "TP:FT5X46I_DJ\n");
			break;
		default:
			ptr += sprintf(ptr, "TP ERROR\n");
			break;
	}

    	ptr += sprintf(ptr,"%s",tp_name_buffer);
   // ptr += sprintf(ptr, "\n");

	len = ptr - page; 			 	
	  if(*ppos >= len)
	  {		
		  kfree(page); 		
		  return 0; 	
	  }	
	  err = copy_to_user(buffer,(char *)page,len); 			
	  *ppos += len; 	
	  if(err) 
	  {		
	    kfree(page); 		
		  return err; 	
	  }	
	  kfree(page); 	
	  return len;	

}

static const struct file_operations tp_name_proc_fops = { 
    .write = tp_name_proc_write_proc,
    .read = tp_name_proc_read_proc
};
#endif


static int tpd_irq_registration(void)
{
	struct device_node *node = NULL;
	int ret = 0;
//	u32 ints[2] = { 0, 0 };

	TPD_DEBUG("Device Tree Tpd_irq_registration!\n");

//	node = of_find_matching_node(node, touch_of_match);
	node = of_find_compatible_node(NULL, NULL, "mediatek,cap_touch");
	if (node) {
//		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
//		gpio_set_debounce(ints[0], ints[1]);

		touch_irq = irq_of_parse_and_map(node, 0);

		ret = request_irq(touch_irq, tpd_eint_interrupt_handler, IRQF_TRIGGER_FALLING,
				TPD_DEVICE, NULL); //IRQF_TRIGGER_FALLING  IRQF_TRIGGER_RISING
		if (ret > 0) {
			ret = -1;
			TPD_DEBUG("tpd request_irq IRQ LINE NOT AVAILABLE!.\n");
		}
		
	} else {
		TPD_DEBUG("tpd request_irq can not find touch eint device node!.\n");
		ret = -1;
	}
//	TPD_DEBUG("[%s]irq:%d, debounce:%d-%d:\n", __func__, touch_irq, ints[0], ints[1]);
	TPD_DEBUG("[%s]irq:%d\n", __func__, touch_irq);

	return ret;
}




static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
 {	 
//	int retval = TPD_OK;
//	char data;
//	u8 report_rate=0;
	//int err=0;
//	int reset_count = 0;
	unsigned char uc_reg_value[1]={0};
	unsigned char uc_reg_addr[1];
	u8 a8_reg_val = 0;
	int retval = TPD_OK;
#ifdef TPD_PROXIMITY
	int err;
	struct hwmsen_object obj_ps;
#endif
//  reset_proc:   
	i2c_client = client;

#if 0   
		//power on, need confirm with SA
#ifdef TPD_POWER_SOURCE_CUSTOM
	hwPowerOn(TPD_POWER_SOURCE_CUSTOM, VOL_3000, "TP");
#else
	hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
#endif
#ifdef TPD_POWER_SOURCE_1800
	hwPowerOn(TPD_POWER_SOURCE_1800, VOL_1800, "TP");
#endif 


#ifdef TPD_CLOSE_POWER_IN_SLEEP	 
	hwPowerDown(TPD_POWER_SOURCE,"TP");
	hwPowerOn(TPD_POWER_SOURCE,VOL_3300,"TP");
	msleep(100);
#else
	
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);  
	msleep(10);
	TPD_DMESG(" fts reset\n");
    TPD_DEBUG(" fts reset\n");
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	msleep(10);
#endif

	mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);
#else
	retval = regulator_enable(tpd->reg);
	if (retval != 0)
		TPD_DMESG("Failed to enable reg-vgp6: %d\n", retval);
#endif

	tpd_gpio_output(0, 0); //ugrec_tky gpio_direction_output(tpd_rst_gpio_number, 0);
	msleep(100);
	tpd_gpio_output(0, 1); //ugrec_tky gpio_direction_output(tpd_rst_gpio_number, 1);
	msleep(200);

//	msg_dma_alloct(client);
	
//    	fts_init_gpio_hw();

	tpd_load_status = 1;

	uc_reg_addr[0] = FTS_REG_POINT_RATE;				
	fts_i2c_Write(i2c_client, uc_reg_addr, 1);
	fts_i2c_Read(i2c_client, uc_reg_addr, 0, uc_reg_value, 1);
	TPD_DEBUG("mtk_tpd[FTS] report rate is %dHz.\n",uc_reg_value[0] * 10);

	uc_reg_addr[0] = FTS_REG_FW_VER;
	fts_i2c_Write(i2c_client, uc_reg_addr, 1);
	fts_i2c_Read(i2c_client, uc_reg_addr, 0, uc_reg_value, 1);
	TPD_DEBUG("mtk_tpd[FTS] Firmware version = 0x%x\n", uc_reg_value[0] );


	uc_reg_addr[0] = FTS_REG_CHIP_ID;
	fts_i2c_Write(i2c_client, uc_reg_addr, 1);
	retval=fts_i2c_Read(i2c_client, uc_reg_addr, 0, uc_reg_value, 1);
	TPD_DEBUG("mtk_tpd[FTS] chip id is 0x%x.\n",uc_reg_value[0] );
	uc_reg_addr[0] = FTS_REG_VENDOR_ID;
	fts_i2c_Write(i2c_client, uc_reg_addr, 1);
	//fts_i2c_Read(i2c_client, uc_reg_addr, 0, uc_reg_value, 1);
	i2c_smbus_read_i2c_block_data(i2c_client,0xA8,1,&a8_reg_val);
	printk("mtk_tpd[FTS] VENDOR id = 0x%x\n", a8_reg_val);
	pt_sensor_id = a8_reg_val;

	
	/*tpd_load_status = 1;*/
/*
	mt65xx_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);
	mt65xx_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_TOUCH_PANEL_POLARITY, tpd_eint_interrupt_handler, 1); 
	mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
*/
//	mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, tpd_eint_interrupt_handler, 1);
//    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	tpd_irq_registration();//ugrec_tky
//	enable_irq(touch_irq);

#ifdef FTS_WAKE_MENU //linhui
    hotknot_menu_proc = proc_create(HOTKNOT_MENU_PROC_FILE, 0666, NULL, &gt_hotknot_menu_proc_fops);
  	
    if (hotknot_menu_proc == NULL)
    {
        TPD_DEBUG("create_proc_entry %s failed\n", HOTKNOT_MENU_PROC_FILE);
    }
#endif

#ifdef  FTS_MODE_HALL//linhui
    hall_state_menu_proc = proc_create(HALL_STATE_PROC_FILE, 0660, NULL, &hall_state_menu_proc_fops);
  	
    if (hall_state_menu_proc == NULL)
    {
        TPD_DEBUG("create_proc_entry %s failed\n", HALL_STATE_PROC_FILE);
    }
#endif

#ifdef FTS_TP_NAME
	 tp_name_proc = proc_create(IDENTIFY_TP, 0664, NULL, &tp_name_proc_fops);
  	
	    if (tp_name_proc == NULL)
	    {
	        TPD_DEBUG("create_proc_entry %s failed\n", IDENTIFY_TP);
	    }
#endif

    #ifdef VELOCITY_CUSTOM_fts
	if((err = misc_register(&tpd_misc_device)))
	{
		TPD_DEBUG("mtk_tpd: tpd_misc_device register failed\n");
		
	}
	#endif

	thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	 if (IS_ERR(thread))
		 { 
		  retval = PTR_ERR(thread);
		  TPD_DMESG(TPD_DEVICE " failed to create kernel thread: %d\n", retval);
		}


	focaltech_get_upgrade_array();
#ifdef SYSFS_DEBUG
                fts_create_sysfs(i2c_client);
#endif
#ifdef FTS_CTL_IIC
		 if (ft_rw_iic_drv_init(i2c_client) < 0)
			 dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n",
					 __func__);
#endif
	 
#ifdef FTS_APK_DEBUG
	fts_create_apk_debug_channel(i2c_client);
#endif


#ifdef TPD_AUTO_UPGRADE
		printk("********************Enter CTP Auto Upgrade********************\n");
		DBG("[Eric_FTS FTS 1 ] upgrade 0  \n");
		fts_ctpm_auto_upgrade(i2c_client);
		DBG("[Eric_FTS	FTS 2 ] upgrade 1  \n");
#endif


#ifdef TPD_PROXIMITY
	{
		obj_ps.polling = 1; //0--interrupt mode;1--polling mode;
		obj_ps.sensor_operate = tpd_ps_operate;
		if ((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
		{
			TPD_DEBUG("hwmsen attach fail, return:%d.", err);
		}
	}
#endif
	 
#ifdef FTS_GESTRUE
	init_para(TPD_RES_X,TPD_RES_Y,60,0,0);


	input_set_capability(tpd->dev, EV_KEY, KEY_POWER);
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_U); 
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_UP); 
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_DOWN);
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_LEFT); 
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_RIGHT); 
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_O);
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_E); 
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_C);   //ugrec_tky
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_M); 
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_L);
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_W);
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_S); 
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_V);
	input_set_capability(tpd->dev, EV_KEY, KEY_GESTURE_Z);
		
	__set_bit(KEY_GESTURE_RIGHT, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_LEFT, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_UP, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_DOWN, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_U, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_O, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_E, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_C, tpd->dev->keybit); //ugrec_tky
	__set_bit(KEY_GESTURE_M, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_W, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_L, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_S, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_V, tpd->dev->keybit);
	__set_bit(KEY_GESTURE_Z, tpd->dev->keybit);
	#endif

#ifdef MT_PROTOCOL_B
	input_mt_init_slots(tpd->dev, MT_MAX_TOUCH_POINTS,1);
	input_set_abs_params(tpd->dev, ABS_MT_TOUCH_MAJOR,0, 255, 0, 0);
	input_set_abs_params(tpd->dev, ABS_MT_POSITION_X, 0, TPD_RES_X, 0, 0);
	input_set_abs_params(tpd->dev, ABS_MT_POSITION_Y, 0, TPD_RES_Y, 0, 0);
	input_set_abs_params(tpd->dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
#endif
	
   TPD_DEBUG("fts Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");
//ugrec_tky
	if(retval < TPD_OK)
	{
		tpd_load_status = 0;
		return retval;
	}
//ugrec_tky	
   return 0;
   
 }

 static int tpd_remove(struct i2c_client *client)
 
 {
 #ifdef __MSG_DMA_MODE__
     msg_dma_release();
#endif
     #ifdef FTS_CTL_IIC
     	ft_rw_iic_drv_exit();
     #endif
     #ifdef SYSFS_DEBUG
     	fts_release_sysfs(client);
     #endif
     #ifdef FTS_APK_DEBUG
     	fts_release_apk_debug_channel();
     #endif

	 TPD_DEBUG("TPD removed\n");
 
   return 0;
 }
 
 static int tpd_local_init(void)
 {
 	int retval=0;
  	TPD_DMESG("Focaltech fts I2C Touchscreen Driver\n");

#ifdef __MSG_DMA_MODE__
	msg_dma_alloct();
#endif

	tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
	retval = regulator_set_voltage(tpd->reg, 2800000, 2800000);
	if (retval != 0) {
		TPD_DMESG("Failed to set reg-vgp6 voltage: %d\n", retval);
		return -1;
	}
	
   	if(i2c_add_driver(&tpd_i2c_driver)!=0)
   	{
	        TPD_DMESG("fts unable to add i2c driver.\n");
	      	return -1;
    	}

//	if(tpd_load_status == 0) 
//	{
//	        TPD_DMESG("fts add error touch panel driver.\n");
//	    	i2c_del_driver(&tpd_i2c_driver);
//	    	return -1;
//	}
	
#ifdef TPD_HAVE_BUTTON     
	tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
#endif   
  
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))    
	TPD_DO_WARP = 1;
	memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT*4);
	memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT*4);
#endif 

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
	memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
	memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);	
#endif  
	TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);  
	tpd_type_cap = 1;
    return 0; 
 }

 static void tpd_resume( struct device *h )
 {
 		u8 flagc1 = 0;
		u8 i = 0;
 	#ifdef FTS_GESTRUE	
		u8 wake_status=1;
	#endif
	 TPD_DMESG("TPD wake up gtp_wakeup_flag=%d\n",gtp_wakeup_flag);
  	#ifdef TPD_PROXIMITY	
		if (tpd_proximity_flag == 1)
		{
			if(tpd_proximity_flag_one == 1)
			{
				tpd_proximity_flag_one = 0;	
				TPD_DMESG(TPD_DEVICE " tpd_proximity_flag_one \n"); 
				return;
			}
		}
	#endif	

 	#ifdef FTS_GESTRUE
	if(gtp_wakeup_flag==1) //ugrec_tky
	{
    		fts_write_reg(i2c_client,0xD0,0x00);
		msleep(20);
		i2c_smbus_read_i2c_block_data(i2c_client,0xd0,1,&wake_status);
		TPD_DEBUG("tpd_resume Reg 0xd0 = %d\n", wake_status);
		if(wake_status != 0)
		{
			for(i = 5; i >0; i--)
			{
				msleep(20);
				fts_write_reg(i2c_client, 0xd0, 0x00); //ugrec_tky
				msleep(20);
				i2c_smbus_read_i2c_block_data(i2c_client,0xd0,1,&wake_status);
				TPD_DEBUG("tpd_resume Reg 0xd0 = %d\n", wake_status);
				if(wake_status == 0)
					break;
			}
		}			
	}
	#endif
#ifdef TPD_CLOSE_POWER_IN_SLEEP	
//	hwPowerOn(TPD_POWER_SOURCE,VOL_3300,"TP");
	retval = regulator_enable(tpd->reg);
	if (retval != 0)
		TPD_DMESG("Failed to enable reg-vgp6: %d\n", retval);
#else

#if 0
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);  
    msleep(1);  
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
#else
	tpd_gpio_output(0, 0); //ugrec_tky gpio_direction_output(tpd_rst_gpio_number, 0);
	msleep(5);
	tpd_gpio_output(0, 1); //ugrec_tky gpio_direction_output(tpd_rst_gpio_number, 1);
	msleep(5);
#endif
#endif
//	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);  
	enable_irq(touch_irq);

	input_report_key(tpd->dev, BTN_TOUCH, 0); 
	input_sync(tpd->dev);	 

	msleep(30);
	
#ifdef FTS_MODE_HALL  //ugrec_tky
	TPD_DEBUG("tpd_resume fts_is_in_hall_mode = %d\n", fts_is_in_hall_mode);
	if(fts_is_in_hall_mode==1)
	{
		TPD_DEBUG("tpd_resume HALL IS CLOSE \n");
		
		/********  randy add for Hall run out ************/

		fts_write_reg(i2c_client, 0xc1, 0x01); //ugrec_tky
		msleep(20);
		i2c_smbus_read_i2c_block_data(i2c_client,0xc1,1,&flagc1);
		TPD_DEBUG("tpd_resume Reg 0xc1 = %d\n", flagc1);
		if(flagc1 != 1)
		{
			for(i = 5; i >0; i--)
			{
				msleep(100);
				fts_write_reg(i2c_client, 0xc1, 0x01); //ugrec_tky
				msleep(20);
				i2c_smbus_read_i2c_block_data(i2c_client,0xc1,1,&flagc1);
				TPD_DEBUG("tpd_resume Reg 0xc1 = %d\n", flagc1);
				if(flagc1 == 1)
					break;
			}
		}
		/********  randy add for Hall run out ************/
	}
	else
	{
		
		TPD_DEBUG("tpd_resume HALL IS OPEN \n");
		fts_write_reg(i2c_client, 0xc1, 0x00); //ugrec_tky
		msleep(20);
		i2c_smbus_read_i2c_block_data(i2c_client,0xc1,1,&flagc1);
		TPD_DEBUG("tpd_resume Reg 0xc1 = %d\n", flagc1);
		if(flagc1 != 0)
		{
			for(i = 5; i >0; i--)
			{
				msleep(100);
				fts_write_reg(i2c_client, 0xc1, 0x00); //ugrec_tky
				msleep(20);
				i2c_smbus_read_i2c_block_data(i2c_client,0xc1,1,&flagc1);
				TPD_DEBUG("tpd_resume Reg 0xc1 = %d\n", flagc1);
				if(flagc1 == 0)
					break;
			}
		}
		/********  randy add for Hall run out ************/
	}
#endif	
	
	tpd_halt = 0;
	TPD_DMESG("TPD wake up done\n");

 }

 static void tpd_suspend( struct device *h )
 {
	 static char data = 0x3;
#ifdef TPD_CLOSE_POWER_IN_SLEEP		 
	 int retval = TPD_OK;
#endif
#ifdef FTS_GESTRUE		 
	 int i;
	 u8 wake_status=0;
#endif
	 TPD_DMESG("TPD enter sleep gtp_wakeup_flag=%d\n",gtp_wakeup_flag);
	#ifdef TPD_PROXIMITY
	if (tpd_proximity_flag == 1)
	{
		tpd_proximity_flag_one = 1;	
		return;
	}
	#endif

	#ifdef FTS_GESTRUE
	if(gtp_wakeup_flag==1)  //ugrec_tky
	{
        	fts_write_reg(i2c_client, 0xd0, 0x01);
		msleep(20);
		i2c_smbus_read_i2c_block_data(i2c_client,0xd0,1,&wake_status);
		TPD_DEBUG("tpd_resume Reg 0xd0 = %d\n", wake_status);
		if(wake_status != 1)
		{
			for(i = 5; i >0; i--)
			{
				msleep(20);
				fts_write_reg(i2c_client, 0xd0, 0x01); //ugrec_tky
				msleep(20);
				i2c_smbus_read_i2c_block_data(i2c_client,0xd0,1,&wake_status);
				TPD_DEBUG("tpd_resume Reg 0xd0 = %d\n", wake_status);
				if(wake_status == 1)
					break;
			}
		}
		
		  if (fts_updateinfo_curr.CHIP_ID==0x54)
		  {
		  	fts_write_reg(i2c_client, 0xd1, 0xff);
			fts_write_reg(i2c_client, 0xd2, 0xff);
			fts_write_reg(i2c_client, 0xd5, 0xff);
			fts_write_reg(i2c_client, 0xd6, 0xff);
			fts_write_reg(i2c_client, 0xd7, 0xff);
			fts_write_reg(i2c_client, 0xd8, 0xff);
		  }
        	return;
	}
	#endif
 	 tpd_halt = 1;

//	 mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
	disable_irq(touch_irq);
	 mutex_lock(&i2c_access);
#ifdef TPD_CLOSE_POWER_IN_SLEEP	
//	hwPowerDown(TPD_POWER_SOURCE,"TP");
	retval = regulator_disable(tpd->reg);
	if (retval != 0)
		TPD_DMESG("Failed to disable reg-vgp6: %d\n", retval);
#else
	i2c_smbus_write_i2c_block_data(i2c_client, 0xA5, 1, &data);  //TP enter sleep mode
#endif
	mutex_unlock(&i2c_access);
    TPD_DMESG("TPD enter sleep done\n");

 } 


 static struct tpd_driver_t tpd_device_driver = {
	.tpd_device_name = "fts6x36",
	.tpd_local_init = tpd_local_init,
	.suspend = tpd_suspend,
	.resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
	.tpd_have_button = 1,
#else
	.tpd_have_button = 0,
#endif		
 };
 /* called when loaded into kernel */
 static int __init tpd_driver_init(void) {
	TPD_DMESG("ft6x36 touch panel driver init.");
	tpd_get_dts_info();
	if (tpd_driver_add(&tpd_device_driver) < 0)
		TPD_DMESG("add generic driver failed\n");
	return 0;
 }
 
 /* should never be called */
 static void __exit tpd_driver_exit(void) {
      TPD_DMESG("MediaTek fts touch panel driver exit\n");
	 //input_unregister_device(tpd->dev);
	tpd_driver_remove(&tpd_device_driver);
 }
 
 module_init(tpd_driver_init);
 module_exit(tpd_driver_exit);


