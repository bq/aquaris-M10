/*
 *drivers/input/touchscreen/focaltech_ex_fun.c
 *
 *FocalTech focaltech_ex_fun.c expand function for debug.
 *
 *Copyright (c) 2010  Focal tech Ltd.
 * 
 *This software is licensed under the terms of the GNU General Public
 *License version 2, as published by the Free Software Foundation, and
 *may be copied, distributed, and modified under those terms.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *Note:the error code of EIO is the general error in this file.
 */

#include "tpd.h"

#include "tpd_custom_fts.h"
#include "focaltech_ex_fun.h"

#include <linux/netdevice.h>
#include <linux/mount.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <../fs/proc/internal.h>
#include <linux/proc_fs.h>
//#include <internal.h>

int fts_6x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);

int fts_6x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);

int fts_5x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);

int fts_5x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);

int  fts_5x46_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth);

int hid_to_i2c(struct i2c_client * client);

static unsigned char CTPM_FW[] = {  //帝晶TP的固件
	#include "Lava_B501_FT5346i_DiJing0x67_V13_D01_20160304_app.i"
};

static unsigned char CTPM_FW_HXD[] = {//华兴达TP的固件
	#include "Lava_B501_FT5346i_HXD0x69_V33_D01_20160307_app.i"
};


static struct mutex g_device_mutex;

int fts_ctpm_auto_clb(struct i2c_client *client)
{
	unsigned char uc_temp = 0x00;
	unsigned char i = 0;

	/*start auto CLB */
	msleep(200);

	fts_write_reg(client, 0, FTS_FACTORYMODE_VALUE);
	/*make sure already enter factory mode */
	msleep(100);
	/*write command to start calibration */
	fts_write_reg(client, 2, 0x4);
	msleep(300);
	if ((fts_updateinfo_curr.CHIP_ID==0x11) ||(fts_updateinfo_curr.CHIP_ID==0x12) ||(fts_updateinfo_curr.CHIP_ID==0x13) ||(fts_updateinfo_curr.CHIP_ID==0x14)) //5x36,5x36i
	{
		for(i=0;i<100;i++)
		{
			fts_read_reg(client, 0x02, &uc_temp);
			if (0x02 == uc_temp ||
				0xFF == uc_temp)
			{
				/*if 0x02, then auto clb ok, else 0xff, auto clb failure*/
			    break;
			}
			msleep(20);	    
		}
	} else {
		for(i=0;i<100;i++)
		{
				fts_read_reg(client, 0, &uc_temp);
			if (0x0 == ((uc_temp&0x70)>>4))  /*return to normal mode, calibration finish*/
			{
			    break;
			}
			msleep(20);	    
		}
	}
	/*calibration OK*/
	fts_write_reg(client, 0, 0x40);  /*goto factory mode for store*/
	msleep(200);   /*make sure already enter factory mode*/
	fts_write_reg(client, 2, 0x5);  /*store CLB result*/
	msleep(300);
	fts_write_reg(client, 0, FTS_WORKMODE_VALUE);	/*return to normal mode */
	msleep(300);

	/*store CLB result OK */
	return 0;
}

/*
upgrade with *.i file
*/
int fts_ctpm_fw_upgrade_with_i_file(struct i2c_client *client)
{
	u8 *pbt_buf = NULL;
	u8 uc_tp_vendor_id = 0x79;
	u8 uc_chip_id = 0;
	int i_ret = 0;
	int fw_len = sizeof(CTPM_FW);

	/*judge the fw that will be upgraded
	* if illegal, then stop upgrade and return.
	*/

	fts_read_reg(client, FTS_REG_VENDOR_ID, &uc_tp_vendor_id);
	fts_read_reg(client, FTS_REG_CHIP_ID, &uc_chip_id);

	DBG("[Eric_FTS	FTS 3 ] upgrade 1   vendor_id = 0x%x, chip_id = 0x%x\n",uc_tp_vendor_id,uc_chip_id);

	if ((fts_updateinfo_curr.CHIP_ID==0x11) ||(fts_updateinfo_curr.CHIP_ID==0x12) ||(fts_updateinfo_curr.CHIP_ID==0x13) ||(fts_updateinfo_curr.CHIP_ID==0x14)
		||(fts_updateinfo_curr.CHIP_ID==0x55) ||(fts_updateinfo_curr.CHIP_ID==0x06) ||(fts_updateinfo_curr.CHIP_ID==0x0a) ||(fts_updateinfo_curr.CHIP_ID==0x08))
	{
		if (fw_len < 8 || fw_len > 32 * 1024) 
		{
			dev_err(&client->dev, "%s:FW length error\n", __func__);
			return -EIO;
		}

		if ((CTPM_FW[fw_len - 8] ^ CTPM_FW[fw_len - 6]) == 0xFF
		&& (CTPM_FW[fw_len - 7] ^ CTPM_FW[fw_len - 5]) == 0xFF
		&& (CTPM_FW[fw_len - 3] ^ CTPM_FW[fw_len - 4]) == 0xFF) 
		{
			/*FW upgrade */
			pbt_buf = CTPM_FW;
			/*call the upgrade function */
			if ((fts_updateinfo_curr.CHIP_ID==0x55) ||(fts_updateinfo_curr.CHIP_ID==0x08) ||(fts_updateinfo_curr.CHIP_ID==0x0a))
			{
				i_ret = fts_5x06_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
			}
			else if ((fts_updateinfo_curr.CHIP_ID==0x11) ||(fts_updateinfo_curr.CHIP_ID==0x12) ||(fts_updateinfo_curr.CHIP_ID==0x13) ||(fts_updateinfo_curr.CHIP_ID==0x14))
			{
				i_ret = fts_5x36_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
			}
			else if ((fts_updateinfo_curr.CHIP_ID==0x06))
			{
				i_ret = fts_6x06_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
			}
			if (i_ret != 0)
				dev_err(&client->dev, "%s:upgrade failed. err.\n",
						__func__);
			else if(fts_updateinfo_curr.AUTO_CLB==AUTO_CLB_NEED)
			{
				fts_ctpm_auto_clb(client);
			}
		} 
		else 
		{
			dev_err(&client->dev, "%s:FW format error\n", __func__);
			return -EBADFD;
		}
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x36))
	{
		if (fw_len < 8 || fw_len > 32 * 1024) 
		{
			dev_err(&client->dev, "%s:FW length error\n", __func__);
			return -EIO;
		}

		pbt_buf = CTPM_FW;
		i_ret = fts_6x36_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
		if (i_ret != 0)
			dev_err(&client->dev, "%s:upgrade failed. err.\n",
					__func__);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x54))
	{
		if (fw_len < 8 || fw_len > 54 * 1024) 
		{
			dev_err(&client->dev, "%s:FW length error\n", __func__);
			return -EIO;
		}

		if(uc_tp_vendor_id==0x69) //华兴达
		{
			DBG("[Eric_FTS	FTS 4 ] upgrade 2   HXD-TP \n");
			pbt_buf = CTPM_FW_HXD;
			i_ret = fts_5x46_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW_HXD));
		}
		else
		{  //帝晶TP，及默认
			DBG("[Eric_FTS	FTS 5 ] upgrade 3   DJ-TP \n");
			pbt_buf = CTPM_FW;
			i_ret = fts_5x46_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
		}
		if (i_ret != 0)
			dev_err(&client->dev, "%s:upgrade failed. err.\n",
					__func__);
	}	
	return i_ret;
}

