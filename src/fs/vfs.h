/*
 * fs/vfs.h
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
 * @brief Virtual File System definitions
 */

#ifndef _VFS_H_
#define _VFS_H_

#include <osZ.h>
#include <fsZ.h>

#define VFS_INODE_TYPE_PIPE         0
#define VFS_INODE_TYPE_SUPERBLOCK   1
#define VFS_INODE_TYPE_DIRECTORY    2
#define VFS_INODE_TYPE_FILE         3
#define VFS_INODE_TYPE_BLKDEV       4
#define VFS_INODE_TYPE_CHRDEV       5

#define VFS_INODE_FLAG_DIRTY        (1<<0)

#define VFS_INODE_ROOT              0
#define VFS_INODE_ZERODEV           1
#define VFS_INODE_RAMDISK           2
#define VFS_INODE_RNDDEV            3
#define VFS_INODE_DEVFS             4

#define VFS_BLKDEV_MEMFS            0
#define VFS_MEMFS_ZERO_DEVICE       0
#define VFS_MEMFS_RAMDISK_DEVICE    1
#define VFS_MEMFS_RANDOM_DEVICE     2

#ifndef _AS
typedef struct {
    const char *name;
    const char *desc;
    bool_t (*detect)(void *blk);
} fsdrv_t;

typedef struct {
} pipe_t;

typedef struct {
    // first is FSZ_DirEntHeader, root directory
    FSZ_DirEnt *entries;
    ino_t storage;
    uint16_t flags;
    uint16_t fs;
} superblock_t;

typedef struct {
    // first is FSZ_DirEntHeader
    FSZ_DirEnt *entries;
} directory_t;

typedef struct {
    void *data;
} file_t;

typedef struct {
    pid_t drivertask;   //major
    dev_t device;       //minor
    blksize_t blksize;
    blkcnt_t size;
    fpos_t startsec;
} blkdev_t;

typedef struct {
    pid_t drivertask;   //major
    dev_t device;       //minor
} chrdev_t;

typedef struct {
    nlink_t nlink;
    ino_t next;
    ino_t prev;
    uint64_t cachedir;
    uint32_t type;
    union {
        pipe_t pipe;
        superblock_t superblock;
        directory_t directory;
        file_t file;
        blkdev_t blkdev;
        chrdev_t chrdev;
    };
} inode_t;

typedef struct {
    pid_t pid;
    fpos_t pos;
    ino_t inode;
} openfile_t;

/* filesystem parsers */
extern uint16_t nfsdrvs;
extern fsdrv_t *fsdrvs;

/* files and directories */
extern uint64_t ninodes;
extern inode_t *inodes;

/* open files, returned by lsof */
extern uint64_t nfiles;
extern openfile_t *files;

extern void mknod();
extern ino_t getinode(ino_t parent,const char *path);
extern uint16_t _vfs_regfs(const fsdrv_t *fs);
extern ino_t vfs_inode(const inode_t *inode);
#if DEBUG
extern void vfs_dump();
#endif
#endif /* _AS */

#endif /* vfs.h */

