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

include $(srctree)/drivers/misc/mediatek/video/mt8127/ccflags.mk

obj-y += ddp_drv.o
obj-y += ddp_path.o
obj-y += ddp_ovl.o
obj-y += ddp_bls.o
obj-y += ddp_color.o
obj-y += ddp_rdma.o
obj-y += ddp_wdma.o
obj-y += ddp_aal.o
obj-y += ddp_debug.o
obj-y += lcd_drv.o
obj-y += dpi_drv.o
obj-$(CONFIG_MTK_HDMI_SUPPORT) += dpi1_drv.o
obj-y += dsi_drv.o
obj-y += ddp_cmdq.o
obj-y += ddp_cmdq_record.o
obj-y += ddp_cmdq_debug.o
obj-y += ddp_cmdq_sec.o
obj-y += cmdq_ccf.o
obj-y += ddp_cmdq_test.o
#obj-y += ddp_dpfd.o

ccflags-y += -DMTK_ENABLE_CMDQ
ccflags-y += -I$(srctree)/drivers/misc/mediatek/leds/

