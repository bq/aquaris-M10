#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"

#ifdef G3DKMD_SUPPORT_FENCE
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/slab.h>
#include "g3dkmd_isr.h"
#include "g3dkmd_fence.h"
#include "g3dkmd_task.h"
#include "g3dkmd_util.h"
#include "g3dkmd_file_io.h"

extern g3dExecuteInfo gExecInfo;

#ifdef SW_SYNC64
struct sw_sync64_timeline *g3dkmd_timeline_create(const char *name)
{
    return sw_sync64_timeline_create(name);
}

void g3dkmd_timeline_destroy(struct sw_sync64_timeline *obj)
{
    sync_timeline_destroy(&obj->obj);
}

void g3dkmd_timeline_inc(struct sw_sync64_timeline *obj, u64 value)
{
    sw_sync64_timeline_inc(obj, value);
}

long g3dkmd_fence_create(struct sw_sync64_timeline *obj, struct fence_data *data)
{
    int fd = get_unused_fd();
    int err;
    struct sync_pt *pt;
    struct sync_fence *fence;

    if (fd < 0)
        return fd;

    pt = sw_sync64_pt_create(obj, data->value);
    if (pt == NULL)
    {
        err = -ENOMEM;
        goto err;
    }

    data->name[sizeof(data->name) - 1] = '\0';
    fence = sync_fence_create(data->name, pt);
    if (fence == NULL)
    {
        sync_pt_free(pt);
        err = -ENOMEM;
        goto err;
    }

    data->fence = fd;

    sync_fence_install(fence, fd);

    return 0;

err:
    put_unused_fd(fd);
    return err;
}
#else
struct sw_sync_timeline *g3dkmd_timeline_create(const char *name)
{
    return sw_sync_timeline_create(name);
}

void g3dkmd_timeline_destroy(struct sw_sync_timeline *obj)
{
    sync_timeline_destroy(&obj->obj);
}

void g3dkmd_timeline_inc(struct sw_sync_timeline *obj, u32 value)
{
    sw_sync_timeline_inc(obj, value);
}

long g3dkmd_fence_create(struct sw_sync_timeline *obj, struct fence_data *data)
{
    int fd = get_unused_fd();
    int err;
    struct sync_pt *pt;
    struct sync_fence *fence;

    if (fd < 0)
        return fd;

    pt = sw_sync_pt_create(obj, data->value);
    if (pt == NULL)
    {
        err = -ENOMEM;
        goto err;
    }

    data->name[sizeof(data->name) - 1] = '\0';
    fence = sync_fence_create(data->name, pt);
    if (fence == NULL)
    {
        sync_pt_free(pt);
        err = -ENOMEM;
        goto err;
    }

    data->fence = fd;

    sync_fence_install(fence, fd);

    return 0;

err:
    put_unused_fd(fd);
    return err;
}
#endif

int g3dkmd_fence_merge(char * const name, int fd1, int fd2)
{
    int fd = get_unused_fd();
    int err;
    struct sync_fence *fence1, *fence2, *fence3;

    if (fd < 0)
        return fd;

    fence1 = sync_fence_fdget(fd1);
    if (NULL == fence1)
    {
        err = -ENOENT;
        goto err_put_fd;
    }

    fence2 = sync_fence_fdget(fd2);
    if (NULL == fence2)
    {
        err = -ENOENT;
        goto err_put_fence1;
    }

    name[sizeof(name) - 1] = '\0';
    fence3 = sync_fence_merge(name, fence1, fence2);
    if (fence3 == NULL)
    {
        err = -ENOMEM;
        goto err_put_fence2;
    }

    sync_fence_install(fence3, fd);
    sync_fence_put(fence2);
    sync_fence_put(fence1);

    return fd;

err_put_fence2:
    sync_fence_put(fence2);

err_put_fence1:
    sync_fence_put(fence1);

err_put_fd:
    put_unused_fd(fd);
    return err;
}

inline int g3dkmd_fence_wait(struct sync_fence *fence, int timeout)
{
    return sync_fence_wait(fence, timeout);
}

