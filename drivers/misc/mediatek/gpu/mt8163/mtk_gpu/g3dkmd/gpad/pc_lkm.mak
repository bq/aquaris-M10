KERNELDIR ?= /lib/modules/$(shell uname -r)/build
ARCH ?= x86
CROSS_COMPILE :=

all:
		make -C $(KERNELDIR) M=$(PWD) modules

clean:
		make -C $(KERNELDIR) M=$(PWD) clean
		rm -f *~
		rm -f *.o *.ko
