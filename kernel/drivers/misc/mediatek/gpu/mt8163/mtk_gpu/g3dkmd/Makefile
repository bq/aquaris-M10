# g3dkmd/Makefile

ifeq ("$(MTK_PROJECT)", "")
MTK_PROJECT = $(MTK_TARGET_PROJECT)
endif

ifeq ($(CONFIG_MTK_PLATFORM), "mt8163")
MTK_INHOUSE_GPU_PLATFORM=y
endif

ifeq ($(MTK_INHOUSE_GPU_PLATFORM), y)
ifeq ($(CONFIG_MTK_INHOUSE_GPU), y)

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
ifeq ("$(ANDROID_VERSION)", "5.0")
export YLROOTDIR := $(abspath $(shell pwd)/../../../../../../../../..)
else
ifeq ("$(ANDROID_VERSION)", "L")
export YLROOTDIR := $(abspath $(shell pwd)/../../../../../../../../..)
else
export YLROOTDIR := $(abspath $(PWD)/../../../../../..)
endif
endif
endif

# use MTK android flow, we shall use 
# ./makeMtk project_name r k ../frameworks/native/opengl/libygl/G3D/g3dkmd to build it 
# the PWD will be alps/kernel 
ifeq ("$(MTK_PROJECT)", "semu")
CROSS_COMPILE ?= /proj/mtk06063/tool/gcc-linaro-arm-linux-gnueabihf-4.8-2013.08_linux/bin/arm-linux-gnueabihf-
    ifeq ("$(SEMU_QEMU)", "yes")
        EXTRA_CFLAGS += -DSEMU_QEMU -O2
    else
        EXTRA_CFLAGS += -DFPGA_G3D_HW -DSEMU -O2
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
EXTRA_CFLAGS += -DFPGA_G3D_HW -DSEMU64 -O2
KERNELDIR ?= $(YLROOTDIR)/../../sapphire-kernel
ARCH ?= $(TARGET_ARCH)
ANDROID_MK = TRUE
YL_VERBOSE_CMD += O=$(KERNELDIR)/out64_3g MAKEFILE_DIR=$(MAKEFILE_DIR)
endif
ifeq ("$(MTK_PROJECT)", "qemuL64")
CROSS_COMPILE ?= $(YLROOTDIR)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
EXTRA_CFLAGS += -DQEMUL64 -fno-pic
KERNELDIR ?= $(YLROOTDIR)/kernel
ARCH ?= arm64
ANDROID_MK = TRUE
YL_VERBOSE_CMD += O=$(KERNELDIR)/out64_3g MAKEFILE_DIR=$(MAKEFILE_DIR)
endif
ifeq ("$(MTK_PROJECT)", "fpga8163")
TARGET_ARCH ?= $(shell cd $(YLROOTDIR);CALLED_FROM_SETUP=true BUILD_SYSTEM=build/core \
                 command make --no-print-directory -f build/core/config.mk dumpvar-TARGET_ARCH)
EXTRA_CFLAGS += -DFPGA_G3D_HW -O2
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
EXTRA_CFLAGS += -DFPGA_G3D_HW -O2
KERNELDIR ?= $(YLROOTDIR)/kernel
ARCH ?= $(TARGET_ARCH)
ANDROID_MK = TRUE
YL_VERBOSE_CMD += O=$(YLROOTDIR)/out/target/product/fpga8163_64/obj/KERNEL_OBJ MAKEFILE_DIR=$(MAKEFILE_DIR) TARGET_ARCH=$(TARGET_ARCH)
endif
ifeq ("$(MTK_PROJECT)", "mt6752_fpga")
EXTRA_CFLAGS += -DFPGA_G3D_HW -O2
KERNELDIR ?= $(PWD)           
ANDROID_MK = TRUE
endif
ifeq ("$(MTK_PROJECT)", "mt6592_fpga")
EXTRA_CFLAGS += -DFPGA_G3D_HW
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
G3DTOP = ../../GL/Mesa-9.0
endif 

KMDSRCTOP = ../..

#include configuration and source
ifeq ("$(G3DKMD_KernelMode)", "")
include $(PWD)/../../GL/Mesa-9.0/build/linux/mode_selection.txt
endif


