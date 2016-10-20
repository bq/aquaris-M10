# drivers/image/jpeg/Makefile

ccflags-y = -DSDEBUG_T

ifeq ($(CONFIG_ARCH_MT8163),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/hv1/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/hv1/mt8163/
endif

ccflags-y += -DJPEG_PM_DOMAIN_ENABLE

obj-y += jpeg_driver.o

jpeg_driver-objs := ../v2/jpeg_drv_enc.o \
                 	../v2/jpeg_cmdq.o \
                 	../v2/jpeg_drv.o

# EOF
