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

#ifdef linux
#if !defined(linux_user_mode)
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#endif
#endif

#include "g3dkmd_define.h"
#include "yl_mmu_system_address_translator.h"
#include "g3dkmd_task.h"
#include "g3dkmd_file_io.h"

#ifdef linux
#if !defined(linux_user_mode)
#define YL_PGD_INVALID_ERROR  ((int)1)
#define YL_PUD_INVALID_ERROR  ((int)2)
#define YL_PMD_INVALID_ERROR  ((int)3)
#define YL_PTE_INVALID_ERROR  ((int)4)
#define YL_PTE_PRESENT_ERROR  ((int)5)

pa_t va_to_pa_after_mmap(va_t address)
{
	pa_t ret = 0;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep = NULL;
	pte_t pte;
	spinlock_t *ptl = (spinlock_t *) NULL;
	int errcode = 0;

	if (gExecInfo.pDMAMemory && address >= (va_t) gExecInfo.pDMAMemory->data0 &&
	    address < ((va_t) gExecInfo.pDMAMemory->data0 + gExecInfo.pDMAMemory->size)) {
		return (pa_t) (address - (va_t) gExecInfo.pDMAMemory->data0 +
			       gExecInfo.pDMAMemory->phyAddr);
	}

	if (address >= VMALLOC_START && address <= VMALLOC_END) {
		/* it's kernel virtual address from vmalloc */
		unsigned int pfn = vmalloc_to_pfn((void *)address);

		return (pfn << PAGE_SHIFT) | (address & ~(PAGE_SIZE - 1));
	}

	if (address > PAGE_OFFSET) {	/* PAGE_OFFSET usually is 0xc0000000, it's a tmp solution */
		/* it's kernel virtual address from kmalloc */
		return (pa_t) __pa((void *)address);
	}
	/* it's user space virtual address */
	pgd = pgd_offset(current->mm, address);
	if (pgd == NULL || pgd_none(*pgd) || unlikely(pgd_bad(*pgd))) {
		errcode = YL_PGD_INVALID_ERROR;
		goto fail;
	}
	pud = pud_offset(pgd, address);
	if (pud == NULL || pud_none(*pud) || unlikely(pud_bad(*pud))) {
		errcode = YL_PUD_INVALID_ERROR;
		goto fail;
	}

	pmd = pmd_offset(pud, address);
	if (pmd == NULL || pmd_none(*pmd) || unlikely(pmd_bad(*pmd))) {
		errcode = YL_PMD_INVALID_ERROR;
		goto fail;
	}

	ptep = pte_offset_map_lock(current->mm, pmd, address, &ptl);
	if (ptep == NULL || pte_none(*ptep) || ptl == NULL) {
		errcode = YL_PTE_INVALID_ERROR;
		goto fail;
	}

	pte = *ptep;

	if (!pte_present(pte)) {
		errcode = YL_PTE_PRESENT_ERROR;
		goto fail;
	}

	ret = (pa_t) ((pte_val(pte) & PAGE_MASK) | (address & ~PAGE_MASK));

fail:
	if (errcode != 0) { /* debug usage */
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_MMU, "%s errcode = %d\n", __func__, errcode);
	}

	if (ptl != NULL)
		pte_unmap_unlock(ptep, ptl);

	return ret;
}
#endif /* linux kernel */
#endif
