ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/gpu/hal/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/fakeHeader/
obj-  := dummy.o

ifeq ($(CONFIG_THERMAL), y)
else
    obj-y += mtk_thermal_dummy.o
endif

obj-$(CONFIG_THERMAL) += mtk_thermal_platform.o

# Only included in non-EVB projects
obj-$(CONFIG_THERMAL) += mtk_ts_cpu.o
obj-$(CONFIG_THERMAL) += mtk_ts_abb.o
obj-$(CONFIG_THERMAL) += mtk_ts_battery.o
# obj-$(CONFIG_THERMAL) += mtk_ts_pmic.o
obj-$(CONFIG_THERMAL) += mtk_ts_pmic6323.o
obj-$(CONFIG_THERMAL) += mtk_ts_wmt.o
#obj-$(CONFIG_THERMAL) += mtk_cooler_bcct.o
