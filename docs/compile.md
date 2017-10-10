OS/Z Compilation
================

Requirements
------------

- GNU toolchain (make, gcc, binutils (gas, ld, objcopy, strip), see [tools/cross-gcc.sh](https://github.com/bztsrc/osz/blob/master/tools/cross-gcc.sh))
- bash or dash (shell scripts are used to [generate different files](https://github.com/bztsrc/osz/blob/master/tools/drivers.sh) during compilation)
- optionally fasm (for recompiling BIOS booting)

Configuration
-------------

The `core` is always compiled for a specific [machine](https://github.com/bztsrc/osz/blob/master/docs/porting.md),
which can be controlled in [Config](https://github.com/bztsrc/osz/blob/master/Config) with `ARCH` and `PLATFORM` variables.
Valid combinations are:

| ARCH   | PLATFORM | Description |
| ----   | -------- | ----------- |
| x86_64 | ibmpc    | For old machines, uses PIC and PIT (or RTC), enumerates PCI bus |
| x86_64 | acpi     | For new machines, APIC, x2APIC, IOAPIC, HPET and parses ACPI tables |
| aarch64 | rpi     | Raspberry Pi 3+ |

Compilation
-----------

The make system is created in a way that you can say `make all` in almost every directory. If you do that in the top level
directory, it will compile everything and will also build disk image for you, but surpressing messages. In subdirectories make
will always display the full command executed during compilation.

For example if you enter `src/fs` and issue `make all` there, then only the filesystem subsystem will be recompiled, and
you'll be able to see the exact commands used.

So to compile all the things, simply type

```shell
$ make all
```

You should see something similar to this:

```
$ make all
TOOLS
  src		mkfs.c
  src		elftool.c
CORE
  gen		x86_64/ibmpc/isrs.S (PIC, numirq 32, idtsz 4096)
  lnk		sys/core (x86_64-ibmpc)
  lnk		sys/lib/libc.so (x86_64)
BASE
  src		sys/ui
  src		sys/bin/sh
  src		sys/bin/test
  src		sys/bin/sys
  src		sys/syslog
  src		sys/sound
  src		sys/fs
  src		sys/inet
  src		sys/init
DRIVERS
  src		sys/drv/fs/gpt.so
  src		sys/drv/fs/pax.so
  src		sys/drv/fs/vfat.so
  src		sys/drv/fs/fsz.so
  src		sys/drv/fs/tmpfs.so
  src		sys/drv/input/ps2.so
  src		sys/drv/display/fb.so
  src		sys/driver
USERSPACE
IMAGES
  mkfs		initrd
  mkfs		boot (ESP)
  mkfs		usr
  mkfs		var
  mkfs		home
  mkfs		bin/osZ-latest-x86_64-ibmpc.dd
```

Non-EFI loader
--------------

If you want to recompile `loader/boot.bin` and `loader/bootboot.bin`, you'll need [fasm](http://flatassembler.net).
Unfortunately GAS is not good enough at mixing 16, 32 and 64 bit instuctions, which is necessary for BIOS booting. To keep
my promise that you'll only need the GNU toolchain, I've added those pre-compiled binaries to the source.

See what you've done!
---------------------

The live disk image is generated to [bin/osZ-(ver)-(arch)-(platform).dd](https://github.com/bztsrc/osz/blob/master/bin). Use the
`dd` command to write it on a USB stick or boot with qemu, bochs or VirtualBox.

```
$ dd if=bin/osZ-latest-x86_64-acpi.dd of=/dev/sdc
$ make testq
$ make testb
$ make testv
```

There's a detailed [tutorial on testing](https://github.com/bztsrc/osz/blob/master/docs/howto1-testing.md).
