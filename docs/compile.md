OS/Z Compile
============

Requirements
------------

- gcc
- optionally fasm (for recompiling non-UEFI booting)

Compilation
-----------

If you want to recompile `loader/mbr.bin` and `loader/bootboot.bin`, you'll need [fasm](http://flatassembler.net).
Unfortunately GAS is not clever enough to mix 16, 32 and 64 bit instuctions which is necessary for BIOS booting. So
I've added those images to the source. With those pre-built binaries you should be able to compile the source using
only GNU toolchain, just don't forget to choose efi loader in Config.

To compile, simply type

```shell
$ make
```

You should see something similar to this:

```
$ make clean all testq
TOOLS
  src		cll.c
  src		mkfs.c
  src		efirom.c
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

The live disk image is generated to `bin/disk.dd`. Use the
`dd` command to write it on a USB stick or boot it with qemu,
bochs or VirtualBox.

```
$ dd if=bin/disk.dd of=/dev/sdc
$ make testq
$ make testb
$ make vdi
```
