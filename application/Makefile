ifeq ("$(APP)", "")
  TARGET := helloworld
else
  TARGET := $(APP)
endif

SUBDIRS := $(wildcard */)

.PHONY: all clean $(SUBDIRS) $(SUBDIRS_RM)

all:
	@echo "*** Target APP: $(TARGET)"
	for DIRS in $(SUBDIRS); do \
		make -C $$DIRS;\
	done
	@cp $(TARGET)/uapp.elf .

clean:
	for DIRS in $(SUBDIRS); do \
		make -C $$DIRS clean;\
	done
	@rm -rf uapp.elf
