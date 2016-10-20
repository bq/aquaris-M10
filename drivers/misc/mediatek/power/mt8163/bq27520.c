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
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/module.h>

#include <mach/mt_typedefs.h>
#include <mt-plat/mt_gpio.h>
#include <linux/proc_fs.h>

#include <asm-generic/unaligned.h>
#include <linux/dma-mapping.h>

#include <mach/bq27520.h>
#if defined(CONFIG_MALATA_HARDWARE_VERSION)
extern int hardware_version;
#endif

#define bq27520_debug	1
#if bq27520_debug
#define bq27520_log	printk
#else
#define bq27520_log	pr_debug
#endif


/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define bq27520_SLAVE_ADDR_WRITE   0xAA
#define bq27520_SLAVE_ADDR_READ    0xAB

#define UPDATE_FIRMWARE_RESTART		0
#define bq27520_NORMAL_MODE 			0		//when in normal mode ,iic addr = 0x55
#define bq27520_ROM_MODE 				1		//when in normal mode ,iic addr = 0x0b

static struct i2c_client *new_client = NULL;
static struct proc_dir_entry *bq27520_proc_entry;
static int bq27520_update_flag = 0;
static int  g_bq27520_mode = bq27520_NORMAL_MODE;
static struct delayed_work bq27520_update_work;

extern kal_bool ext_fg_init_done;
static const char g_filename[]="/system/vendor/firmware/bq27520.dffs";
static const char g_df_version_file[]="/system/vendor/firmware/DF_VERSION";

