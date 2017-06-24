FS/Z File System
================

It was designed to store unimagineable amounts of data. It's a classic UNIX type filesystem with inodes and superblock
but without their limitations. Logical sector numbers are stored in 2^128 bits to be future proof. Current implementation
only uses 2^64 bits (can handle drives up to 64 Zettabytes).

The filesystem is designed to be recoverable as much as possible. A totally screwed up superblock can be reconstructed
by examining the sectors on a disk.

Block size can be 2048, 4096, 8192... etc. The suggested sector size is multiple of 4096 which matches the TLB page size.
That would ease the loading of sectors to memory.

For detailed bit level specification and on disk format see [fsZ.h](https://github.com/bztsrc/osz/blob/master/etc/include/fsZ.h).

Fully Qualified File Path
-------------------------

A fully qualified path looks like:

```
 (drive):(directory)/(filename);(version)#(offset)
```

Drive specifies the media where the filesystem is stored. If not given, `root:` will be substituted, which points to the
initrd located in memory. Drives will be translated to `/dev/(drive)/`.

Directory part is a list of directories, separated by '/'.

Filename can be 111 bytes in length, and may consist of any characters save control characters (<32), delete control (127),
separators (device ':', directory '/', version ';' and offset '#'), as well as invalid UTF-8 sequences. All other
UNICODE characters allowed.

Version part by default is ';0' which points to the current version of the file. The version ';-1' refers to the version
before the current one, ';-2' the version before that and so forth. Providing positive numbers as version refers to a date
in the form ';mmdd' or ';yyyymmdd' or ';yyyymmddHHii'. The actual version for that date will be returned, or the oldest one
if such version is not found. For dates in the future the current version is used.

Offset part defaults to '#0' which is the beginning of the file. Positive numbers are offsets into the file, while negative
ones are relative offsets from the end of the file.

Super Block
-----------

The super block is the first sector of the media and repeated in the last sector. If the superblocks differ, they have
checksums to decide which sector is correct.

File ID
-------

The file id (fid in short) is a scalar logical sector number up to 2^128. It's only valid if the pointed sector holds an
inode structure. In other words, the data must start with the bytes "FSIN".

Inode
-----

The most important structure of FS/Z. It describes portion of the disk as files, directory entries etc. It's the first
1024 bytes of a sector. It can contain several versions thus allowing not only easy snapshot recovery, but per file recovery
as well.

FS/Z also stores meta information (like mime type and checksum) on the contents, and supports meta labels in inodes.

Contents have their own address space (other than fids). To support that, FS/Z has two different fid translation mechanisms.
The first one is similar to memory mapping, and the other stores variable sized areas, so called extents.

Sector List
-----------

Used to implement extents and marking bad and free sector areas on disks. A list item contains a starting fid and the number
of sectors in that area.

Sector Directory
----------------

A sector that has (sectorsize)/16 fid entries. The building block of memory paging like translations. As with sector lists,
sector directories can handle fids up to 2^128, but describe fix sized, continous areas on disk. Not to confuse with common
directories which are used to assign names to fids.

Directory
---------

Every directory has 128 bytes long entries which gives 2^121-1 as the maximum number of entries in one directory. The first
entry is reserved for the directory header, the others are normal fid and name assignments. Entries are alphabetically ordered
thus allowing fast 0(log n) look ups with `libc`'s [bsearch](https://github.com/bztsrc/osz/blob/master/src/lib/libc/stdlib.c#L99).

16 bytes go for the fid, 1 byte reserved for the number of multibyte characters (not bytes) in the filename. That gives 111 bytes for
filename (maybe less in characters as UTF-8 is a variable length encoding). That's the hardest limitation in FS/Z, but let's
face it most likely you've never seen a filename longer than 42 characters in your whole life. Most filenames are below 16 characters.

Mounts
------

You can refer to a piece of data on the disk as a directory or as a file. Directories are always terminated by '/'. This is
also the case if the directory is a mount point for a block device. There refering to as a file (no trailing '/') would
allow to fread() and fwrite() the storage at block level, whilst referencing with a terminating '/' would allow to use opendir()
and readdir(), which would return the root directory of the filesystem stored on that device.

Examples, all pointing to the same directory, as boot partition (the first bootable partition of the first bootable disk,
FS0: in EFI) is mounted on the root volume at /boot:

```
 /boot/EFI/
 root:/boot/EFI/
 boot:/EFI/
 /dev/boot/EFI/
```
```
 /dev/boot       // sees as a block device, you can use fread() and fwrite()
 /dev/boot/      // root directory of the filesystem on the device, use with opendir(), readdir()
```

It worth mentioning this allows automounting just by referring to a file on the device.