void g3dkmd_create_task_timeline(int taskID, pG3dTask task)
{
    const char str_timeline_tmpl[] = "task%03d_timeline";
    unsigned int strSize;
    char timelinename[20];

    strSize = snprintf(NULL, 0, str_timeline_tmpl, taskID);
    YL_KMD_ASSERT(strSize <= 20);
    snprintf(timelinename, strSize+1, str_timeline_tmpl, taskID);
    task->timeline = g3dkmd_timeline_create(timelinename);
    
#ifdef G3D_NATIVEFENCE64_TEST
    task->timeline->value = 0xFFFFF000;
    task->lasttime = 0xFFFFF000;
#endif

}


void g3dkmdCreateFence(int taskID, int* fd, uint64_t stamp)
{
#ifdef SW_SYNC64
    const char str_fence_tmpl[] = "f_%02x_%08x_%016llx";
#else
    const char str_fence_tmpl[] = "f_%02x_%08x_%08x";
#endif

    unsigned int strSize;
    pG3dTask task;
    struct fence_data data;

    if (fd == NULL)
        return;
    
    *fd = -1;

    lock_g3d();
 
    if (taskID >= gExecInfo.taskInfoCount)
    {
        unlock_g3d();
        return;
    }

    task = g3dKmdGetTask(taskID);
    if (task == NULL)
    {
        unlock_g3d();
        return;
    }

    if (task->timeline == NULL)
    {
        g3dkmd_create_task_timeline(taskID, task);
    }

    if (task->timeline == NULL)
    {
        unlock_g3d();
        return;
    }

#ifdef SW_SYNC64
    data.value = stamp;
#else
    data.value = (__u32)stamp;
#endif

    strSize = snprintf(NULL, 0, str_fence_tmpl, taskID, task->tid, data.value);
    YL_KMD_ASSERT(strSize <= 31);
    
    snprintf(data.name, strSize+1, str_fence_tmpl, taskID, task->tid, data.value);
    

    if (g3dkmd_fence_create(task->timeline, &data) == 0)
        *fd = data.fence;

    unlock_g3d();
}

int g3dkmdFenceStatus(int fd)
{
    struct sync_fence *fence;
   
    if (fd < 0)
        return -EINVAL;

    fence = sync_fence_fdget(fd);
    if (NULL == fence)
        return -ENOENT;

    // @status:		1: signaled, 0:active, <0: error
    return fence->status;    
}

void g3dkmdUpdateTimeline(unsigned int taskID, unsigned int fid)
{
    pG3dTask task;
    unsigned int old_lasttime, delta;
    unsigned int maxStampValue = 0xFFFF;

#ifdef G3DKMD_SUPPORT_ISR
    lock_isr();
#endif

    if (taskID >= gExecInfo.taskInfoCount)
    {
#ifdef G3DKMD_SUPPORT_ISR
        unlock_isr();
#endif
        return;
    }

    task = g3dKmdGetTask(taskID);
    if (task == NULL)
    {
#ifdef G3DKMD_SUPPORT_ISR
        unlock_isr();
#endif
        return;
    }

    if (task->timeline == NULL)
    {
#ifdef G3DKMD_SUPPORT_ISR
        unlock_isr();
#endif
        return;
    }

    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_FENCE,"[g3dkmdUpdateTimeline] taskID %d, lastime 0x%x, fid 0x%d\n", taskID, task->lasttime, fid);


#ifdef G3D_STAMP_TEST
    if (G3D_MAX_STAMP_VALUE < maxStampValue)    
    {
        maxStampValue = G3D_MAX_STAMP_VALUE;
    }
#endif
    
    old_lasttime = task->lasttime & maxStampValue;   
    delta = (fid >= old_lasttime) ? fid - old_lasttime :
                                   fid + (maxStampValue+1) - old_lasttime;

    g3dkmd_timeline_inc(task->timeline, delta);
    task->lasttime += delta;
#ifdef G3DKMD_SUPPORT_ISR
    unlock_isr();
#endif

    YL_KMD_ASSERT((task->lasttime & maxStampValue) == fid);
#ifdef SW_SYNC64
    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_FENCE,"update task[%d] timeline to value(0x%016llx)\n", taskID, task->lasttime);
#else    
    KMDDPF(G3DKMD_LLVL_NOTICE | G3DKMD_MSG_FENCE,"update task[%d] timeline to value(0x%x)\n", taskID, task->lasttime);
#endif
}
#endif // G3DKMD_SUPPORT_FENCE
