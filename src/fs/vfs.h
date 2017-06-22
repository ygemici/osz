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
    uint32_t major;
    uint32_t minor;
    blksize_t blksize;
    blkcnt_t size;
    fpos_t startsec;
    pid_t drivertask;
} device_t;

typedef struct {
    dev_t device;
    nlink_t nlink;
    ino_t next;
    ino_t prev;
    uint32_t flags;
    uint32_t type;
    union {
        struct {
        } pipe;
        struct {
        } superblock;
        struct {
        } directory;
        struct {
        } file;
        struct {
        } socket;
        struct {
        } blkdev;
        struct {
        } chrdev;
    };
} inode_t;

typedef struct {
    pid_t owner;
    fpos_t pos;
    ino_t inode;
} file_t;

/* devices */
extern uint64_t ndevices;
extern device_t *devices;

/* files and directories */
extern uint64_t ninodes;
extern inode_t *inodes;

/* open files, returned by lsof */
extern uint64_t nfiles;
extern file_t *files;

extern void mknod();
extern void vfs_init();
