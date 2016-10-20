BUILD_DIR ?= ~/linux_$(ARCH)
QEMU_DIR ?= ~/qemu_linux_$(ARCH)

all:
		make -C $(BUILD_DIR) M=$(PWD) modules
		cp *.ko $(QEMU_DIR)

clean:
		make -C $(BUILD_DIR) M=$(PWD) clean
		rm -f *~
		rm -f *.o *.ko
