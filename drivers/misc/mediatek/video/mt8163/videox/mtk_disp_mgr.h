#ifndef __H_MTK_DISP_MGR__
#define __H_MTK_DISP_MGR__
extern unsigned int is_hwc_enabled;
extern int is_DAL_Enabled(void);

long mtk_disp_mgr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
const char *_disp_format_spy(DISP_FORMAT format);

#endif
