/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
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

#define LCD_DEBUG(fmt, args...)  pr_debug("[KERNEL/LCM]"fmt, ##args)

#ifdef CONFIG_IFLYTEK_XFM10213
extern void xfm10213_change_state(bool is_suspend);
#endif


// ---------------------------------------------------------------------------
//	Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH  (320)
#define FRAME_HEIGHT (240)

#define HSYNC_PULSE_WIDTH 150
#define HSYNC_BACK_PORCH  80
#define HSYNC_FRONT_PORCH 50
#define VSYNC_PULSE_WIDTH 80
#define VSYNC_BACK_PORCH  60
#define VSYNC_FRONT_PORCH 20

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
static LCM_UTIL_FUNCS lcm_util = {
	.set_gpio_out = NULL,
};

#define SET_RESET_PIN(v)					(lcm_util.set_reset_pin((v)))

#define UDELAY(n)							(lcm_util.udelay(n))
#define MDELAY(n)							(lcm_util.mdelay(n))

struct LCM_setting_table{
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};

#define GPIO_OUT_ONE	1
#define GPIO_OUT_ZERO	0

static struct regulator *lcm_vpa;
static struct regulator *lcm_vrf18;
static unsigned int GPIO_LCD_RST_EN;
static unsigned int GPIO_SPI0_CS;
static unsigned int GPIO_SPI0_CLK;
static unsigned int GPIO_SPI0_MO;

void lcm_get_gpio_infor(struct device *dev)
{
	GPIO_LCD_RST_EN = of_get_named_gpio(dev->of_node, "lcm_reset_gpio", 0);
	GPIO_SPI0_CS = of_get_named_gpio(dev->of_node, "lcm_spi_cs", 0);
	GPIO_SPI0_CLK = of_get_named_gpio(dev->of_node, "lcm_spi_ck", 0);
	GPIO_SPI0_MO = of_get_named_gpio(dev->of_node, "lcm_spi_mo", 0);

	gpio_request(GPIO_LCD_RST_EN, "GPIO_LCD_RST_EN");
	gpio_request(GPIO_SPI0_CS, "GPIO_SPI0_CS");
	gpio_request(GPIO_SPI0_CLK, "GPIO_SPI0_CLK");
	gpio_request(GPIO_SPI0_MO, "GPIO_SPI0_MO");
}

static int lcm_get_vpa_supply(struct device *dev)
{
	int ret;
	struct regulator *lcm_vpa_ldo;

	LCD_DEBUG("lcm_get_vpa_supply is going\n");

	lcm_vpa_ldo = devm_regulator_get(dev, "reg-vpa");
	if (IS_ERR(lcm_vpa_ldo)) {
		ret = PTR_ERR(lcm_vpa_ldo);
		dev_err(dev, "failed to get reg-lcm LDO, %d\n", ret);
		return ret;
	}

	LCD_DEBUG("lcm get supply ok.\n");

	ret = regulator_enable(lcm_vpa_ldo);
	/* get current voltage settings */
	ret = regulator_get_voltage(lcm_vpa_ldo);
	LCD_DEBUG("lcm LDO voltage = %d in kernel stage\n", ret);

	lcm_vpa = lcm_vpa_ldo;

	return ret;
}

static int lcm_vpa_supply_enable(void)
{
	int ret;
	unsigned int volt;

	LCD_DEBUG("%s\n",__func__);

	if (NULL == lcm_vpa)
		return 0;

	LCD_DEBUG("set regulator voltage lcm_vpa voltage to 2.8V\n");
	/* set voltage to 2.8V */
	ret = regulator_set_voltage(lcm_vpa, 2800000, 2800000);
	if (ret != 0) {
		pr_err("LCM: lcm failed to set lcm_vpa voltage: %d\n", ret);
		return ret;
	}

	/* get voltage settings again */
	volt = regulator_get_voltage(lcm_vpa);
	if (volt != 2800000) {
		pr_err("LCM: check regulator voltage = 2800000 fail! (voltage: %d)\n", volt);
	}

	ret = regulator_enable(lcm_vpa);
	if (ret != 0) {
		pr_err("LCM: Failed to enable lcm_vpa: %d\n", ret);
	}

	return ret;
}

static int lcm_vpa_supply_disable(void)
{
	int ret = 0;
	unsigned int isenable;

	LCD_DEBUG("%s\n",__func__);

	if (NULL == lcm_vpa)
		return 0;

	/* disable regulator */
	isenable = regulator_is_enabled(lcm_vpa);

	LCD_DEBUG("lcm query regulator enable status[0x%d]\n", isenable);

	if (isenable) {
		ret = regulator_disable(lcm_vpa);
		if (ret != 0) {
			pr_err("LCM: lcm failed to disable lcm_vpa: %d\n", ret);
			return ret;
		}
		/* verify */
		isenable = regulator_is_enabled(lcm_vpa);
		if (!isenable) {
			LCD_DEBUG("lcm regulator disable pass\n");
		}
	}

	return ret;
}

