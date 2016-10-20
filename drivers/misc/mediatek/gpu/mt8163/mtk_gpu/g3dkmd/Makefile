# g3dkmd/Makefile

ifeq ("$(MTK_PROJECT)", "")
MTK_PROJECT = $(MTK_TARGET_PROJECT)
endif

ifeq ($(CONFIG_MTK_PLATFORM), "mt8163")
MTK_INHOUSE_GPU_PLATFORM=y
endif

ifeq ($(MTK_INHOUSE_GPU_PLATFORM), y)
ifeq ($(CONFIG_MTK_GPU_SAPPHIRE_LITE), y)

# used only for built-in kernel
include $(srctree)/drivers/misc/mediatek/gpu/mt8163/mtk_gpu/g3dkmd/builtin.mak
include $(srctree)/drivers/misc/mediatek/gpu/mt8163/mtk_gpu/g3dkmd/sources.mak

obj-y += $(CFILES:.c=.o)
obj-y += mmu/
obj-y += m4u/
obj-y += mpd/
obj-y += bufferfile/
obj-y += test/
obj-y += debugger/debugfs/

obj-$(CONFIG_SW_SYNC64) += sync64/

endif

else
ifeq ($(MTK_INHOUSE_GPU), yes)

YL_VERBOSE = FALSE
ifeq ($(YL_VERBOSE), TRUE)
YL_VERBOSE_CMD = V=1
else
YL_VERBOSE_CMD =
endif

ifndef KERNELDIR
ifneq ("$(ANDROID_VERSION)", "")
export YLROOTDIR := $(ANDROID_BUILD_TOP)
else
export YLROOTDIR := $(abspath $(PWD)/../../../../../..)
endif
endif

# use MTK android flow, we shall use
# ./makeMtk project_name r k ../frameworks/native/opengl/libygl/G3D/g3dkmd to build it
# the PWD will be alps/kernel
ifeq ("$(MTK_PROJECT)", "semu")
CROSS_COMPILE ?= /proj/mtk06063/tool/gcc-linaro-arm-linux-gnueabihf-4.8-2013.08_linux/bin/arm-linux-gnueabihf-
    ifeq ("$(SEMU_QEMU)", "yes")
        ccflags-y += -DSEMU_QEMU -O2
    else
        ccflags-y += -DFPGA_G3D_HW -DSEMU -O2
    endif
KERNELDIR ?= $(YLROOTDIR)/../../sapphire
ARCH ?= arm
ANDROID_MK = TRUE
YL_VERBOSE_CMD += O=$(KERNELDIR)/out32_2g MAKEFILE_DIR=$(MAKEFILE_DIR)
endif
ifeq ("$(MTK_PROJECT)", "semu64")
TARGET_ARCH ?= $(shell cd $(YLROOTDIR);CALLED_FROM_SETUP=true BUILD_SYSTEM=build/core \
                 command make --no-print-directory -f build/core/config.mk dumpvar-TARGET_ARCH)
CROSS_COMPILE ?= /proj/mtk06063/tool/gcc-linaro-aarch64-linux-gnu-4.8-2013.07-1_linux/bin/aarch64-linux-gnu-
ccflags-y += -DFPGA_G3D_HW -DSEMU64 -O2
KERNELDIR ?= $(YLROOTDIR)/../../sapphire-kernel
ARCH ?= $(TARGET_ARCH)
ANDROID_MK = TRUE
YL_VERBOSE_CMD += O=$(KERNELDIR)/out64_3g MAKEFILE_DIR=$(MAKEFILE_DIR)
endif
ifeq ("$(MTK_PROJECT)", "qemu64")
CROSS_COMPILE ?= $(YLROOTDIR)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
ccflags-y += -DQEMU64 -fno-pic
KERNELDIR ?= $(YLROOTDIR)/kernel
ARCH ?= arm64
ANDROID_MK = TRUE
YL_VERBOSE_CMD += MAKEFILE_DIR=$(MAKEFILE_DIR)
ifneq ("$(ANDROID_VERSION)", "M")
YL_VERBOSE_CMD += O=$(KERNELDIR)/out64_3g
endif
endif
ifeq ("$(MTK_PROJECT)", "fpga8163")
TARGET_ARCH ?= $(shell cd $(YLROOTDIR);CALLED_FROM_SETUP=true BUILD_SYSTEM=build/core \
                 command make --no-print-directory -f build/core/config.mk dumpvar-TARGET_ARCH)
