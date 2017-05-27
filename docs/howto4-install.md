OS/Z - How To Series #4 - Installing
====================================

Preface
-------

In the last tutorial, we've checked what can we do with the [rescue shell](https://github.com/bztsrc/osz/blob/master/docs/howto3-rescueshell.md).
In this episode we'll take a system administator's approach again, and we'll see how to install OS/Z.

From Development Environment
----------------------------

It's quite easy to install it on a removable media. Just [download live image](https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true) and use

```sh
dd if=bin/disk.dd of=/dev/sdc
```

Where `/dev/sdc` is the device where you want to install.

Inside OS/Z
-----------

Once you've booted up OS/Z, you can clone the running system with

```sh
sys install /dev/disk2
```

Where `/dev/disk2` is the device where you want to install.

Next time we'll see how to use the [user interface](https://github.com/bztsrc/osz/blob/master/docs/howto5-interface.md).
