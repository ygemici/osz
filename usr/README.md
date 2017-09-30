Userspace
=========

Libraries, applications and user services that are mounted under /usr.

This directory differs in OS/Z to other UNIXes. The first
sub-directory is the package name, and all usual directories
are at second level. Note that this broke [File Hieracrchy Standard](http://www.pathname.com/fhs/) on purpose,
because this makes easy for the package manager to keep track of files. It's like /usr/X11R6 or /opt in Linux and pretty much
like Applications folder under MacOSX.

For POSIX and FHS compatibility, OS/Z uses directory unions, like

```
/usr/share -> /usr/.../share
```

or

```
/etc -> /sys/etc, /usr/.../etc
```

Examples:
---------

| OS/Z version | Usual place on UNIX |
| ------------ | ------------------- |
| /usr/core/include | /usr/include |
| /usr/openssl/include | /usr/include/openssl |
| /usr/gcc/bin/gcc | /usr/bin/gcc |
| /usr/gnupg/lib | /usr/lib/gnupg |
| /usr/GeoIP/share | /usr/share/GeoIP |
| /usr/core/man | /usr/share/man |
| /usr/vlc/man/man1 | /usr/share/man1 |
