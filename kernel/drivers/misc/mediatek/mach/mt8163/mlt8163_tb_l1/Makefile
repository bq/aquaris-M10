
include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifneq ($(CONFIG_ARCH_MTK_PROJECT),)
ARCH_MTK_PROJECT := $(shell echo $(CONFIG_ARCH_MTK_PROJECT))
obj-y   += core/
obj-y   += dct/dct/
endif

ifeq ($(CONFIG_MTK_MPU6050G),y)
obj-y   +=  accelerometer/mpu6050g/
endif

ifeq ($(CONFIG_MTK_MPU6050G_NEW),y)
obj-y   +=  accelerometer/mpu6050g-new/
endif


ifeq ($(CONFIG_MTK_MPU6050GY),y)
obj-y   +=  gyroscope/mpu6050gy/
endif

ifeq ($(CONFIG_MTK_MPU6050GY_NEW),y)
obj-y   +=  gyroscope/mpu6050gy-new/
endif


ifeq ($(CONFIG_MTK_AKM09911),y)
obj-y	+=  magnetometer/akm09911/
endif

ifeq ($(CONFIG_MTK_AKM09911_NEW),y)
obj-y	+=  magnetometer/akm09911-new/
endif

obj-y   +=  camera/camera/
obj-y   +=  flashlight/constant_flashlight/
obj-y   +=  vibrator/
obj-y   +=  leds/
obj-y   +=  keypad/
obj-y   +=  sound/
obj-y   +=  accdet/

ifeq ($(CONFIG_MTK_HDMI_SUPPORT),y)
obj-y	+=  hdmi/
endif
