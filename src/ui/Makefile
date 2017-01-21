TARGET = sbin/ui
EXTOBJ = ../core/logo.o avatar.o
LINKER = sharedlib
GENSYMS = ui.sym

all: avatar.o main.o

avatar.o: avatar.tga
	@objcopy -O elf64-x86-64 -B i386 -I binary avatar.tga avatar.o

include ../Makefile.rules

ifeq ($(DEBUG),1)
gensyms:
	@../../tools/elftool.o -s 200000 ../../bin/root/$(TARGET) >../../bin/$(GENSYMS)
	@../../tools/elftool.o -s 216000 ../../bin/root/lib/libc.so >>../../bin/$(GENSYMS)
endif
