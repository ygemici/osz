BOOTBOOT Reference Implementations
==================================

1. *x86_64-efi* the preferred way of booting on x86_64 architecture.
    Standard GNU toolchain and a few files from gnuefi (included).
    [bootboot.efi](https://github.com/bztsrc/osz/blob/master/loader/bootboot.efi?raw=true) (74k), [bootboot.rom](https://github.com/bztsrc/osz/blob/master/loader/bootboot.rom?raw=true) (74k)

2. *x86_64-bios* BIOS and MultiBoot (GRUB) compatible, OBSOLETE loader.
    If you want to recompile this, you'll need fasm (not included).
    [boot.bin](https://github.com/bztsrc/osz/blob/master/loader/boot.bin?raw=true) (512 bytes, works as MBR and VBR too), [bootboot.bin](https://github.com/bztsrc/osz/blob/master/loader/bootboot.bin?raw=true) (8k)

3. *aarch64-rpi* ARMv8 boot loader for Raspberry Pi 3 (work in progress)
    [kernel8.img](https://github.com/bztsrc/osz/blob/master/loader/kernel8.img?raw=true)

Please note that the reference implementations do not support
the full protocol at level 2, they only handle static mappings
which makes them level 1 loaders. I provide pre-compiled images ready for use.

BOOTBOOT Protocol
=================

Rationale
---------

The protocol describes how to boot an ELF64 or PE32+ executable inside an
initial ram disk image on a GPT disk or from ROM into 64 bit mode, without
using any configuration or even knowing the file system of initrd.

On BIOS based systems, the same image can be loaded via MultiBoot,
chainload from MBR or VBR (GPT hybrid booting) or run as a BIOS Expansion ROM
(so not only the ramdisk can be in ROM, but the loader as well).

On EFI machines, the PCI Option ROM is created from a standard EFI
OS loader application.

On [Raspberry Pi 3](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bootmodes/sdcard.md) board the kernel8.img
is loaded from the boot partition on SD card by start.elf.

The difference to other booting protocols is flexibility and portability;
only clean 64 bit support; and that BOOTBOOT expects the kernel to fit inside the
initial ramdisk. This is ideal for hobby OSes and microkernels. The advantage it gaves is that your kernel
can be splitted up into several files and yet they will be loaded together
as if it were a monolitic kernel. And you can use your own file system for the initrd.

License
-------

The protocol as well as the reference implementations are Public Domain.
You can do anything you want with them. You have no legal obligation to do anything else, although I appreciate attribution.

Glossary
--------

* _boot partition_: the first bootable partition of the first bootable disk,
  a rather small one. Most likely an EFI System Partition with FAT, but can be
  any other partition as well if the partition is bootable (bit 2 set in flags).

* _environment file_: a maximum one page frame long, utf-8 [file](https://github.com/bztsrc/osz/blob/master/loader/README.md#environment-file) on boot partition
  at BOOTBOOT\CONFIG or (when your initrd is the entire partition) /sys/config. It
  has "key=value" pairs (separated by newlines). The protocol
  only specifies two of the keys: "screen" for screen size,
  and "kernel" for the name of the executable inside the initrd.

* _initrd_: initial [ramdisk image](https://github.com/bztsrc/osz/blob/master/loader/README.md#installation)
  (probably in ROM or on boot partition at BOOTBOOT\INITRD or it can occupy the whole partition or loaded
  over network). It's format and whereabouts are not specified (the good part :-) ) and can be optionally gzip compressed.

* _loader_: a native executable on the boot partition or in ROM. For multi-bootable disks
  more loader implementations can co-exists.

* _file system driver_: a separated function that parses initrd for the kernel file.
  Without one the first executable found will be loaded.

* _kernel file_: an ELF64 / PE32+ [executable inside initrd](https://github.com/bztsrc/osz/blob/master/loader/README.md#example-kernel),
  optionally with the following symbols: `fb`, `environment`, `bootboot` (see linker script).

* _BOOTBOOT structure_: an informational structure defined in [bootboot.h](https://github.com/bztsrc/osz/blob/master/loader/bootboot.h).


Boot process
------------

1. the firmware locates the loader, loads it and passes control to it. (On BIOS it's done through boot.bin)
2. the loader initializes hardware (64 bit mode, screen resolution, memory map etc.)
3. then loads environment file and initrd file (probably from the boot partition or from ROM).
4. iterates on file system drivers, and loads kernel file from initrd.
5. if file system is not recognized, scans for the first executable in the initrd.
6. parses executable header and symbols to get link addresses (only level 2 compatible loaders).
7. maps linear framebuffer, [environment](https://github.com/bztsrc/osz/blob/master/etc/sys/config) and [bootboot structure](https://github.com/bztsrc/osz/blob/master/loader/bootboot.h) accordingly.
8. sets up stack, registers and jumps to kernel [entry point](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/start.S). See example kernel below.

Machine state
-------------

When the kernel gains control, the memory mapping looks like this (unless symbol table is provided.
A fully compatible level 2 loader should map these where the symbols in the kernel tell to):

```
   -64M         framebuffer           (0xFFFFFFFFFC000000)
    -2M         bootboot structure    (0xFFFFFFFFFFE00000)
    -2M+1page   environment           (0xFFFFFFFFFFE01000)
    -2M+2page.. v kernel text segment (0xFFFFFFFFFFE02000)
     ..0        ^ kernel stack
    0-16G       RAM identity mapped
```

Interrups are turned off, GDT unspecified, but valid and code segment running on ring 0 (supervisor mode).
The stack is at the top of the memory, starting at zero and growing downwards. The first 16G of RAM is identity mapped.

You can locate your initrd in memory using the [bootboot structure](https://github.com/bztsrc/osz/blob/master/loader/bootboot.h)'s initrd_ptr and initrd_size members.
The boot time and a platform independent memory map is also provided in bootboot structure.

The screen is properly set up with a 32 bit (x8r8g8b8) linear framebuffer, mapped at the defined location (the physical address of the framebuffer can be found in the [bootboot structure](https://github.com/bztsrc/osz/blob/master/loader/bootboot.h)'s fb_ptr field).

Environment file
----------------

[Environment](https://github.com/bztsrc/osz/blob/master/etc/sys/config) is passed to your kernel as newline separated, zero terminated UTF-8 string with "key=value" pairs.

```
// BOOTBOOT Options

// --- Loader specific ---
screen=800x600
kernel=sys/core

// --- Kernel specific, you're choosing ---
anythingyouwant=somevalue
otherstuff=enabled

```

That cannot be larger than a page size (4096 bytes). Temporary variables will be appended at the end (from
UEFI command line). C style single line and multi line comments can be used. BOOTBOOT protocol only uses `screen` and
`kernel` keys, all the others and their values are up to your kernel (or drivers) to parse. Be creative :-)

To modify the environment, one will need to insert the disk into another machine (or boot a simple OS like DOS) and edit
BOOTBOOT\CONFIG on the boot partition. With UEFI, you can use the `edit` command provided by the EFI Shell or append
"key=value" pairs on the command line (value specified on command line takes precedence over the one in the file).

File system drivers
-------------------

For boot partition, BIOS/Multiboot and RPi3 versions expect *defragmented* FAT12, FAT16 or FAT32 file systems if the
initrd is a file and does not occupy the whole boot partition. EFI version relies on any file system that's supported by
EFI Simple File System Protocol.

Luckily the EFI loader can load files from the boot partition with the
help of the firmware. BIOS/MultiBoot and RPi3 implementations are not so lucky,
therefore they need an extra function to do that. That function however
is out of the scope of this specification: the BOOTBOOT Protocol only
states that a compatible loader must be able to load initrd and the environment file,
but does not describe how or from where. They can be loaded from nvram, ROM or over
network for example, it does not matter.

On the other hand BOOTBOOT does specify one API function to locate a file (the kernel)
inside the initrd image, but the ABI is also implementation (and architecture) specific.
This function receives a pointer to initrd in memory as well as the kernel's filename, and
returns a pointer to the first byte of the kernel and it's size. On error (if file system is
not recognized or the kernel file is not found) returns {NULL,0}. Plain simple.

```c
typedef struct {
    uint8_t *ptr;
    uint64_t size;
} file_t;

file_t myfs_initrd(uint8_t *initrd, char *filename);
```

The protocol expects that a BOOTBOOT compliant loader iterates on a list of drivers until one
returns a valid result. If all file system drivers returned {NULL,0}, the loader will brute-force
scan for the first ELF64 / PE32+ image in the initrd. This feature is quite comfortable when you
want to use your own file system but you don't have written an fs driver for it yet. You just copy
your initrd on the boot partition, and you're ready to rock and roll!

The BOOTBOOT Protocol expects the file system drivers ([here](https://github.com/bztsrc/osz/blob/master/loader/x86_64-efi/fs.h),
[here](https://github.com/bztsrc/osz/blob/master/loader/x86_64-bios/fs.inc) and [here](https://github.com/bztsrc/osz/blob/master/loader/aarch64-rpi/fs.h))
to be separated from the rest of the loader's source. This is so because it was designed to help the needs of hobby
OS developers, specially for those who want to write their own file systems.

The reference implementations support [cpio](https://en.wikipedia.org/wiki/Cpio) (all hpodc, newc and crc variants),
[ustar](https://en.wikipedia.org/wiki/Tar_(computing)), osdev.org's SFS and OS/Z's native [FS/Z](https://github.com/bztsrc/osz/blob/master/docs/fs.md).
Gzip compressed initrds also supported to save disk space and fasten up load time.

Example kernel
--------------

A minimal kernel that demostrates how to access BOOTBOOT environment:

```c
#include <bootboot.h>

/* imported virtual addresses, see linker script below */
extern BOOTBOOT bootboot;           // see bootboot.h
extern unsigned char *environment;  // configuration, key=value pairs
extern uint8_t fb;                  // linear framebuffer mapped

void _start()
{
    int x, y, s=bootboot.fb_scanline, w=bootboot.fb_width, h=bootboot.fb_height;

    // cross-hair to see screen dimension detected correctly
    for(y=0;y<h;y++) { *((uint32_t*)(&fb + s*y + (w*2)))=0x00FFFFFF; }
    for(x=0;x<w;x++) { *((uint32_t*)(&fb + s*(h/2)+x*4))=0x00FFFFFF; }

    // red, green, blue boxes
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+10) + (x+10)*4))=0x00FF0000; } }
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+10) + (x+40)*4))=0x0000FF00; } }
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+10) + (x+70)*4))=0x000000FF; } }

    // stop for now
    __asm__ __volatile__ ( "cli;hlt" );
}

```

Compile and link it with:

```shell
gcc -Wall -fpic -ffreestanding -fno-stack-protector -m64 -mno-red-zone -c kernel.c -o kernel.o
ld -nostdlib --nmagic -T link.ld -o mykernel
strip -s -K fb -K bootboot -K environment mykernel
```

A minimal linker script would be:

```c
FBUF_ADDRESS = 0xfffffffffc000000;
CORE_ADDRESS = 0xffffffffffe00000;

ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
SECTIONS
{
    /* bootboot.fb_ptr holds the physical address, which is mapped here */
    . = FBUF_ADDRESS;
        fb = .;
    /* the bootboot structure and your envirenment file mapped here */
    . = CORE_ADDRESS;
        bootboot = .;
        . += 4096;
        environment = .;
        . += 4096;
    /* rest of your code, from the C source above */
    .text : AT(ADDR(.text) - CORE_ADDRESS)
    {
        _code = .;
        *(.text)
        *(.rodata)
        . = ALIGN(4096);
        _data = .;
        *(.data)
        . = ALIGN(4096);
        __bss_start = .;
    }
   _end = .;

   /DISCARD/ :
   {
        *(.comment)
        *(.gnu*)
        *(.note*)
        *(.eh_frame*)
   }
}
```
See [src/core/(platform)/supervisor.ld](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/supervisor.ld) for a more complex, real OS example.

Installation
------------

1. make an initrd with your kernel in it. Example:

```shell
mkdir -r tmp/sys
cp mykernel tmp/sys/core
# copy more files to tmp/ directory
cd tmp
# create your file system image or an archive. For example use one of these:
find . | cpio -H newc -o | gzip > ../INITRD
find . | cpio -H crc -o | gzip > ../INITRD
find . | cpio -H hpodc -o | gzip > ../INITRD
tar -czf ../INITRD *
```

2. Create FS0:\BOOTBOOT directory on the boot partition, and copy the image you've created
        into it. If you want, create a text file named [CONFIG](https://github.com/bztsrc/osz/blob/master/etc/sys/config)
        there too, and put your [environment variables](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md) there.
        If you use a different name than "sys/core" for your kernel, specify "kernel=" in it.

Alternatively you can copy an uncompressed INITRD into the whole partition using your fs only, leaving FAT file system entirely out.
You can also create an Option ROM out of INITRD (on BIOS there's not much space ~64-96k, but on EFI it can be 16M).

3. copy the BOOTBOOT loader on the boot partition:

3.1. *EFI disk*: copy __bootboot.efi__ to **_FS0:\EFI\BOOT\BOOTX64.EFI_**.

3.2. *EFI ROM*: use __bootboot.rom__ which is a standard **_PCI Option ROM image_**.

3.3. *BIOS disk*: copy __bootboot.bin__ to **_FS0:\BOOTBOOT\LOADER_**. You can place it inside your INITRD partition
        or outside of partition area as well (with `dd conv=notrunc oseek=x`). Finally install __boot.bin__ in the
        master boot record (or in volume boot record if you have a boot manager), saving bootboot.bin's first sector's
        LBA number in a dword at 0x1B0. The [mkboot](https://github.com/bztsrc/osz/blob/master/loader/x86_64-bios/mkboot.c)
        utility will do that for you.

3.4. *BIOS ROM*: install __bootboot.bin__ in a **_BIOS Expansion ROM_**.

3.5. *Raspberry Pi 3*: copy __kernel8.img__ on the boot partition, and add `initramfs BOOTBOOT\INITRD 0x80000` to [config.txt](http://elinux.org/RPi_config.txt#Boot).

Troubleshooting
---------------

```
BOOTBOOT-PANIC: LBA support not found
```

Really old hardware. Your BIOS does not support LBA. This message is generated by 1st stage loader (boot.bin).

```
BOOTBOOT-PANIC: FS0:\BOOTBOOT\LOADER not found
```

The loader (bootboot.bin) is not on the disk or it's starting LBA address is not recorded in the boot sector at dword [0x1B0]
(see [mkboot.c](https://github.com/bztsrc/osz/blob/master/loader/x86_64-bios/mkboot.c)). As the boot sector supports RAID mirror,
it will try to load the loader from other drives as well. This message is generated by 1st stage loader (boot.bin).

```
BOOTBOOT-PANIC: Hardware not supported
```

Really old hardware. On x86_64, your CPU is older than family 6.0 or PAE, MSR, LME features not supported.

```
BOOTBOOT-PANIC: No boot partition
```

Either the disk does not have a GPT, or there's no EFI System Partition nor any other bootable
partition on it. Or the FAT file system is found but inconsistent, or doesn't have a BOOTBOOT directory.

```
BOOTBOOT-PANIC: FS0:\BOOTBOOT\INITRD not found
```

The loader could not find the initial ramdisk image on the boot partition. This message will be shown
even if you specify an alternative initrd file on EFI command line.

```
BOOTBOOT-PANIC: Kernel not found in initrd
```

Kernel is not included in the initrd, or initrd's fileformat is not recognized by any of the file system
drivers and scanning haven't found a valid executable header in it.

```
BOOTBOOT-PANIC: Kernel is not a valid executable
```

The file that was specified as kernel could be loaded by fs drivers, but it's not an ELF, it's class is not
CLASS64, endianness does not mach architecture, or does not have any program header with a loadable segment
in the negative p_vaddr range (see linker script); or it's not a 64 bit PE32+ executable for the architecture.
The same error is shown if the address of fb, bootboot and environment symbols are not in the range -4G..0.

```
BOOTBOOT-PANIC: GOP failed, no framebuffer
BOOTBOOT-PANIC: VESA VBE error, no framebuffer
BOOTBOOT-PANIC: VideoCore error, no framebuffer
```

The first part of the message varies on different platforms. It means that the loader was unable to set up linear
framebuffer with packed 32 bit pixels in the requested resolution. Possible solution is to modify screen to
`screen=800x600` in environment, which is the minimal resolution supported.

That's all, hope it will be useful!
