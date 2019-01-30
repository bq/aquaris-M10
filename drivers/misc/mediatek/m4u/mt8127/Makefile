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

ccflags-y += -I$(srctree)/include/trustzone
ccflags-y += -I$(srctree)/drivers/misc/mediatek/smi/variant
ccflags-y += -I$(srctree)/drivers/misc/mediatek/dispsys/$(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mmp
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(subst ",,$(CONFIG_MTK_PLATFORM))/dispsys
obj-y += m4u.o m4u_kernel.o m4u_priv.o
