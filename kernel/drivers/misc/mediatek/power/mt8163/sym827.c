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

#include <cust_acc.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>
#include <linux/xlog.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <mach/mt_idle.h>
#include <mach/mt_spm_idle.h>
#include <mach/mt_sleep.h>

#include <mach/sym827.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define SYM827_SLAVE_ADDR_WRITE   0xc0
#define SYM827_SLAVE_ADDR_READ    0xc1

#define SYM827_MIN_MV	600000
#define SYM827_STEP_MV	12500


#ifdef I2C_EXT_BUCK_CHANNEL
#define SYM827_BUSNUM I2C_EXT_BUCK_CHANNEL
#else
#define SYM827_BUSNUM 1
#endif
#define SYM827_TEST_NODE

#ifdef GPIO_EXT_BUCK_VSEL_PIN
unsigned int g_vproc_vsel_gpio_number = GPIO_EXT_BUCK_VSEL_PIN;
#else
unsigned int g_vproc_vsel_gpio_number = 0;
#endif

void ext_buck_vproc_vsel(int val)
{
	sym827_buck_set_switch(val);
}

static struct i2c_client *new_client;
static const struct i2c_device_id sym827_i2c_id[] = { {"sym827", 0}, {} };

static int sym827_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);
#ifdef CONFIG_OF
static const struct of_device_id sym827_of_match[] = {
	{.compatible = "silergy,sym827",},
	{},
};

MODULE_DEVICE_TABLE(of, sym827_of_match);
#endif
static struct i2c_driver sym827_driver = {
	.driver = {
		   .name = "sym827",
#ifdef CONFIG_OF
		   .of_match_table = of_match_ptr(sym827_of_match),
#endif
		   },
	.probe = sym827_driver_probe,
	.id_table = sym827_i2c_id,
};

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
static DEFINE_MUTEX(sym827_i2c_access);

int g_sym827_driver_ready = 0;
int g_sym827_hw_exist = 0;
/**********************************************************
  *
  *   [I2C Function For Read/Write sym827]
  *
  *********************************************************/
#if 0
int sym827_read_byte(kal_uint8 cmd, kal_uint8 *data)
{
	int ret;

	struct i2c_msg msg[2];

	msg[0].addr = new_client->addr;
	msg[0].buf = &cmd;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[1].addr = new_client->addr;
	msg[1].buf = data;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;

	ret = i2c_transfer(new_client->adapter, msg, 2);

	if (ret != 2)
		pr_err("%s: err=%d\n", __func__, ret);

	return ret == 2 ? 1 : 0;
}

int sym827_write_byte(kal_uint8 cmd, kal_uint8 data)
{
	char buf[2];
	int ret;

	buf[0] = cmd;
	buf[1] = data;

	ret = i2c_master_send(new_client, buf, 2);

	if (ret != 2)
		pr_err("%s: err=%d\n", __func__, ret);

	return ret == 2 ? 1 : 0;
}
#else
int sym827_read_byte(kal_uint8 cmd, kal_uint8 *data)
{
    char     cmd_buf[1]={0x00};
    char     readData = 0;
    int      ret=0;

    mutex_lock(&sym827_i2c_access);
    
    //new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;    
    new_client->ext_flag = ((new_client->ext_flag ) & I2C_MASK_FLAG ) |
	I2C_WR_FLAG | I2C_RS_FLAG | I2C_DIRECTION_FLAG | I2C_HS_FLAG;
	new_client->timing = 1500;
    cmd_buf[0] = cmd;
    ret = i2c_master_send(new_client, &cmd_buf[0], (1<<8 | 1));
    if (ret < 0) 
    {    
        //new_client->addr = new_client->addr & I2C_MASK_FLAG;
        new_client->ext_flag=0;
		
        mutex_unlock(&sym827_i2c_access);
        return 0;
    }
    
    readData = cmd_buf[0];
    *data = readData;

    //new_client->addr = new_client->addr & I2C_MASK_FLAG;
    new_client->ext_flag=0;
	
    mutex_unlock(&sym827_i2c_access);    
    return 1;
}

