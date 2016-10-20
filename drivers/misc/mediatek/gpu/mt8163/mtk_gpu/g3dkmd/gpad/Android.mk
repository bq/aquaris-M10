LOCAL_PATH := $(call my-dir)

#
# Locate kernel module source directory.
#
ifneq ("x$(YOLIG3D_PATH)", "x")
GPAD_PATH := $(abspath $(YOLIG3D_PATH)/gpad)
else
GPAD_PATH := $(abspath $(LOCAL_PATH))
endif
GPAD_TARGET := $(GPAD_PATH)/gpad.ko

#
# Set up for ALPS.
#
KERNELDIR ?= $(ANDROID_BUILD_TOP)/kernel
KERNEL_OBJ_DIR ?= $(ANDROID_BUILD_TOP)/$(PRODUCT_OUT)/obj/KERNEL_OBJ

#
# Set up for SEMU, assume KERNELDIR has been specified outside.
#
ifeq ("$(MTK_PROJECT)", "semu")
KERNEL_OBJ_DIR := $(KERNELDIR)/out32_2g
CROSS_COMPILE := /proj/mtk06063/tool/gcc-linaro-arm-linux-gnueabihf-4.8-2013.08_linux/bin/arm-linux-gnueabihf-
ARCH := arm
endif
ifeq ("$(MTK_PROJECT)", "semu64")
KERNEL_OBJ_DIR := $(KERNELDIR)/out64_3g
CROSS_COMPILE := /proj/mtk06063/tool/gcc-linaro-aarch64-linux-gnu-4.8-2013.07-1_linux/bin/aarch64-linux-gnu-
ARCH := arm64
endif
ifeq ("$(MTK_PROJECT)", "qemuL64")
KERNEL_OBJ_DIR := $(KERNELDIR)/out64_3g
CROSS_COMPILE := $(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
ARCH := arm64
endif


#
# Set up cross-compile toolchain.
# Note that, CROSS_COMIPILE/ARCH have higher priority than TARGET_ARCH.
#
CROSS_OPTIONS =
ifneq ("x$(ARCH)", "x")
CROSS_OPTIONS += ARCH=$(ARCH)
endif
ifneq ("x$(CROSS_COMPILE)", "x")
CROSS_OPTIONS += CROSS_COMPILE=$(CROSS_COMPILE)
endif

#
# Build gpad.ko
#
include $(CLEAR_VARS)

ALL_MODULES += $(GPAD_TARGET)
$(GPAD_TARGET):
	cd $(GPAD_PATH) && \
	rm -f *.o *.ko && \
	make O=$(KERNEL_OBJ_DIR) -C $(KERNELDIR) M=$(GPAD_PATH) TARGET_ARCH=$(TARGET_ARCH) $(CROSS_OPTIONS)
