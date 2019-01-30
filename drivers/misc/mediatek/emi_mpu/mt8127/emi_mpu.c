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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/highmem.h>

#include <linux/timer.h>
#include <linux/workqueue.h>
#include "mach/emi_mpu.h"
#include <mt-plat/mt_device_apc.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/dma.h>

#ifdef CONFIG_MTK_IN_HOUSE_TEE_SUPPORT
#include "trustzone/kree/system.h"
#include "trustzone/tz_cross/trustzone.h"
#include "trustzone/tz_cross/ta_emi.h"
#endif

#ifdef CONFIG_MTK_AEE_FEATURE
#include <mt-plat/aee.h>
#endif

#define MAX_EMI_MPU_STORE_CMD_LEN 128
#define AXI_VIO_MONITOR_TIME    (1 * HZ)
#define DRV_NAME "emi_mpu_ctrl"

/*ver1 for 8127*/
static const char *smi_larb0_port_ver1[10] = {"disp_ovl_0", "disp_rdma1", "disp_rdma",
		"disp_wdma", "mm_cmdq", "mdp_rdma", "mdp_wdma",
		"mdp_roto", "mdp_rotco", "mdp_rotvo"};

static const char *smi_larb1_port_ver1[7] =  {"hw_vdec_mc_ext", "hw_vdec_pp_ext", "hw_vdec_avc_mv_ext",
		"hw_vdec_pred_rd_ext", "hw_vdec_pred_wr_ext",
		"hw_vdec_vld_ext", "hw_vdec_pp_int" };

static const char *smi_larb2_port_ver1[17] = {"cam_imgo", "cam_img2o", "cam_lsci", "cam_imgi",
		"cam_esfko", "cam_aao", "jpgenc_rdma", "jpgenc_bsdma",
		"venc_rd_comv", "venc_sv_comv", "venc_rcpu",
		"venc_rec_frm", "venc_ref_luma", "venc_ref_chroma",
		"venc_bsdma", "venc_cur_luma", "venc_cur_chroma"};

/*ver2 for 2701 & 7623*/
static const char *smi_larb0_port_ver2[11] = {"disp_ovl_0", "disp_rdma1", "disp_rdma",
		"disp_wdma", "mm_cmdq", "mdp_rdma", "mdp_wdma",
		"mdp_roto", "mdp_rotco", "mdp_rotvo", "mdp_rdma1"};

static const char *smi_larb1_port_ver2[10] =  {"hw_vdec_mc_ext", "hw_vdec_pp_ext", "hw_vdec_ppwrap_etx",
		"hw_vdec_avc_mv_ext", "hw_vdec_pred_rd_ext", "hw_vdec_pred_wr_ext",
		"hw_vdec_vld_ext", "hw_vdec_vld2_ext", "hw_vdec_tile_ext", "hw_img_resz_ext"};

static const char *smi_larb2_port_ver2[23] = {"venc_rcpu", "venc_rec_frm", "venc_bsdma", "jpgenc_rdma",
		"venc_lt_rcpu", "venc_lt_rec_frm", "venc_lt_bsdma", "jpgdec_bsdma",
		"venc_sv_comv", "venc_rd_comv", "jpgenc_bsdma",
		"venc_cur_luma", "venc_cur_chroma", "venc_ref_luma",
		"venc_ref_chroma", "img_resz", "venc_lt_sv_comv", "venc_lt_rd_comv", "venc_lt_cur_luma",
		"venc_lt_cur_chroma", "venc_lt_ref_luma", "venc_lt_ref_chroma", "jpgdec_wdma"};


struct mt_emi_mpu_of {
	int emi_mpu_irq;
	void __iomem *emi_mpu_regs;
	const struct smi_port_name *port_name;
};

struct smi_port_name {
	const char **smi_larb0;
	const char **smi_larb1;
	const char **smi_larb2;
	int larb0_number;
	int larb1_number;
	int larb2_number;
};

static const struct smi_port_name mt8127_compat = {
	.smi_larb0 = smi_larb0_port_ver1,
	.smi_larb1 = smi_larb1_port_ver1,
	.smi_larb2 = smi_larb2_port_ver1,
	.larb0_number = ARRAY_SIZE(smi_larb0_port_ver1),
	.larb1_number = ARRAY_SIZE(smi_larb1_port_ver1),
	.larb2_number = ARRAY_SIZE(smi_larb2_port_ver1),
};

