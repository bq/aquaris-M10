#ifndef _GPAD_PM_CONFIG_H
#define _GPAD_PM_CONFIG_H

/*
 * Default configuration sample parsing.
 */
#if GPAD_GPU_VER==0x0314
    #define TRIANGLE_CNT(_sample) \
                (_sample)->data[6]
    #define PP_CYCLE(_sample) \
                (_sample)->data[13]
    #define VP_CYCLE(_sample) \
                (_sample)->data[14]
    #define VERTEX_CNT(_sample) \
                (_sample)->data[18]
    #define PIXEL_CNT(_sample) \
                (_sample)->data[24]
#elif GPAD_GPU_VER==0x0530
    #define TRIANGLE_CNT(_sample) \
                (_sample)->data[3]
    #define PP_CYCLE(_sample) \
                (_sample)->data[1]
    #define VP_CYCLE(_sample) \
                (_sample)->data[0]
    #define VERTEX_CNT(_sample) \
                (_sample)->data[9]
    #define PIXEL_CNT(_sample) \
                (_sample)->data[10]
#elif GPAD_GPU_VER==0x0704
    #define TRIANGLE_CNT(_sample) \
                (_sample)->data[3]
    #define PP_CYCLE(_sample) \
                (_sample)->data[1]
    #define VP_CYCLE(_sample) \
                (_sample)->data[0]
    #define VERTEX_CNT(_sample) \
                (_sample)->data[6]
    #define PIXEL_CNT(_sample) \
                PO_PM_PIXCNT(_sample) + PO_PM_REJ_CZ(_sample)

    #define PO_PM_PIXCNT(_sample) \
                ( (_sample)->data[7] + \
                  ((_sample)->data[8] << 1) + \
                  ((_sample)->data[9] << 2) + \
                  ((_sample)->data[10] << 3) + \
                  ((_sample)->data[11] << 4) )
    #define PO_PM_REJ_CZ(_sample) \
                ( (_sample)->data[12] + \
                  (_sample)->data[13] + \
                  (_sample)->data[14] + \
                  (_sample)->data[15] )
#elif GPAD_GPU_VER>=0x0801
    #define TRIANGLE_CNT(_sample) \
                (_sample)->data[3]
    #define PP_CYCLE(_sample) \
                (_sample)->data[1]
    #define VP_CYCLE(_sample) \
                (_sample)->data[0]
    #define SEGMT_CYCLE(_sample) \
                (_sample)->data[5]
    #define VERTEX_CNT(_sample) \
                (_sample)->data[6]
    #define PIXEL_CNT(_sample) \
                PO_PM_PIXCNT(_sample) + PO_PM_REJ_CZ(_sample)

    #define PO_PM_PIXCNT(_sample) \
                ( (_sample)->data[7] + \
                  ((_sample)->data[8] << 1) + \
                  ((_sample)->data[9] << 2) + \
                  ((_sample)->data[10] << 3) + \
                  ((_sample)->data[11] << 4) )
    #define PO_PM_REJ_CZ(_sample) \
                ( (_sample)->data[12] + \
                  (_sample)->data[13] + \
                  (_sample)->data[14] + \
                  (_sample)->data[15] )
#else
    #error "TODO: top level PM counters parsing"
#endif

#if GPAD_GPU_VER>=0x0801
    #define TOTAL_CYCLE(_sample) \
                SEGMT_CYCLE(_sample)
#else
    #define TOTAL_CYCLE(_sample) \
                ((VP_CYCLE(_sample) > PP_CYCLE(_sample)) ? VP_CYCLE(_sample) : PP_CYCLE(_sample))
#endif

int gpad_pm_is_valid_config(unsigned int config);
int gpad_pm_is_custom_config(unsigned int config);
void gpad_pm_do_config(struct gpad_pm_info *pm_info, unsigned int config, unsigned int ring_addr, unsigned int ring_size);
int gpad_pm_has_backup_config(struct gpad_pm_info *pm_info);
void gpad_pm_reset_backup_config(struct gpad_pm_info *pm_info);
void gpad_pm_backup_config(struct gpad_pm_info *pm_info);
void gpad_pm_restore_config(struct gpad_pm_info *pm_info, unsigned int config, unsigned int ring_addr, unsigned int ring_size);

#endif /* _GPAD_PM_CONFIG_H */
