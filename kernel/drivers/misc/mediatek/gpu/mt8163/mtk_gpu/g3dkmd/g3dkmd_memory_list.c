#if defined(linux) && !defined(linux_user_mode) // linux kernel
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/slab.h>     // kmalloc(), cache()
#endif

#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"
#include "g3dbase_type.h"
#include "g3dkmd_macro.h" // for UNUSED()

#include "g3d_memory.h"
#include "platform/g3dkmd_platform_list.h"
#include "g3dkmd_file_io.h"
#include "g3dkmd_util.h"        // _*_SEM_*LOCK()

#include "g3dkmd_memory_list.h"

#if 0
#ifdef  KMDDPF
#undef  KMDDPF
#define KMDDPF(flgs, fmt, ...) printk("[MEMLIST] %s" fmt, __FUNCTION__, ##__VA_ARGS__)
#endif
#endif


#if defined(ENABLE_G3DMEMORY_LINK_LIST)

#if defined(linux) && !defined(linux_user_mode) // linux kernel
#define MEMLIST_PRINT_LOG_OR_SEQ(seq_file, fmt, args...) \
    do { \
        if (seq_file) { \
            seq_printf(seq_file, fmt, ##args); \
        } else { \
            printk(fmt, ##args); \
        } \
    } while(0);
#else
    #define MEMLIST_PRINT_LOG_OR_SEQ(...)
#endif

typedef struct _memlist_data_
{
    g3dkmd_list_head_t  *memlist_headp;
    long                totalSize;
    int                 nMemory;

    struct kmem_cache   *memlist_cachep;
} G3dMemoryList_Data;


typedef struct _memlist_node_
{
    // list
    g3dkmd_list_head_t  list;
    // key
    void*               fd;
    // data
    G3Dint              pid;
    void*               va;
    G3Duint             size;
    G3Duint             hwAddr;
    G3dMemory*          pMemory;
    G3d_Mem_HEAP_Type   heap_type;
} G3DMemoryListNode, *pG3DMemoryListNode;


static G3DKMD_LIST_HEAD(g_g3dMemoryListHead);
__DEFINE_SEM_LOCK(memlist_lock);

G3dMemoryList_Data g_memlist_data =
{
    &g_g3dMemoryListHead,   // list
    0,                      // totoal size
    0,                      // n Memory
    NULL                    // keme_cache
};


static inline g3dkmd_list_head_t * __getMemListHead(void)
{
    return g_memlist_data.memlist_headp;
}


static inline pG3DMemoryListNode __findNode(void * va_key)
{
    pG3DMemoryListNode pNode;

    g3dkmd_list_for_each_entry(pNode, __getMemListHead(), list)
    {
        if (pNode->va == va_key)
        {
            return pNode;
        }
    }

    return NULL;
}


static inline pG3DMemoryListNode __addNode(void * fd, G3Dint pid, void *va,
    G3Duint size, G3Duint hwAddr, G3d_Mem_HEAP_Type type)
{
    pG3DMemoryListNode pNode;
    UNUSED(type);
    UNUSED(pNode);

    pNode = (pG3DMemoryListNode) kmem_cache_alloc(
                g_memlist_data.memlist_cachep, GFP_KERNEL);

    G3DKMD_INIT_LIST_HEAD(&(pNode->list));
    pNode->fd           = fd;
    pNode->pid          = pid;
    pNode->va           = va;
    pNode->size         = size;
    pNode->hwAddr       = hwAddr;
    pNode->heap_type    = type;

    g3dkmd_list_add(&(pNode->list), __getMemListHead());
    g_memlist_data.totalSize += size;
    g_memlist_data.nMemory   += 1;

    return pNode;
}


static inline void __delNode(pG3DMemoryListNode pNode)
{
   g_memlist_data.totalSize -= pNode->size;
   g_memlist_data.nMemory   -= 1;

   g3dkmd_list_del(&(pNode->list));

   kmem_cache_free(g_memlist_data.memlist_cachep, pNode);
}


void _g3dKmdMemList_Init(void)
{
    __SEM_LOCK(memlist_lock);
    {
        g_memlist_data.memlist_cachep =
            kmem_cache_create("g3kmd_memlist_cache",
                sizeof(G3DMemoryListNode), 0, 0, NULL);

        KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_MEM,
            "[%s]\n"
            "\tg_g3dMemoryListHead: %p (%p,%p)\n"
            "\tg_memlist_data.head: %p\n",
            __FUNCTION__,
            &g_g3dMemoryListHead,
            g_g3dMemoryListHead.prev,
            g_g3dMemoryListHead.next,
            g_memlist_data.memlist_headp
            );
    }
    __SEM_UNLOCK(memlist_lock);
}


void _g3dKmdMemList_Terminate(void)
{
    __SEM_LOCK(memlist_lock);
    {
        pG3DMemoryListNode pNode, pTmp;

        // cleanup all nodes.
        g3dkmd_list_for_each_entry_safe(pNode, pTmp,
            __getMemListHead(), list)
        {
            __delNode(pNode);
        }

        if (NULL != g_memlist_data.memlist_cachep)
        {
            kmem_cache_destroy(g_memlist_data.memlist_cachep);
        }
    }
    __SEM_UNLOCK(memlist_lock);
}


void _g3dKmdMemList_CleanDeviceMemlist(void *fd)
{
    __SEM_LOCK(memlist_lock);
    {
        pG3DMemoryListNode pNode, pTmp;

        g3dkmd_list_for_each_entry_safe(pNode, pTmp,
            __getMemListHead(), list)
        {
            if (pNode->fd == fd)
            {
                 __delNode(pNode);
            }
        }
    }
    __SEM_UNLOCK(memlist_lock);
}


void _g3dKmdMemList_Add(void *fd, G3Dint pid, void *va, G3Duint size,
                        G3Duint hwAddr, G3d_Mem_HEAP_Type type)
{
    pG3DMemoryListNode pNode;

    __SEM_LOCK(memlist_lock);
    {
        pNode = __addNode(fd, pid, va, size, hwAddr, type);
    }
    __SEM_UNLOCK(memlist_lock);

    KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_MEM,
        "(data: %p, size: %u, hwAddr: 0x%x) pNode: %p",
        va, size, hwAddr, pNode);
}


