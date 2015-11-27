#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
//#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h> 
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#include <mach/upmu_common.h>
#endif
#include <cust_gpio_usage.h>
//#include <cust_i2c.h>
#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (800)
#define FRAME_HEIGHT (1280)

#define REGFLAG_DELAY								0XFE
#define REGFLAG_END_OF_TABLE						0xFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
static LCM_UTIL_FUNCS lcm_util = {
    .set_gpio_out = NULL,
};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    



#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
//#include <linux/jiffies.h>
#include <linux/uaccess.h>
//#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
/***************************************************************************** 
 * Define
 *****************************************************************************/

#define TPS_I2C_BUSNUM  2//I2C_I2C_LCD_BIAS_CHANNEL//for I2C channel 0
#define I2C_ID_NAME "tps65132"
#define TPS_ADDR 0x3E

/***************************************************************************** 
 * GLobal Variable
 *****************************************************************************/
static struct i2c_board_info __initdata tps65132_board_info = {I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR)};
static struct i2c_client *tps65132_i2c_client = NULL;


/***************************************************************************** 
 * Function Prototype
 *****************************************************************************/ 
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tps65132_remove(struct i2c_client *client);
/***************************************************************************** 
 * Data Structure
 *****************************************************************************/

 struct tps65132_dev	{	
	struct i2c_client	*client;
	
};

static const struct i2c_device_id tps65132_id[] = {
	{ I2C_ID_NAME, 0 },
	{ }
};

//#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
//static struct i2c_client_address_data addr_data = { .forces = forces,};
//#endif
static struct i2c_driver tps65132_iic_driver = {
	.id_table	= tps65132_id,
	.probe		= tps65132_probe,
	.remove		= tps65132_remove,
	//.detect		= mt6605_detect,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "tps65132",
	},
 
};
/***************************************************************************** 
 * Extern Area
 *****************************************************************************/ 
 
 

/***************************************************************************** 
 * Function
 *****************************************************************************/ 
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id)
{  
	printk( "tps65132_iic_probe\n");
	printk("TPS: info==>name=%s addr=0x%x\n",client->name,client->addr);
	tps65132_i2c_client  = client;		
	return 0;      
}


static int tps65132_remove(struct i2c_client *client)
{  	
	printk( "tps65132_remove\n");
	tps65132_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}


static int tps65132_write_bytes(unsigned char addr, unsigned char value)
{	
	int ret = 0;
	struct i2c_client *client = tps65132_i2c_client;
	char write_data[2]={0};	
	write_data[0]= addr;
	write_data[1] = value;
	ret=i2c_master_send(client, write_data, 2);
	if(ret<0)
		printk("tps65132 write data fail !!\n");	
	return ret ;
}



/*
 * module load/unload record keeping
 */

static int __init tps65132_iic_init(void)
{

   printk( "tps65132_iic_init\n");
   i2c_register_board_info(TPS_I2C_BUSNUM, &tps65132_board_info, 1);
   printk( "tps65132_iic_init2\n");
   i2c_add_driver(&tps65132_iic_driver);
   printk( "tps65132_iic_init success\n");	
   return 0;
}

static void __exit tps65132_iic_exit(void)
{
   printk( "tps65132_iic_exit\n");
   i2c_del_driver(&tps65132_iic_driver);  
}


module_init(tps65132_iic_init);
module_exit(tps65132_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK TPS65132 I2C Driver");
MODULE_LICENSE("GPL"); 

#endif

#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN

#define GPIO_LCD_BIAS_ENP_PIN		GPIO84
#define GPIO_LCM_RST				GPIO83
#define LCD_RST_VALUE				GPIO_OUT_ONE
#define LCD_ID						GPIO32

static struct LCM_setting_table lcm_initialization_setting[] =
{                                                       
	                                                                                                            
	//======Normal Display======                                                                 
	{0x11, 1, {0x00}},

	{REGFLAG_DELAY, 120, {}},

	{0x29, 1, {0x00}},                                        
	{REGFLAG_DELAY, 20, {}},
	    
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	
};                                                      

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {
		case REGFLAG_DELAY :
		    MDELAY(table[i].count);
		    break;
			
		case REGFLAG_END_OF_TABLE :
		    break;
			
		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
	
}


static void lcm_reset(unsigned char enabled)
{
	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);//[A]Set lcm_rst to gpio mode ,xmlwz ,20130313
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	if(enabled)
	{	
		mt_set_gpio_out(GPIO_LCM_RST, LCD_RST_VALUE); //[M]For different reset gpio level,ArthurLin,20130809
		MDELAY(40);
		mt_set_gpio_out(GPIO_LCM_RST, !LCD_RST_VALUE); //[M]For different reset gpio level,ArthurLin,20130809
		MDELAY(15);
		mt_set_gpio_out(GPIO_LCM_RST, LCD_RST_VALUE); //[M]For different reset gpio level,ArthurLin,20130809
	}else{
		mt_set_gpio_out(GPIO_LCM_RST, !LCD_RST_VALUE); 		
	}		
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type     = LCM_TYPE_DSI;
    params->width    = FRAME_WIDTH;
    params->height   = FRAME_HEIGHT;
    params->dsi.mode =BURST_VDO_MODE;

    params->physical_width=135;
    params->physical_height=216;

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM			= LCM_FOUR_LANE;
    params->dsi.data_format.format		= LCM_DSI_FORMAT_RGB888;

    // Highly depends on LCD driver capability.
    // Not support in MT6573
    //params->dsi.packet_size=256;

    // Video mode setting
	
    //params->dsi.intermediat_buffer_num = 2;
		
    params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;//LCM_PACKED_PS_18BIT_RGB666;

    params->dsi.vertical_sync_active				= 50;
    params->dsi.vertical_backporch				= 30;
    params->dsi.vertical_frontporch 				= 36;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active			= 64;
    params->dsi.horizontal_backporch				= 56;
    params->dsi.horizontal_frontporch				= 60;
    params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;

    // Bit rate calculation
    //params->dsi.pll_div1=34;//32		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
    //params->dsi.pll_div2=1; 		// div2=0~15: fout=fvo/(2*div2)
    params->dsi.PLL_CLOCK = 240;
}

