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
#include <sys/stat.h>

public uint64_t _bogomips = 1000;
public uint64_t _alarmstep = 1000;
public uint64_t errn = SUCCESS;

/**
 * set error status, errno
 */
public void seterr(int e)
{
    errn = e;
}

/**
 * get error status, errno
 */
public int errno()
{
    return errn;
}

/**
 * Make PATH be the root directory (the starting point for absolute paths).
 * This call is restricted to the super-user.
 */
public fid_t chroot(const char *path)
{
    return 0;
}

/**
 * Change the process's working directory to PATH.
 */
public fid_t chdir(const char *path)
{
    return 0;
}
