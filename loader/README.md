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

The protocol describes how to boot an ELF64 or PE32+ executable inside an
initial ram disk image on a GPT disk or from ROM into 64 bit mode, without
using any configuration or even knowing the filesystem in question.

On BIOS based systems, the same image can be loaded via MultiBoot,
chainload from MBR (GPT hybrid booting) or run as a BIOS Expansion ROM
(so not only the ramdisk can be in ROM, but the loader as well).

On EFI machines, the PCI Option ROM is created from a standard EFI
OS loader application.

The difference to other booting protocols is flexibility and that
BOOTBOOT expects the kernel to fit inside the ramdisk. This is ideal for
hobby OSes and microkernels. The advantage it gaves is that your kernel
can be splitted into several files and yet they will be loaded together
as if it was a monolitic kernel, and you can use your own filesystem for that.

Also note that the initrd can be in another Option ROM, which is ideal
for diskless, embedded systems.

License
-------

The protocol as well as the reference implementations are Public Domain.
You can do anything you want with them. You have no legal obligation to do anything else, although I appreciate attribution.

Glossary
--------

* _boot partition_: the first bootable partition of the first disk,
  a rather small one. Most likely an EFI System Partition, but can be
  any other FAT partition as well if the partition is bootable (bit 2 set in flags).
  It can hold a FAT filesystem or entirely the INITRD.

* _environment file_: a one page long utf-8 file on boot partition at
  BOOTBOOT\CONFIG or (when initrd is on the partition) /sys/config,
  with "key=value" pairs (separated by newlines). The protocol
  only specifies two of the keys: "screen" for screen size,
  and "kernel" for the name of the executable inside the initrd.

* _initrd_: initial ramdisk image in ROM or on boot partition at
  BOOTBOOT\INITRD or it can occupy the whole partition. It's format is not
  specified (the good part :-) ) and can be optionally gzip compressed.

* _loader_: a native executable on the boot partition or in ROM. For multi-
  bootable disks more loader can co-exists.

* _filesystem driver_: a plug-in that parses initrd for the kernel. Without
  one the first ELF64/PE32+ executable in the initrd will be loaded.

* _kernel file_: an ELF64/PE32+ executable inside initrd, optionally with
  the following symbols: fb, environment, bootboot (see linker script).

* _BOOTBOOT structure_: an informational structure defined in `bootboot.h`.


Boot process
------------

