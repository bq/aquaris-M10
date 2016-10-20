
# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ft6x36/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

# obj-y := $(patsubst %.c,%.o,$(subst $(path)/,,$(call wildcard2,$(path)/*.c)))
obj-y += focaltech_ctl.o
obj-y += focaltech_driver.o
obj-y += focaltech_ex_fun.o
obj-y += ft_gesture_lib.a
