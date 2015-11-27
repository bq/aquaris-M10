#ccflags-y += -Werror

#obj-y += mali/drivers/

#Not build mali when enable inhouse
ifeq ($(CONFIG_MTK_INHOUSE_GPU), y)
obj-y += mtk_gpu/g3dkmd/
obj-y += mtk_gpu/g3dkmd/gpad/
else
obj-y += mali-EAC/drivers/
endif
