OS/Z - an operating system for hackers
======================================

![OS/Z](http://forum.osdev.org/download/file.php?avatar=17273_1482039401.png)

[Download live image, osZ-latest-x86_64.dd](https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true)   - - -   [Documentation](https://github.com/bztsrc/osz/tree/master/docs)   - - -   [Support](http://forum.osdev.org/viewtopic.php?f=2&t=30914&p=266383)

OS/Z is a hobby OS project. As such it's primary goal is not
everyday use. Instead it demostrates different concepts
for those who like hacking with hobby OSes. It's aim is
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

 - 5 Mb free disk space
 - 32 Mb RAM
 - 800 x 600 / ARGB display
 - x86_64 processor with SSE3

Testing
-------

I always push the source to git in a state that's known to [compile](https://github.com/bztsrc/osz/tree/master/docs/compile.md) without errors by a `make clean all` command.
Although I did my best, it doesn't mean it won't crash under unforseen circumstances :-)

The [latest live dd image](https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true) should boot OS/Z in emulators and on real machines. For example type

```shell
qemu-system-x86_64 -hda bin/disk.dd
```

But for your convience I've added make commands. For example if you clone the repo and [compile](https://github.com/bztsrc/osz/blob/master/src/docs/compile.md), you can boot OS/Z right from your `bin/ESP` directory
with TianoCore EFI. For that one should type

```shell
make testefi
```

To run the resulting image in qemu with BIOS

```shell
make testq
```

To run the result with BIOS in bochs

```shell
make testb
```

To convert the raw disk image to something that VirtualBox can be fed with:

```shell
make vdi
```

License
-------

The boot loader and the [BOOTBOOT](https://github.com/bztsrc/osz/blob/master/loader) Protocol are licensed under GPLv3.
All the other parts of OS/Z (like [FS/Z](https://github.com/bztsrc/osz/blob/master/docs/fs.md) filesystem) licensed under CC-by-nc-sa-4.0.

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
