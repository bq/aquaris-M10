#include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-$(CONFIG_MTK_PMIC)          += pmic_mt6323.o pmic_chr_type_det.o

obj-$(CONFIG_MTK_SMART_BATTERY) += battery_meter_hal.o

ifeq ($(CONFIG_MTK_BQ27520_SUPPORT),y)
     obj-$(CONFIG_MTK_SMART_BATTERY) += bq27520.o
endif

ifeq ($(CONFIG_MTK_BQ24261_SUPPORT),y)
     obj-$(CONFIG_MTK_SMART_BATTERY) += bq24261.o charging_hw_bq24261.o
else
ifeq ($(CONFIG_MTK_NCP1854_SUPPORT),y)
     obj-$(CONFIG_MTK_SMART_BATTERY) += ncp1854.o charging_hw_ncp1854.o tbl_helper_func.o
else
ifeq ($(CONFIG_MTK_RT9536_SUPPORT),y)
     obj-$(CONFIG_MTK_SMART_BATTERY) += rt9536/
else
ifeq ($(CONFIG_MTK_BQ24196_SUPPORT),y)
     obj-$(CONFIG_MTK_SMART_BATTERY) += charging_hw_bq24196.o bq24196.o tbl_helper_func.o
else
ifeq ($(CONFIG_MTK_BQ24296_SUPPORT),y)
     obj-$(CONFIG_MTK_SMART_BATTERY) += charging_hw_bq24296.o bq24296.o tbl_helper_func.o
else
     obj-$(CONFIG_MTK_SMART_BATTERY) += charging_hw_pmic.o
endif
endif
endif
endif
endif

