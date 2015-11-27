#!/bin/bash

currentpath=`pwd`

function help()
{
cat <<EOF
create [arg1] [arg2]
arg1 is the product.
arg2 is eng or user version.
for example:
menuconfig e1002 eng
savedefconfig e1002 eng
EOF
}

function menuconfig()
{
    if [ $# -ne 2 ]; then
        echo "Tell me which product to create."
        return
    fi

    if [ ! -d out_$1 ]; then
		mkdir out_$1
	fi

	if [ $2 = "eng" ]; then
		make O=out_$1 $1_debug_defconfig ARCH=arm64
	elif [ $2 = "user" ]; then
		make O=out_$1 $1_defconfig ARCH=arm64
	fi

	make O=out_$1 menuconfig ARCH=arm64
}

function savedefconfig()
{
    if [ $# -ne 2 ]; then
        echo "Tell me which product to save."
        return
    fi

	if [ $2 = "eng" ]; then
		cp out_$1/.config arch/arm64/configs/$1_debug_defconfig
	elif [ $2 = "user" ]; then
		cp out_$1/.config arch/arm64/configs/$1_defconfig
	fi

	if [ -d out_$1 ]; then
		rm -rf out_$1
	fi
}