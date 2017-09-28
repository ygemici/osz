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

/* index to devfs in mtab */
extern uint64_t devmtab;

/** structure of mtab */
typedef struct {
    fid_t mountpoint;   //index to fcb, mount point
    fid_t storage;      //index to fcb, storage device
    uint64_t fstype;    //index to fsdrv
    uint64_t fsflags;   //mount options
    uint16_t len;       //number of access entries
    gid_t *grp;         //access entries
    uint8_t access;     //access for world
    nlink_t nlink;      //number of fcb references
    ino_t rootdir;      //root directory inode on storage
} mtab_t;

/* mount points */
extern uint64_t nmtab;
extern mtab_t *mtab;

extern void mtab_fstab(char *ptr, size_t size);
extern void mtab_init();
extern uint64_t mtab_add(char *dev, char *file, char *opts);
