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

#include <stdarg.h>
#include <linux/slab.h>

#endif

#include "g3dbase_common_define.h" /* for define G3DKMD_USE_BUFFERFILE */
#include "yl_define.h"
#include "g3dkmd_define.h"

#include "g3dbase_type.h"	/* for struct UFileHeader */
#include "g3dkmd_engine.h"	/* for the G3DKMDMALLOC, ... prototype */
#include "g3dkmd_util.h"	/* for SEM_LOCK, G3dKMDMALLOC, ... */
#include "g3dkmd_bufferfile.h"

#include "g3dkmd_file_io.h"	/* for KMDDPF() macro */

#ifdef KMDDPF
#define BF_DPF KMDDPF
#else
#define BF_DPF(type, ...) pr_debug(__VA_ARGS__)
#endif

/* #define L_DEBUG */

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a):(b))
#endif

/*
linux kernel space
Bufferfile Only Available in Linux Kernel mode.
*/
#if defined(linux) && !defined(linux_user_mode)	/* && defined(G3DKMD_USE_BUFFERFILE) */


/*
Bufferfile Lock for file list protection.
  a lock for file struct link list.
  - One thread can create/delete/search once.
*/
static __DEFINE_SEM_LOCK(g_bfFileListModifyLock);
static __DEFINE_SEM_LOCK(g_bfFileListSearchLock);

/*
When user is begin to dumpping files.
   - File could not be create/delete from file list.
   - search file and write data block is allowed.
IMPORTANT:
PS: this may cause deadlock. because context switch process
     a) will lock g3dkmdlock
     b) may write file and enter this lock.
    User trp to dump file from ioctl will
     a) also lock g3dkmdlock
     b) locks this to prevent the data be modified.
   So deadlock happened!

Solution:
    Use 3 queues to store files (openedQ/closedQ/writtenQ),
    move the file which need to be written from closedQ to
    writtenQ, and only dump files in writtenQ in mmap.
*/
static __DEFINE_SEM_LOCK(g_bfUserDumpingFileLock);


/*
A lock for all write data action.
 A better way is one file one lock. but I don't know how to
 add lock into file struct.
*/
static __DEFINE_SEM_LOCK(g_bfWriteDataLock);


/* --------------------------------------------------------------------- */
/* Internal static global variables */
/* --------------------------------------------------------------------- */
/* file structure queue */
static struct Queue_Bufferfile bfQOpened = { NULL, NULL };	/* for OPENED files */
static struct Queue_Bufferfile bfQClosed = { NULL, NULL };	/* for CLOSED files */
static struct Queue_Bufferfile bfQWriten = { NULL, NULL };	/* for WRITEN files */

/* mmap  User File Header page */
#define MAX_USER_HEADER_PAGE 16
static void *_bf_gUFHPages[MAX_USER_HEADER_PAGE] = { NULL };



/* ================================================================ */
/* Function Declaration */
/* Locks */
enum _bf_LOCK_LEVEL {
	LOCK_LEVEL_USER_DUMP = 0,
	LOCK_LEVEL_FILE_SEARCH = 1,
	LOCK_LEVEL_FILE_OPEN = 2,
	LOCK_LEVEL_FILE_CLOSE = 3,
	LOCK_LEVEL_FILE_DELETE = 4,
};



/* Page management */
/* unsigned long _bf_getFreePage(void); */
inline void _bf_freePage(unsigned long addr);
inline int _bf_getAllocPageCount(void);
/* void*         _bf_kmalloc(size_t size, int flags); */
void *_bf_kmalloc(size_t size);
inline void _bf_kfree(const void *ptr);
inline int _bf_getKmallocCount(void);


/* File Queue */
/* void _bf_push(struct Queue_Bufferfile *q, struct BufferfileFile *bf); */
/* void _bf_delete(struct Queue_Bufferfile *q, int status, pid_t pid); */
struct BufferfileFile *_bf_dequeue(struct Queue_Bufferfile *q, bf_file_fd_t filp, pid_t pid);
struct BufferfileFile *_bf_find(struct Queue_Bufferfile *q, bf_file_fd_t filp, pid_t pid);
/* bool _bf_isExist(struct Queue_Bufferfile *q, const char *fn, pid_t pid, int status); */
/* helper function */
struct BufferfileFile *_bf_remove_node(struct Queue_Bufferfile *q, struct BufferfileFile *node,
					     struct BufferfileFile *pre);


