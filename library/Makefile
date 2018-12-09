.PHONY: all clean

ifeq ($(TARGET),)
  TARGET = knl
endif

KERNEL_DIR = ../kernel
ARCH_DIR = ../include
NEWLIB_DIR = /opt/hermit/x86_64-hermit

CSOURCEFILES = $(wildcard *.c)
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))

INCS = -I$(KERNEL_DIR) -I$(ARCH_DIR)/$(TARGET)

CC=gcc

CFLAGS=-c -ffreestanding -Wall -mcmodel=large -fno-common -mno-mmx -mno-3dnow -mno-avx 
CFLAGS+=-fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -DMIC -DTESTBED -DVCONSOLE -D_$(TARGET)_
#CFLAGS += -I $(NEWLIB_DIR)/include -L $(NEWLIB_DIR)/lib

all: ${COBJECTFILES} 
%.o: %.c
	${CC} $(CFLAGS) $(INCS) -o $*.o $<

clean:
	rm -f *.o tags *.o
