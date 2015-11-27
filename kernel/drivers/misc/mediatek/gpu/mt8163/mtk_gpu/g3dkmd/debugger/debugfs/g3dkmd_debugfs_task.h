#ifndef _G3DKMD_DEBUGFS_TASK_H_
#define _G3DKMD_DEBUGFS_TASK_H_

#include <linux/debugfs.h>

void g3dKmd_debugfs_task_create(struct dentry *task_dir);
void g3dKmd_debugfs_task_destroy(void);

#endif //_G3DKMD_DEBUGFS_H_
