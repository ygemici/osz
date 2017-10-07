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
#define BLKPRIO_CRIT     1  // superblock and space allocation info
#define BLKPRIO_DATA     2  // file data
#define BLKPRIO_MAPPING  3  // data mapping info
#define BLKPRIO_META     4  // inode information

extern fid_t cache_currfid; // current fcb id
typedef uint8_t blkprio_t;

extern void cache_init();
extern void *cache_getblock(fid_t fd, blkcnt_t lsn);
extern bool_t cache_setblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio);
extern blkcnt_t cache_freeblocks(fid_t fd, blkcnt_t needed);
extern void cache_flush();
extern bool_t cache_cleardirty(fid_t fd, blkcnt_t lsn);
extern void cache_free();

#if DEBUG
extern void cache_dump();
#endif
