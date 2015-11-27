#ifndef _G3DKMD_CFG_H_
#define _G3DKMD_CFG_H_

typedef enum
{
    CFG_LTC_ENABLE,

    CFG_MMU_ENABLE,
    CFG_MMU_VABASE,

    CFG_DEFER_VARY_SIZE,
    CFG_DEFER_BINHD_SIZE,
    CFG_DEFER_BINBK_SIZE,
    CFG_DEFER_BINTB_SIZE,

    CFG_DUMP_ENABLE,
    CFG_DUMP_START_FRM,
    CFG_DUMP_END_FRM,
    CFG_DUMP_CE_MODE,

    CFG_GCE_ENABLE,

    CFG_PHY_HEAP_ENABLE, //the way physical address generation (for pattern dump)

	CFG_MEM_SHIFT_SIZE,	// in MB

    CFG_VX_BS_HD_TEST,

    CFG_CQ_PMT_MD,
  
    CFG_BKJMP_EN,
    
    CFG_SECURE_ENABLE,

    CFG_UXCMM_REG_DBG_EN,
    
    CFG_UXDWP_REG_ERR_DETECT,

    CFG_MAX,
} CFG_ENUM;

void g3dKmdCfgInit(void);
void g3dKmdCfgSet(CFG_ENUM, unsigned int value);
unsigned int g3dKmdCfgGet(CFG_ENUM);

#endif //_G3DKMD_CFG_H_
