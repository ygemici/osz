OS/Z Loaders
============

1. *x86_64-efi* the preferred way of booting on x86_64 architecture.
    [bootboot.efi](https://github.com/bztsrc/bootboot/blob/master/bootboot.efi?raw=true) (74k), [bootboot.rom](https://github.com/bztsrc/bootboot/blob/master/bootboot.rom?raw=true) (74k)

2. *x86_64-bios* BIOS and Multiboot (GRUB) compatible, OBSOLETE loader.
    [boot.bin](https://github.com/bztsrc/bootboot/blob/master/boot.bin?raw=true) (512 bytes, works as MBR and VBR too), [bootboot.bin](https://github.com/bztsrc/bootboot/blob/master/bootboot.bin?raw=true) (8k)

3. *aarch64-rpi* ARMv8 boot loader for Raspberry Pi 3
    [bootboot.img](https://github.com/bztsrc/bootboot/blob/master/bootboot.img?raw=true) (24k)

BOOTBOOT Protocol
=================

Source and documentation moved to it's own [repository](https://github.com/bztsrc/bootboot).
