PWD := $(shell pwd)
KVERSION := $(shell uname -r)
KERNEL_DIR = /usr/src/linux-headers-$(KVERSION)/

MODULE_NAME = mykobj

obj-m := $(MODULE_NAME).o

CC := /usr/bin/gcc-12

all :
	make -C $(KERNEL_DIR) M=$(PWD) CC=$(CC) modules
clean :
	make -C $(KERNEL_DIR) M=$(PWD) clean
