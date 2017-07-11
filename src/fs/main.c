/*
 * fs/main.c
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
 * @brief File System Service
 */
#include <osZ.h>
#include "cache.h"
#include "vfs.h"

public uint8_t *_initrd_ptr=NULL;   // initrd offset in memory
public uint64_t _initrd_size=0;     // initrd size in bytes
public char *_fstab_ptr=NULL;       // pointer to /etc/fstab data
public size_t _fstab_size=0;        // fstab size
public uint32_t _pathmax=0;         // maximum length of path

/* should be in cache.h and devfs.h, but nobody else allowed to call them */
extern void cache_init();
extern void devfs_init();
extern void devfs_dump();

public void mountfs()
{
devfs_dump();
//cache_dump();
    vfs_fstab(_fstab_ptr, _fstab_size);
vfs_dump();
}

void task_init(int argc, char **argv)
{
    /* allocate directory and block caches */
    cache_init();
    /* initialize dev directory, and in memory "block devices" */
    devfs_init();
    /* reserve space for root directory. uninitialized chroot and cwd will point here */
    //nfcbs=1;
}
