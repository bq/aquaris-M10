#ifndef BUILD_LK
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
//#include <mach/mt_typedefs.h>
#include <linux/i2c-dev.h>
#include <linux/i2c-gpio.h>
#include <linux/module.h>

typedef  unsigned char kal_uint8;


/**********************************************************
  *
  *   [I2C Slave Setting] 
  *
  *********************************************************/
#define   IT6151_EDP_SLAVE_ADDR_WRITE					(0x5C << 0)
#define   IT6151_MIPIRX_SLAVE_ADDR_WRITE 				(0x6C << 0)

#define DEVICE_NAME "IT6151"

#define IT6151_DEBUG

static struct i2c_client *it6151_mipirx = NULL;
static struct i2c_client *it6151_edp = NULL;

static const struct i2c_device_id it6151_i2c_id[] = 
{
	{"it6151_edp",		0},
	{"it6151_mipirx",	0},
};

#ifdef CONFIG_OF
static const struct of_device_id it6151_id[] = {
		{ .compatible = "it,it6151_edp" },
		{ .compatible = "it,it6151_mipirx" },
};
MODULE_DEVICE_TABLE(of, bq27520_id);
#endif

static int it6151_i2c_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

static struct i2c_driver it6151_i2c_driver = {
  .driver = {
      .name    = DEVICE_NAME,
	  #ifdef CONFIG_OF
			.of_match_table = of_match_ptr(it6151_id),
      #endif
  },
  .probe       = it6151_i2c_driver_probe,
  .id_table    = it6151_i2c_id,
};

/**********************************************************
  *
  *   [Global Variable] 
  *
  *********************************************************/
static DEFINE_MUTEX(it6151_i2c_access);



/**********************************************************
  *
  *   [I2C Function For Read/Write it6151] 
  *
  *********************************************************/
int it6151_i2c_read_byte(kal_uint8 dev_addr, kal_uint8 addr, kal_uint8 *returnData)
{
  char     cmd_buf[1]={0x00};
  char     readData = 0;
  int      ret=0;


  mutex_lock(&it6151_i2c_access);

	if(dev_addr == IT6151_MIPIRX_SLAVE_ADDR_WRITE)
	{		
		cmd_buf[0] = addr;
		ret = i2c_master_send(it6151_mipirx, cmd_buf, 1);
		if (ret < 0)
		{
			mutex_unlock(&it6151_i2c_access);
			printk("it6151_mipirx  i2c_master_send fail !!---ret=%d---\n", ret);	
			return 0;
		}
		
		cmd_buf[0] = 0x00;
		ret = i2c_master_recv(it6151_mipirx, (char *)cmd_buf, 1);
		if (ret < 0)
		{
			mutex_unlock(&it6151_i2c_access);
			printk("it6151_mipirx  i2c_master_recv fail !!---ret=%d---\n", ret);	
			return 0;
		}
		readData = cmd_buf[0];
		*returnData = readData;
		//msleep(1);
	}
	else if(dev_addr == IT6151_EDP_SLAVE_ADDR_WRITE)
	{
	    cmd_buf[0] = addr;
	    ret = i2c_master_send(it6151_edp, cmd_buf, 1);
	    if (ret < 0)
	    {
	        mutex_unlock(&it6151_i2c_access);
                printk("it6151_edp  i2c_master_send fail !!---ret=%d---\n", ret);	
	        return 0;
	    }
		
	    cmd_buf[0] = 0x00;
	    ret = i2c_master_recv(it6151_edp, (char *)cmd_buf, 1);
	    if (ret < 0)
	    {
			mutex_unlock(&it6151_i2c_access);
			printk("it6151_edp  i2c_master_recv fail !!---ret=%d---\n", ret);	
			return 0;
	    }
	    readData = cmd_buf[0];
	    *returnData = readData;
	    //msleep(1);
	}
	else
	{
		printk("[it6151_i2c_read_byte]error:  no this dev_addr! \n");
	}
	
  mutex_unlock(&it6151_i2c_access);

#ifdef IT6151_DEBUG
	/* dump write_data for check */
	printk("[KE/it6151_i2c_read_byte] dev_addr = 0x%x, read_data[0x%x] = 0x%x \n", dev_addr, addr, *returnData);
#endif

	
  return 1;
}



