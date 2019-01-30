#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#else
#include <string.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <platform/mt_pmic.h>
	#include <platform/mt_i2c.h>
#elif defined(BUILD_UBOOT)
#else
	//#include <mach/mt_gpio.h>
	//#include <mach/mt_pm_ldo.h>
#endif

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH  (1920)
#define FRAME_HEIGHT (1080)


#define LCD_CONTROL_PIN


//#define EDP_POWER_EN  GPIO_EDP_POWER_PIN
//#define RESETN_EDP  GPIO_EDP_SYSRSTN_PIN
//#define LCM_BIAS_EN  GPIO_LCM_PWR_EN
//#define LCM_BL_EN  GPIO_LCDBL_EN_PIN


#ifndef kal_uint8
typedef unsigned char   kal_uint8;
#endif

#define GPIO_OUT_ONE   1
#define GPIO_OUT_ZERO  0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
static LCM_UTIL_FUNCS lcm_util = 
{
	.set_reset_pin = NULL,
	.udelay = NULL,
	.mdelay = NULL,
};

typedef struct
{
  kal_uint8 dev_addr;	
  kal_uint8 addr;
  kal_uint8 data;
}it6151_setting_table;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)			lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)																		lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)								lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)																			lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   					lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)   

#define LCM_DSI_CMD_MODE	0

#define DP_I2C_ADDR  	(0x5C << 0)
#define MIPI_I2C_ADDR 	(0x6C << 0)
#define REGFLAG_DELAY 	(0xAB)


static struct proc_dir_entry *it6151_reg_proc_entry;
void IT6151_dump_register(void);
void dump_reg(void);


#ifdef LCD_CONTROL_PIN
unsigned int EDP_POWER_EN;
unsigned int RESETN_EDP;
unsigned int LCM_BIAS_EN;
unsigned int LCM_BL_EN;

void lcm_get_gpio_infor(void)
{
	static struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,lcm");
	if (!node) {
		printk("Failed to find device-tree node: mediatek,lcm\n");
		return;
	}

	EDP_POWER_EN = of_get_named_gpio(node, "it6151_power_gpio", 0);
	if (!gpio_is_valid(EDP_POWER_EN))
		printk("can not get valid gpio analogix, it6151_power_gpio\n");

	RESETN_EDP = of_get_named_gpio(node, "it6151_reset_gpio", 0);
	if (!gpio_is_valid(RESETN_EDP))
		printk("can not get valid gpio analogix, it6151_reset_gpio\n");
	
	LCM_BIAS_EN = of_get_named_gpio(node, "lcm_power_gpio", 0);
	if (!gpio_is_valid(LCM_BIAS_EN))
		printk("can not get valid gpio analogix, lcm lcm_power_gpio\n");
	
	LCM_BL_EN = of_get_named_gpio(node, "lcm_bl_en_gpio", 0);
	if (!gpio_is_valid(LCM_BL_EN))
		printk("can not get valid gpio analogix, lcm lcm_bl_en_gpio\n");
}

#endif



ssize_t it6151_reg_proc_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
      printk("---it6151_reg_proc_write----\n");
      return 0;
}

static int it6151_reg_proc_show(struct seq_file *m, void *v)
{
	printk("---it6151_reg_proc_show----\n");

	 IT6151_dump_register();
	 dump_reg();
	
	return 0;
}

int it6151_reg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, it6151_reg_proc_show, NULL);
}

static const struct file_operations it6151_reg_proc_fops = {
	.open = it6151_reg_proc_open,
	.write = it6151_reg_proc_write,
	.read = seq_read,
};


static int lcm_probe(struct device *dev)
{
    printk("LCM: ---lcm_probe---\n");

#ifdef LCD_CONTROL_PIN
	lcm_get_gpio_infor();
#endif

    //create  it6151 registers proc debug 
    it6151_reg_proc_entry = proc_create("it6151_reg", 0666, NULL, &it6151_reg_proc_fops);
    if (it6151_reg_proc_entry == NULL)
    {
	  printk("create_proc_entry  it6151_reg failed!\n");
    }

    return 0;
	
}

static const struct of_device_id lcm_of_ids[] = {
	{.compatible = "mediatek,lcm"},
	{}
};

static struct platform_driver lcm_driver = {
	.driver = {
		   .name = "mtk-lcm",
		   .owner = THIS_MODULE,
		   .probe = lcm_probe,
#ifdef CONFIG_OF
		   .of_match_table = lcm_of_ids,
#endif
		   },
};

