OS/Z Loader
===========

OS/Z uses the [BOOTBOOT](https://github.com/bztsrc/osz/tree/master/loader) protocol to get the system running.
All implementations share the same scheme:

 1. initialize hardware and framebuffer
 2. locate first bootable disk
 3. locate first bootable partition
 4. locate initrd on boot partition
 5. locate core inside initrd
 6. map core and framebuffer at negative address
 7. transfer control to core

Currently both x86_64 BIOS and UEFI boot supported with the same disk image.
