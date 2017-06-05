Userspace
=========

Applications and libraries that are mounted under /usr.

This directory differs in OS/Z to other UNIXes. The first
subdirectory is the package name, and all usual directories
are at second level. Note that this does not broke File
Hieracrchy Standard, and makes easy to keep track of files for
the package manager. It's pretty much like Applications under MacOSX.

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