static int __init lcm_init(void)
{
	printk("LCM: Register lcm driver\n");
	if (platform_driver_register(&lcm_driver)) {
		pr_err("LCM: failed to register disp driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit lcm_exit(void)
{
	platform_driver_unregister(&lcm_driver);
	pr_notice("LCM: Unregister lcm driver done\n");
}
late_initcall(lcm_init);
module_exit(lcm_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("Display subsystem Driver");
MODULE_LICENSE("GPL");



//it6151 chip init table
/*
static it6151_setting_table it6151_init_table[]=
{
	{DP_I2C_ADDR, 	0x05,	0x04},
	{REGFLAG_DELAY,	0x10,	0x00},					//delay 5ms
	
	{DP_I2C_ADDR, 	0x05,	0x29},
	{DP_I2C_ADDR,	0xfd,	0xd9},

	{MIPI_I2C_ADDR,  0x05,	0x00},
	{MIPI_I2C_ADDR,  0x0c,	0x31},
	{MIPI_I2C_ADDR,  0x11,	0x00},
	{MIPI_I2C_ADDR,  0x19,	0x01},
	{MIPI_I2C_ADDR,  0x28,	0x40},
	{MIPI_I2C_ADDR,  0x29,	0xff},
	{MIPI_I2C_ADDR,  0x2e,	0x34},
	{MIPI_I2C_ADDR,  0x2f,	0x01},
	{MIPI_I2C_ADDR,  0x4e,	0x02},
	{MIPI_I2C_ADDR,  0x80,	0x22},
	{MIPI_I2C_ADDR,  0x84,	0x8f},
	{REGFLAG_DELAY,	0x10,	0x00},					//delay 5ms
	
	{DP_I2C_ADDR, 	0x05,	0x00},
	{DP_I2C_ADDR, 	0x0c,	0x08},
	{DP_I2C_ADDR, 	0x21,	0x05},
	{DP_I2C_ADDR, 	0x3a,	0x04},
	{DP_I2C_ADDR, 	0x5f,	0x06},
	{DP_I2C_ADDR, 	0xb5,	0x80},
	{REGFLAG_DELAY,	0x10,	0x00},					//delay 5ms
	{DP_I2C_ADDR, 	0xc9,	0xf5},
	{DP_I2C_ADDR, 	0xca,	0x4c},
	{DP_I2C_ADDR, 	0xcb,	0x37},
	{DP_I2C_ADDR, 	0xd3,	0x03},
	{DP_I2C_ADDR, 	0xd4,	0x60},
	{DP_I2C_ADDR, 	0xe8,	0x11},
	{DP_I2C_ADDR, 	0xec,	0x00},
	{REGFLAG_DELAY,	0x10,	0x00},					//delay 5ms

	{DP_I2C_ADDR, 	0x23,	0x42},
	{DP_I2C_ADDR, 	0x24,	0x07},
	{DP_I2C_ADDR, 	0x25,	0x01},
	{DP_I2C_ADDR, 	0x26,	0x00},
	{DP_I2C_ADDR, 	0x27,	0x10},
	
	{REGFLAG_DELAY,	0x10,	0x00},					//delay 5ms
	
	{DP_I2C_ADDR, 	0x2B,	0x05},
	{DP_I2C_ADDR, 	0x23,	0x40},
	{DP_I2C_ADDR, 	0x16,	0x1E},
	{DP_I2C_ADDR, 	0x0f,	0x01},
	{DP_I2C_ADDR, 	0x76,	0xa7},
	{DP_I2C_ADDR, 	0x77,	0xaf},
	{DP_I2C_ADDR, 	0x7e,	0x8f},
	{DP_I2C_ADDR, 	0x7f,	0x07},
	
	{REGFLAG_DELAY,	0x10,	0x00},					//delay 5ms
	
	{DP_I2C_ADDR, 	0x80,	0xef},
	{DP_I2C_ADDR, 	0x81,	0x5f},
	{DP_I2C_ADDR, 	0x82,	0xef},
	{DP_I2C_ADDR, 	0x83,	0x07},
	{DP_I2C_ADDR, 	0x88,	0x38},
	{REGFLAG_DELAY,	0x10,	0x00},					//delay 5ms
	{DP_I2C_ADDR, 	0x89,	0x1f},
	{DP_I2C_ADDR, 	0x8a,	0x48},
	{DP_I2C_ADDR, 	0x0f,	0x00},
	{DP_I2C_ADDR, 	0x5c,	0xf3},
	{DP_I2C_ADDR, 	0x17,	0x01},
	{REGFLAG_DELAY,	0x10,	0x00},	//delay 5ms
	
	{0xFF,	0x00,	0x00},			//ended flag
};
*/

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
  memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void set_gpio_output(unsigned int GPIO, unsigned int output)
{
  if(GPIO == 0xFFFFFFFF)
  {
  #ifdef BUILD_LK
    printf("[LK/LCM] GPIO not available!\n");	
  #elif (defined BUILD_UBOOT)
  #else	
  #endif
    return;
  }
  #ifdef BUILD_LK
  mt_set_gpio_mode(GPIO, GPIO_MODE_00);
  mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
  mt_set_gpio_out(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
  #else
  gpio_direction_output(GPIO, output);
  gpio_set_value(GPIO, output);
  #endif
}


static void it6151_edp_reset(unsigned char enabled)
{  
	if(enabled)
	{
		set_gpio_output(RESETN_EDP, GPIO_OUT_ONE);
		MDELAY(20);
		set_gpio_output(RESETN_EDP, GPIO_OUT_ZERO);
		MDELAY(50);   //keep low level 40ms at least, add by xmwwy
		set_gpio_output(RESETN_EDP, GPIO_OUT_ONE); 		
	}else{
		set_gpio_output(RESETN_EDP, GPIO_OUT_ZERO);	
	} 
}


#ifdef BUILD_LK
#define IT6151_BUSNUM	I2C2
//#define IT6151_DEBUG

static kal_uint32 it6151_i2c_write_byte(kal_uint8 dev_addr,kal_uint8 addr, kal_uint8 data)
{
  kal_uint32 ret_code = I2C_OK;
  kal_uint8 write_data[I2C_FIFO_SIZE], len;
	struct mt_i2c_t i2c;
	
	i2c.id = IT6151_BUSNUM;
	i2c.addr = dev_addr;
	i2c.mode = ST_MODE;
	i2c.speed = 100;

	write_data[0]= addr;
    write_data[1] = data;
	len = 2;

	#ifdef IT6151_DEBUG
  /* dump write_data for check */
	printf("[it6151_i2c_write] dev_addr = 0x%x, write_data[0x%x] = 0x%x \n", dev_addr, write_data[0], write_data[1]);
	#endif
	
	ret_code = i2c_write(&i2c, write_data, len);

  return ret_code;
}

static kal_uint32 it6151_i2c_read_byte(kal_uint8 dev_addr,kal_uint8 addr, kal_uint8 *dataBuffer)
{
  kal_uint32 ret_code = I2C_OK;
	kal_uint8 len;
	struct mt_i2c_t i2c;
	
	*dataBuffer = addr;

	i2c.id = IT6151_BUSNUM;
	i2c.addr = dev_addr;
	i2c.mode = ST_MODE;
	i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&i2c, dataBuffer, len, len);

	#ifdef IT6151_DEBUG
	/* dump write_data for check */
        printf("[it6151_read_byte] dev_addr = 0x%x, read_data[0x%x] = 0x%x \n", dev_addr, addr, *dataBuffer);
	#endif
	
  return ret_code;
}
 
 /******************************************************************************
 *IIC drvier,:protocol type 2 
 ******************************************************************************/
#else
extern int it6151_i2c_read_byte(kal_uint8 dev_addr, kal_uint8 addr, kal_uint8 *returnData);
extern int it6151_i2c_write_byte(kal_uint8 dev_addr, kal_uint8 addr, kal_uint8 writeData);
#endif


/////////////////////////////////////////////////////////////////////
///       for it6151 defines start                   ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////


//#define PANEL_RESOLUTION_1280x800_NOUFO
//#define PANEL_RESOLUTION_2048x1536_NOUFO_18B
//#define PANEL_RESOLUTION_2048x1536
//#define PANEL_RESOLUTION_2048x1536_NOUFO // FOR INTEL Platform
//#define PANEL_RESOLUTION_1920x1200p60RB
#define PANEL_RESOLUTION_1920x1080p60
//#define PANEL_RESULUTION_1536x2048

#define MIPI_4_LANE 	(3)
#define MIPI_3_LANE 	(2)
#define MIPI_2_LANE 	(1)
#define MIPI_1_LANE	(0)

// MIPI Packed Pixel Stream
#define RGB_24b         (0x3E)
#define RGB_30b         (0x0D)
#define RGB_36b         (0x1D)
#define RGB_18b_P       (0x1E)
#define RGB_18b_L       (0x2E)
#define YCbCr_16b       (0x2C)
#define YCbCr_20b       (0x0C)
#define YCbCr_24b       (0x1C)

// DPTX reg62[3:0]
#define B_DPTXIN_6Bpp   (0)
#define B_DPTXIN_8Bpp   (1)
#define B_DPTXIN_10Bpp  (2)
#define B_DPTXIN_12Bpp  (3)

#define B_LBR    		(1)
#define B_HBR    		(0)

#define B_4_LANE 		(3)
#define B_2_LANE 		(1)
#define B_1_LANE 		(0)

#define B_SSC_ENABLE   	(1)
#define B_SSC_DISABLE   (0)

///////////////////////////////////////////////////////////////////////////
//CONFIGURE
///////////////////////////////////////////////////////////////////////////
#define TRAINING_BITRATE	        (B_HBR)
#define DPTX_SSC_SETTING	(B_SSC_ENABLE)//(B_SSC_DISABLE)
#define HIGH_PCLK			(1)
#define MP_MCLK_INV			(1)
#define MP_CONTINUOUS_CLK	(1)
#define MP_LANE_DESKEW		(1)
#define MP_PCLK_DIV			(2)
#define MP_LANE_SWAP		(0)
#define MP_PN_SWAP			(0)

#define DP_PN_SWAP			(0)
#define DP_AUX_PN_SWAP		(0)
#define DP_LANE_SWAP		(0)	//(0) our convert board need to LANE SWAP for data lane
#define FRAME_RESYNC		(0)
#define LVDS_LANE_SWAP		(0)
#define LVDS_PN_SWAP		(0)
#define LVDS_DC_BALANCE		(0)

#define LVDS_6BIT			(0) // '0' for 8 bit, '1' for 6 bit
#define VESA_MAP		    (1) // '0' for JEIDA , '1' for VESA MAP

#define INT_MASK			(3)
#define MIPI_INT_MASK		(0)
#define TIMER_CNT			(0x0A)
///////////////////////////////////////////////////////////////////////
// Global Setting
///////////////////////////////////////////////////////////////////////
#ifdef PANEL_RESOLUTION_1280x800_NOUFO
#define PANEL_WIDTH 1280
#define VIC 0
#define MP_HPOL 0
#define MP_VPOL 1
#define DPTX_LANE_COUNT  B_2_LANE
#define MIPI_LANE_COUNT  MIPI_4_LANE
#define EN_UFO 0
#define MIPI_PACKED_FMT		RGB_24b
#define MP_H_RESYNC			1
#define MP_V_RESYNC			0
#endif

#ifdef PANEL_RESOLUTION_1920x1080p60
#define PANEL_WIDTH 1920
#define VIC 0x10
#define MP_HPOL 1
#define MP_VPOL 1
#define DPTX_LANE_COUNT  B_2_LANE
#define MIPI_LANE_COUNT  MIPI_4_LANE
#define EN_UFO 0
#define MIPI_PACKED_FMT		RGB_24b
#define MP_H_RESYNC			1
#define MP_V_RESYNC			0
#endif

#ifdef PANEL_RESOLUTION_1920x1200p60RB
#define PANEL_WIDTH 1920
#define VIC 0 // non-Zero value for CEA setting, check the given input format.
#define MP_HPOL 1
#define MP_VPOL 0
#define DPTX_LANE_COUNT  B_2_LANE
#define MIPI_LANE_COUNT  MIPI_4_LANE
#define EN_UFO 0
#define MIPI_PACKED_FMT		RGB_24b
#define MP_H_RESYNC			1
#define MP_V_RESYNC			0
#endif

#ifdef PANEL_RESOLUTION_2048x1536
#define PANEL_WIDTH 2048
#define VIC 0 // non-Zero value for CEA setting, check the given input format.
#define MP_HPOL 0
#define MP_VPOL 1
#define MIPI_LANE_COUNT  MIPI_4_LANE
#define DPTX_LANE_COUNT  B_4_LANE
#define EN_UFO 1
#define MIPI_PACKED_FMT		RGB_24b
#define MP_H_RESYNC			0
#define MP_V_RESYNC			0
#endif

#ifdef PANEL_RESOLUTION_2048x1536_NOUFO
#define PANEL_WIDTH 2048
#define VIC 0 // non-Zero value for CEA setting, check the given input format.
#define MP_HPOL 0
#define MP_VPOL 1
#define MIPI_LANE_COUNT  MIPI_4_LANE
#define DPTX_LANE_COUNT  B_4_LANE
#define EN_UFO 0
#define MIPI_PACKED_FMT		RGB_24b
#define MP_H_RESYNC			1
#define MP_V_RESYNC			0
#endif

#ifdef PANEL_RESOLUTION_2048x1536_NOUFO_18B
#define PANEL_WIDTH 2048
#define VIC 0 // non-Zero value for CEA setting, check the given input format.
#define MP_HPOL 0
#define MP_VPOL 1
#define MIPI_LANE_COUNT  MIPI_4_LANE
#define DPTX_LANE_COUNT  B_4_LANE
#define EN_UFO 0
#define MIPI_PACKED_FMT		RGB_18b_P
#define MP_H_RESYNC			1
#define MP_V_RESYNC			0
#endif

#ifdef PANEL_RESULUTION_1536x2048
#define PANEL_WIDTH 1536
#define VIC 0 // non-Zero value for CEA setting, check the given input format.
#define MP_HPOL 0
#define MP_VPOL 1
#define MIPI_LANE_COUNT  MIPI_4_LANE
#define DPTX_LANE_COUNT  B_4_LANE
#define EN_UFO 1
#define MIPI_PACKED_FMT		RGB_24b
#define MP_H_RESYNC			1
#define MP_V_RESYNC			0
#endif

/*

static void push_table(it6151_setting_table *table, unsigned int count)
{
	unsigned int i;
	unsigned cmd;

  for(i = 0; i < count; i++)
	{   
    cmd = table[i].dev_addr;
		switch (cmd)
		{
      case REGFLAG_DELAY :
      	MDELAY(table[i].data);
        break;
				
      case 0xFF:
        break;
				
      default:
			#ifdef BUILD_LK
			it6151_i2c_write_byte(cmd, table[i].addr, table[i].data);
			#else
			it6151_i2c_write_byte(cmd, table[i].addr, table[i].data);
			#endif
   	}
  }	
}*/

/*
//#ifdef IT6151_DEBUG
static void dump_reg_table(it6151_setting_table *table, unsigned int count)
{
	unsigned int i;
	unsigned char data;
	unsigned cmd;
	
  for(i = 0; i < count; i++)
	{		
    cmd = table[i].dev_addr;
    switch (cmd)
		{	
      case REGFLAG_DELAY :
      	MDELAY(table[i].data);
        break;
				
      case 0xFF:
        break;
				
      default:
			#ifdef BUILD_LK
			it6151_i2c_read_byte(cmd, table[i].addr, &data);
			#else
			it6151_i2c_read_byte(cmd, table[i].addr, &data);
			#endif
   	}
  }
}
//#endif
*/

/////////////////////////////////////////////////////////////////////
///       for it6151 defines end                   /////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// Function
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void IT6151_dump_register(void)
{
	// int tmp, i;
	int i;
	unsigned char tmp;
	
#ifndef BUILD_LK
	printk("[Kernel/LCM] IT6151_dump_register !!!\n");
#else
	printf("[LK/LCM] IT6151_dump_register !!!\n");
#endif	


#ifdef BUILD_LK
	it6151_i2c_read_byte(DP_I2C_ADDR, 0x0d, &tmp);
	printf("[LK/LCM] IT6151 Reg0x0D=0x%x !!!\n", tmp);
	it6151_i2c_read_byte(DP_I2C_ADDR, 0x0e, &tmp);
	printf("[LK/LCM] IT6151 Reg0x0E=0x%x !!!\n", tmp);
	it6151_i2c_read_byte(MIPI_I2C_ADDR, 0x0d, &tmp);
	printf("[LK/LCM] IT6151 Reg0x0D=0x%x !!!\n", tmp);

	for(i = 0x30; i <= 0x43; i++)	
	{		
		it6151_i2c_read_byte(MIPI_I2C_ADDR, i, &tmp);
		printf("[LK/LCM] IT6151 Reg0x%x = 0x%x !!!\n", i, tmp); 		
	}	

	for(i = 0x50; i <= 0x57; i++)	
	{		
		it6151_i2c_read_byte(MIPI_I2C_ADDR, i, &tmp);
		printf("[LK/LCM] IT6151 Reg0x%x = 0x%x !!!\n", i, tmp);
	}
#else
	it6151_i2c_read_byte(DP_I2C_ADDR, 0x0d, &tmp);
	printk("[Kernel/LCM] IT6151 Reg0x0D=0x%x !!!\n", tmp);
	it6151_i2c_read_byte(DP_I2C_ADDR, 0x0e, &tmp);
	printk("[Kernel/LCM] IT6151 Reg0x0E=0x%x !!!\n", tmp);
	it6151_i2c_read_byte(MIPI_I2C_ADDR, 0x0d, &tmp);
	printk("[Kernel/LCM] IT6151 Reg0x0D=0x%x !!!\n", tmp);

	for(i = 0x30; i <= 0x43; i++) 
	{	
		it6151_i2c_read_byte(MIPI_I2C_ADDR, i, &tmp);
		printk("[Kernel/LCM] IT6151 Reg0x%x = 0x%x !!!\n", i, tmp); 		
	} 

	for(i = 0x50; i <= 0x57; i++) 
	{	
		it6151_i2c_read_byte(MIPI_I2C_ADDR, i, &tmp);
		printk("[Kernel/LCM] IT6151 Reg0x%x = 0x%x !!!\n", i, tmp);
	}
#endif

}




void  dump_reg(void)
{
    int i,j ;
    unsigned char ucData ;
#ifdef BUILD_LK	
    printf("       ");
    for(j = 0 ; j < 16 ; j++)
    {
        printf(" %02X",(int)j);
        if((j == 3)||(j==7)||(j==11))
        {
            printf("  ");
        }
    }
    printf("\n        -----------------------------------------------------\n");
    for(i = 0 ; i < 0x100 ; i+=16)
    {
        printf("[%3X]  ",i);
        for(j = 0 ; j < 16 ; j++)
        {
            if( (i+j)!= 0x17)
            {
                it6151_i2c_read_byte(DP_I2C_ADDR,((i+j)&0xFF),&ucData);
                printf(" %02X",(int)ucData);
            }
            else
            {
                printf(" %02X",(int)ucData); // for DDC FIFO
            }
            if((j == 3)||(j==7)||(j==11))
            {
                printf(" -");
            }
        }
        printf("\n");
        if((i % 0x40) == 0x30)
        {
            printf("        -----------------------------------------------------\n");
        }
    }
    for(i = 0x130; i < 0x200 ; i+=16)
    {
        printf("[%3X]  ",i);
        for(j = 0 ; j < 16 ; j++)
        {
            it6151_i2c_read_byte(DP_I2C_ADDR,((i+j)&0xFF),&ucData);
            printf(" %02X",(int)ucData);
            if((j == 3)||(j==7)||(j==11))
            {
                printf(" -");
            }
        }
        printf("\n");
        if((i % 0x40) == 0x20)
        {
            printf("        -----------------------------------------------------\n");
        }
    }
    printf("        -----------------------------------------------------\n");
            
 /******************************************************************************************************************/
	for(j = 0 ; j < 16 ; j++)
    {
        printf(" %02X",(int)j);
        if((j == 3)||(j==7)||(j==11))
        {
            printf("  ");
        }
    }
    printf("\n        -----------------------------------------------------\n");

    for(i = 0 ; i < 0x100 ; i+=16)
    {
        printf("[%3X]  ",i);
        for(j = 0 ; j < 16 ; j++)
        {
            if( (i+j)!= 0x17)
            {
                it6151_i2c_read_byte(MIPI_I2C_ADDR,((i+j)&0xFF),&ucData);
                printf(" %02X",(int)ucData);
            }
            else
            {
                printf(" XX",(int)ucData); // for DDC FIFO
            }
            if((j == 3)||(j==7)||(j==11))
            {
                printf(" -");
            }
        }
        printf("\n");
        if((i % 0x40) == 0x30)
        {
            printf("        -----------------------------------------------------\n");
        }
    }

    for(i = 0x130; i < 0x200 ; i+=16)
    {
        printf("[%3X]  ",i);
        for(j = 0 ; j < 16 ; j++)
        {
            it6151_i2c_read_byte(MIPI_I2C_ADDR,((i+j)&0xFF),&ucData);
            printf(" %02X",(int)ucData);
            if((j == 3)||(j==7)||(j==11))
            {
                printf(" -");
            }
        }
        printf("\n");
        if((i % 0x40) == 0x20)
        {
            printf("        -----------------------------------------------------\n");
        }
    }
    printf("        -----------------------------------------------------\n");
#else
    printk("       ");
    for(j = 0 ; j < 16 ; j++)
    {
        printk(" %02X",(int)j);
        if((j == 3)||(j==7)||(j==11))
        {
            printk("  ");
        }
    }
    printk("\n        -----------------------------------------------------\n");
    for(i = 0 ; i < 0x100 ; i+=16)
    {
        printk("[%3X]  ",i);
        for(j = 0 ; j < 16 ; j++)
        {
            if( (i+j)!= 0x17)
            {
                it6151_i2c_read_byte(DP_I2C_ADDR,((i+j)&0xFF),&ucData);
                printk(" %02X",(int)ucData);
            }
            else
            {
                printk(" XX"); // for DDC FIFO
            }
            if((j == 3)||(j==7)||(j==11))

            {
                printk(" -");
            }
        }
        printk("\n");
        if((i % 0x40) == 0x30)
        {
            printk("        -----------------------------------------------------\n");
        }
    }
    for(i = 0x130; i < 0x200 ; i+=16)
    {
        printk("[%3X]  ",i);
        for(j = 0 ; j < 16 ; j++)
        {
            it6151_i2c_read_byte(DP_I2C_ADDR,((i+j)&0xFF),&ucData);
            printk(" %02X",(int)ucData);
            if((j == 3)||(j==7)||(j==11))
            {
                printk(" -");
            }
        }
        printk("\n");
        if((i % 0x40) == 0x20)
        {
            printk("        -----------------------------------------------------\n");
        }
    }	

     /************************************************************************************************************/
    for(j = 0 ; j < 16 ; j++)
    {
        printk(" %02X",(int)j);
        if((j == 3)||(j==7)||(j==11))
        {
            printk("  ");
        }
    }
    printk("\n        -----------------------------------------------------\n");

    for(i = 0 ; i < 0x100 ; i+=16)
    {
        printk("[%3X]  ",i);
        for(j = 0 ; j < 16 ; j++)
        {
            if( (i+j)!= 0x17)
            {
                it6151_i2c_read_byte(MIPI_I2C_ADDR,((i+j)&0xFF),&ucData);
                printk(" %02X",(int)ucData);
            }
            else
            {
                printk(" XX"); // for DDC FIFO
            }
            if((j == 3)||(j==7)||(j==11))
            {
                printk(" -");
            }
        }
        printk("\n");
        if((i % 0x40) == 0x30)
        {
            printk("        -----------------------------------------------------\n");
        }
    }

    for(i = 0x130; i < 0x200 ; i+=16)
    {
        printk("[%3X]  ",i);
        for(j = 0 ; j < 16 ; j++)
        {
            it6151_i2c_read_byte(MIPI_I2C_ADDR,((i+j)&0xFF),&ucData);
            printk(" %02X",(int)ucData);
            if((j == 3)||(j==7)||(j==11))
            {
                printk(" -");
            }
        }
        printk("\n");
        if((i % 0x40) == 0x20)
        {
            printk("        -----------------------------------------------------\n");
        }
    }
    printk("        -----------------------------------------------------\n");
	
#endif
}





void IT6151_DPTX_init(void)
{   
#ifndef BUILD_LK
	printk("[Kernel/LCM] IT6151_DPTX_init !!!\n");
#else
	printf("[LK/LCM] IT6151_DPTX_init\n");
#endif	
	it6151_i2c_write_byte(DP_I2C_ADDR,0x05,0x29);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x05,0x00);
	
	it6151_i2c_write_byte(DP_I2C_ADDR,0x09,INT_MASK);// Enable HPD_IRQ,HPD_CHG,VIDSTABLE
	it6151_i2c_write_byte(DP_I2C_ADDR,0x0A,0x00);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x0B,0x00);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xC5,0xC1);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xB5,0x00);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xB7,0x80);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xC4,0xF0);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x06,0xFF);// Clear all interrupt
	it6151_i2c_write_byte(DP_I2C_ADDR,0x07,0xFF);// Clear all interrupt
	it6151_i2c_write_byte(DP_I2C_ADDR,0x08,0xFF);// Clear all interrupt
	
	it6151_i2c_write_byte(DP_I2C_ADDR,0x05,0x00);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x0c,0x08);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x21,0x05);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x3a,0x04);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x5f,0x06);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xc9,0xf5);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xca,0x4c);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xcb,0x37);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xce,0x80);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xd3,0x03);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xd4,0x60);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xe8,0x11);
	it6151_i2c_write_byte(DP_I2C_ADDR,0xec,VIC);
	MDELAY(5);			

	it6151_i2c_write_byte(DP_I2C_ADDR,0x23,0x42);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x24,0x07);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x25,0x01);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x26,0x00);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x27,0x10);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x2B,0x05);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x23,0x40);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x22,(DP_AUX_PN_SWAP<<3)|(DP_PN_SWAP<<2)|0x03);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x16,(DPTX_SSC_SETTING<<4)|(DP_LANE_SWAP<<3)|(DPTX_LANE_COUNT<<1)|TRAINING_BITRATE);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x0f,0x01);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x76,0xa7);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x77,0xaf);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x7e,0x8f);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x7f,0x07);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x80,0xef);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x81,0x5f);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x82,0xef);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x83,0x07);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x88,0x38);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x89,0x1f);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x8a,0x48);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x0f,0x00);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x5c,0xf3);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x17,0x04);
	it6151_i2c_write_byte(DP_I2C_ADDR,0x17,0x01);

	//it6151_i2c_write_byte(DP_I2C_ADDR,0x62,0x0); //set  RGB666, add by xmwwy
	//it6151_i2c_write_byte(DP_I2C_ADDR,0x96,0x11); //TEST PATTERN
	
	MDELAY(5);	

}