u8 fts_ctpm_get_i_file_ver(struct i2c_client * client)
{
	u16 ui_sz;
	u8 uc_tp_vendor_id =0x79;
	//u8 uc_tp_chip_id =0x54;

	

	fts_read_reg(client, FTS_REG_VENDOR_ID, &uc_tp_vendor_id);
	//fts_read_reg(client, FTS_REG_CHIP_ID, &uc_tp_chip_id);

	if(uc_tp_vendor_id == 0x69)
	{
		ui_sz = sizeof(CTPM_FW_HXD);
	}
	else
	{
		ui_sz = sizeof(CTPM_FW);
	}

	if (ui_sz > 2)
	{
		if(fts_updateinfo_curr.CHIP_ID==0x36)
			return CTPM_FW[0x10a];
		else if(fts_updateinfo_curr.CHIP_ID==0x54)
		{
			if(uc_tp_vendor_id == 0x69)
				return CTPM_FW_HXD[ui_sz - 2];
			else
				return CTPM_FW[ui_sz - 2];
		}
		else
			return CTPM_FW[ui_sz - 2];
	}

	return 0x00;	/*default value */
}

/*update project setting
*only update these settings for COB project, or for some special case
*/
int fts_ctpm_update_project_setting(struct i2c_client *client)
{
	u8 uc_i2c_addr;	/*I2C slave address (7 bit address)*/
	u8 uc_io_voltage;	/*IO Voltage 0---3.3v;	1----1.8v*/
	u8 uc_panel_factory_id;	/*TP panel factory ID*/
	u8 buf[FTS_SETTING_BUF_LEN];
	u8 reg_val[2] = {0};
	u8 auc_i2c_write_buf[10] = {0};
	u8 packet_buf[FTS_SETTING_BUF_LEN + 6];
	u32 i = 0;
	int i_ret;

	uc_i2c_addr = client->addr;
	uc_io_voltage = 0x0;
	uc_panel_factory_id = 0x5a;


	/*Step 1:Reset  CTPM
	*write 0xaa to register 0xfc
	*/
	if(fts_updateinfo_curr.CHIP_ID==0x06 || fts_updateinfo_curr.CHIP_ID==0x36)
	{
		fts_write_reg(client, 0xbc, 0xaa);
	}
	else fts_write_reg(client, 0xfc, 0xaa);
	msleep(50);

	/*write 0x55 to register 0xfc */
	if(fts_updateinfo_curr.CHIP_ID==0x06 || fts_updateinfo_curr.CHIP_ID==0x36)
	{
		fts_write_reg(client, 0xbc, 0x55);
	}
	else fts_write_reg(client, 0xfc, 0x55);
	msleep(30);

	/*********Step 2:Enter upgrade mode *****/
	auc_i2c_write_buf[0] = 0x55;
	auc_i2c_write_buf[1] = 0xaa;
	do {
		i++;
		i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 2);
		msleep(5);
	} while (i_ret <= 0 && i < 5);


	/*********Step 3:check READ-ID***********************/
	auc_i2c_write_buf[0] = 0x90;
	auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
			0x00;

	fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);

	if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1 && reg_val[1] == fts_updateinfo_curr.upgrade_id_2)
		dev_dbg(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
			 reg_val[0], reg_val[1]);
	else
		return -EIO;

	auc_i2c_write_buf[0] = 0xcd;
	fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
	dev_dbg(&client->dev, "bootloader version = 0x%x\n", reg_val[0]);

	/*--------- read current project setting  ---------- */
	/*set read start address */
	buf[0] = 0x3;
	buf[1] = 0x0;
	buf[2] = 0x78;
	buf[3] = 0x0;

	fts_i2c_Read(client, buf, 4, buf, FTS_SETTING_BUF_LEN);
	dev_dbg(&client->dev, "[FTS] old setting: uc_i2c_addr = 0x%x,\
			uc_io_voltage = %d, uc_panel_factory_id = 0x%x\n",
			buf[0], buf[2], buf[4]);

	 /*--------- Step 4:erase project setting --------------*/
	auc_i2c_write_buf[0] = 0x63;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);
	msleep(100);

	/*----------  Set new settings ---------------*/
	buf[0] = uc_i2c_addr;
	buf[1] = ~uc_i2c_addr;
	buf[2] = uc_io_voltage;
	buf[3] = ~uc_io_voltage;
	buf[4] = uc_panel_factory_id;
	buf[5] = ~uc_panel_factory_id;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;
	packet_buf[2] = 0x78;
	packet_buf[3] = 0x0;
	packet_buf[4] = 0;
	packet_buf[5] = FTS_SETTING_BUF_LEN;

	for (i = 0; i < FTS_SETTING_BUF_LEN; i++)
		packet_buf[6 + i] = buf[i];

	fts_i2c_Write(client, packet_buf, FTS_SETTING_BUF_LEN + 6);
	msleep(100);

	/********* reset the new FW***********************/
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);

	msleep(200);
	return 0;
}

int fts_ctpm_auto_upgrade(struct i2c_client *client)
{
	u8 uc_host_fm_ver = FTS_REG_FW_VER;
	u8 uc_tp_fm_ver;
	u8 uc_tp_vendor_id = 0x79;//0xa8寄存器的值VENDOR_ID
	int i_ret;

	fts_read_reg(client, FTS_REG_FW_VER, &uc_tp_fm_ver);
	msleep(5);
	fts_read_reg(client, FTS_REG_VENDOR_ID, &uc_tp_vendor_id);
	uc_host_fm_ver = fts_ctpm_get_i_file_ver(client);

	DBG("[Eric_FTS FTS 40] uc_tp_fm_ver=0x%x, uc_tp_vendor_id=0x%x,uc_host_fm_ver=0x%x\n",uc_tp_fm_ver,uc_tp_vendor_id, uc_host_fm_ver);

	if (/*the firmware in touch panel maybe corrupted */
	uc_tp_fm_ver == FTS_REG_FW_VER ||
	/*the firmware in host flash is new, need upgrade */
	uc_tp_fm_ver < uc_host_fm_ver
	) 
	{
		msleep(100);
		dev_dbg(&client->dev, "[FTS] uc_tp_fm_ver = 0x%x, uc_host_fm_ver = 0x%x\n",
		uc_tp_fm_ver, uc_host_fm_ver);
		i_ret = fts_ctpm_fw_upgrade_with_i_file(client);

		DBG("[Eric_FTS	FTS 6] upgrade 4   i_ret = %d\n",i_ret);
		if (i_ret == 0)	
		{
			msleep(300);
			uc_host_fm_ver = fts_ctpm_get_i_file_ver(client);
			DBG("[Eric_FTS	FTS 7 ] upgrade 5   uc_host_fm_ver = 0x%x\n",uc_host_fm_ver);
			dev_dbg(&client->dev, "[FTS] upgrade to new version 0x%x\n",
			uc_host_fm_ver);
		} 
		else 
		{
			pr_err("[FTS] upgrade failed ret=%d.\n", i_ret);
			return -EIO;
		}
	}

	return 0;
}