/* File create/close/free/cleanup/write. */
/* bf_file_fd_t bf_fopen(const char *fname, pid_t pid); */
/* void bf_fclose(bf_file_fd_t filp, pid_t pid); */
/* int          bf_fwrite (unsigned int filp, void* data, unsigned int size); */
/* int bf_fwrite(bf_file_fd_t filp, pid_t pid, void *data, unsigned int size); */
/* void bf_freeClosedFiles(pid_t pid); */
/* void bf_releasePidEverything(pid_t pid); */
/* void bf_releaseEverything(void); */
static void _bf_deleteFile(struct BufferfileFile *);


/* BF Data Block */
struct BufferfileDataBlock *_bf_createDataBlock(void);
static void _bf_deleteDBlocks(struct BufferfileDataBlock *pD);
static  int _bf_writeDBlock(struct BufferfileFile *pBF, void *data, unsigned int size);


/*
mmap  User File Header page.
    User file header
    This struct also exit in g3d/bufferfile/g3d_bufferfile_io.h
typedef struct _user_file_header {
	char name[ MAX_BUFFERFILE_NAME ];
	int pageOffset;
	int fileLen;
} UFileHeader;
*/

/* int _bf_createUserFileHeaderPage(pid_t pid); */
static void _bf_deleteUserFileHeaderPage(pid_t pid);
static void _bf_writePidUserFileHeaderPage(void *page, pid_t pid);
static pid_t _bf_getPidUserFileHeaderPage(void *page);

void *_bf_getUserFileHeaderPage(pid_t pid);
/* void _bf_moveClosedFile2WritenList(pid_t pid); */
/*void _bf_getUserFileHeaderPageInfo(void **pFiles_addr, int *pPageCount,
				   pid_t pid, struct Queue_Bufferfile *q); */

UFileHeader *_bf_getNthUserFileHeader(void *page, const int n);
/* void _bf_writeUserFileHeaderCount(void *page, int nFile); */
/* void _bf_writenNthUserFileHeader(void *page, int nFile, const char *name,
				 const int pOff, const int nPages); */
static void _bf_writeUserFileHeader(UFileHeader *p, const char *name,
			     const int pOffset, const int fileLen);

/* User Dump File Interfaces */
/* void bf_writeClosedFileBegin(void **root, int *pCount, pid_t pid); */
/* void bf_writeClosedFileEnd(pid_t pid); */


/* mmap callback */
/* int bf_mmapBufferfile(struct file *filp, struct vm_area_struct *vma, pid_t pid); */


/* ------------------------------------------------------------------------- // */
/* Locks */
/* ------------------------------------------------------------------------- // */
void _bf_sem_lock(int level)
{
	/* need to get required lock. */
	switch (level) {
	case LOCK_LEVEL_USER_DUMP:{
			__SEM_LOCK(g_bfUserDumpingFileLock);
		}
		break;
	case LOCK_LEVEL_FILE_SEARCH:
	case LOCK_LEVEL_FILE_CLOSE:{
			__SEM_LOCK(g_bfUserDumpingFileLock);
			__SEM_LOCK(g_bfFileListSearchLock);
		}
		break;
	case LOCK_LEVEL_FILE_OPEN:
	case LOCK_LEVEL_FILE_DELETE:{
			__SEM_LOCK(g_bfUserDumpingFileLock);
			__SEM_LOCK(g_bfFileListSearchLock);
			__SEM_LOCK(g_bfFileListModifyLock);
		}
		break;
	}
}


void _bf_sem_unlock(int level)
{
	switch (level) {
	case LOCK_LEVEL_USER_DUMP:{
			__SEM_UNLOCK(g_bfUserDumpingFileLock);
		}
		break;
	case LOCK_LEVEL_FILE_SEARCH:
	case LOCK_LEVEL_FILE_CLOSE:{
			__SEM_UNLOCK(g_bfFileListSearchLock);
			__SEM_UNLOCK(g_bfUserDumpingFileLock);
		}
		break;
	case LOCK_LEVEL_FILE_OPEN:
	case LOCK_LEVEL_FILE_DELETE:{
			__SEM_UNLOCK(g_bfFileListModifyLock);
			__SEM_UNLOCK(g_bfFileListSearchLock);
			__SEM_UNLOCK(g_bfUserDumpingFileLock);
		}
		break;
	}
}


