OS/Z Porting to new platforms
=============================

The hardware abstraction layer is divided into one plus three layers:

 * [loader](https://github.com/bztsrc/osz/blob/master/loader) (not part of the OS)
 * [core](https://github.com/bztsrc/osz/blob/master/src/core)
 * [device drivers](https://github.com/bztsrc/osz/blob/master/docs/drivers.md)
 * [libc](https://github.com/bztsrc/osz/blob/master/src/lib/libc)
 
Porting loader
--------------

In order to boot OS/Z, a [BOOTBOOT](https://github.com/bztsrc/bootboot) compliant loader must exists for
the platform. The loader is not strictly part of the OS, as it can load kernels other than OS/Z `core`, and also from
`core`'s perspective the loader abstracts the boot firmware away with a [common interface](https://github.com/bztsrc/osz/blob/master/loader/bootboot.h).

Porting core
------------

Note there's a distinction between platform and architecture. The latter only includes the CPU with some essentional
features like MMU or FPU (usually built-in CPU, but not necessairly), while the former includes everything shipped on
the motherboard or SoC (usually not replaceable by the end user), like interrupt controllers, nvram, PCI(e) bus and
BIOS or UEFI firmware.

For valid architecture and platform combinations, see [compilation](https://github.com/bztsrc/osz/blob/master/docs/compile.md).

The `core` is responsible for handling the lowest level of the hardware, such as

 * CPU management
 * interrupt controllers and generating IRQ messages for driver tasks
 * timers (including pre-emption)
 * MMU management, virtual memory mapping, address space switching
 * managing and scheduling tasks (creating and switching execution environment, strongly CPU dependent)
 * low level inter-task communication with [messaging](https://github.com/bztsrc/osz/blob/master/docs/messages.md#supervisor-mode-ring-0)
 * system bus enumeration for device detection
 * rebooting, powering off the machine

The `core` is loaded by the `loader`, and it [boots](https://github.com/bztsrc/osz/blob/master/docs/boot.md) the
rest of the operating system by initializing lowest level parts, enumerating system buses, loading and initializing
[system services](https://github.com/bztsrc/osz/blob/master/docs/services.md) and device drivers. Once it's finished
with all of that, it passes control the `init` system service.

A significant part of the `core` is platform independent, and written in C, [sys/core](https://github.com/bztsrc/osz/blob/master/src/core).
Hardware specific part is written in C and Assembly, in [sys/core/(platform)](https://github.com/bztsrc/osz/blob/master/src/core/x86_64). This
is the only part of OS/Z that runs in supervisor mode (ring 0 on x86_64), and it must be kept small.

Although OS/Z is a micro-kernel, the `core` does a little more than other typical micro-kernels, like Minix. It is so
because the goal was performance and also to provide a minimal, but well-defined support for each platform. So `core`
abstracts the platform in a minimalistic approach: everything that's required to support individual, pre-empted, auto
detected, inter communicating device driver tasks must be in `core`, but nothing more and specially not the driver's
code itself. Everything else must be pushed out to user space in separate binaries. There's
a simple rule: if the hardware can be physically replaced by the end user (eg.: using PCI card or BIOS setup), it must
be supported by a device driver and not by `core` for sure.

Porting device drivers
----------------------

Drivers that can't be or worth not be written as separate user space tasks (only interrupt controllers, timers,
CPU and memory management) are implemented in `core`.

Device drivers have to be written for each and every platform. Some [drivers are written in C](https://github.com/bztsrc/osz/blob/master/docs/howto3-develop.md)
and can be used on different platforms as-is, but it's most likely they have some Assembly parts and platform specific
implementation in C. To help with that, the build system looks for a [platforms](https://github.com/bztsrc/osz/blob/master/docs/drivers.md)
file which lists all platforms that the driver supports. Likewise there's a `devices` file which lists device specifications
that `core` uses during system bus enumeration to detect if any of the supperted devices exist on the configuration
it's booting on.

Device driver tasks are running at a [higher priority level](https://github.com/bztsrc/osz/blob/master/docs/scheduler.md)
than other user space tasks, and they are allowed to use some driver specific `libc` functions, like
[setirq()](https://github.com/bztsrc/osz/blob/master/etc/include/sys/driver.h). Also they are allowed to access IO space
(which can mean to use specific instructions like inb/outb or passing MMIO addresses to mmap() call depending which
one is supported by the architecture).

Porting libc
------------

This shared library is called `libc` for historic reasons, in reality it's a platform independent interface
to all OS/Z functions, so should be called `libosz`. It provides library functions for user space services, libraries
and applications as well as for device drivers. It's mostly written in C, but some parts had to be written in Assembly
(like memcpy() for performance and calling a supervisor mode service in `core` for compatibility). It hides
all details of low level messaging, but just in case also provides a platform independent, 
[user level abstraction for messaging](https://github.com/bztsrc/osz/blob/master/docs/messages.md#low-level-user-library).

All programs compiled for OS/Z must be dynamically linked with `libc`, and must not use other, lower level abstractions
directly. At `libc` level, OS/Z is totally platform independent.

Porting libraries and applications
----------------------------------

As mentioned before, `libc` does not only provide the usual functions, but also serves as a common interface to
all OS/Z services (including device driver specific functions and user interface functions for example). This ease the
development for new programs, but also makes porting code written for other operating systems a little bit harder. You
have to use `#ifdef _OS_Z_` pre-define blocks. There's no way around this as
[OS/Z is not POSIX](https://github.com/bztsrc/osz/blob/master/docs/posix.md) by design. Although I did my best to make
it similar to POSIX, that's still OS/Z's own interface. For one, `errno()` is a function and not a global
variable. On the other hand this `libc` provides everything for interfacing with OS/Z, which guarantees that all OS
specific stuff is limited to this single library.

OS/Z is designed in a way that at shared library level all applications are binary compatible for a specific
architecture, and source compatible for all architectures and platforms. Therefore libraries should not and applications
must not use any Assembly or platform specific C code. If such a thing is required and cannot be solved with device
drivers, then that code must be placed in a separate library with an interface common on all platforms, and should be
implemented for all the platforms. The pixman library would be a perfect example for that.

Because it is the `libc` level where OS/Z becames platform independent, unit and functionality
[tests](https://github.com/bztsrc/osz/blob/master/src/test) are provided for this level and above, but not for lower
levels.