void _g3dKmdMemList_Del(void *va)
{
    pG3DMemoryListNode pNode;

    __SEM_LOCK(memlist_lock);
    {
        //pNode = (pG3DMemoryListNode) mem->memlist_nodep;

        //if (pNode->va != mem->data0)
        {
            //Something wrong. The G3dMemory is not the same as its allocation.
            //Try to recovery it: Find the coresponding pNode in List.
            //if (NULL == (pNode = __findNode(mem->data0)))
            if (NULL == (pNode = __findNode(va)))
            {
                KMDDPF(G3DKMD_LLVL_WARNING | G3DKMD_MSG_MEM,
                    "Could not found the va(%p) in MemList.\n", va);
                __SEM_UNLOCK(memlist_lock);
                return ;
            }
        }

        KMDDPF(G3DKMD_LLVL_DEBUG | G3DKMD_MSG_MEM,
            "va: %p pNode(va: %p, size: %u, hwAddr: 0x%x) pNode: %p",
            va, pNode->va, pNode->size, pNode->hwAddr, pNode);

        __delNode(pNode);
    }
    __SEM_UNLOCK(memlist_lock);
    return ;
}


G3Duint _g3dKmdMemList_GetTotalMemoryUsage(void)
{
    // [Louis 2015/03/30]: Could not use memlist_lock, due to MET query may in
    //                     interrupt context.
    return g_memlist_data.totalSize;
}

// debug infromation

void _g3dKmdMemList_ListAll(void *seq)
{
    g3dkmd_list_head_t * head;
    pG3DMemoryListNode pNode;

    static char * g3d_mem_heap_type_strs[G3D_Mem_Heap_MAX] =
    {
        "Kernel",
        "User  "
    };

    __SEM_LOCK(memlist_lock);
    {
        head = __getMemListHead();
        MEMLIST_PRINT_LOG_OR_SEQ(seq,
            "============= Memory List Begin ===================\n"
            " Total Size: %lu\n"
            " nMem      : %d\n"
            " --------------------------------------------------\n"
            " Type     pid     va              size      hwAddr\n",
            g_memlist_data.totalSize,
            g_memlist_data.nMemory);

        g3dkmd_list_for_each_entry(pNode, __getMemListHead(), list)
        {
            MEMLIST_PRINT_LOG_OR_SEQ(seq,
//                "Type: %s pid: %5d va: %p size: %8u, hwAddr: 0x%08x\n",
                " %s %5u %p %8u 0x%08x\n",
                g3d_mem_heap_type_strs[pNode->heap_type],
                pNode->pid,
                pNode->va,
                pNode->size,
                pNode->hwAddr);
        }
        MEMLIST_PRINT_LOG_OR_SEQ(seq,
            "============== Memory List End ====================\n");
    }
    __SEM_UNLOCK(memlist_lock);
    return ;
}
#endif //defined(ENABLE_G3DMEMORY_LINK_LIST)