int sym827_write_byte(kal_uint8 cmd, kal_uint8 data)
{
    char    write_data[2] = {0};
    int     ret=0;
    
    mutex_lock(&sym827_i2c_access);
    
    write_data[0] = cmd;
    write_data[1] = data;

    new_client->ext_flag = ((new_client->ext_flag ) & I2C_MASK_FLAG ) |
		I2C_DIRECTION_FLAG | I2C_HS_FLAG;
	new_client->timing = 1500;
    ret = i2c_master_send(new_client, write_data, 2);
    if (ret < 0) 
    {
        new_client->ext_flag=0;    
        mutex_unlock(&sym827_i2c_access);
        return 0;
    }

    new_client->ext_flag=0;    
    mutex_unlock(&sym827_i2c_access);
    return 1;
}
#endif
/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
kal_uint32 sym827_read_interface(kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 sym827_reg = 0;
	int ret = 0;

	ret = sym827_read_byte(RegNum, &sym827_reg);
	sym827_reg &= (MASK << SHIFT);
	*val = (sym827_reg >> SHIFT);

	return ret;
}

kal_uint32 sym827_config_interface(kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 sym827_reg = 0;
	int ret = 0;

	ret = sym827_read_byte(RegNum, &sym827_reg);
	sym827_reg &= ~(MASK << SHIFT);
	sym827_reg |= (val << SHIFT);

	ret = sym827_write_byte(RegNum, sym827_reg);

	return ret;
}

