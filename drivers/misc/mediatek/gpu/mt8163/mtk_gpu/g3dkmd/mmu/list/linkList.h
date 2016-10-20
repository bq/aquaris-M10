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

#ifndef _LINKLIST_H_
#define _LINKLIST_H_

#include <stdlib.h>
#include <stdio.h>
#include "..\yl_mmu.h"

struct LNode {
	int index;		/* the index of the allocation */
	size_t size;		/* the size of the allocation */
	va_t va;
	g3d_va_t g3d_va;
	unsigned char *data;
	struct Node *next;

};


void createList(int len, int *arr);
int isListEmpty(void);
void displayList(FILE *file, int option, size_t size, size_t totalSize);
struct LNode *searchNode(int index);
struct LNode *searchNodeByOrder(unsigned long release_index);
void deleteNode(struct LNode *ptr);
void deleteNodeWithoutData(struct LNode *ptr);
void insertNode(struct LNode *ptr);
void insertNodeWithoutData(struct LNode *ptr);

unsigned long getListNum(void);
int getListIndex(int d);
size_t getListIndexSize(int index);
size_t getNodeSize(unsigned long release_index);
struct LNode *getFirstNode(void);
unsigned char intCharMap(int integer);
int checkMem(struct Mapper *m, g3d_va_t g3d_va, unsigned char *data);
void checkMemoryLayout(struct Mapper *m);
g3d_va_t getG3dOfNode(unsigned long release_index);

#endif /* _LINKLIST_H_ */
