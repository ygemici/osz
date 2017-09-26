OS/Z - How To Series #3 - Developing
====================================

Preface
-------

Last time we've checked how to [debug OS/Z](https://github.com/bztsrc/osz/blob/master/docs/howto2-debug.md) in a virtual machine. Now we'll take a look on how to develop for the system.

The [next turorial](https://github.com/bztsrc/osz/blob/master/docs/howto4-rescueshell.md) is more user than developer oriented as it's about how to use the rescue shell.

Headers
-------

For every application in OS/Z, one header must be included:

```c
#include <osZ.h>
```

That will include everything else required to access standard C library. Although I did everything to keep
function call names and arguments from POSIX to ease porting existing software, OS/Z never
intened to be and is [not POSIX compatible](https://github.com/bztsrc/osz/blob/master/docs/posix.md).

Applications
------------

The mandatory Hello World example is very similar to UNIX like systems, looks like

```c
#include <osZ.h>

int main(int argc, char **argv)
{
    printf( "Hello World!\n" );
    return 0;
}
```

Device drivers
--------------

Drivers are shared libraries which use a common [event dispatcher](https://github.com/bztsrc/osz/blob/master/src/lib/libc/dispatch.c) mechanism. Also they use some privileged libc
functions, and provide services therefore a minimal driver looks like

```c
#include <osZ.h>
#include <sys/driver.h>

public void irq10(uint8_t irq)
{
    /* handle your irq here */
}

public void *ioctl(uint64_t fmt,...)
{
    /* implement ioctl calls here */
}

public void task_init()
{
    /* initialize your driver here. */
}
```

IRQs can be assigned three ways to tasks:

 1. use `irqX()` function, where X is a decimal number. That will assign IRQ X.
 2. using `setirq()` libc call at driver's initialization code.
 3. or autodetected from system tables, in which case implement `irq()`.

In driver's `task_init()` function, it may register device files in "devfs" with `mknod()` calls.
If a device driver requires a configuration, it can't load config files as filesystems are not mounted
when their initialization code is called. Instead it should use a string defined in driver.h:

```c
extern unisgned char *_environment;
```

Which is the [environment](https://github.com/bztsrc/osz/blob/master/etc/sys/config) file mapped by the run-time linker,
so that the driver can parse it looking for it's configuration.

Note that you don't have to do any message receive calls, just create public functions for your message types.

Additionaly to Makefile, the build system looks for two more files along with a device driver source.

 * `platform` - if this file exists, it can limit the driver for specific platforms
 * `devices` - specifies which devices the driver supports, used by `core` during system bus enumeration.

Entry points
------------

### Applications

They start at label `_start` defined in [lib/libc/(platform)/crt0.S](https://github.com/bztsrc/osz/blob/master/src/lib/libc/x86_64/crt0.S).

### Shared libraries

Similarly to executables, but the entry point is called `_init`. If not defined in the library, fallbacks to [lib/libc/(platform)/init.S](https://github.com/bztsrc/osz/blob/master/src/lib/libc/x86_64/init.S).
When an executable loads several shared libraries, their `_init` will be called one by one before `_start` gets called.

### Services and drivers

Service's entry point is also called `_init`, which function must call either `mq_recv()` or `mq_dispatch()`. If not defined otherwise,
fallbacks to the default in [lib/libc/service.c](https://github.com/bztsrc/osz/blob/master/src/lib/libc/service.c) (also
used by device drivers). In that case the service must implement `task_init()` to initialize it's structures. Services can use
shared libraries, in which case the libraries' `_init` will be called one by one before the service's `_init` function.

The [next turorial](https://github.com/bztsrc/osz/blob/master/docs/howto4-rescueshell.md) is about how to use the rescue shell.
