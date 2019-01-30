/*
 * Linux driver for temperature&humidity sensor
 *
 * Copyright (C) 2017.12, Malata XiaMen
 *
 * Author: Jason Zhao<xmzyw@malata.com>
 *
 */
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include "si7006.h"

/*----------------------------------------------------------------------------*/
#define SI7006_DEV_NAME "si7006"
static struct i2c_client * si70xx_i2c_client;
struct si70xx *si70xx_obj;
/*----------------------------------------------------------------------------*/
static int si70xx_measure(struct i2c_client *client, u8 command)
{
	union {
		u8  byte[4];
		s32 value;
	} data;

	/* Measure the humidity or temperature value */
	data.value = i2c_smbus_read_word_data(client, command);
	if (data.value < 0)
		return data.value;

	/* Swap the bytes and clear the status bits */
	return ((data.byte[0] * 256) + data.byte[1]) & ~3;
}
/*----------------------------------------------------------------------------*/
ssize_t si70xx_show_device_id(struct class *class, struct class_attribute *attr, char *buf)
{
        return sprintf(buf, "0x%02x\n", si70xx_obj->device_id);
}
/*----------------------------------------------------------------------------*/
ssize_t si70xx_show_temperature(struct class *class, struct class_attribute *attr, char *buf)
{
	int value;
	int temperature;

        /* Measure the temperature value */
        mutex_lock(&si70xx_obj->lock);
        value = si70xx_measure(si70xx_i2c_client, CMD_MEASURE_TEMPERATURE_HOLD);
        mutex_unlock(&si70xx_obj->lock);
        if (value < 0) 
                return value;

        /* Convert the value to millidegrees Celsius */
        temperature = ((value*21965)>>13)-46850;

        return sprintf(buf, "%d\n", temperature);
}
/*----------------------------------------------------------------------------*/
ssize_t si70xx_show_humidity(struct class *class, struct class_attribute *attr, char *buf)
{
        int value;
        int humidity;

        /* Measure the humidity value */
        mutex_lock(&si70xx_obj->lock);
        value = si70xx_measure(si70xx_i2c_client, CMD_MEASURE_HUMIDITY_HOLD);
        mutex_unlock(&si70xx_obj->lock);
        if (value < 0)
                return value;

        /* Convert the value to milli-percent (pcm) relative humidity */
        value = ((value*15625)>>13)-6000;

        /* Limit the humidity to valid values */
        if (value < 0)
                humidity = 0;
        else if (value > 100000)
                humidity = 100000;
        else
                humidity = value;

        return sprintf(buf, "%d\n", humidity);
}
/*----------------------------------------------------------------------------*/
static CLASS_ATTR(id,0664,si70xx_show_device_id,NULL);
static CLASS_ATTR(temperature,0664,si70xx_show_temperature,NULL);
static CLASS_ATTR(humidity,0664,si70xx_show_humidity,NULL);
/*----------------------------------------------------------------------------*/
static int si7006_create_class_attr(void)
{
        int ret;
        struct class *thtb_class;
        thtb_class = class_create(THIS_MODULE, "thtb");
        if (IS_ERR(thtb_class))
                return PTR_ERR(thtb_class);

        ret = class_create_file(thtb_class,&class_attr_id);
        if(ret)
                pr_err("class_create_file (%s) failed\n",(&class_attr_id)->attr.name);

        ret = class_create_file(thtb_class,&class_attr_temperature);
        if(ret)
                pr_err("class_create_file (%s) failed\n",(&class_attr_temperature)->attr.name);

        ret = class_create_file(thtb_class,&class_attr_humidity);
        if(ret)
                pr_err("class_create_file (%s) failed\n",(&class_attr_humidity)->attr.name);

        return ret;
}
/*----------------------------------------------------------------------------*/
static int si70xx_get_device_id(struct i2c_client *client, int *id)
{
	char buf[6];
	int  error;

	/* Put the 2-byte command into the buffer */
	buf[0] = 0xFC;
	buf[1] = 0xC9;

        pr_err("si7006:addr is 0x%02x\n",client->addr);

	/* Send the command */
	error = i2c_master_send(client, buf, 2);
	if (error < 0)
	       return error;

	/* Receive the 6-byte result */
	error = i2c_master_recv(client, buf, 6);
	if (error < 0)
		return error;

	/* Return the device ID */
	*id = buf[0];

	return 0;  /* Success */
}
/*----------------------------------------------------------------------------*/
static int si7006_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        int ret;
	struct si70xx *si70xx;

        pr_debug("%s\n",__func__);

        si70xx = kzalloc(sizeof(*si70xx), GFP_KERNEL);
        if (!si70xx)
                return -ENOMEM;

        /* Initialize the mutex */
        mutex_init(&si70xx->lock);

        /* Get the device ID from the device */
        mutex_lock(&si70xx->lock);
        ret = si70xx_get_device_id(client, &si70xx->device_id);
        mutex_unlock(&si70xx->lock);
        if (ret) {
                pr_err("si70xx_get_device_id failed\n");
                goto exit;
        }

        /* Validate the device ID */
        if ((si70xx->device_id != ID_SAMPLE) && 
            (si70xx->device_id != ID_SI7006) && 
            (si70xx->device_id != ID_SI7013) && 
            (si70xx->device_id != ID_SI7020) && 
            (si70xx->device_id != ID_SI7021)) {
                ret = -ENODEV;
                goto exit;
        }

        si70xx_i2c_client = client;
        si70xx_obj = si70xx;

        ret = si7006_create_class_attr();
        if(ret) {
                pr_err("si7006:create_class_node failed\n");
                goto exit;
        }

        return 0;
exit:
        kfree(si70xx);
        return ret;
}
/*----------------------------------------------------------------------------*/
static int si7006_i2c_remove(struct i2c_client *client)
{
        return 0;
}
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id si7006_i2c_id[] = {
	{SI7006_DEV_NAME,0},
	{}
};
static const struct of_device_id si7006_of_match[] = {
	{.compatible = "silicon,si7006",},
	{},
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver si7006_i2c_driver = {
	.probe      = si7006_i2c_probe,
	.remove     = si7006_i2c_remove,
	.id_table   = si7006_i2c_id,
	.driver = {
                .name           = SI7006_DEV_NAME,
                .of_match_table = si7006_of_match,
	},
};
/*----------------------------------------------------------------------------*/
static int __init si7006_init(void)
{
        i2c_add_driver(&si7006_i2c_driver);
        return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit si7006_exit(void)
{
        i2c_del_driver(&si7006_i2c_driver);
}
/*----------------------------------------------------------------------------*/
module_init(si7006_init);
module_exit(si7006_exit);
/*----------------------------------------------------------------------------*/
MODULE_DESCRIPTION("si7006 temperature&humidity sensor driver");
MODULE_AUTHOR("Jason Zhao<xmzyw@malata.com>");
MODULE_LICENSE("GPL");