ccflags-y += -DFPGA_G3D_HW -O2
KERNELDIR ?= $(YLROOTDIR)/kernel
ARCH ?= $(TARGET_ARCH)
ANDROID_MK = TRUE
ifeq ("$(TARGET_ARCH)", "arm64")
YL_VERBOSE_CMD += O=$(YLROOTDIR)/out/target/product/fpga8163_64/obj/KERNEL_OBJ MAKEFILE_DIR=$(MAKEFILE_DIR) TARGET_ARCH=$(TARGET_ARCH)
else
YL_VERBOSE_CMD += O=$(YLROOTDIR)/out/target/product/fpga8163/obj/KERNEL_OBJ MAKEFILE_DIR=$(MAKEFILE_DIR) TARGET_ARCH=$(TARGET_ARCH)
endif
endif
ifeq ("$(MTK_PROJECT)", "fpga8163_64")
TARGET_ARCH ?= $(shell cd $(YLROOTDIR);CALLED_FROM_SETUP=true BUILD_SYSTEM=build/core \
                 command make --no-print-directory -f build/core/config.mk dumpvar-TARGET_ARCH)
ccflags-y += -DFPGA_G3D_HW -O2
KERNELDIR ?= $(YLROOTDIR)/kernel
ARCH ?= $(TARGET_ARCH)
ANDROID_MK = TRUE
YL_VERBOSE_CMD += O=$(YLROOTDIR)/out/target/product/fpga8163_64/obj/KERNEL_OBJ MAKEFILE_DIR=$(MAKEFILE_DIR) TARGET_ARCH=$(TARGET_ARCH)
endif
ifeq ("$(MTK_PROJECT)", "mt6752_fpga")
ccflags-y += -DFPGA_G3D_HW -O2
KERNELDIR ?= $(PWD)
ANDROID_MK = TRUE
endif
ifeq ("$(MTK_PROJECT)", "mt6592_fpga")
ccflags-y += -DFPGA_G3D_HW
KERNELDIR ?= $(PWD)
ANDROID_MK = TRUE
endif
ifeq ("$(MTK_PROJECT)", "")
ifeq ("$(ARCH)", "arm")       # android emulator makefile
KERNELDIR ?= $(YLROOTDIR)/kernel
ANDROID_MK = TRUE
else                          # linux makefile
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
ANDROID_MK = FALSE
endif
endif

#it's the same in linux but different in android for sources.mak
ifeq ($(ANDROID_MK), TRUE)
G3DKMD_KernelMode ?= true
G3DTOP = ../..
else
G3DTOP = ../..
endif

KMDSRCTOP = ../..

#include configuration and source
ifeq ("$(G3DKMD_KernelMode)", "")
include $(PWD)/../../../../build/linux/mode_selection.txt
endif


