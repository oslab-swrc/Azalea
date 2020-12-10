ifeq ("$(APP)", "")
  TARGETS := heater
else
  TARGETS := $(APP)
endif

SUBDIRS := $(wildcard */)

.PHONY: all clean $(SUBDIRS) $(SUBDIRS_RM)

all:
	@echo "*** Target APP: $(TARGETS)"
	for DIRS in $(SUBDIRS); do \
		make -C $$DIRS NR=$(NR);\
	done
	@cp $(TARGETS)/uapp.elf .

clean:
	for DIRS in $(SUBDIRS); do \
		make -C $$DIRS clean;\
	done
	@rm -rf uapp.elf