int  init_it6151(void)
{
	// unsigned char VenID[2], DevID[2], RevID;
	unsigned char VenID[2]={0x00};
	unsigned char DevID[2]={0x00};
	unsigned char RevID;
	//unsigned char cmdBuffer;
#ifndef BUILD_LK
	printk("[Kernel/LCM] IT6151_init !!!\n");
#else
	printf("[LK/LCM] IT6151_init\n");
#endif		
	it6151_i2c_read_byte(DP_I2C_ADDR, 0x00, &VenID[0]);	
	it6151_i2c_read_byte(DP_I2C_ADDR, 0x01, &VenID[1]);
	it6151_i2c_read_byte(DP_I2C_ADDR, 0x02, &DevID[0]);
	it6151_i2c_read_byte(DP_I2C_ADDR, 0x03, &DevID[1]);
	it6151_i2c_read_byte(DP_I2C_ADDR, 0x04, &RevID);	
				
#ifndef BUILD_LK	
	printk("[Kernel/LCM]1 Current DPDevID=%02X%02X\n", DevID[1], DevID[0]);
	printk("[Kernel/LCM] Current DPVenID=%02X%02X\n", VenID[1], VenID[0]);
	printk("[Kernel/LCM] Current DPRevID=%02X\n\n", RevID);	
#else
	printf("[LK/LCM]1 Current DPDevID=%02X%02X\n", DevID[1], DevID[0]);
	printf("[LK/LCM] Current DPVenID=%02X%02X\n", VenID[1], VenID[0]);
	printf("[LK/LCM] Current DPRevID=%02X\n\n", RevID);	
#endif
				
	if( VenID[0]==0x54 && VenID[1]==0x49 && DevID[0]==0x51 && DevID[1]==0x61 ){

#ifndef BUILD_LK	
		printk("[Kernel/LCM] Test 1 DP_I2C_ADDR=0x%x, MIPI_I2C_ADDR=0x%x\n", DP_I2C_ADDR, MIPI_I2C_ADDR);
#else
		printf("[LK/LCM] Test 1 DP_I2C_ADDR=0x%x, MIPI_I2C_ADDR=0x%x\n", DP_I2C_ADDR, MIPI_I2C_ADDR);
#endif
		it6151_i2c_write_byte(DP_I2C_ADDR,0x05,0x04);// DP SW Reset
		it6151_i2c_write_byte(DP_I2C_ADDR,0xfd,(MIPI_I2C_ADDR<<1)|1);
		MDELAY(2);
			
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x05,0x00);
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x0c,(MP_LANE_SWAP<<7)|(MP_PN_SWAP<<6)|(MIPI_LANE_COUNT<<4)|EN_UFO);
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x11,MP_MCLK_INV);

        if(RevID == 0xA1){			
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x19, MP_LANE_DESKEW); 
		}else{
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x19,(MP_CONTINUOUS_CLK<<1) | MP_LANE_DESKEW); 
   		}
				
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x27, MIPI_PACKED_FMT);
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x28,((PANEL_WIDTH/4-1)>>2)&0xC0);
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x29,(PANEL_WIDTH/4-1)&0xFF);
		
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x2e,0x34);
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x2f,0x01);
		
		
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x4e,(MP_V_RESYNC<<3)|(MP_H_RESYNC<<2)|(MP_VPOL<<1)|(MP_HPOL));
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x80,(EN_UFO<<5)|MP_PCLK_DIV);
		//it6151_i2c_write_byte(MIPI_I2C_ADDR,0x84,0x8f);
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x09,MIPI_INT_MASK);
		it6151_i2c_write_byte(MIPI_I2C_ADDR,0x92,TIMER_CNT);		
		IT6151_DPTX_init();

		return 0;
	}

