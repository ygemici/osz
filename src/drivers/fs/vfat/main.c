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

bool_t detect(void *blk)
{
    return
     ((uint8_t*)blk)[510]==0x55 && ((uint8_t*)blk)[511]==0xAA &&
     /* FAT12 / FAT16 */
     (!memcmp(blk+54, "FAT1", 4)||
     /* FAT32 */
      !memcmp(blk+84, "FAT3", 4));
}

void _init()
{
    fsdrv_t drv = {
        "vfat",
        "FAT12/16/32",
        detect
    };
    //uint16_t id = 
    _fs_reg(&drv);
}
