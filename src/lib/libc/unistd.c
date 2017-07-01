/*
 * lib/libc/unistd.c
 *
 * Copyright 2017 CC-by-nc-sa bztsrc@github
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
 * @brief Function implementations for unistd.h
 */
#include <osZ.h>

//failsafe
#ifndef SYS_getfcb
#define SYS_getfcb 1
#endif

/* it's on a read-only page in TCB */
extern ino_t _rootdir;
extern void _chroot(ino_t inode);

/* structure to hold file descriptors. It's filld by real time linker */
typedef struct {
    fid_t cwdir;
    fid_t of;
    fid_t lf;
    fid_t *fd;
} _fd_t;

public _fd_t _fd = { 0,0,0,NULL };
public uint64_t _bogomips = 1000;
public uint64_t _alarmstep = 1000;

/**
 * Make PATH be the root directory (the starting point for absolute paths).
 * This call is restricted to the super-user.
 */
public fid_t chroot(const char *path)
{
//    _chroot((fid_t)mq_call(SRV_FS, SYS_getfcb, 0, path));
    return _rootdir;
}

/**
 * Change the process's working directory to PATH.
 */
public fid_t chdir(const char *path)
{
//    _fd.cwdir = (fid_t)mq_call(SRV_FS, SYS_getfcb, _rootdir, path);
    return _fd.cwdir;
}
