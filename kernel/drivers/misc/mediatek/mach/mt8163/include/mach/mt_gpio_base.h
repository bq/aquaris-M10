#ifndef _MT_GPIO_BASE_H_
#define _MT_GPIO_BASE_H_

#include <mach/sync_write.h>
#include <mach/gpio_const.h>

#define GPIO_WR32(addr, data)   mt_reg_sync_writel(data, addr)
#define GPIO_RD32(addr)         __raw_readl(addr)
//#define GPIO_SET_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) = (unsigned long)(BIT))
//#define GPIO_CLR_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) &= ~((unsigned long)(BIT)))
#define GPIO_SW_SET_BITS(BIT,REG)   GPIO_WR32(REG,GPIO_RD32(REG) | ((unsigned long)(BIT)))
#define GPIO_SET_BITS(BIT,REG)   GPIO_WR32(REG, (unsigned long)(BIT))
#define GPIO_CLR_BITS(BIT,REG)   GPIO_WR32(REG,GPIO_RD32(REG) & ~((unsigned long)(BIT)))

/*----------------------------------------------------------------------------*/
typedef struct {         /*FIXME: check GPIO spec*/   
    unsigned short val;
	unsigned short _align1;
	unsigned short set;
	unsigned short _align2;
	unsigned short rst;
	unsigned short _align3[3];
} VAL_REGS;
/*----------------------------------------------------------------------------*/
typedef struct {
   VAL_REGS dir[10];			/*0x0000 ~ 0x009F: 160  bytes*/
   u8		rsv00[96];			/*0x00a0 ~ 0x00FF: 96	 bytes*/ 
   
   VAL_REGS pullen[10]; 		/*0x0100 ~ 0x019F: 160 bytes */
   u8		rsv01[96]; 			/*0x01a0 ~ 0x01FF: 96	bytes */
   VAL_REGS pullsel[10];		/*0x0200 ~ 0x029F: 160 bytes */
   u8		rsv02[352]; 		/*0x02a0 ~ 0x3FF: 352	bytes */
   
   VAL_REGS dout[10];			/*0x0400 ~ 0x049F: 160  bytes*/
   u8		rsv03[96];			/*0x04a0 ~ 0x04FF: 96	 bytes*/
   VAL_REGS din[10];			/*0x0500 ~ 0x059F: 160 bytes*/
   u8		rsv04[96];			/*0x05a0 ~ 0x05FF: 96	 bytes*/
   VAL_REGS mode[31];			/*0x0600 ~ 0x07EF: 496 bytes*/
   u8		rsv05[144]; 		/*0x07F0 ~ 0x087F: 144  bytes*/
   VAL_REGS bank;			    /*0x0880 ~ 0x088F: 16bytes*/


   u8		rsv06[112]; 		/*0x0890 ~ 0x08FF: 112 bytes */
   VAL_REGS ies[2]; 			/*0x0900 ~ 0x091F:	32 bytes */
   VAL_REGS smt[2]; 			/*0x0920 ~ 0x093F:	32 bytes */
   u8		rsv07[192]; 		/*0x0940 ~ 0x09FF: 192 bytes */

   VAL_REGS tdsel[5];			/*0x0A00 ~ 0x0A4F: 80 bytes */
   u8		rsv08[48];			/*0x0A50 ~ 0x0A7F: 48 bytes */
   VAL_REGS rdsel[4];			/*0x0A80 ~ 0x0ABF: 64 bytes */
   
   u8		rsv9[64];			/*0x0AC0 ~ 0x0AFF: 64 bytes */
   VAL_REGS drv_sel[8];		    /*0x0B00 ~ 0x0B7F: 128 bytes */
   u8		rsv10[128]; 		/*0x0B80 ~ 0x0BFF: 128 bytes */
   
	VAL_REGS msdc0_ctrl0;	    /*0x0C00 ~ 0x0C0F:  16 bytes */
	VAL_REGS msdc0_ctrl1;		/*0x0C10 ~ 0x0C1F:  16 bytes */
	VAL_REGS msdc0_ctrl2;		/*0x0C20 ~ 0x0C2F:  16 bytes */
	VAL_REGS msdc0_ctrl3;		/*0x0C30 ~ 0x0C3F:  16 bytes */
	VAL_REGS msdc0_ctrl4;		/*0x0C40 ~ 0x0C4F:  16 bytes */
	VAL_REGS msdc0_ctrl5;		/*0x0C50 ~ 0x0C5F:  16 bytes */
	VAL_REGS msdc0_ctrl6;		/*0x0C60 ~ 0x0C6F:  16 bytes */
	VAL_REGS msdc1_ctrl0;		/*0x0C70 ~ 0x0C7F:  16 bytes */
	VAL_REGS msdc1_ctrl1;		/*0x0C80 ~ 0x0C8F:  16 bytes */
	VAL_REGS msdc1_ctrl2;		/*0x0C90 ~ 0x0C9F:  16 bytes */
	VAL_REGS msdc1_ctrl3;		/*0x0CA0 ~ 0x0CAF:  16 bytes */
	VAL_REGS msdc1_ctrl4;		/*0x0CB0 ~ 0x0CBF:  16 bytes */
	VAL_REGS msdc1_ctrl5;		/*0x0CC0 ~ 0x0CCF:  16 bytes */
	VAL_REGS msdc2_ctrl0;		/*0x0CD0 ~ 0x0CDF:  16 bytes */
	VAL_REGS msdc2_ctrl1;		/*0x0CE0 ~ 0x0CEF:  16 bytes */
	VAL_REGS msdc2_ctrl2;		/*0x0CF0 ~ 0x0CFF:  16 bytes */
	VAL_REGS msdc2_ctrl3;		/*0x0D00 ~ 0x0D0F:  16 bytes */
	VAL_REGS msdc2_ctrl4;		/*0x0D10 ~ 0x0D1F:  16 bytes */
	VAL_REGS msdc2_ctrl5;		/*0x0D20 ~ 0x0D2F:  16 bytes */

	VAL_REGS tm;			   /*0x0D30 ~ 0x0D3F:  16 bytes */
	VAL_REGS usb;			   /*0x0D40 ~ 0x0D4F:  16 bytes */
	VAL_REGS od33_ctrl[3];	   /*0x0D50 ~ 0x0D7F:  48 bytes */
	   
	u8		rsv11[16];		   /*0x0D80 ~ 0x0D8F:  16 bytes */
	
	VAL_REGS kpad_ctrl[2];	   /*0x0D90 ~ 0x0DAF:  32 bytes */
	
	VAL_REGS eint_ctrl[2];	   /*0x0DB0 ~ 0x0DCF:  32 bytes */
	
    u8		rsv12[80];		   /*0x0DD0 ~ 0x0E1F:	 80 bytes */
	
	VAL_REGS bias_ctrl[2];	   /*0x0E20 ~ 0x0E3F:  32 bytes */
	
	u8		rsv13[192]; 	   /* 0x0E40 ~ 0x0EFF: 192 bytes */
	
	
	VAL_REGS msdc3_ctrl0;		/*0x0F00 ~ 0x0F0F:  16 bytes */
	VAL_REGS msdc3_ctrl1;		/*0x0F10 ~ 0x0F1F:  16 bytes */
	VAL_REGS msdc3_ctrl2;		/*0x0F20 ~ 0x0F2F:  16 bytes */
	VAL_REGS msdc3_ctrl3;		/*0x0F30 ~ 0x0F3F:  16 bytes */
	VAL_REGS msdc3_ctrl4;		/*0x0F40 ~ 0x0F4F:  16 bytes */
	VAL_REGS msdc3_ctrl5;		/*0x0F50 ~ 0x0F5F:  16 bytes */	
	VAL_REGS msdc3_ctrl6;		/*0x0F60 ~ 0x0F6F:	16 bytes */

	
} GPIO_REGS;

#endif //_MT_GPIO_BASE_H_
