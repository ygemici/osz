OS/Z Boot Process
=================

Loader
------

OS/Z uses the [BOOTBOOT](https://github.com/bztsrc/osz/tree/master/loader) protocol to get the system running.
All implementations share the same scheme:

 1. initialize hardware and framebuffer
 2. locate first bootable disk
 3. locate first bootable partition
 4. locate initrd on boot partition
 5. locate core inside initrd
 6. map core at -2M..0 and framebuffer at -512M..4M
 7. transfer control to core

Currently both x86_64 BIOS and UEFI boot supported by the same disk image.

Core
----

The ELF `lib/src/core` is mapped at -2M. It starts it's execution at a label `_start`. That can be found
in the file `src/core/(platform)/start.S`. It sets up segments registers, stack etc. and jumps to the C code.

Next, `main` is called in `src/core/main.c` which is a platform independent code. That function will
call the initialization code for each subsystems, then starts multitasking.

User
----

The first real userspace process is either `sbin/init`, or if rescueshell is requested, `bin/sh`.

Init will start the user session service to provide a login prompt.
