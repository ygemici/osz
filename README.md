OS/Z - an operating system for hackers
======================================

<img align="left" style="margin-right:10px;" alt="OS/Z" src="https://github.com/bztsrc/osz/raw/master/logo.png">
<a href="https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true">Download live image</a>, <i>osZ-latest-x86_64.dd</i> <small>(3 Mbyte)</small><br>
<a href="https://github.com/bztsrc/osz/tree/master/docs/README.md">Documentation</a><br>
<a href="http://forum.osdev.org/viewtopic.php?f=2&t=30914&p=266383">Support</a><br><br>

OS/Z is a hobby OS project. As such it's primary goal is not
everyday use. Instead it demostrates different concepts
for those who like hacking with hobby OSes, like the good fellows on [OSDEV](http://forum.osdev.org/). It's aim is
to be small, and to handle enormous amounts of data in
a user friendly way. To achieve that goal, I've eliminated
as many limits as possible by design.
For example only storage capacity limits the number of inodes
on a disk. And only RAM limits the number of concurent tasks
at any given time. If I couldn't eliminate a hard limit, I've
created a [boot option](https://github.com/bztsrc/osz/tree/master/docs/bootopts.md) for it so that you
can tweek without recompilation. This makes OS/Z a very scalable system.

Features
--------

 - [GNU toolchain](https://github.com/bztsrc/osz/tree/master/docs/compile.md) without cross-compiler
 - Microkernel architecture with an effective [messaging system](https://github.com/bztsrc/osz/tree/master/docs/messages.md)
 - Single disk image for [booting](https://github.com/bztsrc/osz/tree/master/docs/boot.md) from MBR, VBR, ROM BIOS, GRUB or UEFI.
 - [Higher half kernel](https://github.com/bztsrc/osz/tree/master/docs/memory.md) mapping, full 64 bit support
 - It's [filesystem](https://github.com/bztsrc/osz/tree/master/docs/fs.md) can handle YotaBytes of data (unimagineable as of writing)
 - ELF64 object format support
 - UNICODE support with UTF-8

Hardware Requirements
---------------------

 - 3 Mb free disk space
 - 32 Mb RAM
 - 800 x 600 / ARGB display
 - x86_64 processor with SSSE3

Testing
-------

The [latest live dd image](https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true) should boot OS/Z in emulators and on real machines. For example type

```shell
qemu-system-x86_64 -hda bin/disk.dd
```
For more options, see [Testing How To](https://github.com/bztsrc/osz/tree/master/docs/howto1-testing.md). I usually test the image
with [qemu](http://www.qemu.org/), [bochs](http://bochs.sourceforge.net/) and [virtualbox](https://www.virtualbox.org/).

License
-------

The boot loader and the [BOOTBOOT](https://github.com/bztsrc/osz/blob/master/loader) Protocol are Public Domain.
All the other parts of OS/Z (like [FS/Z](https://github.com/bztsrc/osz/blob/master/docs/fs.md) filesystem) licensed under CC-by-nc-sa-4.0:

 Copyright 2017 [CC-by-nc-sa-4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) bztsrc@github
 
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

MurmurHash: Austin Appleby

BOOTBOOT, OS/Z: bzt

