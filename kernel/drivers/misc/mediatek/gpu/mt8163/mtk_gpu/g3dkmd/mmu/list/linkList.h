#pragma once
#include <stdlib.h>
#include <stdio.h>
#include "..\yl_mmu.h"

typedef struct  Node
{
	int index; //the index of the allocation
	size_t size;  //the size of the allocation
	va_t va;
	g3d_va_t g3d_va; 
	unsigned char* data;
	struct Node *next;

} LNode,*List;

 
void createList(int len, int *arr);
int isListEmpty();
void displayList(FILE *file, int option, size_t size, size_t totalSize);
List searchNode(int index);
List searchNodeByOrder(unsigned long release_index);
void deleteNode(List ptr);
void deleteNodeWithoutData(List ptr);
void insertNode(List ptr);
void insertNodeWithoutData(List ptr);

unsigned long getListNum();
int getListIndex(int d);
size_t getListIndexSize(int index);
size_t getNodeSize(unsigned long release_index);
List getFirstNode();
unsigned char intCharMap(int integer);
int checkMem(Mapper m, g3d_va_t g3d_va, unsigned char* data);
void checkMemoryLayout(Mapper m);
g3d_va_t getG3dOfNode(unsigned long release_index);
