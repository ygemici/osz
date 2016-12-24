OS/Z Boot Process
=================

Loader
------

OS/Z uses the [BOOTBOOT](https://github.com/bztsrc/osz/tree/master/loader) protocol to get the system running.
The compatible loader is loaded by the firmware, it's purpose is to get the OS running. For that,
all implementations share the same scheme:

 1. initialize hardware and framebuffer
 2. locate first bootable disk
 3. locate first bootable partition
 4. locate initrd on boot partition
 5. locate core inside initrd
 6. map core at -2M..0 and framebuffer at -512M..4M
 7. transfer control to core

Currently both x86_64 [BIOS](https://github.com/bztsrc/osz/blob/master/loader/mb-x86_64/bootboot.asm) and [UEFI](https://github.com/bztsrc/osz/blob/master/loader/efi-x86_64/bootboot.c) boot supported by the same disk image.

Core
----

The core consist of two different parts: one is platform independent, and sources for that located in [src/core](https://github.com/bztsrc/osz/blob/master/src/core), the other part which holds the platform dependent code can be found in [src/core/(platform)](https://github.com/bztsrc/osz/blob/master/src/core/x86_64). The first part is written entirely in C, the other has mixed C and assembly sources.

During boot, the loader locates `lib/src/core` in initrd and maps it at -2M. After that the loader passes control to the entry point of the ELF, and the loader will never used again.

That entry point is at the label `_start`, which can be found in  [src/core/(platform)/start.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/start.S). It disables interrupts, sets up segments registers, stack, and finally jumps to the C startup code `main()` in [src/core/main.c](https://github.com/bztsrc/osz/blob/master/src/core/main.c) which is a platform independent code.

That platform independent code does the following:

 1. in order to allow `kprintf`, it sets up console by calling `kprintf_init` in [src/core/kprintf.c](https://github.com/bztsrc/osz/blob/master/src/core/kprintf.c)
 2. calls `cpu_init()` to detect and enable platform specific cpu features in [src/core/(platform)/start.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/start.S).
 3. `env_init` parses the environment variables passed by the loader in [src/core/env.c](https://github.com/bztsrc/osz/blob/master/src/core/env.c)
 4. `pmm_init()` sets up Physical Memory Manager in [src/core/pmm.c](https://github.com/bztsrc/osz/blob/master/src/core/pmm.c)

After that it initializes system services (subsystems):

 1. first of all, the user space counterpart of core, "system" task by calling `sys_init()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c). It's a very special user process and has a lot of platform dependent code. It loads [device drivers](https://github.com/bztsrc/osz/blob/master/drivers.md) instead of normal shared libraries, and MMIO areas are mapped in it's bss segment.
 2. second is the `fs_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c) which is a normal service, save it has the initrd mapped in it's bss segment.
 3. on order to communicate with the user, user interface is initialized with `ui_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c). That is mandatory, unlike networking and sound services which are optional.
 4. the call `service_init()` in [src/core/service.c](https://github.com/bztsrc/osz/blob/master/src/core/service.c) is used to load several other parts.
 5. as a last thing, switch to userland by calling `sys_enable()` in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c).

That `sys_enable()` function switches to system task, and starts executing it. Because `sys_init()` has cleared Interrupt Flag,
system task can freely call all device driver's initialization code without interruption.

After all driver init codes executed, the system task begun to anwser to events. As there are no messages in the queue, it blocks. By doing so, the true multitasking began, and `sched_pick` in [src/core/sched.c](https://github.com/bztsrc/osz/blob/master/src/core/sched.c) chooses a thread to run.

User
----

The first real userspace process is either [sbin/init](https://github.com/bztsrc/osz/blob/master/src/init/main.c), or if rescueshell was requested in [etc/CONFIG](https://github.com/bztsrc/osz/blob/master/etc/CONFIG), it's [bin/sh](https://github.com/bztsrc/osz/blob/master/src/sh/main.c).

Init will start further services (not subsystems) among others the user session service that provides a login prompt.
