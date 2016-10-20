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

#if defined(linux) && !defined(linux_user_mode)
#include <linux/module.h>
#endif
#include "m4u.h"
#include "g3dkmd_define.h"
#include "g3dkmd_util.h"
#include "g3dkmd_internal_memory.h"
#include "g3dkmd_api_mmu.h"
#include "g3dkmd_cfg.h"
#include "g3dkmd_pattern.h"
#include "g3dkmd_iommu.h"
#include "mmu/yl_mmu_utility.h"
#if defined(linux) && !defined(linux_user_mode)
#include <linux/slab.h>
#ifdef ANDROID
#include <linux/export.h>
#else				/* !ANDROID */
#include <linux/module.h>
#endif /* ANDROID */
#else /* !linux || linux_user_mode */
#include <stdlib.h>
#endif /* linux && !linux_user_mode */

/* enable it always when running emulator with now/ion_m */
/* due to ion module will use it as fake IOMMU if no WCP IOMMU driver */
#if (USE_MMU == MMU_USING_M4U) || (defined(ANDROID) && !defined(G3D_HW))
static __DEFINE_SEM_LOCK(M4U)

G3DKMD_APICALL int G3DAPIENTRY g3d_m4u_alloc_mva(m4u_client_t *client, M4U_PORT_ID port,
						 void *va, struct sg_table *table,
						 unsigned int size, unsigned int prot,
						 unsigned int flags, unsigned int *pMva)
{
	unsigned int yl_flag = 0;

	__SEM_LOCK(M4U);

	yl_flag |= (prot & M4U_PROT_CACHE) ? G3DKMD_MMU_SHARE : 0;
	yl_flag |= (prot & M4U_PROT_SEC) ? G3DKMD_MMU_SECURE : 0;

#if defined(YL_MMU_PATTERN_DUMP_V1) || defined(YL_MMU_PATTERN_DUMP_V2)

#ifdef YL_SECURE
	*pMva = g3dKmdMmuMap_ex((void *)client, va, size, MMU_SECTION_DETECTION, yl_flag, flags);
#else				/* ! YL_SECURE */
	*pMva = g3dKmdMmuMap_ex((void *)client, va, size, MMU_SECTION_DETECTION, yl_flag);
#endif /* YL_SECURE */

#else

#ifdef YL_SECURE
	*pMva = g3dKmdMmuMap_ex(client->mmu, va, size, MMU_SECTION_DETECTION, yl_flag, flags);
#else /* ! YL_SECURE */
	*pMva = g3dKmdMmuMap_ex(client->mmu, va, size, MMU_SECTION_DETECTION, yl_flag);
#endif /* YL_SECURE */

#endif

	__SEM_UNLOCK(M4U);
	return (*pMva == 0) ? 1 : 0;
}

#if defined(linux) && !defined(linux_user_mode) && !defined(FPGA_G3D_HW)
EXPORT_SYMBOL(g3d_m4u_alloc_mva);
#endif

G3DKMD_APICALL int G3DAPIENTRY g3d_m4u_dealloc_mva(m4u_client_t *client, M4U_PORT_ID port,
						   unsigned int mva, unsigned int size)
{
	__SEM_LOCK(M4U);
#if defined(YL_MMU_PATTERN_DUMP_V1) || defined(YL_MMU_PATTERN_DUMP_V2)
	g3dKmdMmuUnmap_ex((void *)client, mva, size);
#else
	g3dKmdMmuUnmap_ex(client->mmu, mva, size);
#endif
	__SEM_UNLOCK(M4U);
	return 0;
}

#if defined(linux) && !defined(linux_user_mode) && !defined(FPGA_G3D_HW)
EXPORT_SYMBOL(g3d_m4u_dealloc_mva);
#endif

G3DKMD_APICALL int G3DAPIENTRY m4u_config_port(struct M4U_PORT *pM4uPort)
{
	return 0;
}

