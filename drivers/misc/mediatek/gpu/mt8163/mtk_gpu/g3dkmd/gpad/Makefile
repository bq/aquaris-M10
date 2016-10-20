#
# Makefile for GPU Performance Analysis and Debugging driver.
#
ifneq ($(CONFIG_MTK_GPU_SAPPHIRE_LITE),)
obj-$(CONFIG_MTK_GPU_SAPPHIRE_LITE) += gpad.o
subdir-ccflags-y += -DGPAD_SUPPORT_MET
subdir-ccflags-y += -DMTK_INHOUSE_GPU
else
obj-m += gpad.o
endif

gpad-objs := gpad_init.o
gpad-objs += gpad_dev.o
gpad-objs += gpad_ioctl.o
gpad-objs += gpad_proc.o
gpad-objs += gpad_hal.o
gpad-objs += gpad_pm.o
gpad-objs += gpad_pm_config.o
gpad-objs += gpad_sdbg.o
gpad-objs += gpad_sdbg_dump.o
gpad-objs += gpad_crypt.o
gpad-objs += crypt_biginteger.o
gpad-objs += crypt_dh.o


GPAD_SUPPORT_DVFS ?= 1
ifeq ($(GPAD_SUPPORT_DVFS),1)
gpad-objs += gpad_ipem.o
gpad-objs += gpad_ipem_dvfs.o
gpad-objs += gpad_ipem_dpm.o
gpad-objs += gpad_ipem_lib.o
gpad-objs += gpad_ipem_utst.o
gpad-objs += gpad_gpufreq_utst.o
subdir-ccflags-y += -DGPAD_SUPPORT_DVFS

ifneq ("$(CONFIG_MTK_PLATFORM)", "")
subdir-ccflags-y += -Idrivers/misc/mediatek/base/power/$(CONFIG_MTK_PLATFORM)/
endif
ifeq ($(ON_QEMU_LKM),1)
subdir-ccflags-y += -I$(M)/../fake_g3dkmd/gpufreq
endif
endif

GPAD_SUPPORT_DFD ?= 1
ifeq ($(GPAD_SUPPORT_DFD),1)
gpad-objs += gpad_dfd.o
subdir-ccflags-y += -DGPAD_SUPPORT_DFD
endif

#
# Internal development use.
#
ifeq ($(ON_QEMU_LKM),1)
subdir-ccflags-y += -DGPAD_SHIM_ON
KBUILD_EXTRA_SYMBOLS ?= $(M)/../fake_g3dkmd/Module.symvers
-include qemu_lkm.mak
endif

ifeq ($(ON_PC_LKM),1)
subdir-ccflags-y += -DGPAD_SHIM_ON
subdir-ccflags-y += -DGPAD_NULL_HAL
-include pc_lkm.mak
endif

ifeq ("$(MTK_PROJECT)", "qemuL64")
subdir-ccflags-y += -DGPAD_SHIM_ON
subdir-ccflags-y += -DGPAD_NULL_HAL
subdir-ccflags-y += -fno-pic
endif

KBUILD_EXTRA_SYMBOLS ?= $(M)/../Module.symvers
