#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

obj-y += mt_spm.o
obj-y += mt_spm_internal.o
obj-$(CONFIG_ARCH_MT8163) += mt_sleep.o
obj-$(CONFIG_ARCH_MT8163) += mt_spm_sleep.o
obj-$(CONFIG_ARCH_MT8163) += mt_spm_dpidle.o
obj-$(CONFIG_ARCH_MT8163) += mt_spm_sodi.o
obj-$(CONFIG_ARCH_MT8163) += mt_spm_fs.o
obj-y += mt_ptp.o
obj-y += mt_cpuidle.o
obj-$(CONFIG_ARM64) += cpu_dormant.o
obj-$(CONFIG_ARM) += cpu_dormant-v7.o
obj-y += mt_pm_init.o
obj-y += mt_cpufreq.o
obj-y += mt_static_power.o
obj-y += mt_dcm.o
obj-y += mtcmos.o

ifneq ($(CONFIG_ARM64), y)

obj-y += mt-smp.o
obj-y += hotplug.o
obj-y += mt-headsmp.o
obj-y += mt_hotplug.o

else

obj-$(CONFIG_MTK_PSCI)  += mt_psci.o

endif

obj-y += mt_idle.o
obj-y += mt_gpufreq.o