static const struct smi_port_name mt7623_compat = {
	.smi_larb0 = smi_larb0_port_ver2,
	.smi_larb1 = smi_larb1_port_ver2,
	.smi_larb2 = smi_larb2_port_ver2,
	.larb0_number = ARRAY_SIZE(smi_larb0_port_ver2),
	.larb1_number = ARRAY_SIZE(smi_larb1_port_ver2),
	.larb2_number = ARRAY_SIZE(smi_larb2_port_ver2),
};

static const struct of_device_id emi_mpu_of_match[] = {
	{ .compatible = "mediatek,mt2701-emi", .data = &mt7623_compat },
	{ .compatible = "mediatek,mt8127-emi", .data = &mt8127_compat },
	{ .compatible = "mediatek,mt7623-emi", .data = &mt7623_compat },
	{}
};

static struct mt_emi_mpu_of emi_mpu_of;
static struct work_struct emi_mpu_work;
static struct workqueue_struct *emi_mpu_workqueue;
static unsigned int vio_addr;

struct mst_tbl_entry {
	u32 master;
	u32 port;
	u32 id_mask;
	u32 id_val;
	char *name;
};

static const struct mst_tbl_entry mst_tbl[] = {
    /* apmcu */
	{ .master = MST_ID_APMCU_0, .port = 0x0, .id_mask = 0x00fc,
		.id_val = 0x0000, .name = "APMCU: Processor Non-Cacheable or STREX" },
	{ .master = MST_ID_APMCU_1, .port = 0x0, .id_mask = 0x00fc,
		.id_val = 0x0004, .name = "APMCU: Processor write to device and Strongly_ordered memory" },
	{ .master = MST_ID_APMCU_2, .port = 0x0, .id_mask = 0x00fc,
		.id_val = 0x0008, .name = "APMCU: Processor write portion of the barrier transactions" },
	{ .master = MST_ID_APMCU_3, .port = 0x0, .id_mask = 0x00ff,
		.id_val = 0x000f,
		.name = "APMCU: Write portion of barrier caused by external DVM synchronization" },
	{ .master = MST_ID_APMCU_4, .port = 0x0, .id_mask = 0x00f0,
		.id_val = 0x0010, .name = "APMCU: Write to cacheable memory from write address buffer" },

    /* MM */
	{ .master = MST_ID_MM_0, .port = 0x1, .id_mask = 0x00e0,
		.id_val = 0x0060, .name = "Larb0 MM Master, MMSYS" },
	{ .master = MST_ID_MM_1, .port = 0x1, .id_mask = 0x00e0,
		.id_val = 0x0040, .name = "Larb1 MM Master, VDEC" },
	{ .master = MST_ID_MM_2, .port = 0x1, .id_mask = 0x00e0,
		.id_val = 0x0020, .name = "Larb2 MM Master, ISP+VENC" },
	{ .master = MST_ID_MM_3, .port = 0x1, .id_mask = 0x00e0,
		.id_val = 0x0000, .name = "G3D Master" },
	{ .master = MST_ID_MM_4, .port = 0x1, .id_mask = 0x00ff,
		.id_val = 0x00fd, .name = "M4U" },
	{ .master = MST_ID_MM_5, .port = 0x1, .id_mask = 0x00ff,
		.id_val = 0x00fe, .name = "M4U" },
	{ .master = MST_ID_MM_6, .port = 0x1, .id_mask = 0x00ff,
		.id_val = 0x00fc, .name = "M4U" },

    /* Periperal */
	{ .master = MST_ID_PERI_0, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x0000, .name = "NFI" },
	{ .master = MST_ID_PERI_1, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x0008, .name = "MSDC0" },
	{ .master = MST_ID_PERI_2, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x0010, .name = "Audio" },
	{ .master = MST_ID_PERI_3, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x0018, .name = "USB2.0" },
	{ .master = MST_ID_PERI_4, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x0002, .name = "PWM" },
	{ .master = MST_ID_PERI_5, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x000a, .name = "MSDC1" },
	{ .master = MST_ID_PERI_6, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x0012, .name = "MSDC2" },
	{ .master = MST_ID_PERI_7, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x001a, .name = "SPI0" },
	{ .master = MST_ID_PERI_8, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x0004, .name = "MD" },
	{ .master = MST_ID_PERI_9, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x000c, .name = "SPM,FHCTL" },
	{ .master = MST_ID_PERI_10, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x0014, .name = "DBG" },
	{ .master = MST_ID_PERI_11, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x001c, .name = "THERM" },
	{ .master = MST_ID_PERI_12, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x0006, .name = "DMA" },
	{ .master = MST_ID_PERI_13, .port = 0x2, .id_mask = 0x00ff,
		.id_val = 0x0001, .name = "Conn2AP" },

    /* Modem */
	{ .master = MST_ID_MDMCU_0, .port = 0x3, .id_mask = 0x0000,
		.id_val = 0x0000, .name = "MDMCU" },

    /* Modem HW (2G/3G) */
	{ .master = MST_ID_MDHW_0, .port = 0x4, .id_mask = 0x0000,
		.id_val = 0x0000, .name = "MDHW" },
};

