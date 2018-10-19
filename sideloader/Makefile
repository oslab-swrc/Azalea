SUBDIRS=lk utils

.PHONY: all clean

all:
	make -C lk UKDIR=$(UKDIR)
	make -C utils 
#	make -C cmds 
clean: 
	make -C lk clean
	make -C utils clean
#	make -C cmds clean
