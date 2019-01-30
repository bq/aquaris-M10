/*
 * Linux driver for iflytek xfm10412
 *
 * Copyright (C) 2018.1 Malata XiaMen
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
#include <cust_eint.h>
#include <mach/mt_gpio.h>
#include <sound/soc.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

static DECLARE_WAIT_QUEUE_HEAD(wakeup_wait);
static DEFINE_SEMAPHORE(sem_wakeup);

static bool block_wait = true;
#define XFM10412_DEV_NAME "xfm10412"
static bool tablet_is_suspend;
static DEFINE_MUTEX(read_lock);
#define USE_INPUT_DEVICE
//#define USE_CODEC_FUNC
/*----------------------------------------------------------------------------*/
struct xfm10412_priv {
        u32 version;
        u32 build;
        u16 degree;
        u16 degree_pre;
        u8 reset;
        u8 dir;

        int irq;

        struct input_dev *input;

        struct i2c_client *client;

        struct delayed_work wakeup_delay_work;
};
static struct xfm10412_priv *xfm10412_obj;
/*----------------------------------------------------------------------------*/
static int xfm10412_i2c_read_reg_32bits(u8 reg, u32 *regValue)
{
        u8 buf[1],recvbuf[4];
        int ret = 0;

        if(!xfm10412_obj->client)
                return -EINVAL;

        mutex_lock(&read_lock);
        buf[0]= reg;
        ret = i2c_master_send(xfm10412_obj->client, buf, 0x1);
        if(ret <= 0) {
                pr_err("%s:read reg send res = %d\n",__func__,ret);
                return ret;
        }

        ret = i2c_master_recv(xfm10412_obj->client, recvbuf, 0x4);
        if(ret <= 0) {
                pr_err("%s:read reg recv res = %d\n",__func__,ret);
                return ret;
        }

        *regValue=(((u32)(recvbuf[3]))<<24) |(((u32)(recvbuf[2]))<<16)|(((u32)(recvbuf[1]))<<8) | ((u32)(recvbuf[0]));
        mutex_unlock(&read_lock);

        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10412_i2c_write_reg_32bits(u8 reg, u32 value)
{
        u8 buf[5];
        int ret = 0;

        if(!xfm10412_obj->client)
                return -EINVAL;

        buf[0] = reg;
        buf[1] = value & 0xff;
        buf[2] = value >> 8;
        buf[3] = value >> 16;
        buf[4] = value >> 24;

        ret = i2c_master_send(xfm10412_obj->client, buf, 0x5);
        if(ret <= 0) {
                pr_err("%s:wirte reg send res = %d\n",__func__,ret);
                return ret;
        }

        return 0;
}
/*----------------------------------------------------------------------------*/
unsigned int xfm10412_send_cmd(u32 cmd, u32 status)
{
        int ret,status_temp;
        int count = 0;
        ret = xfm10412_i2c_write_reg_32bits(0x00,cmd);
        if(ret)
                return ret;
        mdelay(1);

        do {
                ret = xfm10412_i2c_read_reg_32bits(0x00,&status_temp);
                if(ret)
                        return ret;
                mdelay(1);
                count++;
                if(count == 5) {
                        pr_err("%s:timeout\n",__func__);
                        return -1;
                }
        } while(status_temp != status);

        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10412_get_version(void)
{
        u32 cmd = 0x00000F00;
        u32 status = 0x00020001;

        if(xfm10412_send_cmd(cmd,status))
                return -1;

        xfm10412_i2c_read_reg_32bits(0x01,&xfm10412_obj->version);
        xfm10412_i2c_read_reg_32bits(0x02,&xfm10412_obj->build);
        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10412_get_degree(void)
{
        u32 cmd = 0x00001000;
        u32 status = 0x00010001;

        if(xfm10412_send_cmd(cmd,status))
                return -1;

        xfm10412_obj->degree = xfm10412_obj->degree_pre;
        xfm10412_i2c_read_reg_32bits(0x01,&xfm10412_obj->degree);
        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10412_reset(void)
{
        u32 cmd = 0x00001100;
        u32 status = 0x00000001;

        if(xfm10412_send_cmd(cmd,status))
                return -1;

        xfm10412_obj->reset = 1;
        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10412_set_voicedir(u8 value)
{
        u32 cmd = 0x00001200;
        u32 status = 0x00030001;

        if(!(value >= 0x00 && value <= 0x05)) {     //value should be 0x00~0x05.
                pr_err("%s:do not support %d dir value,set 0x00 by default\n", __func__, value);
                value = 0x00;
        }

        cmd |= (value<<16);

        xfm10412_obj->dir = value;
        xfm10412_obj->reset = 0;

        if(xfm10412_send_cmd(cmd,status))
                return -1;

        return 0;
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10412_show_version(struct class *class, struct class_attribute *attr, char *buf)
{
        xfm10412_get_version();
        return sprintf(buf, "Version : 0x%08X  Build : 0x%08X\n", xfm10412_obj->version,xfm10412_obj->build);
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10412_show_degree(struct class *class, struct class_attribute *attr, char *buf)
{
        int ret;
        //wait_event_interruptible(wakeup_wait, block_wait == false);

        //if(down_interruptible(&sem_wakeup)) {
        //        block_wait = true;
        //        return -EFAULT;
        //}
        xfm10412_get_degree();
        ret = sprintf(buf, "degree: %d\n", xfm10412_obj->degree);
        //block_wait = true;

        //up(&sem_wakeup);
        return ret;
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10412_show_dir(struct class *class, struct class_attribute *attr, char *buf)
{
        return sprintf(buf, "voice direction:0x%02x\n", xfm10412_obj->dir);
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10412_store_dir(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
        int dir;
        sscanf(buf, "%x", &dir);
        xfm10412_set_voicedir(dir);
        return count;
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10412_show_reset(struct class *class, struct class_attribute *attr, char *buf)
{
        return sprintf(buf, "reset status:%d\n", xfm10412_obj->reset);
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10412_store_reset(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
        int enable;
        sscanf(buf, "%d", &enable);
        if(enable)
                xfm10412_reset();
        return count;
}
/*----------------------------------------------------------------------------*/
static CLASS_ATTR(version,0644,xfm10412_show_version,NULL);
static CLASS_ATTR(degree,0644,xfm10412_show_degree,NULL);
static CLASS_ATTR(dir,0644,xfm10412_show_dir,xfm10412_store_dir);
static CLASS_ATTR(reset,0644,xfm10412_show_reset,xfm10412_store_reset);
/*----------------------------------------------------------------------------*/
static struct class_attribute *xfm10412_attr_list[] = {
        &class_attr_version,
        &class_attr_degree,
        &class_attr_dir,
        &class_attr_reset,
};
/*----------------------------------------------------------------------------*/
static int xfm10412_create_class_attr(void)
{
        int ret;
        int index,num;
        struct class *iflytek_class;
        iflytek_class = class_create(THIS_MODULE, "xfm10412");
        if (IS_ERR(iflytek_class))
                return PTR_ERR(iflytek_class);

        num = (int)(sizeof(xfm10412_attr_list)/sizeof(xfm10412_attr_list[0]));

        for(index = 0; index < num; index++) {
                ret = class_create_file(iflytek_class,xfm10412_attr_list[index]);
                if(ret) {
                        pr_err("%s:class_create_file (%s) fail:%d\n",__func__,xfm10412_attr_list[index]->attr.name,ret);
                        break;
                }
        }

        return ret;
}
/*----------------------------------------------------------------------------*/
#ifdef USE_CODEC_FUNC
static int xfm10412_set_dai_fmt(struct snd_soc_dai *dai, unsigned int format)
{
	pr_debug("%s\n",__func__);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10412_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
			     struct snd_soc_dai *dai)
{
	pr_debug("%s\n",__func__);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10412_set_bias_level(struct snd_soc_codec *codec,
				  enum snd_soc_bias_level level)
{
	pr_debug("%s\n",__func__);
	return 0;
}
/*----------------------------------------------------------------------------*/
static const struct snd_soc_codec_driver xfm10412_codec = {	
	.set_bias_level = xfm10412_set_bias_level,
	.idle_bias_off = true,
};

static const struct snd_soc_dai_ops xfm10412_dai_ops = {
	.set_fmt	= xfm10412_set_dai_fmt,
	.hw_params	= xfm10412_hw_params,
};

static struct snd_soc_dai_driver xfm10412_dai = {
	.name = "iflytek-xfm10412",
	.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
	.ops = &xfm10412_dai_ops,
};
#endif
/*----------------------------------------------------------------------------*/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void iflytek_early_suspend(struct early_suspend *h)
{
        tablet_is_suspend = true;
}
/*----------------------------------------------------------------------------*/
static void iflytek_early_resume(struct early_suspend *h)
{
        tablet_is_suspend = false;
}
/*----------------------------------------------------------------------------*/
static struct early_suspend iflytek_early_suspend_desc = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
	.suspend = iflytek_early_suspend,
	.resume = iflytek_early_resume,
};
#endif
/*----------------------------------------------------------------------------*/
static void xfm10412_wakeup_work(struct work_struct *work)
{
#ifdef USE_INPUT_DEVICE
        if(!xfm10412_obj->input)
                return;

        if(tablet_is_suspend) {
                //wake up system & open voice assistant
                input_report_key(xfm10412_obj->input, KEY_MIC_EVENT1, 1);
                input_sync(xfm10412_obj->input);
                input_report_key(xfm10412_obj->input, KEY_MIC_EVENT1, 0);
                input_sync(xfm10412_obj->input);
        }
        else {
                //just open voice assistant
                input_report_key(xfm10412_obj->input, KEY_MIC_EVENT2, 1);
                input_sync(xfm10412_obj->input);
                input_report_key(xfm10412_obj->input, KEY_MIC_EVENT2, 0);
                input_sync(xfm10412_obj->input);
        }
#endif
/*
        xfm10412_get_degree();

        if(xfm10412_obj->degree_pre != xfm10412_obj->degree){
                block_wait = false;
                wake_up(&wakeup_wait);
        }
*/
        xfm10412_obj->reset = 0;

        enable_irq(xfm10412_obj->irq);
}
/*----------------------------------------------------------------------------*/
static irqreturn_t xfm10412_wakeup_handler(int irq, void *dev)
{
        disable_irq_nosync(irq);
        schedule_delayed_work(&xfm10412_obj->wakeup_delay_work, msecs_to_jiffies(10));
        return IRQ_HANDLED;
}
/*----------------------------------------------------------------------------*/
static int xfm10412_setup_eint(struct device *dev)
{
        int ret;
        int eint_gpio;
        u32 ints[2] = {0,0};
        struct device_node *irq_node;

        mt_set_gpio_dir(GPIO_MIC_EINT_PIN, GPIO_DIR_IN);
        mt_set_gpio_mode(GPIO_MIC_EINT_PIN, GPIO_MIC_EINT_PIN_M_EINT);
        mt_set_gpio_pull_enable(GPIO_MIC_EINT_PIN, TRUE);
        mt_set_gpio_pull_select(GPIO_MIC_EINT_PIN, GPIO_PULL_UP);

        irq_node = of_find_compatible_node(NULL, NULL, "mediatek, IRQ_MIC-eint");
        if(!irq_node)
		return -EINVAL;

        of_property_read_u32_array(irq_node, "debounce", ints, ARRAY_SIZE(ints));
        eint_gpio = ints[0];
        gpio_request(eint_gpio, "eint-gpio");
        gpio_set_debounce(ints[0], ints[1]);

        xfm10412_obj->irq = irq_of_parse_and_map(irq_node, 0);
        if (!xfm10412_obj->irq) {
                pr_err("gpio_to_irq fail!!\n");
                return -EINVAL;
        }

        ret = request_irq(xfm10412_obj->irq, xfm10412_wakeup_handler, IRQF_TRIGGER_RISING, "iflytek_xfm10412_eint", NULL);
        if(ret) {
                pr_err("request_irq fail:%d\n",ret);
                return ret;
        }

        enable_irq_wake(xfm10412_obj->irq);
        return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef USE_INPUT_DEVICE
static int xfm10412_input_init(struct device *dev)
{
        xfm10412_obj->input = input_allocate_device();
        if(!xfm10412_obj->input) {
                pr_err("%s:allocate input device failed\n",__func__);
                return -ENOMEM;
        }

        __set_bit(EV_KEY, (xfm10412_obj->input)->evbit);
        __set_bit(KEY_MIC_EVENT1, (xfm10412_obj->input)->keybit);
        __set_bit(KEY_MIC_EVENT2, (xfm10412_obj->input)->keybit);

        (xfm10412_obj->input)->name = "xfm10412";
        (xfm10412_obj->input)->id.bustype = BUS_I2C;
        if(input_register_device(xfm10412_obj->input)) {
                pr_err("%s:register input device failed\n",__func__);
                return -EINVAL;
        }

        return 0;
}
#endif
/*----------------------------------------------------------------------------*/
static int xfm10412_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        int ret;

        pr_debug("%s\n", __func__);

        xfm10412_obj = devm_kzalloc(&client->dev, sizeof(struct xfm10412_priv), GFP_KERNEL);
        if(!xfm10412_obj) {
                pr_err("%s:allocate memory for xfm10412 failed\n",__func__);
                return -ENOMEM;
        }

        memset(xfm10412_obj,0,sizeof(struct xfm10412_priv));

        i2c_set_clientdata(client, xfm10412_obj);
        xfm10412_obj->client = client;

        INIT_DELAYED_WORK(&xfm10412_obj->wakeup_delay_work, xfm10412_wakeup_work);

        ret = xfm10412_setup_eint(&client->dev);
        if(ret) {
                pr_err("%s:xfm10412_setup_eint failed\n",__func__);
                return ret;
        }

#ifdef USE_INPUT_DEVICE
        ret = xfm10412_input_init(&client->dev);
        if(ret) {
                pr_err("%s:xfm10412_input_init failed\n",__func__);
                return ret;
        }
#endif

        //for debug
        ret = xfm10412_create_class_attr();
        if(ret) {
                pr_err("%s:xfm10412_create_class_attr failed\n",__func__);
                return ret;
        }

#ifdef CONFIG_HAS_EARLYSUSPEND
        register_early_suspend(&iflytek_early_suspend_desc);
#endif

#ifdef USE_CODEC_FUNC
        pr_err("zyw debug:register codec 10412\n");
        ret = snd_soc_register_codec(&client->dev, &xfm10412_codec, &xfm10412_dai, 1);
        if(ret) {
                pr_err("%s:snd_soc_register_codec failed\n",__func__);
                return ret;
        }
#endif

        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10412_i2c_remove(struct i2c_client *client)
{
        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10412_suspend(struct device *dev)
{
        tablet_is_suspend = true;
        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10412_resume(struct device *dev)
{
        tablet_is_suspend = false;
        return 0;
}
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id xfm10412_i2c_id[] = {
	{XFM10412_DEV_NAME, 0},
	{}
};
#ifdef CONFIG_OF
static const struct of_device_id xfm10412_of_match[] = {
	{.compatible = "iflytek,xfm10412"},
	{},
};
#endif
#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops xfm10412_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(xfm10412_suspend, xfm10412_resume)
};
#endif
/*----------------------------------------------------------------------------*/
static struct i2c_driver xfm10412_driver = {
	.probe = xfm10412_i2c_probe,
	.remove = xfm10412_i2c_remove,
	.id_table = xfm10412_i2c_id,
	.driver = {
                .name = XFM10412_DEV_NAME,
#ifdef CONFIG_OF
                .of_match_table =xfm10412_of_match,
#endif
#ifdef CONFIG_PM_SLEEP
                .pm = &xfm10412_pm_ops,
#endif
	},
};
/*----------------------------------------------------------------------------*/
static int __init xfm10412_init(void)
{
        struct i2c_board_info i2c_xfm10412={I2C_BOARD_INFO(XFM10412_DEV_NAME,0x47)};
        i2c_register_board_info(3, &i2c_xfm10412, 1);
        return i2c_add_driver(&xfm10412_driver);
}
/*----------------------------------------------------------------------------*/
static void __exit xfm10412_exit(void)
{
        i2c_del_driver(&xfm10412_driver);
}
/*----------------------------------------------------------------------------*/
module_init(xfm10412_init);
module_exit(xfm10412_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Jason Zhao<xmzyw@malata.com>");
MODULE_DESCRIPTION("iflytek xfm10412 driver");
MODULE_LICENSE("GPL");

