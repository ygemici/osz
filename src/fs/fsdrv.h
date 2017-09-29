/*
 * fs/fsdrv.h
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
 * @brief File System Drivers
 */

#include <osZ.h>

#ifndef _FSDRV_H_
#define _FSDRV_H_

typedef struct {
    const char *name;
    const char *desc;
    bool_t (*detect)(void *blk);
    ino_t (*locate)(fid_t storage, char *path);
} fsdrv_t;

/* filesystem parsers */
extern uint16_t nfsdrv;
extern fsdrv_t *fsdrv;

extern public uint16_t fsdrv_reg(const fsdrv_t *fs);
extern int16_t fsdrv_get(char *name);
extern int16_t fsdrv_detect(fid_t dev);

#if DEBUG
extern void fsdrv_dump();
#endif

#endif