#ifndef BUILD_LK	
	printk("[Kernel/LCM] Test 2 DP_I2C_ADDR=0x%x, MIPI_I2C_ADDR=0x%x\n", DP_I2C_ADDR, MIPI_I2C_ADDR);
#else
	printf("[LK/LCM] Test 2 DP_I2C_ADDR=0x%x, MIPI_I2C_ADDR=0x%x\n", DP_I2C_ADDR, MIPI_I2C_ADDR);
#endif


	it6151_i2c_read_byte(MIPI_I2C_ADDR, 0x00, &VenID[0]);
	it6151_i2c_read_byte(MIPI_I2C_ADDR, 0x01, &VenID[1]);
	it6151_i2c_read_byte(MIPI_I2C_ADDR, 0x02, &DevID[0]);
	it6151_i2c_read_byte(MIPI_I2C_ADDR, 0x03, &DevID[1]);
	it6151_i2c_read_byte(MIPI_I2C_ADDR, 0x04, &RevID);

#ifndef BUILD_LK
	printk("[Kernel/LCM] 2 Current MPDevID=%02X%02X\n", DevID[1], DevID[0]);
	printk("[Kernel/LCM] Current MPVenID=%02X%02X\n", VenID[1], VenID[0]);
	printk("[Kernel/LCM] Current MPRevID=%02X\n\n", RevID);
