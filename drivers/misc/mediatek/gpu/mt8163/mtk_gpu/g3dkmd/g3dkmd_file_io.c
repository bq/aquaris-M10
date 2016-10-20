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

#if defined(_WIN32) && defined(G3DKMD_EXPORTS)

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#else
#if defined(linux) && defined(linux_user_mode)	/* linux user space */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#else
/*
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/memory.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/slab.h>
*/
#include <stdarg.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/syscalls.h>
#include <linux/debugfs.h>
#include <linux/namei.h>
#include <linux/file.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#endif
#endif

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dkmd_util.h"
#include "g3dkmd_engine.h"
#include "g3dkmd_file_io.h"
#include "mmu/yl_mmu_kernel_alloc.h"


struct g3dKmdFileIo g_kmdFileIo_kfile = {
	g3dKmdFileOpen_kfile,
	g3dKmdFileClose_kfile,
	g3dKmdFprintf_kfile,
	g3dKmdVfprintf_kfile,
	g3dKmdFileWrite_kfile,
	g3dKmdFileRemove_kfile
};

#if defined(linux) && !defined(linux_user_mode)
struct g3dKmdFileIo g_kmdFileIo_bufferfile = {
	g3dKmdFileOpen_bufferfile,
	g3dKmdFileClose_bufferfile,
	g3dKmdFprintf_bufferfile,
	g3dKmdVfprintf_bufferfile,
	g3dKmdFileWrite_bufferfile,
	g3dKmdFileRemove_bufferfile
};
#endif

struct g3dKmdFileIo *g_pKmdFileIo;
static struct g3dKmdFileIo *g_pKmdFileIo_backup;



void g3dKmdInitFileIo(void)
{
#ifdef G3DKMD_USE_BUFFERFILE
	g_pKmdFileIo = &g_kmdFileIo_bufferfile;
#else
	g_pKmdFileIo = &g_kmdFileIo_kfile;
#endif
}

void g3dKmdSetFileIo(enum g3dKmdFileIoMode mode)
{
#if defined(linux) && !defined(linux_user_mode)
	if (mode == G3DKMD_FILEIO_KFILE)
		g_pKmdFileIo = &g_kmdFileIo_kfile;
	else
		g_pKmdFileIo = &g_kmdFileIo_bufferfile;
#else
	g_pKmdFileIo = &g_kmdFileIo_kfile;
#endif
}

void g3dKmdBackupFileIo(void)
{
	g_pKmdFileIo_backup = g_pKmdFileIo;

}

void g3dKmdRestoreFileIo(void)
{
	g_pKmdFileIo = g_pKmdFileIo_backup;
}



#if defined(PATTERN_DUMP) || defined(PRINT2FILE)
#ifdef linux
#if !defined(linux_user_mode)
static struct file *file_open(const char *path)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(path, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);

	set_fs(oldfs);

	if (IS_ERR(filp)) {
		KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FILEIO,
		       "filp_open fail [%s], error code 0x%x\n", path, PTR_ERR(filp));
		/* Below assert will cause KE and then hang, remove it to let caller do error handling. */
		/* YL_KMD_ASSERT(0); */
		return NULL;
	}

	return filp;
}

static void file_close(struct file *filp)
{
	if (filp) {
		fput(filp);
		filp_close(filp, NULL);
	}
}

int file_read(struct file *filp, unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;
	loff_t pos;

	oldfs = get_fs();
	set_fs(get_ds());

#if 1
	pos = filp->f_pos;
	ret = vfs_read(filp, data, size, &pos);
	filp->f_pos = pos;
#else
	filp->f_op->read(filp, data, size, &filp->f_pos);
#endif

	set_fs(oldfs);

	return ret;
}

static int file_write(struct file *filp, unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;
	loff_t pos;

	oldfs = get_fs();
	set_fs(get_ds());

#if 1
	pos = filp->f_pos;
	ret = vfs_write(filp, data, size, &pos);
	filp->f_pos = pos;
#else
	ret = filp->f_op->write(filp, data, size, &filp->f_pos);
#endif

	set_fs(oldfs);
	return ret;
}