# Kernel mode make file
ifeq ("$(G3DKMD_KernelMode)", "true")
ifeq ("$(MTK_PROJECT)", "semu")
MAKEFILE_DIR := $(PWD)
ccflags-y += -DFPGA_mt6752_fpga
ccflags-y += -Idrivers/staging/android
ccflags-y += -Idrivers/mtk/include
endif
ifeq ("$(MTK_PROJECT)", "semu64")
MAKEFILE_DIR := $(PWD)
ccflags-y += -DFPGA_fpga8163
ccflags-y += -Idrivers/staging/android
ccflags-y += -Idrivers/staging/android/ion
ccflags-y += -Idrivers/mtk/include
endif
ifeq ("$(MTK_PROJECT)", "qemu64")
MAKEFILE_DIR := $(PWD)
ccflags-y += -Idrivers/staging/android
endif
ifeq ("$(MTK_PROJECT)", "fpga8163")
MAKEFILE_DIR := $(PWD)
ccflags-y += -DFPGA_$(MTK_PROJECT)
ccflags-y += -Idrivers/staging/android
ccflags-y += -Idrivers/misc/mediatek/mach/mt8163/include
endif
ifeq ("$(MTK_PROJECT)", "fpga8163_64")
MAKEFILE_DIR := $(PWD)
ccflags-y += -DFPGA_fpga8163
ccflags-y += -Idrivers/staging/android
ccflags-y += -Idrivers/misc/mediatek/mach/mt8163/include
endif
ifeq ("$(MTK_PROJECT)", "mt6752_fpga")
MAKEFILE_DIR := ../frameworks/native/opengl/libygl/G3D/g3dkmd
ccflags-y += -DFPGA_$(MTK_PROJECT)
ccflags-y += -Idrivers/staging/android
endif
ifeq ("$(MTK_PROJECT)", "mt6592_fpga")
MAKEFILE_DIR := ../frameworks/native/opengl/libygl/G3D/g3dkmd
ccflags-y += -DFPGA_$(MTK_PROJECT)
endif
ifeq ("$(MTK_PROJECT)", "")
MAKEFILE_DIR := $(PWD)
endif

include $(MAKEFILE_DIR)/sources.mak

ccflags-y += -g
ccflags-y += -Winline -Dlinux
ccflags-y += -I$(MAKEFILE_DIR)/../g3dbase
ccflags-y += -I$(MAKEFILE_DIR)/../g3dkmd

ifeq ("$(PROJ)", "ULC")
  ccflags-y += -I$(MAKEFILE_DIR)/../sapphire_ulc
else ifeq ("$(PROJ)", "AEP")
  ccflags-y += -I$(MAKEFILE_DIR)/../sapphire_aep
else #ifeq ("$(PROJ)", "LIT")
  ccflags-y += -I$(MAKEFILE_DIR)/../sapphire_lite
endif

ifeq ("$(shell uname -m)", "x86_64")    # if 64-bit platform, MMU is not supported.
ifneq ($(ANDROID_MK), TRUE)             #   and not ANDROID
ccflags-y += -DYL_64BITS
endif
endif

obj-m += yolig3d.o
yolig3d-objs := $(CFILES:.c=.o)

G3D_MOD_NAME = yolig3d
G3D_DEV_NAME = yolig3d
G3D_DEV_MOD = 664

ifeq ($(ANDROID_MK), TRUE)
ccflags-y += -DANDROID
endif

ifneq ("$(CONFIG)","NO_CMODEL")
ccflags-y += -DCMODEL
endif

ifeq ("$(USING_FBDEV)","true")
ccflags-y += -DYL_FAKE_X11
endif

ccflags-y += -Wno-error=implicit-function-declaration

ifeq ("$(ARCH)", "")
YL_ARCH_CMD =
else
YL_ARCH_CMD = ARCH=$(ARCH)
endif

ifeq ("$(CROSS_COMPILE)", "")
YL_CROSS_COMPILE_CMD =
else
YL_CROSS_COMPILE_CMD = CROSS_COMPILE=$(CROSS_COMPILE)
endif

all:
	make -C $(KERNELDIR) $(YL_VERBOSE_CMD) M=$(MAKEFILE_DIR) $(YL_ARCH_CMD) $(YL_CROSS_COMPILE_CMD) modules

make_install:
	make -C $(KERNELDIR) $(YL_VERBOSE_CMD) M=$(MAKEFILE_DIR) $(YL_ARCH_CMD) $(YL_CROSS_COMPILE_CMD) modules
	make install

