WHAT IS THIS?
=============

Linux Kernel source code for the devices:
* bq aquaris M10


BUILD INSTRUCTIONS?
===================

Specific sources are separated by branches and each version is tagged with it's corresponding number. First, you should
clone the project:

        $ git clone https://github.com/bq/aquaris-M10.git

After it, choose the version you would like to build:

*Aquaris M10 *

        $ cd aquaris-M10
        $ git checkout aquaris-M10_1.x

At the same level of the "kernel" directory:

Create KERNEL_OUT dir:

        $ mkdir KERNEL_OUT   

Your directory tree should look like this:
* cit-aarch64-linux-android-4.9
* kernel
* KERNEL_OUT

Finally, build the kernel according the next table of product names:

| device                                                                                | product                                                               |
| --------------------------|-------------------------|
| bq aquaris M10                                    | bq_aquaris_m10                                      |


        $ make -C kernel  O=../KERNEL_OUT  ARCH=arm64 CROSS_COMPILE=../cit-aarch64-linux-android-4.9/bin/aarch64-linux-android- {product}_defconfig
        $ make O=../KERNEL_OUT/ -C kernel ARCH=arm64  CROSS_COMPILE=../cit-aarch64-linux-android-4.9/bin/aarch64-linux-android-                      
    
You can specify "-j CORES" argument to speed-up your compilation, example:

        $ make O=../KERNEL_OUT/ -C kernel ARCH=arm64  CROSS_COMPILE=../cit-aarch64-linux-android-4.9/bin/aarch64-linux-android- -j 8

