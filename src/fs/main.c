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

public uint8_t *_initrd_ptr;
public uint64_t _initrd_size;
public char *_fstab_ptr;
public uint64_t _fstab_size;
public uint32_t _pathmax;

extern void cache_init();
extern void devfs_init();

void parse_fstab()
{
#if DEBUG
    dbg_printf("fstab: %x %d\n%s\n",_fstab_ptr,_fstab_size,_fstab_ptr);
#endif
}

public void mountfs()
{
//cache_dump();
    parse_fstab();
vfs_dump();
}

void task_init(int argc, char **argv)
{
    /* allocate directory and block caches */
    cache_init();
    /* add root directory. first inode must be the root */
    //VFS_INODE_ROOT, reserve space for it at the first FCB
    //uninitialized rootdir and cwd will point here
    nfcbs=1;

    /* initialize dev directory, and memory "block devices" */
    devfs_init();
}
