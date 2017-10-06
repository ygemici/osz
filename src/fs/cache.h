/*
 * fs/cache.h
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
 * @brief Cache definitions
 */

#include <osZ.h>

// block priorities for soft update
#define BLKPRIO_NOTDIRTY 0  // for read operations
#define BLKPRIO_ALLOC    1  // allocate space on file system
#define BLKPRIO_DATA     2  // file data
#define BLKPRIO_MAPPING  3  // data mapping info
#define BLKPRIO_META     4  // inode information

typedef uint8_t blkprio_t;

extern public void cache_init();
extern public void *cache_getblock(fid_t fd, blkcnt_t lsn);
extern public void cache_setblock(void *blk, fid_t fd, blkcnt_t lsn, blkprio_t prio);
extern public void cache_freeblocks(fid_t fd);

#if DEBUG
extern void cache_dump();
#endif
