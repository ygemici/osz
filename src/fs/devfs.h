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

#define MEMFS_MAJOR     0
#define MEMFS_ZERO      0
#define MEMFS_RAMDISK   1
#define MEMFS_RANDOM    2
#define MEMFS_NULL      3

/* device type */
typedef struct {
    fid_t fid;          // name in fcb
    pid_t drivertask;   // major
    dev_t device;       // minor
    blkcnt_t total;     // total number of blocks to be written
    blkcnt_t out;       // number of blocks already written out
} devfs_t;

extern uint64_t ndev;
extern devfs_t *dev;

extern void devfs_init();
extern uint64_t devfs_add(char *name, pid_t drivertask, dev_t device, mode_t mode, blksize_t blksize, blkcnt_t blkcnt);
extern void devfs_del(uint64_t idx);

#if DEBUG
extern void devfs_dump();
#endif
