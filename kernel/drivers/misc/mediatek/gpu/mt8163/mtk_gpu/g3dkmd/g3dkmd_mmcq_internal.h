#ifndef _G3DKMD_MMCQ_INTERNAL_H_
#define _G3DKMD_MMCQ_INTERNAL_H_

#include "g3dkmd_mmcq.h"
#include "g3dkmd_mmce.h"
#include "g3d_memory.h"

#define MMCE_TO_PHY(buf, addr)  (((uintptr_t)(addr) - (uintptr_t)(buf)->base1_vir) + (buf)->base1_phy)
#define MMCE_TO_VIR(buf, addr)  (((unsigned int)(addr) - (buf)->base1_phy) + (uintptr_t)(buf)->base1_vir)

typedef struct _MMCQ_BUF
{
    unsigned int*   read_ptr;
    unsigned int*   write_ptr;

    G3dMemory       buf_data;

    void*           base0_vir;
    void*           end0_vir;
    unsigned int    base0_phy;
    unsigned int    end0_phy;
    void*           base1_vir;
    void*           end1_vir;
    unsigned int    base1_phy;
    unsigned int    end1_phy;
} MMCQ_BUF;

typedef struct _MMCQ_INST
{
    MMCQ_STATE      state;
    MMCE_THD_HANDLE thd_handle;
    MMCQ_BUF        inst_buf;   
    unsigned int    backup_pc_phy;
    MMCQ_IS_SUSPENDABLE is_suspendable;
} MMCQ_INST;

#endif // _G3DKMD_MMCQ_INTERNAL_H_

