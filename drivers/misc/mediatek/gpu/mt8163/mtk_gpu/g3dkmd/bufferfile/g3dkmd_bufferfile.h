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

#ifndef _G3DKMD_BUFFERFILE_H_
#define _G3DKMD_BUFFERFILE_H_


#if defined(linux) && !defined(linux_user_mode)	/* && defined(G3DKMD_USE_BUFFERFILE) */

#include <stdarg.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/syscalls.h>
#include <linux/debugfs.h>
#include <linux/namei.h>
#include <linux/file.h>
#include <asm/segment.h>
#include <asm/uaccess.h>


#include "g3dbase_common_define.h"	/* for define G3DKMD_USE_BUFFERFILE */



/* ///////////////////////////////////////////////////////////////////// */
/* Buffer file types */
/* ///////////////////////////////////////////////////////////////////// */

#define bf_file_fd_t uintptr_t

/* ///////////////////////////////////////////////////////////////////// */
/* Buffer block defines */
/* ///////////////////////////////////////////////////////////////////// */

/* #if  defined(_WIN32) || defined(linux_user_mode) // win or linux user mode */
#ifndef PAGE_SIZE
/* #define PAGE_SIZE 4096 */
/* #define PAGE_SIZE 20 */
#endif
/* #endif */

#define MAX_BF_DATA_SIZE PAGE_SIZE


/* --------------------------------------------------------------------- */
/* FILE STATUS */
/* --------------------------------------------------------------------- */
enum BUFFERFILE_STATE {
	OPENED = 1,
	CLOSED = 2,
	WRITING = 3,
	WRITEN = 4,
};


/* ///////////////////////////////////////////////////////////////////// */
/* Buffer block Data structures */
/* ///////////////////////////////////////////////////////////////////// */

/* --------------------------------------------------------------------- */
/* Bufferfile Data Block */
/* - A link list structure, try to fit to page size. */
/* - pNextBlock: a pointer to next data block, NULL if not existed. */
/* - unusedSize: unused size of data. */
/* - data:       an unsigned char array to store data. */
/* --------------------------------------------------------------------- */

struct BufferfileDataBlock {
	struct BufferfileDataBlock *pNextDataBlock;
	unsigned int unusedSize;
	void *data;		/* a pointer to a page aligned */
	/* memory with PAGE_SIZE. */
};


/* --------------------------------------------------------------------- */
/* Bufferfile File Structure */
/* - A link list structure, store the file descriptions */
/* - pNextFile: a pointer to next file, NULL if not existed. */
/* - name     : filename, the length could not be longer than */
/* MAX_BUFFERFILE_NAME */
/* - pid      : user process pid. */
/* - status    : file status. OPENED or CLOSED. */
/* - front    : the head of link list of file structure. */
/* - rear     : the end of link list of file structure. */
/* --------------------------------------------------------------------- */
struct BufferfileFile {
	struct BufferfileFile *pNextFile;
	char name[MAX_BUFFERFILE_NAME];
	pid_t pid;
	bf_file_fd_t fd;
/* enum BUFFERFILE_STATE    status; */
	int status;
	/* __DEFINE_VAR_SEM_LOCK(bfDataLock); */
	/* DECLARE_MUTEX(bfDataLock); */
	struct BufferfileDataBlock *front;
	struct BufferfileDataBlock *rear;
};

/* --------------------------------------------------------------------- */
/* Queue of file structure */
/* - A queue structure for file structure */
/* - front: the head node */
/* - rear : the end node */
/* - currentFilp: the current max file descriptor number */
/* --------------------------------------------------------------------- */
struct Queue_Bufferfile {
	struct BufferfileFile *front;	/* head node */
	struct BufferfileFile *rear;	/* end node */
};





/* ///////////////////////////////////////////////////////////////////// */
/* APIs: Queue of Bufferfile File */
/* ///////////////////////////////////////////////////////////////////// */

/* File create/close/free/cleanup/write. */
/* --------------------------------------------------------------------- */
/* bf_fopen() */
/* API to open bufferfile in linux kernel. */
/*  */
/* inputs: */
/* - fname: file name, its length MUST be less than MAX_BUFFERFILE_NAME */
/* - pid  : user porcess id. */
/* return value: */
/* - bf_file_fd_t: file handler, its type is unsigned int. */
bf_file_fd_t bf_fopen(const char *fname, pid_t pid);


/* --------------------------------------------------------------------- */
/* bf_fclose() */
/* API to close bufferfile in linux kernel. This function don't free */
/* the file memory but set the file status to be CLOSED. */
/* User still need to use mmap to write the data into files in User */
/* mode. */
/*  */
/* inputs: */
/* - filp : file descriptor */
/* - pid  : user porcess id. */
/* return value: */
/* N/A */
/* --------------------------------------------------------------------- */
void bf_fclose(bf_file_fd_t filp, pid_t pid);


/* --------------------------------------------------------------------- */
/* bf_fwrite() */
/* The function help to write data to bufferfile. */
/*  */
/* inputs: */
/* - filp: file descriptor, return by bf_fopen(). */
/* - data: data array. */
/* -size: the length of data. */
/* reutrn value: */
/* return the number of bytes writen into bufferfile. */
/* -1, if fail to write. */
/* --------------------------------------------------------------------- */
int bf_fwrite(bf_file_fd_t filp, pid_t pid, void *data, unsigned int size);


/* --------------------------------------------------------------------- */
/* bf_freeClosedFiles() */
/* Ask bufferfile system free the memory used by closed files. */
/* - Free data blocks and file structure of those files with CLOSED status. */
/* inputs: */
/* - pid:  the pid of the bufferfile associated. */
/* --------------------------------------------------------------------- */
void bf_freeClosedFiles(pid_t pid);


/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/* bf_releasePidEverything() */
/* Release everything hold by Bufferfile module with pid. */
/* pid: the tgid */
/* --------------------------------------------------------------------- */
void bf_releasePidEverything(pid_t pid);


/* bf_releaseEverything() */
/* Release everything hold by Bufferfile module. */
/* --------------------------------------------------------------------- */
void bf_releaseEverything(void);


/* ========================================================================= // */
/* User Dump File Interfaces */
/* --------------------------------------------------------------------- */
/* bf_mmapBufferfile() */
/* Start of dumpping bufferfiles from memory to file system. */
/* inputs: */
/* file_addr: the address user need when doing mmap(). */
/* pCount   : the length user need when doing mmap(). */
/* --------------------------------------------------------------------- */
void bf_writeClosedFileBegin(void **file_addr, int *pCount, pid_t pid);


/* --------------------------------------------------------------------- */
/* bf_writeClosedFaileEnd() */
/* End of dumpping bufferfiles from memory to file system. */
/* --------------------------------------------------------------------- */
void bf_writeClosedFileEnd(pid_t pid);


/* --------------------------------------------------------------------- */
/* bf_mmapBufferfile() */
/* map files to user space as a single buffer. */
/* input: */
/* - filp : callback variable from system. */
/* - vma  : callback variable from system. */
/* - pid  : user porcess id. */
/* --------------------------------------------------------------------- */
int bf_mmapBufferfile(struct file *filp, struct vm_area_struct *vma, pid_t pid);


#endif

#endif /* _G3DKMD_BUFFERFILE_H_ */
