OS/Z Device Drivers
===================

TODO, in short they are shared libs which are loaded to system task's address space and allowed to map MMIO and use in/out instructions.

Supported devices
-----------------

 * VESA2.0 VGA, GOP, UGA (set up by loader)
 * x86_64 syscall, NX protection
 * PIC / IOAPIC + APIC, x2APIC, see [ISRs](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/isrs.sh)
 * PS2 keyboard

Planned drivers
---------------

 * PIT, RTC
 * HPET
