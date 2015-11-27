#ifndef __CMDQ_DEVICE_H__
#define __CMDQ_DEVICE_H__

#include <linux/platform_device.h>
#include <linux/device.h>

struct device *cmdq_dev_get(void);
const uint32_t cmdq_dev_get_irq_id(void);
const uint32_t cmdq_dev_get_irq_secure_id(void);
const long cmdq_dev_get_module_base_VA_GCE(void);
const long cmdq_dev_get_module_base_PA_GCE(void);

const long cmdq_dev_get_module_base_VA_MMSYS_CONFIG(void);
const long cmdq_dev_get_module_base_VA_MDP_RDMA(void);
const long cmdq_dev_get_module_base_VA_MDP_RSZ0(void);
const long cmdq_dev_get_module_base_VA_MDP_RSZ1(void);
const long cmdq_dev_get_module_base_VA_MDP_WDMA(void);
const long cmdq_dev_get_module_base_VA_MDP_WROT(void);
const long cmdq_dev_get_module_base_VA_MDP_TDSHP(void);
const long cmdq_dev_get_module_base_VA_MM_MUTEX(void);
const long cmdq_dev_get_module_base_VA_VENC(void);
const long cmdq_dev_get_module_base_VA_DISP_PWM0(void);

const long cmdq_dev_alloc_module_base_VA_by_name(const char *name);
void cmdq_dev_free_module_base_VA(const long VA);

void cmdq_dev_init(struct platform_device *pDevice);
void cmdq_dev_deinit(void);

#define MMSYS_CONFIG_BASE cmdq_dev_get_module_base_VA_MMSYS_CONFIG()
#define MDP_RDMA_BASE     cmdq_dev_get_module_base_VA_MDP_RDMA()
#define MDP_RSZ0_BASE     cmdq_dev_get_module_base_VA_MDP_RSZ0()
#define MDP_RSZ1_BASE     cmdq_dev_get_module_base_VA_MDP_RSZ1()
#define MDP_TDSHP_BASE    cmdq_dev_get_module_base_VA_MDP_TDSHP()
#define MDP_WROT_BASE     cmdq_dev_get_module_base_VA_MDP_WROT()
#define MDP_WDMA_BASE     cmdq_dev_get_module_base_VA_MDP_WDMA()
#define VENC_BASE         cmdq_dev_get_module_base_VA_VENC()
#define DISP_PWM0_VA      cmdq_dev_get_module_base_VA_DISP_PWM0()

/* for compatible only */
#define MMSYS_CONFIG_BASE_VA	MMSYS_CONFIG_BASE
#define MDP_RDMA_BASE_VA		MDP_RDMA_BASE
#define MDP_RSZ0_BASE_VA		MDP_RSZ0_BASE
#define MDP_RSZ1_BASE_VA		MDP_RSZ1_BASE
#define MDP_TDSHP_BASE_VA		MDP_TDSHP_BASE
#define MDP_WROT_BASE_VA		MDP_WROT_BASE
#define MDP_WDMA_BASE_VA		MDP_WDMA_BASE
#define VENC_BASE_VA			VENC_BASE


#define GCE_BASE_PA				cmdq_dev_get_module_base_PA_GCE()
/* #define GCE_BASE_PA                           0x10212000 */
#define MMSYS_CONFIG_BASE_PA	0x14000000
#define MDP_RDMA_BASE_PA		0x14001000
#define MDP_RSZ0_BASE_PA		0x14002000
#define MDP_RSZ1_BASE_PA		0x14003000
#define MDP_WDMA_BASE_PA		0x14004000
#define MDP_WROT_BASE_PA		0x14005000
#define MDP_TDSHP_BASE_PA		0x14006000
#define DISP_PWM0_PA			0x14014000
#define MM_MUTEX_BASE_PA		0x14015000
#define VENC_BASE_PA			0x17002000



#define CMDQ_TEST_DISP_PWM0_DUMMY_PA    (DISP_PWM0_PA + 0x0030)
#define CMDQ_TEST_DISP_PWM0_DUMMY_VA    (DISP_PWM0_VA + 0x0030)


#endif				/* __CMDQ_DEVICE_H__ */
