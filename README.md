OS/Z - an operating system for hackers
======================================

[Download latest live image, osZ-x86_64-latest.dd](https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true)

OS/Z is a hobby OS project. As such it's primary goal is not
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

 - GNU toolchain without cross-compiler
 - Single disk image for booting from MBR, VBR, ROM BIOS, GRUB or UEFI.
 - ELF64 object format
 - Higher half kernel mapping, full 64 bit support
 - Microkernel architecture
 - It's filesystem can handle YotaBytes of data (unimagineable as of writing)
 - UNICODE support with UTF-8

Requirements
------------

 - 5 Mb free disk space
 - 256 Mb RAM
 - 800x600/ARGB32 display
 - x86_64 processor

Testing
-------

I always push a source to git that known to compile by a simple `make clean all`.

The [latest live dd image](https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true) should boot OS/Z in emulators and on real machines.

You can boot OS/Z in a virtual machine right from your `bin` directory
(not using bin/disk.dd) with TianoCore EFI. For that, type

```shell
make testefi
```

That will start qemu using `bin/ESP` as your EFI Boot Partition. To run
the resulting image in qemu with BIOS, type

```shell
make testq
```

To run the result with BIOS in bochs,

```shell
make testb
```

To convert the raw disk image to something that VirtualBox can be fed with:

```shell
make vdi
```

Non-EFI booting
---------------

If you want to recompile `loader/mbr.bin` and `loader/bootboot.bin`, you'll need [fasm](http://flatassembler.net).
Unfortunately GAS is not clever enough to mix 16, 32 and 64 bit instuctions which is necessary for BIOS booting. So
I've added the BIOS images to the source, and you should be able to compile efi loader with GNU toolchain only.

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
