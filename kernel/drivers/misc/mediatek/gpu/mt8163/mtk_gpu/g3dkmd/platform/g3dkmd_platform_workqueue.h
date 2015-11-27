#ifndef _G3DKMD_WORK_QUEUE_H_
#define _G3DKMD_WORK_QUEUE_H_

#if defined(linux) && !defined(linux_user_mode)   // linux kernel

/// headers
#include <linux/workqueue.h>


/// struct



/// macro && functions

#define G3DKMD_DECLARE_WORK(work, work_func) \
         DECLARE_WORK(work, work_func)

#define G3DKMD_INIT_WORK(work, work_func) \
         INIT_WORK(work, work_func)

#define g3dKmd_schedule_work(work, work_func) \
         schedule_work(work)

#define g3dKmd_queue_work(workqueue, work, work_func) \
         queue_work(work_queue, work)

#else // WIN or linux_user

/// headers


/// struct
struct work_struct {};


/// macro && functions

#define G3DKMD_DECLARE_WORK (work, work_func) \
          struct work_struct g3dkmd_work_queue_##work

#define g3dKmd_schedule_work(work, work_func) \
         work_func(work)

#define g3dKmd_queue_work( workqueue, work, work_func) \
         work_func(work)


#endif // platforms

#endif // _G3DKMD_WORK_QUEUE_H_

