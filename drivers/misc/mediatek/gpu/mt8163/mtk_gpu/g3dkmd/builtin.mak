### Used only when g3dkmd is built in kernel

ifeq ($(CONFIG_MTK_GPU_SAPPHIRE_LITE), y)

G3DKMD_KernelMode = true
#include $(srctree)/drivers/misc/mediatek/Makefile.custom
#include $(srctree)/drivers/misc/mediatek/gpu/mt8163/mtk_gpu/g3dkmd/sources.mak
ccflags-$(CONFIG_MTK_M4U) += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/


ccflags-y += -Winline -Dlinux -DFPGA_G3D_HW -O2 -DANDROID -DG3DKMD_BUILTIN_KERNEL

ifeq ("$(filter fpga%, $(MTK_PROJECT))", "$(MTK_PROJECT)")
ccflags-y += -DFPGA_$(MTK_PROJECT)
else
ccflags-y += -DMT8163
endif

ccflags-y += -I$(srctree)/drivers/staging/android/ion

ccflags-y += -I$(srctree)/drivers/staging/android
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/mt8163/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/gpu/mt8163/mtk_gpu/g3dbase
ccflags-y += -I$(srctree)/drivers/misc/mediatek/gpu/mt8163/mtk_gpu/g3dkmd
ccflags-y += -I$(srctree)/drivers/misc/mediatek/gpu/mt8163/mtk_gpu/sapphire_lite

#For fix include drivers/staging/android/ion/ion_priv.h
#but didn't define ION_RUNTIME_DEBUGGER.
ifeq ($(CONFIG_MT_ENG_BUILD), y)
ccflags-y += -DION_RUNTIME_DEBUGGER=1
else
ccflags-y += -DION_RUNTIME_DEBUGGER=0
endif


endif
