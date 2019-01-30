/*
 * Copyright (C) 2018 Malata, Inc.
 *
 * Author: Xiaojun Huang <xmhxj@malata.com>
 *
 *Hall Key Driver.
 *
 */

#include "kpd.h"
#include <linux/wakelock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>


#define DHALL_DBG 0

#if DHALL_DBG
#define dhall_print(fmt, arg...) pr_err(KPD_SAY fmt, ##arg)
#else
#define dhall_print(fmt, arg...)	do {} while (0)
#endif


typedef enum
{
    KAL_FALSE = 0,
    KAL_TRUE = 1,
} kal_bool;
/*----------------------------------------------------------------------------*/
static u32 gpio_dhall_h1;
static u32 gpio_dhall_h2;
static int gpio_dhall_state_old=0;
static int gpio_dhall_state_tmp;
static kal_bool dhall_thread_timeout = KAL_FALSE;
static DEFINE_MUTEX(dhall_mutex);
static DECLARE_WAIT_QUEUE_HEAD(dhall_thread_wq);
static struct hrtimer dhall_kthread_timer;

/*----------------------------------------------------------------------------*/
static int kpd_dhall_pdrv_probe(struct platform_device *pdev);

/*----------------------------------------------------------------------------*/
static const struct of_device_id kpd_dhall_of_match[] = {
	{.compatible = "mediatek,dhall",},
	{},
};

/*----------------------------------------------------------------------------*/
static struct platform_driver kpd_dhall_pdrv = {
	.probe = kpd_dhall_pdrv_probe,
	.driver = {
		   .name = "dhall",
		   .owner = THIS_MODULE,
		   .of_match_table = kpd_dhall_of_match,
	},
};



/*----------------------------------------------------------------------------*/
enum hrtimer_restart dhall_kthread_timer_func(struct hrtimer *timer)
{
	dhall_print( "******** dhall : dhall_thread_wakeup  ********\n");

	dhall_thread_timeout = KAL_TRUE;
	wake_up(&dhall_thread_wq);

	return HRTIMER_NORESTART;
}
/*----------------------------------------------------------------------------*/
void dhall_kthread_hrtimer_init(void)
{
	ktime_t ktime;

	ktime = ktime_set(1, 0);

	hrtimer_init(&dhall_kthread_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	dhall_kthread_timer.function = dhall_kthread_timer_func;
	hrtimer_start(&dhall_kthread_timer, ktime, HRTIMER_MODE_REL);

	kpd_print("dhall_kthread_timer_init : done\n");
}

/*----------------------------------------------------------------------------*/
int dhall_thread_kthread(void *x)
{
	int hall_h1_state;
	int hall_h2_state;

	/* Run on a process content */
	while (1) {
		mutex_lock(&dhall_mutex);
		/*Get h1 and h2 data*/
 		
        hall_h1_state=gpio_get_value(gpio_dhall_h1);
		hall_h2_state=gpio_get_value(gpio_dhall_h2);

		if((hall_h1_state==0)&&(hall_h2_state == 0)){
			gpio_dhall_state_tmp=0;
		}else if ((hall_h1_state==1)&&(hall_h2_state == 0)){
			gpio_dhall_state_tmp=1;
		}else if ((hall_h1_state==0)&&(hall_h2_state == 1)){
			gpio_dhall_state_tmp=3;
		}else if ((hall_h1_state==1)&&(hall_h2_state == 1)){
			gpio_dhall_state_tmp=2;
		}
		
		dhall_print( "[ xmhxj dhall_thread_kthread] xmhxj hall_h2_state=%d hall_h1_state=%d\n",hall_h2_state,hall_h1_state);
		dhall_print( "[ xmhxj dhall_thread_kthread] xmhxj gpio_dhall_state_tmp=%d\n",gpio_dhall_state_tmp);


		if ( (gpio_dhall_state_tmp != gpio_dhall_state_old))
			kpd_hallkey_report(gpio_dhall_state_tmp);

		gpio_dhall_state_old = gpio_dhall_state_tmp;

		
		mutex_unlock(&dhall_mutex);

		wait_event(dhall_thread_wq, (dhall_thread_timeout == KAL_TRUE));

		dhall_thread_timeout = KAL_FALSE;
		hrtimer_start(&dhall_kthread_timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	
		}

	return 0;
}

/*----------------------------------------------------------------------------*/
static int kpd_dhall_pdrv_probe(struct platform_device *pdev)
{
        long ret=0;
        struct device_node * irq_node;


        //interrupt setup
        irq_node = of_find_matching_node(NULL, kpd_dhall_of_match);
        if(irq_node){
                gpio_dhall_h1 = of_get_named_gpio(irq_node, "int_h1", 0);
				gpio_dhall_h2 = of_get_named_gpio(irq_node, "int_h2", 0);
				gpio_request(gpio_dhall_h1, "int_h1");
				gpio_request(gpio_dhall_h2, "int_h2");
				gpio_direction_input(gpio_dhall_h1);
				gpio_direction_input(gpio_dhall_h2);
				
				dhall_kthread_hrtimer_init();
				kthread_run(dhall_thread_kthread, NULL, "dhall_thread_kthread");
				kpd_print( "[ xmhxj kpd_dhall_pdrv_probe] dhall_thread_kthread Done\n");

        }
        else{
                kpd_print(" xmhxj dhall-eint, node NOT AVAILABLE!\n");
                return -EINVAL;
        }


        return ret;
}

/*----------------------------------------------------------------------------*/
static int __init kpd_dhall_mod_init(void)
{
        int r;
        kpd_print("kpd_dhall_mod_init\n");
        r = platform_driver_register(&kpd_dhall_pdrv);
        if (r) {
                kpd_print("register driver failed (%d)\n", r);
                return r;
        }

        return 0;
}
/*----------------------------------------------------------------------------*/
/* should never be called */
static void __exit kpd_dhall_mod_exit(void)
{

}

module_init(kpd_dhall_mod_init);
module_exit(kpd_dhall_mod_exit);
