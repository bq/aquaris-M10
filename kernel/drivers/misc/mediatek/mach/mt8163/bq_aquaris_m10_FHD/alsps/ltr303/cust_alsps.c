#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_alsps.h>
#include <mach/upmu_common.h>

static struct alsps_hw cust_alsps_hw = {
    .i2c_num    = 2,
	.polling_mode_ps =0,
	.polling_mode_als =1,
    .power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
    .als_level  = { 1,  10,  20,   50,  80,  150,  300, 800, 1600,  2400,  6000, 10000, 14000, 18000, 20000},
    .als_value  = {0, 10, 30,  60, 140, 180,  225,  320,  640,  940,  1280,  3600,  5600, 8200,  10240, 10240},
    .ps_threshold_high = 900,
    .ps_threshold_low = 600,
    .is_batch_supported_ps = false,
    .is_batch_supported_als = false,
};
struct alsps_hw *get_cust_alsps_hw(void) {
    return &cust_alsps_hw;
}

