OS/Z Notable differences to POSIX
=================================

Limits
------

Most of the defines in [limits.h](https://github.com/bztsrc/osz/blob/master/etc/include/limits.h) are meaningless in OS/Z.
Either because I have used an unlimited algorithm or because the limit is boot time [configurable](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md).

Errno
-----

In OS/Z's libc, `errno()` is a function and not a global variable. You can set it with `seterr(errno)`. Because
the system was designed to be limitless as possible, a lot of errno values are meaningless and therefore non-existent
in OS/Z. Like ENFILE (File table overflow) or EMFILE (Too many open files). Using [FS/Z](https://github.com/bztsrc/osz/blob/master/docs/fs.md) you'll
never see EFBIG (File too large) or EMLINK (Too many links) errors either.

It also has [new values](https://github.com/bztsrc/osz/blob/master/etc/include/errno.h), like ENOTUNI (Not an union) or ENOTSHM (Not a shared memory buffer).

Paths
-----

OS/Z does not store `.` and `..` directories on disk. Instead it will resolve paths with string manipulations, eliminating a
lot of disk access and speed up things. For that it is mandatory to end directory names with the '/' character. It's very
similar how VMS worked. OS/Z also implements per file versioning from FILES11 (the file system the author used and liked a lot
at his university times).

As an addition, OS/Z handles a special `...` directory as a joker. It means all sub-directories at that level, and will be
resolved as the first full path that matches. For example, assuming we have '/a/b/c', '/a/d/a' and '/a/e/a' directories, then
'/a/.../a' will return '/a/d/a' only. Unlike in VMS, the three dots only matches one directory level, and does not dive into
their sub-directories recursively.

Another addition, called directory unions. These are symbolic link like constructs (not like Plan9 unions which are overloaded
mounts on the same directory). They have one or more absolute paths (in which the joker directory also allowed). All paths listed
in an union will be checked looking for a full path match. Example: the '/bin' directory in OS/Z is an union of '/sys/bin' and
'/usr/.../bin'. All files under these directories show up in '/bin'. Therefore OS/Z does not use the classic PATH environment
variable at all, since all executables can be found in '/bin'.

