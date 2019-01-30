# drivers/image/jpeg/Makefile

ccflags-y = -DSDEBUG_T

ifeq ($(CONFIG_ARCH_MT8127),y)
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/hv1/
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/hv1/mt8173/
endif

ccflags-y += -DJPEG_PM_DOMAIN_ENABLE
ccflags-y += -DMTK_JPEG_CMDQ_NO_SUPPORT

obj-y += jpeg_driver.o

jpeg_driver-objs := ../v1/jpeg_drv_6589_enc.o \
					../v1/jpeg_drv_6589_dec.o \
                 	../v1/jpeg_drv.o

# EOF
