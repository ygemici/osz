OS/Z Device Drivers
===================

Drivers are shared libraries which are loaded into separate address spaces after a
common, platform independent event dispatcher, [service.c](https://github.com/bztsrc/osz/blob/master/src/lib/libc/service.c).
They are allowed to access IO address space with in/out instructions and to map MMIO at their bss. Otherwise driver tasks
are normal userspace applications.

Supported devices
-----------------

 * VESA2.0 VGA, GOP, UGA (set up by [loader](https://github.com/bztsrc/osz/blob/master/loader), 32 bit frame buffer)
 * x86_64 syscall, NX protection
 * PIC / IOAPIC + APIC, x2APIC, see [ISRs](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isrs.sh)
 * [PIT](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/pit.S), [RTC](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/rtc.S)
 * PS2 [keyboard](https://github.com/bztsrc/osz/blob/master/src/drivers/input/ps2/keyboard.S) and [mouse](https://github.com/bztsrc/osz/blob/master/src/drivers/input/ps2/mouse.S)

Planned drivers
---------------

 * HPET

Directories
-----------

Device drivers are located in [src/drivers](https://github.com/bztsrc/osz/blob/master/src/drivers), groupped in categories.
Each [driver class](https://github.com/bztsrc/osz/blob/master/src/drivers/README.md) has one directory, and
under these directories each driver has exactly one sub-directory. The compiled
driver will be placed in `sys/drv/(class)/(sub-directory).so`. For example, `src/drivers/input/ps2/` will be compiled
to `sys/drv/input/ps2.so`.

Note that for performance, interrupt controllers (like PIC, IOAPIC) do not have separate drivers, they
are built into [core](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isrs.S). To
switch among them, you'll have to edit `ISR_CTRL` define in [isr.h](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isr.h) and recompile.

Timers are also built into the core, but you can switch among HPET, PIT and RTC with a boot time configurable [environment variable](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md).

Files
-----

Among the source files of the driver, there are two special ones:

| Name | Description |
| ---- | ----------- |
| devices | List of device ids, that this driver supports |
| platforms | List of platforms for which this driver should be compiled |

Both files are newline (0x0A) separated list of words. The devices file has the following entries:

| Entry | Description |
| ----- | ----------- |
| *     | Any, means the driver should be loaded regardless to bus enumeration. Such as ISA devices and file system drivers. |
| pciXXX:XXX | A PCI vendor and device id pair |
| clsXXX:XXX | A PCI Class definition |
| (etc) |  |

These informations along with the driver's path will be gathered to `sys/drivers` by
[tools/drivers.sh](https://github.com/bztsrc/osz/blob/master/tools/drivers.sh). This database
will be looked up in order to find out which shared object should be loaded for a specific
device.

Platform files just list platforms, like "x86_64" or "AArch64". At compilation time it will be checked,
and if the platform it's compiling for not listed there, the driver won't be compiled. This
is an easy way to avoid having platform specific drivers in non-compatible images, yet
you won't have to rewrite multi-platform drivers for every architecture (like an usb storage
driver). If the platform file is missing, the driver will be compiled for all platforms.

Developing drivers
------------------

If you want to write a device driver for OS/Z, [here](https://github.com/bztsrc/osz/blob/master/docs/howto3-develop.md) you can find more information.
