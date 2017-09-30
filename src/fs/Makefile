TARGET = sys/fs
LINKER = sharedlib
HASHEADER = 1
GENSYMS = fs.sym

include ../Makefile.rules

ifeq ($(DEBUG),1)
gensyms:
	@../../tools/elftool.o -s 200000 ../../bin/initrd/$(TARGET) >../../bin/$(GENSYMS)
	@../../tools/elftool.o -s 204000 ../../bin/initrd/sys/lib/libc.so >>../../bin/$(GENSYMS)
endif
