#ifndef _G3DKMD_BUFFERFILE_IO_H_
#define _G3DKMD_BUFFERFILE_IO_H_

#include <stdarg.h>
#include <linux/fs.h>              // for struct file
#include "g3dbase_common_define.h" // for define G3DKMD_USE_BUFFERFILE 


/// File IO used in KMD mode
void* g3dKmdFileOpen_bufferfile(const char* path);
void g3dKmdFileClose_bufferfile(void* filp);

void g3dKmdFprintf_bufferfile(void* filp, const char* pszFormat, ...);
void g3dKmdVfprintf_bufferfile(void* filp, const char* pszFormat, va_list args);
int g3dKmdFileWrite_bufferfile(void* filp, unsigned char* data, unsigned int size);

// NOT implemented.
void g3dKmdFileRemove_bufferfile(const char* path);


/// Dump file from User space.
// User Asking Dumpping Bufferfiles to file system.
void _g3dKmdBufferFileDeviceOpenInit(struct file *filp);
void _g3dKmdBufferFileDeviceRelease(struct file *filp);
int  _g3dKmdIsBufferFileMmap(struct file *filp, uintptr_t offsetAddr);

/// G3DKMD APIs
void _g3dKmdBufferFileDumpBegin(struct file *filp, void ** root, int * pCount);
void _g3dKmdBufferFileDumpEnd(struct file *filp, bool isFreeWrittenFiles);

//  mmap
int _g3dkmdBufferFileMmap(struct file *filp, struct vm_area_struct *vma);


#endif //_G3DKMD_BUFFERFILE_H_
