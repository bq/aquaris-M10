#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/dma-mapping.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/mman.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <asm/page.h>
#include <mach/dma.h>
#include <mach/board.h>
#include <mach/devs.h>
#include <mach/mt_typedefs.h>
#include <mach/irqs.h>
#include <mach/mt_gpio.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/mmc/sd_misc.h>
#include "mt_sd.h"
#include "dbg.h"
#include "msdc_debug.h"

#define MSDC_USE_PATCH_BIT2_TURNING_WITH_ASYNC
//#ifndef FPGA_PLATFORM
//#include <mach/mt_pm_wrap.h>
//#endif
#define FEATURE_MET_MMC_INDEX
#if !defined(CONFIG_MT_ENG_BUILD)
#undef FEATURE_MET_MMC_INDEX
#endif

#define MET_USER_EVENT_SUPPORT
#include <linux/met_drv.h>

#ifdef MTK_MSDC_USE_CACHE
#include <mach/mt_boot.h>
#endif

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
#include <linux/xlog.h>
#include <mach/mtk_thermal_monitor.h>
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT

#include <linux/proc_fs.h>
#include "drivers/mmc/card/queue.h"
#include <mach/emi_mpu.h>
#include <mach/memory.h>
#ifdef CONFIG_MTK_AEE_FEATURE
#include <linux/aee.h>
#endif

#ifdef CONFIG_MTK_HIBERNATION
#include "mach/mtk_hibernate_dpm.h"
#endif

#ifndef FPGA_PLATFORM
#include <mach/mt_pm_ldo.h>
#include <mach/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mach/mt_clkmgr.h>
#include <mach/eint.h>
//#include <cust_eint.h>
#endif

#include <mach/mt_storage_logger.h>

#include <mach/power_loss_test.h>

#include <mach/partition.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif


#ifndef EXT_CSD_BOOT_SIZE_MULT
#define EXT_CSD_BOOT_SIZE_MULT           226 /* R */
#endif
#ifndef EXT_CSD_RPMB_SIZE_MULT
#define EXT_CSD_RPMB_SIZE_MULT           168 /* R */
#endif
#ifndef EXT_CSD_GP1_SIZE_MULT
#define EXT_CSD_GP1_SIZE_MULT            143 /* R/W 3 bytes */
#endif
#ifndef EXT_CSD_GP2_SIZE_MULT
#define EXT_CSD_GP2_SIZE_MULT            146 /* R/W 3 bytes */
#endif
#ifndef EXT_CSD_GP3_SIZE_MULT
#define EXT_CSD_GP3_SIZE_MULT            149 /* R/W 3 bytes */
#endif
#ifndef EXT_CSD_GP4_SIZE_MULT
#define EXT_CSD_GP4_SIZE_MULT            152 /* R/W 3 bytes */
#endif
#ifndef EXT_CSD_PART_CFG
#define EXT_CSD_PART_CFG                 179 /* R/W/E & R/W/E_P */
#endif
#ifndef EXT_CSD_CACHE_FLUSH
#define EXT_CSD_CACHE_FLUSH              32
#endif
#ifndef EXT_CSD_CACHE_CTRL
#define EXT_CSD_CACHE_CTRL               33
#endif
#ifndef CAPACITY_2G
#define CAPACITY_2G                      (2 * 1024 * 1024 * 1024ULL)
#endif

#ifdef CONFIG_MTK_EMMC_SUPPORT
u32 g_emmc_mode_switch = 0;
//#define MTK_EMMC_ETT_TO_DRIVER  /* for eMMC off-line apply to driver */
#endif

#ifdef MTK_MSDC_USE_CACHE
#define MSDC_MAX_FLUSH_COUNT    (3)
#define CACHE_UN_FLUSHED        (0)
#define CACHE_FLUSHED           (1)
static unsigned int g_cache_status = CACHE_UN_FLUSHED;
static unsigned long long g_flush_data_size = 0;
static unsigned int g_flush_error_count = 0;
static int g_flush_error_happend = 0;
#endif

#define UNSTUFF_BITS(resp,start,size)          \
({                \
    const int __size = size;        \
    const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;  \
    const int __off = 3 - ((start) / 32);      \
    const int __shft = (start) & 31;      \
    u32 __res;            \
                  \
    __res = resp[__off] >> __shft;        \
    if (__size + __shft > 32)        \
        __res |= resp[__off-1] << ((32 - __shft) % 32);  \
    __res & __mask;            \
})

#ifdef MTK_EMMC_ETT_TO_DRIVER
#include "emmc_device_list.h"
static  u8   m_id = 0;           // Manufacturer ID
static char pro_name[8] = {0};  // Product name
#endif

#if (MSDC_DATA1_INT == 1)
static u16 u_sdio_irq_counter=0;
static u16 u_msdc_irq_counter=0;
static int int_sdio_irq_enable=0;
#endif

struct msdc_host *ghost=NULL;
int src_clk_control=0;
struct mmc_blk_data {
    spinlock_t lock;
    struct gendisk *disk;
    struct mmc_queue queue;

    unsigned int usage;
    unsigned int read_only;
};

#if defined(FEATURE_MET_MMC_INDEX)
static unsigned int met_mmc_bdnum;
#endif

#define DRV_NAME                         "mtk-msdc"

#define MSDC_COOKIE_PIO        (1<<0)
#define MSDC_COOKIE_ASYNC    (1<<1)

#define msdc_use_async_way(x)    (x & MSDC_COOKIE_ASYNC)
#define msdc_async_use_dma(x)   ((x & MSDC_COOKIE_ASYNC) && (!(x & MSDC_COOKIE_PIO)))
#define msdc_async_use_pio(x)   ((x & MSDC_COOKIE_ASYNC) && ((x & MSDC_COOKIE_PIO))) //not used
#ifdef FPGA_PLATFORM
#define HOST_MAX_MCLK          (200000000)//           (12000000)
#else
#define HOST_MAX_MCLK       (200000000) //(104000000)
#endif
#define HOST_MIN_MCLK                    (260000)

#define HOST_MAX_BLKSZ                   (2048)

#define MSDC_OCR_AVAIL                   (MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33)
//#define MSDC_OCR_AVAIL                 (MMC_VDD_32_33 | MMC_VDD_33_34)

#define MSDC1_IRQ_SEL                    (1 << 9)
#define PDN_REG                          (0xF1000010)

#define DEFAULT_DEBOUNCE                 (8)       /* 8 cycles */
#define DEFAULT_DTOC                     (3)      /* data timeout counter. 65536x40(75/77) /1048576 * 3(83/85) sclk. */

#define CMD_TIMEOUT                      (HZ/10 * 5)   /* 100ms x5 */
#define DAT_TIMEOUT                      (HZ    * 5)   /* 1000ms x5 */
#define CLK_TIMEOUT                      (HZ    * 5)  /* 5s    */
#define POLLING_BUSY                     (HZ     * 3)
#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)
    #ifdef MTK_SDIO30_DETECT_THERMAL
#define OT_PERIOD           (HZ    * 60)   /* 1000ms x60 */
#define OT_TEMPDIFF_BOUNDRY 10000          /* 10 degree */ /* trigger online tuning if temperature difference exceed this value */
    #endif
#define OT_TIMEOUT          (HZ/50)           /* 20ms */
#define OT_BLK_SIZE         0x100
#define OT_START_TUNEWND    3
#endif // #if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)
#define MAX_DMA_CNT                      (64 * 1024 - 512)   /* a single transaction for WIFI may be 50K*/
#define MAX_DMA_CNT_SDIO                 (0xFFFFFFFF - 255)   /* a single transaction for LTE may be 128K*/ /* Basic DMA use 32 bits to store transfer size */

#define MAX_HW_SGMTS                     (MAX_BD_NUM)
#define MAX_PHY_SGMTS                    (MAX_BD_NUM)
#define MAX_SGMT_SZ                      (MAX_DMA_CNT)
#define MAX_SGMT_SZ_SDIO                 (MAX_DMA_CNT_SDIO)
#define MAX_REQ_SZ                       (512*1024)

#define CMD_TUNE_UHS_MAX_TIME            (2*32*8*8)
#define CMD_TUNE_HS_MAX_TIME             (2*32)

#define READ_TUNE_UHS_CLKMOD1_MAX_TIME   (2*32*32*8)
#define READ_TUNE_UHS_MAX_TIME           (2*32*32)
#define READ_TUNE_HS_MAX_TIME            (2*32)

#define WRITE_TUNE_HS_MAX_TIME           (2*32)
#define WRITE_TUNE_UHS_MAX_TIME          (2*32*8)


#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
static unsigned int msdc_online_tuning(struct msdc_host   *host, unsigned fn, unsigned addr);
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT

//=================================

#define MSDC_LOWER_FREQ
#define MSDC_MAX_FREQ_DIV                (2)  /* 200 / (4 * 2) */
#define MSDC_MAX_TIMEOUT_RETRY           (1)
#define MSDC_MAX_TIMEOUT_RETRY_EMMC      (2)
#define MSDC_MAX_W_TIMEOUT_TUNE          (5)
#define MSDC_MAX_W_TIMEOUT_TUNE_EMMC     (64)
#define MSDC_MAX_R_TIMEOUT_TUNE          (3)
#define MSDC_MAX_POWER_CYCLE             (3)

#define MSDC_MAX_CONTINUOUS_FAIL_REQUEST_COUNT (50)
#ifdef CONFIG_OF
static struct device_node *eint_node = NULL;
#ifdef MTK_MSDC_BRINGUP_DEBUG
struct device_node *msdc_apmixed_node = NULL;
struct device_node *msdc_topckgen_node = NULL;
struct device_node *msdc_infracfg_node = NULL;
void __iomem *msdc_apmixed_base;
void __iomem *msdc_topckgen_base;
void __iomem *msdc_infracfg_base;
#endif

static unsigned int cd_irq = 0;
#endif

#ifdef FPGA_PLATFORM
#ifdef CONFIG_OF
static void __iomem *fpga_pwr_gpio;
static void __iomem *fpga_pwr_gpio_eo;
#define PWR_GPIO                         (fpga_pwr_gpio)
#define PWR_GPIO_EO                      (fpga_pwr_gpio_eo)
#else
#define PWR_GPIO                         (0xF0001E84)
#define PWR_GPIO_EO                      (0xF0001E88)
#endif

#define PWR_GPIO_L4_DIR                  (1 << 11)
#define PWR_MASK_VOL_33_MASK             (~(1 << 10))
#define PWR_MASK_EN_MASK                 (~(1 << 8))
#define PWR_GPIO_L4_DIR_MASK             (~(1 << 11))

#define PWR_MASK_EN         (0x1 << 8)
#define PWR_MASK_VOL_18     (0x1 << 9)
#define PWR_MASK_VOL_33     (0x1 << 10)
#define PWR_MASK_L4         (0x1 << 11)
#define PWR_MSDC_DIR        (PWR_MASK_EN | PWR_MASK_VOL_18 | PWR_MASK_VOL_33 | PWR_MASK_L4)

#define MSDC0_PWR_MASK_EN         (0x1 << 12)
#define MSDC0_PWR_MASK_VOL_18     (0x1 << 13)
#define MSDC0_PWR_MASK_VOL_33     (0x1 << 14)
#define MSDC0_PWR_MASK_L4         (0x1 << 15)
#define MSDC0_PWR_MSDC_DIR        (MSDC0_PWR_MASK_EN | MSDC0_PWR_MASK_VOL_18 | MSDC0_PWR_MASK_VOL_33 | MSDC0_PWR_MASK_L4)

static void msdc_clr_fpga_pwr(u32 bits)
{
    switch (bits){
        case MSDC0_PWR_MASK_EN:
            /* check for set before */
            sdr_set_field(PWR_GPIO,    (MSDC0_PWR_MASK_EN),0);
			sdr_set_field(PWR_GPIO_EO, (MSDC0_PWR_MASK_EN),0);
            break;
        case MSDC0_PWR_MASK_VOL_18:
            /* check for set before */
            sdr_set_field(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
            sdr_set_field(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
            break;
        case MSDC0_PWR_MASK_VOL_33:
	        /* check for set before */
            sdr_set_field(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
            sdr_set_field(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
            break;
        case MSDC0_PWR_MASK_L4:
			/* check for set before */
            sdr_set_field(PWR_GPIO,    MSDC0_PWR_MASK_L4, 0);
            sdr_set_field(PWR_GPIO_EO, MSDC0_PWR_MASK_L4, 0);
            break;
        case PWR_MASK_EN:
            /* check for set before */
            sdr_set_field(PWR_GPIO,    PWR_MASK_EN,0);
			sdr_set_field(PWR_GPIO_EO, PWR_MASK_EN,0);
            break;
        case PWR_MASK_VOL_18:
            /* check for set before */
            sdr_set_field(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
            sdr_set_field(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
            break;
        case PWR_MASK_VOL_33:
		    /* check for set before */
            sdr_set_field(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
            sdr_set_field(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
            break;
        case PWR_MASK_L4:
		    /* check for set before */
            sdr_set_field(PWR_GPIO,    PWR_MASK_L4, 0);
            sdr_set_field(PWR_GPIO_EO, PWR_MASK_L4, 0);
            break;
        default:
            break;
    }

}

static void msdc_set_fpga_pwr(u32 bits)
{
    switch (bits){
        case MSDC0_PWR_MASK_EN:
            /* check for set before */
            sdr_set_field(PWR_GPIO,    MSDC0_PWR_MASK_EN,1);
		    sdr_set_field(PWR_GPIO_EO, MSDC0_PWR_MASK_EN,1);
            break;
        case MSDC0_PWR_MASK_VOL_18:
            /* check for set before */
            sdr_set_field(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 1);
            sdr_set_field(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 1);
            break;
        case MSDC0_PWR_MASK_VOL_33:
		    /* check for set before */
            sdr_set_field(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 2);
            sdr_set_field(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 2);
            break;
        case MSDC0_PWR_MASK_L4:
		    /* check for set before */
            sdr_set_field(PWR_GPIO,    MSDC0_PWR_MASK_L4, 1);
            sdr_set_field(PWR_GPIO_EO, MSDC0_PWR_MASK_L4, 1);
            break;
        case PWR_MASK_EN:
            /* check for set before */
            sdr_set_field(PWR_GPIO,    PWR_MASK_EN,1);
			sdr_set_field(PWR_GPIO_EO, PWR_MASK_EN,1);
            break;
        case PWR_MASK_VOL_18:
            /* check for set before */
            sdr_set_field(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 1);
            sdr_set_field(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 1);
            break;
        case PWR_MASK_VOL_33:
		    /* check for set before */
            sdr_set_field(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 2);
            sdr_set_field(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 2);
            break;
        case PWR_MASK_L4:
		    /* check for set before */
            sdr_set_field(PWR_GPIO,    PWR_MASK_L4, 1);
            sdr_set_field(PWR_GPIO_EO, PWR_MASK_L4, 1);
            break;
        default:
            break;
    }
}

//#define MSDC0_ACTAS_SDCARD
void msdc_set_fpga_card_pwr(int id, int on)
{
    if (on){
		switch(id){
			case 0:
#ifdef MSDC0_ACTAS_SDCARD				
				msdc_set_fpga_pwr(MSDC0_PWR_MASK_VOL_33);
				msdc_set_fpga_pwr(MSDC0_PWR_MASK_L4);
				msdc_set_fpga_pwr(MSDC0_PWR_MASK_EN);
				MSDC_TRC_PRINT(MSDC_CONFIG,"FPGA 3.3 Volt power on\n");
#else 
				msdc_set_fpga_pwr(MSDC0_PWR_MASK_VOL_18);
				msdc_set_fpga_pwr(MSDC0_PWR_MASK_L4);
				msdc_set_fpga_pwr(MSDC0_PWR_MASK_EN);
				MSDC_TRC_PRINT(MSDC_CONFIG,"FPGA 1.8 Volt power on\n");
#endif
				break;
			case 1:
			case 2:
				msdc_set_fpga_pwr(PWR_MASK_VOL_33);
				msdc_set_fpga_pwr(PWR_MASK_L4);
				msdc_set_fpga_pwr(PWR_MASK_EN);
				MSDC_TRC_PRINT(MSDC_CONFIG,"FPGA 3.3 Volt power on\n");
				break;
			case 3:
				msdc_set_fpga_pwr(PWR_MASK_VOL_18);
				msdc_set_fpga_pwr(PWR_MASK_L4);
				msdc_set_fpga_pwr(PWR_MASK_EN);
				MSDC_TRC_PRINT(MSDC_CONFIG,"FPGA 1.8 Volt power on\n");
				break;
			default:
				MSDC_ERR_PRINT(MSDC_ERROR,"[%s:%s] power on, invalid\n", __FILE__, __func__);
				break;
		}
    } else {
		MSDC_TRC_PRINT(MSDC_CONFIG,"FPGA power off\n");
		switch(id){
			case 0:
				msdc_clr_fpga_pwr(MSDC0_PWR_MASK_EN);
				msdc_clr_fpga_pwr(MSDC0_PWR_MASK_VOL_18);
				msdc_clr_fpga_pwr(MSDC0_PWR_MASK_L4);
				break;
			case 1:
			case 2:
				msdc_clr_fpga_pwr(PWR_MASK_EN);
				msdc_clr_fpga_pwr(PWR_MASK_VOL_33);
				msdc_clr_fpga_pwr(PWR_MASK_L4);
				break;
			case 3:
				msdc_clr_fpga_pwr(PWR_MASK_EN);
				msdc_clr_fpga_pwr(PWR_MASK_VOL_18);
				msdc_clr_fpga_pwr(PWR_MASK_L4);
				break;
			default:
				MSDC_ERR_PRINT(MSDC_ERROR,"[%s:%s] power off, invalid\n", __FILE__, __func__);
				break;
		}
    }
    mdelay(10);
}

bool hwPowerOn_fpga(int id){
 	msdc_set_fpga_card_pwr(id, 1);
 
    return true;
}
EXPORT_SYMBOL(hwPowerOn_fpga);

/* FIX ME:
 * run SD on MSDC0, pwr default is 1.8v
 * if run SD on FPGA msdc1/2, should implement this function to switch voltage
 */
bool hwPowerSwitch_fpga(int id, int lowlevel)
{
	switch(id){
		case 0:
            if(lowlevel){				
				msdc_set_fpga_pwr(MSDC0_PWR_MASK_VOL_18);
            }
		    else{
			    msdc_set_fpga_pwr(MSDC0_PWR_MASK_VOL_33);
		    }
			msdc_set_fpga_pwr(MSDC0_PWR_MASK_L4);
			msdc_set_fpga_pwr(MSDC0_PWR_MASK_EN);
		break;
		case 1:
		case 2:
            if(lowlevel){				
				msdc_set_fpga_pwr(PWR_MASK_VOL_18);
            }
		    else{
			    msdc_set_fpga_pwr(PWR_MASK_VOL_33);
		    }			
			msdc_set_fpga_pwr(PWR_MASK_L4);
			msdc_set_fpga_pwr(PWR_MASK_EN);
			break;
		default:
			MSDC_ERR_PRINT(MSDC_ERROR, "[%s:%s] power on, invalid parameter\n", __FILE__, __func__);
			break;
			}

    return true;
}
EXPORT_SYMBOL(hwPowerSwitch_fpga);

bool hwPowerDown_fpga(int id){
	msdc_set_fpga_card_pwr(id, 0);
     return true;
}
EXPORT_SYMBOL(hwPowerDown_fpga);
#endif

struct msdc_host *mtk_msdc_host[]    = {NULL, NULL, NULL, NULL};
int g_dma_debug[HOST_MAX_NUM]        = {0, 0, 0, 0};
u32 latest_int_status[HOST_MAX_NUM]  = {0, 0, 0, 0};

transfer_mode msdc_latest_transfer_mode[HOST_MAX_NUM] = // 0 for PIO; 1 for DMA; 2 for nothing
{
    TRAN_MOD_NUM,
    TRAN_MOD_NUM,
    TRAN_MOD_NUM,
    TRAN_MOD_NUM,
};

operation_type msdc_latest_operation_type[HOST_MAX_NUM] = // 0 for read; 1 for write; 2 for nothing
{
    OPER_TYPE_NUM,
    OPER_TYPE_NUM,
    OPER_TYPE_NUM,
    OPER_TYPE_NUM,
};

struct dma_addr msdc_latest_dma_address[MAX_BD_PER_GPD];

static int msdc_rsp[] = {
    0,  /* RESP_NONE */
    1,  /* RESP_R1 */
    2,  /* RESP_R2 */
    3,  /* RESP_R3 */
    4,  /* RESP_R4 */
    1,  /* RESP_R5 */
    1,  /* RESP_R6 */
    1,  /* RESP_R7 */
    7,  /* RESP_R1b */
};

enum cg_clk_id msdc_cg_clk_id[HOST_MAX_NUM] = {
    MT_CG_INFRA_MSDC_0,
    MT_CG_INFRA_MSDC_1,
    MT_CG_INFRA_MSDC_2,
    MT_CG_INFRA_MSDC_3
};

/* For Inhanced DMA */
#define msdc_init_gpd_ex(gpd,extlen,cmd,arg,blknum) \
    do { \
        ((gpd_t*)gpd)->extlen = extlen; \
        ((gpd_t*)gpd)->cmd    = cmd; \
        ((gpd_t*)gpd)->arg    = arg; \
        ((gpd_t*)gpd)->blknum = blknum; \
    }while(0)

#define msdc_init_bd(bd, blkpad, dwpad, dptr, dlen) \
    do { \
        MSDC_ASSERT(dlen < 0x10000); \
        ((bd_t*)bd)->blkpad = blkpad; \
        ((bd_t*)bd)->dwpad  = dwpad; \
        ((bd_t*)bd)->ptr    = dptr; \
        ((bd_t*)bd)->buflen = dlen; \
    }while(0)

#define msdc_txfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16)
#define msdc_rxfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) >> 0)
#define msdc_fifo_write32(v)   sdr_write32(MSDC_TXDATA, (v))
#define msdc_fifo_write8(v)    sdr_write8(MSDC_TXDATA, (v))
#define msdc_fifo_read32()   sdr_read32(MSDC_RXDATA)
#define msdc_fifo_read8()    sdr_read8(MSDC_RXDATA)

#define msdc_dma_on()        sdr_clr_bits(MSDC_CFG, MSDC_CFG_PIO)
#define msdc_dma_off()       sdr_set_bits(MSDC_CFG, MSDC_CFG_PIO)
#define msdc_dma_status()    ((sdr_read32(MSDC_CFG) & MSDC_CFG_PIO) >> 3)

#if 0
u32 msdc_clk_ctrl_reg[4] = {MSDC0_GPIO_CLK_BASE, MSDC1_GPIO_CLK_BASE, MSDC2_GPIO_CLK_BASE, MSDC3_GPIO_CLK_BASE};
u32 msdc_cmd_ctrl_reg[4] = {MSDC0_GPIO_CMD_BASE, MSDC1_GPIO_CMD_BASE, MSDC2_GPIO_CMD_BASE, MSDC3_GPIO_CMD_BASE};
u32 msdc_dat_ctrl_reg[4] = {MSDC0_GPIO_DAT_BASE, MSDC1_GPIO_DAT_BASE, MSDC2_GPIO_DAT_BASE, MSDC3_GPIO_DAT_BASE};
u32 msdc_pad_ctrl_reg[4] = {MSDC0_GPIO_PAD_BASE, MSDC1_GPIO_PAD_BASE, MSDC2_GPIO_PAD_BASE, MSDC3_GPIO_PAD_BASE};
u32 msdc_dat1_ctrl_reg[4] = {MSDC0_GPIO_DAT_BASE, MSDC1_GPIO_DAT1_BASE, MSDC2_GPIO_DAT1_BASE, MSDC3_GPIO_DAT1_BASE};

void msdc_dump_padctl(struct msdc_host *host, u32 pin)
{
    switch(pin){
        case GPIO_CLK_CTRL:
            INIT_MSG("  CLK_PAD_CTRL[0x%x]        = 0x%.8x", msdc_clk_ctrl_reg[host->id], sdr_read32(msdc_clk_ctrl_reg[host->id]));
            break;
        case GPIO_CMD_CTRL:
            INIT_MSG("  CMD_PAD_CTRL[0x%x]        = 0x%.8x", msdc_cmd_ctrl_reg[host->id], sdr_read32(msdc_cmd_ctrl_reg[host->id]));
            break;
        case GPIO_DAT_CTRL:
            if(host->id == 0){
                INIT_MSG("  DAT_PAD_CTRL[0x%x]        = 0x%.8x", msdc_dat_ctrl_reg[host->id], sdr_read32(msdc_dat_ctrl_reg[host->id]));
            }else {
                INIT_MSG("  DAT_PAD_CTRL[0x%x]        = 0x%.8x", msdc_dat_ctrl_reg[host->id], sdr_read32(msdc_dat_ctrl_reg[host->id]));
                INIT_MSG("  DAT1_PAD_CTRL[0x%x]       = 0x%.8x", msdc_dat1_ctrl_reg[host->id], sdr_read32(msdc_dat1_ctrl_reg[host->id]));
            }
            break;
        case GPIO_RST_CTRL:
            if(host->id == 0){
                INIT_MSG("  RST_PAD_CTRL[0x%x]        = 0x%.8x", MSDC0_GPIO_RST_BASE, sdr_read32(MSDC0_GPIO_RST_BASE));
            }
            break;
        case GPIO_DS_CTRL:
            if(host->id == 0){
                INIT_MSG("  DS_PAD_CTRL[0x%x]         = 0x%.8x", MSDC0_GPIO_DS_BASE, sdr_read32(MSDC0_GPIO_DS_BASE));
            }
            break;
        case GPIO_PAD_CTRL:
            INIT_MSG("  TDRD_PAD_CTRL[0x%x]           = 0x%.8x", msdc_pad_ctrl_reg[host->id], sdr_read32(msdc_pad_ctrl_reg[host->id]));
            break;
        case GPIO_MODE_CTRL:
            if(host->id == 0){
                INIT_MSG("  PAD_MODE_CTRL[0x%x]         = 0x%.8x", MSDC0_GPIO_MODE0_BASE, sdr_read32(MSDC0_GPIO_MODE0_BASE));
                INIT_MSG("  PAD_MODE_CTRL[0x%x]         = 0x%.8x", MSDC0_GPIO_MODE1_BASE, sdr_read32(MSDC0_GPIO_MODE1_BASE));
                INIT_MSG("  PAD_MODE_CTRL[0x%x]         = 0x%.8x", MSDC0_GPIO_MODE2_BASE, sdr_read32(MSDC0_GPIO_MODE2_BASE));
                INIT_MSG("  PAD_MODE_CTRL[0x%x]         = 0x%.8x", MSDC0_GPIO_MODE3_BASE, sdr_read32(MSDC0_GPIO_MODE3_BASE));
            }else if(host->id == 1){
                INIT_MSG("  PAD_MODE_CTRL[0x%x]         = 0x%.8x", MSDC1_GPIO_MODE0_BASE, sdr_read32(MSDC1_GPIO_MODE0_BASE));
                INIT_MSG("  PAD_MODE_CTRL[0x%x]         = 0x%.8x", MSDC1_GPIO_MODE1_BASE, sdr_read32(MSDC1_GPIO_MODE1_BASE));
            }else if(host->id == 2){
                INIT_MSG("  PAD_MODE_CTRL[0x%x]         = 0x%.8x", MSDC2_GPIO_MODE0_BASE, sdr_read32(MSDC2_GPIO_MODE0_BASE));
                INIT_MSG("  PAD_MODE_CTRL[0x%x]         = 0x%.8x", MSDC2_GPIO_MODE1_BASE, sdr_read32(MSDC2_GPIO_MODE1_BASE));
            }else if(host->id == 3){
                INIT_MSG("  PAD_MODE_CTRL[0x%x]         = 0x%.8x", MSDC3_GPIO_MODE0_BASE, sdr_read32(MSDC3_GPIO_MODE0_BASE));
                INIT_MSG("  PAD_MODE_CTRL[0x%x]         = 0x%.8x", MSDC3_GPIO_MODE1_BASE, sdr_read32(MSDC3_GPIO_MODE1_BASE));
            }
            break;
        default:
            break;
    }
}
#endif

void msdc_dump_padctl(struct msdc_host *host, u32 pin)
{
}

void msdc_dump_register(struct msdc_host *host)
{
    void __iomem *base     = host->base;
#ifndef FPGA_PLATFORM
#ifdef ___MSDC_DEBUG___
	void __iomem *gpiobase = host->gpiobase;
	u32 u4mode, u4rdsel,u4tdsel,u4smt, u4ies, u4sr, u4rx, u4tx;
	u32 u4dir,u4pullen,u4pulllevel,u4level,u4PuR1R0=0;
#endif /* ___MSDC_DEBUG___*/
#endif /* FPGA_PLATFORM */

	u32 id = host->id;

    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 00] MSDC_CFG              = 0x%.8x\n", sdr_read32(MSDC_CFG));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 04] MSDC_IOCON            = 0x%.8x\n", sdr_read32(MSDC_IOCON));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 08] MSDC_PS               = 0x%.8x\n", sdr_read32(MSDC_PS));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 0C] MSDC_INT              = 0x%.8x\n", sdr_read32(MSDC_INT));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 10] MSDC_INTEN            = 0x%.8x\n", sdr_read32(MSDC_INTEN));    
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 14] MSDC_FIFOCS           = 0x%.8x\n", sdr_read32(MSDC_FIFOCS));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 18] MSDC_TXDATA           = not read\n");                       
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 1C] MSDC_RXDATA           = not read\n");
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 30] SDC_CFG               = 0x%.8x\n", sdr_read32(SDC_CFG));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 34] SDC_CMD               = 0x%.8x\n", sdr_read32(SDC_CMD));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 38] SDC_ARG               = 0x%.8x\n", sdr_read32(SDC_ARG));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 3C] SDC_STS               = 0x%.8x\n", sdr_read32(SDC_STS));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 40] SDC_RESP0             = 0x%.8x\n", sdr_read32(SDC_RESP0));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 44] SDC_RESP1             = 0x%.8x\n", sdr_read32(SDC_RESP1));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 48] SDC_RESP2             = 0x%.8x\n", sdr_read32(SDC_RESP2));                                
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 4C] SDC_RESP3             = 0x%.8x\n", sdr_read32(SDC_RESP3));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 50] SDC_BLK_NUM           = 0x%.8x\n", sdr_read32(SDC_BLK_NUM));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 54] SDC_VOL_CHG           = 0x%.8x\n", sdr_read32(SDC_VOL_CHG));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 58] SDC_CSTS              = 0x%.8x\n", sdr_read32(SDC_CSTS));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 5C] SDC_CSTS_EN           = 0x%.8x\n", sdr_read32(SDC_CSTS_EN));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 60] SDC_DCRC_STS          = 0x%.8x\n", sdr_read32(SDC_DCRC_STS));

	if((host->id == 0)||(host->id == 3)){
       MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 70] EMMC_CFG0             = 0x%.8x\n", sdr_read32(EMMC_CFG0));                        
       MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 74] EMMC_CFG1             = 0x%.8x\n", sdr_read32(EMMC_CFG1));
       MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 78] EMMC_STS              = 0x%.8x\n", sdr_read32(EMMC_STS));
       MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 7C] EMMC_IOCON            = 0x%.8x\n", sdr_read32(EMMC_IOCON));  
	}
	
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 80] SDC_ACMD_RESP         = 0x%.8x\n", sdr_read32(SDC_ACMD_RESP));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 84] SDC_ACMD19_TRG        = 0x%.8x\n", sdr_read32(SDC_ACMD19_TRG));      
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 88] SDC_ACMD19_STS        = 0x%.8x\n", sdr_read32(SDC_ACMD19_STS));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 8C]*MSDC_DMA_HIGH4BIT     = 0x%.8x\n", sdr_read32(MSDC_DMA_HIGH4BIT)); /* new register @mt8590 */
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 90] MSDC_DMA_SA           = 0x%.8x\n", sdr_read32(MSDC_DMA_SA));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 94] MSDC_DMA_CA           = 0x%.8x\n", sdr_read32(MSDC_DMA_CA));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 98] MSDC_DMA_CTRL         = 0x%.8x\n", sdr_read32(MSDC_DMA_CTRL));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ 9C] MSDC_DMA_CFG          = 0x%.8x\n", sdr_read32(MSDC_DMA_CFG));                        
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ A0] MSDC_DBG_SEL          = 0x%.8x\n", sdr_read32(MSDC_DBG_SEL));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ A4] SW_DBG_OUT            = 0x%.8x\n", sdr_read32(MSDC_DBG_OUT));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ A8] MSDC_DMA_LEN          = 0x%.8x\n", sdr_read32(MSDC_DMA_LEN));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ B0] MSDC_PATCH_BIT0       = 0x%.8x\n", sdr_read32(MSDC_PATCH_BIT0));            
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ B4] MSDC_PATCH_BIT1       = 0x%.8x\n", sdr_read32(MSDC_PATCH_BIT1));
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ B8]*MSDC_PATCH_BIT2       = 0x%.8x\n", sdr_read32(MSDC_PATCH_BIT2)); /* new register @mt8590 */


	/* Only for SD/SDIO Controller 6 registers below */
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ C0]*DAT0_TUNE_CRC         = 0x%.8x\n", sdr_read32(DAT0_TUNE_CRC)); /* new register @mt8590 */
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ C4]*DAT1_TUNE_CRC         = 0x%.8x\n", sdr_read32(DAT1_TUNE_CRC)); /* new register @mt8590 */
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ C8]*DAT2_TUNE_CRC         = 0x%.8x\n", sdr_read32(DAT2_TUNE_CRC)); /* new register @mt8590 */
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ CC]*DAT3_TUNE_CRC         = 0x%.8x\n", sdr_read32(DAT3_TUNE_CRC)); /* new register @mt8590 */
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ D0]*CMD_TUNE_CRC          = 0x%.8x\n", sdr_read32(CMD_TUNE_CRC)); /* new register @mt8590 */
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ D4]*SDIO_TUNE_WIND        = 0x%.8x\n", sdr_read32(SDIO_TUNE_WIND)); /* new register @mt8590 */

    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ F0] MSDC_PAD_TUNE0	     = 0x%.8x\n", sdr_read32(MSDC_PAD_TUNE0)); 
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ F4]*MSDC_PAD_TUNE1	     = 0x%.8x\n", sdr_read32(MSDC_PAD_TUNE1)); 
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ F8] MSDC_DAT_RDDLY0	     = 0x%.8x\n", sdr_read32(MSDC_DAT_RDDLY0)); 
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[ FC] MSDC_DAT_RDDLY1	     = 0x%.8x\n", sdr_read32(MSDC_DAT_RDDLY1)); 
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[100]*MSDC_DAT_RDDLY2	     = 0x%.8x\n", sdr_read32(MSDC_DAT_RDDLY2)); 
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[104]*MSDC_DAT_RDDLY3	     = 0x%.8x\n", sdr_read32(MSDC_DAT_RDDLY3)); 
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[110] MSDC_HW_DBG	         = 0x%.8x\n", sdr_read32(MSDC_HW_DBG)); 
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[114] MSDC_VERSION	         = 0x%.8x\n", sdr_read32(MSDC_VERSION)); 
    MSDC_TRC_PRINT(MSDC_HREG, "Reg[118] MSDC_ECO_VER  	     = 0x%.8x\n", sdr_read32(MSDC_ECO_VER)); 

    if(host->id == 3){
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[180] EMMC50_PAD_CTL0	     = 0x%.8x\n", sdr_read32(EMMC50_PAD_CTL0));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[184] EMMC50_PAD_DS_CTL0	 = 0x%.8x\n", sdr_read32(EMMC50_PAD_DS_CTL0));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[188] EMMC50_PAD_DS_TUNE	 = 0x%.8x\n", sdr_read32(EMMC50_PAD_DS_TUNE));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[18C] EMMC50_PAD_CMD_TUNE   = 0x%.8x\n", sdr_read32(EMMC50_PAD_CMD_TUNE));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[190] EMMC50_PAD_DAT01_TUNE = 0x%.8x\n", sdr_read32(EMMC50_PAD_DAT01_TUNE));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[194] EMMC50_PAD_DAT23_TUNE = 0x%.8x\n", sdr_read32(EMMC50_PAD_DAT23_TUNE));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[198] EMMC50_PAD_DAT45_TUNE = 0x%.8x\n", sdr_read32(EMMC50_PAD_DAT45_TUNE));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[19C] EMMC50_PAD_DAT67_TUNE = 0x%.8x\n", sdr_read32(EMMC50_PAD_DAT67_TUNE));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[204] EMMC51_CFG0	         = 0x%.8x\n", sdr_read32(EMMC51_CFG0));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[208] EMMC50_CFG0	         = 0x%.8x\n", sdr_read32(EMMC50_CFG0));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[20C] EMMC50_CFG1	         = 0x%.8x\n", sdr_read32(EMMC50_CFG1));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[21C] EMMC50_CFG2	         = 0x%.8x\n", sdr_read32(EMMC50_CFG2));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[220] EMMC50_CFG3	         = 0x%.8x\n", sdr_read32(EMMC50_CFG3));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[224] EMMC50_CFG4	         = 0x%.8x\n", sdr_read32(EMMC50_CFG4));						  
	    MSDC_TRC_PRINT(MSDC_HREG, "Reg[228] EMMC50_BLOCK_LENGTH   = 0x%.8x\n", sdr_read32(EMMC50_BLOCK_LENGTH));						  
    }
		
#ifndef FPGA_PLATFORM
#ifdef ___MSDC_DEBUG___
		if(host->id == 0){
#ifdef CFG_DEV_MSDC0
			sdr_get_field(MSDC_CTRL(MSDC0, 6), MSDC_PAD_RDSEL(MSDC0),	u4rdsel);
			sdr_get_field(MSDC_CTRL(MSDC0, 6), MSDC_PAD_TDSEL(MSDC0),	u4tdsel);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%u Pad RDSEL(%u) TDSEL(%u)\r\n", host->id, u4rdsel,u4tdsel);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC0)), BIT_MODE_GPIOMASK(PIN4_CLK(MSDC0)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC0, 0), MSDC_CLK_SR(MSDC0),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC0, 0), BIT_MSDC_CLK_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC0, 0), MSDC_CLK_IES(MSDC0),   u4ies);
			sdr_get_field(MSDC_CTRL(MSDC0, 0), MSDC_CLK_SMT(MSDC0),   u4smt);
			sdr_get_field(MSDC_CTRL(MSDC0, 0), BIT_MSDC_CLK_R0R1PUPD, u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uCLK :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)	\r\n",\
												host->id, PIN4_CLK(MSDC0), u4mode,u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC0)), BIT_MODE_GPIOMASK(PIN4_CMD(MSDC0)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC0, 1), MSDC_CMD_SR(MSDC0),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CMD_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC0, 1), MSDC_CMD_IES(MSDC0),   u4ies);
			sdr_get_field(MSDC_CTRL(MSDC0, 1), MSDC_CMD_SMT(MSDC0),   u4smt);
			sdr_get_field(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CMD_R0R1PUPD, u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uCMD :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
												host->id, PIN4_CMD(MSDC0), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(MSDC_CTRL(MSDC0, 2), MSDC_DAT_SR(MSDC0),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC0, 2), BIT_MSDC_DAT_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC0, 2), MSDC_DAT_IES(MSDC0),   u4ies);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT0)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT0)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0, DAT0),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT0_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT0:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC0, DAT0), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT1)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT1)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0, DAT1),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT1_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT1:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC0, DAT1), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
	
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT2)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT2)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0, DAT2),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT2_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT2:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC0, DAT2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT3)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT3)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0, DAT3),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT3_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT3:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC0, DAT3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
	
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT4)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT4)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0, DAT4),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT4_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT4:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC0, DAT4), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT5)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT5)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0, DAT5),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT5_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT5:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC0, DAT5), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
	
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT6)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT6)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0, DAT6),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT6_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT6:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC0, DAT6), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT7)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT7)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0, DAT7),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT7_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT7:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC0, DAT7), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
#endif
		}
		else if(host->id == 1){
#ifdef CFG_DEV_MSDC1
			sdr_get_field(MSDC_CTRL(MSDC1, 5), MSDC_PAD_RDSEL(MSDC1),	u4rdsel);
			sdr_get_field(MSDC_CTRL(MSDC1, 5), MSDC_PAD_TDSEL(MSDC1),	u4tdsel);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%u Pad RDSEL(%u) TDSEL(%u) \r\n", host->id, u4rdsel,u4tdsel);
			
			if(host && host->hw && host->hw->flags & MSDC_CD_PIN_EN) {
				sdr_get_field(GPIO_MODE_PIN2REG(PIN4_INS(MSDC1)),          BIT_MODE_GPIOMASK(PIN4_INS(MSDC1)), u4mode);			
				sdr_get_field(GPIO_DIR_PIN2REG(PIN4_INS(MSDC1)), 	       BIT_GPIOMASK(PIN4_INS(MSDC1)),      u4dir);
				sdr_get_field(GPIO_PULLEN_PIN2REG(PIN4_INS(MSDC1)), 	   BIT_GPIOMASK(PIN4_INS(MSDC1)),      u4pullen);
				sdr_get_field(GPIO_PULLSEL_PIN2REG(PIN4_INS(MSDC1)),	   BIT_GPIOMASK(PIN4_INS(MSDC1)),      u4pulllevel);
				sdr_get_field(GPIO_DATIN_PIN2REG(PIN4_INS(MSDC1)),         BIT_GPIOMASK(PIN4_INS(MSDC1)),      u4level);
				if(46 == PIN4_INS(MSDC1)) {
				   sdr_get_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(14), u4PuR1R0);
				}
				else if(47 == PIN4_INS(MSDC1)) {
				   sdr_get_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(15), u4PuR1R0);
				}
				else if(48 == PIN4_INS(MSDC1)) {
				   sdr_get_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(16), u4PuR1R0);
				}
				else if(49 == PIN4_INS(MSDC1)) {
				   sdr_get_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(17), u4PuR1R0);
				}
				else if(142 == PIN4_INS(MSDC1)) {
				   sdr_get_field(EINT_CTRL(1), BIT_EINT_PUPDR1R0(21), u4PuR1R0);
				}

				MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uINS@EINT%u :GPIO%03u:%u: DIR(%u) Pull(%u), PLVL(%u) LVL(%u),PuR1R0(%u)\r\n",\
							host->id,EINT4_INS(MSDC1),PIN4_INS(MSDC1), u4mode, u4dir, u4pullen, u4pulllevel, u4level,u4PuR1R0);
			}
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC1)), BIT_MODE_GPIOMASK(PIN4_CLK(MSDC1)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC1, 0), MSDC_CLK_SR(MSDC1),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC1, 0), BIT_MSDC_CLK_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC1, 0), MSDC_CLK_IES(MSDC1),   u4ies);
			sdr_get_field(MSDC_CTRL(MSDC1, 0), MSDC_CLK_SMT(MSDC1),   u4smt);
			sdr_get_field(MSDC_CTRL(MSDC1, 0), BIT_MSDC_CLK_R0R1PUPD, u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uCLK :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id,PIN4_CLK(MSDC1), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC1)), BIT_MODE_GPIOMASK(PIN4_CMD(MSDC1)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC1, 1), MSDC_CMD_SR(MSDC1),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CMD_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC1, 1), MSDC_CMD_IES(MSDC1),   u4ies);
			sdr_get_field(MSDC_CTRL(MSDC1, 1), MSDC_CMD_SMT(MSDC1),   u4smt);
			sdr_get_field(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CMD_R0R1PUPD, u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uCMD :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id,PIN4_CMD(MSDC1), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(MSDC_CTRL(MSDC1, 2), MSDC_DAT_SR(MSDC1),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC1, 2), BIT_MSDC_DAT_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC1, 2), MSDC_DAT_IES(MSDC1),   u4ies);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT0)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT0)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1, DAT0),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT0_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT0:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC1, DAT0), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT1)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT1)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1, DAT1),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT1_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT1:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC1, DAT1), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
	
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT2)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT2)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1, DAT2),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT2_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT2:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC1, DAT2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT3)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT3)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1, DAT3),u4smt);		
			sdr_get_field(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT3_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT3:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC1, DAT3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
#endif
		}
		else if(host->id == 2){
#ifdef CFG_DEV_MSDC2
			sdr_get_field(MSDC_CTRL(MSDC2, 5), MSDC_PAD_RDSEL(MSDC2),	u4rdsel);
			sdr_get_field(MSDC_CTRL(MSDC2, 5), MSDC_PAD_TDSEL(MSDC2),	u4tdsel);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%u Pad RDSEL(%u) TDSEL(%u) \r\n", host->id, u4rdsel,u4tdsel);
			
			if(host && host->hw && host->hw->flags & MSDC_CD_PIN_EN) {
				sdr_get_field(GPIO_DIR_PIN2REG(PIN4_INS(MSDC2)), 	       BIT_GPIOMASK(PIN4_INS(MSDC2)),      u4dir);
				sdr_get_field(GPIO_PULLEN_PIN2REG(PIN4_INS(MSDC2)), 	   BIT_GPIOMASK(PIN4_INS(MSDC2)),      u4pullen);
				sdr_get_field(GPIO_PULLSEL_PIN2REG(PIN4_INS(MSDC2)),	   BIT_GPIOMASK(PIN4_INS(MSDC2)),      u4pulllevel);
				sdr_get_field(GPIO_MODE_PIN2REG(PIN4_INS(MSDC2)), 	       BIT_MODE_GPIOMASK(PIN4_INS(MSDC2)), u4mode);			
				sdr_get_field(GPIO_DATIN_PIN2REG(PIN4_INS(MSDC2)),         BIT_GPIOMASK(PIN4_INS(MSDC2)), 	   u4level);
				if(46 == PIN4_INS(MSDC2)) {
				   sdr_get_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(14), u4PuR1R0);
				}
				else if(47 == PIN4_INS(MSDC2)) {
				   sdr_get_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(15), u4PuR1R0);
				}
				else if(48 == PIN4_INS(MSDC2)) {
				   sdr_get_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(16), u4PuR1R0);
				}
				else if(49 == PIN4_INS(MSDC2)) {
				   sdr_get_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(17), u4PuR1R0);
				}
				else if(142 == PIN4_INS(MSDC2)) {
				   sdr_get_field(EINT_CTRL(1), BIT_EINT_PUPDR1R0(21), u4PuR1R0);
				}
				
				MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uINS@EINT%u :GPIO%03u:%u: DIR(%u) Pull(%u), PLVL(%u) LVL(%u),PuR1R0(%u)\r\n",\
							host->id,EINT4_INS(MSDC2), PIN4_INS(MSDC2), u4mode, u4dir, u4pullen, u4pulllevel, u4level,u4PuR1R0);
			}

			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC2)), BIT_MODE_GPIOMASK(PIN4_CLK(MSDC2)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC2, 0), MSDC_CLK_SR(MSDC2),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC2, 0), BIT_MSDC_CLK_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC2, 0), MSDC_CLK_IES(MSDC2),   u4ies);
			sdr_get_field(MSDC_CTRL(MSDC2, 0), MSDC_CLK_SMT(MSDC2),   u4smt);
			sdr_get_field(MSDC_CTRL(MSDC2, 0), BIT_MSDC_CLK_R0R1PUPD, u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uCLK :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id,PIN4_CLK(MSDC2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC2)), BIT_MODE_GPIOMASK(PIN4_CMD(MSDC2)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC2, 1), MSDC_CMD_SR(MSDC2),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CMD_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC2, 1), MSDC_CMD_IES(MSDC2),   u4ies);
			sdr_get_field(MSDC_CTRL(MSDC2, 1), MSDC_CMD_SMT(MSDC2),   u4smt);
			sdr_get_field(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CMD_R0R1PUPD, u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uCMD :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id,PIN4_CMD(MSDC2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(MSDC_CTRL(MSDC2, 2), MSDC_DAT_SR(MSDC2),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC2, 2), BIT_MSDC_DAT_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC2, 2), MSDC_DAT_IES(MSDC2),   u4ies);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT0)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT0)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2, DAT0),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT0_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT0:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC2, DAT0), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT1)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT1)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2, DAT1),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT1_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT1:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC2, DAT1), u4mode,u4sr, u4tx, u4ies, u4smt, u4rx);
	
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT2)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT2)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2, DAT2),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT2_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT2:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC2, DAT2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT3)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT3)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2, DAT3),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT3_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT3:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC2, DAT3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
#endif
		}
        else if(host->id == 3){
#ifdef CFG_DEV_MSDC3
			sdr_get_field(MSDC_CTRL(MSDC3, 6), MSDC_PAD_RDSEL(MSDC3),	u4rdsel);
			sdr_get_field(MSDC_CTRL(MSDC3, 6), MSDC_PAD_TDSEL(MSDC3),	u4tdsel);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%u Pad RDSEL(%u) TDSEL(%u) \r\n", host->id, u4rdsel,u4tdsel);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC3)), BIT_MODE_GPIOMASK(PIN4_CLK(MSDC3)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC3, 0), MSDC_CLK_SR(MSDC3),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC3, 0), BIT_MSDC_CLK_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC3, 0), MSDC_CLK_IES(MSDC3),   u4ies);
			sdr_get_field(MSDC_CTRL(MSDC3, 0), MSDC_CLK_SMT(MSDC3),   u4smt);
			sdr_get_field(MSDC_CTRL(MSDC3, 0), BIT_MSDC_CLK_R0R1PUPD, u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uCLK :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id,PIN4_CLK(MSDC3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC3)), BIT_MODE_GPIOMASK(PIN4_CMD(MSDC3)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC3, 1), MSDC_CMD_SR(MSDC3),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CMD_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC3, 1), MSDC_CMD_IES(MSDC3),   u4ies);
			sdr_get_field(MSDC_CTRL(MSDC3, 1), MSDC_CMD_SMT(MSDC3),   u4smt);
			sdr_get_field(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CMD_R0R1PUPD, u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uCMD :GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id,PIN4_CMD(MSDC3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(MSDC_CTRL(MSDC3, 2), MSDC_DAT_SR(MSDC3),    u4sr);
			sdr_get_field(MSDC_CTRL(MSDC3, 2), BIT_MSDC_DAT_E8E4E2,   u4tx);	
			sdr_get_field(MSDC_CTRL(MSDC3, 2), MSDC_DAT_IES(MSDC3),   u4ies);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT0)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT0)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3, DAT0),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT0_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT0:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC3, DAT0), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT1)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT1)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3, DAT1),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT1_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT1:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC3, DAT1), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
	
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT2)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT2)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3, DAT2),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT2_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT2:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC3, DAT2), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT3)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT3)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3, DAT3),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT3_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT3:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC3, DAT3), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
	
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT4)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT4)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3, DAT4),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT4_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT4:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC3, DAT4), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT5)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT5)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3, DAT5),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT5_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT5:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC3, DAT5), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
	
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT6)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT6)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3, DAT6),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT6_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT6:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC3, DAT6), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);
			
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT7)), BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT7)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3, DAT7),u4smt);
			sdr_get_field(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT7_R0R1PUPD,u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDAT7:GPIO%03u:%u: SR(%u) TX(%u), IES(%u) SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DAT(MSDC3, DAT7), u4mode, u4sr, u4tx, u4ies, u4smt, u4rx);	
	
			sdr_get_field(GPIO_MODE_PIN2REG(PIN4_DSL(MSDC3)), BIT_MODE_GPIOMASK(PIN4_DSL(MSDC3)), u4mode);
			sdr_get_field(MSDC_CTRL(MSDC3, 6), MSDC_DSL_SMT(MSDC3),   u4smt);
			sdr_get_field(MSDC_CTRL(MSDC3, 6), BIT_MSDC_DSL_R0R1PUPD, u4rx);
			MSDC_TRC_PRINT(MSDC_GPIO,"MSDC%uDSL :GPIO%03u:%u: SMT(%u) RX(%u)\r\n",\
											host->id, PIN4_DSL(MSDC3), u4mode, u4smt, u4rx); 
#endif
		}
#endif /* ___MSDC_DEBUG___*/
#endif /* FPGA_PLATFORM */		
}


static void msdc_dump_dbg_register(struct msdc_host *host)
{
#ifdef MTK_MSDC_BRINGUP_DEBUG
    void __iomem *base = host->base;
    u32 i;

    for (i=0; i <= 0x27; i++) {
        sdr_write32(MSDC_DBG_SEL, i);
        MSDC_TRC_PRINT(MSDC_HREG, "SW_DBG_SEL: write reg[%x] to 0x%x", OFFSET_MSDC_DBG_SEL, i);
        MSDC_TRC_PRINT(MSDC_HREG, "SW_DBG_OUT: read  reg[%x] to 0x%x", OFFSET_MSDC_DBG_OUT, sdr_read32(MSDC_DBG_OUT));
    }

    sdr_write32(MSDC_DBG_SEL, 0);
#endif
}

static void msdc_dump_clock_sts(struct msdc_host *host)
{
#ifdef MTK_MSDC_BRINGUP_DEBUG
    MSDC_TRC_PRINT(MSDC_INFO, " msdcpll en[%p+0x250][bit0   should be 0x1]=0x%x", msdc_apmixed_base,  sdr_read32(msdc_apmixed_base+0x250));
    MSDC_TRC_PRINT(MSDC_INFO, " msdcpll on[%p+0x25C][bit0~1 should be 0x1]=0x%x", msdc_apmixed_base,  sdr_read32(msdc_apmixed_base+0x25C));

    MSDC_TRC_PRINT(MSDC_INFO, " ckgen pdn, mux[%p+0x070]msdc0(7 2~0), msdc1(15 10~8) msdc2(23 18~16)]=0x%x", msdc_topckgen_base,sdr_read32(msdc_topckgen_base+0x070));
    MSDC_TRC_PRINT(MSDC_INFO, " ckgen pdn, mux[%p+0x080][msdc3(7 2~0) with hclk(15 10~8)]=0x%x", msdc_topckgen_base,sdr_read32(msdc_topckgen_base+0x080));

    MSDC_TRC_PRINT(MSDC_INFO, " infra_ao, status[%p+0x094][bit6,4,5,x for msdc0,1,2,3]=0x%x", msdc_infracfg_base, sdr_read32(msdc_infracfg_base+0x094));
	
#endif
}

static void msdc_dump_ldo_sts(struct msdc_host *host)
{
#ifdef MTK_MSDC_BRINGUP_DEBUG
    u32 ldo_en = 0, ldo_vol = 0;

    switch(host->id){
        case 0:
            pwrap_read( 0x0A18, &ldo_en );
            pwrap_read( 0x0A4A, &ldo_vol );
            MSDC_TRC_PRINT(MSDC_INFO, " VEMC_EN[0x0A18][bit1=%s]=0x%x, VEMC_VOL[0x0A4A][bit9=%s]=0x%x", (((ldo_en >> 1) & 0x01) ? "enable" : "disable"), ldo_en, (((ldo_vol >> 9) & 0x01) ? "3.3V" : "3.0V"), ldo_vol);
            break;
        case 1:
            pwrap_read( 0x0A16, &ldo_en );
            pwrap_read( 0x0A48, &ldo_vol );
            MSDC_TRC_PRINT(MSDC_INFO, " VMC_EN[0x0A16][bit1=%s]=0x%x, VMC_VOL[0x0A48][bit1=%s]=0x%x", (((ldo_en >> 1) & 0x01) ? "enable" : "disable"), ldo_en, (((ldo_vol >> 1) & 0x01) ? "3.3V" : "1.8V"), ldo_vol);
            pwrap_read( 0x0A14, &ldo_en );
            pwrap_read( 0x0A48, &ldo_vol );
            MSDC_TRC_PRINT(MSDC_INFO, " VMCH_EN[0x0A14][bit1=%s]=0x%x, VMCH_VOL[0x0A48][bit9=%s]=0x%x", (((ldo_en >> 1) & 0x01) ? "enable" : "disable"), ldo_en, (((ldo_vol >> 9) & 0x01) ? "3.3V" : "3.0V"), ldo_vol);
            break;
        default:
            break;
    }
#endif
}
unsigned int msdc_dump_reg_level=2;
void msdc_dump_info(u32 id)
{
    struct msdc_host *host = mtk_msdc_host[id];
    void __iomem *base;

    if ( msdc_dump_reg_level==0 ) return;

    MSDC_ASSERT(host);
    base = host->base;

    // 1: dump msdc hw register
    msdc_dump_register(host);
    MSDC_DBG_PRINT(MSDC_DEBUG, "latest_INT_status<0x%.8x>",latest_int_status[id]);

    if ( msdc_dump_reg_level==1 ) return;

    // 2: check msdc clock gate and clock source
    msdc_dump_clock_sts(host);

    // 3: check msdc pmic ldo
    msdc_dump_ldo_sts(host);

    // 4: check gpio and msdc pad control

    // 5: For designer
    msdc_dump_dbg_register(host);
}

/*
 * for AHB read / write debug
 * return DMA status.
 */
int msdc_get_dma_status(u32 id)
{
    int result = -1;

    MSDC_ASSERT(id < HOST_MAX_NUM);

    if(msdc_latest_transfer_mode[id] == TRAN_MOD_DMA)
    {
        switch(msdc_latest_operation_type[id])
        {
            case OPER_TYPE_READ:
                result = 1; // DMA read
                break;
            case OPER_TYPE_WRITE:
                result = 2; // DMA write
                break;
            default:
                break;
        }
    }else if(msdc_latest_transfer_mode[id] == TRAN_MOD_PIO){
        result = 0; // PIO mode
    }

    return result;
}
EXPORT_SYMBOL(msdc_get_dma_status);

struct dma_addr* msdc_get_dma_address(u32 id)
{
    bd_t* bd;
    int i = 0;
    int mode = -1;
    struct msdc_host *host;
    void __iomem *base;
	
    MSDC_ASSERT(id < HOST_MAX_NUM);
    MSDC_ASSERT(mtk_msdc_host[id]);


    host = mtk_msdc_host[id];
    base = host->base;
    //spin_lock(&host->lock);
    sdr_get_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, mode);
    if(mode == 1){
        MSDC_DBG_PRINT(MSDC_DEBUG, "Desc.DMA\n");
        bd = host->dma.bd;
        i = 0;
        while(i < MAX_BD_PER_GPD){
            msdc_latest_dma_address[i].start_address = (u32)bd[i].ptr;
            msdc_latest_dma_address[i].size = bd[i].buflen;
            msdc_latest_dma_address[i].end = bd[i].eol;
            if(i>0)
                msdc_latest_dma_address[i-1].next = &msdc_latest_dma_address[i];

            if(bd[i].eol){
                break;
            }
            i++;
        }
    }
    else if(mode == 0){
        MSDC_DBG_PRINT(MSDC_DEBUG, "Basic DMA\n");
        msdc_latest_dma_address[i].start_address = sdr_read32(MSDC_DMA_SA);
    msdc_latest_dma_address[i].size = sdr_read32(MSDC_DMA_LEN);
        msdc_latest_dma_address[i].end = 1;
    }

    //spin_unlock(&host->lock);

    return msdc_latest_dma_address;

}
EXPORT_SYMBOL(msdc_get_dma_address);

#define msdc_retry(expr, retry, cnt,id) \
    do { \
        int backup = cnt; \
        while (retry) { \
            if (!(expr)) break; \
            if (cnt-- == 0) { \
                retry--; mdelay(1); cnt = backup; \
            } \
        } \
        if (retry == 0) { \
            MSDC_TRACE(MSDC_DEBUG, msdc_dump_info, (id)); \
        } \
        WARN_ON(retry == 0); \
    } while(0)

#define msdc_reset(id) \
    do { \
        int retry = 3, cnt = 1000; \
        sdr_set_bits(MSDC_CFG, MSDC_CFG_RST); \
        mb(); \
        msdc_retry(sdr_read32(MSDC_CFG) & MSDC_CFG_RST, retry, cnt,id); \
    } while(0)

#define msdc_clr_int() \
    do { \
        volatile u32 val = sdr_read32(MSDC_INT); \
        sdr_write32(MSDC_INT, val); \
    } while(0)

#define msdc_clr_fifo(id) \
    do { \
        int retry = 3, cnt = 1000; \
        sdr_set_bits(MSDC_FIFOCS, MSDC_FIFOCS_CLR); \
        msdc_retry(sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_CLR, retry, cnt,id); \
    } while(0)

#define msdc_reset_hw(id) \
    do { \
        msdc_reset(id); \
        msdc_clr_fifo(id); \
        msdc_clr_int(); \
    } while(0)

#ifdef MSDC_TOP_RESET_ERROR_TUNE
//only for axi wrapper of msdc0 dma mode
#define msdc_reset_gdma() \
    do { \
        msdc_reset(3); \
        sdr_set_bits(EMMC50_CFG0, MSDC_EMMC50_CFG_GDMA_RESET); \
    } while (0)

int top_reset_restore_msdc_cfg(struct msdc_host *host, int restore)
{
    int retry = 3, count = 1000;
    void __iomem *base = host->base;

    if(restore) {
        msdc_retry(!(sdr_read32(MSDC_CFG) & MSDC_CFG_CKSTB), retry, count, 0);
        if((retry == 0) && (count == 0)){
            return 1;
        }
    }

    return 0;
}

/*
 *  2013-12-09
 *  when desc/basic dma data error happend, or enhance dma cmd/acmd/dma data error happend, do backup/restore following register bits before/after top layer reset
 */
#define TOP_RESET_BACKUP_REG_NUM (20)
unsigned int emmc_sdc_data_crc_value = 0x0;
unsigned char emmc_top_reset = 0;
struct msdc_reg_control top_reset_backup_reg_list[TOP_RESET_BACKUP_REG_NUM] = {
        {(OFFSET_MSDC_CFG),               0xffffffff, 0x0, 0x0, top_reset_restore_msdc_cfg},
        {(OFFSET_MSDC_IOCON),             0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_MSDC_INTEN),             0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_SDC_CFG),                0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_SDC_CSTS_EN),            0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_MSDC_PATCH_BIT0),        0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_MSDC_PATCH_BIT1),        0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_MSDC_PAD_TUNE),          0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_MSDC_DAT_RDDLY0),        0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_MSDC_DAT_RDDLY1),        0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_EMMC50_PAD_CTL0),        0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_EMMC50_PAD_DS_CTL0),     0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_EMMC50_PAD_DS_TUNE),     0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_EMMC50_PAD_CMD_TUNE),    0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_EMMC50_PAD_DAT01_TUNE),  0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_EMMC50_PAD_DAT23_TUNE),  0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_EMMC50_PAD_DAT45_TUNE),  0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_EMMC50_PAD_DAT67_TUNE),  0xffffffff, 0x0, 0x0, NULL},
        {(OFFSET_EMMC50_CFG0),            0x7fffffff, 0x0, 0x0, NULL},
        {(OFFSET_EMMC50_CFG1),            0xffffffff, 0x0, 0x0, NULL},
};
/*
 *  2013-12-09
 *  when desc/basic dma data error happend, or enhance dma cmd/acmd/dma data error happend, do top layer reset
 */
static void msdc_top_reset(struct msdc_host *host){
    int i = 0, err = 0;
    void __iomem *base;
	u32 id = host->id;
    u64 msdcioaddr = 0; 


    MSDC_ASSERT(id == 3);

    base = host->base;
    sdr_get_field(SDC_DCRC_STS, (SDC_DCRC_STS_NEG | SDC_DCRC_STS_POS), emmc_sdc_data_crc_value);
    for(i = 0; i < TOP_RESET_BACKUP_REG_NUM; i++)
    {
		msdcioaddr = ((u64)host->base + top_reset_backup_reg_list[i].addr);
        sdr_get_field(msdcioaddr, top_reset_backup_reg_list[i].mask,  top_reset_backup_reg_list[i].value);
        if(top_reset_backup_reg_list[i].restore_func){
            err = top_reset_backup_reg_list[i].restore_func(host,0);
            if(err) {
                MSDC_ERR_PRINT(MSDC_ERROR,"failed to backup reg[0x%x][0x%x], expected value[0x%x], actual value[0x%x] err=0x%x",
                        top_reset_backup_reg_list[i].addr, top_reset_backup_reg_list[i].mask, top_reset_backup_reg_list[i].value, sdr_read32(msdcioaddr), err);
            }
        }
    }

    MSDC_TRC_PRINT(MSDC_INFO, "[%s]: msdc%d, top reset\n", __func__, host->id);
    //sdr_set_field(0xf0003000, (0x1 << 19), 0x1);
    //mdelay(1);
    //sdr_set_field(0xf0003000, (0x1 << 19), 0x0);
#ifdef MSDC_TOP_RESET_ERROR_TUNE
    emmc_top_reset = 1;
#endif

    for(i = 0; i < TOP_RESET_BACKUP_REG_NUM; i++)
    {
		msdcioaddr = ((u64)host->base + top_reset_backup_reg_list[i].addr);    
        sdr_set_field(msdcioaddr, top_reset_backup_reg_list[i].mask,  top_reset_backup_reg_list[i].value);
        if(top_reset_backup_reg_list[i].restore_func){
            err = top_reset_backup_reg_list[i].restore_func(host, 1);
            if(err) {
                MSDC_ERR_PRINT(MSDC_ERROR,"failed to restore reg[0x%x][0x%x], expected value[0x%x], actual value[0x%x] err=0x%x",
                        top_reset_backup_reg_list[i].addr, top_reset_backup_reg_list[i].mask, top_reset_backup_reg_list[i].value, sdr_read32(msdcioaddr), err);
            }
        }
    }

    return;
}
#endif

static int msdc_clk_stable(struct msdc_host *host,u32 mode, u32 div, u32 hs400_src){
    void __iomem *base = host->base;
    int retry = 0;
    int cnt = 1000;
    int retry_cnt = 1;
    do{
        retry = 3;
        sdr_set_field(MSDC_CFG,MSDC_CFG_CKMOD_HS400, hs400_src);
        sdr_set_field(MSDC_CFG,MSDC_CFG_CKMOD, mode);
        sdr_set_field(MSDC_CFG,MSDC_CFG_CKDIV,  ((div + retry_cnt) % 0xFFF));
        //sdr_set_field(MSDC_CFG, MSDC_CFG_CKMOD, mode);
        msdc_retry(!(sdr_read32(MSDC_CFG) & MSDC_CFG_CKSTB), retry, cnt,host->id);
        if(retry == 0){
            MSDC_ERR_PRINT(MSDC_ERROR, "host->onclock(%d) failed ===> retry twice\n",host->core_clkon);
#ifndef FPGA_PLATFORM
            msdc_disable_clock(host);
            msdc_enable_clock(host);
#endif
            MSDC_TRACE(MSDC_DEBUG, msdc_dump_info, (host->id));
        }
        retry = 3;
        sdr_set_field(MSDC_CFG, MSDC_CFG_CKDIV, div);
        msdc_retry(!(sdr_read32(MSDC_CFG) & MSDC_CFG_CKSTB), retry, cnt,host->id);
        if(retry == 0){
            MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
        }
        msdc_reset_hw(host->id);
        if(retry_cnt == 2)
            break;
        retry_cnt += 1;
    }while(!retry);

    return 0;
}

#define msdc_irq_save(val) \
    do { \
        val = sdr_read32(MSDC_INTEN); \
        sdr_clr_bits(MSDC_INTEN, val); \
    } while(0)

#define msdc_irq_restore(val) \
    do { \
        sdr_set_bits(MSDC_INTEN, val); \
    } while(0)

/* clock source for host: global */
static u32 hclks_msdc50[] = { 26000000, 400000000, 200000000, 156000000,
                             182000000, 156000000, 100000000, 624000000, 312000000};
static u32 hclks_msdc30[] = { 26000000, 208000000, 200000000, 156000000,
	                         182000000, 156000000, 178280000 };


/* VMCH is for T-card main power.
 * VMC for T-card when no emmc, for eMMC when has emmc.
 * VGP for T-card when has emmc.
 */
u32 g_msdc0_io     = 0;
u32 g_msdc0_flash  = 0;
u32 g_msdc1_io     = 0;
u32 g_msdc1_flash  = 0;
u32 g_msdc2_io     = 0;
u32 g_msdc2_flash  = 0;
u32 g_msdc3_io     = 0;
u32 g_msdc3_flash  = 0;
u32 g_msdc4_io     = 0;
u32 g_msdc4_flash  = 0;

/* set start bit of data sampling */
void msdc_set_startbit(struct msdc_host *host, u8 start_bit)
{
    void __iomem *base = host->base;
    u32 id = host->id;
	
    /* set start bit */
    sdr_set_field(MSDC_CFG, MSDC_CFG_START_BIT, start_bit);
    MSDC_DBG_PRINT(MSDC_DEBUG, "[MSDC%u] finished, start_bit=%d\n", id, start_bit);
}

/* set the edge of data sampling */
void msdc_set_smpl(struct msdc_host *host, u8 HS400, u8 mode, u8 type, u8* edge)
{
    void __iomem *base = host->base;
    int i=0;

    switch(type)
    {
        case TYPE_CMD_RESP_EDGE:
            if(HS400){
                // eMMC5.0 only output resp at CLK pin, so no need to select DS pin
                sdr_set_field(EMMC50_CFG0, MSDC_EMMC50_CFG_PADCMD_LATCHCK, 0); //latch cmd resp at CLK pin
                sdr_set_field(EMMC50_CFG0, MSDC_EMMC50_CFG_CMD_RESP_SEL, 0);//latch cmd resp at CLK pin
            }

            if(mode == MSDC_SMPL_RISING|| mode == MSDC_SMPL_FALLING){
#if 0
                if(HS400){
                    sdr_set_field(EMMC50_CFG0, MSDC_EMMC50_CFG_CMD_EDGE_SEL, mode);
                }else{
                    sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, mode);
                }
#else
                sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, mode);
#endif
            }else {
                MSDC_ERR_PRINT(MSDC_ERROR, "invalid resp parameter: HS400=%d, type=%d, mode=%d\n",HS400, type, mode);
            }
            break;
        case TYPE_WRITE_CRC_EDGE:
            if(HS400){
                sdr_set_field(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 1);//latch write crc status at DS pin
            }else {
                sdr_set_field(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 0);//latch write crc status at CLK pin
            }

            if(mode == MSDC_SMPL_RISING|| mode == MSDC_SMPL_FALLING){
                if(HS400){
                    sdr_set_field(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_EDGE, mode);
                }else{
                    sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 0);
                    sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, mode);
                }
            }else if((mode == MSDC_SMPL_SEPERATE) && !HS400 && (edge != NULL)) {
                sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D0SPL, edge[0]); //only dat0 is for write crc status.
            }else {
                MSDC_ERR_PRINT(MSDC_ERROR, "invalid crc parameter: HS400=%d, type=%d, mode=%d\n", HS400, type, mode);
            }
            break;
        case TYPE_READ_DATA_EDGE:
            if(HS400){
                msdc_set_startbit(host, START_AT_RISING_AND_FALLING); //for HS400, start bit is output both on rising and falling edge
            }else {
                msdc_set_startbit(host, START_AT_RISING); //for the other mode, start bit is only output on rising edge. but DDR50 can try falling edge if error casued by pad delay
            }
            if(mode == MSDC_SMPL_RISING|| mode == MSDC_SMPL_FALLING){
                sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 0);
                sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, mode);
            }else if((mode == MSDC_SMPL_SEPERATE) && (edge != NULL) && (sizeof(edge) == 8)) {
                sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 1);
                for(i=0; i<8; i++);
                {
                    sdr_set_field(MSDC_IOCON, (MSDC_IOCON_R_D0SPL << i), edge[i]);
                }
            }else {
                MSDC_ERR_PRINT(MSDC_ERROR, "invalid read parameter: HS400=%d, type=%d, mode=%d\n", HS400, type, mode);
            }
            break;
        case TYPE_WRITE_DATA_EDGE:
            sdr_set_field(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 0);//latch write crc status at CLK pin

            if(mode == MSDC_SMPL_RISING|| mode == MSDC_SMPL_FALLING){
                sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 0);
                sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, mode);
            }else if((mode == MSDC_SMPL_SEPERATE) && (edge != NULL) && (sizeof(edge) >= 4)) {
                sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 1);
                for(i=0; i<4; i++);
                {
                    sdr_set_field(MSDC_IOCON, (MSDC_IOCON_W_D0SPL << i), edge[i]);//dat0~4 is for SDIO card.
                }
            }else {
                MSDC_ERR_PRINT(MSDC_ERROR, "invalid write parameter: HS400=%d, type=%d, mode=%d\n", HS400, type, mode);
            }
            break;
        default:
            MSDC_ERR_PRINT(MSDC_ERROR, "invalid parameter: HS400=%d, type=%d, mode=%d\n", HS400, type, mode);
            break;
    }
}

static u32 msdc_ldo_power(u32 on, MT_POWER powerId, MT_POWER_VOLTAGE powerVolt, u32 *status)
{
#ifndef FPGA_PLATFORM
    if (on) { // want to power on
        if (*status == 0) {  // can power on
            MSDC_TRC_PRINT(MSDC_INFO, "msdc LDO<%d> power on<%d>\n", powerId, powerVolt);
            hwPowerOn(powerId, powerVolt, "msdc");
            *status = powerVolt;
        } 
		else if (*status == powerVolt) {
            MSDC_TRC_PRINT(MSDC_INFO, "msdc LDO<%d><%d> power on again!\n", powerId, powerVolt);
        }
		else { // for sd3.0 later
            MSDC_TRC_PRINT(MSDC_INFO, "msdc LDO<%d> change<%d> to <%d>\n", powerId, *status, powerVolt);
            hwPowerDown(powerId, "msdc");
            hwPowerOn(powerId, powerVolt, "msdc");
            *status = powerVolt;
        }
    } 
	else {  // want to power off
        if (*status != 0) {  // has been powerred on
            MSDC_TRC_PRINT(MSDC_INFO, "msdc LDO<%d> power off\n", powerId);
            hwPowerDown(powerId, "msdc");
            *status = 0;
        }
		else {
            MSDC_TRC_PRINT(MSDC_INFO, "LDO<%d> not power on\n", powerId);
        }
    }
#endif
    return 0;
}

void msdc_set_sr(struct msdc_host *host,int clk,int cmd, int dat, int rst, int ds)
{
    u32 id;

    MSDC_ASSERT(host);
	MSDC_ASSERT(host->id < HOST_MAX_NUM);
    void __iomem *gpiobase = host->gpiobase;
    MSDC_ASSERT(gpiobase);
	
#ifndef FPGA_PLATFORM	
	id=host->id;
	switch(id){
#ifdef CFG_DEV_MSDC0
		case 0:
            sdr_set_field(MSDC_CTRL(MSDC0, 1), MSDC_CLK_SR(MSDC0),  clk);
            sdr_set_field(MSDC_CTRL(MSDC0, 1), MSDC_CMD_SR(MSDC0),  cmd);
            sdr_set_field(MSDC_CTRL(MSDC0, 2), MSDC_DAT_SR(MSDC0),  dat);
            //sdr_set_field(MSDC_CTRL(MSDC0, 5), MSDC_RSTB_SR(MSDC0), rst);
			break;
#endif
#ifdef CFG_DEV_MSDC1
		case 1:
            sdr_set_field(MSDC_CTRL(MSDC1, 1), MSDC_CLK_SR(MSDC1),  clk);
            sdr_set_field(MSDC_CTRL(MSDC1, 1), MSDC_CMD_SR(MSDC1),  cmd);
            sdr_set_field(MSDC_CTRL(MSDC1, 2), MSDC_DAT_SR(MSDC1),  dat);
			break;
#endif
#ifdef CFG_DEV_MSDC2
		case 2:
            sdr_set_field(MSDC_CTRL(MSDC2, 1), MSDC_CLK_SR(MSDC2),  clk);
            sdr_set_field(MSDC_CTRL(MSDC2, 1), MSDC_CMD_SR(MSDC2),  cmd);
            sdr_set_field(MSDC_CTRL(MSDC2, 2), MSDC_DAT_SR(MSDC2),  dat);
			break;
#endif
#ifdef CFG_DEV_MSDC3
		case 3:
            sdr_set_field(MSDC_CTRL(MSDC3, 1), MSDC_CLK_SR(MSDC3),  clk);
            sdr_set_field(MSDC_CTRL(MSDC3, 1), MSDC_CMD_SR(MSDC3),  cmd);
            sdr_set_field(MSDC_CTRL(MSDC3, 2), MSDC_DAT_SR(MSDC3),  dat);
            //sdr_set_field(MSDC_CTRL(MSDC3, 5), MSDC_RSTB_SR(MSDC3), rst);
            //sdr_set_field(MSDC_CTRL(MSDC3, 5), MSDC_DSL_SR(MSDC3),  ds);
            break;		
#endif
		default:
			break;
		}
#endif
}

#if 10
void msdc_set_rdtdsel_dbg(struct msdc_host *host, bool rdsel, u32 value)
{
		u32 id;

		MSDC_ASSERT(host);
		MSDC_ASSERT(host->id < HOST_MAX_NUM);
		void __iomem *gpiobase = host->gpiobase;
		MSDC_ASSERT(gpiobase);
#ifndef FPGA_PLATFORM	

		id=host->id;
	
		switch(id){
#ifdef CFG_DEV_MSDC0
			case 0: //always 1.8V
			    if (!rdsel)
				   sdr_set_field(MSDC_CTRL(MSDC0, 6), MSDC_PAD_TDSEL(MSDC0), value);
				else
				   sdr_set_field(MSDC_CTRL(MSDC0, 6), MSDC_PAD_RDSEL(MSDC0), value);
                break;
#endif
#ifdef CFG_DEV_MSDC1
            case 1:
			    if (!rdsel)
					sdr_set_field(MSDC_CTRL(MSDC1,5), MSDC_PAD_TDSEL(MSDC1), value);
				else
					sdr_set_field(MSDC_CTRL(MSDC1,5), MSDC_PAD_RDSEL(MSDC1), value);

                break;
#endif
#ifdef CFG_DEV_MSDC2
			case 2:
			    if (!rdsel)
					sdr_set_field(MSDC_CTRL(MSDC2,5), MSDC_PAD_TDSEL(MSDC2), value);
				else 
					sdr_set_field(MSDC_CTRL(MSDC2,5), MSDC_PAD_RDSEL(MSDC2), value);

                break;
#endif
#ifdef CFG_DEV_MSDC3
			case 3: //always 1.8V
			    if (!rdsel)
				   sdr_set_field(MSDC_CTRL(MSDC3,6), MSDC_PAD_TDSEL(MSDC3), value);
				else 
				   sdr_set_field(MSDC_CTRL(MSDC3,6), MSDC_PAD_RDSEL(MSDC3), value);
				break;
#endif
            default:
                break;
        }
#endif

}

void msdc_get_rdtdsel_dbg(struct msdc_host *host, bool rdsel, u32 *value)
{
			u32 id;
		
			MSDC_ASSERT(host);
			MSDC_ASSERT(host->id < HOST_MAX_NUM);
			void __iomem *gpiobase = host->gpiobase;
			MSDC_ASSERT(gpiobase);
#ifndef FPGA_PLATFORM	
			
			id=host->id;
		
        switch (id) {
#ifdef CFG_DEV_MSDC0
				case 0: //always 1.8V
					if (!rdsel)
					   sdr_get_field(MSDC_CTRL(MSDC0, 6), MSDC_PAD_TDSEL(MSDC0), value);
					else
					   sdr_get_field(MSDC_CTRL(MSDC0, 6), MSDC_PAD_RDSEL(MSDC0), value);
                break;
#endif
#ifdef CFG_DEV_MSDC1
				case 1:
					if (!rdsel)
						sdr_get_field(MSDC_CTRL(MSDC1,5), MSDC_PAD_TDSEL(MSDC1), value);
					else
						sdr_get_field(MSDC_CTRL(MSDC1,5), MSDC_PAD_RDSEL(MSDC1), value);

                break;
#endif
#ifdef CFG_DEV_MSDC2
            case 2:
					if (!rdsel)
						sdr_get_field(MSDC_CTRL(MSDC2,5), MSDC_PAD_TDSEL(MSDC2), value);
					else 
						sdr_get_field(MSDC_CTRL(MSDC2,5), MSDC_PAD_RDSEL(MSDC2), value);
	
                break;
#endif
#ifdef CFG_DEV_MSDC3
				case 3: //always 1.8V
					if (!rdsel)
					   sdr_get_field(MSDC_CTRL(MSDC3,6), MSDC_PAD_TDSEL(MSDC3), value);
					else 
					   sdr_get_field(MSDC_CTRL(MSDC3,6), MSDC_PAD_RDSEL(MSDC3), value);
                break;
#endif
            default:
                break;
        }
#endif

}
#endif

void msdc_set_rdtdsel(struct msdc_host *host,bool sd_18)
{
    u32 id;

    MSDC_ASSERT(host);
	MSDC_ASSERT(host->id < HOST_MAX_NUM);
    void __iomem *gpiobase = host->gpiobase;
    MSDC_ASSERT(gpiobase);
#ifndef FPGA_PLATFORM	
	
	id=host->id;

    switch(id){
#ifdef CFG_DEV_MSDC0
        case 0: //always 1.8V
            sdr_set_field(MSDC_CTRL(MSDC0, 6), MSDC_PAD_TDSEL(MSDC0),0x0A);
            sdr_set_field(MSDC_CTRL(MSDC0, 6), MSDC_PAD_RDSEL(MSDC0),0);
            break;
#endif
#ifdef CFG_DEV_MSDC1
        case 1:
            if (sd_18){
                sdr_set_field(MSDC_CTRL(MSDC1,5), MSDC_PAD_TDSEL(MSDC1),0x0A);
                sdr_set_field(MSDC_CTRL(MSDC1,5), MSDC_PAD_RDSEL(MSDC1),0);
            }
			else{
                sdr_set_field(MSDC_CTRL(MSDC1,5), MSDC_PAD_TDSEL(MSDC1),0x0A);
                sdr_set_field(MSDC_CTRL(MSDC1,5), MSDC_PAD_RDSEL(MSDC1),0x0C);
            }
            break;
#endif
#ifdef CFG_DEV_MSDC2
        case 2:
            if (sd_18){
                sdr_set_field(MSDC_CTRL(MSDC2,5), MSDC_PAD_TDSEL(MSDC2),0x0A);
                sdr_set_field(MSDC_CTRL(MSDC2,5), MSDC_PAD_RDSEL(MSDC2),0x0);
            }
			else{
                sdr_set_field(MSDC_CTRL(MSDC2,5), MSDC_PAD_TDSEL(MSDC2),0x0);
                sdr_set_field(MSDC_CTRL(MSDC2,5), MSDC_PAD_RDSEL(MSDC2),0x0C);
            }
            break;
#endif
#ifdef CFG_DEV_MSDC3
        case 3: //always 1.8V
			sdr_set_field(MSDC_CTRL(MSDC3,6), MSDC_PAD_TDSEL(MSDC3),0x0A);
			sdr_set_field(MSDC_CTRL(MSDC3,6), MSDC_PAD_RDSEL(MSDC3),0x0);
            break;
#endif
        default:
            break;
    }
#endif
}

/* pull up means that host driver the line to HIGH
 * pull down means that host driver the line to LOW */
static void msdc_pin_set(struct msdc_host *host, MSDC_PIN_STATE mode)
{
	u32 id;
	
	MSDC_ASSERT(host);
	MSDC_ASSERT(host->id < HOST_MAX_NUM);
	void __iomem *gpiobase = host->gpiobase;
	MSDC_ASSERT(gpiobase);
	
#ifndef FPGA_PLATFORM
	id=host->id;

	switch(id){
#ifdef CFG_DEV_MSDC0
		case 0:
            sdr_set_field(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CLK_R0R1PUPD,  MSDC_PST_PD_50KOHM);
            sdr_set_field(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CMD_R0R1PUPD,  mode);
            sdr_set_field(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT0_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT1_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT2_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC0, 3), BIT_MSDC_DAT3_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT4_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT5_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT6_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC0, 4), BIT_MSDC_DAT7_R0R1PUPD, mode);
			break;
#endif
#ifdef CFG_DEV_MSDC1
		case 1:
            sdr_set_field(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CLK_R0R1PUPD,  MSDC_PST_PD_50KOHM);
            sdr_set_field(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CMD_R0R1PUPD,  mode);
            sdr_set_field(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT0_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT1_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT2_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC1, 3), BIT_MSDC_DAT3_R0R1PUPD, mode); 
			break;
#endif
#ifdef CFG_DEV_MSDC2
		case 2:
            sdr_set_field(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CLK_R0R1PUPD,  MSDC_PST_PD_50KOHM);
            sdr_set_field(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CMD_R0R1PUPD,  mode);
            sdr_set_field(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT0_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT1_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT2_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC2, 3), BIT_MSDC_DAT3_R0R1PUPD, mode);
			break;
#endif
#ifdef CFG_DEV_MSDC3
        case 3:
            sdr_set_field(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CLK_R0R1PUPD,  MSDC_PST_PD_50KOHM);
            sdr_set_field(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CMD_R0R1PUPD,  mode);
            sdr_set_field(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT0_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT1_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT2_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC3, 3), BIT_MSDC_DAT3_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT4_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT5_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT6_R0R1PUPD, mode);
            sdr_set_field(MSDC_CTRL(MSDC3, 4), BIT_MSDC_DAT7_R0R1PUPD, mode);
            //sdr_set_field(MSDC_CTRL(MSDC3, 5), BIT_MSDC_DSL_R0R1PUPD,  mode);
            break;		
#endif
         default:
			break;
		}
#endif	
}

void msdc_set_smt(struct msdc_host *host,int smt)
{
    u32 id;

    MSDC_ASSERT(host);
	MSDC_ASSERT(host->id < HOST_MAX_NUM);
    void __iomem *gpiobase = host->gpiobase;
    MSDC_ASSERT(gpiobase);

#ifndef FPGA_PLATFORM
	id=host->id;
	switch(id){
#ifdef CFG_DEV_MSDC0
		case 0:
            sdr_set_field(MSDC_CTRL(MSDC0, 1), MSDC_CMD_SMT(MSDC0),      smt);
            sdr_set_field(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0,DAT0), smt);
            sdr_set_field(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0,DAT1), smt);
            sdr_set_field(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0,DAT2), smt);
            sdr_set_field(MSDC_CTRL(MSDC0, 3), MSDC_DAT_SMT(MSDC0,DAT3), smt);
            sdr_set_field(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0,DAT4), smt);
            sdr_set_field(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0,DAT5), smt);
            sdr_set_field(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0,DAT6), smt);
            sdr_set_field(MSDC_CTRL(MSDC0, 4), MSDC_DAT_SMT(MSDC0,DAT7), smt);
			break;
#endif
#ifdef CFG_DEV_MSDC1
		case 1:
            sdr_set_field(MSDC_CTRL(MSDC1, 1), MSDC_CMD_SMT(MSDC1),      smt);
            sdr_set_field(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1,DAT0), smt);
            sdr_set_field(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1,DAT1), smt);
            sdr_set_field(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1,DAT2), smt);
            sdr_set_field(MSDC_CTRL(MSDC1, 3), MSDC_DAT_SMT(MSDC1,DAT3), smt);
			break;
#endif
#ifdef CFG_DEV_MSDC2
		case 2:
            sdr_set_field(MSDC_CTRL(MSDC2, 1), MSDC_CMD_SMT(MSDC2),      smt);
            sdr_set_field(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2,DAT0), smt);
            sdr_set_field(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2,DAT1), smt);
            sdr_set_field(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2,DAT2), smt);
            sdr_set_field(MSDC_CTRL(MSDC2, 3), MSDC_DAT_SMT(MSDC2,DAT3), smt);
			break;
#endif
#ifdef CFG_DEV_MSDC3
		case 3:
            sdr_set_field(MSDC_CTRL(MSDC3, 1), MSDC_CMD_SMT(MSDC3),      smt);
            sdr_set_field(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3,DAT0), smt);
            sdr_set_field(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3,DAT1), smt);
            sdr_set_field(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3,DAT2), smt);
            sdr_set_field(MSDC_CTRL(MSDC3, 3), MSDC_DAT_SMT(MSDC3,DAT3), smt);
            sdr_set_field(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3,DAT4), smt);
            sdr_set_field(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3,DAT5), smt);
            sdr_set_field(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3,DAT6), smt);
            sdr_set_field(MSDC_CTRL(MSDC3, 4), MSDC_DAT_SMT(MSDC3,DAT7), smt);
            sdr_set_field(MSDC_CTRL(MSDC3, 5), MSDC_DSL_SMT(MSDC3),   smt);
            break;	
#endif			
		default:
			break;
		}
#endif	
}

/* host can modify from 0-7 */
void msdc_set_driving(struct msdc_host *host, struct msdc_hw *hw, bool sd_18)
{
    u32 id;
	u32 clkdrv, cmddrv, datdrv;
	void __iomem *gpiobase;

	MSDC_ASSERT(host);
	MSDC_ASSERT(host->id < MSDC_MAX_NUM);
	MSDC_ASSERT(hw);
    MSDC_ASSERT(host->gpiobase);

#ifndef FPGA_PLATFORM	
    gpiobase = host->gpiobase;	
	id=host->id;

	if(host && hw){
		clkdrv = (sd_18) ? hw->clk_drv_sd_18 : hw->clk_drv;
		cmddrv = (sd_18) ? hw->cmd_drv_sd_18 : hw->cmd_drv;
		datdrv = (sd_18) ? hw->dat_drv_sd_18 : hw->dat_drv;
		switch(host->id){
#ifdef CFG_DEV_MSDC0
			case 0:
	            sdr_set_field(MSDC_CTRL(MSDC0, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            sdr_set_field(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            sdr_set_field(MSDC_CTRL(MSDC0, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
#endif				
#ifdef CFG_DEV_MSDC1
			case 1:
	            sdr_set_field(MSDC_CTRL(MSDC1, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            sdr_set_field(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            sdr_set_field(MSDC_CTRL(MSDC1, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
#endif
#ifdef CFG_DEV_MSDC2
			case 2:
	            sdr_set_field(MSDC_CTRL(MSDC2, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            sdr_set_field(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            sdr_set_field(MSDC_CTRL(MSDC2, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
#endif
#ifdef CFG_DEV_MSDC3				
			case 3:
	            sdr_set_field(MSDC_CTRL(MSDC3, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            sdr_set_field(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            sdr_set_field(MSDC_CTRL(MSDC3, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
	            break;
#endif				
			default:
				break;
			}
	}
#endif	
}

void msdc_get_driving(struct msdc_host *host,struct msdc_ioctl* msdc_ctl)
{
    u32 id;
	u32 clkdrv=0, cmddrv=0, datdrv=0;
	void __iomem *gpiobase;

	MSDC_ASSERT(host);
	MSDC_ASSERT(host->id < MSDC_MAX_NUM);
	MSDC_ASSERT(msdc_ctl);
    MSDC_ASSERT(host->gpiobase);

#ifndef FPGA_PLATFORM	
    id = host->id;
    gpiobase = host->gpiobase;		
    msdc_ctl->rst_pu_driving = 0;
    msdc_ctl->ds_pu_driving  = 0;
	if(host && msdc_ctl){
		switch(id){
#ifdef CFG_DEV_MSDC0			
			case 0:
	            sdr_get_field(MSDC_CTRL(MSDC0, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            sdr_get_field(MSDC_CTRL(MSDC0, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            sdr_get_field(MSDC_CTRL(MSDC0, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
#endif				
#ifdef CFG_DEV_MSDC1
			case 1:
	            sdr_get_field(MSDC_CTRL(MSDC1, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            sdr_get_field(MSDC_CTRL(MSDC1, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            sdr_get_field(MSDC_CTRL(MSDC1, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
#endif				
#ifdef CFG_DEV_MSDC2				
			case 2:
	            sdr_get_field(MSDC_CTRL(MSDC2, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            sdr_get_field(MSDC_CTRL(MSDC2, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            sdr_get_field(MSDC_CTRL(MSDC2, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
				break;
#endif				
#ifdef CFG_DEV_MSDC3				
			case 3:
	            sdr_get_field(MSDC_CTRL(MSDC3, 0), BIT_MSDC_CLK_E8E4E2, clkdrv);
	            sdr_get_field(MSDC_CTRL(MSDC3, 1), BIT_MSDC_CMD_E8E4E2, cmddrv);
	            sdr_get_field(MSDC_CTRL(MSDC3, 2), BIT_MSDC_DAT_E8E4E2, datdrv);
	            break;
#endif				
			default:
				break;
		}

		msdc_ctl->clk_pu_driving = clkdrv;
		msdc_ctl->cmd_pu_driving = cmddrv;
		msdc_ctl->dat_pu_driving = datdrv;
	}
#endif
}
EXPORT_SYMBOL(msdc_get_driving);

#ifndef FPGA_PLATFORM
/* make sure the pad is msdc mode */
void msdc_set_pad_init(struct msdc_host *host)
{
	u32 id;
	u32 clkdrv=0, cmddrv=0, datdrv=0;
	void __iomem *gpiobase;
	
	MSDC_ASSERT(host);
	MSDC_ASSERT(host->id < MSDC_MAX_NUM);
	MSDC_ASSERT(host->gpiobase);
	
#ifndef FPGA_PLATFORM	
	gpiobase = host->gpiobase;	
    id = host->id;
    switch (id) {
#ifdef CFG_DEV_MSDC0			
        case 0:
			/* msdc0 already init in preloader/LK. Therefore, the following code can be commented out. */
			/*
			* set pull enable. cmd/dat pull resistor to 10K for emmc 1.8v. clk set 50K.
			*/
			
			 /*
			 * set pull_sel cmd/dat/rst. 
			 * (designer comment: when rstb switch to msdc mode, need gpio pull up to drive high)  
			 * set msdc mode. (MC0_DAT0~7, MC0_CMD, MC0_CLK, MC0_RST, MC0_DSL)
			 */
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_CLK(MSDC0)),	       BIT_GPIOMASK(PIN4_CLK(MSDC0)), 	        1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_CLK(MSDC0)), 	   BIT_GPIOMASK(PIN4_CLK(MSDC0)), 	        0);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC0)),	       BIT_MODE_GPIOMASK(PIN4_CLK(MSDC0)),      BV_GPIO_MSDC_MODE);

			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_CMD(MSDC0)),	       BIT_GPIOMASK(PIN4_CMD(MSDC0)), 	        1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_CMD(MSDC0)), 	   BIT_GPIOMASK(PIN4_CMD(MSDC0)), 	        1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC0)),	       BIT_MODE_GPIOMASK(PIN4_CMD(MSDC0)),      BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_RSTB(MSDC0)), 	   BIT_GPIOMASK(PIN4_RSTB(MSDC0)),	        1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_RSTB(MSDC0)),	   BIT_GPIOMASK(PIN4_RSTB(MSDC0)),	        1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_RSTB(MSDC0)),	       BIT_MODE_GPIOMASK(PIN4_RSTB(MSDC0)),     BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC0,DAT0)),  BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT0)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC0,DAT0)), BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT0)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT0)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT0)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC0,DAT1)),  BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT1)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC0,DAT1)), BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT1)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT1)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT1)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC0,DAT2)),  BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT2)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC0,DAT2)), BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT2)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT2)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT2)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC0,DAT3)),  BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT3)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC0,DAT3)), BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT3)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT3)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT3)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC0,DAT4)),  BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT4)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC0,DAT4)), BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT4)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT4)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT4)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC0,DAT5)),  BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT5)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC0,DAT5)), BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT5)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT5)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT5)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC0,DAT6)),  BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT6)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC0,DAT6)), BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT6)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT6)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT6)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC0,DAT7)),  BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT7)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC0,DAT7)), BIT_GPIOMASK(PIN4_DAT(MSDC0,DAT7)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC0,DAT7)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC0,DAT7)),	BV_GPIO_MSDC_MODE);
            break;
#endif
#ifdef CFG_DEV_MSDC1			
        case 1:
			/* msdc0 already init in preloader/LK. Therefore, the following code can be commented out. */
			/*
			* set pull enable. cmd/dat pull resistor to 10K for emmc 1.8v. clk set 50K.
			*/
			
			 /*
			 * set pull_sel cmd/dat/rst. 
			 * (designer comment: when rstb switch to msdc mode, need gpio pull up to drive high)  
			 * set msdc mode. (MC0_DAT0~7, MC0_CMD, MC0_CLK, MC0_RST, MC0_DSL)
			 */
			 if(host && host->hw && host->hw->flags & MSDC_CD_PIN_EN) {
				sdr_set_field(GPIO_DIR_PIN2REG(PIN4_INS(MSDC1)), 	       BIT_GPIOMASK(PIN4_INS(MSDC1)),		0);
				sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_INS(MSDC1)), 	   BIT_GPIOMASK(PIN4_INS(MSDC1)),		1);
				sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_INS(MSDC1)),	   BIT_GPIOMASK(PIN4_INS(MSDC1)),		1);
				sdr_set_field(GPIO_MODE_PIN2REG(PIN4_INS(MSDC1)), 	       BIT_MODE_GPIOMASK(PIN4_INS(MSDC1)),	0);

	            MSDC_TRC_PRINT(MSDC_INFO,"[MSDC1]@GPIO%u:EINT%u confige as CD Pin\n",PIN4_INS(MSDC1), EINT4_INS(MSDC1));
				if(46 == PIN4_INS(MSDC1)) {
				   sdr_set_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(14), BV_EINT_PUR0R1_50KOHM);
				}
				else if(47 == PIN4_INS(MSDC1)) {
				   sdr_set_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(15), BV_EINT_PUR0R1_50KOHM);
				}
				else if(48 == PIN4_INS(MSDC1)) {
				   sdr_set_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(16), BV_EINT_PUR0R1_50KOHM);
				}
				else if(49 == PIN4_INS(MSDC1)) {
				   sdr_set_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(17), BV_EINT_PUR0R1_50KOHM);
				}
				else if(142 == PIN4_INS(MSDC1)) {
				   sdr_set_field(EINT_CTRL(1), BIT_EINT_PUPDR1R0(21), BV_EINT_PUR0R1_50KOHM);
				}
			}
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_CLK(MSDC1)),	       BIT_GPIOMASK(PIN4_CLK(MSDC1)), 	        1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_CLK(MSDC1)), 	   BIT_GPIOMASK(PIN4_CLK(MSDC1)), 	        0);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC1)),	       BIT_MODE_GPIOMASK(PIN4_CLK(MSDC1)),      BV_GPIO_MSDC_MODE);
			

			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_CMD(MSDC1)),	       BIT_GPIOMASK(PIN4_CMD(MSDC1)), 	        1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_CMD(MSDC1)), 	   BIT_GPIOMASK(PIN4_CMD(MSDC1)), 	        1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC1)),	       BIT_MODE_GPIOMASK(PIN4_CMD(MSDC1)),      BV_GPIO_MSDC_MODE);

			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC1,DAT0)),  BIT_GPIOMASK(PIN4_DAT(MSDC1,DAT0)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC1,DAT0)), BIT_GPIOMASK(PIN4_DAT(MSDC1,DAT0)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT0)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT0)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC1,DAT1)),  BIT_GPIOMASK(PIN4_DAT(MSDC1,DAT1)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC1,DAT1)), BIT_GPIOMASK(PIN4_DAT(MSDC1,DAT1)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT1)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT1)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC1,DAT2)),  BIT_GPIOMASK(PIN4_DAT(MSDC1,DAT2)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC1,DAT2)), BIT_GPIOMASK(PIN4_DAT(MSDC1,DAT2)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT2)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT2)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC1,DAT3)),  BIT_GPIOMASK(PIN4_DAT(MSDC1,DAT3)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC1,DAT3)), BIT_GPIOMASK(PIN4_DAT(MSDC1,DAT3)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC1,DAT3)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC1,DAT3)),	BV_GPIO_MSDC_MODE);
            break;
#endif
#ifdef CFG_DEV_MSDC2			
        case 2:
			/* msdc0 already init in preloader/LK. Therefore, the following code can be commented out. */
			/*
			* set pull enable. cmd/dat pull resistor to 10K for emmc 1.8v. clk set 50K.
			*/
			
			 /*
			 * set pull_sel cmd/dat/rst. 
			 * (designer comment: when rstb switch to msdc mode, need gpio pull up to drive high)  
			 * set msdc mode. (MC0_DAT0~7, MC0_CMD, MC0_CLK, MC0_RST, MC0_DSL)
			 */
			if(host && host->hw && host->hw->flags & MSDC_CD_PIN_EN) {
				sdr_set_field(GPIO_DIR_PIN2REG(PIN4_INS(MSDC2)), 	       BIT_GPIOMASK(PIN4_INS(MSDC2)),			0);
				sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_INS(MSDC2)), 	   BIT_GPIOMASK(PIN4_INS(MSDC2)),			1);
				sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_INS(MSDC2)),	   BIT_GPIOMASK(PIN4_INS(MSDC2)),			1);
				sdr_set_field(GPIO_MODE_PIN2REG(PIN4_INS(MSDC2)), 	       BIT_MODE_GPIOMASK(PIN4_INS(MSDC2)),		0);
	            MSDC_TRC_PRINT(MSDC_INFO,"[MSDC2]@GPIO%u:EINT%u confige as CD Pin\n",PIN4_INS(MSDC2), EINT4_INS(MSDC2));
				
				if(46 == PIN4_INS(MSDC2)) {
				   sdr_set_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(14), BV_EINT_PUR0R1_50KOHM);
				}
				else if(47 == PIN4_INS(MSDC2)) {
				   sdr_set_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(15), BV_EINT_PUR0R1_50KOHM);
				}
				else if(48 == PIN4_INS(MSDC2)) {
				   sdr_set_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(16), BV_EINT_PUR0R1_50KOHM);
				}
				else if(49 == PIN4_INS(MSDC2)) {
				   sdr_set_field(EINT_CTRL(0), BIT_EINT_PUPDR1R0(17), BV_EINT_PUR0R1_50KOHM);
				}
				else if(142 == PIN4_INS(MSDC2)) {
				   sdr_set_field(EINT_CTRL(1), BIT_EINT_PUPDR1R0(21), BV_EINT_PUR0R1_50KOHM);
				}
			}
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_CLK(MSDC2)),	       BIT_GPIOMASK(PIN4_CLK(MSDC2)), 	        1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_CLK(MSDC2)), 	   BIT_GPIOMASK(PIN4_CLK(MSDC2)), 	        0);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC2)),	       BIT_MODE_GPIOMASK(PIN4_CLK(MSDC2)),      BV_GPIO_MSDC_MODE);
			

			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_CMD(MSDC2)),	       BIT_GPIOMASK(PIN4_CMD(MSDC2)), 	        1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_CMD(MSDC2)), 	   BIT_GPIOMASK(PIN4_CMD(MSDC2)), 	        1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC2)),	       BIT_MODE_GPIOMASK(PIN4_CMD(MSDC2)),      BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC2,DAT0)),  BIT_GPIOMASK(PIN4_DAT(MSDC2,DAT0)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC2,DAT0)), BIT_GPIOMASK(PIN4_DAT(MSDC2,DAT0)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT0)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT0)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC2,DAT1)),  BIT_GPIOMASK(PIN4_DAT(MSDC2,DAT1)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC2,DAT1)), BIT_GPIOMASK(PIN4_DAT(MSDC2,DAT1)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT1)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT1)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC2,DAT2)),  BIT_GPIOMASK(PIN4_DAT(MSDC2,DAT2)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC2,DAT2)), BIT_GPIOMASK(PIN4_DAT(MSDC2,DAT2)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT2)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT2)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC2,DAT3)),  BIT_GPIOMASK(PIN4_DAT(MSDC2,DAT3)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC2,DAT3)), BIT_GPIOMASK(PIN4_DAT(MSDC2,DAT3)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC2,DAT3)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC2,DAT3)),	BV_GPIO_MSDC_MODE);
            break;
#endif	
#ifdef CFG_DEV_MSDC3
        case 3:
			/* msdc0 already init in preloader/LK. Therefore, the following code can be commented out. */
			/*
			* set pull enable. cmd/dat pull resistor to 10K for emmc 1.8v. clk set 50K.
			*/
			
			 /*
			 * set pull_sel cmd/dat/rst. 
			 * (designer comment: when rstb switch to msdc mode, need gpio pull up to drive high)  
			 * set msdc mode. (MC0_DAT0~7, MC0_CMD, MC0_CLK, MC0_RST, MC0_DSL)
			 */
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_CLK(MSDC3)),	       BIT_GPIOMASK(PIN4_CLK(MSDC3)), 	        1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_CLK(MSDC3)), 	   BIT_GPIOMASK(PIN4_CLK(MSDC3)), 	        0);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_CLK(MSDC3)),	       BIT_MODE_GPIOMASK(PIN4_CLK(MSDC3)),      BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DSL(MSDC3)),	       BIT_GPIOMASK(PIN4_DSL(MSDC3)), 	        1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DSL(MSDC3)), 	   BIT_GPIOMASK(PIN4_DSL(MSDC3)), 	        1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DSL(MSDC3)),	       BIT_MODE_GPIOMASK(PIN4_DSL(MSDC3)),      BV_GPIO_MSDC_MODE);

			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_CMD(MSDC3)),	       BIT_GPIOMASK(PIN4_CMD(MSDC3)), 	        1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_CMD(MSDC3)), 	   BIT_GPIOMASK(PIN4_CMD(MSDC3)), 	        1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_CMD(MSDC3)),	       BIT_MODE_GPIOMASK(PIN4_CMD(MSDC3)),      BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_RSTB(MSDC3)), 	   BIT_GPIOMASK(PIN4_RSTB(MSDC3)),	        1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_RSTB(MSDC3)),	   BIT_GPIOMASK(PIN4_RSTB(MSDC3)),	        1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_RSTB(MSDC3)),	       BIT_MODE_GPIOMASK(PIN4_RSTB(MSDC3)),     BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC3,DAT0)),  BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT0)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC3,DAT0)), BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT0)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT0)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT0)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC3,DAT1)),  BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT1)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC3,DAT1)), BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT1)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT1)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT1)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC3,DAT2)),  BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT2)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC3,DAT2)), BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT2)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT2)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT2)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC3,DAT3)),  BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT3)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC3,DAT3)), BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT3)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT3)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT3)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC3,DAT4)),  BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT4)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC3,DAT4)), BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT4)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT4)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT4)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC3,DAT5)),  BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT5)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC3,DAT5)), BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT5)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT5)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT5)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC3,DAT6)),  BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT6)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC3,DAT6)), BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT6)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT6)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT6)),	BV_GPIO_MSDC_MODE);
			
			sdr_set_field(GPIO_PULLEN_PIN2REG(PIN4_DAT(MSDC3,DAT7)),  BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT7)),      1);
			sdr_set_field(GPIO_PULLSEL_PIN2REG(PIN4_DAT(MSDC3,DAT7)), BIT_GPIOMASK(PIN4_DAT(MSDC3,DAT7)),      1);
			sdr_set_field(GPIO_MODE_PIN2REG(PIN4_DAT(MSDC3,DAT7)),    BIT_MODE_GPIOMASK(PIN4_DAT(MSDC3,DAT7)),	BV_GPIO_MSDC_MODE);
            break;
#endif			
        default:
            MSDC_ERR_PRINT(MSDC_ERROR,"error...msdc_set_pad_init(MSDC%u) out of range!!\n",id);
            break;
    }
#endif	
}
#else
#define msdc_set_pad_init(...)
#endif

static void msdc_emmc_power(struct msdc_host *host,u32 on)
{
    unsigned long tmo = 0;
    void __iomem *base = host->base;
#ifdef MTK_MSDC_BRINGUP_DEBUG
    u32 ldo_en=0, ldo_vol = 0;
#endif

    /* if MMC_CAP_WAIT_WHILE_BUSY not set, mmc core layer will loop for wait sa_timeout */
    if (host->mmc && host->mmc->card && (host->mmc->caps & MMC_CAP_WAIT_WHILE_BUSY) && (on == 0)){
        /* max timeout: 1000ms */
        if ((DIV_ROUND_UP(host->mmc->card->ext_csd.sa_timeout, 10000)) < 1000){
            if (100 == HZ){
                /* one tick equal to 10ms while (HZ == 100) */
                tmo = jiffies + DIV_ROUND_UP(host->mmc->card->ext_csd.sa_timeout, 100000);
            } else if (1000 == HZ) {
                /* one tick equal to 1ms while (HZ == 1000) */
                tmo = jiffies + DIV_ROUND_UP(host->mmc->card->ext_csd.sa_timeout, 10000);
            } else {
                tmo = jiffies + HZ;
            }
        } else {
            tmo = jiffies + HZ;
        }

        while ((sdr_read32(MSDC_PS) & MSDC_PS_DAT0) != MSDC_PS_DAT0) {
            if (time_after(jiffies, tmo)){
                MSDC_TRC_PRINT(MSDC_STACK, "Dat0 keep low before power off, sa_timeout = 0x%x", host->mmc->card->ext_csd.sa_timeout);
                break;
            }
        }
    }

    /* Set to 3.0V - 100mV
     * 4'b0000: 0 mV
     * 4'b0001: -20 mV
     * 4'b0010: -40 mV
     * 4'b0011: -60 mV
     * 4'b0100: -80 mV
     * 4'b0101: -100 mV
     */
    msdc_ldo_power(on, MT6323_POWER_LDO_VEMC_3V3, VOL_3300, &g_msdc0_flash);
    //mt6325_upmu_set_rg_vemc_3v3_cal(0x5);

#ifdef MTK_MSDC_BRINGUP_DEBUG
    pwrap_read( 0x0A18, &ldo_en );
    pwrap_read( 0x0A4A, &ldo_vol );
    MSDC_TRC_PRINT(MSDC_CONFIG, "VEMC_EN[0x0A18][bit1=%s]=0x%x, VEMC_VOL[0x0A4A][bit9=%s]=0x%x\n", (((ldo_en >> 1) & 0x01) ? "enable" : "disable"), ldo_en, (((ldo_vol >> 9) & 0x01) ? "3.3V" : "3.0V"), ldo_vol);
#endif

}

static void msdc_sd_power(struct msdc_host *host,u32 on)
{
#ifdef MTK_MSDC_BRINGUP_DEBUG
    u32 ldo_en=0, ldo_vol = 0;
#endif
    u32 id=host->id;
    switch(id){
        case 1:
            msdc_set_driving(host,host->hw,0);
            msdc_set_rdtdsel(host,0);
#if (defined(CFG_DEV_MSDC0) || defined(CFG_DEV_MSDC3))				
            if (host->hw->flags & MSDC_SD_NEED_POWER) {
                msdc_ldo_power(1, MT6323_POWER_LDO_VMC, VOL_3300,  &g_msdc1_io);
                msdc_ldo_power(1, MT6323_POWER_LDO_VMCH, VOL_3300, &g_msdc1_flash);
            }
            else {
                msdc_ldo_power(on, MT6323_POWER_LDO_VMC, VOL_3300, &g_msdc1_io);
                msdc_ldo_power(on, MT6323_POWER_LDO_VMCH, VOL_3300, &g_msdc1_flash);
            }
#else
            if (host->hw->flags & MSDC_SD_NEED_POWER) {
                msdc_ldo_power(1, MT6323_POWER_LDO_VMC, VOL_3300,  &g_msdc1_io);
			    msdc_ldo_power(1, MT6323_POWER_LDO_VEMC_3V3, VOL_3300, &g_msdc1_flash);
			}
			else {
				msdc_ldo_power(on, MT6323_POWER_LDO_VMC, VOL_3300, &g_msdc1_io);
				msdc_ldo_power(on, MT6323_POWER_LDO_VEMC_3V3, VOL_3300, &g_msdc1_flash);
			}
#endif
            break;
        case 2:
#ifdef CFG_DEV_MSDC2			
            msdc_set_driving(host,host->hw,0);
            msdc_set_rdtdsel(host,0);
            msdc_ldo_power(on, MT6323_POWER_LDO_VMC, VOL_3300, &g_msdc2_io);
            msdc_ldo_power(on, MT6323_POWER_LDO_VMCH, VOL_3300, &g_msdc2_flash);
#endif			
            break;
        default:
            break;
    }

#ifdef MTK_MSDC_BRINGUP_DEBUG
    pwrap_read( 0x0A16, &ldo_en );
    pwrap_read( 0x0A48, &ldo_vol );
    MSDC_TRC_PRINT(MSDC_CONFIG, "VMC_EN[0x0A16][bit1=%s]=0x%x, VMC_VOL[0x0A48][bit1=%s]=0x%x\n", (((ldo_en >> 1) & 0x01) ? "enable" : "disable"), ldo_en, (((ldo_vol >> 1) & 0x01) ? "3.3V" : "1.8V"), ldo_vol);
    pwrap_read( 0x0A14, &ldo_en );
    pwrap_read( 0x0A48, &ldo_vol );
    MSDC_TRC_PRINT(MSDC_CONFIG, "VMCH_EN[0x0A14][bit1=%s]=0x%x, VMCH_VOL[0x0A48][bit9=%s]=0x%x\n", (((ldo_en >> 1) & 0x01) ? "enable" : "disable"), ldo_en, (((ldo_vol >> 9) & 0x01) ? "3.3V" : "3.0V"), ldo_vol);
#endif
}

static void msdc_sd_power_switch(struct msdc_host *host,u32 on)
{
    u32 id=host->id;
    switch(id){
        case 1:
            msdc_ldo_power(on, MT6323_POWER_LDO_VMC, VOL_1800, &g_msdc1_io);
            msdc_set_rdtdsel(host,1);
            msdc_set_driving(host,host->hw,1);
            break;
        case 2:
            msdc_ldo_power(on, MT6323_POWER_LDO_VMC, VOL_1800, &g_msdc2_io);
            msdc_set_rdtdsel(host,1);
            msdc_set_driving(host,host->hw,1);
            break;
        default:
            break;
    }
}

static void msdc_sdio_power(struct msdc_host *host,u32 on)
{
    u32 id=host->id;
    switch(id){
#if defined(CFG_DEV_MSDC2)
        case 2:
            if(MSDC_VIO18_MC2 == host->power_domain) {
                if(on) {
#if 0 //CHECK ME
                    msdc_ldo_power(on, MT65XX_POWER_LDO_VGP6, VOL_1800, &g_msdc2_io); // Bus & device keeps 1.8v
#endif
                } else {
#if 0 //CHECK ME
                    msdc_ldo_power(on, MT65XX_POWER_LDO_VGP6, VOL_3000, &g_msdc2_io); // Bus & device keeps 3.3v
#endif
                }

            } else if(MSDC_VIO28_MC2 == host->power_domain) {
                //msdc_ldo_power(on, MT65XX_POWER_LDO_VIO28, VOL_2800, &g_msdc2_io); // Bus & device keeps 2.8v
            }
            g_msdc2_flash = g_msdc2_io;
            break;
#endif

#if defined(CFG_DEV_MSDC3)
        case 3:
            break;
#endif

        default:    // if host_id is 3, it uses default 1.8v setting, which always turns on
            break;
    }

}

#define sdc_is_busy()          (sdr_read32(SDC_STS) & SDC_STS_SDCBUSY)
#define sdc_is_cmd_busy()      (sdr_read32(SDC_STS) & SDC_STS_CMDBUSY)

#define sdc_send_cmd(cmd,arg) \
    do { \
        sdr_write32(SDC_ARG, (arg)); \
        sdr_write32(SDC_CMD, (cmd)); \
    } while(0)

// can modify to read h/w register.
//#define is_card_present(h)   ((sdr_read32(MSDC_PS) & MSDC_PS_CDSTS) ? 0 : 1);
#define is_card_present(h)     (((struct msdc_host*)(h))->card_inserted)
#define is_card_sdio(h)        (((struct msdc_host*)(h))->hw->register_pm)

typedef enum{
    cmd_counter = 0,
    read_counter,
    write_counter,
    all_counter,
}TUNE_COUNTER;

static void msdc_reset_pwr_cycle_counter(struct msdc_host *host)
{
    host->power_cycle = 0;
    host->power_cycle_enable = 1;
    host->error_tune_enable = 1;
}

static void msdc_reset_tmo_tune_counter(struct msdc_host *host,TUNE_COUNTER index)
{
    u32 id=host->id;
    MSDC_ASSERT(index <= all_counter);
        switch (index)
        {
            case cmd_counter:
                if(host->rwcmd_time_tune != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "TMO TUNE CMD Times(%d)", host->rwcmd_time_tune);
                host->rwcmd_time_tune = 0;
                break;
            case read_counter  :
                if(host->read_time_tune != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "TMO TUNE READ Times(%d)", host->read_time_tune);
                host->read_time_tune = 0;
                break;
            case write_counter :
                if(host->write_time_tune != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "TMO TUNE WRITE Times(%d)", host->write_time_tune);
                host->write_time_tune = 0;
                break;
            case all_counter   :
                if(host->rwcmd_time_tune != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "TMO TUNE CMD Times(%d)", host->rwcmd_time_tune);
                if(host->read_time_tune != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "TMO TUNE READ Times(%d)", host->read_time_tune);
                if(host->write_time_tune != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "TMO TUNE WRITE Times(%d)", host->write_time_tune);
                host->rwcmd_time_tune = 0;
                host->read_time_tune = 0;
                host->write_time_tune = 0;
                break;
            default :
                break;
    }
}

static void msdc_reset_crc_tune_counter(struct msdc_host *host,TUNE_COUNTER index)
{
    u32 id=host->id;
    MSDC_ASSERT(index <= all_counter);
    switch (index)
    {
        case cmd_counter:
            if(host->t_counter.time_cmd != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "CRC TUNE CMD Times(%d)", host->t_counter.time_cmd);
            host->t_counter.time_cmd = 0;
            break;
        case read_counter  :
            if(host->t_counter.time_read != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "CRC TUNE READ Times(%d)", host->t_counter.time_read);
            host->t_counter.time_read = 0;
            break;
        case write_counter :
            if(host->t_counter.time_write != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "CRC TUNE WRITE Times(%d)", host->t_counter.time_write);
            host->t_counter.time_write = 0;
            break;
        case all_counter   :
            if(host->t_counter.time_cmd != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "CRC TUNE CMD Times(%d)", host->t_counter.time_cmd);
            if(host->t_counter.time_read != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "CRC TUNE READ Times(%d)", host->t_counter.time_read);
            if(host->t_counter.time_write != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "CRC TUNE WRITE Times(%d)", host->t_counter.time_write);
            host->t_counter.time_cmd = 0;
            host->t_counter.time_read = 0;
            host->t_counter.time_write = 0;
#ifdef CONFIG_EMMC_50_FEATURE
            if(host->t_counter.time_hs400 != 0)
                MSDC_DBG_PRINT(MSDC_DEBUG, "TUNE HS400 Times(%d)", host->t_counter.time_hs400);
            host->t_counter.time_hs400 = 0;
#endif
            break;
        default :
            break;
    }
}

extern void mmc_remove_card(struct mmc_card *card);

#if 0
static void msdc_set_bad_card_and_remove(struct msdc_host *host)
{
    int got_polarity = 0;
    unsigned long flags;
    u32 id=host->id;

    MSDC_ASSERT(host);
	
    host->card_inserted = 0;

    if((host->mmc != NULL) && (host->mmc->card)){
        spin_lock_irqsave(&host->remove_bad_card,flags);
        got_polarity = host->sd_cd_polarity;
        host->block_bad_card = 1;

        mmc_card_set_removed(host->mmc->card);
        spin_unlock_irqrestore(&host->remove_bad_card,flags);
        if(((host->hw->flags & MSDC_CD_PIN_EN) && (got_polarity ^ host->hw->cd_level)) || (!(host->hw->flags & MSDC_CD_PIN_EN)))
            tasklet_hi_schedule(&host->card_tasklet);
        else
            mmc_remove_card(host->mmc->card);
		
        MSDC_TRC_PRINT(MSDC_INFO, "Do remove the bad card, block_bad_card=%d, card_inserted=%d", host->block_bad_card, host->card_inserted);
    }
}
#endif

unsigned int msdc_do_command(struct msdc_host   *host,
                                      struct mmc_command *cmd,
                                      int                 tune,
                                      unsigned long       timeout);

static int msdc_tune_cmdrsp(struct msdc_host *host);
static int msdc_get_card_status(struct mmc_host *mmc, struct msdc_host *host, u32 *status);
static void msdc_clksrc_onoff(struct msdc_host *host, u32 on);

//host doesn't need the clock on
void msdc_gate_clock(struct msdc_host* host, int delay)
{
    unsigned long flags;

    spin_lock_irqsave(&host->clk_gate_lock, flags);
    if(host->clk_gate_count > 0)
        host->clk_gate_count--;

    if(delay) {
        mod_timer(&host->timer, jiffies + CLK_TIMEOUT);
        MSDC_TRC_PRINT(MSDC_CONFIG, "[%s]: msdc%d, clk_gate_count=%d, delay=%d", __func__, host->id, host->clk_gate_count, delay);
    } else if(host->clk_gate_count == 0) {
        del_timer(&host->timer);
        msdc_clksrc_onoff(host, 0);
        MSDC_TRC_PRINT(MSDC_CONFIG, "[%s]: msdc%d, successfully gate clock, clk_gate_count=%d, delay=%d", __func__, host->id, host->clk_gate_count, delay);
    } else {
        if(is_card_sdio(host))
            host->error = -EBUSY;
        MSDC_TRC_PRINT(MSDC_CONFIG, "[%s]: msdc%d, failed to gate clock, the clock is still needed by host, clk_gate_count=%d, delay=%d", __func__, host->id, host->clk_gate_count, delay);
    }

    spin_unlock_irqrestore(&host->clk_gate_lock, flags);
}

static void msdc_suspend_clock(struct msdc_host* host)
{
    unsigned long flags;

    spin_lock_irqsave(&host->clk_gate_lock, flags);
    if(host->clk_gate_count == 0) {
        del_timer(&host->timer);
        msdc_clksrc_onoff(host, 0);
        MSDC_TRC_PRINT(MSDC_CONFIG, "[%s]: msdc%d, successfully gate clock, clk_gate_count=%d", __func__, host->id, host->clk_gate_count);
    } else {
        if(is_card_sdio(host))
            host->error = -EBUSY;
        MSDC_TRC_PRINT(MSDC_CONFIG, "[%s]: msdc%d, the clock is still needed by host, clk_gate_count=%d", __func__, host->id, host->clk_gate_count);
    }
    spin_unlock_irqrestore(&host->clk_gate_lock, flags);
}

//host does need the clock on
void msdc_ungate_clock(struct msdc_host* host)
{
    unsigned long flags;
    spin_lock_irqsave(&host->clk_gate_lock, flags);
    host->clk_gate_count++;
    MSDC_TRC_PRINT(MSDC_CONFIG, "[%s]: msdc%d, clk_gate_count=%d", __func__, host->id, host->clk_gate_count);
    if(host->clk_gate_count == 1)
        msdc_clksrc_onoff(host, 1);
    spin_unlock_irqrestore(&host->clk_gate_lock, flags);
}

// do we need sync object or not
void msdc_clk_status(int * status)
{
    int g_clk_gate = 0;
    int i=0;
    unsigned long flags;

    for (i=0; i<HOST_MAX_NUM; i++) {
        if (!mtk_msdc_host[i])
            continue;

      #ifndef FPGA_PLATFORM
        spin_lock_irqsave(&mtk_msdc_host[i]->clk_gate_lock, flags);
        if (mtk_msdc_host[i]->clk_gate_count > 0)
            g_clk_gate |= 1 << msdc_cg_clk_id[i];

        spin_unlock_irqrestore(&mtk_msdc_host[i]->clk_gate_lock, flags);
      #endif
    }

    *status = g_clk_gate;
}

#if 10
static void msdc_dump_card_status(struct msdc_host *host, u32 status)
{
    static char *state[] = {
        "Idle",        /* 0 */
        "Ready",       /* 1 */
        "Ident",       /* 2 */
        "Stby",        /* 3 */
        "Tran",        /* 4 */
        "Data",        /* 5 */
        "Rcv",         /* 6 */
        "Prg",         /* 7 */
        "Dis",         /* 8 */
        "Reserved",    /* 9 */
        "Reserved",    /* 10 */
        "Reserved",    /* 11 */
        "Reserved",    /* 12 */
        "Reserved",    /* 13 */
        "Reserved",    /* 14 */
        "I/O mode",    /* 15 */
    };
	
    if (status & R1_OUT_OF_RANGE)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Out of Range");
    if (status & R1_ADDRESS_ERROR)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Address Error");
    if (status & R1_BLOCK_LEN_ERROR)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Block Len Error");
    if (status & R1_ERASE_SEQ_ERROR)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Erase Seq Error");
    if (status & R1_ERASE_PARAM)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Erase Param");
    if (status & R1_WP_VIOLATION)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] WP Violation");
    if (status & R1_CARD_IS_LOCKED)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Card is Locked");
    if (status & R1_LOCK_UNLOCK_FAILED)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Lock/Unlock Failed");
    if (status & R1_COM_CRC_ERROR)
       MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Command CRC Error");
    if (status & R1_ILLEGAL_COMMAND)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Illegal Command");
    if (status & R1_CARD_ECC_FAILED)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Card ECC Failed");
    if (status & R1_CC_ERROR)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] CC Error");
    if (status & R1_ERROR)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Error");
    if (status & R1_UNDERRUN)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Underrun");
    if (status & R1_OVERRUN)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Overrun");
    if (status & R1_CID_CSD_OVERWRITE)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] CID/CSD Overwrite");
    if (status & R1_WP_ERASE_SKIP)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] WP Eraser Skip");
    if (status & R1_CARD_ECC_DISABLED)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Card ECC Disabled");
    if (status & R1_ERASE_RESET)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Erase Reset");
    if ((status & R1_READY_FOR_DATA) == 0)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Not Ready for Data");
    if (status & R1_SWITCH_ERROR)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] Switch error");
    if (status & R1_APP_CMD)
        MSDC_TRC_PRINT(MSDC_DREG, "[CARD_STATUS] App Command");

    MSDC_TRC_PRINT(MSDC_STACK, "[CARD_STATUS] '%s' State", state[R1_CURRENT_STATE(status)]);
}
#endif

static void msdc_set_timeout(struct msdc_host *host, u32 ns, u32 clks)
{
    void __iomem *base = host->base;
    u32 timeout, clk_ns;
    u32 mode = 0;

    host->timeout_ns = ns;
    host->timeout_clks = clks;
    if(host->sclk == 0){
        timeout = 0;
    } else {
        clk_ns  = 1000000000UL / host->sclk;
        timeout = (ns + clk_ns - 1) / clk_ns + clks;
        timeout = (timeout + (1 << 20) - 1) >> 20; /* in 1048576 sclk cycle unit */
        sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD, mode);
        timeout = mode >= 2 ? timeout * 2 : timeout; //DDR mode will double the clk cycles for data timeout
        timeout = timeout > 1 ? timeout - 1 : 0;
        timeout = timeout > 255 ? 255 : timeout;
    }
    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, timeout);

    MSDC_DBG_PRINT(MSDC_DEBUG, "msdc%d, Set read data timeout: %dns %dclks -> %d x 1048576  cycles, mode:%d, clk_freq=%dKHz\n",
        host->id, ns, clks, timeout + 1, mode, (host->sclk / 1000));
}

/* msdc_eirq_sdio() will be called when EIRQ(for WIFI) */
static void msdc_eirq_sdio(void *data)
{
    struct msdc_host *host = (struct msdc_host *)data;

    MSDC_DBG_PRINT(MSDC_IRQ, "[MSDC%u] SDIO EINT", host->id);
#ifdef SDIO_ERROR_BYPASS
    if(host->sdio_error != -EIO){
#endif
		mmc_signal_sdio_irq(host->mmc);
#ifdef SDIO_ERROR_BYPASS
    }
#endif
}

/* msdc_eirq_cd will not be used!  We not using EINT for card detection. */
static void msdc_eirq_cd(void *data)
{
    struct msdc_host *host = (struct msdc_host *)data;

    MSDC_DBG_PRINT(MSDC_IRQ, "[MSDC%u] CD EINT", host->id);

    tasklet_hi_schedule(&host->card_tasklet);
}

/* detect cd interrupt */

static void msdc_tasklet_card(unsigned long arg)
{
    struct msdc_host *host = (struct msdc_host *)arg;
    struct msdc_hw *hw = host->hw;
    //unsigned long flags;
    //void __iomem *base = host->base;
    u32 inserted;
    //u32 status = 0;

    //spin_lock_irqsave(&host->lock, flags);
    //msdc_ungate_clock(host);

    if (hw->get_cd_status) { // NULL
        inserted = hw->get_cd_status();
    } else {
        //status = sdr_read32(MSDC_PS);
        if(hw->cd_level)
            inserted = (host->sd_cd_polarity == 0) ? 1 : 0;
        else
            inserted = (host->sd_cd_polarity == 0) ? 0 : 1;
    }
    if(host->block_bad_card){
        inserted = 0;
        if(host->mmc->card)
            mmc_card_set_removed(host->mmc->card);
        MSDC_TRC_PRINT(MSDC_INFO, "[MSDC%u] remove bad SD card", host->id);
    } else {
        MSDC_TRC_PRINT(MSDC_INFO, "[MSDC%u] card found<%s>", host->id, inserted ? "inserted" : "removed");
    }

    host->card_inserted = inserted;
    host->mmc->f_max = HOST_MAX_MCLK;
    host->hw->cmd_edge = 0; // new card tuning from 0
    host->hw->rdata_edge = 0;
    host->hw->wdata_edge = 0;
    if(hw->flags & MSDC_UHS1){
        host->mmc->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
                           MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104;// | MMC_CAP_SET_XPC_180;
        host->mmc->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
    }
#ifdef CONFIG_EMMC_50_FEATURE
    if(hw->flags & MSDC_HS400){
        host->mmc->caps2 |= MMC_CAP2_HS400_1_8V_DDR;
    }
#endif
    if(hw->flags & MSDC_DDR)
        host->mmc->caps |= MMC_CAP_UHS_DDR50|MMC_CAP_1_8V_DDR;
    msdc_host_mode[host->id] = host->mmc->caps;
    msdc_host_mode2[host->id] = host->mmc->caps2;
    if((hw->flags & MSDC_CD_PIN_EN) && inserted){
        msdc_reset_pwr_cycle_counter(host);
        msdc_reset_crc_tune_counter(host, all_counter);
        msdc_reset_tmo_tune_counter(host, all_counter);
    }

    MSDC_TRC_PRINT(MSDC_INFO, "[MSDC%u] host->suspend(%d)", host->id,host->suspend);
    if (!host->suspend && host->sd_cd_insert_work) {
        mmc_detect_change(host->mmc, msecs_to_jiffies(200));
    }
    MSDC_TRC_PRINT(MSDC_INFO, "[MSDC%u] insert_workqueue(%d)", host->id,host->sd_cd_insert_work);
}

#ifdef CONFIG_SDIOAUTOK_SUPPORT
extern void autok_claim_host(struct msdc_host *host);
extern void autok_release_host(struct msdc_host *host);
#endif  // CONFIG_SDIOAUTOK_SUPPORT

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT

static int ot_thread_func(void *data)
{
    struct ot_work_t *ot_work = (struct ot_work_t *)data;
    struct msdc_host *host = ot_work->host;
    int    chg_volt = host->ot_work.chg_volt;
    int err = 0;
#ifdef MTK_SDIO30_DETECT_THERMAL
    int cur_temperature = 0;
#endif // MTK_SDIO30_DETECT_THERMAL


    if(atomic_read(&host->ot_work.ot_disable))
	{
		MSDC_DBG_PRINT(MSDC_DEBUG, "[SDIO_OnlineTuning][%s] online tuning is disabled\n", __func__);
		if(chg_volt)
		{
			host->ot_work.chg_volt = 0;
			atomic_set(&host->sdio_stopping, 0);
			complete(&host->ot_work.ot_complete);
			autok_release_host(host);
			return -1;
		}
        goto out;
	}

	if(atomic_read(&host->ot_work.autok_done) == 0)
	{
		MSDC_DBG_PRINT(MSDC_DEBUG, "[SDIO_OnlineTuning][%s] auto-K haven't done\n", __func__);
		if(chg_volt)
		{
			host->ot_work.chg_volt = 0;
			atomic_set(&host->sdio_stopping, 0);
			complete(&host->ot_work.ot_complete);
			autok_release_host(host);
			return -1;
		}
        goto out;
	}

    if(host->mmc->card == NULL)
    {
        MSDC_DBG_PRINT(MSDC_DEBUG, "[SDIO_OnlineTuning][%s] sdio card is not ready\n", __func__);
		if(chg_volt)
		{
		    host->ot_work.chg_volt = 0;
			atomic_set(&host->sdio_stopping, 0);
			complete(&host->ot_work.ot_complete);
			autok_release_host(host);
		}
        return -1;
    }

    if(host->hw->host_function != MSDC_SDIO)
    {
        MSDC_DBG_PRINT(MSDC_DEBUG, "[SDIO_OnlineTuning][%s] Non SDIO device enter msdc_online_tuning!!!! \n", __func__);
		if(chg_volt)
		{
		    host->ot_work.chg_volt = 0;
			atomic_set(&host->sdio_stopping, 0);
			complete(&host->ot_work.ot_complete);
			autok_release_host(host);
		}
        return -1;
    }

    if(host->suspend)
    {
        MSDC_DBG_PRINT(MSDC_DEBUG, "[SDIO_OnlineTuning][%s] MSDC%d Suspended!!!! \n", __func__, host->id);
		if(chg_volt)
		{
			host->ot_work.chg_volt = 0;
			atomic_set(&host->sdio_stopping, 0);
			complete(&host->ot_work.ot_complete);
			autok_release_host(host);
			return -1;
		}
        goto out;
    }

#ifdef MTK_SDIO30_DETECT_THERMAL
    if(chg_volt == 0)
    {
        if(host->pre_temper == -127000)
            host->pre_temper = mtk_thermal_get_temp(MTK_THERMAL_SENSOR_CPU);

        cur_temperature = mtk_thermal_get_temp(MTK_THERMAL_SENSOR_CPU);

        MSDC_DBG_PRINT(MSDC_DEBUG, "[SDIO_OnlineTuning][%s] cur_temperature = %d, host->pre_temper = %d\n", __func__, cur_temperature, host->pre_temper);

        if((cur_temperature != -127000) && (host->pre_temper != -127000) && (abs(cur_temperature-host->pre_temper) > OT_TEMPDIFF_BOUNDRY)) {
            if((err = msdc_online_tuning(host, 1, 0x00B0)) != 0) {
                //printk("[SDIO_OnlineTuning][%s] msdc_online_tuning fail, err = %d\n", __func__, err);
            }
            host->pre_temper = cur_temperature;
        }
    }
    else
#endif // MTK_SDIO30_DETECT_THERMAL
    {
        MSDC_DBG_PRINT(MSDC_DEBUG, "[SDIO_OnlineTuning][%s] change volt\n", __func__);
        if((err = msdc_online_tuning(host, 1, 0x00B0)) != 0) {
            //printk("[SDIO_OnlineTuning][%s] msdc_online_tuning fail, err = %d\n", __func__, err);
        }
        MSDC_DBG_PRINT(MSDC_DEBUG, "[SDIO_OnlineTuning][%s] onine tuning done (change volt)\n", __func__);
        return 0;
    }

out:
#ifdef MTK_SDIO30_DETECT_THERMAL
    if(chg_volt == 0)
        mod_timer(&host->ot_timer, jiffies + OT_PERIOD);
#endif // #ifdef MTK_SDIO30_DETECT_THERMAL
    return 0;
}

static void start_online_tuning(unsigned long data)
{
    struct msdc_host *host = (struct msdc_host *)data;
    struct task_struct *task;

    task = kthread_run(&ot_thread_func,(void *)(&host->ot_work),"online tuning");
    if (IS_ERR(task)) {
        MSDC_DBG_PRINT(MSDC_DEBUG, "[%s][SDIO_TEST_MODE] create thread fail, do online tuning directly\n", __func__);
        ot_thread_func((void *)(&host->ot_work));
    }
}

#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT


int msdc_enable_clock(struct msdc_host *host)
{
    int result = 0;
#ifdef CLKMGR_WORDAROUND
    if(host->id == 0) {
	   result = enable_clock(msdc_cg_clk_id[3], "SD");
	   result = enable_clock(msdc_cg_clk_id[0], "SD");
    }
	else {
	   result = enable_clock(msdc_cg_clk_id[host->id], "SD");
	}
#else 
    result = enable_clock(msdc_cg_clk_id[host->id], "SD");
#endif
    return result;
}

int msdc_disable_clock(struct msdc_host *host)
{
    int result=0;
	
#ifdef CLKMGR_WORDAROUND	
    if(host->id == 0) {
	   result = disable_clock(msdc_cg_clk_id[3], "SD");
	   result = disable_clock(msdc_cg_clk_id[0], "SD");
    }
	else {
	   result = disable_clock(msdc_cg_clk_id[host->id], "SD");
	}
#else 
	result = disable_clock(msdc_cg_clk_id[host->id], "SD");
#endif

    return result;
}
static void msdc_select_clksrc(struct msdc_host *host, u32 clksrc, u32 hclksrc)
{
	u32 id;
	u32 clk_src;
	
	MSDC_ASSERT(host);
	MSDC_ASSERT(host->id < HOST_MAX_NUM);
#if 10	
	int mux_id[] = {MT_MUX_MSDC30_0, MT_MUX_MSDC30_1, MT_MUX_MSDC30_2, MT_MUX_MSDC50_3};
	char name[20];
	id=host->id;

    if ( id == 3) {
	    if ( host->mmc && host->mmc->card && mmc_card_hs400(host->mmc->card)) {
	        /* after the card init flow, if the card support hs400 mode \
	                *  modify the mux channel of the msdc pll */
	        clk_src  = MSDC50_CLKSRC_400MHZ;
	        host->hw->hclk_src = hclksrc;
	        host->hclk         = hclks_msdc50[clk_src];
	        MSDC_TRC_PRINT(MSDC_INIT,"[info][%s] hs400 mode, change pll mux to 400Mhz\n", __func__);
	    }
		else {
	        /* Perloader and LK use 200 is ok, no need change source */
	        host->hw->clk_src  = clk_src = clksrc;
		    host->hw->hclk_src = hclksrc;
	        host->hclk         = hclks_msdc50[clksrc];
	    }
    }
	else {
		/* Perloader and LK use 200 is ok, no need change source */
		host->hw->clk_src  = clk_src = clksrc;
		host->hw->hclk_src = hclksrc;
		host->hclk	       = hclks_msdc30[clksrc];	
	}

     sprintf(name, "MSDC%d", host->id);
     clkmux_sel(mux_id[host->id], clksrc, name);
     if ( id == 3) {
	     clkmux_sel(MT_MUX_MSDC50_3_HCLK, hclksrc, name);
     }	
#else
    void __iomem *topckgenbase = host->topckgenbase;
    MSDC_ASSERT(topckgenbase);

	id=host->id;

    if ( id == 3) {
	    if ( host->mmc && host->mmc->card && mmc_card_hs400(host->mmc->card)) {
	        /* after the card init flow, if the card support hs400 mode \
	                *  modify the mux channel of the msdc pll */
	        clk_src  = MSDC50_CLKSRC_400MHZ;
	        host->hw->hclk_src = hclksrc;
	        host->hclk         = hclks_msdc50[clk_src];
	        MSDC_TRC_PRINT(MSDC_INIT,"[info][%s] hs400 mode, change pll mux to 400Mhz\n", __func__);
	    }
		else {
	        /* Perloader and LK use 200 is ok, no need change source */
	        host->hw->clk_src  = clk_src = clksrc;
		    host->hw->hclk_src = hclksrc;
	        host->hclk         = hclks_msdc50[clksrc];
	    }
    }
	else {
		/* Perloader and LK use 200 is ok, no need change source */
		host->hw->clk_src  = clk_src = clksrc;
		host->hw->hclk_src = hclksrc;
		host->hclk	       = hclks_msdc30[clksrc];	
	}

#if  0   /*ndef FPGA_PLATFORM*/
     if ( id == 3) {
		sdr_set_field(TOPCKGEN_CTRL(CLKCFG4) ,MSDC_CKGENHCLK_SEL(MSDC3,CLKCFG4), hclksrc); 	
		sdr_set_field(TOPCKGEN_CTRL(CLKCFGUPDATE),MSDC_CKGENHCLK_UPDATE(MSDC3), 1);
		
		sdr_set_field(TOPCKGEN_CTRL(CLKCFG4_CLR), MSDC_CKGEN_SEL(MSDC3,CLKCFG4), clk_src);		
		sdr_set_field(TOPCKGEN_CTRL(CLKCFGUPDATE),MSDC_CKGEN_UPDATE(MSDC3), 1);
	 }
	 else if ( id == 2) {
	   sdr_set_field(TOPCKGEN_CTRL(CLKCFG3), MSDC_CKGEN_SEL(MSDC2,CLKCFG3), clk_src); 	  
	   sdr_set_field(TOPCKGEN_CTRL(CLKCFGUPDATE),MSDC_CKGEN_UPDATE(MSDC2), 1);
	 }
	 else if ( id == 1) {
	   sdr_set_field(TOPCKGEN_CTRL(CLKCFG3), MSDC_CKGEN_SEL(MSDC1,CLKCFG3), clk_src); 	  
	   sdr_set_field(TOPCKGEN_CTRL(CLKCFGUPDATE),MSDC_CKGEN_UPDATE(MSDC1), 1);
	 }
	 else {
	   sdr_set_field(TOPCKGEN_CTRL(CLKCFG3), MSDC_CKGEN_SEL(MSDC0,CLKCFG3), clk_src); 	  
	   sdr_set_field(TOPCKGEN_CTRL(CLKCFGUPDATE),MSDC_CKGEN_UPDATE(MSDC0), 1);
	    }
#endif	
#endif

    MSDC_TRC_PRINT(MSDC_INIT,"[info][%s] pll_clk %u (%uMHz), pll_hclk %u\n", __func__, clk_src, host->mclk/1000000, hclksrc);
}

void msdc_sdio_set_long_timing_delay_by_freq(struct msdc_host *host, u32 clock)
{
#ifdef CONFIG_SDIOAUTOK_SUPPORT
    void __iomem *base = host->base;

    if(clock >= 200000000){
        sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->hw->wdatcrctactr_sdr200);
        sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->hw->cmdrtactr_sdr200);
        sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,  host->hw->intdatlatcksel_sdr200);
        host->saved_para.cmd_resp_ta_cntr = host->hw->cmdrtactr_sdr200;
        host->saved_para.wrdat_crc_ta_cntr = host->hw->wdatcrctactr_sdr200;
        host->saved_para.int_dat_latch_ck_sel = host->hw->intdatlatcksel_sdr200;
    } else {
        sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->hw->wdatcrctactr_sdr50);
        sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->hw->cmdrtactr_sdr50);
        sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,  host->hw->intdatlatcksel_sdr50);
        host->saved_para.cmd_resp_ta_cntr = host->hw->cmdrtactr_sdr50;
        host->saved_para.wrdat_crc_ta_cntr = host->hw->wdatcrctactr_sdr50;
        host->saved_para.int_dat_latch_ck_sel = host->hw->intdatlatcksel_sdr50;
    }
#else
    return;
#endif
}

volatile int sdio_autok_processed = 0;

static void msdc_set_mclk(struct msdc_host *host, unsigned int state, u32 hz)
{
    //struct msdc_hw *hw = host->hw; 
    void __iomem *base = host->base;
    u32 mode;
    u32 flags;
    u32 div;
    u32 sclk;
    u32 hclk = host->hclk;
    u32 hs400_src = 0;
	u32 u4buswidth;
    //u8  clksrc = hw->clk_src;
    

    if (hz > host->mmc->f_max) {
		hz = host->mmc->f_max;
    }
	
	if (hz < host->mmc->f_min  ) {
		hz = host->mmc->f_min;
	}

    //MSDC_ASSERT( (host->mmc->f_min <= hz) && (hz <= host->mmc->f_max));
    if (!hz) { // set mmc system clock to 0
        MSDC_DBG_PRINT(MSDC_DEBUG, "msdc%d -> set mclk to 0",host->id);
        if (is_card_sdio(host) || (host->hw->flags & MSDC_SDIO_IRQ)) {
            host->saved_para.hz = hz;
#ifdef SDIO_ERROR_BYPASS
            host->sdio_error = 0;
#endif
		}
        host->mclk = 0;
        msdc_reset_hw(host->id);
        return;
    }
    if(host->hw->host_function == MSDC_SDIO && hz >= 100*1000*1000 && sdio_autok_processed == 0)
    {
        hz = 50*1000*1000;
        msdc_sdio_set_long_timing_delay_by_freq(host, hz);
    }
    MSDC_DBG_PRINT(MSDC_DEBUG, "[%s] hz = %d\n", __func__, hz);

    msdc_irq_save(flags);

    if(state ==  MSDC_STATE_HS400){
        mode = 0x3; /* HS400 mode */
        hs400_src = 1;/* if 0, src=800Mhz, then div as DDR50; if 1, src=400Mhz, then no need of div */
        div = 0; /* src=400Mhz, div is ingroe */
        sclk = hclk >> 1;
        #if 0
        if (hz >= (hclk >> 2)) {
            div  = 0;         /* mean div = 1/4 */
            sclk = hclk >> 2; /* sclk = clk / 4 */
        } else {
            div  = (hclk + ((hz << 2) - 1)) / (hz << 2);
            sclk = (hclk >> 2) / div;
            div  = (div >> 1);
        }
        #endif
    }else if(state == MSDC_STATE_DDR){
        mode = 0x2; /* ddr mode and use divisor */
        if (hz >= (hclk >> 2)) {
            div  = 0;         /* mean div = 1/4 */
            sclk = hclk >> 2; /* sclk = clk / 4 */
        } else {
            div  = (hclk + ((hz << 2) - 1)) / (hz << 2);
            sclk = (hclk >> 2) / div;
            div  = (div >> 1);
        }
    } else if (hz >= hclk) {
#ifdef FPGA_PLATFORM
        mode = 0x0; //FPGA doesn't support no divisor
#else
        mode = 0x1; /* no divisor */
#endif
        div  = 0;
        sclk = hclk;
    } else {
        mode = 0x0; /* use divisor */
        if (hz >= (hclk >> 1)) {
            div  = 0;         /* mean div = 1/2 */
            sclk = hclk >> 1; /* sclk = clk / 2 */
        } else {
            div  = (hclk + ((hz << 2) - 1)) / (hz << 2);
            sclk = (hclk >> 2) / div;
        }
    }

    msdc_clk_stable(host,mode, div, hs400_src);

    host->sclk = sclk;
    host->mclk = hz;
    host->state = state;
#if 0
    if (host->sclk > 100000000) {
        sdr_clr_bits(MSDC_PATCH_BIT0, CKGEN_RX_SDClKO_SEL);
    } else {
        sdr_set_bits(MSDC_PATCH_BIT0, CKGEN_RX_SDClKO_SEL);
    }
#endif

    msdc_set_timeout(host, host->timeout_ns, host->timeout_clks); // need because clk changed.

    if(mode == 0x3){
        msdc_set_smpl(host, 1, host->hw->cmd_edge, TYPE_CMD_RESP_EDGE, NULL);
        msdc_set_smpl(host, 1, host->hw->rdata_edge, TYPE_READ_DATA_EDGE, NULL);
        msdc_set_smpl(host, 1, host->hw->wdata_edge, TYPE_WRITE_CRC_EDGE, NULL);
		sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,0);		
		sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,1);		
    }else {
        msdc_set_smpl(host, 0, host->hw->cmd_edge, TYPE_CMD_RESP_EDGE, NULL);
        msdc_set_smpl(host, 0, host->hw->rdata_edge, TYPE_READ_DATA_EDGE, NULL);
        msdc_set_smpl(host, 0, host->hw->wdata_edge, TYPE_WRITE_CRC_EDGE, NULL);
#ifdef MSDC_USE_PATCH_BIT2_TURNING_WITH_ASYNC
		sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,1);		
#else
		sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,0);		
#endif
		sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL,0);
    }

	sdr_get_field(SDC_CFG,SDC_CFG_BUSWIDTH,u4buswidth);
    MSDC_TRC_PRINT(MSDC_INFO, "msdc%d -> !!! Set<%dKHz> Source<%dKHz> -> sclk<%dKHz> state<%d> mode<%d> div<%d> hs400_src<%d> buswidth<%s>" ,
           host->id, hz/1000, hclk/1000, sclk/1000, state, mode, div, hs400_src,\
           (u4buswidth == 0) ? "1-bit" : (u4buswidth == 1)? "4-bits" : (u4buswidth == 2)? "8-bits" : "undefined");

    msdc_irq_restore(flags);
}
extern int mmc_sd_power_cycle(struct mmc_host *host, u32 ocr, struct mmc_card *card);

/* 0 means pass */
static u32 msdc_power_tuning(struct msdc_host *host)
{
    struct mmc_host *mmc = host->mmc;
    struct mmc_card *card;
    struct mmc_request *mrq;
    u32 power_cycle = 0;
    int read_timeout_tune = 0;
    int write_timeout_tune = 0;
    u32 rwcmd_timeout_tune = 0;
    u32 read_timeout_tune_uhs104 = 0;
    u32 write_timeout_tune_uhs104 = 0;
    u32 sw_timeout = 0;
    u32 ret = 1;
    u32 host_err = 0;
    void __iomem *base = host->base;
	if (!mmc)
		return 1;

    card = mmc->card;
    if (card == NULL) {
        MSDC_TRC_PRINT(MSDC_WARN, "mmc->card is NULL");
        return 1;
    }

    // eMMC first
#ifdef CONFIG_MTK_EMMC_SUPPORT
    if (mmc_card_mmc(card) && (host->hw->host_function == MSDC_EMMC)) {
        return 1;
    }
#endif

    if ( !host->error_tune_enable ) {
        return 1;
    }

    if((host->sd_30_busy > 0) && (host->sd_30_busy <= MSDC_MAX_POWER_CYCLE)){
        host->power_cycle_enable = 1;
    }
    if (mmc_card_sd(card) && (host->hw->host_function == MSDC_SD)) {
        if((host->power_cycle < MSDC_MAX_POWER_CYCLE) && (host->power_cycle_enable))
        {
            // power cycle
            MSDC_DBG_PRINT(MSDC_DEBUG, "the %d time, Power cycle start", host->power_cycle);
            spin_unlock(&host->lock);
#ifdef FPGA_PLATFORM
            hwPowerDown_fpga(host->id);
#else
            if(host->power_control)
                host->power_control(host,0);
            else
                MSDC_ERR_PRINT(MSDC_ERROR, "No power control callback. Please check host_function<0x%lx>",host->hw->host_function);
#endif
            mdelay(10);
#ifdef FPGA_PLATFORM
            hwPowerOn_fpga(host->id);
#else
            if(host->power_control)
                host->power_control(host,1);
            else
                MSDC_ERR_PRINT(MSDC_ERROR, "No power control callback. Please check host_function<0x%lx>",host->hw->host_function);
#endif


            spin_lock(&host->lock);
            sdr_get_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, host->hw->ddlsel);
            sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, host->hw->cmd_edge);    // save the para
            sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, host->hw->rdata_edge);
            sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, host->hw->wdata_edge);
            host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
            host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);
            host->saved_para.ddly1 = sdr_read32(MSDC_DAT_RDDLY1);
            sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->saved_para.cmd_resp_ta_cntr);
            sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
            sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, host->saved_para.write_busy_margin);
            sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA, host->saved_para.write_crc_margin);

            if((host->sclk > 100000000) && (host->power_cycle >= 1))
                mmc->caps &= ~MMC_CAP_UHS_SDR104;
            if(((host->sclk <= 100000000) && ((host->sclk > 50000000) || (host->state == MSDC_STATE_DDR))) && (host->power_cycle >= 1)){
                mmc->caps &= ~(MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 | MMC_CAP_UHS_DDR50);
            }

            msdc_host_mode[host->id] = mmc->caps;
            msdc_host_mode2[host->id] = mmc->caps2;

            // clock should set to 260K
            mmc->ios.clock = HOST_MIN_MCLK;
            mmc->ios.bus_width = MMC_BUS_WIDTH_1;
            mmc->ios.timing = MMC_TIMING_LEGACY;
            msdc_set_mclk(host, MSDC_STATE_DEFAULT, HOST_MIN_MCLK);

            //zone_temp = sd_debug_zone[1];
            //sd_debug_zone[1] |= (DBG_EVT_NRW | DBG_EVT_RW);

            // re-init the card!
            mrq = host->mrq;
            host->mrq = NULL;
            power_cycle = host->power_cycle;
            host->power_cycle = MSDC_MAX_POWER_CYCLE;
            read_timeout_tune = host->read_time_tune;
            write_timeout_tune = host->write_time_tune;
            rwcmd_timeout_tune = host->rwcmd_time_tune;
            read_timeout_tune_uhs104 = host->read_timeout_uhs104;
            write_timeout_tune_uhs104 = host->write_timeout_uhs104;
            sw_timeout = host->sw_timeout;
            host_err = host->error;
            spin_unlock(&host->lock);
            ret = mmc_sd_power_cycle(mmc, mmc->ocr, card);
            spin_lock(&host->lock);
            host->mrq = mrq;
            host->power_cycle = power_cycle;
            host->read_time_tune = read_timeout_tune;
            host->write_time_tune = write_timeout_tune;
            host->rwcmd_time_tune = rwcmd_timeout_tune;
            if(host->sclk > 100000000){
                host->write_timeout_uhs104 = write_timeout_tune_uhs104;
            }
            else{
                host->read_timeout_uhs104 = 0;
                host->write_timeout_uhs104 = 0;
            }
            host->sw_timeout = sw_timeout;
            host->error = host_err;
            if(!ret)
                host->power_cycle_enable = 0;
            MSDC_DBG_PRINT(MSDC_DEBUG, "the %d time, Power cycle Done, host->error(0x%x), ret(%d)", host->power_cycle,host->error, ret);
            (host->power_cycle)++;
        }else if (host->power_cycle == MSDC_MAX_POWER_CYCLE){
            MSDC_DBG_PRINT(MSDC_DEBUG, "the %d time, exceed the max power cycle time %d, go to remove the bad card, power_cycle_enable=%d", host->power_cycle, MSDC_MAX_POWER_CYCLE, host->power_cycle_enable);
            spin_unlock(&host->lock);
#if 0
            ERR_MSG("the %d time, exceed the max power cycle time %d, go to remove the bad card, power_cycle_enable=%d", host->power_cycle, MSDC_MAX_POWER_CYCLE, host->power_cycle_enable);
            msdc_set_bad_card_and_remove(host);
#else
            if ( host->error_tune_enable ) {
                ERR_MSG("disable error tune flow for bad SD card");
                host->error_tune_enable = 0;
            }
#endif
            spin_lock(&host->lock);
        }
    }

    return ret;
}


static void msdc_send_stop(struct msdc_host *host)
{
    struct mmc_command stop = {0};
    struct mmc_request mrq = {0};
    u32 err = -1;

    stop.opcode = MMC_STOP_TRANSMISSION;
    stop.arg = 0;
    stop.flags = MMC_RSP_R1B | MMC_CMD_AC;

	mrq.cmd = &stop;
	stop.mrq = &mrq;
    stop.data = NULL;

    err = msdc_do_command(host, &stop, 0, CMD_TIMEOUT);
}

extern void mmc_detach_bus(struct mmc_host *host);
extern void mmc_power_off(struct mmc_host *host);
#if 0
static void msdc_remove_card(struct work_struct *work)
{
    struct msdc_host *host = container_of(work, struct msdc_host, remove_card.work);

    BUG_ON(!host);
    BUG_ON(!(host->mmc));
    MSDC_ERR_PRINT(MSDC_ERROR, "Need remove card");
    if(host->mmc->card){
        if(mmc_card_present(host->mmc->card)){
            MSDC_ERR_PRINT(MSDC_ERROR, "1.remove card");
            mmc_remove_card(host->mmc->card);
        } else{
            MSDC_ERR_PRINT(MSDC_ERROR, "card was not present can not remove");
            host->block_bad_card = 0;
            host->card_inserted = 1;
            host->mmc->card->state &= ~MMC_CARD_REMOVED;
            return;
        }
        mmc_claim_host(host->mmc);
        MSDC_ERR_PRINT(MSDC_ERROR, "2.detach bus");
        host->mmc->card = NULL;
        mmc_detach_bus(host->mmc);
        MSDC_ERR_PRINT(MSDC_ERROR, "3.Power off");
        mmc_power_off(host->mmc);
        MSDC_ERR_PRINT(MSDC_ERROR, "4.Gate clock");
        msdc_gate_clock(host,0);
        mmc_release_host(host->mmc);
    }
    MSDC_ERR_PRINT(MSDC_ERROR, "Card removed");
}
#endif
int msdc_reinit(struct msdc_host *host)
{
    struct mmc_host *mmc;
    struct mmc_card *card;
    //struct mmc_request *mrq;
    int ret = -1;
    u32 err = 0;
    u32 status = 0;
    unsigned long tmo = 12;
    //u32 state = 0;
    if(!host){
        MSDC_ERR_PRINT(MSDC_ERROR, "msdc_host is NULL");
        return -1;
    }
    mmc = host->mmc;
    if (!mmc) {
        MSDC_ERR_PRINT(MSDC_ERROR, "mmc is NULL");
        return -1;
    }

    card = mmc->card;
    if (card == NULL)
        MSDC_ERR_PRINT(MSDC_ERROR, "mmc->card is NULL");
	
    if(host->block_bad_card)
        MSDC_ERR_PRINT(MSDC_ERROR, "Need block this bad SD card from re-initialization");

    // eMMC first
#ifdef CONFIG_MTK_EMMC_SUPPORT
    if (host->hw->host_function == MSDC_EMMC) {
        return -1;
    }
#endif
    if(host->hw->host_function == MSDC_SD){
        if ((!(host->hw->flags & MSDC_CD_PIN_EN)) && (host->block_bad_card == 0)) {
            // power cycle
            MSDC_DBG_PRINT(MSDC_DEBUG, "SD card Re-Init!");
            mmc_claim_host(host->mmc);
            MSDC_DBG_PRINT(MSDC_DEBUG, "SD card Re-Init get host!");
            spin_lock(&host->lock);
            MSDC_DBG_PRINT(MSDC_DEBUG, "SD card Re-Init get lock!");
            msdc_clksrc_onoff(host, 1);
            if(host->app_cmd_arg){
                while((err = msdc_get_card_status(mmc, host, &status))) {
                    MSDC_DBG_PRINT(MSDC_DEBUG, "SD card Re-Init in get card status!err(%d)",err);
                    if(err == (unsigned int)-EIO){
                        if (msdc_tune_cmdrsp(host)) {
                            MSDC_DBG_PRINT(MSDC_DEBUG, "update cmd para failed");
                            break;
                        }
                    }else {
                        break;
                    }
                }
                if(err == 0){
                    msdc_clksrc_onoff(host, 0);
                    spin_unlock(&host->lock);
                    mmc_release_host(host->mmc);
                    MSDC_DBG_PRINT(MSDC_DEBUG, "SD Card is ready.");
                    return 0;
                }
            }
            msdc_clksrc_onoff(host, 0);
            MSDC_DBG_PRINT(MSDC_DEBUG, "Reinit start..");
            mmc->ios.clock = HOST_MIN_MCLK;
            mmc->ios.bus_width = MMC_BUS_WIDTH_1;
            mmc->ios.timing = MMC_TIMING_LEGACY;
            host->card_inserted = 1;
            msdc_clksrc_onoff(host, 1);
            msdc_set_mclk(host, MSDC_STATE_DEFAULT, HOST_MIN_MCLK);
            msdc_clksrc_onoff(host, 0);
            spin_unlock(&host->lock);
            mmc_release_host(host->mmc);
            if(host->mmc->card){
                mmc_remove_card(host->mmc->card);
                host->mmc->card = NULL;
                mmc_claim_host(host->mmc);
                mmc_detach_bus(host->mmc);
                mmc_release_host(host->mmc);
            }
            mmc_power_off(host->mmc);
            mmc_detect_change(host->mmc,0);
            while(tmo){
                if(host->mmc->card && mmc_card_present(host->mmc->card)){
                    ret = 0;
                    break;
                }
                msleep(50);
                tmo--;
            }
            MSDC_DBG_PRINT(MSDC_DEBUG, "Reinit %s",ret == 0 ? "success" : "fail");

       }
		if ((host->hw->flags & MSDC_CD_PIN_EN) && (host->mmc->card)
		    && mmc_card_present(host->mmc->card) && (!mmc_card_removed(host->mmc->card))
		    && (host->block_bad_card == 0))
           ret = 0;
    }
    return ret;
}

static u32 msdc_abort_data(struct msdc_host *host)
{
    struct mmc_host *mmc = host->mmc;
    void __iomem *base = host->base;
    u32 status = 0;
    u32 state = 0;
    u32 err = 0;
    unsigned long tmo = jiffies + POLLING_BUSY;

    while (state != 4) { // until status to "tran"
        msdc_reset_hw(host->id);
        while ((err = msdc_get_card_status(mmc, host, &status))) {
            MSDC_DBG_PRINT(MSDC_DEBUG, "CMD13 ERR<%d>",err);
            if (err != (unsigned int)-EIO) {
                return msdc_power_tuning(host);
            } else if (msdc_tune_cmdrsp(host)) {
                MSDC_DBG_PRINT(MSDC_DEBUG, "update cmd para failed");
                return 1;
            }
        }

        state = R1_CURRENT_STATE(status);
        MSDC_DBG_PRINT(MSDC_DEBUG, "check card state<%d>", state);
        if (state == 5 || state == 6) {
            MSDC_DBG_PRINT(MSDC_DEBUG, "state<%d> need cmd12 to stop", state);
            msdc_send_stop(host); // don't tuning
        } else if (state == 7) {  // busy in programing
            MSDC_DBG_PRINT(MSDC_DEBUG, "state<%d> card is busy", state);
            spin_unlock(&host->lock);
            msleep(100);
            spin_lock(&host->lock);
        } else if (state != 4) {
            MSDC_DBG_PRINT(MSDC_DEBUG, "state<%d> ??? ", state);
            return msdc_power_tuning(host);
        }

        if (time_after(jiffies, tmo)) {
            MSDC_DBG_PRINT(MSDC_DEBUG, "abort timeout. Do power cycle");
			if (host->hw->host_function == MSDC_SD
			    && (host->sclk >= 100000000 || (host->state == MSDC_STATE_DDR)))
                host->sd_30_busy++;
            return msdc_power_tuning(host);
        }
    }

    msdc_reset_hw(host->id);
    return 0;
}
static u32 msdc_polling_idle(struct msdc_host *host)
{
    struct mmc_host *mmc = host->mmc;
    u32 status = 0;
    u32 state = 0;
    u32 err = 0;
    unsigned long tmo = jiffies + POLLING_BUSY;

    while (state != 4) { // until status to "tran"
        while ((err = msdc_get_card_status(mmc, host, &status))) {
            MSDC_DBG_PRINT(MSDC_DEBUG, "CMD13 ERR<%d>",err);
            if (err != (unsigned int)-EIO) {
                return msdc_power_tuning(host);
            } else if (msdc_tune_cmdrsp(host)) {
                MSDC_DBG_PRINT(MSDC_DEBUG, "update cmd para failed");
                return 1;
            }
        }

        state = R1_CURRENT_STATE(status);
        //ERR_MSG("check card state<%d>", state);
        if (state == 5 || state == 6) {
            MSDC_DBG_PRINT(MSDC_DEBUG, "state<%d> need cmd12 to stop", state);
            msdc_send_stop(host); // don't tuning
        } else if (state == 7) {  // busy in programing
            MSDC_DBG_PRINT(MSDC_DEBUG, "state<%d> card is busy", state);
            spin_unlock(&host->lock);
            msleep(100);
            spin_lock(&host->lock);
        } else if (state != 4) {
            MSDC_DBG_PRINT(MSDC_DEBUG, "state<%d> ??? ", state);
            return msdc_power_tuning(host);
        }

        if (time_after(jiffies, tmo)) {
            MSDC_DBG_PRINT(MSDC_DEBUG, "abort timeout. Do power cycle");
            return msdc_power_tuning(host);
        }
    }
    return 0;
}

static void msdc_pin_config(struct msdc_host *host, int mode)
{
    switch (mode) {
        case MSDC_PIN_PULL_UP:
            msdc_pin_set(host, MSDC_PST_PU_10KOHM);
            break;
        case MSDC_PIN_PULL_DOWN:
            msdc_pin_set(host, MSDC_PST_PD_50KOHM);
            break;
        case MSDC_PIN_PULL_NONE:
        default:
            msdc_pin_set(host, MSDC_PST_PD_50KOHM);
            break;
    }

    MSDC_TRC_PRINT(MSDC_CONFIG, "Pins mode(%d), down(%d), up(%d)", mode, MSDC_PIN_PULL_DOWN, MSDC_PIN_PULL_UP);
}


static void msdc_pin_reset(struct msdc_host *host, int mode)
{
    struct msdc_hw *hw = (struct msdc_hw *)host->hw;
    void __iomem *base = host->base;
    int pull = (mode == MSDC_PIN_PULL_UP) ? MSDC_GPIO_PULL_UP : MSDC_GPIO_PULL_DOWN;

    /* Config reset pin */
    if (hw->flags & MSDC_RST_PIN_EN) {
        if (hw->config_gpio_pin) /* NULL */
            hw->config_gpio_pin(MSDC_RST_PIN, pull);

        if (mode == MSDC_PIN_PULL_UP) {
            sdr_clr_bits(EMMC_IOCON, EMMC_IOCON_BOOTRST);
        } else {
            sdr_set_bits(EMMC_IOCON, EMMC_IOCON_BOOTRST);
        }
    }
}

static void msdc_set_power_mode(struct msdc_host *host, u8 mode)
{
    MSDC_TRC_PRINT(MSDC_CONFIG, "Set power mode(%d)", mode);
    if (host->power_mode == MMC_POWER_OFF && mode != MMC_POWER_OFF) {
        msdc_pin_reset (host, MSDC_PIN_PULL_UP);
        msdc_pin_config(host, MSDC_PIN_PULL_UP);

#ifdef FPGA_PLATFORM
        hwPowerOn_fpga(host->id);
#else
        if(host->power_control)
            host->power_control(host,1);
        else
            MSDC_ERR_PRINT(MSDC_ERROR, "No power control callback. Please check host_function<0x%lx> and Power_domain<%d>",host->hw->host_function,host->power_domain);

#endif

        mdelay(10);
    } else if (host->power_mode != MMC_POWER_OFF && mode == MMC_POWER_OFF) {

        if (is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ)) {
            msdc_pin_config(host, MSDC_PIN_PULL_UP);
        }else {

#ifdef FPGA_PLATFORM
            hwPowerDown_fpga(host->id);
#else
            if(host->power_control)
                host->power_control(host,0);
            else
                MSDC_ERR_PRINT(MSDC_ERROR, "No power control callback. Please check host_function<0x%lx> and Power_domain<%d>",host->hw->host_function,host->power_domain);
#endif
            msdc_pin_config(host, MSDC_PIN_PULL_DOWN);
        }
        mdelay(10);
        msdc_pin_reset (host, MSDC_PIN_PULL_DOWN);
    }
    host->power_mode = mode;
}

#ifdef MTK_EMMC_ETT_TO_DRIVER
static int msdc_ett_offline_to_driver(struct msdc_host *host)
{
    int ret = 1;  // 1 means failed
    int size = sizeof(g_mmcTable) / sizeof(mmcdev_info);
    int i, temp;
    void __iomem *base = host->base;
    u32 clkmode;
    int hs400 = 0;

    sdr_get_field(MSDC_CFG,MSDC_CFG_CKMOD,clkmode);
    hs400 = (clkmode == 3) ? 1 : 0;

    //MSDC_DBG_PRINT(MSDC_DEBUG, "msdc_ett_offline_to_driver size<%d> \n", size);

    for (i = 0; i < size; i++) {
        //printk(KERN_ERR"msdc <%d> <%s> <%s>\n", i, g_mmcTable[i].pro_name, pro_name);

        if ((g_mmcTable[i].m_id == m_id) && (!strncmp(g_mmcTable[i].pro_name, pro_name, 6))) {
            MSDC_DBG_PRINT(MSDC_DEBUG,"msdc ett index<%d>: <%d> <%d> <0x%x> <0x%x> <0x%x>\n", i,
                g_mmcTable[i].r_smpl, g_mmcTable[i].d_smpl,
                g_mmcTable[i].cmd_rxdly, g_mmcTable[i].rd_rxdly, g_mmcTable[i].wr_rxdly);

            // set to msdc0

            msdc_set_smpl(host, hs400, g_mmcTable[i].r_smpl, TYPE_CMD_RESP_EDGE, NULL);
            msdc_set_smpl(host, hs400, g_mmcTable[i].d_smpl, TYPE_READ_DATA_EDGE, NULL);

            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, g_mmcTable[i].cmd_rxdly);
            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATRRDLY, g_mmcTable[i].rd_rxdly);
            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, g_mmcTable[i].wr_rxdly);

            temp = g_mmcTable[i].rd_rxdly; temp &= 0x1F;
            sdr_write32(MSDC_DAT_RDDLY0, (temp<<0 | temp<<8 | temp<<16 | temp<<24));
            sdr_write32(MSDC_DAT_RDDLY1, (temp<<0 | temp<<8 | temp<<16 | temp<<24));

            ret = 0;
            break;
        }
    }

    //if (ret) printk(KERN_ERR "msdc failed to find\n");
    return ret;
}
#endif


extern int mmc_card_sleepawake(struct mmc_host *host, int sleep);
//extern int mmc_send_status(struct mmc_card *card, u32 *status);
extern int mmc_go_idle(struct mmc_host *host);
extern int mmc_send_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr);
extern int mmc_all_send_cid(struct mmc_host *host, u32 *cid);
//extern int mmc_attach_mmc(struct mmc_host *host, u32 ocr);

typedef enum MMC_STATE_TAG
{
    MMC_IDLE_STATE,
    MMC_READ_STATE,
    MMC_IDENT_STATE,
    MMC_STBY_STATE,
    MMC_TRAN_STATE,
    MMC_DATA_STATE,
    MMC_RCV_STATE,
    MMC_PRG_STATE,
    MMC_DIS_STATE,
    MMC_BTST_STATE,
    MMC_SLP_STATE,
    MMC_RESERVED1_STATE,
    MMC_RESERVED2_STATE,
    MMC_RESERVED3_STATE,
    MMC_RESERVED4_STATE,
    MMC_RESERVED5_STATE,
}MMC_STATE_T;

typedef enum EMMC_CHIP_TAG{
    SAMSUNG_EMMC_CHIP = 0x15,
    SANDISK_EMMC_CHIP = 0x45,
    HYNIX_EMMC_CHIP   = 0x90,
} EMMC_VENDOR_T;
#if 0
#ifdef CONFIG_MTK_EMMC_SUPPORT
unsigned int sg_emmc_sleep = 0;
static void msdc_config_emmc_pad(int padEmmc)
{
    static int sg_gpio164_mode;
    static int sg_gpio165_mode;
    static int sg_gpio166_mode;
    static int sg_gpio168_mode;
    static int sg_gpio169_mode;
    static int sg_gpio170_mode;
    static int sg_gpio171_mode;
    static int sg_gpio173_mode;
    static int sg_gpio175_mode;

    if (padEmmc == 0){
        sg_gpio164_mode = mt_get_gpio_mode(GPIO164);
        sg_gpio165_mode = mt_get_gpio_mode(GPIO165);
        sg_gpio166_mode = mt_get_gpio_mode(GPIO166);
        sg_gpio168_mode = mt_get_gpio_mode(GPIO168);
        sg_gpio169_mode = mt_get_gpio_mode(GPIO169);
        sg_gpio170_mode = mt_get_gpio_mode(GPIO170);
        sg_gpio171_mode = mt_get_gpio_mode(GPIO171);
        sg_gpio173_mode = mt_get_gpio_mode(GPIO173);
        sg_gpio175_mode = mt_get_gpio_mode(GPIO175);

        mt_set_gpio_mode(GPIO164, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO165, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO166, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO168, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO169, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO170, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO171, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO173, GPIO_MODE_GPIO);
        mt_set_gpio_mode(GPIO175, GPIO_MODE_GPIO);

        mt_set_gpio_dir(GPIO164, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO165, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO166, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO168, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO169, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO170, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO171, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO173, GPIO_DIR_IN);
        mt_set_gpio_dir(GPIO175, GPIO_DIR_IN);

        mt_set_gpio_pull_enable(GPIO164, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO165, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO166, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO168, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO169, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO170, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO171, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO173, GPIO_PULL_DISABLE);
        mt_set_gpio_pull_enable(GPIO175, GPIO_PULL_DISABLE);
    } else {
        mt_set_gpio_mode(GPIO164, sg_gpio164_mode);
        mt_set_gpio_mode(GPIO165, sg_gpio165_mode);
        mt_set_gpio_mode(GPIO166, sg_gpio166_mode);
        mt_set_gpio_mode(GPIO168, sg_gpio168_mode);
        mt_set_gpio_mode(GPIO169, sg_gpio169_mode);
        mt_set_gpio_mode(GPIO170, sg_gpio170_mode);
        mt_set_gpio_mode(GPIO171, sg_gpio171_mode);
        mt_set_gpio_mode(GPIO173, sg_gpio173_mode);
        mt_set_gpio_mode(GPIO175, sg_gpio175_mode);
    }
}

static void msdc_sleep_enter(struct msdc_host *host)
{
    //u32 l_status = MMC_IDLE_STATE;
    //unsigned long tmo;
    struct mmc_host *mmc;
    struct mmc_card *card;

    BUG_ON(!host->mmc);
    BUG_ON(!host->mmc->card);

    mmc = host->mmc;
    card = host->mmc->card;

    /* check card type */
    if (MMC_TYPE_MMC != card->type) {
        printk(KERN_WARNING"[EMMC] not a mmc card, pls check it before sleep\n");
        return ;
    }

    mmc_claim_host(mmc);
    mmc_go_idle(mmc); // Infinity: Ask eMMC into open-drain mode

    // add for hynix emcp chip
    if (host->mmc->card->cid.manfid == HYNIX_EMMC_CHIP){
        u32 l_ocr = mmc->ocr;
        u32 l_cid[4];
        u32 l_rocr;
        u32 l_ret;

        // clk freq down, 26kHz for emmc card init
        msdc_set_mclk(host, MSDC_STATE_DEFAULT, 400000);

        //send CMD1, will loop for card's busy state
        l_ret = mmc_send_op_cond(mmc, l_ocr | (1 << 30), &l_rocr);
        if (l_ret != 0){
            MSDC_ERR_PRINT(MSDC_ERROR, "send cmd1 error while emmc card enter low power state");       /* won't happen. */
        }

        //send CMD2
        l_cid[0] = 0;
        l_cid[1] = 0;
        l_cid[2] = 0;
        l_cid[3] = 0;
        l_ret = mmc_all_send_cid(mmc, l_cid);
        if (l_ret != 0){
            MSDC_ERR_PRINT(MSDC_ERROR, "send cmd2 error while emmc card enter low power state");       /* won't happen. */
        }
    }

    msdc_config_emmc_pad(0);

    mmc_release_host(mmc);
}

static void msdc_sleep_out(struct msdc_host *host)
{
    struct mmc_host *mmc;
    struct mmc_card *card;

    BUG_ON(!host->mmc);
    BUG_ON(!host->mmc->card);

    mmc = host->mmc;
    card = host->mmc->card;


    /* check card type */
    if (MMC_TYPE_MMC != card->type) {
        printk(KERN_WARNING"[EMMC] not a mmc card, pls check it before sleep\n");
        return ;
    }

    mmc_claim_host(mmc);
    msdc_config_emmc_pad(1);
    mmc_release_host(mmc);
}

static void msdc_emmc_sleepawake(struct msdc_host *host, u32 awake)
{
    /* slot0 for emmc, sd/sdio do not support sleep state */
    if (host->id != EMMC_HOST_ID) {
        return;
    }

    /* for emmc card need to go sleep state, while suspend.
     * because we need emmc power always on to guarantee brom can
     * boot from emmc */
    if ((awake == 1) && (sg_emmc_sleep == 1)) {
        msdc_sleep_out(host);
        sg_emmc_sleep = 0;
    } else if((awake == 0) && (sg_emmc_sleep == 0)) {
        msdc_sleep_enter(host);
        sg_emmc_sleep = 1;
    }
}
#endif
#endif
static void msdc_clksrc_onoff(struct msdc_host *host, u32 on)
{
    void __iomem *base = host->base;
    u32 div, mode, hs400_src;
    if (on) {
        if (0 == host->core_clkon) {
#ifndef FPGA_PLATFORM
            if(msdc_enable_clock(host)){
                MSDC_DBG_PRINT(MSDC_DEBUG, "msdc%d on clock failed ===> retry once\n",host->id);
                msdc_disable_clock(host);
                msdc_enable_clock(host);
            }
#endif
            host->core_clkon = 1;
            udelay(10);

            sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);

            sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD, mode);
            sdr_get_field(MSDC_CFG, MSDC_CFG_CKDIV, div);
            sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD_HS400, hs400_src);
            msdc_clk_stable(host,mode, div, hs400_src);

            if (is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ)) {
                //mdelay(1000);  // wait for WIFI stable.
            }

            //INIT_MSG("3G pll = 0x%x when clk<on>", sdr_read32(0xF00071C0));
#ifndef FPGA_PLATFORM
            //freq_meter(0xf, 0);
#endif
        }
    } else {
        if (!((host->hw->flags & MSDC_SDIO_IRQ) && src_clk_control)) {
            if (1 == host->core_clkon) {
                sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_MS);

#ifndef FPGA_PLATFORM
                msdc_disable_clock(host);
#endif
                host->core_clkon = 0;

                //INIT_MSG("3G pll = 0x%x when clk<off>", sdr_read32(0xF00071C0));
#ifndef FPGA_PLATFORM
                //freq_meter(0xf, 0);
#endif
            }
        }
    }
}

/*
   register as callback function of WIFI(combo_sdio_register_pm) .
   can called by msdc_drv_suspend/resume too.
*/
#ifdef CONFIG_PM
static void msdc_save_emmc_setting(struct msdc_host *host)
{
    void __iomem *base = host->base;
    u32 clkmode;

    host->saved_para.hz = host->mclk;
    host->saved_para.sdc_cfg = sdr_read32(SDC_CFG);

    sdr_get_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, host->hw->ddlsel);

    sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, host->hw->cmd_edge); // save the para
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, host->hw->rdata_edge);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, host->hw->wdata_edge);
    host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
    host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);
    host->saved_para.ddly1 = sdr_read32(MSDC_DAT_RDDLY1);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->saved_para.cmd_resp_ta_cntr);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
#ifdef CONFIG_EMMC_50_FEATURE
    sdr_get_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, host->saved_para.ds_dly1);
    sdr_get_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, host->saved_para.ds_dly3);
#endif
    sdr_get_field(MSDC_CFG,MSDC_CFG_CKMOD,clkmode);
    if(clkmode == 2)
        host->saved_para.state = MSDC_STATE_DDR;
    else if(clkmode == 3)
        host->saved_para.state = MSDC_STATE_HS400;
    else
        host->saved_para.state = MSDC_STATE_DEFAULT;
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, host->saved_para.write_busy_margin);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA, host->saved_para.write_crc_margin);
}
static void msdc_restore_emmc_setting(struct msdc_host *host)
{
    void __iomem *base = host->base;
    msdc_set_mclk(host,host->saved_para.state,host->mclk);
    sdr_write32(SDC_CFG,host->saved_para.sdc_cfg);

    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, host->hw->ddlsel);

    msdc_set_smpl(host, host->saved_para.state, host->hw->cmd_edge, TYPE_CMD_RESP_EDGE, NULL);
    msdc_set_smpl(host, host->saved_para.state, host->hw->rdata_edge, TYPE_READ_DATA_EDGE, NULL);
    msdc_set_smpl(host, host->saved_para.state, host->hw->wdata_edge, TYPE_WRITE_CRC_EDGE, NULL);
    sdr_write32(MSDC_PAD_TUNE,host->saved_para.pad_tune);
    sdr_write32(MSDC_DAT_RDDLY0,host->saved_para.ddly0);
    sdr_write32(MSDC_DAT_RDDLY1,host->saved_para.ddly1);
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->saved_para.cmd_resp_ta_cntr);
#ifdef CONFIG_EMMC_50_FEATURE
    sdr_set_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, host->saved_para.ds_dly1);
    sdr_set_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, host->saved_para.ds_dly3);
#endif

    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, host->saved_para.write_busy_margin);
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA, host->saved_para.write_crc_margin);
}


static void msdc_pm(pm_message_t state, void *data)
{
    struct msdc_host *host = (struct msdc_host *)data;

    int evt = state.event;
    void __iomem *base = host->base;

    msdc_ungate_clock(host);

    if (evt == PM_EVENT_SUSPEND || evt == PM_EVENT_USER_SUSPEND) {
        if (host->suspend)
            goto end;

        if (evt == PM_EVENT_SUSPEND && host->power_mode == MMC_POWER_OFF)
            goto end;

        host->suspend = 1;
        host->pm_state = state;
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
        atomic_set(&host->ot_work.autok_done, 0);
#endif

        MSDC_DBG_PRINT(MSDC_DEBUG, "msdc%d -> %s Suspend",host->id, evt == PM_EVENT_SUSPEND ? "PM" : "USR");
        if(host->hw->flags & MSDC_SYS_SUSPEND){
#ifdef CONFIG_MTK_EMMC_SUPPORT
            if(host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card)){
                if (g_emmc_mode_switch == 0){
                    host->mmc->pm_flags |= MMC_PM_KEEP_POWER;
                } else {
                    host->mmc->pm_flags &= (~MMC_PM_KEEP_POWER);
                }
            }
#else
            if(host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card))
                host->mmc->pm_flags |= MMC_PM_KEEP_POWER;
#endif

            (void)mmc_suspend_host(host->mmc);

#ifdef CONFIG_MTK_EMMC_SUPPORT
            if((g_emmc_mode_switch == 0) && host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card)){
                msdc_save_emmc_setting(host);
                host->power_control(host,0);
                msdc_pin_config(host, MSDC_PIN_PULL_DOWN);
                msdc_pin_reset (host, MSDC_PIN_PULL_DOWN);
            }
#else
            if(host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card)){
                msdc_save_emmc_setting(host);
                host->power_control(host,0);
                msdc_pin_config(host, MSDC_PIN_PULL_DOWN);
                msdc_pin_reset (host, MSDC_PIN_PULL_DOWN);
            }
#endif
        } else {
            host->mmc->pm_flags |= MMC_PM_IGNORE_PM_NOTIFY;
            mmc_remove_host(host->mmc);
        }
    } else if (evt == PM_EVENT_RESUME || evt == PM_EVENT_USER_RESUME) {
        if (!host->suspend){
            goto end;
        }

        if (evt == PM_EVENT_RESUME && host->pm_state.event == PM_EVENT_USER_SUSPEND) {
            MSDC_DBG_PRINT(MSDC_DEBUG,"PM Resume when in USR Suspend");
            goto end;
        }

        host->suspend = 0;
        host->pm_state = state;

        MSDC_DBG_PRINT(MSDC_DEBUG, "msdc%d -> %s Resume",host->id,evt == PM_EVENT_RESUME ? "PM" : "USR");

        if(host->hw->flags & MSDC_SYS_SUSPEND) {
#ifdef CONFIG_MTK_EMMC_SUPPORT
            if((g_emmc_mode_switch == 0) && host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card)){
                msdc_reset_hw(host->id);
                msdc_pin_reset (host, MSDC_PIN_PULL_UP);
                msdc_pin_config(host, MSDC_PIN_PULL_UP);
                host->power_control(host,1);
                mdelay(10);
                msdc_restore_emmc_setting(host);
            }
#else
            if(host->hw->host_function == MSDC_EMMC && host->mmc->card && mmc_card_mmc(host->mmc->card)){
                msdc_reset_hw(host->id);
                msdc_pin_reset (host, MSDC_PIN_PULL_UP);
                msdc_pin_config(host, MSDC_PIN_PULL_UP);
                host->power_control(host,1);
                mdelay(10);
                msdc_restore_emmc_setting(host);
            }
#endif
            (void)mmc_resume_host(host->mmc);
        } else {
            host->mmc->pm_flags |= MMC_PM_IGNORE_PM_NOTIFY;
            mmc_add_host(host->mmc);
        }
    }

end:
#ifdef SDIO_ERROR_BYPASS
	if(is_card_sdio(host)){
		host->sdio_error = 0;
	}
#endif
    if ((evt == PM_EVENT_SUSPEND) || (evt == PM_EVENT_USER_SUSPEND)) {
        if((host->hw->host_function == MSDC_SDIO) && (evt == PM_EVENT_USER_SUSPEND)){
            MSDC_DBG_PRINT(MSDC_DEBUG,"msdc%d -> MSDC Device Request Suspend",host->id);
        }
        msdc_gate_clock(host, 0);
    }else {
        msdc_gate_clock(host, 1);
    }
}
#endif

static u64 msdc_get_user_capacity(struct msdc_host *host)
{
    u64 device_capacity = 0;
    u32 device_legacy_capacity = 0;
    struct mmc_host* mmc = NULL;
    BUG_ON(!host);
    BUG_ON(!host->mmc);
    BUG_ON(!host->mmc->card);
    mmc = host->mmc;
    if(mmc_card_mmc(mmc->card)){
        if(mmc->card->csd.read_blkbits){
            device_legacy_capacity = mmc->card->csd.capacity * (2 << (mmc->card->csd.read_blkbits - 1));
        } else{
            device_legacy_capacity = mmc->card->csd.capacity;
            MSDC_DBG_PRINT(MSDC_DEBUG, "XXX read_blkbits = 0 XXX");
        }
        device_capacity = (u64)(mmc->card->ext_csd.sectors)* 512 > device_legacy_capacity ? (u64)(mmc->card->ext_csd.sectors)* 512 : device_legacy_capacity;
    }
    else if(mmc_card_sd(mmc->card)){
        device_capacity = (u64)(mmc->card->csd.capacity) << (mmc->card->csd.read_blkbits);
    }
    return device_capacity;
}

struct msdc_host *msdc_get_host(int host_function,bool boot,bool secondary)
{
    int host_index = 0;
    struct msdc_host *host = NULL;

    for(;host_index < HOST_MAX_NUM;++host_index)
    {
        if(!mtk_msdc_host[host_index])
            continue;
        if((host_function == mtk_msdc_host[host_index]->hw->host_function)
            && (boot == mtk_msdc_host[host_index]->hw->boot))
        {
            host = mtk_msdc_host[host_index];
            break;
        }
    }
    if(secondary && (host_function == MSDC_SD))
        host = mtk_msdc_host[2];
    if(host == NULL){
        MSDC_ERR_PRINT(MSDC_ERROR,"[MSDC] This host(<host_function:%d> <boot:%d><secondary:%d>) isn't in MSDC host config list",host_function,boot,secondary);
        //BUG();
    }

    return host;
}
EXPORT_SYMBOL(msdc_get_host);

#ifdef CONFIG_MTK_EMMC_SUPPORT
u8 ext_csd[512];
int offset = 0;
char partition_access = 0;

static int msdc_get_data(u8* dst,struct mmc_data *data)
{
    int left;
    u8* ptr;
    struct scatterlist *sg = data->sg;
    int num = data->sg_len;
    while (num) {
        left = sg_dma_len(sg);
        ptr = (u8*)sg_virt(sg);
        memcpy(dst,ptr,left);
        sg = sg_next(sg);
        dst+=left;
        num--;
    }
    return 0;
}

#ifdef MTK_MSDC_USE_CACHE
unsigned long long g_cache_part_start;
unsigned long long g_cache_part_end;
unsigned long long g_usrdata_part_start;
unsigned long long g_usrdata_part_end;

void msdc_get_cache_region_func(struct msdc_host   *host)
{
#ifndef CONFIG_MTK_GPT_SCHEME_SUPPORT
    int i = 0;

    if(PartInfo == NULL)
        return;

    for(i=0; i< PART_NUM; i++){
        if(0 == strcmp ("cache", PartInfo[i].name)){
            if(mmc_card_blockaddr(host->mmc->card)){
                g_cache_part_start = PartInfo[i].start_address / 512;
                g_cache_part_end = (PartInfo[i].start_address + PartInfo[i].size) / 512;
            }else {
                g_cache_part_start = PartInfo[i].start_address;
                g_cache_part_end = PartInfo[i].start_address + PartInfo[i].size;
            }
        }
        if(0 == strcmp ("usrdata", PartInfo[i].name)){
            if(mmc_card_blockaddr(host->mmc->card)){
                g_usrdata_part_start = PartInfo[i].start_address / 512;
                g_usrdata_part_end = (PartInfo[i].start_address + PartInfo[i].size) / 512;
            }else {
                g_usrdata_part_start = PartInfo[i].start_address;
                g_usrdata_part_end = PartInfo[i].start_address + PartInfo[i].size;
            }
        }
    }

    MSDC_DBG_PRINT(MSDC_STACK, "cache(0x%lld~0x%lld), usrdata(0x%lld~0x%lld)", g_cache_part_start, g_cache_part_end, g_usrdata_part_start, g_usrdata_part_end);

#else

	struct hd_struct * lp_hd_struct = NULL;
	lp_hd_struct = get_part("cache");
	if (likely(lp_hd_struct)) {
		g_cache_part_start = lp_hd_struct->start_sect;
		g_cache_part_end = g_cache_part_start + lp_hd_struct->nr_sects;
		put_part(lp_hd_struct);
	} else {
		g_cache_part_start = (sector_t)(-1);
		g_cache_part_end = (sector_t)(-1);
		MSDC_DBG_PRINT(MSDC_STACK, "There is no cache info\n");
	}

	lp_hd_struct = NULL;
	lp_hd_struct = get_part("userdata");
	if (likely(lp_hd_struct)) {
		g_usrdata_part_start = lp_hd_struct->start_sect;
		g_usrdata_part_end = g_usrdata_part_start + lp_hd_struct->nr_sects;
		put_part(lp_hd_struct);
	} else {
		g_usrdata_part_start = (sector_t)(-1);
		g_usrdata_part_end = (sector_t)(-1);
		MSDC_DBG_PRINT(MSDC_STACK, "There is no userdata info\n");
	}

    MSDC_DBG_PRINT(MSDC_STACK,"cache(0x%llX~0x%llX, usrdata(0x%llX~0x%llX)\n", g_cache_part_start, g_cache_part_end, g_usrdata_part_start, g_usrdata_part_end);
#endif
    return;
}

static int msdc_can_apply_cache(unsigned long long start_addr, unsigned int size)
{
    /* since cache, userdata partition are connected, so check it as an area, else do check them seperately */
    if(g_cache_part_end == g_usrdata_part_start){
        if(!((start_addr >= g_cache_part_start) && (start_addr + size < g_usrdata_part_end))){
            return 0;
         }
    }else {
        if(!(((start_addr >= g_cache_part_start) && (start_addr + size < g_cache_part_end)) ||
             ((start_addr >= g_usrdata_part_start) && (start_addr + size < g_usrdata_part_end)))) {
            return 0;
         }
    }

    return 1;
}

static int msdc_cache_ctrl(struct msdc_host *host, unsigned int enable, u32 *status)
{
    struct mmc_command cmd;
    struct mmc_request mrq;
    u32 err;

    memset(&cmd, 0, sizeof(struct mmc_command));
    cmd.opcode = MMC_SWITCH;    // CMD6
    cmd.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) | (EXT_CSD_CACHE_CTRL << 16) | (!!enable << 8) | EXT_CSD_CMD_SET_NORMAL;
    cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

    memset(&mrq, 0, sizeof(struct mmc_request));
	mrq.cmd = &cmd;
	cmd.mrq = &mrq;
    cmd.data = NULL;

    MSDC_DBG_PRINT(MSDC_STACK, "do disable Cache, cmd=0x%x, arg=0x%x \n", cmd.opcode, cmd.arg);
    err = msdc_do_command(host, &cmd, 0, CMD_TIMEOUT);  // tune until CMD13 pass.

    if (status) {
        *status = cmd.resp[0];
    }
    if(!err){
        host->mmc->card->ext_csd.cache_ctrl = !!enable;
        host->autocmd |= MSDC_AUTOCMD23;
        MSDC_DBG_PRINT(MSDC_STACK, "enable AUTO_CMD23 because Cache feature is disabled\n");
    }

    return err;
}
#endif

int msdc_get_cache_region(void)
{
#ifdef MTK_MSDC_USE_CACHE
    struct msdc_host *host = NULL;

    host = msdc_get_host(MSDC_EMMC,MSDC_BOOT_EN,0);

    if(!(host->mmc->caps2 & MMC_CAP2_CACHE_CTRL))
        return 0;

    msdc_get_cache_region_func(host);
#endif
    return 0;

}
EXPORT_SYMBOL(msdc_get_cache_region);

#ifdef CONFIG_MTK_EMMC_SUPPORT_OTP
/* parse part_info struct, support otp & mtk reserve */
#ifndef CONFIG_MTK_GPT_SCHEME_SUPPORT
static struct excel_info* msdc_reserve_part_info(unsigned char* name)
{
    int i;

    /* find reserve partition */
    for (i = 0; i < PART_NUM; i++) {
        MSDC_DBG_PRINT(MSDC_DEBUG, "name = %s\n", PartInfo[i].name);  //====================debug
        if (0 == strcmp(name, PartInfo[i].name)){
            MSDC_DBG_PRINT(MSDC_DEBUG, "size = %llu\n", PartInfo[i].size);//=======================debug
            return &PartInfo[i];
        }
    }

    return NULL;
}
#endif
#endif
static u32 msdc_get_other_capacity(void)
{
    u32 device_other_capacity = 0;
    device_other_capacity = ext_csd[EXT_CSD_BOOT_SIZE_MULT]* 128 * 1024
                + ext_csd[EXT_CSD_BOOT_SIZE_MULT] * 128 * 1024
                + ext_csd[EXT_CSD_RPMB_SIZE_MULT] * 128 * 1024
                + ext_csd[EXT_CSD_GP1_SIZE_MULT + 2] * 256 * 256
                + ext_csd[EXT_CSD_GP1_SIZE_MULT + 1] * 256
                + ext_csd[EXT_CSD_GP1_SIZE_MULT + 0]
                + ext_csd[EXT_CSD_GP2_SIZE_MULT + 2] * 256 * 256
                + ext_csd[EXT_CSD_GP2_SIZE_MULT + 1] * 256
                + ext_csd[EXT_CSD_GP2_SIZE_MULT + 0]
                + ext_csd[EXT_CSD_GP3_SIZE_MULT + 2] * 256 * 256
                + ext_csd[EXT_CSD_GP3_SIZE_MULT + 1] * 256
                + ext_csd[EXT_CSD_GP3_SIZE_MULT + 0]
                + ext_csd[EXT_CSD_GP4_SIZE_MULT + 2] * 256 * 256
                + ext_csd[EXT_CSD_GP4_SIZE_MULT + 1] * 256
                + ext_csd[EXT_CSD_GP4_SIZE_MULT + 0];

    return device_other_capacity;
}

int msdc_get_offset(void)
{
    u32 l_offset;

    l_offset = 0; //l_offset =  MBR_START_ADDRESS_BYTE - msdc_get_other_capacity();

    return (l_offset >> 9);
}
EXPORT_SYMBOL(msdc_get_offset);

int msdc_get_reserve(void)
{
    u32 l_offset;
#ifndef CONFIG_MTK_GPT_SCHEME_SUPPORT
    struct excel_info* lp_excel_info;
#endif
    u32 l_otp_reserve = 0;
    u32 l_mtk_reserve = 0;

    l_offset = msdc_get_offset(); //==========check me

#ifndef CONFIG_MTK_GPT_SCHEME_SUPPORT
    lp_excel_info = msdc_reserve_part_info("bmtpool");
    if (NULL == lp_excel_info) {
        MSDC_ERR_PRINT(MSDC_ERROR,"can't get otp info from part_info struct\n");
        return -1;
    }

    l_mtk_reserve = (unsigned int)(lp_excel_info->start_address & 0xFFFFUL) << 8; /* unit is 512B */

    MSDC_DBG_PRINT(MSDC_DEBUG, "mtk reserve: start address = %llu\n", lp_excel_info->start_address); //============================debug

#ifdef CONFIG_MTK_EMMC_SUPPORT_OTP
    lp_excel_info = msdc_reserve_part_info("otp");
    if (NULL == lp_excel_info) {
        MSDC_ERR_PRINT(MSDC_ERROR,"can't get otp info from part_info struct\n");
        return -1;
    }

    l_otp_reserve = (unsigned int)(lp_excel_info->start_address & 0xFFFFUL) << 8; /* unit is 512B */

    MSDC_DBG_PRINT(MSDC_DEBUG, "otp reserve: start address = %llu\n", lp_excel_info->start_address);//========================debug
    l_otp_reserve -= l_mtk_reserve;  /* the size info stored with total reserved size */
#endif
    MSDC_DBG_PRINT(MSDC_DEBUG, "total reserve: l_otp_reserve = 0x%x blocks, l_mtk_reserve = 0x%x blocks, l_offset = 0x%x blocks\n",
             l_otp_reserve, l_mtk_reserve, l_offset);
#endif

    return (l_offset + l_otp_reserve + l_mtk_reserve);
}
EXPORT_SYMBOL(msdc_get_reserve);

#if 0 //removed in new combo eMMC feature
static bool msdc_cal_offset(struct msdc_host *host)
{
/*
    u64 device_capacity = 0;
    offset =  MBR_START_ADDRESS_BYTE - msdc_get_other_capacity();
    device_capacity = msdc_get_user_capacity(host);
    if(mmc_card_blockaddr(host->mmc->card))
        offset /= 512;
    ERR_MSG("Address offset in USER REGION(Capacity %lld MB) is 0x%x",device_capacity/(1024*1024),offset);
    if(offset < 0) {
        ERR_MSG("XXX Address offset error(%d),please check MBR start address!!",(int)offset);
        BUG();
    }
*/
    offset =  0;
    return true;
}
#endif
#endif

u64 msdc_get_capacity(int get_emmc_total)
{
    u64 user_size = 0;
    u32 other_size = 0;
    u64 total_size = 0;
    int index = 0;
    for(index = 0;index < HOST_MAX_NUM;++index){
        if((mtk_msdc_host[index] != NULL) && (mtk_msdc_host[index]->hw->boot)){
            user_size = msdc_get_user_capacity(mtk_msdc_host[index]);
#ifdef CONFIG_MTK_EMMC_SUPPORT
            if(get_emmc_total){
                if(mmc_card_mmc(mtk_msdc_host[index]->mmc->card))
                    other_size = msdc_get_other_capacity();
            }
#endif
            break;
        }
    }
    total_size = user_size + (u64)other_size;
    return total_size/512;
}
EXPORT_SYMBOL(msdc_get_capacity);

#if 0 //for SD boot up
u32 erase_start = 0;
u32 erase_end = 0;
u32 erase_bypass = 0;
extern int mmc_erase_group_aligned(struct mmc_card *card, unsigned int from,unsigned int nr);
#endif

/*--------------------------------------------------------------------------*/
/* mmc_host_ops members                                                      */
/*--------------------------------------------------------------------------*/
static u32 wints_cmd = MSDC_INT_CMDRDY  | MSDC_INT_RSPCRCERR  | MSDC_INT_CMDTMO  |
                       MSDC_INT_ACMDRDY | MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO;
static unsigned int msdc_command_start(struct msdc_host   *host,
                                      struct mmc_command *cmd,
                                      int                 tune,   /* not used */
                                      unsigned long       timeout)
{
    void __iomem *base = host->base;
    u32 opcode = cmd->opcode;
    u32 rawcmd=0;
    u32 rawarg=0;
    u32 resp=0;
    unsigned long tmo;
#ifdef MTK_MSDC_USE_CACHE
    struct mmc_command *sbc =  NULL;
    if ((host->mmc->caps2 & MMC_CAP2_CACHE_CTRL) && (host->autocmd & MSDC_AUTOCMD23)){
        if (host->data && host->data->mrq && host->data->mrq->sbc)
            sbc =  host->data->mrq->sbc;
    }
#endif

    /* Protocol layer does not provide response type, but our hardware needs
     * to know exact type, not just size!
     */
    if (opcode == MMC_SEND_OP_COND || opcode == SD_APP_OP_COND)
        resp = RESP_R3;
    else if (opcode == MMC_SET_RELATIVE_ADDR || opcode == SD_SEND_RELATIVE_ADDR)
        resp = (mmc_cmd_type(cmd) == MMC_CMD_BCR) ? RESP_R6 : RESP_R1;
    else if (opcode == MMC_FAST_IO)
        resp = RESP_R4;
    else if (opcode == MMC_GO_IRQ_STATE)
        resp = RESP_R5;
    else if (opcode == MMC_SELECT_CARD) {
        resp = (cmd->arg != 0) ? RESP_R1B : RESP_NONE;
        host->app_cmd_arg = cmd->arg;
        MSDC_DBG_PRINT(MSDC_DEBUG, "msdc%d select card<0x%.8x>", host->id,cmd->arg);  // select and de-select
    } else if (opcode == SD_IO_RW_DIRECT || opcode == SD_IO_RW_EXTENDED)
        resp = RESP_R1; /* SDIO workaround. */
    else if (opcode == SD_SEND_IF_COND && (mmc_cmd_type(cmd) == MMC_CMD_BCR))
        resp = RESP_R1;
    else {
        switch (mmc_resp_type(cmd)) {
        case MMC_RSP_R1:
            resp = RESP_R1;
            break;
        case MMC_RSP_R1B:
            resp = RESP_R1B;
            break;
        case MMC_RSP_R2:
            resp = RESP_R2;
            break;
        case MMC_RSP_R3:
            resp = RESP_R3;
            break;
        case MMC_RSP_NONE:
        default:
            resp = RESP_NONE;
            break;
        }
    }

    cmd->error = 0;
    /* rawcmd :
     * vol_swt << 30 | auto_cmd << 28 | blklen << 16 | go_irq << 15 |
     * stop << 14 | rw << 13 | dtype << 11 | rsptyp << 7 | brk << 6 | opcode
     */
	MSDC_SET_BITFIELD(rawcmd,SDC_CMD_OPC,opcode);
	MSDC_SET_BITFIELD(rawcmd,SDC_CMD_RSPTYP,msdc_rsp[resp]);
	MSDC_SET_BITFIELD(rawcmd,SDC_CMD_BLKLEN,host->blksz);

    if (opcode == MMC_READ_MULTIPLE_BLOCK) {
		WARN_ON(host->blksz==0);
		MSDC_SET_BITFIELD(rawcmd,SDC_CMD_DTYP,2);
        if (host->autocmd & MSDC_AUTOCMD12)
		    MSDC_SET_BITFIELD(rawcmd,SDC_CMD_AUTOCMD,1);
#ifdef MTK_MSDC_USE_CMD23
        else if((host->autocmd & MSDC_AUTOCMD23))
		    MSDC_SET_BITFIELD(rawcmd,SDC_CMD_AUTOCMD,2);
#endif /* end of MTK_MSDC_USE_CMD23 */
    } else if (opcode == MMC_READ_SINGLE_BLOCK) {
		WARN_ON(host->blksz==0);
		MSDC_SET_BITFIELD(rawcmd,SDC_CMD_DTYP,1);
    } else if (opcode == MMC_WRITE_MULTIPLE_BLOCK) {
	    WARN_ON(host->blksz==0);
		MSDC_SET_BITFIELD(rawcmd,SDC_CMD_DTYP,2);
		MSDC_SET_BITFIELD(rawcmd,SDC_CMD_RW,1);
        if (host->autocmd & MSDC_AUTOCMD12)
		    MSDC_SET_BITFIELD(rawcmd,SDC_CMD_AUTOCMD,1);
#ifdef MTK_MSDC_USE_CMD23
        else if((host->autocmd & MSDC_AUTOCMD23)){
		    MSDC_SET_BITFIELD(rawcmd,SDC_CMD_AUTOCMD,2);
#ifdef MTK_MSDC_USE_CACHE
            if(sbc && host->mmc->card && mmc_card_mmc(host->mmc->card) && (host->mmc->card->ext_csd.cache_ctrl & 0x1)){
                if(sdr_read32(SDC_BLK_NUM) != (sbc->arg & 0xFFFF)){
					//if the block number is bigger than 0xFFFF, then CMD23 arg will be failed to set it.
                    MSDC_DBG_PRINT(MSDC_STACK,"msdc%d: acmd23 arg(0x%x) failed to match the block num(0x%x), SDC_BLK_NUM(0x%x)\n", host->id, sbc->arg, host->mrq->cmd->data->blocks, sdr_read32(SDC_BLK_NUM));
                }
				else{					
					MSDC_BUGON((sbc && sbc->arg==0));
                    sdr_write32(SDC_BLK_NUM, sbc->arg);
				}
                MSDC_DBG_PRINT(MSDC_STACK,"SBC addr<0x%x>, arg<0x%x> ", cmd->arg, sbc->arg);
            }
#endif
        }
#endif /* end of MTK_MSDC_USE_CMD23 */
    } else if (opcode == MMC_WRITE_BLOCK) {
		WARN_ON(host->blksz==0);
		MSDC_SET_BITFIELD(rawcmd,SDC_CMD_DTYP,1);
		MSDC_SET_BITFIELD(rawcmd,SDC_CMD_RW,1);
#ifdef MTK_MSDC_USE_CACHE
        if(host->mmc->card && mmc_card_mmc(host->mmc->card) && (host->mmc->card->ext_csd.cache_ctrl & 0x1))
            MSDC_ERR_PRINT(MSDC_ERROR, "[Warning]: Single write(0x%x) happend when cache is enabled", cmd->arg);
#endif
    } else if (opcode == SD_IO_RW_EXTENDED) {
	    WARN_ON(host->blksz==0);
        if (cmd->data->flags & MMC_DATA_WRITE)
			MSDC_SET_BITFIELD(rawcmd,SDC_CMD_RW,1);
        if (cmd->data->blocks > 1)
			MSDC_SET_BITFIELD(rawcmd,SDC_CMD_DTYP,2);
        else
			MSDC_SET_BITFIELD(rawcmd,SDC_CMD_DTYP,1);
    } else if (opcode == SD_IO_RW_DIRECT && cmd->flags == (unsigned int)-1) {
		       MSDC_SET_BITFIELD(rawcmd,SDC_CMD_STOP,1);
    } else if (opcode == SD_SWITCH_VOLTAGE) {
		       MSDC_SET_BITFIELD(rawcmd,SDC_CMD_VOLSWTH,1);
    } else if ((opcode == SD_APP_SEND_SCR) ||
               (opcode == SD_APP_SEND_NUM_WR_BLKS) ||
               (opcode == SD_SWITCH && (mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
               (opcode == SD_APP_SD_STATUS && (mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
               (opcode == MMC_SEND_EXT_CSD && (mmc_cmd_type(cmd) == MMC_CMD_ADTC))) {
		       MSDC_SET_BITFIELD(rawcmd,SDC_CMD_DTYP,1);
    } else if (opcode == MMC_STOP_TRANSMISSION) {
		       MSDC_SET_BITFIELD(rawcmd,SDC_CMD_STOP,1);
		       MSDC_SET_BITFIELD(rawcmd,SDC_CMD_BLKLEN,0);
    }

    MSDC_DBG_PRINT(MSDC_STACK, "CMD<%d><0x%.8x> Arg<0x%.8x>", opcode , rawcmd, cmd->arg);

    tmo = jiffies + timeout;

    if (opcode == MMC_SEND_STATUS) {
        for (;;) {
            if (!sdc_is_cmd_busy())
                break;

            if (time_after(jiffies, tmo)) {
                MSDC_ERR_PRINT(MSDC_ERROR, "XXX cmd_busy timeout: before CMD<%d>", opcode);
                cmd->error = (unsigned int)-ETIMEDOUT;
                msdc_reset_hw(host->id);
                return cmd->error;
            }
        }
    } else {
        for (;;) {
            if (!sdc_is_busy())
                break;
            if (time_after(jiffies, tmo)) {
                MSDC_ERR_PRINT(MSDC_ERROR,"XXX sdc_busy timeout: before CMD<%d>", opcode);
                cmd->error = (unsigned int)-ETIMEDOUT;
                msdc_reset_hw(host->id);
                return cmd->error;
            }
        }
    }

    //BUG_ON(in_interrupt());
    host->cmd     = cmd;
    host->cmd_rsp = resp;

    /* use polling way */
    sdr_clr_bits(MSDC_INTEN, wints_cmd);
    rawarg = cmd->arg;
#if 0 //for SD boot up
    if(host->hw->host_function == MSDC_EMMC              &&
                host->hw->boot == MSDC_BOOT_EN           &&
                  (cmd->opcode == MMC_READ_SINGLE_BLOCK    ||
                   cmd->opcode == MMC_READ_MULTIPLE_BLOCK  ||
                   cmd->opcode == MMC_WRITE_BLOCK          ||
                   cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ||
                   cmd->opcode == MMC_ERASE_GROUP_START    ||
                   cmd->opcode == MMC_ERASE_GROUP_END)   &&
                   (partition_access == 0)) {
        if(cmd->arg == 0)
            msdc_cal_offset(host);
        rawarg  += offset;
        if(cmd->opcode == MMC_ERASE_GROUP_START)
            erase_start = rawarg;
        if(cmd->opcode == MMC_ERASE_GROUP_END)
            erase_end = rawarg;
    }

    if(cmd->opcode == MMC_ERASE                                          &&
         (cmd->arg == MMC_SECURE_ERASE_ARG || cmd->arg == MMC_ERASE_ARG) &&
           host->mmc->card                                               &&
           host->hw->host_function == MSDC_EMMC                          &&
           host->hw->boot == MSDC_BOOT_EN                                &&
           (!mmc_erase_group_aligned(host->mmc->card, erase_start, erase_end - erase_start + 1))){
        if(mmc_can_trim(host->mmc->card)){
            if(cmd->arg == MMC_SECURE_ERASE_ARG && mmc_can_secure_erase_trim(host->mmc->card))
                rawarg = MMC_SECURE_TRIM1_ARG;
            else if(cmd->arg == MMC_ERASE_ARG ||(cmd->arg == MMC_SECURE_ERASE_ARG && !mmc_can_secure_erase_trim(host->mmc->card)))
                rawarg = MMC_TRIM_ARG;
        }else {
            erase_bypass = 1;
            MSDC_ERR_PRINT(MSDC_ERROR, "cancel format,cmd<%d> arg=0x%x, start=0x%x, end=0x%x, size=%d, erase_bypass=%d",
                    cmd->opcode, rawarg, erase_start, erase_end, (erase_end - erase_start + 1), erase_bypass);
            goto end;
        }
    }
#endif

    sdc_send_cmd(rawcmd, rawarg);

//end:
    return 0;  // irq too fast, then cmd->error has value, and don't call msdc_command_resp, don't tune.
}

static unsigned int msdc_command_resp_polling(struct msdc_host   *host,
        struct mmc_command *cmd,
        int                 tune,
        unsigned long       timeout)
{
    void __iomem *base = host->base;
    u32 intsts;
    u32 resp;
    //u32 status;
    unsigned long tmo;
    //struct mmc_data   *data = host->data;
    u32 cmdsts = MSDC_INT_CMDRDY  | MSDC_INT_RSPCRCERR  | MSDC_INT_CMDTMO;
#ifdef MTK_MSDC_USE_CMD23
    struct mmc_command *sbc =  NULL;
#endif

#if 0 //for SD boot up
    if(erase_bypass && (cmd->opcode == MMC_ERASE)){
        erase_bypass = 0;
        MSDC_ERR_PRINT(MSDC_ERROR, "bypass cmd<%d>, erase_bypass=%d", cmd->opcode, erase_bypass);
        goto out;
    }
#endif

#ifdef MTK_MSDC_USE_CMD23
    if (host->autocmd & MSDC_AUTOCMD23){
        if (host->data && host->data->mrq && host->data->mrq->sbc)
            sbc =  host->data->mrq->sbc;

        /* autocmd interupt disabled, used polling way */
        cmdsts |= MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO;
    }
#endif

    resp = host->cmd_rsp;

    /*polling*/
    tmo = jiffies + timeout;
    while (1){
        if (((intsts = sdr_read32(MSDC_INT)) & cmdsts) != 0){
            /* clear all int flag */
#ifdef MTK_MSDC_USE_CMD23
            // need clear autocmd23 comand ready interrupt
            intsts &= (cmdsts | MSDC_INT_ACMDRDY);
#else
            intsts &= cmdsts;
#endif
            sdr_write32(MSDC_INT, intsts);
            break;
        }

        if (time_after(jiffies, tmo)) {
            MSDC_ERR_PRINT(MSDC_ERROR, "[%s]: msdc%d XXX CMD<%d> polling_for_completion timeout ARG<0x%.8x>", __func__, host->id, cmd->opcode, cmd->arg);
            cmd->error = (unsigned int)-ETIMEDOUT;
            host->sw_timeout++;
            MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
            msdc_reset_hw(host->id);
            goto out;
        }
    }

#ifdef MTK_MSDC_ERROR_TUNE_DEBUG
    if(g_err_tune_dbg_error && (g_err_tune_dbg_count > 0) && (g_err_tune_dbg_host == host->id)) {
        if(g_err_tune_dbg_cmd == cmd->opcode){
            if((g_err_tune_dbg_cmd != MMC_SWITCH) || ((g_err_tune_dbg_cmd == MMC_SWITCH) && (g_err_tune_dbg_arg == ((cmd->arg >> 16) & 0xff)))){
                if(g_err_tune_dbg_error & MTK_MSDC_ERROR_CMD_TMO){
                    intsts = MSDC_INT_CMDTMO;
                    g_err_tune_dbg_count--;
                }else if (g_err_tune_dbg_error & MTK_MSDC_ERROR_CMD_CRC) {
                    intsts = MSDC_INT_RSPCRCERR;
                    g_err_tune_dbg_count--;
                }
                MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: got the error cmd:%d, arg=%d, dbg error=%d, cmd->error=%d, count=%d\n", __func__, g_err_tune_dbg_cmd, g_err_tune_dbg_arg, g_err_tune_dbg_error, cmd->error, g_err_tune_dbg_count);
            }
        }

#ifdef MTK_MSDC_USE_CMD23
        if((g_err_tune_dbg_cmd == MMC_SET_BLOCK_COUNT) && sbc && (host->autocmd & MSDC_AUTOCMD23)) {
            if(g_err_tune_dbg_error & MTK_MSDC_ERROR_ACMD_TMO){
                intsts = MSDC_INT_ACMDTMO;
                g_err_tune_dbg_count--;
                MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: got the error cmd:%d, dbg error=%d, sbc->error=%d, count=%d\n", __func__, g_err_tune_dbg_cmd, g_err_tune_dbg_error, sbc->error, g_err_tune_dbg_count);
            }else if(g_err_tune_dbg_error & MTK_MSDC_ERROR_ACMD_CRC){
                intsts = MSDC_INT_ACMDCRCERR;
                g_err_tune_dbg_count--;
                MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: got the error cmd:%d, dbg error=%d, sbc->error=%d, count=%d\n", __func__, g_err_tune_dbg_cmd, g_err_tune_dbg_error, sbc->error, g_err_tune_dbg_count);
            }
        }
#endif
    }
#endif
    /* command interrupts */
    if (intsts & cmdsts) {
#ifdef MTK_MSDC_USE_CMD23
        if ((intsts & MSDC_INT_CMDRDY) || (intsts & MSDC_INT_ACMD19_DONE)) {
#else
        if ((intsts & MSDC_INT_CMDRDY) || (intsts & MSDC_INT_ACMDRDY) ||
            (intsts & MSDC_INT_ACMD19_DONE)) {
#endif
            u32 *rsp = NULL;
            rsp = &cmd->resp[0];
            switch (host->cmd_rsp) {
                case RESP_NONE:
                    break;
                case RESP_R2:
                    *rsp++ = sdr_read32(SDC_RESP3); *rsp++ = sdr_read32(SDC_RESP2);
                    *rsp++ = sdr_read32(SDC_RESP1); *rsp++ = sdr_read32(SDC_RESP0);
                    break;
                default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
                    *rsp = sdr_read32(SDC_RESP0);
                    break;
            }
        } else if (intsts & MSDC_INT_RSPCRCERR) {
            cmd->error = (unsigned int)-EIO;
            MSDC_DBG_PRINT(MSDC_IRQ, "[%s]: msdc%d XXX CMD<%d> MSDC_INT_RSPCRCERR Arg<0x%.8x>", __func__, host->id, cmd->opcode, cmd->arg);
            //MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
            msdc_reset_hw(host->id);
        } else if (intsts & MSDC_INT_CMDTMO) {
            cmd->error = (unsigned int)-ETIMEDOUT;
            MSDC_DBG_PRINT(MSDC_IRQ, "[%s]: msdc%d XXX CMD<%d> MSDC_INT_CMDTMO Arg<0x%.8x>", __func__, host->id, cmd->opcode, cmd->arg);
            if((cmd->opcode != 52) && (cmd->opcode != 8) && (cmd->opcode != 5) && (cmd->opcode != 55) && (cmd->opcode != 1)) {
                MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
            }
            msdc_reset_hw(host->id);
        }
#ifdef MTK_MSDC_USE_CMD23
        if ((sbc != NULL) && (host->autocmd & MSDC_AUTOCMD23)) {
             if (intsts & MSDC_INT_ACMDRDY) {
                  u32 *arsp = &sbc->resp[0];
                  *arsp = sdr_read32(SDC_ACMD_RESP);
             } else if (intsts & MSDC_INT_ACMDCRCERR) {
                  MSDC_DBG_PRINT(MSDC_STACK, "[%s]: msdc%d, autocmd23 crc error\n", __func__, host->id);
                  sbc->error = (unsigned int)-EIO;
                  cmd->error = (unsigned int)-EIO; // record the error info in current cmd struct
                  //host->error |= REQ_CMD23_EIO;
                  msdc_reset_hw(host->id);
             } else if (intsts & MSDC_INT_ACMDTMO) {
                  MSDC_DBG_PRINT(MSDC_STACK, "[%s]: msdc%d, autocmd23 tmo error\n", __func__, host->id);
                  sbc->error =(unsigned int)-ETIMEDOUT;
                  cmd->error = (unsigned int)-ETIMEDOUT;  // record the error info in current cmd struct
                  MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
                  //host->error |= REQ_CMD23_TMO;
                  msdc_reset_hw(host->id);
             }
        }
#endif /* end of MTK_MSDC_USE_CMD23 */
    }
out:
    host->cmd = NULL;

    return cmd->error;
}
#if 0
static unsigned int msdc_command_resp(struct msdc_host   *host,
                                      struct mmc_command *cmd,
                                      int                 tune,
                                      unsigned long       timeout)
{
    void __iomem *base = host->base;
    u32 opcode = cmd->opcode;
    //u32 resp = host->cmd_rsp;
    //u32 tmo;
    //u32 intsts;

    spin_unlock(&host->lock);
    if(!wait_for_completion_timeout(&host->cmd_done, 10*timeout)){
        MSDC_ERR_PRINT(MSDC_ERROR, "XXX CMD<%d> wait_for_completion timeout ARG<0x%.8x>", opcode, cmd->arg);
        host->sw_timeout++;
        MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
        cmd->error = (unsigned int)-ETIMEDOUT;
        msdc_reset_hw(host->id);
    }
    spin_lock(&host->lock);

    sdr_clr_bits(MSDC_INTEN, wints_cmd);
    host->cmd = NULL;
    /* if (resp == RESP_R1B) {
        while ((sdr_read32(MSDC_PS) & 0x10000) != MSDC_PS_DAT0);
    } */

    return cmd->error;
}
#endif
unsigned int msdc_do_command(struct msdc_host   *host,
                                      struct mmc_command *cmd,
                                      int                 tune,
                                      unsigned long       timeout)
{
    MVG_EMMC_DECLARE_INT32(delay_ns);
    MVG_EMMC_DECLARE_INT32(delay_us);
    MVG_EMMC_DECLARE_INT32(delay_ms);

    if((cmd->opcode == MMC_GO_IDLE_STATE) && (host->hw->host_function == MSDC_SD)){
        mdelay(10);
    }

    MVG_EMMC_ERASE_MATCH(host, (u64)cmd->arg, delay_ms, delay_us, delay_ns, cmd->opcode);

    if (msdc_command_start(host, cmd, tune, timeout))
        goto end;

    MVG_EMMC_ERASE_RESET(delay_ms, delay_us, cmd->opcode);

    if (msdc_command_resp_polling(host, cmd, tune, timeout))
        goto end;
end:

    MSDC_DBG_PRINT(MSDC_STACK, "  return<%d> resp<0x%.8x>", cmd->error, cmd->resp[0]);
    return cmd->error;
}

/* The abort condition when PIO read/write
   tmo:
*/
static int msdc_pio_abort(struct msdc_host *host, struct mmc_data *data, unsigned long tmo)
{
    int  ret = 0;
    void __iomem *base = host->base;

    if (atomic_read(&host->abort)) {
        ret = 1;
    }

    if (time_after(jiffies, tmo)) {
        data->error = (unsigned int)-ETIMEDOUT;
        MSDC_ERR_PRINT(MSDC_ERROR, "XXX PIO Data Timeout: CMD<%d>", host->mrq->cmd->opcode);
        MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
        ret = 1;
    }

    if(ret) {
        msdc_reset_hw(host->id);
        MSDC_ERR_PRINT(MSDC_ERROR, "msdc pio find abort");
    }
    return ret;
}

/*
   Need to add a timeout, or WDT timeout, system reboot.
*/
// pio mode data read/write
int msdc_pio_read(struct msdc_host *host, struct mmc_data *data)
{
    struct scatterlist *sg = data->sg;
    void __iomem *base = host->base;
    u32  num = data->sg_len;
    u32 *ptr;
    u8  *u8ptr;
    u32  left = 0;
    u32  count, size = 0;
    u32  wints = MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR | MSDC_INTEN_XFER_COMPL;
    u32  ints = 0;
    bool get_xfer_done = 0;
    unsigned long tmo = jiffies + DAT_TIMEOUT;
    struct page *hmpage=NULL;
    int i, subpage,totalpages=0;
    int flag=0;
    ulong kaddr[DIV_ROUND_UP(MAX_SGMT_SZ,PAGE_SIZE)];

    BUG_ON(sg == NULL);
    //sdr_clr_bits(MSDC_INTEN, wints);
    while (1) {
        if(!get_xfer_done){
            ints = sdr_read32(MSDC_INT);
            latest_int_status[host->id] = ints;
            ints &= wints;
            sdr_write32(MSDC_INT,ints);
        }
        if(ints & MSDC_INT_DATTMO){
            data->error = (unsigned int)-ETIMEDOUT;
            MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
            msdc_reset_hw(host->id);
            break;
        } else if(ints & MSDC_INT_DATCRCERR){
            data->error = (unsigned int)-EIO;
            //MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
            msdc_reset_hw(host->id);
            //left = sg_dma_len(sg);
            //ptr = sg_virt(sg);
            break;
        } else if(ints & MSDC_INT_XFER_COMPL){
            get_xfer_done = 1;
        }
        if( (get_xfer_done==1) && (num == 0) && (left == 0) )
            break;
        if(msdc_pio_abort(host, data, tmo))
                goto end;
        if((num == 0) && (left == 0))
            continue;
	#ifdef CONFIG_NEED_SG_DMA_LENGTH
		/*
		  * Jie Wu@2014/7/21:
		  * Patch for sg_dma_len,
		  * /kernel-3.10/include/asm-generic/scatterlist.h
		  * #ifdef CONFIG_NEED_SG_DMA_LENGTH
		  * #define sg_dma_len(sg)		((sg)->dma_length)
		  * #else
		  * #define sg_dma_len(sg)		((sg)->length)
		  * #endif
		  * In DMA mode, dma_map_sg() ==> sg->dma_length = sg->length
		  * In PIO mode, dma_map_sg() will not be invoked,
		  * so we need to do sg->dma_length = sg->length before we use the dma scatter list.
		  */
		sg->dma_length = sg->length;
	#endif
        left = sg_dma_len(sg);
        ptr = sg_virt(sg);
        flag = 0;

        if(ptr==NULL||(PageHighMem((struct page *)(sg->page_link & ~0x3))))
        {
            hmpage = (struct page *)(sg->page_link & ~0x3);
            totalpages = DIV_ROUND_UP((left+sg->offset), PAGE_SIZE);
            subpage = (left + sg->offset)%PAGE_SIZE;

            if(subpage!=0|| (sg->offset!=0))
                N_MSG(OPS, "msdc0: This read size or start not align %x,%x, hmpage %p",subpage,left, hmpage);


            for(i=0;i< totalpages;i++)
            {
                kaddr[i] = (ulong) kmap(hmpage + i);
                if(i>0)
                {
                    if((kaddr[i]-kaddr[i-1])!=PAGE_SIZE)
                    {
                        //printk(KERN_ERR "msdc0: kmap not continous %x %x %x \n",left,kaddr[i],kaddr[i-1]);
                        flag =1;
                    }
                }
                if(!kaddr[i])
                    MSDC_ERR_PRINT(MSDC_ERROR, "msdc0:kmap failed %lx", kaddr[i]);

            }


            ptr = sg_virt(sg);

            if(ptr==NULL)
                MSDC_ERR_PRINT(MSDC_ERROR, "msdc0:sg_virt %p", ptr);


#ifdef MTK_MSDC_DUMP_FIFO
			printk("msdc%u: ptr = 0x%p flag = %u (fifocnt: %u:%u)\n",host->id, ptr,flag,msdc_rxfifocnt(),left);
#endif
            if (flag==1)  // High memory and more than 1 va address va and  not continous
            {
                for(i=0;i<totalpages;i++)
                {
                    left = PAGE_SIZE;
                    ptr = (u32*)kaddr[i];

                    if(i==0)
                    {
                        left = PAGE_SIZE-sg->offset;
                        ptr = (u32*)(kaddr[i]+sg->offset);
                    }
                    if((subpage!=0) && (i==(totalpages-1)))
                    {
                        left = subpage;
                    }


                    while (left) {
                        if ((left >=  MSDC_FIFO_THD) && (msdc_rxfifocnt() >= MSDC_FIFO_THD)) {
                            count = MSDC_FIFO_THD >> 2;
                            do {
#ifdef MTK_MSDC_DUMP_FIFO
							   *ptr = msdc_fifo_read32();
								printk("0x%x ", *ptr);
								ptr++;
#else
                                *ptr++ = msdc_fifo_read32();
#endif
                            } while (--count);
                            left -= MSDC_FIFO_THD;
                        } else if ((left < MSDC_FIFO_THD) && msdc_rxfifocnt() >= left) {
                            while (left > 3) {
#ifdef MTK_MSDC_DUMP_FIFO
								*ptr = msdc_fifo_read32();
								printk("0x%x ", *ptr);
								ptr++;
#else
                                *ptr++ = msdc_fifo_read32();
#endif
                                left -= 4;
                            }

                            u8ptr = (u8 *)ptr;
                            while(left) {
#ifdef MTK_MSDC_DUMP_FIFO
                                *u8ptr = msdc_fifo_read8();
                                printk("0x%x ", u8ptr);
								u8ptr++;
#else
                                * u8ptr++ = msdc_fifo_read8();
#endif
                                left--;
                            }
                        }else {
                            ints = sdr_read32(MSDC_INT);
                            latest_int_status[host->id] = ints;

                            if ((ints & MSDC_INT_DATCRCERR) || (ints & MSDC_INT_DATTMO)) {
                                if (ints & MSDC_INT_DATCRCERR){
                                    MSDC_ERR_PRINT(MSDC_ERROR, "[msdc%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                                    data->error = (unsigned int)-EIO;
                                }
                                if (ints & MSDC_INT_DATTMO){
                                    MSDC_ERR_PRINT(MSDC_ERROR, "[msdc%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                                    data->error = (unsigned int)-ETIMEDOUT;
                                }
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
                                if ((host->hw->host_function != MSDC_SDIO) || (atomic_read(&host->ot_work.autok_done) != 0) || (host->mclk <= 50*1000*1000))   // auto-K have done or finished
                                {
                                    MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
                                }
#else
                                MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
#endif
                                sdr_write32(MSDC_INT, ints);
                                msdc_reset_hw(host->id);
                                goto end;
                            }
                        }

                        if (msdc_pio_abort(host, data, tmo)) {
                            goto end;
                        }
                    }
                }
            }

        }
         while (left) {
            if ((left >=  MSDC_FIFO_THD) && (msdc_rxfifocnt() >= MSDC_FIFO_THD)) {
                count = MSDC_FIFO_THD >> 2;
                do {
#ifdef MTK_MSDC_DUMP_FIFO
				 *ptr = msdc_fifo_read32();
                  printk("0x%x ", *ptr);
				  ptr++;
#else
                    *ptr++ = msdc_fifo_read32();
#endif
                } while (--count);
                left -= MSDC_FIFO_THD;
            } else if ((left < MSDC_FIFO_THD) && msdc_rxfifocnt() >= left) {
                while (left > 3) {
#ifdef MTK_MSDC_DUMP_FIFO
					 *ptr = msdc_fifo_read32();
                     printk("0x%x ", *ptr);
					 ptr++;
					 
#else
                    *ptr++ = msdc_fifo_read32();
#endif
                    left -= 4;
                }

                u8ptr = (u8 *)ptr;
                while(left) {
#ifdef MTK_MSDC_DUMP_FIFO
                    *u8ptr = msdc_fifo_read8();
                    printk("0x%x ", *u8ptr);
					u8ptr++;
#else
                    * u8ptr++ = msdc_fifo_read8();
#endif
                    left--;
                }
            }else {
                ints = sdr_read32(MSDC_INT);
                latest_int_status[host->id] = ints;

                if ((ints & MSDC_INT_DATCRCERR) || (ints & MSDC_INT_DATTMO)) {
                    if (ints & MSDC_INT_DATCRCERR){
                        MSDC_ERR_PRINT(MSDC_ERROR, "[msdc%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                        data->error = (unsigned int)-EIO;
                   }
                   if (ints & MSDC_INT_DATTMO){
                        MSDC_ERR_PRINT(MSDC_ERROR, "[msdc%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                        data->error = (unsigned int)-ETIMEDOUT;
                   }

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
                   if ((host->hw->host_function != MSDC_SDIO) || (atomic_read(&host->ot_work.autok_done) != 0) || (host->mclk <= 50*1000*1000))   // auto-K have done or finished
                   {
                       MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
                   }
#else
                   MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
#endif
                   sdr_write32(MSDC_INT, ints);
                   msdc_reset_hw(host->id);
                   goto end;
               }
            }

            if (msdc_pio_abort(host, data, tmo)) {
                goto end;
            }
        }
        if(hmpage !=NULL)
        {
            //printk(KERN_ERR "read msdc0:unmap %x\n", hmpage);
            for(i=0;i< totalpages;i++)
            kunmap(hmpage+i);

            hmpage = NULL;
        }
        size += sg_dma_len(sg);
        sg = sg_next(sg);
        num--;
    }
end:
    if(hmpage !=NULL)
    {
        for(i=0;i< totalpages;i++)
        kunmap(hmpage + i);
        //printk(KERN_ERR "msdc0 read unmap:\n");
    }
    data->bytes_xfered += size;
    N_MSG(FIO, "        PIO Read<%d>bytes", size);

    //sdr_clr_bits(MSDC_INTEN, wints);
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
    if ((is_card_sdio(host)) && (atomic_read(&host->ot_work.autok_done) == 0))   // auto-K have not done or finished
        return data->error;
#endif

    if(data->error)
        MSDC_ERR_PRINT(MSDC_ERROR, "read pio data->error<%d> left<%d> size<%d>", data->error, left, size);
    return data->error;
}

/* please make sure won't using PIO when size >= 512
   which means, memory card block read/write won't using pio
   then don't need to handle the CMD12 when data error.
*/
int msdc_pio_write(struct msdc_host* host, struct mmc_data *data)
{
    void __iomem *base = host->base;
    struct scatterlist *sg = data->sg;
    u32  num = data->sg_len;
    u32 *ptr;
    u8  *u8ptr;
    u32  left;
    u32  count, size = 0;
    u32  wints = MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR | MSDC_INTEN_XFER_COMPL;
    bool get_xfer_done = 0;
    unsigned long tmo = jiffies + DAT_TIMEOUT;
    u32 ints = 0;
    struct page *hmpage = NULL;
    int i, totalpages=0;
    int flag,subpage=0;
    ulong kaddr[DIV_ROUND_UP(MAX_SGMT_SZ,PAGE_SIZE)];

    //sdr_clr_bits(MSDC_INTEN, wints);
    while (1) {
        if(!get_xfer_done){
            ints = sdr_read32(MSDC_INT);
            latest_int_status[host->id] = ints;
            ints &= wints;
            sdr_write32(MSDC_INT,ints);
        }
        if(ints & MSDC_INT_DATTMO){
            data->error = (unsigned int)-ETIMEDOUT;
            MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
            msdc_reset_hw(host->id);
            break;
        }
        else if(ints & MSDC_INT_DATCRCERR){
            data->error = (unsigned int)-EIO;
            //MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
            msdc_reset_hw(host->id);
            break;
        }
        else if(ints & MSDC_INT_XFER_COMPL){
            get_xfer_done = 1;
        }
        if( (get_xfer_done==1) && (num == 0) && (left == 0) )
            break;
        if(msdc_pio_abort(host, data, tmo))
            goto end;
        if((num == 0) && (left == 0))
            continue;
	#ifdef CONFIG_NEED_SG_DMA_LENGTH
		/*
		  * Jie Wu@2014/7/21:
		  * Patch for sg_dma_len,
		  * /kernel-3.10/include/asm-generic/scatterlist.h
		  * #ifdef CONFIG_NEED_SG_DMA_LENGTH
		  * #define sg_dma_len(sg)		((sg)->dma_length)
		  * #else
		  * #define sg_dma_len(sg)		((sg)->length)
		  * #endif
		  * In DMA mode, dma_map_sg() ==> sg->dma_length = sg->length
		  * In PIO mode, dma_map_sg() will not be invoked,
		  * so we need to do sg->dma_length = sg->length before we use the dma scatter list.
		  */
		sg->dma_length = sg->length;
	#endif
        left = sg_dma_len(sg);
        ptr = sg_virt(sg);

        flag = 0;
        // High memory must kmap, if already mapped, only add counter
        if(ptr==NULL||(PageHighMem((struct page *)(sg->page_link & ~0x3))))
        {
            hmpage = (struct page *)(sg->page_link & ~0x3);
            totalpages = DIV_ROUND_UP(left+sg->offset, PAGE_SIZE);
            subpage = (left+sg->offset)%PAGE_SIZE;


            if( (subpage!=0)|| (sg->offset!=0))
                N_MSG(OPS, "msdc0: This write size or start not align %d,%d, hmpage %lx, sg offset %x\n", subpage, left, (ulong) hmpage, sg->offset);


            // Kmap all need pages,
            for(i=0;i< totalpages;i++)
            {
                kaddr[i] = (ulong) kmap(hmpage + i);
                if(i>0)
                {
                    if(kaddr[i]-kaddr[i-1]!=PAGE_SIZE)
                    {
                    //printk(KERN_ERR "msdc0:w kmap not continous %x ,%x, %x\n",left,kaddr[i],kaddr[i-1]);
                    flag =1;
                    }
                }
                if(!kaddr[i])
                    MSDC_ERR_PRINT(MSDC_ERROR, "msdc0:kmap failed %lx\n", kaddr[i]);
            }

            ptr = sg_virt(sg);

            if(ptr==NULL)
                MSDC_ERR_PRINT(MSDC_ERROR, "msdc0:write sg_virt %p\n", ptr);


            // High memory and more than 1 va address va may be not continous
            if (flag==1)
            {
                for(i=0;i< totalpages;i++)
                {

                    left = PAGE_SIZE;
                    ptr = (u32*)kaddr[i];

                    if(i==0)
                    {
                        left = PAGE_SIZE-sg->offset;
                        ptr = (u32*)(kaddr[i]+sg->offset);
                    }
                    if (subpage!=0 && (i==(totalpages-1)))
                    {
                        left = subpage;
                    }

                    while (left) {
                        if (left >= MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
                            count = MSDC_FIFO_SZ >> 2;
                            do {
                                msdc_fifo_write32(*ptr); ptr++;
                            } while (--count);
                            left -= MSDC_FIFO_SZ;
                        } else if (left < MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
                            while (left > 3) {
                                msdc_fifo_write32(*ptr); ptr++;
                                left -= 4;
                            }
                            u8ptr = (u8*)ptr;
                            while(left){
                                msdc_fifo_write8(*u8ptr);    u8ptr++;
                                left--;
                            }
                        } else {
                            ints = sdr_read32(MSDC_INT);
                            latest_int_status[host->id] = ints;

                            if ((ints & MSDC_INT_DATCRCERR) || (ints & MSDC_INT_DATTMO)) {
                                if (ints & MSDC_INT_DATCRCERR){
                                    MSDC_ERR_PRINT(MSDC_ERROR, "[msdc%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                                    data->error = (unsigned int)-EIO;
                                }
                                if (ints & MSDC_INT_DATTMO){
                                    MSDC_ERR_PRINT(MSDC_ERROR, "[msdc%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                                    data->error = (unsigned int)-ETIMEDOUT;
                                }

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
                                if ((host->hw->host_function != MSDC_SDIO) || (atomic_read(&host->ot_work.autok_done) != 0) || (host->mclk <= 50*1000*1000))   // auto-K have done or finished
                                {
                                    MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
                                }
#else
                                MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
#endif
                                sdr_write32(MSDC_INT, ints);
                                msdc_reset_hw(host->id);
                                goto end;
                            }
                        }

                        if (msdc_pio_abort(host, data, tmo)) {
                            goto end;
                        }
                    }
                }
            }
        }
        while (left) {
           if (left >= MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
               count = MSDC_FIFO_SZ >> 2;
               do {
                  msdc_fifo_write32(*ptr); ptr++;
               } while (--count);
               left -= MSDC_FIFO_SZ;
           } else if (left < MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
               while (left > 3) {
                   msdc_fifo_write32(*ptr); ptr++;
                   left -= 4;
               }

               u8ptr = (u8*)ptr;
               while(left){
                  msdc_fifo_write8(*u8ptr);  u8ptr++;
                  left--;
               }
           }else {
                ints = sdr_read32(MSDC_INT);
                latest_int_status[host->id] = ints;

                if ((ints & MSDC_INT_DATCRCERR) || (ints & MSDC_INT_DATTMO)) {
                    if (ints & MSDC_INT_DATCRCERR){
                        MSDC_ERR_PRINT(MSDC_ERROR, "[msdc%d] DAT CRC error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                        data->error = (unsigned int)-EIO;
                   }
                   if (ints & MSDC_INT_DATTMO){
                        MSDC_ERR_PRINT(MSDC_ERROR, "[msdc%d] DAT TMO error (0x%x), Left DAT: %d bytes\n",  host->id, ints, left);
                        data->error = (unsigned int)-ETIMEDOUT;
                   }

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
                   if ((host->hw->host_function != MSDC_SDIO) || (atomic_read(&host->ot_work.autok_done) != 0) || (host->mclk <= 50*1000*1000))   // auto-K have done or finished
                   {
                       MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
                   }
#else
                   MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
#endif
                   sdr_write32(MSDC_INT, ints);
                   msdc_reset_hw(host->id);
                   goto end;
               }
           }

           if (msdc_pio_abort(host, data, tmo)) {
               goto end;
           }
        }
        if(hmpage!=NULL)
        {
            for(i=0;i< totalpages;i++)
            kunmap(hmpage + i);

            hmpage = NULL;

        }
        size += sg_dma_len(sg);
        sg = sg_next(sg);
        num--;
    }
end:
    if(hmpage !=NULL)
    {
        for(i=0;i< totalpages;i++)
        kunmap(hmpage + i);
        printk(KERN_ERR "msdc0 write unmap 0x%x:\n",left);
    }
    data->bytes_xfered += size;
    N_MSG(FIO, "        PIO Write<%d>bytes", size);

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
    if ((is_card_sdio(host)) && (atomic_read(&host->ot_work.autok_done) == 0))   // auto-K have not done or finished
        return data->error;
#endif

    if(data->error) //ERR_MSG("write pio data->error<%d>", data->error);
        MSDC_ERR_PRINT(MSDC_ERROR, "write pio data->error<%d> left<%d> size<%d>", data->error, left, size);

    //sdr_clr_bits(MSDC_INTEN, wints);
    return data->error;
}

static void msdc_dma_start(struct msdc_host *host)
{
    void __iomem *base = host->base;
    u32 wints = MSDC_INTEN_XFER_COMPL | MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR ;

#ifdef MSDC_TOP_RESET_ERROR_TUNE
    if(host->id == 3){ //only msdc0 support AXI bus transaction
        emmc_top_reset = 0;
        //sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_AXIWRAP_CKEN, 0x1); //enable the axi wrapper clock, default on
        //sdr_set_field(EMMC50_CFG2, MSDC_EMMC50_CFG2_AXI_SET_LEN, 0xf); //16 beats per one burst, default 16 beats
        //sdr_set_field(EMMC50_CFG2, MSDC_EMMC50_CFG2_AXI_RD_OUTS_NUM, 0x4); //max 4 for AXI bus
    }
#endif
    if(host->autocmd & MSDC_AUTOCMD12)
        wints |= MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO | MSDC_INT_ACMDRDY;

    /* It will cause error if DAT CRC INT happen but DMA not start */
    sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_START, 1);
    mb();
    sdr_set_bits(MSDC_INTEN, wints);

    N_MSG(DMA, "DMA start");
}

static void msdc_dma_stop(struct msdc_host *host)
{
    void __iomem *base = host->base;
    int retry = 500;
    int count = 1000;
    u32 wints = MSDC_INTEN_XFER_COMPL | MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR ;
    /* handle autocmd12 error in msdc_irq */
    if(host->autocmd & MSDC_AUTOCMD12)
        wints |= MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO | MSDC_INT_ACMDRDY;
    N_MSG(DMA, "DMA status: 0x%.8x",sdr_read32(MSDC_DMA_CFG));
    //while (sdr_read32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS);

    sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_STOP, 1);
    //while (sdr_read32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS);
    msdc_retry((sdr_read32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS),retry,count,host->id);
    if(retry == 0){
        MSDC_ERR_PRINT(MSDC_ERROR, "################################ Failed to stop DMA! please check it here #################################");
        //BUG();
    }
    sdr_clr_bits(MSDC_INTEN, wints); /* Not just xfer_comp */

    N_MSG(DMA, "DMA stop");
}

/* calc checksum */
static u8 msdc_dma_calcs(u8 *buf, u32 len)
{
    u32 i, sum = 0;
    for (i = 0; i < len; i++) {
        sum += buf[i];
    }
    return 0xFF - (u8)sum;
}

/* gpd bd setup + dma registers */
static int msdc_dma_config(struct msdc_host *host, struct msdc_dma *dma)
{
    void __iomem *base = host->base;
    u32 sglen = dma->sglen;
    //u32 i, j, num, bdlen, arg, xfersz;
    u32 j, num, bdlen;
    dma_addr_t dma_address;
    u32 dma_len;
    u8  blkpad, dwpad, chksum;
    struct scatterlist *sg = dma->sg;
    gpd_t *gpd;
    bd_t *bd;

    switch (dma->mode) {
        case MSDC_MODE_DMA_BASIC:
#if defined(FEATURE_MET_MMC_INDEX)
            met_mmc_bdnum = 1;
#endif

            if (host->hw->host_function == MSDC_SDIO)
            {
                BUG_ON(dma->xfersz > 0xFFFFFFFF);
            }
            else
            {
                BUG_ON(dma->xfersz > 65535);
            }
            BUG_ON(dma->sglen != 1);
            dma_address = sg_dma_address(sg);
            dma_len = sg_dma_len(sg);

            N_MSG(DMA, "DMA BASIC mode dma_len<%x> dma_address<%llx>", dma_len, (u64)dma_address);

            sdr_write32(MSDC_DMA_SA, dma_address);

            sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_LASTBUF, 1);
            sdr_write32(MSDC_DMA_LEN, dma_len);
			WARN_ON(dma_len==0);			
            sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BRUSTSZ, dma->burstsz);
            sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 0);
            break;
        case MSDC_MODE_DMA_DESC:
            blkpad = (dma->flags & DMA_FLAG_PAD_BLOCK) ? 1 : 0;
            dwpad  = (dma->flags & DMA_FLAG_PAD_DWORD) ? 1 : 0;
            chksum = (dma->flags & DMA_FLAG_EN_CHKSUM) ? 1 : 0;

            /* calculate the required number of gpd */
            num = (sglen + MAX_BD_PER_GPD - 1) / MAX_BD_PER_GPD;
            BUG_ON(num !=1 );

            gpd = dma->gpd;
            bd  = dma->bd;
            bdlen = sglen;

#if defined(FEATURE_MET_MMC_INDEX)
            met_mmc_bdnum = bdlen;
#endif

            /* modify gpd*/
            //gpd->intr = 0;
            gpd->hwo = 1;  /* hw will clear it */
            gpd->bdp = 1;
            gpd->chksum = 0;  /* need to clear first. */
            gpd->chksum = (chksum ? msdc_dma_calcs((u8 *)gpd, 16) : 0);

            /* modify bd*/
            for (j = 0; j < bdlen; j++) {
#ifdef MSDC_DMA_VIOLATION_DEBUG
                if (g_dma_debug[host->id] && (msdc_latest_operation_type[host->id] == OPER_TYPE_READ)){
                    printk("[%s] msdc%d do write 0x10000\n", __func__, host->id);
                    dma_address = 0x10000;
                } else{
                    dma_address = sg_dma_address(sg);
                }
#else
                dma_address = sg_dma_address(sg);
#endif

                dma_len = sg_dma_len(sg);

                N_MSG(DMA, "DMA DESC mode dma_len<%x> dma_address<%llx>", dma_len, (u64)dma_address);

                msdc_init_bd(&bd[j], blkpad, dwpad, dma_address, dma_len);

                if(j == bdlen - 1) {
                    bd[j].eol = 1;         /* the last bd */
                } else {
                    bd[j].eol = 0;
                }
                bd[j].chksum = 0; /* checksume need to clear first */
                bd[j].chksum = (chksum ? msdc_dma_calcs((u8 *)(&bd[j]), 16) : 0);

                sg++;
            }
#ifdef MSDC_DMA_VIOLATION_DEBUG
            if(g_dma_debug[host->id] && (msdc_latest_operation_type[host->id] == OPER_TYPE_READ))
                g_dma_debug[host->id] = 0;
#endif

            dma->used_gpd += 2;
            dma->used_bd += bdlen;

            #ifdef MSDC_DMA_VIOLATION_DEBUG
            {
                int i;
                u32 *ptr;

                for (i = 0; i < 1; i++) {
                    ptr = (u32*)&gpd[i];
                    N_MSG(DMA, "[SD%d] GD[%d](0x%llxh): %xh %xh %xh %xh %xh %xh %xh\n",
                        host->id, i, (u64)ptr, *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4),
                        *(ptr+5), *(ptr+6));
                }

                for (i = 0; i < bdlen; i++) {
                    ptr = (u32*)&bd[i];
                    N_MSG(DMA, "[SD%d] BD[%d](0x%llxh): %xh %xh %xh %xh\n",
                        host->id, i, (u64)ptr, *ptr, *(ptr+1), *(ptr+2), *(ptr+3));
                }

            }
            #endif

            sdr_set_field(MSDC_DMA_CFG, MSDC_DMA_CFG_DECSEN, chksum);
            sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BRUSTSZ, dma->burstsz);
            sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 1);

            sdr_write32(MSDC_DMA_SA, (u32)dma->gpd_addr);
            break;

        default:
            break;
    }

    N_MSG(DMA, "DMA_CTRL = 0x%x", sdr_read32(MSDC_DMA_CTRL));
    N_MSG(DMA, "DMA_CFG  = 0x%x", sdr_read32(MSDC_DMA_CFG));
    N_MSG(DMA, "DMA_SA   = 0x%x", sdr_read32(MSDC_DMA_SA));

    return 0;
}

static void msdc_dma_setup(struct msdc_host *host, struct msdc_dma *dma,
                           struct scatterlist *sg, unsigned int sglen)
{
    u32 max_dma_len = 0;
    BUG_ON(sglen > MAX_BD_NUM); /* not support currently */

    dma->sg = sg;
    dma->flags = DMA_FLAG_EN_CHKSUM;
    //dma->flags = DMA_FLAG_NONE; /* CHECKME */
    dma->sglen = sglen;
    dma->xfersz = host->xfer_size;
    dma->burstsz = MSDC_BRUST_64B;

    if (host->hw->host_function == MSDC_SDIO)
    {
        max_dma_len = MAX_DMA_CNT_SDIO;
    }
    else
    {
        max_dma_len = MAX_DMA_CNT;
    }

    if (sglen == 1 && sg_dma_len(sg) <= max_dma_len){
            dma->mode = MSDC_MODE_DMA_BASIC;
    } else
        dma->mode = MSDC_MODE_DMA_DESC;

    N_MSG(DMA, "DMA mode<%d> sglen<%d> xfersz<%d>", dma->mode, dma->sglen, dma->xfersz);

    msdc_dma_config(host, dma);
}

/* set block number before send command */
static void msdc_set_blknum(struct msdc_host *host, u32 blknum)
{
    void __iomem *base = host->base;
	WARN_ON(blknum==0);
    sdr_write32(SDC_BLK_NUM, blknum);
}

#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)
/* get block number before send command */
static int msdc_get_blknum(struct msdc_host *host)
{
    void __iomem *base = host->base;
    u32 blknum;
	WARN_ON(blknum==0);
    blknum = sdr_read32(SDC_BLK_NUM);

    return blknum;
}
#endif    // #if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)


#define REQ_CMD_EIO  (0x1 << 0)
#define REQ_CMD_TMO  (0x1 << 1)
#define REQ_DAT_ERR  (0x1 << 2)
#define REQ_STOP_EIO (0x1 << 3)
#define REQ_STOP_TMO (0x1 << 4)
#define REQ_CMD23_EIO (0x1 << 5)
#define REQ_CMD23_TMO (0x1 << 6)
static void msdc_restore_info(struct msdc_host *host){
    void __iomem *base = host->base;
    int retry = 3;
    msdc_reset_hw(host->id);
    host->saved_para.msdc_cfg = host->saved_para.msdc_cfg & 0x03CFFF3B; //force bit5(BV18SDT) to 0
    sdr_write32(MSDC_CFG,host->saved_para.msdc_cfg);

    while(retry--){
        msdc_set_mclk(host, host->saved_para.state, host->saved_para.hz);
        if((sdr_read32(MSDC_CFG)&0x03CFFF3B) != (host->saved_para.msdc_cfg&0x03CFFF3B)){
            MSDC_ERR_PRINT(MSDC_ERROR, "msdc set_mclk is unstable (cur_cfg=%x, save_cfg=%x, cur_hz=%d, save_hz=%d).", sdr_read32(MSDC_CFG), host->saved_para.msdc_cfg, host->mclk, host->saved_para.hz);
        } else
            break;
    }

    sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,    host->saved_para.int_dat_latch_ck_sel); //for SDIO 3.0
    sdr_set_field(MSDC_PATCH_BIT0,  MSDC_PB0_CKGEN_MSDC_DLY_SEL, host->saved_para.ckgen_msdc_dly_sel); //for SDIO 3.0
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->saved_para.cmd_resp_ta_cntr);//for SDIO 3.0
    sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr); //for SDIO 3.0
    sdr_write32(MSDC_DAT_RDDLY0,host->saved_para.ddly0);
    sdr_write32(MSDC_PAD_TUNE,host->saved_para.pad_tune);
    sdr_write32(SDC_CFG,host->saved_para.sdc_cfg);
    sdr_set_field(MSDC_INTEN, MSDC_INT_SDIOIRQ, host->saved_para.inten_sdio_irq); //get INTEN status for SDIO
    sdr_write32(MSDC_IOCON,host->saved_para.iocon);
}

#if defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)


static unsigned int ot_find_next_gear(u32 CRCResult, const u32 tune_wind_size, const u32 cur_gear, u32 *fTestedGear)
{
    int i;
	u32 new_gear = cur_gear;
	u32 test_gear_range = 0xFFFFFFFF >> (32 - (2 * tune_wind_size + 1));

    if(CRCResult & 0x1)	// lsb bit CRC pass, go to smaller gear
    {
        for(i = 1; i <= 2*tune_wind_size; i++)	// Scan from least gear to most gear
        {
            if(((CRCResult>>i)&1) == 0)
            {
                break;
            }
        }

        new_gear = (cur_gear + 32 - (2 * tune_wind_size + 1 - i)) & 0x1F;
    }
    else	// msb bit CRC pass or CRC fail on both side, go to bigger gear
    {
        for(i = 2*tune_wind_size; i >= 0; i--)	// Scan from most gear to least gear
        {
            if(((CRCResult>>i)&1) == 0)
            {
                break;
            }
        }

        new_gear = (cur_gear + i + 1) & 0x1F;
    }

    if(new_gear >= tune_wind_size)
        *fTestedGear |= (test_gear_range << (new_gear - tune_wind_size));
    else
        *fTestedGear |= (test_gear_range >> (tune_wind_size - new_gear));

	MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning][%s], cur_gear = 0x%x, new_gear = 0x%x, tune_wind_size = 0x%x, CRCResult = 0x%x\n", __func__, cur_gear, new_gear, tune_wind_size, CRCResult);

    return new_gear;
}

static int ot_do_command(struct msdc_host *host, u32 rawcmd, u32 rawarg, u32 *pintsts);

#if 0
static int ot_recover_device(struct msdc_host *host, unsigned fn, unsigned addr)
{
#if 0
    void __iomem *base = host->base;

    MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]Enter [%s]\n", __func__);

    sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, 1); // Clock is free running

    msleep(1);

    sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, 0); // Clock is gated to 0 if no command or data is transmitted.
    msdc_reset_hw(host->id);
#endif
    return 0;
}
#endif

static int ot_command_resp_polling(struct msdc_host *host, struct mmc_command *cmd, int tune, unsigned long timeout, u32 *pintsts)
{
    void __iomem *base = host->base;

    u32 intsts;
    u32 resp;
    unsigned long tmo;

    u32 acmd53done = MSDC_INT_ACMDTMO | MSDC_INT_DATTMO | MSDC_INT_ACMD53_DONE | MSDC_INT_CMDRDY | MSDC_INT_CMDTMO | MSDC_INT_RSPCRCERR;
    u32 cmdsts = MSDC_INT_CMDTMO | MSDC_INT_ACMDTMO | MSDC_INT_DATTMO | MSDC_INT_ACMD53_DONE | MSDC_INT_GEAR_OUT_BOUND | MSDC_INT_ACMDCRCERR | MSDC_INT_ACMD53_FAIL | MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR;
    u32 flagTmo = MSDC_INT_ACMDTMO | MSDC_INT_DATTMO;

    resp = host->cmd_rsp;

    /*polling*/
    tmo = jiffies + timeout;

    while (1){
        if (((intsts = sdr_read32(MSDC_INT)) & acmd53done) != 0){
            /* clear all int flag */
            intsts &= cmdsts;
            sdr_write32(MSDC_INT, intsts);
            break;
        }

        if (time_after(jiffies, tmo)) {
            cmd->error = (unsigned int)-ETIMEDOUT;
            host->sw_timeout++;
            MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning][%s] SW timeout, intsts = 0x%x\n", __func__, intsts);
            MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
            msdc_reset_hw(host->id);
            goto out;
        }
    }

    /* command interrupts */
    if((intsts & MSDC_INT_ACMD53_DONE) || (intsts & flagTmo))
    {
        cmd->resp[0] = sdr_read32(DAT0_TUNE_CRC);
        cmd->resp[1] = sdr_read32(DAT1_TUNE_CRC);
        cmd->resp[2] = sdr_read32(DAT2_TUNE_CRC);
        cmd->resp[3] = sdr_read32(DAT3_TUNE_CRC);
    }

    /* timeout */
    if (intsts & flagTmo)
    {
        MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]Device CMD/DAT timeout, MSDC_INT = 0x%x\n", intsts);
        cmd->error = (unsigned int)-ETIMEDOUT;
        msdc_reset_hw(host->id);
    }

out:
    *pintsts = intsts;

    return cmd->error;
}

static u32 wints_cmd_testmode = MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR | MSDC_INT_CMDTMO |
                       MSDC_INT_ACMDRDY | MSDC_INT_ACMDTMO |
                       MSDC_INT_DATTMO | MSDC_INT_GEAR_OUT_BOUND |
                       MSDC_INT_ACMD53_DONE | MSDC_INT_ACMDCRCERR | MSDC_INT_DATCRCERR | MSDC_INT_ACMD53_FAIL;

static int ot_do_command(struct msdc_host *host, u32 rawcmd, u32 rawarg, u32 *pintsts)
{
    void __iomem *base = host->base;
    struct mmc_command *cmd = host->cmd;

    unsigned long tmo;

    cmd->error = 0;

    tmo = jiffies + CMD_TIMEOUT;
    while(sdc_is_cmd_busy() || sdc_is_busy())
    {
        if (time_after(jiffies, tmo)) {
            if(sdc_is_cmd_busy())
                MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]XXX sdc_cmd_busy timeout\n");
            else if(sdc_is_busy())
                MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]XXX sdc_busy timeout\n");
            cmd->error = (unsigned int)-ETIMEDOUT;
            msdc_reset_hw(host->id);
            return cmd->error;  /* Fix me: error handling */
        }
    }

    /* use polling way */
    sdr_clr_bits(MSDC_INTEN, wints_cmd_testmode);

    sdc_send_cmd(rawcmd, rawarg);
    if (ot_command_resp_polling(host, cmd, 0, CMD_TIMEOUT, pintsts))
    {
        MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]ot_command_resp_polling fail, err = %d, resp = 0x%.8x\n", cmd->error, cmd->resp[0]);
        return cmd->error;
    }

    return 0;
}

static unsigned int ot_init(struct msdc_host *host, struct ot_data *potData)
{
    void __iomem *base = host->base;
    u32 rxdly0;
    //struct mmc_host *mmc = host->mmc;

    potData->retry = 0;

    /* reset dly pass */
    potData->cmddlypass = 0;
    potData->dat0rddlypass = 0;
    potData->dat1rddlypass = 0;
    potData->dat2rddlypass = 0;
    potData->dat3rddlypass = 0;

    /* reset testedGear */
    potData->fCmdTestedGear = 0;
    potData->fDatTestedGear = 0;
    potData->fDat0TestedGear = 0;
    potData->fDat1TestedGear = 0;
    potData->fDat2TestedGear = 0;
    potData->fDat3TestedGear = 0;

    potData->datrddly=0;
    potData->orig_paddatwrdly=0;
    potData->datrddlypass=0;
    potData->orig_paddatrddly=0;

    /* Get block number */
    potData->orig_blknum = msdc_get_blknum(host);

    /* Get PATCH_BIT0 value */
    potData->orig_patch_bit0 = sdr_read32(MSDC_PATCH_BIT0);

    /* Get MSDC_IOCON value */
    potData->orig_iocon = sdr_read32(MSDC_IOCON);

    /* Get DMA status */
    potData->orig_dma = msdc_dma_status();

    /* Get eco_ver */
    potData->eco_ver = sdr_read32(MSDC_ECO_VER);

    /* Get current Data Timeout Counter setting and set to 1048576*0x30 ticks */
    sdr_get_field(SDC_CFG, SDC_CFG_DTOC, potData->orig_dtoc);    // Get Data Timeout Counter
    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, 0x10);

    /* Set block number to 1 */
    msdc_set_blknum(host, 1);

    /* Set online tuning interrupt behavior */
    sdr_set_field(MSDC_PATCH_BIT0, MSDC_MASK_ACMD53_CRC_ERR_INTR, 1);
    sdr_set_field(MSDC_PATCH_BIT0, MSDC_ACMD53_FAIL_ONE_SHOT, 1);

    /* Get current data line delay select */
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, potData->orig_ddlsel);    // Get cmd delay

    /* Set DAT line behavior */
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 1);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 1);

    /* Set PIO mode */
    msdc_dma_off();

    /* Set gear window size */
    sdr_write32(SDIO_TUNE_WIND, potData->tune_wind_size);
    MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]gear window size = %d\n", potData->tune_wind_size);

    /* Get current delay settings */
    sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, potData->orig_cmdrdly);    // Get cmd delay
    potData->cmdrdly = potData->orig_cmdrdly;
    MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]cmd delay = 0x%x\n", potData->orig_cmdrdly);

    rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
    MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]DAT read delay line 0 = 0x%x\n", rxdly0);
    if (potData->eco_ver >= 4) {
        potData->orig_dat0rddly = (rxdly0 >> 24) & 0x1F;
        potData->orig_dat1rddly = (rxdly0 >> 16) & 0x1F;
        potData->orig_dat2rddly = (rxdly0 >>  8) & 0x1F;
        potData->orig_dat3rddly = (rxdly0 >>  0) & 0x1F;
    } else {
        potData->orig_dat0rddly = (rxdly0 >>  0) & 0x1F;
        potData->orig_dat1rddly = (rxdly0 >>  8) & 0x1F;
        potData->orig_dat2rddly = (rxdly0 >> 16) & 0x1F;
        potData->orig_dat3rddly = (rxdly0 >> 24) & 0x1F;
    }

    if(potData->orig_ddlsel == 0)
    {
        sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATRRDLY, potData->orig_paddatrddly);    // Get read delay
        MSDC_DBG_PRINT(MSDC_TUNING, "[SDIO_OnlineTuning]PAD DAT RD RXDLY = 0x%x\n", potData->orig_paddatrddly);
        potData->dat0rddly = potData->orig_paddatrddly;
        potData->dat1rddly = potData->orig_paddatrddly;
        potData->dat2rddly = potData->orig_paddatrddly;
        potData->dat3rddly = potData->orig_paddatrddly;
    }
    else
    {
        potData->dat0rddly = potData->orig_dat0rddly;
        potData->dat1rddly = potData->orig_dat1rddly;
        potData->dat2rddly = potData->orig_dat2rddly;
        potData->dat3rddly = potData->orig_dat3rddly;
    }

    /* Setup rawcmd */
    potData->rawcmd = 0;
    potData->rawcmd |= SD_IO_RW_EXTENDED;
    potData->rawcmd |= msdc_rsp[RESP_R1] << 7;
    potData->rawcmd |= 1 << 11;                        // Single block transmission
    potData->rawcmd |= host->blksz << 16;              // Block length
    potData->rawcmd |= 1 << 31;                        // Autocmd53

    /* Setup rawarg */
    potData->rawarg = 0;
    potData->rawarg |= potData->fn << 28;
    potData->rawarg |= potData->addr << 9;
    //potData->rawarg |= 0x08000000;                    // Block mode
    //potData->rawarg |= 0x00000001;                    // Single block
    potData->rawarg |= host->blksz;                    // Transfer one block size

    return 0;
}

static unsigned int ot_deinit(struct msdc_host *host, struct ot_data otData)
{
    void __iomem *base = host->base;

    /* Set orignal block number */
    msdc_set_blknum(host, otData.orig_blknum);

    /* Set original PATCH_BIT0 value */
    sdr_write32(MSDC_PATCH_BIT0, otData.orig_patch_bit0);

    /* Set original MSDC_IOCON value */
    sdr_write32(MSDC_IOCON, otData.orig_iocon);

    /* Set original DTOC */
    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, otData.orig_dtoc);

    /* Set original DMA status */
    if(otData.orig_dma == DMA_ON)
        msdc_dma_on();
    else if(otData.orig_dma == DMA_OFF)
        msdc_dma_off();

    return 0;
}

static int ot_adjust_tunewnd(struct msdc_host *host, u32 *fTestedGear, u32 *tuneWnd, u32 *CRCPass)
{
    if(*fTestedGear == 0xFFFFFFFF)
    {
        *fTestedGear = 0;
#if 1
        return -1;
#else
        *tuneWnd = *tuneWnd - 1;

        /* Set gear window size */
        sdr_write32(SDIO_TUNE_WIND, *tuneWnd);
        MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning][%s] gear window size = %d\n", __func__, *tuneWnd);

        if(*tuneWnd == 0)
            return -1;


        *CRCPass = 0xFFFFFFFF >> (32 - (2 * (*tuneWnd) + 1));
        //MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]CRCPass = %d\n", *CRCPass);
#endif
    }

	return 0;
}

static int ot_process(struct msdc_host *host, struct ot_data *potData, u32 rawcmd, u32 rawarg, u32 CRCPass, bool rw)
{
    void __iomem *base = host->base;
	u32 CRCStatus_CMD=0;
    u32 rxdly0;
    int reset;
    u32 intsts = 0;
	u32 ret = 0;

	return -1;

    if(rw == 0)	// Read
    {
        rawcmd &= ~(1 << 13);
        rawarg &= ~(1 << 31);
    }
    else	// Write
    {
        rawcmd |= 1 << 13;
        rawarg |= 1 << 31;
    }

    MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]enter command start, rw = %d, rawcmd = 0x%x, rawarg = 0x%x\n", rw, rawcmd, rawarg);
    if (ot_do_command(host, rawcmd, rawarg, &intsts) != 0) {
        MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]command start fail, err = %d\n", host->cmd->error);
        ret = host->cmd->error;
#if 0
        if(ret == (unsigned int)-ETIMEDOUT)
        {
            int ret1 = 0;

            /* Get CMD CRC status */
            CRCStatus_CMD = sdr_read32(CMD_TUNE_CRC);

            if( (ret1 = ot_recover_device(host, potData->fn, potData->addr)) == 0)    // if ot_recover_device returns 0, means device recover success, continue online tuning
            {
                if((CRCStatus_CMD != CRCPass) && (potData->cmddlypass == 0))
                {
                    /* Reset delay gear */
                    potData->cmdrdly = ot_find_next_gear(CRCStatus_CMD, potData->tune_wind_size, potData->cmdrdly, &potData->fCmdTestedGear);
                    sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, potData->cmdrdly);    // Set cmd delay
                    MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning](TMO) CRCStatus_CMD = 0x%x, next cmd delay = %d \n", CRCStatus_CMD, potData->cmdrdly);
                }

                reset = 0;

                if((host->cmd->resp[0] != CRCPass) && (potData->dat0rddlypass == 0))
                {
                    /* Reset delay gear */
                    if(potData->orig_ddlsel == 1)
                        potData->dat0rddly = ot_find_next_gear(host->cmd->resp[0], potData->tune_wind_size, potData->dat0rddly, &potData->fDat0TestedGear);
                    reset++;
                    MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning](TMO) host->cmd->resp[0] = 0x%x, next dat0 delay = %d \n", host->cmd->resp[0], potData->dat0rddly);
                }

                if((host->cmd->resp[1] != CRCPass) && (potData->dat1rddlypass == 0))
                {
                    /* Reset delay gear */
                    if(potData->orig_ddlsel == 1)
                        potData->dat1rddly = ot_find_next_gear(host->cmd->resp[1], potData->tune_wind_size, potData->dat1rddly, &potData->fDat1TestedGear);
                    reset++;
                    MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning](TMO) host->cmd->resp[1] = 0x%x, next dat1 delay = %d \n", host->cmd->resp[1], potData->dat1rddly);
                }

                if((host->cmd->resp[2] != CRCPass) && (potData->dat2rddlypass == 0))
                {
                    /* Reset delay gear */
                    if(potData->orig_ddlsel == 1)
                        potData->dat2rddly = ot_find_next_gear(host->cmd->resp[2], potData->tune_wind_size, potData->dat2rddly, &potData->fDat2TestedGear);
                    reset++;
                    MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning](TMO) host->cmd->resp[2] = 0x%x, next dat2 delay = %d \n", host->cmd->resp[2], potData->dat2rddly);
                }

                if((host->cmd->resp[3] != CRCPass) && (potData->dat3rddlypass == 0))
                {
                    /* Reset delay gear */
                    if(potData->orig_ddlsel == 1)
                        potData->dat3rddly = ot_find_next_gear(host->cmd->resp[3], potData->tune_wind_size, potData->dat3rddly, &potData->fDat3TestedGear);
                    reset++;
                    MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning](TMO) host->cmd->resp[3] = 0x%x, next dat3 delay = %d \n", host->cmd->resp[3], potData->dat3rddly);
                }

                if(reset) {
                    if(potData->orig_ddlsel == 0)
                    {
                        u32 crcsts = (host->cmd->resp[0] & host->cmd->resp[1] & host->cmd->resp[2] & host->cmd->resp[3]);
                        u32 datrddly = ot_find_next_gear(crcsts, potData->tune_wind_size, potData->dat0rddly, &potData->fDatTestedGear);
                        potData->dat0rddly = datrddly;
                        potData->dat1rddly = datrddly;
                        potData->dat2rddly = datrddly;
                        potData->dat3rddly = datrddly;
                    }
                    /* Reset delay gear */
                    if (potData->eco_ver >= 4) {
                        rxdly0 = (potData->dat0rddly << 24) | (potData->dat1rddly << 16) | (potData->dat2rddly << 8) | (potData->dat3rddly << 0);
                    } else {
                        rxdly0 = (potData->dat0rddly << 0) | (potData->dat1rddly << 8) | (potData->dat2rddly << 16) | (potData->dat3rddly << 24);
                    }
                    sdr_write32(MSDC_DAT_RDDLY0, rxdly0);
                }

                MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]goto _retry (TMO) \n");
                potData->retry = 1;
                return 0;
            }
        }
#endif
        return ret;
    }

    if((intsts & MSDC_INT_ACMD53_FAIL) == 0)    // All CRC Pass
    {
       MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]CRC Pass, intsts = 0x%x\n", intsts);
        return 0;
    }

    /* Get CMD CRC status */
    CRCStatus_CMD = sdr_read32(CMD_TUNE_CRC);

    /* Check cmd gear */
    if(CRCStatus_CMD != CRCPass)
    {
        /* Reset delay gear */
        //printk("[SDIO_OnlineTuning]cmdrdly = %d, CRCStatus_CMD = 0x%x, CRC Pass = 0x%x\n", cmdrdly, CRCStatus_CMD, CRCPass);

        potData->cmdrdly = ot_find_next_gear(CRCStatus_CMD, potData->tune_wind_size, potData->cmdrdly, &potData->fCmdTestedGear);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, potData->cmdrdly);    // Set cmd delay

        MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]goto _retry (Cmd), cmd delay = %d \n", potData->cmdrdly);
        potData->retry = 1;
        return 0;
    }
    else
    {
        potData->cmddlypass = 1;
    }

    reset = 0;
    /* Check read gear */
    if(potData->orig_ddlsel == 0)
    {
        u32 crcsts = (host->cmd->resp[0] & host->cmd->resp[1] & host->cmd->resp[2] & host->cmd->resp[3]);
        if(crcsts != CRCPass)
        {
            u32 datrddly = ot_find_next_gear(crcsts, potData->tune_wind_size, potData->dat0rddly, &potData->fDatTestedGear);
            potData->dat0rddly = datrddly;
            potData->dat1rddly = datrddly;
            potData->dat2rddly = datrddly;
            potData->dat3rddly = datrddly;
        }
        else
        {
            potData->datrddlypass = 1;
        }
    }
    else
    {
        if(host->cmd->resp[0] != CRCPass)
        {
            //MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]dat0 delay = %d, host->cmd->resp[0] = 0x%x, CRC Pass = 0x%x\n", dat0rddly, host->cmd->resp[0], CRCPass);
            potData->dat0rddly = ot_find_next_gear(host->cmd->resp[0], potData->tune_wind_size, potData->dat0rddly, &potData->fDat0TestedGear);
            reset++;
        }
        else
        {
            potData->dat0rddlypass = 1;
        }

        if(host->cmd->resp[1] != CRCPass)
        {
            //MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]dat1 delay = %d, host->cmd->resp[1] = 0x%x, CRC Pass = 0x%x\n", dat1rddly, host->cmd->resp[1], CRCPass);
            potData->dat1rddly = ot_find_next_gear(host->cmd->resp[1], potData->tune_wind_size, potData->dat1rddly, &potData->fDat1TestedGear);
            reset++;
        }
        else
        {
            potData->dat1rddlypass = 1;
        }

        if(host->cmd->resp[2] != CRCPass)
        {
            //MSDC_DBG_PRINT(MSDC_TUNING, "[SDIO_OnlineTuning]dat2 delay = %d, host->cmd->resp[2] = 0x%x, CRC Pass = 0x%x\n", dat2rddly, host->cmd->resp[2], CRCPass);
            potData->dat2rddly = ot_find_next_gear(host->cmd->resp[2], potData->tune_wind_size, potData->dat2rddly, &potData->fDat2TestedGear);
            reset++;
        }
        else
        {
            potData->dat2rddlypass = 1;
        }

        if(host->cmd->resp[3] != CRCPass)
        {
            //MSDC_DBG_PRINT(MSDC_TUNING, "[SDIO_OnlineTuning]dat3 delay = %d, host->cmd->resp[3] = 0x%x, CRC Pass = 0x%x\n", dat3rddly, host->cmd->resp[3], CRCPass);
            potData->dat3rddly = ot_find_next_gear(host->cmd->resp[3], potData->tune_wind_size, potData->dat3rddly, &potData->fDat3TestedGear);
            reset++;
        }
        else
        {
            potData->dat3rddlypass = 1;
        }
    }

    if(reset) {
        /* Reset delay gear */
        if (potData->eco_ver >= 4) {
            rxdly0 = (potData->dat0rddly << 24) | (potData->dat1rddly << 16) | (potData->dat2rddly << 8) | (potData->dat3rddly << 0);
        } else {
            rxdly0 = (potData->dat0rddly << 0) | (potData->dat1rddly << 8) | (potData->dat2rddly << 16) | (potData->dat3rddly << 24);
        }
        sdr_write32(MSDC_DAT_RDDLY0, rxdly0);

       MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]goto _retry (Read) \n");
        potData->retry = 1;
        return 0;
    }

	return 0;
}

#endif  // defined(MTK_SDIO30_ONLINE_TUNING_SUPPORT) || defined(ONLINE_TUNING_DVTTEST)

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT

extern unsigned int autok_get_current_vcore_offset(void);
extern int msdc_autok_stg1_data_get(void **ppData, int *pLen);
extern int msdc_autok_apply_param(struct msdc_host *host, unsigned int vcore_uv);
//extern void mt_cpufreq_disable(unsigned int type, bool disabled);
extern void mt_vcore_dvfs_disable_by_sdio(unsigned int type, bool disabled);

static unsigned int msdc_online_tuning(struct msdc_host *host, unsigned fn, unsigned addr)
{
    void __iomem *base = host->base;
    u32 rxdly0;
    u32 rawcmd;
    u32 rawarg;
    u32 tune_wind_size_cmd = OT_START_TUNEWND;
    u32 tune_wind_size_dat = OT_START_TUNEWND;
	u32 tune_wind_size_dat0 = OT_START_TUNEWND;
	u32 tune_wind_size_dat1 = OT_START_TUNEWND;
	u32 tune_wind_size_dat2 = OT_START_TUNEWND;
	u32 tune_wind_size_dat3 = OT_START_TUNEWND;
    u32 CRCPass = 0xFFFFFFFF >> (32 - (2 * OT_START_TUNEWND + 1));

    u32 vcore_uv_off = 0;
    int ret = 0;

    unsigned long tmo;

    struct ot_data otData;
    struct mmc_command *orig_cmd = host->cmd;
    struct mmc_command cmd;

    MSDC_DBG_PRINT(MSDC_TUNING, "[SDIO_OnlineTuning]Enter %s \n", __func__);

    /* ungate clock */
    msdc_ungate_clock(host);  // set sw flag

    /* Claim host */
    if(host->ot_work.chg_volt == 0)
    {
        autok_claim_host(host);
    }

    atomic_set(&host->ot_done, 0);
	//mt_cpufreq_disable(1, true);
	mt_vcore_dvfs_disable_by_sdio(1, true);

    vcore_uv_off = autok_get_current_vcore_offset();
    // read auto-K result from auto-K callback function
    //msdc_autok_stg1_data_get(&msdc_param, &len);
    // apply MSDC parameter for current vcore
    //if(len != 0)
    msdc_autok_apply_param(host, vcore_uv_off);

    host->cmd = &cmd;
    host->blksz = OT_BLK_SIZE;

    otData.fn = fn;
    otData.addr = addr;
    otData.tune_wind_size = OT_START_TUNEWND;
    ot_init(host, &otData);
    rawcmd = otData.rawcmd;
    rawarg = otData.rawarg;

    /* Set timeout */
    tmo = jiffies + OT_TIMEOUT;

_retry:

    otData.retry = 0;

    if (time_after(jiffies, tmo))
    {
        MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]online tuning timeout\n");

        ret = -1;

        goto out;
    }

    if(otData.cmddlypass == 0)
    {
        MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning][%s]fCmdTestedGear = 0x%x\n", __func__, otData.fCmdTestedGear);
        ret = ot_adjust_tunewnd(host, &otData.fCmdTestedGear, &tune_wind_size_cmd, &CRCPass);
        otData.tune_wind_size = tune_wind_size_cmd;
        if(ret == -1)
            goto out;
    }
    else if(otData.orig_ddlsel == 0)
    {
        if(otData.datrddlypass == 0)
        {
            MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning][%s]fDatTestedGear = 0x%x\n", __func__, otData.fDatTestedGear);
            ret = ot_adjust_tunewnd(host, &otData.fDatTestedGear, &tune_wind_size_dat, &CRCPass);
            otData.tune_wind_size = tune_wind_size_dat;
            if(ret == -1)
                goto out;
        }
    }
    else
    {
        if(otData.dat0rddlypass == 0)
        {
            MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning][%s]fDat0TestedGear = 0x%x\n", __func__, otData.fDat0TestedGear);
            ret = ot_adjust_tunewnd(host, &otData.fDat0TestedGear, &tune_wind_size_dat0, &CRCPass);
            otData.tune_wind_size = tune_wind_size_dat0;
            if(ret == -1)
                goto out;
        }
        else if(otData.dat1rddlypass == 0)
        {
            MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning][%s]fDat1TestedGear = 0x%x\n", __func__, otData.fDat1TestedGear);
            ret = ot_adjust_tunewnd(host, &otData.fDat1TestedGear, &tune_wind_size_dat1, &CRCPass);
            otData.tune_wind_size = tune_wind_size_dat1;
            if(ret == -1)
                goto out;
        }
        else if(otData.dat2rddlypass == 0)
        {
            MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning][%s]fDat2TestedGear = 0x%x\n", __func__, otData.fDat2TestedGear);
            ret = ot_adjust_tunewnd(host, &otData.fDat2TestedGear, &tune_wind_size_dat2, &CRCPass);
            otData.tune_wind_size = tune_wind_size_dat2;
            if(ret == -1)
                goto out;
        }
        else if(otData.dat3rddlypass == 0)
        {
            MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning][%s]fDat3TestedGear = 0x%x\n", __func__, otData.fDat3TestedGear);
            ret = ot_adjust_tunewnd(host, &otData.fDat3TestedGear, &tune_wind_size_dat3, &CRCPass);
            otData.tune_wind_size = tune_wind_size_dat3;
            if(ret == -1)
                goto out;
        }
    }

    // Process read
    ret = ot_process(host, &otData, rawcmd, rawarg, CRCPass, 0);

    if(ret < 0)
        goto out;

    if(otData.retry == 1)
    {
        goto _retry;
    }

#if 0
    // Process write
    ret = ot_process(host, &otData, rawcmd, rawarg, CRCPass, 1);

    if(ret < 0)
        goto out;

    if(otData.retry == 1)
    {
        goto _retry;
    }
#endif

out:

    if(ret < 0)
    {
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, otData.orig_cmdrdly);    // Set orig cmd delay gear

        /* set orig dat delay gear */
        if (otData.eco_ver >= 4) {
            rxdly0 = (otData.orig_dat0rddly << 24) | (otData.orig_dat1rddly << 16) | (otData.orig_dat2rddly << 8) | (otData.orig_dat3rddly << 0);
        } else {
            rxdly0 = (otData.orig_dat0rddly << 0) | (otData.orig_dat1rddly << 8) | (otData.orig_dat2rddly << 16) | (otData.orig_dat3rddly << 24);
        }
        sdr_write32(MSDC_DAT_RDDLY0, rxdly0);

        MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]ret = %d, vcore_uv_off = 0x%x, cmd delay = 0x%x\n", ret, vcore_uv_off, otData.orig_cmdrdly);
    }
    else if(otData.orig_ddlsel == 0)
    {
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATRRDLY, otData.dat0rddly);    // Set read delay
        MSDC_DBG_PRINT(MSDC_TUNING,"[SDIO_OnlineTuning]ret = %d, dat delay = 0x%x \n", ret, otData.dat0rddly);
    }

    ot_deinit(host, otData);

    /* gate clock */
    msdc_gate_clock(host, 1); // clear flag.

    host->cmd = orig_cmd;

    /* release host */
    host->ot_work.chg_volt = 0;
    atomic_set(&host->sdio_stopping, 0);
    complete(&host->ot_work.ot_complete);
    autok_release_host(host);

    atomic_set(&host->ot_done, 1);

	//mt_cpufreq_disable(1, false);
	mt_vcore_dvfs_disable_by_sdio(1, false);

    return ret;
}

#endif    // MTK_SDIO30_ONLINE_TUNING_SUPPORT

#ifdef ONLINE_TUNING_DVTTEST
#include "mt_online_tuning_test.c"
#endif

static int msdc_do_request(struct mmc_host*mmc, struct mmc_request*mrq)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;
    struct mmc_data *data;
    u32 l_autocmd23_is_set = 0;
#ifdef MTK_MSDC_USE_CMD23
    u32 l_card_no_cmd23 = 0;
#endif
#ifdef MTK_MSDC_USE_CACHE
    u32 l_force_prg = 0;
    u32 l_bypass_flush = 0;
#endif
    void __iomem *base = host->base;
    //u32 intsts = 0;
    unsigned int left=0;
    int dma = 0, read = 1, dir = DMA_FROM_DEVICE, send_type=0;
    u32 map_sg = 0;
    unsigned long pio_tmo;
#ifdef MSDC_TOP_RESET_ERROR_TUNE
    unsigned int is_top_reset = 0;
#endif
#define SND_DAT 0
#define SND_CMD 1


    if (is_card_sdio(host) || (host->hw->flags & MSDC_SDIO_IRQ)) {
        //mb();
        if (host->saved_para.hz)
        {
            if (host->saved_para.suspend_flag)
            {
                MSDC_ERR_PRINT(MSDC_ERROR, "msdc resume[s] cur_cfg=%x, save_cfg=%x, cur_hz=%d, save_hz=%d", sdr_read32(MSDC_CFG), host->saved_para.msdc_cfg, host->mclk, host->saved_para.hz);
                host->saved_para.suspend_flag = 0;
                msdc_restore_info(host);
            }
            else if ((host->saved_para.msdc_cfg !=0) &&
                ((sdr_read32(MSDC_CFG)&0xFFFFFF9F)!= (host->saved_para.msdc_cfg&0xFFFFFF9F)))
            {
                MSDC_ERR_PRINT(MSDC_ERROR, "msdc resume[ns] cur_cfg=%x, save_cfg=%x, cur_hz=%d, save_hz=%d", sdr_read32(MSDC_CFG), host->saved_para.msdc_cfg, host->mclk, host->saved_para.hz);
                msdc_restore_info(host);
            }
        }
    }
#if (MSDC_DATA1_INT == 1)
    if (host->hw->flags & MSDC_SDIO_IRQ)
    {
        //if((!u_sdio_irq_counter) && (!u_msdc_irq_counter))
        //ERR_MSG("Ahsin u_sdio_irq_counter=%d, u_msdc_irq_counter=%d  int_sdio_irq_enable=%d SDC_CFG=%x MSDC_INTEN=%x MSDC_INT=%x MSDC_PATCH_BIT0=%x", u_sdio_irq_counter, u_msdc_irq_counter, int_sdio_irq_enable, sdr_read32(SDC_CFG),sdr_read32(MSDC_INTEN),sdr_read32(MSDC_INT),sdr_read32(MSDC_PATCH_BIT0));
        if ((u_sdio_irq_counter>0) && ((u_sdio_irq_counter%800)==0))
            MSDC_ERR_PRINT(MSDC_ERROR, "Ahsin sdio_irq=%d, msdc_irq=%d  SDC_CFG=%x MSDC_INTEN=%x MSDC_INT=%x ", u_sdio_irq_counter, u_msdc_irq_counter, sdr_read32(SDC_CFG), sdr_read32(MSDC_INTEN), sdr_read32(MSDC_INT));
    }
#endif

    BUG_ON(mmc == NULL);
    BUG_ON(mrq == NULL);

    host->error = 0;
    atomic_set(&host->abort, 0);

    cmd  = mrq->cmd;
    data = mrq->cmd->data;

    /* check msdc is work ok. rule is RX/TX fifocnt must be zero after last request
     * if find abnormal, try to reset msdc first
     */
    if (msdc_txfifocnt() || msdc_rxfifocnt()) {
        MSDC_ERR_PRINT(MSDC_ERROR, "[SD%d] register abnormal,please check!\n",host->id);
        msdc_reset_hw(host->id);
    }

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
  #ifdef MTK_SDIO30_DETECT_THERMAL
    if(host->hw->host_function == MSDC_SDIO) {
        spin_lock_irqsave(&host->clk_gate_lock, flags);
        if(host->ot_period_check_start == 0) {
            mod_timer(&host->ot_timer, jiffies + OT_PERIOD);
            host->ot_period_check_start = 1;
        }
        spin_unlock_irqrestore(&host->clk_gate_lock, flags);
    }
  #endif // #ifdef MTK_SDIO30_DETECT_THERMAL
#endif    // MTK_SDIO30_ONLINE_TUNING_SUPPORT

    if (!data) {
        send_type=SND_CMD;

#ifdef MTK_MSDC_USE_CACHE
    if((host->hw->host_function == MSDC_EMMC) &&
        (cmd->opcode == MMC_SWITCH) &&
        (((cmd->arg >> 16) & 0xFF) == EXT_CSD_CACHE_FLUSH) &&
        (((cmd->arg >> 8) & 0x1)))
    {
        if(g_cache_status == CACHE_FLUSHED){
            MSDC_DBG_PRINT(MSDC_DEBUG, "bypass flush command, g_cache_status=%d", g_cache_status);
            l_bypass_flush = 1;
            goto done;
        }
    }
#endif

        if (msdc_do_command(host, cmd, 0, CMD_TIMEOUT) != 0) {
            goto done;
        }

#ifdef CONFIG_MTK_EMMC_SUPPORT
        if(host->hw->host_function == MSDC_EMMC &&
           host->hw->boot == MSDC_BOOT_EN &&
           cmd->opcode == MMC_SWITCH &&
           (((cmd->arg >> 16) & 0xFF) == EXT_CSD_PART_CFG)){
            partition_access = (char)((cmd->arg >> 8) & 0x07);
        }
#endif

#ifdef MTK_EMMC_ETT_TO_DRIVER
        if ((host->hw->host_function == MSDC_EMMC) && (cmd->opcode == MMC_ALL_SEND_CID)) {
            m_id           = UNSTUFF_BITS(cmd->resp, 120, 8);
            pro_name[0]    = UNSTUFF_BITS(cmd->resp,  96, 8);
            pro_name[1]    = UNSTUFF_BITS(cmd->resp,  88, 8);
            pro_name[2]    = UNSTUFF_BITS(cmd->resp,  80, 8);
            pro_name[3]    = UNSTUFF_BITS(cmd->resp,  72, 8);
            pro_name[4]    = UNSTUFF_BITS(cmd->resp,  64, 8);
            pro_name[5]    = UNSTUFF_BITS(cmd->resp,  56, 8);
            //pro_name[6]    = '\0';
        }
#endif

    } else {
        BUG_ON(data->blksz > HOST_MAX_BLKSZ);
        send_type=SND_DAT;

#ifdef MTK_MSDC_USE_CACHE
        if((host->hw->host_function == MSDC_EMMC) && host->mmc->card && (host->mmc->card->ext_csd.cache_ctrl & 0x1) && (cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))
            l_force_prg = !msdc_can_apply_cache(cmd->arg, data->blocks);
#endif

        data->error = 0;
        read = data->flags & MMC_DATA_READ ? 1 : 0;
        msdc_latest_operation_type[host->id] = read ? OPER_TYPE_READ : OPER_TYPE_WRITE;
        host->data = data;
        host->xfer_size = data->blocks * data->blksz;
        host->blksz = data->blksz;

        /* deside the transfer mode */
        if (drv_mode[host->id] == MODE_PIO) {
            host->dma_xfer = dma = 0;
            msdc_latest_transfer_mode[host->id] = TRAN_MOD_PIO;
        } else if (drv_mode[host->id] == MODE_DMA) {
            host->dma_xfer = dma = 1;
            msdc_latest_transfer_mode[host->id] = TRAN_MOD_DMA;
        } else if (drv_mode[host->id] == MODE_SIZE_DEP) {
            host->dma_xfer = dma = ((host->xfer_size >= dma_size[host->id]) ? 1 : 0);
            msdc_latest_transfer_mode[host->id] = dma ? TRAN_MOD_DMA: TRAN_MOD_PIO;
        }

        if (read) {
            if ((host->timeout_ns != data->timeout_ns) ||
                (host->timeout_clks != data->timeout_clks)) {
                msdc_set_timeout(host, data->timeout_ns, data->timeout_clks);
            }
        }

        msdc_set_blknum(host, data->blocks);
        //msdc_clr_fifo();  /* no need */

#ifdef MTK_MSDC_USE_CMD23
        if (0 == (host->autocmd & MSDC_AUTOCMD23)){
            /* start the cmd23 first, mrq->sbc is NULL with single r/w */
            if (mrq->sbc){
                host->autocmd &= ~MSDC_AUTOCMD12;

                if(host->hw->host_function == MSDC_EMMC){
                  #ifdef MTK_MSDC_USE_CACHE
                    if(l_force_prg)
                        mrq->sbc->arg |= (1 << 24);
                  #endif
                }

                if (msdc_command_start(host, mrq->sbc, 0, CMD_TIMEOUT) != 0)
                    goto done;

                /* then wait command done */
                if (msdc_command_resp_polling(host, mrq->sbc, 0, CMD_TIMEOUT) != 0) {
                    goto stop;
                }
            } else {
                /* some sd card may not support cmd23,
                 * some emmc card have problem with cmd23, so use cmd12 here */
                if(host->hw->host_function != MSDC_SDIO){
                    host->autocmd |= MSDC_AUTOCMD12;
                }
            }
        } else {
            /* enable auto cmd23 */
            if (mrq->sbc){
                host->autocmd &= ~MSDC_AUTOCMD12;
                if(host->hw->host_function == MSDC_EMMC){
                    //mrq->sbc->arg &= ~(1 << 31);
                    //mrq->sbc->arg &= ~(1 << 29);
                  #ifdef MTK_MSDC_USE_CACHE
                    if (l_force_prg) {
                        mrq->sbc->arg |= (1 << 24);
                    } else {
                        mrq->sbc->arg &= ~(1 << 31);
                        mrq->sbc->arg &= ~(1 << 29);
                    }
                  #endif
                }
            } else {
                /* some sd card may not support cmd23,
                 * some emmc card have problem with cmd23, so use cmd12 here */
                if(host->hw->host_function != MSDC_SDIO){
                    host->autocmd &= ~MSDC_AUTOCMD23;
                    host->autocmd |= MSDC_AUTOCMD12;
                    l_card_no_cmd23 = 1;
                }
            }
        }
#endif /* end of MTK_MSDC_USE_CMD23 */

        if (dma) {
            msdc_dma_on();  /* enable DMA mode first!! */
            init_completion(&host->xfer_done);

#ifndef MTK_MSDC_USE_CMD23
            /* start the command first*/
            if(host->hw->host_function != MSDC_SDIO){
                host->autocmd |= MSDC_AUTOCMD12;
            }
#endif
            if (msdc_command_start(host, cmd, 0, CMD_TIMEOUT) != 0)
                goto done;

            dir = read ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
            (void)dma_map_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
            map_sg = 1;

            /* then wait command done */
            if (msdc_command_resp_polling(host, cmd, 0, CMD_TIMEOUT) != 0){ //not tuning
                goto stop;
            }

            /* for read, the data coming too fast, then CRC error
               start DMA no business with CRC. */
            //init_completion(&host->xfer_done);
            msdc_dma_setup(host, &host->dma, data->sg, data->sg_len);
            msdc_dma_start(host);
        #ifdef STO_LOG
            if (unlikely(dumpMSDC()))
                AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"msdc_dma_start",host->xfer_size);
        #endif
            spin_unlock(&host->lock);
            if(!wait_for_completion_timeout(&host->xfer_done, DAT_TIMEOUT)){
                MSDC_ERR_PRINT(MSDC_ERROR, "XXX CMD<%d> ARG<0x%x> wait xfer_done<%d> timeout!!", cmd->opcode, cmd->arg,data->blocks * data->blksz);

                host->sw_timeout++;
            #ifdef STO_LOG
                if (unlikely(dumpMSDC()))
                    AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"msdc_dma ERR",host->xfer_size);
            #endif
                MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
                data->error = (unsigned int)-ETIMEDOUT;
#ifdef MSDC_TOP_RESET_ERROR_TUNE
                if((host->id == 3) && (CHIP_SW_VER_01 == mt_get_chip_sw_ver())){
                    msdc_reset_gdma();
                    is_top_reset = 1;
                }else {
                    msdc_reset(host->id);
                }
#else
                msdc_reset(host->id);
#endif
            }
            spin_lock(&host->lock);
            msdc_dma_stop(host);
            if ((mrq->data && mrq->data->error) || ((host->autocmd & MSDC_AUTOCMD12) && mrq->stop && mrq->stop->error) ||
                (mrq->sbc && (mrq->sbc->error != 0) && (host->autocmd & MSDC_AUTOCMD23))){
                msdc_clr_fifo(host->id);
                msdc_clr_int();
            }

#ifdef MSDC_TOP_RESET_ERROR_TUNE
            if((host->id == 3) && mrq->data && mrq->data->error && (CHIP_SW_VER_01 == mt_get_chip_sw_ver())){
                is_top_reset = 1;
            }
            if((host->id == 3) && is_top_reset){
                msdc_top_reset(host);
            }
#endif
        #ifdef STO_LOG
            if (unlikely(dumpMSDC()))
                AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"msdc_dma_stop");
        #endif
        } else {
            /* Turn off dma */
            if(is_card_sdio(host)){
                msdc_reset_hw(host->id);
                msdc_dma_off();
                data->error = 0;
            }
            /* Firstly: send command */
            host->autocmd &= ~MSDC_AUTOCMD12;  /* need ask the designer, how about autocmd12 or autocmd23 with pio mode */

            l_autocmd23_is_set = 0;
            if (host->autocmd & MSDC_AUTOCMD23){
                l_autocmd23_is_set = 1;
                host->autocmd &= ~MSDC_AUTOCMD23;
            }

            host->dma_xfer = 0;
            if (msdc_do_command(host, cmd, 0, CMD_TIMEOUT) != 0) {
                goto stop;
            }

            /* Secondly: pio data phase */
            if (read) {
#ifdef MTK_MSDC_DUMP_FIFO
                MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: start pio read\n", __func__);
#endif
                if (msdc_pio_read(host, data)){
                    msdc_gate_clock(host,0);
                    msdc_ungate_clock(host);
                    goto stop;      // need cmd12.
                }
            } else {
#ifdef MTK_MSDC_DUMP_FIFO
               MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: start pio write\n", __func__);
#endif
                if (msdc_pio_write(host, data)) {
                    msdc_gate_clock(host,0);
                    msdc_ungate_clock(host);
                    goto stop;
                }
            }

            /* For write case: make sure contents in fifo flushed to device */
            if (!read) {
                pio_tmo = jiffies + DAT_TIMEOUT;
                while (1) {
                    left=msdc_txfifocnt();
                    if (left == 0) {
                        break;
                    }
                    if (msdc_pio_abort(host, data, pio_tmo)) {
                        break;
                    }
                }
            }

        } // PIO mode

stop:
        /* pio mode will disable autocmd23 */
        if (l_autocmd23_is_set == 1){
            l_autocmd23_is_set = 0;
            host->autocmd |= MSDC_AUTOCMD23;
        }

#ifndef MTK_MSDC_USE_CMD23
        /* Last: stop transfer */
        if (data && data->stop){
            if(!((cmd->error == 0) && (data->error == 0) && (host->autocmd & MSDC_AUTOCMD12) &&
                        (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))){
                if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                    goto done;
                }
            }
        }
#else
        if (host->hw->host_function == MSDC_EMMC){
            if (data && data->stop){
                /* multi r/w with no cmd23 and no autocmd12, need send cmd12 manual */
                /* if PIO mode and autocmd23 enable, cmd12 need send, because autocmd23 is disable under PIO */
                if ((((mrq->sbc == NULL) && !(host->autocmd & MSDC_AUTOCMD12)) ||
                    (!dma && mrq->sbc && (host->autocmd & MSDC_AUTOCMD23))) &&
                        (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)){
                    if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                        goto done;
                    }
                }
            }

        } else {
            /* for non emmc card, use old flow */
            if (data && data->stop){
                if(!((cmd->error == 0) && (data->error == 0) && (host->autocmd & MSDC_AUTOCMD12) &&
                     (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))){
                    if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                        goto done;
                    }
                }
            }
        }
#endif

    }

done:

#ifdef MTK_MSDC_USE_CMD23
    /* for msdc use cmd23, but card not supported(sbc is NULL), need enable autocmd23 for next request */
    if (1 == l_card_no_cmd23){
        if(host->hw->host_function != MSDC_SDIO){
            host->autocmd |= MSDC_AUTOCMD23;
            host->autocmd &= ~MSDC_AUTOCMD12;
            l_card_no_cmd23 = 0;
        }
    }
#endif

    if (data != NULL) {
        host->data = NULL;
        host->dma_xfer = 0;

#if 0   //read MBR
#ifdef CONFIG_MTK_EMMC_SUPPORT
        {
            char *ptr = sg_virt(data->sg);
            int i;
            if (cmd->arg == 0x0 && (cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK)){
                MSDC_DBG_PRINT(MSDC_DEBUG, "XXXX CMD<%d> ARG<%X> offset = <%d> data<%s %s> sg <%p> ptr <%p> blksz<%d> block<%d> error<%d>\n",cmd->opcode, cmd->arg, offset, (dma? "dma":"pio"),
                       (read ? "read ":"write"), data->sg, ptr, data->blksz, data->blocks, data->error);

                for(i = 0; i < 512; i ++){
                    if (i%32 == 0)
                        MSDC_RAW_PRINT(MSDC_DEBUG, "\n");
                    MSDC_RAW_PRINT(MSDC_DEBUG, " %2x ", ptr[i]);
                }
            }
        }
#endif
#endif   //end read MBR
        if (dma != 0) {
            msdc_dma_off();
            host->dma.used_bd = 0;
            host->dma.used_gpd = 0;
            if (map_sg == 1) {
                /*if(data->error == 0){
                    int retry = 3;
                    int count = 1000;
                    msdc_retry(host->dma.gpd->hwo,retry,count,host->id);
                }*/
                dma_unmap_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
            }
        }
#ifdef CONFIG_MTK_EMMC_SUPPORT
        if(cmd->opcode == MMC_SEND_EXT_CSD){
            msdc_get_data(ext_csd,data);
        }
#endif
        host->blksz = 0;


        MSDC_DBG_PRINT(MSDC_STACK, "CMD<%d> data<%s %s> blksz<%d> block<%d> error<%d>",cmd->opcode, (dma? "dma":"pio"),
                (read ? "read ":"write") ,data->blksz, data->blocks, data->error);

        if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))){
            if ((cmd->opcode != 17)&&(cmd->opcode != 18)&&(cmd->opcode != 24)&&(cmd->opcode != 25)) {
                MSDC_DBG_PRINT(MSDC_STACK, "CMD<%3d> arg<0x%8x> Resp<0x%8x> data<%s> size<%d>", cmd->opcode, cmd->arg, cmd->resp[0],
                    (read ? "read ":"write") ,data->blksz * data->blocks);
            } else {
                MSDC_DBG_PRINT(MSDC_STACK, "CMD<%3d> arg<0x%8x> Resp<0x%8x> block<%d>", cmd->opcode,
                    cmd->arg, cmd->resp[0], data->blocks);
            }
        }
    } else {
        if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {
            if (cmd->opcode != 13) { // by pass CMD13
                MSDC_DBG_PRINT(MSDC_STACK, "CMD<%3d> arg<0x%8x> resp<%8x %8x %8x %8x>", cmd->opcode, cmd->arg,
                cmd->resp[0],cmd->resp[1], cmd->resp[2], cmd->resp[3]);
            }
        }
    }

    if (mrq->cmd->error == (unsigned int)-EIO) {
        if(((cmd->opcode == MMC_SELECT_CARD) || (cmd->opcode == MMC_SLEEP_AWAKE)) &&
        	 ((host->hw->host_function == MSDC_EMMC) || (host->hw->host_function == MSDC_SD))){
            mrq->cmd->error = 0x0;
        } else {
            host->error |= REQ_CMD_EIO;
            sdio_tune_flag |= 0x1;

            if( mrq->cmd->opcode == SD_IO_RW_EXTENDED )
                sdio_tune_flag |= 0x1;
        }
    }


    if (mrq->cmd->error == (unsigned int)-ETIMEDOUT) host->error |= REQ_CMD_TMO;

    if (mrq->data && mrq->data->error) {
        host->error |= REQ_DAT_ERR;
        sdio_tune_flag |= 0x10;

        if (mrq->data->flags & MMC_DATA_READ)
            sdio_tune_flag |= 0x80;
        else
            sdio_tune_flag |= 0x40;
    }

#ifdef MTK_MSDC_USE_CMD23
    if (mrq->sbc && (mrq->sbc->error == (unsigned int)-EIO)) host->error |= REQ_CMD_EIO;
    if (mrq->sbc && (mrq->sbc->error == (unsigned int)-ETIMEDOUT)) host->error |= REQ_CMD_TMO;
#endif

    if (mrq->stop && (mrq->stop->error == (unsigned int)-EIO)) host->error |= REQ_STOP_EIO;
    if (mrq->stop && (mrq->stop->error == (unsigned int)-ETIMEDOUT)) host->error |= REQ_STOP_TMO;
    //if (host->error) MSDC_ERR_PRINT(MSDC_ERROR, "host->error<%d>", host->error);
#ifdef SDIO_ERROR_BYPASS
    if(is_card_sdio(host) && !host->error){
        host->sdio_error = 0;
    }
#endif

#ifdef MTK_MSDC_USE_CACHE
    if((host->hw->host_function == MSDC_EMMC) && host->mmc->card) {
        if(host->mmc->card->ext_csd.cache_ctrl & 0x1){
            if((cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK) || (cmd->opcode == MMC_WRITE_BLOCK)){
                if((host->error == 0) && mrq->sbc && (((mrq->sbc->arg >> 24) & 0x1) || ((mrq->sbc->arg >> 31) & 0x1))){
                    /* if reliable write, or force prg write emmc device successfully, do set cache flushed status */
                    if(g_cache_status == CACHE_UN_FLUSHED){
                        g_cache_status = CACHE_FLUSHED;
                        MSDC_DBG_PRINT(MSDC_STACK, "reliable / foce prg happend, update g_cache_status = %d, g_flush_data_size=%lld", g_cache_status, g_flush_data_size);
                        g_flush_data_size = 0;
                    }
                }else if(host->error == 0){
                    /* if normal write emmc device successfully, do clear the cache flushed status */
                    if(g_cache_status == CACHE_FLUSHED) {
                        g_cache_status = CACHE_UN_FLUSHED;
                        MSDC_DBG_PRINT(MSDC_STACK, "normal write happned, update g_cache_status = %d", g_cache_status);
                    }
                    g_flush_data_size += data->blocks;
                }else if(host->error){
                    g_flush_data_size += data->blocks;
                    MSDC_ERR_PRINT(MSDC_ERROR, "write error happend, g_flush_data_size=%lld", g_flush_data_size);
                }
            }else if((cmd->opcode == MMC_SWITCH) && (((cmd->arg >> 16) & 0xFF) == EXT_CSD_CACHE_FLUSH) && (((cmd->arg >> 8) & 0x1)) && !l_bypass_flush) {
                if(host->error == 0) {
                    /* if flush cache of emmc device successfully, do set the cache flushed status */
                    g_cache_status = CACHE_FLUSHED;
                    MSDC_DBG_PRINT(MSDC_STACK, "flush happend, update g_cache_status = %d, g_flush_data_size=%lld", g_cache_status, g_flush_data_size);
                    g_flush_data_size = 0;
                }else {
                    g_flush_error_happend = 1;
                }
            }
        }
#if 0
        if((cmd->opcode == MMC_SWITCH) && (((cmd->arg >> 16) & 0xFF) == EXT_CSD_CACHE_CTRL) && (host->error == 0)) {
            if((cmd->arg >> 8) & 0x1){
                host->autocmd &= ~MSDC_AUTOCMD23;
                MSDC_DBG_PRINT(MSDC_STACK, "disable AUTO_CMD23 because Cache feature is enabled");
            }else {
                host->autocmd |= MSDC_AUTOCMD23;
                MSDC_DBG_PRINT(MSDC_STACK, "enable AUTO_CMD23 because Cache feature is disabled");
            }
        }
#endif
    }
#endif

    return host->error;
}

static int msdc_tune_rw_request(struct mmc_host*mmc, struct mmc_request*mrq)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;
    struct mmc_data *data;

#ifdef MTK_MSDC_USE_CMD23
    u32 l_autocmd23_is_set = 0;
#endif

    void __iomem *base = host->base;
    //u32 intsts = 0;
      //unsigned int left=0;
    int read = 1,dma = 1;//dir = DMA_FROM_DEVICE, send_type=0,
#ifdef MSDC_TOP_RESET_ERROR_TUNE
    unsigned int is_top_reset = 0;
#endif
#define SND_DAT 0
#define SND_CMD 1

    MSDC_BUGON(mmc == NULL);
    MSDC_BUGON(mrq == NULL);

    //host->error = 0;
    atomic_set(&host->abort, 0);

    cmd  = mrq->cmd;
    data = mrq->cmd->data;

    /* check msdc is work ok. rule is RX/TX fifocnt must be zero after last request
     * if find abnormal, try to reset msdc first
     */
    if (msdc_txfifocnt() || msdc_rxfifocnt()) {
        MSDC_ERR_PRINT(MSDC_ERROR, "[SD%d] register abnormal,please check!\n",host->id);
        msdc_reset_hw(host->id);
    }


    MSDC_BUGON(data->blksz > HOST_MAX_BLKSZ);
    //send_type=SND_DAT;

    data->error = 0;
    read = data->flags & MMC_DATA_READ ? 1 : 0;
    msdc_latest_operation_type[host->id] = read ? OPER_TYPE_READ : OPER_TYPE_WRITE;
    host->data = data;
    host->xfer_size = data->blocks * data->blksz;
    host->blksz = data->blksz;
    host->dma_xfer = 1;

    /* deside the transfer mode */
    /*
    if (drv_mode[host->id] == MODE_PIO) {
        host->dma_xfer = dma = 0;
        msdc_latest_transfer_mode[host->id] = TRAN_MOD_PIO;
    } else if (drv_mode[host->id] == MODE_DMA) {
        host->dma_xfer = dma = 1;
        msdc_latest_transfer_mode[host->id] = TRAN_MOD_DMA;
    } else if (drv_mode[host->id] == MODE_SIZE_DEP) {
        host->dma_xfer = dma = ((host->xfer_size >= dma_size[host->id]) ? 1 : 0);
        msdc_latest_transfer_mode[host->id] = dma ? TRAN_MOD_DMA: TRAN_MOD_PIO;
    }
    */
    if (read) {
        if ((host->timeout_ns != data->timeout_ns) || (host->timeout_clks != data->timeout_clks))
        {
            msdc_set_timeout(host, data->timeout_ns, data->timeout_clks);
        }
    }

    msdc_set_blknum(host, data->blocks);
    //msdc_clr_fifo();  /* no need */
    msdc_dma_on();  /* enable DMA mode first!! */
    init_completion(&host->xfer_done);

    /* start the command first*/
#ifndef MTK_MSDC_USE_CMD23
    if(host->hw->host_function != MSDC_SDIO){
        host->autocmd |= MSDC_AUTOCMD12;
    }
#else
    if(host->hw->host_function != MSDC_SDIO){
        host->autocmd |= MSDC_AUTOCMD12;

        /* disable autocmd23 in error tuning flow */
        l_autocmd23_is_set = 0;
        if (host->autocmd & MSDC_AUTOCMD23){
            l_autocmd23_is_set = 1;
            host->autocmd &= ~MSDC_AUTOCMD23;
        }
    }
#endif

    if (msdc_command_start(host, cmd, 0, CMD_TIMEOUT) != 0)
        goto done;

    /* then wait command done */
    if (msdc_command_resp_polling(host, cmd, 0, CMD_TIMEOUT) != 0){ //not tuning
        goto stop;
    }

    /* for read, the data coming too fast, then CRC error
     * start DMA no business with CRC. */
    msdc_dma_setup(host, &host->dma, data->sg, data->sg_len);
    msdc_dma_start(host);
    //MSDC_DBG_PRINT(MSDC_STACK, "1.Power cycle enable(%d)",host->power_cycle_enable);
#ifdef STO_LOG
    if (unlikely(dumpMSDC()))
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_tune_rw_request,"msdc_dma_start",host->xfer_size);
#endif
    spin_unlock(&host->lock);
    if(!wait_for_completion_timeout(&host->xfer_done, DAT_TIMEOUT)){
        MSDC_DBG_PRINT(MSDC_STACK, "XXX CMD<%d> ARG<0x%x> wait xfer_done<%d> timeout!!", cmd->opcode, cmd->arg,data->blocks * data->blksz);
        host->sw_timeout++;
#ifdef STO_LOG
        if (unlikely(dumpMSDC()))
            AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_tune_rw_request,"msdc_dma ERR",host->xfer_size);
#endif
        MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
        data->error = (unsigned int)-ETIMEDOUT;

#ifdef MSDC_TOP_RESET_ERROR_TUNE
        if((host->id == 3) && (CHIP_SW_VER_01 == mt_get_chip_sw_ver())){
            msdc_reset_gdma();
            is_top_reset = 1;
        }else {
            msdc_reset(host->id);
        }
#else
        msdc_reset(host->id);
#endif
    }
    spin_lock(&host->lock);
    //MSDC_DBG_PRINT(MSDC_STACK, "2.Power cycle enable(%d)",host->power_cycle_enable);
    msdc_dma_stop(host);
    if ((mrq->data && mrq->data->error)||(host->autocmd & MSDC_AUTOCMD12 && mrq->stop && mrq->stop->error) ||
            (mrq->sbc && (mrq->sbc->error != 0) && (host->autocmd & MSDC_AUTOCMD23))){
        msdc_clr_fifo(host->id);
        msdc_clr_int();
    }

#ifdef MSDC_TOP_RESET_ERROR_TUNE
    if((host->id == 3) && mrq->data && mrq->data->error && (CHIP_SW_VER_01 == mt_get_chip_sw_ver())){
        is_top_reset = 1;
    }
    if((host->id == 3) && is_top_reset){
        msdc_top_reset(host);
    }
#endif

#ifdef STO_LOG
    if (unlikely(dumpMSDC()))
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_tune_rw_request,"msdc_dma_stop");
#endif

stop:
    /* Last: stop transfer */

    if (data->stop){
        if(!((cmd->error == 0) && (data->error == 0) && (host->autocmd == MSDC_AUTOCMD12) && (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))){
            if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                goto done;
            }
        }
    }

done:
    host->data = NULL;
    host->dma_xfer = 0;
    msdc_dma_off();
    host->dma.used_bd  = 0;
    host->dma.used_gpd = 0;
    host->blksz = 0;


    MSDC_DBG_PRINT(MSDC_STACK, "CMD<%d> data<%s %s> blksz<%d> block<%d> error<%d>",cmd->opcode, (dma? "dma":"pio"),
                (read ? "read ":"write") ,data->blksz, data->blocks, data->error);

    if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {
        if ((cmd->opcode != 17)&&(cmd->opcode != 18)&&(cmd->opcode != 24)&&(cmd->opcode != 25)) {
             MSDC_DBG_PRINT(MSDC_STACK, "CMD<%3d> arg<0x%8x> Resp<0x%8x> data<%s> size<%d>", cmd->opcode, cmd->arg, cmd->resp[0],
                    (read ? "read ":"write") ,data->blksz * data->blocks);
        } else {
             MSDC_DBG_PRINT(MSDC_STACK, "CMD<%3d> arg<0x%8x> Resp<0x%8x> block<%d>", cmd->opcode,
                    cmd->arg, cmd->resp[0], data->blocks);
        }
    } else {
        if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {
            if (cmd->opcode != 13) { // by pass CMD13
                MSDC_DBG_PRINT(MSDC_STACK, "CMD<%3d> arg<0x%8x> resp<%8x %8x %8x %8x>", cmd->opcode, cmd->arg,
                    cmd->resp[0],cmd->resp[1], cmd->resp[2], cmd->resp[3]);
            }
        }
    }
    host->error = 0;
    if (mrq->cmd->error == (unsigned int)-EIO) {
        if(((cmd->opcode == MMC_SELECT_CARD) || (cmd->opcode == MMC_SLEEP_AWAKE)) &&
        	 ((host->hw->host_function == MSDC_EMMC) || (host->hw->host_function == MSDC_SD))){
            mrq->cmd->error = 0x0;
        } else {
            host->error |= REQ_CMD_EIO;
        }
    }
    if (mrq->cmd->error == (unsigned int)-ETIMEDOUT) host->error |= REQ_CMD_TMO;
    if (mrq->data && (mrq->data->error)) host->error |= REQ_DAT_ERR;
    if (mrq->stop && (mrq->stop->error == (unsigned int)-EIO)) host->error |= REQ_STOP_EIO;
    if (mrq->stop && (mrq->stop->error == (unsigned int)-ETIMEDOUT)) host->error |= REQ_STOP_TMO;


#ifdef MTK_MSDC_USE_CMD23
    if (l_autocmd23_is_set == 1){
        /* restore the value */
        host->autocmd |= MSDC_AUTOCMD23;
    }
#endif
    return host->error;
}

static void msdc_pre_req(struct mmc_host *mmc, struct mmc_request *mrq, bool is_first_req)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_data *data;
    struct mmc_command *cmd = mrq->cmd;
    int read = 1, dir = DMA_FROM_DEVICE;
    BUG_ON(!cmd);
    data = mrq->data;
    if(data)
    data->host_cookie = MSDC_COOKIE_ASYNC;
    if(data && (cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)){
        host->xfer_size = data->blocks * data->blksz;
        read = data->flags & MMC_DATA_READ ? 1 : 0;
        if (drv_mode[host->id] == MODE_PIO) {
            data->host_cookie |= MSDC_COOKIE_PIO;
            msdc_latest_transfer_mode[host->id] = TRAN_MOD_PIO;
        } else if (drv_mode[host->id] == MODE_DMA) {

            msdc_latest_transfer_mode[host->id] = TRAN_MOD_DMA;
        } else if (drv_mode[host->id] == MODE_SIZE_DEP) {
        if(host->xfer_size < dma_size[host->id])
            {
                data->host_cookie |= MSDC_COOKIE_PIO;
                msdc_latest_transfer_mode[host->id] = TRAN_MOD_PIO;
            }
            else
            {
                msdc_latest_transfer_mode[host->id] = TRAN_MOD_DMA;
            }
        }
        if (msdc_async_use_dma(data->host_cookie)){
            dir = read ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
            (void)dma_map_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
        }
        MSDC_DBG_PRINT(MSDC_STACK, "CMD<%d> ARG<0x%x>data<%s %s> blksz<%d> block<%d> error<%d>",mrq->cmd->opcode,mrq->cmd->arg, (data->host_cookie ? "dma":"pio"),
                (read ? "read ":"write") ,data->blksz, data->blocks, data->error);
    }
    return;
}
static void msdc_dma_clear(struct msdc_host *host)
{
    void __iomem *base = host->base;
    host->data = NULL;
    host->mrq = NULL;
    host->dma_xfer = 0;
    msdc_dma_off();
    host->dma.used_bd  = 0;
    host->dma.used_gpd = 0;
    host->blksz = 0;
}
static void msdc_post_req(struct mmc_host *mmc, struct mmc_request *mrq, int err)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_data *data;
    //struct mmc_command *cmd = mrq->cmd;
    int  read = 1, dir = DMA_FROM_DEVICE;
    data = mrq->data;
    if(data && (msdc_async_use_dma(data->host_cookie)))    {
        host->xfer_size = data->blocks * data->blksz;
        read = data->flags & MMC_DATA_READ ? 1 : 0;
        dir = read ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
        dma_unmap_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
        data->host_cookie = 0;
        MSDC_DBG_PRINT(MSDC_STACK, "CMD<%d> ARG<0x%x> blksz<%d> block<%d> error<%d>",mrq->cmd->opcode,mrq->cmd->arg,
                data->blksz, data->blocks, data->error);
    }
    data->host_cookie = 0;
    return;

}

extern void met_mmc_issue(struct mmc_host *host, struct mmc_request *req);
static int msdc_do_request_async(struct mmc_host*mmc, struct mmc_request*mrq)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;
    struct mmc_data *data;
    void __iomem *base = host->base;

#ifdef MTK_MSDC_USE_CMD23
    u32 l_card_no_cmd23 = 0;
#endif

#ifdef MTK_MSDC_USE_CACHE
    u32 l_force_prg = 0;
#endif

    //u32 intsts = 0;
    //unsigned int left=0;
    int dma = 0, read = 1;//, dir = DMA_FROM_DEVICE;
    MVG_EMMC_DECLARE_INT32(delay_ns);
    MVG_EMMC_DECLARE_INT32(delay_us);
    MVG_EMMC_DECLARE_INT32(delay_ms);

    MSDC_BUGON(mmc == NULL);
    MSDC_BUGON(mrq == NULL);
    if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
        MSDC_ERR_PRINT(MSDC_ERROR, "cmd<%d> arg<0x%x> card<%d> power<%d>", mrq->cmd->opcode,mrq->cmd->arg,is_card_present(host), host->power_mode);
        mrq->cmd->error = (unsigned int)-ENOMEDIUM;
        if(mrq->done)
            mrq->done(mrq);         // call done directly.
        return 0;
    }
    msdc_ungate_clock(host);
    host->tune = 0;

    host->error = 0;
    atomic_set(&host->abort, 0);
    spin_lock(&host->lock);
    cmd  = mrq->cmd;
    data = mrq->cmd->data;
    host->mrq = mrq;
    /* check msdc is work ok. rule is RX/TX fifocnt must be zero after last request
     * if find abnormal, try to reset msdc first
     */
    if (msdc_txfifocnt() || msdc_rxfifocnt()) {
        MSDC_ERR_PRINT(MSDC_ERROR, "[SD%d] register abnormal,please check!\n",host->id);
        msdc_reset_hw(host->id);
    }

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
  #ifdef MTK_SDIO30_DETECT_THERMAL
    if(host->hw->host_function == MSDC_SDIO) {
        spin_lock_irqsave(&host->clk_gate_lock, flags);
        if(host->ot_period_check_start == 0) {
            mod_timer(&host->ot_timer, jiffies + OT_PERIOD);
            host->ot_period_check_start = 1;
        }
        spin_unlock_irqrestore(&host->clk_gate_lock, flags);
    }
  #endif // #ifdef MTK_SDIO30_DETECT_THERMAL
#endif    // MTK_SDIO30_ONLINE_TUNING_SUPPORT

    MSDC_BUGON(data->blksz > HOST_MAX_BLKSZ);
    //send_type=SND_DAT;

#ifdef MTK_MSDC_USE_CACHE
   if((host->hw->host_function == MSDC_EMMC) && host->mmc->card && (host->mmc->card->ext_csd.cache_ctrl & 0x1) && (cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))
       l_force_prg = !msdc_can_apply_cache(cmd->arg, data->blocks);
#endif

    data->error = 0;
    read = data->flags & MMC_DATA_READ ? 1 : 0;
    msdc_latest_operation_type[host->id] = read ? OPER_TYPE_READ : OPER_TYPE_WRITE;
    host->data = data;
    host->xfer_size = data->blocks * data->blksz;
    host->blksz = data->blksz;
    host->dma_xfer = 1;
    /* deside the transfer mode */

    if (read) {
        if ((host->timeout_ns != data->timeout_ns) ||
            (host->timeout_clks != data->timeout_clks)) {
             msdc_set_timeout(host, data->timeout_ns, data->timeout_clks);
        }
    }

    msdc_set_blknum(host, data->blocks);
    //msdc_clr_fifo();  /* no need */
    msdc_dma_on();  /* enable DMA mode first!! */
    //init_completion(&host->xfer_done);

#ifdef MTK_MSDC_USE_CMD23
        // if tuning flow run here, no problem?? need check!!!!!!!
        if (0 == (host->autocmd & MSDC_AUTOCMD23)){
            /* start the cmd23 first*/
            if (mrq->sbc){
                host->autocmd &= ~MSDC_AUTOCMD12;

                if(host->hw->host_function == MSDC_EMMC){
                  #ifdef MTK_MSDC_USE_CACHE
                    if(l_force_prg)
                        mrq->sbc->arg |= (1 << 24);
                  #endif
                }

                if (msdc_command_start(host, mrq->sbc, 0, CMD_TIMEOUT) != 0)
                    goto done;

                /* then wait command done */
                if (msdc_command_resp_polling(host, mrq->sbc, 0, CMD_TIMEOUT) != 0) {
                    goto stop;
                }
            } else {
                /* some sd card may not support cmd23,
                 * some emmc card have problem with cmd23, so use cmd12 here */
                if(host->hw->host_function != MSDC_SDIO){
                    host->autocmd |= MSDC_AUTOCMD12;
                }
            }
        } else {
            if (mrq->sbc){
                host->autocmd &= ~MSDC_AUTOCMD12;
                if(host->hw->host_function == MSDC_EMMC){
                    //mrq->sbc->arg &= ~(1 << 29);
                    //mrq->sbc->arg &= ~(1 << 31);
                  #ifdef MTK_MSDC_USE_CACHE
                    if (l_force_prg) {
                        mrq->sbc->arg |= (1 << 24);
                    } else {
                        mrq->sbc->arg &= ~(1 << 31);
                        mrq->sbc->arg &= ~(1 << 29);
                    }
                  #endif
                }
            } else {
                /* some sd card may not support cmd23,
                 * some emmc card have problem with cmd23, so use cmd12 here */
                if(host->hw->host_function != MSDC_SDIO){
                    host->autocmd &= ~MSDC_AUTOCMD23;
                    host->autocmd |= MSDC_AUTOCMD12;
                    l_card_no_cmd23 = 1;
                }
            }
        }

#else
        /* start the command first*/
        if(host->hw->host_function != MSDC_SDIO){
            host->autocmd |= MSDC_AUTOCMD12;
        }
#endif /* end of MTK_MSDC_USE_CMD23 */

    if (msdc_command_start(host, cmd, 0, CMD_TIMEOUT) != 0)
        goto done;

    /* then wait command done */
    if (msdc_command_resp_polling(host, cmd, 0, CMD_TIMEOUT) != 0) { // not tuning.
        goto stop;
    }

    /* for read, the data coming too fast, then CRC error
    start DMA no business with CRC. */
    //init_completion(&host->xfer_done);
    msdc_dma_setup(host, &host->dma, data->sg, data->sg_len);
    msdc_dma_start(host);
    //ERR_MSG("0.Power cycle enable(%d)",host->power_cycle_enable);

    MVG_EMMC_WRITE_MATCH(host, (u64)cmd->arg, delay_ms, delay_us, delay_ns, cmd->opcode, host->xfer_size);

    spin_unlock(&host->lock);

    met_mmc_issue(host->mmc, host->mrq);

#ifdef MTK_MSDC_USE_CMD23
    /* for msdc use cmd23, but card not supported(sbc is NULL), need enable autocmd23 for next request */
    if (1 == l_card_no_cmd23){
        if(host->hw->host_function != MSDC_SDIO){
            host->autocmd |= MSDC_AUTOCMD23;
            host->autocmd &= ~MSDC_AUTOCMD12;
            l_card_no_cmd23 = 0;
        }
    }
#endif

#ifdef MTK_MSDC_USE_CACHE
    if((host->hw->host_function == MSDC_EMMC) && host->mmc->card && (host->mmc->card->ext_csd.cache_ctrl & 0x1)){
        if((cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK) || (cmd->opcode == MMC_WRITE_BLOCK)){
            if((host->error == 0) && mrq->sbc && (((mrq->sbc->arg >> 24) & 0x1) || ((mrq->sbc->arg >> 31) & 0x1))){
                /* if reliable write, or force prg write emmc device successfully, do set cache flushed status */
                if(g_cache_status == CACHE_UN_FLUSHED){
                    g_cache_status = CACHE_FLUSHED;
                    MSDC_DBG_PRINT(MSDC_STACK, "reliable/force prg write happend,update g_cache_status = %d, g_flush_data_size=%lld", g_cache_status, g_flush_data_size);
                    g_flush_data_size = 0;
                }
            }else if(host->error == 0) {
                /* if normal write emmc device successfully, do clear the cache flushed status */
                if(g_cache_status == CACHE_FLUSHED){
                    g_cache_status = CACHE_UN_FLUSHED;
                    MSDC_DBG_PRINT(MSDC_STACK, "normal write happend, update g_cache_status = %d", g_cache_status);
                }
                g_flush_data_size += data->blocks;
            }else if(host->error) {
                g_flush_data_size += data->blocks;
                MSDC_ERR_PRINT(MSDC_ERROR, "write error happend, g_flush_data_size=%lld", g_flush_data_size);
            }
        }
    }
#endif

    return 0;


stop:
#ifndef MTK_MSDC_USE_CMD23
    /* Last: stop transfer */
    if (data && data->stop){
        if(!((cmd->error == 0) && (data->error == 0) && (host->autocmd & MSDC_AUTOCMD12) && (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))){
            if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                goto done;
            }
        }
    }
#else

    if (host->hw->host_function == MSDC_EMMC) {
        /* error handle will do msdc_abort_data() */
        } else {
            if (data && data->stop){
                if(!((cmd->error == 0) && (data->error == 0) && (host->autocmd & MSDC_AUTOCMD12) &&
                     (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))){
                    if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
                        goto done;
                    }
                }
            }
        }
#endif

done:
#ifdef MTK_MSDC_USE_CMD23
    /* for msdc use cmd23, but card not supported(sbc is NULL), need enable autocmd23 for next request */
    if (1 == l_card_no_cmd23){
        if(host->hw->host_function != MSDC_SDIO){
            host->autocmd |= MSDC_AUTOCMD23;
            host->autocmd &= ~MSDC_AUTOCMD12;
            l_card_no_cmd23 = 0;
        }
    }
#endif

    msdc_dma_clear(host);

    MSDC_DBG_PRINT(MSDC_STACK,  "CMD<%d> data<%s %s> blksz<%d> block<%d> error<%d>",cmd->opcode, (dma? "dma":"pio"),
                (read ? "read ":"write") ,data->blksz, data->blocks, data->error);

    if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {
        if ((cmd->opcode != 17)&&(cmd->opcode != 18)&&(cmd->opcode != 24)&&(cmd->opcode != 25)) {
            MSDC_DBG_PRINT(MSDC_STACK,  "CMD<%3d> arg<0x%8x> Resp<0x%8x> data<%s> size<%d>", cmd->opcode, cmd->arg, cmd->resp[0],
                   (read ? "read ":"write") ,data->blksz * data->blocks);
        } else {
            MSDC_DBG_PRINT(MSDC_STACK, "CMD<%3d> arg<0x%8x> Resp<0x%8x> block<%d>", cmd->opcode,cmd->arg, cmd->resp[0], data->blocks);
        }
    }

#ifdef MTK_MSDC_USE_CMD23
    if (mrq->sbc && (mrq->sbc->error == (unsigned int)-EIO)) host->error |= REQ_CMD_EIO;
    if (mrq->sbc && (mrq->sbc->error == (unsigned int)-ETIMEDOUT)) host->error |= REQ_CMD_TMO;
#endif

    if (mrq->cmd->error == (unsigned int)-EIO) {
        if(((cmd->opcode == MMC_SELECT_CARD) || (cmd->opcode == MMC_SLEEP_AWAKE)) &&
        	 ((host->hw->host_function == MSDC_EMMC) || (host->hw->host_function == MSDC_SD))){
            mrq->cmd->error = 0x0;
        } else {
            host->error |= REQ_CMD_EIO;
        }
    }
    if (mrq->cmd->error == (unsigned int)-ETIMEDOUT) host->error |= REQ_CMD_TMO;
    if (mrq->stop && (mrq->stop->error == (unsigned int)-EIO)) host->error |= REQ_STOP_EIO;
    if (mrq->stop && (mrq->stop->error == (unsigned int)-ETIMEDOUT)) host->error |= REQ_STOP_TMO;

#ifdef MTK_MSDC_USE_CACHE
    if((host->hw->host_function == MSDC_EMMC) && host->mmc->card && (host->mmc->card->ext_csd.cache_ctrl & 0x1)){
        if((cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK) || (cmd->opcode == MMC_WRITE_BLOCK)){
            if((host->error == 0) && mrq->sbc && (((mrq->sbc->arg >> 24) & 0x1) || ((mrq->sbc->arg >> 31) & 0x1))){
                /* if reliable write, or force prg write emmc device successfully, do set cache flushed status */
                if(g_cache_status == CACHE_UN_FLUSHED){
                    g_cache_status = CACHE_FLUSHED;
                    MSDC_DBG_PRINT(MSDC_STACK, "reliable/force prg write happend,update g_cache_status = %d, g_flush_data_size=%lld", g_cache_status, g_flush_data_size);
                    g_flush_data_size = 0;
                }
            }else if(host->error == 0) {
                /* if normal write emmc device successfully, do clear the cache flushed status */
                if(g_cache_status == CACHE_FLUSHED){
                    g_cache_status = CACHE_UN_FLUSHED;
                    MSDC_DBG_PRINT(MSDC_STACK, "normal write happend, update g_cache_status = %d", g_cache_status);
                }
                g_flush_data_size += data->blocks;
            }else if(host->error) {
                g_flush_data_size += data->blocks;
                MSDC_DBG_PRINT(MSDC_STACK, "write error happend, g_flush_data_size=%lld", g_flush_data_size);
            }
        }
    }
#endif

    if(mrq->done)
        mrq->done(mrq);

    msdc_gate_clock(host,1);
    spin_unlock(&host->lock);
    return host->error;
}

static int msdc_app_cmd(struct mmc_host *mmc, struct msdc_host *host)
{
    struct mmc_command cmd = {0};
    struct mmc_request mrq = {0};
    u32 err = -1;

    cmd.opcode = MMC_APP_CMD;
    cmd.arg = host->app_cmd_arg;  /* meet mmc->card is null when ACMD6 */
    cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

    mrq.cmd = &cmd; cmd.mrq = &mrq;
    cmd.data = NULL;

    err = msdc_do_command(host, &cmd, 0, CMD_TIMEOUT);
    return err;
}

static int msdc_lower_freq(struct msdc_host *host)
{
    u32 div, mode, hs400_src;
	u32 *hclks=NULL;
    void __iomem *base = host->base;

    MSDC_DBG_PRINT(MSDC_DEBUG, "need to lower freq");
    msdc_reset_crc_tune_counter(host,all_counter);
    sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD, mode);
    sdr_get_field(MSDC_CFG, MSDC_CFG_CKDIV, div);
    sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD_HS400, hs400_src);

#ifndef FPGA_PLATFORM
        hclks = (host->id == 3) ?  hclks_msdc50 : hclks_msdc30;
#endif

    if (div >= MSDC_MAX_FREQ_DIV) {
        MSDC_DBG_PRINT(MSDC_DEBUG, "but, div<%d> power tuning", div);
        return msdc_power_tuning(host);
#ifdef CONFIG_EMMC_50_FEATURE
    } else if((mode == 3) && (host->id == 3)){ //when HS400 low freq, you cannot change to mode 2 (DDR mode), else read data will be latched by clk, but not ds pin when card speed mode is still HS400.
        if(hs400_src == 1){
            return 0;
#if 0
            hs400_src = 0; //change from 400Mhz to 800Mhz, because CCKDIV is invalid when 400Mhz clk src.
            msdc_clock_src[host->id] = MSDC50_CLKSRC_800MHZ; //switch to 800Mhz
            host->hw->clk_src = msdc_clock_src[host->id];
            msdc_select_clksrc(host, host->hw->clk_src, host->hw->hclk_src);
#endif
        }
        msdc_clk_stable(host,mode, div, hs400_src);
        host->sclk = hclks[host->hw->clk_src]/(2*4*(div+1));

        MSDC_DBG_PRINT(MSDC_DEBUG, "new div<%d>, mode<%d> new freq.<%dKHz>", div + 1, mode,host->sclk/1000);
        return 0;
#endif
    } else if(mode == 1){
        mode = 0;
        msdc_clk_stable(host,mode, div, hs400_src);
        host->sclk = (div == 0) ? hclks[host->hw->clk_src]/2 : hclks[host->hw->clk_src]/(4*div);

        MSDC_DBG_PRINT(MSDC_DEBUG, "new div<%d>, mode<%d> new freq.<%dKHz>", div, mode,host->sclk/1000);
        return 0;
    }
    else{
        msdc_clk_stable(host,mode, div + 1, hs400_src);
        host->sclk = (mode == 2) ? hclks[host->hw->clk_src]/(2*4*(div+1)) : hclks[host->hw->clk_src]/(4*(div+1));
        MSDC_DBG_PRINT(MSDC_DEBUG, "new div<%d>, mode<%d> new freq.<%dKHz>", div + 1, mode,host->sclk/1000);
        return 0;
    }
}

static int msdc_tune_cmdrsp(struct msdc_host *host)
{
    int result = 0;
    void __iomem *base = host->base;
    u32 sel = 0;
    u32 cur_rsmpl = 0, orig_rsmpl;
    u32 cur_rrdly = 0, orig_rrdly;
    u32 cur_cntr  = 0,orig_cmdrtc;
    u32 cur_dl_cksel = 0, orig_dl_cksel;
    u32 clkmode;
    int hs400 = 0;

    if ( !host->error_tune_enable ) {
        return 1;
    }

    sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
    sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, orig_rrdly);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR, orig_cmdrtc);
    sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
    sdr_get_field(MSDC_CFG,MSDC_CFG_CKMOD,clkmode);
    hs400 = (clkmode == 3) ? 1 : 0;
#ifdef STO_LOG
    if (unlikely(dumpMSDC()))
    {
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"sd_tune_ori RSPL",orig_rsmpl);
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"sd_tune_ori RRDLY",orig_rrdly);
    }
#endif
#if 1
    if (host->mclk >= 100000000){
        sel = 1;
        //sdr_set_field(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL,0);
    } else {
        sdr_set_field(MSDC_PATCH_BIT1,MSDC_PB1_CMD_RSP_TA_CNTR,1);
        //sdr_set_field(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL,1);
        sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,0);
    }

    cur_rsmpl = (orig_rsmpl + 1);
    msdc_set_smpl(host, hs400, cur_rsmpl % 2, TYPE_CMD_RESP_EDGE, NULL);
    if (host->mclk <= 400000){
        msdc_set_smpl(host, hs400, 0, TYPE_CMD_RESP_EDGE, NULL);
        cur_rsmpl = 2;
    }
    if(cur_rsmpl >= 2){
        cur_rrdly = (orig_rrdly + 1);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, cur_rrdly % 32);
    }
    if(cur_rrdly >= 32){
        if(sel){
            cur_cntr = (orig_cmdrtc + 1) ;
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR, cur_cntr % 8);
        }
    }
    if(cur_cntr >= 8){
        if(sel){
            cur_dl_cksel = (orig_dl_cksel +1);
            sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel % 8);
        }
    }
    ++(host->t_counter.time_cmd);
    if((sel && host->t_counter.time_cmd == CMD_TUNE_UHS_MAX_TIME)||(sel == 0 && host->t_counter.time_cmd == CMD_TUNE_HS_MAX_TIME)){
#ifdef MSDC_LOWER_FREQ
        result = msdc_lower_freq(host);
#else
        result = 1;
#endif
        host->t_counter.time_cmd = 0;
    }
#else
    if (orig_rsmpl == 0) {
        cur_rsmpl = 1;
        msdc_set_smpl(host, hs400, cur_rsmpl, TYPE_CMD_RESP_EDGE, NULL);
    } else {
        cur_rsmpl = 0;
        msdc_set_smpl(host, hs400, cur_rsmpl, TYPE_CMD_RESP_EDGE, NULL); // need second layer
        cur_rrdly = (orig_rrdly + 1);
        if (cur_rrdly >= 32) {
            MSDC_ERR_PRINT(MSDC_ERROR, "failed to update rrdly<%d>", cur_rrdly);
            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, 0);
#ifdef MSDC_LOWER_FREQ
            return (msdc_lower_freq(host));
#else
            return 1;
#endif
        }
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, cur_rrdly);
    }

#endif
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
    sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, orig_rrdly);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR, orig_cmdrtc);
    sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
    MSDC_DBG_PRINT(MSDC_TUNING, "TUNE_CMD: rsmpl<%d> rrdly<%d> cmdrtc<%d> dl_cksel<%d> sfreq.<%d>", orig_rsmpl, orig_rrdly,orig_cmdrtc,orig_dl_cksel,host->sclk);
#ifdef STO_LOG
    if (unlikely(dumpMSDC()))
    {
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"sd_tune_ok RSPL",orig_rsmpl);
        AddStorageTrace(STORAGE_LOGGER_MSG_MSDC_DO,msdc_do_request,"sd_tune_ok RRDLY",orig_rrdly);
    }
#endif
    return result;
}

#ifdef CONFIG_EMMC_50_FEATURE
#define MAX_HS400_TUNE_COUNT (576) //(32*18)

int hs400_restore_pad_tune(struct msdc_host *host,int restore)
{
    void __iomem *base = host->base;

	host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
	
    return 0;
}
int hs400_restore_pb1(struct msdc_host *host,int restore)
{
    void __iomem *base = host->base;


    if (!restore) {
        sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, 0x2);
    }
	
    sdr_get_field((MSDC_PATCH_BIT1), MSDC_PB1_WRDAT_CRCS_TA_CNTR, mtk_msdc_host[0]->saved_para.wrdat_crc_ta_cntr);

}
int hs400_restore_ddly0(struct msdc_host *host,int restore)
{
    void __iomem *base = host->base;

    host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);

    return 0;
}
int hs400_restore_ddly1(struct msdc_host *host,int restore)
{
    void __iomem *base = host->base;

    host->saved_para.ddly1 = sdr_read32(MSDC_DAT_RDDLY1);

    return 0;
}

int hs400_restore_cmd_tune(struct msdc_host *host,int restore)
{
    void __iomem *base = host->base;

    if(!restore){
        sdr_set_field(EMMC50_PAD_CMD_TUNE, MSDC_EMMC50_PAD_CMD_TUNE_TXDLY, 0x8);
    }

    return 0;
}

int hs400_restore_dat01_tune(struct msdc_host *host,int restore)
{
    void __iomem *base = host->base;

    if(0) { //(!restore){
       sdr_set_field(EMMC50_PAD_DAT01_TUNE, MSDC_EMMC50_PAD_DAT0_TXDLY, 0x4);
       sdr_set_field(EMMC50_PAD_DAT01_TUNE, MSDC_EMMC50_PAD_DAT1_TXDLY, 0x4);
    }
	
    return 0;
}
int hs400_restore_dat23_tune(struct msdc_host *host,int restore)
{
    void __iomem *base = host->base;

    if(0) { //(!restore){
        sdr_set_field(EMMC50_PAD_DAT23_TUNE, MSDC_EMMC50_PAD_DAT2_TXDLY, 0x4);
        sdr_set_field(EMMC50_PAD_DAT23_TUNE, MSDC_EMMC50_PAD_DAT3_TXDLY, 0x4);
    }

    return 0;
}
int hs400_restore_dat45_tune(struct msdc_host *host,int restore)
{
    void __iomem *base = host->base;

    if(0) { //(!restore){
        sdr_set_field(EMMC50_PAD_DAT45_TUNE, MSDC_EMMC50_PAD_DAT4_TXDLY, 0x4);
        sdr_set_field(EMMC50_PAD_DAT45_TUNE, MSDC_EMMC50_PAD_DAT5_TXDLY, 0x4);
    }

    return 0;
}
int hs400_restore_dat67_tune(struct msdc_host *host,int restore)
{
    void __iomem *base = host->base;

    if(0) { //(!restore){
        sdr_set_field(EMMC50_PAD_DAT67_TUNE, MSDC_EMMC50_PAD_DAT6_TXDLY, 0x4);
        sdr_set_field(EMMC50_PAD_DAT67_TUNE, MSDC_EMMC50_PAD_DAT7_TXDLY, 0x4);
    }

    return 0;
}
int hs400_restore_ds_tune(struct msdc_host *host,int restore)
{
    void __iomem *base = host->base;

    if(0) { //(!restore){
        sdr_set_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, 0x7);
        sdr_set_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, 0x18);
    }

    return 0;
}
/*
 *  2013-12-09
 *  different register settings between eMMC 4.5 backward speed mode and HS400 speed mode
 */
#define HS400_BACKUP_REG_NUM (12)
static struct msdc_reg_control hs400_backup_reg_list[HS400_BACKUP_REG_NUM] = {
	  /*   addr                                       mask       value    default     restore_func */
    {(OFFSET_MSDC_PATCH_BIT0),        (MSDC_PB0_INT_DAT_LATCH_CK_SEL), 0x0, 0x0, NULL},
    {(OFFSET_MSDC_PATCH_BIT1),        (MSDC_PB1_WRDAT_CRCS_TA_CNTR), 0x0, 0x0, hs400_restore_pb1}, // the defalut init value is 0x1, but HS400 need the value be 0x0
    {(OFFSET_MSDC_IOCON),             (MSDC_IOCON_R_D_SMPL | MSDC_IOCON_DDLSEL | MSDC_IOCON_R_D_SMPL_SEL | MSDC_IOCON_R_D0SPL | MSDC_IOCON_W_D_SMPL_SEL | MSDC_IOCON_W_D_SMPL), 0x0, 0x0, NULL},
    {(OFFSET_MSDC_PAD_TUNE),          (MSDC_PAD_TUNE_DATWRDLY | MSDC_PAD_TUNE_DATRRDLY), 0x0, 0x0, hs400_restore_pad_tune},
    {(OFFSET_MSDC_DAT_RDDLY0),        (MSDC_DAT_RDDLY0_D3 | MSDC_DAT_RDDLY0_D2 | MSDC_DAT_RDDLY0_D1 | MSDC_DAT_RDDLY0_D0), 0x0, 0x0, hs400_restore_ddly0},
    {(OFFSET_MSDC_DAT_RDDLY1),        (MSDC_DAT_RDDLY1_D7 | MSDC_DAT_RDDLY1_D6 | MSDC_DAT_RDDLY1_D5 | MSDC_DAT_RDDLY1_D4), 0x0, 0x0, hs400_restore_ddly1},
    {(OFFSET_EMMC50_PAD_CMD_TUNE),    (MSDC_EMMC50_PAD_CMD_TUNE_TXDLY), 0x0, 0x00000200, hs400_restore_cmd_tune},
    {(OFFSET_EMMC50_PAD_DS_TUNE),     (MSDC_EMMC50_PAD_DS_TUNE_DLY1 | MSDC_EMMC50_PAD_DS_TUNE_DLY3), 0x0, 0x0, hs400_restore_ds_tune},
    {(OFFSET_EMMC50_PAD_DAT01_TUNE),  (MSDC_EMMC50_PAD_DAT0_TXDLY | MSDC_EMMC50_PAD_DAT1_TXDLY), 0x0, 0x01000100, hs400_restore_dat01_tune},
    {(OFFSET_EMMC50_PAD_DAT23_TUNE),  (MSDC_EMMC50_PAD_DAT2_TXDLY | MSDC_EMMC50_PAD_DAT3_TXDLY), 0x0, 0x01000100, hs400_restore_dat23_tune},
    {(OFFSET_EMMC50_PAD_DAT45_TUNE),  (MSDC_EMMC50_PAD_DAT4_TXDLY | MSDC_EMMC50_PAD_DAT5_TXDLY), 0x0, 0x01000100, hs400_restore_dat45_tune},
    {(OFFSET_EMMC50_PAD_DAT67_TUNE),  (MSDC_EMMC50_PAD_DAT6_TXDLY | MSDC_EMMC50_PAD_DAT7_TXDLY), 0x0, 0x01000100, hs400_restore_dat67_tune},
};

/*
 *  2013-12-09
 *  when switch from eMMC 4.5 backward speed mode to HS400 speed mode
 *  do back up the eMMC 4.5 backward speed mode tunning result, and init them with defalut value for HS400 speed mode
 */
static void emmc_hs400_backup(struct msdc_host *host)
{
    int i = 0, err = 0;
    u64 msdcioaddr = 0; 

    for(i = 0; i < HS400_BACKUP_REG_NUM; i++){
		msdcioaddr = ((u64)host->base + hs400_backup_reg_list[i].addr);
        sdr_get_field(msdcioaddr, hs400_backup_reg_list[i].mask,  hs400_backup_reg_list[i].value);
        if(hs400_backup_reg_list[i].restore_func){
            err = hs400_backup_reg_list[i].restore_func(host,0);
            if(err) {
                MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: failed to restore reg[%llx][0x%x], expected value[0x%x], actual value[0x%x] err=0x%x",
                        __func__, msdcioaddr, hs400_backup_reg_list[i].mask, hs400_backup_reg_list[i].default_value, sdr_read32(msdcioaddr), err);
            }
        }
    }
}
/*
 *  2013-12-09
 *  when switch from HS400 speed mode to eMMC 4.5 backward speed mode
 *  do restore the eMMC 4.5 backward speed mode tunning result
 */
static void emmc_hs400_restore(struct msdc_host *host)
{
    int i = 0, err = 0;
    u64 msdcioaddr = 0; 

    for(i = 0; i < HS400_BACKUP_REG_NUM; i++){
		msdcioaddr = ((u64)host->base + hs400_backup_reg_list[i].addr);
        sdr_set_field(msdcioaddr, hs400_backup_reg_list[i].mask,  hs400_backup_reg_list[i].value);
        if(hs400_backup_reg_list[i].restore_func){
            err = hs400_backup_reg_list[i].restore_func(host,1);
            if(err) {
                 MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]:failed to restore reg[%llx][0x%x], expected value[0x%x], actual value[0x%x] err=0x%x",
                        __func__, msdcioaddr, hs400_backup_reg_list[i].mask, hs400_backup_reg_list[i].value, sdr_read32(msdcioaddr), err);
            }
        }
        MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]:i:%d, reg=%llx, value=0x%x\n", __func__, i, msdcioaddr, sdr_read32(msdcioaddr));
    }
}

/*
 *  2013-12-09
 *  HS400 error tune flow of read/write data error
 *  HS400 error tune flow of cmd error is same as eMMC4.5 backward speed mode.
 */
static int emmc_hs400_tune_rw(struct msdc_host *host)
{
    void __iomem *base = host->base;
    int cur_ds_dly1 = 0, cur_ds_dly3 = 0, orig_ds_dly1 = 0, orig_ds_dly3 = 0;
    int err = 0;

    if((host->id != 3) || (host->state != MSDC_STATE_HS400)){
        return err;
    }

    sdr_get_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, orig_ds_dly1);
    sdr_get_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, orig_ds_dly3);

    cur_ds_dly1 = orig_ds_dly1 - 1;
    cur_ds_dly3 = orig_ds_dly3;
    if(cur_ds_dly1 < 0)
    {
        cur_ds_dly1 = 17;
        cur_ds_dly3 = orig_ds_dly3 + 1;
        if(cur_ds_dly3 >= 32){
            cur_ds_dly3 = 0;
        }
    }

    if(++host->t_counter.time_hs400 == MAX_HS400_TUNE_COUNT) {
        MSDC_TRC_PRINT(MSDC_TUNING, "Failed to update EMMC50_PAD_DS_TUNE_DLY, cur_ds_dly3=0x%x, cur_ds_dly1=0x%x", cur_ds_dly3, cur_ds_dly1);
#ifdef MSDC_LOWER_FREQ
        err = msdc_lower_freq(host);
#else
        err = 1;
#endif
        goto out;
    }else {
        sdr_set_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, cur_ds_dly1);
        if(cur_ds_dly3 != orig_ds_dly3){
            sdr_set_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, cur_ds_dly3);
        }
        MSDC_TRC_PRINT(MSDC_TUNING, "HS400_TUNE: orig_ds_dly1<0x%x>, orig_ds_dly3<0x%x>, cur_ds_dly1<0x%x>, cur_ds_dly3<0x%x>",
                                orig_ds_dly1, orig_ds_dly3, cur_ds_dly1, cur_ds_dly3 );
   }
out:
    return err;
}
#endif

static int msdc_tune_read(struct msdc_host *host)
{
    void __iomem *base = host->base;
    u32 sel = 0;
    u32 ddr = 0, hs400 = 0;
    u32 dcrc;
    u32 clkmode = 0;
    u32 cur_rxdly0, cur_rxdly1;
    u32 cur_dsmpl = 0, orig_dsmpl;
    u32 cur_dsel = 0,orig_dsel;
    u32 cur_dl_cksel = 0,orig_dl_cksel;
    u32 cur_dat0 = 0, cur_dat1 = 0, cur_dat2 = 0, cur_dat3 = 0,
    cur_dat4 = 0, cur_dat5 = 0, cur_dat6 = 0, cur_dat7 = 0;
    u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3, orig_dat4, orig_dat5, orig_dat6, orig_dat7;
    int result = 0;

    if ( !host->error_tune_enable ) {
        return 1;
    }

#if 1
    if (host->mclk >= 100000000){
        sel = 1;
    } else{
        sdr_set_field(MSDC_PATCH_BIT0,  MSDC_PB0_CKGEN_MSDC_DLY_SEL, 0);
    }
    sdr_get_field(MSDC_CFG,MSDC_CFG_CKMOD,clkmode);
    ddr = (clkmode == 2) ? 1 : 0;
    hs400 = (clkmode == 3) ? 1 : 0;

    sdr_get_field(MSDC_PATCH_BIT0,  MSDC_PB0_CKGEN_MSDC_DLY_SEL, orig_dsel);
    sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_dsmpl);

    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
    cur_dsmpl = (orig_dsmpl + 1) ;
    msdc_set_smpl(host, hs400, cur_dsmpl % 2, TYPE_READ_DATA_EDGE, NULL);

    if(cur_dsmpl >= 2){
#ifdef MSDC_TOP_RESET_ERROR_TUNE
        if((host->id == 3) && emmc_top_reset){
            dcrc = emmc_sdc_data_crc_value;
        }else{
            sdr_get_field(SDC_DCRC_STS, SDC_DCRC_STS_POS | SDC_DCRC_STS_NEG, dcrc);
        }
#else
        sdr_get_field(SDC_DCRC_STS, SDC_DCRC_STS_POS | SDC_DCRC_STS_NEG, dcrc);
#endif
        if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;

        cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
        cur_rxdly1 = sdr_read32(MSDC_DAT_RDDLY1);

            orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
            orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
            orig_dat2 = (cur_rxdly0 >>  8) & 0x1F;
            orig_dat3 = (cur_rxdly0 >>  0) & 0x1F;
            orig_dat4 = (cur_rxdly1 >> 24) & 0x1F;
            orig_dat5 = (cur_rxdly1 >> 16) & 0x1F;
            orig_dat6 = (cur_rxdly1 >>  8) & 0x1F;
            orig_dat7 = (cur_rxdly1 >>  0) & 0x1F;

        if (ddr) {
            cur_dat0 = (dcrc & (1 << 0) || dcrc & (1 <<  8)) ? (orig_dat0 + 1) : orig_dat0;
            cur_dat1 = (dcrc & (1 << 1) || dcrc & (1 <<  9)) ? (orig_dat1 + 1) : orig_dat1;
            cur_dat2 = (dcrc & (1 << 2) || dcrc & (1 << 10)) ? (orig_dat2 + 1) : orig_dat2;
            cur_dat3 = (dcrc & (1 << 3) || dcrc & (1 << 11)) ? (orig_dat3 + 1) : orig_dat3;
            cur_dat4 = (dcrc & (1 << 4) || dcrc & (1 << 12)) ? (orig_dat4 + 1) : orig_dat4;
            cur_dat5 = (dcrc & (1 << 5) || dcrc & (1 << 13)) ? (orig_dat5 + 1) : orig_dat5;
            cur_dat6 = (dcrc & (1 << 6) || dcrc & (1 << 14)) ? (orig_dat6 + 1) : orig_dat6;
            cur_dat7 = (dcrc & (1 << 7) || dcrc & (1 << 15)) ? (orig_dat7 + 1) : orig_dat7;
        } else {
            cur_dat0 = (dcrc & (1 << 0)) ? (orig_dat0 + 1) : orig_dat0;
            cur_dat1 = (dcrc & (1 << 1)) ? (orig_dat1 + 1) : orig_dat1;
            cur_dat2 = (dcrc & (1 << 2)) ? (orig_dat2 + 1) : orig_dat2;
            cur_dat3 = (dcrc & (1 << 3)) ? (orig_dat3 + 1) : orig_dat3;
            cur_dat4 = (dcrc & (1 << 4)) ? (orig_dat4 + 1) : orig_dat4;
            cur_dat5 = (dcrc & (1 << 5)) ? (orig_dat5 + 1) : orig_dat5;
            cur_dat6 = (dcrc & (1 << 6)) ? (orig_dat6 + 1) : orig_dat6;
            cur_dat7 = (dcrc & (1 << 7)) ? (orig_dat7 + 1) : orig_dat7;
        }


        cur_rxdly0 = ((cur_dat0 & 0x1F) << 24) | ((cur_dat1 & 0x1F) << 16) |
                     ((cur_dat2 & 0x1F) << 8)  | ((cur_dat3 & 0x1F) << 0);
        cur_rxdly1 = ((cur_dat4 & 0x1F) << 24) | ((cur_dat5 & 0x1F) << 16) |
                     ((cur_dat6 & 0x1F) << 8)  | ((cur_dat7 & 0x1F) << 0);


        sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
        sdr_write32(MSDC_DAT_RDDLY1, cur_rxdly1);

    }
    if((cur_dat0 >= 32) || (cur_dat1 >= 32) || (cur_dat2 >= 32) || (cur_dat3 >= 32)||
        (cur_dat4 >= 32) || (cur_dat5 >= 32) || (cur_dat6 >= 32) || (cur_dat7 >= 32)){
        if(sel){
            sdr_write32(MSDC_DAT_RDDLY0, 0);
            sdr_write32(MSDC_DAT_RDDLY1, 0);
            cur_dsel = (orig_dsel + 1) ;
            sdr_set_field(MSDC_PATCH_BIT0,  MSDC_PB0_CKGEN_MSDC_DLY_SEL, cur_dsel % 32);
        }
    }
    if(cur_dsel >= 32 ){
        if(clkmode == 1 && sel){
            cur_dl_cksel = (orig_dl_cksel + 1);
            sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel % 8);
        }
    }
    ++(host->t_counter.time_read);
    if((sel == 1 && clkmode == 1 && host->t_counter.time_read == READ_TUNE_UHS_CLKMOD1_MAX_TIME)||
        (sel == 1 && (clkmode == 0 ||clkmode == 2) && host->t_counter.time_read == READ_TUNE_UHS_MAX_TIME)||
        (sel == 0 && (clkmode == 0 ||clkmode == 2) && host->t_counter.time_read == READ_TUNE_HS_MAX_TIME)){
#ifdef MSDC_LOWER_FREQ
        result = msdc_lower_freq(host);
#else
        result = 1;
#endif
        host->t_counter.time_read = 0;
    }
#else
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

    cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
    cur_rxdly1 = sdr_read32(MSDC_DAT_RDDLY1);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_dsmpl);
    if (orig_dsmpl == 0) {
        cur_dsmpl = 1;
        msdc_set_smpl(host, hs400, cur_dsmpl, TYPE_READ_DATA_EDGE, NULL);
    } else {
        cur_dsmpl = 0;
        msdc_set_smpl(host, hs400, cur_dsmpl, TYPE_READ_DATA_EDGE, NULL); // need second layer

#ifdef MSDC_TOP_RESET_ERROR_TUNE
        if((host->id == 3) && emmc_top_reset){
            dcrc = emmc_sdc_data_crc_value;
        }else{
            sdr_get_field(SDC_DCRC_STS, SDC_DCRC_STS_POS | SDC_DCRC_STS_NEG, dcrc);
        }
#else
        sdr_get_field(SDC_DCRC_STS, SDC_DCRC_STS_POS | SDC_DCRC_STS_NEG, dcrc);
#endif
        if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;

        if (sdr_read32(MSDC_ECO_VER) >= 4) {
            orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
            orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
            orig_dat2 = (cur_rxdly0 >>  8) & 0x1F;
            orig_dat3 = (cur_rxdly0 >>  0) & 0x1F;
            orig_dat4 = (cur_rxdly1 >> 24) & 0x1F;
            orig_dat5 = (cur_rxdly1 >> 16) & 0x1F;
            orig_dat6 = (cur_rxdly1 >>  8) & 0x1F;
            orig_dat7 = (cur_rxdly1 >>  0) & 0x1F;
        } else {
            orig_dat0 = (cur_rxdly0 >>  0) & 0x1F;
            orig_dat1 = (cur_rxdly0 >>  8) & 0x1F;
            orig_dat2 = (cur_rxdly0 >> 16) & 0x1F;
            orig_dat3 = (cur_rxdly0 >> 24) & 0x1F;
            orig_dat4 = (cur_rxdly1 >>  0) & 0x1F;
            orig_dat5 = (cur_rxdly1 >>  8) & 0x1F;
            orig_dat6 = (cur_rxdly1 >> 16) & 0x1F;
            orig_dat7 = (cur_rxdly1 >> 24) & 0x1F;
        }

        if (ddr) {
            cur_dat0 = (dcrc & (1 << 0) || dcrc & (1 << 8))  ? (orig_dat0 + 1) : orig_dat0;
            cur_dat1 = (dcrc & (1 << 1) || dcrc & (1 << 9))  ? (orig_dat1 + 1) : orig_dat1;
            cur_dat2 = (dcrc & (1 << 2) || dcrc & (1 << 10)) ? (orig_dat2 + 1) : orig_dat2;
            cur_dat3 = (dcrc & (1 << 3) || dcrc & (1 << 11)) ? (orig_dat3 + 1) : orig_dat3;
        } else {
            cur_dat0 = (dcrc & (1 << 0)) ? (orig_dat0 + 1) : orig_dat0;
            cur_dat1 = (dcrc & (1 << 1)) ? (orig_dat1 + 1) : orig_dat1;
            cur_dat2 = (dcrc & (1 << 2)) ? (orig_dat2 + 1) : orig_dat2;
            cur_dat3 = (dcrc & (1 << 3)) ? (orig_dat3 + 1) : orig_dat3;
        }
        cur_dat4 = (dcrc & (1 << 4)) ? (orig_dat4 + 1) : orig_dat4;
        cur_dat5 = (dcrc & (1 << 5)) ? (orig_dat5 + 1) : orig_dat5;
        cur_dat6 = (dcrc & (1 << 6)) ? (orig_dat6 + 1) : orig_dat6;
        cur_dat7 = (dcrc & (1 << 7)) ? (orig_dat7 + 1) : orig_dat7;

        if (cur_dat0 >= 32 || cur_dat1 >= 32 || cur_dat2 >= 32 || cur_dat3 >= 32) {
            MSDC_ERR_PRINT(MSDC_ERROR, "failed to update <%xh><%xh><%xh><%xh>", cur_dat0, cur_dat1, cur_dat2, cur_dat3);
            sdr_write32(MSDC_DAT_RDDLY0, 0);
            sdr_write32(MSDC_DAT_RDDLY1, 0);

#ifdef MSDC_LOWER_FREQ
            return (msdc_lower_freq(host));
#else
            return 1;
#endif
        }

        if (cur_dat4 >= 32 || cur_dat5 >= 32 || cur_dat6 >= 32 || cur_dat7 >= 32) {
            MSDC_ERR_PRINT(MSDC_ERROR, "failed to update <%xh><%xh><%xh><%xh>", cur_dat4, cur_dat5, cur_dat6, cur_dat7);
            sdr_write32(MSDC_DAT_RDDLY0, 0);
            sdr_write32(MSDC_DAT_RDDLY1, 0);

#ifdef MSDC_LOWER_FREQ
            return (msdc_lower_freq(host));
#else
            return 1;
#endif
        }

        cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) | (cur_dat2 << 8) | (cur_dat3 << 0);
        cur_rxdly1 = (cur_dat4 << 24) | (cur_dat5 << 16) | (cur_dat6 << 8) | (cur_dat7 << 0);

        sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
        sdr_write32(MSDC_DAT_RDDLY1, cur_rxdly1);
    }

#endif
    sdr_get_field(MSDC_PATCH_BIT0,  MSDC_PB0_CKGEN_MSDC_DLY_SEL, orig_dsel);
    sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_dsmpl);
    cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
    cur_rxdly1 = sdr_read32(MSDC_DAT_RDDLY1);
    MSDC_TRC_PRINT(MSDC_TUNING, "TUNE_READ: dsmpl<%d> rxdly0<0x%x> rxdly1<0x%x> dsel<%d> dl_cksel<%d> sfreq.<%d>", orig_dsmpl, cur_rxdly0, cur_rxdly1,orig_dsel,orig_dl_cksel,host->sclk);

    return result;
}

static int msdc_tune_write(struct msdc_host *host)
{
    void __iomem *base = host->base;

    //u32 cur_wrrdly = 0, orig_wrrdly;
    u32 cur_dsmpl = 0,  orig_dsmpl;
    u32 cur_rxdly0 = 0;
    u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3;
    u32 cur_dat0 = 0,cur_dat1 = 0,cur_dat2 = 0,cur_dat3 = 0;
    u32 cur_d_cntr = 0 , orig_d_cntr;
    int result = 0;

    int sel = 0;
    int clkmode = 0;
    int hs400 = 0;

    if ( !host->error_tune_enable ) {
        return 1;
    }

#if 1
    if (host->mclk >= 100000000)
        sel = 1;
    else {
        sdr_set_field(MSDC_PATCH_BIT1,MSDC_PB1_WRDAT_CRCS_TA_CNTR,1);
    }
    sdr_get_field(MSDC_CFG,MSDC_CFG_CKMOD,clkmode);
    //sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, orig_wrrdly);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, orig_dsmpl);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, orig_d_cntr);
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

    cur_dsmpl = (orig_dsmpl + 1);
    hs400 = (clkmode == 3) ? 1 : 0;
    msdc_set_smpl(host, hs400, cur_dsmpl % 2, TYPE_WRITE_CRC_EDGE, NULL);
#if 0
    if(cur_dsmpl >= 2){
        cur_wrrdly = (orig_wrrdly+1);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, cur_wrrdly % 32);
    }
#endif
    if(cur_dsmpl >= 2){
        cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);

        orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
        orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
        orig_dat2 = (cur_rxdly0 >>  8) & 0x1F;
        orig_dat3 = (cur_rxdly0 >>  0) & 0x1F;

        cur_dat0 = (orig_dat0 + 1); /* only adjust bit-1 for crc */
        cur_dat1 = orig_dat1;
        cur_dat2 = orig_dat2;
        cur_dat3 = orig_dat3;

        cur_rxdly0 = ((cur_dat0 & 0x1F) << 24) | ((cur_dat1 & 0x1F) << 16) |
        ((cur_dat2 & 0x1F) << 8) | ((cur_dat3 & 0x1F) << 0);


        sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
    }
    if(cur_dat0 >= 32){
        if(sel){
            cur_d_cntr= (orig_d_cntr + 1 );
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, cur_d_cntr % 8);
        }
    }
    ++(host->t_counter.time_write);
    if((sel == 0 && host->t_counter.time_write == WRITE_TUNE_HS_MAX_TIME) || (sel && host->t_counter.time_write == WRITE_TUNE_UHS_MAX_TIME)){
#ifdef MSDC_LOWER_FREQ
        result = msdc_lower_freq(host);
#else
        result = 1;
#endif
        host->t_counter.time_write = 0;
    }

#else

    /* Tune Method 2. just DAT0 */
    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
    cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
    if (sdr_read32(MSDC_ECO_VER) >= 4) {
        orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
        orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
        orig_dat2 = (cur_rxdly0 >>  8) & 0x1F;
        orig_dat3 = (cur_rxdly0 >>  0) & 0x1F;
    } else {
        orig_dat0 = (cur_rxdly0 >>  0) & 0x1F;
        orig_dat1 = (cur_rxdly0 >>  8) & 0x1F;
        orig_dat2 = (cur_rxdly0 >> 16) & 0x1F;
        orig_dat3 = (cur_rxdly0 >> 24) & 0x1F;
    }

    sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, orig_wrrdly);
    cur_wrrdly = orig_wrrdly;
    sdr_get_field(MSDC_IOCON,    MSDC_IOCON_W_D_SMPL,        orig_dsmpl );
    if (orig_dsmpl == 0) {
        cur_dsmpl = 1;
        msdc_set_smpl(host, hs400, cur_dsmpl, TYPE_WRITE_CRC_EDGE, NULL);
    } else {
        cur_dsmpl = 0;
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, cur_dsmpl);  // need the second layer

        cur_wrrdly = (orig_wrrdly + 1);
        if (cur_wrrdly < 32) {
            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, cur_wrrdly);
        } else {
            cur_wrrdly = 0;
            sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, cur_wrrdly);  // need third

            cur_dat0 = orig_dat0 + 1; /* only adjust bit-1 for crc */
            cur_dat1 = orig_dat1;
            cur_dat2 = orig_dat2;
            cur_dat3 = orig_dat3;

            if (cur_dat0 >= 32) {
                MSDC_ERR_PRINT(MSDC_ERROR, "update failed <%xh>", cur_dat0);
                sdr_write32(MSDC_DAT_RDDLY0, 0);

#ifdef MSDC_LOWER_FREQ
                    return (msdc_lower_freq(host));
#else
                    return 1;
#endif
            }

            cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) | (cur_dat2 << 8) | (cur_dat3 << 0);
            sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
        }

    }

#endif
    //sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, orig_wrrdly);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, orig_dsmpl);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, orig_d_cntr);
    cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
    MSDC_TRC_PRINT(MSDC_TUNING, "TUNE_WRITE: dsmpl<%d> rxdly0<0x%x> d_cntr<%d> sfreq.<%d>", orig_dsmpl,cur_rxdly0,orig_d_cntr,host->sclk);

    return result;
}

static int msdc_get_card_status(struct mmc_host *mmc, struct msdc_host *host, u32 *status)
{
    struct mmc_command cmd;
    struct mmc_request mrq;
    u32 err;

    memset(&cmd, 0, sizeof(struct mmc_command));
    cmd.opcode = MMC_SEND_STATUS;    // CMD13
    cmd.arg = host->app_cmd_arg;
    cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;

    memset(&mrq, 0, sizeof(struct mmc_request));
    mrq.cmd = &cmd; cmd.mrq = &mrq;
    cmd.data = NULL;

    err = msdc_do_command(host, &cmd, 0, CMD_TIMEOUT);  // tune until CMD13 pass.

    if (status) {
        *status = cmd.resp[0];
    }

    return err;
}

//#define TUNE_FLOW_TEST
#ifdef TUNE_FLOW_TEST
static void msdc_reset_para(struct msdc_host *host)
{
    void __iomem *base = host->base;
    u32 dsmpl, rsmpl, clkmode;
    int hs400 = 0;

    // because we have a card, which must work at dsmpl<0> and rsmpl<0>

    sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, dsmpl);
    sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, rsmpl);
    sdr_get_field(MSDC_CFG,MSDC_CFG_CKMOD,clkmode);
    hs400 = (clkmode == 3) ? 1 : 0;

    if (dsmpl == 0) {
        msdc_set_smpl(host, hs400, 1, TYPE_READ_DATA_EDGE, NULL);
        MSDC_DBG_PRINT(MSDC_DEBUG, "set dspl<0>");
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, 0);
    }

    if (rsmpl == 0) {
        msdc_set_smpl(host, hs400, 1, TYPE_CMD_RESP_EDGE, NULL);
        MSDC_DBG_PRINT(MSDC_DEBUG, "set rspl<0>");
        sdr_write32(MSDC_DAT_RDDLY0, 0);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, 0);
    }
}
#endif

static void msdc_dump_trans_error(struct msdc_host   *host,
                                  struct mmc_command *cmd,
                                  struct mmc_data    *data,
                                  struct mmc_command *stop,
                                  struct mmc_command *sbc)
{
    //void __iomem *base = host->base;

    if ((cmd->opcode == 52) && (cmd->arg == 0xc00)) return;
    if ((cmd->opcode == 52) && (cmd->arg == 0x80000c08)) return;

    if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) { // by pass the SDIO CMD TO for SD/eMMC
        if ((host->hw->host_function == MSDC_SD) && (cmd->opcode == 5)) return;
    }else {
        if (cmd->opcode == 8) return;
    }

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
    if (is_card_sdio(host))   // auto-K have not done or finished
    {
        if(atomic_read(&host->ot_work.autok_done) == 0 && (cmd->opcode == 52 || cmd->opcode ==53))
            return;
    }
#endif

    MSDC_TRC_PRINT(MSDC_DEBUG, "XXX CMD<%d><0x%x> Error<%d> Resp<0x%x>", cmd->opcode, cmd->arg, cmd->error, cmd->resp[0]);

    if (data) {
        MSDC_TRC_PRINT(MSDC_DEBUG, "XXX DAT block<%d> Error<%d>", data->blocks, data->error);
    }

    if (stop) {
        MSDC_TRC_PRINT(MSDC_DEBUG, "XXX STOP<%d><0x%x> Error<%d> Resp<0x%x>", stop->opcode, stop->arg, stop->error, stop->resp[0]);
    }

    if (sbc) {
        MSDC_TRC_PRINT(MSDC_DEBUG, "XXX SBC<%d><0x%x> Error<%d> Resp<0x%x>", sbc->opcode, sbc->arg, sbc->error, sbc->resp[0]);
    }

#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
    if((host->hw->host_function == MSDC_SDIO) && (cmd) && (data) &&
        ((cmd->error == -EIO) || (data->error == -EIO)))
    {
        u32 vcore_uv_off = autok_get_current_vcore_offset();
        int cur_temperature = mtk_thermal_get_temp(MTK_THERMAL_SENSOR_CPU);

        MSDC_TRC_PRINT(MSDC_DEBUG, "XXX Vcore<0x%x> CPU_Temperature<%d>", vcore_uv_off, cur_temperature);
    }
#endif

    if((host->hw->host_function == MSDC_SD) &&
            (host->sclk > 100000000) &&
            (data) &&
            (data->error != (unsigned int)-ETIMEDOUT)){
        if((data->flags & MMC_DATA_WRITE) && (host->write_timeout_uhs104))
            host->write_timeout_uhs104 = 0;
        if((data->flags & MMC_DATA_READ) && (host->read_timeout_uhs104))
            host->read_timeout_uhs104 = 0;
    }

    if((host->hw->host_function == MSDC_EMMC) &&
            (data) &&
            (data->error != (unsigned int)-ETIMEDOUT)){
        if((data->flags & MMC_DATA_WRITE) && (host->write_timeout_emmc))
            host->write_timeout_emmc = 0;
        if((data->flags & MMC_DATA_READ) && (host->read_timeout_emmc))
            host->read_timeout_emmc = 0;
    }
#ifdef SDIO_ERROR_BYPASS
    if(is_card_sdio(host)&&(host->sdio_error!=-EIO)&&(cmd->opcode==53)&&(sg_dma_len(data->sg)>4)){
        host->sdio_error = -EIO;
        MSDC_TRC_PRINT(MSDC_DEBUG, "XXX SDIO Error ByPass");
    }
#endif
}

/* ops.request */
static void msdc_ops_request_legacy(struct mmc_host *mmc, struct mmc_request *mrq)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;
    struct mmc_data *data;
    struct mmc_command *stop = NULL;
    struct mmc_command *sbc = NULL;
    int data_abort = 0;
    //=== for sdio profile ===
    u32 old_H32 = 0, old_L32 = 0, new_H32 = 0, new_L32 = 0;
    u32 ticks = 0, opcode = 0, sizes = 0, bRx = 0;
    msdc_reset_crc_tune_counter(host,all_counter);
    if(host->mrq){
        MSDC_ERR_PRINT(MSDC_ERROR, "XXX host->mrq<%p> cmd<%d>arg<0x%x>", host->mrq,host->mrq->cmd->opcode,host->mrq->cmd->arg);
    }

    if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
        MSDC_DBG_PRINT(MSDC_DEBUG, "cmd<%d> arg<0x%x> card<%d> power<%d>", mrq->cmd->opcode,mrq->cmd->arg,is_card_present(host), host->power_mode);
        mrq->cmd->error = (unsigned int)-ENOMEDIUM;

#if 1
        if(mrq->done)
            mrq->done(mrq);         // call done directly.
#else
        mrq->cmd->retries = 0;  // please don't retry.
        mmc_request_done(mmc, mrq);
#endif

        return;
    }

    /* start to process */
    spin_lock(&host->lock);
    host->power_cycle_enable = 1;

    cmd = mrq->cmd;
    data = mrq->cmd->data;
    if (data) stop = data->stop;

#ifdef MTK_MSDC_USE_CMD23
    if (data) sbc = mrq->sbc;
#endif

    msdc_ungate_clock(host);  // set sw flag

    if (sdio_pro_enable) {  //=== for sdio profile ===
        if (mrq->cmd->opcode == 52 || mrq->cmd->opcode == 53) {
            //GPT_GetCounter64(&old_L32, &old_H32);
        }
    }

    host->mrq = mrq;

    while (msdc_do_request(mmc,mrq)) { // there is some error
        // becasue ISR executing time will be monitor, try to dump the info here.
        msdc_dump_trans_error(host, cmd, data, stop, sbc);
        data_abort = 0;
        if (is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ)) {
            goto out;  // sdio not tuning
        }

#ifdef MTK_MSDC_USE_CMD23
        if ((sbc != NULL) && (sbc->error == (unsigned int)-ETIMEDOUT)) {
            if (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ){
                /* not tuning, go out directly */
                MSDC_DBG_PRINT(MSDC_DEBUG, "===[%s:%d]==cmd23 timeout==\n", __func__, __LINE__);
                goto out;
            }
        }
#endif


#ifdef MTK_MSDC_USE_CMD23
        /* cmd->error also set when autocmd23 crc error */
        if ((cmd->error == (unsigned int)-EIO) || (stop && (stop->error == (unsigned int)-EIO)) ||
                (sbc && (sbc->error == (unsigned int)-EIO))) {
#else
        if ((cmd->error == (unsigned int)-EIO) || (stop && (stop->error == (unsigned int)-EIO))) {
#endif
            if (msdc_tune_cmdrsp(host)){
                MSDC_DBG_PRINT(MSDC_TUNING, "failed to updata cmd para");
                goto out;
            }
        }

        if (data && (data->error == (unsigned int)-EIO)) {
#ifdef CONFIG_EMMC_50_FEATURE
            if((host->id == 3) && (host->state == MSDC_STATE_HS400)){
                if(emmc_hs400_tune_rw(host)){
                    MSDC_DBG_PRINT(MSDC_TUNING,"failed to updata write para");
                    goto out;
                }
            } else if (data->flags & MMC_DATA_READ) {  // read
                if (msdc_tune_read(host)) {
                    MSDC_DBG_PRINT(MSDC_TUNING,"failed to updata read para");
                    goto out;
                }
            } else {
                if (msdc_tune_write(host)) {
                    MSDC_DBG_PRINT(MSDC_TUNING,"failed to updata write para");
                    goto out;
                }
            }
#else
            if (data->flags & MMC_DATA_READ) {  // read
                if (msdc_tune_read(host)) {
                    MSDC_DBG_PRINT(MSDC_TUNING,"failed to updata read para");
                    goto out;
                }
            } else {
                if (msdc_tune_write(host)) {
                    MSDC_DBG_PRINT(MSDC_TUNING,"failed to updata write para");
                    goto out;
                }
            }
#endif
        }

        // bring the card to "tran" state
        if (data || ((cmd->opcode == MMC_SWITCH) && (host->hw->host_function != MSDC_SDIO))) {
            if (msdc_abort_data(host)) {
                MSDC_DBG_PRINT(MSDC_DEBUG,"abort failed");
                data_abort = 1;
                if(host->hw->host_function == MSDC_SD){
                    if(host->card_inserted){
                        MSDC_TRC_PRINT(MSDC_INFO, "go to remove the bad card");
                        spin_unlock(&host->lock);
#if 0
                        ERR_MSG("go to remove the bad card");
                        msdc_set_bad_card_and_remove(host);
#else
                        if ( host->error_tune_enable ) {
                            ERR_MSG("disable error tune flow for bad SD card");
                            host->error_tune_enable=0;
                        }
#endif
                        spin_lock(&host->lock);
                    }
                    goto out;
                }
            }
        }

        // CMD TO -> not tuning
        if (cmd->error == (unsigned int)-ETIMEDOUT) {
            if(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ){
                if(data_abort){
                    if(msdc_power_tuning(host))
                    goto out;
                }
            }else {
                goto out;
            }
        }

        if ( cmd->error == (unsigned int)-ENOMEDIUM ) {
            goto out;
        }

        // [ALPS114710] Patch for data timeout issue.
        if (data && (data->error == (unsigned int)-ETIMEDOUT)) {
            if (data->flags & MMC_DATA_READ) {
                if( !(host->sw_timeout) &&
                    (host->hw->host_function == MSDC_SD) &&
                    (host->sclk > 100000000) &&
                    (host->read_timeout_uhs104 < MSDC_MAX_R_TIMEOUT_TUNE))
                {
                    if(host->t_counter.time_read)
                        host->t_counter.time_read--;
                    host->read_timeout_uhs104++;
                    msdc_tune_read(host);
                }
                else if((host->sw_timeout) || (host->read_timeout_uhs104 >= MSDC_MAX_R_TIMEOUT_TUNE) || (++(host->read_time_tune) > MSDC_MAX_TIMEOUT_RETRY)){
                    MSDC_DBG_PRINT(MSDC_DEBUG, "msdc%d exceed max read timeout retry times(%d) or SW timeout(%d) or read timeout tuning times(%d),Power cycle",host->id,host->read_time_tune,host->sw_timeout,host->read_timeout_uhs104);
                    if(msdc_power_tuning(host))
                        goto out;
                }
            } else if(data->flags & MMC_DATA_WRITE){
                if( (!(host->sw_timeout)) &&
                   (host->hw->host_function == MSDC_SD) &&
                   (host->sclk > 100000000) &&
                   (host->write_timeout_uhs104 < MSDC_MAX_W_TIMEOUT_TUNE))
                {
                    if(host->t_counter.time_write)
                        host->t_counter.time_write--;
                    host->write_timeout_uhs104++;
                    msdc_tune_write(host);
                } else if ( !(host->sw_timeout) &&
                    (host->hw->host_function == MSDC_EMMC) &&
                    (host->write_timeout_emmc < MSDC_MAX_W_TIMEOUT_TUNE_EMMC))
                {
                    if(host->t_counter.time_write)
                        host->t_counter.time_write--;
                    host->write_timeout_emmc++;

#ifdef CONFIG_EMMC_50_FEATURE
                    if((host->id == 3) && (host->state == MSDC_STATE_HS400)){
                        emmc_hs400_tune_rw(host);
                    }else {
                        msdc_tune_write(host);
                    }
#else
                    msdc_tune_write(host);
#endif
                } else if((host->hw->host_function == MSDC_SD) && ((host->sw_timeout) || (host->write_timeout_uhs104 >= MSDC_MAX_W_TIMEOUT_TUNE) || (++(host->write_time_tune) > MSDC_MAX_TIMEOUT_RETRY))){
                    MSDC_DBG_PRINT(MSDC_DEBUG, "msdc%d exceed max write timeout retry times(%d) or SW timeout(%d) or write timeout tuning time(%d),Power cycle",host->id,host->write_time_tune,host->sw_timeout,host->write_timeout_uhs104);
                    if(!(host->sd_30_busy) && msdc_power_tuning(host))
                        goto out;
                } else if((host->hw->host_function == MSDC_EMMC) && ((host->sw_timeout) || (++(host->write_time_tune) > MSDC_MAX_TIMEOUT_RETRY_EMMC))){
                    MSDC_DBG_PRINT(MSDC_DEBUG, "msdc%d exceed max write timeout retry times(%d) or SW timeout(%d) or write timeout tuning time(%d),Power cycle",host->id,host->write_time_tune,host->sw_timeout,host->write_timeout_emmc);
                    host->write_timeout_emmc = 0;
                    goto out;
                }
            }
        }

        // clear the error condition.
        cmd->error = 0;
        if (data) data->error = 0;
        if (stop) stop->error = 0;

#ifdef MTK_MSDC_USE_CMD23
        if (sbc) sbc->error = 0;
#endif

        // check if an app commmand.
        if (host->app_cmd) {
            while (msdc_app_cmd(host->mmc, host)) {
                if (msdc_tune_cmdrsp(host)){
                    MSDC_DBG_PRINT(MSDC_TUNING,"failed to updata cmd para for app");
                    goto out;
                }
            }
        }

        if (!is_card_present(host)) {
            goto out;
        }
    }


    if((host->read_time_tune)&&(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK)){
        host->read_time_tune = 0;
        MSDC_DBG_PRINT(MSDC_DEBUG,"Read recover");
        msdc_dump_trans_error(host, cmd, data, stop, sbc);
    }
    if((host->write_time_tune) && (cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)){
        host->write_time_tune = 0;
        MSDC_DBG_PRINT(MSDC_DEBUG, "Write recover");
        msdc_dump_trans_error(host, cmd, data, stop, sbc);
    }
    host->sw_timeout = 0;
out:
    msdc_reset_crc_tune_counter(host,all_counter);
#ifdef MTK_MSDC_USE_CACHE
    if(g_flush_error_happend && (host->hw->host_function == MSDC_EMMC) && host->mmc->card && (host->mmc->card->ext_csd.cache_ctrl & 0x1)){
        if((cmd->opcode == MMC_SWITCH) && (((cmd->arg >> 16) & 0xFF) == EXT_CSD_CACHE_FLUSH) && (((cmd->arg >> 8) & 0x1))){
            g_flush_error_count++;
            g_flush_error_happend = 0;
            MSDC_DBG_PRINT(MSDC_DEBUG, "the %d time flush error happned, g_flush_data_size=%lld", g_flush_error_count, g_flush_data_size);
            if(g_flush_error_count >=  MSDC_MAX_FLUSH_COUNT){
                if(!msdc_cache_ctrl(host, 0, NULL))
                    host->mmc->caps2 &= ~MMC_CAP2_CACHE_CTRL;
                MSDC_DBG_PRINT(MSDC_DEBUG,"flush failed %d times, exceed the max flush error count %d, disable cache feature, caps2=0x%x, MMC_CAP2_CACHE_CTRL=0x%x, cache_ctrl=%d",
                                g_flush_error_count, MSDC_MAX_FLUSH_COUNT, host->mmc->caps2, MMC_CAP2_CACHE_CTRL, host->mmc->card->ext_csd.cache_ctrl);
            }
        }
    }
#endif

#ifdef TUNE_FLOW_TEST
    if (!is_card_sdio(host)) {
        msdc_reset_para(host);
    }
#endif

    /* ==== when request done, check if app_cmd ==== */
    if (mrq->cmd->opcode == MMC_APP_CMD) {
        host->app_cmd = 1;
        host->app_cmd_arg = mrq->cmd->arg;  /* save the RCA */
    } else {
        host->app_cmd = 0;
        //host->app_cmd_arg = 0;
    }

    host->mrq = NULL;

    //=== for sdio profile ===
    if (sdio_pro_enable) {
        if (mrq->cmd->opcode == 52 || mrq->cmd->opcode == 53) {
            //GPT_GetCounter64(&new_L32, &new_H32);
            ticks = msdc_time_calc(old_L32, old_H32, new_L32, new_H32);

            opcode = mrq->cmd->opcode;
            if (mrq->cmd->data) {
                sizes = mrq->cmd->data->blocks * mrq->cmd->data->blksz;
                bRx = mrq->cmd->data->flags & MMC_DATA_READ ? 1 : 0 ;
            } else {
                bRx = mrq->cmd->arg    & 0x80000000 ? 1 : 0;
            }

            if (!mrq->cmd->error) {
                msdc_performance(opcode, sizes, bRx, ticks);
            }
        }
    }

    msdc_gate_clock(host, 1); // clear flag.
    spin_unlock(&host->lock);

    mmc_request_done(mmc, mrq);

   return;
}

static void msdc_tune_async_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;
    struct mmc_data *data;
    struct mmc_command *stop = NULL;
    struct mmc_command *sbc = NULL;
    //msdc_reset_crc_tune_counter(host,all_counter);
    if(host->mrq){
#ifdef CONFIG_MTK_AEE_FEATURE
        aee_kernel_warning("MSDC","MSDC request not clear.\n host attached<0x%p> current<0x%p>.\n", host->mrq, mrq);
#else
        MSDC_BUGON(host->mrq);
#endif
        MSDC_DBG_PRINT(MSDC_DEBUG,"XXX host->mrq<0x%p> cmd<%d>arg<0x%x>", host->mrq,host->mrq->cmd->opcode,host->mrq->cmd->arg);
        if(host->mrq->data){
            MSDC_DBG_PRINT(MSDC_DEBUG,"XXX request data size<%d>",host->mrq->data->blocks * host->mrq->data->blksz);
            MSDC_DBG_PRINT(MSDC_DEBUG,"XXX request attach to host force data timeout and retry");
            host->mrq->data->error = (unsigned int)-ETIMEDOUT;
        } else{
            MSDC_DBG_PRINT(MSDC_DEBUG,"XXX request attach to host force cmd timeout and retry");
            host->mrq->cmd->error = (unsigned int)-ETIMEDOUT;
        }
        MSDC_DBG_PRINT(MSDC_DEBUG,"XXX current request <0x%p> cmd<%d>arg<0x%x>",mrq,mrq->cmd->opcode,mrq->cmd->arg);
        if(mrq->data)
            MSDC_DBG_PRINT(MSDC_DEBUG,"XXX current request data size<%d>",mrq->data->blocks * mrq->data->blksz);
    }

    if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
        MSDC_DBG_PRINT(MSDC_DEBUG,"cmd<%d> arg<0x%x> card<%d> power<%d>", mrq->cmd->opcode,mrq->cmd->arg, is_card_present(host), host->power_mode);
        mrq->cmd->error = (unsigned int)-ENOMEDIUM;
        //mrq->done(mrq);         // call done directly.
        return;
    }

    cmd = mrq->cmd;
    data = mrq->cmd->data;
    if (msdc_async_use_pio(mrq->data->host_cookie)){
        return;
    }
    if (data) stop = data->stop;
#ifdef MTK_MSDC_USE_CMD23
    if (data) sbc = mrq->sbc;
#endif

#ifdef MTK_MSDC_USE_CMD23
    if(((sbc == NULL) || (sbc && sbc->error == 0)) && (cmd->error == 0) && (data && data->error == 0) && (!stop || stop->error == 0)){
#else
    if((cmd->error == 0) && (data && data->error == 0) && (!stop || stop->error == 0)){
#endif
        if(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK)
            host->read_time_tune = 0;
        if(cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)
            host->write_time_tune = 0;
        host->rwcmd_time_tune = 0;
        host->power_cycle_enable = 1;
        return;
    }
    /* start to process */
    spin_lock(&host->lock);

    /*if(host->error & REQ_CMD_EIO)
        cmd->error = (unsigned int)-EIO;
    else if(host->error & REQ_CMD_TMO)
        cmd->error = (unsigned int)-ETIMEDOUT;
    */

    msdc_ungate_clock(host);  // set sw flag
    host->tune = 1;
    host->mrq = mrq;
    do{
        msdc_dump_trans_error(host, cmd, data, stop, sbc);         // becasue ISR executing time will be monitor, try to dump the info here.
        /*if((host->t_counter.time_cmd % 16 == 15)
        || (host->t_counter.time_read % 16 == 15)
            || (host->t_counter.time_write % 16 == 15))
        {
            spin_unlock(&host->lock);
            msleep(150);
            MSDC_DBG_PRINT(MSDC_DEBUG, "sleep 150ms here!");   //sleep in tuning flow, to avoid printk watchdong timeout
            spin_lock(&host->lock);
            goto out;
        }*/

#ifdef MTK_MSDC_USE_CMD23
        if ((sbc != NULL) && (sbc->error == (unsigned int)-ETIMEDOUT)) {
            if (cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ){
                /* not tuning, go out directly */
                MSDC_DBG_PRINT(MSDC_STACK, "===[%s:%d]==cmd23 timeout==\n", __func__, __LINE__);
                goto out;
            }
        }
#endif

#ifdef MTK_MSDC_USE_CMD23
        /* cmd->error also set when autocmd23 crc error */
        if ((cmd->error == (unsigned int)-EIO) || (stop && (stop->error == (unsigned int)-EIO)) ||
                (sbc && (sbc->error == (unsigned int)-EIO))) {
#else
        if ((cmd->error == (unsigned int)-EIO) || (stop && (stop->error == (unsigned int)-EIO))) {
#endif
            if (msdc_tune_cmdrsp(host)){
                MSDC_DBG_PRINT(MSDC_TUNING, "failed to updata cmd para");
                goto out;
            }
        }

        if (data && (data->error == (unsigned int)-EIO)) {
#ifdef CONFIG_EMMC_50_FEATURE
            if((host->id == 3) && (host->state == MSDC_STATE_HS400)){
                if(emmc_hs400_tune_rw(host)){
                    MSDC_DBG_PRINT(MSDC_TUNING,"failed to updata write para");
                    goto out;
                }
            }else if (data->flags & MMC_DATA_READ) {  // read
                if (msdc_tune_read(host)) {
                    MSDC_DBG_PRINT(MSDC_TUNING,"failed to updata read para");
                    goto out;
                }
            } else {
                if (msdc_tune_write(host)) {
                    MSDC_DBG_PRINT(MSDC_TUNING,"failed to updata write para");
                    goto out;
                }
            }
#else
            if (data->flags & MMC_DATA_READ) {  // read
                if (msdc_tune_read(host)) {
                    MSDC_DBG_PRINT(MSDC_TUNING,"failed to updata read para");
                    goto out;
                }
            } else {
                if (msdc_tune_write(host)) {
                    MSDC_DBG_PRINT(MSDC_TUNING,"failed to updata write para");
                    goto out;
                }
            }
#endif
        }
        // bring the card to "tran" state
        // tuning param done if cmd crc error
        if (data) {
            if (msdc_abort_data(host)) {
                MSDC_DBG_PRINT(MSDC_DEBUG,"abort failed");
                if(host->hw->host_function == MSDC_SD){
                    if(host->card_inserted){
                        MSDC_TRC_PRINT(MSDC_INFO,"go to remove the bad card");
                        spin_unlock(&host->lock);
#if 0
                        MSDC_ERR_PRINT(MSDC_ERROR,"go to remove the bad card");
                        msdc_set_bad_card_and_remove(host);
#else
                        if ( host->error_tune_enable ) {
                            MSDC_ERR_PRINT(MSDC_ERROR,"disable error tune flow for bad SD card");
                            host->error_tune_enable=0;
                        }
#endif
                        spin_lock(&host->lock);
                    }
                    goto out;
                }
            }
        }

        // CMD TO -> not tuning. cmd->error also set when autocmd23 TO error
        if (cmd->error == (unsigned int)-ETIMEDOUT) {
            if(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ){
                if((host->sw_timeout) || (++(host->rwcmd_time_tune) > MSDC_MAX_TIMEOUT_RETRY)){
                    MSDC_DBG_PRINT(MSDC_TUNING, "msdc%d exceed max r/w cmd timeout tune times(%d) or SW timeout(%d),Power cycle",host->id,host->rwcmd_time_tune,host->sw_timeout);
                    if(!(host->sd_30_busy) && msdc_power_tuning(host))
                        goto out;
                }
            }else {
                goto out;
            }
        }

        if ( cmd->error == (unsigned int)-ENOMEDIUM ) {
            goto out;
        }

        // [ALPS114710] Patch for data timeout issue.
        if (data && (data->error == (unsigned int)-ETIMEDOUT)) {
            if (data->flags & MMC_DATA_READ) {
                if( !(host->sw_timeout) &&
                        (host->hw->host_function == MSDC_SD) &&
                        (host->sclk > 100000000) &&
                        (host->read_timeout_uhs104 < MSDC_MAX_R_TIMEOUT_TUNE)){
                    if(host->t_counter.time_read)
                        host->t_counter.time_read--;
                    host->read_timeout_uhs104++;
                    msdc_tune_read(host);
                }
                else if((host->sw_timeout) || (host->read_timeout_uhs104 >= MSDC_MAX_R_TIMEOUT_TUNE) || (++(host->read_time_tune) > MSDC_MAX_TIMEOUT_RETRY)){
                    MSDC_DBG_PRINT(MSDC_TUNING, "msdc%d exceed max read timeout retry times(%d) or SW timeout(%d) or read timeout tuning times(%d),Power cycle",host->id,host->read_time_tune,host->sw_timeout,host->read_timeout_uhs104);
                    if(!(host->sd_30_busy) && msdc_power_tuning(host))
                        goto out;
                }
            }
            else if(data->flags & MMC_DATA_WRITE){
                if( !(host->sw_timeout) &&
                        (host->hw->host_function == MSDC_SD) &&
                        (host->sclk > 100000000) &&
                        (host->write_timeout_uhs104 < MSDC_MAX_W_TIMEOUT_TUNE)){
                    if(host->t_counter.time_write)
                        host->t_counter.time_write--;
                    host->write_timeout_uhs104++;
                    msdc_tune_write(host);
                } else if ( !(host->sw_timeout) &&
                        (host->hw->host_function == MSDC_EMMC) &&
                        (host->write_timeout_emmc < MSDC_MAX_W_TIMEOUT_TUNE_EMMC)){
                    if(host->t_counter.time_write)
                        host->t_counter.time_write--;
                    host->write_timeout_emmc++;
#ifdef CONFIG_EMMC_50_FEATURE
                    if((host->id == 3) && (host->state == MSDC_STATE_HS400)){
                        emmc_hs400_tune_rw(host);
                    } else {
                        msdc_tune_write(host);
                    }
#else
                    msdc_tune_write(host);
#endif
                } else if((host->hw->host_function == MSDC_SD) && ((host->sw_timeout) || (host->write_timeout_uhs104 >= MSDC_MAX_W_TIMEOUT_TUNE) || (++(host->write_time_tune) > MSDC_MAX_TIMEOUT_RETRY))){
                    MSDC_DBG_PRINT(MSDC_TUNING, "msdc%d exceed max write timeout retry times(%d) or SW timeout(%d) or write timeout tuning time(%d),Power cycle",host->id,host->write_time_tune,host->sw_timeout,host->write_timeout_uhs104);
                    if(!(host->sd_30_busy) && msdc_power_tuning(host))
                        goto out;
                } else if((host->hw->host_function == MSDC_EMMC) && ((host->sw_timeout) || (++(host->write_time_tune) > MSDC_MAX_TIMEOUT_RETRY_EMMC))){
                    MSDC_DBG_PRINT(MSDC_TUNING, "msdc%d exceed max write timeout retry times(%d) or SW timeout(%d) or write timeout tuning time(%d),Power cycle",host->id,host->write_time_tune,host->sw_timeout,host->write_timeout_emmc);
                        host->write_timeout_emmc = 0;
                        goto out;
                }
            }
        }

        // clear the error condition.
        cmd->error = 0;
        if (data) data->error = 0;
        if (stop) stop->error = 0;

#ifdef MTK_MSDC_USE_CMD23
        if (sbc) sbc->error = 0;
#endif

        host->sw_timeout = 0;
        if (!is_card_present(host)) {
            goto out;
        }
    }while(msdc_tune_rw_request(mmc,mrq));

    if((host->rwcmd_time_tune)&&(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)){
        host->rwcmd_time_tune = 0;
        MSDC_DBG_PRINT(MSDC_TUNING, "RW cmd recover");
        msdc_dump_trans_error(host, cmd, data, stop, sbc);
    }
    if((host->read_time_tune)&&(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK) ){
        host->read_time_tune = 0;
        MSDC_DBG_PRINT(MSDC_TUNING, "Read recover");
        msdc_dump_trans_error(host, cmd, data, stop, sbc);
    }
    if((host->write_time_tune) && (cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)){
        host->write_time_tune = 0;
        MSDC_DBG_PRINT(MSDC_TUNING, "Write recover");
        msdc_dump_trans_error(host, cmd, data, stop, sbc);
    }
    host->power_cycle_enable = 1;
    host->sw_timeout = 0;

out:
    if(host->sclk <= 50000000 && (host->state != MSDC_STATE_DDR))
        host->sd_30_busy = 0;
    msdc_reset_crc_tune_counter(host,all_counter);
    host->mrq = NULL;
    msdc_gate_clock(host, 1); // clear flag.
    host->tune = 0;
    spin_unlock(&host->lock);

    //mmc_request_done(mmc, mrq);
       return;
}
static void msdc_ops_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
    struct mmc_data *data;
    int host_cookie = 0;
    struct msdc_host *host = mmc_priv(mmc);
    BUG_ON(mmc == NULL);
    BUG_ON(mrq == NULL);

    if ((host->hw->host_function == MSDC_SDIO) && !wake_lock_active(&host->trans_lock))
        wake_lock(&host->trans_lock);
    data = mrq->data;
    if (data){
        host_cookie = data->host_cookie;
    }
    //Asyn only support  DMA and asyc CMD flow
    if (msdc_async_use_dma(host_cookie))
    {
        msdc_do_request_async(mmc,mrq);
    }
    else
    {
        msdc_ops_request_legacy(mmc,mrq);
    }

    if ((host->hw->host_function == MSDC_SDIO) && wake_lock_active(&host->trans_lock))
        wake_unlock(&host->trans_lock);

    return;
}


/* called by ops.set_ios */
static void msdc_set_buswidth(struct msdc_host *host, u32 width)
{
	u32 val,mode, div;
    void __iomem *base = host->base;

	val = (width == MMC_BUS_WIDTH_8) ? 2 :
		      (width == MMC_BUS_WIDTH_4) ? 1 : 0;
			  
	sdr_set_field(SDC_CFG, SDC_CFG_BUSWIDTH, val);
	
	sdr_get_field(MSDC_CFG,MSDC_CFG_CKMOD,mode);
	sdr_get_field(MSDC_CFG,MSDC_CFG_CKDIV,div);
	MSDC_TRC_PRINT(MSDC_INFO,"[MSDC%d] MCLK (%uKHz), HCLK(%uMHz), SCLK(%dkHz) MODE(%d) DIV(%d) buswidth(%u-bits)\n",
			         host->id, host->mclk/1000, host->hclk/1000000,host->sclk/1000, mode, div,\
			         (width == MMC_BUS_WIDTH_8) ? 8 : (width == MMC_BUS_WIDTH_4) ? 4 : 1 );

}

static void msdc_apply_ett_settings (struct msdc_host *host, int mode) {
    unsigned int i = 0;
    void __iomem *base = host->base;
    struct msdc_ett_settings *ett = NULL;

    if(host->hw->ett_count == 0){
        return;
    }

    for(i = 0; i < host->hw->ett_count; i++) {
        ett = host->hw->ett_settings + i;
        if(mode == ett->speed_mode){
            sdr_set_field((volatile u32*)(base + ett->reg_addr), ett->reg_offset, ett->value);
            //MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: msdc%d, reg[0x%x], offset[0x%x], value[0x%x], readback[0x%x]\n", __func__, host->id, ett->reg_addr, ett->reg_offset, ett->value, sdr_read32((volatile u32*)(base + ett->reg_addr)));
        }
    }

    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->saved_para.cmd_resp_ta_cntr);
    host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
    host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);
    host->saved_para.ddly1 = sdr_read32(MSDC_DAT_RDDLY1);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
  #ifdef CONFIG_EMMC_50_FEATURE
    if((host->id == 3) && (mode == MSDC_HS400_MODE)) {
        sdr_get_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, host->saved_para.ds_dly1);
        sdr_get_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, host->saved_para.ds_dly3);
    }
  #endif
}

/* ops.set_ios */

static void msdc_ops_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct msdc_hw *hw=host->hw;
    void __iomem *base = host->base;
    u32 hs400 = 0, state = 0;
    //unsigned long flags;
    u32 cur_rxdly0, cur_rxdly1;

#ifdef MT_SD_DEBUG
    static char *vdd[] = {
        "1.50v", "1.55v", "1.60v", "1.65v", "1.70v", "1.80v", "1.90v",
        "2.00v", "2.10v", "2.20v", "2.30v", "2.40v", "2.50v", "2.60v",
        "2.70v", "2.80v", "2.90v", "3.00v", "3.10v", "3.20v", "3.30v",
        "3.40v", "3.50v", "3.60v"
    };
    static char *power_mode[] = {
        "OFF", "UP", "ON"
    };
    static char *bus_mode[] = {
        "UNKNOWN", "OPENDRAIN", "PUSHPULL"
    };
    static char *timing[] = {
        "LEGACY", "MMC_HS", "SD_HS"
    };

    MSDC_DBG_PRINT(MSDC_INFO,"[MSDC%u]SET_IOS: CLK(%dkHz), BUS(%s), BW(%u), PWR(%s), VDD(%s), TIMING(%s)",host->id,
        ios->clock / 1000, bus_mode[ios->bus_mode],
        (ios->bus_width == MMC_BUS_WIDTH_4) ? 4 : 1,
        power_mode[ios->power_mode], vdd[ios->vdd], timing[ios->timing]);
#endif

    spin_lock(&host->lock);

    if (ios->timing == MMC_TIMING_UHS_DDR50) {
        state = MSDC_STATE_DDR;
    } else if (ios->timing == MMC_TIMING_MMC_HS200) {
        state = MSDC_STATE_HS200;
    } else {
        state = MSDC_STATE_DEFAULT;
    }

#ifdef CONFIG_EMMC_50_FEATURE
    if (ios->timing == MMC_TIMING_MMC_HS400){
        hs400 = 1;
        state = MSDC_STATE_HS400;
        msdc_clock_src[host->id] = MSDC50_CLKSRC_400MHZ;
    } else {
    	  if (host->id == 0) {
            msdc_clock_src[host->id] = MSDC50_CLKSRC_200MHZ;
        }
    }
#endif

    msdc_ungate_clock(host);

    msdc_set_buswidth(host, ios->bus_width);

    /* Power control ??? */
    switch (ios->power_mode) {
        case MMC_POWER_OFF:
        case MMC_POWER_UP:
#ifndef FPGA_PLATFORM
            msdc_set_driving(host,hw,0);
#endif
            spin_unlock(&host->lock);
            msdc_set_power_mode(host, ios->power_mode);
            spin_lock(&host->lock);
            break;
        case MMC_POWER_ON:
            host->power_mode = MMC_POWER_ON;
            break;
        default:
            break;
    }
    if((msdc_host_mode[host->id] != mmc->caps) || (msdc_host_mode2[host->id] != mmc->caps2)) {
        mmc->caps = msdc_host_mode[host->id];
        mmc->caps2 = msdc_host_mode2[host->id];
#ifdef CONFIG_MTK_EMMC_SUPPORT
        if (1 == g_emmc_mode_switch){
          #ifdef CONFIG_EMMC_50_FEATURE
            if(!(mmc->caps2 & MMC_CAP2_HS200_1_8V_SDR) && !(mmc->caps2 & MMC_CAP2_HS400_1_8V_DDR)){
          #else
            if(!(mmc->caps2 & MMC_CAP2_HS200_1_8V_SDR)){
          #endif
                host->mmc->f_max = 50000000;
                if(mmc->card && mmc->card->ext_csd.hs_max_dtr  > 52000000){
                    mmc->card->state &= ~ (MMC_STATE_HIGHSPEED_200 | MMC_STATE_HIGHSPEED_400);
                    if((mmc->card->ext_csd.raw_card_type & EXT_CSD_CARD_TYPE_MASK) & EXT_CSD_CARD_TYPE_DDR_1_8V )
                        mmc->card->ext_csd.card_type |= EXT_CSD_CARD_TYPE_DDR_1_8V;
                }
            } else {
                host->mmc->f_max = HOST_MAX_MCLK;
                if(mmc->card && mmc->card->ext_csd.hs_max_dtr  > 52000000){
                    mmc->card->ext_csd.card_type &= ~EXT_CSD_CARD_TYPE_DDR_1_8V;
                }
            }
            MSDC_DBG_PRINT(MSDC_DEBUG,"[%s]: msdc%d, ext_csd.card_type=0x%x\n", __func__, host->id, mmc->card->ext_csd.card_type);
        }
#endif

        sdr_write32(MSDC_PAD_TUNE,   0x00000000);
	sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_RXDLYSEL, 1);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, host->hw->datwrddly);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRRDLY, host->hw->cmdrrddly);
        sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, host->hw->cmdrddly);
        sdr_write32(MSDC_IOCON,      0x00000000);
#if 1
        sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
        cur_rxdly0 = ((host->hw->dat0rddly & 0x1F) << 24) | ((host->hw->dat1rddly & 0x1F) << 16) |
            ((host->hw->dat2rddly & 0x1F) << 8)  | ((host->hw->dat3rddly & 0x1F) << 0);
        cur_rxdly1 = ((host->hw->dat4rddly & 0x1F) << 24) | ((host->hw->dat5rddly & 0x1F) << 16) |
            ((host->hw->dat6rddly & 0x1F) << 8)  | ((host->hw->dat7rddly & 0x1F) << 0);
        sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
        sdr_write32(MSDC_DAT_RDDLY1, cur_rxdly1);
#else
        sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);
        sdr_write32(MSDC_DAT_RDDLY1, 0x00000000);
#endif
#if 0    //use default setting for MT8163
        //sdr_write32(MSDC_PATCH_BIT0, 0x403C004F); /* bit0 modified: Rx Data Clock Source: 1 -> 2.0*/
        if ((host->hw->host_function == MSDC_EMMC) || (host->hw->host_function == MSDC_SD)){
            sdr_write32(MSDC_PATCH_BIT1, 0xFFFE00C9);
        } else {
            sdr_write32(MSDC_PATCH_BIT1, 0xFFFE0009);
        }
#endif

        if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {  /* internal clock: latch read data, not apply to sdio */
            //sdr_set_bits(MSDC_PATCH_BIT0, MSDC_PATCH_BIT_CKGEN_CK);
            host->hw->cmd_edge  = 0; // tuning from 0
            host->hw->rdata_edge = 0;
            host->hw->wdata_edge = 0;
        }else if (hw->flags & MSDC_INTERNAL_CLK) {
            //sdr_set_bits(MSDC_PATCH_BIT0, MSDC_PATCH_BIT_CKGEN_CK);
        }
    }
    if(msdc_clock_src[host->id] != hw->clk_src){
        hw->clk_src = msdc_clock_src[host->id];
        msdc_select_clksrc(host, hw->clk_src,hw->hclk_src);
    }

    if (host->mclk != ios->clock || host->state != state) { /* not change when clock Freq. not changed state need set clock*/
        if(ios->clock >= 25000000) {
            if(ios->clock > 100000000){
#ifndef FPGA_PLATFORM
                msdc_set_driving(host,hw,1);
#endif
            }

            if( is_card_sdio(host) && sdio_enable_tune) {
                //Only Enable when ETT is running
                u32 cur_rxdly0;//,cur_rxdly1;

                sdio_tune_flag = 0;
                sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
                //Latch edge
                host->hw->cmd_edge = sdio_iocon_rspl;
                host->hw->rdata_edge = sdio_iocon_dspl;
                host->hw->wdata_edge = sdio_iocon_w_dspl;

                //CMD and DATA delay
                sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, sdio_pad_tune_rdly);
                sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRRDLY, sdio_pad_tune_rrdly);
                sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATWRDLY, sdio_pad_tune_wrdly);
                sdr_set_field(MSDC_PAD_TUNE,MSDC_PAD_TUNE_DATRRDLY, sdio_dat_rd_dly0_0);
                cur_rxdly0 = (sdio_dat_rd_dly0_0 << 24) | (sdio_dat_rd_dly0_1 << 16) |
                    (sdio_dat_rd_dly0_2 << 8) | (sdio_dat_rd_dly0_3 << 0);

                host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
                host->saved_para.ddly0 = cur_rxdly0;
            }

            //MSDC_DBG_PRINT(MSDC_DEBUG, "SD latch rdata<%d> wdatea<%d> cmd<%d>", hw->rdata_edge,hw->wdata_edge, hw->cmd_edge);
            sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, hw->ddlsel);
            msdc_set_smpl(host, hs400, hw->cmd_edge, TYPE_CMD_RESP_EDGE, NULL);
            msdc_set_smpl(host, hs400, hw->rdata_edge, TYPE_READ_DATA_EDGE, NULL);
            msdc_set_smpl(host, hs400, hw->wdata_edge, TYPE_WRITE_CRC_EDGE, NULL);
            if ((host->hw->host_function == MSDC_EMMC) || (host->hw->host_function == MSDC_SD)){
                sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, host->saved_para.write_busy_margin);
                sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA, host->saved_para.write_crc_margin);
            }
#ifndef CONFIG_EMMC_50_FEATURE
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->saved_para.cmd_resp_ta_cntr);
            sdr_write32(MSDC_PAD_TUNE,host->saved_para.pad_tune);
            sdr_write32(MSDC_DAT_RDDLY0,host->saved_para.ddly0);
            sdr_write32(MSDC_DAT_RDDLY1,host->saved_para.ddly1);
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
#else
            if(host->id != 3) {
                sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->saved_para.cmd_resp_ta_cntr);
                sdr_write32(MSDC_PAD_TUNE,host->saved_para.pad_tune);
                sdr_write32(MSDC_DAT_RDDLY0,host->saved_para.ddly0);
                sdr_write32(MSDC_DAT_RDDLY1,host->saved_para.ddly1);
                sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
                if((host->id == 1) || (host->id == 2)){
                    sdr_set_field(MSDC_PATCH_BIT1, (1 << 6),1);
                    sdr_set_field(MSDC_PATCH_BIT1, (1 << 7),1);
                }
            }
#endif
#ifdef MTK_EMMC_ETT_TO_DRIVER
            if (host->hw->host_function == MSDC_EMMC) {
                msdc_ett_offline_to_driver(host);
            }
#endif
        }

        if (ios->clock == 0) {
            if(ios->power_mode == MMC_POWER_OFF){
                msdc_set_smpl(host, hs400, hw->cmd_edge, TYPE_CMD_RESP_EDGE, NULL);
                msdc_set_smpl(host, hs400, hw->rdata_edge, TYPE_READ_DATA_EDGE, NULL);
                msdc_set_smpl(host, hs400, hw->wdata_edge, TYPE_WRITE_CRC_EDGE, NULL);
                host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
                host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);
                host->saved_para.ddly1 = sdr_read32(MSDC_DAT_RDDLY1);
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->saved_para.cmd_resp_ta_cntr);
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, host->saved_para.write_busy_margin);
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA, host->saved_para.write_crc_margin);
#ifdef CONFIG_EMMC_50_FEATURE
               if(host->id == 3 ){
                   sdr_get_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, host->saved_para.ds_dly1);
                   sdr_get_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, host->saved_para.ds_dly3);
               }
#endif
                //MSDC_DBG_PRINT(MSDC_DEBUG, "save latch rdata<%d> wdata<%d> cmd<%d>", hw->rdata_edge,hw->wdata_edge,hw->cmd_edge);
            }
            /* reset to default value */
            sdr_write32(MSDC_IOCON,      0x00000000);
            sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);
            sdr_write32(MSDC_DAT_RDDLY1, 0x00000000);
            sdr_write32(MSDC_PAD_TUNE,   0x00000000);
	    sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_RXDLYSEL, 1);
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,1);
            sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR,1);
            if ((host->hw->host_function == MSDC_EMMC) || (host->hw->host_function == MSDC_SD)){
                sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, 1);
                sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA, 1);
            }
        }
        //MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: id:%d, host->state=0x%x, state=0x%x, MSDC_STATE_HS400=0x%x, MSDC_STATE_HS200=0x%x\n", __func__, host->id, host->state, state, MSDC_STATE_HS400, MSDC_STATE_HS200);
        if((host->id == 3) && (host->state != state)) {
            #ifdef CONFIG_EMMC_50_FEATURE
            if (state == MSDC_STATE_HS400){//switch from eMMC 4.5 backward speed mode to HS400
                emmc_hs400_backup(host);
                msdc_apply_ett_settings(host, MSDC_HS400_MODE);
            }
            if (host->state == MSDC_STATE_HS400){//switch from HS400 to eMMC 4.5 backward speed mode
                emmc_hs400_restore(host);
            }
            #endif
            if(state == MSDC_STATE_HS200){
                msdc_apply_ett_settings(host, MSDC_HS200_MODE);
            }
        }
        msdc_set_mclk(host, state, ios->clock);
    }

#if 0  // PM Resume -> set 0 -> 260KHz
    if (ios->clock == 0) { // only gate clock when set 0Hz
        msdc_gate_clock(host, 1);
    }
#endif
    msdc_gate_clock(host, 1);
    spin_unlock(&host->lock);
}

/* ops.get_ro */
static int msdc_ops_get_ro(struct mmc_host *mmc)
{
    struct msdc_host *host = mmc_priv(mmc);
    void __iomem *base = host->base;
    unsigned long flags;
    int ro = 0;

    spin_lock_irqsave(&host->lock, flags);
    msdc_ungate_clock(host);
    if (host->hw->flags & MSDC_WP_PIN_EN) { /* set for card */
        ro = (sdr_read32(MSDC_PS) >> 31);
    }
    msdc_gate_clock(host, 1);
    spin_unlock_irqrestore(&host->lock, flags);
    return ro;
}

/* ops.get_cd */
static int msdc_ops_get_cd(struct mmc_host *mmc)
{
    struct msdc_host *host = mmc_priv(mmc);
    void __iomem *base;
    unsigned long flags;
    //int present = 1;

    base = host->base;
    spin_lock_irqsave(&host->lock, flags);

    /* for sdio, depends on USER_RESUME */
    if (is_card_sdio(host)) {
        if (!(host->hw->flags & MSDC_SDIO_IRQ))
        {
            host->card_inserted = (host->pm_state.event == PM_EVENT_USER_RESUME) ? 1 : 0;
            //INIT_MSG("sdio ops_get_cd<%d>", host->card_inserted);
            goto end;
        }
    }

    /* for emmc, MSDC_REMOVABLE not set, always return 1 */
    if (!(host->hw->flags & MSDC_REMOVABLE)) {
        host->card_inserted = 1;
        goto end;
    }

    //msdc_ungate_clock(host);

    if (host->hw->flags & MSDC_CD_PIN_EN) { /* for card, MSDC_CD_PIN_EN set*/
        if(host->hw->cd_level)
            host->card_inserted = (host->sd_cd_polarity == 0) ? 1 : 0;
        else
            host->card_inserted = (host->sd_cd_polarity == 0) ? 0 : 1;
    } else {
        host->card_inserted = 1; /* TODO? Check DAT3 pins for card detection */
    }

    //host->card_inserted = 1;
#if 0
    if (host->card_inserted == 0) {
        msdc_gate_clock(host, 0);
    }else {
        msdc_gate_clock(host, 1);
    }
#endif
    if(host->hw->host_function == MSDC_SD && host->block_bad_card)
        host->card_inserted = 0;
    MSDC_TRC_PRINT(MSDC_INIT,"Card insert<%d> Block bad card<%d>", host->card_inserted,host->block_bad_card);
end:
    spin_unlock_irqrestore(&host->lock, flags);
    return host->card_inserted;
}

/* ops.enable_sdio_irq */
static void msdc_ops_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct msdc_hw *hw = host->hw;
    void __iomem *base = host->base;
    u32 tmp;

    if (hw->flags & MSDC_EXT_SDIO_IRQ) { /* yes for sdio */
        if (enable) {
            hw->enable_sdio_eirq();  /* combo_sdio_enable_eirq */
        } else {
            hw->disable_sdio_eirq(); /* combo_sdio_disable_eirq */
        }
    } else {
       /* so never enter here */
#if (MSDC_DATA1_INT == 1)
       if (host->hw->flags & MSDC_SDIO_IRQ)
       {
           int_sdio_irq_enable = enable;
           if (!u_sdio_irq_counter)
           {
               MSDC_DBG_PRINT(MSDC_DEBUG, "msdc2 u_sdio_irq_counter=1 \n");
           }

           if (u_sdio_irq_counter < 0xFFFF)
               u_sdio_irq_counter = u_sdio_irq_counter + 1;
           else
               u_sdio_irq_counter = 1;

           if (u_sdio_irq_counter <7)
               MSDC_DBG_PRINT(MSDC_DEBUG,"msdc2 sdio_irq enable: %d \n",int_sdio_irq_enable);

           //MSDC_DBG_PRINT(MSDC_DEBUG,"Ahsin int_sdio_irq_enable=%d  u_sdio_irq_counter=%d",int_sdio_irq_enable,u_sdio_irq_counter);
       }
#endif
       tmp = sdr_read32(SDC_CFG);
       if (enable) {
           tmp |= (SDC_CFG_SDIOIDE | SDC_CFG_SDIOINTWKUP);
#if (MSDC_DATA1_INT == 1)
           sdr_set_bits(MSDC_INTEN, MSDC_INT_SDIOIRQ);
#endif
       } else {
           //tmp &= ~(SDC_CFG_SDIOIDE | SDC_CFG_SDIOINTWKUP);
#if (MSDC_DATA1_INT == 1)
           sdr_clr_bits(MSDC_INTEN, MSDC_INT_SDIOIRQ);
#endif
       }
       sdr_write32(SDC_CFG, tmp);
    }
}

static int msdc_ops_switch_volt(struct mmc_host *mmc, struct mmc_ios *ios)
{
    struct msdc_host *host = mmc_priv(mmc);
    void __iomem *base = host->base;
    int err = 0;
    u32 timeout = 100;
    u32 retry = 10;
    u32 status;
    u32 sclk = host->sclk;
    u32 u4swvolt=0;

    if (host->hw->host_function == MSDC_EMMC)
        return 0;

    if(ios->signal_voltage != MMC_SIGNAL_VOLTAGE_330){
        /* make sure SDC is not busy (TBC) */
        //WAIT_COND(!SDC_IS_BUSY(), timeout, timeout);
        err = (unsigned int)-EIO;
        msdc_retry(sdc_is_busy(), retry, timeout,host->id);
        if (timeout == 0 && retry == 0) {
            err = (unsigned int)-ETIMEDOUT;
            goto out;
        }

        /* pull up disabled in CMD and DAT[3:0] to allow card drives them to low */
        /* check if CMD/DATA lines both 0 */
        if ((sdr_read32(MSDC_PS) & (MSDC_PS_CMD | MSDC_PS_DAT04)) == 0) {
            /* pull up disabled in CMD and DAT[3:0] */
            msdc_pin_config(host, MSDC_PIN_PULL_NONE);

            /* change signal from 3.3v to 1.8v for FPGA this can not work*/
            if(ios->signal_voltage == MMC_SIGNAL_VOLTAGE_180){
#ifdef FPGA_PLATFORM
                hwPowerSwitch_fpga(host->id, 1);
#else
                if(host->power_switch)
                    host->power_switch(host,1);
                else
                    MSDC_ERR_PRINT(MSDC_ERROR, "No power switch callback. Please check host_function<0x%lx>",host->hw->host_function);
#endif
            }
            /* wait at least 5ms for 1.8v signal switching in card */
            mdelay(10);

            /* config clock to 10~12MHz mode for volt switch detection by host. */
			if(sclk > 65000000){
				/* config clock to 260KHz~65MHz mode for volt switch detection by host. */
				msdc_set_mclk(host, MSDC_STATE_DEFAULT, HOST_MIN_MCLK);
			}
			
			 u4swvolt = host->sclk/1000;
			 sdr_set_field(SDC_VOL_CHG,SDC_VOL_CHG_VCHGCNT, u4swvolt);
			 MSDC_TRC_PRINT(MSDC_INFO, "[MSDC%u][msdc_ops_switch_volt] sd sclk %u SDC_VOL_CHG=0x%x\n", host->id, host->sclk, u4swvolt);


            /* pull up enabled in CMD and DAT[3:0] */
            msdc_pin_config(host, MSDC_PIN_PULL_UP);
            mdelay(105);

            /* start to detect volt change by providing 1.8v signal to card */
            sdr_set_bits(MSDC_CFG, MSDC_CFG_BV18SDT);

            /* wait at max. 1ms */
            mdelay(1);
            //ERR_MSG("before read status");

            while ((status = sdr_read32(MSDC_CFG)) & MSDC_CFG_BV18SDT);

            if (status & MSDC_CFG_BV18PSS){
                err = 0;
            }
            else {
				MSDC_ERR_PRINT(MSDC_ERROR, "msdc V1800 status (0x%x),err(%d)",status,err);
            }
            /* config clock back to init clk freq. */
		    if(sclk > 65000000){	
                msdc_set_mclk(host, MSDC_STATE_DEFAULT, sclk);
		    }
        }
    }
out:

    return err;
}

/*                          mmc_pre_req()                                                                                     __mmc_start_req()
 *                              | |                                                                                                  | |
 * async way: mmc_start_req() ->  __mmc_start_req() -> mmc_start_request() -> request() -> mmc_wait_for_req_done() -> msdc_ops_stop()  -> mmc_post_req()
 * legacy way: mmc_wait_for_req() -> __mmc_start_req -> mmc_start_request() -> request()
 * msdc_send_stop() just for async way. for when to trigger stop cmd(arg=0):
 * 1 aysnc way but used pio mode will call msdc_ops_request_legacy(), and pio mode will disable autocmd12, so cmd12 will send in msdc_do_request()
 * 2 aysnc way with non-cmd23 mode: if host not used autocmd12, sw need send cmd12 after mrq->completion(polling under mmc_core.c) is done
 * 3 aysnc way with cmd23 mode: no need to send cmd12 here
 * sd card will not enable cmd23 */
static void msdc_ops_stop(struct mmc_host *mmc,struct mmc_request *mrq)
{
    struct msdc_host *host = mmc_priv(mmc);
    u32 err = -1;

    if (msdc_async_use_pio(mrq->data->host_cookie))
    {
        return;
    }
    if(host->hw->host_function != MSDC_SDIO){
        if(!mrq->stop)
            return;

#ifndef MTK_MSDC_USE_CMD23
        if((host->autocmd & MSDC_AUTOCMD12))
            return;
#else
        if (host->hw->host_function == MSDC_EMMC) {
            /* if transfer error occur, cmd12 will send under msdc_abort_data() */
            if (mrq->sbc)
                return;
        } else {
            if ((host->autocmd & MSDC_AUTOCMD12))
                return;
        }
#endif /* end of MTK_MSDC_USE_CMD23 */

        MSDC_DBG_PRINT(MSDC_STACK, "MSDC Stop for non-autocmd12 host->error(%d)host->autocmd(%d)",host->error,host->autocmd);
        err = msdc_do_command(host, mrq->stop, 0, CMD_TIMEOUT);
        if(err){
            if (mrq->stop->error == (unsigned int)-EIO) host->error |= REQ_STOP_EIO;
            if (mrq->stop->error == (unsigned int)-ETIMEDOUT) host->error |= REQ_STOP_TMO;
        }
    }
}

extern u32 __mmc_sd_num_wr_blocks(struct mmc_card* card);
static bool msdc_check_written_data(struct mmc_host *mmc,struct mmc_request *mrq)
{
    u32 result = 0;
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_card *card;
    if (msdc_async_use_pio(mrq->data->host_cookie))
    {
        return 0;
    }
    if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
        MSDC_DBG_PRINT(MSDC_DEBUG, "cmd<%d> arg<0x%x> card<%d> power<%d>", mrq->cmd->opcode,mrq->cmd->arg,is_card_present(host), host->power_mode);
        mrq->cmd->error = (unsigned int)-ENOMEDIUM;
        return 0;
    }
    if(mmc->card)
        card = mmc->card;
    else
        return 0;
    if((host->hw->host_function == MSDC_SD)
        && (host->sclk > 100000000)
        && mmc_card_sd(card)
        && (mrq->data)
        && (mrq->data->flags & MMC_DATA_WRITE)
        && (host->error == 0)){
        msdc_ungate_clock(host);
        spin_lock(&host->lock);
        if(msdc_polling_idle(host)){
            spin_unlock(&host->lock);
            msdc_gate_clock(host, 1);
            return 0;
        }
        spin_unlock(&host->lock);
        result = __mmc_sd_num_wr_blocks(card);
        if((result != mrq->data->blocks) && (is_card_present(host)) && (host->power_mode == MMC_POWER_ON)){
            mrq->data->error = (unsigned int)-EIO;
            host->error |= REQ_DAT_ERR;
            MSDC_DBG_PRINT(MSDC_DEBUG, "written data<%d> blocks isn't equal to request data blocks<%d>",result,mrq->data->blocks);
            msdc_gate_clock(host, 1);
            return 1;
        }
        msdc_gate_clock(host, 1);
    }
    return 0;
}

extern void init_tune_sdio(struct msdc_host *host);
int msdc_execute_tuning(struct mmc_host *mmc, u32 opcode){
    struct msdc_host *host = mmc_priv(mmc);
    if(host->hw->host_function == MSDC_SDIO){
        init_tune_sdio(host);
    }
    return 0;
}

static void msdc_dma_error_reset(struct mmc_host *mmc)
{
    struct msdc_host *host = mmc_priv(mmc);
    void __iomem *base = host->base;
    struct mmc_data *data = host->data;
    if(data && host->dma_xfer && (msdc_async_use_dma(data->host_cookie)) && (host->tune == 0))
    {
        host->sw_timeout++;
        host->error |= REQ_DAT_ERR;
        MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
#ifdef MSDC_TOP_RESET_ERROR_TUNE
        if((host->id == 3) && (CHIP_SW_VER_01 == mt_get_chip_sw_ver())){
            msdc_reset_gdma();
        }else {
            msdc_reset(host->id);
        }
#else
        msdc_reset(host->id);
#endif
        msdc_dma_stop(host);
        msdc_clr_fifo(host->id);
        msdc_clr_int();

#ifdef MSDC_TOP_RESET_ERROR_TUNE
        if((host->id == 3) && (CHIP_SW_VER_01 == mt_get_chip_sw_ver())){
            msdc_top_reset(host);
        }
#endif
        msdc_dma_clear(host);
        msdc_gate_clock(host, 1);
    }
}
static struct mmc_host_ops mt_msdc_ops = {
    .post_req                      = msdc_post_req,
    .pre_req                       = msdc_pre_req,
    .request                       = msdc_ops_request,
    .tuning                        = msdc_tune_async_request,
    .set_ios                       = msdc_ops_set_ios,
    .get_ro                        = msdc_ops_get_ro,
    .get_cd                        = msdc_ops_get_cd,
    .enable_sdio_irq               = msdc_ops_enable_sdio_irq,
    .start_signal_voltage_switch   = msdc_ops_switch_volt,
    .send_stop                     = msdc_ops_stop,
    .dma_error_reset               = msdc_dma_error_reset,
    .check_written_data            = msdc_check_written_data,
    .execute_tuning                = msdc_execute_tuning,
};

/*--------------------------------------------------------------------------*/
/* interrupt handler                 */
/*--------------------------------------------------------------------------*/
//static __tcmfunc irqreturn_t msdc_irq(int irq, void *dev_id)
#ifndef FPGA_PLATFORM
static irqreturn_t msdc1_eint_handler(void)
{
    struct msdc_host *host = mtk_msdc_host[1];
    int got_bad_card = 0;
    unsigned long flags;

    spin_lock_irqsave(&host->remove_bad_card,flags);
    if (host->hw->cd_level ^ host->sd_cd_polarity) {
        got_bad_card = host->block_bad_card;
        host->card_inserted = 0;
        if (host->mmc && host->mmc->card) {
            mmc_card_set_removed(host->mmc->card);
        }
    }
    host->sd_cd_polarity = (~(host->sd_cd_polarity))&0x1;
    spin_unlock_irqrestore(&host->remove_bad_card,flags);
    host->block_bad_card = 0;

    if (host->sd_cd_polarity == 0) {
        irq_set_irq_type(cd_irq, IRQ_TYPE_LEVEL_LOW);
    } else {
        irq_set_irq_type(cd_irq, IRQ_TYPE_LEVEL_HIGH);
    }

    if (got_bad_card == 0)
        tasklet_hi_schedule(&host->card_tasklet);
    MSDC_DBG_PRINT(MSDC_IRQ,"SD card %s(%x:%x)",(host->hw->cd_level ^ host->sd_cd_polarity) ? "insert":"remove", host->hw->cd_level, host->sd_cd_polarity);

    return IRQ_HANDLED;
}
#endif

#if defined(FEATURE_MET_MMC_INDEX)
extern void met_mmc_dma_stop(struct mmc_host *host, u32 lba, unsigned int len, u32 opcode, unsigned int bd_num);
#endif

static irqreturn_t msdc_irq(int irq, void *dev_id)
{
    struct msdc_host  *host = (struct msdc_host *)dev_id;
    struct mmc_data   *data = host->data;
    struct mmc_command *cmd = host->cmd;
    struct mmc_command *stop = NULL;
    struct mmc_request *mrq = NULL;
    void __iomem *base = host->base;

    u32 cmdsts = MSDC_INT_RSPCRCERR  | MSDC_INT_CMDTMO  | MSDC_INT_CMDRDY  |
                 MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO | MSDC_INT_ACMDRDY |
                 MSDC_INT_ACMD19_DONE;
    u32 datsts = MSDC_INT_DATCRCERR  |MSDC_INT_DATTMO;
    u32 intsts, inten;

    if (0 == host->core_clkon) {
#ifndef FPGA_PLATFORM
        msdc_enable_clock(host);
#endif
        host->core_clkon = 1;
        sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);
        intsts = sdr_read32(MSDC_INT);
#if 0
        if (sdr_read32(MSDC_ECO_VER) >= 4) {
            sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);  /* E2 */
            intsts = sdr_read32(MSDC_INT);
            sdr_set_field(MSDC_CLKSRC_REG, MSDC1_IRQ_SEL, 0);
        } else {
            intsts = sdr_read32(MSDC_INT);
        }
#endif
    } else {
        intsts = sdr_read32(MSDC_INT);
    }

    latest_int_status[host->id] = intsts;
    inten  = sdr_read32(MSDC_INTEN);
#if (MSDC_DATA1_INT == 1)
     if (host->hw->flags & MSDC_SDIO_IRQ)
     {
         intsts &= inten;
     }
     else
#endif
     {
          inten &= intsts;
     }

    sdr_write32(MSDC_INT, intsts);  /* clear interrupts */



    /* MSG will cause fatal error */
#if 0
    /* card change interrupt */
    if (intsts & MSDC_INT_CDSC){
        IRQ_MSG("MSDC_INT_CDSC irq<0x%.8x>", intsts);
        tasklet_hi_schedule(&host->card_tasklet);
        /* tuning when plug card ? */
    }
#endif
    /* sdio interrupt */
    if (intsts & MSDC_INT_SDIOIRQ){
        MSDC_DBG_PRINT(MSDC_IRQ, "XXX MSDC_INT_SDIOIRQ");  /* seems not sdio irq */
#if (MSDC_DATA1_INT == 1)
        if (host->hw->flags & MSDC_SDIO_IRQ)
        {
            if (!u_msdc_irq_counter)
            {
                MSDC_DBG_PRINT(MSDC_IRQ, "msdc2 u_msdc_irq_counter=1 \n");
            }
            if (u_msdc_irq_counter <0xFFFF)
                u_msdc_irq_counter = u_msdc_irq_counter + 1;
            else
                u_msdc_irq_counter = 1;

            //if (u_msdc_irq_counter < 3)
                //printk("msdc2 u_msdc_irq_counter=%d SDC_CFG=%x MSDC_INTEN=%x MSDC_INT=%x \n",u_msdc_irq_counter,sdr_read32(SDC_CFG),sdr_read32(MSDC_INTEN),sdr_read32(MSDC_INT));
            //ERR_MSG("Ahsin u_msdc_irq_counter=%d SDC_CFG=%x MSDC_INTEN=%x MSDC_INT=%x MSDC_PATCH_BIT0=%x",u_msdc_irq_counter,sdr_read32(SDC_CFG),sdr_read32(MSDC_INTEN),sdr_read32(MSDC_INT),sdr_read32(MSDC_PATCH_BIT0));

            mmc_signal_sdio_irq(host->mmc);
        }
#endif
    }

    /* transfer complete interrupt */
    if (data != NULL) {
#ifdef MTK_MSDC_ERROR_TUNE_DEBUG
        if(g_err_tune_dbg_error && (g_err_tune_dbg_count > 0) && (g_err_tune_dbg_host == host->id)) {
            if(g_err_tune_dbg_cmd == (sdr_read32(SDC_CMD) & SDC_CMD_OPC)){
                if(g_err_tune_dbg_error & MTK_MSDC_ERROR_DAT_TMO){
                    intsts = MSDC_INT_DATTMO;
                    g_err_tune_dbg_count--;
                }else if (g_err_tune_dbg_error & MTK_MSDC_ERROR_DAT_CRC){
                    intsts = MSDC_INT_DATCRCERR;
                    g_err_tune_dbg_count--;
                }
                MSDC_DBG_PRINT(MSDC_IRQ,"[%s]: got the error cmd:%d, dbg error 0x%x, data->error=%d, count=%d \n", __func__, g_err_tune_dbg_cmd, g_err_tune_dbg_error,data->error, g_err_tune_dbg_count);
            }

            if((g_err_tune_dbg_cmd == MMC_STOP_TRANSMISSION) && stop && (host->autocmd & MSDC_AUTOCMD12)){
                if(g_err_tune_dbg_error & MTK_MSDC_ERROR_ACMD_TMO){
                    intsts =MSDC_INT_ACMDTMO;
                    g_err_tune_dbg_count--;
                }else if(g_err_tune_dbg_error & MTK_MSDC_ERROR_ACMD_CRC){
                    intsts =MSDC_INT_ACMDCRCERR;
                    g_err_tune_dbg_count--;
                }
                MSDC_DBG_PRINT(MSDC_IRQ, "[%s]: got the error cmd:%d, dbg error 0x%x, stop->error=%d, host->error=%d, count=%d\n", __func__, g_err_tune_dbg_cmd, g_err_tune_dbg_error, stop->error, host->error, g_err_tune_dbg_count);
            }
        }
#endif
        stop = data->stop;
#if (MSDC_DATA1_INT == 1)
        if (host->hw->flags & MSDC_SDIO_IRQ)
        {
            if (intsts & MSDC_INT_XFER_COMPL) {
                data->bytes_xfered = host->dma.xfersz;
            if((msdc_async_use_dma(data->host_cookie)) && (host->tune == 0)){
                    msdc_dma_stop(host);
                    mrq = host->mrq;
                    msdc_dma_clear(host);
                    if(mrq->done)
                        mrq->done(mrq);
                    msdc_gate_clock(host, 1);
                    host->error &= ~REQ_DAT_ERR;
                } else {
                    complete(&host->xfer_done);
                }

#if defined(FEATURE_MET_MMC_INDEX)
                if ((data->mrq != NULL) && (data->mrq->cmd != NULL)) {
                    met_mmc_dma_stop(host->mmc, data->mrq->cmd->arg, data->blocks, data->mrq->cmd->opcode, met_mmc_bdnum);
                }
#endif
            }
        }else
#endif
        {
            if (inten & MSDC_INT_XFER_COMPL) {
                data->bytes_xfered = host->dma.xfersz;
                if ((msdc_async_use_dma(data->host_cookie)) && (host->tune == 0)){
                    msdc_dma_stop(host);
                    mrq = host->mrq;
                    msdc_dma_clear(host);
                    if(mrq->done)
                        mrq->done(mrq);
                    msdc_gate_clock(host, 1);
                    host->error &= ~REQ_DAT_ERR;
                }else {
                    complete(&host->xfer_done);
                }

#if defined(FEATURE_MET_MMC_INDEX)
                if ((data->mrq != NULL) && (data->mrq->cmd != NULL)) {
                    met_mmc_dma_stop(host->mmc, data->mrq->cmd->arg, data->blocks, data->mrq->cmd->opcode, met_mmc_bdnum);
                }
#endif
            }
        }

        if (intsts & datsts) {
            /* do basic reset, or stop command will sdc_busy */
            if (intsts & MSDC_INT_DATTMO){
                MSDC_TRACE(MSDC_DEBUG, msdc_dump_info,(host->id));
            }

#ifdef MSDC_TOP_RESET_ERROR_TUNE
            if((host->id == 3) && host->dma_xfer && (CHIP_SW_VER_01 == mt_get_chip_sw_ver())){
                msdc_reset_gdma();
            }else {
                if (host->dma_xfer) {
                    msdc_reset(host->id);
                }
                else {
                    msdc_reset_hw(host->id);
                }
            }
#else
            if (host->dma_xfer) {
                msdc_reset(host->id);
            }
            else {
                msdc_reset_hw(host->id);
            }
#endif
            atomic_set(&host->abort, 1);  /* For PIO mode exit */

            if (intsts & MSDC_INT_DATTMO){
                data->error = (unsigned int)-ETIMEDOUT;
                msdc_set_timeout(host, host->timeout_ns * 2, host->timeout_clks);
                MSDC_DBG_PRINT(MSDC_IRQ,"XXX CMD<%d> Arg<0x%.8x> MSDC_INT_DATTMO", host->mrq->cmd->opcode, host->mrq->cmd->arg);
            }
            else if (intsts & MSDC_INT_DATCRCERR){
                data->error = (unsigned int)-EIO;
                MSDC_DBG_PRINT(MSDC_IRQ,"XXX CMD<%d> Arg<0x%.8x> MSDC_INT_DATCRCERR, SDC_DCRC_STS<0x%x>",
                      host->mrq->cmd->opcode, host->mrq->cmd->arg, sdr_read32(SDC_DCRC_STS));
            }

            if (host->dma_xfer) {
                 if ((msdc_async_use_dma(data->host_cookie)) && (host->tune == 0)){
                    msdc_dma_stop(host);
                    msdc_clr_fifo(host->id);
                    msdc_clr_int();
#ifdef MSDC_TOP_RESET_ERROR_TUNE
                    if((host->id == 3) && (CHIP_SW_VER_01 == mt_get_chip_sw_ver())){
                        msdc_top_reset(host);
                    }
#endif
                    mrq = host->mrq;
                    msdc_dma_clear(host);
                    if(mrq->done)
                        mrq->done(mrq);
                    msdc_gate_clock(host, 1);
                    host->error |= REQ_DAT_ERR;
                } else {
                    complete(&host->xfer_done); /* Read CRC come fast, XFER_COMPL not enabled */
                }

#if defined(FEATURE_MET_MMC_INDEX)
                if ((data->mrq != NULL) && (data->mrq->cmd != NULL)) {
                    met_mmc_dma_stop(host->mmc, data->mrq->cmd->arg, data->blocks, data->mrq->cmd->opcode, met_mmc_bdnum);
                }
#endif
            } /* PIO mode can't do complete, because not init */
        }
        if ((stop != NULL) && (host->autocmd & MSDC_AUTOCMD12) && (intsts & cmdsts)) {
            if (intsts & MSDC_INT_ACMDRDY) {
                u32 *arsp = &stop->resp[0];
               *arsp = sdr_read32(SDC_ACMD_RESP);
            }else if (intsts & MSDC_INT_ACMDCRCERR) {
                stop->error =(unsigned int)-EIO;
                host->error |= REQ_STOP_EIO;
                if (host->dma_xfer) {
                    msdc_reset(host->id);
                }
                else {
                    msdc_reset_hw(host->id);
                }
            } else if (intsts & MSDC_INT_ACMDTMO) {
                stop->error =(unsigned int)-ETIMEDOUT;
                host->error |= REQ_STOP_TMO;
                if (host->dma_xfer) {
                    msdc_reset(host->id);
                }
                else {
                    msdc_reset_hw(host->id);
                }
            }
            if((intsts & MSDC_INT_ACMDCRCERR) || (intsts & MSDC_INT_ACMDTMO)){
                if (host->dma_xfer){
                     if ((msdc_async_use_dma(data->host_cookie)) && (host->tune == 0)){
                        msdc_dma_stop(host);
                        msdc_clr_fifo(host->id);
                        msdc_clr_int();
                        mrq = host->mrq;
                        msdc_dma_clear(host);
                        if(mrq->done)
                            mrq->done(mrq);
                        msdc_gate_clock(host, 1);
                    } else {
                        complete(&host->xfer_done); //Autocmd12 issued but error occur, the data transfer done INT will not issue,so cmplete is need here
                    }

#if defined(FEATURE_MET_MMC_INDEX)
                if ((data->mrq != NULL) && (data->mrq->cmd != NULL)) {
                    met_mmc_dma_stop(host->mmc, data->mrq->cmd->arg, data->blocks, data->mrq->cmd->opcode, met_mmc_bdnum);
                }
#endif
                }/* PIO mode can't do complete, because not init */
            }
        }
    }
    /* command interrupts */
    if ((cmd != NULL) && (intsts & cmdsts)) {
#ifdef MTK_MSDC_ERROR_TUNE_DEBUG
        if(g_err_tune_dbg_error && (g_err_tune_dbg_count > 0) && (g_err_tune_dbg_host == host->id)) {
            if(g_err_tune_dbg_cmd == cmd->opcode){
                if((g_err_tune_dbg_cmd != MMC_SWITCH) || ((g_err_tune_dbg_cmd == MMC_SWITCH) && (g_err_tune_dbg_arg == ((cmd->arg >> 16) & 0xff)))){
                    if(g_err_tune_dbg_error & MTK_MSDC_ERROR_CMD_TMO){
                        intsts = MSDC_INT_CMDTMO;
                        g_err_tune_dbg_count--;
                    }else if (g_err_tune_dbg_error & MTK_MSDC_ERROR_CMD_CRC){
                        intsts = MSDC_INT_RSPCRCERR;
                        g_err_tune_dbg_count--;
                    }
                    MSDC_DBG_PRINT(MSDC_IRQ,"[%s]: got the error cmd:%d, arg=%d, dbg error=%d, cmd->error=%d, count=%d\n", __func__, g_err_tune_dbg_cmd, g_err_tune_dbg_arg, g_err_tune_dbg_error, cmd->error, g_err_tune_dbg_count);
                }
            }
        }
#endif
        if (intsts & MSDC_INT_CMDRDY) {
            u32 *rsp = NULL;
            rsp = &cmd->resp[0];

            switch (host->cmd_rsp) {
                case RESP_NONE:
                    break;
                case RESP_R2:
                    *rsp++ = sdr_read32(SDC_RESP3); *rsp++ = sdr_read32(SDC_RESP2);
                    *rsp++ = sdr_read32(SDC_RESP1); *rsp++ = sdr_read32(SDC_RESP0);
                    break;
                default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
                    *rsp = sdr_read32(SDC_RESP0);
                    break;
            }
        } else if (intsts & MSDC_INT_RSPCRCERR) {
            cmd->error = (unsigned int)-EIO;
            MSDC_DBG_PRINT(MSDC_IRQ,"XXX CMD<%d> MSDC_INT_RSPCRCERR Arg<0x%.8x>",cmd->opcode, cmd->arg);
            msdc_reset_hw(host->id);
        }else if (intsts & MSDC_INT_CMDTMO) {
            cmd->error = (unsigned int)-ETIMEDOUT;
            MSDC_DBG_PRINT(MSDC_IRQ, "XXX CMD<%d> MSDC_INT_CMDTMO Arg<0x%.8x>",cmd->opcode, cmd->arg);
            msdc_reset_hw(host->id);
        }
        if(intsts & (MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR | MSDC_INT_CMDTMO))
            complete(&host->cmd_done);
    }
    /* mmc irq interrupts */
    if (intsts & MSDC_INT_MMCIRQ) {
        //printk(KERN_INFO "msdc[%d] MMCIRQ: SDC_CSTS=0x%.8x\r\n", host->id, sdr_read32(SDC_CSTS));
    }
    latest_int_status[host->id] = 0;
    return IRQ_HANDLED;
}

/*--------------------------------------------------------------------------*/
/* platform_driver members                                                      */
/*--------------------------------------------------------------------------*/
/* called by msdc_drv_probe/remove */
static void msdc_enable_cd_irq(struct msdc_host *host, int enable)
{
#ifndef FPGA_PLATFORM
    struct msdc_hw *hw = host->hw;

#ifdef CONFIG_OF
    struct device_node *node;
    unsigned int gpiopin, debounce;
    unsigned int ints[2] = {0, 0};
#endif

    void __iomem *base = host->base;
    sdr_clr_bits(MSDC_PS, MSDC_PS_CDEN);
    sdr_clr_bits(MSDC_INTEN, MSDC_INTEN_CDSC);
    sdr_clr_bits(SDC_CFG, SDC_CFG_INSWKUP);

#ifdef CONFIG_OF
//    MSDC_ASSERT((eint_node != NULL) && (cd_irq != 0));//fix:user version can;t run to system by xmyyq

    if (enable) {
        /* get gpio pin & debounce time */
        of_property_read_u32_array(eint_node, "debounce", ints, ARRAY_SIZE(ints));
		/* to use workaround solution before fix dct tool shuai.zhang*/
        gpiopin  = PIN4_INS(MSDC1);
        debounce = ints[DEBOUNCE]/1000;
		
        /* set debounce */
        gpiopin = ints[GPIOPIN];
        debounce = ints[DEBOUNCE];
        mt_gpio_set_debounce(gpiopin, debounce);
		MSDC_TRC_PRINT(MSDC_INFO, "[MSDC%u] gpiopin:%d,debounce:%d\n",host->id, gpiopin, debounce);

        if (hw->cd_level) {
            host->sd_cd_polarity = 1;
            /* request irq for eint (either way) */
            if (request_irq(cd_irq, msdc1_eint_handler, IRQF_TRIGGER_HIGH, "MSDC1_INS-eint", NULL)) {
                MSDC_ERR_PRINT(MSDC_ERROR,"[MSDC%u] IRQ LINE NOT AVAILABLE!!\n", host->id);
            }
			MSDC_TRC_PRINT(MSDC_INFO, "[MSDC%u] high\n", host->id);
        } else {
            host->sd_cd_polarity = 0;
            /* request irq for eint (either way) */
            if (request_irq(cd_irq, msdc1_eint_handler, IRQF_TRIGGER_LOW, "MSDC1_INS-eint", NULL)) {
                MSDC_ERR_PRINT(MSDC_ERROR,"[MSDC%u] IRQ LINE NOT AVAILABLE!!\n", host->id);
            }
			MSDC_TRC_PRINT(MSDC_INFO, "[MSDC%u] low\n", host->id);
        }
    } else {
        disable_irq(cd_irq);
    }
#endif
#if 0//def CUST_EINT_MSDC1_INS_NUM
    if(enable){
        if((host->id == 1) && (MSDC_SD == hw->host_function) && (hw->flags & MSDC_CD_PIN_EN)){

            if(hw->cd_level)
                host->sd_cd_polarity = (~EINT_MSDC1_INS_POLARITY)&0x1;
            else
                host->sd_cd_polarity = EINT_MSDC1_INS_POLARITY;

            mt_eint_set_hw_debounce(CUST_EINT_MSDC1_INS_NUM, CUST_EINT_MSDC1_INS_DEBOUNCE_CN);

            if (0 == host->sd_cd_polarity){
                mt_eint_registration(CUST_EINT_MSDC1_INS_NUM, CUST_EINTF_TRIGGER_LOW, msdc1_eint_handler, 1);
            } else {
                mt_eint_registration(CUST_EINT_MSDC1_INS_NUM, CUST_EINTF_TRIGGER_HIGH, msdc1_eint_handler, 1);
            }

            MSDC_TRC_PRINT(MSDC_INIT,"SD card detection eint resigter, sd_cd_polarity=%d\n", host->sd_cd_polarity);
        }
        if((host->id == 2) && (MSDC_SD == hw->host_function) && (hw->flags & MSDC_CD_PIN_EN)){
            // sdio need handle eint11
        }
    } else{
        if(host->id == 1){
            mt_eint_mask(CUST_EINT_MSDC1_INS_NUM);
        }
        if(host->id == 2){
            // sdio need handle eint11
        }
    }
#endif
#else
    struct msdc_hw *hw = host->hw;
    void __iomem *base = host->base;

    /* for sdio, not set */
    if ((hw->flags & MSDC_CD_PIN_EN) == 0) {
        sdr_clr_bits(MSDC_PS, MSDC_PS_CDEN);
        sdr_clr_bits(MSDC_INTEN, MSDC_INTEN_CDSC);
        sdr_clr_bits(SDC_CFG, SDC_CFG_INSWKUP);
        return;
    }

    MSDC_TRC_PRINT(MSDC_INIT,"CD IRQ Eanable(%d)", enable);

    if (enable) {
        if (hw->enable_cd_eirq) { /* not set, never enter */
            hw->enable_cd_eirq();
        } else {
            /* card detection circuit relies on the core power so that the core power
             * shouldn't be turned off. Here adds a reference count to keep
             * the core power alive.
             */
            if (hw->config_gpio_pin) /* NULL */
                hw->config_gpio_pin(MSDC_CD_PIN, MSDC_GPIO_PULL_UP);

            sdr_set_field(MSDC_PS, MSDC_PS_CDDEBOUNCE, DEFAULT_DEBOUNCE);
            sdr_set_bits(MSDC_PS, MSDC_PS_CDEN);
            sdr_set_bits(MSDC_INTEN, MSDC_INTEN_CDSC);
            sdr_set_bits(SDC_CFG, SDC_CFG_INSWKUP);
        }
    } else {
        if (hw->disable_cd_eirq) {
            hw->disable_cd_eirq();
        } else {
            if (hw->config_gpio_pin) /* NULL */
                hw->config_gpio_pin(MSDC_CD_PIN, MSDC_GPIO_PULL_DOWN);

            sdr_clr_bits(SDC_CFG, SDC_CFG_INSWKUP);
            sdr_clr_bits(MSDC_PS, MSDC_PS_CDEN);
            sdr_clr_bits(MSDC_INTEN, MSDC_INTEN_CDSC);

            /* Here decreases a reference count to core power since card
             * detection circuit is shutdown.
             */
        }
    }
#endif
}

/* called by msdc_drv_probe */

static void msdc_init_hw(struct msdc_host *host)
{
    void __iomem *base = host->base;
    struct msdc_hw *hw = host->hw;
    u32 cur_rxdly0, cur_rxdly1;


    /* Power on */
    msdc_pin_reset(host, MSDC_PIN_PULL_UP);
#ifndef FPGA_PLATFORM
    msdc_enable_clock(host);
#endif
    host->core_clkon = 1;
    msdc_select_clksrc(host, hw->clk_src,hw->hclk_src);

    /* Configure to MMC/SD mode */
    sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);

    /* Reset */
    msdc_reset_hw(host->id);

    /* Disable card detection */
    sdr_clr_bits(MSDC_PS, MSDC_PS_CDEN);

    /* Disable and clear all interrupts */
    sdr_clr_bits(MSDC_INTEN, sdr_read32(MSDC_INTEN));
    sdr_write32(MSDC_INT, sdr_read32(MSDC_INT));

#if 1
    /* reset tuning parameter */
    sdr_write32(MSDC_PAD_TUNE,   0x00000000);
    sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_RXDLYSEL, 1);

    sdr_write32(MSDC_IOCON,    0x00000000);

    sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
    cur_rxdly0 = ((hw->dat0rddly & 0x1F) << 24) | ((hw->dat1rddly & 0x1F) << 16) |
                      ((hw->dat2rddly & 0x1F) << 8)  | ((hw->dat3rddly & 0x1F) << 0);
    cur_rxdly1 = ((hw->dat4rddly & 0x1F) << 24) | ((hw->dat5rddly & 0x1F) << 16) |
                     ((hw->dat6rddly & 0x1F) << 8)  | ((hw->dat7rddly & 0x1F) << 0);

    if (MSDC_EMMC == host->hw->host_function){
        //for write tuning, only tuning DAT0
        sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
    } else {
        sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);
    }

    sdr_write32(MSDC_DAT_RDDLY1, 0x00000000);

#if 0  //use default setting for MT8163
    if ((host->hw->host_function == MSDC_EMMC) || (host->hw->host_function == MSDC_SD)){
        sdr_write32(MSDC_PATCH_BIT1, 0xFFFE00C9);
    } else {
        sdr_write32(MSDC_PATCH_BIT1, 0xFFFE0009);
    }
#endif	
    //data delay settings should be set after enter high speed mode(now is at ios function >25MHz), detail information, please refer to P4 description
     host->saved_para.pad_tune = (((hw->cmdrrddly & 0x1F) << 22) | ((hw->cmdrddly & 0x1F) << 16) | ((hw->datwrddly & 0x1F) << 0) | BIT(15));//sdr_read32(MSDC_PAD_TUNE);
    host->saved_para.ddly0 = cur_rxdly0; //sdr_read32(MSDC_DAT_RDDLY0);
    host->saved_para.ddly1 = cur_rxdly1; //sdr_read32(MSDC_DAT_RDDLY1);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->saved_para.cmd_resp_ta_cntr);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, host->saved_para.write_busy_margin);
    sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA, host->saved_para.write_crc_margin);

    if(is_card_sdio(host)) {
        msdc_sdio_set_long_timing_delay_by_freq(host, 50*1000*1000);
    }
#ifdef CONFIG_EMMC_50_FEATURE
    if(host->id == 3) {
        sdr_get_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, host->saved_para.ds_dly1);
        sdr_get_field(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, host->saved_para.ds_dly3);
    }
#endif

    if (!(is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))) {  /* internal clock: latch read data, not apply to sdio */
        host->hw->cmd_edge  = 0; // tuning from 0
        host->hw->rdata_edge = 0;
        host->hw->wdata_edge = 0;
    }else if (hw->flags & MSDC_INTERNAL_CLK) {
        //sdr_set_bits(MSDC_PATCH_BIT0, MSDC_PATCH_BIT_CKGEN_CK);
    }
    //sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

#endif

    /* for safety, should clear SDC_CFG.SDIO_INT_DET_EN & set SDC_CFG.SDIO in
       pre-loader,uboot,kernel drivers. and SDC_CFG.SDIO_INT_DET_EN will be only
       set when kernel driver wants to use SDIO bus interrupt */
    /* Configure to enable SDIO mode. it's must otherwise sdio cmd5 failed */
    sdr_set_bits(SDC_CFG, SDC_CFG_SDIO);

    /* disable detect SDIO device interupt function */
    sdr_clr_bits(SDC_CFG, SDC_CFG_SDIOIDE);

#ifndef FPGA_PLATFORM
    msdc_set_smt(host, 1);
    msdc_set_driving(host, hw, 0);
    msdc_set_pad_init(host);
#endif

    MSDC_TRC_PRINT(MSDC_INIT,"msdc drving<clk %d,cmd %d,dat %d>",hw->clk_drv,hw->cmd_drv,hw->dat_drv);

    /* write crc timeout detection */
    sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_DETWR_CRCTMO, 1);

    /* Configure to default data timeout */
    sdr_set_field(SDC_CFG, SDC_CFG_DTOC, DEFAULT_DTOC);

    msdc_set_buswidth(host, MMC_BUS_WIDTH_1);
#ifdef MSDC_USE_PATCH_BIT2_TURNING_WITH_ASYNC
		sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,1);
		sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP,0);
#else
		sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,0);
		sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP,1);
#endif

    MSDC_TRC_PRINT(MSDC_INIT, "init hardware done!");
}

/* called by msdc_drv_remove */
static void msdc_deinit_hw(struct msdc_host *host)
{
    void __iomem *base = host->base;

    /* Disable and clear all interrupts */
    sdr_clr_bits(MSDC_INTEN, sdr_read32(MSDC_INTEN));
    sdr_write32(MSDC_INT, sdr_read32(MSDC_INT));

    /* Disable card detection */
    msdc_enable_cd_irq(host, 0);
    msdc_set_power_mode(host, MMC_POWER_OFF);   /* make sure power down */
}

/* init gpd and bd list in msdc_drv_probe */
static void msdc_init_gpd_bd(struct msdc_host *host, struct msdc_dma *dma)
{
    gpd_t *gpd = dma->gpd;
    bd_t  *bd  = dma->bd;
    bd_t  *ptr, *prev;

    /* we just support one gpd */
    int bdlen = MAX_BD_PER_GPD;

    /* init the 2 gpd */
    memset(gpd, 0, sizeof(gpd_t) * 2);
    gpd->next = ((u32)dma->gpd_addr + sizeof(gpd_t));

    //gpd->intr = 0;
    gpd->bdp  = 1;   /* hwo, cs, bd pointer */
    //gpd->ptr  = (void*)virt_to_phys(bd);
    gpd->ptr = dma->bd_addr; /* physical address */

    memset(bd, 0, sizeof(bd_t) * bdlen);
    ptr = bd + bdlen - 1;
    while (ptr != bd) {
        prev = ptr - 1;
        prev->next = ((u32)dma->bd_addr + sizeof(bd_t) *(ptr - bd));
        ptr = prev;
    }
}

static void msdc_init_dma_latest_address(void)
{
    struct dma_addr *ptr, *prev;
    int bdlen = MAX_BD_PER_GPD;

    memset(msdc_latest_dma_address, 0, sizeof(struct dma_addr) * bdlen);
    ptr = msdc_latest_dma_address + bdlen - 1;
    while(ptr != msdc_latest_dma_address){
        prev = ptr - 1;
        prev->next = (msdc_latest_dma_address + sizeof(struct dma_addr) * (ptr - msdc_latest_dma_address));
        ptr = prev;
    }

}

struct gendisk * mmc_get_disk(struct mmc_card *card)
{
    struct mmc_blk_data *md;
    //struct gendisk *disk;

    BUG_ON(!card);
    md = mmc_get_drvdata(card);
    BUG_ON(!md);
    BUG_ON(!md->disk);

    return md->disk;
}

#if defined(CONFIG_MTK_EMMC_SUPPORT) && defined(CONFIG_PROC_FS)
static struct proc_dir_entry *proc_emmc;

#ifdef CONFIG_MTK_GPT_SCHEME_SUPPORT
static inline int emmc_proc_info(struct seq_file *m, struct hd_struct *this)
{
    char *no_partition_name = "n/a";

    return seq_printf(m, "emmc_p%d: %8.8x %8.8x \"%s\"\n", this->partno,
               (unsigned int)this->start_sect,
               (unsigned int)this->nr_sects,
                ((this->info) ? (char *)(this->info->volname) : no_partition_name));
}
#else
static inline int emmc_proc_info(struct seq_file *m, struct hd_struct *this)
{
    int i = 0;
    char *no_partition_name = "n/a";

    for (i = 0; i < PART_NUM; i++) {
        if (PartInfo[i].partition_idx != 0 && PartInfo[i].partition_idx == this->partno) {
            break;
        }
    }

    return seq_printf(m, "emmc_p%d: %8.8x %8.8x \"%s\"\n", this->partno,
               (unsigned int)this->start_sect,
               (unsigned int)this->nr_sects, (i >= PART_NUM ? no_partition_name : PartInfo[i].name));
}
#endif

static int proc_emmc_show(struct seq_file *m, void *v)
{
    struct disk_part_iter piter;
    struct hd_struct *part;
    struct msdc_host *host;
    struct gendisk *disk;

    /* emmc always in slot0 */
    host = msdc_get_host(MSDC_EMMC,MSDC_BOOT_EN,0);
    BUG_ON(!host);
    BUG_ON(!host->mmc);
    BUG_ON(!host->mmc->card);
    disk = mmc_get_disk(host->mmc->card);

    seq_printf(m, "partno:    start_sect   nr_sects  partition_name\n");
    disk_part_iter_init(&piter, disk, 0);
    while ((part = disk_part_iter_next(&piter))){
        emmc_proc_info(m, part);
    }
    disk_part_iter_exit(&piter);

    return 0;
}

static int proc_emmc_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_emmc_show, NULL);
}

static const struct file_operations proc_emmc_fops = {
    .open = proc_emmc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

#endif /* CONFIG_MTK_EMMC_SUPPORT && CONFIG_PROC_FS */

#ifdef MTK_MSDC_FLUSH_BY_CLK_GATE
extern int mmc_flush_cache(struct mmc_card *card);
static void msdc_tasklet_flush_cache(unsigned long arg)
{
    struct msdc_host *host = (struct msdc_host *)arg;

    if(host->mmc->card)
    {
        mmc_claim_host(host->mmc);
        mmc_flush_cache(host->mmc->card);
        mmc_release_host(host->mmc);
    }

    return;
}
#endif

/* This is called by run_timer_softirq */
static void msdc_timer_pm(unsigned long data)
{
    struct msdc_host *host = (struct msdc_host *)data;
    unsigned long flags;

    spin_lock_irqsave(&host->clk_gate_lock, flags);
    if (host->clk_gate_count == 0) {
        msdc_clksrc_onoff(host, 0);
        MSDC_TRC_PRINT(MSDC_CONFIG, "time out, dsiable clock, clk_gate_count=%d", host->clk_gate_count);
    }
#ifdef MTK_MSDC_FLUSH_BY_CLK_GATE
    if((host->hw->host_function == MSDC_EMMC) &&
       host->mmc->card && (host->mmc->card->ext_csd.cache_ctrl & 0x1))
    {
        tasklet_hi_schedule(&host->flush_cache_tasklet);
    }
#endif

    spin_unlock_irqrestore(&host->clk_gate_lock, flags);
}

static u32 first_probe = 0;
#ifndef FPGA_PLATFORM
static void msdc_set_host_power_control(struct msdc_host *host)
{

   switch(host->id){
      case 0:
          if(MSDC_EMMC == host->hw->host_function){
              host->power_control = msdc_emmc_power;
          }else{
                MSDC_ERR_PRINT(MSDC_ERROR, "Host function defination error. Please check host_function<0x%lx>",host->hw->host_function);
          }
          break;
      case 1:
          if(MSDC_SD == host->hw->host_function){
                host->power_control = msdc_sd_power;
                host->power_switch = msdc_sd_power_switch;
          }else{
                MSDC_ERR_PRINT(MSDC_ERROR,"Host function defination error. Please check host_function<0x%lx>",host->hw->host_function);
          }
          break;
      case 2:
          if(MSDC_SDIO == host->hw->host_function){
                host->power_control = msdc_sdio_power;
          }else{
                MSDC_ERR_PRINT(MSDC_ERROR,"Host function defination error. Please check host_function<0x%lx>",host->hw->host_function);
          }
          break;
      case 3:
          if(MSDC_EMMC == host->hw->host_function){
                host->power_control = msdc_emmc_power;
          }else{
                MSDC_ERR_PRINT(MSDC_ERROR,"Host function defination error. Please check host_function<0x%lx>",host->hw->host_function);
          }
          break;
      default:
          break;
    }
}
#endif /* end of FPGA_PLATFORM */

void SRC_trigger_signal(int i_on)
{
    if ((ghost!= NULL) && (ghost->hw->flags & MSDC_SDIO_IRQ))
    {
        MSDC_DBG_PRINT(MSDC_DEBUG, "msdc2 SRC_trigger_signal %d \n",i_on);
        src_clk_control = i_on;
        if (src_clk_control)
        {
            msdc_clksrc_onoff(ghost, 1);
            //mb();
            if(ghost->mmc->sdio_irq_thread &&  (atomic_read(&ghost->mmc->sdio_irq_thread_abort) == 0))      //if (ghost->mmc->sdio_irq_thread)
            {
                mmc_signal_sdio_irq(ghost->mmc);
                if (u_msdc_irq_counter<3)
                    MSDC_DBG_PRINT(MSDC_DEBUG, "msdc2 SRC_trigger_signal mmc_signal_sdio_irq \n");
            }
            //printk("msdc2 SRC_trigger_signal ghost->id=%d\n",ghost->id);
        }
    }

}
EXPORT_SYMBOL(SRC_trigger_signal);

#ifdef CONFIG_SDIOAUTOK_SUPPORT
/*************************************************************************
* FUNCTION
*  sdio_stop_transfer
*
* DESCRIPTION
*  This function for DVFS, stop SDIO transfer and wait for transfer complete
*
* PARAMETERS
*
* RETURN VALUES
*    0: transfer complete
*    <0: error code
*************************************************************************/

//static DEFINE_SPINLOCK(mt_sdio_stop_lock);
static int count = 0;

int sdio_stop_transfer(void)
{
    int hostidx = 0;
    int host_claimed = 0;
//    unsigned long flags;
    //printk("[%s] Enter\n", __func__);

    for(hostidx = 0; hostidx < HOST_MAX_NUM; hostidx++)
    {
        if(mtk_msdc_host[hostidx]==NULL)
        {
            MSDC_DBG_PRINT(MSDC_DEBUG, "[%s] mtk_msdc_host[%d] = NULL\n", __func__, hostidx);
            continue;
        }
        if(is_card_sdio(mtk_msdc_host[hostidx]))
        {
            struct msdc_host *host = mtk_msdc_host[hostidx];
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
            while(1)
            {
                if (atomic_read(&host->ot_done)) {
                    MSDC_DBG_PRINT(MSDC_DEBUG, "[%s] msdc%d prev. online tuning done, host->ot_work.chg_volt = %d\n", __func__, host->id, host->ot_work.chg_volt);
                    break;
                }

                msleep(1);
            }
#endif  // MTK_SDIO30_ONLINE_TUNING_SUPPORT
            if(atomic_read(&host->sdio_stopping))
            {
                MSDC_DBG_PRINT(MSDC_DEBUG, "[%s] msdc%d sdio stopping\n", __func__, host->id);
                wait_for_completion_interruptible(&host->ot_work.ot_complete);
                //return -1;
            }
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
			else if(atomic_read(&host->ot_done) == 0)
			{
				MSDC_DBG_PRINT(MSDC_DEBUG, "[%s] msdc%d online tuning is working\n", __func__, host->id);
				wait_for_completion_interruptible(&host->ot_work.ot_complete);
				//return -1;
			}
#endif
            //else
            {
                atomic_set(&host->sdio_stopping, 1);
                autok_claim_host(host);
                host_claimed++;
            }
        }
    }

    //printk("[%s] Exit\n", __func__);
    if(host_claimed)
    {
        count++;
        return 0;
    }
    else
    {
        return -1;
    }
}
EXPORT_SYMBOL(sdio_stop_transfer);

/*************************************************************************
* FUNCTION
*  sdio_start_ot_transfer
*
* DESCRIPTION
*  This function for DVFS, start online tuning and start SDIO transfer
*
* PARAMETERS
*    voltage: The voltage that DVFS changes to
*
* RETURN VALUES
*    0: success
*    <0: error code
*************************************************************************/
extern unsigned int mt65x2_vcore_tbl[];
extern unsigned int msdc_autok_get_vcore(unsigned int vcore_uv, unsigned int *pfIdentical);
int sdio_start_ot_transfer(void)
{
    int hostidx = 0;

	count--;
	if(count != 0)
		MSDC_DBG_PRINT(MSDC_DEBUG, "[%s] sdio stop and start are not pairwise, count = %d\n", __func__, count);

    for(hostidx = 0; hostidx < HOST_MAX_NUM; hostidx++)
    {
        if(mtk_msdc_host[hostidx]==NULL)
        {
            MSDC_DBG_PRINT(MSDC_DEBUG, "[%s] mtk_msdc_host[%d] = NULL\n", __func__, hostidx);
            continue;
        }
        if(is_card_sdio(mtk_msdc_host[hostidx]))
        {
            struct msdc_host *host = mtk_msdc_host[hostidx];
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
            u32 vcore_uv_off, vcore_sel, vcore_uv, fIdent = 0;
            static u32 prev_vcore_sel = 0;

            MSDC_DBG_PRINT(MSDC_DEBUG, "[%s] msdc%d Check vcore\n", __func__, host->id);

            vcore_uv_off = autok_get_current_vcore_offset();
            vcore_uv = mt65x2_vcore_tbl[vcore_uv_off];
            vcore_sel = msdc_autok_get_vcore(vcore_uv, &fIdent);
            if(vcore_sel == prev_vcore_sel) {
                MSDC_DBG_PRINT(MSDC_DEBUG, "[%s] voltage doesn't change\n", __func__);
                atomic_set(&host->sdio_stopping, 0);
                complete(&host->ot_work.ot_complete);
                autok_release_host(host);
                continue;
            } else {
                prev_vcore_sel = vcore_sel;
    			host->ot_work.chg_volt = 1;
                init_completion(&host->ot_work.ot_complete);
    			start_online_tuning((unsigned long)host);
    		}
#else
            atomic_set(&host->sdio_stopping, 0);
            complete(&host->ot_work.ot_complete);
            autok_release_host(host);
#endif  // MTK_SDIO30_ONLINE_TUNING_SUPPORT
        }
    }

    return 0;
}
EXPORT_SYMBOL(sdio_start_ot_transfer);

#endif  // CONFIG_SDIOAUTOK_SUPPORT

#ifdef CONFIG_MTK_HIBERNATION
extern unsigned int mt_eint_get_polarity(unsigned int eint_num);
int msdc_drv_pm_restore_noirq(struct device *device)
{
    struct platform_device *pdev = to_platform_device(device);
    struct mmc_host *mmc = NULL;
    struct msdc_host *host = NULL;
    u32 l_polarity = 0;
    BUG_ON(pdev == NULL);
    mmc = platform_get_drvdata(pdev);
    host = mmc_priv(mmc);
    if(host->hw->host_function == MSDC_SD){
        if((host->id == 1) && (host->hw->flags & MSDC_CD_PIN_EN)){
            l_polarity = mt_eint_get_polarity(cd_irq);
            if (l_polarity == MT_POLARITY_LOW){
                host->sd_cd_polarity = 0;
            } else {
                host->sd_cd_polarity = 1;
            }

            if(!(host->hw->cd_level ^ host->sd_cd_polarity) && host->mmc->card){
                mmc_card_set_removed(host->mmc->card);
                host->card_inserted = 0;
            }
        } else if((host->id == 2) && (host->hw->flags & MSDC_CD_PIN_EN)){
            // sdio need handle here
        }
        host->block_bad_card = 0;
    }
    return 0;
}
#endif

#ifdef CONFIG_OF
extern struct msdc_hw msdc0_hw;
#if defined(CFG_DEV_MSDC1)
extern struct msdc_hw msdc1_hw;
#endif
#if defined(CFG_DEV_MSDC2)
extern struct msdc_hw msdc2_hw;
#endif
#if defined(CFG_DEV_MSDC3)
extern struct msdc_hw msdc3_hw;
#endif

#define EFUSE_EMMC_MSDC3_DIS            (1ul <<  7)
#define EFUSE_EMMC_MSDC0_DIS            (1ul <<  6)

static int msdc_drv_probe(struct platform_device *pdev)
{
    struct mmc_host *mmc=NULL;
    struct msdc_host *host=NULL;
    struct msdc_hw *hw=NULL;
    void __iomem *base=NULL;
    void __iomem *gpiobase=NULL; 
    void __iomem *topckgenbase=NULL;
	unsigned int irq;
    int ret;
    struct irq_data l_irq_data;
#ifdef MTK_MSDC_USE_CACHE
    BOOTMODE boot_mode;
#endif
    void __iomem *msdc_efuse4emmc_io=NULL;
    u32  u4efuse=0;
	u32  id;

	
    MSDC_TRC_PRINT(MSDC_INIT,"of msdc DT probe %s! \n", pdev->dev.of_node->name);
	
#if defined(CFG_DEV_MSDC0)
		if (strcmp(pdev->dev.of_node->name, "MSDC0") == 0) {
			id = 0;
			
			msdc_efuse4emmc_io = of_iomap(pdev->dev.of_node, 2);
			MSDC_TRC_PRINT(MSDC_INIT,"MSDC(0,3) efuse control address 0x%p\n", msdc_efuse4emmc_io);
			
			sdr_get_field(msdc_efuse4emmc_io,EFUSE_EMMC_MSDC0_DIS,u4efuse);
			MSDC_TRC_PRINT(MSDC_INIT, "MSDC0 efuse(%p) current %s \n",msdc_efuse4emmc_io, u4efuse ? "Disable" : "Enable");
			if (u4efuse == 0 ) {
				hw = &msdc0_hw;
				pdev->id = 0;
				printk("platform_data hw:0x%p, is msdc0_hw \n", hw);
			}
		}
#endif
#if defined(CFG_DEV_MSDC1)
		if (strcmp(pdev->dev.of_node->name, "MSDC1") == 0) {
			id = 1;
			hw = &msdc1_hw;
			pdev->id = 1;
			printk("platform_data hw:0x%p, is msdc1_hw \n", hw);
		}
#endif
#if defined(CFG_DEV_MSDC2)
		if (strcmp(pdev->dev.of_node->name, "MSDC2") == 0) {
			id = 2;
			hw = &msdc2_hw;
			pdev->id = 2;
			printk("platform_data hw:0x%p, is msdc2_hw \n", hw);
		}
#endif
#if defined(CFG_DEV_MSDC3)
		if (strcmp(pdev->dev.of_node->name, "MSDC3") == 0) {
			id = 3;

			msdc_efuse4emmc_io = of_iomap(pdev->dev.of_node, 2);
			MSDC_TRC_PRINT(MSDC_INIT,"MSDC(0,3) efuse control address 0x%p\n", msdc_efuse4emmc_io);
			
			sdr_get_field(msdc_efuse4emmc_io,EFUSE_EMMC_MSDC3_DIS,u4efuse);
			printk("MSDC3 efuse(%p) current %s \n",msdc_efuse4emmc_io, u4efuse ? "Disable" : "Enable");
			if (u4efuse == 0 ) {
				hw = &msdc3_hw;
				pdev->id = 3;
				printk("platform_data hw:0x%p, is msdc3_hw \n", hw);
			}
		}
#endif	
		if (hw == NULL){		
			return -ENODEV;
		}


    /* iomap register */
    base = of_iomap(pdev->dev.of_node, 0);
    if (!base) {
        MSDC_ERR_PRINT(MSDC_ERROR,"can't of_iomap for msdc!!\n");
        return -ENOMEM;
    }
    else {
        MSDC_TRC_PRINT(MSDC_INIT,"of_iomap for msdc @ 0x%p\n", base);
    }

    /* get irq #  */
    irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
    MSDC_DBG_PRINT(MSDC_INIT,"msdc get irq # %d\n", irq);
    MSDC_BUGON(irq < 0);
    l_irq_data.irq = irq;

struct device_node *msdc_apmixed_node = NULL;
struct device_node *msdc_topckgen_node = NULL;
struct device_node *msdc_infracfg_node = NULL;
void __iomem *msdc_apmixed_base;
void __iomem *msdc_topckgen_base;
void __iomem *msdc_infracfg_base;

#ifdef MTK_MSDC_BRINGUP_DEBUG
    if (msdc_apmixed_node == NULL) {
 	    msdc_apmixed_node = of_find_compatible_node(NULL, NULL, "mediatek,APMIXED");
        msdc_apmixed_base = of_iomap(msdc_apmixed_node, 0);
        MSDC_TRC_PRINT(MSDC_INIT,"of_iomap for APMIXED base @ 0x%p\n", msdc_apmixed_base);
    }

    if (msdc_topckgen_node == NULL) {
 	    msdc_topckgen_node = of_find_compatible_node(NULL, NULL, "mediatek,TOPCKGEN");
        msdc_topckgen_base = of_iomap(msdc_topckgen_node, 0);
        MSDC_TRC_PRINT(MSDC_INIT,"of_iomap for TOPCKGEN base @ 0x%p\n", msdc_topckgen_base);
    }

    if (msdc_infracfg_node == NULL) {
	    msdc_infracfg_node = of_find_compatible_node(NULL, NULL, "mediatek,INFRACFG_AO");
	    msdc_infracfg_base = of_iomap(msdc_infracfg_node, 0);
	    MSDC_TRC_PRINT(MSDC_INIT,"of_iomap for INFRACFG_AO base @ 0x%p\n", msdc_infracfg_base);
    }	
#endif

#ifdef FPGA_PLATFORM
    if (fpga_pwr_gpio == NULL) {
        fpga_pwr_gpio = of_iomap(pdev->dev.of_node, 1);
        fpga_pwr_gpio_eo = fpga_pwr_gpio + 0x4;
        MSDC_TRC_PRINT(MSDC_INIT,"FPAG PWR_GPIO, PWR_GPIO_EO address 0x%p, 0x%p\n", fpga_pwr_gpio, fpga_pwr_gpio_eo);
    }
#endif

	if (gpiobase == NULL){
			gpiobase = of_iomap(pdev->dev.of_node, ((id == 0)||(id == 3)) ? 3 : 2 );
			MSDC_TRC_PRINT(MSDC_INIT,"MSDC(0,3) GPIO control address 0x%p\n", gpiobase);
	}
	
	if (topckgenbase == NULL){
			topckgenbase = of_iomap(pdev->dev.of_node, ((id == 0)||(id == 3)) ? 4 : 3);
			MSDC_TRC_PRINT(MSDC_INIT,"MSDC(0,3) CKGEN control address 0x%p\n", topckgenbase);
	}

    if (first_probe == 0) {
        first_probe ++;
    }

    if ((hw->flags & MSDC_CD_PIN_EN) && (pdev->id == 1) && (hw->host_function == MSDC_SD) && (eint_node == NULL)) {
        eint_node = of_find_compatible_node(NULL, NULL, "mediatek, MSDC1_INS-eint");
        if (eint_node) {
            MSDC_TRC_PRINT(MSDC_INIT,"[MSDC%u] find MSDC1_INS-eint node!!\n", pdev->id);
            hw->flags |= (MSDC_CD_PIN_EN | MSDC_REMOVABLE);

            /* get irq #  */
            if (!cd_irq) {
                cd_irq = irq_of_parse_and_map(eint_node, 0);
				cd_irq = mt_gpio_to_irq(PIN4_INS(MSDC1)); /* to use workaround solution before fix irq_of_parse_and_map maoguang.meng*/
            }
            if (!cd_irq) {
                MSDC_TRC_PRINT(MSDC_INIT,"[MSDC%u]can't irq_of_parse_and_map for card detect eint!!\n", pdev->id);
            } else {
                MSDC_TRC_PRINT(MSDC_INIT,"[MSDC%u] EINT get irq #%u\n", pdev->id, cd_irq);
            }
        } else {
            MSDC_ERR_PRINT(MSDC_ERROR,"[MSDC%u] can't find MSDC1_INS-eint compatible node\n", pdev->id);
        }
    }

    /* Allocate MMC host for this device */
    mmc = mmc_alloc_host(sizeof(struct msdc_host), &pdev->dev);
    if (!mmc) return -ENOMEM;

    /* Set host parameters to mmc */
    mmc->ops        = &mt_msdc_ops;
    mmc->f_min      = HOST_MIN_MCLK;
    mmc->f_max      = HOST_MAX_MCLK;
    mmc->ocr_avail  = MSDC_OCR_AVAIL;

    /* For sd card: MSDC_SYS_SUSPEND | MSDC_WP_PIN_EN | MSDC_CD_PIN_EN | MSDC_REMOVABLE | MSDC_HIGHSPEED,
       For sdio   : MSDC_EXT_SDIO_IRQ | MSDC_HIGHSPEED */
    if (hw->flags & MSDC_HIGHSPEED) {
        mmc->caps   = MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED;
    }
    if (hw->data_pins == 4) {
        mmc->caps  |= MMC_CAP_4_BIT_DATA;
    } else if (hw->data_pins == 8) {
        mmc->caps  |= MMC_CAP_8_BIT_DATA | MMC_CAP_4_BIT_DATA;
    }
    if ((hw->flags & MSDC_SDIO_IRQ) || (hw->flags & MSDC_EXT_SDIO_IRQ))
        mmc->caps |= MMC_CAP_SDIO_IRQ;  /* yes for sdio */
    if(hw->flags & MSDC_UHS1){
        mmc->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104;    // | MMC_CAP_SET_XPC_180;
        mmc->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
    }
#ifdef CONFIG_EMMC_50_FEATURE
    MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: hw->flags=0x%lx, MSDC_HS400=0x%x\n", __func__, hw->flags, MSDC_HS400);
    if(hw->flags & MSDC_HS400){
        mmc->caps2 |= MMC_CAP2_HS400_1_8V_DDR;
    }
#endif
    if(hw->flags & MSDC_DDR) {
        mmc->caps |= MMC_CAP_UHS_DDR50|MMC_CAP_1_8V_DDR;
	}
    if(!(hw->flags & MSDC_REMOVABLE))
        mmc->caps |= MMC_CAP_NONREMOVABLE;
    //else
        //mmc->caps2 |= MMC_CAP2_DETECT_ON_ERR;

#ifdef MTK_MSDC_USE_CMD23
    if (hw->host_function == MSDC_EMMC){
        mmc->caps |= MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY | MMC_CAP_CMD23;
    } else {
        mmc->caps |= MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY;
    }
#else
    mmc->caps |= MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY;
#endif

#ifdef MTK_MSDC_USE_CACHE
    /* only enable the emmc cache feature for normal boot up, alarm boot up, and sw reboot*/
    boot_mode = get_boot_mode();

    if((hw->host_function == MSDC_EMMC) && (hw->flags & MSDC_CACHE) &&
        ((boot_mode == NORMAL_BOOT) || (boot_mode == ALARM_BOOT) || (boot_mode == SW_REBOOT)))
        mmc->caps2 |= MMC_CAP2_CACHE_CTRL;

    MSDC_DBG_PRINT(MSDC_DEBUG,"[%s]: boot_mode = %d, caps2 = %x \n", __func__, boot_mode, mmc->caps2);
#endif

    /* MMC core transfer sizes tunable parameters */
    //mmc->max_hw_segs   = MAX_HW_SGMTS;
    mmc->max_segs   = MAX_HW_SGMTS;
    //mmc->max_phys_segs = MAX_PHY_SGMTS;
    if(hw->host_function == MSDC_SDIO)
        mmc->max_seg_size  = MAX_SGMT_SZ_SDIO;
    else
        mmc->max_seg_size  = MAX_SGMT_SZ;
    mmc->max_blk_size  = HOST_MAX_BLKSZ;
    mmc->max_req_size  = MAX_REQ_SZ;
    mmc->max_blk_count = mmc->max_req_size;

    host = mmc_priv(mmc);
    host->hw             = hw;
    host->mmc            = mmc;  /* msdc_check_init_done() need */
    host->id             = pdev->id;
    host->error          = 0;
    host->irq            = irq;
    host->base           = base;
    host->gpiobase       = gpiobase;
    host->topckgenbase   = topckgenbase;
    host->mclk           = 0;                   /* mclk: the request clock of mmc sub-system */
    host->hclk           = (pdev->id == 3) ? hclks_msdc50[hw->clk_src] : hclks_msdc30[hw->clk_src];  /* hclk: clock of clock source to msdc controller */
    host->sclk           = 0;                   /* sclk: the really clock after divition */
    host->pm_state       = PMSG_RESUME;
    host->suspend        = 0;
    host->core_clkon     = 0;
    host->card_clkon     = 0;
    host->clk_gate_count = 0;
    host->core_power     = 0;
    host->power_mode     = MMC_POWER_OFF;
    host->power_control  = NULL;
    host->power_switch   = NULL;
#ifndef FPGA_PLATFORM
    msdc_set_host_power_control(host);
    if((host->hw->host_function == MSDC_SD) && (host->hw->flags & MSDC_CD_PIN_EN)){
        msdc_sd_power(host,1); //work around : hot-plug project SD card LDO alway on if no SD card insert
        msdc_sd_power(host,0);
    }
#endif
    if(host->hw->host_function == MSDC_EMMC && !(host->hw->flags & MSDC_UHS1))
       host->mmc->f_max = 50000000;
    //host->card_inserted = hw->flags & MSDC_REMOVABLE ? 0 : 1;
    host->timeout_ns = 0;
    host->timeout_clks = DEFAULT_DTOC * 1048576;

#ifndef MTK_MSDC_USE_CMD23
    if(host->hw->host_function != MSDC_SDIO)
        host->autocmd |= MSDC_AUTOCMD12;
    else
        host->autocmd &= ~MSDC_AUTOCMD12;
#else
    if (host->hw->host_function == MSDC_EMMC){
        host->autocmd &= ~MSDC_AUTOCMD12;

#if (1 == MSDC_USE_AUTO_CMD23)
        host->autocmd |= MSDC_AUTOCMD23;
#endif

    } else if (host->hw->host_function == MSDC_SD){
        host->autocmd |= MSDC_AUTOCMD12;
    } else {
        host->autocmd &= ~MSDC_AUTOCMD12;
    }
#endif /* end of MTK_MSDC_USE_CMD23 */

    host->mrq = NULL;
    //init_MUTEX(&host->sem); /* we don't need to support multiple threads access */

    host->dma.used_gpd = 0;
    host->dma.used_bd = 0;

    /* using dma_alloc_coherent*/  /* todo: using 1, for all 4 slots */
    host->dma.gpd = dma_alloc_coherent(&pdev->dev, MAX_GPD_NUM * sizeof(gpd_t), &host->dma.gpd_addr, GFP_KERNEL);
	DMAGPDPTR_DBGCHK(host->dma.gpd,16);

	host->dma.bd =  dma_alloc_coherent(&pdev->dev, MAX_BD_NUM  * sizeof(bd_t),  &host->dma.bd_addr,  GFP_KERNEL);
	DMABDPTR_DBGCHK(host->dma.bd,16);	
	MSDC_BUGON((!host->dma.gpd) || (!host->dma.bd));
	
    msdc_init_gpd_bd(host, &host->dma);
    msdc_clock_src[host->id] = hw->clk_src;
    msdc_host_mode[host->id] = mmc->caps;
    msdc_host_mode2[host->id] = mmc->caps2;
    /*for emmc*/
    mtk_msdc_host[pdev->id] = host;
    host->write_timeout_uhs104 = 0;
    host->write_timeout_emmc = 0;
    host->read_timeout_uhs104 = 0;
    host->read_timeout_emmc = 0;
    host->sw_timeout = 0;
    host->tune = 0;
    host->state = 0;
    host->sd_cd_insert_work = 0;
    host->block_bad_card = 0;
    host->sd_30_busy = 0;
    msdc_reset_tmo_tune_counter(host, all_counter);
    msdc_reset_pwr_cycle_counter(host);

    if (is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))
    {
        host->saved_para.suspend_flag = 0;
        host->saved_para.msdc_cfg = 0;
        host->saved_para.mode = 0;
        host->saved_para.div = 0;
        host->saved_para.sdc_cfg = 0;
        host->saved_para.iocon = 0;
        host->saved_para.state = 0;
        host->saved_para.hz = 0;
        host->saved_para.cmd_resp_ta_cntr = 0; //for SDIO 3.0
        host->saved_para.wrdat_crc_ta_cntr = 0; //for SDIO 3.0
        host->saved_para.int_dat_latch_ck_sel = 0; //for SDIO 3.0
        host->saved_para.ckgen_msdc_dly_sel = 0; //for SDIO 3.0
        host->saved_para.inten_sdio_irq = 0; //default disable
        wake_lock_init(&host->trans_lock, WAKE_LOCK_SUSPEND, "MSDC Transfer Lock");
    }
    tasklet_init(&host->card_tasklet, msdc_tasklet_card, (ulong)host);
#ifdef MTK_MSDC_FLUSH_BY_CLK_GATE
    if((host->hw->host_function == MSDC_EMMC) && (host->mmc->caps2 & MMC_CAP2_CACHE_CTRL))
        tasklet_init(&host->flush_cache_tasklet, msdc_tasklet_flush_cache, (ulong)host);
#endif
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
    atomic_set(&host->ot_done, 1);
    atomic_set(&host->sdio_stopping, 0);
    host->ot_work.host = host;
    host->ot_work.chg_volt = 0;
    atomic_set(&host->ot_work.ot_disable, 0);
    atomic_set(&host->ot_work.autok_done, 0);
    init_completion(&host->ot_work.ot_complete);
  #ifdef MTK_SDIO30_DETECT_THERMAL
    host->pre_temper = -127000;
    host->ot_period_check_start = 0;
    init_timer(&host->ot_timer);
    host->ot_timer.function	= start_online_tuning;
    host->ot_timer.data		= (unsigned long)host;
  #endif // MTK_SDIO30_DETECT_THERMAL
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT
    //INIT_DELAYED_WORK(&host->remove_card, msdc_remove_card);
    spin_lock_init(&host->lock);
    spin_lock_init(&host->clk_gate_lock);
    spin_lock_init(&host->remove_bad_card);
    /* init dynamtic timer */
    init_timer(&host->timer);
    //host->timer.expires = jiffies + HZ;
    host->timer.function = msdc_timer_pm;
    host->timer.data = (unsigned long)host;

    msdc_init_hw(host);

    //mt65xx_irq_set_sens(irq, MT65xx_EDGE_SENSITIVE);
    //mt65xx_irq_set_polarity(irq, MT65xx_POLARITY_LOW);
    ret = request_irq((unsigned int)irq, msdc_irq, IRQF_TRIGGER_NONE, DRV_NAME, host);
    if (ret) goto release;
    //mt65xx_irq_unmask(&l_irq_data);
    //enable_irq(irq);

    MVG_EMMC_SETUP(host);

    if (hw->flags & MSDC_CD_PIN_EN) { /* not set for sdio */
        if (hw->request_cd_eirq) {
            hw->request_cd_eirq(msdc_eirq_cd, (void*)host); /* msdc_eirq_cd will not be used! */
        }
    }

    if (hw->request_sdio_eirq) /* set to combo_sdio_request_eirq() for WIFI */
        hw->request_sdio_eirq(msdc_eirq_sdio, (void*)host); /* msdc_eirq_sdio() will be called when EIRQ */

#ifdef CONFIG_PM
    if (hw->register_pm) {/* yes for sdio */
        hw->register_pm(msdc_pm, (void*)host);  /* combo_sdio_register_pm() */
        if(hw->flags & MSDC_SYS_SUSPEND) { /* will not set for WIFI */
            MSDC_ERR_PRINT(MSDC_ERROR, "MSDC_SYS_SUSPEND and register_pm both set");
        }
        mmc->pm_flags |= MMC_PM_IGNORE_PM_NOTIFY; /* pm not controlled by system but by client. */
    }
#endif

    platform_set_drvdata(pdev, mmc);

#ifdef CONFIG_MTK_HIBERNATION
    if(pdev->id == 1)
       register_swsusp_restore_noirq_func(ID_M_MSDC, msdc_drv_pm_restore_noirq, &(pdev->dev));
#endif

    /* Config card detection pin and enable interrupts */
    if (hw->flags & MSDC_CD_PIN_EN) {  /* set for card */
        msdc_enable_cd_irq(host, 1);
    }

    ret = mmc_add_host(mmc);
    if (ret) goto free_irq;
    if(host->hw->flags & MSDC_SDIO_IRQ)
    {
        ghost = host;
        sdr_set_bits(SDC_CFG, SDC_CFG_SDIOIDE); //enable sdio detection
    }
    //if (hw->flags & MSDC_CD_PIN_EN)
    host->sd_cd_insert_work = 1;

#ifdef DEBUG_TEST_FOR_SIGNAL
    /* use EINT1 for trigger signal */
    /* need to remove gpio warning log at
     * mediatek/kernel/include/mach/mt_gpio_core.h
     * mediatek/platform/{project}/kernel/drivers/gpio/mt_gpio_affix.c */
    mt_set_gpio_mode(1, GPIO_MODE_00);
    mt_set_gpio_dir(1, GPIO_DIR_OUT);
    mt_set_gpio_pull_enable(1, 1);

    mt_set_gpio_out(1, 0); //1-high, 0-low
#endif

#ifdef MTK_MSDC_BRINGUP_DEBUG
    MSDC_TRC_PRINT(MSDC_INFO,  "PROBE Done with mmc->caps=0x%x, mmc->caps2=0x%x\n", mmc->caps, mmc->caps2);
    MSDC_DBG_PRINT(MSDC_DEBUG, "msdcpll en[0x250][bit0 should be 0x1]=0x%x, msdcpll on[0x25C][bit0~1 should be 0x1]=0x%x, mux[0x0070][bit23, 16~19 for msdc0, bit31, 24~26 for msdc1]=0x%x\n",
                      sdr_read32(msdc_apmixed_base+0x250), sdr_read32(msdc_apmixed_base+0x25C),  sdr_read32(msdc_topckgen_base+0x070));
#endif

#ifdef FPGA_PLATFORM
  #ifdef CONFIG_MTK_EMMC_SUPPORT
    MSDC_DBG_PRINT(MSDC_DEBUG,"[%s]: waiting emmc init complete\n", __func__);
    host->mmc->card_init_wait(host->mmc);
	    MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: start msdc%d read write compare test\n", __func__, host->id);
	    emmc_multi_rw_compare(host->id, 0x200, 0xf);
	    MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: finish read write compare test\n", __func__);
  #endif
#endif

#ifdef ___MSDC_DEBUG___	
	msdc_dump_register(host);	
#endif /* ___MSDC_DEBUG___ */

    return 0;

free_irq:
    free_irq(irq, host);
    MSDC_ERR_PRINT(MSDC_ERROR,"[%s]: msdc%d init fail free irq!\n", __func__, host->id);
release:
    platform_set_drvdata(pdev, NULL);
    msdc_deinit_hw(host);
    MSDC_ERR_PRINT(MSDC_ERROR,"[%s]: msdc%d init fail release!\n", __func__, host->id);

    tasklet_kill(&host->card_tasklet);
#ifdef MTK_MSDC_FLUSH_BY_CLK_GATE
    if((host->hw->host_function == MSDC_EMMC) && (host->mmc->caps2 & MMC_CAP2_CACHE_CTRL))
        tasklet_kill(&host->flush_cache_tasklet);
#endif
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
  #ifdef MTK_SDIO30_DETECT_THERMAL
    del_timer(&host->ot_timer);
  #endif // MTK_SDIO30_DETECT_THERMAL
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT

    mmc_free_host(mmc);

    return ret;
}
#endif
#if 0
static int msdc_drv_probe(struct platform_device *pdev)
{
    struct mmc_host *mmc;
    struct resource *mem;
    struct msdc_host *host;
    struct msdc_hw *hw;
    void __iomem *base;
    int ret, irq;
    struct irq_data l_irq_data;
#ifdef MTK_MSDC_USE_CACHE
    BOOTMODE boot_mode;
#endif

    if (first_probe == 0) {
        first_probe ++;

#ifndef FPGA_PLATFORM
        // work aqround here.
        enable_clock(msdc_cg_clk_id[0], "SD");
        enable_clock(msdc_cg_clk_id[1], "SD");
        enable_clock(msdc_cg_clk_id[2], "SD");
        enable_clock(msdc_cg_clk_id[3], "SD");

        disable_clock(msdc_cg_clk_id[0], "SD");
        disable_clock(msdc_cg_clk_id[1], "SD");
        disable_clock(msdc_cg_clk_id[2], "SD");
        disable_clock(msdc_cg_clk_id[3], "SD");
#endif

    }

    /* Allocate MMC host for this device */
    mmc = mmc_alloc_host(sizeof(struct msdc_host), &pdev->dev);
    if (!mmc) return -ENOMEM;
#if 0
    workqueue = alloc_ordered_workqueue("kmmrmcd", 0);
    if (!workqueue){
        mmc_remove_host(mmc);
        return -ENOMEM;
    }
#endif
    hw   = (struct msdc_hw*)pdev->dev.platform_data;
    mem  = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    irq  = platform_get_irq(pdev, 0);
    base = IO_PHYS_TO_VIRT(mem->start);

    l_irq_data.irq = irq;

    BUG_ON((!hw) || (!mem) || (irq < 0));

    mem = request_mem_region(mem->start, mem->end - mem->start + 1, DRV_NAME);
    if (mem == NULL) {
        mmc_free_host(mmc);
        return -EBUSY;
    }

    /* Set host parameters to mmc */
    mmc->ops        = &mt_msdc_ops;
    mmc->f_min      = HOST_MIN_MCLK;
    mmc->f_max      = HOST_MAX_MCLK;
    mmc->ocr_avail  = MSDC_OCR_AVAIL;

    /* For sd card: MSDC_SYS_SUSPEND | MSDC_WP_PIN_EN | MSDC_CD_PIN_EN | MSDC_REMOVABLE | MSDC_HIGHSPEED,
       For sdio   : MSDC_EXT_SDIO_IRQ | MSDC_HIGHSPEED */
    if (hw->flags & MSDC_HIGHSPEED) {
        mmc->caps   = MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED;
    }
    if (hw->data_pins == 4) {
        mmc->caps  |= MMC_CAP_4_BIT_DATA;
    } else if (hw->data_pins == 8) {
        mmc->caps  |= MMC_CAP_8_BIT_DATA | MMC_CAP_4_BIT_DATA;
    }
    if ((hw->flags & MSDC_SDIO_IRQ) || (hw->flags & MSDC_EXT_SDIO_IRQ))
        mmc->caps |= MMC_CAP_SDIO_IRQ;  /* yes for sdio */
    if(hw->flags & MSDC_UHS1){
        mmc->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104;    // | MMC_CAP_SET_XPC_180;
        mmc->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
    }
#ifdef CONFIG_EMMC_50_FEATURE
    MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: hw->flags=0x%lx, MSDC_HS400=0x%x\n", __func__, hw->flags, MSDC_HS400);
    if(hw->flags & MSDC_HS400){
        mmc->caps2 |= MMC_CAP2_HS400_1_8V_DDR;
    }
#endif
    if(hw->flags & MSDC_DDR)
        mmc->caps |= MMC_CAP_UHS_DDR50|MMC_CAP_1_8V_DDR;
    if(!(hw->flags & MSDC_REMOVABLE))
        mmc->caps |= MMC_CAP_NONREMOVABLE;
    //else
        //mmc->caps2 |= MMC_CAP2_DETECT_ON_ERR;

#ifdef MTK_MSDC_USE_CMD23
    if (hw->host_function == MSDC_EMMC){
        mmc->caps |= MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY | MMC_CAP_CMD23;
    } else {
        mmc->caps |= MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY;
    }
#else
    mmc->caps |= MMC_CAP_ERASE | MMC_CAP_WAIT_WHILE_BUSY;
#endif

#ifdef MTK_MSDC_USE_CACHE
    /* only enable the emmc cache feature for normal boot up, alarm boot up, and sw reboot*/
    boot_mode = get_boot_mode();

    if((hw->host_function == MSDC_EMMC) && (hw->flags & MSDC_CACHE) &&
        ((boot_mode == NORMAL_BOOT) || (boot_mode == ALARM_BOOT) || (boot_mode == SW_REBOOT)))
        mmc->caps2 |= MMC_CAP2_CACHE_CTRL;
#endif

    /* MMC core transfer sizes tunable parameters */
    //mmc->max_hw_segs   = MAX_HW_SGMTS;
    mmc->max_segs   = MAX_HW_SGMTS;
    //mmc->max_phys_segs = MAX_PHY_SGMTS;
    if(hw->host_function == MSDC_SDIO)
        mmc->max_seg_size  = MAX_SGMT_SZ_SDIO;
    else
        mmc->max_seg_size  = MAX_SGMT_SZ;
    mmc->max_blk_size  = HOST_MAX_BLKSZ;
    mmc->max_req_size  = MAX_REQ_SZ;
    mmc->max_blk_count = mmc->max_req_size;

    host = mmc_priv(mmc);
    host->hw             = hw;
    host->mmc            = mmc;  /* msdc_check_init_done() need */
    host->id             = pdev->id;
    host->error          = 0;
    host->irq            = irq;
    host->base           = base;
    host->mclk           = 0;                   /* mclk: the request clock of mmc sub-system */
    host->hclk           = (pdev->id == 3) ? hclks_msdc50[hw->clk_src] : hclks_msdc30[hw->clk_src];  /* hclk: clock of clock source to msdc controller */
    host->sclk           = 0;                   /* sclk: the really clock after divition */
    host->pm_state       = PMSG_RESUME;
    host->suspend        = 0;
    host->core_clkon     = 0;
    host->card_clkon     = 0;
    host->clk_gate_count = 0;
    host->core_power     = 0;
    host->power_mode     = MMC_POWER_OFF;
    host->power_control  = NULL;
    host->power_switch   = NULL;
#ifdef SDIO_ERROR_BYPASS
    host->sdio_error = 0;
#endif
#ifndef FPGA_PLATFORM
    msdc_set_host_power_control(host);
    if((host->hw->host_function == MSDC_SD) && (host->hw->flags & MSDC_CD_PIN_EN)){
        msdc_sd_power(host,1); //work around : hot-plug project SD card LDO alway on if no SD card insert
        msdc_sd_power(host,0);
    }
#endif
    if(host->hw->host_function == MSDC_EMMC && !(host->hw->flags & MSDC_UHS1))
       host->mmc->f_max = 50000000;
    //host->card_inserted = hw->flags & MSDC_REMOVABLE ? 0 : 1;
    host->timeout_ns = 0;
    host->timeout_clks = DEFAULT_DTOC * 1048576;

#ifndef MTK_MSDC_USE_CMD23
    if(host->hw->host_function != MSDC_SDIO)
        host->autocmd |= MSDC_AUTOCMD12;
    else
        host->autocmd &= ~MSDC_AUTOCMD12;
#else
    if (host->hw->host_function == MSDC_EMMC){
        host->autocmd &= ~MSDC_AUTOCMD12;

#if (1 == MSDC_USE_AUTO_CMD23)
        host->autocmd |= MSDC_AUTOCMD23;
#endif

    } else if (host->hw->host_function == MSDC_SD){
        host->autocmd |= MSDC_AUTOCMD12;
    } else {
        host->autocmd &= ~MSDC_AUTOCMD12;
    }
#endif /* end of MTK_MSDC_USE_CMD23 */

    host->mrq = NULL;
    //init_MUTEX(&host->sem); /* we don't need to support multiple threads access */

    host->dma.used_gpd = 0;
    host->dma.used_bd = 0;

    /* using dma_alloc_coherent*/  /* todo: using 1, for all 4 slots */
    host->dma.gpd = dma_alloc_coherent(NULL, MAX_GPD_NUM * sizeof(gpd_t), &host->dma.gpd_addr, GFP_KERNEL);
	DMAGPDPTR_DBGCHK(host->dma.gpd,16);
	
    host->dma.bd =  dma_alloc_coherent(NULL, MAX_BD_NUM  * sizeof(bd_t),  &host->dma.bd_addr,  GFP_KERNEL);
    DMABDPTR_DBGCHK(host->dma.bd,16);
	
   	BUG_ON((!host->dma.gpd) || (!host->dma.bd));
    msdc_init_gpd_bd(host, &host->dma);
    msdc_clock_src[host->id] = hw->clk_src;
    msdc_host_mode[host->id] = mmc->caps;
    msdc_host_mode2[host->id] = mmc->caps2;
    /*for emmc*/
    mtk_msdc_host[pdev->id] = host;
    host->write_timeout_uhs104 = 0;
    host->write_timeout_emmc = 0;
    host->read_timeout_uhs104 = 0;
    host->read_timeout_emmc = 0;
    host->sw_timeout = 0;
    host->tune = 0;
    host->state = 0;
    host->sd_cd_insert_work = 0;
    host->block_bad_card = 0;
    host->sd_30_busy = 0;
    msdc_reset_tmo_tune_counter(host, all_counter);
    msdc_reset_pwr_cycle_counter(host);

    if (is_card_sdio(host)|| (host->hw->flags & MSDC_SDIO_IRQ))
    {
        host->saved_para.suspend_flag = 0;
        host->saved_para.msdc_cfg = 0;
        host->saved_para.mode = 0;
        host->saved_para.div = 0;
        host->saved_para.sdc_cfg = 0;
        host->saved_para.iocon = 0;
        host->saved_para.state = 0;
        host->saved_para.hz = 0;
        host->saved_para.cmd_resp_ta_cntr = 0; //for SDIO 3.0
        host->saved_para.wrdat_crc_ta_cntr = 0; //for SDIO 3.0
        host->saved_para.int_dat_latch_ck_sel = 0; //for SDIO 3.0
        host->saved_para.ckgen_msdc_dly_sel = 0; //for SDIO 3.0
        host->saved_para.inten_sdio_irq = 0; //default disable
        wake_lock_init(&host->trans_lock, WAKE_LOCK_SUSPEND, "MSDC Transfer Lock");
    }
    tasklet_init(&host->card_tasklet, msdc_tasklet_card, (ulong)host);
#ifdef MTK_MSDC_FLUSH_BY_CLK_GATE
    if((host->hw->host_function == MSDC_EMMC) && (host->mmc->caps2 & MMC_CAP2_CACHE_CTRL))
        tasklet_init(&host->flush_cache_tasklet, msdc_tasklet_flush_cache, (ulong)host);
#endif
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
    atomic_set(&host->ot_done, 1);
	atomic_set(&host->sdio_stopping, 0);
    host->ot_work.host = host;
    host->ot_work.chg_volt = 0;
    atomic_set(&host->ot_work.ot_disable, 0);
	atomic_set(&host->ot_work.autok_done, 0);
	init_completion(&host->ot_work.ot_complete);
  #ifdef MTK_SDIO30_DETECT_THERMAL
    host->pre_temper = -127000;
    host->ot_period_check_start = 0;
    init_timer(&host->ot_timer);
    host->ot_timer.function	= start_online_tuning;
    host->ot_timer.data		= (unsigned long)host;
  #endif // MTK_SDIO30_DETECT_THERMAL
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT
    //INIT_DELAYED_WORK(&host->remove_card, msdc_remove_card);
    spin_lock_init(&host->lock);
    spin_lock_init(&host->clk_gate_lock);
    spin_lock_init(&host->remove_bad_card);
    /* init dynamtic timer */
    init_timer(&host->timer);
    //host->timer.expires = jiffies + HZ;
    host->timer.function = msdc_timer_pm;
    host->timer.data = (unsigned long)host;

    msdc_init_hw(host);

    //mt65xx_irq_set_sens(irq, MT65xx_EDGE_SENSITIVE);
    //mt65xx_irq_set_polarity(irq, MT65xx_POLARITY_LOW);
    ret = request_irq((unsigned int)irq, msdc_irq, IRQF_TRIGGER_LOW, DRV_NAME, host);
    if (ret) goto release;
    //mt65xx_irq_unmask(&l_irq_data);
    //enable_irq(irq);

    if (hw->flags & MSDC_CD_PIN_EN) { /* not set for sdio */
        if (hw->request_cd_eirq) {
            hw->request_cd_eirq(msdc_eirq_cd, (void*)host); /* msdc_eirq_cd will not be used! */
        }
    }

    if (hw->request_sdio_eirq) /* set to combo_sdio_request_eirq() for WIFI */
        hw->request_sdio_eirq(msdc_eirq_sdio, (void*)host); /* msdc_eirq_sdio() will be called when EIRQ */

    if (hw->register_pm) {/* yes for sdio */
        hw->register_pm(msdc_pm, (void*)host);  /* combo_sdio_register_pm() */
        if(hw->flags & MSDC_SYS_SUSPEND) { /* will not set for WIFI */
            MSDC_ERR_PRINT(MSDC_ERROR,"MSDC_SYS_SUSPEND and register_pm both set");
        }
        mmc->pm_flags |= MMC_PM_IGNORE_PM_NOTIFY; /* pm not controlled by system but by client. */
    }

    platform_set_drvdata(pdev, mmc);

#ifdef CONFIG_MTK_HIBERNATION
    if(pdev->id == 1)
       register_swsusp_restore_noirq_func(ID_M_MSDC, msdc_drv_pm_restore_noirq, &(pdev->dev));
#endif

    /* Config card detection pin and enable interrupts */
    if (hw->flags & MSDC_CD_PIN_EN) {  /* set for card */
        msdc_enable_cd_irq(host, 1);
    } else {
        msdc_enable_cd_irq(host, 0);
    }

    ret = mmc_add_host(mmc);
    if (ret) goto free_irq;
    if(host->hw->flags & MSDC_SDIO_IRQ)
    {
        ghost = host;
        sdr_set_bits(SDC_CFG, SDC_CFG_SDIOIDE); //enable sdio detection
    }
    //if (hw->flags & MSDC_CD_PIN_EN)
    host->sd_cd_insert_work = 1;

#ifdef DEBUG_TEST_FOR_SIGNAL
    /* use EINT1 for trigger signal */
    /* need to remove gpio warning log at
     * mediatek/kernel/include/mach/mt_gpio_core.h
     * mediatek/platform/{project}/kernel/drivers/gpio/mt_gpio_affix.c */
    mt_set_gpio_mode(1, GPIO_MODE_00);
    mt_set_gpio_dir(1, GPIO_DIR_OUT);
    mt_set_gpio_pull_enable(1, 1);

    mt_set_gpio_out(1, 0); //1-high, 0-low
#endif

#ifdef MTK_MSDC_BRINGUP_DEBUG
    MSDC_TRC_PRINT(MSDC_INIT, "[%s]: msdc%d, mmc->caps=0x%x, mmc->caps2=0x%x\n", __func__, host->id, mmc->caps, mmc->caps2);
    MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: msdc%d, msdcpll en[0xF000C250][bit0 should be 0x1]=0x%x, msdcpll on[0xF000C25C][bit0~1 should be 0x1]=0x%x, mux[0xF0000070][bit23, 16~19 for msdc0, bit31, 24~26 for msdc1]=0x%x\n",
                     __func__, host->id, sdr_read32(apmixed_base1+0x250), sdr_read32(apmixed_base1+0x25C),  sdr_read32(topckgen_base+0x070));
#endif

#ifdef FPGA_PLATFORM
#if 0 //def CONFIG_MTK_EMMC_SUPPORT
    MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: waiting emmc init complete\n", __func__);
    host->mmc->card_init_wait(host->mmc);
    MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: start read write compare test\n", __func__);
    emmc_multi_rw_compare(0, 0x200, 0xf);
    MSDC_DBG_PRINT(MSDC_DEBUG, "[%s]: finish read write compare test\n", __func__);
#endif
#endif

    return 0;

free_irq:
    free_irq(irq, host);
release:
    platform_set_drvdata(pdev, NULL);
    msdc_deinit_hw(host);

    tasklet_kill(&host->card_tasklet);
#ifdef MTK_MSDC_FLUSH_BY_CLK_GATE
    if((host->hw->host_function == MSDC_EMMC) && (host->mmc->caps2 & MMC_CAP2_CACHE_CTRL))
        tasklet_kill(&host->flush_cache_tasklet);
#endif
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
  #ifdef MTK_SDIO30_DETECT_THERMAL
    del_timer(&host->ot_timer);
  #endif // MTK_SDIO30_DETECT_THERMAL
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT
    if (mem)
        release_mem_region(mem->start, mem->end - mem->start + 1);

    mmc_free_host(mmc);

    return ret;
}
#endif

/* 4 device share one driver, using "drvdata" to show difference */
static int msdc_drv_remove(struct platform_device *pdev)
{
    struct mmc_host *mmc;
    struct msdc_host *host;
    struct resource *mem;

    mmc = platform_get_drvdata(pdev);
    MSDC_BUGON(!mmc);

    host = mmc_priv(mmc);
    MSDC_BUGON(!host);

    MSDC_DBG_PRINT(MSDC_DEBUG, "removed !!!");

    platform_set_drvdata(pdev, NULL);
    mmc_remove_host(host->mmc);
    msdc_deinit_hw(host);

    tasklet_kill(&host->card_tasklet);
#ifdef MTK_MSDC_FLUSH_BY_CLK_GATE
    if((host->hw->host_function == MSDC_EMMC) && (host->mmc->caps2 & MMC_CAP2_CACHE_CTRL))
        tasklet_kill(&host->flush_cache_tasklet);
#endif
#ifdef MTK_SDIO30_ONLINE_TUNING_SUPPORT
  #ifdef MTK_SDIO30_DETECT_THERMAL
    del_timer(&host->ot_timer);
  #endif // MTK_SDIO30_DETECT_THERMAL
#endif // MTK_SDIO30_ONLINE_TUNING_SUPPORT
    free_irq(host->irq, host);

    dma_free_coherent(NULL, MAX_GPD_NUM * sizeof(gpd_t), host->dma.gpd, host->dma.gpd_addr);
    dma_free_coherent(NULL, MAX_BD_NUM  * sizeof(bd_t),  host->dma.bd,  host->dma.bd_addr);

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

    if (mem)
        release_mem_region(mem->start, mem->end - mem->start + 1);

    mmc_free_host(host->mmc);

    return 0;
}

#ifdef CONFIG_PM
static int msdc_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
    int ret = 0;
    struct mmc_host *mmc = platform_get_drvdata(pdev);
    struct msdc_host *host = mmc_priv(mmc);
    void __iomem *base = host->base;

    if (mmc && state.event == PM_EVENT_SUSPEND && (host->hw->flags & MSDC_SYS_SUSPEND)) { /* will set for card */
        msdc_pm(state, (void*)host);
    }

    // WIFI slot should be off when enter suspend
    if (mmc && state.event == PM_EVENT_SUSPEND && (!(host->hw->flags & MSDC_SYS_SUSPEND))) {
        msdc_suspend_clock(host);
        if(host->error == -EBUSY){
            ret = host->error;
            host->error = 0;
        }
    }

    if (is_card_sdio(host) || (host->hw->flags & MSDC_SDIO_IRQ))
    {
        if(host->clk_gate_count > 0){
            host->error = 0;
            return -EBUSY;
        }
        if (host->saved_para.suspend_flag==0)
        {
            host->saved_para.hz = host->mclk;
            if (host->saved_para.hz)
            {
                host->saved_para.suspend_flag = 1;
                //mb();
                msdc_ungate_clock(host);
                sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD, host->saved_para.mode);
                sdr_get_field(MSDC_CFG, MSDC_CFG_CKDIV, host->saved_para.div);
                sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,  host->saved_para.int_dat_latch_ck_sel);  //for SDIO 3.0
                sdr_get_field(MSDC_PATCH_BIT0,  MSDC_PB0_CKGEN_MSDC_DLY_SEL,    host->saved_para.ckgen_msdc_dly_sel); //for SDIO 3.0
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,    host->saved_para.cmd_resp_ta_cntr); //for SDIO 3.0
                sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr); //for SDIO 3.0
                sdr_get_field(MSDC_INTEN, MSDC_INT_SDIOIRQ, host->saved_para.inten_sdio_irq); //get INTEN status for SDIO
                host->saved_para.msdc_cfg = sdr_read32(MSDC_CFG);
                host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);
                host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE);
                host->saved_para.sdc_cfg = sdr_read32(SDC_CFG);
                host->saved_para.iocon = sdr_read32(MSDC_IOCON);
                host->saved_para.state = host->state;
                msdc_gate_clock(host, 0);
                if(host->error == -EBUSY){
                    ret = host->error;
                    host->error = 0;
                }
            }
            MSDC_DBG_PRINT(MSDC_DEBUG, "msdc suspend cur_cfg=%x, save_cfg=%x, cur_hz=%d, save_hz=%d", sdr_read32(MSDC_CFG), host->saved_para.msdc_cfg, host->mclk, host->saved_para.hz);
        }
    }
    return ret;
}

static int msdc_drv_resume(struct platform_device *pdev)
{
    int ret = 0;
    struct mmc_host *mmc = platform_get_drvdata(pdev);
    struct msdc_host *host = mmc_priv(mmc);
    struct pm_message state;

    if(host->hw->flags & MSDC_SDIO_IRQ)
    {
        MSDC_DBG_PRINT(MSDC_DEBUG, "msdc msdc_drv_resume \n");
    }
    state.event = PM_EVENT_RESUME;
    if (mmc && (host->hw->flags & MSDC_SYS_SUSPEND)) {/* will set for card */
        msdc_pm(state, (void*)host);
    }

    /* This mean WIFI not controller by PM */

    return ret;
}
#endif

#ifdef CONFIG_OF
static const struct of_device_id msdc_of_ids[] = {
    {   .compatible = "mediatek,MSDC0", },
    {   .compatible = "mediatek,MSDC1", },
    {   .compatible = "mediatek,MSDC2", },
    {   .compatible = "mediatek,MSDC3", },
    { },
};
#endif

static struct platform_driver mt_msdc_driver = {
    .probe   = msdc_drv_probe,
    .remove  = msdc_drv_remove,
#ifdef CONFIG_PM
    .suspend = msdc_drv_suspend,
    .resume  = msdc_drv_resume,
#endif
    .driver  = {
        .name  = DRV_NAME,
        .owner = THIS_MODULE,
      #ifdef CONFIG_OF
        .of_match_table = msdc_of_ids,
      #endif
    },
};

/*--------------------------------------------------------------------------*/
/* module init/exit                                                      */
/*--------------------------------------------------------------------------*/
static int __init mt_msdc_init(void)
{
    int ret;

    ret = platform_driver_register(&mt_msdc_driver);
    if (ret) {
        MSDC_ERR_PRINT(MSDC_ERROR, DRV_NAME ": Can't register driver");
        return ret;
    }

#if defined(CONFIG_MTK_EMMC_SUPPORT) && defined(CONFIG_PROC_FS)
    if ((proc_emmc = proc_create( "emmc", 0, NULL, &proc_emmc_fops)));
#endif /* CONFIG_MTK_EMMC_SUPPORT && CONFIG_PROC_FS */

    MSDC_TRC_PRINT(MSDC_INIT, DRV_NAME ": MediaTek MSDC Driver\n");

    msdc_debug_proc_init();
    msdc_init_dma_latest_address();
    return 0;
}

static void __exit mt_msdc_exit(void)
{
    platform_driver_unregister(&mt_msdc_driver);

#ifdef CONFIG_MTK_HIBERNATION
    unregister_swsusp_restore_noirq_func(ID_M_MSDC);
#endif
}

module_init(mt_msdc_init);
module_exit(mt_msdc_exit);
#ifdef CONFIG_MTK_EMMC_SUPPORT
late_initcall_sync(msdc_get_cache_region);
#endif
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek SD/MMC Card Driver");
#ifdef CONFIG_MTK_EMMC_SUPPORT
EXPORT_SYMBOL(ext_csd);
EXPORT_SYMBOL(g_emmc_mode_switch);
#endif
EXPORT_SYMBOL(mtk_msdc_host);