install:
	\
  if grep '^staff:' /etc/group > /dev/null; then \
      G3D_DEV_GROUP=$(USER) ; \
  else \
      G3D_DEV_GROUP="wheel" ; \
  fi ; \
  sudo /sbin/insmod -f ./${G3D_MOD_NAME}.ko || exit 1 ; \
  G3D_DEV_MAJOR=`cat /proc/devices | awk "\\$$2==\"${G3D_MOD_NAME}\" {print \\$$1}"` ; \
  sudo rm -f /dev/${G3D_DEV_NAME} ; \
  sudo mknod /dev/${G3D_DEV_NAME} c $$G3D_DEV_MAJOR 0 ; \
  sudo chgrp $$G3D_DEV_GROUP /dev/${G3D_DEV_NAME} ; \
  sudo chmod $(G3D_DEV_MOD)  /dev/${G3D_DEV_NAME}

clean:
	make -C $(KERNELDIR) M=$(MAKEFILE_DIR) $(YL_ARCH_CMD) $(YL_CROSS_COMPILE_CMD) clean

# User mode make file
else

include $(G3DTOP)/../../build/linux/configure

include sources.mak

#define makefile variable
G3DKMD_LIB_NAME = libG3DKMD.so
G3DKMD_LIB = G3DKMD

G3DKMD_MAJOR = 1
G3DKMD_MINOR = 0

G3DKMDIMM = $(G3DTOP)/../../$(IMM_DIR)/g3dkmd

OBJECTS= $(subst $(G3DMKDSRC)/,$(G3DKMDIMM)/, $(CFILES:.c=.o))

#define compiler flag and rule
#LOCAL_CFLAGS = -Dlinux_user_mode
#-Winline

$(G3DKMDIMM)/%.o: $(G3DMKDSRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(INCLUDE_DIRS) $(CXXFLAGS) $(LOCAL_CFLAGS) $< -o $@

$(G3DKMDIMM)/%.o: $(G3DMKDSRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(LOCAL_CFLAGS) $< -o $@

#define compiler target
default: $(G3DTOP)/../../$(LIB_DIR)/$(G3DKMD_LIB_NAME)

# libpatgen.so or libcmv2.so linking setting
DRVOUTLIB :=
ifneq ("$(DRVOUTNAME)","")
  DRVOUTLIB += -L$(G3DTOP)/../../$(PREBUILT_DIR) -l$(DRVOUTNAME)
  DRVOUTSRC := $(G3DTOP)/../../$(PREBUILT_DIR)/lib$(DRVOUTNAME).so
  DRVOUTDST := $(G3DTOP)/../../$(LIB_DIR)/lib$(DRVOUTNAME).so

  ifeq ("$(wildcard $(DRVOUTSRC))","")
    $(error $(DRVOUTSRC) not found!)
  endif
  ifeq ("$(wildcard $(DRVOUTDST))","")
    $(shell cp $(DRVOUTSRC) $(DRVOUTDST))
  endif
endif

$(G3DTOP)/../../$(LIB_DIR)/$(G3DKMD_LIB_NAME): $(OBJECTS)
	$(MKLIB) -o $(G3DKMD_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
           -major $(G3DKMD_MAJOR) -minor $(G3DKMD_MINOR) \
	         -install $(G3DTOP)/../../$(LIB_DIR) $(MKLIB_OPTIONS) $(LIB_DEPS) $(DRVOUTLIB) $(OBJECTS)

clean:
	-rm -f -r $(G3DKMDIMM)
	-rm -f $(G3DTOP)/../../$(LIB_DIR)/$(G3DKMD_LIB_NAME)

$(G3DKMDIMM)/depend: $(CFILES)
	@ mkdir -p $(G3DKMDIMM)
	@ echo "running $(MKDEP)"
	@ rm -f $@
	@ touch $@
	$(MKDEP) -f$@ $(INCLUDE_DIRS) $(DEFINES) $(LOCAL_CFLAGS) \
		$(CFILES) > /dev/null 2>/dev/null
	$(SED) -e 's,^$(G3DMKDSRC),$(G3DKMDIMM),' $@ > $@.tmp && mv $@ $@.org && mv $@.tmp $@


-include $(G3DKMDIMM)/depend

endif
endif # ifeq ($(MTK_INHOUSE_GPU), yes) # only in 8163
endif # ifeq ($(MTK_INHOUSE_GPU_PLATFORM), y)
