FS/Z file system
================

It was designed to store unimagineable amounts of data. It's a classic UNIX type filesystem with inodes and superblock.

Block size can be 2048, 4096, 8192... etc. The suggested sector size is 4096 which matches the TLB page size. That would
ease the loading of sectors to memory.

For detailed specification, see [fsZ.h](https://github.com/bztsrc/osz/blob/master/etc/include/fsZ.h).

Super Block
-----------

The super block is the first sector of the media and repeated in the last sector. If the superblocks differ, they have
checksums to decide which sector is correct.

File ID
-------

The file id (fid in short) is a logical sector number that holds an inode structure. It can go up to 2^128.

Inode
-----

The most important structure of FS/Z. It describes portion of the disk as files, directory entries etc. It's important
that it can hold pointers to the old versions of a file thus making per file versioning possible just like in FILES11.

Sector List
-----------

All list have 32 bytes entries with a starting logical sector number and a number of sectors. Both can go up to 2^128.

Sector Directory
----------------

A sector that has 16 bytes long logical sector addresses.

Directory
---------

Every directory has 128 bytes long entries. The first entry is the directory header, the others are normal entries.

16 bytes go for the fid, 1 more byte for the number of characters in the filename. That makes filename up to 111 bytes long
(maybe less characters as UTF-8 is a variable length encoding).

File paths
----------

All full path to a file looks like:

```
 (drive):(directory)/(filename);(version)#(offset)
```

Drive specifies the media where the filesystem is stored. If not given, `root:` will be substituted, which points to the
initrd loacted in memory.

Directory part is a list of directories, separated by '/'.

Filename can be 111 bytes in length, and may consist of any characters save control characters (<32), delete control (127),
separators (device ':', directory '/', version ';' and offset '#'), as well as invalid UTF-8 sequences. All other
UNICODE characters allowed.

Version part by default is ';0' which points to the current version of the file. The version ';-1' refers to the version
before the current one, ';-2' the version before that and so forth. Providing positive numbers as version refers to a date
in the form ';mmdd' or ';yyyymd' or ';yyyymmddHHii'. The actual version for that date will be returned, or the oldest one
if such version is not found. For dates in the future the current version is used.

Offset part defaults to '#0' which is the beginning of the file. Positive numbers are offsets into the file, while negative
ones are relative offsets from the end of the file.

Mounts
------

You can refer to a piece of data on the disk as a directory or as a file. Directories are always terminated by '/'. This is
also the case if the directory is a mount point for a block device. There refering to as a file (no trailing '/') would
allow to fread() and fwrite() the storage at block level, whilst referencing with a terminating '/' would allow to use opendir()
and readdir(), which would return the root directory of the filesystem stored on that device.
