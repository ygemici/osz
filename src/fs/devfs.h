/*
 * fs/devfs.h
 *
 * Copyright 2016 CC-by-nc-sa bztsrc@github
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * You are free to:
 *
 * - Share — copy and redistribute the material in any medium or format
 * - Adapt — remix, transform, and build upon the material
 *     The licensor cannot revoke these freedoms as long as you follow
 *     the license terms.
 *
 * Under the following terms:
 *
 * - Attribution — You must give appropriate credit, provide a link to
 *     the license, and indicate if changes were made. You may do so in
 *     any reasonable manner, but not in any way that suggests the
 *     licensor endorses you or your use.
 * - NonCommercial — You may not use the material for commercial purposes.
 * - ShareAlike — If you remix, transform, or build upon the material,
 *     you must distribute your contributions under the same license as
 *     the original.
 *
 * @brief Device fs definitions
 */
#include <osZ.h>
#include "vfs.h"

#define VFS_DEVICE_MEMFS            0
#define VFS_MEMFS_ZERO_DEVICE       0
#define VFS_MEMFS_RAMDISK_DEVICE    1
#define VFS_MEMFS_RANDOM_DEVICE     2
#define VFS_MEMFS_NULL_DEVICE       3
#define VFS_MEMFS_TMPFS_DEVICE      4

/* device type */
typedef struct {
    char name[16];
    pid_t drivertask;   //major
    dev_t device;       //minor
    blksize_t blksize;
    blkcnt_t size;
    fpos_t startsec;
    uint64_t cacheidx;
} device_t;

extern uint64_t ndevdir;
extern device_t *devdir;

extern ino_t devfs_locate(mount_t *mnt, ino_t parent, char *file);

#if DEBUG
extern void devfs_dump();
#endif