# Kernel mode make file
ifeq ("$(G3DKMD_KernelMode)", "true")
ifeq ("$(MTK_PROJECT)", "semu")
MAKEFILE_DIR := $(PWD)
EXTRA_CFLAGS += -DFPGA_mt6752_fpga
EXTRA_CFLAGS += -Idrivers/staging/android
EXTRA_CFLAGS += -Idrivers/mtk/include
endif
ifeq ("$(MTK_PROJECT)", "semu64")
MAKEFILE_DIR := $(PWD)
EXTRA_CFLAGS += -DFPGA_fpga8163
EXTRA_CFLAGS += -Idrivers/staging/android
EXTRA_CFLAGS += -Idrivers/mtk/include
endif
ifeq ("$(MTK_PROJECT)", "qemuL64")
MAKEFILE_DIR := $(PWD)
EXTRA_CFLAGS += -Idrivers/staging/android
endif
ifeq ("$(MTK_PROJECT)", "fpga8163")
MAKEFILE_DIR := $(PWD)
EXTRA_CFLAGS += -DFPGA_$(MTK_PROJECT)
EXTRA_CFLAGS += -Idrivers/staging/android
EXTRA_CFLAGS += -Idrivers/misc/mediatek/mach/mt8163/include
endif
ifeq ("$(MTK_PROJECT)", "fpga8163_64")
MAKEFILE_DIR := $(PWD)
EXTRA_CFLAGS += -DFPGA_fpga8163
EXTRA_CFLAGS += -Idrivers/staging/android
EXTRA_CFLAGS += -Idrivers/misc/mediatek/mach/mt8163/include
endif
ifeq ("$(MTK_PROJECT)", "mt6752_fpga")
MAKEFILE_DIR := ../frameworks/native/opengl/libygl/G3D/g3dkmd
EXTRA_CFLAGS += -DFPGA_$(MTK_PROJECT)
EXTRA_CFLAGS += -Idrivers/staging/android
endif
ifeq ("$(MTK_PROJECT)", "mt6592_fpga")
MAKEFILE_DIR := ../frameworks/native/opengl/libygl/G3D/g3dkmd
EXTRA_CFLAGS += -DFPGA_$(MTK_PROJECT)
endif
ifeq ("$(MTK_PROJECT)", "")
MAKEFILE_DIR := $(PWD)
endif

include $(MAKEFILE_DIR)/sources.mak

EXTRA_CFLAGS += -g
EXTRA_CFLAGS += -Winline -Dlinux
EXTRA_CFLAGS += -I$(MAKEFILE_DIR)/../g3dbase
EXTRA_CFLAGS += -I$(MAKEFILE_DIR)/../g3dkmd
EXTRA_CFLAGS += -I$(MAKEFILE_DIR)/../sapphire_lite

ifeq ("$(shell uname -m)", "x86_64")    # if 64-bit platform, MMU is not supported.
ifneq ($(ANDROID_MK), TRUE)             #   and not ANDROID
EXTRA_CFLAGS += -DYL_64BITS
endif
endif

obj-m += yolig3d.o
yolig3d-objs := $(CFILES:.c=.o)

G3D_MOD_NAME = yolig3d
G3D_DEV_NAME = yolig3d
G3D_DEV_MOD = 664 

ifeq ($(ANDROID_MK), TRUE)
EXTRA_CFLAGS += -DANDROID
endif

ifneq ("$(CONFIG)","NO_CMODEL")
EXTRA_CFLAGS += -DCMODEL
endif

ifeq ("$(USING_FBDEV)","true")
EXTRA_CFLAGS += -DYL_FAKE_X11
endif

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

include $(G3DTOP)/build/linux/configure

include sources.mak

#define makefile variable
G3DKMD_LIB_NAME = libG3DKMD.so
G3DKMD_LIB = G3DKMD

G3DKMD_MAJOR = 1
G3DKMD_MINOR = 0

G3DKMDIMM = $(G3DTOP)/$(IMM_DIR)/g3dkmd

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
default: $(G3DTOP)/$(LIB_DIR)/$(G3DKMD_LIB_NAME)

$(G3DTOP)/$(LIB_DIR)/$(G3DKMD_LIB_NAME): $(OBJECTS)
	$(MKLIB) -o $(G3DKMD_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(G3DKMD_MAJOR) -minor $(G3DKMD_MINOR) \
	 -install $(G3DTOP)/$(LIB_DIR) $(MKLIB_OPTIONS) $(LIB_DEPS) $(OBJECTS)

clean:
	-rm -f -r $(G3DKMDIMM)
	-rm -f $(G3DTOP)/$(LIB_DIR)/$(G3DKMD_LIB_NAME)

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
