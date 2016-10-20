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

#ifndef _GPAD_SDBG_DUMP_H
#define _GPAD_SDBG_DUMP_H

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "gpad_sdbg.h"
#include "gpad_sdbg_reg.h"

/* dump information */
void gpad_sdbg_dump_areg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);
void gpad_sdbg_dump_common_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);
void gpad_sdbg_dump_dualwavepool_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);
void gpad_sdbg_dump_global_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);
void gpad_sdbg_dump_instruction_cache(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);
void gpad_sdbg_dump_wave_info(struct seq_file *m, unsigned int wpid, unsigned int wid, struct gpad_sdbg_info *sdbg_info);
void gpad_dfd_dump_dwp_debug_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);
void gpad_dfd_dump_areg_all_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);
void gpad_dfd_dump_freg_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);
void gpad_dfd_dump_ff_debug_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);
void gpad_dfd_dump_ux_cmn_reg(struct seq_file *m, struct gpad_sdbg_info *sdbg_info);

#endif /* #define __INC_GPAD_SDBG_DUMP_H */
