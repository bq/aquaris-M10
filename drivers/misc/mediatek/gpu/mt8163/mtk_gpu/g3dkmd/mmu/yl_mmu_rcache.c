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

#include "yl_mmu_rcache.h"

#include "yl_mmu_kernel_alloc.h"


void Rcache_Init(struct Rcache *self)
{
	int i;

	yl_mmu_zeromem((void *)self, sizeof(struct Rcache));

	for (i = 1; i < REMAPPER_RCACHE_ITEM_LENGTH; ++i) {
		/* self->items[ i ].q_prev = (Rcache_link_t)( i-1 ); */
		self->items[i].q_next = (Rcache_link_t) (i + 1);
	}

	self->empty_queue.head = (Rcache_link_t) 1;
	self->empty_queue.tail = (Rcache_link_t) (REMAPPER_RCACHE_ITEM_LENGTH - 1);
	self->items[REMAPPER_RCACHE_ITEM_LENGTH - 1].q_next = (Rcache_link_t) 0;

	self->used_count = 0;
}

void Rcache_Uninit(struct Rcache *self)
{
	yl_mmu_zeromem((void *)self, sizeof(struct Rcache));
}
