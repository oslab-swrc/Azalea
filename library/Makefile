.PHONY: all clean

ifeq ($(TARGET),)
  TARGET = knl
endif

UKDIR = ..

INCS = -I. -I$(UKDIR)/include -I$(UKDIR)/include/$(TARGET)
CFLAGS=-ffreestanding -Wall -mcmodel=large -g 
CFLAGS+=-fno-common -mno-mmx -mno-3dnow -mno-avx 
CFLAGS+=-fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -DVCONSOLE -O3 -D_$(TARGET)_

CC=gcc

CSOURCEFILES = $(wildcard *.c)
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))

all: ${COBJECTFILES} 
%.o: %.c
	${CC} $(CFLAGS) $(INCS) -c $<

clean:
	rm -f *.o tags *.o