/* ------------------------------------------------------------------------- // */
/* Page management */
/* ------------------------------------------------------------------------- // */
static int _bf_alloc_page_count;
static int _bf_kmalloc_count;

inline unsigned long _bf_getFreePage(void)
{
	unsigned long addr = __get_free_page(GFP_KERNEL);

	if (addr) {		/* ok */
		_bf_alloc_page_count++;
		return addr;
	}
	return (unsigned long)NULL;
}


inline void _bf_freePage(unsigned long addr)
{
	free_page(addr);
	_bf_alloc_page_count--;
}

inline int _bf_getAllocPageCount(void)
{
	return _bf_alloc_page_count;
}

/* void* _bf_kmalloc(size_t size, int flags) */
void *_bf_kmalloc(size_t size)
{
	/* void *ret = kmalloc(size,flags); // kmalloc */
	void *ret = G3DKMDMALLOC(size);	/* kmalloc */

	if (!ret)
		return NULL;

	_bf_kmalloc_count++;
	return ret;
}

inline void _bf_kfree(const void *ptr)
{
	/* kfree(ptr); // kfree */
	G3DKMDFREE(ptr);	/* kfree */
	_bf_kmalloc_count--;
}

inline int _bf_getKmallocCount(void)
{
	return _bf_kmalloc_count;
}

/* ------------------------------------------------------------------------- // */
/* File Queue */
/* ------------------------------------------------------------------------- // */

void _bf_push(struct Queue_Bufferfile *q, struct BufferfileFile *bf)
{
	_bf_sem_lock(LOCK_LEVEL_FILE_OPEN);
	{
		if (q->front == NULL) {	/* empty */
			q->front = bf;
			q->rear = bf;
		} else {	/* not empty */
			q->rear->pNextFile = bf;
			q->rear = bf;
		}
		bf->pNextFile = NULL;
	}
	_bf_sem_unlock(LOCK_LEVEL_FILE_OPEN);

}

void _bf_delete(struct Queue_Bufferfile *q, int status, pid_t pid)
/* status: -1 means ignore status filter. */
/* pid   : 0 means ignore pid filter. */
{
	struct BufferfileFile *pre = NULL, *n, *nextN;

	_bf_sem_lock(LOCK_LEVEL_FILE_DELETE);
	n = q->front;
	while (n != NULL) {	/* traverse file list */
		nextN = n->pNextFile;

		if ((status == -1 || n->status == status) &&	/* selected status or -1 */
		    (pid == 0 || n->pid == pid)) {	/* selected pid    or  0 */
			/* remvoe from link list. */
			n = _bf_remove_node(q, n, pre);
			/* free data blocks & file structure. */
			_bf_deleteFile(n);
		} else {
			pre = n;
		}
		n = nextN;
	}
	_bf_sem_unlock(LOCK_LEVEL_FILE_DELETE);
}

struct BufferfileFile *_bf_dequeue(struct Queue_Bufferfile *q, bf_file_fd_t filp, pid_t pid)
{
	struct BufferfileFile *pre = NULL, *n, *ret = NULL;

	_bf_sem_lock(LOCK_LEVEL_FILE_DELETE);
	n = q->front;
	while (n != NULL) {	/* traverse file list */
		if ((filp == -1 || n->fd == filp) && (pid == 0 || n->pid == pid)) {
			ret = _bf_remove_node(q, n, pre);
			break;
		}
		pre = n;
		n = n->pNextFile;
	}
	_bf_sem_unlock(LOCK_LEVEL_FILE_DELETE);
	return ret;
}

struct BufferfileFile *_bf_find(struct Queue_Bufferfile *q, bf_file_fd_t filp, pid_t pid)
{
	struct BufferfileFile *n, *ret = NULL;