#else
	printf("[LK/LCM] 2 Current DPDevID=%02X%02X\n", DevID[1], DevID[0]);
	printf("[LK/LCM] Current DPVenID=%02X%02X\n", VenID[1], VenID[0]);
	printf("[LK/LCM] Current DPRevID=%02X\n\n", RevID);	
#endif

	if( VenID[0]==0x54 && VenID[1]==0x49 && DevID[0]==0x51 && DevID[1]==0x61 ){
	
#ifndef BUILD_LK	
			printk("[Kernel/LCM] Test 3 DP_I2C_ADDR=0x%x, MIPI_I2C_ADDR=0x%x\n", DP_I2C_ADDR, MIPI_I2C_ADDR);
#else
			printf("[LK/LCM] Test 3 DP_I2C_ADDR=0x%x, MIPI_I2C_ADDR=0x%x\n", DP_I2C_ADDR, MIPI_I2C_ADDR);
#endif
			it6151_i2c_write_byte(DP_I2C_ADDR,0x05,0x04);// DP SW Reset
			it6151_i2c_write_byte(DP_I2C_ADDR,0xfd,(MIPI_I2C_ADDR<<1)|1);
			MDELAY(2);
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x05,0x00);
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x0c,(MP_LANE_SWAP<<7)|(MP_PN_SWAP<<6)|(MIPI_LANE_COUNT<<4)|EN_UFO);
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x11,MP_MCLK_INV);
	
			if(RevID == 0xA1){			
				it6151_i2c_write_byte(MIPI_I2C_ADDR,0x19, MP_LANE_DESKEW); 
			}else{
				it6151_i2c_write_byte(MIPI_I2C_ADDR,0x19,(MP_CONTINUOUS_CLK<<1) | MP_LANE_DESKEW); 
			}
					
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x27, MIPI_PACKED_FMT);
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x28,((PANEL_WIDTH/4-1)>>2)&0xC0);
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x29,(PANEL_WIDTH/4-1)&0xFF);
			
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x2e,0x34);
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x2f,0x01);
			
			
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x4e,(MP_V_RESYNC<<3)|(MP_H_RESYNC<<2)|(MP_VPOL<<1)|(MP_HPOL));
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x80,(EN_UFO<<5)|MP_PCLK_DIV);
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x84,0x8f);
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x09,MIPI_INT_MASK);
			it6151_i2c_write_byte(MIPI_I2C_ADDR,0x92,TIMER_CNT);		
			IT6151_DPTX_init();
	
			return 0;
		}

	if(VenID[0]==0x54 && VenID[1]==0x49 && DevID[0]==0x21 && DevID[1]==0x61 ){
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x05,0x33);
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x05,0x40);
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x05,0x00);
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x0c,(MP_LANE_SWAP<<7)|(MP_PN_SWAP<<6)|(MIPI_LANE_COUNT<<4));
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x11, MP_MCLK_INV); 
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x19,(MP_CONTINUOUS_CLK<<1) | MP_LANE_DESKEW);  
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x4B,(FRAME_RESYNC<<4));
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x4E,(MP_V_RESYNC<<3)|(MP_H_RESYNC<<2)|(MP_VPOL<<1)|(MP_HPOL));      
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x72,0x01); 
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x73,0x03); 
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0x80,MP_PCLK_DIV); 
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0xC0,(HIGH_PCLK<< 4) | 0x0F);   
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0xC1,0x01);  
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0xC2,0x47);  
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0xC3,0x67);  
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0xC4,0x04);  
		    it6151_i2c_write_byte(MIPI_I2C_ADDR,0xCB,(LVDS_PN_SWAP<<5)|(LVDS_LANE_SWAP<<4)|(LVDS_6BIT<<2)|(LVDS_DC_BALANCE<<1)| VESA_MAP);  
		    return 1;
  }	
	return -1;
}


