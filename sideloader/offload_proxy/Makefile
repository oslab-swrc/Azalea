.PHONY: prepare all

ifeq ($(TARGET),)
        TARGET=knl
endif

UKDIR = ../..

INCS = -I$(UKDIR)/include/$(TARGET) -I$(UKDIR)/include
CFLAGS = -Wall -D_GNU_SOURCE -g  


.DEFAULT_GOAL := all

CC = gcc
offload_local_proxy: offload_local_proxy.c

	$(CC) $(CFLAGS) $(INCS) $< offload_channel.o offload_mmap.o offload_message.o offload_fio.o -lpthread -o $@

prepare:
	$(CC) $(CFLAGS) $(INCS) offload_channel.c -o offload_channel.o -c
	$(CC) $(CFLAGS) $(INCS) offload_mmap.c -o offload_mmap.o -c
	$(CC) $(CFLAGS) $(INCS) offload_message.c -o offload_message.o -c
	$(CC) $(CFLAGS) $(INCS) offload_fio.c -o offload_fio.o -c

all: prepare offload_local_proxy

clean:
	rm -f *.o
	rm -f tags
	rm -f offload_local_proxy 