	_bf_sem_lock(LOCK_LEVEL_FILE_SEARCH);
	for (n = q->front; n != NULL; n = n->pNextFile) {
		/* ignore filp when -1, pid when 0. */
		if ((filp == -1 || n->fd == filp) && (pid == 0 || n->pid == pid))
			ret = n;
	}
	_bf_sem_unlock(LOCK_LEVEL_FILE_SEARCH);
	return ret;
}

bool _bf_isExist(struct Queue_Bufferfile *q, const char *fn, pid_t pid, int status)
{
	struct BufferfileFile *n, *ret = NULL;

	_bf_sem_lock(LOCK_LEVEL_FILE_SEARCH);
	for (n = q->front; n != NULL; n = n->pNextFile) {
		/* ignore filp when -1, pid when 0. */
		if (!strcmp(n->name, fn) && (pid == 0 || n->pid == pid) && n->status == status)
			ret = n;
	}
	_bf_sem_unlock(LOCK_LEVEL_FILE_SEARCH);
	return ret;
}

struct BufferfileFile *_bf_remove_node(struct Queue_Bufferfile *q, struct BufferfileFile *n, struct BufferfileFile *pre)
{
	if (pre == NULL) {	/* located at 1st node */
		q->front = n->pNextFile;
	} else if (n->pNextFile == NULL) {	/* located at last node */
		pre->pNextFile = NULL;
		q->rear = pre;
	} else {		/* located at middle */
		pre->pNextFile = n->pNextFile;
	}
	return n;
}

/* ------------------------------------------------------------------------- // */
/* File create/close/free/cleanup/write. */
/* ------------------------------------------------------------------------- // */

bf_file_fd_t bf_fopen(const char *fname, pid_t pid)
{
	struct BufferfileFile *pBF = NULL;

	if (MAX_BUFFERFILE_NAME - 1 <= strlen(fname)) {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
		       "[%s] file name (%s) is too long\n", __func__, fname);
		return 0;
	}
	/* Need to check if the (fname-pid,OPEN) is existed. */
	/* if yes, return -1 */
	if (_bf_isExist(&bfQOpened, fname, pid, OPENED)) {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
		       "[%s] file name (%s) is opened!\n", __func__, fname);
		return 0;
	}

	pBF = (struct BufferfileFile *) _bf_kmalloc(sizeof(struct BufferfileFile));
	if (!pBF) {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE, "[%s] kmalloc fail (size %d)\n",
		       __func__, sizeof(struct BufferfileFile));
		return 0;
	}
	/* initialized */
	strcpy(pBF->name, fname);
	pBF->pid = pid;
	pBF->fd = (bf_file_fd_t) pBF;	/* use the address of file structure */
	/* as its file handle number. */
	pBF->status = OPENED;
	pBF->pNextFile = NULL;
	pBF->front = NULL;
	pBF->rear = NULL;

	/* push to file list. */
	_bf_push(&bfQOpened, pBF);

/* #ifdef L_DEBUG */
	if (KMDDPFCHECK(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE)) {
		BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
		       "[bufferfile] Open file: %s(%lu), pid:%d\n", pBF->name, pBF->fd, pid);
		g3dkmd_backtrace();
	}
/* #endif */

	return pBF->fd;
}


void bf_fclose(bf_file_fd_t filp, pid_t pid)
{
	/* struct BufferfileFile *pBF  = _bf_find(&bfQOpened, filp, pid); */
	/* Remove from OpenFile Queue. */
	struct BufferfileFile *pBF = _bf_dequeue(&bfQOpened, filp, pid);

	if (pBF) {
		/* Modify the status */
		pBF->status = CLOSED;
		/* Push into closed file queue */
		_bf_push(&bfQClosed, pBF);

		BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
		       "[bufferfile] Close file(%lu), pid: %d\n", filp, pid);
		BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "[bufferfile] file: %s (%d)\n",
		       pBF->name, pBF->status);

	} else {
		/* close buffer file fail. */
	}
}


