include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Linux driver folder
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/ft5626/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/ft5626/

obj-y	+=  focaltech_core.o
obj-y	+=  focaltech_ctl.o
obj-y	+=  focaltech_ex_fun.o
obj-y	+=  focaltech_flash.o
#obj-y	+=  focaltech_Gesture.o
#obj-y	+=	ft_gesture_lib.a