void delay_qt_ms(unsigned long  w_ms)
{
	unsigned long i;
	unsigned long j;

	for (i = 0; i < w_ms; i++)
	{
		for (j = 0; j < 1000; j++)
		{
			 udelay(1);
		}
	}
}

int fts_6x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf,
			  u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u32 fw_length;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	


	if(pbt_buf[0] != 0x02)
	{
		DBG("[FTS] FW first byte is not 0x02. so it is invalid \n");
		return -1;
	}

	if(dw_lenth > 0x11f)
	{
		fw_length = ((u32)pbt_buf[0x100]<<8) + pbt_buf[0x101];
		if(dw_lenth < fw_length)
		{
			DBG("[FTS] Fw length is invalid \n");
			return -1;
		}
	}
	else
	{
		DBG("[FTS] Fw length is invalid \n");
		return -1;
	}
	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
		/*********Step 1:Reset  CTPM *****/
		/*write 0xaa to register 0xbc */
		
		fts_write_reg(client, 0xbc, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);

		/*write 0x55 to register 0xbc */
		fts_write_reg(client, 0xbc, FTS_UPGRADE_55);

		msleep(fts_updateinfo_curr.delay_55);

		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		fts_i2c_Write(client, auc_i2c_write_buf, 1);

		auc_i2c_write_buf[0] = FTS_UPGRADE_AA;
		fts_i2c_Write(client, auc_i2c_write_buf, 1);
		msleep(fts_updateinfo_curr.delay_readid);

		/*********Step 3:check READ-ID***********************/		
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
			0x00;
		reg_val[0] = 0x00;
		reg_val[1] = 0x00;
		fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);


		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2) {
			DBG("[FTS] Step 3: GET CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			break;
		} else {
			dev_err(&client->dev, "[FTS] Step 3: GET CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
		}
	}
	if (i >= FTS_UPGRADE_LOOP)
		return -EIO;

	auc_i2c_write_buf[0] = 0x90;
	auc_i2c_write_buf[1] = 0x00;
	auc_i2c_write_buf[2] = 0x00;
	auc_i2c_write_buf[3] = 0x00;
	auc_i2c_write_buf[4] = 0x00;
	fts_i2c_Write(client, auc_i2c_write_buf, 5);
	
	//auc_i2c_write_buf[0] = 0xcd;
	//fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);


	/*Step 4:erase app and panel paramenter area*/
	DBG("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = 0x61;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);	/*erase app area */
	msleep(fts_updateinfo_curr.delay_earse_flash);

	for(i = 0;i < 200;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		auc_i2c_write_buf[1] = 0x00;
		auc_i2c_write_buf[2] = 0x00;
		auc_i2c_write_buf[3] = 0x00;
		reg_val[0] = 0x00;
		reg_val[1] = 0x00;
		fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);
		if(0xb0 == reg_val[0] && 0x02 == reg_val[1])
		{
			DBG("[FTS] erase app finished \n");
			break;
		}
		msleep(50);
	}

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	DBG("Step 5:write firmware(FW) to ctpm flash\n");

	dw_lenth = fw_length;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) {
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) {
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		
		fts_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		
		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			auc_i2c_write_buf[1] = 0x00;
			auc_i2c_write_buf[2] = 0x00;
			auc_i2c_write_buf[3] = 0x00;
			reg_val[0] = 0x00;
			reg_val[1] = 0x00;
			fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);
			if(0xb0 == (reg_val[0] & 0xf0) && (0x03 + (j % 0x0ffd)) == (((reg_val[0] & 0x0f) << 8) |reg_val[1]))
			{
				DBG("[FTS] write a block data finished \n");
				break;
			}
			msleep(1);
		}
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) {
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}

		fts_i2c_Write(client, packet_buf, temp + 6);

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			auc_i2c_write_buf[1] = 0x00;
			auc_i2c_write_buf[2] = 0x00;
			auc_i2c_write_buf[3] = 0x00;
			reg_val[0] = 0x00;
			reg_val[1] = 0x00;
			fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);
			if(0xb0 == (reg_val[0] & 0xf0) && (0x03 + (j % 0x0ffd)) == (((reg_val[0] & 0x0f) << 8) |reg_val[1]))
			{
				DBG("[FTS] write a block data finished \n");
				break;
			}
			msleep(1);
		}
	}


	/*********Step 6: read out checksum***********************/
	/*send the opration head */
	DBG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0xcc;
	fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) {
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
					reg_val[0],
					bt_ecc);
		return -EIO;
	}

	/*********Step 7: reset the new FW***********************/
	DBG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);
	msleep(300);	/*make sure CTP startup normally */

	return 0;
}

int fts_6x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;

	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
		/*********Step 1:Reset  CTPM *****/
		/*write 0xaa to register 0xbc */
		
		fts_write_reg(client, 0xbc, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);

		/*write 0x55 to register 0xbc */
		fts_write_reg(client, 0xbc, FTS_UPGRADE_55);

		msleep(fts_updateinfo_curr.delay_55);

		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		do {
			i++;
			i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 2);
			msleep(5);
		} while (i_ret <= 0 && i < 5);


		/*********Step 3:check READ-ID***********************/
		msleep(fts_updateinfo_curr.delay_readid);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
			0x00;
		fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);


		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2) {
			DBG("[FTS] Step 3: CTPM ID OK ,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			break;
		} else {
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
		}
	}
	if (i > FTS_UPGRADE_LOOP)
		return -EIO;
	auc_i2c_write_buf[0] = 0xcd;

	fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);


	/*Step 4:erase app and panel paramenter area*/
	DBG("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = 0x61;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);	/*erase app area */
	msleep(fts_updateinfo_curr.delay_earse_flash);
	/*erase panel parameter area */
	auc_i2c_write_buf[0] = 0x63;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);
	msleep(100);

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	DBG("Step 5:write firmware(FW) to ctpm flash\n");

	dw_lenth = dw_lenth - 8;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) {
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) {
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		
		fts_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		msleep(FTS_PACKET_LENGTH / 6 + 1);
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) {
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}

		fts_i2c_Write(client, packet_buf, temp + 6);
		msleep(20);
	}


	/*send the last six byte */
	for (i = 0; i < 6; i++) {
		temp = 0x6ffa + i;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = 1;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;
		packet_buf[6] = pbt_buf[dw_lenth + i];
		bt_ecc ^= packet_buf[6];
		fts_i2c_Write(client, packet_buf, 7);
		msleep(20);
	}


	/*********Step 6: read out checksum***********************/
	/*send the opration head */
	DBG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0xcc;
	fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) {
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
					reg_val[0],
					bt_ecc);
		return -EIO;
	}

	/*********Step 7: reset the new FW***********************/
	DBG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);
	msleep(300);	/*make sure CTP startup normally */

	return 0;
}

