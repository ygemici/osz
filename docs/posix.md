OS/Z Notable differences to POSIX
=================================

Limits
------

Most of the defines int [limits.h](https://github.com/bztsrc/osz/blob/master/etc/include/limits.h) are meaningless in OS/Z.
Either because I have used an unlimited algorithm or because the limit is boot time [configurable](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md).

Paths
-----

OS/Z does not use `.` and `..` directories. Instead it will try to resolve paths with string manipulations as far as possible,
eliminating a lot of disk access and speed up things. It's similar how VMS worked. OS/Z also implemented per file
versioning from FILES11 (the revision the author used and liked a lot at the university).

