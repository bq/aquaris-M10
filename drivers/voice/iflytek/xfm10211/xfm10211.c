/*
 * Linux driver for iflytek xfm10211 module
 *
 * Copyright (C) 2018.3, Malata XiaMen
 *
 * Author: Jason Zhao<xmzyw@malata.com>
 *
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <cust_eint.h>
#include <mach/mt_gpio.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define XFM10211_DEV_NAME "xfm10211"
bool tablet_is_suspend;
#define USE_INPUT_DEVICE

/*----------------------------------------------------------------------------*/
struct xfm10211_priv {
        int irq;

        struct input_dev *input;
};
static struct xfm10211_priv *xfm10211_obj;
/*----------------------------------------------------------------------------*/
ssize_t xfm10211_show_depth(struct class *class, struct class_attribute *attr, char *buf)
{
        struct irq_desc *desc = irq_to_desc(xfm10211_obj->irq);
        return sprintf(buf,"%d\n",desc->depth);
}
/*----------------------------------------------------------------------------*/
ssize_t xfm10211_show_suspend(struct class *class, struct class_attribute *attr, char *buf)
{
        return sprintf(buf,"%d\n",tablet_is_suspend);
}
/*----------------------------------------------------------------------------*/
static CLASS_ATTR(depth,0644,xfm10211_show_depth,NULL);
static CLASS_ATTR(suspend,0644,xfm10211_show_suspend,NULL);
/*----------------------------------------------------------------------------*/
static struct class_attribute *xfm10211_attr_list[] = {
        &class_attr_depth,
        &class_attr_suspend,
};
/*----------------------------------------------------------------------------*/
static int xfm10211_create_class_attr(void)
{
        int ret;
        int index,num;
        struct class *iflytek_class;

        iflytek_class = class_create(THIS_MODULE, "xfm10211");
        if (IS_ERR(iflytek_class))
                return PTR_ERR(iflytek_class);

        num = (int)(sizeof(xfm10211_attr_list)/sizeof(xfm10211_attr_list[0]));

        for(index = 0; index < num; index++)
        {
                ret = class_create_file(iflytek_class, xfm10211_attr_list[index]);
                if(ret) {
                        pr_err("class_create_file (%s) failed:%d\n",xfm10211_attr_list[index]->attr.name,ret);
                        break;
                }
        }

        return ret;
}
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
static irqreturn_t xfm10211_handler(int irq, void *desc)
{
        disable_irq_nosync(xfm10211_obj->irq);

#ifdef USE_INPUT_DEVICE
        if(!xfm10211_obj->input)
                goto exit;

        if(tablet_is_suspend) {
                input_report_key(xfm10211_obj->input, KEY_MIC_EVENT1, 1);
                input_sync(xfm10211_obj->input);
                input_report_key(xfm10211_obj->input, KEY_MIC_EVENT1, 0);
                input_sync(xfm10211_obj->input);
        }
        else {
                input_report_key(xfm10211_obj->input, KEY_MIC_EVENT2, 1);
                input_sync(xfm10211_obj->input);
                input_report_key(xfm10211_obj->input, KEY_MIC_EVENT2, 0);
                input_sync(xfm10211_obj->input);
        }
#endif

exit:
        enable_irq(xfm10211_obj->irq);
        return IRQ_HANDLED;
}
/*----------------------------------------------------------------------------*/
static int xfm10211_setup_eint(struct device *dev)
{
        int ret;
        int eint_gpio;
        u32 ints[2] = {0,0};
        struct device_node *irq_node;

        mt_set_gpio_dir(GPIO_MIC_EINT_PIN, GPIO_DIR_IN);
        mt_set_gpio_mode(GPIO_MIC_EINT_PIN, GPIO_MIC_EINT_PIN_M_EINT);
        mt_set_gpio_pull_enable(GPIO_MIC_EINT_PIN, TRUE);
        mt_set_gpio_pull_select(GPIO_MIC_EINT_PIN, GPIO_PULL_DOWN);

        irq_node = of_find_compatible_node(NULL, NULL, "mediatek, IRQ_MIC-eint");
        if(!irq_node)
		return -EINVAL;

        of_property_read_u32_array(irq_node, "debounce", ints, ARRAY_SIZE(ints));
        eint_gpio = ints[0];
        gpio_request(eint_gpio, "eint-gpio");
        gpio_set_debounce(ints[0], ints[1]);

	xfm10211_obj->irq = irq_of_parse_and_map(irq_node, 0);
	if (!xfm10211_obj->irq) {
		pr_err("gpio_to_irq fail!!\n");
		return -EINVAL;
	}

	ret = request_irq(xfm10211_obj->irq, xfm10211_handler, IRQF_TRIGGER_RISING, "iflytek_xfm10211_eint", NULL);
	if(ret) {
		pr_err("request_irq fail:%d\n",ret);
		return ret;
	}

	enable_irq_wake(xfm10211_obj->irq);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10211_input_init(void)
{
        xfm10211_obj->input = input_allocate_device();
        if (!xfm10211_obj->input) {
                pr_err("allocate input device failed\n");
                return -ENOMEM;
        }

        __set_bit(EV_KEY, (xfm10211_obj->input)->evbit);
        __set_bit(KEY_MIC_EVENT1, (xfm10211_obj->input)->keybit);
        __set_bit(KEY_MIC_EVENT2, (xfm10211_obj->input)->keybit);

        (xfm10211_obj->input)->name = "iflytek";
        (xfm10211_obj->input)->id.bustype = BUS_I2C;
        if(input_register_device(xfm10211_obj->input)){
                pr_err("%s:register input device failed\n",__func__);
                return -EINVAL;
        }

        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10211_probe(struct platform_device *pdev)
{
        int ret;

        xfm10211_obj = devm_kzalloc(&pdev->dev, sizeof(struct xfm10211_priv), GFP_KERNEL);
        if(!xfm10211_obj) {
                pr_err("%s:allocate memory for xfm10211 failed\n",__func__);
                return -ENOMEM;
        }

        memset(xfm10211_obj,0,sizeof(struct xfm10211_priv));

        ret = xfm10211_setup_eint(&pdev->dev);
        if(ret){
                pr_err("%s:xfm10211_setup_eint failed\n",__func__);
                return ret;
        }

#ifdef USE_INPUT_DEVICE
        ret = xfm10211_input_init();
        if(ret) {
                pr_err("%s:xfm10211_input_init failed\n",__func__);
                return ret;
        }
#endif

        //for debug
        ret = xfm10211_create_class_attr();
        if(ret){
                pr_err("xfm10211_create_class_attr failed\n");
                return ret;
        }

#ifdef CONFIG_HAS_EARLYSUSPEND
        register_early_suspend(&iflytek_early_suspend_desc);
#endif

        return 0;
}
/*----------------------------------------------------------------------------*/
static int xfm10211_remove(struct platform_device *pdev)
{
        return 0;
}
/*----------------------------------------------------------------------------*/
static const struct of_device_id xfm10211_of_pltmatch[] = {
	{.compatible = "iflytek,xfm10211",},
	{},
};
/*----------------------------------------------------------------------------*/
static struct platform_driver xfm10211_driver = {
	.probe = xfm10211_probe,
	.remove = xfm10211_remove,
	.driver = {
		.name  = XFM10211_DEV_NAME,
		.of_match_table = xfm10211_of_pltmatch,
	}
};
/*----------------------------------------------------------------------------*/
static int __init xfm10211_init(void)
{
        platform_driver_register(&xfm10211_driver);
        return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit xfm10211_exit(void)
{
        platform_driver_unregister(&xfm10211_driver);
}
/*----------------------------------------------------------------------------*/
module_init(xfm10211_init);
module_exit(xfm10211_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Jason Zhao<xmzyw@malata.com>");
MODULE_DESCRIPTION("iflytek xfm10211 driver");
MODULE_LICENSE("GPL");
