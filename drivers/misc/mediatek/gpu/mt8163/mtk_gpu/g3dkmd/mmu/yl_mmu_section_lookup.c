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


/* #include <assert.h> */

#include "yl_mmu_system_address_translator.h"
#include "yl_mmu_section_lookup.h"

void PaSectionLookupTable_set_invalid(struct PaSectionLookupTable *self)
{
	self->is_valid = 0;
}

void PaSectionLookupTable_set_valid(struct PaSectionLookupTable *self)
{
	self->is_valid = 1;
}

pa_t PaSectionLookupTable_use_if_availabile(struct Mapper *m, struct PaSectionLookupTable *self, int index1,
					    va_t va_base)
{
	return (self->is_valid) ? self->entries[index1] : va_to_pa(m, va_base);
}
