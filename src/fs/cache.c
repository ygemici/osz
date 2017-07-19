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
#include "devfs.h"

/* directory cache */
uint64_t ncachedir = 0;
cachedir_t *cachedir = NULL;

/* cache index */
uint64_t ncache = 0;
cachedir_t *cache = NULL;

/* cache blocks */
cacheblk_t cacheblk[8];

extern uint64_t delayanswer();

void cache_init()
{
}

/**
 * read a block from cache
 */
void *cache_getblock(uint64_t fs, dev_t dev, blkcnt_t blk)
{
    void *ptr;
    // devive file?
    if(fs==devmtab) {
        if(dev>=ndevdir)
            return NULL;
        /* built-in device? */
        if(devdir[dev].drivertask==VFS_DEVICE_MEMFS) {
            if(blk>=devdir[dev].size)
                return NULL;
            switch(devdir[dev].device) {
                case VFS_MEMFS_ZERO_DEVICE:
                    ptr=(void*)malloc(devdir[dev].blksize);
                    return ptr;
                break;
                case VFS_MEMFS_RAMDISK_DEVICE:
                    return (void*)(devdir[dev].startsec+devdir[dev].blksize*blk);
                break;
            }
        } else {
            /* TODO: send an async SYS_getblock message to the driver with delayedmsg index */
            delayanswer();
        }
    } else {
    // simple file on another filesystem
    }
    return NULL;
}

/**
 * store a block in cache, called by drivers
 */
void cache_setblock(msg_t *msg)
{
}

#if DEBUG
void cache_dump()
{
    int i;
    dbg_printf("Directory cache %x %d:\n",cachedir,ncachedir);
    for(i=0;i<ncachedir;i++)
        dbg_printf("%3d. %5d %s\n",i,cachedir[i].fid,cachedir[i].name);
/*
    dbg_printf("\nBlock cache %x %d:\n",cache,ncache);
    for(i=0;i<ncache;i++)
        dbg_printf("%3d. \n",i);

    dbg_printf("\n");
*/
}
#endif
