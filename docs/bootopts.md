OS/Z Boot Options
=================

Configuration file
------------------

The boot options are kept on the first bootable partition on the first bootable disk under FS0:\BOOTBOOT\CONFIG. When you're
creating a disk image, the contents of that file are taken from [etc/CONFIG](https://github.com/bztsrc/osz/blob/master/etc/CONFIG).

This file is a plain ASCII file with key=value pairs parsed by [core/env.c](https://github.com/bztsrc/osz/blob/master/src/core/env.c). No whitespaces allowed, and each pair is separated by a newline (0x0A) character.
The file has to be one page long (4096 bytes on x86_64), and filled up with spaces. You can put comments in it with '//'.

Keys are ASCII names without spaces, values can be decimal and hexadecimal numbers, booleans or strings.

Boot Parameters
---------------

| Parameter | Default | Type | Parsed by | Description |
| --------- | :-----: | ---- | --------- | ----------- |
| width     | 800    | number | [loader](https://github.com/bztsrc/osz/blob/master/loader) | required screen resolution |
| height    | 600    | number | loader |  -II-  |
| kernel    | lib/sys/core | string | loader | the name of kernel executable on initrd |
| debug     | 0      | decimal | core | specifies which debug information to show (if compiled with debugging, see below) |
| hpet      | -      | hexdec | core | override autodetected HPET address |
| apic      | -      | hexdec | core | override autodetected LAPIC address |
| ioapic    | -      | hexdec | core | override autodetected IOAPIC address |
| nrphymax  | 2      | number | core | the number of pages to store physical RAM fragments (16 bytes each) |
| nrmqmax   | 1      | number | core | the number of pages for Message Queue (64 bytes each) |
| nrsrvmax  | 1      | number | core | the number of pages for services pid translation table (8 bytes each) |
| nrlogmax  | 16     | number | core | the number of pages for early syslog buffer |
| nrirqmax  | 8      | number | core | the maximum number of IRQ handlers per IRQ |
| quantum   | 100    | number | core | scheduler frequency, a thread can allocate CPU continously for 1/(quantum) second. |
| fps       | 10     | number | core | requested frame rate |
| display   | 0      | number | core | selects output mode (see below) |
| networking | true | boolean | core | disable networking [system service](https://github.com/bztsrc/osz/blob/master/docs/services.md) |
| sound | true | boolean | core | disable sound service |
| identity  | false  | boolean | core | force running first time setup to get machine's identity, such as hostname |
| rescueshell | false | boolean | core | if true, starts `/bin/sh` instead of `/sbin/init` |

The available values for debug parameter and display can be found in [core/env.h](https://github.com/bztsrc/osz/blob/master/src/core/env.h).

Debugging
---------

| Value | Define | Description |
| ----: | ------ | ----------- |
| 0     | DBG_NONE | no debug information |
| 1     | DBG_MEMMAP | dump memory map (either E820 or EFI) |
| 2     | DBG_THREADS | dump [system services](https://github.com/bztsrc/osz/blob/master/docs/services.md) with TCB physical addresses |
| 4     | DBG_ELF | debug [ELF loading](https://github.com/bztsrc/osz/blob/master/src/core/service.c) |
| 8     | DBG_RTIMPORT | debug [run-time linker](https://github.com/bztsrc/osz/blob/master/src/core/service.c) imported values |
| 16    | DBG_RTEXPORT | debug run-time linker exported values |
| 32    | DBG_IRQ | dump [IRQ Routing Table](https://github.com/bztsrc/osz/blob/master/src/core/service.c) |
| 64    | DBG_DEVICES | dump [System Tables](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/acpi.c) and [PCI devices](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/pci.c) |
| 128   | DBG_SCHED | debug [scheduler](https://github.com/bztsrc/osz/blob/master/src/core/sched.c) |
| 256   | DBG_MSG | debug [message sending](https://github.com/bztsrc/osz/blob/master/src/core/msg.c) |

Display
-------

| Value | Define | Description |
| ----: | ------ | ----------- |
| 0     | DSP_MONO_MONO | a simple 2D pixelbuffer grayscale filtered on framebuffer copy |
| 1     | DSP_MONO_COLOR | a simple 2D pixelbuffer with 32ARGB0 color pixels |
| 2     | DSP_STEREO_MONO | two 2D pixelbuffers, they are converted grayscale and a red-cyan filtering applied |
| 3     | DSP_STEREO_COLOR | two 2D pixelbuffers, the way of combining left and right eye's view is 100% driver specific |