static const char *UNKNOWN_MASTER = "unknown";
static spinlock_t emi_mpu_lock;

#ifdef CONFIG_MTK_IN_HOUSE_TEE_SUPPORT

static int __match_id(u32 axi_id, int tbl_idx, u32 port_ID)
{
	u32 mm_larb;
	u32 smi_port;

	if (((axi_id & mst_tbl[tbl_idx].id_mask) == mst_tbl[tbl_idx].id_val)
		&& (port_ID == mst_tbl[tbl_idx].port)) {
		switch (port_ID) {
		case 0: /* ARM */
		case 2: /* Peripheral */
		case 3: /* MD */
		case 4: /* MD HW (2G/3G) */
			pr_info("Violation master name is %s.\n", mst_tbl[tbl_idx].name);
			break;
		case 1: /* MM */
			mm_larb = axi_id>>5;
			smi_port = axi_id & 0x1f;
			if (mm_larb == 0x3) {
				if (smi_port >= 10) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n", mst_tbl[tbl_idx].name,
						*(emi_mpu_of.port_name->smi_larb0 + smi_port));
			} else if (mm_larb == 0x2) {
				if (smi_port >= 10) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n", mst_tbl[tbl_idx].name,
						*(emi_mpu_of.port_name->smi_larb1 + smi_port));
			} else if (mm_larb == 0x1) {
				if (smi_port >= 10) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n", mst_tbl[tbl_idx].name,
						*(emi_mpu_of.port_name->smi_larb2 + smi_port));
			} else {
				pr_err("Violation master name is %s.\n", mst_tbl[tbl_idx].name);
			}
			break;
		default:
			pr_err("[EMI MPU ERROR] Invalidate port ID! lookup bus ID table failed!\n");
			break;
		}
	return 1;
	}
	return 0;
}

static char *__id2name(u32 id)
{
	int i;
	u32 axi_ID;
	u32 port_ID;

	axi_ID = (id >> 3) & 0x000000FF;
	port_ID = id & 0x00000007;

	pr_err("[EMI MPU] axi_id = %x, port_id = %x\n", axi_ID, port_ID);

	for (i = 0; i < ARRAY_SIZE(mst_tbl); i++) {
		if (__match_id(axi_ID, i, port_ID))
			return mst_tbl[i].name;
	}
	return (char *)UNKNOWN_MASTER;
}


