PWD := $(shell pwd)
KVERSION := $(shell uname -r)
KERNEL_DIR = /usr/src/linux-headers-$(KVERSION)/

MODULE_NAME = mykset

obj-m := $(MODULE_NAME).o

CC := /usr/bin/gcc-12
SYSFS_PATH := /usr/src/linux-headers-6.5.0-27-generic/include/linux/sysfs.h

all :
	make -C $(KERNEL_DIR) M=$(PWD) CC=$(CC) -I$(SYSFS_PATH) modules
clean :
	make -C $(KERNEL_DIR) M=$(PWD) clean