int fts_5x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u8 is_5336_new_bootloader = 0;
	u8 is_5336_fwsize_30 = 0;
	u32  packet_number;
	u32  j;
	u32  temp;
	u32  lenght;
	u8 	packet_buf[FTS_PACKET_LENGTH + 6];
	u8  	auc_i2c_write_buf[10];
	u8  	bt_ecc;
	int	i_ret;
	int	fw_filenth = sizeof(CTPM_FW);

	if(CTPM_FW[fw_filenth-12] == 30)
	{
		is_5336_fwsize_30 = 1;
	}
	else 
	{
		is_5336_fwsize_30 = 0;
	}

	for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
    	/*********Step 1:Reset  CTPM *****/
    	/*write 0xaa to register 0xfc*/
		/*write 0xaa to register 0xfc*/
	   	fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);
		
		 /*write 0x55 to register 0xfc*/
		fts_write_reg(client, 0xfc, FTS_UPGRADE_55);   
		msleep(fts_updateinfo_curr.delay_55);   


		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		
	    i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 2);
	  
	    /*********Step 3:check READ-ID***********************/   
		msleep(fts_updateinfo_curr.delay_readid);
	   	auc_i2c_write_buf[0] = 0x90; 
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;

 

                                fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);

                                

                                if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1 

                                                && reg_val[1] == fts_updateinfo_curr.upgrade_id_2)

                                {

                                                dev_dbg(&client->dev, "[FTS] Step 3: CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);

                                                break;

                                }

                                else

                                {

                                                dev_err(&client->dev, "[FTS] Step 3: CTPM ID FAILD,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);

                                                continue;

                                }

                }

                if (i >= FTS_UPGRADE_LOOP)

                                return -EIO;

                                

                auc_i2c_write_buf[0] = 0xcd;

                fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);

	/*********20130705 mshl ********************/
	if (reg_val[0] <= 4)
	{
		is_5336_new_bootloader = BL_VERSION_LZ4 ;
	}
	else if(reg_val[0] == 7)
	{
		is_5336_new_bootloader = BL_VERSION_Z7 ;
	}
	else if(reg_val[0] >= 0x0f)
	{
		is_5336_new_bootloader = BL_VERSION_GZF ;
	}

     /*********Step 4:erase app and panel paramenter area ********************/
	if(is_5336_fwsize_30)
	{
		auc_i2c_write_buf[0] = 0x61;
		fts_i2c_Write(client, auc_i2c_write_buf, 1); /*erase app area*/	
   		 msleep(fts_updateinfo_curr.delay_earse_flash); 

		 auc_i2c_write_buf[0] = 0x63;
		fts_i2c_Write(client, auc_i2c_write_buf, 1); /*erase config area*/	
   		 msleep(50);
	}
	else
	{
		auc_i2c_write_buf[0] = 0x61;
		fts_i2c_Write(client, auc_i2c_write_buf, 1); /*erase app area*/	
   		msleep(fts_updateinfo_curr.delay_earse_flash); 
	}

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;

	if(is_5336_new_bootloader == BL_VERSION_LZ4 || is_5336_new_bootloader == BL_VERSION_Z7 )
	{
		dw_lenth = dw_lenth - 8;
	}
	else if(is_5336_new_bootloader == BL_VERSION_GZF) dw_lenth = dw_lenth - 14;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;
	for (j=0;j<packet_number;j++)
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8)(temp>>8);
		packet_buf[3] = (u8)temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8)(lenght>>8);
		packet_buf[5] = (u8)lenght;

		for (i=0;i<FTS_PACKET_LENGTH;i++)
		{
		    packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i]; 
		    bt_ecc ^= packet_buf[6+i];
		}

		fts_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH+6);
		msleep(FTS_PACKET_LENGTH/6 + 1);
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8)(temp>>8);
		packet_buf[3] = (u8)temp;

		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8)(temp>>8);
		packet_buf[5] = (u8)temp;

		for (i=0;i<temp;i++)
		{
		    packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i]; 
		    bt_ecc ^= packet_buf[6+i];
		}
  
		fts_i2c_Write(client, packet_buf, temp+6);
		msleep(20);
	}

 

                /*send the last six byte*/

                if(is_5336_new_bootloader == BL_VERSION_LZ4 || is_5336_new_bootloader == BL_VERSION_Z7 )

                {

                                for (i = 0; i<6; i++)

                                {

                                                if (is_5336_new_bootloader  == BL_VERSION_Z7 /*&& DEVICE_IC_TYPE==IC_FT5x36*/) 

                                                {

                                                                temp = 0x7bfa + i;

                                                }

                                                else if(is_5336_new_bootloader == BL_VERSION_LZ4)

                                                {

                                                                temp = 0x6ffa + i;

                                                }

                                                packet_buf[2] = (u8)(temp>>8);

                                                packet_buf[3] = (u8)temp;

                                                temp =1;

                                                packet_buf[4] = (u8)(temp>>8);

                                                packet_buf[5] = (u8)temp;

                                                packet_buf[6] = pbt_buf[ dw_lenth + i]; 

                                                bt_ecc ^= packet_buf[6];

  

                                                fts_i2c_Write(client, packet_buf, 7);

                                                msleep(10);

                                }

                }

                else if(is_5336_new_bootloader == BL_VERSION_GZF)

                {

                                for (i = 0; i<12; i++)

                                {

                                                if (is_5336_fwsize_30 /*&& DEVICE_IC_TYPE==IC_FT5x36*/) 

                                                {

                                                                temp = 0x7ff4 + i;

                                                }

                                                else if (1/*DEVICE_IC_TYPE==IC_FT5x36*/) 

                                                {

                                                                temp = 0x7bf4 + i;

                                                }

                                                packet_buf[2] = (u8)(temp>>8);

                                                packet_buf[3] = (u8)temp;

                                                temp =1;

                                                packet_buf[4] = (u8)(temp>>8);

                                                packet_buf[5] = (u8)temp;

                                                packet_buf[6] = pbt_buf[ dw_lenth + i]; 

                                                bt_ecc ^= packet_buf[6];

  
			fts_i2c_Write(client, packet_buf, 7);
			msleep(10);

		}
	}

	/*********Step 6: read out checksum***********************/
	/*send the opration head*/
	auc_i2c_write_buf[0] = 0xcc;
	fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1); 

	if(reg_val[0] != bt_ecc)
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n", reg_val[0], bt_ecc);
	    	return -EIO;
	}

	/*********Step 7: reset the new FW***********************/
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);
	msleep(300);  /*make sure CTP startup normally*/

	return 0;
}

