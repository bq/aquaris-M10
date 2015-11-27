#ifndef _G3DKMD_DEBUGFS_AREGS_H_
#define _G3DKMD_DEBUGFS_AREGS_H_
#include <linux/debugfs.h>



// register to debugfs creation function.
void g3dKmd_debugfs_Areg_create(struct dentry *reg_dir);

// clean up function.
void g3dKmd_debugfs_Areg_destroy(void);

#endif //_G3DKMD_DEBUGFS_AREGS_H_


