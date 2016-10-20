/*
 * Copyright (C) 2010 MediaTek, Inc.
 *
 * Author: Terry Chang <terry.chang@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "kpd.h"
#include <linux/wakelock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>

#define KPD_MHALL_NAME	"mtk-mhall"
#define KPD_MHALL_INIT_GPIO

extern bool kpd_hallkey_report(int slid);
//#define CUST_EINT_MHALL_POLARITY  ((CUST_EINT_MHALL_TYPE==CUST_EINTF_TRIGGER_LOW) ?(MT_EINT_POL_NEG):(MT_EINT_POL_POS))//CUST_EINT_MHALL_TYPE
//#define CUST_EINT_MHALL_SENSITIVE CUST_EINTF_TRIGGER_LOW
//static int kpd_auto_test_for_factorymode_flag=0;
static void kpd_mhallkey_handler(unsigned long data);
static DECLARE_TASKLET(kpd_mhallkey_tasklet, kpd_mhallkey_handler, 0);
//static unsigned int kpd_hallkey_state = 0;//((CUST_EINT_MHALL_TYPE==CUST_EINTF_TRIGGER_LOW)?0:1);
//static int kpd_hallkey_cnt=0;
static u32 irq_gpio_mhall;
static struct device_node *irq_node_mhall=NULL;
static int irq_num_mhall;
static int gpio_num_mhall;

static int kpd_mhall_pdrv_probe(struct platform_device *pdev);
static int kpd_mhall_pdrv_remove(struct platform_device *pdev);
#ifndef USE_EARLY_SUSPEND
static int kpd_mhall_pdrv_suspend(struct platform_device *pdev, pm_message_t state);
static int kpd_mhall_pdrv_resume(struct platform_device *pdev);
#endif

static const struct of_device_id kpd_mhall_of_match[] = {
	{.compatible = "mediatek, mhall",},
	{},
};

static struct platform_driver kpd_mhall_pdrv = {
	.probe = kpd_mhall_pdrv_probe,
	.remove = kpd_mhall_pdrv_remove,
#ifndef USE_EARLY_SUSPEND
	.suspend = kpd_mhall_pdrv_suspend,
	.resume = kpd_mhall_pdrv_resume,
#endif
	.driver = {
		   .name = KPD_MHALL_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = kpd_mhall_of_match,
	},
};

static void kpd_mhallkey_handler(unsigned long data)
{
	int slid;
	//u8 old_state = kpd_hallkey_state;
	//kpd_hallkey_state = !kpd_hallkey_state;
	slid=gpio_get_value(gpio_num_mhall);
	kpd_hallkey_report(slid);
	//kpd_print("get MHALL gpio = %d\n", slid);
	//if(slid!=kpd_hallkey_state){
	//slid = (kpd_hallkey_state == !!CUST_EINT_MHALL_POLARITY);
	//kpd_hallkey_cnt++;
	//input_report_switch(kpd_input_dev, SW_LID, ((0==slid) ? 1 : 0));
	//input_sync(kpd_input_dev);
	//kpd_print("report MHALL QWERTY = %s(%d)\n", slid ? "slid" : "closed", slid);
	//if(1==kpd_auto_test_for_factorymode_flag){
	//    input_report_key(kpd_input_dev, KEY_HALL_SLEEP, slid);
	//    input_sync(kpd_input_dev);
	//}
	//kpd_hallkey_state=slid;
	//}
	//mt_eint_set_polarity(CUST_EINT_MHALL_NUM, old_state);
	irq_set_irq_type(irq_num_mhall, ((1==slid) ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH));
	enable_irq(irq_num_mhall);//mt_eint_unmask(CUST_EINT_MHALL_NUM);
	enable_irq_wake(irq_num_mhall);//xmyyq-set irq to wake up source 
}
static irqreturn_t kpd_mhallkey_eint_handler(int irq, void *dev_id)
{
	//kpd_info("MHALL-eint,interrupts: occur!");
	disable_irq_nosync(irq_num_mhall);
	tasklet_schedule(&kpd_mhallkey_tasklet);

	return IRQ_HANDLED;
}

//struct of_device_id mhall_of_match[] = {
//	{ .compatible = "mediatek, mhall-eint", },
//	{},
//};

static int kpd_mhall_pdrv_probe(struct platform_device *pdev)
{
	u32 ints[2] = {0,0};
	unsigned long flags_trigger_type;
#ifdef KPD_MHALL_INIT_GPIO 
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;
#endif
	long ret=0;

#ifdef KPD_MHALL_INIT_GPIO        
	//gpio setting
	pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		kpd_info("Cannot find  pinctrl!\n");
		goto EXIT;
	}
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		//ret = PTR_ERR(pins_default);
		kpd_info("Cannot find pinctrl default!\n");
		//goto EXIT;
	}
	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		kpd_info("Cannot find pinctrl pin_cfg!\n");
		goto EXIT;
	}
	pinctrl_select_state(pinctrl, pins_cfg);
	kpd_info("MHALL-eint, GPIO init ok!\n");
#endif

	 //interrupt setup
	irq_node_mhall= of_find_matching_node(NULL, kpd_mhall_of_match);
	if(irq_node_mhall)
	{
		gpio_num_mhall = of_get_named_gpio(irq_node_mhall, "int", 0);
		kpd_info("MHALL-eint,GPIO %d!!\n", gpio_num_mhall);
		of_property_read_u32_array(irq_node_mhall, "interrupts", ints, ARRAY_SIZE(ints));
		flags_trigger_type=( (8==ints[1]) ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);//((8==ints[1])?1:0);//wait for low level;
		kpd_info("MHALL-eint,interrupts:ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
		of_property_read_u32_array(irq_node_mhall, "debounce", ints, ARRAY_SIZE(ints));
		irq_gpio_mhall=ints[0];
		//gpio_request(ints[0], "MHALL-key");
		gpio_set_debounce(ints[0], ints[1]);
		kpd_info("MHALL-eint,debounce:ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
		irq_num_mhall= irq_of_parse_and_map(irq_node_mhall, 0);
		if (irq_num_mhall < 0)
		{
			kpd_info("MHALL-eint,map irq_num_hall fail!\n");
			ret=-1;
		} else {
			if (request_irq(irq_num_mhall, (irq_handler_t) kpd_mhallkey_eint_handler, IRQF_TRIGGER_LOW, "mhall-eint", NULL) > 0)
			{//IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING
				kpd_info("MHALL-eint,request_irq IRQ NOT AVAILABLE!.");
				ret=-1;
			} else {
				kpd_info("MHALL-eint,request_irq irq_num_hall %d Ok\n",irq_num_mhall);
				//disable_irq(irq_num_mhall);
				//enable_irq(irq_num_mhall);
			}
		}
	} else {
		kpd_info("MHALL-eint, node NOT AVAILABLE!\n");
		ret=-1;
	}

#ifdef KPD_MHALL_INIT_GPIO     
EXIT:
#endif
	return ret;
}


/* should never be called */
static int kpd_mhall_pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

#ifndef USE_EARLY_SUSPEND
static int kpd_mhall_pdrv_suspend(struct platform_device *pdev, pm_message_t state)
{
	//enable_irq_wake(irq_num_mhall);//mt_eint_unmask(CUST_EINT_MHALL_NUM);
	return 0;
}

static int kpd_mhall_pdrv_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define kpd_mhall_pdrv_suspend	    NULL
#define kpd_mhall_pdrv_resume	    NULL
#endif


static int __init kpd_mhall_mod_init(void)
{
	int ret;
	
	kpd_print("kpd_mhall_mod_init\n");
	ret = platform_driver_register(&kpd_mhall_pdrv);
	if (ret) {
		kpd_info("register driver failed (%d)\n", ret);
		return ret;
	}

	return 0;
}

/* should never be called */
static void __exit kpd_mhall_mod_exit(void)
{

}

module_init(kpd_mhall_mod_init);
module_exit(kpd_mhall_mod_exit);