/*EMI MPU violation handler : get EMI MPU violation information from TZ*/
static irqreturn_t mpu_violation_irq(int irq, void *dev_id)
{
	u32 dbg_s, dbg_t, dbg_pqry;
	u32 master_ID, domain_ID, wr_vio;
	s32 region;
	char *master_name;

	KREE_SESSION_HANDLE emi_session;
	MTEEC_PARAM param[4];
	TZ_RESULT ret;

	ret = KREE_CreateSession(TZ_TA_EMI_UUID, &emi_session);
	if (ret != TZ_RESULT_SUCCESS)
		return -1;

	ret = KREE_TeeServiceCall(emi_session, TZCMD_EMI_REG,
				TZ_ParamTypes4(TZPT_VALUE_OUTPUT, TZPT_VALUE_OUTPUT,
				TZPT_VALUE_OUTPUT, TZPT_VALUE_OUTPUT), param);

	dbg_s = (uint32_t)(param[1].value.a);
	dbg_t = (uint32_t)(param[1].value.b);

	if (dbg_s == 0) {
		pr_err("It's not a MPU violation.\n");
		return IRQ_NONE;
	}

	master_ID = dbg_s & 0x000007FF;
	domain_ID = (dbg_s >> 12) & 0x00000003;
	wr_vio = (dbg_s >> 28) & 0x00000003;
	region = (dbg_s >> 16) & 0xFF;

	switch (domain_ID) {
	case 0:
		dbg_pqry = (uint32_t)(param[2].value.a);
		break;
	case 1:
		dbg_pqry = (uint32_t)(param[2].value.b);
		break;
	case 2:
		dbg_pqry = (uint32_t)(param[3].value.a);
		break;
	case 3:
		dbg_pqry = (uint32_t)(param[3].value.b);
		break;
	default:
		dbg_pqry = 0;
		break;
	}

	ret = KREE_CloseSession(emi_session);
	if (ret != TZ_RESULT_SUCCESS)
		return -1;

	/*TBD: print the violation information*/
	pr_err("EMI MPU violation.\n");
	pr_err("[EMI MPU] Debug info start ----------------------------------------\n");
	pr_err("EMI_MPUS = %x, EMI_MPUT = %x.\n", dbg_s, dbg_t);
	pr_err("Current process is \"%s \" (pid: %i).\n", current->comm, current->pid);
	pr_err("Violation address is 0x%x.\n", dbg_t + EMI_PHY_OFFSET);
	pr_err("Violation master ID is 0x%x.\n", master_ID);

	/*print out the murderer name*/
	master_name = __id2name(master_ID);
	pr_err("Violation domain ID is 0x%x.\n", domain_ID);
	pr_err("%s violation.\n", (wr_vio == 1) ? "Write" : "Read");
	pr_err("Corrupted region is %d\n\r", region);
	if (dbg_pqry & OOR_VIO)
		pr_err("Out of range violation.\n");
	pr_err("[EMI MPU] Debug info end------------------------------------------\n");

#ifdef CONFIG_MTK_AEE_FEATURE
	aee_kernel_exception("EMI MPU",
				"EMI MPU violation.\nEMP_MPUS = 0x%x, EMI_MPUT = 0x%x, module is %s.\n",
				dbg_s, dbg_t, master_name);
#endif

	vio_addr = dbg_t + EMI_PHY_OFFSET;

	return IRQ_HANDLED;
}

#endif

/*
 * emi_mpu_set_region_protection: protect a region.
 * @start: start address of the region
 * @end: end address of the region
 * @region: EMI MPU region id
 * @access_permission: EMI MPU access permission
 * Return 0 for success, otherwise negative status code.
 */