static void lcm_get_params(LCM_PARAMS *params)
{	
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;
	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

  #if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
  #else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
  #endif

	// DSI
	/* Command mode setting */
	// Three lane or Four lane
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;

	// Video mode setting		
	params->dsi.intermediat_buffer_num = 0;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count=FRAME_WIDTH*3;

	params->dsi.ufoe_enable = 0;
	
	params->dsi.vertical_sync_active				= 6;
	params->dsi.vertical_backporch					= 3;
	params->dsi.vertical_frontporch					= 20;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 
	
	params->dsi.horizontal_sync_active			= 32;
	params->dsi.horizontal_backporch			= 59;
	params->dsi.horizontal_frontporch			= 220;
	params->dsi.horizontal_active_pixel			= FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 490;
	  
	params->dsi.cont_clock = 1;
	
	//params->dsi.HS_TRAIL = 0x8;
	//params->dsi.HS_ZERO = 0xA;
	//params->dsi.HS_PRPR = 0x6;
	//params->dsi.LPX = 0x5;
	//params->dsi.DA_HS_EXIT = 0x6;

	//params->dsi.edp_panel = 1;
	//params->dsi.ssc_disable = 1;
	
}

static void lcm_init_lcm(void)
{	
#ifdef BUILD_LK
	printf("[LK/LCM] lcm_init() enter\n");
#endif
}

