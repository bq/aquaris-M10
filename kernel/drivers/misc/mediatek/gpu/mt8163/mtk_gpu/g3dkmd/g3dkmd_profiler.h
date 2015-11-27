#ifndef _G3DKMD_PROFILER_H_
#define _G3DKMD_PROFILER_H_

#if defined(linux) && !defined(linux_user_mode) // linux kernel
   #include <linux/seq_file.h>
#endif

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dbase_type.h"          // for G3Duint

#if defined(G3D_PROFILER)
enum {
    LOG_LEVEL_NONE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_PROFILE,
};

enum {
  #if defined(G3D_PROFILER_RINGBUFFER_STATISTIC)
    RINGBUFFER_PROFIELR_PRIVATE_DATA_IDX,
  #endif
  #if defined(G3D_KMD_MEMORY_STATISTIC)
    KMEMORY_PROFILER_PRIVATE_DATA_IDX,
  #endif
    MAX_PROFILER_PRIVATE_DATA_IDX
};

typedef struct _g3d_profiler_private_data G3d_profiler_private_data, *pG3d_profiler_private_data;

#define PROFILER_PRINT_LOG_OR_SEQ(seq_file, fmt, args...) \
    do { \
        if (seq_file) { \
            seq_printf(seq_file, fmt, ##args); \
        } else { \
            printk(fmt, ##args); \
        } \
    } while(0);


uintptr_t _g3dKmdGetDevicePrivateDataPtr(uintptr_t prflr_private_data, int idx);
uintptr_t _g3dKmdGetDevicePrivateData(uintptr_t prflr_private_data, int idx);
void _g3dKmdProfilerDeviceInit(uintptr_t *pdevice_profiler_private_data);
void _g3dKmdProfilerDeviceTerminate(uintptr_t device_profiler_private_data);

void _g3dKmdProfilerModuleInit(void);
void _g3dKmdProfilerModuleTerminate(void);
#else
  #define _g3dKmdProfilerDeviceInit(...)
  #define _g3dKmdProfilerDeviceTerminate(...)
  #define _g3dKmdProfilerModuleInit(...)
  #define _g3dKmdProfilerModuleTerminate(...)
#endif

#endif //_G3DKMD_PROFILER_H_