int fts_5x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;
	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
		/*********Step 1:Reset  CTPM *****/
		/*write 0xaa to register 0xfc */
		fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);

		/*write 0x55 to register 0xfc */
		fts_write_reg(client, 0xfc, FTS_UPGRADE_55);
		msleep(fts_updateinfo_curr.delay_55);
		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		do {
			i++;
			i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 2);
			msleep(5);
		} while (i_ret <= 0 && i < 5);


		/*********Step 3:check READ-ID***********************/
		msleep(fts_updateinfo_curr.delay_readid);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
			0x00;
		fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);


		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2) {
			DBG("[FTS] Step 3: CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			break;
		} else {
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
		}
	}
	if (i >= FTS_UPGRADE_LOOP)
		return -EIO;
	/*Step 4:erase app and panel paramenter area*/
	DBG("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = 0x61;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);	/*erase app area */
	msleep(fts_updateinfo_curr.delay_earse_flash);
	/*erase panel parameter area */
	auc_i2c_write_buf[0] = 0x63;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);
	msleep(100);

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	DBG("Step 5:write firmware(FW) to ctpm flash\n");

	dw_lenth = dw_lenth - 8;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) {
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) {
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		
		fts_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		msleep(FTS_PACKET_LENGTH / 6 + 1);
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) {
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}

		fts_i2c_Write(client, packet_buf, temp + 6);
		msleep(20);
	}


	/*send the last six byte */
	for (i = 0; i < 6; i++) {
		temp = 0x6ffa + i;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = 1;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;
		packet_buf[6] = pbt_buf[dw_lenth + i];
		bt_ecc ^= packet_buf[6];
		fts_i2c_Write(client, packet_buf, 7);
		msleep(20);
	}


	/*********Step 6: read out checksum***********************/
	/*send the opration head */
	DBG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0xcc;
	fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) {
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
					reg_val[0],
					bt_ecc);
		return -EIO;
	}
		
	/*********Step 7: reset the new FW***********************/
	DBG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);
	msleep(300);	/*make sure CTP startup normally */
	return 0;
}

int hid_to_i2c(struct i2c_client * client)
{
	u8 auc_i2c_write_buf[5] = {0};
	int bRet = 0;

	auc_i2c_write_buf[0] = 0xeb;
	auc_i2c_write_buf[1] = 0xaa;
	auc_i2c_write_buf[2] = 0x09;

	fts_i2c_Write(client, auc_i2c_write_buf, 3);

	msleep(10);

	auc_i2c_write_buf[0] = auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = 0;

	fts_i2c_Read(client, auc_i2c_write_buf, 0, auc_i2c_write_buf, 3);

	if(0xeb==auc_i2c_write_buf[0] && 0xaa==auc_i2c_write_buf[1] && 0x08==auc_i2c_write_buf[2])
	{
		bRet = 1;		
	}
	else bRet = 0;

	return bRet;
	
}

int  fts_5x46_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth)
{
	
	u8 reg_val[4] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;
	int reflash_flag = 0;
	u8 uc_tp_fm_ver;

ft_fw_reflash_1:
	i_ret = hid_to_i2c(client);

	if(i_ret == 0)
	{
		DBG("[Eric_FTS	FTS 8 ] hid change to i2c fail ! \n");
	}

	for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
		msleep(100);
		DBG("[Eric_FTS FTS 9 ] Upgrade Step 1:Reset  CTPM\n"); 
		/*********Step 1:Reset	CTPM *****/
		/*write 0xaa to register 0xfc */
		i_ret = fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);

		//write 0x55 to register 0xfc 
		i_ret |= fts_write_reg(client, 0xfc, FTS_UPGRADE_55);
		
		if (i_ret < 0)
		{
			DBG("[Eric_FTS FTS 10] Upgrade Step 1 reset i2c transfer error\n");
		}
		if (i <= FTS_UPGRADE_LOOP / 2)
		{
			msleep(fts_updateinfo_curr.delay_55 + i * 3);
		}
		else
		{
			msleep(fts_updateinfo_curr.delay_55 - (i - 15) * 2);
		}
		//msleep(200);
		/*********Step 2:Enter upgrade mode *****/
		DBG("[Eric_FTS FTS 11] Upgrade Step 2:Enter upgrade mode \n");
		i_ret = hid_to_i2c(client);

		if(i_ret == 0)
		{
			DBG("[Eric_FTS	FTS 12 ] hid change to i2c fail ! \n");
			//continue;
		}
		msleep(10);
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 1);
		msleep(5);
		auc_i2c_write_buf[0] = FTS_UPGRADE_AA;
		i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 1);
		if(i_ret < 0)
		{
			DBG("[Eric_FTS FTS 13] Upgrade Step 1 reset i2c transfer error\n");
			//continue;
		}

		/*********Step 3:check READ-ID***********************/
		msleep(10);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
			0x00;
		
		reg_val[0] = reg_val[1] = 0x00;
		
		fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);

		DBG("[Eric_FTS FTS 14] Upgrade Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n", reg_val[0], reg_val[1]);
		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2) {
			DBG("[Eric_FTS FTS 15] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
				   reg_val[0], reg_val[1]);
			break;
		} else {
			DBG("[Eric_FTS FTS 16] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
				   reg_val[0], reg_val[1]);
			
			continue;
		}
	}
	if (i >= FTS_UPGRADE_LOOP )
		return -EIO;

