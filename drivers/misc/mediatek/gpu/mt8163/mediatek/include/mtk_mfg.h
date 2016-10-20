#ifndef _MTK_MFG_H_
#define _MTK_MFG_H_

void mfg_dump_regs(const char * prefix);

void mtk_mfg_enable_gpu(void);

void mtk_mfg_disable_gpu(void);

void mtk_mfg_enable_pll(void);

void mtk_mfg_disable_pll(void);

void mtk_mfg_wait_gpu_idle(void);

unsigned long mtk_mfg_get_frequence(void);

bool mtk_mfg_is_ready(void);

#endif
