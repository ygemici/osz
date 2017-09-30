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
char *errnums[] = { "SUCCESS", "EPERM", "EAGAIN", "ESRCH", "EFAULT", "EINVAL", "EBUSY", "EACCESS", "ENOMEM",
    "ENOEXEC", "EEXIST", "ENOENT", "ENODEV", "ENOTDIR", "ENOFS", "EBADFS", "EROFS", "ENOSPC", "EIO",
    "EPIPE", "ESPIPE" };
char *errstrs[] = { "", "Operation not permitted.", "Try again", "No such task", "Bad address", "Invalid argument",
    "Device or resource busy", "Access denied", "Out of memory", "Exec format error", "File exists",
    "No such file or directory", "No such device", "Not a directory", "Unknown file system", "Corrupt file system",
    "Read-only file system", "No space left on device", "I/O error", "Broken pipe", "Illegal seek" };

/* must match signal.h */
char *sigs[] = { "NONE", "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "EMT", "FPE", "KILL", "BUS", "SEGV",
    "SYS", "PIPE", "ALRM", "TERM", "URG", "STOP", "TSTP", "CONT", "CHLD", "TTIN", "TTOU", "IO", "XCPU", "XFSZ",
    "VTALRM", "PROF", "WINCH", "INFO", "USR1", "USR2" };