ft_fw_reflash_4:
	/*Step 4:erase app and panel paramenter area*/
	DBG("[Eric_FTS FTS 17] Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = 0x61;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);	//erase app area 
	msleep(2500);

	for(i = 0;i < 15;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 2);

		if(0xF0==reg_val[0] && 0xAA==reg_val[1])
		{
			break;
		}
		msleep(50);
		
	}
	DBG("[Eric_FTS FTS 18] reg_val[0]=0x%x, reg_val[1]=0x%x\n",reg_val[0],reg_val[1]);

	auc_i2c_write_buf[0] = 0xB0;
	auc_i2c_write_buf[1] = (u8) ((dw_lenth >> 16) & 0xFF);
	auc_i2c_write_buf[2] = (u8) ((dw_lenth >> 8) & 0xFF);
	auc_i2c_write_buf[3] = (u8) (dw_lenth & 0xFF);

	i_ret |= fts_i2c_Write(client, auc_i2c_write_buf, 4);

	if (i_ret < 0)
	{
		DBG("[Eric_FTS FTS 19] Upgrade Step 4 i2c transfer error\n");
	}
	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	DBG("[Eric_FTS FTS 20] Step 5:write firmware(FW) to ctpm flash\n");
	temp = 0;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;
	i_ret = 0;

	for (j = 0; j < packet_number; j++) {
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) {
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		i_ret |= fts_i2c_Write(client, packet_buf, FTS_PACKET_LENGTH + 6);

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			i_ret |= fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 2);
			
			DBG("[Eric_FTS FTS 21] j =%d, i=%d, reg_val[0]= 0x%x, reg_val[1]=0x%x\n",j,i,reg_val[0],reg_val[1]);
			
			if ((j + 0x1000) == (((reg_val[0]) << 8) | reg_val[1]))
			{
				break;
			}
			msleep(1);
			
		}
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) {
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}	
		fts_i2c_Write(client, packet_buf, temp + 6);

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			i_ret |= fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 2);
			DBG("[Eric_FTS FTS 22] i=%d,reg_val[0]= 0x%x, reg_val[1]=0x%x\n",i,reg_val[0],reg_val[1]);
			if ((j + 0x1000) == (((reg_val[0]) << 8) | reg_val[1]))
			{
				break;
			}
			msleep(1);
			
		}
	}

	if (i_ret < 0)
	{
		DBG("[Eric_FTS FTS 23] Upgrade Step 5 i2c transfer error\n");
	}
	msleep(50);
	
	DBG("[Eric_FTS FTS 24] Upgrade Step 6: read out checksum\n");
	/*********Step 6: read out checksum***********************/
	/*send the opration head */
	//DBG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0x64;
	fts_i2c_Write(client, auc_i2c_write_buf, 1); 
	msleep(300);

	temp = 0;
	auc_i2c_write_buf[0] = 0x65;
	auc_i2c_write_buf[1] = (u8)(temp >> 16);
	auc_i2c_write_buf[2] = (u8)(temp >> 8);
	auc_i2c_write_buf[3] = (u8)(temp);
	temp = dw_lenth;
	auc_i2c_write_buf[4] = (u8)(temp >> 8);
	auc_i2c_write_buf[5] = (u8)(temp);
	i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 6); 
	msleep(dw_lenth/256);

	for(i = 0;i < 100;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 2);
		DBG("[Eric_FTS FTS 25] i=%d,reg_val[0]= 0x%x, reg_val[1]=0x%x\n",i,reg_val[0],reg_val[1]);
		if (0xF0==reg_val[0] && 0x55==reg_val[1])
		{
			break;
		}
		msleep(1);
			
	}
	auc_i2c_write_buf[0] = 0x66;
	fts_i2c_Read(client, auc_i2c_write_buf, 1, reg_val, 1);
	DBG("[Eric_FTS FTS 26] reg_val[0]= 0x%x, bt_ecc=0x%x\n",reg_val[0],bt_ecc);
	if (reg_val[0] != bt_ecc) 
	{
		DBG("[Eric_FTS FTS 27] Upgrade Step 6: ecc error! FW=%02x bt_ecc=%02x\n",
			   reg_val[0],
			   bt_ecc);
		if (reflash_flag)
		{
			DBG("[Eric_FTS FTS 28] check ecc failed in twice, abort\n");
			return -EIO;
		}
		else
		{
			reflash_flag++;
			DBG("[Eric_FTS FTS 29] check ecc error, reflash again\n");
			goto ft_fw_reflash_4;
		}
	}


	DBG("[Eric_FTS FTS 30] Upgrade Step 7: reset the new FW\n");
	/*********Step 7: reset the new FW***********************/
	//DBG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_Write(client, auc_i2c_write_buf, 1);
	msleep(500);	//make sure CTP startup normally 

	for(i=0; i<9; i++)
	{
		i_ret = hid_to_i2c(client);//Android to Std i2c.
		if(i_ret ==1)
		{
			DBG("[Eric_FTS FTS 31] hid_to_i2c,i = %d \n",i);
			break;
		}
	}
	
	msleep(2);
	fts_read_reg(client, FTS_REG_FW_VER, &uc_tp_fm_ver);
	msleep(2);
	DBG("[Eric_FTS FTS 32] pbt_buf[dw_lenth - 2]= 0x%x, uc_tp_fm_ver=0x%x\n",pbt_buf[dw_lenth - 2],uc_tp_fm_ver);
	if (pbt_buf[dw_lenth - 2] != uc_tp_fm_ver)
	{
		DBG("[Eric_FTS FTS 33] reflash_flag = %d,  Upgrade Step 7 reset failed\n", reflash_flag);
		if (reflash_flag < 4)
		{
			reflash_flag++;
			goto ft_fw_reflash_1;
		}
		else
		{
			DBG("[Eric_FTS FTS 34] reset error, abort\n");
			return -EIO;
		}
	}

	DBG("[Eric_FTS FTS 35] Upgrade Ok!!!\n");	
	return 0;
}


/*sysfs debug*/

/*
*get firmware size

@firmware_name:firmware name
*note:the firmware default path is sdcard.
	if you want to change the dir, please modify by yourself.
*/
static int fts_GetFirmwareSize(char *firmware_name)
{
	struct file *pfile = NULL;
	struct inode *inode;
	unsigned long magic;
	off_t fsize = 0;
	char filepath[128];
	memset(filepath, 0, sizeof(filepath));

 

                sprintf(filepath, "/storage/sdcard0/%s", firmware_name);

	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);

	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	filp_close(pfile, NULL);
	return fsize;
}



/*
*read firmware buf for .bin file.

@firmware_name: fireware name
@firmware_buf: data buf of fireware

note:the firmware default path is sdcard.
	if you want to change the dir, please modify by yourself.
*/
static int fts_ReadFirmware(char *firmware_name,
			       unsigned char *firmware_buf)
{
	struct file *pfile = NULL;
	struct inode *inode;
	unsigned long magic;
	off_t fsize;
	char filepath[128];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "/sdcard/%s", firmware_name);
	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_read(pfile, firmware_buf, fsize, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);

	return 0;
}



/*
upgrade with *.bin file
*/

int fts_ctpm_fw_upgrade_with_app_file(struct i2c_client *client,
				       char *firmware_name)
{
	u8 *pbt_buf = NULL;
	int i_ret = 0;
	int fwsize = fts_GetFirmwareSize(firmware_name);

	if (fwsize <= 0) {
		dev_err(&client->dev, "%s ERROR:Get firmware size failed\n",
					__func__);
		return -EIO;
	}

	if (fwsize < 8 || fwsize > 54 * 1024) {
		dev_dbg(&client->dev, "%s:FW length error\n", __func__);
		return -EIO;
	}

	/*=========FW upgrade========================*/
	pbt_buf = kmalloc(fwsize + 1, GFP_ATOMIC);

	if (fts_ReadFirmware(firmware_name, pbt_buf)) {
		dev_err(&client->dev, "%s() - ERROR: request_firmware failed\n",
					__func__);
		kfree(pbt_buf);
		return -EIO;
	}
	
	/*call the upgrade function */
	//i_ret = fts_ctpm_fw_upgrade(client, pbt_buf, fwsize);

	if ((fts_updateinfo_curr.CHIP_ID==0x55) ||(fts_updateinfo_curr.CHIP_ID==0x08) ||(fts_updateinfo_curr.CHIP_ID==0x0a))
	{
		i_ret = fts_5x06_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x11) ||(fts_updateinfo_curr.CHIP_ID==0x12) ||(fts_updateinfo_curr.CHIP_ID==0x13) ||(fts_updateinfo_curr.CHIP_ID==0x14))
	{
		i_ret = fts_5x36_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x06))
	{
		i_ret = fts_6x06_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x36))
	{
		i_ret = fts_6x36_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x54))
	{
		i_ret = fts_5x46_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	
	if (i_ret != 0)
		dev_err(&client->dev, "%s() - ERROR:[FTS] upgrade failed..\n",
					__func__);
	else if(fts_updateinfo_curr.AUTO_CLB==AUTO_CLB_NEED)
	{
		fts_ctpm_auto_clb(client);
	}
	
	kfree(pbt_buf);

	return i_ret;
}

