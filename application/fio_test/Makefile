.PHONY: all clean

ifeq ($(TARGET),)
  TARGET = knl
endif

UKDIR = ../..
LIB_DIR = $(UKDIR)/library
APP_DIR = ..
NEWLIB_DIR = /opt/hermit/x86_64-hermit

CSOURCEFILES = $(wildcard *.c) 
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))

INCS = -I$(UKDIR)/include/$(TARGET) -I$(LIB_DIR) 

CC=gcc
LD=ld
CFLAGS=-c -m64 -Wall -mcmodel=large -fno-common -g -l$(LIB_DIR) -fno-stack-protector

LD = ${CROSS_COMPILE}ld 

all: ${COBJECTFILES}
%.o: %.c
	${CC} ${CFLAGS} ${INCS} -o $*.o $<
	${LD} -o uapp.elf -e main $*.o ${NEWLIB_DIR}/lib/libc.a ${LIB_DIR}/*.o 

clean:
	rm -f *.elf *.o tags

