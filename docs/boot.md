OS/Z Boot Process
=================

Loader
------

OS/Z uses the [BOOTBOOT](https://github.com/bztsrc/osz/tree/master/loader) protocol to get the system running.
The compatible loader is loaded by the firmware as the last step of POST. On every platforms, the loader initializes
the hardware (including the framebuffer), loads initrd and locates `sys/core` inside. When found, it maps that at the
top of the address space (-2M..0) and passes control to it.

Core
----

First of all we have to clearify that `core` consist of two parts: one is platform independent, the other part is
architecture and platform specific (see [porting](https://github.com/bztsrc/osz/blob/master/docs/porting.md)).
During boot, the execution is jumping forth and back from one part to the another.

The entry point that the loader calls is `_start`, which can be found in [src/core/(platform)/start.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/start.S).
It disables interrupts, sets up segments registers, stack frame and checks required CPU features. It also initializes [kprintf](https://github.com/bztsrc/osz/blob/master/src/core/kprintf.c), so that it can call kpanic if needed.
Finally jumps to the C startup code `main()` in [src/core/main.c](https://github.com/bztsrc/osz/blob/master/src/core/main.c) which is a platform independent code.

That platform independent `main()` does the following:

 1. as OS/Z is polite, greets you with a "OS/Z starting..." message, then
 2. `env_init` parses the [environment variables](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md) in [src/core/env.c](https://github.com/bztsrc/osz/blob/master/src/core/env.c) passed by the loader.
 3. `pmm_init()` sets up Physical Memory Manager in [src/core/pmm.c](https://github.com/bztsrc/osz/blob/master/src/core/pmm.c).
 4. initializes [system services](https://github.com/bztsrc/osz/blob/master/docs/services.md), starting with `sys_init()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c), which loads `idle` task
 and enumerates system buses to locate and load [device drivers](https://github.com/bztsrc/osz/blob/master/docs/drivers.md). It will also initialize
the [IRQ Routing Table](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isr.c#121) (IRT).
 5. second service is the `fs_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c) which is a normal system service, except it has the initrd entirely mapped in in it's bss.
 6. user interface is initialized with `ui_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c). These first three services (aka `idle`, `FS`, `UI`) are mandatory, unlike the rest.
 7. loads additional, non-critical tasks by several `service_init()` calls in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c) like the `syslog`, `inet`, `sound` and `init` system services.
 8. drops supervisor privileges by calling `sys_enable()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c).

That function switches to the first task, and then the scheduler, `sched_pick` in [src/core/sched.c](https://github.com/bztsrc/osz/blob/master/src/core/sched.c)
chooses driver and service tasks one by one until all of them blocks. Note that pre-emption is not enabled at this point.
Driver tasks perform hardware initialization and they fill up the IRT.

When the `idle` task first scheduled, it will call `sys_ready()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c), which
 1. enables IRQs with entries in IRT. It also enables timer IRQ and with that pre-emption may begin.
 2. finally sends a SYS_mountfs message to "FS" task in the name of "init" task.

The control is passed to ["FS" task](https://github.com/bztsrc/osz/blob/master/src/fs/main.c), which receives the
SYS_mountfs message and parses [fstab](https://github.com/bztsrc/osz/blob/master/etc/sys/etc/fstab).
When mounting finished, it notifies the ["init" task](https://github.com/bztsrc/osz/blob/master/src/init/main.c) and
normal operation begins.

(NOTE: If OS/Z was compiled with debug support and `debug=tests` passed in [environment](https://github.com/bztsrc/osz/blob/master/etc/sys/config),
then `core` loads [system test](https://github.com/bztsrc/osz/blob/master/src/test/main.c) instead of `sys/init`,
which will perform various unit and functionality tests.)

User land
---------

The first real 100% userspace process is started by the [init](https://github.com/bztsrc/osz/blob/master/src/init/main.c) system service.
If rescueshell was requested in [environment](https://github.com/bztsrc/osz/blob/master/etc/sys/config), then init
starts [bin/sh](https://github.com/bztsrc/osz/blob/master/src/sh/main.c). If it is the first run, then starts `bin/identity`
to query computer identity such as hostname from the user. Finally starts all user servies. User services are classic
UNIX daemons, among others the user session service that provides a login prompt.

End game
--------

When the `init` system service quits, the execution is [passed back](https://github.com/bztsrc/osz/blob/master/src/core/msg.c#L180) to `core`.
Then OS/Z says 'Good Bye' with `kprintf_reboot()` or with `kprintf_poweroff()` (depending on the exit status), and the platform
dependent part restarts or turns off the computer.