int it6151_i2c_write_byte(kal_uint8 dev_addr, kal_uint8 addr, kal_uint8 writeData)
{
  char    write_data[2] = {0};
  int     ret=0;

#ifdef IT6151_DEBUG
    /* dump write_data for check */
	printk("[KE/it6151_i2c_write] dev_addr = 0x%x, write_data[0x%x] = 0x%x \n", dev_addr, addr, writeData);
#endif
  
  mutex_lock(&it6151_i2c_access);
  
  write_data[0] = addr;
  write_data[1] = writeData;

	if(dev_addr == IT6151_MIPIRX_SLAVE_ADDR_WRITE)
	{
		it6151_mipirx->addr = IT6151_MIPIRX_SLAVE_ADDR_WRITE;
		ret = i2c_master_send(it6151_mipirx, write_data, 2);
		if(ret < 0)
		{
			mutex_unlock(&it6151_i2c_access);
			printk("it6151_mipirx  it6151_i2c_write_byte fail !!---ret=%d---\n", ret);	
			return 0;
		}
		//msleep(2);
	}
	else if(dev_addr == IT6151_EDP_SLAVE_ADDR_WRITE)
	{
	    it6151_edp->addr = IT6151_EDP_SLAVE_ADDR_WRITE;
	    ret = i2c_master_send(it6151_edp, write_data, 2);
	    if (ret < 0)
	    {
	        mutex_unlock(&it6151_i2c_access);
		    printk("it6151_edp  it6151_i2c_write_byte fail !!---ret=%d---\n", ret);
	        return 0;
	    }
	   //msleep(2);
	}	
	else
	{
         printk("[it6151_i2c_write_byte]error:  no this dev_addr! \n");
	}
	
  mutex_unlock(&it6151_i2c_access);
	
  return 1;
}


static int match_id(const struct i2c_client *client, const struct i2c_device_id *id)
{
	if (strcmp(client->name, id->name) == 0)
		return true;
	else
		return false;
}

static int it6151_i2c_driver_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{
  int err=0; 

	printk("[it6151_i2c_driver_probe] start!\n");

	if(match_id(client, &it6151_i2c_id[0]))
	{
	  if (!(it6151_mipirx = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) 
		{
	    err = -ENOMEM;
	    goto exit;
	  }
		
	  memset(it6151_mipirx, 0, sizeof(struct i2c_client));

	  it6151_mipirx = client;    
	}
	else if(match_id(client, &it6151_i2c_id[1]))
	{
		if (!(it6151_edp = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) 
		{
			err = -ENOMEM;
			goto exit;
		}
		
		memset(it6151_edp, 0, sizeof(struct i2c_client));
	
		it6151_edp = client; 	 
	}
	else
	{
		printk("[it6151_i2c_driver_probe] error!\n");

		err = -EIO;
		goto exit;
	}

	printk("[it6151_i2c_driver_probe] %s i2c sucess!\n", client->name);
	
  return 0;

exit:
  return err;

}


#ifndef CONFIG_OF
#define IT6151_BUSNUM 2
static struct i2c_board_info __initdata it6151_I2C[] = 
{
	{I2C_BOARD_INFO("it6151_edp", 	IT6151_EDP_SLAVE_ADDR_WRITE)},
	{I2C_BOARD_INFO("it6151_mipirx", 	IT6151_MIPIRX_SLAVE_ADDR_WRITE)},
};
#endif

static int __init it6151_init(void)
{    
  printk("[it6151_init] init start\n");
#ifndef CONFIG_OF
  i2c_register_board_info(IT6151_BUSNUM, it6151_I2C, 2);
#endif

  if(i2c_add_driver(&it6151_i2c_driver)!=0)
  {
    printk("[it6151_init] failed to register it6151 i2c driver.\n");
  }
  else
  {
    printk("[it6151_init] Success to register it6151 i2c driver.\n");
  }

  return 0;
}

static void __exit it6151_exit(void)
{
  i2c_del_driver(&it6151_i2c_driver);
}

module_init(it6151_init);
module_exit(it6151_exit);
   
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C it6151 Driver");
MODULE_AUTHOR("James Lo<james.lo@mediatek.com>");
#endif
