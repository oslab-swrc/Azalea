.PHONY: prepare all

ifeq ($(TARGET),)
        TARGET=knl
endif

UKDIR = ../..

#INCS = -I$(UKDIR)/include/$(TARGET) -I$(UKDIR)/include
INCS = -I$(UKDIR)/include/$(TARGET) 
CFLAGS = -Wall -D_GNU_SOURCE -g -O0 

.DEFAULT_GOAL := all

CC = gcc -g
console_proxy: console_proxy.c

	$(CC) $(CFLAGS) $(INCS) $< console_channel.o console_mmap.o console_message.o console_function.o -lpthread -o $@

prepare:
	$(CC) $(CFLAGS) $(INCS) console_channel.c -o console_channel.o -c
	$(CC) $(CFLAGS) $(INCS) console_mmap.c -o console_mmap.o -c
	$(CC) $(CFLAGS) $(INCS) console_message.c -o console_message.o -c
	$(CC) $(CFLAGS) $(INCS) console_function.c -o console_function.o -c

all: prepare console_proxy

clean:
	rm -f *.o
	rm -f tags
	rm -f console_proxy 
