OS/Z - an operating system for hackers
======================================

<img align="left" style="margin-right:10px;" alt="OS/Z" src="https://github.com/bztsrc/osz/raw/master/logo.png">
<a href="https://github.com/bztsrc/osz/blob/master/bin/">Download live images</a>,  <small>(~3 Mbyte)</small><br>
<a href="https://github.com/bztsrc/osz/tree/master/docs/README.md">Documentation</a><br>
<a href="https://github.com/bztsrc/osz/issues">Support</a><br><br>

OS/Z is a POSIXish hobby OS project. As such it demostrates [different concepts to POSIX](https://github.com/bztsrc/osz/tree/master/docs/posix.md)
for those who like hacking with OSes, like the good fellows on [OSDEV](http://forum.osdev.org/). It's aim is
to be small, elegant, [portable](https://github.com/bztsrc/osz/tree/master/docs/porting.md) and to be able to handle enormous amounts of data in
a user friendly way. To achieve that goal, I've eliminated as many limits as possible by design.
For example only storage capacity limits the number of inodes on a disk. And only amount of RAM limits the number of
concurent tasks at any given time. If I couldn't eliminate a hard limit, I've
created a [boot option](https://github.com/bztsrc/osz/tree/master/docs/bootopts.md) for it so that you can tweek it without
recompilation. This makes OS/Z a very scalable system.

Features
--------

 - [GNU toolchain](https://github.com/bztsrc/osz/tree/master/docs/compile.md)
 - Microkernel architecture with an effective [messaging system](https://github.com/bztsrc/osz/tree/master/docs/messages.md)
 - Single disk image for [booting](https://github.com/bztsrc/osz/tree/master/docs/boot.md) from BIOS or from UEFI or on Raspberry Pi 3.
 - [Higher half kernel](https://github.com/bztsrc/osz/tree/master/docs/memory.md) mapping, full 64 bit support
 - It's [filesystem](https://github.com/bztsrc/osz/tree/master/docs/fs.md) can handle YottaBytes of data (unimagineable as of writing)
 - ELF64 object format support
 - UNICODE support with UTF-8

Hardware Requirements
---------------------

 - 10 Mb free disk space
 - 32 Mb RAM
 - 800 x 600 / ARGB display
 - IBM PC with x86_64 processor  - or -
 - Raspberry Pi 3 with AArch64 processor
 - [Supported devices](https://github.com/bztsrc/osz/tree/master/docs/drivers.md)

Testing
-------

The [latest live dd image](https://github.com/bztsrc/osz/blob/master/bin/osZ-latest-x86_64-ibmpc.dd?raw=true) should boot OS/Z in emulators and on real machines. For example type

```shell
qemu-system-x86_64 -hda bin/osZ-latest-x86_64-ibmpc.dd
```
For more options, see [Testing How To](https://github.com/bztsrc/osz/tree/master/docs/howto1-testing.md). I usually test the image
with [qemu](http://www.qemu.org/), [bochs](http://bochs.sourceforge.net/) and [virtualbox](https://www.virtualbox.org/).

License
-------

The boot loader, the [BOOTBOOT](https://github.com/bztsrc/osz/blob/master/loader) Protocol and the
[on disk format of FS/Z](https://github.com/bztsrc/osz/blob/master/etc/include/fsZ.h) are Public Domain.
I hereby grant you the right to use them in your own project without any resctictions, although I would appreciate attribution.
All the other parts of OS/Z (including my [FS/Z](https://github.com/bztsrc/osz/blob/master/docs/fs.md) implementation) licensed under CC-by-nc-sa-4.0:

 Copyright 2017 bzt (bztsrc@github) [CC-by-nc-sa-4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/)
 
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

Authors
-------

efirom: Michael Brown

zlib: Mark Adler

qsort: Copyright The Regents of the University of California

BOOTBOOT, FS/Z, OS/Z, bztalloc: bzt

