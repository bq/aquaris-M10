/******************************************************************************
 * mt_gpio_base.c - MTKLinux GPIO Device Driver
 * 
 * Copyright 2008-2009 MediaTek Co.,Ltd.
 * 
 * DESCRIPTION:
 *     This file provid the other drivers GPIO relative functions
 *
 ******************************************************************************/

#include <mach/sync_write.h>
//#include <mach/mt_reg_base.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_gpio_core.h>
#include <mach/mt_gpio_base.h>
//autogen
#include <mach/gpio_cfg.h>
#ifdef CONFIG_OF
#include <linux/of_address.h>
#endif
#ifdef CONFIG_MD32_SUPPORT
#include <linux/workqueue.h>
#include <mach/md32_ipi.h>
#endif

spinlock_t		 mtk_gpio_lock;
/******************************************************************************/
/*-------for special kpad pupd-----------*/
struct kpad_pupd {
	unsigned char pin;
	unsigned char reg;
	unsigned char bit;
};
static struct kpad_pupd kpad_pupd_spec[] = {
	{GPIO33, 0, 2},	/* KROW0 */
	{GPIO34, 0, 6},	/* KROW1 */
	{GPIO35, 0, 10},/* KROW2 */
	{GPIO36, 1, 2},	/* KCOL0 */
	{GPIO37, 1, 6},	/* KCOL1 */
	{GPIO38, 1, 10}	/* KCOL2 */
};

/*---------------------------------------*/


struct mt_ies_smt_set {
	unsigned char index_start;
	unsigned char index_end;
	unsigned char reg_index;
	unsigned char bit;
};
static struct mt_ies_smt_set mt_ies_smt_map[] = {
	{GPIO0, GPIO9, 0, 0},    /*IES0/SMT0*/
	{GPIO10, GPIO13, 0, 1},  /*IES1/SMT1*/
	{GPIO14, GPIO28, 0, 2},  /*IES2/SMT2*/
	{GPIO29, GPIO32, 1, 3},  /*IES3/SMT3*/
	{GPIO33, GPIO33, 1, 11}, /*IES27/SMT27*/
	{GPIO34, GPIO38, 0, 10},  /*IES10/SMT10*/
	{GPIO39, GPIO42, 0, 11},  /*IES11/SMT11*/
	{GPIO43, GPIO45, 0, 12},  /*IES12/SMT12*/
	{GPIO46, GPIO49, 0, 13},  /*IES13/SMT13*/
	{GPIO50, GPIO52, 1, 10},  /*IES26/SMT26*/
	{GPIO53, GPIO56, 0, 14},  /*IES14/SMT14*/
	{GPIO57, GPIO58, 1, 0},  /*IES16/SMT16*/
	{GPIO59, GPIO65, 1, 2},  /*IES18/SMT18*/
	{GPIO66, GPIO71, 1, 3}, /*IES19/SMT19*/
	{GPIO72, GPIO74, 1, 4}, /*IES20/SMT20*/
	{GPIO75, GPIO76, 0, 15}, /*IES15/SMT15*/
	{GPIO77, GPIO78, 1, 1},  /*IES17/SMT17*/
	{GPIO79, GPIO82, 1, 5},  /*IES21/SMT21*/
	{GPIO83, GPIO84, 1, 6},  /*IES22/SMT22*/
	{GPIO85, GPIO90, 0, 0},  /*MSDC2*/
	{GPIO91, GPIO100, 0, 0},  /*TDP/TCP/TCN */
	{GPIO101, GPIO116, 0, 0},  /*GPI*/
	{GPIO117, GPIO120, 1, 7},/*IES23/SMT23*/
	{GPIO121, GPIO126, 0, 0},  /*MSDC1*/
	{GPIO127, GPIO137, 0, 0},  /*MSDC0*/
	{GPIO138, GPIO141, 1, 9},/*IES25/SMT25*/
	{GPIO142, GPIO142, 0, 13},/*IES13/SMT13*/
	{GPIO143, GPIO154, 0, 0},  /*MSDC3*/
};

/*---------------------------------------*/

#ifdef CONFIG_MD32_SUPPORT
struct GPIO_IPI_Packet
{
    u32 touch_pin;
};
static struct work_struct work_md32_cust_pin;
static struct workqueue_struct *wq_md32_cust_pin;
#endif

//unsigned long GPIO_COUNT=0;
//unsigned long uart_base;

