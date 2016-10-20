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

#ifndef _GPAD_IPEM_DVFS_H
#define _GPAD_IPEM_DVFS_H
#include <linux/types.h>
#include "gpad_common.h"
#include "gpad_ipem_lib.h"




/*
 * *******************************************************************
 *  macro
 * *******************************************************************
 */




/*
 * *******************************************************************
 *  enumeration
 * *******************************************************************
 */




/*
 * *******************************************************************
 *  function prototype
 * *******************************************************************
 */
extern void gpad_ipem_dvfs_init(struct gpad_device *dev);
extern void gpad_ipem_dvfs_exit(struct gpad_device *dev);
extern gpad_ipem_action gpad_ipem_dvfs_do(struct gpad_ipem_work_info *work_info);
extern void gpad_ipem_dvfs_update(struct gpad_ipem_work_info *work_info);

extern u32 gpad_ipem_dvfs_policy_get(struct gpad_device *dev);
extern void gpad_ipem_dvfs_policy_set(struct gpad_device *dev, u32 policy_id);
extern struct gpad_ipem_dvfs_policy_info *gpad_ipem_dvfs_policies_get(struct gpad_device *dev);




/*
 * *******************************************************************
 *  data structure
 * *******************************************************************
 */
struct gpad_ipem_dvfs_policy_info {
	char                         *name;
	gpad_ipem_policy_init_func    init_func;
	gpad_ipem_policy_exit_func    exit_func;
	gpad_ipem_policy_do_func      do_func;
	gpad_ipem_policy_update_func  update_func;
};

struct gpad_ipem_dvfs_lookup_info {
};

struct gpad_ipem_dvfs_thermal_info {
};

struct gpad_ipem_dvfs_lookahead_info {
};

struct gpad_ipem_dvfs_qlearn_info {
};

struct gpad_ipem_dvfs_sarsa_info {
};

struct gpad_ipem_dvfs_type1_info {
};

struct gpad_ipem_dvfs_type2_info {
};

struct gpad_ipem_dvfs_type4_info {
	u32 gpuload_prev;
	u32 gpuload_next;
};

struct gpad_ipem_dvfs_info {
	struct gpad_ipem_dvfs_policy_info      *policies;
	u32                                     policy_id;
	gpad_ipem_policy_do_func                policy_do;
	gpad_ipem_policy_update_func            policy_update;

	struct gpad_ipem_dvfs_lookup_info       lookup_info;
	struct gpad_ipem_dvfs_thermal_info      thermal_info;
	struct gpad_ipem_dvfs_lookahead_info    lookahead_info;
	struct gpad_ipem_dvfs_qlearn_info       qlearn_info;
	struct gpad_ipem_dvfs_sarsa_info        sarsa_info;
	struct gpad_ipem_dvfs_type1_info        type1_info;
	struct gpad_ipem_dvfs_type2_info        type2_info;
	struct gpad_ipem_dvfs_type4_info        type4_info;
};

#endif /* #define _GPAD_IPEM_DVFS_H */