static const struct i2c_device_id bq27520_i2c_id[] = { {"bq27520", 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id bq27520_id[] = {
		{ .compatible = "ti,bq27520" },
		{},
};
MODULE_DEVICE_TABLE(of, bq27520_id);
#endif


static int bq27520_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

static struct i2c_driver bq27520_driver = {
	.driver = {
		.name    = "bq27520",
		#ifdef CONFIG_OF
				.of_match_table = of_match_ptr(bq27520_id),
        #endif
	},
	.probe       	= bq27520_driver_probe,
	.id_table    	= bq27520_i2c_id,
};


/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
static DEFINE_MUTEX(bq27520_i2c_access);
/**********************************************************
  *
  *   [I2C Function For Read/Write bq27520]
  *
  *********************************************************/
int bq27520_set_cmd_read(kal_uint8 cmd, char *returnData,u8 len)
{
	char     cmd_buf[3] = {0x00, 0x00, 0x00};
	int      ret = 0;

    mutex_lock(&bq27520_i2c_access);

	cmd_buf[0] = cmd;
	
	ret = i2c_master_send(new_client, cmd_buf, 1);
	if(ret < 0)
	{  
	   printk("bq27520_set_cmd_read-i2c_master_send failed\n");
	}

    ret = i2c_master_recv(new_client, (char *)cmd_buf, len);
    if(ret < 0)
	{  
	   printk("bq27520_set_cmd_read-i2c_master_recv failed\n");
	}

    memcpy(returnData, &cmd_buf[0], len);


	msleep(1);

	mutex_unlock(&bq27520_i2c_access);

	return ret;
       

}

int bq27520_set_cmd_write(kal_uint8 cmd, u8 *WriteData,u8 len)
{
    int      ret=0;
	char     cmd_buf[3]={0x00, 0x00, 0x00};

	
	mutex_lock(&bq27520_i2c_access);

	cmd_buf[0] = cmd;
	memcpy(cmd_buf+1, WriteData, len);

	ret = i2c_master_send(new_client, cmd_buf, len+1);
	if(ret < 0)
	{  
	   printk("bq27520_set_cmd_write i2c write failed\n");
	}
		
	msleep(1);

	mutex_unlock(&bq27520_i2c_access);

	return ret;

}




/***********************************************************************
**
**[ As a interface, called by ext function]
**
***********************************************************************/
int ext_get_data_from_bq27520(kal_uint8 cmd, char *returnData,u8 len)
{
    int ret = 0;

	printk("***ext_get_data_from_bq27520, call bq27520_set_cmd_read,cmd: 0x%02x***\n",cmd);

	if( (NULL == returnData) || (0 == len) )
		return 0;

	//when bq27520 is updating firmware, return 0
	if(2 == bq27520_update_flag)
		return 0;

    //call  bq27520_set_cmd_read()
    ret = bq27520_set_cmd_read(cmd, returnData, len);
	if(ret < 0)
	{  
	   printk("ext_get_data_from_bq27520, call bq27520_set_cmd_read failed\n");
	   return 0;
	}

	//when success return 1
	return 1;
	
	
}



//note that, this platform, DMA have already been used in underlying i2c driver, according to MTK eService
/*
u8 *bq27520_dma_buff_va = NULL;
dma_addr_t bq27520_dma_buff_pa = 0;

static void msg_dma_alloct(void){
	
	new_client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	bq27520_dma_buff_va = (u8 *)dma_alloc_coherent(&new_client->dev, 1024, &bq27520_dma_buff_pa, GFP_KERNEL);  //GFP_DMA32

    if(!bq27520_dma_buff_va){
        printk("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
	return ;
    }
	memset(bq27520_dma_buff_va, 0, 255);
}

static void msg_dma_release(void){
	if(bq27520_dma_buff_va){
     	dma_free_coherent(&new_client->dev, 255, bq27520_dma_buff_va, bq27520_dma_buff_pa);
        bq27520_dma_buff_va = NULL;
        bq27520_dma_buff_pa = 0;
		printk("[DMA][release] Allocate DMA I2C Buffer release!\n");
    }
}
*/

int GetMaxValue(int x, int y)
{
   return ( (x >= y)? x : y );
}

int bq27520_set_cmd_read_bytes(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen)
{
	int ret = -1;
	u8 *w_buf = NULL;

	if((NULL == writebuf) || (NULL == readbuf))
		return -1;
	if((writelen <= 0) || (readlen <= 0))
		return -1;

	//mutex_lock(&bq27520_i2c_access);

	w_buf = kzalloc(GetMaxValue(writelen,readlen)+1, GFP_KERNEL);
	if(NULL == w_buf)
		return -1;

	//DMA Write
	memcpy(w_buf, writebuf, writelen);
	if((ret = i2c_master_send(client, (const char *)w_buf, writelen))!= writelen)
		printk("bq27520_set_cmd_read_bytes-i2c_master_send failed!\n");

	//DMA Read
	if((ret = i2c_master_recv(client, (char *)w_buf, readlen))!= readlen)
		printk("bq27520_set_cmd_read_bytes-i2c_master_recv failed!\n");

	memcpy(readbuf, w_buf, readlen);

    if(w_buf){
       kfree(w_buf);
	   w_buf = NULL;
	}
		
	udelay(80);

	//mutex_unlock(&bq27520_i2c_access);

	return ret;

}



int bq27520_set_cmd_write_bytes(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret = -1;
	u8 *w_buf = NULL;

	if((NULL == writebuf) || (writelen <= 0))
		return -1;

	//mutex_lock(&bq27520_i2c_access);
	
    w_buf = kzalloc(writelen+1, GFP_KERNEL);
	if(NULL == w_buf)
		return -1;

	memcpy(w_buf, writebuf, writelen);
	if((ret=i2c_master_send(client, (const char *)w_buf, writelen))!=writelen)
		printk("bq27520_set_cmd_write_bytes failed!\n");

    if(w_buf){
       kfree(w_buf);
	   w_buf = NULL;
	}
  
	udelay(80);
	
	//mutex_unlock(&bq27520_i2c_access); 

	return ret;

}



static void bq27520_unseal_device(void)
{
	u8 buf[2];

	if(g_bq27520_mode == bq27520_NORMAL_MODE){
		/*W: AA 00 14 04*/
		buf[0] = 0x14;
		buf[1] = 0x04;
		bq27520_set_cmd_write(0x00,buf,2);

		/*W: AA 00 72 36*/
		buf[0] = 0x72;
		buf[1] = 0x36;
		bq27520_set_cmd_write(0x00,buf,2);

		/*W: AA 00 FF FF*/
		buf[0] = 0xFF;
		buf[1] = 0xFF;
		bq27520_set_cmd_write(0x00,buf,2);

		/*W: AA 00 FF FF*/
		buf[0] = 0xFF;
		buf[1] = 0xFF;
		bq27520_set_cmd_write(0x00,buf,2);

		/*X: 1000*/
		msleep(2000);
	}
}


static void bq27520_update_enter_rom_mode(void)
{
	u8 buf[2];
	int ret = 0;

	if(g_bq27520_mode == bq27520_NORMAL_MODE){
		/*W: AA 00 00 0F*/
		buf[0] = 0x00;
		buf[1] = 0x0F;
		ret = bq27520_set_cmd_write(0x00,buf,2);
	}
	/*X: 1000*/
	msleep(1000);
	new_client->addr = 0x0B;//when enter rom mode,the iic addr is 0x0B
}


/*
static void bq27520_exit_rom_mode(void)
{
	u8 buf[2];

	new_client->addr = 0x0b;


	buf[0] = 0x0f;
	bq27520_set_cmd_write(0x00,buf,1);

	buf[0] = 0x0f;
	buf[1] = 0x00;
	bq27520_set_cmd_write(0x64,buf,2);
	msleep(4000);
    	new_client->addr = 0x55;
}
*/

void bq27520_soft_reset(void)
{
	u8 buf[2];

	if(g_bq27520_mode == bq27520_NORMAL_MODE){
		buf[0] = 0x41;						//soft reset
		buf[1] = 0x00;
		bq27520_set_cmd_write(0x00,buf,2);
	}
}

void bq27520_sealed(void)
{
	u8 buf[2];

	if(g_bq27520_mode == bq27520_NORMAL_MODE){
		buf[0] = 0x20;						//seal
		buf[1] = 0x00;
		bq27520_set_cmd_write(0x00,buf,2);
	}
}

static struct file * update_file_open(char * path, mm_segment_t * old_fs_p)
{
	struct file * filp = NULL;

	filp = filp_open(path, O_RDONLY, 0644);

	if(!filp || IS_ERR(filp))
	{
		printk(KERN_ERR "The update file for Guitar open error.\n");
		return NULL;
	}
	*old_fs_p = get_fs();
	set_fs(get_ds());

	filp->f_op->llseek(filp,0,0);
	return filp ;
}

static void update_file_close(struct file * filp, mm_segment_t old_fs)
{
	set_fs(old_fs);
	if(filp)
		filp_close(filp, NULL);
}

static int update_get_flen(char * path)
{
	struct file * file_ck = NULL;
	mm_segment_t old_fs;
	int length ;

	file_ck = update_file_open(path, &old_fs);
	if(file_ck == NULL)
		return 0;

	length = file_ck->f_op->llseek(file_ck, 0, SEEK_END);
	printk("******File length: %d**********\n", length);
	if(length < 0)
		length = 0;
	update_file_close(file_ck, old_fs);
	return length;
}

/*
static int bq27520_write_batt_insert(struct i2c_client *client)
{
	int ret = 0;
	int control_status = 0;
	u8 buf[2];
	int flags = 0;

	if(g_bq27520_mode == bq27520_NORMAL_MODE){
		buf[0] = 0x00;	//CONTROL_STATUS
		buf[1] = 0x00;
		bq27520_set_cmd_write(0x00,buf,2);

		ret = bq27520_set_cmd_read(0x00,buf,2);
		control_status = get_unaligned_le16(buf);
		bq27520_log("control_status=0x%x \n",control_status);

		ret = bq27520_set_cmd_read(bq27520_CMD_Flags,buf,2);
		if (ret < 0) {
			printk("error reading flags\n");
			return ret;
		}
		flags = get_unaligned_le16(buf);

		bq27520_log("1 Enter:%s --flags = 0x%x\n",__FUNCTION__,flags);

		if (!(flags & bq27520_FLAG_BAT_DET))
		{
			bq27520_log("*******begin battery insert!*******\n");
			buf[0] = 0x0D;				//batt insert
			buf[1] = 0x00;
			bq27520_set_cmd_write(0x00,buf,2);
			//udelay(80);

			ret = bq27520_set_cmd_read(bq27520_CMD_Flags,buf,2);
			if (ret < 0) {
				printk("error reading flags\n");
				return ret;
			}
			flags = get_unaligned_le16(buf);
			if (!(flags & bq27520_FLAG_BAT_DET)){
				printk("27520: %s, flags = 0x%x, set virtual_battery_enable = 1\n",__FUNCTION__,flags);
				virtual_battery_enable = 1;
			}
		}
	}

	return 0;
}
*/
	
int bq27520_write_batt_temperature(int temp)
{
#if 0
	int ret = 0;
	int temp_k = 0;
	int read_temp = 0;
	u8 buf[2];
	char old_op_conf_b_byte = 0;
	char new_op_conf_b_byte = 0;
	char old_checksum =0;
	char new_checksum = 0;

	/*unseal device*/
	bq27520_unseal_device();

	printk("*********enable block data flash control!***********\n");
	/*enable block data flash control: W: AA 61 00*/
	buf[0] = 0x61;
	buf[1] = 0x00;
	bq27520_set_cmd_write_bytes(new_client, buf, 2);
	//udelay(80);

	printk("*********access the registers subclass!***********\n");
	/*access the registers subclass: W: AA 3E 40*/
	buf[0] = 0x3E;
	buf[1] = 0x40;
	bq27520_set_cmd_write_bytes(new_client, buf, 2);
	//udelay(80);

	printk("*********Write the block offset location!***********\n");
	/*Write the block offset location: W: AA 3F 00*/
	buf[0] = 0x3F;
	buf[1] = 0x00;
	bq27520_set_cmd_write_bytes(new_client, buf, 2);
	//udelay(80);

	printk("*********read the data of OpConfig B!***********\n");
	/*read the data of OpConfig B: R: AA 4B*/
	bq27520_set_cmd_read(0x4b,&old_op_conf_b_byte,1);

	printk("*********read the 1-byte checksum!***********\n");
	/*read the 1-byte checksum: R: AA 60*/
	bq27520_set_cmd_read(0x60,&old_checksum,1);

	printk("*********write new_op_conf_b_byte!***********\n");
	/*set WRTEMP by bit7: W: AA 4B new_op_conf_b_byte_LSB new_op_conf_b_byte_MSB*/
	new_op_conf_b_byte = old_op_conf_b_byte | 0x80;
	buf[0] = new_op_conf_b_byte;
	bq27520_set_cmd_write(0x4B, buf,1);

	printk("*******write new_checksum!***********\n");
	/* transferre new_checksum to the data flash: W: AA 60 new_checksum*/
	new_checksum = 255 - ((255 - old_checksum - old_op_conf_b_byte)%256 + new_op_conf_b_byte)%256;
	buf[0] = new_checksum;
	bq27520_set_cmd_write(0x60, buf,1);
	msleep(2000);

	printk("old_op_conf_b_byte=0x%X, old_checksum=0x%X, new_op_conf_b_byte=0x%X, new_checksum=0x%X\n", old_op_conf_b_byte, old_checksum, new_op_conf_b_byte, new_checksum);
	/*ensure the new data flash parameter goes into effect: W: AA 00 41 00*/
	buf[0] = 0x41;
	buf[1] = 0x00;
	bq27520_set_cmd_write(0x00, buf,2);
	msleep(2000);

	/*write temperature : W : AA 06 temp_k_LSB temp_k_MSB*/
	temp_k = ((temp * 10) + 2731);
	if(g_bq27520_mode == bq27520_NORMAL_MODE){
		buf[0] = (temp_k & 0xFF);
		buf[1] = ((temp_k >> 8) & 0xFF);
		bq27520_set_cmd_write(bq27520_CMD_Temperature,buf,2);
		msleep(3000); //delay 3 seconds
		printk("bq27520_write_batt_temperature temp = %d ret = %d\n",temp,ret);

		buf[0] =0;
		buf[1] =0;
		ret = bq27520_set_cmd_read(bq27520_CMD_Temperature,buf,2);
		read_temp = get_unaligned_le16(buf);
		read_temp -= 2731;
		read_temp = read_temp/10;
		printk("bq27520 read back batt_temperature = %d \n",read_temp);
	}

	/*seal device*/
	bq27520_sealed();
#else
	int ret = 0;
	int temp_k = 0;
	u8 buf[2];

	/*write temperature : W : AA 06 temp_k_LSB temp_k_MSB*/
	temp_k = ((temp * 10) + 2731);
	if(g_bq27520_mode == bq27520_NORMAL_MODE){
		buf[0] = (temp_k & 0xFF);
		buf[1] = ((temp_k >> 8) & 0xFF);
		bq27520_set_cmd_write(bq27520_CMD_Temperature,buf,2);
		//msleep(3000); //delay 3 seconds
		bq27520_log("bq27520_write_batt_temperature temp = %d ret = %d\n",temp,ret);
	}
#endif
	return 0;
}



//when bq27520 firmware update OK,write flag to register DF_VERSION
static int write_version_when_bq27520_updated(u16 nNewDfVersion)
{
	u8 buf[2];
	u16 new_df_version = nNewDfVersion;   //bq27520_FLASH_UPDATE_VERSION
	u8 old_df_version_h,old_df_version_l;
	u8 old_checksum =0;
	u8 new_checksum = 0;

    /*unseal device*/
	bq27520_unseal_device();

	/*enable block data flash control: W: AA 61 00*/
	buf[0] = 0x00;
	bq27520_set_cmd_write(0x61, buf, 1);

    /*access the registers subclass: W: AA 3E 30*/
	buf[0] = 0x30;
	bq27520_set_cmd_write(0x3e, buf, 1);

    /*Write the block offset location: W: AA 3F 00*/
	buf[0] = 0x00;
	bq27520_set_cmd_write(0x3f, buf, 1);

	/*read the data of DF_VERSION: R: AA 59*/
	/*0x40 + 25%32 = 0x59*/
	//mind bq27520 data flash big-endian.but when we read old_df_version out, we can see it is 0,different from RD 0x00 0x1f 0x00.
	bq27520_set_cmd_read(0x59,buf,2);
	old_df_version_h = buf[0];
	old_df_version_l = buf[1];

	/*read the 1-byte checksum: R: AA 60*/
	bq27520_set_cmd_read(0x60,&old_checksum,1);
	printk("****read the 1-byte checksum :0x%x****\n",old_checksum);

    /*write the new DF_VERSION*/
	buf[0] = new_df_version & 0xff;
	buf[1] = (new_df_version >> 8) & 0xff;
	bq27520_set_cmd_write(0x59, buf,2);
	printk("***write the new DF_VERSION :0x%x***\n",new_df_version);

    /* transfer new_checksum to the data flash: W: AA 60 new_checksum*/
	new_checksum = 255 - ((255 - old_checksum - (old_df_version_h + old_df_version_l) )%256 + ( (new_df_version & 0xff) + (new_df_version >> 8) ))%256;
	buf[0] = new_checksum;
	bq27520_set_cmd_write(0x60, buf,1);
	msleep(2000);

	/*ensure the new data flash parameter goes into effect: W: AA 00 41 00*/
	buf[0] = 0x41;
	buf[1] = 0x00;
	bq27520_set_cmd_write(0x00, buf,2);

	/*seal device*/
	bq27520_sealed();

	return 0;


}


/*************************************************************************
**read_file_DF_VERSION()*****************************************************
**function:get df_version from file DF_VERSION*************************************
**return value : df_version > 0*************************************************
*************************************************************************/
static u16 read_file_DF_VERSION(const char *path,unsigned long len)
{
	u16 df_version = 0;
	struct file * file = NULL;
	mm_segment_t old_fs;
	int file_len;
	loff_t pos = 0;
	char buf[50] = {0};
	char val_buf[10] = {0};
	int ret;

	if( (NULL == path)||(0 == len) )
		return 0;

	//get file lenth
	file_len = update_get_flen((char *)path);
	if(file_len <= 0)
		return 0;

	//open file DF_VERSION
	file = update_file_open((char *)path, &old_fs);
	if(NULL == file)
	{
		return 0;
	}

	//Read file
	printk("****read file DF_VERSION********\n");
	ret = vfs_read(file,buf,len, &pos);
	if(ret <= 0)
	{
		update_file_close(file, old_fs);
		return 0;
	}

	memcpy(val_buf,buf,4);

	//str to Hex
	df_version = (u16)simple_strtoul(val_buf, NULL, 16);
	printk("func %s, df_version in config file is : 0x%x\n",__func__,df_version);

	//Close file
	update_file_close(file, old_fs);

	return df_version;
}



/*************************************************************
**get old df_version*****************************************
**************************************************************/
static u16 get_old_df_version(void)
{
	u8  buf[2];
	u16 df_version = 0;

	buf[0] = 0x1f;
	buf[1] = 0x00;
	bq27520_set_cmd_write(0x00,buf,2);
	bq27520_set_cmd_read(0x00,buf,2);
	df_version = buf[0]*256 + buf[1];  //mind bq27520 data flash big-endian

	return df_version;
}

int bq27520_set_hibernate_mode(int enable)
{
	int ret =0;
	char buf[2];

	/*run to hibernate mode: W: AA 00 14 04*/
	buf[0] = 0x11;
	buf[1] = (char)enable;
	ret = bq27520_set_cmd_write(bq27520_CMD_ControlStatus, buf, 2);

	return ret;
}

static int bq27520_update_fw(struct file *filp,  char *buff, unsigned long len, void *data)
{
	struct file * file_data = NULL;
	mm_segment_t old_fs;
	int file_len;
	char *data_buf = NULL;
	int ret = -1;
	char buf[128];
	char read_buf[4];
	char *pch = NULL;
	s32 base = 16;
	int count = 0;
	int wCount = 0;
	int xCount = 0;
	int cCount = 0;
    	int i=0;
	#define isspace(c)	((c) == ' ')

	file_len = update_get_flen(buff);

	///Open update file.
	file_data = update_file_open(buff, &old_fs);
	if(file_data == NULL)
	{
		return -1;
	}
	data_buf = kzalloc(file_len, GFP_KERNEL);

	if(file_len > 0)
	{
		ret = file_data->f_op->read(file_data, data_buf, file_len, &file_data->f_pos);
		//printk("get file strlen(data_buf): %d.\n", strlen(data_buf));

		pch = data_buf;

		while(count < file_len)
		{
			if(*pch == 'W')
			{
				wCount = 0;
				pch +=2;
				count +=2;
				while(*pch != 'W' && *pch != 'X' && *pch != 'C')
				{
					while(isspace(*pch))
					{
						pch++;
						count ++;
					}

					ret = simple_strtoul(pch, NULL, base);
					buf[wCount] = ret;
					wCount++;
					bq27520_log("W:%x ", ret);
					pch +=2;
					count +=2;
					while((*pch == '\r') || (*pch == '\n'))
					{
						pch +=2;
						count +=2;
					}
				}
				bq27520_log("\n");
				bq27520_log("\n bq27520_update_fw :W count = %d , wCount=%d\n", count, wCount);
				ret = bq27520_set_cmd_write_bytes(new_client,&buf[1],wCount - 1);

				bq27520_log("bq27520_update_fw:W ret =%d ", ret);
				if(ret < 0)
					break;
			}
			else if(*pch == 'X')
			{
				xCount = 0;
				pch +=2;
				count +=2;
				while(*pch != 'W' && *pch != 'X' && *pch != 'C')
				{
					while(isspace(*pch))
					{
						pch++;
						count ++;
					}

					ret = simple_strtoul(pch, NULL, 10);
					bq27520_log("X:%d ", ret);
					xCount++;

					if(ret < 0x10)
					{
						pch +=1;
						count +=1;
					}
					else if(ret < 0x100){
						pch +=2;
						count +=2;
					}
					else if(ret < 0x1000){
						pch +=3;
						count +=3;
					}
					else if(ret < 0x10000){
						pch +=4;
						count +=4;
					}

					while((*pch == '\r') || (*pch == '\n'))
					{
						pch +=2;
						count +=2;
					}

					if(ret == 4000)
					{
						msleep(ret);
						count = file_len;
						break;
					}
					msleep(ret);
				}
				bq27520_log("\n bq27520_update_fw:X count = %d, xCount=%d \n", count, xCount);
				bq27520_log("\n");
			}
			else if(*pch == 'C')
			{
				cCount = 0;
				pch +=2;
				count +=2;
				while(*pch != 'W' && *pch != 'X' && *pch != 'C')
				{
					while(isspace(*pch))
					{
						pch++;
						count ++;
					}

					ret = simple_strtoul(pch, NULL, base);
					buf[cCount] = ret;
					bq27520_log("C:%x ", ret);
					cCount++;

					pch +=2;
					count +=2;
					while((*pch == '\r') || (*pch == '\n'))
					{
						pch +=2;
						count +=2;

						break;
					}
				}
				bq27520_log("\n");
				bq27520_log("\n bq27520_update_fw: C count = %d , cCount=%d", count, cCount);
				ret = bq27520_set_cmd_read_bytes(new_client, &buf[1], 1, &read_buf[0], cCount - 2);
				bq27520_log("bq27520_update_fw:C ret =%d\n", ret);
                for(i=0; i < (cCount - 2); i++){
					   bq27520_log("read_buf[%d] =%x ", i, read_buf[i]);
					   if(buf[i+2] != read_buf[i])
						   printk("read checksum error!");
                }
                bq27520_log("\n");
				if(ret < 0)
					break;
			}
		}
	}

	if(data_buf){
		kfree(data_buf);
		data_buf = NULL;
	}
	///Close file
	update_file_close(file_data, old_fs);

	return ret;
}



static int bq27520_update_flash_data(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
	int ret = 0;
	u8 buf[2];

//	mutex_lock(&bq27520_i2c_access);

	bq27520_log("***begin unseal devices***\n");
	bq27520_unseal_device();

	bq27520_log("***begin enter rom mode****\n");
	bq27520_update_enter_rom_mode();

	ret = bq27520_set_cmd_read(0x66,buf,2);
	bq27520_log("***set_cmd_read OK!:ret is %d***\n",ret);
	if(ret < 0)
		bq27520_update_flag = 1;
	else{
		mutex_lock(&bq27520_i2c_access);
		ret = bq27520_update_fw(filp, (char *)buff, len, data);
		mutex_unlock(&bq27520_i2c_access);
	}
	g_bq27520_mode = bq27520_NORMAL_MODE;
	new_client->addr = 0x55;

	bq27520_log("***begin soft reset********\n");
	bq27520_soft_reset();

	bq27520_log("***begin seal devices******\n");
	bq27520_sealed();

	if(ret > 0)
	{
		printk("%s,success!\n", __FUNCTION__);
	}
	else{
		printk("%s,failed!\n", __FUNCTION__);
	}

//	mutex_unlock(&bq27520_i2c_access);

	return ret;
}



//judge whether or not we update firmware of bq27520 according to Register  DF_VERSION
static u8 whether_update_bq27520_firmware(u16 nDfVersionNow)
{
	int ret = 0;
	u16 old_df_version = 0;

	//get the old df_version
	old_df_version = get_old_df_version();
	bq27520_log("***bq27520 old df_version = 0x%x***********\n",old_df_version);

	if(0 == old_df_version) //a new board ,need update,default value of Register  DF_VERSION of bq27520:0x0000
	{
		ret = 1;
		bq27520_log("****a new board,begin update bq27520 firmware***\n");
	}
	else
	{
		if(nDfVersionNow != old_df_version) //new df_version, update firmware
		{
			ret = 1;
			bq27520_log("***a new df_version:0x%x,begin update bq27520 firmware***\n",nDfVersionNow);
		}
		else
		{
			bq27520_log("***the same df_version, needn't to update bq27520 firmware!!!***\n");
		}
	}

	return ret;
}



/***************************************************************************************
**Generally, in manual updating firmware mode, after updating firmware, if we do not restart the system, we ******
**should write new df_version to REG DF_VERSION  in order to get correct old_df_version next time .************
**We find that after updating firmware,if we do not write new df_version to REG DF_VERSION,and do not restart****
**the system,registers couldn't read back correctly in manual updating firmware mode. So,after updating firmware,**
**we should restart the system, or write new df_version to REG DF_VERSION******************************
****************************************************************************************/
ssize_t bq27520_update_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	int ret = 0;
	int ret_val = 0;
	char update_file[100];
	u16 df_version_cfg = 0;

	//get df_version in config file
	df_version_cfg = read_file_DF_VERSION(g_df_version_file,6);
	if(0 == df_version_cfg)
	{
		return len;
	}

	//judge whether or not we update firmware of bq27520
	if(0 == whether_update_bq27520_firmware(df_version_cfg))
	{
		bq27520_update_flag = 7; //the same df_version, already latest!
		return len; 
	}

	//begin update
	memset(update_file, 0, 100);

	if(copy_from_user(&update_file, buff, len))
	{
		return -EFAULT;
	}

	update_file[len-1] = 0;
	ret = update_get_flen(update_file);
	if(ret == 0){
		printk("bq27520_update_write:update_get_flen error.\n");
		return -EFAULT;
	}

	printk("update file: %ld-[%s].\n", len, update_file);
	if(bq27520_update_flag !=2)
	{
		bq27520_update_flag = 2;
		msleep(500);
		ret_val = bq27520_update_flash_data(filp, update_file, len, data);
		if(ret_val < 0){
			printk("bq27520_update_flash_data failed!retry again\n");
			ret_val = bq27520_update_flash_data(filp, update_file, len, data);
		}
	}

	if(ret_val > 0)
	{
		printk("%s,update firmware successfully!\n", __FUNCTION__);
		bq27520_update_flag = 3;
		msleep(200);

		//write new df_version to DF_VERSION register
		write_version_when_bq27520_updated(df_version_cfg);
	}else if(!ret_val){
		printk("bq27520_update_write:updating now.\n");
	}else{
		printk("bq27520_update_write:update failed.\n");
		bq27520_update_flag = 4;
	}

	return len;
}

static int bq27520_update_show(struct seq_file *m, void *v)
{
	printk("bq27520_update_show: bq27520_update_flag=%d\n", bq27520_update_flag);
	seq_printf(m, "%d\n", bq27520_update_flag);
	return 0;
}

int bq27520_update_open(struct inode *inode, struct file *file)
{
	return single_open(file, bq27520_update_show, NULL);
}

static const struct file_operations bq27520_proc_fops = {
	.open = bq27520_update_open,
	.write = bq27520_update_write,
	.read = seq_read,
};


static void bq27520_update_firmware(u16 nDfVersionNow)
{
	struct file *filp;
	int retval;
	int retval_bq = 0;
	unsigned long len = sizeof(g_filename);

	retval = update_get_flen((char *)g_filename);
	if(retval == 0){
		printk("bq27520_battery_update_work:update_get_flen error.\n");
		return;
	}

    //why judge bq27520_update_flag? when kernel starting,this function just do once, but,in order to update firmware accurately,we can do more than once.
	if(bq27520_update_flag != 2) 
	{
		bq27520_update_flag = 2;
		msleep(100);
		retval_bq = bq27520_update_flash_data(filp, g_filename, len, NULL);
		if(retval_bq > 0)
		{
			printk("bq27520_battery_update_work:update success.\n");
			bq27520_update_flag = 3;
			msleep(200);

			//write REG DF_VERSION
			printk("*****begin write DF_VERSION!*******\n");
		    write_version_when_bq27520_updated(nDfVersionNow);

		}
		else if(retval_bq < 0)
		{
			printk("bq27520_battery_update_work:update failed.\n");
			bq27520_update_flag = 4;
		}
	}
	else
	{
		printk("bq27520_battery_update_work: on updating.....\n");
	}

}


static void bq27520_battery_update_work(struct work_struct *work)
{
	u16 df_version_cfg = 0;

	
	//get df_version in config file
	df_version_cfg = read_file_DF_VERSION(g_df_version_file,6);
	if(0 == df_version_cfg)
		return;


	//judge whether or not we update firmware of bq27520
	if(whether_update_bq27520_firmware(df_version_cfg))
	{
		bq27520_update_firmware(df_version_cfg);
	}
	
}

static int bq27520_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err=0;

#if defined(CONFIG_MALATA_HARDWARE_VERSION)
	if (MATCH_BQ27520_HARDWARE_VERSION != hardware_version)
	{
		printk("[bq27520_driver_probe] hardware_version is %d, don't support bq27520!!!\n",hardware_version);
		return -ENODEV;
	}
#endif

	printk("**bq27520 addr is:0x%02x**\n",client->addr);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
		client->addr = 0x0B;
		if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
			return -ENODEV;
		}
		else{
			g_bq27520_mode = bq27520_ROM_MODE;
		}
	}
	else{
		g_bq27520_mode = bq27520_NORMAL_MODE;
	}
	printk("g_bq27520_mode=%d \n", g_bq27520_mode);

	if (!(new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}
	memset(new_client, 0, sizeof(struct i2c_client));

	new_client = client;

	//msg_dma_alloct();
    //bq27520_write_batt_insert(client);
    //bq27520_write_batt_temperature(bat_temp);

	bq27520_proc_entry = proc_create("bq27520-update", 0666, NULL, &bq27520_proc_fops);
	if (bq27520_proc_entry == NULL)
	{
		printk("create_proc_entry bq27520-update failed\n");
	}

	INIT_DELAYED_WORK(&bq27520_update_work, bq27520_battery_update_work);
	if(get_old_df_version() != 0)//20160805-xmyyq-a new board ,don;t need update bq27520 data
		schedule_delayed_work(&bq27520_update_work, msecs_to_jiffies(10 * 1000)); //update

	ext_fg_init_done = KAL_TRUE;

	return 0;

exit:
	return err;
}


/**********************************************************
  *
  *   [platform_driver API]
  *
  *********************************************************/
#define bq27520_BUSNUM 2

//static struct i2c_board_info __initdata i2c_bq27520 = { I2C_BOARD_INFO("bq27520", (0xaa>>1))};

static int __init bq27520_init(void)
{
	printk("[bq27520_init] init start\n");

#ifndef CONFIG_OF
	    i2c_register_board_info(bq27520_BUSNUM, &i2c_bq27520, 1);
#endif
	
	if(i2c_add_driver(&bq27520_driver)!=0)
	{
		printk("[bq27520_init] failed to register bq27520 i2c driver.\n");
	}
	else
	{
		printk("[bq27520_init] Success to register bq27520 i2c driver.\n");
	}

	return 0;
}

static void __exit bq27520_exit(void)
{
	i2c_del_driver(&bq27520_driver);
}

module_init(bq27520_init);
module_exit(bq27520_exit);

MODULE_AUTHOR("YT Lee<yt.lee@mediatek.com>");
MODULE_DESCRIPTION("I2C bq27520 Driver");
MODULE_LICENSE("GPL");