static int lcm_get_vrf18_supply(struct device *dev)
{
	int ret;
	struct regulator *lcm_vrf18_ldo;

	LCD_DEBUG("lcm_get_vrf18_supply is going\n");

	lcm_vrf18_ldo = devm_regulator_get(dev, "reg-vrf18");
	if (IS_ERR(lcm_vrf18_ldo)) {
		ret = PTR_ERR(lcm_vrf18_ldo);
		dev_err(dev, "failed to get lcm-vrf18 LDO, %d\n", ret);
		return ret;
	}

	LCD_DEBUG("lcm get vrf18 supply ok.\n");

	ret = regulator_enable(lcm_vrf18_ldo);
	/* get current voltage settings */
	ret = regulator_get_voltage(lcm_vrf18_ldo);
	LCD_DEBUG("lcm LDO voltage = %d in kernel stage\n", ret);

	lcm_vrf18 = lcm_vrf18_ldo;

	return ret;
}

static int lcm_vrf18_supply_enable(void)
{
	int ret;

	LCD_DEBUG("%s\n",__func__);

	if (NULL == lcm_vrf18)
		return 0;

	ret = regulator_enable(lcm_vrf18);
	if (ret != 0) {
		pr_err("LCM: Failed to enable lcm_vrf18: %d\n", ret);
	}

	return ret;
}

static int lcm_vrf18_supply_disable(void)
{
	int ret = 0;
	unsigned int isenable;

	LCD_DEBUG("%s\n",__func__);

	if (NULL == lcm_vrf18)
		return 0;

	/* disable regulator */
	isenable = regulator_is_enabled(lcm_vrf18);

	LCD_DEBUG("lcm query regulator enable status[0x%d]\n", isenable);

	if (isenable) {
		ret = regulator_disable(lcm_vrf18);
		if (ret != 0) {
			pr_err("LCM: lcm failed to disable lcm_vrf18: %d\n", ret);
			return ret;
		}
		/* verify */
		isenable = regulator_is_enabled(lcm_vrf18);
		if (!isenable) {
			LCD_DEBUG("lcm regulator disable pass\n");
		}
	}

	return ret;
}

static int lcm_probe(struct device *dev)
{
	lcm_get_vpa_supply(dev);
	lcm_get_vrf18_supply(dev);
	lcm_get_gpio_infor(dev);

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
	LCD_DEBUG("Register lcm driver\n");
	if (platform_driver_register(&lcm_driver)) {
		pr_err("LCM: failed to register disp driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit lcm_exit(void)
{
	platform_driver_unregister(&lcm_driver);
}
late_initcall(lcm_init);
module_exit(lcm_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("Display subsystem Driver");
MODULE_LICENSE("GPL");

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DPI;
	params->ctrl = LCM_CTRL_GPIO;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->dpi.width = FRAME_WIDTH;
	params->dpi.height = FRAME_HEIGHT;

	params->dpi.format = LCM_DPI_FORMAT_RGB666;
	params->dpi.rgb_order = LCM_COLOR_ORDER_RGB;

	params->dpi.clk_pol = LCM_POLARITY_FALLING;
	params->dpi.de_pol = LCM_POLARITY_RISING;
	params->dpi.vsync_pol = LCM_POLARITY_RISING;
	params->dpi.hsync_pol = LCM_POLARITY_RISING;

	params->dpi.hsync_pulse_width = HSYNC_PULSE_WIDTH;
	params->dpi.hsync_back_porch = HSYNC_BACK_PORCH;
	params->dpi.hsync_front_porch = HSYNC_FRONT_PORCH;
	params->dpi.vsync_pulse_width = VSYNC_PULSE_WIDTH;
	params->dpi.vsync_back_porch = VSYNC_BACK_PORCH;
	params->dpi.vsync_front_porch = VSYNC_FRONT_PORCH;

	params->dpi.PLL_CLOCK = 15;

	params->dpi.lvds_tx_en = 0;
	params->dpi.ssc_disable = 1;

	params->dpi.io_driving_current = LCM_DRIVING_CURRENT_4MA;
}

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	gpio_direction_output(GPIO, output);
}

static void lcm_reset(unsigned char enabled)
{
	if(enabled) {
		lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
		MDELAY(40);
		lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ZERO);
		MDELAY(15);
		lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
	} else {
		lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ZERO);
	}
}

static void __spi_write_cmd(unsigned char val)
{
	unsigned int n;
	unsigned char out;
	lcm_set_gpio_output(GPIO_SPI0_CS, GPIO_OUT_ZERO);
	lcm_set_gpio_output(GPIO_SPI0_CLK, GPIO_OUT_ZERO);
	lcm_set_gpio_output(GPIO_SPI0_MO, GPIO_OUT_ZERO); //This value to define send cmd or data
	UDELAY(3);
	lcm_set_gpio_output(GPIO_SPI0_CLK, GPIO_OUT_ONE);
	UDELAY(3);

	for (n = 0; n < 8; n++) {
		lcm_set_gpio_output(GPIO_SPI0_CLK, GPIO_OUT_ZERO);
		out = val & 0x80;
		lcm_set_gpio_output(GPIO_SPI0_MO, (!!out));
		UDELAY(3);
		lcm_set_gpio_output(GPIO_SPI0_CLK, GPIO_OUT_ONE);
		UDELAY(3);
		val <<= 1;
	}
	lcm_set_gpio_output(GPIO_SPI0_CS, GPIO_OUT_ONE);
}

