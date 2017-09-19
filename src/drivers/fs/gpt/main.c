/*
 * drivers/fs/gpt/main.c
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
 * @brief GUID Partition Table driver
 */

#include <osZ.h>
#include <cache.h>

ino_t detect(void *blk)
{
    return !memcmp(blk+512, "EFI PART", 8) ? 0 : -1;
}

ino_t locate(mount_t *mnt, ino_t parent, char *path)
{
dbg_printf("GPT locate '%s'\n",path);
    return -1;
}

void _init()
{
    fsdrv_t drv = {
        "gpt",
        "GUID Partition Table",
        detect,
        locate
    };
    //uint16_t id = 
    fsdrv_reg(&drv);
}