struct mt_gpio_vbase gpio_vbase;
static GPIO_REGS *gpio_reg;
/*---------------------------------------------------------------------------*/
int mt_set_gpio_dir_base(unsigned long pin, unsigned long dir)
{
    unsigned long pos;
    unsigned long bit;
    GPIO_REGS *reg = gpio_reg;

    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;
    
    if (dir == GPIO_DIR_IN)
        GPIO_SET_BITS((1L << bit), &reg->dir[pos].rst);
    else
        GPIO_SET_BITS((1L << bit), &reg->dir[pos].set);
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/

int mt_get_gpio_dir_base(unsigned long pin)
{    
    unsigned long pos;
    unsigned long bit;
    unsigned long data;
    GPIO_REGS *reg = gpio_reg;

    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;
    
    data = GPIO_RD32(&reg->dir[pos].val);
    return (((data & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
/*
int mt_set_gpio_pull_enable_base(unsigned long pin, unsigned long enable)
{

    if(PULLEN_offset[pin].offset == -1){
	  return GPIO_PULL_EN_UNSUPPORTED;
    }
    else{
    	  if (enable == GPIO_PULL_DISABLE)
		GPIO_SET_BITS((1L << (PULLEN_offset[pin].offset)), PULLEN_addr[pin].addr + 8);
	  else
		GPIO_SET_BITS((1L << (PULLEN_offset[pin].offset)), PULLEN_addr[pin].addr + 4);
    }

    return RSUCCESS;
}
*/

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_enable_base(unsigned long pin, unsigned long enable)
{
	unsigned long pos;
	unsigned long bit;
	unsigned long i;
	GPIO_REGS *reg = gpio_reg;

	/*for special kpad pupd, NOTE DEFINITION REVERSE!!! */
	/*****************for special kpad pupd, NOTE DEFINITION REVERSE!!!*****************/
	for (i = 0; i < sizeof(kpad_pupd_spec) / sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
			if (enable == GPIO_PULL_DISABLE) {
				GPIO_SET_BITS((3L << (kpad_pupd_spec[i].bit - 2)),
					      &reg->kpad_ctrl[kpad_pupd_spec[i].reg].rst);
			} else {
				GPIO_SET_BITS((1L << (kpad_pupd_spec[i].bit - 2)), &reg->kpad_ctrl[kpad_pupd_spec[i].reg].set);	/* single key: 75K */
			}
			return RSUCCESS;
		}
	}

	/********************************* MSDC special *********************************/
	if (pin == GPIO127) {	/* ms0 dat7 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<13, &reg->msdc0_ctrl4.rst);
		} else {
			GPIO_SET_BITS(1L<<14, &reg->msdc0_ctrl4.set);	/* 1L:10K */
		}
		return RSUCCESS;
	}else if (pin == GPIO128) {	/* ms0 dat6 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<9, &reg->msdc0_ctrl4.rst);
		} else {
			GPIO_SET_BITS(1L<<10, &reg->msdc0_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO129) {	/* ms0 dat5*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<5, &reg->msdc0_ctrl4.rst);
		} else {
			GPIO_SET_BITS(1L<<6, &reg->msdc0_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO130) {	/* ms0 dat4*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<1, &reg->msdc0_ctrl4.rst);
		} else {
			GPIO_SET_BITS(1L<<2, &reg->msdc0_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO131) {	/* ms0 RST */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<1, &reg->msdc0_ctrl5.rst);
		} else {
			GPIO_SET_BITS(1L<<2, &reg->msdc0_ctrl5.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO132) {	/* ms0 cmd */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<9, &reg->msdc0_ctrl1.rst);
		} else {
			GPIO_SET_BITS(1L<<10, &reg->msdc0_ctrl1.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO133) {	/* ms0 clk */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<9, &reg->msdc0_ctrl0.rst);
		} else {
			GPIO_SET_BITS(1L<<10, &reg->msdc0_ctrl0.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO134) {	/* ms0 dat3 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<13, &reg->msdc0_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L<<14, &reg->msdc0_ctrl3.set);
		}
		return RSUCCESS;
	}else if (pin == GPIO135) {	/* ms0 dat2 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<9, &reg->msdc0_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L<<10, &reg->msdc0_ctrl3.set);
		}
		return RSUCCESS;
	}else if (pin == GPIO136) {	/* ms0 dat1*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<5, &reg->msdc0_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L<<6, &reg->msdc0_ctrl3.set);
		}
		return RSUCCESS;
	}else if (pin == GPIO137) {	/* ms0 dat0 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<1, &reg->msdc0_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L<<2, &reg->msdc0_ctrl3.set);
		}
		return RSUCCESS;
	
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO121) {	/* ms1 cmd */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<9, &reg->msdc1_ctrl1.rst);
		} else {
			GPIO_SET_BITS(1L<<10, &reg->msdc1_ctrl1.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO122) {	/* ms1 clk */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<9, &reg->msdc1_ctrl0.rst);
		} else {
			GPIO_SET_BITS(1L<<10, &reg->msdc1_ctrl0.set);
		}
		return RSUCCESS;
	}else if (pin == GPIO123) {	/* ms1 dat0 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<1, &reg->msdc1_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L<<2, &reg->msdc1_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO124) {	/* ms1 dat1 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 5), &reg->msdc1_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 6), &reg->msdc1_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO125) {	/* ms1 dat2 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 9), &reg->msdc1_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 10), &reg->msdc1_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO126) {	/* ms1 dat3 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 13), &reg->msdc1_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 14), &reg->msdc1_ctrl3.set);
		}
		return RSUCCESS;
	 
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO85) {	/* ms2 cmd */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<9, &reg->msdc2_ctrl1.rst);
		} else {
			GPIO_SET_BITS(1L<<10, &reg->msdc2_ctrl1.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO86) {	/* ms2 clk */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<9, &reg->msdc2_ctrl0.rst);
		} else {
			GPIO_SET_BITS(1L<<10, &reg->msdc2_ctrl0.set);
		}
		return RSUCCESS;	
	}else if (pin == GPIO87) {	/* ms2 dat0 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<1, &reg->msdc2_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L<<2, &reg->msdc2_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO88) {	/* ms2 dat1 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 5), &reg->msdc2_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 6), &reg->msdc2_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO89) {	/* ms2 dat2 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 9), &reg->msdc2_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 10), &reg->msdc2_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO90) {	/* ms2 dat3 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 13), &reg->msdc2_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 14), &reg->msdc2_ctrl3.set);
		}
		return RSUCCESS;
	
		/* //////////////////////////////////////////////// */
	}else if (pin == GPIO143) {	/* ms3 dat7*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 13, &reg->msdc3_ctrl4.rst);
		} else {
			GPIO_SET_BITS(1L << 14, &reg->msdc3_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO144) {/* ms3 dat6 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 9), &reg->msdc3_ctrl4.rst);
		} else {
			GPIO_SET_BITS((1L << 10), &reg->msdc3_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO145) {/* ms3 dat5*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 5), &reg->msdc3_ctrl4.rst);
		} else {
			GPIO_SET_BITS((1L << 6), &reg->msdc3_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO146) {/* ms3 dat4*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 1), &reg->msdc3_ctrl4.rst);
		} else {
			GPIO_SET_BITS((1L << 2), &reg->msdc3_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO147) {/* ms3 RST */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<1, &reg->msdc3_ctrl5.rst);
		} else {
			GPIO_SET_BITS(1L<<2, &reg->msdc3_ctrl5.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO148) {/* ms3 cmd */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 9, &reg->msdc3_ctrl1.rst);
		} else {
			GPIO_SET_BITS(1L << 10, &reg->msdc3_ctrl1.set);
		}
		return RSUCCESS;
	}else if (pin == GPIO149) {	/* ms3 clk */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 9, &reg->msdc3_ctrl0.rst);
		} else {
			GPIO_SET_BITS(1L << 10, &reg->msdc3_ctrl0.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO150) {/* ms3 dat3*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 13, &reg->msdc3_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L << 14, &reg->msdc3_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO151) {/* ms3 dat2 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 9), &reg->msdc3_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 10), &reg->msdc3_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO152) {/* ms3 dat1*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 5), &reg->msdc3_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 6), &reg->msdc3_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO153) {/* ms3 dat0*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 1), &reg->msdc3_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 2), &reg->msdc3_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO154) {/* ms3 DS */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<5, &reg->msdc3_ctrl5.rst);
		} else {
			GPIO_SET_BITS(1L<<6, &reg->msdc3_ctrl5.set);	/* 1L:10K */
		}
		return RSUCCESS;
	}

	if (0) {
		return GPIO_PULL_EN_UNSUPPORTED;
	} else {
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;

		if (enable == GPIO_PULL_DISABLE)
			GPIO_SET_BITS((1L << bit), &reg->pullen[pos].rst);
		else
			GPIO_SET_BITS((1L << bit), &reg->pullen[pos].set);
	}
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_enable_base(unsigned long pin)
{
    unsigned long pos;
	unsigned long bit;
	unsigned long data;
	GPIO_REGS *reg = gpio_reg;
	unsigned long i;
	/*****************for special kpad pupd, NOTE DEFINITION REVERSE!!!*****************/
	for (i = 0; i < sizeof(kpad_pupd_spec) / sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
			return (((GPIO_RD32(&reg->kpad_ctrl[kpad_pupd_spec[i].reg].val) &
				  (3L << (kpad_pupd_spec[i].bit - 2))) != 0) ? 1 : 0);
		}
	}

	/********************************* MSDC special *********************************/
	if (pin == GPIO127) {	/* ms0 dat7 */		
		return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (3L << 13)) != 0) ? 1 : 0);
	}else if (pin == GPIO128) {	/* ms0 dat6 */
		return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO129) {	/* ms0 dat5*/
		return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (3L << 5)) != 0) ? 1 : 0);
	} else if (pin == GPIO130) {	/* ms0 dat4*/
		return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO131) {	/* ms0 RST */
		return (((GPIO_RD32(&reg->msdc0_ctrl5.val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO132) {	/* ms0 cmd */		
		return (((GPIO_RD32(&reg->msdc0_ctrl1.val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO133) {	/* ms0 clk */		
		return (((GPIO_RD32(&reg->msdc0_ctrl0.val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO134) {	/* ms0 dat3 */		
		return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (3L << 13)) != 0) ? 1 : 0);
	}else if (pin == GPIO135) {	/* ms0 dat2 */
	    return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (3L << 9)) != 0) ? 1 : 0);
	}else if (pin == GPIO136) {	/* ms0 dat1*/		
		return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (3L << 5)) != 0) ? 1 : 0);
	}else if (pin == GPIO137) {	/* ms0 dat0 */		
		return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (3L << 1)) != 0) ? 1 : 0);
	
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO121) {	/* ms1 cmd */		
		return (((GPIO_RD32(&reg->msdc1_ctrl1.val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO122) {	/* ms1 clk */	
		return (((GPIO_RD32(&reg->msdc1_ctrl0.val) & (3L << 9)) != 0) ? 1 : 0);
	}else if (pin == GPIO123) {	/* ms1 dat0 */		
		return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO124) {	/* ms1 dat1 */		
		return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (3L << 5)) != 0) ? 1 : 0);
	} else if (pin == GPIO125) {	/* ms1 dat2 */		
		return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO126) {	/* ms1 dat3 */		
		return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (3L << 13)) != 0) ? 1 : 0);
	 
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO85) {	/* ms2 cmd */		
		return (((GPIO_RD32(&reg->msdc2_ctrl1.val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO86) {	/* ms2 clk */
		return (((GPIO_RD32(&reg->msdc2_ctrl0.val) & (3L << 9)) != 0) ? 1 : 0);
	}else if (pin == GPIO87) {	/* ms2 dat0 */
		return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO88) {	/* ms2 dat1 */
		return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (3L << 5)) != 0) ? 1 : 0);
	} else if (pin == GPIO89) {	/* ms2 dat2 */
		return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO90) {	/* ms2 dat3 */
		return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (3L << 13)) != 0) ? 1 : 0);
	
		/* //////////////////////////////////////////////// */
	}else if (pin == GPIO143) {	/* ms3 dat7*/
		return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (3L << 13)) != 0) ? 1 : 0);
	} else if (pin == GPIO144) {/* ms3 dat6 */
		return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO145) {/* ms3 dat5*/		
		return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (3L << 5)) != 0) ? 1 : 0);
	} else if (pin == GPIO146) {/* ms3 dat4*/		
		return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO147) {/* ms3 RST */
		return (((GPIO_RD32(&reg->msdc3_ctrl5.val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO148) {/* ms3 cmd */		
		return (((GPIO_RD32(&reg->msdc3_ctrl1.val) & (3L << 9)) != 0) ? 1 : 0);
	}else if (pin == GPIO149) {	/* ms3 clk */		
		return (((GPIO_RD32(&reg->msdc3_ctrl0.val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO150) {/* ms3 dat3*/		
		return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (3L << 13)) != 0) ? 1 : 0);
	} else if (pin == GPIO151) {/* ms3 dat2 */		
		return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO152) {/* ms3 dat1*/		
		return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (3L << 5)) != 0) ? 1 : 0);
	} else if (pin == GPIO153) {/* ms3 dat0*/		
		return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO154) {/* ms3 DS */
		return (((GPIO_RD32(&reg->msdc3_ctrl5.val) & (3L << 5)) != 0) ? 1 : 0);
	}

	if (0) {
		return GPIO_PULL_EN_UNSUPPORTED;
	} else {
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;
		data = GPIO_RD32(&reg->pullen[pos].val);
	}
	return (((data & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_smt_base(unsigned long pin, unsigned long enable)
{
	    int i = 0;
		GPIO_REGS *reg = gpio_reg;
	
		for (i = 0; i < 19; i++) {
				if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
					if (enable == GPIO_SMT_DISABLE)
						GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								   &reg->ies[mt_ies_smt_map[i].reg_index].rst);
					else
						GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								   &reg->ies[mt_ies_smt_map[i].reg_index].set); 
					}
			}
	
	
		if(i == 19)
			{
			/*only set smt,ies not set*/
			  if(pin == GPIO85){				
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc2_ctrl1.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc2_ctrl1.set);
				   }
			  }else if(pin == GPIO86){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc2_ctrl0.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc2_ctrl0.set);
				   }
			  }else if(pin == GPIO87){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 3, &reg->msdc2_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 3, &reg->msdc2_ctrl3.set);
				   }
			  }else if(pin == GPIO88){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 7, &reg->msdc2_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 7, &reg->msdc2_ctrl3.set);
				   }
			  }else if(pin == GPIO89){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc2_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc2_ctrl3.set);
				   }
			  }else if(pin == GPIO90){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 15, &reg->msdc2_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 15, &reg->msdc2_ctrl3.set);
				   }
			  }
			 i++;
			  
			}
		i=i+3;/*similar to TDP/RDP/.../not uesd*/
		if (i == 22) {
				if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
					if (enable == GPIO_IES_DISABLE)
						GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								   &reg->ies[mt_ies_smt_map[i].reg_index].rst);
					else
						GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								   &reg->ies[mt_ies_smt_map[i].reg_index].set); 
					}
			}
		i++;
		if(i == 23)
			{
			/*only set smt,ies not set*/
			  if(pin == GPIO121){
				
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc1_ctrl1.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc1_ctrl1.set);
				   }
			  }else if(pin == GPIO122){
				
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc1_ctrl0.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc1_ctrl0.set);
				   }
			  }else if(pin == GPIO123){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 3, &reg->msdc1_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 3, &reg->msdc1_ctrl3.set);
				   }
			  }else if(pin == GPIO124){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 7, &reg->msdc1_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 7, &reg->msdc1_ctrl3.set);
				   }
			  }else if(pin == GPIO125){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc1_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc1_ctrl3.set);
				   }
			  }else if(pin == GPIO126){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 15, &reg->msdc1_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 15, &reg->msdc1_ctrl3.set);
				   }
			  }
			 i++;
			  
			}
		if(i == 24)
			{
			/*only set smt,ies not set*/
			  if(pin == GPIO127){
				
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 15, &reg->msdc0_ctrl4.rst);
				   } else {
					   GPIO_SET_BITS(1L << 15, &reg->msdc0_ctrl4.set);
				   }
			  }else if(pin == GPIO128){
				
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc0_ctrl4.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc0_ctrl4.set);
				   }
			  }else if(pin == GPIO129){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 7, &reg->msdc0_ctrl4.rst);
				   } else {
					   GPIO_SET_BITS(1L << 7, &reg->msdc0_ctrl4.set);
				   }
			  }else if(pin == GPIO130){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 3, &reg->msdc0_ctrl4.rst);
				   } else {
					   GPIO_SET_BITS(1L << 3, &reg->msdc0_ctrl4.set);
				   }
			  }else if(pin == GPIO131){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 3, &reg->msdc0_ctrl5.rst);
				   } else {
					   GPIO_SET_BITS(1L << 3, &reg->msdc0_ctrl5.set);
				   }
			  }
			  else if(pin == GPIO132){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc0_ctrl1.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc0_ctrl1.set);
				   }
			  }else if(pin == GPIO133){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc0_ctrl0.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc0_ctrl0.set);
				   }
			  }else if(pin == GPIO134){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 15, &reg->msdc0_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 15, &reg->msdc0_ctrl3.set);
				   }
			  }else if(pin == GPIO135){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc0_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc0_ctrl3.set);
				   }
			  }else if(pin == GPIO136){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 7, &reg->msdc0_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 7, &reg->msdc0_ctrl3.set);
				   }
			  }else if(pin == GPIO137){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 3, &reg->msdc0_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 3, &reg->msdc0_ctrl3.set);
				   }
			  }
			 i++;
			  
			}
		for (;i < 27; i++ ) {
				if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
					if (enable == GPIO_IES_DISABLE)
						GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								   &reg->ies[mt_ies_smt_map[i].reg_index].rst);
					else
						GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								   &reg->ies[mt_ies_smt_map[i].reg_index].set); 
					}
			}
		if(i == 27)
			{
			/*only set smt,ies not set*/
			  if(pin == GPIO143){
				
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 15, &reg->msdc3_ctrl4.rst);
				   } else {
					   GPIO_SET_BITS(1L << 15, &reg->msdc3_ctrl4.set);
				   }
			  }else if(pin == GPIO144){
				
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc3_ctrl4.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc3_ctrl4.set);
				   }
			  }else if(pin == GPIO145){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 7, &reg->msdc3_ctrl4.rst);
				   } else {
					   GPIO_SET_BITS(1L << 7, &reg->msdc3_ctrl4.set);
				   }
			  }else if(pin == GPIO146){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 3, &reg->msdc3_ctrl4.rst);
				   } else {
					   GPIO_SET_BITS(1L << 3, &reg->msdc3_ctrl4.set);
				   }
			  }else if(pin == GPIO147){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc3_ctrl5.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc3_ctrl5.set);
				   }
			  }else if(pin == GPIO148){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc3_ctrl1.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc3_ctrl1.set);
				   }
			  }else if(pin == GPIO149){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc3_ctrl0.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc3_ctrl0.set);
				   }
			  }else if(pin == GPIO150){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 15, &reg->msdc3_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 15, &reg->msdc3_ctrl3.set);
				   }
			  }else if(pin == GPIO151){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 11, &reg->msdc3_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 11, &reg->msdc3_ctrl3.set);
				   }
			  }else if(pin == GPIO152){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 7, &reg->msdc3_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 7, &reg->msdc3_ctrl3.set);
				   }
			  }else if(pin == GPIO153){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 3, &reg->msdc3_ctrl3.rst);
				   } else {
					   GPIO_SET_BITS(1L << 3, &reg->msdc3_ctrl3.set);
				   }
			  }else if(pin == GPIO154){			
				   if (enable == GPIO_SMT_DISABLE) {
					   GPIO_SET_BITS(1L << 3, &reg->msdc3_ctrl5.rst);
				   } else {
					   GPIO_SET_BITS(1L << 3, &reg->msdc3_ctrl5.set);
				   }
			  }		  
			  
			}		

	  if (i > ARRAY_SIZE(mt_ies_smt_map)) {
		return -ERINVAL;
	  }
	  return RSUCCESS;

    }
