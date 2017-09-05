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
#include <sys/stat.h>

#ifndef _AS

// mount point cache is not write-through
#define VFS_MOUNT_FLAG_ASYNC        (1<<0)

/* index to devfs in mtab */
extern uint64_t devmtab;

/** structure of mtab */
typedef struct {
    uint64_t fs_parent; //index to mtab, parent filesystem
    ino_t fs_spec;      //index to devdir, block special device (if fs_parent==devmtab)
    char *fs_file;      //path where it's mounted on
    uint16_t fs_type;   //index to fsdrvs, autodetected
    uint16_t fs_flags;  //mount options
    uint16_t len;       //number of access entries
    gid_t *grp;         //access entries
    uint8_t access;     //access for world
    nlink_t nlink;      //number of fcb references
    ino_t rootdir;      //root directory inode
} mount_t;

typedef struct {
    const char *name;
    const char *desc;
    ino_t (*detect)(void *blk);
    ino_t (*locate)(mount_t *mnt, ino_t parent, char *path);
} fsdrv_t;

#define VFS_FCB_ROOT                0

#define VFS_FCB_TYPE_FILE           0
#define VFS_FCB_TYPE_DIRECTORY      1
#define VFS_FCB_TYPE_PIPE           2

#define VFS_FCB_FLAG_DIRTY          (1<<0)
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
extern uint64_t nmtab;
extern mount_t *mtab;

/* file control blocks */
extern uint64_t nfcbs;
extern fcb_t *fcbs;

/* open files, returned by lsof */
extern uint64_t nfiles;
extern openfile_t *files;

/* public syscalls */
extern fid_t vfs_locate(fid_t parent, char *path, uint64_t type);

/* private functions */
extern uint16_t fsdrv_get(char *name);
extern uint16_t fsdrv_reg(const fsdrv_t *fs);
extern uint64_t mtab_add(char *dev, char *file, char *opts);
extern void mtab_del(uint64_t mid);
extern fid_t fcb_add(const fcb_t *fcb);
extern void fcb_del(fid_t fid);
extern void vfs_fstab(char *ptr,size_t size);
#if DEBUG
extern void vfs_dump();
#endif
#endif /* _AS */

#endif /* vfs.h */

