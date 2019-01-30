/*
 * Linux driver for iflytek xfm10213
 *
 * Copyright (C) 2018.3 Malata XiaMen
 *
 * Author: Jason Zhao<xmzyw@malata.com>
 *
 */
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/workqueue.h>
#include <linux/of_irq.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/string.h>
#include <linux/regulator/consumer.h>

#define XFM10213_DEV_NAME "xfm10213"
#define GAP 20  //ms
bool tablet_is_suspend;

unsigned char reg101[2]={0x01,0x01};        //cmd status
unsigned char reg102[2]={0x01,0x02};        //sys status
unsigned char reg103[2]={0x01,0x03};        //wakeup score
unsigned char reg10A[2]={0x01,0x0A};        //version
unsigned char reg110[2]={0x01,0x10};        //mode set
unsigned char reg111[2]={0x01,0x11};        //channel set
unsigned char reg112[2]={0x01,0x12};        //word change
unsigned char reg113[2]={0x01,0x13};        //wake up score thd
unsigned char reg114[2]={0x01,0x14};        //wake up punishment thd
unsigned char reg11A[2]={0x01,0x1A};        //output gain
unsigned char reg11B[2]={0x01,0x1B};        //i2s mode
unsigned char reg11C[2]={0x01,0x1C};        //uart level
unsigned char reg11D[2]={0x01,0x1D};        //uart speed

/*----------------------------------------------------------------------------*/
struct xfm10213_priv {
        bool init_done;

        int irq;

        unsigned int rst;

        struct input_dev *input;

        struct i2c_client *client;

        struct delayed_work wakeup_delay_work;
};
static struct xfm10213_priv *xfm10213_obj;
/*----------------------------------------------------------------------------*/
static int xfm10213_i2c_read(unsigned char reg[], unsigned char *val, unsigned char len)
{
        int ret;
        struct i2c_msg msg[2];
        struct i2c_adapter *adap = xfm10213_obj->client->adapter;

        if (!xfm10213_obj->client)
                return -EINVAL;

        msg[0].addr = xfm10213_obj->client->addr;
        msg[0].flags = 0;
        msg[0].len = 2;
        msg[0].buf = reg;

        msg[1].addr = xfm10213_obj->client->addr;
        msg[1].flags = I2C_M_RD;
        msg[1].len = len;
        msg[1].buf = val;

        ret = i2c_transfer(adap, msg, 2);
        return (ret == 2) ? 0 : ret;
}
/*----------------------------------------------------------------------------*/
static int xfm10213_i2c_write(unsigned char reg[], unsigned char *val, unsigned char len)
{
        int ret;
        struct i2c_msg msg[2];
        struct i2c_adapter *adap = xfm10213_obj->client->adapter;

        if (!xfm10213_obj->client)
                return -EINVAL;

        msg[0].addr = xfm10213_obj->client->addr;
        msg[0].flags = 0;
        msg[0].len = 2;
        msg[0].buf = reg;

        msg[1].addr = xfm10213_obj->client->addr;
        msg[1].flags = 0;
        msg[1].len = len;
        msg[1].buf = val;

        ret = i2c_transfer(adap, msg, 2);
        return (ret == 2) ? 0 : ret;
}
/*----------------------------------------------------------------------------*/
static int xfm10213_read(unsigned char reg[], unsigned int *val)
{
        int ret;
        unsigned char recvbuf[2];
        ret = xfm10213_i2c_read(reg,recvbuf,2);
        if (ret) {
                pr_err("xfm10213_i2c_read error:%d\n",ret);
                return ret;
        }

        *val=(((unsigned char)(recvbuf[0]))<<8) | ((unsigned char)(recvbuf[1]));

        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10213_write(unsigned char reg[],unsigned int value)
{
        int ret;
        int status;
        ret = xfm10213_i2c_write(reg,(unsigned char *)&value,2);
        if (ret) {
                pr_err("xfm10213_i2c_write error:%d\n",ret);
                return ret;
        }
        mdelay(GAP);
        ret = xfm10213_i2c_read(reg101,(unsigned char *)&status,2);
        if (ret) {
                pr_err("xfm10213_i2c_read error:%d\n",ret);
                return ret;
        }
        else if (status != 0x00) {
                pr_err("reg101:wrong status:%d\n",status);
                return status;
        }

        return 0;
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10213_show_version(struct class *class, struct class_attribute *attr, char *buf)
{
        unsigned int version;
        int ret;
        ret = xfm10213_read(reg10A,&version);
        if (ret)
                return sprintf(buf, "error:%d\n",ret);
        return sprintf(buf, "0x%04x\n", version);
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10213_show_reg(struct class *class, struct class_attribute *attr, char *buf)
{
        unsigned int status1,status2,status3;
        int ret;
        ret = xfm10213_read(reg110,&status1);
        if (ret)
                return sprintf(buf, "error:%d\n",ret);

        xfm10213_read(reg111,&status2);
        xfm10213_read(reg11A,&status3);

        return sprintf(buf, "reg110:0x%04x   reg111:0x%04x   reg11A:0x%04x\n", status1,status2,status3);
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10213_store_reg(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
        unsigned int reg, data_mode;

        if (2 == sscanf(buf, "%d %x", &reg, &data_mode)) {
		 if (reg == 0)
		       xfm10213_write(reg110,data_mode);
		 else if (reg == 1)
		       xfm10213_write(reg111,data_mode);
		 else if (reg == 2)
		       xfm10213_write(reg11A,data_mode);
        }

        return count;
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10213_show_rst(struct class *class, struct class_attribute *attr, char *buf)
{
        return sprintf(buf, "%d\n",gpio_get_value(xfm10213_obj->rst));
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10213_store_rst(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
        unsigned int value;
        if (1 == sscanf(buf, "%d", &value)) {
                gpio_direction_output(xfm10213_obj->rst, value);
        }
        return count;
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10213_show_thd(struct class *class, struct class_attribute *attr, char *buf)
{
        unsigned int thd;
        int ret;
        ret = xfm10213_read(reg113,&thd);
        if (ret)
                return sprintf(buf, "error:%d\n",ret);
        return sprintf(buf, "0x%04x\n", thd);
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10213_store_thd(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
        unsigned int value;
        if (1 == sscanf(buf, "%x", &value)) {
                xfm10213_write(reg113,value);
        }
        return count;
}
/*----------------------------------------------------------------------------*/
static CLASS_ATTR(version,0664,xfm10213_show_version,NULL);
static CLASS_ATTR(reg,0664,xfm10213_show_reg,xfm10213_store_reg);
static CLASS_ATTR(rst,0664,xfm10213_show_rst,xfm10213_store_rst);
static CLASS_ATTR(thd,0664,xfm10213_show_thd,xfm10213_store_thd);
/*----------------------------------------------------------------------------*/
static struct class_attribute *xfm10213_attr_list[] = {
        &class_attr_version,
        &class_attr_reg,
        &class_attr_rst,
        &class_attr_thd,
};
/*----------------------------------------------------------------------------*/
static int xfm10213_create_class_attr(void)
{
        int ret;
        int index,num;
        struct class *iflytek_class;

        iflytek_class = class_create(THIS_MODULE, "xfm10213");
        if (IS_ERR(iflytek_class))
                return PTR_ERR(iflytek_class);

        num = (int)(sizeof(xfm10213_attr_list)/sizeof(xfm10213_attr_list[0]));

        for (index = 0; index < num; index++) {
                ret = class_create_file(iflytek_class,xfm10213_attr_list[index]);
                if(ret) {
                        pr_err("%s:class_create_file (%s) fail:%d\n",__func__,xfm10213_attr_list[index]->attr.name,ret);
                        break;
                }
        }

        return ret;
}
/*----------------------------------------------------------------------------*/
void xfm10213_change_state(bool is_suspend)
{
        if (is_suspend)
                tablet_is_suspend = true;
        else
                tablet_is_suspend = false;
}
EXPORT_SYMBOL(xfm10213_change_state);
/*----------------------------------------------------------------------------*/
static void xfm10213_wakeup_work(struct work_struct *work)
{
        int ret;
        int sys_status,cmd_status;
        int wakeup_score;
        unsigned int data_mode = 0x0400;
        unsigned int data_chan = 0x2060;
        unsigned int uart_level = 0x0200;
        unsigned int uart_speed = 0x0024;
        unsigned int output_gain = 0x7777;
        //unsigned int i2s_mode = 0x1101;
        //unsigned int thd_value = 0x0007;

        ret = xfm10213_read(reg102,&sys_status);
        if (ret) {
                pr_err("xfm10213_read reg102 error:%d\n",ret);
                goto exit;
        }

        if (xfm10213_obj->init_done == false && sys_status == 0) {
                //xfm10213_write(reg11B,i2s_mode);
                xfm10213_write(reg110,data_mode);
                xfm10213_write(reg111,data_chan);
                xfm10213_write(reg11A,output_gain);
                xfm10213_write(reg11C,uart_level);
                xfm10213_write(reg11D,uart_speed);
                //xfm10213_write(reg113,thd_value);
                xfm10213_obj->init_done = true;
                goto exit;
        }

        ret = xfm10213_read(reg101,&cmd_status);
        if (ret) {
                pr_err("xfm10213_read reg101 error:%d\n",ret);
                goto exit;
        }
        else if (cmd_status != 0x00) {
                pr_err("reg101 wrong status:%d\n",cmd_status);
                goto exit;
        }

        if (sys_status == 0x03) {
                ret = xfm10213_read(reg103,&wakeup_score);
                if (ret) {
                        pr_err("xfm10213_read error:%d\n",ret);
                        goto exit;
                }
                pr_debug("%s:reg103 wakeup score:%d\n",__func__, wakeup_score);

                if (xfm10213_obj->input == NULL)
                        goto exit;

                if (tablet_is_suspend) {
                        //wake up system & open voice assistant
                        input_report_key(xfm10213_obj->input, KEY_MIC_EVENT1, 1);
                        input_sync(xfm10213_obj->input);
                        input_report_key(xfm10213_obj->input, KEY_MIC_EVENT1, 0);
                        input_sync(xfm10213_obj->input);
                }
                else {
                        //just open voice assistant
                        input_report_key(xfm10213_obj->input, KEY_MIC_EVENT2, 1);
                        input_sync(xfm10213_obj->input);
                        input_report_key(xfm10213_obj->input, KEY_MIC_EVENT2, 0);
                        input_sync(xfm10213_obj->input);
                }
        }
exit:
        enable_irq(xfm10213_obj->irq);
}
/*----------------------------------------------------------------------------*/
static irqreturn_t xfm10213_wakeup_handler(int irq, void *dev)
{
        disable_irq_nosync(irq);
        schedule_delayed_work(&xfm10213_obj->wakeup_delay_work, msecs_to_jiffies(10));
        return IRQ_HANDLED;
}
/*----------------------------------------------------------------------------*/
static int xfm10213_setup_eint(struct device *dev)
{
        int ret;
        int debounce;
        int gpiopin;
        struct pinctrl * pinctrl;
        struct pinctrl_state * pins_eint;
        struct pinctrl_state * i2s0_bck;
        struct pinctrl_state * i2s0_lrck;
        struct pinctrl_state * i2s0_data;
        struct device_node *irq_node;

        pinctrl = devm_pinctrl_get(dev);
        if (IS_ERR(pinctrl)) {
                ret = PTR_ERR(pinctrl);
                pr_err("Cannot find alsps pinctrl!\n");
                return ret;
        }

        pins_eint = pinctrl_lookup_state(pinctrl, "pins_eint");
        if (IS_ERR(pins_eint)) {
                ret = PTR_ERR(pins_eint);
                pr_err("Cannot find pins_eint!\n");
        }

        i2s0_bck = pinctrl_lookup_state(pinctrl, "i2s0_bck");
        if (IS_ERR(i2s0_bck)) {
                ret = PTR_ERR(i2s0_bck);
                pr_err("Cannot find i2s0_bck!\n");
        }

        i2s0_lrck = pinctrl_lookup_state(pinctrl, "i2s0_lrck");
        if (IS_ERR(i2s0_lrck)) {
                ret = PTR_ERR(i2s0_lrck);
                pr_err("Cannot find i2s0_lrck!\n");
        }

        i2s0_data = pinctrl_lookup_state(pinctrl, "i2s0_data");
        if (IS_ERR(i2s0_data)) {
                ret = PTR_ERR(i2s0_data);
                pr_err("Cannot find i2s0_data!\n");
        }

        ret = pinctrl_select_state(pinctrl, pins_eint);
        if (ret)
                pr_err("could not set pins_eint pins\n");

        ret = pinctrl_select_state(pinctrl, i2s0_bck);
        if (ret)
                pr_err("could not set i2s0_bck pins\n");

        ret = pinctrl_select_state(pinctrl, i2s0_lrck);
        if (ret)
                pr_err("could not set i2s0_lrck pins\n");       

        ret = pinctrl_select_state(pinctrl, i2s0_data);
        if (ret)
                pr_err("could not set i2s0_data pins\n");

        irq_node = of_find_compatible_node(NULL, NULL, "iflytek,xfm10213");
        if (!irq_node)
		return -EINVAL;

        gpiopin = of_get_named_gpio(irq_node, "int-gpio", 0);
        if (gpiopin < 0)
                pr_err("Not find int-gpio\n");

        of_property_read_u32(irq_node, "eint-debounce", &debounce);

        ret = gpio_request(gpiopin, "xfmeint-gpio");
        if (ret)
                pr_err("gpio_request fail, ret(%d)\n", ret);

        gpio_direction_input(gpiopin);
        gpio_set_debounce(gpiopin, debounce);

        xfm10213_obj->irq = irq_of_parse_and_map(irq_node, 0);
        if (!xfm10213_obj->irq) {
                pr_err("gpio_to_irq fail!!\n");
                return -EINVAL;
        }

        ret = request_irq(xfm10213_obj->irq, xfm10213_wakeup_handler, IRQ_TYPE_EDGE_RISING, "xfm10213_eint", NULL);
        if(ret) {
                pr_err("request_irq fail:%d\n",ret);
                return ret;
        }

        enable_irq_wake(xfm10213_obj->irq);
        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10213_input_init(struct device *dev)
{
        xfm10213_obj->input = input_allocate_device();
        if(!xfm10213_obj->input) {
                pr_err("%s:allocate input device failed\n",__func__);
                return -ENOMEM;
        }

        __set_bit(EV_KEY, (xfm10213_obj->input)->evbit);
        __set_bit(KEY_MIC_EVENT1, (xfm10213_obj->input)->keybit);
        __set_bit(KEY_MIC_EVENT2, (xfm10213_obj->input)->keybit);

        (xfm10213_obj->input)->name = "xfm10213";
        (xfm10213_obj->input)->id.bustype = BUS_I2C;
        if(input_register_device(xfm10213_obj->input)) {
                pr_err("%s:register input device failed\n",__func__);
                return -EINVAL;
        }

        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10213_power_init(struct device *dev)
{
	int ret;
	struct regulator *ldo_vgp3;
	struct regulator *ldo_sim1;
	struct regulator *ldo_sim2;

	ldo_vgp3 = devm_regulator_get(dev, "reg-vgp3");
	if (IS_ERR(ldo_vgp3)) {
		ret = PTR_ERR(ldo_vgp3);
		dev_err(dev, "failed to get reg-vgp3, %d\n", ret);
		return ret;
	}

	ldo_sim1 = devm_regulator_get(dev, "reg-vsim1");
	if (IS_ERR(ldo_sim1)) {
		ret = PTR_ERR(ldo_sim1);
		dev_err(dev, "failed to get reg-vsim1, %d\n", ret);
		return ret;
	}

	ldo_sim2 = devm_regulator_get(dev, "reg-vsim2");
	if (IS_ERR(ldo_sim2)) {
		ret = PTR_ERR(ldo_sim2);
		dev_err(dev, "failed to get reg-vsim2, %d\n", ret);
		return ret;
	}

	ret = regulator_set_voltage(ldo_vgp3, 1200000, 1200000);
	if (ret) {
		dev_err(dev, "failed to set reg-vgp3 voltage, %d\n", ret);
		return ret;
	}

	ret = regulator_set_voltage(ldo_sim1, 3000000, 3000000);
	if (ret) {
		dev_err(dev, "failed to set reg-vsim1 voltage, %d\n", ret);
		return ret;
	}

	ret = regulator_set_voltage(ldo_sim2, 1800000, 1800000);
	if (ret) {
		dev_err(dev, "failed to set reg-vsim2 voltage, %d\n", ret);
		return ret;
	}

	ret = regulator_enable(ldo_vgp3);
	if (ret) {
		dev_err(dev, "failed to enable ldo_vgp3: %d\n", ret);
		return ret;
	}

	ret = regulator_enable(ldo_sim1);
	if (ret) {
		dev_err(dev, "failed to enable ldo_sim1: %d\n", ret);
		return ret;
	}

	ret = regulator_enable(ldo_sim2);
	if (ret) {
		dev_err(dev, "failed to enable ldo_sim2: %d\n", ret);
		return ret;
	}

	return ret;
}
/*----------------------------------------------------------------------------*/
static int xfm10213_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        int ret;

        xfm10213_obj = devm_kzalloc(&client->dev, sizeof(struct xfm10213_priv), GFP_KERNEL);
        if (!xfm10213_obj) {
                pr_err("%s:allocate memory for xfm10213 failed\n",__func__);
                return -ENOMEM;
        }

        memset(xfm10213_obj,0,sizeof(struct xfm10213_priv));

        i2c_set_clientdata(client, xfm10213_obj);
        xfm10213_obj->client = client;
        xfm10213_obj->init_done = false;

        INIT_DELAYED_WORK(&xfm10213_obj->wakeup_delay_work, xfm10213_wakeup_work);

        //get and pull down reset pin
        xfm10213_obj->rst = of_get_named_gpio((&client->dev)->of_node, "rst-gpio", 0);
        gpio_request(xfm10213_obj->rst, "iflytek_rst");
        gpio_direction_output(xfm10213_obj->rst, 0);

        ret = xfm10213_power_init(&client->dev);
        if (ret) {
                pr_err("%s:xfm10213_power_init failed\n",__func__);
                return ret;
        }

        ret = xfm10213_setup_eint(&client->dev);
        if (ret) {
                pr_err("%s:xfm10213_setup_eint failed\n",__func__);
                return ret;
        }

        ret = xfm10213_input_init(&client->dev);
        if (ret) {
                pr_err("%s:xfm10213_input_init failed\n",__func__);
                return ret;
        }

        //for debug
        ret = xfm10213_create_class_attr();
        if (ret) {
                pr_err("%s:xfm10213_create_class_attr failed\n",__func__);
                return ret;
        }

        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10213_i2c_remove(struct i2c_client *client)
{
        return 0;
}
/*----------------------------------------------------------------------------*/
static void xfm10213_i2c_shutdown(struct i2c_client *client)
{
        gpio_direction_output(xfm10213_obj->rst, 0);
}
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id xfm10213_i2c_id[] = {
	{XFM10213_DEV_NAME, 0},
	{}
};
#ifdef CONFIG_OF
static const struct of_device_id xfm10213_of_match[] = {
	{.compatible = "iflytek,xfm10213"},
	{},
};
#endif
/*----------------------------------------------------------------------------*/
static struct i2c_driver xfm10213_driver = {
	.probe = xfm10213_i2c_probe,
	.remove = xfm10213_i2c_remove,
	.shutdown = xfm10213_i2c_shutdown,
	.id_table = xfm10213_i2c_id,
	.driver = {
                .name = XFM10213_DEV_NAME,
#ifdef CONFIG_OF
                .of_match_table =xfm10213_of_match,
#endif
	},
};
/*----------------------------------------------------------------------------*/
static int __init xfm10213_init(void)
{
        pr_notice("%s\n",__func__);
        return i2c_add_driver(&xfm10213_driver);
}
/*----------------------------------------------------------------------------*/
static void __exit xfm10213_exit(void)
{
        i2c_del_driver(&xfm10213_driver);
}
/*----------------------------------------------------------------------------*/
module_init(xfm10213_init);
module_exit(xfm10213_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Jason Zhao<xmzyw@malata.com>");
MODULE_DESCRIPTION("iflytek xfm10213 driver");
MODULE_LICENSE("GPL");

