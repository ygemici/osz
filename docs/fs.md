FS/Z File System
================

It was designed to store unimagineable amounts of data. It's a classic UNIX type filesystem with inodes and superblock
but without their limitations. There's no limit on number of files or directories in FS/Z (save storage capacity).
Logical sector numbers are stored in 2^128 bits to be future proof. Current implementation
only uses 2^64 bits (can handle drives up to 64 Zettabytes with 4096 bytes logical sector size).

The filesystem is designed to be recoverable by `fsck` as much as possible. A totally screwed up superblock can be reconstructed
by examining the sectors on a disk, looking for inodes of meta data.

Block size can be 2048, 4096, 8192... etc. The suggested size matches the memory page size on the architecture. That is 4096
for x86_64 and aarch64.

For detailed bit level specification and on disk format see [fsZ.h](https://github.com/bztsrc/osz/blob/master/etc/include/fsZ.h).

Implementations
---------------
The filesystem driver is implemented 3 times:
 1. in the [loader](https://github.com/bztsrc/osz/blob/master/loader), which supports GPT and FAT to locate initrd, and FS/Z, cpio, tar, sfs to locate kernel inside the initrd. This is a read-only implementation, that requires defragmented files.
 2. in the [core](https://github.com/bztsrc/osz/blob/master/src/core/fs.c), which is used at early boot stage to load various files (like the FS/Z filesystem driver) from initrd. This is also a read-only implementation for defragmented files.
 3. as a [filesystem driver](https://github.com/bztsrc/osz/blob/master/src/drivers/fs/fsz/main.c) for the FS service, which is a fully featured implementation.

Fully Qualified File Path
-------------------------

A fully qualified path looks like:

```
 (drive):(directory)/(filename);(version)#(offset)
```

Drive specifies the media where the filesystem is stored. It's optional, and fallbacks to the root filesystem.
Drives will be translated to `/dev/(drive)/` where automount will take care of the rest. It is useful for removable media,
like `dvd0:`.

Directory part is a list of directories, separated by '/'. A special joker directory name '...' can be used to dive into every
sub-directories. In that case the first full path that matched will be used. Directories can be unions, a special
construct [unknown to UNIX](https://github.com/bztsrc/osz/blob/master/docs/posix.md)es. It works like the joker, but
limits the match to the listed directories.

Filename can be 111 bytes in length, and may consist of any characters save control characters (<32), delete control (127),
separators (device ':', directory '/', version ';', offset '#'), as well as invalid UTF-8 sequences.
Any other UTF-8 encoded UNICODE character allowed.

Version part by default is ';0' which points to the current version of the file. The version ';-1' refers to the version
before the current one, ';-2' the version before that and so forth up to 5 pervious versions.

Offset part defaults to '#0' which is the beginning of the file.
Positive numbers are offsets into the file, while negative ones are relative offsets from the end of the file.

Super Block
-----------

The super block is the first sector of the media and optionally repeated in the last sector. If the superblocks differ, they have
checksums to decide which sector is correct.

Logical Sector Number
---------------------

The size of a logical sector is recorded in the superblock. Sector number is a scalar value up to 2^128 and relative to
the superblock, which therefore resides on LSN 0, regardless whether it's on LBA 0 (whole disk) or any other (partition).
This means you can move the filesystem image around without the need of relocating sector addresses. Because LSN 0
stores the superblock which is never referenced from any file, so in mappings LSN 0 encodes a sector full of zeros,
allowing spare files and gaps inside files.

File ID
-------

The file id (fid in short) is a logical sector number, which points to a sector with i-node structure. That can be checked
by starting with the magic bytes "FSIN".

Inode
-----

The most important structure of FS/Z. It describes portion of the disk as files, directory entries etc. It's the first
1024 bytes of a logical sector. It can contain several versions thus allowing not only easy snapshot recovery, but per file
recovery as well.

FS/Z also stores meta information (like mime type and checksum) on the contents, and supports meta labels in inodes.

Contents have their own address space mapping (other than LSNs). To support that, FS/Z has two different LSN translation
mechanisms. The first one is similar to memory mapping, and the other stores variable sized areas, so called extents.

Sector List
-----------

Used to implement extents and marking bad and free sector areas on disks. A list item contains a starting LSN and the number
of logical sectors in that area, describing a continuous area on disk. Therefore every extent is 32 bytes long.

Sector Directory
----------------

A sector that has (logical sector size)/16 entries. The building block of memory paging like translations. As with sector lists,
sector directories can handle LSNs up to 2^128, but describe fix sized, non-continous areas on disk. Not to confuse with common
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

