OS/Z Boot Process
=================

Loader
------

OS/Z uses the [BOOTBOOT](https://github.com/bztsrc/osz/tree/master/loader) protocol to get the system running.
The compatible loader is loaded by the firmware, presumeably from ROM. It does the following (platform independently):

 1. initialize hardware and framebuffer
 2. locate first bootable disk
 3. locate first bootable partition
 4. locate initrd on boot partition
 5. locate `lib/sys/core` inside initrd
 6. map it's elf segments at -2M..0 and framebuffer at -512M..-4M
 7. transfer control to core

Currently both x86_64 [BIOS](https://github.com/bztsrc/osz/blob/master/loader/mb-x86_64/bootboot.asm) and [UEFI](https://github.com/bztsrc/osz/blob/master/loader/efi-x86_64/bootboot.c) boot supported by the same disk image.

Core
----

The core consist of two different parts: one is platform independent, with sources located in [src/core](https://github.com/bztsrc/osz/blob/master/src/core). The other part is platform specific, and can be found in [src/core/(platform)](https://github.com/bztsrc/osz/blob/master/src/core/x86_64). The first part is written entirely in C, the second has mixed C and assembly sources.

During boot, the loader locates the `lib/sys/core` in initrd and maps it at -2M. After that the loader passes control to the entry point of the ELF, and the loader will be never invoked again.

That entry point is at the label `_start`, which can be found in  [src/core/(platform)/start.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/start.S). It disables interrupts, sets up segments registers, stack frame and checks required CPU features. It also initializes [kprintf](https://github.com/bztsrc/osz/blob/master/src/core/kprintf.c), so that it can write kpanic messages if a CPU feature is missing. Finally jumps to the C startup code `main()` in [src/core/main.c](https://github.com/bztsrc/osz/blob/master/src/core/main.c) which is a platform independent code.

That platform independent code does the following:

 1. as OS/Z core is polite, greets you with a "starting" text, then
 2. `env_init` parses the environment variables passed by the loader in [src/core/env.c](https://github.com/bztsrc/osz/blob/master/src/core/env.c)
 3. `pmm_init()` sets up Physical Memory Manager in [src/core/pmm.c](https://github.com/bztsrc/osz/blob/master/src/core/pmm.c)

After that it initializes subsystems (or system [services](https://github.com/bztsrc/osz/blob/master/docs/services.md)), begining with the three critical ones:

 1. first is the user space counterpart of core, "SYS" task. Loaded by `sys_init()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c). It's a very special user process and has a lot of platform dependent code. It loads [device drivers](https://github.com/bztsrc/osz/blob/master/docs/drivers.md) instead of normal shared libraries, and MMIO areas are mapped in it's bss segment.
 2. second is the `fs_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c) which is a normal service, save it has the initrd entirely mapped in it's bss segment.
 3. in order to communicate with the user, user interface is initialized with `ui_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c). That is mandatory, unlike networking and sound services which are optional.
 4. initializes syslog service by `syslog_init()` in[src/core/syslog.c](https://github.com/bztsrc/osz/blob/master/src/core/syslog.c).
 5. loads additional, non-critical services by `service_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c) like the `init` service daemon.
 6. says I'm "ready" covering out the "starting" message.
 7. and as a last thing, switches to user space by calling `sys_enable()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c).

That `sys_enable()` function switches to system task, and starts executing it. Because `sys_init()` has disabled IRQs,
system task can freely call all device driver's initialization code without interruption. 

Driver Initialization
---------------------

The "SYS" task starts by calling all shared libraries (device drivers) entry points to initialize. After that
it enables all IRQs that it has a handler for, and notifies core to enable interrupt flag as well. And with that
the true multitasking began, and `sched_pick` in [src/core/sched.c](https://github.com/bztsrc/osz/blob/master/src/core/sched.c)
chooses a thread to run. See [testing tutorial](https://github.com/bztsrc/osz/blob/master/docs/howto1-testing.md) on
how to find out which task was interrupted by the debugger.

When scheduler returns control to "SYS" task again, it will be in an endless loop waiting for IRQ events.

User land
---------

The first real 100% userspace process is either [sbin/init](https://github.com/bztsrc/osz/blob/master/src/init/main.c), or if rescueshell was requested in [etc/CONFIG](https://github.com/bztsrc/osz/blob/master/etc/CONFIG), it's [bin/sh](https://github.com/bztsrc/osz/blob/master/src/sh/main.c).

Init will start further services (not subsystems) among others the user session service that provides a login prompt.

### Executables

They start at label `_start` defined in [lib/libc/(platform)/crt0.S](https://github.com/bztsrc/osz/blob/master/src/lib/libc/x86_64/crt0.S).

### Shared libraries

Similarly to executables, but the entry point is called `_init`. If not defined in the library, fallbacks to [lib/libc/(platform)/init.S](https://github.com/bztsrc/osz/blob/master/src/lib/libc/x86_64/init.S).

### Services

Service's entry point is also called `_init`, which function must call either `mq_recv()` or `mq_dispatch()`. If not defined, fallbacks to [lib/libc/service.c](https://github.com/bztsrc/osz/blob/master/src/lib/libc/service.c).
