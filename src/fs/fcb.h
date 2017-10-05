/*
 * fs/fcb.h
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
 * @brief File Control Block
 */

#include <osZ.h>

#ifndef _FCB_H_
#define _FCB_H_ 1

// file types in fcb list
#define FCB_TYPE_REG_FILE   0
#define FCB_TYPE_REG_DIR    1
#define FCB_TYPE_UNION      2
#define FCB_TYPE_DEVICE     3
#define FCB_TYPE_PIPE       4
#define FCB_TYPE_SOCKET     5

// regular. Files and directories
typedef struct {
    fid_t storage;
    ino_t inode;
    fpos_t filesize;
    blksize_t blksize;
    uint16_t fs;
} fcb_reg_t;

// Directory unions
typedef struct {
    fid_t storage;
    ino_t inode;
    fpos_t filesize;
    fid_t *unionlist;
    uint16_t fs;
} fcb_union_t;

// uses the same as regular files, only this time
// storage points to devfs
typedef fcb_reg_t fcb_dev_t;

typedef struct {
} fcb_pipe_t;

typedef struct {
} fcb_socket_t;

typedef struct {
    char *abspath;
    uint64_t nopen;
    uint8_t type;
    mode_t mode;
    union {
        fcb_reg_t reg;
        fcb_dev_t device;
        fcb_union_t uni;
        fcb_pipe_t pipe;
        fcb_socket_t socket;
    };
} fcb_t;

extern uint64_t nfcb;
extern uint64_t nfiles;
extern fcb_t *fcb;

extern fid_t fcb_get(char *abspath);
extern fid_t fcb_add(char *abspath, uint8_t type);
extern void fcb_del(fid_t idx);
extern void fcb_unionlist_build(fid_t idx);

#if DEBUG
extern void fcb_dump();
#endif

#endif
