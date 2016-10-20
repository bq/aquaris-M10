### Lists of source files, included by Makefiles


ifeq ($(CONFIG_MTK_GPU_SAPPHIRE_LITE), y)
G3DMKDSRC = .
VERSION_DIR = ../../
else
G3DMKDSRC = $(KMDSRCTOP)/G3D/g3dkmd
VERSION_DIR = $(KMDSRCTOP)/G3D/g3dbase
endif

ifeq ("$(G3DKMD_KernelMode)", "true")
CFILES = \
    $(G3DMKDSRC)/g3dkmd_api.c \
    $(G3DMKDSRC)/g3dkmd_api_mmu.c \
    $(G3DMKDSRC)/g3dkmd_buffer.c \
    $(G3DMKDSRC)/g3dkmd_engine.c \
    $(G3DMKDSRC)/g3dkmd_file_io.c \
    $(G3DMKDSRC)/g3dkmd_gce.c \
    $(G3DMKDSRC)/g3dkmd_hwdbg.c \
    $(G3DMKDSRC)/g3dkmd_internal_memory.c \
    $(G3DMKDSRC)/g3dkmd_ioctl.c \
    $(G3DMKDSRC)/g3dkmd_iommu.c \
    $(G3DMKDSRC)/g3dkmd_mmce.c \
    $(G3DMKDSRC)/g3dkmd_mmcq.c \
    $(G3DMKDSRC)/g3dkmd_pattern.c \
    $(G3DMKDSRC)/g3dkmd_scheduler.c \
    $(G3DMKDSRC)/g3dkmd_signal.c \
    $(G3DMKDSRC)/g3dkmd_task.c \
    $(G3DMKDSRC)/g3dkmd_fakemem.c \
    $(G3DMKDSRC)/g3dkmd_msgq.c \
    $(G3DMKDSRC)/g3dkmd_fdq.c \
    $(G3DMKDSRC)/g3dkmd_isr.c \
    $(G3DMKDSRC)/g3dkmd_cfg.c \
    $(G3DMKDSRC)/g3dkmd_fence.c \
    $(G3DMKDSRC)/g3dkmd_recovery.c \
    $(G3DMKDSRC)/g3dkmd_pm.c \
    $(G3DMKDSRC)/g3dkmd_power.c \
    $(G3DMKDSRC)/g3dkmd_profiler.c \
    $(G3DMKDSRC)/g3dkmd_memory_list.c \
    $(G3DMKDSRC)/g3dkmd_ion.c \
    $(G3DMKDSRC)/g3dkmd_met.c \
    $(G3DMKDSRC)/g3dkmd_profiler.c

else
CFILES = \
    $(G3DMKDSRC)/g3dkmd_api.c \
    $(G3DMKDSRC)/g3dkmd_api_mmu.c \
    $(G3DMKDSRC)/g3dkmd_buffer.c \
    $(G3DMKDSRC)/g3dkmd_engine.c \
    $(G3DMKDSRC)/g3dkmd_file_io.c \
    $(G3DMKDSRC)/g3dkmd_gce.c \
    $(G3DMKDSRC)/g3dkmd_hwdbg.c \
    $(G3DMKDSRC)/g3dkmd_internal_memory.c \
    $(G3DMKDSRC)/g3dkmd_iommu.c \
    $(G3DMKDSRC)/g3dkmd_mmce.c \
    $(G3DMKDSRC)/g3dkmd_mmcq.c \
    $(G3DMKDSRC)/g3dkmd_pattern.c \
    $(G3DMKDSRC)/g3dkmd_scheduler.c \
    $(G3DMKDSRC)/g3dkmd_task.c \
    $(G3DMKDSRC)/g3dkmd_fakemem.c \
    $(G3DMKDSRC)/g3dkmd_msgq.c \
    $(G3DMKDSRC)/g3dkmd_fdq.c \
    $(G3DMKDSRC)/g3dkmd_isr.c \
    $(G3DMKDSRC)/g3dkmd_cfg.c \
    $(G3DMKDSRC)/g3dkmd_pm.c \
    $(G3DMKDSRC)/g3dkmd_profiler.c \
    $(G3DMKDSRC)/g3dkmd_memory_list.c \
    $(G3DMKDSRC)/g3dkmd_power.c
