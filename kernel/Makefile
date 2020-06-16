ifeq ($(TARGET),)
        TARGET=knl
endif

TEXT_BASE64=0xFFFF8000C0300000
INCLUDES=-I. -Ioffload -Iconsole -Ishm -Isystemcall -I../include/$(TARGET) -I../include/api 
CFLAG=-m64 -mno-red-zone -nostdlib -ffreestanding -Wall -mcmodel=large -g
CFLAG+=-mno-mmx -mno-3dnow -mno-avx
CFLAG+=-fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -DVCONSOLE -O0 -D_$(TARGET)_
LDFLAG=-melf_x86_64 -T elf_x86_64.x

GCC64=gcc $(CFLAG) $(INCLUDES)
LD64=ld $(LDFLAG) -nostdlib -e Main -Ttext $(TEXT_BASE64)
OBJCOPY64=objcopy -j .text -j .data -j .rodata -j .bss --set-section-flags .bss=alloc,load,contents -S -O binary

AFLAG=-D__ASSEMBLY__

ENTRYPOINTSOURCEFILE=entrypoint.S
ENTRYPOINTOBJECTFILE=entrypoint.o

CSOURCEFILES=$(patsubst %_32.c, ,$(wildcard *.c) $(wildcard ./offload/*.c) $(wildcard ./console/*.c) $(wildcard ./shm/*.c) $(wildcard ./systemcall/*.c))
ASSEMBLYSOURCEFILES=$(patsubst %_32.S, ,$(wildcard *.S))
COBJECTFILES=$(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))
ASSEMBLYOBJECTFILES=$(notdir $(patsubst %.S,%.o,$(ASSEMBLYSOURCEFILES)))

.PHONEY: prepare all ExecuteInternalBuild InternalDependency dep

all: prepare kernel_32.bin kernel64.bin 

kernel_32.bin: 
	make -f Makefile_32 TARGET=$(TARGET) 

kernel64.bin: ExecuteInternalBuild
	$(OBJCOPY64) kernel64.elf $@

ExecuteInternalBuild: dep kernel64.elf

dep:
	@echo "*** Making dependency file"
	$(GCC64) -MM $(CSOURCEFILES) > Dependency.dep
	@echo "*** Dependency search completed"

kernel64.elf: $(ENTRYPOINTOBJECTFILE) $(COBJECTFILES) $(ASSEMBLYOBJECTFILES) 
	$(LD64) -o $@ $^ 

$(ENTRYPOINTOBJECTFILE): $(ENTRYPOINTSOURCEFILE)
	$(GCC64) -D__ASSEMBLY__ -o $@ -c $<

%.o: %.c
	$(GCC64) -c $<
%.o: %.S
	$(GCC64) $(AFLAG) -c $<

prepare:
	make -C offload
	make -C console 
	make -C shm 
	make -C systemcall

clean:
	rm -f *.bin *.o *.elf tags Dependency.dep
	make -f Makefile_32 clean
	make -C offload clean
	make -C console clean
	make -C shm clean
	make -C systemcall clean

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
