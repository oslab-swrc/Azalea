.PHONY: all clean

ifeq ($(TARGET),)
  TARGET = knl
endif

NEWLIB_DIR = /opt/hermit/x86_64-hermit
UKDIR = ../
CSOURCEFILES = $(wildcard *.c)
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))

INCS = -I. -I$(UKDIR)/include -I$(UKDIR)/include/$(TARGET) -I$(NEWLIB_DIR)/include

CC=gcc

CFLAGS=-c -ffreestanding -Wall -mcmodel=large -fno-common -mno-mmx -mno-3dnow -mno-avx 
CFLAGS+=-fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -DMIC -DTESTBED -DVCONSOLE -D_$(TARGET)_
#CFLAGS += -I $(NEWLIB_DIR)/include -L $(NEWLIB_DIR)/lib

all: ${COBJECTFILES} 
%.o: %.c
	${CC} $(CFLAGS) $(INCS) -o $*.o $<

clean:
	rm -f *.o tags *.o
