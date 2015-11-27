#include <linux/debugfs.h>


#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#include "g3dkmd_debugfs_log.h"
#include "g3dkmd_file_io.h"      // for KMDDPF()




/*****************************************************************************
 * Areg Dump by debugfs
 *****************************************************************************/

void g3dKmd_debugfs_Log_create(struct dentry *klog_dir)
{
    struct dentry *kmddpf_dir;

    kmddpf_dir = debugfs_create_dir("KMDDPF", klog_dir);

    debugfs_create_x32("logfilter", (S_IWUGO|S_IRUGO|S_IWUSR),
        kmddpf_dir, g3dKmd_GetKMDDebugFlagsPtr());
    debugfs_create_u32("loglevel", (S_IWUGO|S_IRUGO|S_IWUSR),
        kmddpf_dir, g3dKmd_GetKMDDebugLogLevelPtr());
}

void g3dKmd_debugfs_Log_destroy(void)
{
    return ;
}
