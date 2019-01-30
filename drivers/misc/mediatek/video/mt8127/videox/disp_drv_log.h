/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __DISP_DRV_LOG_H__
#define __DISP_DRV_LOG_H__

extern unsigned int disp_log_level;

#define DISPLOG_ALL		(0xFFFF)
#define DISPLOG_MSG		(1<<0)
#define DISPLOG_TRACE	(1<<1)
#define DISPLOG_MTKFB	(1<<2)
#define DISPLOG_HAL		(1<<3)
#define DISPLOG_DDP		(1<<4)
#define DISPLOG_IRQ		(1<<5)
#define DISPLOG_IRQERR	(1<<6)
#define DISPLOG_OVLENG	(1<<7)
#define DISPLOG_FENCE	(1<<8)

#define DISP_MSG(fmt, args...) \
	do { \
		if (disp_log_level & DISPLOG_MSG) \
			pr_notice("[DISP/MSG]"fmt, ##args); \
	} while (0)

#define DISP_TRACE(fmt, args...) \
	do { \
		if (disp_log_level & DISPLOG_TRACE) \
			pr_notice("[DISP/TRACE]%s_%d\n", \
					__fun__, __LINE__); \
	} while (0)

#define DISP_FB(fmt, args...) \
	do { \
		if (disp_log_level & DISPLOG_MTKFB) \
			pr_notice("[DISP/FB]"fmt, ##args); \
	} while (0)

#define DISP_HAL(fmt, args...) \
	do { \
		if (disp_log_level & DISPLOG_HAL) \
			pr_notice("[DISP/HAL]"fmt, ##args); \
	} while (0)

#define DISP_DDP(fmt, args...) \
	do { \
		if (disp_log_level & DISPLOG_DDP) \
			pr_notice("[DISP/DDP]"fmt, ##args); \
	} while (0)

#define DISP_IRQ(fmt, args...) \
	do { \
		if (disp_log_level & DISPLOG_IRQ) \
			pr_notice("[DISP/IRQ]"fmt, ##args); \
	} while (0)

#ifdef CONFIG_ANDROID
#define DISP_IRQERR(fmt) \
	do { \
		if (disp_log_level & DISPLOG_IRQERR) \
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT, "DDP, "fmt, fmt); \
	} while (0)
#else
#define DISP_IRQERR(fmt) \
	do { \
		if (disp_log_level & DISPLOG_IRQERR) \
			pr_err("DDP, "fmt); \
	} while (0)

#endif


#define DISP_OVLENG(fmt, args...) \
	do { \
		if (disp_log_level & DISPLOG_OVLENG) \
			pr_notice("[DISP/OVLENG]"fmt, ##args); \
	} while (0)

#define DISP_FENCE(fmt, args...) \
	do { \
		if (disp_log_level & DISPLOG_FENCE) \
			pr_notice("[DISP/FENCE]"fmt, ##args); \
	} while (0)

#define DISP_DXI(fmt, args...) \
	do { \
		if (disp_log_level & DISPLOG_DXI) \
			pr_notice("[DISP/OVLENG]"fmt, ##args); \
	} while (0)

#define DISP_INFO(fmt, args...) pr_info("[DISP/INFO]"fmt, ##args)
#define DISP_NOTICE(fmt, args...) pr_notice("[DISP/NOTICE]"fmt, ##args)
#define DISP_DBG(fmt, args...) pr_debug("[DISP/DBG]"fmt, ##args)
#define DISP_WARN(fmt, args...) pr_warn("[DISP/WARN]"fmt, ##args)
#define DISP_ERR(fmt, args...) pr_err("[DISP/ERROR]"fmt, ##args)

#define ANDROID_LOG_INFO 0
#define ANDROID_LOG_WARN 1
#define ANDROID_LOG_ERROR 2
#define DISP_LOG_PRINT(level, sub_module, fmt, arg...) \
	do { \
		if (level == ANDROID_LOG_INFO) \
			pr_info("DISP/"sub_module"] "fmt, ##arg); \
		else if (level == ANDROID_LOG_WARN) \
			pr_warn("DISP/"sub_module"] "fmt, ##arg); \
		else if (level == ANDROID_LOG_ERROR) \
			pr_err("DISP/"sub_module"] "fmt, ##arg); \
	} while (0)

#endif				/* __DISP_DRV_LOG_H__ */
