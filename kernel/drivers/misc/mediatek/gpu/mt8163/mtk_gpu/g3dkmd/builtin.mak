### Used only when g3dkmd is built in kernel

ifeq ($(CONFIG_MTK_INHOUSE_GPU), y)

G3DKMD_KernelMode = true
include $(srctree)/drivers/misc/mediatek/Makefile.custom
#include $(srctree)/drivers/misc/mediatek/gpu/mt8163/mtk_gpu/g3dkmd/sources.mak

ccflags-y += -Winline -Dlinux -DFPGA_G3D_HW -O2 -DMT8163 -DANDROID -DG3DKMD_BUILTIN_KERNEL
ccflags-y += -I$(srctree)/drivers/staging/android/ion

ccflags-y += -I$(srctree)/drivers/staging/android
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/mt8163/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/gpu/mt8163/mtk_gpu/g3dbase
ccflags-y += -I$(srctree)/drivers/misc/mediatek/gpu/mt8163/mtk_gpu/g3dkmd
ccflags-y += -I$(srctree)/drivers/misc/mediatek/gpu/mt8163/mtk_gpu/sapphire_lite

endif