static ssize_t fts_tpfwver_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	u8 fwver = 0;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	mutex_lock(&g_device_mutex);

	if (fts_read_reg(client, FTS_REG_FW_VER, &fwver) < 0)
		num_read_chars = snprintf(buf, FT_PAGE_SIZE,
					"get tp fw version fail!\n");
	else
		num_read_chars = snprintf(buf, FT_PAGE_SIZE, "%02X\n", fwver);

	mutex_unlock(&g_device_mutex);

	return num_read_chars;
}

static ssize_t fts_tpfwver_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}



static ssize_t fts_tprwreg_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/*place holder for future use*/
	return -EPERM;
}
int isNumCh(const char ch){
	int result = 0;

	if(ch >= '0' && ch <= '9'){
		result = 1;//(int)(ch - '0');
	}
	else if(ch >= 'a' && ch <= 'f'){
		result = 1;//(int)(ch - 'a') + 10;
	}
	else if(ch >= 'A' && ch <= 'F'){
		result = 1;//(int)(ch - 'A') + 10;
	}
	else{
		result = 0;
	}

	return result;
}
int hexCharToValue(const char ch){
	int result = 0;

	if(ch >= '0' && ch <= '9'){
		result = (int)(ch - '0');
	}
	else if(ch >= 'a' && ch <= 'f'){
		result = (int)(ch - 'a') + 10;
	}
	else if(ch >= 'A' && ch <= 'F'){
		result = (int)(ch - 'A') + 10;
	}
	else{
		result = -1;
	}

	return result;
}

int hexToStr(char *hex, int iHexLen, char *ch, int *iChLen)
{
	int high=0;
	int low=0;
	int tmp = 0;
	int i = 0; 
	int iCharLen = 0;
	if(hex == NULL || ch == NULL){
		return -1;
	}

	printk("iHexLen: %d in function:%s\n\n", iHexLen, __func__);

	if(iHexLen %2 == 1){		
		return -2;
	}

	for(i=0; i<iHexLen; i+=2)
	{
		high = hexCharToValue(hex[i]);
		if(high < 0){
			ch[iCharLen] = '\0';
			return -3;
		}

		low = hexCharToValue(hex[i+1]);
		if(low < 0){
			ch[iCharLen] = '\0';
			return -3;
		}
		tmp = (high << 4) + low;
		ch[iCharLen++] = (char)tmp;  
	}
	ch[iCharLen] = '\0';
	*iChLen = iCharLen;
	printk("iCharLen: %d, iChLen: %d in function:%s\n\n", iCharLen, *iChLen, __func__);
	return 0;
}
void strToBytes(char * bufStr, int iLen, char* uBytes, int *iBytesLen)
{
	int i=0;
	int iNumChLen=0;

	*iBytesLen=0;

	for(i=0; i<iLen; i++)
	{		
		if(isNumCh(bufStr[i]))//filter illegal chars
		{
			bufStr[iNumChLen++] = bufStr[i];
		}
	}

	bufStr[iNumChLen] = '\0';
	
	hexToStr(bufStr, iNumChLen, uBytes, iBytesLen);	
}

static ssize_t fts_tprwreg_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg = 0;
	u8 regaddr = 0xff, regvalue = 0xff;
	u8 valbuf[5] = {0};

	memset(valbuf, 0, sizeof(valbuf));
	mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 2) {
		if (num_read_chars != 4) {
			pr_info("please input 2 or 4 character\n");
			goto error_return;
		}
	}

	memcpy(valbuf, buf, num_read_chars);
//	retval = strict_strtoul(valbuf, 16, &wmreg);
	strToBytes((char*)buf, num_read_chars, valbuf, &retval);

	if(1==retval) 
		{
		regaddr = valbuf[0];
		retval = 0;
		}
	else if(2==retval)
		{
		regaddr = valbuf[0];
		regvalue = valbuf[1];
		retval = 0;
		}
	else retval =-1;
	

	if (0 != retval) {
		dev_err(&client->dev, "%s() - ERROR: Could not convert the "\
						"given input to a number." \
						"The given input was: \"%s\"\n",
						__func__, buf);
		goto error_return;
	}

	if (2 == num_read_chars) {
		/*read register*/
		regaddr = wmreg;
		if (fts_read_reg(client, regaddr, &regvalue) < 0)
			dev_err(&client->dev, "Could not read the register(0x%02x)\n",
						regaddr);
		else
			pr_info("the register(0x%02x) is 0x%02x\n",
					regaddr, regvalue);
	} else {
		regaddr = wmreg >> 8;
		regvalue = wmreg;
		if (fts_write_reg(client, regaddr, regvalue) < 0)
			dev_err(&client->dev, "Could not write the register(0x%02x)\n",
							regaddr);
		else
			dev_err(&client->dev, "Write 0x%02x into register(0x%02x) successful\n",
							regvalue, regaddr);
	}

error_return:
	mutex_unlock(&g_device_mutex);

	return count;
}

static ssize_t fts_fwupdate_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/* place holder for future use */
	return -EPERM;
}

/*upgrade from *.i*/
static ssize_t fts_fwupdate_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	u8 uc_host_fm_ver;
	int i_ret = 0;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	mutex_lock(&g_device_mutex);

	disable_irq(client->irq);

	DBG("[Eric_FTS	FTS 36 ] fts_fwupdate_store ! \n");
	i_ret = fts_ctpm_fw_upgrade_with_i_file(client);
	if (i_ret == 0) {
		msleep(300);
		uc_host_fm_ver = fts_ctpm_get_i_file_ver(client);
//By Gujh_Eric, 2016/3/11, 12:04:38 // 		pr_info("%s [FTS] upgrade to new version 0x%x\n", __func__,
//By Gujh_Eric, 2016/3/11, 12:04:38 // 					 uc_host_fm_ver);
		DBG("[Eric_FTS	FTS 37 ] %s [FTS] upgrade to new version 0x%x ! \n", __func__,
					 uc_host_fm_ver);
	} else
		dev_err(&client->dev, "%s ERROR:[FTS] upgrade failed.\n",
					__func__);

	DBG("[Eric_FTS	FTS 38 ]  MARK  \n");

	enable_irq(client->irq);
	mutex_unlock(&g_device_mutex);

	return count;
}

static ssize_t fts_fwupgradeapp_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/*place holder for future use*/
	return -EPERM;
}


/*upgrade from app.bin*/
static ssize_t fts_fwupgradeapp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	char fwname[128];
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "%s", buf);
	fwname[count - 1] = '\0';

	mutex_lock(&g_device_mutex);
	disable_irq(client->irq);

	fts_ctpm_fw_upgrade_with_app_file(client, fwname);

	enable_irq(client->irq);
	mutex_unlock(&g_device_mutex);

	return count;
}


