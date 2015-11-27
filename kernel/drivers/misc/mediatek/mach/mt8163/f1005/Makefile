
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

ifeq ($(CONFIG_MTK_KXTJ2_1009_NEW),y)
obj-y   +=  accelerometer/KXTJ2_1009-new/
endif

ifeq ($(CONFIG_MTK_BMC056_ACC),y)
obj-y   +=  accelerometer/bmc056_acc/
endif

ifeq ($(CONFIG_MTK_BMC056_MAG),y)
obj-y   +=  magnetometer/bmc056_mag/
endif

ifeq ($(CONFIG_MTK_LTR303),y)
obj-y   +=  alsps/ltr303/
endif

obj-y   +=  camera/camera/
obj-y   +=  flashlight/constant_flashlight/
obj-y   +=  vibrator/
obj-y   +=  leds/
obj-y   +=  imgsensor/
obj-y   +=  keypad/
obj-y   +=  sound/
obj-y   +=  accdet/

ifeq ($(CONFIG_MTK_HDMI_SUPPORT),y)
obj-y	+=  hdmi/
endif