static void __spi_write_data(unsigned char val)
{
	unsigned int n;
	unsigned char out;
	lcm_set_gpio_output(GPIO_SPI0_CS, GPIO_OUT_ZERO);
	lcm_set_gpio_output(GPIO_SPI0_CLK, GPIO_OUT_ZERO);
	lcm_set_gpio_output(GPIO_SPI0_MO, GPIO_OUT_ONE); //This value to define send cmd or data
	UDELAY(3);
	lcm_set_gpio_output(GPIO_SPI0_CLK, GPIO_OUT_ONE);
	UDELAY(3);

	for (n = 0; n < 8; n++) {
		lcm_set_gpio_output(GPIO_SPI0_CLK, GPIO_OUT_ZERO);
		out = val & 0x80;
		lcm_set_gpio_output(GPIO_SPI0_MO, (!!out));
		UDELAY(3);
		lcm_set_gpio_output(GPIO_SPI0_CLK, GPIO_OUT_ONE);
		UDELAY(3);
		val <<= 1;
	}
	lcm_set_gpio_output(GPIO_SPI0_CS, GPIO_OUT_ONE);
}

static void init_lcm_registers(void)
{
	LCD_DEBUG("%s\n", __func__);

	__spi_write_cmd(0xC8);
	__spi_write_data(0xFF);
	__spi_write_data(0x93);
	__spi_write_data(0x42);

	__spi_write_cmd(0x36);
	__spi_write_data(0x00);

	__spi_write_cmd(0x3A);
	__spi_write_data(0x65);

	__spi_write_cmd(0xF6);
	__spi_write_data(0x01);
	__spi_write_data(0x00);
	__spi_write_data(0x06);

	__spi_write_cmd(0xC0);
	__spi_write_data(0x11);
	__spi_write_data(0x11);

	__spi_write_cmd(0xC1);
	__spi_write_data(0x01);

	__spi_write_cmd(0xC5);
	__spi_write_data(0xF9);

	__spi_write_cmd(0xB1);
	__spi_write_data(0x00);
	__spi_write_data(0x18);

	__spi_write_cmd(0xB4);
	__spi_write_data(0x02);

	__spi_write_cmd(0xE0);
	__spi_write_data(0x00);
	__spi_write_data(0x0A);
	__spi_write_data(0x0E);
	__spi_write_data(0x00);
	__spi_write_data(0x0E);
	__spi_write_data(0x05);
	__spi_write_data(0x2C);
	__spi_write_data(0x47);
	__spi_write_data(0x3F);
	__spi_write_data(0x0A);
	__spi_write_data(0x0B);
	__spi_write_data(0x09);
	__spi_write_data(0x0F);
	__spi_write_data(0x12);
	__spi_write_data(0x0F);

	__spi_write_cmd(0xE1);
	__spi_write_data(0x00);
	__spi_write_data(0x28);
	__spi_write_data(0x2A);
	__spi_write_data(0x02);
	__spi_write_data(0x0D);
	__spi_write_data(0x08);
	__spi_write_data(0x4A);
	__spi_write_data(0x03);
	__spi_write_data(0x63);
	__spi_write_data(0x0A);
	__spi_write_data(0x1D);
	__spi_write_data(0x0F);
	__spi_write_data(0x3E);
	__spi_write_data(0x3E);
	__spi_write_data(0x0F);

	__spi_write_cmd(0x11);
	MDELAY(120);
	__spi_write_cmd(0x29);
	MDELAY(20);
}

static void lcm_init_power(void)
{

}

static void lcm_resume_power(void)
{
	LCD_DEBUG("%s\n",__func__);

	lcm_vpa_supply_enable();
	MDELAY(5);
	lcm_vrf18_supply_enable();
}

static void lcm_suspend_power(void)
{
	LCD_DEBUG("%s\n",__func__);

	lcm_vpa_supply_disable();
	MDELAY(5);
	lcm_vrf18_supply_disable();
}

static void lcm_init_lcm(void)
{

}

static void lcm_suspend(void)
{
	LCD_DEBUG("%s\n",__func__);

	lcm_suspend_power();
	MDELAY(10);
	lcm_reset(0);
	MDELAY(10);
#ifdef CONFIG_IFLYTEK_XFM10213
	xfm10213_change_state(true);
#endif
}

static void lcm_resume(void)
{
	LCD_DEBUG("%s\n",__func__);

	lcm_reset(1);
	MDELAY(10);

	init_lcm_registers();
#ifdef CONFIG_IFLYTEK_XFM10213
	xfm10213_change_state(false);
#endif
}

LCM_DRIVER itl9342c_qvganl_dpi_lcm_drv = {
	.name = "itl9342c_qvganl_dpi",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
};
