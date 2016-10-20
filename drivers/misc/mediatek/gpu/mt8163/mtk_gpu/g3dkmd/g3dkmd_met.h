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

#ifndef _G3DKMD_MET_H_
#define _G3DKMD_MET_H_

#define G3DKMD_MET_PER_CPU_VARIABLE

/*
 * In met is built-in in kernel, we undefine MET_USER_EVENT_SUPPORT flag
 * and use the built-ine implementation.
 * Otherwise, we clone a MET implementation to prevent dependency between
 * g3dkmd and met.
 */
#ifdef MET_USER_EVENT_SUPPORT	/* MET is built-in. */
#include <linux/met_drv.h>

#else				/* MET clone by G3D */
#include <linux/version.h>
#include <linux/device.h>
#include <linux/percpu.h>
#include <linux/hardirq.h>

    #ifndef CONFIG_PREEMPT_COUNT
    #define preempt_enable_no_resched()
    #endif
    /*
     * Minimal clone of MET kernel mode tagging API.
     * All of them call to tracing_mark_write() for output the same function name
     * as a signature as MET log.
     */
#define met_tag_start(class_id, name) \
	    tracing_mark_write(TYPE_START, class_id, name, 0)
#define met_tag_end(class_id, name) \
	    tracing_mark_write(TYPE_END, class_id, name, 0)
#define met_tag_oneshot(class_id, name, value) \
	    tracing_mark_write(TYPE_ONESHOT, class_id, name, value)
#define met_tag_async_start(class_id, name, cookie) \
	    tracing_mark_write(TYPE_ASYNC_START, class_id, name, cookie)
#define met_tag_async_end(class_id, name, cookie) \
	    tracing_mark_write(TYPE_ASYNC_END, class_id, name, cookie)

    /*
     * Clone of tracing_mark_write().
     */
#define TYPE_START		1
#define TYPE_END		2
#define TYPE_ONESHOT	3
#define TYPE_ENABLE		4
#define TYPE_DISABLE	5
#define TYPE_REC_SET	6
#define TYPE_DUMP		7
#define TYPE_DUMP_SIZE	8
#define TYPE_DUMP_SAVE	9
#define TYPE_USRDATA	10
#define TYPE_ASYNC_START	11
#define TYPE_ASYNC_END	12

int tracing_mark_write(int type, unsigned int class_id, const char *name, unsigned int value);

    /*
     * Clone of MET_PRINTK().
     * It is a Wrapper to ftrace, and tracing_mark_write() shall depend on it.
     */
#if !defined(G3DKMD_MET_PER_CPU_VARIABLE)
#define MET_PRINTK(FORMAT, args...) trace_printk(FORMAT, ##args)
#else
#define MET_STRBUF_SIZE        512
DECLARE_PER_CPU(char[MET_STRBUF_SIZE], g3dkmd_met_strbuf_nmi);
DECLARE_PER_CPU(char[MET_STRBUF_SIZE], g3dkmd_met_strbuf_irq);
DECLARE_PER_CPU(char[MET_STRBUF_SIZE], g3dkmd_met_strbuf_sirq);
DECLARE_PER_CPU(char[MET_STRBUF_SIZE], g3dkmd_met_strbuf);
#define MET_PRINTK(FORMAT, args...) \
	do { \
		char *pg3dkmd_met_strbuf; \
		preempt_disable(); \
		if (in_nmi()) \
			pg3dkmd_met_strbuf = per_cpu(g3dkmd_met_strbuf_nmi, smp_processor_id()); \
		else if (in_irq()) \
			pg3dkmd_met_strbuf = per_cpu(g3dkmd_met_strbuf_irq, smp_processor_id()); \
		else if (in_softirq()) \
			pg3dkmd_met_strbuf = per_cpu(g3dkmd_met_strbuf_sirq, smp_processor_id()); \
		else \
			pg3dkmd_met_strbuf = per_cpu(g3dkmd_met_strbuf, smp_processor_id()); \
		snprintf(pg3dkmd_met_strbuf, MET_STRBUF_SIZE, FORMAT, ##args); \
		trace_puts(pg3dkmd_met_strbuf); \
		preempt_enable_no_resched(); \
	} while (0)
#endif
#endif /* MET_USER_EVENT_SUPPORT */
#endif /* _G3DKMD_MET_H_ */
