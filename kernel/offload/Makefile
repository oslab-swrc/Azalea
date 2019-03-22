ifeq ($(TARGET),)
        TARGET=knl
endif

CFLAG=-m64 -mno-red-zone -nostdlib -ffreestanding -Wall -mcmodel=large -g -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -DVCONSOLE -O3 -I. -I.. -I../../include/api -I../../include/$(TARGET)

GCC64=gcc $(CFLAG)

CSOURCEFILES=$(wildcard *.c)
COBJECTFILES=$(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))

.PHONEY: all 

all: $(COBJECTFILES)
	cp *.o ..

%.o: %.c
	$(GCC64) -c $<

clean:
	rm -f *.o