static void file_remove(const char *path)
{
#if 0				/* kernel 3.4 don't support path_lookup function */
	char *filename = NULL;
	int ret = 0;
	struct nameidata nd;
	struct dentry *dentry;

	ret = path_lookup(filename, LOOKUP_PARENT, &nd);
	if (ret != 0)
		return;

	dentry = lookup_one_len(nd.last.name, nd.path.dentry, strlen(nd.last.name));
	if (IS_ERR(dentry))
		return;

	vfs_unlink(nd.path.dentry->d_inode, dentry);

	dput(dentry);
#endif
}
#endif /* linux kernel */
#endif

void *g3dKmdFileOpen_kfile(const char *path)
{
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))	/* linux user space */
	return (void *)fopen(path, "w+b");
#elif defined(linux)
	return (void *)file_open(path);
#endif
}

void g3dKmdFileClose_kfile(void *filp)
{
	if (filp)
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))/* linux user space */
		fclose((FILE *) filp);
#elif defined(linux)
		file_close((struct file *)filp);
#endif

}

void g3dKmdVfprintf_kfile(void *filp, const char *pszFormat, va_list args)
{
	if (!filp)
		return;

#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))/* linux user space */
	vfprintf((FILE *) filp, pszFormat, args);
#elif defined(linux)
	{
		int len;
		char *data;

		len = snprintf(NULL, 0, pszFormat, args);
		/* data = (char*)vmalloc(len + 1); */
		data = (char *)G3DKMDVMALLOC(len + 1);
		if (!data) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FILEIO, "vmalloc fail (size %d)\n", len + 1);
			return;
		}
		len = vsnprintf(data, len + 1, pszFormat, args);

		file_write((struct file *)filp, data, len);

		/* vfree(data); */
		G3DKMDVFREE(data);
	}
#endif
}

void g3dKmdFprintf_kfile(void *filp, const char *pszFormat, ...)
{
	if (filp) {
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))	/* linux user space */
		va_list args;

		va_start(args, pszFormat);

		vfprintf((FILE *) filp, pszFormat, args);

		va_end(args);
#elif defined(linux)
		int len;
		char *data;
		va_list args;

		va_start(args, pszFormat);

/* len = snprintf(NULL, 0, pszFormat, args); */
		len = vsnprintf(NULL, 0, pszFormat, args);

		/* data = (char*)vmalloc(len + 1); */
		data = (char *)G3DKMDVMALLOC(len + 1);
		if (!data) {
			KMDDPF(G3DKMD_LLVL_ERROR | G3DKMD_MSG_FILEIO, "vmalloc fail (size %d)\n", len + 1);
			return;
		}

		len = vsnprintf(data, len + 1, pszFormat, args);
		file_write((struct file *)filp, data, len);

		/* vfree(data); */
		G3DKMDVFREE(data);
		va_end(args);
#endif
	}
}

int g3dKmdFileWrite_kfile(void *filp, unsigned char *data, unsigned int size)
{
	if (filp) {
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))	/* linux user space */
		return fwrite(data, 1, size, (FILE *) filp);
#elif defined(linux)
		return file_write((struct file *)filp, data, size);
#endif
	}

	return -1;
}

void g3dKmdFileRemove_kfile(const char *path)
{
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))	/* linux user space */
	remove(path);
#elif defined(linux)
	file_remove(path);
#endif
}
#endif /* PATTERN_DUMP */




#if defined(G3DKMDDEBUG) || defined(G3DKMDDEBUG_ENG)
G3Duint g_g3dkmd_debug_loglevel = G3DKMD_LLVL_DEFALUT;
G3Duint g_g3dkmd_debug_flags = G3DKMD_MSG_DEFALUT;

G3Duint *g3dKmd_GetKMDDebugFlagsPtr(void)
{
	return &g_g3dkmd_debug_flags;
}

G3Duint *g3dKmd_GetKMDDebugLogLevelPtr(void)
{
	return &g_g3dkmd_debug_loglevel;
}

#endif


