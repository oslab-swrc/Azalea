.PHONY: all clean

ifeq ($(TARGET),)
        TARGET=knl
endif

UKDIR=../..

INCS=-I. -I.. -I../offload -I$(UKDIR)/include/api -I$(UKDIR)/include/$(TARGET)
#INCS=-I. -I.. -I$(UKDIR)/include/api -I$(UKDIR)/include/$(TARGET)
CFLAGS=-m64 -mno-red-zone -nostdlib -ffreestanding -Wall -mcmodel=large -g 
CFLAGS+=-mno-mmx -mno-3dnow -mno-avx
CFLAGS+=-fno-common -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -DVCONSOLE -O3 -D_$(TARGET)_

GCC64=gcc

CSOURCEFILES=$(wildcard *.c)
COBJECTFILES=$(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))

all: $(COBJECTFILES)
	cp *.o ..

%.o: %.c
	$(GCC64) $(CFLAGS) $(INCS) -c $<

clean:
	rm -f *.o



