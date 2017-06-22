/*
 * drivers/fs/vfat/main.c
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
 * @brief Virtual File Allocation Table driver
 */

#include <osZ.h>
#include <vfs.h>
#include <fsZ.h>

bool_t vfatdetect(void *blk)
{
    return true;
}

void _init()
{
    fsdrv_t vfatdrv = {
        "vfat",
        "FAT12/16/32",
        vfatdetect
    };
    //uint16_t id = 
    _vfs_regfs(&vfatdrv);
}
