
#ifndef __HDMI_DRV_H__
#define __HDMI_DRV_H__

#ifdef CONFIG_MTK_INTERNAL_HDMI_SUPPORT

#include "hdmictrl.h"
#include "hdmiedid.h"
#include "hdmicec.h"

#define AVD_TMR_ISR_TICKS   10
#define MDI_BOUCING_TIMING  50//20 //20ms

typedef enum {
	HDMI_SHELL,
	HDMI_DDC,
	HDMI_CEC,
	HDMI_REG_NUM,
} HDMI_REG_ENUM;

typedef enum {
	AP_CCIF0,		/*  PLL/TVD */
	TOPCK_GEN,
	INFRA_SYS,
	MMSYS_CONFIG,
	GPIO_REG,
	DPI1_REG,
	EFUSE_REG,
	GIC_REG,
	PERISYS_REG,
	HDMI_REF_REG_NUM,
} HDMI_REF_MODULE_ENUM;

typedef enum {
	MMSYS_POWER,		/* Must be first,  power of the mmsys */
	PERI_DDC,
	MMSYS_HDMI_HDCP,
	MMSYS_HDMI_HDCP24,
	INFRA_SYS_CEC,
	MMSYS_HDMI_PLL,
	MMSYS_HDMI_PIXEL,
	MMSYS_HDMI_AUDIO,
	MMSYS_HDMI_SPIDIF,
	TOP_HDMI_SEL,
	TOP_HDMISEL_DIG_CTS,
	TOP_HDMISEL_D2,
	TOP_HDMISEL_D3,
	TOP_HDCP_SEL,
	TOP_HDCPSEL_SYS4D2,
	TOP_HDCPSEL_SYS3D4,
	TOP_HDCPSEL_UNIV2D2,
	TOP_HDCP24_SEL,
	TOP_HDCP24SEL_UNIVPD26,
	TOP_HDCP24SEL_UNIVPD52,
	TOP_HDCP24SEL_UNIVP2D8,
	HDMI_SEL_CLOCK_NUM,
} HDMI_REF_CLOCK_ENUM;

extern unsigned long hdmi_reg[HDMI_REG_NUM];
extern unsigned long hdmi_ref_reg[HDMI_REF_REG_NUM];
extern struct clk *hdmi_ref_clock[HDMI_SEL_CLOCK_NUM];
extern unsigned char hdmi_dpi_output;

#define HDMIDRV_BASE   (hdmi_reg[HDMI_SHELL])
#define HDMIDDC_BASE   (hdmi_reg[HDMI_DDC])
#define HDMICEC_BASE   (hdmi_reg[HDMI_CEC])

#define HDMISYS_BASE   (hdmi_ref_reg[MMSYS_CONFIG])
#define HDMIPLL_BASE   (hdmi_ref_reg[AP_CCIF0])	/* pll */
#define HDMICKGEN_BASE (hdmi_ref_reg[TOPCK_GEN])
#define HDMIPAD_BASE   (hdmi_ref_reg[GPIO_REG])
#define HDMI_INFRA_SYS (hdmi_ref_reg[INFRA_SYS])
#define HDMI_EFUSE_BASE (hdmi_ref_reg[EFUSE_REG])


typedef enum
{
 HDMI_CEC_CMD=0,
 HDMI_PLUG_DETECT_CMD,
 HDMI_HDCP_PROTOCAL_CMD,	
 HDMI_DISABLE_HDMI_TASK_CMD,
 MAX_HDMI_TMR_NUMBER
	
}HDMI_TASK_COMMAND_TYPE_T;

#endif

#ifndef ARY_SIZE
#define ARY_SIZE(x) (sizeof((x)) / sizeof((x[0])))
#endif

typedef enum
{
    HDMI_POLARITY_RISING  = 0,
    HDMI_POLARITY_FALLING = 1
}HDMI_POLARITY;

typedef enum
{
    HDMI_CLOCK_PHASE_0  = 0,
    HDMI_CLOCK_PHASE_90 = 1
}HDMI_CLOCK_PHASE;

