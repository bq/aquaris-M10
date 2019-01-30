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

ccflags-y += -Werror

ccflags-$(CONFIG_MTK_LCM) += -I$(srctree)/drivers/misc/mediatek/lcm/inc
ccflags-$(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT) += -I$(srctree)/include -I$(srctree)/include/trustzone
ccflags-$(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) += -I$(srctree)/drivers/misc/mediatek/hdmi/hdmitx/mt8127
ccflags-$(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) += -I$(srctree)/drivers/misc/mediatek/ext_disp/mt8127
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/mt8127/dispsys   \
             -I$(srctree)/drivers/misc/mediatek/video/mt8127/videox   \
             -I$(srctree)/drivers/misc/mediatek/video/mt8127/videox/ovl_engine   \
             -I$(srctree)/drivers/misc/mediatek/sync/      \
             -I$(srctree)/drivers/misc/mediatek/m4u/mt8127       \
             -I$(srctree)/drivers/misc/mediatek/cmdq/mt8127      \
             -I$(srctree)/drivers/misc/mediatek/cmdq/      \
             -I$(srctree)/drivers/misc/mediatek/externaldisplay/mt8127 \
             -I$(srctree)/drivers/misc/mediatek/mach/mt8127/include/mach/ \
             -I$(srctree)/drivers/misc/mediatek/base/power/ \
             -I$(srctree)/drivers/misc/mediatek/leds/ \
             -I$(srctree)/drivers/misc/mediatek/leds/mt8127 \
             -I$(srctree)/drivers/misc/mediatek/mmp/ \
             -I$(srctree)/drivers/misc/mediatek/ext_disp/mt8127 \
             -I$(srctree)/drivers/misc/mediatek/hdmi/hdmitx/mt8127/inc/ \
             -I$(srctree)/drivers/misc/mediatek/hdmi/inc/ \
             -I$(srctree)/drivers/misc/mediatek/leds/ \
             -I$(srctree)/drivers/staging/android/ion/ \
             -I$(srctree)/drivers/staging/android/ion/mtk

ccflags-$(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT) += -DMTK_SEC_VIDEO_PATH_SUPPORT
ccflags-$(CONFIG_MTK_DITHERING_SUPPORT) += -DDITHERING_SUPPORT
ccflags-$(CONFIG_MTK_GPU_SUPPORT) += -DHWGPU_SUPPORT
ccflags-$(CONFIG_MTK_INTERNAL_HDMI_SUPPORT) += -DMTK_INTERNAL_HDMI_SUPPORT
ccflags-$(CONFIG_MTK_OVERLAY_ENGINE_SUPPORT) += -DMTK_OVERLAY_ENGINE_SUPPORT
