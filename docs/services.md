OS/Z Services
=============

There are two different kind of services: system services and user services. System services
must reside on the initial ramdisk, and they are the server counterparts of libc, and they
cannot be controlled from userspace. User services on the other hand are controlled by the
init system service, and they can be loaded from external disks as well.

Core
----

Special service which runs in supervisor mode. It's always mapped in all memory map.
The lowest level code of OS/Z like ISRs, physical memory allocation, task switch, timers are implemented here.
All the other services (including system services) run in non-privileged user mode (ring 3).

Typical functions: alarm(), setuid(), setsighandler(), yield().

FS
--

The file system daemon. It's responsible for calling the right driver to load the block from disk
and to pass that data to a filesystem driver to interpret it. This process has the initrd mapped in
it's bss, and uses free memory as a disk cache.

Typical functions: fopen(), fread(), fclose(), mount().

Syslog
------

The system logger daemon. Syslog's bss segment is periodically flushed to disk.

Only one function: syslog().

UI
--

User Interface daemon. This concept is unknown to any existing OS. It's something like putting tty
consoles, Xorg and libGL in a single library that also acts as a Wayland compositor. It receives
scancodes from input devices and sends keycodes to the focused application. It has an intimate
connection with the video driver to blit to the framebuffer.

Typical functions: printf(), draw_box(), draw_arc(), glPush().

Net
---

Service that is responsible for handling interfaces and routes, and all other networking stuff.

Typical functions: connect(), accept(), bind(), listen().

Init
----

This service manages all the other, non-system services, like mailer daemon, print daemon, web server etc. 
which have their own shared library other than libc. On startup init is forked as the login process and
it manages the user session as well.

Typical functions: start(), stop(), restart(), status().
