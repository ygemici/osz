OS/Z Device Drivers Classes
===========================

| Class    | Description |
| -------- | ----------- |
| comctl   | Communication controller |
| display  | Display controller |
| docking  | Docking station |
| encrypt  | Encryption controller |
| fs       | File System Drivers |
| generic  | Generic system peripheral |
| inet     | Netowrking stack protocols |
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
driver will be placed in `sys/drv/(class)/(driver).so`

The resemblance with [PCI device classes](http://pci-ids.ucw.cz/read/PD) is not a coincidence.

Note that for performance, CPU, memory management and interrupt controllers (like PIC, IOAPIC) do not have separate drivers,
they are built into [core](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isrs.sh).

Likewise, timers are also built into core, but you can switch among them with a boot time configurable [environment variable](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md#L67).

For more information on device drivers, see [documentation](https://github.com/bztsrc/osz/blob/master/docs/drivers.md) and [development how-to](https://github.com/bztsrc/osz/blob/master/docs/howto3-develop.md).
