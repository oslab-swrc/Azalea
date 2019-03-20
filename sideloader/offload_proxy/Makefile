ifeq ($(TARGET),)
        TARGET=knl
endif

CFLAGS := -g -I$(UKDIR)/include/$(TARGET) -I$(UKDIR)/include 
#-I$(UKDIR)/kernel

.PHONY: prepare all

.DEFAULT_GOAL := all

CC = $(CROSS_COMPILE)gcc
offload_proxy: offload_proxy.c
	$(CC) $(CFLAGS) $(LDFLAGS)  -Wall -D_GNU_SOURCE $< offload_channel.o offload_mmap.o offload_message.o offload_fio.o -lpthread -o $@

prepare:
#	$(CC) $(CFLAGS) -Wall -D_GNU_SOURCE offload.c -o offload.o -c
	$(CC) $(CFLAGS) -Wall -D_GNU_SOURCE offload_channel.c -o offload_channel.o -c
#$(CC) $(CFLAGS) -Wall -D_GNU_SOURCE offload_channel.c -o offload_channel.o -c
	$(CC) $(CFLAGS) -Wall -D_GNU_SOURCE offload_mmap.c -o offload_mmap.o -c
	$(CC) $(CFLAGS) -Wall -D_GNU_SOURCE offload_message.c -o offload_message.o -c
	$(CC) $(CFLAGS) -Wall -D_GNU_SOURCE offload_fio.c -o offload_fio.o -c
#	$(CC) $(CFLAGS) -Wall -D_GNU_SOURCE offload_aio.c -o offload_aio.o -c
#	$(CC) $(CFLAGS) -Wall -D_GNU_SOURCE offload_eventpoll.c -o offload_eventpoll.o -c
#	$(CC) $(CFLAGS) -Wall -D_GNU_SOURCE offload_readdir.c -o offload_readdir.o -c
#	$(CC) $(CFLAGS) -Wall -D_GNU_SOURCE offload_select.c -o offload_select.o -c
#	$(CC) $(CFLAGS) -Wall -D_GNU_SOURCE offload_time.c -o offload_time.o -c

all: prepare offload_proxy

clean:
	rm -f *.o
	rm -f tags
	rm -f offload_proxy 
