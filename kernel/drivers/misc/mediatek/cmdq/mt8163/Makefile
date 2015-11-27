include $(srctree)/drivers/misc/mediatek/Makefile.custom

# drivers/cmdq

ccflags-y += -I$(srctree)/drivers/misc/mediatek/dispsys/$(MTK_PLATFORM) \
	     -I$(srctree)/drivers/misc/mediatek/cmdq \
	     -I$(srctree)/include/linux

ifeq ($(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT),y)
$(warning start to build cmdq secure)
	ccflags-y += -DMTK_SEC_VIDEO_PATH_SUPPORT
	ccflags-y += -DCMDQ_SECURE_PATH_SUPPORT
	ccflags-y += -DDEBUG_SVP_INFO
else

endif

# Enable MET
ccflags-y += -DMET_USER_EVENT_SUPPORT
ccflags-y += -DDEBUG_INFO

obj-y += cmdq_record.o
obj-y += cmdq_core.o
obj-y += cmdq_driver.o
obj-y += cmdq_mmp.o
obj-y += cmdq_mdp_common.o
obj-y += cmdq_prof.o
obj-y += cmdq_test.o
obj-y += cmdq_sec.o
obj-y += cmdq_mdp.o
obj-y += cmdq_device.o
obj-y += cmdq_platform.o
obj-y += cmdq_mutex.o
