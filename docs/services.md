OS/Z Services
=============

There are two different kind of services: system services and user services. System services
must reside on the initial ramdisk, as they are the server counterparts of `libc`, and they
cannot be controlled from userspace. User services on the other hand are controlled by the
init system service, and they can be loaded from external disks as well.

### Service hierarchy

```
        +--------------------------------------------------+
ring 0  |                      CORE                        | (supervisor)
--------+---------+----+----+------------------------------+---------------------
ring 3  | Drivers | FS | UI | syslog | inet | sound | init | (system services)
        +-------------------------------------------+      |
        | logind | prnd | httpd | ...                      | (user services controlled by init)
        +--------------------------------------------------+

        +--------------------------------------------------+
        | identity | sh | sys | fsck | test | ...          | (normal user applications)
        +--------------------------------------------------+
```

CORE
----

Special service which runs in privileged supervisor mode. It's always mapped in all memory map.
The lowest level code of OS/Z like ISRs, physical memory allocation, task switching, timers and such
are implemented here. Although it looks like a separate task, `idle` is part of core as it needs to halt the CPU.
All the other services (including the driver tasks) are running in non-privileged user mode (ring 3).


Typical functions: alarm(), setuid(), setsighandler(), yield(), fork(), execve(), mmap(), munmap().

Drivers
-------

Each device driver has it's own task. They are allowed to map MMIO into their bss segment, and to access I/O address space.
Only "FS" task and CORE allowed to send messages to them. There's one exception, the video driver, which receives
messages from "UI" task, not "FS".

Typical functions: IRQ(), ioctl(), setcursor().

FS
--

The file system service. It's a big database server, that emulates all layers as files.
OS/Z shares [Plan 9](https://en.wikipedia.org/wiki/Plan_9_from_Bell_Labs)'s famous "everything is a file" approach.
This process has the initrd mapped in it's bss segment, and uses free memory as a disk cache. On boot, the file system
drivers are loaded into it's address space as shared libraries. It has a couple built-in file systems, like "devfs".

Typical functions: printf(), fopen(), fread(), fclose(), mount().

UI
--

User Interface service. It's job is to manage tty consoles, window surfaces and GL contexts. And
when it's required, composites all of them in a single frame and tells the hardware to flush video memory.
It receives scancode from input devices and sends keycode messages to the focused application in return. It shares one half
of a double screen buffer with the video card driver which can hold a stereographic composited image.

Typical functions: draw_box(), draw_arc(), glPush(), SDL_Init().

Syslog
------

The system logger daemon. Syslog's bss segment is a circular buffer that periodically flushed to disk. On boot,
CORE is gathering it's logs in a temporary buffer (using [syslog_early()](https://github.com/bztsrc/osz/blob/master/src/core/syslog.c) function). The size of the buffer can be
[configured](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md) with `nrlogmax=4`. That buffer is shared with the "syslog" task. You can turn logging off with the
boot paramter `syslog=false`.

Typical functions: syslog(), setlogmask().

Inet
----

Service that is responsible for handling interfaces and IP routes, and all other networking stuff. You can disable
networking with the [boot environment](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md) variable `networking=false`.

Typical functions: connect(), accept(), bind(), listen().

Sound
-----

Service that mixes several audio channels into a single stream. It opens sound card device excusively, and offers a mixer device
for applications instead. You can disable audio as a whole with the [boot parameter](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md) `sound=false`.

Typical functions: speak(), setvolume().

Init
----

This service manages all the other, non-system services. Those include user session daemon, mailer daemon, 
print daemon, web server etc. You can command init to start only a rescue shell with the boot environment variable `rescueshell=true`.
Also, if compiled with [DEBUG = 1](https://github.com/bztsrc/osz/blob/master/Config) and if you set `debug=test`
[boot parameter](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md), a special [test](https://github.com/bztsrc/osz/blob/master/src/test) process will be started instead
of `init` that will do system tests.

Typical functions: start(), stop(), restart(), status().

