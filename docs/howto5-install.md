OS/Z - How To Series #5 - Installing
====================================

Preface
-------

In the last tutorial, we've checked what can we do with the [rescue shell](https://github.com/bztsrc/osz/blob/master/docs/howto4-rescueshell.md).
In this episode we'll take a system administator's approach again, and we'll see how to install OS/Z and how to install packages.

The next tutorial will be end user related, about how to use the [user interface](https://github.com/bztsrc/osz/blob/master/docs/howto6-interface.md).

System Install
--------------

### From Development Environment

It's quite easy to install it on a removable media. Just [download live image](https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true) and use

```sh
dd if=bin/disk.dd of=/dev/sdc
```

Where `/dev/sdc` is the device where you want to install.

### Inside OS/Z

Once you've booted up OS/Z, you can clone the running system with

```sh
sys clone /dev/disk2
```

Where `/dev/disk2` is the device where you want to install the system.

Package Install
---------------

### Refresh package list

The available sources are stored in `etc/sys/packages`. To refresh list, issue

```sh
sys update
```

### Upgrade the system to latest version

Without second argument, it only displays a list of packages that can be updated.

```sh
sys upgrade all
sys upgrade (package)
```

### Search for packages

```sh
sys search (string)
```

### Install a package

```sh
sys install (package)
```

### Remove a package

```sh
sys remove (package)
```

Next time we'll see how to use the [user interface](https://github.com/bztsrc/osz/blob/master/docs/howto6-interface.md).
