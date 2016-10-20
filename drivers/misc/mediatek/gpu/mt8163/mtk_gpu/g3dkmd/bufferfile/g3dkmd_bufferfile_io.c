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

/* #include <linux/fs_struct.h> // for get_fs_pwd(). */
/* #include <linux/syscalls.h>  // sys_getcwd(); */

#include <linux/fs.h>		/* for struct file */
#include <linux/sched.h>	/* for current->* */


#include "g3dbase_common_define.h"	/* for define G3DKMD_USE_BUFFERFILE */
#include "yl_define.h"
#include "g3dkmd_define.h"
/* #include "g3dkmd_task.h"           // for gExecInfo */
#include "g3dbase_ioctl.h"	/* for filp->private_data->address */
#include "g3dkmd_engine.h"	/* for the G3DKMDMALLOC, ... prototype */
#include "g3dkmd_util.h"	/* for G3DKMDVMALLOC */
#include "g3dkmd_bufferfile_io.h"
#include "g3dkmd_bufferfile.h"
#include "g3dkmd_file_io.h"	/* for KMDDPF() macro */

#ifdef KMDDPF
#define BF_DPF KMDDPF
#else
#define BF_DPF(type, ...) pr_debug(__VA_ARGS__)
#endif

#endif



/* linux kernel space */
/* Bufferfile Only Available in Linux Kernel mode. */
#if defined(linux) && !defined(linux_user_mode)	/* && defined(G3DKMD_USE_BUFFERFILE) */


#if defined(G3DKMD_TEST_BUFFERFILE)
#define MAX_FILES 1024
struct _bf_file_map {
	void *kffd;
	unsigned int bffd;
};

static struct _bf_file_map _bfio_ftable[MAX_FILES] = { 0, 0 };
#endif


/*  Memory clean up. */
/* avoid memory leek when crash. */
/*void _bfio_releasePidEverything(struct file *filp);*/
/* void _g3dKmdBufferFileReleaseEverything(void); */



#if defined(G3DKMD_TEST_BUFFERFILE)
/* ONLY for single process test. */
void *_bfio_find_file(unsigned int bffd);

int _bfio_find_file_idx(unsigned int bffd)
{
	int idx;

	for (idx = 0; idx < MAX_FILES; ++idx) {
		if (_bfio_ftable[idx].bffd == bffd)
			return idx;
	}
	return -1;
}


int _bfio_add_file(unsigned int bffd, void *kffd)
{
	int idx = _bfio_find_file_idx(0);

	if (idx < 0 || idx >= MAX_FILES) {
		BF_DPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_BUFFERFILE,
		       "[%s:%d] (bffd: %u, kffd %p)idx(%d) out of range: (%d-%d)\n", __func__,
		       __LINE__, bffd, kffd, idx, 0, MAX_FILES);
		return -1;
	}
	BF_DPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_BUFFERFILE,
	       "[%s:%d] Add(bffd, ffd) at idx(%d) (%u, %p)\n", __func__, __LINE__, idx, bffd,
	       kffd);
	_bfio_ftable[idx].bffd = bffd;
	_bfio_ftable[idx].kffd = kffd;

	return idx;
}

void *_bfio_find_file(unsigned int bffd)
{
	int idx = _bfio_find_file_idx(bffd);

	if (-1 == idx) {
		BF_DPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_BUFFERFILE, "[%s:%d] No bffd(%u) found.\n",
		       __func__, __LINE__, bffd);
		return NULL;
	}

	return _bfio_ftable[idx].kffd;
}

int _bfio_remove_file(unsigned int bffd)
{

	int idx = _bfio_find_file_idx(bffd);

	if (-1 == idx) {
		BF_DPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_BUFFERFILE, "[%s:%d] No bffd(%u) found.\n",
		       __func__, __LINE__, bffd);
		return -1;
	}
	BF_DPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_BUFFERFILE, "[%s:%d] Remove bffd(%u) at idx(%d).\n",
	       __func__, __LINE__, bffd, idx);

	_bfio_ftable[idx].bffd = 0;
	_bfio_ftable[idx].kffd = NULL;

	return 0;
}

