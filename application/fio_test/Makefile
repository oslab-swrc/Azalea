.PHONY: all clean

ifeq ($(TARGET),)
  TARGET = knl
endif

#KERNEL_DIR = ../../kernel
UKDIR = ../../
LIB_DIR = $(UKDIR)/library
#ARCH_DIR = ../../include
APP_DIR = ..
NEWLIB_DIR = /opt/hermit/x86_64-hermit

CSOURCEFILES = $(wildcard *.c) 
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))

#INCS = -I$(LIB_DIR) -I$(KERNEL_DIR) -I$(ARCH_DIR)/$(TARGET)
INCS =  -I$(UKDIR)/include/$(TARGET) -I$(LIB_DIR) -I$(NEWLIB_DIR)/include 

CC=gcc
LD=ld
CFLAGS=-c -m64 -Wall -mcmodel=large -fno-common -g -l$(LIB_DIR) -fno-stack-protector

LD = ${CROSS_COMPILE}ld 

all: ${COBJECTFILES} COPY 
%.o: %.c
	${CC} ${CFLAGS} ${INCS} -o $*.o $<
#	${LD} -o uapp.elf $*.o  /usr/lib64/libc.a -static ${LIB_DIR}/utility.o 
	${LD} -o uapp.elf -e main $*.o ${NEWLIB_DIR}/lib/libc.a ${LIB_DIR}/*.o 

COPY:
	cp *.elf ${APP_DIR}/

clean:
	rm -f *.elf *.o tags
	rm -f ${APP_DIR}/*.elf

