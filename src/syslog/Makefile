TARGET = sbin/syslog
LINKER = sharedlib
EXTOBJ = ../../bin/service.o
GENSYMS = syslog.sym

include ../Makefile.rules

ifeq ($(DEBUG),1)
gensyms:
	@../../tools/elftool.o -s 200000 ../../bin/initrd/$(TARGET) >../../bin/$(GENSYMS)
	@../../tools/elftool.o -s 202000 ../../bin/initrd/lib/libc.so >>../../bin/$(GENSYMS)
endif
