OS/Z - How To Series #4 - Rescue Shell
======================================

Preface
-------

In the last tutorial, we took a look at how to [develop](https://github.com/bztsrc/osz/blob/master/docs/howto3-develop.md)
applications, which is (naturally) quite developer oriented tutorial.
In this episode we'll take a system administator's approach, and we'll see how to use the rescue shell on OS/Z.

Activating Rescue Shell
-----------------------

You'll have to enable rescue shell in [environment](https://github.com/bztsrc/osz/blob/master/etc/sys/config) by setting `rescueshell=true`. When you boot, OS/Z won't start the `init` subsystem to initialize user services, rather it will drop you
in a root shell.

Available Commands
------------------

TODO: write howto

Next time we'll talk about how to [install](https://github.com/bztsrc/osz/blob/master/docs/howto5-install.md) OS/Z on different media.
