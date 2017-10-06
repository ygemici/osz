/*
 * fs/taskctx.h
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
 * @brief Task context for file system services
 */

#include <osZ.h>

#define OF_MODE_READDIR (1<<15)
#define OF_MODE_EOF     (1<<16)

/* describe an opened file */
typedef struct {
    fid_t fid;
    mode_t mode;
    fpos_t offs;
    uint32_t unionidx;
} openfiles_t;

/* task context for file system operations */
typedef struct {
    pid_t pid;
    fid_t rootdir;
    fid_t workdir;
    size_t workoffs;
    size_t workleft;
    uint64_t nopenfiles;
    uint64_t nfiles;
    openfiles_t *openfiles;
    msg_t msg;
    uint16_t fs;
    void *next;
} taskctx_t;

extern uint64_t ntaskctx;
extern taskctx_t *taskctx[];

extern taskctx_t *ctx;

extern taskctx_t *taskctx_get(pid_t pid);
extern void taskctx_del(pid_t pid);
extern void taskctx_rootdir(taskctx_t *tc, fid_t fid);
extern void taskctx_workdir(taskctx_t *tc, fid_t fid);
extern uint64_t taskctx_open(taskctx_t *tc, fid_t fid, mode_t mode, fpos_t offs, uint64_t idx);
extern bool_t taskctx_close(taskctx_t *tc, uint64_t idx, bool_t dontfree);
extern bool_t taskctx_seek(taskctx_t *tc, uint64_t idx, off_t offs, uint8_t whence);
extern bool_t taskctx_validfid(taskctx_t *tc, uint64_t idx);

#if DEBUG
extern void taskctx_dump();
#endif
