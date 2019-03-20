SUBDIRS=lk utils offload_driver offload_proxy

.PHONY: all clean

all:
	make -C lk UKDIR=$(UKDIR)
	make -C utils 
#	make -C cmds 
	make -C offload_driver UKDIR=$(UKDIR)
	make -C offload_proxy UKDIR=$(UKDIR)
clean: 
	make -C lk clean
	make -C utils clean
#	make -C cmds clean
	make -C offload_driver clean
	make -C offload_proxy clean
