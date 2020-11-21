#
# Makefile for kernel test
#
PWD         := $(shell pwd) 
KVERSION    := $(shell uname -r)
KERNEL_DIR   = /home/lumia/arm-reference-platforms/linux/out/juno/android

MODULE_NAME  = lkm_etm
obj-m       := $(MODULE_NAME).o   

all:
	make -C $(KERNEL_DIR) M=$(PWD) ARCH=arm64 CROSS_COMPILE=/home/lumia/arm-reference-platforms/tools/gcc/gcc-linaro-6.2.1-2016.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu- modules
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean