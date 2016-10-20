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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "linkList.h"
#include "../yl_mmu_system_address_translator.h"

struct LNode *firstNode = NULL;

void createList(int len, int *arr)
{
	struct LNode *newNode;
	for(int i = 0;i<len;i++) {
		newNode = (struct LNode *)malloc(sizeof(struct LNode));
		newNode->index = i;
		newNode->size = arr[i];
		newNode->next = firstNode;
		firstNode = newNode;
	}
}

int isListEmpty()
{
	if(firstNode == NULL)
		return 1;
	else
		return 0;
}

unsigned long getListNum()
{
	unsigned long num = 0;
	struct LNode *current = firstNode;

	while(current) {
		current = current->next;
		++num;
	}

	return num;
}

int getListIndex(int d)
{
	int i = 0;
	struct LNode *current = firstNode;

	for(i = 0;i<d;i++) {
		current = current->next;
	}
	return current->index;
}


void displayList(FILE *file, int option ,size_t size, size_t totalSize)
{
	if(option %2 == 0)
		fprintf(file,"alloc [ %f MB] this time, total size turn out to [ %f MB].\n",(float)size/(1024.0*1024.0),(float)totalSize/(1024.0*1024.0));
	else
		fprintf(file,"release [ %f MB] this time, total size turn out to [ %f MB].\n",(float)size/(1024.0*1024.0),(float)totalSize/(1024.0*1024.0));
}

struct LNode *searchNode(int index)
{
	struct LNode *current = firstNode;
	while(current != NULL) {
		if(current->index == index)
			return current;
		current = current->next;
	}

	assert(0);
	return NULL;
}

void deleteNodeWithoutData(struct LNode *ptr)
{
	struct LNode *current = firstNode;

	if(isListEmpty())
		return;

	if(ptr == firstNode) {
		firstNode = firstNode->next;
	}
	else {
		while(current->next != ptr)
			current = current->next;

		if(ptr->next == NULL)
			current->next = NULL;
		else
			current->next = ptr->next;
	}

	free(ptr);
}

void deleteNode(struct LNode *ptr)
{
	struct LNode *current = firstNode;

	if(isListEmpty())
		return;

	if(ptr == firstNode) {
		firstNode = firstNode->next;
	} else {
		while(current->next != ptr)
			current = current->next;

		if(ptr->next == NULL)
			current->next = NULL;
		else
			current->next = ptr->next;
	}

	free(ptr->data);
	free(ptr);
}

void insertNodeWithoutData(struct LNode *ptr)
{
	struct LNode *newNode;
	newNode = (struct LNode *) malloc(sizeof(struct LNode));

	newNode->index = ptr->index;
	newNode->size = ptr->size;

	newNode->g3d_va = ptr->g3d_va;
	newNode->va = ptr->va;
	newNode->next = firstNode;
	firstNode = newNode;
}

void insertNode(struct LNode *ptr)
{
	struct LNode *newNode;
	newNode = (struct LNode *) malloc(sizeof(struct LNode));

	newNode->index = ptr->index;
	newNode->size = ptr->size;

	newNode->data = (unsigned char*)malloc(ptr->size);
	memcpy(newNode->data,ptr->data,ptr->size);

	newNode->g3d_va = ptr->g3d_va;
	newNode->va = ptr->va;
	newNode->next = firstNode;
	firstNode = newNode;
}

size_t getNodeSize(unsigned long release_index)
{
	unsigned long index = 0;
	struct LNode *currentnode = firstNode;
	while(index != release_index) {
		index++;
		currentnode = currentnode->next;
	}
	return currentnode->size;
}


size_t getListIndexSize(int index)
{
	struct LNode *current = firstNode;

	while(current != NULL) {
		if(current->index == index)
			return current->size;

		if(current->next!=NULL)
			current = current->next;
	}

	assert(0);
	return 0;
}

struct LNode *getFirstNode()
{
	return firstNode;
}

unsigned char intCharMap( int integer )
{
	unsigned char character;
	integer = integer % 10;
	switch(integer)
	{
	case 1:
		character = '1';
		break;
	case 2:
		character = '2';
		break;
	case 3:
		character = '3';
		break;
	case 4:
		character = '4';
		break;
	case 5:
		character = '5';
		break;
	case 6:
		character = '6';
		break;
	case 7:
		character = '7';
		break;
	case 8:
		character = '8';
		break;
	case 9:
		character = '9';
		break;
	case 0:
		character = '0';
		break;
	default:
		character = 'A';
		break;
	}
	return character;
}

unsigned char* getDataFromVa(va_t va)
{
	unsigned char *data = NULL;
	struct LNode *current = firstNode;
	while(current != NULL) {
		if(va == current->va) {
			data = (unsigned char*)malloc(current->size);
			memcpy(data,current->data,current->size);
			break;
		}
		current = current->next;
	}
	return data;
}

size_t getNodeSize(va_t va)
{
	size_t size = 0L;
	struct LNode *current = firstNode;
	while(current != NULL) {
		if(va == current->va)
			return current->size;

		current = current->next;
	}
	return 0;
}

int checkMem(struct Mapper *m, g3d_va_t g3d_va, unsigned char *data)
{
	int compareResult = 0;
	//convert the g3d_va to pa
	pa_t pa = Mapper_g3d_va_to_pa(m, g3d_va); //crash here

	//convert the pa to va
	va_t va = pa_to_va(m, pa);

	//reading according to va
	unsigned char *dataFromVa = getDataFromVa(va);
	size_t nodesize = getNodeSize(va);
	//compare the value located at va and g3d_va
	compareResult = memcmp(data,dataFromVa,nodesize);//crash

	free(dataFromVa);

	if(compareResult == 0) return 1;
	else return 0;

}

void checkMemoryLayout(struct Mapper *m)
{
	struct LNode *currentnode = firstNode;
	while(currentnode != NULL) {
		assert(checkMem(m, currentnode->g3d_va ,currentnode->data));//when traversing, data becomes to null
		currentnode = currentnode->next;//currentnode data broken
	}
}

g3d_va_t getG3dOfNode( unsigned long release_index )
{
	unsigned long index = 0;
	struct LNode *currentnode = firstNode;
	while(index != release_index) {
		index++;
		currentnode = currentnode->next;//currentnode data broken
	}
	return currentnode->g3d_va;
}

struct LNode *searchNodeByOrder( unsigned long release_index )
{
	unsigned long index = 0;
	struct LNode *currentnode = firstNode;
	while(index != release_index) {
		index++;
		currentnode = currentnode->next;//currentnode data broken
	}
	return currentnode;
}