typedef enum
{
    HDMI_COLOR_ORDER_RGB = 0,
    HDMI_COLOR_ORDER_BGR = 1
}HDMI_COLOR_ORDER;

typedef enum
{    
   IO_DRIVING_CURRENT_8MA       = (1 << 0),
   IO_DRIVING_CURRENT_4MA       = (1 << 1),
   IO_DRIVING_CURRENT_2MA       = (1 << 2),
   IO_DRIVING_CURRENT_SLEW_CNTL = (1 << 3),
}IO_DRIVING_CURRENT;


typedef enum
{
    HDMI_VIN_FORMAT_RGB565,
    HDMI_VIN_FORMAT_RGB666,
    HDMI_VIN_FORMAT_RGB888,
} HDMI_VIDEO_INPUT_FORMAT;

typedef enum
{
	HDMI_VOUT_FORMAT_RGB888,
	HDMI_VOUT_FORMAT_YUV422,
	HDMI_VOUT_FORMAT_YUV444,
}HDMI_VIDEO_OUTPUT_FORMAT;

typedef enum
{
	HDMI_AUDIO_PCM_16bit_48000,
	HDMI_AUDIO_PCM_16bit_44100,
	HDMI_AUDIO_PCM_16bit_32000,
	HDMI_AUDIO_SOURCE_STREAM,
}HDMI_AUDIO_FORMAT; 

typedef struct
{
	HDMI_VIDEO_RESOLUTION		vformat;
	HDMI_VIDEO_INPUT_FORMAT 	vin;
	HDMI_VIDEO_OUTPUT_FORMAT 	vout;
	HDMI_AUDIO_FORMAT			aformat;
}HDMI_CONFIG;

typedef enum{
    HDMI_OUTPUT_MODE_LCD_MIRROR,
    HDMI_OUTPUT_MODE_VIDEO_MODE,
    HDMI_OUTPUT_MODE_DPI_BYPASS
}HDMI_OUTPUT_MODE;
typedef enum{
    HDMI_CABLE,
    MHL_CABLE,
    MHL_SMB_CABLE,
    MHL_2_CABLE ///MHL 2.0
}HDMI_CABLE_TYPE;

typedef struct
{
	unsigned int width;
	unsigned int height;

	HDMI_CONFIG init_config;

    /* polarity parameters */
    HDMI_POLARITY clk_pol;
    HDMI_POLARITY de_pol;
    HDMI_POLARITY vsync_pol;
    HDMI_POLARITY hsync_pol;

    /* timing parameters */
    unsigned int hsync_pulse_width;
    unsigned int hsync_back_porch;
    unsigned int hsync_front_porch;
    unsigned int vsync_pulse_width;
    unsigned int vsync_back_porch;
    unsigned int vsync_front_porch;
    
    /* output format parameters */
    HDMI_COLOR_ORDER rgb_order;

    /* intermediate buffers parameters */
    unsigned int intermediat_buffer_num; // 2..3

    /* iopad parameters */
    IO_DRIVING_CURRENT io_driving_current;
    HDMI_OUTPUT_MODE    output_mode;

    int is_force_awake;
    int is_force_landscape;

    unsigned int scaling_factor; // determine the scaling of output screen size, valid value 0~10
                                 // 0 means no scaling, 5 means scaling to 95%, 10 means 90%
     bool NeedSwHDCP;
     HDMI_CABLE_TYPE cabletype;    
}HDMI_PARAMS;

typedef struct 
{
	unsigned char	e_hdmi_aud_in;
	unsigned char	e_iec_frame;
	unsigned char	e_hdmi_fs;
	unsigned char	e_aud_code;
	unsigned char	u1Aud_Input_Chan_Cnt;
	unsigned char	e_I2sFmt;
	unsigned char	u1HdmiI2sMclk;
	unsigned char	bhdmi_LCh_status[5];
	unsigned char	bhdmi_RCh_status[5];
}	HDMIDRV_AUDIO_PARA;

typedef enum{
	HDMI_STATE_NO_DEVICE,
	HDMI_STATE_ACTIVE,
	HDMI_STATE_PLUGIN_ONLY,
	HDMI_STATE_EDID_UPDATE,
	HDMI_STATE_CEC_UPDATE
}HDMI_STATE;