int emi_mpu_set_region_protection(unsigned int start, unsigned int end, int region, unsigned int access_permission)
{
	int ret = 0;
	unsigned int tmp;
	unsigned long flags;

	if ((end != 0) || (start != 0)) {
		/*Address 64KB alignment*/
		start -= EMI_PHY_OFFSET;
		end -= EMI_PHY_OFFSET;
		start = start >> 16;
		end = end >> 16;

		if (end <= start)
			return -EINVAL;
	}

	spin_lock_irqsave(&emi_mpu_lock, flags);

	switch (region) {
	case 0:
		/*Marcos: Clear access right before setting MPU address (Mt6582 design)*/
		tmp = readl(IOMEM(EMI_MPUI)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUI);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUA);
		mt_reg_sync_writel(tmp | access_permission, EMI_MPUI);
		break;

	case 1:
		/*Marcos: Clear access right before setting MPU address (Mt6582 design)*/
		tmp = readl(IOMEM(EMI_MPUI)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUI);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUB);
		mt_reg_sync_writel(tmp | (access_permission << 16), EMI_MPUI);
		break;

	case 2:
		/*Marcos: Clear access right before setting MPU address (Mt6582 design)*/
		tmp = readl(IOMEM(EMI_MPUJ)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUJ);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUC);
		mt_reg_sync_writel(tmp | access_permission, EMI_MPUJ);
		break;

	case 3:
		/*Marcos: Clear access right before setting MPU address (Mt6582 design)*/
		tmp = readl(IOMEM(EMI_MPUJ)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUJ);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUD);
		mt_reg_sync_writel(tmp | (access_permission << 16), EMI_MPUJ);
		break;

	case 4:
		/*Marcos: Clear access right before setting MPU address (Mt6582 design)*/
		tmp = readl(IOMEM(EMI_MPUK)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUK);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUE);
		mt_reg_sync_writel(tmp | access_permission, EMI_MPUK);
		break;

	case 5:
		/*Marcos: Clear access right before setting MPU address (Mt6582 design)*/
		tmp = readl(IOMEM(EMI_MPUK)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUK);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUF);
		mt_reg_sync_writel(tmp | (access_permission << 16), EMI_MPUK);
		break;

	case 6:
		/*Marcos: Clear access right before setting MPU address (Mt6582 design)*/
		tmp = readl(IOMEM(EMI_MPUL)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUL);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUG);
		mt_reg_sync_writel(tmp | access_permission, EMI_MPUL);
		break;

	case 7:
		/*Marcos: Clear access right before setting MPU address (Mt6582 design)*/
		tmp = readl(IOMEM(EMI_MPUL)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUL);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUH);
		mt_reg_sync_writel(tmp | (access_permission << 16), EMI_MPUL);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock_irqrestore(&emi_mpu_lock, flags);

	return ret;
}
EXPORT_SYMBOL(emi_mpu_set_region_protection);


