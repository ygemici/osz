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

#define ROOTMTAB 0
#define ROOTFCB 0
#define DEVFCB 1
#define DEVPATH "/dev/"

#define PATHEND(a) (a==';' || a=='#' || a==0)

/* low level functions */
extern char *canonize(const char *path, char *result);
extern public void *readblock(fid_t idx, fpos_t offs, uint64_t bs);

/* libc function implementations */
