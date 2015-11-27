
#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dbase_type.h"          // for G3Duint
#include "g3dkmd_util.h"           // for __DEFINE_SEM_LOCK
#include "g3dkmd_file_io.h"        // for KMDDPF
#include "profiler/g3dkmd_prf_kmemory.h"

#ifndef MAX
  #define MAX(a,b) ((a)>(b))? (a):(b)
#endif  // MAX



/// debug message printer

  #define L_MSG_NONE      0x0
  #define L_MSG_INTERFACE 0x1
  #define L_MSG_ERROR     0x2
  #define L_MSG_WARNING   0x4
  #define L_MSG_DEBUG     0x8


  #define USE_LOCAL_PRINTK  0
  #define LOCAL_TAG         "(Prf_KMem)"
  #define L_MSG_FLAGS       (L_MSG_NONE)
  //#define L_MSG_FLAGS       (L_MSG_INTERFACE | L_MSG_ERROR)
  //#define L_MSG_FLAGS       (L_MSG_ERROR | L_MSG_WARNING | L_MSG_DEBUG)
  //#define L_MSG_FLAGS       (L_MSG_INTERFACE | L_MSG_ERROR | L_MSG_WARNING | L_MSG_DEBUG)

  #define L_CHECKFLAG(f)  ((L_MSG_FLAGS) & (f))

  #if USE_LOCAL_PRINTK
    #define LPRINTER(g3dflag, flag, tag, ...) do{ if(L_CHECKFLAG(flag)) { printk(          KERN_INFO LOCAL_TAG #tag __VA_ARGS__);}} while(0)
  #else
    #define LPRINTER(g3dflag, flag, tag, ...) do{ if(L_CHECKFLAG(flag)) { KMDDPF( g3dflag, KERN_INFO LOCAL_TAG #tag __VA_ARGS__);}} while(0)
  #endif

  #define KMDPF_E(...) do { LPRINTER(G3DKMD_LLVL_ERROR | G3DKMD_MSG_PROFILER, L_MSG_ERROR,     "[E] ", __VA_ARGS__); } while(0)
  #define KMDPF_I(...) do { LPRINTER(G3DKMD_LLVL_INFO  | G3DKMD_MSG_PROFILER, L_MSG_INTERFACE, "[I] ", __VA_ARGS__); } while(0)
  #define KMDPF_D(...) do { LPRINTER(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_PROFILER, L_MSG_DEBUG,     "[D] ", __VA_ARGS__); } while(0)


/// Implmentations
#if defined(G3D_KMD_MEMORY_STATISTIC)
struct _memory_usage_prfiler_data
{
    G3Duint log_level;
//    G3Duint pid;
    G3Duint max_usage;
    G3Duint current_usage;
};

#endif //defined(G3D_KMD_MEMORY_STATISTIC)

#if defined(G3D_KMD_MEMORY_STATISTIC)
__DEFINE_SEM_LOCK(g_mem_usage_lock);
static G3d_profiler_memory_usage_data  g_memory_usage_data;

static inline pG3d_profiler_memory_usage_data _getInstanceKMemoryUsage(void)
{
    return &g_memory_usage_data;
}

static inline G3Duint _getMaxKMemoryUsage(pG3d_profiler_memory_usage_data p)
{ 
    G3Duint ret;

    __SEM_LOCK(g_mem_usage_lock);
    ret = p->max_usage; 
    __SEM_UNLOCK(g_mem_usage_lock);
    return ret;
}

static inline G3Duint _getCurrentKMemoryUsage(pG3d_profiler_memory_usage_data p)
{
    G3Duint ret;

    __SEM_LOCK(g_mem_usage_lock);
    ret = p->current_usage; 
    __SEM_UNLOCK(g_mem_usage_lock);
    return ret;
}

static G3Duint * _getLogLevelVariablePointerKMemoryUsage(pG3d_profiler_memory_usage_data p)
{
    return &(p->log_level);
}

