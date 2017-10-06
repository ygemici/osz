/*
 * fs/cache.c
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
 * @brief Cache implementation
 */
#include <osZ.h>
#include "cache.h"

extern uint16_t cachelines;     // number of cache lines

void cache_init()
{
}

/**
 * read a block from cache
 */
public void* cache_getblock(fid_t fd, blkcnt_t lsn)
{
    // TODO: use dev[fcb[fd].device.inode].drivertask, dev[fcb[fd].device.inode].device
    // fcb[fd].device.blksize
    return NULL;
}

/**
 * store a block in cache, called by drivers
 */
public void cache_setblock(void *blk, fid_t fd, blkcnt_t lsn, blkprio_t prio)
{
}

/**
 * remove all blocks of a device from cache
 */
public void cache_freeblocks(fid_t fd)
{
}

#if DEBUG
void cache_dump()
{
}
#endif
