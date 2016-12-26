OS/Z Device Drivers
===================

They are shared libs which are loaded to system task's address space and allowed to map MMIO and use in/out instructions.

Supported devices
-----------------

 * VESA2.0 VGA, GOP, UGA (set up by loader)
 * x86_64 syscall, NX protection
 * PIC / IOAPIC + APIC, x2APIC, see [ISRs](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isrs.sh)
 * PS2 [keyboard](https://github.com/bztsrc/osz/blob/master/drv/input/ps2/keyboard.S) and [mouse](https://github.com/bztsrc/osz/blob/master/drv/input/ps2/mouse.S)

Planned drivers
---------------

 * PIT, RTC
 * HPET

Directories
-----------

Device drivers are located in [src/drv](https://github.com/bztsrc/osz/blob/master/drv), categorized:

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
| proc     | Processor (and related controllers, like timers) |
| satellite | Satellite communications controller |
| serial   | Serial bus controller |
| signal   | Signal processing controller |
| storage  | Mass storage controller (block devices) |
| wireless | Wireless controller |

Under these directories each driver has exactly one sub-directory. The compiled
driver will be placed in `lib/sys/(class)/(driver).so`

The resemblance with [PCI device classes](http://pci-ids.ucw.cz/read/PD) is not a coincidence.

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
| *     | Any, means the driver should be loaded. Such as ISA devices, like PS2 and file system drivers, like [vfat](https://github.com/bztsrc/osz/blob/master/src/drv/vfat) |
| pciXXX:XXX | A PCI vendor and device id pair |
| (etc) |  |

These informations along with the driver's path will be gathered to `etc/sys/drivers` by
[tools/drivers.sh](https://github.com/bztsrc/osz/blob/master/tools/drivers.sh). This database
will be looked up in order to find out which shared object should be loaded for a specific
device.

Platform files just list platforms, like "x86_64". At compilation time it will be checked,
and if the platform it's compiling for not listed there, the driver won't be compiled. This
way it's easy to avoid having PS2 driver when creating an AArch64 image.

