/*
 * Copyright (c) 2014 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _GPAD_SDBG_REG_H
#define _GPAD_SDBG_REG_H
/*
 * Base address
 */

#define SDBG_MMU_BASE                    (0x00005000)
#define SDBG_MX_DBG_BASE                 (0x00004000)
#define SDBG_REG_DWP_BASE                (0x00003000)
#define SDBG_REG_CMN_BASE                (0x00002000)
#define SDBG_AREG_BASE                   (0x00001000)
#define SDBG_MFG_BASE                    (0x00000000)

#define SDBG_AREG_IRQ_RAW                (SDBG_AREG_BASE | 0x208)
#define SDBG_AREG_PA_IDLE_STATUS0        (SDBG_AREG_BASE | 0x180)
#define SDBG_AREG_PA_IDLE_STATUS1        (SDBG_AREG_BASE | 0x184)
#define SDBG_AREG_DEBUG_MODE_SEL         (SDBG_AREG_BASE | 0x1C4)
#define SDBG_AREG_DEBUG_BUS              (SDBG_AREG_BASE | 0x1C8)

#define DFD_AREG_REG_EN                  (SDBG_AREG_BASE | 0xE00)
#define DFD_AREG_RD_OFFSET               (SDBG_AREG_BASE | 0xE04)
#define DFD_AREG_QREG_RD                 (SDBG_AREG_BASE | 0xE08)
#define DFD_AREG_RSMINFO_RD              (SDBG_AREG_BASE | 0xE0C)
#define DFD_AREG_VPFMREG_RD              (SDBG_AREG_BASE | 0xE10)
#define DFD_AREG_PPFMREG_RD              (SDBG_AREG_BASE | 0xE14)


/*
 * Common register
 */
#define SDBG_REG_CMN_STATUS_REPORT0      (SDBG_REG_CMN_BASE | 0x48)
#define SDBG_REG_CMN_STATUS_REPORT1      (SDBG_REG_CMN_BASE | 0x4c)
#define SDBG_REG_CMN_STATUS_REPORT2      (SDBG_REG_CMN_BASE | 0x50)
#define SDBG_REG_CMN_STATUS_REPORT3      (SDBG_REG_CMN_BASE | 0x54)

#define SDBG_REG_CMN_ADVDBG              (SDBG_REG_CMN_BASE | 0x14)
#define SDBG_REG_CMN_BREAK_SID           (SDBG_REG_CMN_BASE | 0x18)
#define SDBG_REG_CMN_BREAK_IBASE         (SDBG_REG_CMN_BASE | 0x1C)
#define SDBG_REG_CMN_BREAK_LASTPC        (SDBG_REG_CMN_BASE | 0x20)

#define SDBG_REG_CMN_DEBUG_SET           (SDBG_REG_CMN_BASE)
#define SDBG_REG_CMN_INTP_STATUS         (SDBG_REG_CMN_BASE | 0x4)

#ifdef MARCH_VER
#define SDBG_REG_CMN_FRAME_INFO          (SDBG_REG_CMN_BASE | 0x7C)
#else
#define SDBG_REG_CMN_FRAME_INFO          (SDBG_REG_CMN_BASE | 0x84)
#endif

#define SDBG_REG_CMN_CPRF_CTRL           (SDBG_REG_CMN_BASE | 0x24)
#define SDBG_REG_CMN_CPRF_WRDATA         (SDBG_REG_CMN_BASE | 0x28)
#define SDBG_REG_CMN_CPRF_RDDATA         (SDBG_REG_CMN_BASE | 0x2C)

#define SDBG_REG_CMN_INST_PREFETCH       (SDBG_REG_CMN_BASE | 0x34)
#define SDBG_REG_CMN_INST_RELEASE        (SDBG_REG_CMN_BASE | 0x38)
#define SDBG_REG_CMN_INST_READ           (SDBG_REG_CMN_BASE | 0x3C)
#define SDBG_REG_CMN_INST_DATA_LOW       (SDBG_REG_CMN_BASE | 0x40)
#define SDBG_REG_CMN_INST_DATA_HIGH      (SDBG_REG_CMN_BASE | 0x44)

#define SDBG_REG_CMN_ACTIVITY_DETECT     (SDBG_REG_CMN_BASE | 0x08)
#define SDBG_REG_CMN_ACTIVITY_TIMEOUT    (SDBG_REG_CMN_BASE | 0x0C)
#define SDBG_REG_CMN_RETIRE_TIMEOUT      (SDBG_REG_CMN_BASE | 0x10)

#define SDBG_INTP_WP0_BIT                (0x1000)
#define SDBG_INTP_WP2_BIT                (0x4000)

/*
 * Common register status
 */