/*---------------------------------------------------------------------------*/
int mt_get_gpio_smt_base(unsigned long pin)
{
#if 0
    unsigned long data;

    if(SMT_offset[pin].offset == -1){
	  return GPIO_SMT_UNSUPPORTED;
    }
    else{
	  data = GPIO_RD32(SMT_addr[pin].addr);

          return (((data & (1L << (SMT_offset[pin].offset))) != 0)? 1: 0);
    }
#endif

   	int i = 0;
	unsigned long data;
	GPIO_REGS *reg = gpio_reg;

	for (i = 0; i < 19; i++) {
				if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
					data = GPIO_RD32(&reg->ies[mt_ies_smt_map[i].reg_index].val);
	                return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
					}
			}
	
	
		if(i == 19)
			{
			/*only set smt,ies not set*/
			  if(pin == GPIO85){				  
				   return (((GPIO_RD32(&reg->msdc2_ctrl1.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO86){				
				   return (((GPIO_RD32(&reg->msdc2_ctrl0.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO87){					  
				   return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (1L << 3)) != 0) ? 1 : 0);
			  }else if(pin == GPIO88){					  
				   return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (1L << 7)) != 0) ? 1 : 0);
			  }else if(pin == GPIO89){				 
				   return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO90){				  
				   return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (1L << 15)) != 0) ? 1 : 0);
			  }
			 i++;
			  
			}
		i=i+3;/*similar to TDP/RDP/.../not uesd*/
		if (i == 22) {
				if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){					 
					data = GPIO_RD32(&reg->ies[mt_ies_smt_map[i].reg_index].val);
	                return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
					}
			}
		i++;
		if(i == 23)
			{
			/*only set smt,ies not set*/
			  if(pin == GPIO121){				   
				   return (((GPIO_RD32(&reg->msdc1_ctrl1.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO122){			    
				   return (((GPIO_RD32(&reg->msdc1_ctrl0.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO123){					   
				   return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (1L << 3)) != 0) ? 1 : 0);
			  }else if(pin == GPIO124){					   
				   return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (1L << 7)) != 0) ? 1 : 0);
			  }else if(pin == GPIO125){						   
				   return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO126){					   
				   return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (1L << 15)) != 0) ? 1 : 0);
			  }
			 i++;
			  
			}
		if(i == 24)
			{
			/*only set smt,ies not set*/
			  if(pin == GPIO127){				 
				   return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (1L << 15)) != 0) ? 1 : 0);
			  }else if(pin == GPIO128){				 
				    return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO129){				    
				    return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (1L << 7)) != 0) ? 1 : 0);
			  }else if(pin == GPIO130){					   
				    return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (1L << 3)) != 0) ? 1 : 0);
			  }else if(pin == GPIO131){				   
				    return (((GPIO_RD32(&reg->msdc0_ctrl5.val) & (1L << 3)) != 0) ? 1 : 0);
			  }else if(pin == GPIO132){				   
				    return (((GPIO_RD32(&reg->msdc0_ctrl1.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO133){					   
				    return (((GPIO_RD32(&reg->msdc0_ctrl0.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO134){			
				    return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (1L << 15)) != 0) ? 1 : 0);
			  }else if(pin == GPIO135){				   
				   return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO136){				    
				    return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (1L << 7)) != 0) ? 1 : 0);
			  }else if(pin == GPIO137){				    
				    return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (1L << 3)) != 0) ? 1 : 0);
			  }
			 i++;
			  
			}
		for (;i < 27; i++ ) {
				if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
					 
					  data = GPIO_RD32(&reg->ies[mt_ies_smt_map[i].reg_index].val);
	                  return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
					}
			}
		if(i == 27)
			{
			/*only set smt,ies not set*/
			  if(pin == GPIO143){				
				   return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (1L << 15)) != 0) ? 1 : 0);
			  }else if(pin == GPIO144){
				   return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO145){					  
				   return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (1L << 7)) != 0) ? 1 : 0);
			  }else if(pin == GPIO146){			
				   return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (1L << 3)) != 0) ? 1 : 0);
			  }else if(pin == GPIO147){			
				   return (((GPIO_RD32(&reg->msdc3_ctrl5.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO148){			
				   return (((GPIO_RD32(&reg->msdc3_ctrl1.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO149){			
				   return (((GPIO_RD32(&reg->msdc3_ctrl0.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO150){			
				   return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (1L << 15)) != 0) ? 1 : 0);
			  }else if(pin == GPIO151){			
				    return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (1L << 11)) != 0) ? 1 : 0);
			  }else if(pin == GPIO152){			
				   return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (1L << 7)) != 0) ? 1 : 0);
			  }else if(pin == GPIO153){			
				   return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (1L << 3)) != 0) ? 1 : 0);
			  }else if(pin == GPIO154){			
				   return (((GPIO_RD32(&reg->msdc3_ctrl5.val) & (1L << 3)) != 0) ? 1 : 0);
			  }		  
			  
			}		

	if (i > ARRAY_SIZE(mt_ies_smt_map)) {
		return -ERINVAL;
	}
}
/*---------------------------------------------------------------------------*/
/*
int mt_set_gpio_ies_base(unsigned long pin, unsigned long enable)
{
    unsigned long flags;

    if(IES_offset[pin].offset == -1){
	  return GPIO_IES_UNSUPPORTED;
    }
    else if((pin >= 149) && (pin < 179)){
	spin_lock_irqsave(&mtk_gpio_lock, flags);
	  if (enable == GPIO_IES_DISABLE)
		GPIO_CLR_BITS((1L << (IES_offset[pin].offset)), IES_addr[pin].addr);
	  else
		GPIO_SW_SET_BITS((1L << (IES_offset[pin].offset)), IES_addr[pin].addr);
	spin_unlock_irqrestore(&mtk_gpio_lock, flags);
    }
    else{
    	  if (enable == GPIO_IES_DISABLE)
		GPIO_SET_BITS((1L << (IES_offset[pin].offset)), IES_addr[pin].addr + 8);
	  else
		GPIO_SET_BITS((1L << (IES_offset[pin].offset)), IES_addr[pin].addr + 4);
    }

    return RSUCCESS;
}
*/