/* int bf_fwrite(void *filp, unsigned char* data, unsigned int size) */
/* int bf_fwrite (unsigned int filp, void* data, unsigned int size) */
int bf_fwrite(bf_file_fd_t filp, pid_t pid, void *data, unsigned int size)
{
	int totalWrite = 0;

	if (filp) {
		/* get last data block of fd, ignore pid. */
		struct BufferfileFile *currentFile = _bf_find(&bfQOpened, (bf_file_fd_t) filp, 0);

		if (currentFile == NULL) {
			BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
			       "[%s:%d] Could not find struct BufferfileFile with (filp,tgid):(%u, -)\n",
			       __func__, __LINE__, filp);

#ifdef L_DEBUG
			BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
			       "[%s:%d] Could not find struct BufferfileFile with (filp,tgid):(%lu, -)\n",
			       __func__, __LINE__, filp);
			BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE, "tgid: %d\n", pid);
#endif
			g3dkmd_backtrace();

			return -1;
		}
		while (size > 0) {
			int len = _bf_writeDBlock(currentFile, data, size);

			if (len == -1)
				return -1;

			size -= len;
			totalWrite += len;
			data += len;
		}
	}

	return totalWrite;
}


void bf_freeClosedFiles(pid_t pid)
{
#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "===========================\n");
#endif
	/* _bf_delete(&bfQWriten, CLOSED, pid); */
	_bf_delete(&bfQWriten, WRITEN, pid);

}

void bf_releasePidEverything(pid_t pid)
{

#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "[%s] release Everything:\n",
	       __func__);
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "Opened Queue:\n");
#endif
	_bf_delete(&bfQOpened, -1, pid);	/* delete all files. */

#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "Closed Queue:\n");
#endif
	_bf_delete(&bfQClosed, -1, pid);	/* delete all files. */

#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "Written Queue:\n");
#endif
	_bf_delete(&bfQWriten, -1, pid);	/* delete all files. */

	_bf_deleteUserFileHeaderPage(pid);
#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "alloc page count: %d\n",
	       _bf_getAllocPageCount());
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "kmalloc    count: %d\n",
	       _bf_getKmallocCount());
#endif

}

void bf_releaseEverything(void)
{
#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "[%s] release Everything:\n",
	       __func__);
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "Deleting Opened Queue:\n");
#endif
	_bf_delete(&bfQOpened, -1, 0);	/* delete all files. */

#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "Deleting Closed Queue:\n");
#endif
	_bf_delete(&bfQClosed, -1, 0);	/* delete all files. */

#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "Deleting Written Queue:\n");
#endif
	_bf_delete(&bfQWriten, -1, 0);	/* delete all files. */

	_bf_deleteUserFileHeaderPage(0);

}


static void _bf_deleteFile(struct BufferfileFile *pBF)
{
	struct BufferfileDataBlock *pD = pBF->front;

#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
	       "[bufferfile] Delete file: %s(%lu), pid: %d\n", pBF->name, pBF->fd, pBF->pid);
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "file: %s, status: %d\n", pBF->name,
	       pBF->status);
#endif
	_bf_deleteDBlocks(pD);
	_bf_kfree(pBF);

}


/* ------------------------------------------------------------------------- // */
/* BF Data Block */
/* ------------------------------------------------------------------------- // */

struct BufferfileDataBlock *_bf_createDataBlock(void)
{
	/* struct BufferfileDataBlock *pData =
		(struct BufferfileDataBlock*) _bf_kmalloc( sizeof(struct BufferfileDataBlock), GFP_KERNEL); */
	struct BufferfileDataBlock *pData =
	    (struct BufferfileDataBlock *) _bf_kmalloc(sizeof(struct BufferfileDataBlock));

	if (!pData) {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE, "[%s] kmalloc fail (size %d)\n",
		       __func__, sizeof(struct BufferfileDataBlock));
		return (void *)0;
	}
	pData->pNextDataBlock = NULL;
	pData->unusedSize = MAX_BF_DATA_SIZE;
	pData->data = (void *)_bf_getFreePage();	/* __get_free_page(GFP_KERNEL); */
	return pData;
}


static void _bf_deleteDBlocks(struct BufferfileDataBlock *pD)
{
	struct BufferfileDataBlock *tmp;

	while (pD) {		/* traverse data block from head. */
		tmp = pD->pNextDataBlock;
		_bf_freePage((unsigned long)pD->data);
		_bf_kfree(pD);
		pD = tmp;
	}

}


