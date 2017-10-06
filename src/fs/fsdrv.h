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
#define _FSDRV_H_ 1

/* state of locating a file */
typedef struct {
    fid_t inode;
    fpos_t filesize;
    void *fileblk;
    char *path;
    uint8_t type;
    bool_t creat;
} locate_t;

/* file system driver structure */
typedef struct {
    const char *name;                                                               // unix name of the file system
    const char *desc;                                                               // human readable name
    bool_t  (*detect)   (fid_t fd, void *blk);                                      // check magic in block
    uint8_t (*locate)   (fid_t fd, ino_t parent, locate_t *loc);                    // locate a path in file system
    void    (*resizefs) (fid_t fd);                                                 // resize the file system
    bool_t  (*checkfs)  (fid_t fd);                                                 // check fs consistency
    bool_t  (*stat)     (fid_t fd, ino_t file, stat_t *st);                         // return stat structure
    void*   (*read)     (fid_t fd, ino_t file, fpos_t offs, size_t *s);             // read from a file
    bool_t  (*write)    (fid_t fd, ino_t file, fpos_t offs, void *blk, size_t *s);  // write to a file
    size_t  (*getdirent)(void *buf, fpos_t offs, dirent_t *dirent);                 // parse directory entry to dirent
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
