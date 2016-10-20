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

/* file */
/* brief A temp object to reduce va to pa translation. */


#ifndef _YL_MMU_SECTION_LOOKUP_H_
#define _YL_MMU_SECTION_LOOKUP_H_

#include "yl_mmu_section_lookup_public_class.h"

#include "yl_mmu_common.h"


void PaSectionLookupTable_set_valid(struct PaSectionLookupTable *self);
void PaSectionLookupTable_set_invalid(struct PaSectionLookupTable *self);
pa_t PaSectionLookupTable_use_if_availabile(struct Mapper *m, struct PaSectionLookupTable *self,
						     int index1, va_t va_base);

#endif /* _YL_MMU_iSECTION_LOOKUP_H_ */
