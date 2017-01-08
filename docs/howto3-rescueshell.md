OS/Z - How To Series #3 - Rescue Shell
======================================

Preface
-------

In the first two tutorials, we took a look at how to [test](https://github.com/bztsrc/osz/blob/master/docs/howto1-testing.md)
and [debug](https://github.com/bztsrc/osz/blob/master/docs/howto2-debug.md), which are quite developer oriented tutorials. In this episode we'll take a system administator's approach, and we'll see how to use the rescue shell on OS/Z.

Activating Rescue Shell
-----------------------

You'll have to enable rescue shell in [FS0:\BOOTBOOT\CONFIG](https://github.com/bztsrc/osz/blob/master/etc/CONFIG) by setting `rescueshell=true`. When you boot, OS/Z won't start the `init` subsystem to initialize user services, rather it will drop you
in a root shell.

Available Commands
------------------

TODO: write howto

Next time we'll talk about how to [install](https://github.com/bztsrc/osz/blob/master/docs/howto4-install.md) OS/Z on different media.
