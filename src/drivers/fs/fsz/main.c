/*
 * drivers/fs/fsz/main.c
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
 * @brief FS/Z filesystem driver
 */

#include <osZ.h>
#include <vfs.h>
#include <fsZ.h>
#include <crc32.h>

bool_t detect(void *blk)
{
    return
     !memcmp(((FSZ_SuperBlock *)blk)->magic, FSZ_MAGIC,4) &&
     !memcmp(((FSZ_SuperBlock *)blk)->magic2,FSZ_MAGIC,4) &&
     ((FSZ_SuperBlock *)blk)->checksum == crc32_calc((char*)((FSZ_SuperBlock *)blk)->magic, 508);
}

ino_t locate(mount_t *mnt, char *path, uint64_t type)
{
dbg_printf("FS/Z locate '%s'\n",path);
    return -1;
}

void _init()
{
    fsdrv_t drv = {
        "fsz",
        "FS/Z",
        detect,
        locate
    };
    //uint16_t id = 
    _fs_reg(&drv);
}
