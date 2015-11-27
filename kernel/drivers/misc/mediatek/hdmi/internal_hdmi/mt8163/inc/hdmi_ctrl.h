#ifndef __hdmi_ctrl_h__
#define __hdmi_ctrl_h__
#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT

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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/byteorder/generic.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/rtpm_prio.h>
#include <linux/dma-mapping.h>
#include <linux/syscalls.h>
#include <linux/reboot.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/completion.h>

#include "internal_hdmi_drv.h"

#include <cust_eint.h>
#include "cust_gpio_usage.h"
#include "mach/eint.h"
#include "mach/irqs.h"

#ifdef MT6575
#include <mach/mt6575_devs.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_pm_ldo.h>
#else
#include <mach/devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_pm_ldo.h>
#endif

extern size_t hdmidrv_log_on;

#define hdmiplllog         (0x1)
#define hdmidgilog         (0x2)
#define hdmitxhotpluglog   (0x4)
#define hdmitxvideolog     (0x8)
#define hdmitxaudiolog     (0x10)
#define hdmihdcplog        (0x20)
#define hdmiceclog         (0x40)
#define hdmiddclog         (0x80)
#define hdmiedidlog        (0x100)
#define hdmidrvlog         (0x200)

#define hdmialllog   (0x3bf)

//////////////////////////////////////////////PLL//////////////////////////////////////////////////////
#define HDMI_PLL_LOG(fmt, arg...) \
	do { \
		if (hdmidrv_log_on&hdmiplllog) {printk("[hdmi_pll]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
	}while (0)

#define HDMI_PLL_FUNC()	\
	do { \
		if(hdmidrv_log_on&hdmiplllog) {printk("[hdmi_pll] %s\n", __func__);} \
	}while (0)

//////////////////////////////////////////////DGI//////////////////////////////////////////////////////

#define HDMI_DGI_LOG(fmt, arg...) \
    do { \
        if (hdmidrv_log_on&hdmidgilog) {printk("[hdmi_dgi1]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
    }while (0)

#define HDMI_DGI_FUNC()	\
    do { \
        if(hdmidrv_log_on&hdmidgilog) {printk("[hdmi_dgi1] %s\n", __func__);} \
    }while (0)

//////////////////////////////////////////////PLUG//////////////////////////////////////////////////////

#define HDMI_PLUG_LOG(fmt, arg...) \
    do { \
        if (hdmidrv_log_on&hdmitxhotpluglog) {printk("[hdmi_plug]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
    }while (0)

#define HDMI_PLUG_FUNC()	\
    do { \
        if(hdmidrv_log_on&hdmitxhotpluglog) {printk("[hdmi_plug] %s\n", __func__);} \
    }while (0)

		
////////////////////////////////////////////////VIDEO////////////////////////////////////////////////////

#define HDMI_VIDEO_LOG(fmt, arg...) \
    do { \
        if (hdmidrv_log_on&hdmitxvideolog) {printk("[hdmi_video]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
    }while (0)

#define HDMI_VIDEO_FUNC()	\
    do { \
        if(hdmidrv_log_on&hdmitxvideolog) {printk("[hdmi_video] %s\n", __func__);} \
    }while (0)

////////////////////////////////////////////////AUDIO////////////////////////////////////////////////////

#define HDMI_AUDIO_LOG(fmt, arg...) \
    do { \
        if (hdmidrv_log_on&hdmitxaudiolog) {printk("[hdmi_audio]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
    }while (0)

#define HDMI_AUDIO_FUNC()	\
    do { \
        if(hdmidrv_log_on&hdmitxaudiolog) {printk("[hdmi_audio] %s\n", __func__);} \
    }while (0)
/////////////////////////////////////////////////HDCP///////////////////////////////////////////////////

#define HDMI_HDCP_LOG(fmt, arg...) \
    do { \
        if (hdmidrv_log_on&hdmihdcplog) {printk("[hdmi_hdcp]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
    }while (0)

#define HDMI_HDCP_FUNC()	\
    do { \
        if(hdmidrv_log_on&hdmihdcplog) {printk("[hdmi_hdcp] %s\n", __func__);} \
    }while (0)

/////////////////////////////////////////////////CEC///////////////////////////////////////////////////

#define HDMI_CEC_LOG(fmt, arg...) \
    do { \
        if (hdmidrv_log_on&hdmiceclog) {printk("[hdmi_cec]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
    }while (0)

#define HDMI_CEC_FUNC()	\
    do { \
        if(hdmidrv_log_on&hdmiceclog) {printk("[hdmi_cec] %s\n", __func__);} \
    }while (0)
/////////////////////////////////////////////////DDC//////////////////////////////////////////////////
#define HDMI_DDC_LOG(fmt, arg...) \
	do { \
		if (hdmidrv_log_on&hdmiddclog) {printk("[hdmi_ddc]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
	}while (0)
		
#define HDMI_DDC_FUNC()	\
	do { \
		if(hdmidrv_log_on&hdmiddclog) {printk("[hdmi_ddc] %s\n", __func__);} \
	}while (0)
/////////////////////////////////////////////////EDID//////////////////////////////////////////////////
#define HDMI_EDID_LOG(fmt, arg...) \
	do { \
		if (hdmidrv_log_on&hdmiedidlog) {printk("[hdmi_edid]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
	}while (0)

#define HDMI_EDID_FUNC()	\
	do { \
		if(hdmidrv_log_on&hdmiedidlog) {printk("[hdmi_edid] %s\n", __func__);} \
	}while (0)
	
//////////////////////////////////////////////////DRV/////////////////////////////////////////////////

#define HDMI_DRV_LOG(fmt, arg...)	\
    do { \
        if (hdmidrv_log_on&hdmidrvlog) {printk("[hdmi_drv]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
    }while (0)

#define HDMI_DRV_FUNC()	\
    do { \
        if(hdmidrv_log_on&hdmidrvlog) {printk("[hdmi_drv] %s\n", __func__);} \
    }while (0)
///////////////////////////////////////////////////////////////////////////////////////////////////

#define RETNULL(cond)       if ((cond)){HDMI_DRV_LOG("return in %d\n",__LINE__);return;}
#define RETINT(cond, rslt)       if ((cond)){HDMI_DRV_LOG("return in %d\n",__LINE__);return (rslt);}


//extern int hdmi_i2c_read(unsigned short addr, unsigned int *data);
//extern int hdmi_i2c_write(unsigned short addr, unsigned int data);
extern 	void hdmi_write(unsigned long u2Reg, unsigned int u4Data);

extern void vSetClk(void);

#endif
#endif
