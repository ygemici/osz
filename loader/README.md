BOOTBOOT Reference Implementations
==================================

1. *efi-x86_64* the preferred way of booting on x86_64 architecture.
    Standard GNU toolchain and a few files from gnuefi (included).

2. *mb-x86_64* BIOS and MultiBoot (GRUB) compatible, OBSOLETE loader.
    If you want to recompile this, you'll need fasm (not included).

3. *rpi-AArch64* ARM boot loader for Raspberry Pi 3 (only planned as of now)

Please note that the reference implementations do not support
the full protocol at level 2, they only handle static mappings
which makes them level 1 loaders.

BOOTBOOT Protocol
=================

Rationale
---------

The protocol describes how to boot an ELF64 executable inside an
initial ram disk image on a GPT disk into 64 bit mode from ROM, without
using any configuration or even knowing the filesystem in question.

On BIOS based systems, the same image can be loaded via MultiBoot,
chainload from MBR or run as a BIOS Expansion ROM (GPT hybrid booting).

On EFI machines, the PCI Option ROM is created from a standard EFI
OS loader application.

The difference to other booting protocols is flexibility and that
BOOTBOOT expects the kernel to fit inside the ramdisk. This is ideal for
hobby OSes and microkernels. The advantage it gaves is that your kernel
can be splitted into several files and yet they will be loaded together
as if it was a monolitic kernel, and you can use your own filesystem for that.

License
-------

The protocol as well as the reference implementations are Public Domain.
You can do anything you want with them. You have no legal obligation to do anything else, although I appreciate attribution.

Glossary
--------

* _boot partition_: the first bootable partition of the first disk,
  a rather small one. Most likely an EFI System Partition with FAT32,
  but can be any other FS as well if the partition is bootable (bit 2 set in flags).

* _environment file_: a one page long utf-8 file on boot partition
  with "key=value" pairs (separated by newlines). The protocol only
  specifies three of the keys: "width" and "height" for screen size,
  and "kernel" for the name of the ELF executable inside the initrd.

* _initrd file_: initial ramdisk image on boot partition. It's
  filesystem should have a filesystem driver (see below).

* _loader_: a native executable on the boot partition. For multi-
  bootable disks more loader can co-exists.

* _filesystem driver_: a plug-in that parses initrd for the kernel

* _kernel file_: an ELF executable inside initrd, optionally with
  the following symbols: fb, environment, bootboot (see linker script).

* _BOOTBOOT structure_: an informational structure defined in `bootboot.h`.


Boot process
------------