#define SDBG_REG_CMN_CXM_GLOBAL_CTRL     (SDBG_REG_CMN_BASE | 0x58)
#define SDBG_REG_CMN_CXM_GLOBAL_DATA     (SDBG_REG_CMN_BASE | 0x5C)
#define SDBG_REG_CMN_LSU_ERR_MADDR       (SDBG_REG_CMN_BASE | 0x60)
#define SDBG_REG_CMN_CPL1_ERR_MADDR      (SDBG_REG_CMN_BASE | 0x64)
#define SDBG_REG_CMN_ISC_ERR_MADDR       (SDBG_REG_CMN_BASE | 0x68)
#define SDBG_REG_CMN_CPRF_ERR_INFO       (SDBG_REG_CMN_BASE | 0x6C)
#define SDBG_REG_CMN_STAGE_IO            (SDBG_REG_CMN_BASE | 0x70)
#define SDBG_REG_CMN_TX_LTC_IO           (SDBG_REG_CMN_BASE | 0x74)
#define SDBG_REG_CMN_ISC_IO              (SDBG_REG_CMN_BASE | 0x78)
#define SDBG_REG_CMN_IDLE_STATUS         (SDBG_REG_CMN_BASE | 0x7C)
#define SDBG_REG_CMN_TYPE_WAVE_CNT       (SDBG_REG_CMN_BASE | 0x80)
#define SDBG_REG_CMN_STOP_WAVE           (SDBG_REG_CMN_BASE | 0x84)
#define SDBG_REG_CMN_VS0i_SID            (SDBG_REG_CMN_BASE | 0x88)
#define SDBG_REG_CMN_VS1i_SID            (SDBG_REG_CMN_BASE | 0x8C)
#define SDBG_REG_CMN_FSi_SID             (SDBG_REG_CMN_BASE | 0x90)
#define SDBG_REG_CMN_WP_WAVE_CNT         (SDBG_REG_CMN_BASE | 0x94)
#define SDBG_REG_CMN_WP0_LSM_REM         (SDBG_REG_CMN_BASE | 0x98)
#define SDBG_REG_CMN_WP0_VRF_CRF_REM     (SDBG_REG_CMN_BASE | 0x9C)
#define SDBG_REG_CMN_WP1_LSM_REM         (SDBG_REG_CMN_BASE | 0xA0)
#define SDBG_REG_CMN_WP1_VRF_CRF_REM     (SDBG_REG_CMN_BASE | 0xA4)
#define SDBG_REG_CMN_WP2_LSM_REM         (SDBG_REG_CMN_BASE | 0xA8)
#define SDBG_REG_CMN_WP2_VRF_CRF_REM     (SDBG_REG_CMN_BASE | 0xAC)
#define SDBG_REG_CMN_WP3_LSM_REM         (SDBG_REG_CMN_BASE | 0xB0)
#define SDBG_REG_CMN_WP3_VRF_CRF_REM     (SDBG_REG_CMN_BASE | 0xB4)



/*
 * Dual wave pool register
 */
#ifndef MARCH_VER

#define SDBG_REG_DWP_ADVDBG               (SDBG_REG_DWP_BASE | 0x20)
#define SDBG_REG_DWP_ERR_DETECT           (SDBG_REG_DWP_BASE | 0x14)
#define SDBG_REG_DWP_VPU_ERR_INFO         (SDBG_REG_DWP_BASE | 0x18)
#define SDBG_REG_DWP_VPRF_ERR_INFO        (SDBG_REG_DWP_BASE | 0x1C)

#define SDBG_REG_DWP_WAVE_INFO            (SDBG_REG_DWP_BASE | 0x24)

#define SDBG_REG_DWP_VPRF_CTRL            (SDBG_REG_DWP_BASE | 0x28)
#define SDBG_REG_DWP_VPRF_WRDATA          (SDBG_REG_DWP_BASE | 0x2C)
#define SDBG_REG_DWP_VPRF_RDDATA          (SDBG_REG_DWP_BASE | 0x30)

#define SDBG_REG_DWP_LSM_CTRL             (SDBG_REG_DWP_BASE | 0x34)
#define SDBG_REG_DWP_LSM_WRDATA           (SDBG_REG_DWP_BASE | 0x38)
#define SDBG_REG_DWP_LSM_RDDATA           (SDBG_REG_DWP_BASE | 0x3C)

#define SDBG_REG_DWP_EU_STATUS_A_SET      (SDBG_REG_DWP_BASE | 0x40)
#define SDBG_REG_DWP_EU_STATUS_A_GET      (SDBG_REG_DWP_BASE | 0x44)
#define SDBG_REG_DWP_EU_STATUS_B_SET      (SDBG_REG_DWP_BASE | 0x48)
#define SDBG_REG_DWP_EU_STATUS_B_GET      (SDBG_REG_DWP_BASE | 0x4C)

#define SDBG_REG_DWP_STATUS_REPORT0       (SDBG_REG_DWP_BASE | 0x58)
#define SDBG_REG_DWP_STATUS_REPORT1       (SDBG_REG_DWP_BASE | 0x5C)
#define SDBG_REG_DWP_STATUS_REPORT2       (SDBG_REG_DWP_BASE | 0x60)
#define SDBG_REG_DWP_STATUS_REPORT3       (SDBG_REG_DWP_BASE | 0x64)

#else

#define SDBG_REG_DWP_ADVDBG               (SDBG_REG_DWP_BASE | 0x1C)
#define SDBG_REG_DWP_ERR_DETECT           (SDBG_REG_DWP_BASE | 0x10)
#define SDBG_REG_DWP_VPU_ERR_INFO         (SDBG_REG_DWP_BASE | 0x14)
#define SDBG_REG_DWP_VPRF_ERR_INFO        (SDBG_REG_DWP_BASE | 0x18)

#define SDBG_REG_DWP_VPRF_CTRL            (SDBG_REG_DWP_BASE | 0x2C)
#define SDBG_REG_DWP_VPRF_WRDATA          (SDBG_REG_DWP_BASE | 0x30)
#define SDBG_REG_DWP_VPRF_RDDATA          (SDBG_REG_DWP_BASE | 0x34)

#define SDBG_REG_DWP_WAVE_INFO            (SDBG_REG_DWP_BASE | 0x20)
#define SDBG_REG_DWP_WAVE_DATA_LOW        (SDBG_REG_DWP_BASE | 0x24)
#define SDBG_REG_DWP_WAVE_DATA_HIGH       (SDBG_REG_DWP_BASE | 0x28)

#define SDBG_REG_DWP_LSM                  (SDBG_REG_DWP_BASE | 0x38)

#endif /* ifndef MARCH_VER */

#endif /* ifndef _GPAD_SDBG_REG_H */
