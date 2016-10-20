#include <linux/string.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif

#include "lcm_drv.h"

#define LCD_DEBUG(fmt)  printk(fmt)
#define TPS65132_DEVICE
#define LCD_CONTROL_PIN

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (800)
#define FRAME_HEIGHT (1280)

#define REGFLAG_DELAY								0xFE
#define REGFLAG_END_OF_TABLE						0xFF   // END OF REGISTERS MARKER

#define GPIO_OUT_ONE		1
#define GPIO_OUT_ZERO	0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
static LCM_UTIL_FUNCS lcm_util = {
	.set_gpio_out = NULL,
};

#define SET_RESET_PIN(v)					(lcm_util.set_reset_pin((v)))

#define UDELAY(n)							(lcm_util.udelay(n))
#define MDELAY(n)							(lcm_util.mdelay(n))


struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)			lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)		lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)			lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)						lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)					lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
/*****************************************************************************
 * Define
 *****************************************************************************/

#ifdef TPS65132_DEVICE
/*****************************************************************************
 * tps65132 register
 *****************************************************************************/

/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
static struct i2c_client *tps65132_i2c_client = NULL;


/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id);
/*****************************************************************************
 * Data Structure
 *****************************************************************************/

 struct tps65132_dev	{
	struct i2c_client	*client;
};

#ifdef CONFIG_OF
static const struct of_device_id tps65132_id[] = {
	{ .compatible = "ti,tps65132" },
	{},
};
MODULE_DEVICE_TABLE(of, tps65132_id);
#endif

static const struct i2c_device_id tps65132_i2c_id[] = { {"tps65132", 0}, {} };

static struct i2c_driver tps65132_iic_driver = {
	.driver = {
			.name    = "tps65132",
#ifdef CONFIG_OF
			.of_match_table = of_match_ptr(tps65132_id),
#endif
	},
	.probe       = tps65132_probe,
	.id_table	= tps65132_i2c_id,
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
	i2c_add_driver(&tps65132_iic_driver);
	printk( "tps65132_iic_init success\n");
	return 0;
}

static void __exit tps65132_iic_exit(void)
{
	printk( "tps65132_iic_exit\n");
	i2c_del_driver(&tps65132_iic_driver);
}

late_initcall(tps65132_iic_init);
module_exit(tps65132_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK TPS65132 I2C Driver");
MODULE_LICENSE("GPL");
#endif

/*****************************************************************************
 * lcm info
 *****************************************************************************/

static struct regulator *lcm_vgp;
#ifdef LCD_CONTROL_PIN
static unsigned int GPIO_LCD_PWR_EN;
static unsigned int GPIO_LCD_RST_EN;

void lcm_get_gpio_infor(void)
{
	static struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,lcm");
	if (!node) {
		printk("Failed to find device-tree node: mediatek,lcm\n");
		return;
	}

	GPIO_LCD_PWR_EN = of_get_named_gpio(node, "lcm_power_gpio", 0);
	if (gpio_is_valid(GPIO_LCD_PWR_EN) < 0)
		printk("can not get valid gpio analogix, lcm lcm_power_gpio\n");

	GPIO_LCD_RST_EN = of_get_named_gpio(node, "lcm_reset_gpio", 0);
	if (gpio_is_valid(GPIO_LCD_RST_EN) < 0)
		printk("can not get valid gpio analogix, lcm lcm_reset_gpio\n");
}

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, output);
}
#endif

/* get LDO supply */
static int lcm_get_vgp_supply(struct device *dev)
{
	int ret;
	struct regulator *lcm_vgp_ldo;

	printk("LCM: lcm_get_vgp_supply is going\n");

	lcm_vgp_ldo = devm_regulator_get(dev, "reg-lcm");
	if (IS_ERR(lcm_vgp_ldo)) {
		ret = PTR_ERR(lcm_vgp_ldo);
		dev_err(dev, "failed to get reg-lcm LDO, %d\n", ret);
		return ret;
	}

	printk("LCM: lcm get supply ok.\n");

	ret = regulator_enable(lcm_vgp_ldo);
	/* get current voltage settings */
	ret = regulator_get_voltage(lcm_vgp_ldo);
	printk("lcm LDO voltage = %d in kernel stage\n", ret);

	lcm_vgp = lcm_vgp_ldo;

	return ret;
}

