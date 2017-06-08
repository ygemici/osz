OS/Z Compilation
================

Requirements
------------

- GNU toolchain (make, gcc, gas, ld, objcopy, strip)
- bash (shell scripts are used to generate different files during compilation)
- optionally fasm (for recompiling non-UEFI booting)

Compilation
-----------

The make system is created in a way that you can say `make all` in almost every directory. If you do that in the top level
directory, it will compile everything and will also build disk image for you, but surpressing messages. In subdirectories make
will always display the full command executed during compilation.

For example if you enter `src/fs` and issue `make all` there, then only the filesystem subsystem will be recompiled, and
you'll be able to see the exact commands used.

So to compile all the things, simply type

```shell
$ make all
```

You should see something similar to this:

```
$ make all
TOOLS
  src		mkfs.c
  src		elftool.c
CORE
  gen		x86_64/isrs.S (PIC, numirq 32, idtsz 4096)
  lnk		lib/sys/core (x86_64)
  lnk		lib/libc.so (x86_64)
BASE
  src		sbin/ui
  src		bin/sh
  src		bin/test
  src		sbin/sys
  src		sbin/syslog
  src		sbin/sound
  src		sbin/fs
  src		sbin/net
  src		sbin/init
DRIVERS
  src		fs/gpt.so
  src		fs/vfat.so
  src		fs/fsz.so
  src		mmedia/sblive.so
  src		network/ne2k.so
  src		input/ps2.so
  src		display/fb.so
  src		lib/sys/drv
USERSPACE
IMAGES
  mkfs		initrd
  mkfs		boot (ESP)
  mkfs		usr
  mkfs		var
  mkfs		home
  mkfs		bin/disk.dd
```

Non-EFI loader
--------------

If you want to recompile `loader/mbr.bin` and `loader/bootboot.bin`, you'll need [fasm](http://flatassembler.net).
Unfortunately GAS is not good at mixing 16, 32 and 64 bit instuctions, which is necessary for BIOS booting. So
I've added those images to the source. With those pre-built binaries you should be able to compile the source using
GNU toolchain only.

See what you've done!
---------------------

The live disk image is generated to [bin/disk.dd](https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true). Use the
`dd` command to write it on a USB stick or boot with qemu, bochs or VirtualBox.

```
$ dd if=bin/disk.dd of=/dev/sdc
$ make testq
$ make testb
$ make testv
```

There's a detailed [tutorial on testing](https://github.com/bztsrc/osz/blob/master/docs/howto1-testing.md).
