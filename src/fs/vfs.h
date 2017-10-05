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

#define PATHEND(a) (a==';' || a=='#' || a==0)

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
extern void *readblock(fid_t idx, fpos_t offs);
/* wrappers around file system driver functions */
extern fid_t lookup(char *abspath, bool_t creat);
extern size_t readfs(taskctx_t *tc, fid_t idx, virt_t ptr, size_t size);
extern size_t writefs(taskctx_t *tc, fid_t idx, void *ptr, size_t size);
extern stat_t *statfs(fid_t idx);
extern dirent_t *readdirfs(taskctx_t *tc, fid_t idx, void *ptr, size_t size);