void _bfio_reset_file(void)
{
	int idx;

	for (idx = 0; idx < MAX_FILES; ++idx) {
		_bfio_ftable[idx].bffd = 0;
		_bfio_ftable[idx].kffd = 0;
	}
	BF_DPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_BUFFERFILE, "reset file\n");
}

#endif /* G3DKMD_TEST_BUFFERFILE */



/* =========================================================================== */
/* g3dkmd kernel */
/* =========================================================================== */
inline pid_t getPID(void)
{
	return current->tgid;
}

/* File IO used in KMD mode */
/* return file discripter. */
void *g3dKmdFileOpen_bufferfile(const char *path)
{
	bf_file_fd_t filp = bf_fopen(path, getPID());

	if (!filp) {
		BF_DPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_BUFFERFILE,
		       "[%s:%d] bufferfile open fail.\n", __func__, __LINE__);
		return (void *)-1;
	}
#if defined(G3DKMD_TEST_BUFFERFILE)
	{
		char fname[64];
		void *kffd;

		sprintf(fname, "%s.kfile", path);
		kffd = g3dKmdFileOpen_kfile(fname);

		BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "kfile Open(%s)(%u, %u)\n", fname,
		       (unsigned int)kffd, filp);

		_bfio_add_file(filp, kffd);
	}
#endif /* G3DKMD_TEST_BUFFERFILE */
	return (void *)filp;
}


void g3dKmdFileClose_bufferfile(void *filp)
{
	bf_fclose((bf_file_fd_t) filp, getPID());


#if defined(G3DKMD_TEST_BUFFERFILE)
	{
		void *kffd = _bfio_find_file((unsigned int)filp);

		BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "kfile Close(%u, %u)\n",
		       (unsigned int)kffd, filp);
		g3dKmdFileClose_kfile(kffd);
		_bfio_remove_file((unsigned int)filp);
	}
#endif /* G3DKMD_TEST_BUFFERFILE */
}

void g3dKmdVfprintf_bufferfile(void *filp, const char *pszFormat, va_list args)
{
	if (!filp)
		return;

	{
		int len;
		char *data;

		len = snprintf(NULL, 0, pszFormat, args);
		/* data = (char*)vmalloc(len + 1); */
		data = (char *)G3DKMDVMALLOC(len + 1);
		if (!data) {
			BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
			       "[%s] vmalloc fail (size %d)\n", __func__, len + 1);
			return;
		}
		len = vsnprintf(data, len + 1, pszFormat, args);
		g3dKmdFileWrite_bufferfile(filp, (unsigned char *)data, len);
		/* vfree(data); */
		G3DKMDVFREE(data);
	}
}

void g3dKmdFprintf_bufferfile(void *filp, const char *pszFormat, ...)
{
	if (filp) {
		int len;
		char *data;
		va_list args;

		va_start(args, pszFormat);

		len = snprintf(NULL, 0, pszFormat, args);
		/* data = (char*)vmalloc(len + 1); */
		data = (char *)G3DKMDVMALLOC(len + 1);
		if (!data) {
			BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE,
			       "[%s] vmalloc fail (size %d)\n", __func__, len + 1);
			return;
		}
		len = vsnprintf(data, len + 1, pszFormat, args);
		/* file_write((struct file*)filp, data, len); */
		g3dKmdFileWrite_bufferfile(filp, (unsigned char *)data, len);

		/* vfree(data); */
		G3DKMDVFREE(data);
		va_end(args);
	}
}


int g3dKmdFileWrite_bufferfile(void *filp, unsigned char *data, unsigned int size)
{
	int ret = bf_fwrite((bf_file_fd_t) filp, getPID(), data, size);
#if defined(G3DKMD_TEST_BUFFERFILE)
	if (-1 != ret) {
		void *kffd = _bfio_find_file((bf_file_fd_t) filp);

		BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "kfile write(%u, %u)\n",
		       (bf_file_fd_t) kffd, filp);

		g3dKmdFileWrite_kfile(kffd, data, size);
	}
