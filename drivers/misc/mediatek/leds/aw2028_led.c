/**************************************************************************
*  AW2028_LED.c
* 
*  Create Date : 
* 
*  Modify Date : 
*
*  Create by   : AWINIC Technology CO., LTD
*
*  Version     : 1.0.0 , 2017/09/18
**************************************************************************/

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>

#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/gameport.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>

#include <linux/wakelock.h>

#define AW2028_I2C_NAME		"AW2028_LED" 
#define AW2028_I2C_BUS		0
#define AW2028_I2C_ADDR		0x65
#define LED_1		1
#define LED_2		2
#define LED_3		3


static int open_state=0;

static ssize_t AW2028_set_enable(struct device* cd, struct device_attribute *attr, const char* buf, size_t len);
static ssize_t AW2028_get_enable(struct device* cd, struct device_attribute *attr, char* buf);
static ssize_t AW2028_set_Current(struct device* cd, struct device_attribute *attr, const char* buf, size_t len);
static ssize_t AW2028_get_Current(struct device* cd, struct device_attribute *attr, char* buf);
static ssize_t AW2028_set_breath(struct device* cd, struct device_attribute *attr, const char* buf, size_t len);
static ssize_t AW2028_get_breath(struct device* cd, struct device_attribute *attr, char* buf);
static ssize_t AW2028_set_mode(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t AW2028_get_mode(struct device* cd, struct device_attribute *attr, char* buf);
static ssize_t AW2028_set_reg(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t AW2028_get_reg(struct device* cd,struct device_attribute *attr, char* buf);

static DEVICE_ATTR(enable, 0664, AW2028_get_enable,  AW2028_set_enable);
static DEVICE_ATTR(Current, 0664, AW2028_get_Current,  AW2028_set_Current);
static DEVICE_ATTR(breath, 0664, AW2028_get_breath,  AW2028_set_breath);
static DEVICE_ATTR(mode, 0664, AW2028_get_mode,  AW2028_set_mode);
static DEVICE_ATTR(reg, 0664, AW2028_get_reg,  AW2028_set_reg);



struct i2c_client *AW2028_i2c_client;

//////////////////////////////////////////////////////
// i2c write and read
//////////////////////////////////////////////////////
unsigned char I2C_write_reg(unsigned char addr, unsigned char reg_data)
{
	char ret;
	u8 wdbuf[512] = {0};

	struct i2c_msg msgs[] = {
		{
			.addr	= AW2028_i2c_client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= wdbuf,
		},
	};
	
	if(NULL == AW2028_i2c_client)
	{
		pr_err("AW2028_i2c_client is NULL\n");
		return -1;
	}

	wdbuf[0] = addr;
	wdbuf[1] = reg_data;

	ret = i2c_transfer(AW2028_i2c_client->adapter, msgs, 1);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

    return ret;
}

unsigned char I2C_read_reg(unsigned char addr)
{
	unsigned char ret;
	u8 rdbuf[512] = {0};

	struct i2c_msg msgs[] = {
		{
			.addr	= AW2028_i2c_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rdbuf,
		},
		{
			.addr	= AW2028_i2c_client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= rdbuf,
		},
	};
	
	if(NULL == AW2028_i2c_client)
	{
		pr_err("AW2028_i2c_client is NULL\n");
		return -1;
	}

	rdbuf[0] = addr;

	ret = i2c_transfer(AW2028_i2c_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

    return rdbuf[0];
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AW2028 LED 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char ms2timer(unsigned int time)
{
	unsigned char i,ret;
	unsigned int ref[16] = {4, 128, 256, 384, 512, 762, 1024, 1524, 2048, 2560, 3072, 4096, 5120, 6144, 7168, 8192};
	ret = 0;
	for(i=0; i<15; i++)
	{
		if(time <= ref[0])
		{
			return 0;
		}
		else if(time > ref[15])
		{
			return 15;
		}
		else if((time>ref[i]) && (time<=ref[i+1]))
		{
			if((time-ref[i]) <= (ref[i+1]-time))
			{
				ret = i;
				break;
			}
			else
			{
				ret = i+1;
				break;
			}
		}
	}
	return ret;
}	

/*
unsigned char AW2028_LED_ON(unsigned char r, unsigned char g, unsigned char b)
{
	I2C_write_reg(0x00, 0x55);		// software reset

	I2C_write_reg(0x01, 0x01);		// GCR
	I2C_write_reg(0x03, 0x01);		// IMAX
	I2C_write_reg(0x04, 0x00);		// LCFG1
	I2C_write_reg(0x05, 0x00);		// LCFG2
	I2C_write_reg(0x06, 0x00);		// LCFG3
	I2C_write_reg(0x07, 0x07);		// LEDEN
	
	I2C_write_reg(0x10, r);		// ILED1
	I2C_write_reg(0x11, g);		// ILED2
	I2C_write_reg(0x12, b);		// ILED3
	I2C_write_reg(0x1C, 0xFF);		// PWM1
	I2C_write_reg(0x1D, 0xFF);		// PWM2
	I2C_write_reg(0x1E, 0xFF);		// PWM3	
	
	return 0;
}
*/

unsigned char AW2028_LED_ON(unsigned char vlaue)
{
	I2C_write_reg(0x00, 0x55);		// software reset

	I2C_write_reg(0x01, 0x01);		// GCR
	I2C_write_reg(0x03, 0x01);		// IMAX
	I2C_write_reg(0x04, 0x00);		// LCFG1
	I2C_write_reg(0x05, 0x00);		// LCFG2
	I2C_write_reg(0x06, 0x00);		// LCFG3
	I2C_write_reg(0x07, vlaue);		// LEDEN
	
	I2C_write_reg(0x10, 0xFF);		// ILED1
	I2C_write_reg(0x11, 0xFF);		// ILED2
	I2C_write_reg(0x12, 0xFF);		// ILED3
	I2C_write_reg(0x1C, 0xFF);		// PWM1
	I2C_write_reg(0x1D, 0xFF);		// PWM2
	I2C_write_reg(0x1E, 0xFF);		// PWM3	
	
	return 0;
}


unsigned char AW2028_LED_OFF(void)
{
	I2C_write_reg(0x00, 0x55);		// software reset	
	return 0;
}

unsigned char AW2028_LED_Blink(unsigned char r, unsigned char g, unsigned char b, unsigned int trise_ms, unsigned int ton_ms, unsigned int tfall_ms, unsigned int toff_ms)
{
	unsigned char trise, ton, tfall, toff;
	
	trise = ms2timer(trise_ms);
	ton   = ms2timer(ton_ms);
	tfall = ms2timer(tfall_ms);
	toff  = ms2timer(toff_ms);
	
	I2C_write_reg(0x00, 0x55);		// software reset

	I2C_write_reg(0x01, 0x01);		// GCR
	I2C_write_reg(0x03, 0x01);		// IMAX
	I2C_write_reg(0x04, 0x01);		// LCFG1
	I2C_write_reg(0x05, 0x01);		// LCFG2
	I2C_write_reg(0x06, 0x01);		// LCFG3
	I2C_write_reg(0x07, 0x07);		// LEDEN
	I2C_write_reg(0x08, 0x08);		// LEDCTR
	
	I2C_write_reg(0x10, r);		// ILED1
	I2C_write_reg(0x11, g);		// ILED2
	I2C_write_reg(0x12, b);		// ILED3
	I2C_write_reg(0x1C, 0xFF);		// PWM1
	I2C_write_reg(0x1D, 0xFF);		// PWM2
	I2C_write_reg(0x1E, 0xFF);		// PWM3	
	
	I2C_write_reg(0x30, (trise<<4)|ton);		// PAT_T1		Trise & Ton
	I2C_write_reg(0x31, (tfall<<4)|toff);		// PAT_T2		Tfall & Toff
	I2C_write_reg(0x32, 0x00);		// PAT_T3				Tdelay
	I2C_write_reg(0x33, 0x00);		// PAT_T4 	  PAT_CTR & Color
	I2C_write_reg(0x34, 0x00);		// PAT_T5		    Timer
	
	I2C_write_reg(0x09, 0x07);		// PAT_RIN	
	return 0;
}

unsigned char AW2028_Audio_Corss_Zero(void)
{
	I2C_write_reg(0x00, 0x55);		// software reset

	I2C_write_reg(0x01, 0x01);		// GCR
	I2C_write_reg(0x03, 0x01);		// IMAX
	I2C_write_reg(0x07, 0x07);		// LEDEN
	I2C_write_reg(0x10, 0xFF);		// ILED1
	I2C_write_reg(0x11, 0xFF);		// ILED2
	I2C_write_reg(0x12, 0xFF);		// ILED3
	I2C_write_reg(0x1C, 0xFF);		// PWM1
	I2C_write_reg(0x1D, 0xFF);		// PWM2
	I2C_write_reg(0x1E, 0xFF);		// PWM3
	
	I2C_write_reg(0x40, 0x11);		// AUDIO_CTR
	I2C_write_reg(0x41, 0x07);		// AUDIO_LEDEN
	I2C_write_reg(0x42, 0x00);		// AUDIO_FLT
	I2C_write_reg(0x43, 0x1A);		// AGC_GAIN
	I2C_write_reg(0x44, 0x1F);		// GAIN_MAX
	I2C_write_reg(0x45, 0x3D);		// AGC_CFG
	I2C_write_reg(0x46, 0x14);		// ATTH
	I2C_write_reg(0x47, 0x0A);		// RLTH
	I2C_write_reg(0x48, 0x00);		// NOISE
	I2C_write_reg(0x49, 0x02);		// TIMER
	I2C_write_reg(0x40, 0x13);		// AUDIO_CTR

	return 0;
}

unsigned char AW2028_Audio_Timer(void)
{
	I2C_write_reg(0x00, 0x55);		// software reset

	I2C_write_reg(0x01, 0x01);		// GCR
	I2C_write_reg(0x03, 0x01);		// IMAX
	I2C_write_reg(0x07, 0x07);		// LEDEN
	I2C_write_reg(0x10, 0xFF);		// ILED1
	I2C_write_reg(0x11, 0xFF);		// ILED2
	I2C_write_reg(0x12, 0xFF);		// ILED3
	I2C_write_reg(0x1C, 0xFF);		// PWM1
	I2C_write_reg(0x1D, 0xFF);		// PWM2
	I2C_write_reg(0x1E, 0xFF);		// PWM3
	
	I2C_write_reg(0x40, 0x11);		// AUDIO_CTR
	I2C_write_reg(0x41, 0x07);		// AUDIO_LEDEN
	I2C_write_reg(0x42, 0x00);		// AUDIO_FLT
	I2C_write_reg(0x43, 0x1A);		// AGC_GAIN
	I2C_write_reg(0x44, 0x1F);		// GAIN_MAX
	I2C_write_reg(0x45, 0x3D);		// AGC_CFG
	I2C_write_reg(0x46, 0x14);		// ATTH
	I2C_write_reg(0x47, 0x0A);		// RLTH
	I2C_write_reg(0x48, 0x00);		// NOISE
	I2C_write_reg(0x49, 0x00);		// TIMER
	I2C_write_reg(0x40, 0x0B);		// AUDIO_CTR
	
	return 0;
}


unsigned char AW2028_Audio(unsigned char mode)
{
	if(mode > 5)
	{
		mode = 0;
	}
	I2C_write_reg(0x00, 0x55);		// software reset

	I2C_write_reg(0x01, 0x01);		// GCR
	I2C_write_reg(0x03, 0x01);		// IMAX
	I2C_write_reg(0x07, 0x07);		// LEDEN
	I2C_write_reg(0x10, 0xFF);		// ILED1
	I2C_write_reg(0x11, 0xFF);		// ILED2
	I2C_write_reg(0x12, 0xFF);		// ILED3
	I2C_write_reg(0x1C, 0xFF);		// PWM1
	I2C_write_reg(0x1D, 0xFF);		// PWM2
	I2C_write_reg(0x1E, 0xFF);		// PWM3
	
	I2C_write_reg(0x40, (mode<<3)|0x01);		// AUDIO_CTR
	I2C_write_reg(0x41, 0x07);		// AUDIO_LEDEN
	I2C_write_reg(0x42, 0x00);		// AUDIO_FLT
	I2C_write_reg(0x43, 0x1A);		// AGC_GAIN
	I2C_write_reg(0x44, 0x1F);		// GAIN_MAX
	I2C_write_reg(0x45, 0x3D);		// AGC_CFG
	I2C_write_reg(0x46, 0x14);		// ATTH
	I2C_write_reg(0x47, 0x0A);		// RLTH
	I2C_write_reg(0x48, 0x00);		// NOISE
	I2C_write_reg(0x49, 0x00);		// TIMER
	I2C_write_reg(0x40, (mode<<3)|0x03);		// AUDIO_CTR
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static ssize_t AW2028_set_enable(struct device* cd, struct device_attribute *attr, const char* buf, size_t len)
{
	unsigned int databuf[1];
	if(1 != sscanf(buf,"%d",&databuf[0])){
		open_state = 0;
		return len;
	}

	if (open_state) {
		I2C_write_reg(0x07, databuf[0]);		// LEDEN
		open_state = databuf[0];
		return len;
	}
	
	if (databuf[0]){
		AW2028_LED_ON(databuf[0]);
		open_state = databuf[0];
	}else{
		AW2028_LED_OFF();
		open_state = 0;
	}

	return len;
}

static ssize_t AW2028_get_enable(struct device* cd, struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;
	len += snprintf(buf+len, PAGE_SIZE-len, "AW2028_enable(%d)\n",open_state);
	len += snprintf(buf+len, PAGE_SIZE-len, "AW2028_LED_OFF(void)\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 0 > enable\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "AW2028_LED_ON(LED1, LED2, LED3)\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 1-7  > enable\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;
}
static ssize_t AW2028_set_Current(struct device* cd, struct device_attribute *attr, const char* buf, size_t len)
{
	unsigned int databuf[2];
	unsigned int value=0;
	if(2 != sscanf(buf,"%d %d",&databuf[0],&databuf[1])){
		return len;
	}
	value = databuf[1];

	switch (databuf[0]) {
		case LED_1:{
			I2C_write_reg(0x10, databuf[1]); 	// ILED1
			break;
			}
		case LED_2:{
			I2C_write_reg(0x11, databuf[1]); 	// ILED2
			break;
			}
		case LED_3:{
			I2C_write_reg(0x12, databuf[1]); 	// ILED3
			break;
			}
		default:{
			I2C_write_reg(0x10, 0);		// ILED1
			I2C_write_reg(0x11, 0);		// ILED2
			I2C_write_reg(0x12, 0);		// ILED3
			}
	}
	return len;
}

static ssize_t AW2028_get_Current(struct device* cd, struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;
	len += snprintf(buf+len, PAGE_SIZE-len, "ILED1=%d\n",I2C_read_reg(0x10));
	len += snprintf(buf+len, PAGE_SIZE-len, "ILED1=%d\n",I2C_read_reg(0x11));
	len += snprintf(buf+len, PAGE_SIZE-len, "ILED1=%d\n",I2C_read_reg(0x12));
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;

}

static ssize_t AW2028_set_breath(struct device* cd, struct device_attribute *attr, const char* buf, size_t len)
{
	unsigned int databuf[1];
	if(1 != sscanf(buf,"%d",&databuf[0])){
		return len;
	}
	if(databuf[0] == 1) {
		AW2028_LED_Blink(128,128,128,1000,1000,1000,1000);
		open_state = 7;
	} else {
		AW2028_LED_OFF();
		open_state = 0;
	}

	return len;
}

static ssize_t AW2028_get_breath(struct device* cd, struct device_attribute *attr, char* buf)
{

	ssize_t len = 0;
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 0 or 1 > breath\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;

}

static ssize_t AW2028_set_mode(struct device* cd, struct device_attribute *attr,const char* buf, size_t len)
{
	unsigned int databuf[1];
	unsigned int mode = -1 ;
	if(1 != sscanf(buf,"%d",&databuf[0])){
		return len;
	}
	mode = databuf[0];
	
	if(mode != 0) {
		AW2028_Audio(mode-1);
		open_state = 7;
	} else {
		AW2028_LED_OFF();
		open_state = 0;
	}
	
	return len;

}

static ssize_t AW2028_get_mode(struct device* cd, struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 0 - 5 > breath\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;
}


static ssize_t AW2028_get_reg(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char reg_val;
	unsigned char i;
	ssize_t len = 0;
	for(i=0;i<0x4B;i++)
	{
		reg_val = I2C_read_reg(i);
		len += snprintf(buf+len, PAGE_SIZE-len, "reg%2X = 0x%2X, \n", i,reg_val);
	}

	return len;
}

static ssize_t AW2028_set_reg(struct device* cd, struct device_attribute *attr, const char* buf, size_t len)
{
	unsigned int databuf[2];
	if(2 == sscanf(buf,"%x %x",&databuf[0], &databuf[1]))
	{
		I2C_write_reg((unsigned char)databuf[0],databuf[1]);
	}
	return len;
}
/*
static ssize_t AW2028_get_TEST(struct device* cd,struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;
	len += snprintf(buf+len, PAGE_SIZE-len, "AW2028_LED_OFF(void)\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 0 > TEST\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "AW2028_LED_ON(r, g, b)\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 1  r   g   b  > TEST\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 1 255 255 255 > TEST\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "AW2028_LED_Blink(r, g, b, trise, ton, tfall, tfall)\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 2  r   g   b  trise ton tfall toff > TEST\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 2 255 255 255 1000    0 1000  1000 > TEST\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "AW2028_LED_Audio(mode)\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 3 mode > TEST\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 3   1  > TEST\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "AW2028_set_reg(cd,attr,buf,len)\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo reg value > reg\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 0x10 0xff > reg\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");

	return len;
}

static ssize_t AW2028_set_TEST(struct device* cd, struct device_attribute *attr, const char* buf, size_t len)
{
	unsigned int databuf[16];
	sscanf(buf,"%d",&databuf[0]);
	if(databuf[0] == 0) {		// OFF
		AW2028_LED_OFF();
		open_state = 0;
	} else if(databuf[0] == 1) {	//ON
		sscanf(&buf[1], "%d %d %d", &databuf[1], &databuf[2], &databuf[3]);
		AW2028_LED_ON(databuf[1], databuf[2], databuf[3]);
		open_state = 1;
	} else if(databuf[0] == 2) {	//Blink
		sscanf(&buf[1], "%d %d %d %d %d %d %d", &databuf[1], &databuf[2], &databuf[3], &databuf[4], &databuf[5], &databuf[6], &databuf[7]);
		AW2028_LED_Blink(databuf[1], databuf[2], databuf[3], databuf[4], databuf[5], databuf[6], databuf[7]);
		open_state = 1;
	} else if(databuf[0] == 3) {	//Audio
		sscanf(&buf[1], "%d", &databuf[1]);
		AW2028_Audio(databuf[1]);
		open_state = 1;
	}
	return len;
}

*/



static int AW2028_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);

	err = device_create_file(dev, &dev_attr_enable);
	err = device_create_file(dev, &dev_attr_Current);
	err = device_create_file(dev, &dev_attr_breath);
	err = device_create_file(dev, &dev_attr_mode);
	err = device_create_file(dev, &dev_attr_reg);
	return err;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int AW2028_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	unsigned char reg_value;
	unsigned char cnt = 5;
	int err = 0;
	printk("%s start\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	AW2028_i2c_client = client;

	while(cnt>0)
	{
		reg_value = I2C_read_reg(0x00);
		printk("AW2028 CHIPID=0x%2x\n", reg_value);
		if(reg_value == 0xB1)
		{
			break;
		}
		msleep(10);
		cnt--;
	}
	if(!cnt)
	{
		err = -ENODEV;
		goto exit_create_singlethread;
	}

	AW2028_create_sysfs(client);

	/*xmhxj 20180713 for test*/
	{
		AW2028_LED_Blink(128,128,128,1000,1000,1000,1000);
		open_state = 7;
	}

	return 0;

exit_create_singlethread:
	AW2028_i2c_client = NULL;
exit_check_functionality_failed:
	return err;	
}

static int AW2028_i2c_remove(struct i2c_client *client)
{
//	AW2028_i2c_client = NULL;
	i2c_set_clientdata(client, NULL);
	return 0;
}

				
static const struct i2c_device_id AW2028_i2c_id[] = {
	{ AW2028_I2C_NAME, 0 },
	{ }
};

#ifdef CONFIG_OF
static const struct of_device_id extled_of_match[] = {
	{.compatible = "awinic, aw2028_i2c"},
	{},
};
#endif

static struct i2c_driver AW2028_i2c_driver = {
    .driver = {
            .owner  = THIS_MODULE,
            .name   = AW2028_I2C_NAME,
#ifdef CONFIG_OF
				.of_match_table = extled_of_match,
#endif
    },

    .probe          = AW2028_i2c_probe,
    .remove         = AW2028_i2c_remove,
    .id_table       = AW2028_i2c_id,
		
};



static int AW2028_led_remove(struct platform_device *pdev)
{
	printk("AW2028 remove\n");
	i2c_del_driver(&AW2028_i2c_driver);
	return 0;
}

static int AW2028_led_probe(struct platform_device *pdev)
{
	int ret;

	printk("%s start!\n", __func__);
	ret = i2c_add_driver(&AW2028_i2c_driver);
	if (ret != 0) {
		printk("[%s] failed to register AW2028 i2c driver.\n", __func__);
		return ret;
	} else {
		printk("[%s] Success to register AW2028 i2c driver.\n", __func__);
	}

	
	

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id aw2028plt_of_match[] = {
	{.compatible = "awinic,aw2028_led",},
	{},
};
#endif

static struct platform_driver AW2028_led_driver = {
	.probe	 = AW2028_led_probe,
	.remove	 = AW2028_led_remove,
	.driver = {
		.name  = "aw2028_led",
	#ifdef CONFIG_OF
		.of_match_table = aw2028plt_of_match,
	#endif
	}
};

static int __init AW2028_LED_init(void) {
	int ret;
	printk("%s start\n", __func__);

	ret = platform_driver_register(&AW2028_led_driver);
	if (ret) {
		printk("****[%s] Unable to register driver (%d)\n", __func__, ret);
		return ret;
	}		
	
	return 0;
}

static void __exit AW2028_LED_exit(void) {
	printk("%s exit\n", __func__);
	platform_driver_unregister(&AW2028_led_driver);
}

module_init(AW2028_LED_init);
module_exit(AW2028_LED_exit);

MODULE_AUTHOR("<liweilei@awinic.com.cn>");
MODULE_DESCRIPTION("AWINIC AW2028 LED Driver");
MODULE_LICENSE("GPL");