static void           _initKMemoryUsage(pG3d_profiler_memory_usage_data p)
{ 
    __SEM_LOCK(g_mem_usage_lock);
    {
        p->log_level     = LOG_LEVEL_NONE;
//        p->log_level     = LOG_LEVEL_PROFILE;
        p->max_usage     = 0;
        p->current_usage = 0;
    }
    __SEM_UNLOCK(g_mem_usage_lock);
}

static void           _allocKMemoryUsage(pG3d_profiler_memory_usage_data p, const G3Duint size)
{
    __SEM_LOCK(g_mem_usage_lock);
    {
        p->current_usage += size;
        p->max_usage     = MAX(p->max_usage, p->current_usage);
    }
    __SEM_UNLOCK(g_mem_usage_lock);
}

static void           _freeKMemoryUsage(pG3d_profiler_memory_usage_data p, const G3Duint size)
{
    __SEM_LOCK(g_mem_usage_lock);
    {
        p->current_usage -= size;
    }
    __SEM_UNLOCK(g_mem_usage_lock);
}

static void           _reportKmdMemoryUsage(void* seq, const pG3d_profiler_memory_usage_data pMemoryUsage)
{
    if (pMemoryUsage->log_level >= LOG_LEVEL_PROFILE)
    {
        PROFILER_PRINT_LOG_OR_SEQ(seq, "[Profiler] Kernel Memory Usage Current/Max: %u / %u\n", pMemoryUsage->current_usage, pMemoryUsage->max_usage);
    }
}

void _g3dKmdKMemoryUsageDeviceInit(pG3d_profiler_memory_usage_data pMemoryUsage)
{
    KMDPF_I("%s(pMemory: %p)\n", __FUNCTION__, pMemoryUsage);
}

void _g3dKmdKMemoryUsageDeviceTerminate(pG3d_profiler_memory_usage_data pMemoryUsage) 
{
   KMDPF_I("%s(pMemory: %p)\n", __FUNCTION__, pMemoryUsage);
    _reportKmdMemoryUsage(NULL, _getInstanceKMemoryUsage());
}


void _g3dKmdKMemoryUsageModuleInit(void) 
{
    KMDPF_I("%s()\n", __FUNCTION__);
    _initKMemoryUsage(_getInstanceKMemoryUsage());
}

void _g3dKmdKMemoryUsageModuleTerminate(void) 
{
    KMDPF_I("%s()\n", __FUNCTION__);

    _reportKmdMemoryUsage(NULL, _getInstanceKMemoryUsage());
}


void _g3dKmdProfilerKMemoryUsageReport(struct seq_file * seq)
{
    KMDPF_I("%s(seq: %p)\n", __FUNCTION__, seq);
    _reportKmdMemoryUsage(seq, _getInstanceKMemoryUsage());
}


void __g3dKmdProfilerKMemoryUsageAlloc(const G3Duint size, const G3Denum resType, const G3Duint pid, const G3Dchar *caller_name)
{
    (void) caller_name;
    KMDPF_I("%s(size:%u, resType: %u, pid: %u)\n", caller_name, size, resType, pid);
    _allocKMemoryUsage(_getInstanceKMemoryUsage(), size);
}

void __g3dKmdProfilerKMemoryUsageFree (const G3Duint size, const G3Denum resType, const G3Duint pid, const G3Dchar *caller_name)
{
    (void) caller_name;
    KMDPF_I("%s(size:%u, resType: %u, pid: %u)\n", caller_name, size, resType, pid);
    _freeKMemoryUsage(_getInstanceKMemoryUsage(), size);
}



G3Duint * _g3dKmdProfilerKMemoryUsageGetLogLevelVariablePointer(void)
{
    KMDPF_I("%s()\n", __FUNCTION__);
    return _getLogLevelVariablePointerKMemoryUsage(_getInstanceKMemoryUsage());
}

#endif   //(G3D_KMD_MEMORY_STATISTIC)
