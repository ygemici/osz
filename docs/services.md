OS/Z Services
=============

There are two different kind of services: system services and user services. System services
must reside on the initial ramdisk, and they are the server counterparts of libc, and they
cannot be controlled from userspace. User services on the other hand are controlled by the
init system service, and they can be loaded from external disks as well. The system services
group is divided into another two fractions: critical and non-critical

### Service levels
```
        +-----------------------------------+
ring 0  |                CORE               | (supervisor)
--------+-----------------------------------+--------------------------
ring 3  |   SYSTEM   |    FS    |    UI     | (critical services)
        +-----------------------------------+
        | syslog | net | sound | init | ... | (respawnable, non-critical system services)
        +-----------------------------------+
        | prnd | sessiond | httpd | ...     | (respawnable, user services controlled by init)
        +-----------------------------------+
```

Core
----

Special service which runs in privileged supervisor mode. It's always mapped in all memory map.
The lowest level code of OS/Z like ISRs, physical memory allocation, task switch, timers and such
are implemented here. All the other services (including the "system" task) are running in non-privileged
user mode (ring 3).

Typical functions: alarm(), setuid(), setsighandler(), yield(), fork(), execve().

System
------

The user space counterpart of core that runs in non-privileged mode.
This process has all the device drivers mapped in as shared libraries. Whenever an IRQ occurs, the core
switches to this process. It's also the idle thread.

Functions: only one, SYS_IRQ event handler.

FS
--

The file system service. It's responsible for calling the right driver to load the block from disk
and to pass that data to a filesystem driver to interpret it. This process has the initrd mapped in
it's bss, and uses free memory as a disk cache. On boot, the file system drivers are loaded into it's
address space as shared libraries.

Typical functions: printf(), fopen(), fread(), fclose(), mount().

UI
--

User Interface service. This concept is unknown to any existing OS. It's something like putting tty
consoles, Xorg and libGL in a single library that also acts as a compositor. It receives
scancodes from input devices and sends keycodes to the focused application. It shares one half of a double
screen buffer with the video card driver.

Typical functions: draw_box(), draw_arc(), glPush(), SDL_Init().

Syslog
------

The system logger daemon. Syslog's bss segment is a circular buffer that periodically flushed to disk.

Typical functions: syslog(), setlogmask().

Net
---

Service that is responsible for handling interfaces and routes, and all other networking stuff. You
can disable networking with the boot parameter `networking=false`.

Typical functions: connect(), accept(), bind(), listen().

Sound
-----

Service that mixes several audio channels into a single stream. You
can disable audio as a whole with the boot parameter `sound=false`.

Typical functions: speak(), setvolume().

Init
----

This service manages all the other, non-system services, like mailer daemon, print daemon, web server etc. 
which have their own shared library other than libc. On startup init is forked as the login process and
it manages the user session as well. You can skip init and start a rescue shell instead with the boot patamerer
`rescueshell=true`.

Typical functions: start(), stop(), restart(), status().

