.PHONY: all clean

ifeq ($(TARGET),)
  TARGET = knl
endif

KERNEL_DIR = ../../kernel
LIB_DIR = ../../library
ARCH_DIR = ../../include
NEWLIB_DIR = /opt/hermit/x86_64-hermit
APP_DIR = ..

CSOURCEFILES = $(wildcard *.c) 
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))

INCS = -I$(LIB_DIR) -I$(KERNEL_DIR) -I$(ARCH_DIR)/$(TARGET) -I../../include/api

CC=gcc
LD=ld
CFLAGS=-c -m64 -Wall -mcmodel=large -fno-common -g -l$(LIB_DIR) -fno-stack-protector

LD = ${CROSS_COMPILE}ld 

all: ${COBJECTFILES} 
%.o: %.c
	${CC} ${CFLAGS} ${INCS} -o $*.o $<
	${LD} -o uapp.elf -e main $*.o ${LIB_DIR}/*.o $(NEWLIB_DIR)/lib/libc.a -static -nostdlib

clean:
	rm -f *.elf *.o tags

