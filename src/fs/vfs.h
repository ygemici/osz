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
 * @brief Virtual File System functions. This header included by fs drivers too
 */

#include <osZ.h>
#include "cache.h"
#include "fcb.h"
#include "fsdrv.h"
#include "taskctx.h"

#define ROOTMTAB 0
#define ROOTFCB 0
#define DEVFCB 1
#define DEVPATH "/dev/"
#define PATHSTACKSIZE 64

// lookup return statuses
#define NOBLOCK     1   // block not found in cache
#define NOTFOUND    2   // file not found
#define FSERROR     3   // file system error
#define UPDIR       4   // directory up outside of device
#define FILEINPATH  5   // file image referenced as directory
#define SYMINPATH   6   // symlink in path
#define UNIONINPATH 7   // union in path
#define NOSPACE     8   // no space left on device

#define PATHEND(a) (a==';' || a=='#' || a==0)

#define FSCKSTEP_SB     0
#define FSCKSTEP_ALLOC  1
#define FSCKSTEP_DIR    2
#define FSCKSTEP_NLINK  3
#define FSCKSTEP_DONE  4
typedef struct {
    fid_t dev;
    uint8_t step;
    bool_t fix;
    uint16_t fs;
} fsck_t;

/* file system check state */
extern fsck_t fsck;

extern uint8_t ackdelayed;      // flag to indicate async block read
extern uint32_t pathmax;        // maximum length of path
extern void *zeroblk;           // zero block
extern stat_t st;               // buffer for lstat() and fstat()
extern dirent_t dirent;         // buffer for readdir()
extern char *lastlink;          // last symlink's target, filled in by fsdrv's stat()

typedef struct {
    ino_t inode;
    char *path;
} pathstack_t;

/* low level functions */
extern void pathpush(ino_t lsn, char *path);
extern pathstack_t *pathpop();
extern char *pathcat(char *path, char *filename);
extern char *canonize(const char *path);
extern uint8_t getver(char *abspath);
extern fpos_t getoffs(char *abspath);
extern void *readblock(fid_t fd, blkcnt_t lsn);
public bool_t writeblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio);
/* wrappers around file system driver functions */
extern fid_t lookup(char *abspath, bool_t creat);
extern stat_t *statfs(fid_t idx);
extern bool_t dofsck(fid_t fd, bool_t fix);