#ifdef BUILD_LK

#define TPS65132_SLAVE_ADDR_WRITE  0x7C  
static struct mt_i2c_t TPS65132_i2c;

static int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    TPS65132_i2c.id = 2;//I2C_I2C_LCD_BIAS_CHANNEL;//I2C2;
    /* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
    TPS65132_i2c.addr = (TPS65132_SLAVE_ADDR_WRITE >> 1);
    TPS65132_i2c.mode = ST_MODE;
    TPS65132_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&TPS65132_i2c, write_data, len);
    //printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

    return ret_code;
}

#endif

static void lcm_suspend_power(void)
{
	mt_set_gpio_mode(GPIO_65132_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_EN, GPIO_OUT_ZERO);
}

static void lcm_resume_power(void)
{
	mt_set_gpio_mode(GPIO_65132_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_EN, GPIO_OUT_ONE);
#if 1
	int ret=0;
	unsigned char cmd = 0x00;
	unsigned char data = 0x0A;

	MDELAY(10);
#ifdef BUILD_LK
	ret=TPS65132_write_byte(cmd,data);
	if(ret)    	
		dprintf(0, "[LK]auo101dp26v1----tps65132----cmd=%0x--i2c write error----\n",cmd);    	
	else
		dprintf(0, "[LK]auo101dp26v1----tps65132----cmd=%0x--i2c write success----\n",cmd);    		
#else
	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]auo101dp26v1----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]auo101dp26v1----tps65132---cmd=%0x-- i2c write success-----\n",cmd);
#endif

	cmd=0x01;
	data=0x0A;//DAC 5.0V 
#ifdef BUILD_LK
	ret=TPS65132_write_byte(cmd,data);
	if(ret)    	
		dprintf(0, "[LK]auo101dp26v1----tps65132----cmd=%0x--i2c write error----\n",cmd);
	else
		dprintf(0, "[LK]auo101dp26v1----tps65132----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]auo101dp26v1----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]auo101dp26v1----tps65132---cmd=%0x-- i2c write success-----\n",cmd);
#endif
	cmd=0x03;
	data=0x43;/*[A]Set APPLICATION in Tablet mode (default is 0x03 ,smartphone mode), xmlwz , 20150317 */
#ifdef BUILD_LK
	ret=TPS65132_write_byte(cmd,data);
	if(ret)    	
		dprintf(0, "[LK]auo101dp26v1----tps65132----cmd=%0x--i2c write error----\n",cmd);    	
	else
		dprintf(0, "[LK]auo101dp26v1----tps65132----cmd=%0x--i2c write success----\n",cmd);   
#else
	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]auo101dp26v1----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]auo101dp26v1----tps65132---cmd=%0x-- i2c write success-----\n",cmd);
#endif
#endif
}

static void init_lcm_registers(void)
{
    unsigned int data_array[16];
#if 0  
    
#ifdef BUILD_LK
    printf("%s, LK \n", __func__);
#else
    printk("%s, kernel", __func__);
#endif
    
    data_array[0] = 0x00010500;  //software reset					 
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);
    DSI_clk_HS_mode(1);
    MDELAY(80);
    
    data_array[0] = 0x00290500;  //display on                        
    dsi_set_cmdq(data_array, 1, 1);
#endif
#ifdef BUILD_LK
    printf(" %s, LK  exit \n", __func__);
#else
    printk(" %s, kernel exit\n", __func__);
#endif

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    
}

static void lcm_init(void)
{
#ifdef BUILD_LK
	lcm_reset(0);//[malata]Init reset-pin low ,xmlwz,20150507 
	MDELAY(1);
	upmu_set_vpa_vosel(0);
	upmu_set_vpa_en(0);
	MDELAY(1);
#endif

	//VDD power on ->VGP3_PMU 1.8V
	upmu_set_rg_vgp3_vosel(3);
	upmu_set_rg_vgp3_en(0x1);
	MDELAY(5);

	//power on avdd and avee
	lcm_resume_power();

	MDELAY(20);
	lcm_reset(1);
	MDELAY(10);//[malata]more than 5ms,xmlwz,20150507
	init_lcm_registers();
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00280500;  //display off                        
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(5);
	data_array[0] = 0x00100500;  //display off                        
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(5);
	lcm_reset(0);
	lcm_suspend_power();

	//power on vddin disable
	upmu_set_rg_vgp3_vosel(0);
	upmu_set_rg_vgp3_en(0);

	//DSI_clk_HS_mode(0);
	MDELAY(10);
}


static void lcm_resume(void)
{
	lcm_init();
}
static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
#if defined(BUILD_LK)
	printf("auo101dp26v1_dsi_vdo_tps65132  lcm_compare_id \n");
#endif
	id = mt_get_gpio_in(LCD_ID);

	return (id==0)?1:0;
}
LCM_DRIVER auo101dp26v1_dsi_vdo_tps65132_lcm_drv = 
{
	.name			= "auo101dp26v1_dsi_vdo_tps65132_lcm_drv",
	.set_util_funcs		= lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init				= lcm_init,
	.suspend			= lcm_suspend,
	.resume			= lcm_resume,
//	.compare_id		= lcm_compare_id,
};

