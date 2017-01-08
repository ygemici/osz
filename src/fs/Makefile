TARGET = sbin/fs
LINKER = sharedlib
EXTOBJ = ../../bin/service.o
GENSYMS = fs.sym

include ../Makefile.rules

ifeq ($(DEBUG),1)
gensyms:
	@../../tools/elftool.o -s 200000 ../../bin/root/$(TARGET) >../../bin/$(GENSYMS)
	@../../tools/elftool.o -s 202000 ../../bin/root/lib/libc.so >>../../bin/$(GENSYMS)
endif
