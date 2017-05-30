OS/Z Device Drivers
===================

/* FIME: rewrite, drivers are now separated into threads */
They are shared libs which are loaded to device driver task's address space and allowed to map MMIO and use in/out instructions.
They have a common, platform independent event dispatcher, [src/drivers/drv.c](https://github.com/bztsrc/osz/blob/master/src/drivers/drv.c).

Supported devices
-----------------

 * VESA2.0 VGA, GOP, UGA (set up by loader, 32 bit frame buffer)
 * x86_64 syscall, NX protection
 * PIC / IOAPIC + APIC, x2APIC, see [ISRs](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isrs.sh)
 * PIT, RTC
 * PS2 [keyboard](https://github.com/bztsrc/osz/blob/master/src/drivers/input/ps2/keyboard.S) and [mouse](https://github.com/bztsrc/osz/blob/master/src/drivers/input/ps2/mouse.S)

Planned drivers
---------------

 * HPET

Directories
-----------

Device drivers are located in [src/drivers](https://github.com/bztsrc/osz/blob/master/src/drivers), categorized:

| Class    | Description |
| -------- | ----------- |
| comctl   | Communication controller |
| display  | Display controller |
| docking  | Docking station |
| encrypt  | Encryption controller |
| fs       | File System Driver |
| generic  | Generic system peripheral |
| input    | Input device controller |
| intelligent | Intelligent controller |
| memory   | Memory controller |
| mmedia   | Multimedia controller |
| network  | Network controller (wired) |
| proc     | Processor (and related controllers) |
| satellite | Satellite communications controller |
| serial   | Serial bus controller |
| signal   | Signal processing controller |
| storage  | Mass storage controller (block devices) |
| wireless | Wireless controller |

Under these directories each driver has exactly one sub-directory. The compiled
driver will be placed in `lib/sys/(class)/(driver).so`

The resemblance with [PCI device classes](http://pci-ids.ucw.cz/read/PD) is not a coincidence.

Note that for performance, interrupt controllers (like PIC, IOAPIC) do not have drivers, they
are built into [core](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isrs.S). To
switch among them, you'll have to edit `ISR_CTRL` define in [isr.h](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isr.h) and recompile.

Timers are also built into the core, but you can switch among HPET, PIT and RTC with a boot time configurable environment variable.

Files
-----

Among the source files of the driver, there are two special ones:

| Name | Description |
| ---- | ----------- |
| devices | List of device ids, that this driver supports |
| platforms | List of platforms for which this driver should be compiled |

Both files are newline (0x0A) separated list of words. The devices file
has special entries:

| Entry | Description |
| ----- | ----------- |
| *     | Any, means the driver should be loaded regardless to bus enumeration. Such as ISA devices, like PS2 and file system drivers, like [vfat](https://github.com/bztsrc/osz/blob/master/src/drivers/fs/vfat) |
| isa   | ISA device drivers that you don't want to autoload |
| pciXXX:XXX | A PCI vendor and device id pair |
| (etc) |  |

These informations along with the driver's path will be gathered to `etc/sys/drivers` by
[tools/drivers.sh](https://github.com/bztsrc/osz/blob/master/tools/drivers.sh). This database
will be looked up in order to find out which shared object should be loaded for a specific
device.

Platform files just list platforms, like "x86_64". At compilation time it will be checked,
and if the platform it's compiling for not listed there, the driver won't be compiled. This
is an easy to avoid having PS2 driver and such when creating for example an AArch64 image, yet
you won't have to rewrite drivers for every architecture that supports it (like a usb storage
driver).