static int _bf_writeDBlock(struct BufferfileFile *pBF, void *data, unsigned int size)
{
	/* Only one therad can write. */
	/* a better way is to use per-file lock. */

	int totalWrite = 0;
	struct BufferfileDataBlock *currentDataBlock;

	if (!pBF) {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE, "[%s:%d] pBF is NULL.\n",
		       __func__, __LINE__);
		return -1;
	}
	if (pBF->status != OPENED) {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
		       "[%s:%d] File is not opened.(fname: %sfilp:%lu,pid:%d)\n", __func__,
		       __LINE__, pBF->name, (unsigned long)pBF->fd, pBF->pid);
		return -1;
	}

	__SEM_LOCK(g_bfWriteDataLock);

	if (pBF->front == NULL) {	/* empty */
		currentDataBlock = pBF->rear = pBF->front = _bf_createDataBlock();
	} else if (pBF->rear->unusedSize == 0) {	/* is full */
		currentDataBlock = _bf_createDataBlock();
		pBF->rear->pNextDataBlock = currentDataBlock;
		pBF->rear = currentDataBlock;
	} else {		/* not empty */
		currentDataBlock = pBF->rear;
	}

	__SEM_UNLOCK(g_bfWriteDataLock);

	if (!currentDataBlock)
		return -1;


	totalWrite = MIN(size, currentDataBlock->unusedSize);

	memcpy((currentDataBlock->data + MAX_BF_DATA_SIZE - currentDataBlock->unusedSize), data,
	       totalWrite);
	currentDataBlock->unusedSize -= totalWrite;


	return totalWrite;
}


/* ------------------------------------------------------------------------- // */
/* mmap  User File Header page */
/* ------------------------------------------------------------------------- // */

int _bf_createUserFileHeaderPage(pid_t pid)
{
	int idx;
	void *page = NULL;

	for (idx = 0; idx < MAX_USER_HEADER_PAGE; ++idx) {
		if (_bf_gUFHPages[idx])
			continue;

		page = _bf_gUFHPages[idx] = (void *)_bf_getFreePage();

		if (!_bf_gUFHPages[idx]) {
			BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
			       "[%s:%d] _bf_gUFHPages[idx] alloc page fail.\n", __func__,
			       __LINE__);
			return -1;
		}
		_bf_writePidUserFileHeaderPage(page, pid);
#ifdef L_DEBUG
		BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
		       "get header page[%d]:%lu pid: %d\n", idx, (unsigned long)page, pid);
#endif /* L_DEBUG */
		break;
	}			/* for */

	if (!page) {
		BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
		       "[%s:%d] _bf_gUFHPages[MAX_USER_HEADER_PAGE(%d)] is full.\n", __func__,
		       __LINE__, MAX_USER_HEADER_PAGE);
	}

	return 0;
}


static void _bf_deleteUserFileHeaderPage(pid_t ipid)
{
	int idx;

	for (idx = 0; idx < MAX_USER_HEADER_PAGE; ++idx) {
		void *page = _bf_gUFHPages[idx];

		if (page) {
			pid_t pid = _bf_getPidUserFileHeaderPage(page);

			if (ipid == 0 || ipid == pid) {
				_bf_freePage((unsigned long)page);
				_bf_gUFHPages[idx] = NULL;
#ifdef L_DEBUG
				BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
				       "[%s:%d] page[%d]: %lu, pid: %d ipid:%d\n", __func__,
				       __LINE__, idx, (unsigned long)page, pid, ipid);
#endif
				if (ipid != 0)
					break;
			}
		}
	}

}

static void _bf_writePidUserFileHeaderPage(void *page, pid_t pid)
{
	*((pid_t *) page) = pid;
}

static pid_t _bf_getPidUserFileHeaderPage(void *page)
{
	return *((pid_t *) page);
}

void *_bf_getUserFileHeaderPage(pid_t pid)
{
	int idx;

	for (idx = 0; idx < MAX_USER_HEADER_PAGE; ++idx) {
		if (pid == 0 || pid == _bf_getPidUserFileHeaderPage(_bf_gUFHPages[idx]))
			break;
	}

	if (idx >= MAX_USER_HEADER_PAGE)
		return NULL;


	return _bf_gUFHPages[idx];
}

