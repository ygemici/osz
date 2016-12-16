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

The most important structure of FS/Z. It describes portion of the disk as files, directory entries etc.

Sector List
-----------

All list have entries with a starting logical sector number and a number of sectors. Both can go up to 2^128.

Sector Directory
----------------

A sector that has 16 bytes long logical sector addresses.

Directory
---------

Every directory has 128 bytes long entries. The first entry is the directory header, the others are normal entries.

16 bytes go for the fid, 1 more byte for filename length. That makes file names size up to 111 bytes (maybe less
characters as UTF-8 is a variable length encoding).