int lcm_vgp_supply_enable(void)
{
	int ret;
	unsigned int volt;

	printk("LCM: lcm_vgp_supply_enable\n");

	if (NULL == lcm_vgp)
		return 0;

	printk("LCM: set regulator voltage lcm_vgp voltage to 1.8V\n");
	/* set voltage to 1.8V */
	ret = regulator_set_voltage(lcm_vgp, 1800000, 1800000);
	if (ret != 0) {
		pr_err("LCM: lcm failed to set lcm_vgp voltage: %d\n", ret);
		return ret;
	}

	/* get voltage settings again */
	volt = regulator_get_voltage(lcm_vgp);
	if (volt == 1800000)
		pr_err("LCM: check regulator voltage=1800000 pass!\n");
	else
		pr_err("LCM: check regulator voltage=1800000 fail! (voltage: %d)\n", volt);

	ret = regulator_enable(lcm_vgp);
	if (ret != 0) {
		pr_err("LCM: Failed to enable lcm_vgp: %d\n", ret);
		return ret;
	}

	return ret;
}

int lcm_vgp_supply_disable(void)
{
	int ret = 0;
	unsigned int isenable;

	if (NULL == lcm_vgp)
		return 0;

	/* disable regulator */
	isenable = regulator_is_enabled(lcm_vgp);

	printk("LCM: lcm query regulator enable status[0x%d]\n", isenable);

	if (isenable) {
		ret = regulator_disable(lcm_vgp);
		if (ret != 0) {
			pr_err("LCM: lcm failed to disable lcm_vgp: %d\n", ret);
			return ret;
		}
		/* verify */
		isenable = regulator_is_enabled(lcm_vgp);
		if (!isenable)
			pr_err("LCM: lcm regulator disable pass\n");
	}

	return ret;
}