void _bf_moveClosedFile2WritenList(pid_t pid)
{
	struct BufferfileFile *fp, *n, *pre, *next;	/* fQOpened.front; */
	struct Queue_Bufferfile *qc = &bfQClosed, *qw = &bfQWriten;
	int nFile = 0;

	/* for each file in Closed file Queue */
	fp = qc->front;
	pre = NULL;
	while (fp != NULL) {
		next = fp->pNextFile;
		if (fp->pid == pid) {
			n = _bf_remove_node(qc, fp, pre);
			n->status = WRITEN;
			_bf_push(qw, n);
			nFile += 1;
		} else {
			pre = fp;
		}
		fp = next;
	}

#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "[%s:%d] nFiles: %d\n", __func__,
	       __LINE__, nFile);
#endif
}

void _bf_getUserFileHeaderPageInfo(void **pRoot, int *pPageCount, pid_t pid, struct Queue_Bufferfile *q)
{
	struct BufferfileFile *fd = q->front;	/* fQOpened.front; */
	struct BufferfileDataBlock *pD;

	int nPage = 0;
	int nFile = 0;

	while (fd != NULL) {
		if (fd->pid == pid && fd->status == WRITEN) {
			nFile += 1;
			for (pD = fd->front; pD != NULL; pD = pD->pNextDataBlock)
				nPage += 1;
		}
#ifdef L_DEBUG
		BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
		       "[%s:%d] fname: %s, acPages: %d\n", __func__, __LINE__, fd->name, nPage);
#endif
		fd = fd->pNextFile;
	}

	*pPageCount = nPage + 1;	/* add 1 for header file itself. */
	*pRoot = q->front;

}


UFileHeader *_bf_getNthUserFileHeader(void *page, const int n)
{
	const int MAX_USER_HEADER = (PAGE_SIZE - sizeof(int) - sizeof(pid_t)) / sizeof(UFileHeader);

	if (n > MAX_USER_HEADER) {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
		       "[%s:%d] bufferfile: nFile(%d)>MAX_USERHEADER(%d)", __func__, __LINE__,
		       n, MAX_USER_HEADER);
		return NULL;
	}
	return (UFileHeader *) (page + (n - 1) * sizeof(UFileHeader) + sizeof(int) + sizeof(pid_t));
}


void _bf_writeUserFileHeaderCount(void *page, int nFile)
{
	*((int *)(page + sizeof(pid_t))) = nFile;
}


void _bf_writenNthUserFileHeader(void *page, int nFile,
				 const char *name, const int pOff, const int nPages)
{
	UFileHeader *fhead = _bf_getNthUserFileHeader(page, nFile);

	if (NULL == fhead)
		return;

	_bf_writeUserFileHeader(fhead, name, pOff, nPages);

#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "[%s:%d] fname:%s offset:%d flen: %d\n",
	       __func__, __LINE__, fhead->name, fhead->pageOffset, fhead->fileLen);
#endif
}


static void _bf_writeUserFileHeader(UFileHeader *p, const char *name,
			     const int pOffset, const int fileLen)
{
	memcpy(p->name, name, MAX_BUFFERFILE_NAME);
	p->pageOffset = pOffset;
	p->fileLen = fileLen;
}


/* ------------------------------------------------------------------------- // */
/* mmap callback */
/* ------------------------------------------------------------------------- // */

#define remapPage2(vma, pageIdx, page) \
	(vm_insert_page((vma), ((vma)->vm_start + PAGE_SIZE * pageIdx), (page)))


int bf_mmapBufferfile(struct file *filp, struct vm_area_struct *vma, pid_t pid)
{

#ifdef L_DEBUG
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
	       "[%s:%d] bf_mmap = 0x%lx 0x%lx 0x%lx 0x%lx, len: %lu (%lx)\n", __func__,
	       __LINE__, vma->vm_start, vma->vm_end, vma->vm_pgoff,
	       *(unsigned long *)&vma->vm_page_prot, (vma->vm_end - vma->vm_start),
	       (vma->vm_end - vma->vm_start));
