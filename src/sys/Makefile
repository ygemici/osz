TARGET = sbin/sys
GENSYMS = sys.sym

include ../Makefile.rules

ifeq ($(DEBUG),1)
gensyms:
	@../../tools/elftool.o -s 200000 ../core/$(ARCH)/user.o >../../bin/$(GENSYMS)
	@../../tools/elftool.o -s 203000 ../../bin/root/lib/libc.so >>../../bin/$(GENSYMS)
	@../../tools/elftool.o -s 205000 ../../bin/root/lib/sys/proc/pit.so >>../../bin/$(GENSYMS)
	@../../tools/elftool.o -s 207000 ../../bin/root/lib/sys/input/ps2.so >>../../bin/$(GENSYMS)
	@../../tools/elftool.o -s 209000 ../../bin/root/lib/sys/display/fb.so >>../../bin/$(GENSYMS)
endif
