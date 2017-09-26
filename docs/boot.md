OS/Z Boot Process
=================

Loader
------

OS/Z uses the [BOOTBOOT](https://github.com/bztsrc/osz/tree/master/loader) protocol to get the system running.
The compatible loader is loaded by the firmware, presumeably from ROM. It does the following (platform independently):

 1. initialize hardware and framebuffer
 2. locate initrd on boot partition (or in ROM)
 3. locate `sys/core` inside initrd
 4. map it's text segment at -2M..0 and framebuffer at -64M..-4M
 5. transfer control to core

Currently both x86_64 [BIOS](https://github.com/bztsrc/osz/blob/master/loader/mb-x86_64/bootboot.asm) and [UEFI](https://github.com/bztsrc/osz/blob/master/loader/efi-x86_64/bootboot.c) boot supported by the same disk image.

Core
----

First we have to clearify that the `core` consist of two parts: one is platform independent, with sources
located in [src/core](https://github.com/bztsrc/osz/blob/master/src/core). The other part is platform specific
(see [porting](https://github.com/bztsrc/osz/blob/master/docs/porting.md)),
and can be found in [src/core/(platform)](https://github.com/bztsrc/osz/blob/master/src/core/x86_64).
The first part is written entirely in C, the second has mixed C and assembly sources. During boot, the execution is
jumping forth and back from one part to the other.

The entry point that the loader calls is at the label `_start`, which can be found in  [src/core/(platform)/start.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/start.S).
It disables interrupts, sets up segments registers, stack frame and checks required CPU features. It also initializes [kprintf](https://github.com/bztsrc/osz/blob/master/src/core/kprintf.c), so that it can write kpanic messages if a CPU feature is missing.
Finally jumps to the C startup code `main()` in [src/core/main.c](https://github.com/bztsrc/osz/blob/master/src/core/main.c) which is a platform independent code.

That platform independent code does the following:

 1. as OS/Z core is polite, greets you with a "starting" text, then
 2. `env_init` parses the environment variables passed by the loader in [src/core/env.c](https://github.com/bztsrc/osz/blob/master/src/core/env.c)
 3. `pmm_init()` sets up Physical Memory Manager in [src/core/pmm.c](https://github.com/bztsrc/osz/blob/master/src/core/pmm.c)

After that it initializes subsystems (or system [services](https://github.com/bztsrc/osz/blob/master/docs/services.md)), begining with the critical ones:

 1. first is `sys_init()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c), that loads idle task
 and enumerates system buses to locate and load [device drivers](https://github.com/bztsrc/osz/blob/master/docs/drivers.md). It will also fill up entries
in [IRQ Routing Table](https://github.com/bztsrc/osz/blob/master/docs/howto3-develop.md) (IRT).
 2. second is the `fs_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c) which is a normal system service, save it has the initrd entirely mapped in it's bss segment.
 3. in order to communicate with the user, user interface is initialized with `ui_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c). That is mandatory, unlike the rest.
 4. loads additional, non-critical system services by several `service_init()` calls in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c) like the `syslog`, `net`, `sound` and `init` service daemon.
 5. and as a last thing, switches to user space by calling `sys_enable()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c). That `sys_enable()` function switches to the "FS" task, and starts executing it. When it blocks, the scheduler,
 `sched_pick` in [src/core/sched.c](https://github.com/bztsrc/osz/blob/master/src/core/sched.c) will choose drivers and services one by one until all blocks. Note that preemption is not enabled at this point. 
 6. When the `idle` task first scheduled, it will call `sys_ready()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c). It will enable IRQs which has entries in IRT.
 It will also enable timer IRQ and with that enables pre-emption. As a last thing, `sys_ready()` will send a message to "FS" task to mount all filesystems in the name of "init" task.
 So FS will notify "init" task when it's done, and with that normal operation will start.

User land
---------

The first real 100% userspace process is started by [sys/init](https://github.com/bztsrc/osz/blob/master/src/init/main.c) system service.
If rescueshell was requested in [environment](https://github.com/bztsrc/osz/blob/master/etc/sys/config), then init starts [bin/sh](https://github.com/bztsrc/osz/blob/master/src/sh/main.c)
instead of user services. Services are classic UNIX daemons, among others the user session service that provides a login prompt.
