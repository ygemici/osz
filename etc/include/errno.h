/*
 * errno.h
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
 * @brief Available errno values
 */
#ifndef _ERRNO_H
#define _ERRNO_H 1

#ifndef _AS
extern void seterr(int errno);
extern int errno();
#endif

#define SUCCESS      0  // No error
#define EPERM        1  // Operation not permitted
#define EAGAIN       2  // Try again
#define ESRCH        3  // No such process
#define EFAULT       4  // Bad address
#define EINVAL       5  // Invalid argument
#define EBUSY        6  // Device or resource busy
#define EACCES       7  // Access denied
#define ENOMEM       8  // Out of memory
#define ENOEXEC      9  // Exec format error
#define EEXIST      10  // File exists
#define ENOENT      11  // No such file or directory
#define ENODEV      12  // No such device
#define ENOTDIR     13  // Not a directory
#define EISDIR      14  // Is a directory
#define ENOTUNI     15  // Not an union
#define EISUNI      16  // Is an union
#define ENOFS       17  // No filesystem found
#define EBADFS      18  // Corrupt filesystem
#define EROFS       19  // Read-only file system
#define ENOSPC      20  // No space left on device
#define ENOTSUP     21  // File type not supported by file system
#define EIO         22  // I/O error
#define EPIPE       23  // Broken pipe
#define ESPIPE      24  // Illegal seek
#define ENOTSHM     25  // Not a shared memory buffer
#define EBADF       26  // Bad file number
#define ENOTBIG     27  // Buffer not big enough

/*
#define EINTR        4  // Interrupted system call
#define ENXIO        6  // No such device or address
#define E2BIG        7  // Argument list too long
#define ECHILD      10  // No child processes
#define ENOTBLK     15  // Block device required
#define EXDEV       18  // Cross-device link
#define ENFILE      23  // File table overflow
#define EMFILE      24  // Too many open files
#define ENOTTY      25  // Not a typewriter
#define ETXTBSY     26  // Text file busy
#define EFBIG       27  // File too large
#define EMLINK      31  // Too many links
#define EDOM        33  // Math argument out of domain of func
#define ERANGE      34  // Math result not representable
*/
#define EUNKWN      255 // Unknown reason

#endif /* errno.h */
