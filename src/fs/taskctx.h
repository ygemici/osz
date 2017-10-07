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
    uint32_t trid;
} openfiles_t;

/* task context for file system operations */
typedef struct {
    pid_t pid;              // task id
    fid_t rootdir;          // root directory fcb
    fid_t workdir;          // working directory fcb
    size_t workoffs;        // block aligned offset during read / write
    size_t workleft;        // bytes left during read / write
    uint64_t nopenfiles;    // number of open files array
    uint64_t nfiles;        // real number of open files
    openfiles_t *openfiles; // open files array
    msg_t msg;              // suspended message for task
    void *next;             // next task context
} taskctx_t;

/* task contexts */
extern uint64_t ntaskctx;
extern taskctx_t *taskctx[];

/* current context */
extern taskctx_t *ctx;

/* task context functions */
extern taskctx_t *taskctx_get(pid_t pid);
extern void taskctx_del(pid_t pid);
extern void taskctx_rootdir(taskctx_t *tc, fid_t fid);
extern void taskctx_workdir(taskctx_t *tc, fid_t fid);
extern uint64_t taskctx_open(taskctx_t *tc, fid_t fid, mode_t mode, fpos_t offs, uint64_t idx);
extern bool_t taskctx_close(taskctx_t *tc, uint64_t idx, bool_t dontfree);
extern bool_t taskctx_seek(taskctx_t *tc, uint64_t idx, off_t offs, uint8_t whence);
extern bool_t taskctx_validfid(taskctx_t *tc, uint64_t idx);
extern size_t taskctx_read(taskctx_t *tc, fid_t idx, virt_t ptr, size_t size);
extern size_t taskctx_write(taskctx_t *tc, fid_t idx, void *ptr, size_t size);

#if DEBUG
extern void taskctx_dump();
#endif