m4u_client_t *g3d_m4u_create_client(void)
{
	m4u_client_t *client;

	__SEM_LOCK(M4U);

#if defined(YL_MMU_PATTERN_DUMP_V1) || defined(YL_MMU_PATTERN_DUMP_V2)
	client = (m4u_client_t *) g3dKmdMmuInit_ex(g3dKmdCfgGet(CFG_MMU_VABASE));
	YL_KMD_ASSERT(client);
#else
	client = G3DKMDVMALLOC(sizeof(m4u_client_t));
	if (client != NULL) {
		client->mmu = g3dKmdMmuInit_ex(g3dKmdCfgGet(CFG_MMU_VABASE));
		YL_KMD_ASSERT(client->mmu);
	}
#endif

	__SEM_UNLOCK(M4U);
	return client;
}

#if defined(linux) && !defined(linux_user_mode) && !defined(FPGA_G3D_HW)
EXPORT_SYMBOL(g3d_m4u_create_client);
#endif

int g3d_m4u_destroy_client(m4u_client_t *client)
{
	if (!client)
		return 1;

	__SEM_LOCK(M4U);

#if defined(YL_MMU_PATTERN_DUMP_V1) || defined(YL_MMU_PATTERN_DUMP_V2)
	g3dKmdMmuUninit_ex(client);
#else
	if (client->mmu)
		g3dKmdMmuUninit_ex(client->mmu);

	G3DKMDVFREE(client);
#endif

	__SEM_UNLOCK(M4U);
	return 0;
}

#if defined(linux) && !defined(linux_user_mode) && !defined(FPGA_G3D_HW)
EXPORT_SYMBOL(g3d_m4u_destroy_client);
#endif

G3DKMD_APICALL unsigned long G3DAPIENTRY m4u_mva_to_pa(m4u_client_t *client, M4U_PORT_ID port,
						       unsigned int mva)
{
	unsigned int rtnVal;

	__SEM_LOCK(M4U);
#if defined(YL_MMU_PATTERN_DUMP_V1) || defined(YL_MMU_PATTERN_DUMP_V2)
	rtnVal = g3dKmdMmuG3dVaToPa_ex((void *)client, mva);
#else
	rtnVal = g3dKmdMmuG3dVaToPa_ex(client->mmu, mva);
#endif
	__SEM_UNLOCK(M4U);

	return (unsigned long)rtnVal;
}

G3DKMD_APICALL unsigned int G3DAPIENTRY m4u_mva_to_va(m4u_client_t *client, M4U_PORT_ID port,
						      unsigned int mva)
{
	unsigned int rtnVal;

	__SEM_LOCK(M4U);
#if defined(YL_MMU_PATTERN_DUMP_V1) || defined(YL_MMU_PATTERN_DUMP_V2)
	rtnVal = g3dKmdMmuG3dVaToVa_ex((void *)client, mva);
#else
	rtnVal = g3dKmdMmuG3dVaToVa_ex(client->mmu, mva);
#endif
	__SEM_UNLOCK(M4U);

	return rtnVal;
}

G3DKMD_APICALL void G3DAPIENTRY m4u_get_pgd(m4u_client_t *client, M4U_PORT_ID port, void **pgd_va,
					    void **pgd_pa, unsigned int *size)
{
	__SEM_LOCK(M4U);
#if defined(YL_MMU_PATTERN_DUMP_V1) || defined(YL_MMU_PATTERN_DUMP_V2)
	g3dKmdMmuGetTable_ex((void *)client, pgd_va, pgd_pa, size);
#else
	g3dKmdMmuGetTable_ex(client->mmu, pgd_va, pgd_pa, size);
#endif
	__SEM_UNLOCK(M4U);
}

G3DKMD_APICALL void G3DAPIENTRY m4u_register_regwirte_callback(m4u_client_t *client,
							       m4u_regwrite_callback_t *fn)
{
	g3dKmdIommuRegCallback(fn);
}
#endif /* (USE_MMU == MMU_USING_M4U) */
