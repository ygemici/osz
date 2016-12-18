TARGET = sbin/ui
EXTOBJ = ../core/logo.o avatar.o
LINKER = sharedlib

avatar.o: avatar.tga
	@objcopy -O elf64-x86-64 -B i386 -I binary avatar.tga avatar.o

include ../Makefile.rules