endif


ifneq ($(CONFIG_MTK_GPU_SAPPHIRE_LITE), y)

ifeq ("$(G3DKMD_KernelMode)", "true")
CFILES += \
    $(G3DMKDSRC)/bufferfile/g3dkmd_bufferfile_io.c \
    $(G3DMKDSRC)/bufferfile/g3dkmd_bufferfile.c \
    $(G3DMKDSRC)/profiler/g3dkmd_prf_kmemory.c \
    $(G3DMKDSRC)/profiler/g3dkmd_prf_ringbuffer.c \
    $(G3DMKDSRC)/test/tester_core.c \
    $(G3DMKDSRC)/test/tester_non_polling_qload.c    \
    $(G3DMKDSRC)/test/tester_non_polling_flushcmd.c \
    $(G3DMKDSRC)/debugger/debugfs/g3dkmd_debugfs.c  \
    $(G3DMKDSRC)/debugger/debugfs/g3dkmd_debugfs_helper.c  \
    $(G3DMKDSRC)/debugger/debugfs/g3dkmd_debugfs_aregs.c \
    $(G3DMKDSRC)/debugger/debugfs/g3dkmd_debugfs_task.c  \
    $(G3DMKDSRC)/debugger/debugfs/g3dkmd_debugfs_log.c  \
    $(G3DMKDSRC)/debugger/debugfs/g3dkmd_debugfs_profiler.c \
    $(G3DMKDSRC)/debugger/debugfs/g3dkmd_debugfs_fdq.c \
    $(G3DMKDSRC)/debugger/debugfs/g3dkmd_debugfs_helper.c \
    $(G3DMKDSRC)/mpd/g3dkmd_mpd.c

ifeq ($(ANDROID_MK), TRUE)
EXTRA_CFLAGS += -DCONFIG_SW_SYNC64
CFILES += \
    $(G3DMKDSRC)/sync64/sw_sync64.c
endif

endif

CFILES += \
    $(G3DMKDSRC)/mmu/yl_mmu_bin_internal.c \
    $(G3DMKDSRC)/mmu/yl_mmu_common_internal.c \
    $(G3DMKDSRC)/mmu/yl_mmu_hw_table.c \
    $(G3DMKDSRC)/mmu/yl_mmu_iterator.c \
    $(G3DMKDSRC)/mmu/yl_mmu_kernel_alloc.c \
    $(G3DMKDSRC)/mmu/yl_mmu_mapper.c \
    $(G3DMKDSRC)/mmu/yl_mmu_mapper_helper.c \
    $(G3DMKDSRC)/mmu/yl_mmu_mapper_internal.c \
    $(G3DMKDSRC)/mmu/yl_mmu_memory_functions.c \
    $(G3DMKDSRC)/mmu/yl_mmu_rcache.c \
    $(G3DMKDSRC)/mmu/yl_mmu_section_lookup.c \
    $(G3DMKDSRC)/mmu/yl_mmu_system_address_translator.c \
    $(G3DMKDSRC)/mmu/yl_mmu_trunk_internal.c \
    $(G3DMKDSRC)/mmu/yl_mmu_trunk_metadata_allocator.c \
    $(G3DMKDSRC)/mmu/yl_mmu_utility.c

CFILES += \
    $(G3DMKDSRC)/m4u/m4u.c

CFILES += \
    $(VERSION_DIR)/yl_version.c
endif

ifeq ("$(G3DKMD_KernelMode)", "false")

INCLUDE_DIRS = -I$(G3DTOP)/include \
               -I$(G3DTOP)/src/gallium/drivers/softpipe