typedef enum{
	HDMI_CEC_STATE_PLUG_OUT=0,
	HDMI_CEC_STATE_GET_PA,
	HDMI_CEC_STATE_TX_STS,
	HDMI_CEC_STATE_GET_CMD
}HDMI_CEC_STATE;
// ---------------------------------------------------------------------------

typedef struct
{
    void (*set_reset_pin)(unsigned int value);
    int  (*set_gpio_out)(unsigned int gpio, unsigned int value);
    void (*udelay)(unsigned int us);
    void (*mdelay)(unsigned int ms);
    void (*wait_transfer_done)(void);
	void (*state_callback)(HDMI_STATE state);
    void (*cec_state_callback)(HDMI_CEC_STATE state);
}HDMI_UTIL_FUNCS;


typedef struct
{
    void (*set_util_funcs)(const HDMI_UTIL_FUNCS *util);
    void (*get_params)(HDMI_PARAMS *params);

    int (*init)(void);
    int (*enter)(void);
    int (*exit)(void);
    void (*suspend)(void);
    void (*resume)(void);
	int  (*audio_config)(HDMI_AUDIO_FORMAT aformat);
	int  (*video_config)(HDMI_VIDEO_RESOLUTION vformat, HDMI_VIDEO_INPUT_FORMAT vin, HDMI_VIDEO_OUTPUT_FORMAT vou);
	int  (*video_enable)(unsigned char enable);
	int  (*audio_enable)(unsigned char enable);
	int	 (*irq_enable)(unsigned char enable);
	int  (*power_on)(void);
	void (*power_off)(void);
    HDMI_STATE (*get_state)(void);
    void (*set_mode)(unsigned char ucMode);
    void (*dump)(void);
	void (*read)(unsigned int u2Reg, unsigned int *p4Data);
    void (*write)(unsigned int u2Reg, unsigned int u4Data);
    void (*log_enable)(unsigned short enable);
	void (*InfoframeSetting)(unsigned char i1typemode, unsigned char i1typeselect);
	void (*checkedid)(unsigned char i1noedid);
	void (*colordeep)(unsigned char u1colorspace, unsigned char u1deepcolor);
	void (*enablehdcp)(unsigned char u1hdcponoff);
	void (*setcecrxmode)(unsigned char u1cecrxmode);
	void (*hdmistatus)(void);
	void (*hdcpkey)(unsigned char *pbhdcpkey);
	void (*getedid)(HDMI_EDID_INFO_T *pv_get_info);
	void (*setcecla)(CEC_DRV_ADDR_CFG_T* prAddr);
	void (*sendsltdata)(unsigned char *pu1Data);
	void (*getceccmd)(CEC_FRAME_DESCRIPTION* frame);
	void (*getsltdata)(CEC_SLT_DATA* rCecSltData);
	void (*setceccmd)(CEC_SEND_MSG_T* msg);
       void (*getcectxstatus)(CEC_ACK_INFO_T *pt);
	void (*cecenable)(unsigned char u1EnCec);
	void (*getcecaddr)(CEC_ADDRESS *cecaddr);
	int (*audiosetting)(HDMIDRV_AUDIO_PARA* audio_para);
	int (*tmdsonoff)(unsigned char u1ionoff);
	void (*mutehdmi)(unsigned char u1flagvideomute, unsigned char u1flagaudiomute);
	void (*svpmutehdmi)(unsigned char u1svpvideomute, unsigned char u1svpaudiomute);
    void (*cecusrcmd)(unsigned int cmd, unsigned int* result);	   
	void (*checkedidheader)(void);   	
} HDMI_DRIVER;


// ---------------------------------------------------------------------------
//  HDMI Driver Functions
// ---------------------------------------------------------------------------

const HDMI_DRIVER* HDMI_GetDriver(void);

int hdmi_internal_probe(struct platform_device *pdev);
int hdmi_internal_video_config(HDMI_VIDEO_RESOLUTION vformat, HDMI_VIDEO_INPUT_FORMAT vin, HDMI_VIDEO_OUTPUT_FORMAT vout);

#endif
