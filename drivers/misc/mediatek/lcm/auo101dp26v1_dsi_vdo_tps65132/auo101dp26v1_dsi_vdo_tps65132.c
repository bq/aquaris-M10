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
	//======Normal Display======
	{0x11, 1, {0x00}},

	{REGFLAG_DELAY, 120, {}},

	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_suspend_setting[] =
{
	//======Internal setting======
	{0x28, 0, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x10, 0, {0x00}},
	{REGFLAG_DELAY, 120, {}},
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

	params->physical_width		= 135;
	params->physical_height	= 216;

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

	params->dsi.vertical_sync_active				= 50;
	params->dsi.vertical_backporch				= 30;
	params->dsi.vertical_frontporch 				= 36;
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active			= 64;
	params->dsi.horizontal_backporch				= 56;
	params->dsi.horizontal_frontporch				= 60;
	params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;


	// Bit rate calculation
	params->dsi.PLL_CLOCK = 240;
	//params->dsi.cont_clock = 1;
}

#ifdef LCD_CONTROL_PIN
static void lcm_reset(unsigned char enabled)
{
	lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
	if(enabled)
	{
		lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
		MDELAY(40);
		lcm_set_gpio_output(GPIO_LCD_RST_EN, !GPIO_OUT_ONE);
		MDELAY(15);
		lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
	}else{
		lcm_set_gpio_output(GPIO_LCD_RST_EN, !GPIO_OUT_ONE);
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
	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]auo101dp26v1----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]auo101dp26v1----tps65132---cmd=%0x-- i2c write success-----\n",cmd);


	cmd=0x01;
	data=0x0A;//DAC 5.0V

	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]auo101dp26v1----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]auo101dp26v1----tps65132---cmd=%0x-- i2c write success-----\n",cmd);

	cmd=0x03;
	data=0x43;/*[A]Set APPLICATION in Tablet mode (default is 0x03 ,smartphone mode), xmlwz , 20150317 */

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
	printk(" %s, kernel\n", __func__);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_init_lcm(void)
{
	printk("[Kernel/LCM] lcm_init() enter\n");
}


static void lcm_suspend(void)
{
//	unsigned int data_array[16];

	printk("[Kernel/LCM] lcm_suspend() enter\n");

#if 0
	data_array[0] = 0x00280500;  //display off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(5);
	data_array[0] = 0x00100500;  //display off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(5);
#else
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
#endif
	lcm_reset(0);
	lcm_suspend_power();
	lcm_vgp_supply_disable();
	MDELAY(100);
}


static void lcm_resume(void)
{
	printk("[Kernel/LCM] lcm_resume() enter\n");

	lcm_vgp_supply_enable();
	MDELAY(5);

	//power on avdd and avee
	lcm_resume_power();

	MDELAY(20);
	lcm_reset(1);
	MDELAY(10);//[malata]more than 5ms,xmlwz,20150507

	init_lcm_registers();
}

LCM_DRIVER auo101dp26v1_dsi_vdo_tps65132_lcm_drv =
{
	.name			= "auo101dp26v1_dsi_vdo_tps65132_lcm_drv",
	.set_util_funcs		= lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init				= lcm_init_lcm,
	.suspend			= lcm_suspend,
	.resume			= lcm_resume,
};

