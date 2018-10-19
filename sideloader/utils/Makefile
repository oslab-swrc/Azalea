

SRCS=$(shell ls *.c)
OBJS=$(SRCS:.c=.o)
TARGETS=$(OBJS:.o=)
CFLAGS := -g -I../../include/knl/ 

.c.o:
	$(CROSS_COMPILE)gcc -c -o $@ $< $(CFLAGS)

.PHONEY: all

all: $(TARGETS)
	
$(TARGETS): %: %.o
	$(CROSS_COMPILE)gcc -o $@ $<

clean:
	rm -f $(OBJS) $(TARGETS)

