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
#include <osZ.h>

typedef struct {
    const char *name;
    const char *desc;
    bool_t (*detect)(void *blk);
} fsdrv_t;

typedef struct {
} pipe_t;

typedef struct {
} superblock_t;

typedef struct {
} directory_t;

typedef struct {
} file_t;

typedef struct {
    pid_t drivertask;
    dev_t device;
    blksize_t blksize;
    blkcnt_t size;
    fpos_t startsec;
} blkdev_t;

typedef struct {
    pid_t drivertask;
    dev_t device;
} chrdev_t;

typedef struct {
    nlink_t nlink;
    ino_t next;
    ino_t prev;
    uint16_t flags;
    uint16_t fs;
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
extern ino_t getinode(const char *path);
extern uint16_t _vfs_regfs(const fsdrv_t *fs);
extern void vfs_init();
#if DEBUG
extern void vfs_dump();
#endif
