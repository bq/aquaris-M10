#
# Makefile for external display driver.
#
ifeq ($(CONFIG_MTK_HDMI_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/  \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/ \
			 -I$(srctree)/drivers/misc/mediatek/sync/ \
             -I$(srctree)/drivers/misc/mediatek/hdmi/$(CONFIG_CUSTOM_KERNEL_HDMI)/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/hv1/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/hv1/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/hdmi/internal_hdmi/$(MTK_PLATFORM)/inc/ \
             -I$(srctree)/drivers/misc/mediatek/hdmi/hdmitx/$(MTK_PLATFORM)/inc/ \
             -I$(srctree)/drivers/staging/android/ion \
             -I$(srctree)/drivers/staging/android/ion/mtk/ \
             -I$(srctree)/drivers/misc/mediatek/video/     \
             -I$(srctree)/drivers/misc/mediatek/ext_disp/ \
             -I$(srctree)/drivers/misc/mediatek/lcm/inc/ \
             -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/mmp/ \


ifeq ($(CONFIG_CUSTOM_KERNEL_HDMI),"Sii8338")
	ccflags-y +=	-DMHL_SII8338
endif

ifeq ($(CONFIG_CUSTOM_KERNEL_HDMI),"Sii8348")
	ccflags-y +=	-DMHL_SII8348
endif


ifeq ($(CONFIG_MTK_SEC_VIDEO_PATH_FULL_SUPPORT),y)
ccflags-y += -I$(srctree)/include/trustzone
ccflags-y += -I$(srctree)/include
endif


obj-y :=  \
    mtk_ext_disp_mgr.o   \
    extd_lcm.o \
    extd_fence.o \
    external_display.o \
    extd_debug.o \
    extd_utils.o \
    Kara_1280x720.o \
    extd_hdmi_drv.o \

else
obj-y := dummy.o
endif
