OS/Z Core
=========

This directory holds the source of the code running in supervisor mode.
It's splitted into two parts:

 - platform independent part
 - platform dependent part

The C source files in this directory are the independent ones. [Platform
dependent code](https://github.com/bztsrc/osz/blob/master/docs/porting.md) is organized in sub-directories, and have .S assembler
sources as well, at least start.S .

Files
-----

`main.c` - the "heart". Before this could be run, a little housekeeping is done
in [(platform)/start.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/start.S). If you are interested in how
OS/Z is booted, you should read the [documentation on booting](https://github.com/bztsrc/osz/blob/master/docs/boot.md).

`env.c` - environment parser. Parses [FS0:\BOOTBOOT\CONFIG](https://github.com/bztsrc/osz/blob/master/etc/sys/config).

`font.h` - PC Screen Font version 2 format.

`fs.c` - an early file system driver to locate files on initial ramdisk. Once booted, the [filesystem service](https://github.com/bztsrc/osz/blob/master/src/fs) is used to retrieve and write files.

`kprintf.c` - console print commands. It's sole purpose is to display messages during boot. Later the [user interface service](https://github.com/bztsrc/osz/blob/master/src/ui) is responsible for
displaying windows and terminals.

`msg.c` - messaging functions. As OS/Z is a microkernel, it heavily realies on an effective message bus.

`pmm.c` - Physical Memory Manager. If you're interested how memory management is done, read the [documentation on memory](https://github.com/bztsrc/osz/blob/master/docs/memory.md).

`sched.c` - task scheduler. OS/Z guarantees no starvation with a O(1) algorithm. [Read more](https://github.com/bztsrc/osz/blob/master/docs/scheduler.md).

`service.c` - functions to load system [services](https://github.com/bztsrc/osz/blob/master/docs/services.md) and [device drivers](https://github.com/bztsrc/osz/blob/master/docs/drivers.md).

`syscall.c` - message handler for SYS_core destination.

`syslog.c` - boot time syslog implementation.

`(platform)/envarch.c` - hardware specific environment parser.

`(platform)/vmm.c` - Virtual Memory manager.

`(platform)/sys.c` - system bus enumeration code and idle task.

`(platform)/task.c` - task routines.

`(platform)/libk.S` - low level kernel library, like kmap(), kstrcpy() or ksend().

`(platform)/supervisor.ld` - linker script for core (privileged supervisor mode).

`(platform)/sharedlib.ld` - linker script for shared libraries and services.

`(platform)/executable.ld` - linker script for normal applications.
