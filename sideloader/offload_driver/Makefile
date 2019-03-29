
ifeq ($(TARGET),)
	TARGET=knl
endif

obj-m := offload.o
offload-y += offload_driver.o

PWD     := $(shell pwd)
CC      := gcc
AS      := as

ccflags-y := -I$(UKDIR)/include/$(TARGET) 

KDIR    := /lib/modules/$(shell uname -r)/build

default: modules
modules:
	make -C $(KDIR) M=$(PWD) modules

modules_install:
	rm -f $(MODULE_DIR)offload.k
	mkdir -p $(MODULE_DIR)
	cp offload.ko $(MODULE_DIR)
	depmod -a

clean:
	make -C $(KDIR) M=$(PWD) clean 
