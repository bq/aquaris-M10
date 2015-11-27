/*
 * (c) MediaTek Inc. 2010
 */


#ifndef __M4U_H__
#define __M4U_H__

#include "m4u_port.h"
#include "g3dkmd_api_common.h"
#if defined(YL_MMU_PATTERN_DUMP_V1) || defined(YL_MMU_PATTERN_DUMP_V2)
#include "mmu/yl_mmu_mapper_public_class.h"
#endif

#if defined(WIN32) || (defined(linux) && defined(linux_user_mode))
struct sg_table
{
    unsigned int reserved;
};
#else
//#include <linux/ioctl.h>
//#include <linux/fs.h>
//#include <mach/m4u_port.h>
#include <linux/scatterlist.h>
#endif

typedef int M4U_PORT_ID;

#define M4U_PROT_READ   (1<<0)
#define M4U_PROT_WRITE  (1<<1)
#define M4U_PROT_CACHE  (1<<2)
#define M4U_PROT_SHARE  (1<<3)
#define M4U_PROT_SEC    (1<<4)

#define M4U_FLAGS_SEQ_ACCESS (1<<0)
#define M4U_FLAGS_FIX_MVA   (1<<1)


typedef enum
{
	RT_RANGE_HIGH_PRIORITY=0,
	SEQ_RANGE_LOW_PRIORITY=1
} M4U_RANGE_PRIORITY_ENUM;


// port related: virtuality, security, distance
typedef struct _M4U_PORT
{  
	M4U_PORT_ID ePortID;		   //hardware port ID, defined in M4U_PORT_ID
	unsigned int Virtuality;						   
	unsigned int Security;
    unsigned int domain;            //domain : 0 1 2 3
	unsigned int Distance;
	unsigned int Direction;         //0:- 1:+
}M4U_PORT_STRUCT;



typedef enum
{
    M4U_CACHE_CLEAN_BY_RANGE,
    M4U_CACHE_INVALID_BY_RANGE,
    M4U_CACHE_FLUSH_BY_RANGE,

    M4U_CACHE_CLEAN_ALL,
    M4U_CACHE_INVALID_ALL,
    M4U_CACHE_FLUSH_ALL,
} M4U_CACHE_SYNC_ENUM;

typedef struct
{
#if defined(YL_MMU_PATTERN_DUMP_V1) || defined(YL_MMU_PATTERN_DUMP_V2)
    MapperRec mmu;
#else
    void* mmu;
#endif
    //struct mutex dataMutex;
    //pid_t open_pid;
    //pid_t open_tgid;
    //struct list_head mvaList;
} m4u_client_t;

int m4u_dump_info(int m4u_index);
int m4u_power_on(int m4u_index);
int m4u_power_off(int m4u_index);

G3DKMD_APICALL int G3DAPIENTRY g3d_m4u_alloc_mva(m4u_client_t *client, M4U_PORT_ID port, 
                  void* va, struct sg_table *table, 
                  unsigned int size, unsigned int prot, unsigned int flags,
                  unsigned int *pMva);

G3DKMD_APICALL int G3DAPIENTRY g3d_m4u_dealloc_mva(m4u_client_t *client, M4U_PORT_ID port, unsigned int mva, unsigned int size);

                                                                                             
G3DKMD_APICALL int G3DAPIENTRY m4u_config_port(M4U_PORT_STRUCT* pM4uPort);
int m4u_monitor_start(int m4u_id);
int m4u_monitor_stop(int m4u_id);


int m4u_cache_sync(m4u_client_t *client, M4U_PORT_ID port, 
                unsigned int va, unsigned int size, unsigned int mva,
                M4U_CACHE_SYNC_ENUM sync_type);

int m4u_mva_map_kernel(unsigned int mva, unsigned int size, 
        unsigned int *map_va, unsigned int *map_size);
int m4u_mva_unmap_kernel(unsigned int mva, unsigned int size, unsigned int va);
m4u_client_t * g3d_m4u_create_client(void);
int g3d_m4u_destroy_client(m4u_client_t *client);




typedef enum m4u_callback_ret 
{
    M4U_CALLBACK_HANDLED,
    M4U_CALLBACK_NOT_HANDLED,
}m4u_callback_ret_t;

typedef m4u_callback_ret_t (m4u_reclaim_mva_callback_t)(int alloc_port, unsigned int mva, 
                            unsigned int size, void* data);
int m4u_register_reclaim_callback(int port, m4u_reclaim_mva_callback_t *fn, void* data);
int m4u_unregister_reclaim_callback(int port);

typedef m4u_callback_ret_t (m4u_fault_callback_t)(int port, unsigned int mva, void* data);
int m4u_register_fault_callback(int port, m4u_fault_callback_t *fn, void* data);
int m4u_unregister_fault_callback(int port);

// sapphire defined API
G3DKMD_APICALL unsigned int G3DAPIENTRY m4u_mva_to_va(m4u_client_t *client, M4U_PORT_ID port, unsigned int mva);

typedef void (m4u_regwrite_callback_t)(unsigned int reg, unsigned int value);

G3DKMD_APICALL void G3DAPIENTRY m4u_register_regwirte_callback(m4u_client_t *client, m4u_regwrite_callback_t *fn);

G3DKMD_APICALL void G3DAPIENTRY m4u_get_pgd(m4u_client_t* client, M4U_PORT_ID port, void** pgd_va, void** pgd_pa, unsigned int* size);
G3DKMD_APICALL unsigned long G3DAPIENTRY m4u_mva_to_pa(m4u_client_t* client, M4U_PORT_ID port, unsigned int mva);


// m4u driver internal use ---------------------------------------------------
//

#endif