kal_uint32 sym827_get_reg_value(kal_uint32 reg)
{
	kal_uint32 ret = 0;
	kal_uint8 reg_val = 0;

	ret = sym827_read_interface((kal_uint8) reg, &reg_val, 0xFF, 0x0);

	return reg_val;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void sym827_dump_register(void)
{
	kal_uint8 i = 0;

	for (i = SYM827_REG_VSEL_0; i <= SYM827_REG_PGOOD; i++)
		pr_err("[sym827] [0x%x]=0x%x\n", i, sym827_get_reg_value(i));

}

int sym827_buck_power_on()
{
	int ret = 0;

	if (!is_sym827_sw_ready()) {
		pr_err("%s: sym827 driver is not exist\n", __func__);
		return 0;
	}
	if (!is_sym827_exist()) {
		pr_err("%s: sym827 is not exist\n", __func__);
		return 0;
	}

	ret = sym827_config_interface(SYM827_REG_VSEL_0,
			SYM827_BUCK_ENABLE, 0x1, SYM827_BUCK_EN0_SHIFT);
	ret = sym827_config_interface(SYM827_REG_VSEL_1,
			SYM827_BUCK_ENABLE, 0x1, SYM827_BUCK_EN0_SHIFT);

	return ret;
}

int sym827_buck_power_off()
{
	int ret = 0;

	if (!is_sym827_sw_ready()) {
		pr_err("%s: sym827 driver is not exist\n", __func__);
		return 0;
	}
	if (!is_sym827_exist()) {
		pr_err("%s: sym827 is not exist\n", __func__);
		return 0;
	}

	ret = sym827_config_interface(SYM827_REG_VSEL_0,
			SYM827_BUCK_DISABLE, 0x1, SYM827_BUCK_EN0_SHIFT);
	ret = sym827_config_interface(SYM827_REG_VSEL_1,
			SYM827_BUCK_DISABLE, 0x1, SYM827_BUCK_EN0_SHIFT);

	return ret;
}

int sym827_buck_get_state()
{
	int ret = 0;
	kal_uint8 reg = 0;
	kal_uint8 val = 0;

	if (!is_sym827_sw_ready()) {
		pr_err("%s: sym827 driver is not exist\n", __func__);
		return 0;
	}
	if (!is_sym827_exist()) {
		pr_err("%s: sym827 is not exist\n", __func__);
		return 0;
	}

	ret = sym827_read_interface(SYM827_REG_VSEL_0, &val, 0x1, SYM827_BUCK_EN0_SHIFT);

	return val;
}

void sym827_hw_init(void)
{
	kal_uint32 ret = 0;
}

void sym827_hw_component_detect(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = sym827_read_interface(SYM827_REG_ID_1, &val, 0x07, SYM827_ID_VENDOR_SHIFT);

	/* check default SPEC. value */
	if (val == 0x04)
		g_sym827_hw_exist = 1;
	else
		g_sym827_hw_exist = 0;

	ret = sym827_read_interface(SYM827_REG_ID_1, &val, 0x0F, 0x0);

	if (ret) {
		pr_err("%s: DIE_ID = %d.\n", __func__, val);
		if (!val) {
			slp_cpu_dvs_en(0);
			set_slp_spm_deepidle_flags(0);
			spm_sodi_cpu_dvs_en(0);
			g_vproc_vsel_gpio_number = 0;
		} else {
			if (g_vproc_vsel_gpio_number > 0) {
				mt_set_gpio_mode(g_vproc_vsel_gpio_number, GPIO_MODE_00);
				mt_set_gpio_dir(g_vproc_vsel_gpio_number, GPIO_DIR_OUT);
				mt_set_gpio_out(g_vproc_vsel_gpio_number, GPIO_OUT_ONE);
			} else {
				slp_cpu_dvs_en(0);
				set_slp_spm_deepidle_flags(0);
				spm_sodi_cpu_dvs_en(0);
				g_vproc_vsel_gpio_number = 0;
			}
		}
	} else {
		pr_err("%s: sym827_read_interface failed\n", __func__);
	}
}

int is_sym827_sw_ready(void)
{
	return g_sym827_driver_ready;
}

int is_sym827_exist(void)
{
	return g_sym827_hw_exist;
}

int sym827_buck_set_mode(int mode)
{
	int ret = 0;
	kal_uint8 reg = 0;

	if (!is_sym827_sw_ready()) {
		pr_err("%s: sym827 driver is not exist\n", __func__);
		return 0;
	}
	if (!is_sym827_exist()) {
		pr_err("%s: sym827 is not exist\n", __func__);
		return 0;
	}

	ret = sym827_config_interface(SYM827_REG_VSEL_0,
			mode, 0x1, SYM827_BUCK_MODE_SHIFT);
	ret = sym827_config_interface(SYM827_REG_VSEL_1,
			mode, 0x1, SYM827_BUCK_MODE_SHIFT);

	return ret;
}

int sym827_buck_get_mode()
{
	int ret = 0;
	kal_uint8 reg = 0;
	kal_uint8 val = 0;

	if (!is_sym827_sw_ready()) {
		pr_err("%s: sym827 driver is not exist\n", __func__);
		return 0;
	}
	if (!is_sym827_exist()) {
		pr_err("%s: sym827 is not exist\n", __func__);
		return 0;
	}

	ret = sym827_read_interface(SYM827_REG_VSEL_0, &val, 0x1, SYM827_BUCK_MODE_SHIFT);
	return val;
}

int sym827_buck_set_switch(int val)
{
	int ret = 0;
	kal_uint8 reg = 0;

	return ret;
}

int sym827_buck_get_switch()
{
	int ret = 0;

	return ret;
}

int sym827_buck_set_voltage(unsigned long voltage)
{
	int ret = 1;
	int vol_sel = 0;
	kal_uint8 reg = 0;

	/* 600mV~13875mV, step=12.5mV */
	/*
	ret = sym827_read_interface(SYM827_REG_VSEL_0, &reg, 0x80, 0x0);
	if (!ret) {
		pr_err("sym827 I2C read failed, do noting...\n");
		return ret;
	}
	*/
	if (!g_sym827_driver_ready) {
		pr_err("%s : g_sym827_driver_ready : %d\n", __func__, g_sym827_driver_ready);
		return 0;
	}
		
	if (g_vproc_vsel_gpio_number)
		reg = SYM827_REG_VSEL_1;
	else
		reg = SYM827_REG_VSEL_0;

	vol_sel = ((voltage) - SYM827_MIN_MV) / SYM827_STEP_MV;
	if (vol_sel > 63)
		vol_sel = 63;

	ret = sym827_write_byte(reg, vol_sel | 0x80);

	pr_debug("sym827 voltage = %lu, reg = %d ,vol_sel = %x, get = %x\n",
		voltage, reg, vol_sel, sym827_get_reg_value(reg));

	return ret;
}

unsigned long sym827_buck_get_voltage()
{
	kal_uint8 vol_sel = 0;
	kal_uint8 reg = 0;
	unsigned long voltage = 0;

	if (!g_sym827_driver_ready) {
		pr_err("%s : g_sym827_driver_ready : %d\n", __func__, g_sym827_driver_ready);
		return 0;
	}

	if (g_vproc_vsel_gpio_number)
		reg = SYM827_REG_VSEL_1;
	else
		reg = SYM827_REG_VSEL_0;

	sym827_read_interface(reg, &vol_sel,
		0x3F, SYM827_BUCK_NSEL_SHIFT);
	voltage = (SYM827_MIN_MV + vol_sel * SYM827_STEP_MV);
	return voltage;
}

#ifdef CONFIG_OF
static int of_get_sym827_platform_data(struct device *dev)
{
	int ret, num;

	if (dev->of_node) {
		const struct of_device_id *match;
		match = of_match_device(of_match_ptr(sym827_of_match), dev);
		if (!match) {
			dev_err(dev, "Error: No device match found\n");
			return -ENODEV;
		}
	}
	#if 0
	ret = of_property_read_u32(dev->of_node, "vbuck-gpio", &num);
	if (!ret)
		g_vproc_vsel_gpio_number = num;
	#endif
	dev_err(dev, "g_vproc_vsel_gpio_number %u\n", g_vproc_vsel_gpio_number);
	return 0;
}
#else
static int of_get_sym827_platform_data(struct device *dev)
{
	return 0;
}
#endif
#ifdef SYM827_TEST_NODE
unsigned int reg_value_sym827 = 0;
static ssize_t show_sym827_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", reg_value_sym827);
}

