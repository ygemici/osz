OS/Z - How To Series #5 - Installing
====================================

Preface
-------

In the last tutorial, we've checked what can we do with the [rescue shell](https://github.com/bztsrc/osz/blob/master/docs/howto4-rescueshell.md).
In this episode we'll take a system administator's approach again, and we'll see how to install OS/Z and how to install packages.

System Install
--------------

### From Development Environment

It's quite easy to install it on a hard drive or removable media, like an USB stick.
Just [download live image](https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true) and use

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

You can also induvidually install, upgrade and remove applications.

### Refresh package list

The list of repositories from where packages can be installed are stored in `/sys/repos`.
The meta information for all available packages from all repositories are stored in `/sys/packages`.
To refresh that list, issue

```sh
sys update
```

### Upgrade the system

Without second argument, all packages with newer versions (so the whole system) will be upgraded.
When a package name is given as second argument, only that package is upgraded.

```sh
sys upgrade
sys upgrade (package)
```

### Search for packages

You can search the package meta information database with

```sh
sys search (string)
```

### Install a package

In order to install a package that was not previously on the computer, use

```sh
sys install (package)
```

### Remove a package

If you decide that an application is no longer useful, you can delete it by issuing

```sh
sys remove (package)
```

Next time we'll see how to manage [services](https://github.com/bztsrc/osz/blob/master/docs/howto6-services.md).