#endif
	{
		int pageCount = 0;	/* the fileheader. */
		int nPage_pre;
		int nFile = 0;
		void *root;
		struct BufferfileFile *fp;
		void *header = _bf_getUserFileHeaderPage(pid);

		if (!header) {
			BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
			       "[%s:%d] mmap fail: user file header page alloc fail.\n",
			       __func__, __LINE__);
			return -1;
		}
		/* map header page. */
		if (remapPage2(vma, pageCount, virt_to_page(header))) {
			BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
			       "[%s:%d] mmap fail. header page map fail.\n", __func__,
			       __LINE__);
			return -1;
		}
		++pageCount;

		_bf_getUserFileHeaderPageInfo(&root, &nPage_pre, pid, &bfQWriten);

		BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
		       "[%s:%d] bf_mmap: root: 0x%lX, nPage_pre: %d\n", __func__, __LINE__,
		       (unsigned long int)root, nPage_pre);

		/* Write file headers into user header. */
		for (fp = root; fp != NULL; fp = fp->pNextFile) {
			if (fp->status == WRITEN && fp->pid == pid) {
				struct BufferfileDataBlock *pD;
				int flen = 0;
				int offset = pageCount;

				nFile += 1;
/* TODO: Need Search Lock */
				for (pD = fp->front; pD != NULL; pD = pD->pNextDataBlock) {
					flen += (PAGE_SIZE - pD->unusedSize);
					if (remapPage2(vma, pageCount, virt_to_page(pD->data))) {
						BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
						       "[%s:%d] mmap fail. pageCount:%d\n",
						       __func__, __LINE__, pageCount);
						return -1;
					}
					pageCount++;
				}
/* TODO: Need Search unlock */
				/* write to header page. */
				_bf_writenNthUserFileHeader(header, nFile, fp->name, offset, flen);

				BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
				       "[%s:%d] fname:%s pid: %d offset:%d flen: %d\n",
				       __func__, __LINE__, fp->name, pid, offset, flen);
			}
		}		/* for */
		_bf_writeUserFileHeaderCount(header, nFile);
/* #ifdef L_DEBUG */
		if (KMDDPFCHECK(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE)) {
			int i;

			BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
			       "========== List Mapped Files =========\n");
			for (i = 1; i <= nFile; ++i) {
				/* pD2 must be not NULL, because
				 *  1._bf_getN...r() is adding a offset to the pointer.
				 *  2. header is checked NULL above.
				 */
				UFileHeader *pD2 = _bf_getNthUserFileHeader(header, i);

				BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
				       "[%s:%d] fname:%s pid: %d offset:%d flen: %d\n",
				       __func__, __LINE__, pD2->name, pid,
				       pD2->pageOffset, pD2->fileLen);
			}
			BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
			       "======================================\n");
			BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
			       "[%s:%d] mmap OK! pageCount:%d\n", __func__, __LINE__,
			       pageCount);
		}
/* #endif */
	}

	return 0;
}


/* ------------------------------------------------------------------------- // */
/* User Dump File Interfaces */
/* ------------------------------------------------------------------------- // */

void bf_writeClosedFileBegin(void **root, int *pCount, pid_t pid)
{
	int nPage;

/* _bf_sem_lock(USER_DUMP); */

	if (_bf_createUserFileHeaderPage(pid)) {
		/* create page fail. */
		/* Bufferfile_Not_Enough_UserFileHeaderPage_For_Mmap */
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE, "[%s:%d] fail\n", __func__,
		       __LINE__);
		YL_KMD_ASSERT(0);
	}
	_bf_moveClosedFile2WritenList(pid);
	_bf_getUserFileHeaderPageInfo(root, &nPage, pid, &bfQWriten);

	*root = _bf_getUserFileHeaderPage(pid);
	*pCount = PAGE_SIZE * nPage;

	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE,
	       "[%s:%d] pCount: %d (%x), npage: %d(%x)\n", __func__, __LINE__, *pCount, *pCount,
	       nPage, nPage);
}


void bf_writeClosedFileEnd(pid_t pid)
{
	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "============[%s]\n", __func__);
	_bf_deleteUserFileHeaderPage(pid);

/* _bf_sem_unlock(USER_DUMP); */
}


#endif /* defined(linux) && !defined(linux_user_mode) && defined(G3DKMD_USE_BUFFERFILE) */
