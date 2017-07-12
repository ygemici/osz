TARGET = sys/bin/sys
HASHEADER = 1
GENSYMS = sys.sym

include ../Makefile.rules

ifeq ($(DEBUG),1)
gensyms:
	@../../tools/elftool.o -s 200000 ../../bin/initrd/$(TARGET) >../../bin/$(GENSYMS)
	@../../tools/elftool.o -s 202000 ../../bin/initrd/sys/lib/libc.so >>../../bin/$(GENSYMS)
endif