1. the firmware locates the loader, loads it and passes control to it.
2. the loader initializes hardware (long mode, screen resolution, memory map etc.)
3. then loads environment file and initrd file from the boot partition (or from ROM).
4. iterates on filesystem drivers, and loads kernel from initrd.
5. if filesystem is not recognized, scans for the first executable in initrd.
6. parses executable header and symbols to get link addresses (only level 2 compatible loaders).
7. maps framebuffer, [environment](https://github.com/bztsrc/osz/blob/master/etc/sys/config) and [bootboot structure](https://github.com/bztsrc/osz/blob/master/loader/bootboot.h) accordingly.
8. sets up stack, registers and jumps to [entry point](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/start.S). See example kernel below.

Machine state
-------------

When the kernel gains control, the memory mapping looks like this (unless symbol table is provided.
A fully compatible level 2 loader should map these where the symbols tell to):

```
   -64M framebuffer                       (0xFFFFFFFFFC000000)
    -2M core          bootboot structure  (0xFFFFFFFFFFE00000)
        -2M+1page     environment         (0xFFFFFFFFFFE01000)
        -2M+2page.. v kernel text segment (0xFFFFFFFFFFE02000)
         ..0        ^ kernel stack
    0-16G RAM identity mapped
```

And the registers:

```
    rax/r0     magic 'BOOTBOOT' (0x544f4f42544f4f42)
    rbx/r1     virtual address of bootboot structure
    rcx/r2     virtual address of environment variables
    rdx/r3     virtual address of linear framebuffer
```

Interrups are turned off, GDT unspecified, but valid and code segment running on ring 0 (supervisor mode).
The stack is at the top of the memory, starting at zero and growing downwards.

You can locate your initrd in memory using the [bootboot structure](https://github.com/bztsrc/osz/blob/master/loader/bootboot.h)'s initrd_ptr and initrd_size members.
The boot time and a platform independent memory map is also provided in bootboot structure.
The screen is properly set up with a 32 bit (x8r8g8b8) linear framebuffer, mapped at the defined location (the physical address of the framebuffer can be found in the bootboot structure's fb_ptr field).

[Environment](https://github.com/bztsrc/osz/blob/master/etc/sys/config) is passed to your kernel as newline separated "key=value" pairs.

Filesystem drivers
------------------

Luckily the EFI loader can load files from the boot partition with the
help of the firmware. MultiBoot implementation is not so lucky,
therefore it needs an extra functions to do that. That function however
is out of the scope of this specification: the BOOTBOOT Protocol only
states that a compatible loader must be able to load initrd and environment
from the boot partition, but does not describe how.

On the other hand it does specify one API function to locate a file (the kernel)
inside initrd, but the ABI is also implementation specific. This function
receives a pointer to initrd in memory, and returns a pointer to the
first byte of the kernel and it's size. On error (if filesystem is not recognized or
the kernel is not found) returns {NULL,0}. Plain simple.

```c
typedef struct {
    uint8_t *ptr;
    uint64_t size;
} file_t;

file_t myfs_initrd(uint8_t *initrd, char *filename);
```

The protocol expects that a BOOTBOOT compliant loader iterates on a list of drivers until one
returns a non-NULL.

If all filesystem drivers returned {NULL,0}, the loader will brute-force
scan for the first ELF64/PE32+ image in the initrd, which is quite comfortable specially
when you want to use your own filesystem but you don't have an fs driver for it.
You just copy your initrd on the EFI System Partition, and ready to rock and roll!

The BOOTBOOT Protocol expects the [filesystem drivers](https://github.com/bztsrc/osz/blob/master/loader/efi-x86_64/fs.h) to be separated from
the rest of the loader in source. This is so because it was designed to help the needs of hobby
OS developers, specially those who want to write their own filesystem.

For boot partition, Multiboot version expects *defragmented* FAT12, FAT16 or FAT32 filesystems.
EFI version relies on any filesystem that's supported by EFI Simple FileSystem Protocol. As for initrd, 
the reference implementations support cpio (hpodc, newc and crc variants), ustar and [FS/Z](https://github.com/bztsrc/osz/blob/master/docs/fs.md).
Gzip compressed initrds also supported to save disk space and load time.

Example kernel
--------------

A minimal kernel that demostrates how to access BOOTBOOT environment:

```c
#include <bootboot.h>

/* imported virtual addresses, see linker script */
extern BOOTBOOT bootboot;
extern unsigned char *environment;
extern uint8_t fb;

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
gcc -Wall -fpic -ffreestanding -m64 -mno-red-zone -c kernel.c -o kernel.o
gcc -nostdlib -Xlinker --nmagic -T link.ld -o kernel
strip -s -K fb -K bootboot -K environment kernel
```

A minimal linker script would be:

```c
FBUF_ADDRESS = 0xfffffffffc000000;
CORE_ADDRESS = 0xffffffffffe00000;

ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
SECTIONS
{
    /* bootboot.fb_ptr holds the physical address, this is the linear */
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
cp yourkernel tmp/sys/core
# copy more files to tmp/ directory
cd tmp
# create your filesystem image or an archive. For the latter, use one of these:
find . | cpio -H newc -o | gzip > ../INITRD
find . | cpio -H crc -o | gzip > ../INITRD
find . | cpio -H hpodc -o | gzip > ../INITRD
tar -czf ../INITRD *
```

2. Create FS0:\BOOTBOOT directory on boot partition, and copy the archive you've created
        into it. If you want, create a text file named [CONFIG](https://github.com/bztsrc/osz/blob/master/etc/sys/config)
        there too, and put your [environment variables](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md) there.
        It cannot be larger than 4095 bytes (1 page minus 1 byte) long. Temporary variables will be appended at the end.

```
// BOOTBOOT Options

// --- Loader specific ---
screen=800x600
kernel=sys/core

// --- Kernel specific, you're choosing ---
anythingyouwant=somevalue
otherstuff=enabled

```

Alternatively you can copy an uncompressed INITRD into the whole partition using your fs only, leaving FAT filesystem enitrely out.
You can also create an Option ROM out of INITRD (on BIOS there's not much space ~64-96k, but on EFI it can be 16M).

3. copy the BOOTBOOT loader on the boot partition:

3.1. *EFI disk*: copy __bootboot.efi__ to **_FS0:\EFI\BOOT\BOOTX64.EFI_**.

3.2. *EFI ROM*: use __bootboot.rom__ which is a standard **_PCI Option ROM image_**.

3.3. *BIOS disk*: copy __bootboot.bin__ to **_FS0:\BOOTBOOT\LOADER_**. You can place it inside your INITRD partition
        or outside of partition area as well. Install __mbr.bin__ in the master boot record, saving bootboot.bin's first
        logical sector's number in a dword at 0x1B0.
    
3.4. *BIOS ROM*: install __bootboot.bin__ in a **_BIOS Expansion ROM_**.

Troubleshooting
---------------

```
BOOTBOOT-PANIC: LBA support not found
```

Really old hardware. Your BIOS does not support LBA. This message is generated by 1st stage loader (mbr.bin).

```
BOOTBOOT-PANIC: FS0:\BOOTBOOT\LOADER not found
```

The 2nd stage loader (bootboot.bin) is not on the disk or it's starting LBA address is not recorded
in (P)MBR at dword [0x1B0]. This message is generated by 1st stage loader (mbr.bin).

```
BOOTBOOT-PANIC: Hardware not supported
```

Really old hardware. Your CPU is older than family 6.0 or PAE, MSR, LME features not supported.

```
BOOTBOOT-PANIC: No boot partition
```

Either the disk does not have a GPT, or there's no EFI System Partition nor any other bootable
partition on it. Or the FAT filesystem is found but inconsistent, or doesn't have a BOOTBOOT directory.

```
BOOTBOOT-PANIC: FS0:\BOOTBOOT\INITRD not found
```

The loader could not find the initial ramdisk image on the boot partition. This message will be shown
even if you specify an alternative initrd file on EFI command line.

```
BOOTBOOT-PANIC: Kernel not found in initrd
```

Kernel is not included in the initrd, or initrd's fileformat is not recognized by any of the filesystem
drivers and scanning haven't found a valid executable header in it.

```
BOOTBOOT-PANIC: Kernel is not a valid executable
```

The file that was specified as kernel could be loaded by fs drivers, but it's not an ELF, it's class is not
CLASS64, endianness does not mach architecture, or does not have any program header with a loadable segment
in the negative p_vaddr range (see linker script); or it's not a 64 bit PE32+ executable for the architecture.

```
BOOTBOOT-PANIC: GOP failed, no framebuffer
```
```
BOOTBOOT-PANIC: VESA VBE error, no framebuffer
```

The loader was unable to set up linear framebuffer with packed 32 bit pixels in the requested resolution.
Possible soultion is to modify screen to "screen=800x600" in CONFIG, which is the minimal resolution supported.

That's all, hope it will be useful!
