#
# Makefile for misc devices that really don't fit anywhere else.
#
include $(srctree)/drivers/misc/mediatek/Makefile.custom

#ifneq ($(CONFIG_ARM64), y)
obj-y				+= kd_sensorlist.o

#ifeq ($(wildcard $(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/imgsensor),)
ifeq ($(wildcard $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/imgsensor),)

custom_kernel_imgsensor := $(addsuffix /, $(shell echo $(CONFIG_CUSTOM_KERNEL_IMGSENSOR)))

obj-y += $(custom_kernel_imgsensor)

endif
#endif
