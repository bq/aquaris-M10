/*
 * Linux driver for digital potentiometer
 *
 * Copyright (C) 2018.3 Malata XiaMen
 *
 * Author: Jason Zhao<xmzyw@malata.com>
 *
 */

#include "kpd.h"
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/kthread.h>

/*----------------------------------------------------------------------------*/
static int gpio_evr_p;
static int gpio_evr_n;
static bool potentiometer_thread_timeout;
int previous_state;
int current_state;
int count = 0;
bool is_enabled;

#define VOLUME_UP 0
#define VOLUME_DOWN 1

typedef enum
{
    CW = 0,
    CCW = 1,
    CW_UNKNOWN = 2,
} clock_dir;

clock_dir clkdir = CW_UNKNOWN;

static DECLARE_WAIT_QUEUE_HEAD(potentiometer_thread_wq);
static struct hrtimer potentiometer_kthread_timer;
/*----------------------------------------------------------------------------*/
enum hrtimer_restart potentiometer_kthread_timer_func(struct hrtimer *timer)
{
	potentiometer_thread_timeout = true;
	wake_up(&potentiometer_thread_wq);

	return HRTIMER_NORESTART;
}
/*----------------------------------------------------------------------------*/
void potentiometer_kthread_hrtimer_init(void)
{
	ktime_t ktime;

	ktime = ktime_set(3, 0);

	hrtimer_init(&potentiometer_kthread_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	potentiometer_kthread_timer.function = potentiometer_kthread_timer_func;
	hrtimer_start(&potentiometer_kthread_timer, ktime, HRTIMER_MODE_REL);
}

/*----------------------------------------------------------------------------*/
static int potentiometer_get_state(void)
{
        int state = 0;
        int gpio_evr_p_state;
        int gpio_evr_n_state;
        gpio_evr_p_state = gpio_get_value(gpio_evr_p);
        gpio_evr_n_state = gpio_get_value(gpio_evr_n);

        state = gpio_evr_p_state * 10 + gpio_evr_n_state;
        return state;
}
/*----------------------------------------------------------------------------*/
static int potentiometer_get_dir(void)
{
        int i;
        clock_dir dir = CW_UNKNOWN;
        int pre_index = 0;
        int cur_index = 0;
        int state_buf[4] = {00,10,11,01};

        for (i = 0; i < 4; i++) {
                if (previous_state == state_buf[i]) {
                        pre_index = i;
                        break;
                }
        }

        for (i = 0; i < 4; i++) {
                if (current_state == state_buf[i]) {
                        cur_index = i;
                        break;
                }
        }

        if (previous_state == 01 && current_state == 00)
                cur_index = 4;
        else if (previous_state == 00 && current_state == 01)
                cur_index = -1;

        if (cur_index > pre_index)
                dir = CW;
        else
                dir = CCW;

        return dir;
}
/*----------------------------------------------------------------------------*/
static int potentiometer_getstate_kthread(void *x)
{
        while (1) {
                current_state = potentiometer_get_state();

                if (current_state != previous_state) {
                        clkdir = potentiometer_get_dir();
                        if (clkdir == CW)
                                count++;
                        else
                                count--;

                        if (count == 3) {
                                kpd_volume_report(VOLUME_DOWN);
                                count = 0;
                        }
                        else if (count == -3) {
                                kpd_volume_report(VOLUME_UP);
                                count = 0;
                        }
                }

                previous_state = current_state;

                msleep(1);

        }

	return 0;
}
/*----------------------------------------------------------------------------*/
static int potentiometer_check_kthread(void *x)
{
        while (1) {

                if (count != 3 || count != -3)
                        count = 0;

                wait_event(potentiometer_thread_wq, (potentiometer_thread_timeout == true));

                potentiometer_thread_timeout = false;
                hrtimer_start(&potentiometer_kthread_timer, ktime_set(3, 0), HRTIMER_MODE_REL);
        }

	return 0;
}
/*----------------------------------------------------------------------------*/
static int potentiometer_probe(struct platform_device *pdev)
{
        int ret = 0;
        struct device * dev = &pdev->dev;

        gpio_evr_p = of_get_named_gpio(dev->of_node, "evr_p", 0);
        if (gpio_is_valid(gpio_evr_p)) {
                gpio_request(gpio_evr_p, "gpio_evr_p");
                gpio_direction_input(gpio_evr_p);
        }
        else {
                pr_err("can not get valid gpio for evr_p\n");
                ret = -1;
                goto exit;
        }

        gpio_evr_n = of_get_named_gpio(dev->of_node, "evr_n", 0);
        if (gpio_is_valid(gpio_evr_n)) {
                gpio_request(gpio_evr_n, "gpio_evr_n");
                gpio_direction_input(gpio_evr_n);
        }
        else {
                pr_err("can not get valid gpio for evr_n\n");
                ret = -1;
                goto exit;
        }

        previous_state = potentiometer_get_state();

        potentiometer_kthread_hrtimer_init();
        kthread_run(potentiometer_getstate_kthread, NULL, "potentiometer_getstate_kthread");
        kthread_run(potentiometer_check_kthread, NULL, "potentiometer_check_kthread");

        return 0;
exit:
        pr_err("%s error\n",__func__);
        return ret;
}
/*----------------------------------------------------------------------------*/
static const struct of_device_id potentiometer_of_match[] = {
	{.compatible = "mediatek,potentiometer",},
	{},
};

static struct platform_driver potentiometer_driver = {
	.probe = potentiometer_probe,
	.driver = {
		   .name = "potentiometer",
		   .owner = THIS_MODULE,
		   .of_match_table = potentiometer_of_match,
	},
};
/*----------------------------------------------------------------------------*/
static int __init potentiometer_init(void)
{
        int ret;
        pr_debug("%s\n",__func__);
        ret = platform_driver_register(&potentiometer_driver);
        if (ret) {
                pr_err("register driver failed (%d)\n", ret);
                return ret;
        }

        return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit potentiometer_exit(void)
{

}
/*----------------------------------------------------------------------------*/
module_init(potentiometer_init);
module_exit(potentiometer_exit);
