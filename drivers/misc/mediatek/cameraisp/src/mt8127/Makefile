#
# Makefile for misc devices that really don't fit anywhere else.
#

# Tablet SMI path
ccflags-y += -I$(srctree)/drivers/misc/mediatek/smi/variant/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/

obj-y += cameraisp_dummy2.o
obj-y += camera_isp.o
obj-y += camera_sysram.o
obj-y += camera_pipe_mgr.o

# $(info cameraisp drv by platform $(platform_drv))
