OS/Z Services
=============

There are two different kind of services: system services and user services. System services
must reside on the initial ramdisk, as they are the server counterparts of libc, and they
cannot be controlled from userspace. User services on the other hand are controlled by the
init system service, and they can be loaded from external disks as well. The system services
group is divided into another two fractions: critical and non-critical ones.

### Service hierarchy

```
        +-----------------------------+
ring 0  |              CORE           | (supervisor)
--------+-----------+--------+--------+--------------------------
ring 3  |  Drivers  |   FS   |   UI   | (critical services)
        +-----------------------------+
        | syslog | net | sound | init | (respawnable, non-critical system services)
        +----------------------+      |
        | logind | prnd | ...         | (respawnable, user services controlled by init)
        +-----------------------------+

        +-----------------------------+
        | identity | sh | sys | ...   | (normal user applications)
        +-----------------------------+
```

CORE
----

Special service which runs in privileged supervisor mode. It's always mapped in all memory map.
The lowest level code of OS/Z like ISRs, physical memory allocation, task switch, timers and such
are implemented here. All the other services (including the driver tasks) are running in non-privileged
user mode (ring 3).

Typical functions: alarm(), setuid(), setsighandler(), yield(), fork(), execve().

Drivers
-------

Each device driver has it's own task. They are allowed to map MMIO into their bss, and to access i/o address space.
Only "FS" task and CORE allowed to send message to them.

FS
--

The file system service. It's a big database server, that emulates all layers as files.
OS/Z shares [Plan 9](https://en.wikipedia.org/wiki/Plan_9_from_Bell_Labs)'s famous "everything is a file" approach.
This process has the initrd mapped in it's bss segment, and uses free memory as a disk cache. On boot, the file system
drivers are loaded into it's address space as shared libraries. It has a couple built-in file systems, like "sysfs".

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
CORE is gathering it's logs in a temporary buffer (using syslong_early() function). The size of the buffer can be
configured with `nrlogmax=4`. That buffer is shared with the "syslog" task. You can turn logging off with the
boot paramter `syslog=false`. When disabled, the same service can be provided by an alternative non-system service.

Typical functions: syslog(), setlogmask().

Net
---

Service that is responsible for handling interfaces and IP routes, and all other networking stuff. You
can disable networking with the boot parameter `networking=false`. When disabled, the same service can be
provided by an alternative non-system service.

Typical functions: connect(), accept(), bind(), listen().

Sound
-----

Service that mixes several audio channels into a single stream. You
can disable audio as a whole with the boot parameter `sound=false`. When disabled, the same service can be
provided by an alternative non-system service.

Typical functions: speak(), setvolume().

Init
----

This service manages all the other, non-system services. Those include user session daemon, mailer daemon, 
print daemon, web server etc. 
You can skip init and start a rescue shell instead with the boot patamerer `rescueshell=true`.

Typical functions: start(), stop(), restart(), status().

