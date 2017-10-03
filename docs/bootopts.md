OS/Z Boot Options
=================

Environment configuration file
------------------------------

The boot options are kept on the first bootable partition on the first bootable disk under `FS0:\BOOTBOOT\CONFIG` or `/sys/config`. When you're
creating a disk image, the contents of that file are taken from [etc/sys/config](https://github.com/bztsrc/osz/blob/master/etc/sys/config).

This file is a plain ASCII file with `key=value` pairs, parsed by [core/env.c](https://github.com/bztsrc/osz/blob/master/src/core/env.c)
and [libc/env.c](https://github.com/bztsrc/osz/blob/master/src/lib/libc/env.c). No whitespaces allowed, and each pair is separated by a newline (0x0A) character.
The file can't be longer than a page (4096 bytes on x86_64). You can put comments in it with '#", '//' and '/*'.

Keys are ASCII names without spaces, values can be decimal and hexadecimal [numbers, booleans or strings](https://github.com/bztsrc/osz/blob/master/docs/howto3-develop.md#configure).

Boot Parameters
---------------

| Parameter | Default  | Type | Subsystem | Description |
| --------- | :------: | ---- | --------- | ----------- |
| screen    | 1024x768 | num<i>x</i>num | [loader](https://github.com/bztsrc/osz/blob/master/loader) | required screen resolution |
| kernel    | sys/core | string  | loader | the name of kernel executable on initrd |
| debug     | 0        | decimal | core | specifies which debug information to show (if [compiled with debugging](https://github.com/bztsrc/osz/blob/master/Config), see below) |
| nrphymax  | 2        | number  | core | the number of pages to store physical RAM fragments (16 bytes each) |
| nrmqmax   | 1        | number  | core | the number of pages for Message Queue (64 bytes each) |
| nrlogmax  | 8        | number  | core | the number of pages for early syslog buffer |
| quantum   | 100      | number  | core | scheduler frequency, a task can allocate CPU continously for (quantum) timer interrupts. |
| display   | 0        | number  | core | selects output mode, see below |
| syslog    | true     | boolean | core | disable syslog [service](https://github.com/bztsrc/osz/blob/master/docs/services.md) |
| networking | true    | boolean | core | disable inet service |
| sound     | true     | boolean | core | disable sound service |
| pathmax   | 512      | number  | fs   | Maximum length of path, minimum 256 |
| fps       | 10       | number  | ui   | requested frame rate |
| keymap    | en_us    | string  | ui   | keyboard layout, see [etc/kbd](https://github.com/bztsrc/osz/blob/master/etc/sys/etc/kbd) |
| lefthanded | false   | boolean | ui   | swap pointers |
| identity  | false    | boolean | init | run first time setup before servies to get machine's identity |
| rescueshell | false  | boolean | init | if true, starts `/bin/sh` instead of user services |
| clock     | 0        | number  | platform | override autodetected clock source |
| hpet      | -        | hexdec  | platform | x86_64-acpi override autodetected HPET address |
| apic      | -        | hexdec  | platform | x86_64-acpi override autodetected LAPIC address |
| ioapic    | -        | hexdec  | platform | x86_64-acpi override autodetected IOAPIC address |

Debugging
---------

This can be a numeric value, or a comma separated list of flags, see [debug.h](https://github.com/bztsrc/osz/blob/master/etc/include/sys/debug.h).

| Value | Flag | Define | Description |
| ----: | ---- | ------ | ----------- |
| 0     |      | DBG_NONE | no debug information |
| 1     | me   | DBG_MEMMAP | dump memory map on console (also dumped to syslog) |
| 2     | ta   | DBG_TASKS | dump [system services](https://github.com/bztsrc/osz/blob/master/docs/services.md) with TCB physical addresses |
| 4     | el   | DBG_ELF | debug [ELF loading](https://github.com/bztsrc/osz/blob/master/src/core/elf.c#L66) |
| 8     | ri   | DBG_RTIMPORT | debug [run-time linker](https://github.com/bztsrc/osz/blob/master/src/core/elf.c#L486) imported values |
| 16    | re   | DBG_RTEXPORT | debug run-time linker exported values |
| 32    | ir   | DBG_IRQ | dump [IRQ Routing Table](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isr.c#L121) |
| 64    | de   | DBG_DEVICES | dump [System Tables](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/acpi/acpi.c) and [PCI devices](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pci.c) |
| 128   | sc   | DBG_SCHED | debug [scheduler](https://github.com/bztsrc/osz/blob/master/src/core/sched.c) |
| 256   | ms   | DBG_MSG | debug [message sending](https://github.com/bztsrc/osz/blob/master/src/core/msg.c) |
| 512   | lo   | DBG_LOG | dump [early syslog](https://github.com/bztsrc/osz/blob/master/src/core/syslog.c) |
| 1024  | pm   | DBG_PMM | debug [physical memory manager](https://github.com/bztsrc/osz/blob/master/src/core/pmm.c) |
| 2048  | vm   | DBG_VMM | debug [virtual memory manager](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/vmm.c) |
| 4096  | ma   | DBG_MALLOC | debug [libc memory allocation](https://github.com/bztsrc/osz/blob/master/src/lib/libc/bztalloc.c) |
| 8192  | bl   | DBG_BLKIO | debug [block level I/O](https://github.com/bztsrc/osz/blob/master/src/fs/vfs.c) |
| 16284 | te   | DBG_TESTS | run [tests](https://github.com/bztsrc/osz/blob/master/src/test) instead of [init](https://github.com/bztsrc/osz/blob/master/src/init) task |

Most of these only available when compiled with [DEBUG = 1](https://github.com/bztsrc/osz/blob/master/Config). Normally you can only use two to troubleshoot boot: DBG_DEVICES and DBG_LOG.

Display
-------

A numeric value or exactly one flag.

| Value | Flag | Define | Description |
| ----: | ---- | ------ | ----------- |
| 0     | mc   | DSP_MONO_COLOR | a simple 2D pixelbuffer with 32xRGB0 color pixels |
| 1     | sm,an | DSP_STEREO_MONO | two 2D pixelbuffers*, they are converted grayscale and a red-cyan filtering applied, anaglyph |
| 2     | sc,re | DSP_STEREO_COLOR | two 2D pixelbuffers*, the way of combining left and right eye's view is 100% driver specific, real 3D |

(* the two buffers are concatenated in a one big double heighted buffer)


Clock Source
------------

Either a numeric value or exactly one flag.

| Value | Flag | Platform     | Description |
| ----: | ---- | --------     | ----------- |
| 0     |      | -            | auto detect |
| 1     | hp   | x86_64-acpi  | High Precision Event Timer (default) |
| 2     | pi   | x86_64-acpi  | Programmable Interval Timer (fallback) |
| 2     | pi   | x86_64-ibmpc | Programmable Interval Timer (default) |
| 3     | rt   | x86_64-ibmpc | Real Time Clock |
| 1     | bu   | AArch64-rpi  | Built-in ARM timer (default) |
| 2     | rt   | AArch64-rpi  | External Real Time Clock chip on GPIO |

