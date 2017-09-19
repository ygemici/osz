OS/Z Core
=========

This directory holds the source of the code running in supervisor mode.
It's splitted into two parts:

 - platform independent part
 - platform dependent part

The C source files in this directory are the independent ones. Platform
dependent code are organized in sub-directories, and may have .S assembler
sources as well, at least start.S .

Files
-----

`main.c` - the "heart". Before this could be run, a little housekeeping is done
in [(platform)/start.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/start.S). If you are interested in how
OS/Z is booted, you should read the [documentation on booting](https://github.com/bztsrc/osz/blob/master/docs/boot.md).

`env.c` - environment parser. You can pass key=value pairs to core in [FS0:\BOOTBOOT\CONFIG](https://github.com/bztsrc/osz/blob/master/etc/sys/config). For a complete list of boot options, read the [manual](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md).

`font.h` - PC Screen Font version 2 format.

`fs.c` - an early file system driver to locate files on initial ramdisk. Once booted, the [filesystem service](https://github.com/bztsrc/osz/blob/master/src/fs) is used to retrieve and write files.

`kprintf.c` - console print commands. It's not optimal or effective, but that was never intended. It's sole purpose is to display
messages during boot. Later the [user interface service](https://github.com/bztsrc/osz/blob/master/src/ui) is responsible for
displaying windows.

`msg.c` - messaging functions. As OS/Z is a microkernel, it heavily realies on an effective message bus.

`pmm.c` - Physical Memory Manager. If you're interested how memory management is done, read the [documentation on memory](https://github.com/bztsrc/osz/blob/master/docs/memory.md).

`sched.c` - task scheduler. OS/Z guarantees no starvation with a O(1) algorithm. [Read more](https://github.com/bztsrc/osz/blob/master/docs/scheduler.md).

`service.c` - functions to load system [services](https://github.com/bztsrc/osz/blob/master/docs/services.md).

`(platform)/sys.c` - system service initialization code.

`(platform)/task.c` - task routines. For efficientcy, it has a platform specific implementation.

`(platform)/user.S` - the "system" service, which runs in a non-privileged mode.

`(platform)/libk.S` - low level kernel library, like kmap() or kstrcpy().

`(platform)/supervisor.ld` - linker script for core (privileged).

`(platform)/sharedlib.ld` - linker script for shared libraries and services.

`(platform)/executable.ld` - linker script for normal applications.

Platform
--------

Mostly for effectiveness it's partially compile time configurable. Platform dependent part is responsible for:

 - interrupt controllers and IRQ messages
 - preemption (counting isr_ticks)
 - random seed generation
 - virtual memory mapping
 - address space switching
 - system bus enumeration
