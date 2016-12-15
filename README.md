OS/Z - an operating system for hackers
======================================

Is a hobby OS project. As such it's primary goal is not
everyday use. Instead it demostrates different concepts
for those who like hacking with hobby OSes. It's aim is
to be small, and to handle enormous amounts of data in
a user friendly way. To achieve that goal, I've eliminated
as many limits as possible by design.
For example only storage capacity limits the number of inodes
on a disk. And only RAM limits the number of concurent tasks
at any given time.

Features
--------

 - GNU toolchain only (save MultiBoot loader)
 - Single disk image for booting from MBR, VBR, ROM BIOS, GRUB or UEFI.
 - ELF64 object format
 - Higher half kernel mapping, full 64 bit support
 - Microkernel architecture
 - It's filesystem can handle YotaBytes of data (unimagineable as of writing)
 - UNICODE support with UTF-8

Requirements
------------

- gcc
- qemu / bochs / VirtualBox
- optionally fasm

Compilation
-----------

To compile, simply type

```shell
$ make
```

You should see something similar to this:

```
TOOLS
  src		cll.c
  src		mkfs.c
LOADER
  src		efi-x86_64
CORE
  src		lib/core (x86_64)
  src		lib/libc.so (x86_64)
APPLICATIONS
  src		sbin/ui
  src		bin/sh
  src		sbin/syslog
  src		sbin/fs
  src		sbin/net
  src		sbin/init
IMAGES
  mkfs		initrd
  mkfs		ESP
  mkfs		usr
  mkfs		var
  mkfs		home
  mkfs		gpt disk image
```

The disk image is generated to `bin/disk.dd`. Use the `dd`
command to write it on a USB stick and you're ready to go.

Testing
-------

That disk image should boot OS/Z in emulators and on real machines
regardless to configuration.

You can boot OS/Z in a virtual machine right from your `bin` directory
with TianoCore EFI. For that, type

```shell
make testefi
```

That will start qemu using `bin/ESP` as your EFI Boot Partition. To run
the resulting image in qemu with BIOS, type

```shell
make testq
```

To run the result in bochs,

```shell
make testb
```

To convert the raw disk image to something that VirtualBox can be fed with:

```shell
make vdi
```

There's a rule called 'test' in Makefile which points to one of the above.

Non-EFI booting
---------------

If you want to recompile `loader/mbr.bin` and `loader/bootboot.bin`, you'll need [fasm](http://flatassembler.net).
Unfortunately GAS is not clever enough to mix 16, 32 and 64 bit instuctions which is necessary for BIOS booting.

The `bootboot.bin` can be booted via 
 - `mbr.bin`
 - BIOS extension ROM (tested with bochs at 0D0000h)
 - GRUB using the MultiBoot protocol (not tested).

License
-------

The boot loader and the whole BOOBOOT Protocol is licensed under GPLv3.
Other parts of OS/Z like FS/Z filesystem licensed under CC-by-nc-sa-4.0.

 Copyright 2016 [CC-by-nc-sa-4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) bztsrc@github
 
**You are free to**:

 - **Share** — copy and redistribute the material in any medium or format

 - **Adapt** — remix, transform, and build upon the material
     The licensor cannot revoke these freedoms as long as you follow
     the license terms.
 
**Under the following terms**:

 - **Attribution** — You must give appropriate credit, provide a link to
     the license, and indicate if changes were made. You may do so in
     any reasonable manner, but not in any way that suggests the
     licensor endorses you or your use.

 - **NonCommercial** — You may not use the material for commercial purposes.

 - **ShareAlike** — If you remix, transform, or build upon the material,
     you must distribute your contributions under the same license as
     the original.

Author
------

bztemail AT gmail DOT com
