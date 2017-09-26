TARGET = sys/ui
EXTOBJ = ../core/logo.o avatar.o ../../bin/service.o
LINKER = sharedlib
GENSYMS = ui.sym

all: avatar.o main.o

avatar.o: avatar.tga
	@$(LD) -r -b binary -o avatar.o avatar.tga

../core/logo.o: ../../etc/logo.tga
	@make -e --no-print-directory -C ../core logo.o

include ../Makefile.rules

ifeq ($(DEBUG),1)
gensyms:
	@../../tools/elftool.o -s 200000 ../../bin/initrd/$(TARGET) >../../bin/$(GENSYMS)
	@../../tools/elftool.o -s 216000 ../../bin/initrd/sys/lib/libc.so >>../../bin/$(GENSYMS)
endif
