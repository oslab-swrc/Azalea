KDIR = /lib/modules/$(shell uname -r)/build
UKDIR ?= $(shell pwd)
ifneq ("$(V)", "1")
  VERBOSE := --quiet
endif

.PHONY: all sideloader kernel_src utility_src sideloader library application clean

ifeq ($(TARGET),)
all: kernel_src utility_src sideloader library application disk.img ctags
else
all: kernel_src utility_src sideloader library application qdisk.img ctags
endif

kernel_src:
	@echo "===== Build Kernel start    ====="
	make -C kernel $(VERBOSE)
	@echo "===== Build Kernel complete ====="
	@echo

utility_src:
	@echo "===== Build Utility start    ====="
	make -C utility $(VERBOSE)
	@echo "===== Build Utility complete ====="
	@echo

library:
	@echo "=====  Build Library start    ====="
	make -C library $(VERBOSE)
	@echo "===== Build Library complete ====="
	@echo

application:
	@echo "===== Build Application start    ====="
	make -C application $(VERBOSE) APP=$(APP) NR=${NR}
	@echo "===== Build Application complete ====="
	@echo

disk.img: kernel/kernel_32.bin kernel/kernel64.bin application/uapp.elf
	@echo "===== Disk image build start    ====="
	./utility/imagemaker/imagemaker $^
	@echo "===== Disk image build complete ====="
	@echo

qdisk.img: kernel/bootloader/bootloader.bin kernel/kernel_32.bin kernel/bootloader/page.bin kernel/kernel64.bin application/uapp.elf
	@echo "===== qDisk image build start    ====="
	./utility/imagemaker/qimagemaker $^
	@echo "===== qDisk image build complete ====="
	@echo

sideloader:
	@echo "===== Build Sideloader start    ====="
	make -C sideloader KDIR=${KDIR} UKDIR=${UKDIR} $(VERBOSE)
	@echo "===== Build Sideloader complete ====="
	@echo

ctags:
	ctags -R .

clean:
	make -C kernel $(VERBOSE) clean
	make -C utility $(VERBOSE) clean
	make -C library $(VERBOSE) clean
	make -C application $(VERBOSE) clean
	make -C sideloader clean KDIR=${KDIR} UKDIR=${UKDIR} $(VERBOSE)

	rm -f tags disk.img qdisk.img
