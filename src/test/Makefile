include ../../Config
ifeq ($(DEBUG),1)
TARGET = sys/bin/test
GENSYMS = test.sym

include ../Makefile.rules

gensyms:
	@../../tools/elftool.o -s 200000 ../../bin/initrd/$(TARGET) >../../bin/$(GENSYMS)
	@../../tools/elftool.o -s 202000 ../../bin/initrd/sys/lib/libc.so >>../../bin/$(GENSYMS)
else
all:
	@true
endif
