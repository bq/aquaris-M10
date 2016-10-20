#ccflags-y += -Werror

#obj-y += mali/drivers/

#Not build mali when enable inhouse
ifeq ($(CONFIG_MTK_GPU_SAPPHIRE_LITE), y)
obj-y += mtk_gpu/g3dkmd/
obj-y += mtk_gpu/g3dkmd/gpad/
else
#obj-y += mali-EAC/drivers/
obj-y += mali-r7p0-02rel0/drivers/
obj-y += mediatek/
endif