1. the firmware locates the loader, loads it and passes control to it.
2. the loader initializes hardware (long mode, screen resolution, memory map etc.)
3. then loads initrd file and environment file from the boot partition.
4. iterates on filesystem drivers, and loads kernel from initrd.
5. if filesystem is not recognized, scans for the first ELF executable in initrd.
6. parses ELF program header and symbols to get link addresses.
7. maps framebuffer, [environment](https://github.com/bztsrc/osz/blob/master/etc/CONFIG) and [bootboot structure](https://github.com/bztsrc/osz/blob/master/loader/bootboot.h) accordingly.
8. sets up stack, registers and jumps to [ELF's entry point](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/start.S).

Machine state
-------------

When the kernel gains control, the memory mapping looks like (unless
symbol table is provided. A fully compatible loader should map these
where the ELF kernel wants them):

```
    -512M framebuffer                   (0xFFFFFFFFE0000000)
    -2M core        bootboot structure  (0xFFFFFFFFFFE00000)
        -2M+1page   environment         (0xFFFFFFFFFFE01000)
        -2M+2page.. kernel text segment (0xFFFFFFFFFFE02000)
         ..0        kernel stack
    0-16G RAM identity mapped
```

And the registers:

```
    rax     magic 'BOOTBOOT' (0x544f4f42544f4f42)
    rbx     virtual address of bootboot structure
    rcx     virtual address of environment variables
    rdx     virtual address of linear framebuffer
```

Interrups are turned off, GDT unspecified, but valid and code segment
running on ring 0. The stack is at the top of the memory, starting at
zero and growing downwards.

You can locate the initrd in memory using the [bootboot structure](https://github.com/bztsrc/osz/blob/master/loader/bootboot.h)'s initrd_ptr and initrd_size members.

The boot time and a platform independent memory map is also provided.

[Environment](https://github.com/bztsrc/osz/blob/master/etc/CONFIG) is passed to your kernel as newline separated "key=value" pairs.

The screen is properly set up with a 32 bit (x8r8g8b8) linear framebuffer.

Filesystem drivers
------------------

Luckily the EFI loader can load files from the boot partition with the
help of the firmware. MultiBoot implementation is not so lucky,
therefore it needs an extra function to do that. That function however
is out of the scope of this specification: the BOOTBOOT Protocol only
states that a compatible loader must be able to load initrd and environment
from the boot partition, but does not describe how.

On the other hand it does specify one API function to locate the kernel
inside initrd, but the ABI is also implementation specific. This function
receives a pointer to initrd in memory, and returns a pointer to the
first byte of the kernel. On error (if filesystem is not recognized or
the kernel is not found) returns NULL. Plain simple.

```c
uint8_t *myfs_initrd(uint8_t *initrd, char *filename);
```

The protocol expects that a compliant loader iterates on a list of
drivers until one returns a non-NULL.

If all filesystem drivers failed, BOOTBOOT compliant loader will brute-force
scan for the first ELF image in the first initrd image on the first partition,
which is quite comfortable specially when you boot it from ROM. You just copy
your initrd on the EFI System Partition, and ready to rock and roll!

The BOOTBOOT Protocol expects the [filesystem drivers](https://github.com/bztsrc/osz/blob/master/loader/efi-x86_64/fs.h) to be separated from
the rest of the loader in source. This is so because it was designed to
help the needs of hobby OS developers, specially those who want to write
their own filesystem.

For boot partition, Multiboot version uses magic bytes to locate initrd, so it's 100% filesystem independent.
EFI version relies on any filesystem that's supported by EFI Simple FileSystem Protocol. As for initrd, 
the reference implementations support cpio, tar and [FS/Z](https://github.com/bztsrc/osz/blob/master/docs/fs.md).
Gzip compressed initrds are only handled by the EFI version, sopport in the BIOS version coming soon.

Example kernel
--------------

A minimal kernel that demostrates how to access BOOTBOOT environment:

```c
#include <bootboot.h>

/* imported virtual addresses
extern BOOTBOOT bootboot;
extern unsigned char environment[4096];
extern uint8_t fb;
*/

void _start()
{
    int x, y, s=bootboot.fb_scanline, w=bootboot.fb_width, h=bootboot.fb_height;

    // cross-hair
    for(y=0;y<h;y++) { *((uint32_t*)(&fb + s*y + (w*2)))=0x00FFFFFF; }
    for(x=0;x<w;x++) { *((uint32_t*)(&fb + s*(h/2)+x*4))=0x00FFFFFF; }

    // red, green, blue boxes
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+10) + (x+10)*4))=0x00FF0000; } }
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+10) + (x+40)*4))=0x0000FF00; } }
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+10) + (x+70)*4))=0x000000FF; } }

    __asm__ __volatile__ ( "mov %0, %%rsi;cli;hlt" : : "m"(bootboot) : "memory" );
}

```

Compile and link it with:

```shell
gcc -Wall -fpic -ffreestanding -m64 -mno-red-zone -c kernel.c -o kernel.o
gcc -nostdlib -nodefaultlibs -nostartfiles -Xlinker --nmagic -T link.ld -o kernel
strip -s -K fb -K bootboot -K environment kernel
```

A minimal linker script would be:

```c
FBUF_ADDRESS = 0xffffffffe0000000;
CORE_ADDRESS = 0xffffffffffe00000;
ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
SECTIONS
{
    . = FBUF_ADDRESS;
        fb = .;
    . = CORE_ADDRESS;
        bootboot = .;
        . += 4096;
        environment = .;
        . += 4096;
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
See [src/core/(platform)/supervisor.ld](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/supervisor.ld) for a more complex, real example.

Installation
------------

1. make an initrd with your kernel in it. Example:

```shell
mkdir -r tmp/lib/sys
cp kernel tmp/lib/sys/core
# copy more files to tmp/ directory
cd tmp
find . | cpio -H hpodc -o | gzip > ../INITRD
```

Or if you prefer ustar format, the last line could be:

```
tar -czf ../INITRD *
```

You can compress it with gzip or "z" flag, but note that currently only EFI version supports decompression.

2. Create FS0:\BOOTBOOT directory, and copy the archive you've created
            into it. If you want, create a text file named [CONFIG](https://github.com/bztsrc/osz/blob/master/etc/CONFIG)
            there too, and put your [environment variables](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md) there. Start it
            with a comment "// BOOTBOOT" and fill up with newlines so that
            the file became exactly 4096 bytes (1 page) long. Temporary variables will be copied there.

```
// BOOTBOOT Options
// this file has to be 4096 bytes long.

// --- Loader specific ---
width=800
height=600
kernel=lib/sys/core

// --- Kernel specific, you're choosing ---
anythingyouwant=somevalue
otherstuff=enabled

```

3. copy the BOOTBOOT loader on the boot partition:

3.1. *EFI disk*: copy __bootboot.efi__ to **_FS0:\EFI\BOOT\BOOTX64.EFI_**. Done!

3.2. *EFI ROM*: use __bootboot.rom__ which is a standard **_PCI Option ROM image_**.

3.3. *BIOS disk*: copy __bootboot.bin__ to **_FS0:\BOOTBOOT\LOADER_**. Install
          __mbr.bin__ in the master boot record, saving bootboot.bin's first
          logical sector's number in a dword at 0x1B0.

3.4. *BIOS ROM*: install __bootboot.bin__ in a **_BIOS Expansion ROM_**.

Troubleshooting
---------------

```
BOOTBOOT-PANIC: LBA support not found
```

Really old hardware. This message is generated by 1st stage loader.

```
BOOTBOOT-PANIC: FS0:\BOOTBOOT\LOADER not found
```

The 2nd stage loader is not on the boot partition or it's starting LBA
address is not recorded in (P)MBR at dword [0x1B0]. This message is
generated by 1st stage loader.


```
BOOTBOOT-PANIC: GUID Partition Table not found or corrupt
```

Either the disk does not have a GPT, or there's no EFI System Partition
nor any other bootable partition on it. If that's the case, BOOTBOOT
Protocol compatible loaders should try to use the first partition if any.

```
BOOTBOOT-PANIC: FS0:\BOOTBOOT\INITRD not found
```

The loader could not find the initial ramdisk image on the boot partition.

```
BOOTBOOT-PANIC: Kernel not found in initrd
```

Kernel is not included in the initrd, or initrd's fileformat is not
recognized by any of the filesystem drivers.

```
BOOTBOOT-PANIC: Kernel is not an executable ELF64 for x86_64
```

The file that was specified as kernel is not an ELF, it's class is not
CLASS64, not little endian, or does not have program header with a loadable
segment in the negative p_vaddr range (see linker script).

That's all, hope it will be useful!
