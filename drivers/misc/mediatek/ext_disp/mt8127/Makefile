ifeq ($(CONFIG_MTK_INTERNAL_HDMI_SUPPORT), y)
include $(srctree)/drivers/misc/mediatek/video/mt8127/ccflags.mk
obj-y := hdmitx.o ext_disp_debug.o  hdmitx_test.o
else
obj-y := dummy.o
endif