/*---------------------------------------------------------------------------*/
int mt_set_gpio_ies_base(unsigned long pin, unsigned long enable)
{

	int i = 0;
	GPIO_REGS *reg = gpio_reg;

	for (i = 0; i < 19; i++) {
			if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
				if (enable == GPIO_IES_DISABLE)
		            GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
			                   &reg->ies[mt_ies_smt_map[i].reg_index].rst);
	            else
		            GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
			                   &reg->ies[mt_ies_smt_map[i].reg_index].set);	
				}
		}
	
	        i=i+4;/*i=19~21,similar to TDP/RDP/.../,ies not uesd*/
			if (i == 22) {
					if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
						if (enable == GPIO_IES_DISABLE)
							GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
									   &reg->ies[mt_ies_smt_map[i].reg_index].rst);
						else
							GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
									   &reg->ies[mt_ies_smt_map[i].reg_index].set); 
						}
				}
			i++;
			if(i == 23)
				{
				/*only set MSDC1 ies */
				  if(pin == GPIO121){
					
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &reg->msdc1_ctrl1.rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &reg->msdc1_ctrl1.set);
					   }
				  }else if(pin == GPIO122){
					
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &reg->msdc1_ctrl0.rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &reg->msdc1_ctrl0.set);
					   }
				  }else if(pin == GPIO123){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &reg->msdc1_ctrl2.rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &reg->msdc1_ctrl2.set);
					   }
				  }else if(pin == GPIO124){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 7, &reg->msdc1_ctrl2.rst);
					   } else {
						   GPIO_SET_BITS(1L << 7, &reg->msdc1_ctrl2.set);
					   }
				  }else if(pin == GPIO125){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 11, &reg->msdc1_ctrl2.rst);
					   } else {
						   GPIO_SET_BITS(1L << 11, &reg->msdc1_ctrl2.set);
					   }
				  }else if(pin == GPIO126){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 15, &reg->msdc1_ctrl2.rst);
					   } else {
						   GPIO_SET_BITS(1L << 15, &reg->msdc1_ctrl2.set);
					   }
				  }
				 i++;
				  
				}
			if(i == 24)
				{
				/*only set MSDC0 ies*/
				  if(pin >= GPIO127 && pin <= GPIO131){
					
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl2.rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl2.set);
					   }
				  } else if(pin == GPIO132){			
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl1.rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl1.set);
					   }
				  }else if(pin == GPIO133){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl0.rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl0.set);
					   }
				  }else if(pin >= GPIO134 && pin <= GPIO137){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl2.rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl2.set);
					   }
				  }
				 i++;
				  
				}
			for (;i < 27; i++ ) {
					if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
						if (enable == GPIO_IES_DISABLE)
							GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
									   &reg->ies[mt_ies_smt_map[i].reg_index].rst);
						else
							GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
									   &reg->ies[mt_ies_smt_map[i].reg_index].set); 
						}
				}
			if(i == 27)
				{
				/*only set smt,ies not set*/
				  if(pin >= GPIO143 && pin <= GPIO147){
					
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &reg->msdc3_ctrl2.rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &reg->msdc3_ctrl2.set);
					   }
				  }else if(pin == GPIO148){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &reg->msdc3_ctrl1.rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &reg->msdc3_ctrl1.set);
					   }
				  }else if(pin == GPIO149){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &reg->msdc3_ctrl0.rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &reg->msdc3_ctrl0.set);
					   }
				  }else if(pin >= GPIO150 && pin <= GPIO153){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &reg->msdc3_ctrl2.rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &reg->msdc3_ctrl2.set);
					   }
				  }else if(pin == GPIO154){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 7, &reg->msdc3_ctrl2.rst);
					   } else {
						   GPIO_SET_BITS(1L << 7, &reg->msdc3_ctrl2.set);
					   }
				  } 	  
				  
				}	


	




	

	if (i > ARRAY_SIZE(mt_ies_smt_map)) {
		return -ERINVAL;
	}




	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_ies_base(unsigned long pin)
{
	int i = 0;
	unsigned long data;
	GPIO_REGS *reg = gpio_reg;

	for (i = 0; i < 19; i++) {
				if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
					    data = GPIO_RD32(&reg->ies[mt_ies_smt_map[i].reg_index].val);
	                    return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
					}
			}
		
	i=i+4;/*i=19~21,similar to TDP/RDP/.../,ies not uesd*/
	if (i == 22) {
				if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
						data = GPIO_RD32(&reg->ies[mt_ies_smt_map[i].reg_index].val);
	                    return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
					}
			}
	i++;			
	if(i == 23){/*only set MSDC1 ies */
			          if(pin == GPIO121){
						return (((GPIO_RD32(&reg->msdc1_ctrl1.val) & (1L << 4)) != 0) ? 1 : 0);						
					  }else if(pin == GPIO122){
						return (((GPIO_RD32(&reg->msdc1_ctrl0.val) & (1L << 4)) != 0) ? 1 : 0);						   
					  }else if(pin == GPIO123){ 
					    return (((GPIO_RD32(&reg->msdc1_ctrl2.val) & (1L << 4)) != 0) ? 1 : 0);						  
					  }else if(pin == GPIO124){ 
					    return (((GPIO_RD32(&reg->msdc1_ctrl2.val) & (1L << 7)) != 0) ? 1 : 0);						   
					  }else if(pin == GPIO125){
					    return (((GPIO_RD32(&reg->msdc1_ctrl2.val) & (1L << 11)) != 0) ? 1 : 0);						   
					  }else if(pin == GPIO126){ 
					    return (((GPIO_RD32(&reg->msdc1_ctrl2.val) & (1L << 15)) != 0) ? 1 : 0);						   
					  }
					 i++;
					  
					}
				if(i == 24)
					{
					/*only set MSDC0 ies*/
					  if(pin >= GPIO127 && pin <= GPIO131){
					  	   return (((GPIO_RD32(&reg->msdc0_ctrl2.val) & (1L << 4)) != 0) ? 1 : 0);						  
					  } else if(pin == GPIO132){
					       return (((GPIO_RD32(&reg->msdc0_ctrl1.val) & (1L << 4)) != 0) ? 1 : 0);						  
					  }else if(pin == GPIO133){ 							  
						   return (((GPIO_RD32(&reg->msdc0_ctrl0.val) & (1L << 4)) != 0) ? 1 : 0);
					  }else if(pin >= GPIO134 && pin <= GPIO137){						   
						   return (((GPIO_RD32(&reg->msdc0_ctrl2.val) & (1L << 4)) != 0) ? 1 : 0);
					  }
					 i++;
					  
					}
				for (;i < 27; i++ ) {
						if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
							data = GPIO_RD32(&reg->ies[mt_ies_smt_map[i].reg_index].val);
	                        return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
							}
					}
				if(i == 27)
					{
					/*only set smt,ies not set*/
					  if(pin >= GPIO143 && pin <= GPIO147){						  
						   return (((GPIO_RD32(&reg->msdc3_ctrl2.val) & (1L << 4)) != 0) ? 1 : 0);
					  }else if(pin == GPIO148){ 						  
						   return (((GPIO_RD32(&reg->msdc3_ctrl1.val) & (1L << 4)) != 0) ? 1 : 0);
					  }else if(pin == GPIO149){ 						   
						   return (((GPIO_RD32(&reg->msdc3_ctrl0.val) & (1L << 4)) != 0) ? 1 : 0);
					  }else if(pin >= GPIO150 && pin <= GPIO153){								  
						   return (((GPIO_RD32(&reg->msdc3_ctrl2.val) & (1L << 4)) != 0) ? 1 : 0);
					  }else if(pin == GPIO154){ 		
						   return (((GPIO_RD32(&reg->msdc3_ctrl2.val) & (1L << 7)) != 0) ? 1 : 0);
						 
					  } 	  
					  
					}	
	if (i >= ARRAY_SIZE(mt_ies_smt_map)) {
		return -ERINVAL;
	}
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_select_base(unsigned long pin, unsigned long select)
{
    
	unsigned long pos;
	unsigned long bit;
	unsigned long i;
	GPIO_REGS *reg = gpio_reg;
	
	/***********************for special kpad pupd, NOTE DEFINITION REVERSE!!!**************************/
		for (i = 0; i < sizeof(kpad_pupd_spec) / sizeof(kpad_pupd_spec[0]); i++) {
			if (pin == kpad_pupd_spec[i].pin) {
				if (select == GPIO_PULL_DOWN)
					GPIO_SET_BITS((1L << kpad_pupd_spec[i].bit),
							  &reg->kpad_ctrl[kpad_pupd_spec[i].reg].set);
				else
					GPIO_SET_BITS((1L << kpad_pupd_spec[i].bit),
							  &reg->kpad_ctrl[kpad_pupd_spec[i].reg].rst);
				return RSUCCESS;
			}
		}


   	/********************************* MSDC special *********************************/
	if (pin == GPIO127) {	/* ms0 dat7 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 12, &reg->msdc0_ctrl4.rst);
		} else {
			GPIO_SET_BITS(1L << 12, &reg->msdc0_ctrl4.set);
		}
		return RSUCCESS;
	}else if (pin == GPIO128) {	/* ms0 dat6 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &reg->msdc0_ctrl4.rst);
		} else {
			GPIO_SET_BITS(1L << 8, &reg->msdc0_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO129) {	/* ms0 dat5*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl4.rst);
		} else {
			GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO130) {	/* ms0 dat4*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 0, &reg->msdc0_ctrl4.rst);
		} else {
			GPIO_SET_BITS(1L << 0, &reg->msdc0_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO131) {	/* ms0 RST */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 0, &reg->msdc0_ctrl5.rst);
		} else {
			GPIO_SET_BITS(1L << 0, &reg->msdc0_ctrl5.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO132) {	/* ms0 cmd */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &reg->msdc0_ctrl1.rst);
		} else {
			GPIO_SET_BITS(1L << 8, &reg->msdc0_ctrl1.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO133) {	/* ms0 clk */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &reg->msdc0_ctrl0.rst);
		} else {
			GPIO_SET_BITS(1L << 8, &reg->msdc0_ctrl0.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO134) {	/* ms0 dat3 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 12, &reg->msdc0_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L << 12, &reg->msdc0_ctrl3.set);
		}
		return RSUCCESS;
	}else if (pin == GPIO135) {	/* ms0 dat2 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &reg->msdc0_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L << 8, &reg->msdc0_ctrl3.set);
		}
		return RSUCCESS;
	}else if (pin == GPIO136) {	/* ms0 dat1*/
		if (select== GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L << 4, &reg->msdc0_ctrl3.set);
		}
		return RSUCCESS;
	}else if (pin == GPIO137) {	/* ms0 dat0 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 0, &reg->msdc0_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L << 0, &reg->msdc0_ctrl3.set);
		}
		return RSUCCESS;
	
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO121) {	/* ms1 cmd */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &reg->msdc1_ctrl1.rst);
		} else {
			GPIO_SET_BITS(1L << 8, &reg->msdc1_ctrl1.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO122) {	/* ms1 clk */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &reg->msdc1_ctrl0.rst);
		} else {
			GPIO_SET_BITS(1L << 8, &reg->msdc1_ctrl0.set);
		}
		return RSUCCESS;
	}else if (pin == GPIO123) {	/* ms1 dat0 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 0, &reg->msdc1_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L << 0, &reg->msdc1_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO124) {	/* ms1 dat1 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 4), &reg->msdc1_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 4), &reg->msdc1_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO125) {	/* ms1 dat2 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 8), &reg->msdc1_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 8), &reg->msdc1_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO126) {	/* ms1 dat3 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 12), &reg->msdc1_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 12), &reg->msdc1_ctrl3.set);
		}
		return RSUCCESS;
	 
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO85) {	/* ms2 cmd */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &reg->msdc2_ctrl1.rst);
		} else {
			GPIO_SET_BITS(1L << 8, &reg->msdc2_ctrl1.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO86) {	/* ms2 clk */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &reg->msdc2_ctrl0.rst);
		} else {
			GPIO_SET_BITS(1L << 8, &reg->msdc2_ctrl0.set);
		}
		return RSUCCESS;	
	}else if (pin == GPIO87) {	/* ms2 dat0 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 0, &reg->msdc2_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L << 0, &reg->msdc2_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO88) {	/* ms2 dat1 */
		if (select == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((1L << 4), &reg->msdc2_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 4), &reg->msdc2_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO89) {	/* ms2 dat2 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 8), &reg->msdc2_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 8), &reg->msdc2_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO90) {	/* ms2 dat3 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 12), &reg->msdc2_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 12), &reg->msdc2_ctrl3.set);
		}
		return RSUCCESS;
	
		/* //////////////////////////////////////////////// */
	}else if (pin == GPIO143) {	/* ms3 dat7*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 12, &reg->msdc3_ctrl4.rst);
		} else {
			GPIO_SET_BITS(1L << 12, &reg->msdc3_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO144) {/* ms3 dat6 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 8), &reg->msdc3_ctrl4.rst);
		} else {
			GPIO_SET_BITS((1L << 8), &reg->msdc3_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO145) {/* ms3 dat5*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 4), &reg->msdc3_ctrl4.rst);
		} else {
			GPIO_SET_BITS((1L << 4), &reg->msdc3_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO146) {/* ms3 dat4*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 0), &reg->msdc3_ctrl4.rst);
		} else {
			GPIO_SET_BITS((1L << 0), &reg->msdc3_ctrl4.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO147) {/* ms3 RST */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(3L<<1, &reg->msdc3_ctrl5.rst);
		} else {
			GPIO_SET_BITS(1L<<2, &reg->msdc3_ctrl5.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO148) {/* ms3 cmd */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &reg->msdc3_ctrl1.rst);
		} else {
			GPIO_SET_BITS(1L << 8, &reg->msdc3_ctrl1.set);
		}
		return RSUCCESS;
	}else if (pin == GPIO149) {	/* ms3 clk */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &reg->msdc3_ctrl0.rst);
		} else {
			GPIO_SET_BITS(1L << 8, &reg->msdc3_ctrl0.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO150) {/* ms3 dat3*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 12, &reg->msdc3_ctrl3.rst);
		} else {
			GPIO_SET_BITS(1L << 12, &reg->msdc3_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO151) {/* ms3 dat2 */
		if (select== GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 8), &reg->msdc3_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 8), &reg->msdc3_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO152) {/* ms3 dat1*/
		if (select ==GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 4), &reg->msdc3_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 4), &reg->msdc3_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO153) {/* ms3 dat0*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 0), &reg->msdc3_ctrl3.rst);
		} else {
			GPIO_SET_BITS((1L << 0), &reg->msdc3_ctrl3.set);
		}
		return RSUCCESS;
	} else if (pin == GPIO154) {/* ms3 DS */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 4, &reg->msdc3_ctrl5.rst);
		} else {
			GPIO_SET_BITS(1L << 4, &reg->msdc3_ctrl5.set);	/* 1L:10K */
		}
		return RSUCCESS;
	}

	if (0) {
		return GPIO_PULL_EN_UNSUPPORTED;
	} else {
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;

		if (select == GPIO_PULL_DOWN)
			GPIO_SET_BITS((1L << bit), &reg->pullsel[pos].rst);
		else
			GPIO_SET_BITS((1L << bit), &reg->pullsel[pos].set);
	}
	return RSUCCESS;
		
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_select_base(unsigned long pin)
{
    unsigned long pos;
	unsigned long bit;
	unsigned long data;
	unsigned long i;
	GPIO_REGS *reg = gpio_reg;


	/*********************************for special kpad pupd*********************************/
	for (i = 0; i < sizeof(kpad_pupd_spec) / sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
			data = GPIO_RD32(&reg->kpad_ctrl[kpad_pupd_spec[i].reg].val);
			return (((data & (1L << kpad_pupd_spec[i].bit)) != 0) ? 0 : 1);
		}
	}

	/********************************* MSDC special *********************************/
	if (pin == GPIO127) {	/* ms0 dat7 */		
		return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (1L << 12)) != 0) ? 1 : 0);
	}else if (pin == GPIO128) {	/* ms0 dat6 */
		return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (1L << 8)) != 0) ? 1 : 0);
	} else if (pin == GPIO129) {	/* ms0 dat5*/
		return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (1L << 4)) != 0) ? 1 : 0);
	} else if (pin == GPIO130) {	/* ms0 dat4*/
		return (((GPIO_RD32(&reg->msdc0_ctrl4.val) & (1L << 0)) != 0) ? 1 : 0);
	} else if (pin == GPIO131) {	/* ms0 RST */
		return (((GPIO_RD32(&reg->msdc0_ctrl5.val) & (1L << 0)) != 0) ? 1 : 0);
	} else if (pin == GPIO132) {	/* ms0 cmd */		
		return (((GPIO_RD32(&reg->msdc0_ctrl1.val) & (1L << 8)) != 0) ? 1 : 0);
	} else if (pin == GPIO133) {	/* ms0 clk */		
		return (((GPIO_RD32(&reg->msdc0_ctrl0.val) & (1L << 8)) != 0) ? 1 : 0);
	} else if (pin == GPIO134) {	/* ms0 dat3 */		
		return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (1L << 12)) != 0) ? 1 : 0);
	}else if (pin == GPIO135) {	/* ms0 dat2 */
	    return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (1L << 8)) != 0) ? 1 : 0);
	}else if (pin == GPIO136) {	/* ms0 dat1*/		
		return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (1L << 4)) != 0) ? 1 : 0);
	}else if (pin == GPIO137) {	/* ms0 dat0 */		
		return (((GPIO_RD32(&reg->msdc0_ctrl3.val) & (1L << 0)) != 0) ? 1 : 0);
	
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO121) {	/* ms1 cmd */		
		return (((GPIO_RD32(&reg->msdc1_ctrl1.val) & (1L << 8)) != 0) ? 1 : 0);
	} else if (pin == GPIO122) {	/* ms1 clk */	
		return (((GPIO_RD32(&reg->msdc1_ctrl0.val) & (1L << 8)) != 0) ? 1 : 0);
	}else if (pin == GPIO123) {	/* ms1 dat0 */		
		return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (1L << 0)) != 0) ? 1 : 0);
	} else if (pin == GPIO124) {	/* ms1 dat1 */		
		return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (1L << 4)) != 0) ? 1 : 0);
	} else if (pin == GPIO125) {	/* ms1 dat2 */		
		return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (1L << 8)) != 0) ? 1 : 0);
	} else if (pin == GPIO126) {	/* ms1 dat3 */		
		return (((GPIO_RD32(&reg->msdc1_ctrl3.val) & (1L << 12)) != 0) ? 1 : 0);
	 
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO85) {	/* ms2 cmd */		
		return (((GPIO_RD32(&reg->msdc2_ctrl1.val) & (1L << 8)) != 0) ? 1 : 0);
	} else if (pin == GPIO86) {	/* ms2 clk */
		return (((GPIO_RD32(&reg->msdc2_ctrl0.val) & (1L << 8)) != 0) ? 1 : 0);
	}else if (pin == GPIO87) {	/* ms2 dat0 */
		return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (1L << 0)) != 0) ? 1 : 0);
	} else if (pin == GPIO88) {	/* ms2 dat1 */
		return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (1L << 4)) != 0) ? 1 : 0);
	} else if (pin == GPIO89) {	/* ms2 dat2 */
		return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (1L << 8)) != 0) ? 1 : 0);
	} else if (pin == GPIO90) {	/* ms2 dat3 */
		return (((GPIO_RD32(&reg->msdc2_ctrl3.val) & (1L << 12)) != 0) ? 1 : 0);
	
		/* //////////////////////////////////////////////// */
	}else if (pin == GPIO143) {	/* ms3 dat7*/
		return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (1L << 12)) != 0) ? 1 : 0);
	} else if (pin == GPIO144) {/* ms3 dat6 */
		return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (1L << 8)) != 0) ? 1 : 0);
	} else if (pin == GPIO145) {/* ms3 dat5*/		
		return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (1L << 4)) != 0) ? 1 : 0);
	} else if (pin == GPIO146) {/* ms3 dat4*/		
		return (((GPIO_RD32(&reg->msdc3_ctrl4.val) & (1L << 0)) != 0) ? 1 : 0);
	} else if (pin == GPIO147) {/* ms3 RST */
		return (((GPIO_RD32(&reg->msdc3_ctrl5.val) & (1L << 0)) != 0) ? 1 : 0);
	} else if (pin == GPIO148) {/* ms3 cmd */		
		return (((GPIO_RD32(&reg->msdc3_ctrl1.val) & (1L << 8)) != 0) ? 1 : 0);
	}else if (pin == GPIO149) {	/* ms3 clk */		
		return (((GPIO_RD32(&reg->msdc3_ctrl0.val) & (1L << 8)) != 0) ? 1 : 0);
	} else if (pin == GPIO150) {/* ms3 dat3*/		
		return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (1L << 12)) != 0) ? 1 : 0);
	} else if (pin == GPIO151) {/* ms3 dat2 */		
		return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (1L << 8)) != 0) ? 1 : 0);
	} else if (pin == GPIO152) {/* ms3 dat1*/		
		return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (1L << 4)) != 0) ? 1 : 0);
	} else if (pin == GPIO153) {/* ms3 dat0*/		
		return (((GPIO_RD32(&reg->msdc3_ctrl3.val) & (1L << 0)) != 0) ? 1 : 0);
	} else if (pin == GPIO154) {/* ms3 DS */
		return (((GPIO_RD32(&reg->msdc3_ctrl5.val) & (1L << 4)) != 0) ? 1 : 0);
	}

	if (0) {
		return GPIO_PULL_EN_UNSUPPORTED;
	} else {
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;
		data = GPIO_RD32(&reg->pullsel[pos].val);
	}
	return (((data & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_inversion_base(unsigned long pin, unsigned long enable)
{/*FIX-ME
   */
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_inversion_base(unsigned long pin)
{/*FIX-ME*/
   	return 0;//FIX-ME
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_out_base(unsigned long pin, unsigned long output)
{
    unsigned long pos;
    unsigned long bit;
    GPIO_REGS *reg = gpio_reg;

    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;
    
    if (output == GPIO_OUT_ZERO)
        GPIO_SET_BITS((1L << bit), &reg->dout[pos].rst);
    else
        GPIO_SET_BITS((1L << bit), &reg->dout[pos].set);
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_out_base(unsigned long pin)
{
    unsigned long pos;
    unsigned long bit;
    unsigned long data;
    GPIO_REGS *reg = gpio_reg;

    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    data = GPIO_RD32(&reg->dout[pos].val);
    return (((data & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_in_base(unsigned long pin)
{
    unsigned long pos;
    unsigned long bit;
    unsigned long data;
    GPIO_REGS *reg = gpio_reg;

    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    data = GPIO_RD32(&reg->din[pos].val);
    return (((data & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_mode_base(unsigned long pin, unsigned long mode)
{
    unsigned long pos;
    unsigned long bit;
    unsigned long data;
    unsigned long mask = (1L << GPIO_MODE_BITS) - 1;    
    GPIO_REGS *reg = gpio_reg;

	
    pos = pin / MAX_GPIO_MODE_PER_REG;
    bit = pin % MAX_GPIO_MODE_PER_REG;

   
    data = GPIO_RD32(&reg->mode[pos].val);
    
    data &= ~(mask << (GPIO_MODE_BITS*bit));
    data |= (mode << (GPIO_MODE_BITS*bit));
    
    GPIO_WR32(&reg->mode[pos].val, data);

#if 0 

    data = ((1L << (GPIO_MODE_BITS*bit)) << 3) | (mode << (GPIO_MODE_BITS*bit));

    GPIO_WR32(&reg->mode[pos]._align1, data);
#endif
	
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_mode_base(unsigned long pin)
{
    unsigned long pos;
    unsigned long bit;
    unsigned long data;
    unsigned long mask = (1L << GPIO_MODE_BITS) - 1;    
    GPIO_REGS *reg = gpio_reg;

	
    pos = pin / MAX_GPIO_MODE_PER_REG;
    bit = pin % MAX_GPIO_MODE_PER_REG;
    
    data = GPIO_RD32(&reg->mode[pos].val);
    
    return ((data >> (GPIO_MODE_BITS*bit)) & mask);
	
}
/*---------------------------------------------------------------------------*/
void fill_addr_reg_array(PIN_addr *addr, PIN_addr_t *addr_t)
{
    unsigned long i;

    for (i = 0; i < MT_GPIO_BASE_MAX; i++) {
	if (strcmp(addr_t->s1,"GPIO_BASE")==0)
		addr->addr = GPIO_BASE_1 + addr_t->offset;
	else if (strcmp(addr_t->s1,"IOCFG_L_BASE")==0)
		addr->addr = IOCFG_L_BASE_1 + addr_t->offset;
	else if (strcmp(addr_t->s1,"IOCFG_B_BASE")==0)
		addr->addr = IOCFG_B_BASE_1 + addr_t->offset;
	else if (strcmp(addr_t->s1,"IOCFG_R_BASE")==0)
		addr->addr = IOCFG_R_BASE_1 + addr_t->offset;
	else if (strcmp(addr_t->s1,"IOCFG_T_BASE")==0)
		addr->addr = IOCFG_T_BASE_1 + addr_t->offset;
	else if (strcmp(addr_t->s1,"MIPI_TX0_BASE")==0)
		addr->addr = MIPI_TX0_BASE_1 + addr_t->offset;
	else if (strcmp(addr_t->s1,"MIPI_RX_ANA_CSI0_BASE")==0)
		addr->addr = MIPI_RX_ANA_CSI0_BASE_1 + addr_t->offset;
	else if (strcmp(addr_t->s1,"MIPI_RX_ANA_CSI1_BASE")==0)
		addr->addr = MIPI_RX_ANA_CSI1_BASE_1 + addr_t->offset;
	else
		addr->addr = 0;

	//GPIOLOG("GPIO_COUNT=%d ,addr->addr=0x%lx, i=%d\n", GPIO_COUNT, addr->addr, i);

	addr += 1;
	addr_t += 1;
    }
    //GPIO_COUNT++;
}
/*---------------------------------------------------------------------------*/
void get_gpio_vbase(struct device_node *node)
{
    /* Setup IO addresses */
    gpio_vbase.gpio_regs = of_iomap(node, 0);
    GPIOLOG("gpio_vbase.gpio_regs=0x%p\n", gpio_vbase.gpio_regs);

    gpio_reg = (GPIO_REGS*)(GPIO_BASE_1);
    
    //printk("[DTS] gpio_regs=0x%lx\n", gpio_vbase.gpio_regs);
    spin_lock_init(&mtk_gpio_lock);
}
/*---------------------------------------------------------------------------*/
void get_io_cfg_vbase(void)
{
    struct device_node *node = NULL;
    //unsigned long i;
    
    node = of_find_compatible_node(NULL, NULL, "mediatek,IOCFG_L");
    if(node){
	/* Setup IO addresses */
	gpio_vbase.IOCFG_L_regs = of_iomap(node, 0);
	//GPIOLOG("gpio_vbase.IOCFG_L_regs=0x%lx\n", gpio_vbase.IOCFG_L_regs);
    }
    
    node = of_find_compatible_node(NULL, NULL, "mediatek,IOCFG_B");
    if(node){
	/* Setup IO addresses */
	gpio_vbase.IOCFG_B_regs = of_iomap(node, 0);
	//GPIOLOG("gpio_vbase.IOCFG_B_regs=0x%lx\n", gpio_vbase.IOCFG_B_regs);
    }  
 
    node = of_find_compatible_node(NULL, NULL, "mediatek,IOCFG_R");
    if(node){
	/* Setup IO addresses */
	gpio_vbase.IOCFG_R_regs = of_iomap(node, 0);
	//GPIOLOG("gpio_vbase.IOCFG_R_regs=0x%lx\n", gpio_vbase.IOCFG_R_regs);
    }    

    node = of_find_compatible_node(NULL, NULL, "mediatek,IOCFG_T");
    if(node){
	/* Setup IO addresses */
	gpio_vbase.IOCFG_T_regs = of_iomap(node, 0);
	//GPIOLOG("gpio_vbase.IOCFG_T_regs=0x%lx\n", gpio_vbase.IOCFG_T_regs);
    }

    node = of_find_compatible_node(NULL, NULL, "mediatek,MIPI_TX0");
    if(node){
	/* Setup IO addresses */
	gpio_vbase.MIPI_TX0_regs = of_iomap(node, 0);
	//GPIOLOG("gpio_vbase.MIPI_TX0_regs=0x%lx\n", gpio_vbase.MIPI_TX0_regs);
    }

    node = of_find_compatible_node(NULL, NULL, "mediatek,MIPI_RX_ANA_CSI0");
    if(node){
	/* Setup IO addresses */
	gpio_vbase.MIPI_RX_CSI0_regs = of_iomap(node, 0);
	//GPIOLOG("gpio_vbase.MIPI_RX_CSI0_regs=0x%lx\n", gpio_vbase.MIPI_RX_CSI0_regs);
    }

    node = of_find_compatible_node(NULL, NULL, "mediatek,MIPI_RX_ANA_CSI1");
    if(node){
	/* Setup IO addresses */
	gpio_vbase.MIPI_RX_CSI1_regs = of_iomap(node, 0);
	//GPIOLOG("gpio_vbase.MIPI_RX_CSI1_regs=0x%lx\n", gpio_vbase.MIPI_RX_CSI1_regs);
    }


    fill_addr_reg_array(IES_addr, IES_addr_t);
    fill_addr_reg_array(SMT_addr, SMT_addr_t);
    fill_addr_reg_array(PULLEN_addr, PULLEN_addr_t);
    fill_addr_reg_array(PULL_addr, PULL_addr_t);
    fill_addr_reg_array(PU_addr, PU_addr_t);
    fill_addr_reg_array(PD_addr, PD_addr_t);
#if 0
    for (i = 0; i < MT_GPIO_BASE_MAX; i++) {
	if (strcmp(IES_addr_t[i].s1,"GPIO_BASE")==0)
		IES_addr[i].addr = GPIO_BASE_1 + IES_addr_t[i].offset;
	else if (strcmp(IES_addr_t[i].s1,"IOCFG_L_BASE")==0)
		IES_addr[i].addr = IOCFG_L_BASE_1 + IES_addr_t[i].offset;
	else if (strcmp(IES_addr_t[i].s1,"IOCFG_B_BASE")==0)
		IES_addr[i].addr = IOCFG_B_BASE_1 + IES_addr_t[i].offset;
	else if (strcmp(IES_addr_t[i].s1,"IOCFG_R_BASE")==0)
		IES_addr[i].addr = IOCFG_R_BASE_1 + IES_addr_t[i].offset;
	else if (strcmp(IES_addr_t[i].s1,"IOCFG_T_BASE")==0)
		IES_addr[i].addr = IOCFG_T_BASE_1 + IES_addr_t[i].offset;
	else if (strcmp(IES_addr_t[i].s1,"MIPI_TX0_BASE")==0)
		IES_addr[i].addr = MIPI_TX0_BASE_1 + IES_addr_t[i].offset;
	else if (strcmp(IES_addr_t[i].s1,"MIPI_RX_ANA_CSI0_BASE")==0)
		IES_addr[i].addr = MIPI_RX_ANA_CSI0_BASE_1 + IES_addr_t[i].offset;
	else if (strcmp(IES_addr_t[i].s1,"MIPI_RX_ANA_CSI1_BASE")==0)
		IES_addr[i].addr = MIPI_RX_ANA_CSI1_BASE_1 + IES_addr_t[i].offset;
	else
		IES_addr[i].addr = 0;
    }
#endif
}
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_MD32_SUPPORT
/*---------------------------------------------------------------------------*/
void md32_send_cust_pin_from_irq(void)
{
    ipi_status ret = BUSY;
    struct GPIO_IPI_Packet ipi_pkt;

    GPIOLOG("Enter ipi wq function to send cust pin... \n");
    ipi_pkt.touch_pin = (GPIO_CTP_EINT_PIN & ~(0x80000000));

    ret = md32_ipi_send(IPI_GPIO, &ipi_pkt, sizeof(ipi_pkt), true);
    if (ret != DONE)
        GPIOLOG("MD32 side IPI busy... %d\n", ret);

    //GPIOLOG("IPI get touch pin num = %d ...\n", GPIO_RD32(uart_base+0x40));
}
/*---------------------------------------------------------------------------*/
void gpio_ipi_handler(int id, void *data, uint len)
{
    GPIOLOG("Enter gpio_ipi_handler... \n");
    queue_work(wq_md32_cust_pin, &work_md32_cust_pin);
}
/*---------------------------------------------------------------------------*/
void md32_gpio_handle_init(void)
{
    //struct device_node *node = NULL;
    md32_ipi_registration(IPI_GPIO, gpio_ipi_handler, "GPIO IPI");
    wq_md32_cust_pin = create_workqueue("MD32_CUST_PIN_WQ");
    INIT_WORK(&work_md32_cust_pin,(void(*)(void))md32_send_cust_pin_from_irq);

    //node = of_find_compatible_node(NULL, NULL, "mediatek,AP_UART1");
    //if(node){
    //    /* iomap registers */
    //    uart_base = (unsigned long)of_iomap(node, 0);
    //}
}
/*---------------------------------------------------------------------------*/
#endif /*CONFIG_MD32_SUPPORT*/
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM 
/*---------------------------------------------------------------------------*/
void mt_gpio_suspend(void)
{
	/* compatible with HAL */
}
/*---------------------------------------------------------------------------*/
void mt_gpio_resume(void)
{
	/* compatible with HAL */
}
/*---------------------------------------------------------------------------*/
#endif /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/
