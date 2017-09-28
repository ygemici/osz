OS/Z Boot Process
=================

Loader
------

OS/Z uses the [BOOTBOOT](https://github.com/bztsrc/osz/tree/master/loader) protocol to get the system running.
The compatible loader is loaded by the firmware, presumeably from ROM. It does the following (platform independently):

 1. initialize hardware and framebuffer
 2. load initrd (from boot partition or from ROM)
 3. locate `sys/core` inside initrd and map it at -2M..0
 4. transfer control to `core`

Currently both x86_64 [BIOS](https://github.com/bztsrc/osz/blob/master/loader/x86_64-bios/bootboot.asm) and [UEFI](https://github.com/bztsrc/osz/blob/master/loader/x86_64-efi/bootboot.c) boot supported by the same disk image.

Core
----

First we have to clearify that `core` consist of two parts: one is platform independent, the other part is platform specific
(see [porting](https://github.com/bztsrc/osz/blob/master/docs/porting.md)). During boot, the execution is jumping forth and back
from one part to the another.

The entry point that the loader calls is at the label `_start`, which can be found in  [src/core/(platform)/start.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/start.S).
It disables interrupts, sets up segments registers, stack frame and checks required CPU features. It also initializes [kprintf](https://github.com/bztsrc/osz/blob/master/src/core/kprintf.c), so that it can write kpanic messages if a CPU feature is missing.
Finally jumps to the C startup code `main()` in [src/core/main.c](https://github.com/bztsrc/osz/blob/master/src/core/main.c) which is a platform independent code.

That platform independent `main()` does the following:

 1. as OS/Z core is polite, greets you with a "starting" text, then
 2. `platform_init()` sets up some default values for the platform
 3. `env_init` parses the environment variables passed by the loader in [src/core/env.c](https://github.com/bztsrc/osz/blob/master/src/core/env.c)
 4. `pmm_init()` sets up Physical Memory Manager in [src/core/pmm.c](https://github.com/bztsrc/osz/blob/master/src/core/pmm.c)
 5. initializes system [services](https://github.com/bztsrc/osz/blob/master/docs/services.md), starting with `sys_init()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c), which loads idle task
 and enumerates system buses to locate and load [device drivers](https://github.com/bztsrc/osz/blob/master/docs/drivers.md). It will also fill up entries
in [IRQ Routing Table](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isr.c#123) (IRT).
 6. second service is the `fs_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c) which is a normal system service, save it has the initrd entirely mapped in it's bss segment.
 7. user interface is initialized with `ui_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c). These first three service are mandatory, unlike the rest.
 8. loads additional, non-critical tasks by several `service_init()` calls in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c) like the `syslog`, `net`, `sound` and `init` system services.
 9. switches to user space by calling `sys_enable()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c).

That function switches to the "FS" task, and starts executing it. When it blocks, the scheduler,
`sched_pick` in [src/core/sched.c](https://github.com/bztsrc/osz/blob/master/src/core/sched.c)
will choose drivers and services one by one until all blocks. Note that pre-emption is not enabled at this point.

When the `idle` task first scheduled, it will call `sys_ready()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c), which
 1. enables IRQs with valid entries in IRT. It will also enable timer IRQ and with that pre-emption.
 2. sends a message to "FS" task to mount all filesystems in the name of "init" task.

The control is passed to "FS" task again, which will parse fstab. When mounting is finished, will notify "init" task and
normal operation begins.

User land
---------

The first real 100% userspace process is started by the [init](https://github.com/bztsrc/osz/blob/master/src/init/main.c) system service.
If rescueshell was requested in [environment](https://github.com/bztsrc/osz/blob/master/etc/sys/config), then init starts [bin/sh](https://github.com/bztsrc/osz/blob/master/src/sh/main.c)
instead of user services. Services are classic UNIX daemons, among others the user session service that provides a login prompt.

End game
--------

When the `init` system service exists, the execution will be passed back to core. It will say 'Good Bye', depending on
the exit status, with `kprintf_reboot()` or `kprintf_poweroff()`.
