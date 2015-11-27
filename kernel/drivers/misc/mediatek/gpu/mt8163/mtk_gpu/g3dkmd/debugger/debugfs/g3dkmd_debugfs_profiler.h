#ifndef _G3DKMD_DEBUGFS_PROFILER_H_
#define _G3DKMD_DEBUGFS_PROFILER_H_
#include <linux/debugfs.h>



// register to debugfs creation function.
void g3dKmd_debugfs_profiler_create(struct dentry *reg_dir);

// clean up function.
void g3dKmd_debugfs_profiler_destroy(void);

#endif //_G3DKMD_DEBUGFS_PROFILER_H_
