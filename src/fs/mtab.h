/*
 * fs/mtab.h
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
 * @brief Mount points
 */

#include <osZ.h>

// mount point cache is not write-through
#define MTAB_FLAG_ASYNC        (1<<0)

/** structure of mtab */
typedef struct {
    fid_t mountpoint;   //index to fcb, mount point
    fid_t storage;      //index to fcb, storage device
    uint64_t fstype;    //index to fsdrv
    uint16_t len;       //number of access entries
    gid_t *grp;         //access entries
} mtab_t;

/* mount points */
extern uint16_t nmtab;
extern mtab_t *mtab;

extern void mtab_fstab(char *ptr, size_t size);
extern void mtab_init();
extern int16_t mtab_add(char *dev, char *file, char *opts);
extern bool_t mtab_del(char *dev, char *file);

#if DEBUG
extern void mtab_dump();
#endif
