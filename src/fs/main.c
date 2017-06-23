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

void parse_fstab()
{
#if DEBUG
//    dbg_printf("fstab: %x %d\n%s\n",_fstab_ptr,_fstab_size,_fstab_ptr);
    
    cache_dump();
    vfs_dump();
#endif
}

public void mountfs()
{
    parse_fstab();
}

void task_init(int argc, char **argv)
{
    cache_init();
    vfs_init();
}