#endif /* G3DKMD_TEST_BUFFERFILE */

	return ret;

}

void g3dKmdFileRemove_bufferfile(const char *path)
{
	/* not implemented */
}


void _g3dKmdBufferFileDumpBegin(struct file *filp, void **root, int *pCount)
{
	bf_writeClosedFileBegin(root, pCount, getPID());

	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "[bufferfile] root pa: %p\n", *root);

	*root = (void *)__pa(*root);	/* return PA to user mode. */

	/* Record the address in gExecInfo */
	/* so that g3dkmd g3d_mmap can forward the request */
	/* to there. */
	if (filp && filp->private_data) {
		g3d_private_data *p = filp->private_data;

		p->bf_mmap_address = (bf_file_fd_t) (*root);
	} else {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE, "[%s:%d] fail\n", __func__,
		       __LINE__);
	}

}

void _bfio_freeClosedFiles(void)
{

	bf_freeClosedFiles(getPID());
}

void _g3dKmdBufferFileDumpEnd(struct file *filp, bool isFreeWrittenFiles)
{
	if (filp && filp->private_data) {
		g3d_private_data *p = filp->private_data;

		p->bf_mmap_address = (uintptr_t) NULL;
	} else {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE, "[%s:%d] fail\n", __func__,
		       __LINE__);
	}

	if (isFreeWrittenFiles)
		_bfio_freeClosedFiles();

	bf_writeClosedFileEnd(getPID());
}




void _bfio_releasePidEverything(struct file *filp)
{
	if (filp && filp->private_data) {
		g3d_private_data *p = filp->private_data;

		p->bf_mmap_address = (uintptr_t) NULL;
	} else {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE, "[%s:%d] fail\n", __func__,
		       __LINE__);
	}

#if defined(G3DKMD_TEST_BUFFERFILE)
	_bfio_reset_file();
#endif


	return bf_releasePidEverything(getPID());
}

/*
void _g3dKmdBufferFileReleaseEverything(void)
{
    extern struct g3dExecuteInfo gExecInfo;

    gExecInfo.bufferfileRootAddr = (unsigned int)NULL;
    //BF_DPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_BUFFERFILE,"[bufferfile_io] release everyting.\n");

    return bf_releaseEverything();
}
*/

/*  bufferfile mmap */
void _g3dKmdBufferFileDeviceOpenInit(struct file *filp)
{
	/* mmap address */
	if (filp && filp->private_data) {
		g3d_private_data *p = filp->private_data;

		p->bf_mmap_address = (uintptr_t) NULL;
	} else {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE, "[%s:%d] fail\n", __func__,
		       __LINE__);
	}
}

void _g3dKmdBufferFileDeviceRelease(struct file *filp)
{
	/* clean up */
	if (filp && filp->private_data) {
		g3d_private_data *p = filp->private_data;

		p->bf_mmap_address = (uintptr_t) NULL;
	} else {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE, "[%s:%d] fail\n", __func__,
		       __LINE__);
	}
	_bfio_releasePidEverything(filp);
}

int _g3dKmdIsBufferFileMmap(struct file *filp, uintptr_t offsetAddr)
{
	int retval = 0;

	BF_DPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_BUFFERFILE, "[bufferfile] (%s) offsetAddr: %p\n",
	       __func__, offsetAddr);

	if (filp && filp->private_data) {
		g3d_private_data *p = filp->private_data;

		retval = (offsetAddr == p->bf_mmap_address);
	} else {
		BF_DPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_BUFFERFILE, "[%s:%d] fail\n", __func__,
		       __LINE__);
	}
	return retval;
}

int _g3dkmdBufferFileMmap(struct file *filp, struct vm_area_struct *vma)
{
	return bf_mmapBufferfile(filp, vma, getPID());
}

#endif /* defined(linux) && !defined(linux_user_mode) && defined(G3DKMD_USE_BUFFERFILE) */
