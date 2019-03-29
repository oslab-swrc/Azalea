ifeq ($(TARGET),)
        TARGET=knl
endif

LDASM32=ld -melf_x86_64 --oformat=binary -nostdlib -Ttext 0x7c00 
GCC32=gcc -ffreestanding -g -I../../include/$(TARGET) -D_$(TARGET)_

all: bootloader.bin pagetable page.bin

bootloader.o: bootloader.S
	$(GCC32) -o $@ -c $<

bootloader.bin: bootloader.o
	$(LDASM32) -o $@ $^

pagetable: pagetable.c
	gcc -o pagetable pagetable.c

page.bin: pagetable
	./pagetable

clean:
	@rm -f *.bin pagetable *.o