static ssize_t emi_mpu_show(struct device_driver *driver, char *buf)
{
	char *ptr = buf;
	unsigned int start, end;
	unsigned int reg_value;
	unsigned int d0, d1, d2, d3;
	static const char *permission[6] = {
		"No protect",
		"Only R/W for secure access",
		"Only R/W for secure access, and non-secure read access",
		"Only R/W for secure access, and non-secure write access",
		"Only R for secure/non-secure",
		"Both R/W are forbidden"
	};

	reg_value = readl(IOMEM(EMI_MPUA));
	start = ((reg_value >> 16) << 16) + EMI_PHY_OFFSET;
	end = ((reg_value & 0xFFFF) << 16) + EMI_PHY_OFFSET;
	ptr += sprintf(ptr, "Region 0 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUB));
	start = ((reg_value >> 16) << 16) + EMI_PHY_OFFSET;
	end = ((reg_value & 0xFFFF) << 16) + EMI_PHY_OFFSET;
	ptr += sprintf(ptr, "Region 1 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUC));
	start = ((reg_value >> 16) << 16) + EMI_PHY_OFFSET;
	end = ((reg_value & 0xFFFF) << 16) + EMI_PHY_OFFSET;
	ptr += sprintf(ptr, "Region 2 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUD));
	start = ((reg_value >> 16) << 16) + EMI_PHY_OFFSET;
	end = ((reg_value & 0xFFFF) << 16) + EMI_PHY_OFFSET;
	ptr += sprintf(ptr, "Region 3 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUE));
	start = ((reg_value >> 16) << 16) + EMI_PHY_OFFSET;
	end = ((reg_value & 0xFFFF) << 16) + EMI_PHY_OFFSET;
	ptr += sprintf(ptr, "Region 4 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUF));
	start = ((reg_value >> 16) << 16) + EMI_PHY_OFFSET;
	end = ((reg_value & 0xFFFF) << 16) + EMI_PHY_OFFSET;
	ptr += sprintf(ptr, "Region 5 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUG));
	start = ((reg_value >> 16) << 16) + EMI_PHY_OFFSET;
	end = ((reg_value & 0xFFFF) << 16) + EMI_PHY_OFFSET;
	ptr += sprintf(ptr, "Region 6 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUH));
	start = ((reg_value >> 16) << 16) + EMI_PHY_OFFSET;
	end = ((reg_value & 0xFFFF) << 16) + EMI_PHY_OFFSET;
	ptr += sprintf(ptr, "Region 7 --> 0x%x to 0x%x\n", start, end);

	ptr += sprintf(ptr, "\n");

	reg_value = readl(IOMEM(EMI_MPUI));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 0 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
					permission[d0],  permission[d1],  permission[d2], permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 1 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
					permission[d0],  permission[d1],  permission[d2], permission[d3]);

	reg_value = readl(IOMEM(EMI_MPUJ));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 2 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
					permission[d0],  permission[d1],  permission[d2], permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 3 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
					permission[d0],  permission[d1],  permission[d2], permission[d3]);

	reg_value = readl(IOMEM(EMI_MPUK));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 4 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
					permission[d0],  permission[d1],  permission[d2], permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 5 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
					permission[d0],  permission[d1],  permission[d2], permission[d3]);

	reg_value = readl(IOMEM(EMI_MPUL));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 6 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
					permission[d0],  permission[d1],  permission[d2], permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 7 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
					permission[d0],  permission[d1],  permission[d2], permission[d3]);

	return strlen(buf);
}

static ssize_t emi_mpu_store(struct device_driver *driver, const char *buf, size_t count)
{
	int i, ret;
	unsigned int start_addr;
	unsigned int end_addr;
	unsigned int region;
	unsigned int access_permission;
	char *command;
	char *ptr;
	char *token[5];

	ret = 0;
	if ((strlen(buf) + 1) > MAX_EMI_MPU_STORE_CMD_LEN) {
		pr_err("emi_mpu_store command overflow.");
		return count;
	}
	pr_info("emi_mpu_store: %s\n", buf);

	command = kmalloc((size_t)MAX_EMI_MPU_STORE_CMD_LEN, GFP_KERNEL);
	if (!command)
		return count;

	strcpy(command, buf);
	ptr = (char *)buf;

	if (!strncmp(buf, EN_MPU_STR, strlen(EN_MPU_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_info("token[%d] = %s\n", i, token[i]);
			i++;
		}
		ret += kstrtoul(token[1], 16, (unsigned long *) &start_addr);
		ret += kstrtoul(token[2], 16, (unsigned long *) &end_addr);
		ret += kstrtoul(token[3], 16, (unsigned long *) &region);
		ret += kstrtoul(token[4], 16, (unsigned long *) &access_permission);
		if (ret)
			pr_err("Fail to parse cmd.\n");

		emi_mpu_set_region_protection(start_addr, end_addr, region, access_permission);
		pr_info("Set EMI_MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%x.\n",
				start_addr, end_addr, region, access_permission);
	} else if (!strncmp(buf, DIS_MPU_STR, strlen(DIS_MPU_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_info("token[%d] = %s\n", i, token[i]);
			i++;
		}
		ret = kstrtoul(token[3], 16, (unsigned long *) &region);
		if (ret)
			pr_err("Fail to parse cmd.\n");

		emi_mpu_set_region_protection(0x0, 0x0, region,
									SET_ACCESS_PERMISSON(NO_PROTECTION,
									NO_PROTECTION, NO_PROTECTION, NO_PROTECTION));
		pr_info("set EMI MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%x\n",
				0, 0, region, SET_ACCESS_PERMISSON(NO_PROTECTION, NO_PROTECTION,
									NO_PROTECTION, NO_PROTECTION));
	} else {
		pr_err("Unknown emi_mpu command.\n");
	}

	kfree(command);

	return count;
}

DRIVER_ATTR(mpu_config, 0644, emi_mpu_show, emi_mpu_store);

void mtk_search_full_pgtab(void)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	unsigned long addr;
#ifndef CONFIG_ARM_LPAE
	pte_t *pte;
	unsigned long addr_2nd, addr_2nd_end;
#endif
	unsigned int v_addr = vio_addr;

	for (addr = 0xC0000000; addr < 0xFFF00000; addr += 0x100000) {
		pgd = pgd_offset(&init_mm, addr);
		if (pgd_none(*pgd) || !pgd_present(*pgd))
			continue;

		pud = pud_offset(pgd, addr);
		if (pud_none(*pud) || !pud_present(*pud))
			continue;

		pmd = pmd_offset(pud, addr);
		if (pmd_none(*pmd) || !pmd_present(*pmd))
			continue;

#ifndef CONFIG_ARM_LPAE
		if ((pmd_val(*pmd) & PMD_TYPE_MASK) == PMD_TYPE_TABLE) {
			addr_2nd = addr;
			addr_2nd_end = addr_2nd + 0x100000;
			for (; addr_2nd < (addr_2nd_end); addr_2nd += 0x1000) {
				pte = pte_offset_map(pmd, addr_2nd);
				if (((unsigned long)v_addr & PAGE_MASK) ==
					((unsigned long)pte_val(*(pte)) & PAGE_MASK)) {
					pr_info("[EMI MPU] Find page entry section at pte: %lx. violation address = 0x%x\n",
							(unsigned long)(pte), v_addr);
					return;
				}
			}
		} else if (((unsigned long)pmd_val(*(pmd)) & SECTION_MASK) ==
					((unsigned long)v_addr & SECTION_MASK))
					return;
#else
	/* TBD */
#endif
	}
	pr_err("[EMI MPU] ****** Can not find page table entry! violation address = 0x%x ******\n", v_addr);
}

void emi_mpu_work_callback(struct work_struct *work)
{
	pr_info("[EMI MPU] Enter EMI MPU workqueue!\n");
	mtk_search_full_pgtab();
	pr_info("[EMI MPU] Exit EMI MPU workqueue!\n");
}

static ssize_t pgt_scan_show(struct device_driver *driver, char *buf)
{
	return 0;
}

static ssize_t pgt_scan_store(struct device_driver *driver, const char *buf, size_t count)
{
	unsigned int value;
	unsigned int ret;

	if (unlikely(kstrtoint(buf, 0, &value) != 1))
		return -EINVAL;

	if (value == 1) {
		ret = queue_work(emi_mpu_workqueue, &emi_mpu_work);
		if (!ret)
			pr_err("[EMI MPU] submit workqueue failed, ret = %d\n", ret);
	}

	return count;
}
DRIVER_ATTR(pgt_scan, 0644, pgt_scan_show, pgt_scan_store);


static int emi_mpu_probe(struct platform_device *dev)
{
	const struct of_device_id *of_id;

	/* Setup IRQ numbers */
	emi_mpu_of.emi_mpu_irq = irq_of_parse_and_map(dev->dev.of_node, 0);

	/* Setup IO addresses */
	emi_mpu_of.emi_mpu_regs = of_iomap(dev->dev.of_node, 0);

	/*Get compatible data*/
	of_id = of_match_node(emi_mpu_of_match, dev->dev.of_node);
	if (!of_id)
		return -EINVAL;

	emi_mpu_of.port_name = of_id->data;

	return 0;
}


static struct platform_driver emi_mpu_driver = {
	.probe = emi_mpu_probe,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = emi_mpu_of_match,
	},
};

static int __init emi_mpu_mod_init(void)
{
	int ret;

	ret = platform_driver_register(&emi_mpu_driver);
	if (ret) {
		pr_err("[EMI MPU] Unable to register driver (%d)\n", ret);
		return ret;
	}

	spin_lock_init(&emi_mpu_lock);

	/** NoteXXX: Interrupts of vilation (including SPC in SMI, or EMI MPU)
	*are triggered by the device APC.
	*Need to share the interrupt with the SPC driver.*/

#ifdef CONFIG_MTK_IN_HOUSE_TEE_SUPPORT
	ret = request_irq(emi_mpu_of.emi_mpu_irq, (irq_handler_t)mpu_violation_irq,
		IRQF_TRIGGER_LOW | IRQF_SHARED, "mt_emi_mpu", &emi_mpu_driver);
	if (ret != 0) {
		pr_err("Fail to request EMI_MPU interrupt. Error = %d.\n", ret);
		return ret;
	}
#endif

#if !defined(USER_BUILD_KERNEL)
	ret = driver_create_file(&emi_mpu_driver.driver, &driver_attr_mpu_config);
	if (ret)
		pr_err("Fail to create MPU config sysfs file.\n");
#endif

	/* Create a workqueue to search pagetable entry */
	emi_mpu_workqueue = create_singlethread_workqueue("emi_mpu");
	INIT_WORK(&emi_mpu_work, emi_mpu_work_callback);

	return 0;
}




static void __exit emi_mpu_mod_exit(void)
{
}

module_init(emi_mpu_mod_init);
module_exit(emi_mpu_mod_exit);