ifeq ($(ANDROID_MK), TRUE)
INCLUDE_DIRS += -I$(G3DTOP)/G3D/g3d \
                -I$(G3DTOP)/G3D/g3dkmd \
                -I$(G3DTOP)/G3D/g3dbase \
                -I$(G3DTOP)/G3D/sapphire_lite
else
INCLUDE_DIRS += -I$(G3DTOP)/../../G3D/g3d \
                -I$(G3DTOP)/../../G3D/g3dkmd \
                -I$(G3DTOP)/../../G3D/g3dbase \
                -I$(G3DTOP)/../../G3D/sapphire_lite
endif

CFILES += \
	$(G3DMKDSRC)/mmu/dump/yl_mmu_utility_common.c \
	$(G3DMKDSRC)/mmu/dump/yl_mmu_utility_helper.c \
	$(G3DMKDSRC)/mmu/dump/yl_mmu_utility_private.c \
	$(G3DMKDSRC)/mmu/dump/yl_mmu_utility_sanity_page.c \
	$(G3DMKDSRC)/mmu/dump/yl_mmu_utility_sanity_trunk.c \
	$(G3DMKDSRC)/mmu/dump/yl_mmu_utility_statistics_trunk.c
else
ifeq ($(ANDROID_MK), TRUE)
CHEADERS = \
    $(G3DMKDSRC)/g3dkmd_api_common.h \
    $(G3DMKDSRC)/g3dkmd_api.h \
    $(G3DMKDSRC)/g3dkmd_api_mmu.h \
    $(G3DMKDSRC)/g3dkmd_buffer.h \
    $(G3DMKDSRC)/g3dkmd_cfg.h \
    $(G3DMKDSRC)/g3dkmd_define.h \
    $(G3DMKDSRC)/g3dkmd_engine.h \
    $(G3DMKDSRC)/g3dkmd_fdq.h \
    $(G3DMKDSRC)/g3dkmd_fence.h \
    $(G3DMKDSRC)/g3dkmd_file_io.h \
    $(G3DMKDSRC)/bufferfile/g3dkmd_bufferfile_io.h \
    $(G3DMKDSRC)/bufferfile/g3dkmd_bufferfile.h \
    $(G3DMKDSRC)/test/tester_non_polling_flushcmd.h \
    $(G3DMKDSRC)/g3dkmd_gce.h \
    $(G3DMKDSRC)/g3dkmd_hwdbg.h \
    $(G3DMKDSRC)/g3dkmd_internal_memory.h \
    $(G3DMKDSRC)/g3dkmd_iommu.h \
    $(G3DMKDSRC)/g3dkmd_iommu_reg.h \
    $(G3DMKDSRC)/g3dkmd_isr.h \
    $(G3DMKDSRC)/g3dkmd_macro.h \
    $(G3DMKDSRC)/g3dkmd_mmce.h \
    $(G3DMKDSRC)/g3dkmd_mmce_reg.h \
    $(G3DMKDSRC)/g3dkmd_mmcq.h \
    $(G3DMKDSRC)/g3dkmd_mmcq_internal.h \
    $(G3DMKDSRC)/g3dkmd_msgq.h \
    $(G3DMKDSRC)/g3dkmd_pattern.h \
    $(G3DMKDSRC)/g3dkmd_scheduler.h \
    $(G3DMKDSRC)/g3dkmd_signal.h \
    $(G3DMKDSRC)/g3dkmd_task.h \
    $(G3DMKDSRC)/g3dkmd_util.h \
    $(G3DMKDSRC)/g3dkmd_pm.h \
    $(G3DMKDSRC)/g3dkmd_power.h \
    $(G3DMKDSRC)/g3dkmd_ion.h \
    $(G3DMKDSRC)/g3dkmd_met.h \
    $(G3DMKDSRC)/g3dkmd_fakemem.h \
    $(G3DMKDSRC)/g3dkmd_recovery.h \
    $(G3DMKDSRC)/mmu/yl_mmu_bin_internal.h \
    $(G3DMKDSRC)/mmu/yl_mmu_bin_public_class.h \
    $(G3DMKDSRC)/mmu/yl_mmu_common.h \
    $(G3DMKDSRC)/mmu/yl_mmu_common_internal.h \
    $(G3DMKDSRC)/mmu/yl_mmu.h \
    $(G3DMKDSRC)/mmu/yl_mmu_hw_table.h \
    $(G3DMKDSRC)/mmu/yl_mmu_hw_table_helper.h \
    $(G3DMKDSRC)/mmu/yl_mmu_hw_table_private.h \
    $(G3DMKDSRC)/mmu/yl_mmu_hw_table_public_class.h \
    $(G3DMKDSRC)/mmu/yl_mmu_hw_table_public_const.h \
    $(G3DMKDSRC)/mmu/yl_mmu_index_helper.h \
    $(G3DMKDSRC)/mmu/yl_mmu_index_public_class.h \
    $(G3DMKDSRC)/mmu/yl_mmu_iterator.h \
    $(G3DMKDSRC)/mmu/yl_mmu_iterator_public_class.h \
    $(G3DMKDSRC)/mmu/yl_mmu_kernel_alloc.h \
    $(G3DMKDSRC)/mmu/yl_mmu_mapper.h \
    $(G3DMKDSRC)/mmu/yl_mmu_mapper_helper.h \
    $(G3DMKDSRC)/mmu/yl_mmu_mapper_internal.h \
    $(G3DMKDSRC)/mmu/yl_mmu_mapper_public_class.h \
    $(G3DMKDSRC)/mmu/yl_mmu_memory_functions.h \
    $(G3DMKDSRC)/mmu/yl_mmu_rcache.h \
    $(G3DMKDSRC)/mmu/yl_mmu_rcache_public_class.h \
    $(G3DMKDSRC)/mmu/yl_mmu_section_lookup.h \
    $(G3DMKDSRC)/mmu/yl_mmu_section_lookup_public_class.h \
    $(G3DMKDSRC)/mmu/yl_mmu_size_helper.h \
    $(G3DMKDSRC)/mmu/yl_mmu_system_address_translator.h \
    $(G3DMKDSRC)/mmu/yl_mmu_trunk_helper_link.h \
    $(G3DMKDSRC)/mmu/yl_mmu_trunk_helper_size.h \
    $(G3DMKDSRC)/mmu/yl_mmu_trunk_internal.h \
    $(G3DMKDSRC)/mmu/yl_mmu_trunk_metadata_allocator.h \
    $(G3DMKDSRC)/mmu/yl_mmu_trunk_private.h \
    $(G3DMKDSRC)/mmu/yl_mmu_trunk_public_class.h \
    $(G3DMKDSRC)/mmu/yl_mmu_utility.h \
    $(G3DMKDSRC)/m4u/m4u_port.h \
    $(G3DMKDSRC)/m4u/m4u.h \
    $(G3DMKDSRC)/../g3dbase/g3d_memory.h \
    $(G3DMKDSRC)/../g3dbase/g3d_memory_utility.h \
    $(G3DMKDSRC)/../g3dbase/g3dbase_buffer.h \
    $(G3DMKDSRC)/../g3dbase/g3dbase_common.h \
    $(G3DMKDSRC)/../g3dbase/g3dbase_common_define.h \
    $(G3DMKDSRC)/../g3dbase/g3dbase_type.h \
    $(G3DMKDSRC)/../g3dbase/g3dbase_ioctl.h \
    $(G3DMKDSRC)/../g3dbase/yl_define.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_areg.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_cmd.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_cmodel.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_freg.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_ht.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_mx_dbg.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_pp_cb.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_pp_cpb.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_pp_cpf.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_pp_fs.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_pp_xy.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_pp_zpb.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_pp_zpf.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_qreg.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_reg.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_rsm.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_tx.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_ux_cmn.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_ux_dwp.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_vp.h \
    $(G3DMKDSRC)/../sapphire_lite/sapphire_vp_vl_pkt.h \
    $(VERSION_DIR)/yl_version.h
endif
endif