static void lcm_init_power(void)
{	
#ifdef BUILD_LK
	printf("[LK/LCM] lcm_init_power() enter\n");
#endif
    
}

static void lcm_suspend(void)
{
	printk("---lcm_suspend---\n");
    //back-light off, high level disable BL
	set_gpio_output(LCM_BL_EN,GPIO_OUT_ONE);
	MDELAY(2);
	
	//lcm power off
	set_gpio_output(LCM_BIAS_EN,GPIO_OUT_ZERO);
	MDELAY(2);
	
	//it6151 power off
	set_gpio_output(EDP_POWER_EN,GPIO_OUT_ZERO);
	MDELAY(2);
	
	it6151_edp_reset(0);
	
	MDELAY(120);
}

static void lcm_resume(void)
{
	printk("---lcm_resume---\n");
	
	//it6151 power on
	set_gpio_output(EDP_POWER_EN,GPIO_OUT_ONE);

	//lcm power on, as early as possible, add by xmwwy
	set_gpio_output(LCM_BIAS_EN,GPIO_OUT_ONE);
	
	//it6151 reset, low level reset
	it6151_edp_reset(1);
	MDELAY(80);
	
	init_it6151();
	MDELAY(10);
	
	//back-light on, low level enable BL, xmwwy add
	set_gpio_output(LCM_BL_EN,GPIO_OUT_ZERO);
}


LCM_DRIVER it6151_nv140fhm_edp_dsi_video_lcm_drv = 
{
	.name			= "it6151_nv140fhm_edp_dsi_video",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init_lcm,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.init_power	= lcm_init_power,
};
