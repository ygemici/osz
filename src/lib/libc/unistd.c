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
#ifndef SYS_getinode
#define SYS_getinode 1
#endif

/* structure to hold file descriptors. It's filld by real time linker */
typedef struct {
    ino_t rootdir;
    ino_t cwdir;
    uint64_t of;
    uint64_t lf;
    ino_t *fd;
} _fd_t;

public _fd_t _fd = { 0,0,0,0,NULL };

/* Make PATH be the root directory (the starting point for absolute paths).
   This call is restricted to the super-user.  */
public ino_t chroot(const char *path)
{
    _fd.rootdir = (ino_t)mq_call(SRV_FS, SYS_getinode, 0, path);
    return _fd.rootdir;
}

/* Change the process's working directory to PATH.  */
public ino_t chdir(const char *path)
{
    _fd.cwdir = (ino_t)mq_call(SRV_FS, SYS_getinode, _fd.rootdir, path);
    return _fd.cwdir;
}
