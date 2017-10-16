OS/Z Loaders
============

1. *x86_64-efi* the preferred way of booting on x86_64 architecture.
    [bootboot.efi](https://github.com/bztsrc/osz/blob/master/loader/bootboot.efi?raw=true) (74k), [bootboot.rom](https://github.com/bztsrc/osz/blob/master/loader/bootboot.rom?raw=true) (74k)

2. *x86_64-bios* BIOS and Multiboot (GRUB) compatible, OBSOLETE loader.
    [boot.bin](https://github.com/bztsrc/osz/blob/master/loader/boot.bin?raw=true) (512 bytes, works as MBR and VBR too), [bootboot.bin](https://github.com/bztsrc/osz/blob/master/loader/bootboot.bin?raw=true) (8k)

3. *aarch64-rpi* ARMv8 boot loader for Raspberry Pi 3
    [kernel8.img](https://github.com/bztsrc/osz/blob/master/loader/kernel8.img?raw=true)

BOOTBOOT Protocol
=================

Source moved to it's own [repository](https://github.com/bztsrc/bootboot).
