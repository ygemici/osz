OS/Z Compilation
================

Requirements
------------

- GNU toolchain (make, gcc, gas, ld, objcopy, strip)
- bash (shell scripts are used to generate different files during compilation)
- optionally fasm (for recompiling non-UEFI booting)

Compilation
-----------

The make system is created in a way that you can say `make all` in
almost every directory. If you do that in the top level directory,
it will compile everything and will also build disk image for you.

For example if you enter `src/fs` and issue the command there, then only
the filesystem subsystem will be recompiled.

So to compile all the things, simply type

```shell
$ make
```

You should see something similar to this:

```
$ make clean all testq
TOOLS
  src		mkfs.c
  src		efirom.c
  src		elftool.c
LOADER
  src		efi/inffast.c
  src		efi/inflate.c
  src		efi/inftrees.c
  src		efi/bootboot.c
  src		efi-x86_64
CORE
  src		lib/sys/core (x86_64)
  src		lib/libc.so (x86_64)
USERSPACE
  src		sbin/ui
  src		bin/sh
  src		sbin/syslog
  src		sbin/fs
  src		sbin/net
  src		sbin/system
  src		sbin/init
DRIVERS
  src		mmedia/sblive.so
  src		network/ne2k.so
  src		input/ps2.so
  src		fs/gpt.so
  src		fs/vfat.so
  src		fs/fsz.so
  src		display/fb.so
IMAGES
  mkfs		initrd
  mkfs		ESP partition
  mkfs		usr partition
  mkfs		var partition
  mkfs		home partition
  mkfs		bin/disk.dd
TEST

qemu-system-x86_64 -name OS/Z -sdl -m 256 -d guest_errors -hda bin/disk.dd -option-rom loader/bootboot.bin -monitor stdio
QEMU 2.7.0 monitor - type 'help' for more information
(qemu)
```

The live disk image is generated to [bin/disk.dd](https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true). Use the
`dd` command to write it on a USB stick or boot it with qemu,
bochs or VirtualBox.

```
$ dd if=bin/disk.dd of=/dev/sdc
$ make testq
$ make testb
$ make vdi
```

Non-EFI loader
--------------

If you want to recompile `loader/mbr.bin` and `loader/bootboot.bin`, you'll need [fasm](http://flatassembler.net).
Unfortunately GAS is not good at mixing 16, 32 and 64 bit instuctions, which is necessary for BIOS booting. So
I've added those images to the source. With those pre-built binaries you should be able to compile the source using
GNU toolchain only.

The `loader/bootboot.bin` can be booted via 
 - `loader/mbr.bin` installed in MBR or VBR (see [mkfs](https://github.com/bztsrc/osz/blob/master/tools/mkfs.c) utility)
 - BIOS extension ROM (tested with qemu and bochs at 0D0000h)
 - GRUB using the MultiBoot protocol (not tested, but should be fine).
