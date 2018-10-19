
ifeq ($(TARGET),)
	TARGET=knl
endif

obj-m := lk.o
lk-y += lkernel.o

PWD     := $(shell pwd)
CC      := gcc
AS      := as

ccflags-y := -I$(UKDIR)/include/$(TARGET) 

KDIR    := /lib/modules/$(shell uname -r)/build

default: modules
modules:
	make -C $(KDIR) M=$(PWD) modules

modules_install:
	rm -f $(MODULE_DIR)lk.k
	mkdir -p $(MODULE_DIR)
	cp lk.ko $(MODULE_DIR)
	depmod -a

clean:
	make -C $(KDIR) M=$(PWD) clean 
