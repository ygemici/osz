/*
 * loader/mykernel/Makefile
 *
 * Copyright 2017 Public Domain BOOTBOOT bztsrc@github
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief An example Makefile for sample kernel
 *
 */

all: mykernel.x86_64.elf mykernel.aarch64.elf

mykernel.x86_64.elf: kernel.c
	x86_64-elf-gcc -Wall -fpic -ffreestanding -fno-stack-protector -nostdinc -nostdlib -I../ -mno-red-zone -c kernel.c -o kernel.o
	x86_64-elf-ld -nostdlib -nostartfiles --nmagic -T link.ld kernel.o -o mykernel.x86_64.elf
	x86_64-elf-strip -s -K fb -K bootboot -K environment mykernel.x86_64.elf

mykernel.aarch64.elf: kernel.c
	aarch64-elf-gcc -Wall -fpic -ffreestanding -fno-stack-protector -nostdinc -nostdlib -I../ -c kernel.c -o kernel.o
	aarch64-elf-ld -nostdlib -nostartfiles --nmagic -T link.ld kernel.o -o mykernel.aarch64.elf
	aarch64-elf-strip -s -K fb -K bootboot -K environment mykernel.aarch64.elf

clean:
	rm *.o *.elf