/*sysfs */
/*get the fw version
*example:cat ftstpfwver
*/
static DEVICE_ATTR(ftstpfwver, S_IRUGO | S_IWUSR, fts_tpfwver_show,
			fts_tpfwver_store);

/*upgrade from *.i
*example: echo 1 > ftsfwupdate
*/
static DEVICE_ATTR(ftsfwupdate, S_IRUGO | S_IWUSR, fts_fwupdate_show,
			fts_fwupdate_store);

/*read and write register
*read example: echo 88 > ftstprwreg ---read register 0x88
*write example:echo 8807 > ftstprwreg ---write 0x07 into register 0x88
*
*note:the number of input must be 2 or 4.if it not enough,please fill in the 0.
*/
static DEVICE_ATTR(ftstprwreg, S_IRUGO | S_IWUSR, fts_tprwreg_show,
			fts_tprwreg_store);


/*upgrade from app.bin
*example:echo "*_app.bin" > ftsfwupgradeapp
*/
static DEVICE_ATTR(ftsfwupgradeapp, S_IRUGO | S_IWUSR, fts_fwupgradeapp_show,
			fts_fwupgradeapp_store);


/*add your attr in here*/
static struct attribute *fts_attributes[] = {
	&dev_attr_ftstpfwver.attr,
	&dev_attr_ftsfwupdate.attr,
	&dev_attr_ftstprwreg.attr,
	&dev_attr_ftsfwupgradeapp.attr,
	NULL
};

static struct attribute_group fts_attribute_group = {
	.attrs = fts_attributes
};

/*create sysfs for debug*/
int fts_create_sysfs(struct i2c_client *client)
{
	int err;
	err = sysfs_create_group(&client->dev.kobj, &fts_attribute_group);
	if (0 != err) {
		dev_err(&client->dev,
					 "%s() - ERROR: sysfs_create_group() failed.\n",
					 __func__);
		sysfs_remove_group(&client->dev.kobj, &fts_attribute_group);
		return -EIO;
	} else {
		mutex_init(&g_device_mutex);
		pr_info("fts:%s() - sysfs_create_group() succeeded.\n",
				__func__);
	}
	return err;
}

void fts_release_sysfs(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &fts_attribute_group);
	mutex_destroy(&g_device_mutex);
}

/*create apk debug channel*/
#define PROC_UPGRADE			0
#define PROC_READ_REGISTER		1
#define PROC_WRITE_REGISTER		2
#define PROC_AUTOCLB			4
#define PROC_UPGRADE_INFO		5
#define PROC_WRITE_DATA			6
#define PROC_READ_DATA			7


#define PROC_NAME	"ft5x0x-debug"
static unsigned char proc_operate_mode = PROC_UPGRADE;
static struct proc_dir_entry *fts_proc_entry;
static struct i2c_client *ft6x36_i2c_client = NULL;
/*interface of write proc*/
//static int fts_debug_write(struct file *filp, 	const char __user *buff, unsigned long len, void *data)
static ssize_t fts_debug_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	struct i2c_client *client = ft6x36_i2c_client;//ugrec_tky (struct i2c_client *)fts_proc_entry->data;
	unsigned char writebuf[FTS_PACKET_LENGTH];
	int buflen = len;
	int writelen = 0;
	int ret = 0;
	
	if (copy_from_user(&writebuf, buff, buflen)) {
		dev_err(&client->dev, "%s:copy from user error\n", __func__);
		return -EFAULT;
	}
	proc_operate_mode = writebuf[0];

	//DBG("[Eric_FTS FTS 80 ] fts_debug_write   proc_operate_mode =%d\n",proc_operate_mode);

	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		{
			char upgrade_file_path[128];
			memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
			sprintf(upgrade_file_path, "%s", writebuf + 1);
			upgrade_file_path[buflen-1] = '\0';
			DBG("%s\n", upgrade_file_path);
			disable_irq(client->irq);

			ret = fts_ctpm_fw_upgrade_with_app_file(client, upgrade_file_path);

			enable_irq(client->irq);
			if (ret < 0) {
				dev_err(&client->dev, "%s:upgrade failed.\n", __func__);
				return ret;
			}
		}
		break;
	case PROC_READ_REGISTER:
		writelen = 1;
		ret = fts_i2c_Write(client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_WRITE_REGISTER:
		writelen = 2;
		ret = fts_i2c_Write(client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_AUTOCLB:
		DBG("%s: autoclb\n", __func__);
		fts_ctpm_auto_clb(client);
		break;
	case PROC_READ_DATA:
	case PROC_WRITE_DATA:
		writelen = len - 1;
		ret = fts_i2c_Write(client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	default:
		break;
	}

	return len;
}

/*interface of read proc*/
//static int fts_debug_read(struct file *filp, 	const char __user *buff, unsigned long len, void *data)
static ssize_t fts_debug_read(struct file *filp, char __user *buff, size_t len, loff_t *data)
{
	struct i2c_client *client = ft6x36_i2c_client; //(struct i2c_client *)fts_proc_entry->data;
	int ret = 0;
	unsigned char buf[FT_PAGE_SIZE];
	int num_read_chars = 0;
	int readlen = 0;
	u8 regvalue = 0x00, regaddr = 0x00;
	
	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		/*after calling ft5x0x_debug_write to upgrade*/
		regaddr = 0xA6;
		ret = fts_read_reg(client, regaddr, &regvalue);
		if (ret < 0)
			num_read_chars = sprintf(buf, "%s", "get fw version failed.\n");
		else
			num_read_chars = sprintf(buf, "current fw version:0x%02x\n", regvalue);
		break;
	case PROC_READ_REGISTER:
		readlen = 1;
		ret = fts_i2c_Read(client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&client->dev, "%s:read iic error\n", __func__);
			return ret;
		} 
		num_read_chars = 1;
		break;
	case PROC_READ_DATA:
		readlen = len;
		ret = fts_i2c_Read(client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&client->dev, "%s:read iic error\n", __func__);
			return ret;
		}
		
		num_read_chars = readlen;
		break;
	case PROC_WRITE_DATA:
		break;
	default:
		break;
	}


	if (copy_to_user(buff, buf, num_read_chars)) {
		dev_err(&client->dev, "%s:copy to user error\n", __func__);
		return -EFAULT;
	}
	return num_read_chars;
}

static const struct file_operations fts_proc_fops = {
		.write = fts_debug_write,
		.read = fts_debug_read
};
int fts_create_apk_debug_channel(struct i2c_client * client)
{
	fts_proc_entry = proc_create(PROC_NAME, 0666, NULL, &fts_proc_fops);
	if (NULL == fts_proc_entry) {
		dev_err(&client->dev, "Couldn't create proc entry!\n");
		return -ENOMEM;
	} else {
		dev_info(&client->dev, "Create proc entry success!\n");
		//fts_proc_entry->data = client;
		ft6x36_i2c_client = client;
	}
	return 0;
}

void fts_release_apk_debug_channel(void)
{
	if (fts_proc_entry)
		remove_proc_entry(PROC_NAME, NULL);
}


