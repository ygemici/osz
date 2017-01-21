OS/Z Services
=============

There are two different kind of services: system services and user services. System services
must reside on the initial ramdisk, and they are the server counterparts of libc, and they
cannot be controlled from userspace. User services on the other hand are controlled by the
init system service, and they can be loaded from external disks as well. The system services
group is divided into another two fractions: critical and non-critical

### Service hierarchy

```
        +-----------------------------+
ring 0  |              CORE           | (supervisor)
--------+-----------+--------+--------+--------------------------
ring 3  |    SYS    |   FS   |   UI   | (critical services)
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
are implemented here. All the other services (including the "SYS" task) are running in non-privileged
user mode (ring 3).

Typical functions: alarm(), setuid(), setsighandler(), yield(), fork(), execve().

SYS
---

The user space counterpart of CORE that runs in non-privileged mode.
This process has all the device drivers mapped in as shared libraries. Whenever an IRQ occurs, the core
switches to this process. It's also the idle thread. Has mappings shared with "FS", "UI" and "syslog" tasks.

Functions: only one, SYS_IRQ event handler.

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
of a double screen buffer with the video card driver in "SYS" task which can hold a stereographic composited image.

Typical functions: draw_box(), draw_arc(), glPush(), SDL_Init().

Syslog
------

The system logger daemon. Syslog's bss segment is a circular buffer that periodically flushed to disk. On boot,
CORE is gathering it's logs in a temporary buffer. The size can be configured with `nrlogmax=4`. That buffer is flushed to
the "syslog" task as soon as it starts, and other services can use the normal buffer.

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

This service manages all the other, non-system services. Those includes user session daemon, mailer daemon, print daemon, web server etc. 
You can skip init and start a rescue shell instead with the boot patamerer `rescueshell=true`.

Typical functions: start(), stop(), restart(), status().