#if defined(G3DKMDDEBUG) || defined(G3DKMDDEBUG_ENG)
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))	/* linux user space */
#ifdef PRINT2FILE
FILE *g3dkmdFileHandle = NULL;
#endif
#else
#ifdef PRINT2FILE
struct file *g3dkmdFileHandle = NULL;
#endif
/* static int   gStartPos = 0; */
#endif
void g3dKmd_openDebugFile(void)
{
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))	/* linux user space */
#ifdef PRINT2FILE
	if (g3dkmdFileHandle == NULL)
		g3dkmdFileHandle = fopen("g3dkmd_debug.log", "w+");
#endif
#else
#ifdef PRINT2FILE
	if (g3dkmdFileHandle == NULL) {
		g3dkmdFileHandle = g_pKmdFileIo->fileOpen("/tmp/g3dkmd_debug.log");
/* g3dkmdFileHandle = file_open("/tmp/g3dkmd_debug.log"); */
	}
#endif
#endif

}

void g3dKmd_closeDebugFile(void)
{
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))	/* linux user space */
#ifdef PRINT2FILE
	if (g3dkmdFileHandle)
		fclose(g3dkmdFileHandle);
	g3dkmdFileHandle = NULL;
#endif
#else
	/* pr_debug("g3dKmd_closeDebugFile = 0x%x\n", g3dkmdFileHandle); */
#ifdef PRINT2FILE
	if (g3dkmdFileHandle) {
		g_pKmdFileIo->fileClose(g3dkmdFileHandle);
/* file_close(g3dkmdFileHandle); */
	}
	g3dkmdFileHandle = 0;
#endif
#endif
}

static void _g3dkmd_debug_print(unsigned int loglvl, unsigned int type, const char *format, va_list args)
{
	static char string[512];
	int size = 0;

	switch (loglvl) {
	case G3DKMD_LLVL_ERROR:
		size += sprintf(string + size, "(G3DERROR) ");
		break;

	case G3DKMD_LLVL_WARNING:
		size += sprintf(string + size, "(G3DWARNING) ");
		break;

	case G3DKMD_LLVL_INFO:
		size += sprintf(string + size, "(G3DCMD) ");
		break;

	case G3DKMD_LLVL_NOTICE:
		size += sprintf(string + size, "(G3DNOTICE) ");
		break;

	default:
		break;
	}

	switch (type) {
	case G3DKMD_MSG_IOCTL:
		size += sprintf(string + size, "(G3DKMDIOCTL) ");
		break;

	default:
		size += sprintf(string + size, "(%x) ", type);
		break;
	}

	size += vsprintf(string + size, format, args);
#if defined(_WIN32) || (defined(linux) && defined(linux_user_mode))	/* linux user space */
#ifdef PRINT2FILE
	if (g3dkmdFileHandle) {
		fprintf(g3dkmdFileHandle, "%s", string);
		fflush(g3dkmdFileHandle);
	}
#else
#ifdef _WIN32
	OutputDebugString(string);
#else
	fprintf(stderr, "%s", string);
#endif
#endif
#else
	/* output : /var/log/messages */
	switch (loglvl) {
	case G3DKMD_LLVL_ERROR:
		pr_err("%s", string);
		break;

	case G3DKMD_LLVL_WARNING:
		pr_warn("%s", string);
		break;

	case G3DKMD_LLVL_INFO:
		pr_debug("%s", string);
		break;

	case G3DKMD_LLVL_NOTICE:
		pr_debug("%s", string);
		break;

	default:
		pr_debug("%s", string);
		break;
	}



	/* pr_debug("A size = %d, f_pos = 0x%x\n", size, g3dkmdFileHandle->f_pos); */
#ifdef PRINT2FILE
	if (g3dkmdFileHandle) {
		/* int len = */
		/* file_write(g3dkmdFileHandle, string, size); */
		g3dKmdFileWrite_kfile(g3dkmdFileHandle, string, size);
		/* pr_debug("size = %d, f_pos = 0x%x\n", size, g3dkmdFileHandle->f_pos); */
	}
#endif
#endif
}

void g3dKmdDebugPrint(unsigned int type, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	_g3dkmd_debug_print(type & G3DKMD_LLVL_MASK, type & G3DKMD_MSG_MASK, format, args);

}
#endif /* G3DKMDDEBUG */