static int lcm_probe(struct device *dev)
{
	lcm_get_vgp_supply(dev);
#ifdef LCD_CONTROL_PIN
	lcm_get_gpio_infor();
#endif

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


static struct LCM_setting_table lcm_initialization_setting[] =
{
	//======Internal setting======
	{0xFF, 4, {0xAA, 0x55, 0xA5, 0x80}},

	//=MIPI ralated timing setting======
	{0x6F, 2, {0x11, 0x00}},
	{0xF7, 2, {0x20, 0x00}},
	//=Improve ESD option======
	{0x6F, 1, {0x06}},
	{0xF7, 1, {0xA0}},
	{0x6F, 1, {0x19}},
	{0xF7, 1, {0x12}},
	{0xF4, 1, {0x03}},

	//=Vcom floating======
	{0x6F, 1, {0x08}},
	{0xFA, 1, {0x40}},
	{0x6F, 1, {0x11}},
	{0xF3, 1, {0x01}},

	//=Page0 relative======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x00}},
	{0xC8, 1, {0x80}},

	//=Set WXGA resolution 0x6C======
	/*
	Tt parameter of below command,
	ol value is 0x07,
	s0x01 to rotate 180 degress,
	ter option value is 0x03 & 0x05.
	*/
	{0xB1, 2, {0x6C, 0x01}},

	//=Set source output hold time======
	{0xB6, 1, {0x08}},

	//=EQ control function======
	{0x6F, 1, {0x02}},
	{0xB8, 1, {0x08}},

	//=Set bias current for GOP and SOP======
	{0xBB, 2, {0x74, 0x44}},

	//=Inversion setting======
	{0xBC, 2, {0x00, 0x00}},

	//=DSP timing Settings update for BIST======
	{0xBD, 5, {0x02, 0xB0, 0x0C, 0x0A, 0x00}},

	//=Page1 relative======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x01}},

	//=Setting AVDD, AVEE clamp======
	{0xB0, 2, {0x05, 0x05}},
	{0xB1, 2, {0x05, 0x05}},

	//=VGMP, VGMN, VGSP, VGSN setting======
	{0xBC, 2, {0x90, 0x01}},
	{0xBD, 2, {0x90, 0x01}},

	//=Gate signal control======
	{0xCA, 1, {0x00}},

	//=Power IC control======
	{0xC0, 1, {0x04}},

	//=VCOM -1.88V======
	{0xBE, 1, {0x29}},

	//=Setting VGH=15V, VGL=-11V======
	{0xB3, 2, {0x37, 0x37}},
	{0xB4, 2, {0x19, 0x19}},

	//=Power control for VGH, VGL======
	{0xB9, 2, {0x44, 0x44}},
	{0xBA, 2, {0x24, 0x24}},

	//=Page2 relative======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x02}},

	//=Gamma control register control======
	{0xEE, 1, {0x01}},
	//=Gradient control for Gamma voltage======
	{0xEF, 4, {0x09, 0x06, 0x15, 0x18}},

	{0xB0, 6, {0x00, 0x00, 0x00, 0x25, 0x00, 0x43}},
	{0x6F, 1, {0x06}},
	{0xB0, 6, {0x00, 0x54, 0x00, 0x68, 0x00, 0xA0}},
	{0x6F, 1, {0x0C}},
	{0xB0, 4, {0x00, 0xC0, 0x01, 0x00}},
	{0xB1, 6, {0x01, 0x30, 0x01, 0x78, 0x01, 0xAE}},
	{0x6F, 1, {0x06}},
	{0xB1, 6, {0x02, 0x08, 0x02, 0x52, 0x02, 0x54}},
	{0x6F, 1, {0x0C}},
	{0xB1, 4, {0x02, 0x99, 0x02, 0xF0}},
	{0xB2, 6, {0x03, 0x20, 0x03, 0x56, 0x03, 0x76}},
	{0x6F, 1, {0x06}},
	{0xB2, 6, {0x03, 0x93, 0x03, 0xA4, 0x03, 0xB9}},
	{0x6F, 1, {0x0C}},
	{0xB2, 4, {0x03, 0xC9, 0x03, 0xE3}},
	{0xB3, 4, {0x03, 0xFC, 0x03, 0xFF}},

	//=GOA relative======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x06}},
	{0xB0, 2, {0x00, 0x10}},
	{0xB1, 2, {0x12, 0x14}},
	{0xB2, 2, {0x16, 0x18}},
	{0xB3, 2, {0x1A, 0x29}},
	{0xB4, 2, {0x2A, 0x08}},
	{0xB5, 2, {0x31, 0x31}},
	{0xB6, 2, {0x31, 0x31}},
	{0xB7, 2, {0x31, 0x31}},
	{0xB8, 2, {0x31, 0x0A}},
	{0xB9, 2, {0x31, 0x31}},
	{0xBA, 2, {0x31, 0x31}},
	{0xBB, 2, {0x0B, 0x31}},
	{0xBC, 2, {0x31, 0x31}},
	{0xBD, 2, {0x31, 0x31}},
	{0xBE, 2, {0x31, 0x31}},
	{0xBF, 2, {0x09, 0x2A}},
	{0xC0, 2, {0x29, 0x1B}},
	{0xC1, 2, {0x19, 0x17}},
	{0xC2, 2, {0x15, 0x13}},
	{0xC3, 2, {0x11, 0x01}},
	{0xE5, 2, {0x31, 0x31}},
	{0xC4, 2, {0x09, 0x1B}},
	{0xC5, 2, {0x19, 0x17}},
	{0xC6, 2, {0x15, 0x13}},
	{0xC7, 2, {0x11, 0x29}},
	{0xC8, 2, {0x2A, 0x01}},
	{0xC9, 2, {0x31, 0x31}},
	{0xCA, 2, {0x31, 0x31}},
	{0xCB, 2, {0x31, 0x31}},
	{0xCC, 2, {0x31, 0x0B}},
	{0xCD, 2, {0x31, 0x31}},
	{0xCE, 2, {0x31, 0x31}},
	{0xCF, 2, {0x0A, 0x31}},
	{0xD0, 2, {0x31, 0x31}},
	{0xD1, 2, {0x31, 0x31}},
	{0xD2, 2, {0x31, 0x31}},
	{0xD3, 2, {0x00, 0x2A}},
	{0xD4, 2, {0x29, 0x10}},
	{0xD5, 2, {0x12, 0x14}},
	{0xD6, 2, {0x16, 0x18}},
	{0xD7, 2, {0x1A, 0x08}},
	{0xE6, 2, {0x31, 0x31}},
	{0xD8, 5, {0x00, 0x00, 0x00, 0x54, 0x00}},
	{0xD9, 5, {0x00, 0x15, 0x00, 0x00, 0x00}},
	{0xE7, 1, {0x00}},

	//=Page3, gate timing control======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x03}},
	{0xB0, 2, {0x20, 0x00}},
	{0xB1, 2, {0x20, 0x00}},
	{0xB2, 5, {0x05, 0x00, 0x00, 0x00, 0x00}},

	{0xB6, 5, {0x05, 0x00, 0x00, 0x00, 0x00}},
	{0xB7, 5, {0x05, 0x00, 0x00, 0x00, 0x00}},

	{0xBA, 5, {0x57, 0x00, 0x00, 0x00, 0x00}},
	{0xBB, 5, {0x57, 0x00, 0x00, 0x00, 0x00}},

	{0xC0, 4, {0x00, 0x00, 0x00, 0x00}},
	{0xC1, 4, {0x00, 0x00, 0x00, 0x00}},

	{0xC4, 1, {0x60}},
	{0xC5, 1, {0x40}},

	//=Page5======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x05}},
	{0xBD, 5, {0x03, 0x01, 0x03, 0x03, 0x03}},
	{0xB0, 2, {0x17, 0x06}},
	{0xB1, 2, {0x17, 0x06}},
	{0xB2, 2, {0x17, 0x06}},
	{0xB3, 2, {0x17, 0x06}},
	{0xB4, 2, {0x17, 0x06}},
	{0xB5, 2, {0x17, 0x06}},

	{0xB8, 1, {0x00}},
	{0xB9, 1, {0x00}},
	{0xBA, 1, {0x00}},
	{0xBB, 1, {0x02}},
	{0xBC, 1, {0x00}},

	{0xC0, 1, {0x07}},

	{0xC4, 1, {0x80}},
	{0xC5, 1, {0xA4}},

	{0xC8, 2, {0x05, 0x30}},
	{0xC9, 2, {0x01, 0x31}},

	{0xCC, 3, {0x00, 0x00, 0x3C}},
	{0xCD, 3, {0x00, 0x00, 0x3C}},

	{0xD1, 5, {0x00, 0x04, 0xFD, 0x07, 0x10}},
	{0xD2, 5, {0x00, 0x05, 0x02, 0x07, 0x10}},

	{0xE5, 1, {0x06}},
	{0xE6, 1, {0x06}},
	{0xE7, 1, {0x06}},
	{0xE8, 1, {0x06}},
	{0xE9, 1, {0x06}},
	{0xEA, 1, {0x06}},

	{0xED, 1, {0x30}},

	//=Reload setting======
	{0x6F, 1, {0x11}},
	{0xF3, 1, {0x01}},

	//=Normal Display======
	{0x35, 0, {}},
	{0x11, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 0, {}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},
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

	params->type		= LCM_TYPE_DSI;
	params->width 	= FRAME_WIDTH;
	params->height 	= FRAME_HEIGHT;
	params->dsi.mode	= BURST_VDO_MODE;

	params->physical_width		= 108;
	params->physical_height	= 172;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	params->dsi.data_format.format			= LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	//params->dsi.packet_size=256;

	// Video mode setting

	//params->dsi.intermediat_buffer_num = 2;
	//params->dsi.word_count=FRAME_WIDTH*3;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;//LCM_PACKED_PS_18BIT_RGB666;

	params->dsi.vertical_sync_active				= 5;
	params->dsi.vertical_backporch				= 3;
	params->dsi.vertical_frontporch 				= 8;
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active			= 5;
	params->dsi.horizontal_backporch			= 59;
	params->dsi.horizontal_frontporch			= 16;
	params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;


	// Bit rate calculation
	params->dsi.PLL_CLOCK = 234;
	//params->dsi.cont_clock = 1;
}

#ifdef LCD_CONTROL_PIN
static void lcm_reset(unsigned char enabled)
{
	if(enabled)
	{
		lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
		MDELAY(1);
		lcm_set_gpio_output(GPIO_LCD_RST_EN, !GPIO_OUT_ONE);
		MDELAY(15);
		lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
	}else{
		lcm_set_gpio_output(GPIO_LCD_RST_EN, !GPIO_OUT_ONE);
		MDELAY(1);
	}
}

static void lcm_suspend_power(void)
{
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
}

static void lcm_resume_power(void)
{
#ifdef TPS65132_DEVICE
	unsigned char cmd = 0x0;
	unsigned char data = 0x0A;
 	int ret=0;
#endif

	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	MDELAY(10);

#ifdef TPS65132_DEVICE
	cmd=0x00;
	data=0x12;//DAC -5.8V

	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write success-----\n",cmd);

	cmd=0x01;
	data=0x12;//DAC 5.8V

	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write success-----\n",cmd);

	cmd=0x03;
	data=0x43;/*Set APPLICATION in Tablet mode (default is 0x03 ,smartphone mode)*/

	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write success-----\n",cmd);
#endif
#endif
}

static void init_lcm_registers(void)
{
	printk(" %s, kernel\n", __func__);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_init_lcm(void)
{
	printk("[Kernel/LCM] lcm_init() enter\n");
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];

	printk("[Kernel/LCM] lcm_suspend() enter\n");

	data_array[0] = 0x00280500;  //display off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
	lcm_reset(0);
	lcm_suspend_power();
	lcm_vgp_supply_disable();
	MDELAY(1);
}


static void lcm_resume(void)
{
	printk("[Kernel/LCM] lcm_resume() enter\n");

	lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ZERO);

	lcm_vgp_supply_enable();
	MDELAY(5);

	//power on avdd and avee
	lcm_resume_power();

	MDELAY(50);
	lcm_reset(1);
	MDELAY(10);
	lcm_reset(0);
	MDELAY(50);
	lcm_reset(1);
	MDELAY(100);//Must > 120ms

	init_lcm_registers();
}

LCM_DRIVER inn080dp10v5_dsi_vdo_tps65132_lcm_drv =
{
	.name			= "inn080dp10v5_dsi_vdo_tps65132_lcm_drv",
	.set_util_funcs		= lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init				= lcm_init_lcm,
	.suspend			= lcm_suspend,
	.resume			= lcm_resume,
};

