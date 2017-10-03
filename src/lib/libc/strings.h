/*
 * lib/libc/strings.h
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
 * @brief String constants
 */

/* must match errno.h */
char *errstrs[] = { ""/*Success*/,
    "EPERM Operation not permitted",
    "EAGAIN Try again",
    "ESRCH No such task",
    "EFAULT Bad address",
    "EINVAL Invalid argument",
    "EBUSY Device or resource busy",
    "EACCES Access denied",
    "ENOMEM Out of memory",
    "ENOEXEC Exec format error",
    "EEXIST File exists",
    "ENOENT No such file or directory",
    "ENODEV No such device",
    "ENOTDIR Not a directory",
    "ENOFS Unknown file system",
    "EBADFS Corrupt file system",
    "EROFS Read-only file system",
    "ENOSPC No space left on device",
    "EIO I/O error",
    "EPIPE Broken pipe",
    "ESPIPE Illegal seek",
    "ENOTSHM Not a shared memory buffer" };

/* must match signal.h */
char *sigs[] = { "NONE", "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "EMT", "FPE", "KILL", "BUS", "SEGV",
    "SYS", "PIPE", "ALRM", "TERM", "URG", "STOP", "TSTP", "CONT", "CHLD", "TTIN", "TTOU", "IO", "XCPU", "XFSZ",
    "VTALRM", "PROF", "WINCH", "INFO", "USR1", "USR2" };