static ssize_t store_sym827_access(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	if (buf != NULL && size != 0) {
		reg_address = simple_strtoul(buf, &pvalue, 16);

		if (size > 4) {
			reg_value = simple_strtoul((pvalue + 1), NULL, 16);
			ret = sym827_config_interface(reg_address, reg_value, 0xFF, 0x0);
		} else {
			ret = sym827_read_interface(reg_address, &reg_value_sym827, 0xFF, 0x0);
		}
	}
	return size;
}
static DEVICE_ATTR(access, 0664, show_sym827_access, store_sym827_access);
#endif
static int sym827_driver_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	int err = 0;
	dev_err(&i2c->dev, "[sym827_driver_probe]\n");
	of_get_sym827_platform_data(&i2c->dev);
	new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!new_client) {
		err = -ENOMEM;
		goto exit;
	}
	memset(new_client, 0, sizeof(struct i2c_client));

	new_client = i2c;

	sym827_hw_component_detect();

	if (g_sym827_hw_exist == 1) {
		sym827_hw_init();
		//sym827_dump_register();
	}
	g_sym827_driver_ready = 1;

	if (g_sym827_hw_exist == 0) {
		dev_err(&i2c->dev, "[sym827_driver_probe] return err\n");
		return err;
	}
#ifdef SYM827_TEST_NODE
	/* Debug sysfs */
	device_create_file(&(i2c->dev), &dev_attr_access);
#endif
	return 0;

 exit:
	return err;

}

static struct i2c_board_info __initdata i2c_sym827 = { I2C_BOARD_INFO("sym827", (SYM827_SLAVE_ADDR_WRITE >> 1))};
static int __init sym827_init(void)
{
	pr_err("sym827 init start. ch=%d\n", SYM827_BUSNUM);

	i2c_register_board_info(SYM827_BUSNUM, &i2c_sym827, 1);

	if (i2c_add_driver(&sym827_driver) != 0)
		pr_err("failed to register sym827 i2c driver.\n");
	else
		pr_info("Success to register sym827 i2c driver.\n");

	return 0;
}

static void __exit sym827_exit(void)
{
	i2c_del_driver(&sym827_driver);
}
module_init(sym827_init);
module_exit(sym827_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C sym827 Driver");
