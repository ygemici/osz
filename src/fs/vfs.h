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

#define VFS_FCB_TYPE_FILE           0
#define VFS_FCB_TYPE_PIPE           1
#define VFS_FCB_TYPE_DIRECTORY      2

#define VFS_FCB_FLAG_DIRTY          (1<<0)

#ifndef _AS
typedef struct {
    const char *name;
    const char *desc;
    bool_t (*detect)(void *blk);
} fsdrv_t;

#define VFS_MOUNT_FLAG_ASYNC        (1<<0)

typedef struct {
    uint64_t fs_spec;   //index to devdir, block special device
    fid_t fs_file;      //path where it's mounted on, index to fcb
    uint64_t fs_type;   //index to fsdrvs, autodetected
    uint16_t fs_flags;  //mount options
    uint16_t fs_passno; //pass number on mount all
    uint16_t len;       //number of access entries
    gid_t *grp;         //access entries
    uint8_t access;     //access for world
} mount_t;

//fcb types
typedef struct {
    fid_t fid[2];
    uint64_t flags;
} pipe_t;

typedef struct {
    uint64_t mount;
    ino_t inode;
    fpos_t size;
    void *data;
} file_t;

typedef struct {
    uint64_t mount;
    ino_t inode;
    fpos_t size;
    FSZ_DirEnt *data;
} directory_t;

typedef struct {
    char *path;
    nlink_t nlink;
    uint8_t type;
    union {
        pipe_t pipe;
        file_t file;
        directory_t directory;
    };
} fcb_t;

typedef struct {
    pid_t pid;  //owner task
    fpos_t pos; //seek position
    fid_t fid;  //fcb index
} openfile_t;

/* filesystem parsers */
extern uint16_t nfsdrvs;
extern fsdrv_t *fsdrvs;

/* mount points */
extern uint64_t nmounts;
extern mount_t *mounts;

/* file control blocks */
extern uint64_t nfcbs;
extern fcb_t *fcbs;

/* open files, returned by lsof */
extern uint64_t nfiles;
extern openfile_t *files;

extern uint16_t _fs_get(char *name);
extern uint16_t _fs_reg(const fsdrv_t *fs);
extern fid_t fcb_add(const fcb_t *fcb);
extern void vfs_fstab(char *ptr,size_t size);
#if DEBUG
extern void vfs_dump();
#endif
#endif /* _AS */

#endif /* vfs.h */

