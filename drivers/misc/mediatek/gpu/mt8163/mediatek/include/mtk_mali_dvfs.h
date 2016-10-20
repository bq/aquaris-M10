#ifndef _MTK_DVFS_H_
#define _MTK_DVFS_H_


void mtk_mali_dvfs_init(void);
void mtk_mali_dvfs_deinit(void);
void mtk_mali_dvfs_notify_utilization(int utilization) ;
void mtk_mali_dvfs_set_threshold(int down, int up);
void mtk_mali_dvfs_get_threshold(int *down, int *up);
void mtk_mali_dvfs_power_on(void);
void mtk_mali_dvfs_power_off(void);
void mtk_mali_dvfs_enable(void);
void mtk_mali_dvfs_disable(void);

#endif
